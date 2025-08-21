#include "RenderSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    RenderSystem::RenderSystem(Gnosis::ECS* ecsSystem, const GameCore::PlatformDelegates& platformDelegates)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_activeCamera(0)
        , m_useRenderLayers(true)
        , m_screenInfoValid(false)
    {
        GN_LOG_INFO("RenderSystem initialized with unified rendering");
        m_renderQueue.reserve(1000); // Pre-allocate for performance
        
        // Initialize screen info
        UpdateScreenInfo();
        SetupLayout();
    }

    RenderSystem::~RenderSystem() {
        GN_LOG_INFO("RenderSystem destroyed");
    }

    void RenderSystem::Render() {
        if (!m_ecsSystem) {
            return;
        }

        // Clear render queue
        m_renderQueue.clear();
        
        // Collect all renderable items
        CollectRenderItems();
        
        // Sort by layer and depth
        SortRenderQueue();
        
        // Render world space items (backgrounds, game objects, player)
        RenderWorldSpace();
        
        // Render screen space items (UI)
        RenderScreenSpace();
    }

    void RenderSystem::SetActiveCamera(Gnosis::Entity cameraEntity) {
        m_activeCamera = cameraEntity;
        GN_LOG_INFO("Active camera set to entity: " + std::to_string(cameraEntity));
    }

    void RenderSystem::CollectRenderItems() {
        // Collect all entities with Transform and Sprite components
        auto renderableEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite>();
        for (Gnosis::Entity entity : renderableEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);
            if (!transform || !sprite) continue;
            // Respect coin/pickup visibility and active state to avoid rendering collected coins
            if (!sprite->visible) continue;
            if (auto pickup = m_ecsSystem->GetComponent<Pickup>(entity)) {
                if (!pickup->isActive) {
                    GN_LOG_DEBUG(std::string("RenderSystem: skip inactive pickup id=") + std::to_string(entity));
                    continue;
                }
            }

            // CULLING: Only render sprites that are on-screen or near-screen
            // This dramatically improves performance by not rendering off-screen obstacles
            // IMPORTANT: Only cull objects on the right side, not objects scrolling left
            float screenWidth = m_screenInfoValid ? m_screenInfo.pixelWidth : 1179.0f;
            float cullMargin = 200.0f; // Extra margin to avoid pop-in
            float entityScreenX = WorldToScreen(transform->position).x;
            
            // Skip if entity is too far off-screen to the RIGHT only
            if (entityScreenX > screenWidth + cullMargin) {
                continue;
            }

            if (!transform || !sprite || !sprite->visible) {
                continue;
            }
            
            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = sprite;
            item.text = nullptr;  // No text for sprite items
            item.shape = nullptr;
            item.layer = sprite->layer;
            
            // Calculate depth based on position and layer
            // Higher layers are rendered on top, within layers Y position determines depth
            item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;
            
            m_renderQueue.push_back(item);
        }
        
        // Collect all entities with Transform and Text components
        auto textEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Text>();
        GN_LOG_INFO("RenderSystem: Found " + std::to_string(textEntities.size()) + " text entities");
        
        for (Gnosis::Entity entity : textEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto text = m_ecsSystem->GetComponent<Text>(entity);
            
            GN_LOG_INFO("RenderSystem: Text entity " + std::to_string(entity) + 
                       " - transform=" + (transform ? "yes" : "no") + 
                       ", text=" + (text ? "yes" : "no") + 
                       ", visible=" + (text ? std::to_string(text->visible) : "N/A") +
                       ", content='" + (text ? text->text : "N/A") + "'");
            
            if (!transform || !text || !text->visible) {
                GN_LOG_INFO("RenderSystem: Skipping text entity " + std::to_string(entity) + " - missing components or not visible");
                continue;
            }
            
            // CULLING: Only render text that is on-screen or near-screen
            // This improves performance by not rendering off-screen text
            // IMPORTANT: Only cull objects on the right side, not objects scrolling left
            float screenWidth = m_screenInfoValid ? m_screenInfo.pixelWidth : 1179.0f;
            float cullMargin = 200.0f; // Extra margin to avoid pop-in
            float entityScreenX = WorldToScreen(transform->position).x;
            
            // Skip if entity is too far off-screen to the RIGHT only
            if (entityScreenX > screenWidth + cullMargin) {
                continue;
            }
            
            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = nullptr;  // No sprite for text
            item.text = text;
            item.shape = nullptr;
            item.layer = text->layer;
            
            // Calculate depth based on position and layer
            item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;
            
            m_renderQueue.push_back(item);
            GN_LOG_INFO("RenderSystem: Added text entity " + std::to_string(entity) + " to render queue at layer " + std::to_string(text->layer));
        }
        
    // Collect all entities with UIElement components (buttons, UI sprites)
    auto uiElementEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, UIElement>();
    GN_LOG_INFO("RenderSystem: Found " + std::to_string(uiElementEntities.size()) + " UI entities");
    
    for (Gnosis::Entity entity : uiElementEntities) {
        auto transform = m_ecsSystem->GetComponent<Transform>(entity);
        auto uiElement = m_ecsSystem->GetComponent<UIElement>(entity);
        auto sprite = m_ecsSystem->GetComponent<Sprite>(entity); // UI elements may have sprites
        
        if (!transform || !uiElement || !uiElement->visible) {
            continue;
        }
        
        RenderItem item;
        item.entity = entity;
        item.transform = transform;
        item.sprite = sprite; // May be null for text-only UI elements
        item.text = nullptr;  // UI text is handled via UIElement.buttonText
        item.shape = nullptr;
        item.layer = uiElement->textLayer; // Use textLayer for UI elements
        
        // Calculate depth based on position and layer
        item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;
        
        m_renderQueue.push_back(item);
        GN_LOG_INFO("RenderSystem: Added UI entity " + std::to_string(entity) + " to render queue at layer " + std::to_string(uiElement->textLayer));
    }

        // Collect all entities with DebugDraw components for debug overlays
        auto debugEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, DebugDraw>();
        GN_LOG_DEBUG("RenderSystem: Found " + std::to_string(debugEntities.size()) + " debug entities");
        
        int debugEntitiesRendered = 0;
        for (Gnosis::Entity entity : debugEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto debugDraw = m_ecsSystem->GetComponent<DebugDraw>(entity);
            
            if (!transform || !debugDraw) {
                continue;
            }
            
            // CULLING: For debug entities, use much more lenient culling to allow debugging off-screen hitboxes
            // This allows us to see debug hitboxes even when entities are off-screen for debugging purposes
            float screenWidth = m_screenInfoValid ? m_screenInfo.pixelWidth : 1179.0f;
            float debugCullMargin = 10000.0f; // Much larger margin for debug entities (10x normal)
            float entityScreenX = WorldToScreen(transform->position).x;
            
            // Skip if entity is extremely far off-screen to the RIGHT only (very lenient for debug)
            if (entityScreenX > screenWidth + debugCullMargin) {
                GN_LOG_DEBUG("RenderSystem: Culling debug entity " + std::to_string(entity) + " at screen X " + std::to_string(entityScreenX) + " (extreme right cull)");
                continue;
            }
            
            GN_LOG_DEBUG("RenderSystem: Rendering debug entity " + std::to_string(entity) + " at screen X " + std::to_string(entityScreenX));
            debugEntitiesRendered++;
            
            // Add debug rectangles as render items using Hitbox (unified)
            auto hitbox = m_ecsSystem->GetComponent<Hitbox>(entity);
            if (hitbox) {
                if (debugDraw->showBounds) {
            RenderItem debugItem;
                    debugItem.entity = entity;
                    debugItem.transform = transform;
                    // Provide sprite so overlay centering uses sprite half-dimensions
                    debugItem.sprite = m_ecsSystem->GetComponent<Sprite>(entity);
                    debugItem.text = nullptr;
            debugItem.shape = nullptr;
                    debugItem.layer = debugDraw->debugLayer;  // High priority layer
                    debugItem.depth = static_cast<float>(debugItem.layer) * 1000.0f + transform->position.y;
                    debugItem.isDebugBounds = true;
                    debugItem.debugColor = debugDraw->boundsColor;
                    debugItem.debugAlpha = debugDraw->alpha;
                    debugItem.debugWidth = hitbox->width;
                    debugItem.debugHeight = hitbox->height;
                    debugItem.debugOffsetX = hitbox->offsetX;
                    debugItem.debugOffsetY = hitbox->offsetY;
                    
                    // Log when adding spike ball debug items
                    auto obstacle = m_ecsSystem->GetComponent<Obstacle>(entity);
                    if (obstacle && obstacle->obstacleType == "SpikeBall") {
                        GN_LOG_DEBUG("RenderSystem: Adding spike ball debug bounds to render queue - entity " + std::to_string(entity) + 
                                   " at pos (" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + 
                                   ") size (" + std::to_string(hitbox->width) + ", " + std::to_string(hitbox->height) + ")");
                    }
                    
                    m_renderQueue.push_back(debugItem);
                }

                if (debugDraw->showCollider) {
            RenderItem debugItem;
                    debugItem.entity = entity;
                    debugItem.transform = transform;
                    // Provide sprite so overlay centering uses sprite half-dimensions
                    debugItem.sprite = m_ecsSystem->GetComponent<Sprite>(entity);
                    debugItem.text = nullptr;
            debugItem.shape = nullptr;
                    debugItem.layer = debugDraw->debugLayer;  // High priority layer
                    debugItem.depth = static_cast<float>(debugItem.layer) * 1000.0f + transform->position.y;
                    debugItem.isDebugCollider = true;
                    debugItem.debugColor = debugDraw->colliderColor;
                    debugItem.debugAlpha = debugDraw->alpha;
                    debugItem.debugOffsetX = hitbox->offsetX;
                    debugItem.debugOffsetY = hitbox->offsetY;
                    
                    // Check if this is a spikeball - use absolute positioning for spikeballs
                    auto obstacle = m_ecsSystem->GetComponent<Obstacle>(entity);
                    if (obstacle && obstacle->obstacleType == "SpikeBall") {
                        debugItem.debugAbsolutePos = true;  // Use absolute world coordinates for spikeballs
                    }
                    
                    // Circle vs rectangle collider visualization
                    if (hitbox->type == GameCore::ColliderType::Circle) {
                        debugItem.debugIsCircle = true;
                        debugItem.debugRadius = hitbox->radius;
                    } else {
                        debugItem.debugIsCircle = false;
                        debugItem.debugWidth = hitbox->width;
                        debugItem.debugHeight = hitbox->height;
                    }
                    
                    // Log when adding spike ball debug items  
                    if (obstacle && obstacle->obstacleType == "SpikeBall") {
                        GN_LOG_DEBUG("RenderSystem: Adding spike ball debug collider to render queue - entity " + std::to_string(entity) + 
                                   " at pos (" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + 
                                   ") type " + (hitbox->type == GameCore::ColliderType::Circle ? "Circle" : "Rectangle") +
                                   ") ABSOLUTE_POS_MODE");
                    }
                    
                    m_renderQueue.push_back(debugItem);
                }
            }
        }
        
        GN_LOG_DEBUG("RenderSystem: Debug rendering summary - " + std::to_string(debugEntitiesRendered) + " of " + std::to_string(debugEntities.size()) + " debug entities rendered");

        // Collect UIElement-only entities so they can render buttonText in screen space
        // This ensures UI elements without Sprite/Text still enter the render queue and get layered properly
        auto uiEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, UIElement>();
    for (Gnosis::Entity entity : uiEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto ui = m_ecsSystem->GetComponent<UIElement>(entity);

            if (!transform || !ui || !ui->visible) {
                continue;
            }

        RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = nullptr;
        item.text = nullptr;
        item.shape = nullptr;
            item.layer = ui->textLayer; // Use UI text layer for ordering
            item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;

            m_renderQueue.push_back(item);
        }

        // Collect UIShape entities (simple rectangles/lines in screen space)
        auto shapeEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, UIShape>();
        for (Gnosis::Entity entity : shapeEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto shape = m_ecsSystem->GetComponent<UIShape>(entity);
            if (!transform || !shape || !shape->visible) continue;

            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = nullptr;
            item.text = nullptr;
            item.shape = shape;
            item.layer = shape->layer;
            item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;
            m_renderQueue.push_back(item);
        }
    }

    void RenderSystem::SortRenderQueue() {
        if (!m_useRenderLayers) {
            return;
        }
        
        // Sort by layer first, then by depth within layer
        std::sort(m_renderQueue.begin(), m_renderQueue.end(), 
            [](const RenderItem& a, const RenderItem& b) {
                if (a.layer != b.layer) {
                    return a.layer < b.layer; // Lower layers render first (background)
                }
                return a.depth < b.depth; // Within layer, sort by depth
            });
    }

    void RenderSystem::RenderWorldSpace() {
        // Render all world space items
        for (const RenderItem& item : m_renderQueue) {
            // Skip UI elements (they're rendered in screen space)
            if (m_ecsSystem->HasComponent<UIElement>(item.entity)) {
                continue;
            }

            // If this is a sprite but drawSprite is unavailable, skip just this item
            if (item.sprite && !m_platformDelegates.renderer.drawSprite) {
                continue;
            }

            // Debug rectangles and text have their own delegate checks inside RenderSingleItem
            RenderSingleItem(item);
        }
    }

    void RenderSystem::RenderScreenSpace() {
        // Render UI elements in screen space (no camera transformation)
        for (const RenderItem& item : m_renderQueue) {
            // Render UI sprites (entities with UIElement component)
            if (m_ecsSystem->HasComponent<UIElement>(item.entity)) {
                UIElement* uiElement = m_ecsSystem->GetComponent<UIElement>(item.entity);
                if (!uiElement || !uiElement->visible) {
                    continue;
                }
                
                // Determine which texture to use based on button state
                std::string textureId;
                if (uiElement->isPressed && !uiElement->pressedTextureId.empty()) {
                    textureId = uiElement->pressedTextureId;
                } else if (uiElement->isHovered && !uiElement->hoverTextureId.empty()) {
                    textureId = uiElement->hoverTextureId;
                } else if (!uiElement->normalTextureId.empty()) {
                    textureId = uiElement->normalTextureId;
                }
                
                // Only render if we have a texture ID
                if (!textureId.empty() && m_platformDelegates.renderer.drawSprite) {
                    // Render UI directly in screen coordinates (PIXELS)
                    Gnosis::GNVector2 screenPos = item.transform->position;
                    GN_LOG_DEBUG("RenderSystem(UI): entity=" + std::to_string(item.entity) +
                                  " transformPos=(" + std::to_string(screenPos.x) + "," + std::to_string(screenPos.y) + ")" );
                    // UI should not be affected by camera zoom; use transform scale only
                    float scale = item.transform->scale.x;
                    
                    // Load texture from UIElement component (not Sprite component)
                    uint32_t textureHandle = GetOrLoadTexture(textureId, item.entity);
                    if (textureHandle == 0) {
                        GN_LOG_WARN("RenderSystem(UI): Invalid/zero texture handle for id '" + textureId + "' — skipping UI sprite draw");
                    } else {
                        // Use sprite dimensions if available, otherwise use default button texture size
                        float width = 90.0f;  // Actual button texture width (90x16 as shown in asset)
                        float height = 16.0f; // Actual button texture height
                        if (item.sprite) {
                            width = item.sprite->width;
                            height = item.sprite->height;
                        }
                        
                        // Draw centered in PIXEL space if needed; our screenPos is already in pixels
                        m_platformDelegates.renderer.drawSpriteScaled(
                            textureHandle,
                            screenPos.x,
                            screenPos.y,
                            scale,  // Use scale factor, not pixel dimensions
                            scale,  // Use scale factor, not pixel dimensions
                            item.transform->rotation
                        );
                        GN_LOG_DEBUG("RenderSystem(UI): drawSpriteScaled at (" + std::to_string(screenPos.x) + "," + std::to_string(screenPos.y) + ") scale=" + std::to_string(scale));
                        GN_LOG_INFO("RenderSystem(UI): Rendered UI sprite '" + textureId + "' at (" + 
                                   std::to_string(screenPos.x) + "," + std::to_string(screenPos.y) + 
                                   ") scale " + std::to_string(scale) + "x" + std::to_string(scale) + 
                                   " (texture: " + std::to_string(width) + "x" + std::to_string(height) + ")");
                    }
                }
            }
            // Render Text components (UI text like scores, pipe counter) - check this FIRST
            if (item.text) {
                if (!m_platformDelegates.renderer.drawText) {
                    GN_LOG_ERROR("RenderSystem: drawText delegate is null for entity " + std::to_string(item.entity));
                    continue;
                }
                
                float r = item.text->color.r / 255.0f;
                float g = item.text->color.g / 255.0f;
                float b = item.text->color.b / 255.0f;
                float a = item.text->color.a / 255.0f;
                
                GN_LOG_INFO("RenderSystem: Drawing text '" + item.text->text + "' at (" + 
                           std::to_string(item.transform->position.x) + ", " + 
                           std::to_string(item.transform->position.y) + ") with color (" +
                           std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a) + ")");
                
                m_platformDelegates.renderer.drawText(
                    item.text->text,
                    item.transform->position.x,
                    item.transform->position.y,
                    item.text->fontSize,
                    r, g, b, a
                );
            }
            // Also render UIElement buttonText if it exists (for buttons without Text components)
            else if (m_ecsSystem->HasComponent<UIElement>(item.entity)) {
                UIElement* ui = m_ecsSystem->GetComponent<UIElement>(item.entity);
                if (!ui || !ui->visible || ui->buttonText.empty()) {
                    continue;
                }

                // Choose text color based on hover state
                float r, g, b, a;
                if (ui->isHovered) {
                    r = ui->textHoverColor.r / 255.0f;
                    g = ui->textHoverColor.g / 255.0f;
                    b = ui->textHoverColor.b / 255.0f;
                    a = ui->textHoverColor.a / 255.0f;
                } else {
                    r = ui->textColor.r / 255.0f;
                    g = ui->textColor.g / 255.0f;
                    b = ui->textColor.b / 255.0f;
                    a = ui->textColor.a / 255.0f;
                }

                if (a < 0.01f) {
                    GN_LOG_ERROR("RenderSystem: UI text alpha is very low (" + std::to_string(a) + ") - text may be invisible!");
                }

                // Determine button bounds if a Sprite is attached
                float buttonWidth = 90.0f * item.transform->scale.x;
                float buttonHeight = 16.0f * item.transform->scale.y;
                if (Sprite* s = m_ecsSystem->GetComponent<Sprite>(item.entity)) {
                    buttonWidth = s->width * item.transform->scale.x;
                    buttonHeight = s->height * item.transform->scale.y;
                }

                // Compute text position with centering and baseline correction (pixels only)
                float textX = item.transform->position.x;
                float textY = item.transform->position.y;

                const bool isTextOnly = ui->normalTextureId.empty();

                if (ui->centerTextHorizontally) {
                    textX = isTextOnly ? item.transform->position.x
                                       : item.transform->position.x + (buttonWidth * 0.5f);
                }

                if (ui->centerTextVertically) {
                    if (isTextOnly) {
                        textY = item.transform->position.y; // already a center Y for text-only elements
                    } else {
                        // Pass the true visual center to the centered draw path.
                        // Vertical centering and baseline alignment are handled in MetalRenderer.
                        const float buttonCenterY = item.transform->position.y + (buttonHeight * 0.5f);
                        textY = buttonCenterY;
                    }
                }

                // Apply manual offsets
                textX += ui->textOffsetX;
                textY += ui->textOffsetY;

                // Draw using centered or non-centered path in pixel coordinates
                // Use the font size as provided by UIElement; do not auto-scale with button transform
                float effectiveFontSize = ui->fontSize;
                if (ui->centerTextHorizontally && ui->centerTextVertically && m_platformDelegates.renderer.drawTextCenteredOutlined) {
                    // Centered outlined multi-line support
                    float outlineWidth = 14.0f; // thicker outline for better readability
                    float uiOutline = (ui->textOutlineWidth > 0.0f) ? ui->textOutlineWidth : outlineWidth;
                    std::vector<std::string> lines;
                    {
                        std::string s = ui->buttonText;
                        size_t start = 0;
                        while (true) {
                            size_t pos = s.find('\n', start);
                            if (pos == std::string::npos) { lines.push_back(s.substr(start)); break; }
                            lines.push_back(s.substr(start, pos - start));
                            start = pos + 1;
                        }
                    }
                    float lineHeight = effectiveFontSize * 1.1f;
                    float totalHeight = lineHeight * static_cast<float>(lines.size());
                    float startY = textY - totalHeight * 0.5f + lineHeight * 0.5f;
                    for (size_t i = 0; i < lines.size(); ++i) {
                        float lineY = startY + lineHeight * static_cast<float>(i);
                        m_platformDelegates.renderer.drawTextCenteredOutlined(
                            lines[i], textX, lineY, effectiveFontSize,
                            r, g, b, a,
                            0.0f, 0.0f, 0.0f, 1.0f,
                            uiOutline
                        );
                    }
                } else if (ui->centerTextHorizontally && ui->centerTextVertically && m_platformDelegates.renderer.drawTextCentered) {
                    // Support multi-line center: split on '\n' and stack lines vertically, centered on the anchor
                    {
                        float totalHeight = 0.0f;
                        std::vector<std::string> lines;
                        {
                            std::string s = ui->buttonText;
                            size_t start = 0;
                            while (true) {
                                size_t pos = s.find('\n', start);
                                if (pos == std::string::npos) { lines.push_back(s.substr(start)); break; }
                                lines.push_back(s.substr(start, pos - start));
                                start = pos + 1;
                            }
                        }
                        // Measure each line height approximately using font size; MetalRenderer will baseline-correct.
                        float lineHeight = effectiveFontSize * 1.1f;
                        totalHeight = lineHeight * static_cast<float>(lines.size());
                        float startY = textY - totalHeight * 0.5f + lineHeight * 0.5f;
                        for (size_t i = 0; i < lines.size(); ++i) {
                            float lineY = startY + lineHeight * static_cast<float>(i);
                            m_platformDelegates.renderer.drawTextCentered(
                                lines[i], textX, lineY, effectiveFontSize, r, g, b, a
                            );
                        }
                    }
                } else if (m_platformDelegates.renderer.drawTextOutlined) {
                    // Left/top anchored multiline support
                    std::string s = ui->buttonText;
                    size_t start = 0;
                    float lineHeight = effectiveFontSize * 1.1f;
                    float currentY = textY;
                    // Use UI-provided outline width when available; fallback to 10 for parity with centered path
                    float uiOutline = (ui->textOutlineWidth > 0.0f) ? ui->textOutlineWidth : 10.0f;
                    while (true) {
                        size_t pos = s.find('\n', start);
                        std::string line = (pos == std::string::npos) ? s.substr(start) : s.substr(start, pos - start);
                        m_platformDelegates.renderer.drawTextOutlined(
                            line, textX, currentY, effectiveFontSize,
                            r, g, b, a,
                            0.0f, 0.0f, 0.0f, 1.0f,
                            uiOutline
                        );
                        if (pos == std::string::npos) break;
                        start = pos + 1;
                        currentY += lineHeight;
                    }
                } else if (m_platformDelegates.renderer.drawText) {
                    // Left/top anchored multiline support
                    std::string s = ui->buttonText;
                    size_t start = 0;
                    float lineHeight = effectiveFontSize * 1.1f;
                    float currentY = textY;
                    while (true) {
                        size_t pos = s.find('\n', start);
                        std::string line = (pos == std::string::npos) ? s.substr(start) : s.substr(start, pos - start);
                        m_platformDelegates.renderer.drawText(
                            line, textX, currentY, effectiveFontSize, r, g, b, a
                        );
                        if (pos == std::string::npos) break;
                        start = pos + 1;
                        currentY += lineHeight;
                    }
                } else {
                    GN_LOG_ERROR("RenderSystem: No text drawing delegate available for UIElement text");
                }
            }
        }
    }

    // Consolidated into RenderSingleItem

    void RenderSystem::RenderSingleItem(const RenderItem& item) {
        // Debug overlays must take precedence even if a Sprite is present on the item.
        // This allows passing Sprite for centering math without re-rendering the sprite.
        if (item.isDebugBounds || item.isDebugCollider) {
            // Render debug overlays in pixel coordinates with transform scaling
            // Hitbox offsets are relative to the sprite CENTER.
            // Transform position is sprite top-left; convert to center for correct overlay placement.
            Gnosis::GNVector2 screenPosTopLeft = WorldToScreen(item.transform->position);

            // Scale offsets and sizes by transform scale to match sprite scaling
            float sx = item.transform->scale.x;
            float sy = item.transform->scale.y;

            float width  = item.debugWidth * sx;
            float height = item.debugHeight * sy;
            float radius = item.debugRadius * ((sx + sy) * 0.5f);

            // Calculate center position based on positioning mode
            float centerX, centerY;
            
            if (item.debugAbsolutePos) {
                // ABSOLUTE POSITIONING: Use transform position + offset directly (no sprite center adjustment)
                centerX = screenPosTopLeft.x + (item.debugOffsetX * sx);
                centerY = screenPosTopLeft.y + (item.debugOffsetY * sy);
            } else {
                // RELATIVE POSITIONING: Use CENTER-based offsets to match Hitbox semantics; add sprite half-dimensions if provided
                float spriteHalfW = 0.0f;
                float spriteHalfH = 0.0f;
                if (item.sprite) {
                    spriteHalfW = (item.sprite->width * sx) * 0.5f;
                    spriteHalfH = (item.sprite->height * sy) * 0.5f;
                }
                centerX = screenPosTopLeft.x + spriteHalfW + (item.debugOffsetX * sx);
                centerY = screenPosTopLeft.y + spriteHalfH + (item.debugOffsetY * sy);
            }
            float debugX = centerX - (width * 0.5f);
            float debugY = centerY - (height * 0.5f);

            // Convert color to normalized values with debug alpha
            float r = item.debugColor.r / 255.0f;
            float g = item.debugColor.g / 255.0f;
            float b = item.debugColor.b / 255.0f;
            float a = item.debugAlpha;

            if (item.debugIsCircle && m_platformDelegates.renderer.drawCircle) {
                // Circles are drawn from center
                m_platformDelegates.renderer.drawCircle(
                    centerX,
                    centerY,
                    radius,
                    r, g, b, a
                );
            } else if (m_platformDelegates.renderer.drawRectangle) {
                m_platformDelegates.renderer.drawRectangle(
                    debugX,
                    debugY,
                    width,
                    height,
                    r, g, b, a
                );
            }
        } else if (item.shape) {
            // Render UIShape (screen-space)
            if (!m_platformDelegates.renderer.drawRectangle) return;
            const float r = item.shape->color.r / 255.0f;
            const float g = item.shape->color.g / 255.0f;
            const float b = item.shape->color.b / 255.0f;
            const float a = item.shape->color.a / 255.0f;

            m_platformDelegates.renderer.drawRectangle(
                item.transform->position.x,
                item.transform->position.y,
                item.shape->width * item.transform->scale.x,
                item.shape->height * item.transform->scale.y,
                r, g, b, a
            );
        } else if (item.text) {
            // Render text - use screen coordinates directly (no world-to-screen transform for UI)
            if (m_platformDelegates.renderer.drawText) {
                float r = item.text->color.r / 255.0f;
                float g = item.text->color.g / 255.0f;
                float b = item.text->color.b / 255.0f;
                float a = item.text->color.a / 255.0f;
                
                m_platformDelegates.renderer.drawText(
                    item.text->text,
                    item.transform->position.x,
                    item.transform->position.y,
                    item.text->fontSize,
                    r, g, b, a
                );
            }
        } else if (item.sprite) {
            // Render sprite
            // Transform world position to screen position
            Gnosis::GNVector2 screenPos = WorldToScreen(item.transform->position);
            float scale = GetCameraScale() * item.transform->scale.x;
            
            // Apply parallax scaling for background layers
            if (item.layer >= BACKGROUND_LAYER_START && item.layer <= BACKGROUND_LAYER_END) {
                Parallax* parallax = m_ecsSystem->GetComponent<Parallax>(item.entity);
                if (parallax) {
                    // Parallax layers might have different scaling
                    scale *= 1.0f; // Could be adjusted per layer
                }
            }
            
            // 1:1 with SpriteSystem: resolve via local cache/loading
            uint32_t textureHandle = GetOrLoadTexture(item.sprite->textureId, item.entity);
            if (textureHandle == 0) {
                GN_LOG_WARN("RenderSystem: Invalid/zero texture handle for id '" + item.sprite->textureId + "' — skipping sprite draw");
            } else {
                // Compute final scale based on sprite frame vs logical size
                bool usesCenteredRendering = m_ecsSystem->HasComponent<RotationRenderer>(item.entity);
                bool usesPivotRotation = m_ecsSystem->HasComponent<PivotRotationRenderer>(item.entity);
                
                // Debug: Log when we have a PivotRotationRenderer
                if (usesPivotRotation) {
                    PivotRotationRenderer* pivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(item.entity);
                    if (pivotRenderer) {
                        GN_LOG_DEBUG("RenderSystem: Entity " + std::to_string(item.entity) + " has PivotRotationRenderer with pivot (" + 
                                   std::to_string(pivotRenderer->pivotX) + ", " + std::to_string(pivotRenderer->pivotY) + ")");
                    }
                }
                
                // Debug: Log when we have a PivotRotationRenderer
                if (usesPivotRotation) {
                    PivotRotationRenderer* pivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(item.entity);
                    if (pivotRenderer) {
                        GN_LOG_DEBUG("RenderSystem: Entity " + std::to_string(item.entity) + " has PivotRotationRenderer with pivot (" + 
                                   std::to_string(pivotRenderer->pivotX) + ", " + std::to_string(pivotRenderer->pivotY) + ")");
                    }
                }
                float scaleX = item.sprite->width / item.sprite->frameWidth;
                float scaleY = item.sprite->height / item.sprite->frameHeight;
                float finalScaleX = scaleX * item.transform->scale.x * GetCameraScale();
                float finalScaleY = scaleY * item.transform->scale.y * GetCameraScale();

                // If animated and we have a with-source delegate, render the correct frame sub-rect
                if (item.sprite->isAnimated && m_platformDelegates.renderer.drawSpriteScaledWithSource) {
                    int safeFrameCount = item.sprite->frameCount > 0 ? item.sprite->frameCount : 1;
                    int currentFrame = item.sprite->currentFrame % safeFrameCount;
                    int frameX = currentFrame * static_cast<int>(item.sprite->frameWidth);
                    int frameY = 0;

                    m_platformDelegates.renderer.drawSpriteScaledWithSource(
                        textureHandle,
                        screenPos.x,
                        screenPos.y,
                        finalScaleX,
                        finalScaleY,
                        item.transform->rotation,
                        static_cast<float>(frameX),
                        static_cast<float>(frameY),
                        item.sprite->frameWidth,
                        item.sprite->frameHeight
                    );
                } else if (usesCenteredRendering && m_platformDelegates.renderer.drawSpriteScaledCentered) {
                    // Centered rendering path (e.g., for rotating hats)
                    m_platformDelegates.renderer.drawSpriteScaledCentered(
                        textureHandle,
                        screenPos.x,
                        screenPos.y,
                        finalScaleX,
                        finalScaleY,
                        item.transform->rotation
                    );
                } else if (usesPivotRotation && m_platformDelegates.renderer.drawSpriteScaledPivoted) {
                    // Pivot-based rotation rendering using the new pivot function (PRIORITY)
                    PivotRotationRenderer* pivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(item.entity);
                    if (pivotRenderer) {
                        // Pass pivot coordinates as sprite-relative pixels (no conversion here)
                        // Pivot values are in pixels relative to sprite center (e.g., 0, -45 for spikeball chain connection)
                        // MetalRenderer will handle all coordinate conversions in one place
                        float pivotPixelX = pivotRenderer->pivotX;  // Keep as center-relative pixels
                        float pivotPixelY = pivotRenderer->pivotY;  // Keep as center-relative pixels
                        
                        // Use the new pivot-based rendering function
                        m_platformDelegates.renderer.drawSpriteScaledPivoted(
                            textureHandle,
                            screenPos.x,
                            screenPos.y,
                            finalScaleX,
                            finalScaleY,
                            item.transform->rotation,
                            pivotPixelX,
                            pivotPixelY
                        );
                    }
                } else if (usesPivotRotation && m_platformDelegates.renderer.drawSpriteScaledCentered) {
                    // Fallback pivot-based rotation rendering (e.g., for spike ball rotating from base)
                    PivotRotationRenderer* pivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(item.entity);
                    if (pivotRenderer) {
                        // Calculate pivot offset in world space
                        float pivotOffsetX = pivotRenderer->pivotX * item.transform->scale.x;
                        float pivotOffsetY = pivotRenderer->pivotY * item.transform->scale.y;
                        
                        // Adjust screen position to account for pivot
                        float adjustedX = screenPos.x - pivotOffsetX;
                        float adjustedY = screenPos.y - pivotOffsetY;
                        
                        // Use centered rendering with adjusted position
                        m_platformDelegates.renderer.drawSpriteScaledCentered(
                            textureHandle,
                            adjustedX,
                            adjustedY,
                            finalScaleX,
                            finalScaleY,
                            item.transform->rotation
                        );
                    }
                } else if (m_platformDelegates.renderer.drawSpriteScaled) {
                    // Default top-left rendering
                    m_platformDelegates.renderer.drawSpriteScaled(
                        textureHandle,
                        screenPos.x,
                        screenPos.y,
                        finalScaleX,
                        finalScaleY,
                        item.transform->rotation
                    );
                } else {
                    GN_LOG_ERROR("RenderSystem: No sprite rendering delegate available for entity " + std::to_string(item.entity));
                }
            }
        }
    }

    Gnosis::GNVector2 RenderSystem::WorldToScreen(const Gnosis::GNVector2& worldPos) {
        if (m_activeCamera == 0) {
            // No camera, return world position directly
            return worldPos;
        }
        
        Gnosis::GNVector2 cameraPos = GetCameraPosition();
        
        // Basic camera transformation (subtract camera position)
        // In a side-scroller, we typically only care about X offset
        return Gnosis::GNVector2(
            worldPos.x - cameraPos.x,
            worldPos.y  // Y stays the same in a typical Flappy Bird style game
        );
    }

    float RenderSystem::GetCameraScale() const {
        if (m_activeCamera == 0) {
            return 1.0f;
        }
        
        Camera* camera = m_ecsSystem->GetComponent<Camera>(m_activeCamera);
        return camera ? camera->zoom : 1.0f;
    }

    Gnosis::GNVector2 RenderSystem::GetCameraPosition() const {
        if (m_activeCamera == 0) {
            return Gnosis::GNVector2(0.0f, 0.0f);
        }
        
        Camera* camera = m_ecsSystem->GetComponent<Camera>(m_activeCamera);
        if (camera) {
            return camera->position;
        }
        
        // Fallback to camera transform
        Transform* transform = m_ecsSystem->GetComponent<Transform>(m_activeCamera);
        return transform ? transform->position : Gnosis::GNVector2(0.0f, 0.0f);
    }

    void RenderSystem::UpdateScreenInfo() {
        // Always use enhanced getScreenInfo when available; avoid 800x600 legacy fallback
        if (m_platformDelegates.renderer.getScreenInfo) {
            m_platformDelegates.renderer.getScreenInfo(&m_screenInfo);
            m_screenInfoValid = true;
            GN_LOG_INFO("Screen info updated: " +
                        std::to_string((int)m_screenInfo.logicalWidth) + "x" +
                        std::to_string((int)m_screenInfo.logicalHeight) +
                        " (" + std::to_string((int)m_screenInfo.pixelWidth) + "x" +
                        std::to_string((int)m_screenInfo.pixelHeight) + " pixels)");
            return;
        }
        // If enhanced info is not available, do NOT override existing valid info with legacy values.
        if (!m_screenInfoValid && m_platformDelegates.renderer.getScreenSize) {
            m_platformDelegates.renderer.getScreenSize(&m_screenInfo.logicalWidth, &m_screenInfo.logicalHeight);
            m_screenInfo.pixelWidth = m_screenInfo.logicalWidth;
            m_screenInfo.pixelHeight = m_screenInfo.logicalHeight;
            m_screenInfo.scaleFactor = 1.0f;
            m_screenInfo.isPortrait = m_screenInfo.logicalHeight > m_screenInfo.logicalWidth;
            m_screenInfo.deviceModel = "Unknown";
            m_screenInfoValid = true;
            GN_LOG_WARN("Legacy screen size used once: " + std::to_string((int)m_screenInfo.logicalWidth) + "x" + std::to_string((int)m_screenInfo.logicalHeight));
        }
    }

    float RenderSystem::GetDynamicScale() const {
        if (!m_screenInfoValid) {
            return 1.0f;
        }
        
        // Calculate dynamic scale based on screen size
        // Use logical height as base reference (iPhone 16 logical height is 852)
        const float referenceHeight = 852.0f;  // iPhone 16 logical height
        return m_screenInfo.logicalHeight / referenceHeight;
    }

    float RenderSystem::GetUIScale() const {
        if (!m_screenInfoValid) {
            return 1.0f;
        }
        
        // UI scaling is different - we want consistent UI sizes across devices
        // Use a base scale that looks good on most devices
        float baseScale = GetDynamicScale();
        
        // Clamp UI scale to reasonable bounds
        return std::max(0.8f, std::min(2.0f, baseScale));
    }

    void RenderSystem::SetupLayout() {
        if (!m_screenInfoValid) {
            GN_LOG_WARN("Cannot setup layout - screen info not valid");
            return;
        }
        
#ifdef PLATFORM_IOS
        SetupIOSLayout();
#else
        SetupDesktopLayout();
#endif
    }

    void RenderSystem::SetupIOSLayout() {
        GN_LOG_INFO("Setting up iOS layout for device: " + m_screenInfo.deviceModel);
        
        // iOS-specific layout calculations
        float scale = GetDynamicScale();
        GN_LOG_INFO("Calculated dynamic scale: " + std::to_string(scale));
        
        // Additional iOS-specific setup can go here
        // e.g., safe area calculations, notch handling, etc.
    }

    void RenderSystem::SetupDesktopLayout() {
        GN_LOG_INFO("Setting up desktop layout");
        
        // Desktop-specific layout calculations
        float scale = GetDynamicScale();
        GN_LOG_INFO("Calculated dynamic scale: " + std::to_string(scale));
        
        // Additional desktop-specific setup can go here
    }

    void RenderSystem::CalculateDynamicScaling() {
        // This method can be called when screen info changes
        // to recalculate any cached scaling values
        UpdateScreenInfo();
        SetupLayout();
    }

    // 1:1 with SpriteSystem: return textureId directly for iOS asset catalog
    std::string RenderSystem::GetFullTexturePath(const std::string& textureId) const {
        // For iOS asset catalog, just return the texture ID directly
        // The asset catalog structure already organizes assets by folder
        return textureId;
    }

    // 1:1 with SpriteSystem: check cache/pending, enqueue async load via AssetDelegate
    uint32_t RenderSystem::GetOrLoadTexture(const std::string& textureId, Gnosis::Entity entity) {
        if (textureId.empty()) {
            return 0;
        }
        auto it = m_textureCache.find(textureId);
        if (it != m_textureCache.end()) {
            return it->second;
        }
        if (m_pendingTextures.find(textureId) != m_pendingTextures.end()) {
            return 0; // still loading
        }

        std::string fullPath = GetFullTexturePath(textureId);
        if (m_platformDelegates.asset.fileExists && !m_platformDelegates.asset.fileExists(fullPath.c_str())) {
            GN_LOG_ERROR("RenderSystem: Texture file not found: " + fullPath);
            return 0;
        }

        const char* assetPath = m_platformDelegates.asset.getAssetPath ?
                                m_platformDelegates.asset.getAssetPath(fullPath.c_str()) :
                                fullPath.c_str();

        if (m_platformDelegates.asset.loadTexture) {
            m_pendingTextures.insert(textureId);
            TextureLoadContext* context = new TextureLoadContext(textureId, this, entity);
            m_platformDelegates.asset.loadTexture(
                std::string(assetPath),
                [](GameCore::TextureData* textureData, const char* error, void* userData) {
                    RenderSystem::HandleTextureLoaded(textureData, error, userData);
                },
                context
            );
            return 0;
        }

        GN_LOG_ERROR("RenderSystem: No texture loading interface available for '" + textureId + "'");
        return 0;
    }

    // 1:1 with SpriteSystem: populate local handle cache on load
    void RenderSystem::SetTextureBasePath(const std::string& basePath) {
        m_textureBasePath = basePath;
        GN_LOG_INFO("RenderSystem: Set texture base path to '" + basePath + "'");
    }

    void RenderSystem::HandleTextureLoaded(GameCore::TextureData* textureData, const char* error, void* userData) {
        TextureLoadContext* context = static_cast<TextureLoadContext*>(userData);
        RenderSystem* system = context ? context->system : nullptr;
        if (!system || !context) {
            delete context;
            return;
        }

        if (textureData && textureData->platformTexture) {
            bool hadHandle = (system->m_textureCache.find(context->textureId) != system->m_textureCache.end());
            bool hadDims = (system->m_textureDimensions.find(context->textureId) != system->m_textureDimensions.end());

            uint32_t handle = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(textureData->platformTexture));
            system->m_textureCache[context->textureId] = handle;
            system->m_textureDimensions[context->textureId] = {textureData->width, textureData->height};

            GN_LOG_INFO(
                std::string("RenderSystem: CACHE_STORE id='") + context->textureId +
                "' handle=" + std::to_string(handle) +
                " size=" + std::to_string(textureData->width) + "x" + std::to_string(textureData->height) +
                " first_time=" + std::string((!hadHandle && !hadDims) ? "true" : "false") +
                " source=" + (context->entity != 0 ? std::string("GetOrLoad(entity=") + std::to_string(context->entity) + ")" : "Preload(LoadTexture)")
            );
        } else {
            GN_LOG_ERROR("RenderSystem: Failed to load texture '" + context->textureId + "': " + (error ? error : "Unknown error"));
        }

        system->m_pendingTextures.erase(context->textureId);
        delete context;
    }

    // Ensure a texture is scheduled for loading and will populate metadata cache
    void RenderSystem::EnsureTextureReady(const std::string& textureId) {
        if (textureId.empty()) {
            return;
        }
        // If already pending, skip
        if (m_pendingTextures.find(textureId) != m_pendingTextures.end()) {
            return;
        }
        // If metadata already indicates loaded, nothing to do
        if (m_platformDelegates.asset.getTextureMetadata) {
            GameCore::TextureMetadata meta;
            if (m_platformDelegates.asset.getTextureMetadata(textureId.c_str(), &meta)) {
                if (meta.isLoaded && meta.platformHandle != 0) {
                    // Populate caches if available
                    m_textureCache[textureId] = static_cast<uint32_t>(meta.platformHandle);
                    if (meta.width > 0 && meta.height > 0) {
                        m_textureDimensions[textureId] = {meta.width, meta.height};
                    }
                    return;
                }
            }
        }

        if (!m_platformDelegates.asset.loadTexture) {
            return;
        }

        m_pendingTextures.insert(textureId);

        // Resolve asset path if available; otherwise pass textureId directly
        std::string path = textureId;
        if (m_platformDelegates.asset.getAssetPath) {
            const char* resolved = m_platformDelegates.asset.getAssetPath(textureId.c_str());
            if (resolved && *resolved) {
                path = resolved;
            }
        }

        // Allocate context and enqueue
        auto* ctx = new TextureLoadContext(textureId, this);
        m_platformDelegates.asset.loadTexture(
            path,
            [](GameCore::TextureData* textureData, const char* error, void* userData) {
                RenderSystem::HandleTextureLoaded(textureData, error, userData);
            },
            ctx
        );
    }

    // Public unified texture APIs
    bool RenderSystem::PreloadTexture(const std::string& textureId) {
        if (textureId.empty()) return false;
        // If already loaded, nothing to do
        if (m_textureCache.find(textureId) != m_textureCache.end()) {
            GN_LOG_DEBUG("RenderSystem: CACHE_HIT PreloadTexture '" + textureId + "'");
            return true;
        }
        // If platform has metadata and it's loaded, populate caches
        if (m_platformDelegates.asset.getTextureMetadata) {
            GameCore::TextureMetadata meta;
            if (m_platformDelegates.asset.getTextureMetadata(textureId.c_str(), &meta) && meta.isLoaded && meta.platformHandle != 0) {
                m_textureCache[textureId] = static_cast<uint32_t>(meta.platformHandle);
                if (meta.width > 0 && meta.height > 0) {
                    m_textureDimensions[textureId] = {meta.width, meta.height};
                }
                GN_LOG_DEBUG("RenderSystem: META_POPULATE PreloadTexture '" + textureId + "'");
                return true;
            }
        }
        // If already pending, just return
        if (m_pendingTextures.find(textureId) != m_pendingTextures.end()) {
            GN_LOG_DEBUG("RenderSystem: PENDING PreloadTexture '" + textureId + "'");
            return false;
        }
        // Enqueue load
        GN_LOG_INFO("RenderSystem: PRELOAD_START '" + textureId + "'");
        EnsureTextureReady(textureId);
        return false;
    }

    void RenderSystem::PreloadTextures(const std::vector<std::string>& textureIds) {
        for (const auto& id : textureIds) {
            PreloadTexture(id);
        }
    }

    bool RenderSystem::IsTextureLoaded(const std::string& textureId) const {
        if (textureId.empty()) return false;
        if (m_textureCache.find(textureId) != m_textureCache.end()) return true;
        if (m_platformDelegates.asset.getTextureMetadata) {
            GameCore::TextureMetadata meta;
            if (m_platformDelegates.asset.getTextureMetadata(textureId.c_str(), &meta)) {
                return meta.isLoaded && meta.platformHandle != 0;
            }
        }
        return false;
    }

    uint32_t RenderSystem::GetTextureHandle(const std::string& textureId) const {
        auto it = m_textureCache.find(textureId);
        if (it != m_textureCache.end()) return it->second;
        if (m_platformDelegates.asset.getTextureMetadata) {
            GameCore::TextureMetadata meta;
            if (m_platformDelegates.asset.getTextureMetadata(textureId.c_str(), &meta) && meta.isLoaded && meta.platformHandle != 0) {
                return static_cast<uint32_t>(meta.platformHandle);
            }
        }
        return 0;
    }

    bool RenderSystem::GetTextureSize(const std::string& textureId, int& outWidth, int& outHeight) const {
        auto it = m_textureDimensions.find(textureId);
        if (it != m_textureDimensions.end()) {
            outWidth = it->second.first;
            outHeight = it->second.second;
            return true;
        }
        if (m_platformDelegates.asset.getTextureMetadata) {
            GameCore::TextureMetadata meta;
            if (m_platformDelegates.asset.getTextureMetadata(textureId.c_str(), &meta) && meta.width > 0 && meta.height > 0) {
                outWidth = meta.width;
                outHeight = meta.height;
                return true;
            }
        }
        return false;
    }

} // namespace GameCore
