#include "LoadingState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include "../Components/GameComponents.h"
#include <iostream>
#include <cmath>

namespace GameCore {

    // Static constants
    const float LoadingState::LOADING_DURATION = 3.0f;  // 3 seconds
    const float LoadingState::ROTATION_SPEED = 360.0f;  // degrees per second
    const float LoadingState::ORBIT_RADIUS = 50.0f;     // pixels from center

    LoadingState::LoadingState(Gnosis::ECS* ecsCoordinator)
        : m_ecsCoordinator(ecsCoordinator)
        , m_finished(false)
        , m_loadingTimer(0.0f)
        , m_rotationAngle(0.0f)
        , m_poopHatEntity(nullptr) {
    }

    void LoadingState::Enter() {
        GN_LOG_INFO("Entering Loading State");
        m_finished = false;
        m_loadingTimer = 0.0f;
        m_rotationAngle = 0.0f;
        
        // Create the rotating poop hat loading icon
        CreateLoadingEntities();
    }

    void LoadingState::Exit() {
        GN_LOG_INFO("Exiting Loading State");
        DestroyLoadingEntities();
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
        // This will be handled by the RenderSystem
        // For now, we'll just ensure the ECS renders our entities
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Render();
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
        
        // Add Transform component (position will be updated in UpdatePoopHatPosition)
        Transform transform;
        transform.position = Gnosis::GNVector2(400.0f, 300.0f); // Screen center (placeholder)
        transform.scale = Gnosis::GNVector2(1.0f, 1.0f);
        m_ecsCoordinator->AddComponent<Transform>(poopHatEntityId, transform);
        
        // Add Sprite component (static sprite for now)
        Sprite sprite("poophat.png", 64.0f, 64.0f);
        sprite.layer = 10; // High layer for UI elements
        m_ecsCoordinator->AddComponent<Sprite>(poopHatEntityId, sprite);
        
        // Store entity ID (convert to void* for compatibility)
        m_poopHatEntity = reinterpret_cast<void*>(static_cast<uintptr_t>(poopHatEntityId));
        
        GN_LOG_INFO("Created poop hat entity with ID: %u", poopHatEntityId);
    }

    void LoadingState::DestroyLoadingEntities() {
        GN_LOG_INFO("Destroying loading screen entities");
        
        if (m_poopHatEntity && m_ecsCoordinator) {
            Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
            
            if (m_ecsCoordinator->IsEntityValid(poopHatEntityId)) {
                m_ecsCoordinator->DestroyEntity(poopHatEntityId);
                GN_LOG_INFO("Destroyed poop hat entity with ID: %u", poopHatEntityId);
            }
            
            m_poopHatEntity = nullptr;
        }
    }

    void LoadingState::UpdatePoopHatPosition() {
        if (!m_poopHatEntity || !m_ecsCoordinator) {
            return;
        }
        
        // Calculate orbital position
        float radians = m_rotationAngle * (3.14159f / 180.0f);
        
        // TODO: Get actual screen center coordinates from renderer
        float centerX = 400.0f; // Placeholder - should get from renderer
        float centerY = 300.0f; // Placeholder - should get from renderer
        
        // Calculate new position
        float poopHatX = centerX + cos(radians) * ORBIT_RADIUS;
        float poopHatY = centerY + sin(radians) * ORBIT_RADIUS;
        
        // Update the entity's Transform component
        Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
        
        if (m_ecsCoordinator->IsEntityValid(poopHatEntityId)) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(poopHatEntityId);
            if (transform) {
                transform->position.x = poopHatX;
                transform->position.y = poopHatY;
                transform->rotation = m_rotationAngle; // Rotate the sprite itself too
            }
        }
        
        // Log position for debugging (remove in final version)
        if (static_cast<int>(m_rotationAngle) % 90 == 0) {
            GN_LOG_DEBUG("Poop hat position: (%.1f, %.1f) at angle %.1f°", 
                        poopHatX, poopHatY, m_rotationAngle);
        }
    }

    float LoadingState::GetLoadingProgress() const {
        return std::min(m_loadingTimer / LOADING_DURATION, 1.0f);
    }

} // namespace GameCore