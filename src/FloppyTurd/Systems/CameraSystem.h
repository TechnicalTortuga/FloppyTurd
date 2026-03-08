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
        void ResetForNewGame();

        // Getters
        Gnosis::Entity GetMainCamera() const { return m_mainCamera; }
        Gnosis::GNVector2 GetCameraPosition() const;
        float GetWorldPosition() const { return m_worldPosition; }
        float GetWorldScrollSpeed() const { return m_worldScrollSpeed; }
        void SetWorldScrollSpeed(float speed) { 
            m_worldScrollSpeed = speed; 
            m_worldScrollSpeedFixed = FloatToFixed(speed);  // Update fixed-point version
        }

    private:
        Gnosis::ECS* m_ecsSystem;
        Gnosis::Entity m_mainCamera;
        
        // World scrolling settings
        float m_worldScrollSpeed;
        float m_worldPosition;  // Legacy - kept for compatibility
        
        // Fixed-point arithmetic system for sub-pixel perfect scrolling
        int64_t m_worldPositionFixed;     // World position in fixed-point (16.16 format)
        int64_t m_worldScrollSpeedFixed;  // Scroll speed in fixed-point (16.16 format)
        
        // Constants
        static constexpr float DEFAULT_SCROLL_SPEED = 200.0f;
        static constexpr int32_t FIXED_POINT_SCALE = 65536;  // 2^16 for 16.16 fixed-point
        
        // Fixed-point arithmetic helpers
        static int64_t FloatToFixed(float f) { return static_cast<int64_t>(f * FIXED_POINT_SCALE); }
        static float FixedToFloat(int64_t fixed) { return static_cast<float>(fixed) / FIXED_POINT_SCALE; }
        static int32_t FixedToInt(int64_t fixed) { return static_cast<int32_t>(fixed >> 16); }
    };

} // namespace GameCore

#endif // FLOPPY_TURD_CAMERA_SYSTEM_H
