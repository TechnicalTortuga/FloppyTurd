#ifndef FLOPPY_TURD_PLAYER_CONTROLLER_SYSTEM_H
#define FLOPPY_TURD_PLAYER_CONTROLLER_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "SpriteSystem.h"
#include <memory>

namespace GameCore {

    /**
     * @brief Player Animation State Machine
     * 
     * Enum for tracking player animation states with immediate transitions
     */
    enum class PlayerAnimationState {
        IDLE,
        JUMPING,
        SHOOTING,
        HURT
    };

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
        void HandleTouchInput(float x, float y, bool isJustPressed);
        void HandleJumpInput();
        void HandleJumpInputWithForce(float force);  // New method for variable jump force
        void HandleJumpRelease();  // New method for variable jump height
        void HandleShootInput();

        // Player state management
        void SetPlayerEntity(Gnosis::Entity playerEntity);
        void ChangePlayerAnimation(const std::string& animationName);
        void TransitionToState(PlayerAnimationState newState);
        void ResetPlayer();

        // Getters
        Gnosis::Entity GetPlayerEntity() const { return m_playerEntity; }
        bool IsPlayerAlive() const { return m_playerAlive; }
        PlayerAnimationState GetCurrentState() const { return m_currentState; }
        
        // Animation control methods (public for external state management)
        void PlayIdleAnimation();
        void PlayJumpAnimation();
        void PlayShootAnimation();
        void PlayHurtAnimation();

    private:
        // Constants
        static constexpr float PLAYER_X_POSITION = 300.0f;  // Configurable player X position for optimal gameplay visibility
        
        // Core systems
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
        
        // Touch state tracking to prevent input cascading
        bool m_touchActive;
        float m_lastTouchX;
        float m_lastTouchY;
        
        // Touch session for frame-independent timing
        struct TouchSession {
            bool active = false;
            uint64_t startTime = 0;
            float startX = 0.0f;
            float startY = 0.0f;
        } m_touchSession;
        
        // Enhanced jump mechanics state
        bool m_jumpButtonHeld;           // Is jump button currently held down
        float m_jumpHoldTime;            // How long has jump been held
        bool m_isAscending;              // Is player currently moving upward
        float m_lastVerticalVelocity;    // Previous frame's Y velocity for direction detection
        
        // Player state
        bool m_isGrounded;
        
        // Animation state machine
        PlayerAnimationState m_currentState;
        PlayerAnimationState m_previousState;
        
        // Animation name mappings
        std::string m_idleAnimation;
        std::string m_jumpAnimation;
        std::string m_shootAnimation;
        std::string m_hurtAnimation;
        
        // Enhanced Jump Physics Constants (adjusted for snappier feel based on user feedback)
        static constexpr float JUMP_FORCE = 2400.0f;         // INCREASED: Stronger jump impulse for more dramatic jumps
        static constexpr float GRAVITY_UP = 2000.0f;          // INCREASED: More gravity while ascending for less floaty feel
        static constexpr float GRAVITY_DOWN = 4800.0f;        // INCREASED: Much heavier gravity while falling for faster drop
        static constexpr float TERMINAL_VELOCITY = 1800.0f;   // INCREASED: Higher maximum falling speed for faster descent
        static constexpr float AUTO_JUMP_THRESHOLD = 0.2f;    // Auto-jump after 200ms (1/5 second) of holding
        static constexpr float VARIABLE_JUMP_THRESHOLD = 0.8f; // Extended time window for variable jump height
        static constexpr float EARLY_RELEASE_MULTIPLIER = 0.5f; // Stronger velocity reduction on early release
        
        static constexpr float JUMP_COOLDOWN = 0.15f; // Reduced for more responsive input
        static constexpr float SHOOT_COOLDOWN = 0.3f;
        static constexpr float SCREEN_HEIGHT = 2556.0f; // iPhone 16 portrait height
        static constexpr float SCREEN_WIDTH = 1179.0f;  // Screen width
        static constexpr float WORLD_SPEED = 200.0f; // Speed at which the world moves past the player
        static constexpr float TOP_SPAWN_Y = 300.0f; // Top spawn position (near top of screen)
        
        // Helper methods
        void UpdatePlayerPhysics(float deltaTime);
        void UpdatePlayerAnimation(float deltaTime);
        void UpdatePlayerState(float deltaTime);
        void HandleCollisions();
        void SpawnProjectile();
        
        // State machine helpers
        std::string GetStateName(PlayerAnimationState state) const;
        std::string GetStateAnimationName(PlayerAnimationState state) const;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_PLAYER_CONTROLLER_SYSTEM_H 