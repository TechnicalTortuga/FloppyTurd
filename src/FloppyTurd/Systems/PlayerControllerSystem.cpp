#include "PlayerControllerSystem.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Utility/Utils.h"
#include <algorithm>

namespace GameCore {

    PlayerControllerSystem::PlayerControllerSystem(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, SpriteSystem* spriteSystem, ProjectileSystem* projectileSystem, HatsSystem* hatsSystem, SkillSystem* skillSystem, int currentLevelId)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_spriteSystem(spriteSystem)
        , m_projectileSystem(projectileSystem)
        , m_hatsSystem(hatsSystem)
        , m_skillSystem(skillSystem)
        , m_playerEntity(0)
        , m_hatSpriteEntity(0)
        , m_playerAlive(true)
        , m_currentLevelId(currentLevelId)
        , m_jumpPressed(false)
        , m_shootPressed(false)
        , m_jumpCooldown(0.0f)
        , m_shootCooldown(0.0f)
        , m_inputDelayTimer(0.0f)
        , m_isGrounded(true)
        , m_currentState(PlayerAnimationState::IDLE)
        , m_previousState(PlayerAnimationState::IDLE)
        , m_touchActive(false)
        , m_lastTouchX(0.0f)
        , m_lastTouchY(0.0f)
        , m_jumpButtonHeld(false)
        , m_jumpHoldTime(0.0f)
        , m_isAscending(false)
        , m_lastVerticalVelocity(0.0f)
        , m_idleAnimation("TurdletIdle")
        , m_jumpAnimation("TurdletJump")
        , m_shootAnimation("TurdletShoot")
        , m_hurtAnimation("TurdletHurt")
    {
        GN_LOG_INFO("PlayerControllerSystem initialized");
    }

    PlayerControllerSystem::~PlayerControllerSystem() {
        // Clean up hat sprite entity
        if (m_hatSpriteEntity != 0 && m_ecsSystem) {
            m_ecsSystem->DestroyEntity(m_hatSpriteEntity);
            m_hatSpriteEntity = 0;
        }
        GN_LOG_INFO("PlayerControllerSystem destroyed");
    }

    void PlayerControllerSystem::Update(float deltaTime) {
        if (m_playerEntity == 0) {
            return; // Only return if no player entity, not if player is dead
        }

        // DEBUG: Show flag states at start of update - LOG ALWAYS for debugging
        GN_LOG_INFO("🎯 Update flags: m_touchSession.active=" + std::to_string(m_touchSession.active) + ", m_jumpButtonHeld=" + std::to_string(m_jumpButtonHeld) + ", m_jumpHoldTime=" + std::to_string(m_jumpHoldTime * 1000.0f) + "ms, startTime=" + std::to_string(m_touchSession.startTime));

        // Update cooldowns
        if (m_jumpCooldown > 0.0f) {
            m_jumpCooldown -= deltaTime;
        }
        if (m_shootCooldown > 0.0f) {
            m_shootCooldown -= deltaTime;
        }

        // Update input delay timer (prevents accidental shooting at game start)
        if (m_inputDelayTimer > 0.0f) {
            m_inputDelayTimer -= deltaTime;
            if (m_inputDelayTimer <= 0.0f) {
                m_inputDelayTimer = 0.0f;
                GN_LOG_INFO("Input delay timer expired - shooting now enabled");
            }
        }

        // Update variable jump mechanics - handle hold time and auto-jump
        if (m_jumpButtonHeld && m_touchSession.active) {
            // Use timestamp-based calculation instead of deltaTime accumulation
            uint64_t currentTime = GameCore::GetCurrentTimestamp();
            uint64_t holdDurationMs = currentTime - m_touchSession.startTime;
            
            // Add debug logging to track timestamp-based timing
            GN_LOG_INFO("🔥 Jump hold duration: " + std::to_string(holdDurationMs) + "ms (timestamp-based)");
            
            // Check for auto-jump after 200ms
            if (holdDurationMs >= 200) { // AUTO_JUMP_THRESHOLD converted to milliseconds
                GN_LOG_INFO("Auto-jump triggered at " + std::to_string(holdDurationMs) + "ms - executing max force jump!");
                
                // Trigger max force jump for auto-jump
                HandleJumpInputWithForce(JUMP_FORCE);
                
                // Clear jump process - we don't need to wait for release anymore
                m_jumpButtonHeld = false;
                m_jumpHoldTime = 0.0f;
                // Keep m_touchSession.active true so we can ignore the eventual release
                GN_LOG_INFO("Auto-jump complete - release events will be ignored");
            } else {
                // Update legacy m_jumpHoldTime for compatibility with other systems that might read it
                m_jumpHoldTime = holdDurationMs / 1000.0f;
            }
        }

        // Update player systems
        UpdatePlayerPhysics(deltaTime);
        UpdatePlayerAnimation(deltaTime);
        UpdatePlayerState(deltaTime);

        // Update hat sprite position to follow player
        UpdateHatSpritePosition();

        // Update skill effects
        if (m_skillSystem) {
            m_skillSystem->UpdateSkillEffects(deltaTime, m_playerEntity);
        }

        // Collision with pickups is handled centrally in GameplayState now
    }

    void PlayerControllerSystem::HandleTouchInput(float x, float y, bool isJustPressed) {
        GN_LOG_INFO("PlayerControllerSystem::HandleTouchInput called with (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(isJustPressed) + ")");
        
        if (!m_playerAlive) {
            GN_LOG_WARN("Input ignored - player not alive");
            return;
        }

        uint64_t currentTime = GameCore::GetCurrentTimestamp();

        if (isJustPressed) {
            // This is a isJustPressed event - only triggers ONCE when touch starts
            GN_LOG_INFO("Touch JUST PRESSED detected - starting touch session");
            
            // Don't start new touch if we're already tracking one
            if (m_touchSession.active) {
                GN_LOG_WARN("Ignoring new touch - already tracking active touch session");
                return;
            }
            
            // Start new touch session with timestamp
            m_touchSession.active = true;
            m_touchSession.startTime = currentTime;
            m_touchSession.startX = x;
            m_touchSession.startY = y;
            
            // Determine if touch is in bottom area (shoot zone) or general area (jump zone)
            float screenHeight = 2556.0f; // iPhone 16 game height
            float shootZoneHeight = screenHeight * 0.33f; // Bottom 1/3 of screen for shooting
            
            GN_LOG_INFO("Touch press at y=" + std::to_string(y) + ", shootZone starts at y=" + std::to_string(screenHeight - shootZoneHeight));
            
            if (y > (screenHeight - shootZoneHeight)) {
                // Bottom area - shoot immediately on press
                GN_LOG_INFO("Shoot zone pressed at y=" + std::to_string(y) + "! Calling HandleShootInput...");
                HandleShootInput();
                m_touchSession.active = false; // Shooting doesn't use hold mechanics
            } else {
                // Upper area - START jump hold tracking (don't jump yet!)
                GN_LOG_INFO("Jump zone pressed - starting hold timer (no jump yet)!");
                m_jumpButtonHeld = true;
                m_jumpHoldTime = 0.0f; // Keep this for compatibility with existing Update() logic
                
                // DON'T trigger jump here - wait for release or auto-jump
            }
            
        } else {
            // This is a isJustReleased event - only triggers ONCE when touch ends
            GN_LOG_INFO("Touch JUST RELEASED detected");
            
            if (m_touchSession.active) {
                // Calculate actual hold duration using timestamps
                uint64_t holdDurationMs = currentTime - m_touchSession.startTime;
                double holdDurationSeconds = holdDurationMs / 1000.0;
                
                GN_LOG_INFO("Touch session duration: " + std::to_string(holdDurationMs) + "ms (" + std::to_string(holdDurationSeconds) + "s)");
                
                if (m_jumpButtonHeld) {
                    // We were holding a jump and haven't auto-jumped yet - trigger jump with variable force
                    GN_LOG_INFO("Jump button released after " + std::to_string(holdDurationMs) + "ms hold - executing jump now!");
                    
                    // Calculate jump force based on hold time
                    float jumpForce = JUMP_FORCE;
                    if (holdDurationSeconds < VARIABLE_JUMP_THRESHOLD) {
                        // Early release - reduce jump force based on how long it was held
                        float holdRatio = holdDurationSeconds / VARIABLE_JUMP_THRESHOLD;
                        jumpForce = JUMP_FORCE * (EARLY_RELEASE_MULTIPLIER + holdRatio * (1.0f - EARLY_RELEASE_MULTIPLIER));
                        GN_LOG_INFO("Variable jump: Early release at " + std::to_string(holdDurationMs) + "ms, force=" + std::to_string(jumpForce));
                    } else {
                        GN_LOG_INFO("Variable jump: Full hold completed, force=" + std::to_string(jumpForce));
                    }
                    
                    // Execute the jump with calculated force
                    HandleJumpInputWithForce(jumpForce);
                } else {
                    // Release after auto-jump or in shoot zone - ignore it
                    GN_LOG_INFO("Release ignored - auto-jump already occurred or was in shoot zone");
                }
            }
            
            // Reset touch session and jump state
            m_touchSession.active = false;
            m_touchSession.startTime = 0;
            m_touchSession.startX = 0.0f;
            m_touchSession.startY = 0.0f;
            m_jumpButtonHeld = false;
            m_jumpHoldTime = 0.0f;
        }
    }

    void PlayerControllerSystem::HandleJumpInput() {
        HandleJumpInputWithForce(JUMP_FORCE);
    }

    void PlayerControllerSystem::HandleJumpInputWithForce(float force) {
        if (!m_playerAlive || m_jumpCooldown > 0.0f) {
            GN_LOG_DEBUG("Jump input ignored - alive=" + std::to_string(m_playerAlive) + 
                        ", cooldown=" + std::to_string(m_jumpCooldown));
            return;
        }
        
        // Apply jump force regardless of being grounded - this is Flappy Bird style
        if (m_playerEntity != 0) {
            Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            if (physics) {
                // Apply jump impulse with specified force
                physics->velocity.y = -force; // Negative Y is up in our coordinate system
                m_jumpCooldown = JUMP_COOLDOWN;
                m_isAscending = true;  // Track that we're now ascending
                
                // ONLY transition to JUMPING state if we actually jumped AND not in hurt state
                // Don't interrupt hurt animation with jump animation
                if (m_currentState != PlayerAnimationState::HURT) {
                    TransitionToState(PlayerAnimationState::JUMPING);
                } else {
                    GN_LOG_INFO("Player jumped but staying in HURT animation state");
                }
                
                GN_LOG_INFO("Player jumped with force (" + std::to_string(force) + ") - transitioned to JUMPING state");
            }
        }
    }

    void PlayerControllerSystem::HandleJumpRelease() {
        if (!m_playerAlive || m_playerEntity == 0) {
            return;
        }
        
        Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
        if (physics && m_isAscending && physics->velocity.y < 0) {
            // Apply early release penalty - cut the jump short by reducing upward velocity
            if (m_jumpHoldTime < VARIABLE_JUMP_THRESHOLD) {
                physics->velocity.y *= EARLY_RELEASE_MULTIPLIER;
                GN_LOG_INFO("Variable jump: Early release penalty applied at " + 
                           std::to_string(m_jumpHoldTime * 1000.0f) + "ms, velocity reduced from " + 
                           std::to_string(physics->velocity.y / EARLY_RELEASE_MULTIPLIER) + 
                           " to " + std::to_string(physics->velocity.y));
            } else {
                GN_LOG_INFO("Variable jump: Full jump completed, hold time: " + 
                           std::to_string(m_jumpHoldTime * 1000.0f) + "ms");
            }
        }
    }

    void PlayerControllerSystem::HandleShootInput() {
        GN_LOG_INFO("HandleShootInput called!");
        if (!m_playerAlive || m_shootCooldown > 0.0f) {
            GN_LOG_DEBUG("Shoot input ignored - alive=" + std::to_string(m_playerAlive) +
                        ", cooldown=" + std::to_string(m_shootCooldown));
            return;
        }

        GN_LOG_INFO("Shooting conditions met, spawning projectile...");

        // Check if we're in the input delay period (prevent accidental shooting at game start)
        if (m_inputDelayTimer > 0.0f) {
            GN_LOG_DEBUG("Shoot input ignored - in input delay period (%.2fs remaining)", m_inputDelayTimer);
            return;
        }

        // Set shoot cooldown and spawn projectile - this happens ONCE per input
        m_shootCooldown = SHOOT_COOLDOWN;
        SpawnProjectile();

        // ONLY transition to SHOOTING state if we actually shot (passed cooldown check)
        TransitionToState(PlayerAnimationState::SHOOTING);

        GN_LOG_INFO("Player shot projectile - transitioned to SHOOTING state");
    }

    void PlayerControllerSystem::SetPlayerEntity(Gnosis::Entity playerEntity) {
        m_playerEntity = playerEntity;
        GN_LOG_INFO("Player entity set: %d", playerEntity);

        // Create hat sprite entity when player entity is set
        if (playerEntity != 0) {
            CreateHatSprite();
        }
    }

    std::string PlayerControllerSystem::GetHatAdjustedTextureName(const std::string& baseAnimationName) {
        // Get the equipped hat index from HatsSystem
        if (!m_hatsSystem) {
            GN_LOG_DEBUG("PlayerController: No hats system available, using base animation: %s", baseAnimationName.c_str());
            return baseAnimationName; // No hats system, use base animation
        }

        int equippedHatIndex = m_hatsSystem->GetEquippedHatIndex();
        if (equippedHatIndex < 0) {
            GN_LOG_DEBUG("PlayerController: No hat equipped (index: %d), using base animation: %s", equippedHatIndex, baseAnimationName.c_str());
            return baseAnimationName; // No hat equipped, use base animation
        }

        // Get hat data to determine the correct texture
        const auto* hatData = m_hatsSystem->GetHatData(equippedHatIndex);
        if (!hatData) {
            GN_LOG_WARN("PlayerController: Invalid hat data for equipped hat index: %d, using base animation: %s", equippedHatIndex, baseAnimationName.c_str());
            return baseAnimationName; // Invalid hat data, use base animation
        }

        GN_LOG_DEBUG("PlayerController: Hat equipped - index: %d, name: %s", equippedHatIndex, hatData->name.c_str());

        // Determine which texture to use based on animation type
        std::string selectedTexture;
        if (baseAnimationName == "TurdletIdle") {
            selectedTexture = hatData->turdletIdlePath;
            GN_LOG_DEBUG("PlayerController: TurdletIdle -> using turdletIdlePath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "TurdletJump") {
            selectedTexture = hatData->turdletJumpPath;
            GN_LOG_DEBUG("PlayerController: TurdletJump -> using turdletJumpPath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "TurdletShoot") {
            selectedTexture = hatData->turdletShootPath;
            GN_LOG_DEBUG("PlayerController: TurdletShoot -> using turdletShootPath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "TurdletHurt") {
            selectedTexture = hatData->turdletJumpPath; // Hurt uses jump animation
            GN_LOG_DEBUG("PlayerController: TurdletHurt -> using turdletJumpPath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "TeenIdle") {
            selectedTexture = hatData->teenageIdlePath;
            GN_LOG_DEBUG("PlayerController: TeenIdle -> using teenageIdlePath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "TeenJump") {
            selectedTexture = hatData->teenageJumpPath;
            GN_LOG_DEBUG("PlayerController: TeenJump -> using teenageJumpPath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "TeenShoot") {
            selectedTexture = hatData->teenageShootPath;
            GN_LOG_DEBUG("PlayerController: TeenShoot -> using teenageShootPath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "BigIdle") {
            selectedTexture = hatData->bigTurdIdlePath;
            GN_LOG_DEBUG("PlayerController: BigIdle -> using bigTurdIdlePath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "BigJump") {
            selectedTexture = hatData->bigTurdJumpPath;
            GN_LOG_DEBUG("PlayerController: BigJump -> using bigTurdJumpPath: '%s'", selectedTexture.c_str());
        } else if (baseAnimationName == "BigShoot") {
            selectedTexture = hatData->bigTurdShootPath;
            GN_LOG_DEBUG("PlayerController: BigShoot -> using bigTurdShootPath: '%s'", selectedTexture.c_str());
        } else {
            // Unknown animation, return base name
            GN_LOG_WARN("PlayerController: Unknown animation '%s', using base animation", baseAnimationName.c_str());
            return baseAnimationName;
        }

        // Check if the selected texture is empty
        if (selectedTexture.empty()) {
            GN_LOG_WARN("PlayerController: Selected texture is empty for animation '%s' and hat '%s', using base animation",
                       baseAnimationName.c_str(), hatData->name.c_str());
            return baseAnimationName;
        }

        GN_LOG_INFO("PlayerController: Using hat texture '%s' for animation '%s'",
                   selectedTexture.c_str(), baseAnimationName.c_str());
        return selectedTexture;
    }

    void PlayerControllerSystem::ChangePlayerAnimation(const std::string& animationName) {
        if (m_spriteSystem && m_playerEntity != 0) {
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
            if (sprite) {
                // Set base player texture (without hat)
                std::string baseTextureName;

                // Map animation names to base textures
                if (animationName == "TurdletIdle") {
                    baseTextureName = "TurdletIdle";
                } else if (animationName == "TurdletJump") {
                    baseTextureName = "TurdletJump";
                } else if (animationName == "TurdletShoot") {
                    baseTextureName = "TurdletShoot";
                } else if (animationName == "TurdletHurt") {
                    baseTextureName = "TurdletHurt";
                } else {
                    baseTextureName = "TurdletIdle"; // Default fallback
                }

                std::string oldTexture = sprite->textureId;

                // Configure animation properties based on type FIRST
                if (animationName == "TurdletIdle") {
                    // Idle should be single frame, not animated
                    sprite->frameCount = 1;
                    sprite->isAnimated = false;
                    sprite->playing = false;
                    sprite->loop = false;
                    sprite->frameTime = 0.1f; // Not used for idle, but set anyway
                } else if (animationName == "TurdletJump") {
                    sprite->frameCount = 6;
                    sprite->isAnimated = true;
                    sprite->playing = true;
                    sprite->loop = false;
                    sprite->frameTime = 0.08f;
                } else if (animationName == "TurdletShoot") {
                    sprite->frameCount = 6;
                    sprite->isAnimated = true;
                    sprite->playing = true;
                    sprite->loop = false;
                    sprite->frameTime = 0.1f;
                } else if (animationName == "TurdletHurt") {
                    sprite->frameCount = 6;
                    sprite->isAnimated = true;
                    sprite->playing = true;
                    sprite->loop = false;
                    sprite->frameTime = 0.12f;
                }

                // Set base player texture (no hat) AFTER configuration
                sprite->textureId = baseTextureName;
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                sprite->hasCompleted = false; // Reset completion flag

                GN_LOG_INFO("PlayerController: Changed base animation to '%s' (texture: '%s' -> '%s')",
                           animationName.c_str(), oldTexture.c_str(), baseTextureName.c_str());

                // Update hat sprite if equipped
                UpdateHatSpriteTexture(animationName);

                GN_LOG_DEBUG("PlayerController: Animation configured - frameCount: %d, isAnimated: %d, playing: %d",
                           sprite->frameCount, sprite->isAnimated, sprite->playing);

                GN_LOG_INFO("PlayerController: Changed animation to '%s' with %d frames, isAnimated=%d, playing=%d",
                           animationName.c_str(), sprite->frameCount, sprite->isAnimated, sprite->playing);
            }
        }
    }

    void PlayerControllerSystem::TransitionToState(PlayerAnimationState newState) {
        if (newState == m_currentState) {
            // Same state - don't restart animation, just log that we're already in this state
            GN_LOG_DEBUG("PlayerController: Already in state " + GetStateName(newState) + ", keeping current animation");
            return;
        }
        
        // Different state - log transition
        GN_LOG_INFO("PlayerController: State transition from " + GetStateName(m_currentState) + " to " + GetStateName(newState));
        m_previousState = m_currentState;
        m_currentState = newState;
        
        // Get animation name for this state and play it
        std::string animationName = GetStateAnimationName(newState);
        ChangePlayerAnimation(animationName);
    }

    std::string PlayerControllerSystem::GetStateName(PlayerAnimationState state) const {
        switch (state) {
            case PlayerAnimationState::IDLE:     return "IDLE";
            case PlayerAnimationState::JUMPING:  return "JUMPING";
            case PlayerAnimationState::SHOOTING: return "SHOOTING";
            case PlayerAnimationState::HURT:     return "HURT";
            default:                             return "UNKNOWN";
        }
    }

    std::string PlayerControllerSystem::GetStateAnimationName(PlayerAnimationState state) const {
        switch (state) {
            case PlayerAnimationState::IDLE:     return m_idleAnimation;
            case PlayerAnimationState::JUMPING:  return m_jumpAnimation;
            case PlayerAnimationState::SHOOTING: return m_shootAnimation;
            case PlayerAnimationState::HURT:     return m_hurtAnimation;
            default:                             return m_idleAnimation;
        }
    }

    void PlayerControllerSystem::ResetPlayer() {
        m_playerAlive = true;
        m_isGrounded = false; // Start in the air for Flappy Bird style
        
        // Reset state machine to IDLE
        m_currentState = PlayerAnimationState::IDLE;
        m_previousState = PlayerAnimationState::IDLE;
        
        // Reset enhanced jump mechanics state
        m_jumpButtonHeld = false;
        m_jumpHoldTime = 0.0f;
        m_isAscending = false;
        m_lastVerticalVelocity = 0.0f;
        
        if (m_playerEntity != 0) {
            // Reset player position and physics
            Transform* transform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
            Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            
            if (transform) {
                // Start player at the fixed X position and near the top of screen
                transform->position = Gnosis::GNVector2(PLAYER_X_POSITION, TOP_SPAWN_Y);
            }
            
            if (physics) {
                physics->velocity = Gnosis::GNVector2(0.0f, 0.0f);
                physics->acceleration = Gnosis::GNVector2(0.0f, 0.0f);
                physics->drag = 0.99f; // Slightly higher drag for better control
                physics->useGravity = true;
            }
            
            TransitionToState(PlayerAnimationState::IDLE);
        }
        
        GN_LOG_INFO("Player reset for enhanced Flappy Bird mode - state machine and physics reset to IDLE");
    }

    void PlayerControllerSystem::SetPlayerAlive(bool alive) {
        m_playerAlive = alive;
        GN_LOG_INFO("PlayerControllerSystem: Player alive state set to " + std::string(alive ? "true" : "false"));
    }

    void PlayerControllerSystem::UpdatePlayerPhysics(float deltaTime) {
        if (m_playerEntity == 0) {
            return;
        }

        Transform* transform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
        
        if (!transform || !physics) {
            return;
        }
        
        // Enhanced physics system with variable gravity and terminal velocity
        // Based on modern platformer best practices from research
        
        // Track velocity direction for ascending/descending detection
        bool wasAscending = m_isAscending;
        m_isAscending = (physics->velocity.y < 0); // Negative Y is upward
        
        // Detect velocity direction change (peak of jump reached)
        if (wasAscending && !m_isAscending) {
            GN_LOG_DEBUG("Jump peak reached, switching to descending gravity");
        }
        
        // In Flappy Bird, the player moves forward at a constant rate
        // The world moves past the player, but in our implementation we'll keep the player
        // at a fixed X position and handle world movement separately in the obstacle system
        physics->velocity.x = 0; // Player doesn't actually move horizontally in screen space
        
        // Apply enhanced gravity system - different rates for ascending vs falling
        if (physics->useGravity) {
            if (m_isAscending) {
                // Lighter gravity while ascending for floaty feel
                physics->acceleration.y = GRAVITY_UP;
                GN_LOG_TRACE("Applying ascending gravity: " + std::to_string(GRAVITY_UP));
            } else {
                // Much heavier gravity while falling for fast, satisfying drops
                physics->acceleration.y = GRAVITY_DOWN;
                GN_LOG_TRACE("Applying descending gravity: " + std::to_string(GRAVITY_DOWN));
            }
        } else {
            // DEBUG: Check if gravity is disabled for dead players
            if (!m_playerAlive) {
                GN_LOG_WARN("Dead player has useGravity = false - this might prevent falling!");
            }
        }
        
        // Update velocity with enhanced gravity
        physics->velocity += physics->acceleration * deltaTime;
        
        // Apply terminal velocity for realistic falling (only limit downward velocity)
        if (physics->velocity.y > TERMINAL_VELOCITY) {
            physics->velocity.y = TERMINAL_VELOCITY;
            GN_LOG_DEBUG("Terminal velocity reached: " + std::to_string(TERMINAL_VELOCITY));
        }
        
        // Apply drag (only to Y velocity in Flappy Bird style)
        physics->velocity.y = physics->velocity.y * physics->drag;
        
        // Update position (only Y position changes for the player)
        transform->position.y += physics->velocity.y * deltaTime;
        
        // DEBUG: Log player physics state during game over
        if (!m_playerAlive) {
            GN_LOG_INFO("Dead player physics - Y: " + std::to_string(transform->position.y) + 
                       ", Velocity: " + std::to_string(physics->velocity.y) + 
                       ", Delta: " + std::to_string(deltaTime));
        }
        
        // Keep player at a fixed horizontal position based on level
        // Boss level (6): position at 100px from left edge
        // Other levels: use standard PLAYER_X_POSITION (centered)
        if (m_currentLevelId == 6) {
            // Boss level - position at 100px from left edge
            transform->position.x = 100.0f;
        } else {
            // Normal levels - use standard centered position
            transform->position.x = PLAYER_X_POSITION;
        }
        
        // Get player sprite to calculate actual size for proper boundary checking
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        float playerHeight = sprite ? sprite->height * transform->scale.y : 64.0f; // Default to 64 if no sprite
        
        // Only reset when player is completely off screen below (entire sprite past bottom)
        if (transform->position.y > SCREEN_HEIGHT + playerHeight) {
            // Player is completely off screen below - trigger damage and reset position
            if (m_playerAlive) {
                GN_LOG_INFO("Player completely off screen below - resetting position");
                transform->position.y = TOP_SPAWN_Y;
                physics->velocity.y = 0.0f;
                m_isAscending = false;
                m_jumpButtonHeld = false;
                m_jumpHoldTime = 0.0f;
                // TODO: Trigger damage event
            } else {
                GN_LOG_INFO("Player is dead and off screen - not resetting position");
            }
        }
        
        // Check if player hits top of screen - only apply ceiling collision when alive
        if (transform->position.y <= 0.0f) {
            if (m_playerAlive) {
                transform->position.y = 0.0f;
                physics->velocity.y = 0.0f;
                m_isAscending = false;
                GN_LOG_INFO("Player hit ceiling - alive player stopped");
            } else {
                // Dead player can fall through ceiling - just prevent them from going above screen
                transform->position.y = 0.0f;
                // Don't reset velocity - let them fall naturally
                GN_LOG_INFO("Player hit ceiling - dead player continues falling");
            }
        }
        
        // Store current velocity for next frame's direction detection
        m_lastVerticalVelocity = physics->velocity.y;
        
        // Reset acceleration
        physics->acceleration = Gnosis::GNVector2(0.0f, 0.0f);
    }

    void PlayerControllerSystem::UpdatePlayerAnimation(float deltaTime) {
        if (m_playerEntity == 0 || !m_spriteSystem) {
            return;
        }

        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        if (!sprite) {
            return;
        }

        // DEBUG: Log current animation state
        GN_LOG_DEBUG("Animation Debug - State: " + GetStateName(m_currentState) + 
                    ", Playing: " + std::to_string(sprite->playing) + 
                    ", Completed: " + std::to_string(sprite->hasCompleted) + 
                    ", Frame: " + std::to_string(sprite->currentFrame) + "/" + std::to_string(sprite->frameCount) +
                    ", JumpCooldown: " + std::to_string(m_jumpCooldown));

        // Check for animation completion based on state
        if (m_currentState != PlayerAnimationState::IDLE && sprite->hasCompleted) {
            GN_LOG_INFO("PlayerController: Animation completed in state " + GetStateName(m_currentState) + ", returning to IDLE");
            TransitionToState(PlayerAnimationState::IDLE);
            return;
        }
        
        // Force completion detection for non-looping animations at last frame
        if (m_currentState != PlayerAnimationState::IDLE && !sprite->loop && 
            sprite->currentFrame >= sprite->frameCount - 1 && sprite->playing) {
            
            GN_LOG_INFO("PlayerController: Animation at last frame in state " + GetStateName(m_currentState) + 
                       ", forcing completion, currentFrame=" + std::to_string(sprite->currentFrame) + 
                       ", frameCount=" + std::to_string(sprite->frameCount));
            
            // Force animation completion
            sprite->hasCompleted = true;
            sprite->playing = false;
            
            TransitionToState(PlayerAnimationState::IDLE);
            return;
        }
        
        // Check if non-idle animation stopped unexpectedly
        if (m_currentState != PlayerAnimationState::IDLE && !sprite->playing && !sprite->hasCompleted) {
            GN_LOG_WARN("PlayerController: Animation stopped unexpectedly in state " + GetStateName(m_currentState) + ", returning to IDLE");
            TransitionToState(PlayerAnimationState::IDLE);
            return;
        }
    }

    void PlayerControllerSystem::UpdatePlayerState(float deltaTime) {
        // Cooldown timers are updated in main Update() method
        // State management is now handled by the state machine
        // Animation completion is handled by UpdatePlayerAnimation()
    }

    void PlayerControllerSystem::HandleCollisions() {
        // Legacy pickup collision removed; kept as stub for compatibility
    }

    void PlayerControllerSystem::SpawnProjectile() {
        if (m_playerEntity == 0 || !m_projectileSystem) {
            return;
        }

        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        if (!playerTransform) {
            return;
        }

        // Determine projectile type based on player form (for now, use Floppy Poop)
        // TODO: Make this configurable based on player form/state
        ProjectileType projectileType = ProjectileType::POOP_BALL;

        // Calculate spawn position (from player's mouth position, accounting for scale)
        Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        float playerScale = playerTransform->scale.x; // Assuming uniform scaling
        float scaledOffsetX = 42.0f * playerScale;
        float scaledOffsetY = 32.0f * playerScale;
        GNVector2 spawnPosition = playerTransform->position + GNVector2(scaledOffsetX, scaledOffsetY);

        // Calculate projectile direction (right-facing for now)
        GNVector2 direction(1.0f, 0.0f); // Move right

        // Spawn projectile using the ProjectileSystem
        Entity projectileEntity = m_projectileSystem->SpawnPlayerProjectile(
            spawnPosition,
            direction,
            projectileType
        );

        if (projectileEntity != 0) {
            GN_LOG_INFO("Player spawned projectile entity: %d at position (%.1f, %.1f)",
                       projectileEntity, spawnPosition.x, spawnPosition.y);
        } else {
            GN_LOG_WARN("Failed to spawn player projectile - no available projectiles in pool");
        }
    }

    void PlayerControllerSystem::PlayIdleAnimation() {
        TransitionToState(PlayerAnimationState::IDLE);
    }

    void PlayerControllerSystem::PlayJumpAnimation() {
        TransitionToState(PlayerAnimationState::JUMPING);
    }

    void PlayerControllerSystem::PlayShootAnimation() {
        TransitionToState(PlayerAnimationState::SHOOTING);
    }

    void PlayerControllerSystem::PlayHurtAnimation() {
        TransitionToState(PlayerAnimationState::HURT);
    }

    void PlayerControllerSystem::ResetInputDelay(float delaySeconds) {
        m_inputDelayTimer = delaySeconds;
        GN_LOG_INFO("Input delay timer reset to %.2fs", delaySeconds);
    }

    void PlayerControllerSystem::CreateHatSprite() {
        if (m_hatSpriteEntity != 0) {
            // Hat sprite already exists - clean it up first
            GN_LOG_WARN("PlayerController: Hat sprite already exists (entity %d), cleaning up before creating new one", m_hatSpriteEntity);
            HideHatSprite();
            if (m_ecsSystem) {
                m_ecsSystem->DestroyEntity(m_hatSpriteEntity);
            }
            m_hatSpriteEntity = 0;
        }

        if (m_playerEntity == 0 || !m_ecsSystem) {
            return;
        }

        // Create hat sprite entity
        m_hatSpriteEntity = m_ecsSystem->CreateEntity();

        // Get player transform for positioning
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        if (!playerTransform) {
            return;
        }

        // Create hat transform (same position as player)
        Transform hatTransform = *playerTransform;
        m_ecsSystem->AddComponent<Transform>(m_hatSpriteEntity, hatTransform);

        // Create hat sprite with placeholder texture initially
        Sprite hatSprite("TurdletIdle", 64.0f, 64.0f, 64, 64, 1, 0.1f);
        hatSprite.color = Gnosis::GNColor(255, 255, 255, 255);
        hatSprite.visible = false; // Initially hidden
        hatSprite.layer = 5; // Above player layer (4) so it renders on top
        hatSprite.isAnimated = false;
        hatSprite.playing = false;
        hatSprite.loop = false;
        hatSprite.frameCount = 1;
        hatSprite.currentFrame = 0;
        hatSprite.currentFrameTime = 0.0f;
        hatSprite.hasCompleted = false;

        m_ecsSystem->AddComponent<Sprite>(m_hatSpriteEntity, hatSprite);

        GN_LOG_INFO("PlayerController: Created hat sprite entity %d", m_hatSpriteEntity);
    }

    void PlayerControllerSystem::UpdateHatSpritePosition() {
        if (m_hatSpriteEntity == 0 || m_playerEntity == 0 || !m_ecsSystem) {
            return;
        }

        // Copy player's position to hat sprite
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        Transform* hatTransform = m_ecsSystem->GetComponent<Transform>(m_hatSpriteEntity);

        if (playerTransform && hatTransform) {
            hatTransform->position = playerTransform->position;
            hatTransform->rotation = playerTransform->rotation;
            hatTransform->scale = playerTransform->scale;
        }
    }

    void PlayerControllerSystem::UpdateHatSpriteTexture(const std::string& animationName) {
        if (m_hatSpriteEntity == 0 || !m_ecsSystem || !m_hatsSystem) {
            return;
        }

        int equippedHatIndex = m_hatsSystem->GetEquippedHatIndex();
        if (equippedHatIndex < 0) {
            // No hat equipped, hide hat sprite
            HideHatSprite();
            return;
        }

        // Get hat data
        const auto* hatData = m_hatsSystem->GetHatData(equippedHatIndex);
        if (!hatData) {
            HideHatSprite();
            return;
        }

        // Get appropriate hat texture for this animation
        std::string hatTexture = GetHatAdjustedTextureName(animationName);
        if (hatTexture.empty() || hatTexture == animationName) {
            // No specific hat texture, hide hat sprite
            HideHatSprite();
            return;
        }

        // Update hat sprite texture
        Sprite* hatSprite = m_ecsSystem->GetComponent<Sprite>(m_hatSpriteEntity);
        if (hatSprite) {
            hatSprite->textureId = hatTexture;
            hatSprite->visible = true;

            // Configure hat sprite animation properties
            if (animationName == "TurdletIdle") {
                // For idle, use single frame (first frame of texture)
                hatSprite->frameCount = 1;
                hatSprite->isAnimated = false;
                hatSprite->playing = false;
                hatSprite->loop = false;
                hatSprite->frameTime = 0.1f; // Not used for idle
                GN_LOG_DEBUG("PlayerController: Hat sprite configured for IDLE - texture: %s, frameCount: %d, isAnimated: %d, playing: %d",
                           hatTexture.c_str(), hatSprite->frameCount, hatSprite->isAnimated, hatSprite->playing);
            } else if (animationName == "TurdletJump") {
                // For jump, use full 6-frame animation
                hatSprite->frameCount = 6;
                hatSprite->isAnimated = true;
                hatSprite->playing = true;
                hatSprite->loop = false;
                hatSprite->frameTime = 0.08f;
                GN_LOG_DEBUG("PlayerController: Hat sprite configured for JUMP - texture: %s, frameCount: %d, isAnimated: %d, playing: %d",
                           hatTexture.c_str(), hatSprite->frameCount, hatSprite->isAnimated, hatSprite->playing);
            } else if (animationName == "TurdletShoot") {
                // For shoot, use full 6-frame animation
                hatSprite->frameCount = 6;
                hatSprite->isAnimated = true;
                hatSprite->playing = true;
                hatSprite->loop = false;
                hatSprite->frameTime = 0.1f;
                GN_LOG_DEBUG("PlayerController: Hat sprite configured for SHOOT - texture: %s, frameCount: %d, isAnimated: %d, playing: %d",
                           hatTexture.c_str(), hatSprite->frameCount, hatSprite->isAnimated, hatSprite->playing);
            } else if (animationName == "TurdletHurt") {
                // For hurt, use full 6-frame animation
                hatSprite->frameCount = 6;
                hatSprite->isAnimated = true;
                hatSprite->playing = true;
                hatSprite->loop = false;
                hatSprite->frameTime = 0.12f;
                GN_LOG_DEBUG("PlayerController: Hat sprite configured for HURT - texture: %s, frameCount: %d, isAnimated: %d, playing: %d",
                           hatTexture.c_str(), hatSprite->frameCount, hatSprite->isAnimated, hatSprite->playing);
            }

            hatSprite->currentFrame = 0;
            hatSprite->currentFrameTime = 0.0f;
            hatSprite->hasCompleted = false;

            GN_LOG_INFO("PlayerController: Updated hat sprite texture to '%s' for animation '%s'",
                       hatTexture.c_str(), animationName.c_str());
        }
    }

    void PlayerControllerSystem::HideHatSprite() {
        if (m_hatSpriteEntity != 0 && m_ecsSystem) {
            Sprite* hatSprite = m_ecsSystem->GetComponent<Sprite>(m_hatSpriteEntity);
            if (hatSprite) {
                hatSprite->visible = false;
                GN_LOG_DEBUG("PlayerController: Hidden hat sprite");
            }
        }
    }

    void PlayerControllerSystem::ShowHatSprite() {
        if (m_hatSpriteEntity != 0 && m_ecsSystem) {
            Sprite* hatSprite = m_ecsSystem->GetComponent<Sprite>(m_hatSpriteEntity);
            if (hatSprite) {
                hatSprite->visible = true;
                GN_LOG_DEBUG("PlayerController: Shown hat sprite");
            }
        }
    }

} // namespace GameCore 