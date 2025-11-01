#include "RenderSystem.h"
#include "OverlaySystem.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace GameCore {

    RenderSystem::RenderSystem(Gnosis::ECS* ecsSystem, const PlatformDelegates& platformDelegates)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_activeCamera(0)
        , m_useRenderLayers(true)
        , m_screenInfoValid(false)
        , m_currentLevelId(1)  // Default to level 1
        , m_renderCacheDirty(true)
        , m_cachedEntityCount(0)
        , m_renderDebugLogging(false)
        , m_cachedTransformVersion(0)
        , m_cachedSpriteVersion(0)
        , m_cachedTextVersion(0)
        , m_cachedUIElementVersion(0)
        , m_cachedDebugDrawVersion(0)
        , m_cachedUIShapeVersion(0)
        , m_frameProfiler("RenderSystem")
        , m_profilingEnabled(false)
    {
        GN_LOG_INFO("RenderSystem initialized with unified rendering");
        m_renderQueue.reserve(1000); // Pre-allocate for performance

        // Initialize screen info
        UpdateScreenInfo();
        SetupLayout();
        
        // Enable profiling by default to investigate performance issues
        SetProfilingEnabled(true);
    }



    RenderSystem::~RenderSystem() {
        GN_LOG_INFO("RenderSystem destroyed");
    }
    
    void RenderSystem::SetProfilingEnabled(bool enabled) {
        m_profilingEnabled = enabled;
        m_frameProfiler.SetEnabled(enabled);
        GN_LOG_INFO("RenderSystem profiling " + std::string(enabled ? "enabled" : "disabled"));
    }

    void RenderSystem::Render() {
        m_frameProfiler.BeginFrame();
        
        if (!m_ecsSystem) {
            m_frameProfiler.EndFrame();
            return;
        }

        // Track FPS using system time
        m_frameProfiler.StartSection("FPSTracking");
        static auto lastTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        m_frameTimeAccum += deltaTime;
        m_frameCount++;
        
        // Update FPS display every 10 frames for stability
        static float displayedFrameTime = 0.016f;
        if (m_frameCount >= 10) {
            displayedFrameTime = m_frameTimeAccum / 10.0f;
            m_currentFPS = 1.0f / displayedFrameTime;
            m_frameTimeAccum = 0.0f;
            m_frameCount = 0;
        }
        m_frameProfiler.EndSection("FPSTracking");

        // Clear render queue
        m_frameProfiler.StartSection("ClearQueue");
        m_renderQueue.clear();
        m_frameProfiler.EndSection("ClearQueue");
        
        // Collect all renderable items
        m_frameProfiler.StartSection("CollectRenderItems");
        CollectRenderItems();
        m_frameProfiler.EndSection("CollectRenderItems");
        
        // Sort by layer and depth
        m_frameProfiler.StartSection("SortRenderQueue");
        SortRenderQueue();
        m_frameProfiler.EndSection("SortRenderQueue");
        
        // Render world space items (backgrounds, game objects, player)
        m_frameProfiler.StartSection("RenderWorldSpace");
        RenderWorldSpace();
        m_frameProfiler.EndSection("RenderWorldSpace");
        
        // Render overlays (snowfall, etc.) between world and UI
        m_frameProfiler.StartSection("RenderOverlays");
        RenderOverlays();
        m_frameProfiler.EndSection("RenderOverlays");
        
        // Render screen space items (UI)
        m_frameProfiler.StartSection("RenderScreenSpace");
        RenderScreenSpace();
        m_frameProfiler.EndSection("RenderScreenSpace");
        
        // Render FPS counter on top of everything
        m_frameProfiler.StartSection("DrawFPS");
        if (m_showFPS && m_platformDelegates.renderer.drawText) {
            char fpsText[64];
            snprintf(fpsText, sizeof(fpsText), "FPS: %.1f (%.1fms)", m_currentFPS, displayedFrameTime * 1000.0f);
            
            // Color code based on FPS
            uint8_t r, g, b;
            if (m_currentFPS >= 55.0f) {
                r = 0; g = 255; b = 0; // Green
            } else if (m_currentFPS >= 45.0f) {
                r = 255; g = 255; b = 0; // Yellow
            } else if (m_currentFPS >= 30.0f) {
                r = 255; g = 165; b = 0; // Orange
            } else {
                r = 255; g = 0; b = 0; // Red
            }
            
            m_platformDelegates.renderer.drawText(fpsText, 10.0f, 40.0f, 24.0f, r, g, b, 255);
        }
        m_frameProfiler.EndSection("DrawFPS");
        
        m_frameProfiler.EndFrame();
    }

    void RenderSystem::SetActiveCamera(Gnosis::Entity cameraEntity) {
        m_activeCamera = cameraEntity;
        GN_LOG_INFO("Active camera set to entity: " + std::to_string(cameraEntity));
    }

    void RenderSystem::CollectRenderItems() {
        if (!m_ecsSystem) {
            return;
        }

        size_t transformVersion = m_ecsSystem->GetComponentVersion<Transform>();
        size_t spriteVersion = m_ecsSystem->GetComponentVersion<Sprite>();
        size_t textVersion = m_ecsSystem->GetComponentVersion<Text>();
        size_t uiElementVersion = m_ecsSystem->GetComponentVersion<UIElement>();
        size_t debugDrawVersion = m_ecsSystem->GetComponentVersion<DebugDraw>();
        size_t uiShapeVersion = m_ecsSystem->GetComponentVersion<UIShape>();

        bool versionChanged =
            transformVersion != m_cachedTransformVersion ||
            spriteVersion != m_cachedSpriteVersion ||
            textVersion != m_cachedTextVersion ||
            uiElementVersion != m_cachedUIElementVersion ||
            debugDrawVersion != m_cachedDebugDrawVersion ||
            uiShapeVersion != m_cachedUIShapeVersion;

        if (versionChanged) {
            m_renderCacheDirty = true;
        }

        size_t totalEntities = m_ecsSystem->GetEntityCount();
        if (totalEntities != m_cachedEntityCount) {
            m_renderCacheDirty = true;
        }

        if (m_renderCacheDirty) {
            RebuildRenderCaches();
            m_renderCacheDirty = false;

            m_cachedTransformVersion = transformVersion;
            m_cachedSpriteVersion = spriteVersion;
            m_cachedTextVersion = textVersion;
            m_cachedUIElementVersion = uiElementVersion;
            m_cachedDebugDrawVersion = debugDrawVersion;
            m_cachedUIShapeVersion = uiShapeVersion;
            m_cachedEntityCount = totalEntities;
        }

        // PERFORMANCE: Fast world-space camera bounds for early rejection
        Gnosis::GNVector2 cameraPos(0.0f, 0.0f);
        if (m_activeCamera != 0) {
            if (auto* camTransform = m_ecsSystem->GetComponent<Transform>(m_activeCamera)) {
                cameraPos = camTransform->position;
            }
        }
        
        float screenWidth = m_screenInfoValid ? m_screenInfo.pixelWidth : 1179.0f;
        float screenHeight = m_screenInfoValid ? m_screenInfo.pixelHeight : 2556.0f;
        float cameraScale = GetCameraScale();
        
        // Calculate world-space visible area (camera's view in world coordinates)
        float worldScreenWidth = screenWidth / cameraScale;
        float worldScreenHeight = screenHeight / cameraScale;
        
        // ADAPTIVE CULLING: Margins scale with screen size to prevent pop-in/pop-out
        // Industry standard: 1-2x screen dimension for smooth culling in scrolling games
        float horizontalMargin = worldScreenWidth * 0.5f;  // 50% of screen width on each side
        float verticalMargin = worldScreenHeight * 0.3f;    // 30% of screen height (less vertical scrolling)
        
        float worldViewLeft = cameraPos.x - horizontalMargin;
        float worldViewRight = cameraPos.x + worldScreenWidth + horizontalMargin;
        float worldViewTop = cameraPos.y - verticalMargin;
        float worldViewBottom = cameraPos.y + worldScreenHeight + verticalMargin;

        // World-space sprites
        for (Gnosis::Entity entity : m_cachedSpriteEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);
            if (!transform || !sprite || !sprite->visible) {
                continue;
            }

            // Invalidate cached handle if texture changed
            if (sprite->textureHandleValid && sprite->cachedTextureId != sprite->textureId) {
                sprite->textureHandleValid = false;
                sprite->cachedTextureHandle = 0;
            }

            if (auto pickup = m_ecsSystem->GetComponent<Pickup>(entity); pickup && !pickup->isActive) {
                continue;
            }

            // SMART CULLING: Special handling for different sprite types
            // Background layers (0-1) and UI layers (100+) should never be culled
            bool isBackground = (sprite->layer <= 1);
            bool isUI = (sprite->layer >= 100);
            bool neverCull = isBackground || isUI;
            
            if (!neverCull) {
                // PERFORMANCE: Fast world-space AABB culling (avoids expensive WorldToScreen transform)
                // Calculate sprite bounds in world space using CENTER-based positioning
                float spriteWorldWidth = sprite->frameWidth * transform->scale.x;
                float spriteWorldHeight = sprite->frameHeight * transform->scale.y;
                
                // Sprite bounds (assuming top-left origin, which is standard for 2D engines)
                float spriteWorldLeft = transform->position.x;
                float spriteWorldRight = transform->position.x + spriteWorldWidth;
                float spriteWorldTop = transform->position.y;
                float spriteWorldBottom = transform->position.y + spriteWorldHeight;
                
                // Early reject ONLY if sprite is completely outside the extended view frustum
                // This AABB test is conservative - if ANY part overlaps, we render it
                if (spriteWorldRight < worldViewLeft ||
                    spriteWorldLeft > worldViewRight ||
                    spriteWorldBottom < worldViewTop ||
                    spriteWorldTop > worldViewBottom) {
                    continue;  // Sprite is completely outside view - safe to cull
                }
            }

            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = sprite;
            item.text = nullptr;
            item.shape = nullptr;
            item.layer = sprite->layer;
            item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;

            if (m_renderDebugLogging) {
                if (auto enemy = m_ecsSystem->GetComponent<Enemy>(entity)) {
                    GN_LOG_DEBUG("RenderSystem: Enqueue enemy entity " + std::to_string(entity) +
                                 " at (" + std::to_string(transform->position.x) + ", " +
                                 std::to_string(transform->position.y) + ") texture='" + sprite->textureId + "'");
                }
            }

            m_renderQueue.push_back(item);
        }

        // Screen-space text elements (typically very few, so fast culling is sufficient)
        for (Gnosis::Entity entity : m_cachedTextEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto text = m_ecsSystem->GetComponent<Text>(entity);
            if (!transform || !text || !text->visible) {
                continue;
            }

            // PERFORMANCE: Simple world-space X culling (text is typically UI, doesn't need precise culling)
            if (transform->position.x > worldViewRight) {
                continue;
            }

            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = nullptr;
            item.text = text;
            item.shape = nullptr;
            item.layer = text->layer;
            item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;
            m_renderQueue.push_back(item);
        }

        // UI elements (screen space)
        for (Gnosis::Entity entity : m_cachedUIEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto uiElement = m_ecsSystem->GetComponent<UIElement>(entity);
            if (!transform || !uiElement || !uiElement->visible) {
                continue;
            }

            RenderItem item;
            item.entity = entity;
            item.transform = transform;
            item.sprite = m_ecsSystem->GetComponent<Sprite>(entity);
            item.text = nullptr;
            item.shape = nullptr;
            item.layer = uiElement->textLayer;
            item.depth = static_cast<float>(item.layer) * 1000.0f + transform->position.y;
            m_renderQueue.push_back(item);
        }

        // Debug overlays
        for (Gnosis::Entity entity : m_cachedDebugEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto debugDraw = m_ecsSystem->GetComponent<DebugDraw>(entity);
            if (!transform || !debugDraw) {
                continue;
            }

            // PERFORMANCE: Fast world-space culling for debug overlays
            float debugCullMargin = 1500.0f;  // Very generous margin for debug
            if (transform->position.x > worldViewRight + debugCullMargin) {
                continue;
            }

            auto hitbox = m_ecsSystem->GetComponent<Hitbox>(entity);
            if (!hitbox) {
                continue;
            }

            if (debugDraw->showBounds) {
                RenderItem boundsItem;
                boundsItem.entity = entity;
                boundsItem.transform = transform;
                boundsItem.sprite = m_ecsSystem->GetComponent<Sprite>(entity);
                boundsItem.text = nullptr;
                boundsItem.shape = nullptr;
                boundsItem.layer = debugDraw->debugLayer;
                boundsItem.depth = static_cast<float>(boundsItem.layer) * 1000.0f + transform->position.y;
                boundsItem.isDebugBounds = true;
                boundsItem.debugColor = debugDraw->boundsColor;
                boundsItem.debugAlpha = debugDraw->alpha;
                boundsItem.debugWidth = hitbox->width;
                boundsItem.debugHeight = hitbox->height;
                boundsItem.debugOffsetX = hitbox->offsetX;
                boundsItem.debugOffsetY = hitbox->offsetY;
                m_renderQueue.push_back(boundsItem);
            }

            if (debugDraw->showCollider) {
                RenderItem colliderItem;
                colliderItem.entity = entity;
                colliderItem.transform = transform;
                colliderItem.sprite = m_ecsSystem->GetComponent<Sprite>(entity);
                colliderItem.text = nullptr;
                colliderItem.shape = nullptr;
                colliderItem.layer = debugDraw->debugLayer;
                colliderItem.depth = static_cast<float>(colliderItem.layer) * 1000.0f + transform->position.y;
                colliderItem.isDebugCollider = true;
                colliderItem.debugColor = debugDraw->colliderColor;
                colliderItem.debugAlpha = debugDraw->alpha;
                colliderItem.debugOffsetX = hitbox->offsetX;
                colliderItem.debugOffsetY = hitbox->offsetY;
                if (hitbox->type == GameCore::ColliderType::Circle) {
                    colliderItem.debugIsCircle = true;
                    colliderItem.debugRadius = hitbox->radius;
                } else {
                    colliderItem.debugWidth = hitbox->width;
                    colliderItem.debugHeight = hitbox->height;
                }
                if (auto obstacle = m_ecsSystem->GetComponent<Obstacle>(entity); obstacle && obstacle->type == ObstacleType::SpikeBall) {
                    colliderItem.debugAbsolutePos = true;
                }
                m_renderQueue.push_back(colliderItem);
            }
        }

        // UI shapes (screen space primitives)
        for (Gnosis::Entity entity : m_cachedShapeEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto shape = m_ecsSystem->GetComponent<UIShape>(entity);
            if (!transform || !shape || !shape->visible) {
                continue;
            }

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

    void RenderSystem::MarkRenderCacheDirty() {
        m_renderCacheDirty = true;
    }

    void RenderSystem::RebuildRenderCaches() {
        if (!m_ecsSystem) {
            m_cachedSpriteEntities.clear();
            m_cachedTextEntities.clear();
            m_cachedUIEntities.clear();
            m_cachedDebugEntities.clear();
            m_cachedShapeEntities.clear();
            return;
        }

        m_cachedSpriteEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite>();
        m_cachedTextEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Text>();
        m_cachedUIEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, UIElement>();
        m_cachedDebugEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, DebugDraw>();
        m_cachedShapeEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, UIShape>();

        if (m_renderDebugLogging) {
            GN_LOG_DEBUG("RenderSystem: Cache rebuild sprites=" + std::to_string(m_cachedSpriteEntities.size()) +
                         " text=" + std::to_string(m_cachedTextEntities.size()) +
                         " ui=" + std::to_string(m_cachedUIEntities.size()) +
                         " debug=" + std::to_string(m_cachedDebugEntities.size()) +
                         " shapes=" + std::to_string(m_cachedShapeEntities.size()));
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
        // IMPORTANT: m_renderQueue is already sorted by layer (line 390-396)
        // We must preserve layer order while batching!
        
        // Clear and reuse member variables to avoid per-frame allocations
        // This prevents 60 heap allocations per second of complex nested containers
        for (auto& [layer, batches] : m_layeredBatches) {
            for (auto& [handle, items] : batches) {
                items.clear(); // Clear vectors but keep capacity
            }
        }
        for (auto& [layer, items] : m_layeredNonBatchable) {
            items.clear(); // Clear vectors but keep capacity
        }
        // Note: we don't clear the maps themselves to preserve allocations
        
        // Collect and group world-space items, preserving layer order
        for (const RenderItem& item : m_renderQueue) {
            // Skip UI elements (they're rendered in screen space)
            if (m_ecsSystem->HasComponent<UIElement>(item.entity)) {
                continue;
            }
            
            // Group sprites by texture handle for batching
            if (item.sprite && !item.isDebugBounds && !item.isDebugCollider) {
                // Check if sprite uses special rendering that breaks batching
                bool usesCenteredRendering = m_ecsSystem->HasComponent<RotationRenderer>(item.entity);
                bool usesPivotRotation = m_ecsSystem->HasComponent<PivotRotationRenderer>(item.entity);
                
                // Only special rendering modes need individual draws
                // Animated sprites CAN be batched - GPU instancing handles per-sprite UVs
                if (usesCenteredRendering || usesPivotRotation) {
                    // IMPORTANT: Still need to ensure texture is loaded for non-batchable sprites!
                    if (!item.sprite->textureHandleValid || item.sprite->cachedTextureHandle == 0) {
                        uint32_t textureHandle = GetOrLoadTexture(item.sprite->textureId, item.entity);
                        if (textureHandle != 0) {
                            item.sprite->cachedTextureHandle = textureHandle;
                            item.sprite->textureHandleValid = true;
                            item.sprite->cachedTextureId = item.sprite->textureId;
                        }
                    }
                    
                    // Render individually - these need special rendering
                    int layer = item.sprite->layer;
                    m_layeredNonBatchable[layer].push_back(&item);
                    continue;
                }
                
                // CRITICAL: Ensure texture is loaded and cached BEFORE batching
                uint32_t textureHandle = 0;
                if (item.sprite->textureHandleValid && item.sprite->cachedTextureHandle != 0) {
                    // Already cached
                    textureHandle = item.sprite->cachedTextureHandle;
                } else {
                    // Load and cache texture NOW (not during rendering)
                    textureHandle = GetOrLoadTexture(item.sprite->textureId, item.entity);
                    if (textureHandle != 0) {
                        item.sprite->cachedTextureHandle = textureHandle;
                        item.sprite->textureHandleValid = true;
                        item.sprite->cachedTextureId = item.sprite->textureId;
                    }
                }
                
                if (textureHandle != 0) {
                    // Sprite has valid texture - add to layer-specific batch
                    int layer = item.sprite->layer;
                    size_t batchSizeBefore = m_layeredBatches[layer][textureHandle].size();
                    m_layeredBatches[layer][textureHandle].push_back(&item);
                    
                    // DEBUG: Log every sprite addition to see batching in action
                    static int logCounter = 0;
                    if (++logCounter % 120 == 0) {
                        GN_LOG_INFO("🔧 Adding sprite '" + item.sprite->textureId + 
                                   "' handle=" + std::to_string(textureHandle) + 
                                   " layer=" + std::to_string(layer) +
                                   " batchSize=" + std::to_string(batchSizeBefore) + "→" + std::to_string(batchSizeBefore + 1));
                    }
                } else {
                    // Failed to load texture - skip this sprite
                    GN_LOG_WARN("RenderSystem: Failed to load texture '" + item.sprite->textureId + "' for batching - skipping sprite");
                }
            }
            // Debug items rendered individually
            else if (item.isDebugBounds || item.isDebugCollider) {
                int layer = item.layer;
                m_layeredNonBatchable[layer].push_back(&item);
            }
        }
        
        // Render batches BY LAYER (preserves correct draw order!)
        if (m_platformDelegates.renderer.drawSpriteBatch) {
            // DEBUG: Batch summary logs ENABLED for debugging
            static int logFrameCounter = 0;
            bool shouldLog = (++logFrameCounter % 60 == 0); // Log every 60 frames
            
            // Build complete layer set (union of both batches and non-batchable)
            // Clear and reuse member variable to avoid per-frame allocation
            m_currentFrameLayers.clear();
            for (const auto& [layer, _] : m_layeredBatches) {
                m_currentFrameLayers.insert(layer);
            }
            for (const auto& [layer, _] : m_layeredNonBatchable) {
                m_currentFrameLayers.insert(layer);
            }
            
            // Render each layer in order
            for (int layer : m_currentFrameLayers) {
                // Render batches for this layer (if any)
                auto batchIt = m_layeredBatches.find(layer);
                if (batchIt != m_layeredBatches.end()) {
                    if (shouldLog) {
                        GN_LOG_INFO("  Layer " + std::to_string(layer) + ": " + std::to_string(batchIt->second.size()) + " batches");
                    }
                    
                    for (const auto& [textureHandle, items] : batchIt->second) {
                        if (!items.empty()) {
                            if (shouldLog) {
                                std::string textureId = items[0]->sprite ? items[0]->sprite->textureId : "unknown";
                                GN_LOG_INFO("    Batch: '" + textureId + "' (handle=" + std::to_string(textureHandle) + 
                                           "), " + std::to_string(items.size()) + " sprites");
                            }
                            RenderSpriteBatch(textureHandle, items);
                        }
                    }
                }
                
                // Render non-batchable items for this layer (if any)
                auto nonBatchIt = m_layeredNonBatchable.find(layer);
                if (nonBatchIt != m_layeredNonBatchable.end()) {
                    // Render without logging (logs disabled for performance)
                    for (const RenderItem* item : nonBatchIt->second) {
                        RenderSingleItem(*item);
                    }
                }
            }
        }
        // Fallback: Render individually if batching not available
        else {
            for (const auto& [layer, batches] : m_layeredBatches) {
                for (const auto& [textureHandle, items] : batches) {
                    for (const RenderItem* item : items) {
                        RenderSingleItem(*item);
                    }
                }
                
                auto nonBatchIt = m_layeredNonBatchable.find(layer);
                if (nonBatchIt != m_layeredNonBatchable.end()) {
                    for (const RenderItem* item : nonBatchIt->second) {
                        RenderSingleItem(*item);
                    }
                }
            }
        }
    }

    void RenderSystem::RenderOverlays() {
        if (!m_overlaySystem || !m_overlaySystem->IsSnowfallEnabled()) return;
        if (!m_platformDelegates.renderer.drawSpriteScaledWithSource) return;
        
        std::string textureName = m_overlaySystem->GetSnowfallTextureName();
        uint32_t textureHandle = GetOrLoadTexture(textureName, 0);
        if (textureHandle == 0) return;
        
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float tileSize = m_overlaySystem->GetSnowfallFrameSize() * m_overlaySystem->GetSnowfallScale();
        
        int tilesX = static_cast<int>(std::ceil(screenInfo.pixelWidth / tileSize)) + 1;
        int tilesY = static_cast<int>(std::ceil(screenInfo.pixelHeight / tileSize)) + 1;
        
        int currentFrame = m_overlaySystem->GetSnowfallCurrentFrame();
        float sourceX = currentFrame * m_overlaySystem->GetSnowfallFrameSize();
        
        for (int row = 0; row < tilesY; ++row) {
            for (int col = 0; col < tilesX; ++col) {
                m_platformDelegates.renderer.drawSpriteScaledWithSource(
                    textureHandle, col * tileSize, row * tileSize,
                    m_overlaySystem->GetSnowfallScale(), m_overlaySystem->GetSnowfallScale(),
                    0.0f, sourceX, 0.0f,
                    m_overlaySystem->GetSnowfallFrameSize(), m_overlaySystem->GetSnowfallFrameSize()
                );
            }
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
                    // UI should not be affected by camera zoom; use transform scale only
                    float scale = item.transform->scale.x;
                    
                    // Load texture from UIElement component (not Sprite component)
                    uint32_t textureHandle = GetOrLoadTexture(textureId, item.entity);
                    if (textureHandle != 0) {
                        // Use sprite dimensions if available, otherwise use default button texture size
                        float width = 90.0f;  // Actual button texture width (90x16 as shown in asset)
                        float height = 16.0f; // Actual button texture height
                        if (item.sprite) {
                            width = item.sprite->width;
                            height = item.sprite->height;
                        }
                        
                        // Draw the texture using the regular drawing function
                        m_platformDelegates.renderer.drawSpriteScaled(
                            textureHandle,
                            screenPos.x,
                            screenPos.y,
                            scale,  // Use scale factor, not pixel dimensions
                            scale,  // Use scale factor, not pixel dimensions
                            item.transform->rotation
                        );
                    }
                }
            }
            
            // Render UIShape components in screen space (for UI rectangles, tracks, circles, etc.)
            if (item.shape) {
                // Convert color from 0-255 to 0.0-1.0 range
                const float r = item.shape->color.r / 255.0f;
                const float g = item.shape->color.g / 255.0f;
                const float b = item.shape->color.b / 255.0f;
                const float a = item.shape->color.a / 255.0f;

                if (item.shape->type == UIShapeType::Circle && m_platformDelegates.renderer.drawCircle) {
                    // Render circle outline in screen coordinates
                    m_platformDelegates.renderer.drawCircle(
                        item.transform->position.x,
                        item.transform->position.y,
                        item.shape->radius * item.transform->scale.x,
                        r, g, b, a
                    );
                } else if (item.shape->type == UIShapeType::FilledCircle && m_platformDelegates.renderer.drawFilledCircle) {
                    // Render filled circle in screen coordinates
                    m_platformDelegates.renderer.drawFilledCircle(
                        item.transform->position.x,
                        item.transform->position.y,
                        item.shape->radius * item.transform->scale.x,
                        r, g, b, a
                    );
                } else if ((item.shape->type == UIShapeType::Rectangle || item.shape->type == UIShapeType::Line) && 
                           m_platformDelegates.renderer.drawRectangle) {
                    // Render rectangle or line in screen coordinates (no camera transformation)
                    m_platformDelegates.renderer.drawRectangle(
                        item.transform->position.x,
                        item.transform->position.y,
                        item.shape->width * item.transform->scale.x,
                        item.shape->height * item.transform->scale.y,
                        r, g, b, a
                    );
                }
            }
            
            // Render Text components (UI text like scores, pipe counter) - check this FIRST
            if (item.text && m_platformDelegates.renderer.drawText) {
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

                // Skip invisible text
                if (a < 0.01f) {
                    continue;
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
            const float r = item.shape->color.r / 255.0f;
            const float g = item.shape->color.g / 255.0f;
            const float b = item.shape->color.b / 255.0f;
            const float a = item.shape->color.a / 255.0f;

            if (item.shape->type == UIShapeType::Circle && m_platformDelegates.renderer.drawCircle) {
                // Render circle outline in screen coordinates
                m_platformDelegates.renderer.drawCircle(
                    item.transform->position.x,
                    item.transform->position.y,
                    item.shape->radius * item.transform->scale.x,
                    r, g, b, a
                );
            } else if (item.shape->type == UIShapeType::FilledCircle && m_platformDelegates.renderer.drawFilledCircle) {
                // Render filled circle in screen coordinates
                m_platformDelegates.renderer.drawFilledCircle(
                    item.transform->position.x,
                    item.transform->position.y,
                    item.shape->radius * item.transform->scale.x,
                    r, g, b, a
                );
            } else if ((item.shape->type == UIShapeType::Rectangle || item.shape->type == UIShapeType::Line) && 
                       m_platformDelegates.renderer.drawRectangle) {
                // Render rectangle or line in screen coordinates
                m_platformDelegates.renderer.drawRectangle(
                    item.transform->position.x,
                    item.transform->position.y,
                    item.shape->width * item.transform->scale.x,
                    item.shape->height * item.transform->scale.y,
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
            
            // Resolve or reuse cached texture handle
            uint32_t textureHandle = 0;
            if (item.sprite->textureHandleValid && item.sprite->cachedTextureHandle != 0) {
                textureHandle = item.sprite->cachedTextureHandle;
            } else {
                textureHandle = GetOrLoadTexture(item.sprite->textureId, item.entity);
                if (textureHandle != 0) {
                    item.sprite->cachedTextureHandle = textureHandle;
                    item.sprite->textureHandleValid = true;
                    item.sprite->cachedTextureId = item.sprite->textureId;
                }
            }
            if (textureHandle == 0) {
                GN_LOG_WARN("RenderSystem: Invalid/zero texture handle for id '" + item.sprite->textureId + "' — skipping sprite draw");
                // Debug: Log enemy texture failures
                if (auto enemy = m_ecsSystem->GetComponent<Enemy>(item.entity)) {
                    GN_LOG_ERROR("RenderSystem: Failed to load texture '" + item.sprite->textureId + "' for enemy " + std::to_string(item.entity));
                }
            } else {
                if (m_renderDebugLogging) {
                    if (auto enemy = m_ecsSystem->GetComponent<Enemy>(item.entity)) {
                        GN_LOG_DEBUG("RenderSystem: Using texture '" + item.sprite->textureId + "' (handle=" + std::to_string(textureHandle) + ") for enemy " + std::to_string(item.entity));
                    }
                }
                // Compute final scale based on sprite frame vs logical size
                bool usesCenteredRendering = m_ecsSystem->HasComponent<RotationRenderer>(item.entity);
                bool usesPivotRotation = m_ecsSystem->HasComponent<PivotRotationRenderer>(item.entity);

                // Performance: Disabled per-frame rotation component logging
                // if (usesCenteredRendering) {
                //     GN_LOG_DEBUG("RenderSystem: Entity " + std::to_string(item.entity) + " has RotationRenderer (centered) - rotation: " +
                //                std::to_string(item.transform->rotation) + "°");
                // }
                if (usesPivotRotation) {
                    PivotRotationRenderer* pivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(item.entity);
                    if (pivotRenderer) {
                        // Performance: Disabled per-frame pivot rotation logging
                        // GN_LOG_DEBUG("RenderSystem: Entity " + std::to_string(item.entity) + " has PivotRotationRenderer with pivot (" +
                        //            std::to_string(pivotRenderer->pivotX) + ", " + std::to_string(pivotRenderer->pivotY) +
                        //            "), speed: " + std::to_string(pivotRenderer->rotationSpeed) +
                        //            "), manual: " + std::to_string(pivotRenderer->manualControl) +
                        //            ", rotation: " + std::to_string(item.transform->rotation) + "°");
                    }
                }
                

                float scaleX = item.sprite->width / item.sprite->frameWidth;
                float scaleY = item.sprite->height / item.sprite->frameHeight;
                float finalScaleX = scaleX * item.transform->scale.x * GetCameraScale();
                float finalScaleY = scaleY * item.transform->scale.y * GetCameraScale();

                float srcX = 0.0f;
                float srcY = 0.0f;
                float srcW = item.sprite->frameWidth;
                float srcH = item.sprite->frameHeight;

                if (item.sprite->sourceWidth > 0.0f) {
                    srcX = item.sprite->sourceX;
                    srcY = item.sprite->sourceY;
                    srcW = item.sprite->sourceWidth;
                    srcH = item.sprite->sourceHeight;
                } else if (item.sprite->isAnimated) {
                    int currentFrame = item.sprite->currentFrame % item.sprite->frameCount;
                    srcX = static_cast<float>(currentFrame * item.sprite->frameWidth);
                    srcY = 0.0f; // Assume horizontal sheet
                }

                // Check if we need to render a sub-rect (animated OR spritesheet with multiple frames)
                bool needsSourceRect = item.sprite->isAnimated;
                if (!needsSourceRect) {
                    // Even if not animated, check if texture is a spritesheet by comparing dimensions
                    int textureWidth, textureHeight;
                    if (GetCachedTextureInfo(item.sprite->textureId, textureWidth, textureHeight)) {
                        // If texture width is larger than frame width, it's likely a spritesheet
                        needsSourceRect = (textureWidth > item.sprite->frameWidth);
                    }
                }

                // If we need source rect rendering, check rotation components and use appropriate command
                if (needsSourceRect && m_platformDelegates.renderer.drawSpriteScaledWithSource) {
                    int safeFrameCount = item.sprite->frameCount > 0 ? item.sprite->frameCount : 1;
                    int currentFrame = item.sprite->currentFrame % safeFrameCount;
                    
                    // CRITICAL FIX: Use ACTUAL texture dimensions for proper frame calculation
                    int textureWidth = static_cast<int>(item.sprite->width);
                    int textureHeight = static_cast<int>(item.sprite->height);
                    auto dimIt = m_textureDimensions.find(item.sprite->textureId);
                    if (dimIt != m_textureDimensions.end()) {
                        textureWidth = dimIt->second.first;
                        textureHeight = dimIt->second.second;
                    }
                    
                    // Calculate frame position based on texture layout (horizontal or vertical)
                    int framesPerRow = textureWidth / item.sprite->frameWidth;
                    if (framesPerRow <= 0) framesPerRow = 1;
                    
                    int frameX = (currentFrame % framesPerRow) * static_cast<int>(item.sprite->frameWidth);
                    int frameY = (currentFrame / framesPerRow) * static_cast<int>(item.sprite->frameHeight);

                    // Debug: Log enemy drawing attempts with source rect
                    if (auto enemy = m_ecsSystem->GetComponent<Enemy>(item.entity)) {
                        GN_LOG_INFO("RenderSystem: DRAWING enemy (WITH SOURCE) " + std::to_string(item.entity) + 
                                   " at screen pos (" + std::to_string(screenPos.x) + "," + std::to_string(screenPos.y) + 
                                   ") scale (" + std::to_string(finalScaleX) + "," + std::to_string(finalScaleY) + 
                                   ") texture '" + item.sprite->textureId + "' handle=" + std::to_string(textureHandle) +
                                   " frame=" + std::to_string(currentFrame) + "/" + std::to_string(safeFrameCount) +
                                   " tex=" + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) +
                                   " rect=(" + std::to_string(frameX) + "," + std::to_string(frameY) + ")");
                    }

                    // Priority: Check rotation components first for animated sprites
                    // 1. Pivot rotation (highest priority - boss arms, rotating obstacles)
                    if (usesPivotRotation && m_platformDelegates.renderer.drawSpriteScaledWithSourcePivoted) {
                        PivotRotationRenderer* pivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(item.entity);
                        if (pivotRenderer) {
                            GN_LOG_DEBUG("RenderSystem: Using drawSpriteScaledWithSourcePivoted for animated entity " + std::to_string(item.entity));
                            m_platformDelegates.renderer.drawSpriteScaledWithSourcePivoted(
                                textureHandle,
                                screenPos.x,
                                screenPos.y,
                                finalScaleX,
                                finalScaleY,
                                item.transform->rotation,
                                pivotRenderer->pivotX,
                                pivotRenderer->pivotY,
                                srcX,
                                srcY,
                                srcW,
                                srcH
                            );
                        }
                    }
                    // 2. Centered rotation (medium priority - rotating animated sprites)
                    else if (usesCenteredRendering && m_platformDelegates.renderer.drawSpriteScaledWithSourceCentered) {
                        GN_LOG_DEBUG("RenderSystem: Using drawSpriteScaledWithSourceCentered for animated entity " + std::to_string(item.entity));
                        m_platformDelegates.renderer.drawSpriteScaledWithSourceCentered(
                            textureHandle,
                            screenPos.x,
                            screenPos.y,
                            finalScaleX,
                            finalScaleY,
                            item.transform->rotation,
                            srcX,
                            srcY,
                            srcW,
                            srcH
                        );
                    }
                    // 3. Basic rotation (default - top-left origin)
                    else {
                        m_platformDelegates.renderer.drawSpriteScaledWithSource(
                            textureHandle,
                            screenPos.x,
                            screenPos.y,
                            finalScaleX,
                            finalScaleY,
                            item.transform->rotation,
                            srcX,
                            srcY,
                            srcW,
                            srcH
                        );
                    }
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
                    GN_LOG_DEBUG("RenderSystem: Using drawSpriteScaledPivoted for entity " + std::to_string(item.entity));
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
                    GN_LOG_DEBUG("RenderSystem: Using drawSpriteScaledCentered (fallback) for entity " + std::to_string(item.entity));
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
                    GN_LOG_WARN("RenderSystem: Using basic drawSpriteScaled (no rotation) for entity " + std::to_string(item.entity) + " - rotation: " + std::to_string(item.transform->rotation) + "°");

                    // Debug: Log enemy drawing attempts
                    if (auto enemy = m_ecsSystem->GetComponent<Enemy>(item.entity)) {
                        GN_LOG_INFO("RenderSystem: DRAWING enemy " + std::to_string(item.entity) + " at screen pos (" +
                                   std::to_string(screenPos.x) + "," + std::to_string(screenPos.y) + ") scale (" +
                                   std::to_string(finalScaleX) + "," + std::to_string(finalScaleY) + ") texture '" +
                                   item.sprite->textureId + "' handle=" + std::to_string(textureHandle));
                    }

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
        // Use ConfigManager as primary source for screen info (set directly from Swift at startup)
        const ScreenInfo& configScreenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        
        // Check if ConfigManager has valid screen info
        if (configScreenInfo.pixelWidth > 0 && configScreenInfo.pixelHeight > 0) {
            m_screenInfo = configScreenInfo;
            m_screenInfoValid = true;
            GN_LOG_INFO("Screen info updated from ConfigManager: " +
                        std::to_string((int)m_screenInfo.logicalWidth) + "x" +
                        std::to_string((int)m_screenInfo.logicalHeight) +
                        " (" + std::to_string((int)m_screenInfo.pixelWidth) + "x" +
                        std::to_string((int)m_screenInfo.pixelHeight) + " pixels)");

            // Update ECS with screen dimensions so all systems can access them
            if (m_ecsSystem) {
                // Check if current level requires landscape mode
                bool forceLandscape = false;
                if (m_currentLevelId == 6) { // Boss level
                    forceLandscape = true;
                }

                // For landscape mode, swap dimensions to match rotated screen
                float effectivePixelWidth = m_screenInfo.pixelWidth;
                float effectivePixelHeight = m_screenInfo.pixelHeight;
                float effectiveLogicalWidth = m_screenInfo.logicalWidth;
                float effectiveLogicalHeight = m_screenInfo.logicalHeight;

                if (forceLandscape && m_screenInfo.pixelWidth < m_screenInfo.pixelHeight) {
                    // Device is in portrait but level requires landscape - swap dimensions
                    effectivePixelWidth = m_screenInfo.pixelHeight;
                    effectivePixelHeight = m_screenInfo.pixelWidth;
                    effectiveLogicalWidth = m_screenInfo.logicalHeight;
                    effectiveLogicalHeight = m_screenInfo.logicalWidth;
                }

                m_ecsSystem->SetScreenDimensions(effectivePixelWidth, effectivePixelHeight,
                                               effectiveLogicalWidth, effectiveLogicalHeight);
            }
            return;
        }

        // Fallback to delegates only if ConfigManager doesn't have valid info
        if (m_platformDelegates.renderer.getScreenInfo) {
            m_platformDelegates.renderer.getScreenInfo(&m_screenInfo);
            m_screenInfoValid = true;
            GN_LOG_INFO("Screen info updated from delegates: " +
                        std::to_string((int)m_screenInfo.logicalWidth) + "x" +
                        std::to_string((int)m_screenInfo.logicalHeight) +
                        " (" + std::to_string((int)m_screenInfo.pixelWidth) + "x" +
                        std::to_string((int)m_screenInfo.pixelHeight) + " pixels)");

            // Update ECS with screen dimensions so all systems can access them
            if (m_ecsSystem) {
                // Check if current level requires landscape mode
                bool forceLandscape = false;
                if (m_currentLevelId == 6) { // Boss level
                    forceLandscape = true;
                }

                // For landscape mode, swap dimensions to match rotated screen
                float effectivePixelWidth = m_screenInfo.pixelWidth;
                float effectivePixelHeight = m_screenInfo.pixelHeight;
                float effectiveLogicalWidth = m_screenInfo.logicalWidth;
                float effectiveLogicalHeight = m_screenInfo.logicalHeight;

                if (forceLandscape && m_screenInfo.pixelWidth < m_screenInfo.pixelHeight) {
                    // Device is in portrait but level requires landscape - swap dimensions
                    effectivePixelWidth = m_screenInfo.pixelHeight;
                    effectivePixelHeight = m_screenInfo.pixelWidth;
                    effectiveLogicalWidth = m_screenInfo.logicalHeight;
                    effectiveLogicalHeight = m_screenInfo.logicalWidth;
                }

                m_ecsSystem->SetScreenDimensions(effectivePixelWidth, effectivePixelHeight,
                                               effectiveLogicalWidth, effectiveLogicalHeight);
            }
            return;
        }
        
        // Legacy fallback
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
            if (entity != 0 && m_ecsSystem && m_ecsSystem->IsEntityValid(entity)) {
                if (auto sprite = m_ecsSystem->GetComponent<Sprite>(entity)) {
                    sprite->cachedTextureHandle = it->second;
                    sprite->textureHandleValid = true;
                    sprite->cachedTextureId = textureId;
                }
            }
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

            // 🎯 NEW: Update our synchronous metadata cache
            system->UpdateCacheFromAsyncResult(context->textureId, handle, textureData->width, textureData->height);

            // Populate sprite cache if entity still exists
            if (context->entity != 0 && system->m_ecsSystem && system->m_ecsSystem->IsEntityValid(context->entity)) {
                if (auto sprite = system->m_ecsSystem->GetComponent<Sprite>(context->entity)) {
                    sprite->cachedTextureHandle = handle;
                    sprite->textureHandleValid = true;
                    sprite->cachedTextureId = context->textureId;
                }
            }

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

    void RenderSystem::RenderSpriteBatch(uint32_t textureHandle, const std::vector<const RenderItem*>& items) {
        if (items.empty()) return;
        
        // Build batch data from render items
        std::vector<GameCore::SpriteBatchData> batchData;
        batchData.reserve(items.size());
        
        for (const RenderItem* item : items) {
            GameCore::SpriteBatchData data;
            data.textureHandle = textureHandle;
            
            // World to screen transform
            Gnosis::GNVector2 screenPos = WorldToScreen(item->transform->position);
            data.x = screenPos.x;
            data.y = screenPos.y;
            
            // Scale: Send the MULTIPLIER, not final pixel size
            // MetalRenderer will multiply this by texture dimensions
            // Formula matches RenderSingleItem's finalScale calculation
            float cameraScale = GetCameraScale();
            float scaleX = item->sprite->width / static_cast<float>(item->sprite->frameWidth);
            float scaleY = item->sprite->height / static_cast<float>(item->sprite->frameHeight);
            data.scaleX = scaleX * item->transform->scale.x * cameraScale;
            data.scaleY = scaleY * item->transform->scale.y * cameraScale;
            
            // Rotation
            data.rotation = item->transform->rotation;
            
            // Source rect (for sprite sheets/animations)
            if (item->sprite->isAnimated && item->sprite->frameCount > 1) {
                // CRITICAL FIX: Use ACTUAL texture dimensions from cache, not sprite->width/height
                // sprite->width/height may be set to frame size instead of texture size
                int textureWidth = static_cast<int>(item->sprite->width);
                int textureHeight = static_cast<int>(item->sprite->height);
                
                // Try to get actual texture dimensions from cache
                auto dimIt = m_textureDimensions.find(item->sprite->textureId);
                if (dimIt != m_textureDimensions.end()) {
                    textureWidth = dimIt->second.first;
                    textureHeight = dimIt->second.second;
                }
                
                // Calculate source rect from current frame using ACTUAL texture dimensions
                int framesPerRow = textureWidth / item->sprite->frameWidth;
                if (framesPerRow > 0) {
                    int frameX = (item->sprite->currentFrame % framesPerRow) * item->sprite->frameWidth;
                    int frameY = (item->sprite->currentFrame / framesPerRow) * item->sprite->frameHeight;
                    data.sourceX = static_cast<float>(frameX);
                    data.sourceY = static_cast<float>(frameY);
                    data.sourceWidth = static_cast<float>(item->sprite->frameWidth);
                    data.sourceHeight = static_cast<float>(item->sprite->frameHeight);
                    
                    // DEBUG: Log animation frame data periodically
                    static int animLogCounter = 0;
                    if (++animLogCounter % 120 == 0) {
                        GN_LOG_INFO("🎬 ANIM: '" + item->sprite->textureId + 
                                   "' frame=" + std::to_string(item->sprite->currentFrame) + 
                                   "/" + std::to_string(item->sprite->frameCount) +
                                   " tex=" + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) +
                                   " sourceRect=(" + std::to_string(static_cast<int>(data.sourceX)) + "," + 
                                   std::to_string(static_cast<int>(data.sourceY)) + "," +
                                   std::to_string(static_cast<int>(data.sourceWidth)) + "," +
                                   std::to_string(static_cast<int>(data.sourceHeight)) + ")");
                    }
                } else {
                    // Full texture if calculation fails
                    data.sourceX = 0;
                    data.sourceY = 0;
                    data.sourceWidth = 0;  // 0 means use full texture
                    data.sourceHeight = 0;
                    GN_LOG_WARN("RenderSystem: Animation sprite '" + item->sprite->textureId + 
                               "' has invalid framesPerRow (texWidth=" + std::to_string(textureWidth) + 
                               " frameWidth=" + std::to_string(item->sprite->frameWidth) + ")");
                }
            } else {
                // Use full texture for non-animated sprites
                data.sourceX = 0;
                data.sourceY = 0;
                data.sourceWidth = 0;  // 0 means use full texture
                data.sourceHeight = 0;
            }
            
            batchData.push_back(data);
        }
        
        // Single draw call for entire batch!
        m_platformDelegates.renderer.drawSpriteBatch(batchData);
    }

    // 🎯 NEW: Synchronous texture metadata cache implementation
    bool RenderSystem::GetCachedTextureInfo(const std::string& textureId, int& width, int& height) {
        auto it = m_textureMetadataCache.find(textureId);
        if (it != m_textureMetadataCache.end() && it->second.isLoaded) {
            width = it->second.width;
            height = it->second.height;
            GN_LOG_INFO("📊 Cache hit: " + textureId + " = " + std::to_string(width) + "x" + std::to_string(height));
            return true;
        }

        GN_LOG_DEBUG("🔍 Cache miss for: " + textureId);
        return false;
    }

    void RenderSystem::UpdateCacheFromAsyncResult(const std::string& textureId, uint32_t handle, int width, int height) {
        CachedTextureInfo info;
        info.handle = handle;
        info.width = width;
        info.height = height;
        info.isLoaded = (width > 0 && height > 0);
        info.assetPath = textureId;

        m_textureMetadataCache[textureId] = info;

        GN_LOG_INFO("✅ Cached texture metadata: " + textureId + " = " +
                   std::to_string(width) + "x" + std::to_string(height) +
                   " (handle: " + std::to_string(handle) + ")");
    }

} // namespace GameCore
