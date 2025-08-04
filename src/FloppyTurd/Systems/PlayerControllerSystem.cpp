#include "PlayerControllerSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    PlayerControllerSystem::PlayerControllerSystem(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, SpriteSystem* spriteSystem)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_spriteSystem(spriteSystem)
        , m_playerEntity(0)
        , m_playerAlive(true)
        , m_jumpPressed(false)
        , m_shootPressed(false)
        , m_jumpCooldown(0.0f)
        , m_shootCooldown(0.0f)
        , m_isGrounded(true)
        , m_isJumping(false)
        , m_isShooting(false)
        , m_jumpTimer(0.0f)
        , m_shootTimer(0.0f)
        , m_currentAnimation("idle")
        , m_idleAnimation("TurdletIdle")
        , m_jumpAnimation("TurdletJump")
        , m_shootAnimation("TurdletShoot")
        , m_hurtAnimation("TurdletHurt")
    {
        GN_LOG_INFO("PlayerControllerSystem initialized");
    }

    PlayerControllerSystem::~PlayerControllerSystem() {
        GN_LOG_INFO("PlayerControllerSystem destroyed");
    }

    void PlayerControllerSystem::Update(float deltaTime) {
        if (!m_playerAlive || m_playerEntity == 0) {
            return;
        }

        // Update cooldowns
        if (m_jumpCooldown > 0.0f) {
            m_jumpCooldown -= deltaTime;
        }
        if (m_shootCooldown > 0.0f) {
            m_shootCooldown -= deltaTime;
        }

        // Update timers
        if (m_jumpTimer > 0.0f) {
            m_jumpTimer -= deltaTime;
        }
        if (m_shootTimer > 0.0f) {
            m_shootTimer -= deltaTime;
        }

        // Update player systems
        UpdatePlayerPhysics(deltaTime);
        UpdatePlayerAnimation(deltaTime);
        UpdatePlayerState(deltaTime);
        HandleCollisions();
    }

    void PlayerControllerSystem::HandleTouchInput(float x, float y, bool isPressed) {
        GN_LOG_INFO("PlayerControllerSystem::HandleTouchInput called with (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(isPressed) + ")");
        
        if (!m_playerAlive) {
            GN_LOG_WARN("Input ignored - player not alive");
            return;
        }

        // Determine if touch is in bottom area (shoot zone) or general area (jump zone)
        float screenHeight = 2556.0f; // iPhone 16 game height
        float shootZoneHeight = screenHeight * 0.33f; // Bottom 1/3 of screen for shooting
        
        if (isPressed) {
            GN_LOG_INFO("Touch detected at y=" + std::to_string(y) + ", shootZone starts at y=" + std::to_string(screenHeight - shootZoneHeight));
            
            if (y > (screenHeight - shootZoneHeight)) {
                // Bottom area - shoot
                GN_LOG_INFO("Shoot zone touched!");
                HandleShootInput();
            } else {
                // Upper area - jump
                GN_LOG_INFO("Jump zone touched!");
                HandleJumpInput();
            }
        }
    }

    void PlayerControllerSystem::HandleJumpInput() {
        if (!m_playerAlive || m_jumpCooldown > 0.0f || m_isShooting) {
            return;
        }

        if (m_isGrounded) {
            // Apply jump force
            Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            if (physics) {
                physics->velocity.y = -JUMP_FORCE; // Negative Y is up in our coordinate system
                m_isGrounded = false;
                m_isJumping = true;
                m_jumpCooldown = JUMP_COOLDOWN;
                m_jumpTimer = 1.2f; // Jump animation duration (6 frames * 0.2f frameTime)
                
                PlayJumpAnimation();
                GN_LOG_INFO("Player jumped");
            }
        }
    }

    void PlayerControllerSystem::HandleShootInput() {
        if (!m_playerAlive || m_shootCooldown > 0.0f || m_isShooting) {
            return;
        }

        m_isShooting = true;
        m_shootCooldown = SHOOT_COOLDOWN;
        m_shootTimer = SHOOT_ANIMATION_DURATION;
        
        PlayShootAnimation();
        SpawnProjectile();
        GN_LOG_INFO("Player shot projectile");
    }

    void PlayerControllerSystem::SetPlayerEntity(Gnosis::Entity playerEntity) {
        m_playerEntity = playerEntity;
        GN_LOG_INFO("Player entity set: %d", playerEntity);
    }

    void PlayerControllerSystem::ChangePlayerAnimation(const std::string& animationName) {
        if (m_spriteSystem && m_playerEntity != 0) {
            // Update the sprite's texture ID to the new animation
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
            if (sprite) {
                sprite->textureId = animationName;
                sprite->playing = true;
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                m_currentAnimation = animationName;
                
                // Store current sprite dimensions to maintain consistency
                float currentWidth = sprite->width;
                float currentHeight = sprite->height;
                
                // Update frame count and animation properties based on animation type
                if (animationName == "TurdletIdle") {
                    sprite->frameCount = 1; // 64x64 pixels, single frame
                    sprite->frameWidth = 64; // Each frame is 64x64 pixels
                    sprite->frameHeight = 64;
                    sprite->frameTime = 0.1f; // Frame timing (not used for single frame)
                    sprite->isAnimated = false; // Single frame, not animated
                    sprite->playing = false;
                } else if (animationName == "TurdletJump") {
                    sprite->frameCount = 6; // 384x64 pixels = 6 frames
                    sprite->frameWidth = 64; // Each frame is 64x64 pixels
                    sprite->frameHeight = 64;
                    sprite->frameTime = 0.2f; // Slower for jump animation
                    sprite->isAnimated = true; // Multi-frame animation
                    sprite->playing = true;
                    sprite->loop = false; // Don't loop - play once and stop
                } else if (animationName == "TurdletShoot") {
                    sprite->frameCount = 5; // 320x64 pixels = 5 frames
                    sprite->frameWidth = 64; // Each frame is 64x64 pixels
                    sprite->frameHeight = 64;
                    sprite->frameTime = 0.15f; // Slower for shoot animation
                    sprite->isAnimated = true; // Multi-frame animation
                    sprite->playing = true;
                    sprite->loop = false; // Don't loop - play once and stop
                } else if (animationName == "TurdletHurt") {
                    sprite->frameCount = 6; // 384x64 pixels = 6 frames
                    sprite->frameWidth = 64; // Each frame is 64x64 pixels
                    sprite->frameHeight = 64;
                    sprite->frameTime = 0.18f; // Slower for hurt animation
                    sprite->isAnimated = true; // Multi-frame animation
                    sprite->playing = true;
                    sprite->loop = false; // Don't loop - play once and stop
                }
                
                // Maintain consistent sprite dimensions across all animations
                sprite->width = currentWidth;
                sprite->height = currentHeight;
                
                GN_LOG_INFO("Changed animation to '" + animationName + "' with " + std::to_string(sprite->frameCount) + " frames, isAnimated=" + std::to_string(sprite->isAnimated) + ", playing=" + std::to_string(sprite->playing) + ", frameTime=" + std::to_string(sprite->frameTime));
            }
        }
    }

    void PlayerControllerSystem::ResetPlayer() {
        m_playerAlive = true;
        m_isGrounded = true;
        m_isJumping = false;
        m_isShooting = false;
        m_jumpTimer = 0.0f;
        m_shootTimer = 0.0f;
        m_jumpCooldown = 0.0f;
        m_shootCooldown = 0.0f;
        
        if (m_playerEntity != 0) {
            // Reset player position and physics
            Transform* transform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
            Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            
            if (transform) {
                transform->position = Gnosis::GNVector2(589.5f, GROUND_Y); // Centered on screen
            }
            
            if (physics) {
                physics->velocity = Gnosis::GNVector2(0.0f, 0.0f);
                physics->acceleration = Gnosis::GNVector2(0.0f, 0.0f);
            }
            
            PlayIdleAnimation();
        }
        
        GN_LOG_INFO("Player reset");
    }

    void PlayerControllerSystem::UpdatePlayerPhysics(float deltaTime) {
        if (m_playerEntity == 0) {
            return;
        }

        Transform* transform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        
        if (!transform || !physics) {
            return;
        }

        // FREEZE PHYSICS DURING ANIMATIONS to prevent position jumping
        // Only update physics if we're in idle state (not animating)
        bool isAnimating = (sprite && sprite->isAnimated && sprite->playing && m_currentAnimation != m_idleAnimation);
        
        if (isAnimating) {
            // During animations, only apply gravity but don't update position
            if (physics->useGravity) {
                physics->acceleration.y = 800.0f; // Gravity force
            }
            
            // Update velocity for physics simulation
            physics->velocity += physics->acceleration * deltaTime;
            physics->velocity = physics->velocity * physics->drag;
            
            // DON'T update transform position during animations
            // This prevents the sprite from jumping between frames
            
            // Reset acceleration
            physics->acceleration = Gnosis::GNVector2(0.0f, 0.0f);
            return;
        } else if (sprite && sprite->isAnimated && !sprite->playing && m_currentAnimation != m_idleAnimation) {
            // Animation just finished - reset velocity to prevent position jumps
            physics->velocity = Gnosis::GNVector2(0.0f, 0.0f);
            physics->acceleration = Gnosis::GNVector2(0.0f, 0.0f);
            GN_LOG_INFO("Animation finished, resetting physics velocity to prevent position jump");
        }

        // Normal physics update when not animating
        // Apply gravity
        if (physics->useGravity) {
            physics->acceleration.y = 800.0f; // Gravity force
        }

        // Update velocity
        physics->velocity += physics->acceleration * deltaTime;
        
        // Apply drag
        physics->velocity = physics->velocity * physics->drag;
        
        // Update position
        transform->position += physics->velocity * deltaTime;
        
        // Check ground collision
        if (transform->position.y >= GROUND_Y) {
            transform->position.y = GROUND_Y;
            physics->velocity.y = 0.0f;
            m_isGrounded = true;
            m_isJumping = false;
        }
        
        // Reset acceleration
        physics->acceleration = Gnosis::GNVector2(0.0f, 0.0f);
    }

    void PlayerControllerSystem::UpdatePlayerAnimation(float deltaTime) {
        if (m_playerEntity == 0 || !m_spriteSystem) {
            return;
        }

        // Check if current animation has finished (for non-looping animations)
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        if (sprite && sprite->isAnimated && !sprite->playing && m_currentAnimation != m_idleAnimation) {
            // Animation finished, return to idle
            GN_LOG_INFO("Animation finished, returning to idle: " + m_currentAnimation);
            ChangePlayerAnimation(m_idleAnimation);
            return;
        }
        
        // Also check if we're in an animation state but the sprite is not playing
        if (sprite && m_currentAnimation != m_idleAnimation && !sprite->playing) {
            GN_LOG_INFO("Animation stopped playing, returning to idle: " + m_currentAnimation);
            ChangePlayerAnimation(m_idleAnimation);
            return;
        }

        // Determine which animation to play based on state
        std::string targetAnimation = m_idleAnimation;
        
        if (m_isShooting && m_shootTimer > 0.0f) {
            targetAnimation = m_shootAnimation;
            GN_LOG_DEBUG("PlayerController: Shooting animation requested, timer=" + std::to_string(m_shootTimer));
        } else if (m_isJumping && m_jumpTimer > 0.0f) {
            targetAnimation = m_jumpAnimation;
            GN_LOG_DEBUG("PlayerController: Jump animation requested, timer=" + std::to_string(m_jumpTimer));
        } else {
            targetAnimation = m_idleAnimation;
            GN_LOG_DEBUG("PlayerController: Idle animation requested");
        }
        
        // Change animation if needed
        if (targetAnimation != m_currentAnimation) {
            GN_LOG_INFO("Animation state change: " + m_currentAnimation + " -> " + targetAnimation);
            ChangePlayerAnimation(targetAnimation);
        }
    }

    void PlayerControllerSystem::UpdatePlayerState(float deltaTime) {
        // Update shooting state
        if (m_isShooting && m_shootTimer <= 0.0f) {
            m_isShooting = false;
        }
        
        // Update jumping state
        if (m_isJumping && m_jumpTimer <= 0.0f) {
            m_isJumping = false;
        }
    }

    void PlayerControllerSystem::HandleCollisions() {
        // TODO: Implement collision detection with obstacles, enemies, pickups
        // This will be expanded in Phase 2 with the CollisionSystem
    }

    void PlayerControllerSystem::SpawnProjectile() {
        if (m_playerEntity == 0) {
            return;
        }

        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        if (!playerTransform) {
            return;
        }

        // Create projectile entity
        Gnosis::Entity projectileEntity = m_ecsSystem->CreateEntity();
        
        // Add transform component
        Transform projectileTransform(playerTransform->position + Gnosis::GNVector2(50.0f, 0.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(projectileEntity, projectileTransform);
        
        // Add physics component
        Physics projectilePhysics;
        projectilePhysics.velocity = Gnosis::GNVector2(300.0f, 0.0f); // Move right
        projectilePhysics.useGravity = false;
        m_ecsSystem->AddComponent<Physics>(projectileEntity, projectilePhysics);
        
        // Add sprite component (placeholder)
        Sprite projectileSprite("projectile", 16.0f, 16.0f);
        projectileSprite.color = Gnosis::GNColor(255, 255, 0, 255); // Yellow
        m_ecsSystem->AddComponent<Sprite>(projectileEntity, projectileSprite);
        
        // Add collider component
        Collider projectileCollider;
        projectileCollider.type = ColliderType::Circle;
        projectileCollider.radius = 8.0f;
        projectileCollider.tag = "projectile";
        m_ecsSystem->AddComponent<Collider>(projectileEntity, projectileCollider);
        
        // Add projectile component
        Projectile projectileData;
        projectileData.damage = 1;
        projectileData.speed = 300.0f;
        projectileData.lifetime = 3.0f;
        m_ecsSystem->AddComponent<Projectile>(projectileEntity, projectileData);
        
        // Add lifetime component
        Lifetime lifetime(3.0f);
        m_ecsSystem->AddComponent<Lifetime>(projectileEntity, lifetime);
        
        GN_LOG_INFO("Spawned projectile entity: %d", projectileEntity);
    }

    void PlayerControllerSystem::PlayIdleAnimation() {
        ChangePlayerAnimation(m_idleAnimation);
    }

    void PlayerControllerSystem::PlayJumpAnimation() {
        ChangePlayerAnimation(m_jumpAnimation);
    }

    void PlayerControllerSystem::PlayShootAnimation() {
        ChangePlayerAnimation(m_shootAnimation);
    }

    void PlayerControllerSystem::PlayHurtAnimation() {
        ChangePlayerAnimation(m_hurtAnimation);
    }

} // namespace GameCore 