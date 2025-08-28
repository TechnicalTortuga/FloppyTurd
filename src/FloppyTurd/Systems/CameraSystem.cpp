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

    void CameraSystem::ResetForNewGame() {
        m_worldPosition = 0.0f;
        GN_LOG_INFO("CameraSystem reset for new game - world position reset to 0");
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
                // If entity has ScrollSpeed, use that absolute speed. Otherwise use world speed.
                float speed = m_worldScrollSpeed;
                if (auto scr = m_ecsSystem->GetComponent<ScrollSpeed>(entity)) {
                    speed = scr->speed;
                }
                transform->position.x -= speed * deltaTime;
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

        // SYNCHRONIZED BACKGROUND MOVEMENT BY LAYER
        // Group backgrounds by render layer and move all in same layer together
        // This prevents gaps and ensures perfect alignment within each layer
        std::map<int, std::vector<Gnosis::Entity>> layerEntities;
        std::map<int, float> layerMovementDeltas;

        // First pass: group entities by render layer and calculate movement deltas
        for (Gnosis::Entity entity : parallaxEntities) {
            auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);
            auto parallax = m_ecsSystem->GetComponent<Parallax>(entity);

            if (sprite && parallax) {
                int renderLayer = sprite->layer;
                layerEntities[renderLayer].push_back(entity);

                // Calculate movement delta for this layer (all entities in same layer use same speed)
                if (layerMovementDeltas.find(renderLayer) == layerMovementDeltas.end()) {
                    layerMovementDeltas[renderLayer] = parallax->scrollSpeed * deltaTime;
                }
            }
        }

        // Second pass: apply synchronized movement to all backgrounds in each layer
        for (const auto& layerPair : layerEntities) {
            int renderLayer = layerPair.first;
            const auto& entities = layerPair.second;
            float movementDelta = layerMovementDeltas[renderLayer];

            if (!entities.empty()) {
                if (shouldLog) {
                    GN_LOG_INFO("🎨 SYNCHRONIZED LAYER " + std::to_string(renderLayer) +
                               " MOVEMENT: Moving " + std::to_string(entities.size()) +
                               " backgrounds by " + std::to_string(movementDelta) + " pixels");
                }

                for (Gnosis::Entity layerEntity : entities) {
                    auto transform = m_ecsSystem->GetComponent<Transform>(layerEntity);
                    if (transform) {
                        transform->position.x -= movementDelta;
                    }
                }
            }
        }

        // Third pass: handle wrapping for all backgrounds (Sewer and non-Sewer)
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
                
                // Pure integer arithmetic seamless wrapping logic for continuous scrolling
                if (instance->textureWidth > 0.0f) {
                    // Use pure integer arithmetic to eliminate any floating point precision issues
                    int currentPixelX = static_cast<int>(std::round(transform->position.x));
                    int texturePixelWidth = static_cast<int>(std::round(instance->textureWidth));
                    int totalLayerPixelWidth = texturePixelWidth * instance->totalInstances;

                    // Special debug logging for Sewer level
                    bool isSewerLevel = (sprite->textureId.find("Sewer") != std::string::npos);
                    if (isSewerLevel && shouldLog) {
                        GN_LOG_INFO("🚽 SEWER WRAP CHECK: entity=" + std::to_string(entity) +
                                   " texture='" + sprite->textureId + "' currentX=" + std::to_string(currentPixelX) +
                                   " textureWidth=" + std::to_string(texturePixelWidth) +
                                   " totalLayerWidth=" + std::to_string(totalLayerPixelWidth) +
                                   " instance=" + std::to_string(instance->instanceIndex) + "/" + std::to_string(instance->totalInstances));
                    }

                    // When an instance moves completely off-screen to the left,
                    // wrap it around to the right side for seamless scrolling
                    if (currentPixelX <= -texturePixelWidth) {
                        // Calculate the exact pixel position for seamless wrapping
                        // Move to the rightmost position of all instances (exact pixel boundary)
                        int rightmostPixelX = currentPixelX + totalLayerPixelWidth;

                        // Additional safeguard: ensure we're exactly at a texture boundary
                        // This prevents any sub-pixel drift that could cause gaps
                        int boundaryOffset = rightmostPixelX % texturePixelWidth;
                        if (boundaryOffset != 0) {
                            // Snap to the nearest texture boundary
                            rightmostPixelX -= boundaryOffset;
                            if (shouldLog) {
                                GN_LOG_INFO("CameraSystem: Corrected boundary offset " + std::to_string(boundaryOffset) +
                                           " for seamless wrapping");
                            }
                        }

                        // Final safeguard: ensure the position is exactly on a pixel boundary
                        // This eliminates any remaining sub-pixel artifacts
                        transform->position.x = static_cast<float>(rightmostPixelX);
                        transform->position.y = std::round(transform->position.y); // Also snap Y for consistency

                        // If this entity supports background variants, swap to a random one on wrap
                        ParallaxVariants* variants = m_ecsSystem->GetComponent<ParallaxVariants>(entity);
                        if (variants && !variants->textureIds.empty()) {
                            const std::string previousId = sprite->textureId;
                            // Use more deterministic variant selection to avoid gaps
                            static unsigned int variantSeed = 0;
                            int idx = (variantSeed++) % variants->textureIds.size();
                            sprite->textureId = variants->textureIds[idx];

                            // Ensure all sewer variants are the same size (512x512)
                            // If they're not, this could cause wrapping gaps
                            if (shouldLog) {
                                GN_LOG_INFO("CameraSystem: Parallax variant swap '" + previousId + "' -> '" + sprite->textureId +
                                           "' (deterministic index " + std::to_string(idx) + " of " +
                                           std::to_string(variants->textureIds.size()) + ")");
                            }
                        }

                        if (shouldLog) {
                            if (isSewerLevel) {
                                GN_LOG_INFO("🚽 SEWER WRAPPED: '" + sprite->textureId +
                                           "' instance " + std::to_string(instance->instanceIndex) +
                                           " to x=" + std::to_string(transform->position.x) +
                                           " (from " + std::to_string(currentPixelX) + " to " + std::to_string(rightmostPixelX) + ")");
                            } else {
                                GN_LOG_INFO("CameraSystem: Wrapped parallax layer '" + sprite->textureId +
                                           "' instance " + std::to_string(instance->instanceIndex) +
                                           " to x=" + std::to_string(transform->position.x) +
                                           " (textureWidth=" + std::to_string(texturePixelWidth) +
                                           ", totalLayerWidth=" + std::to_string(totalLayerPixelWidth) + ")");
                            }
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
