#include "LoadingState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include <iostream>
#include <cmath>

namespace GameCore {

    // Static constants
    const float LoadingState::LOADING_DURATION = 3.0f;  // 3 seconds
    const float LoadingState::ROTATION_SPEED = 360.0f;  // degrees per second
    const float LoadingState::ORBIT_RADIUS = 50.0f;     // pixels from center

    LoadingState::LoadingState(Gnosis::ECS* ecsCoordinator)
        : m_ecsCoordinator(ecsCoordinator)
        , m_poopHatEntity(nullptr)
        , m_loadingTextEntity(nullptr) {
    }

    void LoadingState::Enter() {
        GN_LOG_INFO("Entering Loading State");
        m_finished = false;
        m_loadingTimer = 0.0f;
        m_rotationAngle = 0.0f;
        
        // Create the rotating poop hat loading icon
        CreateLoadingEntities();
        
        // Create loading text
        CreateLoadingText();

        // Preload essential assets during loading screen
        extern GameCore::FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.asset.preloadEssentialAssets) {
                delegates.asset.preloadEssentialAssets();
                GN_LOG_INFO("Initiated asset preloading during loading screen");
            }
        }
    }

    void LoadingState::Exit() {
        GN_LOG_INFO("Exiting Loading State");
        // Clean up debug loading entities - we don't want them in main menu anymore
        DestroyLoadingEntities();
        GN_LOG_INFO("Cleaned up loading entities for cleaner main menu");
    }

    void LoadingState::Pause() {
        // Loading state doesn't need to handle pause
    }

    void LoadingState::Resume() {
        // Loading state doesn't need to handle resume
    }

    void LoadingState::Update(float deltaTime) {
        // Update loading timer
        m_loadingTimer += deltaTime;
        
        // Update rotation angle for the poop hat
        m_rotationAngle += ROTATION_SPEED * deltaTime;
        if (m_rotationAngle >= 360.0f) {
            m_rotationAngle -= 360.0f;
        }
        
        // Update poop hat position (circular orbit around center)
        UpdatePoopHatPosition();
        
        // Check if loading is complete
        if (m_loadingTimer >= LOADING_DURATION) {
            m_finished = true;
        }
        
        // Update ECS systems
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Update(deltaTime);
        }
    }

    void LoadingState::Render() {
        // Draw debug rectangle at poophat's position and size
        if (m_poopHatEntity && m_ecsCoordinator) {
            Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(poopHatEntityId);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(poopHatEntityId);
            if (transform && sprite) {
                float x = transform->position.x - (sprite->width * transform->scale.x) / 2.0f;
                float y = transform->position.y - (sprite->height * transform->scale.y) / 2.0f;
                float width = sprite->width * transform->scale.x;
                float height = sprite->height * transform->scale.y;
                
                GN_LOG_DEBUG("[LoadingState] Drawing debug rectangle at (" + std::to_string(x) + ", " + std::to_string(y) + 
                           ") size (" + std::to_string(width) + "x" + std::to_string(height) + 
                           ") for sprite '" + sprite->textureId + "'");
                
                // REMOVED: Debug rectangle - coordinates are confirmed working
                // extern GameCore::FloppyTurdGame* g_Game;
                // if (g_Game && g_Game->GetPlatformDelegates().renderer.drawRectangle) {
                //     g_Game->GetPlatformDelegates().renderer.drawRectangle(x, y, width, height, 1.0f, 0.0f, 1.0f, 0.5f);
                // }
                GN_LOG_DEBUG("[LoadingState] Drawing debug rectangle at (" + std::to_string(x) + ", " + std::to_string(y) + 
                           ") size (" + std::to_string(width) + "x" + std::to_string(height) + ")");
            } else {
                GN_LOG_WARN("[LoadingState] Transform or Sprite component missing for poophat entity");
            }
        } else {
            GN_LOG_WARN("[LoadingState] Poophat entity or ECS coordinator is null");
        }
        
        // Render ECS entities as usual
        if (m_ecsCoordinator) {
            GN_LOG_DEBUG("[LoadingState] Calling ECS Render()");
            m_ecsCoordinator->Render();
        } else {
            GN_LOG_WARN("[LoadingState] ECS coordinator is null during render");
        }
    }

    void LoadingState::HandleInput() {
        // Loading state doesn't handle input
        // Could add skip functionality later if desired
    }

    void LoadingState::CreateLoadingEntities() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in LoadingState");
            return;
        }
        
        GN_LOG_INFO("Creating loading screen entities");
        
        // Create rotating poop hat entity
        Gnosis::Entity poopHatEntityId = m_ecsCoordinator->CreateEntity();
        
        GN_LOG_INFO("Created entity with ID: " + std::to_string(poopHatEntityId));
        
        GN_LOG_INFO("[LoadingState] m_ecsCoordinator ptr: " + std::to_string(reinterpret_cast<uintptr_t>(m_ecsCoordinator)));
        GN_LOG_INFO("[LoadingState] IsEntityValid(" + std::to_string(poopHatEntityId) + "): " + (m_ecsCoordinator && m_ecsCoordinator->IsEntityValid(poopHatEntityId) ? "true" : "false"));
        // Add Transform component (position will be updated in UpdatePoopHatPosition)
        GN_LOG_INFO("[LoadingState] About to add Transform component to entity " + std::to_string(poopHatEntityId));
        Transform transform;
        transform.position = Gnosis::GNVector2(1179.0f / 2.0f, 2556.0f / 2.0f); // Screen center
        transform.scale = Gnosis::GNVector2(1.0f, 1.0f);
        m_ecsCoordinator->AddComponent<Transform>(poopHatEntityId, transform);
        GN_LOG_INFO("[LoadingState] Added Transform component to entity " + std::to_string(poopHatEntityId));
        
        GN_LOG_INFO("Added Transform component to entity " + std::to_string(poopHatEntityId) + " at position (" + 
                   std::to_string(transform.position.x) + ", " + std::to_string(transform.position.y) + ")");
        
        // Add Sprite component using just the asset name for iOS asset catalog
        GN_LOG_INFO("[LoadingState] About to add Sprite component to entity " + std::to_string(poopHatEntityId));
        // Try using a different texture to test if the issue is with the poophat texture specifically
        Sprite sprite("poophat", 256.0f, 256.0f); // Much bigger size!
        sprite.frameWidth = 16;  // Set correct frame dimensions for scaling
        sprite.frameHeight = 16; // Set correct frame dimensions for scaling
        sprite.layer = 10; // High layer for UI elements
        sprite.visible = true; // Ensure sprite is visible
        m_ecsCoordinator->AddComponent<Sprite>(poopHatEntityId, sprite);
        GN_LOG_INFO("[LoadingState] Added Sprite component to entity " + std::to_string(poopHatEntityId));
        
        GN_LOG_INFO("Added Sprite component to entity " + std::to_string(poopHatEntityId) + " with texture '" + 
                   sprite.textureId + "' size (" + std::to_string(sprite.width) + "x" + std::to_string(sprite.height) + 
                   ") frame (" + std::to_string(sprite.frameWidth) + "x" + std::to_string(sprite.frameHeight) + 
                   ") layer " + std::to_string(sprite.layer) + " visible " + (sprite.visible ? "true" : "false"));
        
        // Store entity ID (convert to void* for compatibility)
        m_poopHatEntity = reinterpret_cast<void*>(static_cast<uintptr_t>(poopHatEntityId));
        
        GN_LOG_INFO("Created poop hat entity with ID: " + std::to_string(poopHatEntityId));
    }

    void LoadingState::DestroyLoadingEntities() {
        GN_LOG_INFO("Destroying loading screen entities");
        
        // Destroy poop hat entity
        if (m_poopHatEntity && m_ecsCoordinator) {
            Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
            
            if (m_ecsCoordinator->IsEntityValid(poopHatEntityId)) {
                m_ecsCoordinator->DestroyEntity(poopHatEntityId);
                GN_LOG_INFO("Destroyed poop hat entity with ID: " + std::to_string(poopHatEntityId));
            }
            
            m_poopHatEntity = nullptr;
        }
        
        // Destroy loading text entity
        if (m_loadingTextEntity && m_ecsCoordinator) {
            Gnosis::Entity loadingTextEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_loadingTextEntity));
            
            if (m_ecsCoordinator->IsEntityValid(loadingTextEntityId)) {
                m_ecsCoordinator->DestroyEntity(loadingTextEntityId);
                GN_LOG_INFO("Destroyed loading text entity with ID: " + std::to_string(loadingTextEntityId));
            }
            
            m_loadingTextEntity = nullptr;
        }
    }

    void LoadingState::UpdatePoopHatPosition() {
        if (!m_poopHatEntity || !m_ecsCoordinator) {
            return;
        }
        
        // TEMPORARILY DISABLED: Calculate orbital position
        // float radians = m_rotationAngle * (3.14159f / 180.0f);
        
        // Get actual screen center coordinates from viewport
        float centerX = 1179.0f / 2.0f; // Viewport width / 2
        float centerY = 2556.0f / 2.0f; // Viewport height / 2
        
        // TEMPORARILY CENTERED: Calculate new position
        // float poopHatX = centerX + cos(radians) * ORBIT_RADIUS;
        // float poopHatY = centerY + sin(radians) * ORBIT_RADIUS;
        float poopHatX = centerX;
        float poopHatY = centerY;
        
        // Update the entity's Transform component
        Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
        
        if (m_ecsCoordinator->IsEntityValid(poopHatEntityId)) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(poopHatEntityId);
            if (transform) {
                transform->position.x = poopHatX;
                transform->position.y = poopHatY;
                transform->rotation = m_rotationAngle; // Apply rotation
            }
        }
        
        // Log position for debugging (remove in final version)
        if (static_cast<int>(m_rotationAngle) % 90 == 0) {
            GN_LOG_DEBUG("Poop hat position: (" + std::to_string(poopHatX) + ", " + std::to_string(poopHatY) + 
                        ") at angle " + std::to_string(m_rotationAngle) + "°");
        }
    }

    float LoadingState::GetLoadingProgress() const {
        return std::min(m_loadingTimer / LOADING_DURATION, 1.0f);
    }

    void LoadingState::CreateLoadingText() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in LoadingState");
            return;
        }
        
        GN_LOG_INFO("Creating loading text entity");
        
        // Create loading text entity
        Gnosis::Entity loadingTextEntityId = m_ecsCoordinator->CreateEntity();
        
        // Add Transform component
        Transform transform;
        transform.position = Gnosis::GNVector2(1179.0f / 2.0f, 2556.0f / 2.0f + 200.0f); // Below the poop hat
        transform.scale = Gnosis::GNVector2(1.0f, 1.0f);
        m_ecsCoordinator->AddComponent<Transform>(loadingTextEntityId, transform);
        
        // Add UIElement component for text rendering
        UIElement loadingText;
        loadingText.buttonText = "LOADING...";
        loadingText.fontSize = 160.0f; // Larger font size for mobile readability
        loadingText.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        loadingText.textLayer = 15; // Higher layer than poop hat
        m_ecsCoordinator->AddComponent<UIElement>(loadingTextEntityId, loadingText);
        
        // Store entity ID
        m_loadingTextEntity = reinterpret_cast<void*>(static_cast<uintptr_t>(loadingTextEntityId));
        
        GN_LOG_INFO("Created loading text entity with ID: " + std::to_string(loadingTextEntityId));
    }

} // namespace GameCore