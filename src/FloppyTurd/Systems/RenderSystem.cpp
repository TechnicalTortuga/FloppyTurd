#include "RenderSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    RenderSystem::RenderSystem(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates& platformDelegates)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_activeCamera(0)
        , m_useRenderLayers(true)
    {
        GN_LOG_INFO("RenderSystem initialized with unified rendering");
        m_renderQueue.reserve(1000); // Pre-allocate for performance
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
            item.layer = sprite->layer;
            
            // Calculate depth based on position and layer
            // Higher layers are rendered on top, within layers Y position determines depth
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
        if (!m_platformDelegates.renderer.drawSprite) {
            GN_LOG_WARN("RenderSystem: drawSprite function not available");
            return;
        }
        
        // Render all world space items
        for (const RenderItem& item : m_renderQueue) {
            // Skip UI elements (they're rendered in screen space)
            if (m_ecsSystem->HasComponent<UIElement>(item.entity)) {
                continue;
            }
            
            RenderSingleItem(item);
        }
    }

    void RenderSystem::RenderScreenSpace() {
        if (!m_platformDelegates.renderer.drawSprite) {
            return;
        }
        
        // Render UI elements in screen space (no camera transformation)
        for (const RenderItem& item : m_renderQueue) {
            // Only render UI elements
            if (!m_ecsSystem->HasComponent<UIElement>(item.entity)) {
                continue;
            }
            
            // Render UI directly in screen coordinates
            Gnosis::GNVector2 screenPos = item.transform->position;
            float scale = GetCameraScale() * item.transform->scale.x;
            
            // Convert texture ID string to handle (simplified hash for now)
            uint32_t textureHandle = item.sprite->textureId.empty() ? 0 : 
                static_cast<uint32_t>(std::hash<std::string>{}(item.sprite->textureId));
            
            m_platformDelegates.renderer.drawSpriteScaled(
                textureHandle,
                screenPos.x,
                screenPos.y,
                item.sprite->width * scale / 100.0f, // Normalize scale
                item.sprite->height * scale / 100.0f, // Normalize scale
                item.transform->rotation
            );
        }
    }

    void RenderSystem::RenderSingleItem(const RenderItem& item) {
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
        
        // Render the sprite
        // Convert texture ID string to handle (simplified hash for now)
        uint32_t textureHandle = item.sprite->textureId.empty() ? 0 : 
            static_cast<uint32_t>(std::hash<std::string>{}(item.sprite->textureId));
        
        m_platformDelegates.renderer.drawSpriteScaled(
            textureHandle,
            screenPos.x,
            screenPos.y,
            item.sprite->width * scale / 100.0f, // Normalize scale
            item.sprite->height * scale / 100.0f, // Normalize scale
            item.transform->rotation
        );
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

} // namespace GameCore
