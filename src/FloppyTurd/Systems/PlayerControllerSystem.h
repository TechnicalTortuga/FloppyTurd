#ifndef FLOPPY_TURD_PLAYER_CONTROLLER_SYSTEM_H
#define FLOPPY_TURD_PLAYER_CONTROLLER_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "SpriteSystem.h"
#include <memory>

namespace GameCore {

    /**
     * @brief Player Controller System
     * 
     * Handles player input, movement, jumping, and shooting:
     * - Tap screen to jump
     * - Tap bottom area to shoot
     * - Animation state management
     * - Physics integration
     */
    class PlayerControllerSystem {
    public:
        PlayerControllerSystem(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, SpriteSystem* spriteSystem);
        ~PlayerControllerSystem();

        // Main update method
        void Update(float deltaTime);

        // Input handling
        void HandleTouchInput(float x, float y, bool isPressed);
        void HandleJumpInput();
        void HandleShootInput();

        // Player state management
        void SetPlayerEntity(Gnosis::Entity playerEntity);
        void ChangePlayerAnimation(const std::string& animationName);
        void ResetPlayer();

        // Getters
        Gnosis::Entity GetPlayerEntity() const { return m_playerEntity; }
        bool IsPlayerAlive() const { return m_playerAlive; }

    private:
        Gnosis::ECS* m_ecsSystem;
        GameCore::PlatformDelegates* m_platformDelegates;
        SpriteSystem* m_spriteSystem;
        
        Gnosis::Entity m_playerEntity;
        bool m_playerAlive;
        
        // Input state
        bool m_jumpPressed;
        bool m_shootPressed;
        float m_jumpCooldown;
        float m_shootCooldown;
        
        // Player state
        bool m_isGrounded;
        bool m_isJumping;
        bool m_isShooting;
        float m_jumpTimer;
        float m_shootTimer;
        
        // Animation states
        std::string m_currentAnimation;
        std::string m_idleAnimation;
        std::string m_jumpAnimation;
        std::string m_shootAnimation;
        std::string m_hurtAnimation;
        
        // Constants
        static constexpr float JUMP_FORCE = 400.0f;
        static constexpr float JUMP_COOLDOWN = 0.1f;
        static constexpr float SHOOT_COOLDOWN = 0.3f;
        static constexpr float SHOOT_ANIMATION_DURATION = 0.75f; // 5 frames * 0.15f frameTime
        static constexpr float GROUND_Y = 1278.0f; // Ground level (centered on screen)
        
        // Helper methods
        void UpdatePlayerPhysics(float deltaTime);
        void UpdatePlayerAnimation(float deltaTime);
        void UpdatePlayerState(float deltaTime);
        void HandleCollisions();
        void SpawnProjectile();
        
        // Animation helpers
        void PlayIdleAnimation();
        void PlayJumpAnimation();
        void PlayShootAnimation();
        void PlayHurtAnimation();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_PLAYER_CONTROLLER_SYSTEM_H 