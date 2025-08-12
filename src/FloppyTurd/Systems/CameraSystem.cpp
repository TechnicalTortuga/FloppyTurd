#include "CameraSystem.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
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
        
        // Camera stays stationary at (0,0) for side-scrolling games
        // Only track world scroll distance, don't move the camera
        Camera* camera = m_ecsSystem->GetComponent<Camera>(m_mainCamera);
        if (camera) {
            // Keep camera at origin - only objects move, not the camera
            camera->position.x = 0.0f;
            camera->position.y = 0.0f;
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
            
            // Skip UI elements (they should stay fixed in screen space)
            if (m_ecsSystem->HasComponent<UIElement>(entity)) {
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
        // Update parallax background layers - only works with ParallaxInstance component
        auto parallaxEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite, Parallax, ParallaxInstance>();
        
        static float debugTimer = 0.0f;
        debugTimer += deltaTime;
        bool shouldLog = debugTimer >= 1.0f; // Log every 1 second for better debugging
        if (shouldLog) {
            debugTimer = 0.0f;
            GN_LOG_INFO("=== PARALLAX DEBUG: Found " + std::to_string(parallaxEntities.size()) + " parallax entities ===");
            
            // Also check if we have any entities with just Transform, Sprite, Parallax
            auto basicParallaxEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite, Parallax>();
            GN_LOG_INFO("=== BASIC PARALLAX: Found " + std::to_string(basicParallaxEntities.size()) + " basic parallax entities ===");
        }
        
        for (Gnosis::Entity entity : parallaxEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);
            auto parallax = m_ecsSystem->GetComponent<Parallax>(entity);
            auto instance = m_ecsSystem->GetComponent<ParallaxInstance>(entity);
            
            if (transform && sprite && parallax && instance && parallax->autoScroll) {
                if (shouldLog) {
                    GN_LOG_INFO("Entity " + std::to_string(entity) + " texture '" + sprite->textureId + 
                               "' instance " + std::to_string(instance->instanceIndex) + "/" + std::to_string(instance->totalInstances) +
                               " at x=" + std::to_string(transform->position.x) + 
                               " y=" + std::to_string(transform->position.y) +
                               " scale=" + std::to_string(transform->scale.x) +
                               " textureWidth=" + std::to_string(instance->textureWidth) +
                               " scrollSpeed=" + std::to_string(parallax->scrollSpeed) +
                               " autoScroll=" + (parallax->autoScroll ? "true" : "false") +
                               " visible=" + (sprite->visible ? "true" : "false"));
                }
                
                // Move background based on its scroll speed
                float oldX = transform->position.x;
                transform->position.x -= parallax->scrollSpeed * deltaTime;
                
                if (shouldLog) {
                    GN_LOG_INFO("Entity " + std::to_string(entity) + " moved from x=" + std::to_string(oldX) + 
                               " to x=" + std::to_string(transform->position.x) + 
                               " (delta=" + std::to_string(parallax->scrollSpeed * deltaTime) + ")");
                }
                
                // Seamless wrapping logic for continuous scrolling
                if (instance->textureWidth > 0.0f) {
                    // Calculate the total width of all instances for this layer
                    float totalLayerWidth = instance->textureWidth * instance->totalInstances;
                    
                    // When an instance moves completely off-screen to the left,
                    // wrap it around to the right side for seamless scrolling
                    if (transform->position.x <= -instance->textureWidth) {
                        // Move it to the right edge of all instances
                        transform->position.x += totalLayerWidth;

                        // If this entity supports background variants, swap to a random one on wrap
                        ParallaxVariants* variants = m_ecsSystem->GetComponent<ParallaxVariants>(entity);
                        if (variants && !variants->textureIds.empty()) {
                            const std::string previousId = sprite->textureId;
                            const int idx = rand() % variants->textureIds.size();
                            sprite->textureId = variants->textureIds[idx];
                            if (shouldLog) {
                                GN_LOG_INFO("CameraSystem: Parallax variant swap '" + previousId + "' -> '" + sprite->textureId + "'");
                            }
                        }

                        if (shouldLog) {
                            GN_LOG_INFO("CameraSystem: Wrapped parallax layer '" + sprite->textureId + 
                                       "' instance " + std::to_string(instance->instanceIndex) +
                                       " to x=" + std::to_string(transform->position.x) + 
                                       " (textureWidth=" + std::to_string(instance->textureWidth) + 
                                       ", totalLayerWidth=" + std::to_string(totalLayerWidth) + ")");
                        }
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
