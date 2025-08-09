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
            
            if (!transform || !sprite || !sprite->visible) {
                continue;
            }
            
            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = sprite;
            item.text = nullptr;  // No text for sprite items
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
            
            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = nullptr;  // No sprite for text
            item.text = text;
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
        item.layer = uiElement->textLayer; // Use textLayer for UI elements
        
        // Calculate depth based on position and layer
        item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;
        
        m_renderQueue.push_back(item);
        GN_LOG_INFO("RenderSystem: Added UI entity " + std::to_string(entity) + " to render queue at layer " + std::to_string(uiElement->textLayer));
    }

        // Collect all entities with DebugDraw components for debug overlays
        auto debugEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, DebugDraw>();
        for (Gnosis::Entity entity : debugEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto debugDraw = m_ecsSystem->GetComponent<DebugDraw>(entity);
            
            if (!transform || !debugDraw) {
                continue;
            }
            
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
                    debugItem.layer = debugDraw->debugLayer;  // High priority layer
                    debugItem.depth = static_cast<float>(debugItem.layer) * 1000.0f + transform->position.y;
                    debugItem.isDebugBounds = true;
                    debugItem.debugColor = debugDraw->boundsColor;
                    debugItem.debugAlpha = debugDraw->alpha;
                    debugItem.debugWidth = hitbox->width;
                    debugItem.debugHeight = hitbox->height;
                    debugItem.debugOffsetX = hitbox->offsetX;
                    debugItem.debugOffsetY = hitbox->offsetY;
                    m_renderQueue.push_back(debugItem);
                }

                if (debugDraw->showCollider) {
                    RenderItem debugItem;
                    debugItem.entity = entity;
                    debugItem.transform = transform;
                    // Provide sprite so overlay centering uses sprite half-dimensions
                    debugItem.sprite = m_ecsSystem->GetComponent<Sprite>(entity);
                    debugItem.text = nullptr;
                    debugItem.layer = debugDraw->debugLayer;  // High priority layer
                    debugItem.depth = static_cast<float>(debugItem.layer) * 1000.0f + transform->position.y;
                    debugItem.isDebugCollider = true;
                    debugItem.debugColor = debugDraw->colliderColor;
                    debugItem.debugAlpha = debugDraw->alpha;
                    debugItem.debugOffsetX = hitbox->offsetX;
                    debugItem.debugOffsetY = hitbox->offsetY;
                    // Circle vs rectangle collider visualization
                    if (hitbox->type == GameCore::ColliderType::Circle) {
                        debugItem.debugIsCircle = true;
                        debugItem.debugRadius = hitbox->radius;
                    } else {
                        debugItem.debugIsCircle = false;
                        debugItem.debugWidth = hitbox->width;
                        debugItem.debugHeight = hitbox->height;
                    }
                    m_renderQueue.push_back(debugItem);
                }
            }
        }

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
            item.layer = ui->textLayer; // Use UI text layer for ordering
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
                    // Render UI directly in screen coordinates (raw pixels for Metal renderer)
                    Gnosis::GNVector2 screenPos = item.transform->position;
                    // UI should not be affected by camera zoom; use transform scale only
                    float scale = item.transform->scale.x;
                    
                    // Load texture from UIElement component (not Sprite component)
                    uint32_t textureHandle = GetOrLoadTexture(textureId, item.entity);
                    if (textureHandle == 0) {
                        GN_LOG_WARN("RenderSystem(UI): Invalid/zero texture handle for id '" + textureId + "' — skipping UI sprite draw");
                    } else {
                        // Use sprite dimensions if available, otherwise use actual button texture size
                        float width = 90.0f;  // Actual button texture width (90x16 as shown in asset)
                        float height = 16.0f; // Actual button texture height
                        if (item.sprite) {
                            width = item.sprite->width;
                            height = item.sprite->height;
                        }
                        
                        m_platformDelegates.renderer.drawSpriteScaled(
                            textureHandle,
                            screenPos.x,
                            screenPos.y,
                            scale,  // Use scale factor, not pixel dimensions
                            scale,  // Use scale factor, not pixel dimensions
                            item.transform->rotation
                        );
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
                        textY = item.transform->position.y;
                    } else {
                        const float buttonCenterY = item.transform->position.y + (buttonHeight * 0.5f);
                        const float visualAdjustment = ui->fontSize * 0.15f; // baseline correction
                        textY = buttonCenterY + visualAdjustment;
                    }
                }

                // Apply manual offsets
                textX += ui->textOffsetX;
                textY += ui->textOffsetY;

                // Draw using centered or non-centered path in pixel coordinates
                if (ui->centerTextHorizontally && ui->centerTextVertically && m_platformDelegates.renderer.drawTextCentered) {
                    m_platformDelegates.renderer.drawTextCentered(
                        ui->buttonText, textX, textY, ui->fontSize, r, g, b, a
                    );
                } else if (m_platformDelegates.renderer.drawText) {
                    m_platformDelegates.renderer.drawText(
                        ui->buttonText, textX, textY, ui->fontSize, r, g, b, a
                    );
                } else {
                    GN_LOG_ERROR("RenderSystem: No text drawing delegate available for UIElement text");
                }
            }
        }
    }

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

            // Use CENTER-based offsets to match Hitbox semantics; add sprite half-dimensions if provided
            float spriteHalfW = 0.0f;
            float spriteHalfH = 0.0f;
            if (item.sprite) {
                spriteHalfW = (item.sprite->width * sx) * 0.5f;
                spriteHalfH = (item.sprite->height * sy) * 0.5f;
            }
            float centerX = screenPosTopLeft.x + spriteHalfW + (item.debugOffsetX * sx);
            float centerY = screenPosTopLeft.y + spriteHalfH + (item.debugOffsetY * sy);
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
        if (m_platformDelegates.renderer.getScreenInfo) {
            m_platformDelegates.renderer.getScreenInfo(&m_screenInfo);
            m_screenInfoValid = true;
            
            GN_LOG_INFO("Screen info updated: " + 
                       std::to_string((int)m_screenInfo.logicalWidth) + "x" + 
                       std::to_string((int)m_screenInfo.logicalHeight) + 
                       " (" + std::to_string((int)m_screenInfo.pixelWidth) + "x" + 
                       std::to_string((int)m_screenInfo.pixelHeight) + " pixels)");
        } else {
            // Fallback to legacy screen size if available
            if (m_platformDelegates.renderer.getScreenSize) {
                m_platformDelegates.renderer.getScreenSize(&m_screenInfo.logicalWidth, &m_screenInfo.logicalHeight);
                m_screenInfo.pixelWidth = m_screenInfo.logicalWidth;
                m_screenInfo.pixelHeight = m_screenInfo.logicalHeight;
                m_screenInfo.scaleFactor = 1.0f;
                m_screenInfo.isPortrait = m_screenInfo.logicalHeight > m_screenInfo.logicalWidth;
                m_screenInfo.deviceModel = "Unknown";
                m_screenInfoValid = true;
                
                GN_LOG_WARN("Using legacy screen size: " + 
                           std::to_string((int)m_screenInfo.logicalWidth) + "x" + 
                           std::to_string((int)m_screenInfo.logicalHeight));
            } else {
                GN_LOG_ERROR("No screen size information available from platform delegates");
                m_screenInfoValid = false;
            }
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
            system->m_textureCache[context->textureId] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(textureData->platformTexture));
            GN_LOG_INFO("RenderSystem: Successfully loaded texture '" + context->textureId + "' (" + std::to_string(textureData->width) + "x" + std::to_string(textureData->height) + ")");
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

} // namespace GameCore
