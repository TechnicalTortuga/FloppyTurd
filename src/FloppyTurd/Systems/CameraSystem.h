#ifndef FLOPPY_TURD_CAMERA_SYSTEM_H
#define FLOPPY_TURD_CAMERA_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"

namespace GameCore {

    /**
     * @brief Camera System
     * 
     * Handles camera movement and world scrolling:
     * - Manages camera position and movement
     * - Updates parallax scrolling backgrounds
     * - Handles world-to-screen coordinate transformations
     */
    class CameraSystem {
    public:
        CameraSystem(Gnosis::ECS* ecsSystem);
        ~CameraSystem();

        // Main update method
        void Update(float deltaTime);

        // Camera management
        void SetMainCamera(Gnosis::Entity cameraEntity);
        void UpdateWorldScrolling(float deltaTime);
        void UpdateParallaxLayers(float deltaTime);

        // Getters
        Gnosis::Entity GetMainCamera() const { return m_mainCamera; }
        Gnosis::GNVector2 GetCameraPosition() const;
        float GetWorldPosition() const { return m_worldPosition; }
        float GetWorldScrollSpeed() const { return m_worldScrollSpeed; }
        void SetWorldScrollSpeed(float speed) { m_worldScrollSpeed = speed; }

    private:
        Gnosis::ECS* m_ecsSystem;
        Gnosis::Entity m_mainCamera;
        
        // World scrolling settings
        float m_worldScrollSpeed;
        float m_worldPosition;
        
        // Constants
        static constexpr float DEFAULT_SCROLL_SPEED = 200.0f;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_CAMERA_SYSTEM_H
