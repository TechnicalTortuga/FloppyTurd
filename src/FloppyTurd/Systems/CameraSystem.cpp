#include "CameraSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    CameraSystem::CameraSystem(Gnosis::ECS* ecsSystem)
        : m_ecsSystem(ecsSystem)
        , m_mainCamera(0)
        , m_worldScrollSpeed(DEFAULT_SCROLL_SPEED)
        , m_worldPosition(0.0f)
    {
        GN_LOG_INFO("CameraSystem initialized");
    }

    CameraSystem::~CameraSystem() {
        GN_LOG_INFO("CameraSystem destroyed");
    }

    void CameraSystem::Update(float deltaTime) {
        if (!m_ecsSystem || m_mainCamera == 0) {
            return;
        }

        // Update world scrolling (everything except player moves left)
        UpdateWorldScrolling(deltaTime);
        
        // Update parallax background layers
        UpdateParallaxLayers(deltaTime);
    }

    void CameraSystem::SetMainCamera(Gnosis::Entity cameraEntity) {
        m_mainCamera = cameraEntity;
        GN_LOG_INFO("Main camera set to entity: " + std::to_string(cameraEntity));
    }

    void CameraSystem::UpdateWorldScrolling(float deltaTime) {
        // Update world position (this represents how far the world has scrolled)
        m_worldPosition += m_worldScrollSpeed * deltaTime;
        
        // Update camera position
        Camera* camera = m_ecsSystem->GetComponent<Camera>(m_mainCamera);
        if (camera) {
            camera->position.x = m_worldPosition;
        }
        
        // Move all non-player, non-background entities to the left
        auto entitiesWithTransform = m_ecsSystem->GetEntitiesWithComponents<Transform>();
        for (Gnosis::Entity entity : entitiesWithTransform) {
            // Skip the player (which should have a PlayerComponent)
            if (m_ecsSystem->HasComponent<PlayerComponent>(entity)) {
                continue;
            }
            
            // Skip parallax backgrounds (they have their own system)
            if (m_ecsSystem->HasComponent<Parallax>(entity)) {
                continue;
            }
            
            // Skip camera entities
            if (entity == m_mainCamera) {
                continue;
            }
            
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            if (transform) {
                // Move everything else to the left (simulate world movement)
                transform->position.x -= m_worldScrollSpeed * deltaTime;
            }
        }
    }

    void CameraSystem::UpdateParallaxLayers(float deltaTime) {
        // Update parallax background layers
        auto parallaxEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite, Parallax>();
        for (Gnosis::Entity entity : parallaxEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);
            auto parallax = m_ecsSystem->GetComponent<Parallax>(entity);
            
            if (transform && sprite && parallax && parallax->autoScroll) {
                // Move background based on its scroll speed
                transform->position.x -= parallax->scrollSpeed * deltaTime;
                
                // Handle wrapping for seamless scrolling
                if (parallax->repeatWidth > 0.0f) {
                    if (transform->position.x <= -parallax->repeatWidth) {
                        transform->position.x += parallax->repeatWidth * 2.0f;
                    }
                }
            }
        }
    }

    Gnosis::GNVector2 CameraSystem::GetCameraPosition() const {
        if (!m_ecsSystem || m_mainCamera == 0) {
            return Gnosis::GNVector2(0.0f, 0.0f);
        }
        
        Camera* camera = m_ecsSystem->GetComponent<Camera>(m_mainCamera);
        if (camera) {
            return camera->position;
        }
        
        return Gnosis::GNVector2(0.0f, 0.0f);
    }

} // namespace GameCore
