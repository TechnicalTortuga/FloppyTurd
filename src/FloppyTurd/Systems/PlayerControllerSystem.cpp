#include "PlayerControllerSystem.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Utility/Utils.h"
#include "../../Engine/Platform/HapticHelpers.h"
#include <algorithm>
#include <cmath>

namespace GameCore {

    PlayerControllerSystem::PlayerControllerSystem(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates, SpriteSystem* spriteSystem, ProjectileSystem* projectileSystem, HatsSystem* hatsSystem, SkillSystem* skillSystem, int currentLevelId, const LevelConfig* levelConfig)
        : m_ecsSystem(ecsSystem)
        , m_renderSystem(nullptr)
        , m_platformDelegates(platformDelegates)
        , m_spriteSystem(spriteSystem)
        , m_projectileSystem(projectileSystem)
        , m_hatsSystem(hatsSystem)
        , m_skillSystem(skillSystem)
        , m_playerEntity(0)
        , m_hatSpriteEntity(0)
        , m_playerAlive(true)
        , m_currentLevelId(currentLevelId)
        , m_levelConfig(levelConfig)
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
            
            // Check for auto-jump after 150ms
            if (holdDurationMs >= 150) { // AUTO_JUMP_THRESHOLD converted to milliseconds
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

        // DIAGNOSTIC: Check for duplicate hat entities (only log once every 60 frames)
        static int frameCount = 0;
        if (++frameCount >= 60) {
            frameCount = 0;
            if (m_ecsSystem && m_hatSpriteEntity != 0) {
                Sprite* hatSprite = m_ecsSystem->GetComponent<Sprite>(m_hatSpriteEntity);
                if (hatSprite && hatSprite->visible) {
                    GN_LOG_INFO("🎩 HAT DIAGNOSTIC: Entity " + std::to_string(m_hatSpriteEntity) + " | Layer: " + std::to_string(hatSprite->layer) + " | Visible: " + std::to_string(hatSprite->visible) + " | Texture: " + hatSprite->textureId + " | Frame: " + std::to_string(hatSprite->currentFrame) + "/" + std::to_string(hatSprite->frameCount) + " | Animated: " + std::to_string(hatSprite->isAnimated) + " | Playing: " + std::to_string(hatSprite->playing) + " | FrameW/H: " + std::to_string(hatSprite->frameWidth) + "/" + std::to_string(hatSprite->frameHeight) + " | W/H: " + std::to_string((int)hatSprite->width) + "/" + std::to_string((int)hatSprite->height));
                    
                    // SANITY CHECK: If playing=false and hasCompleted=true, FORCE frame to 0 (idle state)
                    if (!hatSprite->playing && hatSprite->hasCompleted && hatSprite->currentFrame != 0) {
                        GN_LOG_ERROR("🚨 HAT BUG DETECTED: currentFrame=" + std::to_string(hatSprite->currentFrame) + " but should be 0! FORCING to 0");
                        hatSprite->currentFrame = 0;
                        hatSprite->currentFrameTime = 0.0f;
                    }
                }
            }
        }
        
        // Update player state
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
        GN_LOG_INFO("PlayerControllerSystem::HandleTouchInput called with (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(isJustPressed) +
                   ") level: " + std::to_string(m_currentLevelId) + " boss: " + std::to_string(m_currentLevelId == 6));

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

                // Check if shooting is enabled for this level
                bool shootingEnabled = m_levelConfig ? m_levelConfig->shootingEnabled : (m_currentLevelId != 1);
                GN_LOG_INFO("PlayerController: Shooting enabled check - level: " + std::to_string(m_currentLevelId) +
                           ", shootingEnabled: " + std::to_string(shootingEnabled) +
                           ", levelConfig exists: " + std::to_string(m_levelConfig != nullptr) +
                           ", config shootingEnabled: " + std::to_string(m_levelConfig ? m_levelConfig->shootingEnabled : false));

                if (!shootingEnabled) {
                    // LEVEL WITH NO SHOOTING: Whole screen is jump zone
                    GN_LOG_INFO("Level " + std::to_string(m_currentLevelId) + " has no shooting - whole screen jump zone!");
                    m_jumpButtonHeld = true;
                    m_jumpHoldTime = 0.0f;
                } else {
                    // OTHER LEVELS: Use adjusted shooting zone in bottom right
                    GN_LOG_INFO("Level " + std::to_string(m_currentLevelId) + " has shooting enabled - using shooting zone");
                // Use normalized coordinates (0.0 to 1.0)
                GN_LOG_INFO("Other level - Touch press at normalized (" + std::to_string(x) + ", " + std::to_string(y) + ")");

                // DEBUG: Check coordinate range
                GN_LOG_INFO("DEBUG: Coordinate range check - x=" + std::to_string(x) + " (0.0-1.0?), y=" + std::to_string(y) + " (0.0-1.0?)");

                // Calculate shooting zone boundaries dynamically based on screen dimensions
                // CRITICAL FIX: Use ECS screen dimensions instead of platform delegates
                float screenWidth, screenHeight;
                bool screenDimensionsValid = false;
                
                // Use RenderSystem screen dimensions directly (same as GameplayState) for consistency
                if (m_renderSystem) {
                    const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
                    screenWidth = screenInfo.pixelWidth;
                    screenHeight = screenInfo.pixelHeight;
                    screenDimensionsValid = (screenWidth > 0 && screenHeight > 0);
                    GN_LOG_INFO("PlayerController: Using RenderSystem screen dimensions: " +
                               std::to_string((int)screenWidth) + "x" + std::to_string((int)screenHeight) +
                               " valid=" + std::to_string(screenDimensionsValid));
                } else if (m_ecsSystem) {
                    // Fallback to ECS dimensions
                    screenWidth = m_ecsSystem->GetScreenWidth();
                    screenHeight = m_ecsSystem->GetScreenHeight();
                    screenDimensionsValid = (screenWidth > 0 && screenHeight > 0);
                    GN_LOG_INFO("PlayerController: Using ECS screen dimensions (fallback): " +
                               std::to_string((int)screenWidth) + "x" + std::to_string((int)screenHeight) +
                               " valid=" + std::to_string(screenDimensionsValid));
                }
                
                if (!screenDimensionsValid) {
                    // Fallback to platform delegates only if ECS fails
                    screenWidth = 1179.0f; // Default fallback
                    screenHeight = 2556.0f; // Default fallback
                    ScreenInfo screenInfo;
                    
                    if (m_platformDelegates && m_platformDelegates->renderer.getScreenInfo) {
                        m_platformDelegates->renderer.getScreenInfo(&screenInfo);
                        screenWidth = screenInfo.pixelWidth;
                        screenHeight = screenInfo.pixelHeight;
                        GN_LOG_INFO("PlayerController: Using platform delegate screen dimensions: " +
                                   std::to_string((int)screenWidth) + "x" + std::to_string((int)screenHeight));
                    } else {
                        GN_LOG_WARN("PlayerController: Using hardcoded fallback screen dimensions: 1179x2556");
                    }
                }
                
                // Create screenInfo for orientation detection
                ScreenInfo screenInfo;
                screenInfo.pixelWidth = screenWidth;
                screenInfo.pixelHeight = screenHeight;
                screenInfo.isPortrait = (screenWidth < screenHeight);

                // Determine if we're in landscape mode using the same method as other systems
                // This ensures consistency with ScreenInfo.isPortrait from MetalRenderer
                bool isLandscape = (screenInfo.pixelWidth > screenInfo.pixelHeight);
                GN_LOG_INFO("PlayerController: Orientation detection - pixelWidth: " + std::to_string(screenInfo.pixelWidth) +
                           ", pixelHeight: " + std::to_string(screenInfo.pixelHeight) +
                           ", isLandscape: " + std::to_string(isLandscape) +
                           ", isPortrait flag: " + std::to_string(screenInfo.isPortrait) +
                           ", level: " + std::to_string(m_currentLevelId));

                // Shooting zone boundaries (pixel-based) - must match GameplayState positioning exactly
                // Use orientation-specific positioning that matches GameplayState RepositionUIElements functions
                float shootingZoneLeftX, shootingZoneTopY, shootingZoneBottomY, shootingZoneRightX;
                
                // EXACT SAME CALCULATIONS as GameplayState::CreateUI() and RepositionUIElements*()
                // Use the exact same constants and positioning logic
                const float COINBAG_X_PERCENT = 0.01f; // Both portrait and landscape use 0.01f
                const float PORTRAIT_COINBAG_Y_PERCENT = 0.87f; // Match GameplayState - 87% from top
                const float LANDSCAPE_COINBAG_Y_PERCENT = 0.55f;
                const float COINBAG_SCALE = 8.0f;
                const float COINBAG_WIDTH = 32.0f * COINBAG_SCALE;
                const float COUNTER_GAP = 8.0f;
                const float ESTIMATED_COUNTER_WIDTH = 200.0f;
                const float SHOOTING_ZONE_TOP_PERCENT = 0.80f; // Match GameplayState updated position (80%)
                const float SHOOTING_ZONE_BOTTOM_PERCENT = 0.95f; // Match GameplayState updated position (95%)
                const float SHOOTING_ZONE_RIGHT_PERCENT = 0.95f; // Exact same as GameplayState

                float coinBagX = screenWidth * COINBAG_X_PERCENT;
                float coinBagY = screenHeight * (isLandscape ? LANDSCAPE_COINBAG_Y_PERCENT : PORTRAIT_COINBAG_Y_PERCENT);
                float coinCounterX = coinBagX + COINBAG_WIDTH + COUNTER_GAP;
                float coinCounterRightX = coinCounterX + ESTIMATED_COUNTER_WIDTH;

                shootingZoneLeftX = coinCounterRightX + COUNTER_GAP; // Exact same 8.0f gap
                shootingZoneTopY = screenHeight * SHOOTING_ZONE_TOP_PERCENT;
                shootingZoneBottomY = screenHeight * SHOOTING_ZONE_BOTTOM_PERCENT;
                shootingZoneRightX = screenWidth * SHOOTING_ZONE_RIGHT_PERCENT;

                // Convert to normalized coordinates for comparison
                float shootingZoneLeftXNorm = shootingZoneLeftX / screenWidth;
                float shootingZoneTopYNorm = shootingZoneTopY / screenHeight;
                float shootingZoneRightXNorm = shootingZoneRightX / screenWidth;
                float shootingZoneBottomYNorm = shootingZoneBottomY / screenHeight;

                GN_LOG_INFO("SHOOTING ZONE BOUNDARIES (" + std::string(isLandscape ? "LANDSCAPE" : "PORTRAIT") + "):");
                GN_LOG_INFO("  Left: " + std::to_string(shootingZoneLeftXNorm) + ", Right: " + std::to_string(shootingZoneRightXNorm));
                GN_LOG_INFO("  Top: " + std::to_string(shootingZoneTopYNorm) + ", Bottom: " + std::to_string(shootingZoneBottomYNorm));
                GN_LOG_INFO("  Touch at: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                GN_LOG_INFO("  Pixel boundaries - Left: " + std::to_string(shootingZoneLeftX) + ", Right: " + std::to_string(shootingZoneRightX) +
                           ", Top: " + std::to_string(shootingZoneTopY) + ", Bottom: " + std::to_string(shootingZoneBottomY));

                // Check if touch is within shooting zone boundaries
                bool inShootZone = (x >= shootingZoneLeftXNorm) && (x <= shootingZoneRightXNorm) &&
                                  (y >= shootingZoneTopYNorm) && (y <= shootingZoneBottomYNorm);

                GN_LOG_INFO("SHOOT ZONE CHECK: touch(" + std::to_string(x) + "," + std::to_string(y) + ") vs zone(" +
                           std::to_string(shootingZoneLeftXNorm) + "," + std::to_string(shootingZoneTopYNorm) + "," +
                           std::to_string(shootingZoneRightXNorm) + "," + std::to_string(shootingZoneBottomYNorm) + ") = " +
                           std::to_string(inShootZone) + " (level: " + std::to_string(m_currentLevelId) + ")");


                if (inShootZone) {
                    // Bottom right area - shoot immediately on press
                    GN_LOG_INFO("Shoot zone pressed at (" + std::to_string(x) + ", " + std::to_string(y) + ")! Calling HandleShootInput...");
                    HandleShootInput();
                    m_touchSession.active = false; // Shooting doesn't use hold mechanics
                } else {
                    // Left side or upper area - START jump hold tracking
                    GN_LOG_INFO("Jump zone pressed at (" + std::to_string(x) + ", " + std::to_string(y) + ") - starting hold timer!");
                    m_jumpButtonHeld = true;
                    m_jumpHoldTime = 0.0f;
                }
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
                
                // Trigger haptic feedback for jump
                if (m_platformDelegates) {
                    HapticHelpers::TriggerJump(*m_platformDelegates);
                }
                
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

        GN_LOG_INFO("Shooting conditions met, checking coin cost...");

        // Check if we're in the input delay period (prevent accidental shooting at game start)
        if (m_inputDelayTimer > 0.0f) {
            GN_LOG_DEBUG("Shoot input ignored - in input delay period (%.2fs remaining)", m_inputDelayTimer);
            return;
        }

        // Check if player has enough session coins to shoot (costs 1 coin per shot)
        PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
        if (!player || player->sessionCoins < 1) {
            GN_LOG_INFO("Shoot blocked - insufficient session coins (have: " + 
                       std::to_string(player ? player->sessionCoins : 0) + ", need: 1)");
            return;
        }

        // Deduct coin cost for shooting
        player->sessionCoins -= 1;
        GN_LOG_INFO("💰 Shooting cost 1 coin - sessionCoins now: " + std::to_string(player->sessionCoins));

        // Set shoot cooldown and spawn projectile - this happens ONCE per input
        m_shootCooldown = SHOOT_COOLDOWN;
        SpawnProjectile();
        
        // Trigger haptic feedback for shooting
        if (m_platformDelegates) {
            HapticHelpers::TriggerButtonPress(*m_platformDelegates);
        }

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
        if (equippedHatIndex == 0) {
            GN_LOG_DEBUG("PlayerController: No hat equipped (index: %d = unequipped), using base animation: %s", equippedHatIndex, baseAnimationName.c_str());
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
                    sprite->frameCount = 5;  // TurdletShoot spritesheet is 320x64 = 5 frames
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
                // Start player at 5% from left and near the top of screen
                float screenWidth = 1179.0f; // Default fallback
                if (m_ecsSystem) {
                    screenWidth = m_ecsSystem->GetScreenWidth();
                }
                transform->position = Gnosis::GNVector2(screenWidth * 0.05f, TOP_SPAWN_Y);
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
        
        // Apply enhanced gravity system - different rates for ascending vs falling vs death
        if (physics->useGravity) {
            if (!m_playerAlive) {
                // Extra heavy gravity on death for fast fall to game over screen
                physics->acceleration.y = GRAVITY_DEATH;
                GN_LOG_TRACE("Applying death gravity: " + std::to_string(GRAVITY_DEATH));
            } else if (m_isAscending) {
                // Interpolate gravity from GRAVITY_UP to GRAVITY_DOWN as we approach apex
                // This creates a smooth transition instead of abrupt switching
                float velocityMagnitude = std::abs(physics->velocity.y);
                
                // Interpolation zone: 0-200 velocity range
                // At high velocity (200+): use GRAVITY_UP
                // At low velocity (0-50): use GRAVITY_DOWN (apex)
                // In between: smooth interpolation
                float t = 1.0f - std::min(velocityMagnitude / 200.0f, 1.0f); // 0.0 at high speed, 1.0 at low speed
                float interpolatedGravity = GRAVITY_UP + (GRAVITY_DOWN - GRAVITY_UP) * t;
                
                physics->acceleration.y = interpolatedGravity;
                GN_LOG_TRACE("Applying interpolated ascending gravity: " + std::to_string(interpolatedGravity) + " (t=" + std::to_string(t) + ")");
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
        // Use much higher terminal velocity for dead players to fall faster to game over screen
        float terminalVelocity = m_playerAlive ? TERMINAL_VELOCITY : TERMINAL_VELOCITY_DEATH;
        if (physics->velocity.y > terminalVelocity) {
            physics->velocity.y = terminalVelocity;
            GN_LOG_DEBUG("Terminal velocity reached: " + std::to_string(terminalVelocity) + (m_playerAlive ? " (alive)" : " (DEATH)"));
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
        // Boss level (6): position at percentage from left edge to avoid UI overlap
        // Other levels: use standard PLAYER_X_POSITION (centered)
        // Keep player at a fixed horizontal position based on level
        // UPDATED: Now using 5% from left edge for ALL levels (maximum screen space for gameplay)
        float screenWidth = 1179.0f; // Default fallback
        if (m_ecsSystem) {
           screenWidth = m_ecsSystem->GetScreenWidth();
        }
        
        // Position at 5% from left edge to give maximum screen space
        transform->position.x = screenWidth * 0.05f;
        
        // Get player sprite and hitbox to calculate actual size for proper boundary checking
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        Hitbox* hitbox = m_ecsSystem->GetComponent<Hitbox>(m_playerEntity);
        float playerHeight = sprite ? sprite->height * transform->scale.y : 64.0f; // Default to 64 if no sprite
        float hitboxRadius = hitbox ? hitbox->radius * transform->scale.y : 12.0f; // Default to 12 if no hitbox
        
        // Fall detection is now handled by GameplayState collision system
        // GameplayState will trigger damage, play sound, and reset position
        // This allows proper integration with invulnerability and hurt animation
        
        // Check if player hits top of screen - allow half the hitbox to go offscreen
        // Stop when center (position + half sprite height) reaches top of screen
        float playerCenterY = transform->position.y + (playerHeight * 0.5f);
        float minAllowedCenterY = hitboxRadius * 0.5f; // Allow half the hitbox radius above screen
        
        if (playerCenterY <= minAllowedCenterY) {
            if (m_playerAlive) {
                // Clamp center to minimum, then calculate position from that
                transform->position.y = minAllowedCenterY - (playerHeight * 0.5f);
                physics->velocity.y = 0.0f;
                m_isAscending = false;
                GN_LOG_INFO("Player hit ceiling - alive player stopped (half hitbox offscreen allowed)");
            } else {
                // Dead player can fall through ceiling - just prevent center from going too far above screen
                transform->position.y = minAllowedCenterY - (playerHeight * 0.5f);
                // Don't reset velocity - let them fall naturally
                GN_LOG_INFO("Player hit ceiling - dead player continues falling");
            }
        }
        
        // NOTE: Off-screen death check is handled in GameplayState::CheckToiletCollisions()
        // to avoid needing ConfigManager here
        
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
        // Adjusted 8px left and 8px up from original offset for better visual alignment
        Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        float playerScale = playerTransform->scale.x; // Assuming uniform scaling
        float scaledOffsetX = 34.0f * playerScale;  // 42.0 - 8.0 (8px left)
        float scaledOffsetY = 24.0f * playerScale;  // 32.0 - 8.0 (8px up)
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
            // Play random spit sound (spit1 - spit6)
            int randomSpit = (rand() % 6) + 1;  // Random number 1-6
            std::string spitSound = "spit" + std::to_string(randomSpit);
            if (GameCore::GetGame()) {
                GameCore::GetGame()->PlaySFX(spitSound);
            }
            
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
        // Trigger haptic feedback for player getting hurt
        if (m_platformDelegates) {
            HapticHelpers::TriggerCollision(*m_platformDelegates, 0.8f);
        }
        
        TransitionToState(PlayerAnimationState::HURT);
    }

    void PlayerControllerSystem::ResetInputDelay(float delaySeconds) {
        m_inputDelayTimer = delaySeconds;
        GN_LOG_INFO("Input delay timer reset to %.2fs", delaySeconds);
    }

    void PlayerControllerSystem::CreateHatSprite() {
        if (m_hatSpriteEntity != 0) {
            // Hat sprite already exists - clean it up first
            GN_LOG_ERROR("⚠️ PlayerController: Hat sprite ALREADY EXISTS (entity " + std::to_string(m_hatSpriteEntity) + ")! This should not happen. Cleaning up before creating new one.");
            HideHatSprite();
            if (m_ecsSystem) {
                m_ecsSystem->DestroyEntity(m_hatSpriteEntity);
            }
            m_hatSpriteEntity = 0;
        } else {
            GN_LOG_INFO("PlayerController: Creating NEW hat sprite entity (no existing entity)");
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
        // CRITICAL: Hat sprite sheets are 384x64 (6 frames), set frameCount=6 and isAnimated=true for frame extraction
        Sprite hatSprite("TurdletIdle", 64.0f, 64.0f, 64, 64, 6, 0.1f);  // 6 frames, not 1!
        hatSprite.color = Gnosis::GNColor(255, 255, 255, 255);
        hatSprite.visible = false; // Initially hidden
        hatSprite.layer = 7; // Above player layer (6) so it renders on top
        hatSprite.isAnimated = true;  // TRUE - needed for sprite sheet frame extraction
        hatSprite.playing = false;  // FALSE - don't play animation
        hatSprite.loop = false;
        hatSprite.frameCount = 6;  // Actual sprite sheet has 6 frames
        hatSprite.currentFrame = 0;  // Show only frame 0
        hatSprite.currentFrameTime = 0.0f;
        hatSprite.hasCompleted = true;  // CRITICAL: Mark as complete to prevent auto-advancement

        m_ecsSystem->AddComponent<Sprite>(m_hatSpriteEntity, hatSprite);

        GN_LOG_INFO("PlayerController: Created hat sprite entity " + std::to_string(m_hatSpriteEntity) + " with width=" + std::to_string(hatSprite.width) + " height=" + std::to_string(hatSprite.height) + " frameW=" + std::to_string(hatSprite.frameWidth) + " frameH=" + std::to_string(hatSprite.frameHeight) + " frameCount=" + std::to_string(hatSprite.frameCount) + " currentFrame=" + std::to_string(hatSprite.currentFrame) + " hasCompleted=" + std::to_string(hatSprite.hasCompleted));
        
        // Apply equipped hat texture immediately at level start
        UpdateHatSpriteTexture("TurdletIdle");
        GN_LOG_INFO("PlayerController: Applied initial hat texture for TurdletIdle animation");
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
        if (equippedHatIndex == 0) {
            // No hat equipped (index 0 = unequipped), hide hat sprite
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
            // CRITICAL: Invalidate texture cache when changing textures
            if (hatSprite->textureId != hatTexture) {
                hatSprite->textureHandleValid = false;
                hatSprite->cachedTextureId = "";
                GN_LOG_INFO("PlayerController: Invalidating texture cache - switching from '" + hatSprite->textureId + "' to '" + hatTexture + "'");
            }
            
            hatSprite->textureId = hatTexture;
            hatSprite->visible = true;
            
            // CRITICAL: Hat textures are horizontal sprite sheets (6 frames * 64x64 = 384x64)
            // Set frame dimensions to extract single 64x64 frames from the sheet
            hatSprite->frameWidth = 64;
            hatSprite->frameHeight = 64;
            hatSprite->width = 64.0f;
            hatSprite->height = 64.0f;

            // Configure hat sprite animation properties
            if (animationName == "TurdletIdle") {
                // For idle, show ONLY first frame (frame 0) of the jump sprite sheet
                // CRITICAL: Must set isAnimated=TRUE to use sprite sheet frame extraction logic!
                // SpriteSystem only calculates frame positions when isAnimated=true
                hatSprite->frameCount = 6;  // Actual sprite sheet has 6 frames
                hatSprite->currentFrame = 0;  // Show ONLY frame 0
                hatSprite->isAnimated = true;  // TRUE - enables sprite sheet frame extraction
                hatSprite->playing = false;  // FALSE - prevents animation from playing
                hatSprite->loop = false;
                hatSprite->frameTime = 0.1f;
                hatSprite->currentFrameTime = 0.0f;
                hatSprite->hasCompleted = true;  // Mark as complete so SpriteSystem doesn't advance frames
                GN_LOG_INFO("PlayerController: ⚠️ Hat IDLE - texture: " + hatTexture + " | frameCount: " + std::to_string(hatSprite->frameCount) + " | currentFrame: " + std::to_string(hatSprite->currentFrame) + " | isAnimated: " + std::to_string(hatSprite->isAnimated) + " | playing: " + std::to_string(hatSprite->playing) + " | frameW/H: " + std::to_string(hatSprite->frameWidth) + "/" + std::to_string(hatSprite->frameHeight) + " | width/height: " + std::to_string((int)hatSprite->width) + "/" + std::to_string((int)hatSprite->height));
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
                // For shoot, use full 5-frame animation (TurdletShoot spritesheet is 320x64 = 5 frames)
                hatSprite->frameCount = 5;
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