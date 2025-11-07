#include "GameplayState.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../Config/EnemyConfigs.h"
#include "../../Engine/Utility/Utils.h"
#include "../../Engine/Platform/HapticHelpers.h"
#include <algorithm>
#include <set>
#include "../Game/FloppyTurdGame.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include "../Input/InputManager.h"

namespace GameCore {

    GameplayState::GameplayState(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates, int levelId)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_currentLevelId(levelId)
        , m_currentLevelConfig(LevelConfigFactory::GetLevelConfig(levelId))
        , m_cachedScreenWidth(1179.0f)  // Default iPhone 16 width - will be updated in Enter()
        , m_cachedScreenHeight(2556.0f) // Default iPhone 16 height - will be updated in Enter()
        , m_currentScore(0)
        , m_currentLives(STARTING_LIVES)
        , m_gameTime(0.0f)
        , m_difficultyTimer(0.0f)
        , m_difficultyLevel(1.0f)
        , m_playerAlive(true)
        , m_invulnerabilityTimer(0.0f)
        , m_pipesCleared(0)
        , m_sessionCoinsCollected(0)
        , m_pipeIncrementCooldown(0.0f)
        , m_finished(false)
        , m_levelCompleted(false)
        , m_currentSubState(GameplaySubState::Playing)
        , m_gameOverTimer(0.0f)
        , m_morteFloatOffset(0.0f)
        , m_pipesLabelEntity(0)
        , m_coinsLabelEntity(0)
        , m_frameProfiler("GameplayState")
        , m_profilingEnabled(false)
        , m_obstacleSpawnTimer(0.0f)
        , m_pickupSpawnTimer(0.0f)
        , m_enemySpawnTimer(0.0f)
        , m_bossCoinSpawnTimer(0.0f)
        , m_bossRainbowHeartSpawned(false)
        , m_inputDelayTimer(0.0f)
        , m_lastSettingsButtonPressTime(0.0f)
        , m_settingsButtonDebounceDelay(0.1f)  // 100ms debounce delay for better responsiveness
        , m_shootingZoneEntity(0)
        , m_debugButtonRect(0)
        , m_debugShootingZoneRect(0)
    {
        GN_LOG_INFO("GameplayState created for level: " + std::to_string(levelId) + " (" + m_currentLevelConfig.levelName + ")");
    }

    GameplayState::~GameplayState() {
        GN_LOG_INFO("GameplayState destroyed");
        
        // CRITICAL: Disconnect OverlaySystem from RenderSystem before it's destroyed
        // This prevents dangling pointer bugs where MainMenuState tries to render deleted overlay
        if (m_renderSystem && m_overlaySystem) {
            m_renderSystem->SetOverlaySystem(nullptr);
            GN_LOG_INFO("GameplayState: Disconnected OverlaySystem from RenderSystem");
        }
        
        // CRITICAL: Clear the ConfigManager callback to prevent crash on orientation change
        // If we don't do this, ConfigManager will try to call our methods after we're destroyed!
        ConfigManager::Instance().SetScreenInfoUpdateCallback(nullptr);
        GN_LOG_INFO("GameplayState: Cleared ConfigManager callback");
    }

    void GameplayState::Enter() {
        GN_LOG_INFO("Entering GameplayState for level: " + std::to_string(m_currentLevelId));
        
        // Initialize systems (will be implemented in Phase 2)
        InitializeSystems();

        // TEMP: Enable gameplay frame profiling for performance investigation
        SetProfilingEnabled(true);

        // InputManager singleton is initialized by FloppyTurdGame

        // Cache screen dimensions once (eliminates 40+ repeated GetScreenInfo() calls)
        CacheScreenDimensions();

        // Register callback for screen info updates (orientation changes)
        RegisterScreenInfoCallback();
        
        // Create game entities
        CreateGameEntities();
        
    // Create UI
    CreateUI();

    // Ensure UI is positioned correctly for current orientation (critical for Boss level landscape)
    UpdateUILayoutForOrientation();

    // Reset game state
        m_currentScore = 0;
        m_currentLives = STARTING_LIVES;
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        m_pipesCleared = 0;
        m_sessionCoinsCollected = 0; // Reset session coins at start of session
        m_pipeIncrementCooldown = 0.0f;
        m_finished = false;
        m_levelCompleted = false;

        // Reset player session coins at start of new session
        if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
            player->sessionCoins = 0;
            GN_LOG_INFO("💰 Reset PlayerComponent::sessionCoins to 0 for new session");
        }

        // Reset player input delay to prevent accidental shooting at game start
        if (m_playerControllerSystem) {
            m_playerControllerSystem->ResetInputDelay(0.4f);
        }

        
        // Reset spawn timers
        m_obstacleSpawnTimer = 0.0f;
        m_pickupSpawnTimer = 0.0f;
        m_enemySpawnTimer = 0.0f;
        m_bossCoinSpawnTimer = 0.0f;
        m_bossRainbowHeartSpawned = false;
        m_activeBossCoins.clear();
        
        // Reset boss level timer (Level 6 only)
        m_bossLevelTimer = 0.0f;
        m_bossCompletionTime = 0.0f;
        m_bossDying = false;
        m_bossDefeated = false;
        
        // Reset input delay timer to prevent immediate input processing
        m_inputDelayTimer = 0.0f;
        
        // Clear any lingering input commands to prevent auto-shooting when entering level
        if (m_platformDelegates && m_platformDelegates->input.clearInputBuffer) {
            m_platformDelegates->input.clearInputBuffer();
            GN_LOG_INFO("GameplayState: Cleared input buffer to prevent lingering touch inputs");
        }
        
        // Start level music based on current difficulty
        StartLevelMusic();
        
        // Create pause system for handling all pause menu functionality
        m_pauseSystem = std::make_unique<PauseSystem>(m_ecsSystem, *m_platformDelegates, this);

        // Update pause system with current screen dimensions
        m_pauseSystem->UpdateScreenDimensions(m_cachedScreenWidth, m_cachedScreenHeight);

        GN_LOG_INFO("PauseSystem initialized with screen dimensions: " + std::to_string((int)m_cachedScreenWidth) + "x" + std::to_string((int)m_cachedScreenHeight));
        GN_LOG_INFO("GameplayState entered successfully");
    }

    void GameplayState::Exit() {
        GN_LOG_INFO("Exiting GameplayState");

        // NOTE: Orientation handling is now done by ScreenPromptState
        // Don't automatically lock orientation here - let ScreenPromptState handle it

        // Stop level music
        StopLevelMusic();

        // Save game progress
        SaveGameProgress();
        
        // Clean up game over UI if active
        if (m_currentSubState == GameplaySubState::GameOver) {
            DestroyGameOverUI();
        }
        
        // Clean up entities
        DestroyGameEntities();
        DestroyUI();
        
        // Clean up pause menu system
        if (m_pauseSystem) {
            m_pauseSystem->Cleanup();
        }
        
        GN_LOG_INFO("GameplayState exited");
    }

    void GameplayState::Pause() {
        TriggerPause();
    }

    void GameplayState::Resume() {
        TriggerResume();
    }

    void GameplayState::SetProfilingEnabled(bool enabled) {
        m_profilingEnabled = enabled;
        m_frameProfiler.SetEnabled(enabled);
        GN_LOG_INFO(std::string("GameplayState profiling ") + (enabled ? "enabled" : "disabled"));
    }

    void GameplayState::Update(float deltaTime) {
        m_frameProfiler.BeginFrame();

        // Update InputManager singleton (handles all input processing)
        m_frameProfiler.StartSection("InputManager");
        InputManager* inputManager = InputManager::GetInstance();
        if (inputManager) {
            inputManager->Update(deltaTime);
        }
        m_frameProfiler.EndSection("InputManager");

        // Update overlay system (snowfall animation, etc.)
        if (m_overlaySystem) {
            m_overlaySystem->Update(deltaTime);
        }
        
        // Update button debounce timers
        m_lastSettingsButtonPressTime += deltaTime;

        // BACKUP: Poll for orientation changes since callback system may not work reliably
        static bool s_lastOrientationBackup = IsLandscapeMode();
        bool currentOrientationBackup = IsLandscapeMode();
        if (currentOrientationBackup != s_lastOrientationBackup) {
            // Get screen info to check if rotation is complete
            if (m_renderSystem) {
                const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
                
                // CRITICAL: Only reposition if rotation is complete (MTKView has updated)
                if (!screenInfo.isOrientationChanging) {
                    std::string fromOrient = s_lastOrientationBackup ? "landscape" : "portrait";
                    std::string toOrient = currentOrientationBackup ? "landscape" : "portrait";
                    GN_LOG_INFO("🔄 BACKUP: Orientation change COMPLETE: " + fromOrient + " → " + toOrient + " - repositioning UI");

                    // Update cached screen dimensions
                    m_cachedScreenWidth = screenInfo.pixelWidth;
                    m_cachedScreenHeight = screenInfo.pixelHeight;

                    GN_LOG_INFO("Updated cached screen dimensions: " +
                               std::to_string((int)m_cachedScreenWidth) + "x" +
                               std::to_string((int)m_cachedScreenHeight));

                    // Reposition UI elements for new orientation
                    UpdateUILayoutForOrientation();

                    // Update boss system if active
                    if (m_bossSystem && m_currentLevelId == 6) { // Level 6 is boss level
                        m_bossSystem->UpdateScreenDimensions(m_cachedScreenWidth, m_cachedScreenHeight);
                        GN_LOG_INFO("Updated boss system screen dimensions");
                    }

                    // Update pause system
                    if (m_pauseSystem) {
                        m_pauseSystem->UpdateScreenDimensions(m_cachedScreenWidth, m_cachedScreenHeight);
                        GN_LOG_INFO("Updated pause system screen dimensions");
                    }

                    s_lastOrientationBackup = currentOrientationBackup;
                } else {
                    GN_LOG_INFO("🔒 BACKUP: Orientation change in progress, waiting for MTKView to complete rotation");
                }
            }
        }

        // Handle different sub-states
        m_frameProfiler.StartSection("UpdateSubState");
        UpdateSubState(deltaTime);
        m_frameProfiler.EndSection("UpdateSubState");
        
        // Only update game time when playing
        if (m_currentSubState == GameplaySubState::Playing) {
            // Update game time
            m_gameTime += deltaTime;
        
        // Update input delay timer
        m_inputDelayTimer += deltaTime;

        // PlayerControllerSystem now handles hurt state transitions automatically
        // No manual hurt state management needed
        
        // Update invulnerability timer
        if (m_invulnerabilityTimer > 0.0f) {
            m_invulnerabilityTimer -= deltaTime;
            if (m_invulnerabilityTimer <= 0.0f) {
                GN_LOG_INFO("Player invulnerability ended");
            }
        }

            if (m_inputDelayTimer >= INPUT_DELAY_TIME) {
                m_frameProfiler.StartSection("HandleInput");
                HandleInput();
                m_frameProfiler.EndSection("HandleInput");
            }

            // Always handle settings button input (pause menu) regardless of delay
            m_frameProfiler.StartSection("HandleSettingsButtonInput");
            HandleSettingsButtonInput();
            m_frameProfiler.EndSection("HandleSettingsButtonInput");

            if (m_cameraSystem && m_currentSubState != GameplaySubState::Paused) {
                m_frameProfiler.StartSection("CameraSystem");
                m_cameraSystem->Update(deltaTime);
                m_frameProfiler.EndSection("CameraSystem");
            }

            // Update UI system for menu button rendering
            if (m_uiSystem) {
                m_frameProfiler.StartSection("UISystem");
                m_uiSystem->Update(deltaTime);
                m_frameProfiler.EndSection("UISystem");
            }

            // Update heart system for health display (only when playing)
            if (m_heartSystem && m_currentSubState == GameplaySubState::Playing) {
                m_frameProfiler.StartSection("HeartSystem");
                m_heartSystem->Update(deltaTime);
                m_frameProfiler.EndSection("HeartSystem");
            }

            // Update game logic
            m_frameProfiler.StartSection("UpdateGameLogic");
            UpdateGameLogic(deltaTime);
            m_frameProfiler.EndSection("UpdateGameLogic");

            // Update object pools FIRST (wrapping, oscillation, rotation)
            m_frameProfiler.StartSection("UpdateObjectPools");
            UpdateObjectPools(deltaTime);
            m_frameProfiler.EndSection("UpdateObjectPools");

            // Check toilet collisions and pipe clearing (after hitbox updates)
            m_frameProfiler.StartSection("CheckToiletCollisions");
            CheckToiletCollisions();
            m_frameProfiler.EndSection("CheckToiletCollisions");

            // CRITICAL: Update boss death flag (for Level 6 only)
            // This flag is used throughout to freeze all gameplay systems during boss death
            m_bossDying = (m_currentLevelId == 6 && m_bossSystem && 
                          (m_bossSystem->GetCurrentState() == RatKingState::DEATH || 
                           m_bossSystem->IsDeathSequenceComplete()));

            // Handle pickups via PickupSystem - FREEZE during boss death
            if (m_pickupSystem && !m_bossDying) {
                m_frameProfiler.StartSection("PickupSystem");
                m_pickupSystem->Update(deltaTime);
                m_frameProfiler.EndSection("PickupSystem");
            }

            // Update projectiles via ProjectileSystem - FREEZE during boss death
            if (m_projectileSystem && !m_bossDying) {
                m_frameProfiler.StartSection("ProjectileSystem");
                m_projectileSystem->Update(deltaTime);
                m_frameProfiler.EndSection("ProjectileSystem");
            }

            // Update enemy system (AFTER projectiles so collision detection has current projectile list)
            if (m_enemySystem) {
                m_frameProfiler.StartSection("EnemySystem");
                m_enemySystem->Update(deltaTime);
                m_frameProfiler.EndSection("EnemySystem");
            }

            // Update boss system (level 6 only)
            if (m_bossSystem && m_currentLevelId == 6) {
                // Set player position for boss aiming
                if (m_playerEntity != 0 && m_ecsSystem) {
                    Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
                    Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
                    Hitbox* playerHitbox = m_ecsSystem->GetComponent<Hitbox>(m_playerEntity);
                    if (playerTransform && playerSprite && playerHitbox) {
                        // Use HITBOX CENTER for aiming - this is where the player's collision actually is
                        float halfWidth = (playerSprite->width * playerTransform->scale.x) / 2.0f;
                        float halfHeight = (playerSprite->height * playerTransform->scale.y) / 2.0f;
                        
                        // Calculate hitbox center (sprite center + hitbox offset)
                        float hitboxCenterX = playerTransform->position.x + halfWidth + (playerHitbox->offsetX * playerTransform->scale.x);
                        float hitboxCenterY = playerTransform->position.y + halfHeight + (playerHitbox->offsetY * playerTransform->scale.y);
                        
                        GNVector2 playerCenter = { hitboxCenterX, hitboxCenterY };
                        m_bossSystem->SetPlayerPosition(playerCenter);
                        
                        GN_LOG_DEBUG("🎯 Player hitbox center: (" + std::to_string(hitboxCenterX) + ", " + std::to_string(hitboxCenterY) + 
                                    "), offset=(" + std::to_string(playerHitbox->offsetX) + ", " + std::to_string(playerHitbox->offsetY) + ")");
                    } else if (playerTransform) {
                        // Fallback to transform position if no sprite
                        GNVector2 playerPos = {playerTransform->position.x, playerTransform->position.y};
                        m_bossSystem->SetPlayerPosition(playerPos);
                    }
                }

                m_frameProfiler.StartSection("BossSystem");
                m_bossSystem->Update(deltaTime);
                m_frameProfiler.EndSection("BossSystem");
                
                // Update boss level timer (stop when boss starts death sequence)
                // Use m_bossDying flag
                if (!m_bossDying && m_bossCompletionTime == 0.0f) {
                    m_bossLevelTimer += deltaTime;
                }
            }

            // Update explosion system (level 6 only) - ALWAYS update even during death sequence
            if (m_explosionSystem && m_currentLevelId == 6) {
                m_frameProfiler.StartSection("ExplosionSystem");
                m_explosionSystem->Update(deltaTime);
                m_frameProfiler.EndSection("ExplosionSystem");
            }

            // Update boss health bar (level 6 only)
            if (m_bossHealthBar && m_currentLevelId == 6) {
                m_frameProfiler.StartSection("BossHealthBar");
                m_bossHealthBar->Update(deltaTime);
                m_frameProfiler.EndSection("BossHealthBar");
            }
            
            // UNCONDITIONAL LOG - MUST ALWAYS APPEAR IF CODE RUNS
            GN_LOG_INFO("🔴🔴🔴 REACHED LINE 397 IN GAMEPLAYSTATE UPDATE - LEVEL: " + std::to_string(m_currentLevelId));
            
            // Spawn boss level coin groups (level 6 only)
            if (m_currentLevelId == 6) {
                GN_LOG_INFO("⚠️ BOSS LEVEL DEBUG: levelId=6, pickupSystem=" + std::to_string(m_pickupSystem != nullptr) + 
                           ", bossSystem=" + std::to_string(m_bossSystem != nullptr) + 
                           ", bossActive=" + std::to_string(m_bossSystem ? m_bossSystem->IsActive() : false));
            }
            
            if (m_currentLevelId == 6 && m_pickupSystem && m_bossSystem && m_bossSystem->IsActive()) {
                GN_LOG_INFO("⚠️ BOSS COIN SPAWNING CODE REACHED!");
                // Check for rainbow heart spawn at 50% health (once per boss fight)
                float healthPercent = static_cast<float>(m_bossSystem->GetHealth()) / static_cast<float>(m_bossSystem->GetMaxHealth());
                GN_LOG_DEBUG("Boss health: " + std::to_string(m_bossSystem->GetHealth()) + "/" + std::to_string(m_bossSystem->GetMaxHealth()) + 
                            " (" + std::to_string(healthPercent * 100.0f) + "%), rainbowSpawned=" + (m_bossRainbowHeartSpawned ? "true" : "false"));
                
                if (!m_bossRainbowHeartSpawned && healthPercent <= 0.5f) {
                    m_bossRainbowHeartSpawned = true;
                    GN_LOG_INFO("🌈 Triggering rainbow heart spawn at " + std::to_string(healthPercent * 100.0f) + "% health");
                    Gnosis::Entity rainbowHeart = m_pickupSystem->SpawnRainbowHeart(m_cachedScreenWidth, m_cachedScreenHeight);
                    if (rainbowHeart != 0) {
                        m_activeBossCoins.push_back(rainbowHeart);
                        GN_LOG_INFO("🌈 Rainbow heart spawned successfully! Entity=" + std::to_string(rainbowHeart));
                    } else {
                        GN_LOG_ERROR("🌈 Failed to spawn rainbow heart!");
                    }
                }
                
                m_bossCoinSpawnTimer += deltaTime;
                GN_LOG_INFO("⚠️ Boss coin timer: " + std::to_string(m_bossCoinSpawnTimer) + " seconds");
                
                // Spawn coin groups every 10 seconds (slower pacing for coin economy + shooting costs)
                if (m_bossCoinSpawnTimer >= 10.0f) {
                    m_bossCoinSpawnTimer = 0.0f;
                    GN_LOG_INFO("Boss coin spawn timer triggered! Spawning coin group...");
                    auto coins = m_pickupSystem->SpawnBossLevelCoinGroup(m_cachedScreenWidth, m_cachedScreenHeight);
                    if (coins.size() > 0) {
                        m_activeBossCoins.insert(m_activeBossCoins.end(), coins.begin(), coins.end());
                        GN_LOG_INFO("✅ Spawned boss coin group: " + std::to_string(coins.size()) + " coins, total active: " + std::to_string(m_activeBossCoins.size()));
                    } else {
                        GN_LOG_ERROR("❌ Failed to spawn boss coin group!");
                    }
                }
                
                // CRITICAL: Manually scroll boss coins left (boss level has no world scroll)
                // ScrollSpeed component won't work without world scrolling, so we manually update positions
                // FREEZE coins during boss death sequence (use m_bossDying flag)
                if (!m_activeBossCoins.empty() && !m_bossDying) {
                    GN_LOG_DEBUG("⚠️ Manually scrolling " + std::to_string(m_activeBossCoins.size()) + " boss coins");
                }
                
                if (!m_bossDying) {
                    for (Gnosis::Entity coinEntity : m_activeBossCoins) {
                        Transform* t = m_ecsSystem->GetComponent<Transform>(coinEntity);
                        ScrollSpeed* scrollSpeed = m_ecsSystem->GetComponent<ScrollSpeed>(coinEntity);
                        if (t && scrollSpeed) {
                            // Manually apply scroll speed (move LEFT - coins come from right)
                            t->position.x -= scrollSpeed->speed * deltaTime;
                        }
                    }
                }
                
                // Clean up off-screen boss coins from tracking (PickupSystem will destroy them)
                auto it = m_activeBossCoins.begin();
                while (it != m_activeBossCoins.end()) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(*it);
                    // Check if off-screen LEFT (coins scroll left from right)
                    if (t && t->position.x < -200.0f) {
                        // Off screen to the left, remove from tracking only
                        // PickupSystem handles entity destruction via collision or ClearAll
                        GN_LOG_DEBUG("Removing off-screen boss coin from tracking at x=" + std::to_string(t->position.x));
                        it = m_activeBossCoins.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            
            // Update white fade overlay for boss death sequence
            if (m_bossSystem && m_currentLevelId == 6 && m_whiteFadeEntity != 0) {
                float fadeAlpha = m_bossSystem->GetWhiteFadeAlpha();
                
                GN_LOG_INFO("White fade update: alpha=" + std::to_string(fadeAlpha) + 
                           ", entity=" + std::to_string(m_whiteFadeEntity) + 
                           ", deathComplete=" + std::to_string(m_bossSystem->IsDeathSequenceComplete()));
                
                if (fadeAlpha > 0.0f) {
                    UIShape* fadeShape = m_ecsSystem->GetComponent<UIShape>(m_whiteFadeEntity);
                    if (fadeShape) {
                        fadeShape->visible = true;
                        fadeShape->color.a = static_cast<uint8_t>(fadeAlpha * 255.0f);
                        
                        GN_LOG_INFO("White fade overlay updated: visible=true, alpha=" + 
                                   std::to_string(fadeShape->color.a) + 
                                   ", layer=" + std::to_string(fadeShape->layer));
                        
                        // Only set finished when fade is COMPLETE (alpha >= 1.0)
                        if (fadeAlpha >= 1.0f && m_bossSystem->IsDeathSequenceComplete()) {
                            // Store the final boss completion time if not already stored
                            if (m_bossCompletionTime == 0.0f) {
                                m_bossCompletionTime = m_bossLevelTimer;
                                GN_LOG_INFO("🏁 Boss defeated! Final time: " + std::to_string(m_bossCompletionTime) + " seconds");
                                
                                // Update high score for level 6 with the completion time
                                if (GameCore::GetGame()) {
                                    int levelId = 6;
                                    int score = m_pipesCleared;  // Pipes for score
                                    int coins = 0;  // Coins collected (if tracked)
                                    GameCore::GetGame()->UpdateLevelHighScore(levelId, score, coins, m_bossCompletionTime);
                                }
                            }
                            
                            GN_LOG_INFO("🎉 Boss defeated! Fade complete - transitioning to credits...");
                            // Signal state to finish and transition to credits
                            m_bossDefeated = true;  // Mark that boss was actually defeated
                            m_finished = true;
                        }
                    } else {
                        GN_LOG_ERROR("White fade UIShape component is NULL!");
                    }
                } else {
                    GN_LOG_DEBUG("White fade alpha is 0 or negative: " + std::to_string(fadeAlpha));
                }
            }

            // Check player projectile collisions with boss (level 6 only)
            if (m_bossSystem && m_projectileSystem && m_currentLevelId == 6 && m_bossSystem->IsActive()) {
                m_frameProfiler.StartSection("BossProjectileCollision");
                
                const auto& activeProjectiles = m_projectileSystem->GetActivePlayerProjectiles();
                GNVector2 bossPosition = m_bossSystem->GetPosition();
                
                // Boss hitbox center calculation matching old scripts:
                // Old: { position.x + 32, position.y + 32, scaledWidth, scaledHeight } at scale 1.0
                // With current scale: offset = (32 * scale) + (128 * scale / 2) = (32 * scale) + (64 * scale)
                float bossScale = m_bossSystem->GetScale();
                float hitboxOffset = (32.0f * bossScale) + (128.0f * bossScale * 0.5f);
                float bossCenterX = bossPosition.x + hitboxOffset;
                float bossCenterY = bossPosition.y + hitboxOffset;
                
                // Boss hitbox radius (half of 1024px)
                float bossHitboxRadius = 512.0f;
                
                for (Entity projEntity : activeProjectiles) {
                    Transform* projTransform = m_ecsSystem->GetComponent<Transform>(projEntity);
                    Hitbox* projHitbox = m_ecsSystem->GetComponent<Hitbox>(projEntity);
                    Projectile* proj = m_ecsSystem->GetComponent<Projectile>(projEntity);
                    
                    if (!projTransform || !projHitbox || !proj || !proj->isActive) continue;
                    
                    // Calculate projectile center accounting for sprite dimensions and hitbox offsets
                    Sprite* projSprite = m_ecsSystem->GetComponent<Sprite>(projEntity);
                    float spriteHalfWidth = projSprite ? (projSprite->width * projTransform->scale.x) * 0.5f : 0.0f;
                    float spriteHalfHeight = projSprite ? (projSprite->height * projTransform->scale.y) * 0.5f : 0.0f;
                    
                    // Position is top-left, so add half sprite dimensions plus hitbox offset to get true center
                    float projCenterX = projTransform->position.x + spriteHalfWidth + (projHitbox->offsetX * projTransform->scale.x);
                    float projCenterY = projTransform->position.y + spriteHalfHeight + (projHitbox->offsetY * projTransform->scale.y);
                    
                    // Calculate distance to boss center (using adjusted center position)
                    float dx = bossCenterX - projCenterX;
                    float dy = bossCenterY - projCenterY;
                    float distance = std::sqrt(dx * dx + dy * dy);
                    
                    // Scale projectile hitbox
                    float scaledProjRadius = projHitbox->radius * ((std::abs(projTransform->scale.x) + std::abs(projTransform->scale.y)) * 0.5f);
                    float combinedRadius = bossHitboxRadius + scaledProjRadius;
                    
                    // Check collision
                    if (distance < combinedRadius) {
                        GN_LOG_INFO("[BOSS_COLLISION] Player projectile HIT Rat King! Projectile: " + std::to_string(projEntity) + 
                                   " distance: " + std::to_string(distance) + " < " + std::to_string(combinedRadius));
                        
                        // Damage the boss - 10 damage per hit (200 health / 20 hits = 10 damage)
                        m_bossSystem->HandleDamage(10);
                        
                        // Deactivate the projectile
                        proj->isActive = false;
                        
                        GN_LOG_INFO("[BOSS_COLLISION] Boss health now: " + std::to_string(m_bossSystem->GetHealth()) + 
                                   "/" + std::to_string(m_bossSystem->GetMaxHealth()));
                    }
                }
                
                m_frameProfiler.EndSection("BossProjectileCollision");
            }

            // Update pipe counter UI
            m_frameProfiler.StartSection("UpdatePipeCounterUI");
            UpdatePipeCounterUI();
            m_frameProfiler.EndSection("UpdatePipeCounterUI");
            m_frameProfiler.StartSection("UpdateCoinCounterUI");
            UpdateCoinCounterUI();
            m_frameProfiler.EndSection("UpdateCoinCounterUI");

            // Update difficulty
            m_frameProfiler.StartSection("UpdateDifficulty");
            UpdateDifficulty(deltaTime);
            m_frameProfiler.EndSection("UpdateDifficulty");

            // Handle game events
            m_frameProfiler.StartSection("HandleGameEvents");
            HandleGameEvents();
            m_frameProfiler.EndSection("HandleGameEvents");

            // Clean up offscreen entities
            m_frameProfiler.StartSection("CleanupOffscreenEntities");
            CleanupOffscreenEntities();
            m_frameProfiler.EndSection("CleanupOffscreenEntities");

            // Check level completion
            m_frameProfiler.StartSection("CheckLevelCompletion");
            CheckLevelCompletion();
            m_frameProfiler.EndSection("CheckLevelCompletion");
        } // End of Playing sub-state
        
        // Update essential systems regardless of sub-state (needed for physics during falling)
        if (m_spriteSystem && m_currentSubState != GameplaySubState::Paused) {
            m_frameProfiler.StartSection("SpriteSystem");
            m_spriteSystem->Update(deltaTime);
            m_frameProfiler.EndSection("SpriteSystem");
        }
        
        // Check if boss death sequence is active - freeze gameplay like pause menu
        // This check happens AFTER boss system updates so death sequence can start
        bool bossDyingFreeze = false;
        if (m_bossSystem && m_currentLevelId == 6) {
            GN_LOG_INFO("🔍 Boss freeze check: m_bossSystem exists, currentLevel=6, bossHealth=" + std::to_string(m_bossSystem->GetHealth()));
            // Check if boss entered death state
            if (m_bossSystem->GetHealth() <= 0) {
                bossDyingFreeze = true;
                GN_LOG_INFO("🔒 BOSS DYING FREEZE ACTIVATED - health <= 0");
                
                // Stop music once when boss dies (check if death sequence just started)
                if (m_platformDelegates && m_platformDelegates->audio.stopMusic) {
                    // Only stop if boss just entered death state (health exactly 0 or death sequence starting)
                    static int lastCheckedHealth = 200; // Initialize to max health
                    if (lastCheckedHealth > 0 && m_bossSystem->GetHealth() <= 0) {
                        m_platformDelegates->audio.stopMusic();
                        GN_LOG_INFO("🎵 Boss death - music stopped, freeze activated");
                    }
                    lastCheckedHealth = m_bossSystem->GetHealth();
                }
            }
        } else {
            if (m_bossSystem) {
                GN_LOG_INFO("🔍 Boss freeze check: m_bossSystem exists but currentLevel=" + std::to_string(m_currentLevelId) + " (not 6)");
            }
        }
        
        GN_LOG_INFO("🎮 bossDyingFreeze=" + std::to_string(bossDyingFreeze) + ", currentLevel=" + std::to_string(m_currentLevelId));
        
        // PlayerControllerSystem updates player physics - only when not paused
        // Allow during GameOver for player falling animation, but not during boss death
        if (m_playerControllerSystem && m_currentSubState != GameplaySubState::Paused && !bossDyingFreeze) {
            m_frameProfiler.StartSection("PlayerController");
            m_playerControllerSystem->Update(deltaTime);
            m_frameProfiler.EndSection("PlayerController");
        }
        
        m_frameProfiler.EndFrame();
    }

    void GameplayState::UpdateSubState(float deltaTime) {
        switch (m_currentSubState) {
            case GameplaySubState::Playing:
                // Check if player reached 0 hearts
                if (m_heartSystem && m_playerEntity != 0) {
                    PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
                    if (player) {
                        GN_LOG_INFO("Player hearts: " + std::to_string(player->hearts) + 
                                   ", Live slices: " + std::to_string(player->liveSlices));
                        if (player->liveSlices <= 0) {
                            GN_LOG_INFO("Player has 0 live slices - triggering game over!");
                            TriggerGameOver();
                        }
                    }
                }
                break;
                
            case GameplaySubState::Paused:
                // Update pause system for debouncing timers
                if (m_pauseSystem) {
                    m_pauseSystem->Update(deltaTime);
                }
                break;
                
            case GameplaySubState::GameOver:
                // Update game over sequence - check if player has fallen off screen
                m_gameOverTimer += deltaTime;
                
                // Check if player has fallen completely off screen
                if (!HasPlayerFallenOffScreen()) {
                    // Player is still falling, don't show UI yet
                    GN_LOG_INFO("Player still falling... waiting for complete fall off screen");
                    break;
                }
                
                // Player has fallen off screen, show game over UI if not already shown
                if (m_gameOverBackgroundEntity == 0) {
                    GN_LOG_INFO("Player has fallen off screen - creating game over UI");
                    
                    // Stop background music (stinger already played when hearts reached 0)
                    if (m_platformDelegates && m_platformDelegates->audio.stopMusic) {
                        m_platformDelegates->audio.stopMusic();
                    }
                    
                    // Hide all regular UI elements
                    HideRegularUI();
                    
                    // Create game over UI
                    CreateGameOverUI();
                    
                    GN_LOG_INFO("Game over UI created successfully");
                    
                    // Trigger ad system check NOW (after UI is shown but before player clicks Try Again)
                    // This gives player time to see their death stats while ad loads/shows
                    if (GameCore::GetGame()) {
                        GameCore::GetGame()->TriggerGameOverAd();
                        GN_LOG_INFO("Ad system triggered after game over UI shown");
                    }
                } else {
                    GN_LOG_INFO("Game over UI already exists, skipping creation");
                }
                
                UpdateMorteFloating(deltaTime);
                HandleGameOverInput();
                break;
        }
    }

    void GameplayState::Render() {
        // All rendering now handled by unified SystemManager::Render() -> RenderSystem::Render() path
        // Remove direct m_renderSystem->Render() call to prevent duplicate rendering
        
        // UI rendering is now handled by RenderSystem; avoid calling UISystem::Render() to prevent duplication

        // Boss health bar UI entities are automatically rendered by RenderSystem

        // Draw debug rectangles overlay (after world/UI render so they appear on top)
        // DrawDebugRectangles(); // Disabled for production
    }

    void GameplayState::HandleSettingsButtonInput() {
        // Only handle settings button input (no other gameplay input)
        if (m_currentSubState == GameplaySubState::GameOver) {
            return; // No input during game over
        }

        // Don't handle settings button when paused - pause system handles it
        if (m_currentSubState == GameplaySubState::Paused) {
            return;
        }

        // Get InputManager singleton
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) return;

        // Check for any active touches and see if they hit the settings button
        // Don't rely on the PAUSE action zone since it's in the wrong location
        auto touches = inputManager->GetActiveTouches();
        for (const auto& touch : touches) {
            if (touch.state == TouchState::PRESSED || touch.state == TouchState::RELEASED) {
                // Use pixel coordinates directly (already converted by InputManager)
                float pixelX = touch.rawX;
                float pixelY = touch.rawY;

                // Debug logging for settings button input
                GN_LOG_INFO("Settings button check: pixel coords (" + std::to_string(pixelX) + ", " + std::to_string(pixelY) +
                           ") Level: " + std::to_string(m_currentLevelId) +
                           " Boss: " + std::to_string(m_currentLevelId == 6) +
                           " Landscape: " + std::to_string(inputManager->IsLandscapeOrientation()));

                // Check if this touch hits the settings button
                if (CheckSettingsButtonClick(pixelX, pixelY)) {
                    // Settings button was clicked, don't check other touches
                    break;
                }
            }
        }
    }

    void GameplayState::HandleInput() {
        if (m_currentSubState == GameplaySubState::GameOver) {
            // No input allowed during game over falling sequence
            // Input will be handled by HandleGameOverInput once UI is shown
            return;
        }

        // Check if InputManager singleton is available
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) {
            GN_LOG_ERROR("GameplayState: InputManager singleton is NULL!");
            return;
        }

        // Handle settings button input (works in both Playing and Paused states)
        HandleSettingsButtonInput();

        // Handle pause menu input if we're paused
        if (m_currentSubState == GameplaySubState::Paused) {
            HandlePauseMenuInput();
            return; // Don't process gameplay input when paused
        }

        // Handle gameplay input
        HandleGameplayInput();
    }


    void GameplayState::HandlePauseMenuInput() {
        GN_LOG_INFO("🎮 In paused state, processing pause menu input");

        // Get InputManager singleton
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) return;

        // Get active touches for pause menu processing
        auto touches = inputManager->GetActiveTouches();

        for (const auto& touch : touches) {
            if (touch.state == TouchState::PRESSED || touch.state == TouchState::HELD || touch.state == TouchState::RELEASED) {
                // Convert to pixel coordinates for pause menu
                float pixelX, pixelY;
                inputManager->NormalizedToScreen(touch.x, touch.y, pixelX, pixelY);

                std::string stateStr = (touch.state == TouchState::PRESSED) ? "PRESSED" :
                                      (touch.state == TouchState::HELD) ? "HELD" : "RELEASED";
                GN_LOG_INFO("🎮 Pause menu " + stateStr + " touch at (" + std::to_string(pixelX) + ", " + std::to_string(pixelY) + ")");

                // For PRESSED touches, check settings button first
                if (touch.state == TouchState::PRESSED) {
                    if (CheckSettingsButtonClick(pixelX, pixelY)) {
                        GN_LOG_INFO("Settings button tapped from pause menu - handling directly");
                        return; // Settings button was clicked, don't process other pause menu input
                    }
                }

                // Delegate pause menu interactions to PauseSystem for all touch states
                if (m_pauseSystem) {
                    m_pauseSystem->HandleInput(pixelX, pixelY, touch.state);

                    // Check if user tapped outside menu area to close it (original implementation)
                    if (IsTapOutsideMenuArea(pixelX, pixelY)) {
                        GN_LOG_INFO("Tap outside menu area detected - closing pause menu");
                        m_pauseSystem->Hide();
                        TriggerResume();
                        return; // Menu closed, don't process other input
                    }
                } else {
                    GN_LOG_ERROR("PauseSystem not available for input handling");
                }
            }
        }
    }

    void GameplayState::HandleGameplayInput() {
        // Handle gameplay input using InputManager
        if (!m_playerControllerSystem) {
            return;
        }

        // Get InputManager singleton
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) return;

        // Get active touches from InputManager
        auto touches = inputManager->GetActiveTouches();

        for (const auto& touch : touches) {
            if (touch.state == TouchState::PRESSED) {
                // Send touch press event to PlayerControllerSystem
                GN_LOG_INFO("🎮 Sending touch press to PlayerControllerSystem: (" +
                           std::to_string(touch.x) + ", " + std::to_string(touch.y) + ")");
                m_playerControllerSystem->HandleTouchInput(touch.x, touch.y, true);
            } else if (touch.state == TouchState::RELEASED) {
                // Send touch release event to PlayerControllerSystem
                GN_LOG_INFO("🎮 Sending touch release to PlayerControllerSystem: (" +
                           std::to_string(touch.x) + ", " + std::to_string(touch.y) + ")");
                m_playerControllerSystem->HandleTouchInput(touch.x, touch.y, false);
            }
        }

        // Handle touch dragging for pause menu sliders (if dragging)
        if (m_currentSubState == GameplaySubState::Paused && m_pauseSystem && m_pauseSystem->IsDragging()) {
            for (const auto& touch : touches) {
                if (touch.state == TouchState::HELD) {
                    float pixelX, pixelY;
                    inputManager->NormalizedToScreen(touch.x, touch.y, pixelX, pixelY);
                    m_pauseSystem->HandleInput(pixelX, pixelY);
                }
            }
        }

        // Handle touch release for stopping dragging
        if (m_pauseSystem && m_pauseSystem->IsDragging()) {
            for (const auto& touch : touches) {
                if (touch.state == TouchState::RELEASED) {
                    GN_LOG_INFO("Stopping knob drag");
                    m_pauseSystem->StopDragging();
                    break; // Only need to stop dragging once
                }
            }
        }
    }

    void GameplayState::SetLevel(int levelId) {
        GN_LOG_INFO("Setting level to: " + std::to_string(levelId));
        m_currentLevelId = levelId;

        // Update RenderSystem with current level ID
        if (m_renderSystem) {
            m_renderSystem->SetCurrentLevelId(levelId);
        }

        // Load level configuration
        m_currentLevelConfig = LevelConfigFactory::GetLevelConfig(levelId);
        GN_LOG_INFO("Loaded configuration for: " + m_currentLevelConfig.levelName);

        // Set orientation lock based on level
        if (m_platformDelegates && m_platformDelegates->renderer.lockToLandscape && m_platformDelegates->renderer.lockToPortrait) {
            if (levelId == 6) { // Boss level - landscape only
                GN_LOG_INFO("Boss level detected - locking to landscape orientation");
                m_platformDelegates->renderer.lockToLandscape();
            } else {
                // Other levels - force portrait mode
                GN_LOG_INFO("Non-boss level - locking to portrait orientation");
                m_platformDelegates->renderer.lockToPortrait();
            }
        }

        // Show/hide shooting zone visual indicator based on level
        GN_LOG_INFO("SetLevel: Checking shooting zone visibility for level " + std::to_string(m_currentLevelId));
        GN_LOG_INFO("SetLevel: shootingEnabled = " + std::to_string(m_currentLevelConfig.shootingEnabled));
        GN_LOG_INFO("SetLevel: m_shootingZoneEntity = " + std::to_string(m_shootingZoneEntity));

        if (m_shootingZoneEntity != 0 && m_ecsSystem) {
            GN_LOG_INFO("SetLevel: Entity exists, getting components...");
            auto uiElement = m_ecsSystem->GetComponent<UIElement>(m_shootingZoneEntity);
            auto uiShape = m_ecsSystem->GetComponent<UIShape>(m_shootingZoneEntity);
            bool shouldBeVisible = m_currentLevelConfig.shootingEnabled;

            GN_LOG_INFO("SetLevel: uiElement found = " + std::to_string(uiElement != nullptr));
            GN_LOG_INFO("SetLevel: uiShape found = " + std::to_string(uiShape != nullptr));
            GN_LOG_INFO("SetLevel: shouldBeVisible = " + std::to_string(shouldBeVisible));

            if (uiElement) {
                uiElement->visible = shouldBeVisible;
                GN_LOG_INFO("SetLevel: Set UIElement visible = " + std::to_string(shouldBeVisible));
            } else {
                GN_LOG_ERROR("SetLevel: UIElement component not found!");
            }
            if (uiShape) {
                uiShape->visible = shouldBeVisible;
                GN_LOG_INFO("SetLevel: Set UIShape visible = " + std::to_string(shouldBeVisible));
            } else {
                GN_LOG_ERROR("SetLevel: UIShape component not found!");
            }

            GN_LOG_INFO("Shooting zone visibility set to " + std::to_string(shouldBeVisible) + " for level " + std::to_string(m_currentLevelId));
        } else {
            GN_LOG_ERROR("SetLevel: Shooting zone entity is 0 or ECS system is null!");
        }

        // Reset game state for new level
        m_currentScore = 0;
        m_pipesCleared = 0;
        m_sessionCoinsCollected = 0; // Reset session progress for new level
        m_pipeIncrementCooldown = 0.0f;
        m_currentLives = STARTING_LIVES;

        // Reset player session coins for new level
        if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
            player->sessionCoins = 0;
            GN_LOG_INFO("💰 Reset PlayerComponent::sessionCoins to 0 for new level");
        }
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        m_levelCompleted = false;

        // CRITICAL: Reset player position and physics for fresh start
        if (m_playerEntity != 0) {
            Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
            Physics* playerPhysics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            
            if (playerTransform) {
                // Reset to starting position (left side of screen, vertically centered)
                playerTransform->position.x = 300.0f;
                playerTransform->position.y = 1278.0f; // Screen center-ish
                GN_LOG_INFO("[RESET] Player position reset to (" + 
                           std::to_string(playerTransform->position.x) + ", " + 
                           std::to_string(playerTransform->position.y) + ")");
            }
            
            if (playerPhysics) {
                // Reset velocity
                playerPhysics->velocity = Gnosis::GNVector2(0.0f, 0.0f);
                GN_LOG_INFO("[RESET] Player velocity reset to zero");
            }
        }
        
        // Reset camera system world position for fresh start
        if (m_cameraSystem) {
            m_cameraSystem->ResetForNewGame();
        }
        
        // CRITICAL: Reset all entities for fresh spawning
        if (m_levelManager) {
            m_levelManager->ResetEnemiesForRetry();
            GN_LOG_INFO("[RESET] Enemy positions reset for level retry");
        }
        
        // Reset obstacles for clean level start
        if (m_levelManager && m_levelManager->GetObstacleSystem()) {
            m_levelManager->GetObstacleSystem()->Cleanup();
            GN_LOG_INFO("[RESET] Obstacles cleared for level retry");
        }
        
        // Clear all pickups for fresh spawning
        if (m_pickupSystem) {
            m_pickupSystem->ClearAll();
            GN_LOG_INFO("[RESET] Pickups cleared for level retry");
        }
        
        // Reset boss coin spawning (coins already destroyed by PickupSystem::ClearAll above)
        m_bossCoinSpawnTimer = 0.0f;
        m_bossRainbowHeartSpawned = false;
        m_activeBossCoins.clear();

        // Reset background positions to initial state
        if (m_levelManager) {
            m_levelManager->ResetBackgroundPositions();
        }

        // Reset skill system for new level (safety net, etc.)
        if (m_skillSystem) {
            m_skillSystem->ResetForNewLevel();
        }

        // Reset spawn timers
        m_obstacleSpawnTimer = 0.0f;
        m_pickupSpawnTimer = 0.0f;
        m_enemySpawnTimer = 0.0f;
    }

    void GameplayState::RestartLevel() {
        GN_LOG_INFO("Restarting level: " + std::to_string(m_currentLevelId));
        SetLevel(m_currentLevelId);
    }

    void GameplayState::GameOver() {
        GN_LOG_INFO("Game Over!");

        m_playerAlive = false;
        SaveGameProgress();
    }

    void GameplayState::LevelComplete() {
        GN_LOG_INFO("Level Complete!");
        m_levelCompleted = true;

        // Update level high score and check for unlocks
        if (GameCore::GetGame()) {
            // Get session coins from PlayerComponent
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            int sessionCoins = player ? player->sessionCoins : 0;

            GameCore::GetGame()->UpdateLevelHighScore(m_currentLevelId, m_pipesCleared, sessionCoins);
        }

        SaveGameProgress();
    }

    void GameplayState::TogglePause() {
        if (m_currentSubState == GameplaySubState::Paused) {
            TriggerResume();
        } else if (m_currentSubState == GameplaySubState::Playing) {
            TriggerPause();
        }
    }

    void GameplayState::InitializeSystems() {
        GN_LOG_INFO("Initializing gameplay systems");
        
        if (!m_platformDelegates) {
            GN_LOG_ERROR("Platform delegates is null!");
            return;
        }
        
        // Create sprite system
        m_spriteSystem = std::make_unique<SpriteSystem>(m_ecsSystem, *m_platformDelegates);

        // Create projectile system
        m_projectileSystem = std::make_unique<ProjectileSystem>(m_ecsSystem);
        m_projectileSystem->Initialize();
        
        // Create overlay system for snowfall and other effects
        m_overlaySystem = std::make_unique<OverlaySystem>(m_ecsSystem);
        m_overlaySystem->SetPlatformDelegates(*m_platformDelegates);
        m_overlaySystem->Initialize();
        
        // Enable snowfall for snow level (level 4)
        if (m_currentLevelId == 4) {
            m_overlaySystem->EnableSnowfall(true);
            m_overlaySystem->SetSnowfallAnimSpeed(12.0f); // 12 FPS animation
        }

        // Create hats system for cosmetics management (MUST be before PlayerControllerSystem)
        m_hatsSystem = std::make_unique<HatsSystem>(m_ecsSystem, *m_platformDelegates);

        // Create skill system for managing player skills
        m_skillSystem = std::make_unique<SkillSystem>(m_ecsSystem, m_platformDelegates);

        // Notify LevelManager that ProjectileSystem is ready
        if (m_levelManager) {
            m_levelManager->OnProjectileSystemReady();
        }

        // Create player controller system
        m_playerControllerSystem = std::make_unique<PlayerControllerSystem>(m_ecsSystem, m_platformDelegates, m_spriteSystem.get(), m_projectileSystem.get(), m_hatsSystem.get(), m_skillSystem.get(), m_currentLevelId, &m_currentLevelConfig);

        // Set RenderSystem reference for screen dimension access
        if (m_renderSystem) {
            m_playerControllerSystem->SetRenderSystem(m_renderSystem);
        }
        
        // Create camera system
        m_cameraSystem = std::make_unique<CameraSystem>(m_ecsSystem);
        
        // 🎯 NEW: Get the existing RenderSystem from SystemManager instead of creating a new one
        if (m_ecsSystem && m_ecsSystem->GetSystemManager()) {
            m_renderSystem = m_ecsSystem->GetSystemManager()->GetRenderSystem();
            GN_LOG_INFO("GameplayState: Using existing RenderSystem from SystemManager");
            
            // CRITICAL: Set current level ID immediately to prevent crash during transitions
            if (m_renderSystem) {
                m_renderSystem->SetCurrentLevelId(m_currentLevelId);
                // Force screen info update to handle boss level orientation requirements
                m_renderSystem->UpdateScreenInfo();
                GN_LOG_INFO("GameplayState: Set RenderSystem level ID to " + std::to_string(m_currentLevelId) + " and updated screen info");
            }
        } else {
            GN_LOG_ERROR("❌ GameplayState: Could not get RenderSystem from SystemManager!");
        }
        
        // Create UI system for text and button rendering
        m_uiSystem = std::make_unique<UISystem>(m_ecsSystem, *m_platformDelegates);
        
        // Create level manager system
        m_levelManager = std::make_unique<LevelManager>(m_ecsSystem);
        if (m_platformDelegates) {
            m_levelManager->SetPlatformDelegates(*m_platformDelegates);
        }

        // Create boss systems (only for level 6)
        if (m_currentLevelId == 6) {
            // Create ExplosionSystem for boss death sequence
            m_explosionSystem = std::make_unique<ExplosionSystem>(m_ecsSystem);
            GN_LOG_INFO("ExplosionSystem initialized for boss level");
            
            m_bossSystem = std::make_unique<BossSystem>((ECS*)m_ecsSystem, m_levelManager.get(), m_projectileSystem.get(), m_explosionSystem.get(), m_platformDelegates);

            // Update BossSystem with current screen dimensions once during initialization
            if (m_renderSystem) {
                const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
                if (screenInfo.pixelWidth > 0 && screenInfo.pixelHeight > 0) {
                    m_bossSystem->UpdateScreenDimensions(screenInfo.pixelWidth, screenInfo.pixelHeight);
                    GN_LOG_INFO("BossSystem initialized with screen dimensions: " + std::to_string((int)screenInfo.pixelWidth) + "x" + std::to_string((int)screenInfo.pixelHeight));
                }
            }

            m_bossHealthBar = std::make_unique<BossHealthBar>(m_bossSystem.get(), "Rat King", m_ecsSystem);
            GN_LOG_INFO("Boss systems initialized for level 6");
            
            // Create white fade overlay entity (initially invisible)
            m_whiteFadeEntity = m_ecsSystem->CreateEntity();
            if (m_whiteFadeEntity != 0) {
                Transform fadeTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                m_ecsSystem->AddComponent<Transform>(m_whiteFadeEntity, fadeTransform);
                
                // Get actual screen dimensions from RenderSystem (will be landscape for boss level)
                const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
                float screenWidth = screenInfo.pixelWidth;
                float screenHeight = screenInfo.pixelHeight;
                
                // Create a full-screen white rectangle using UIShape
                UIShape whiteOverlay;
                whiteOverlay.type = UIShapeType::Rectangle;
                whiteOverlay.width = screenWidth;
                whiteOverlay.height = screenHeight;
                whiteOverlay.color = Gnosis::GNColor(255, 255, 255, 0);  // White, fully transparent initially
                whiteOverlay.visible = false;  // Initially hidden
                whiteOverlay.layer = 250;  // Very high layer (above explosions at 200)
                m_ecsSystem->AddComponent<UIShape>(m_whiteFadeEntity, whiteOverlay);
                
                GN_LOG_INFO("White fade overlay created: " + std::to_string(screenWidth) + "x" + std::to_string(screenHeight) + " using UIShape (landscape)");
            }
        }

        // 🎯 NEW: Set RenderSystem reference for texture metadata cache access
        if (m_renderSystem) {
            m_levelManager->SetRenderSystem(m_renderSystem);
            
            // Wire up OverlaySystem to RenderSystem for overlay rendering
            m_renderSystem->SetOverlaySystem(m_overlaySystem.get());
            GN_LOG_INFO("OverlaySystem connected to RenderSystem");
        }
        // Create pickup system and pass dependencies
        m_pickupSystem = std::make_unique<PickupSystem>(m_ecsSystem, m_levelManager.get(), m_platformDelegates, &m_currentLevelConfig);
        
        // NEW: Register PickupSystem with LevelManager for orchestrator pattern
        m_levelManager->SetPickupSystem(m_pickupSystem.get());
        
        // Set up collection callbacks to connect PickupSystem to collection handlers
        m_pickupSystem->SetCoinCollectedCallback([this](int value) {
            this->OnCoinCollected(value);
        });

        m_pickupSystem->SetHeartCollectedCallback([this](int healAmount) {
            this->OnHeartCollected(healAmount);
        });

        // Connect SkillSystem to PickupSystem for magnet effects
        if (m_skillSystem) {
            m_skillSystem->SetPickupSystem(m_pickupSystem.get());
        }
        
        // Create enemy system for behaviors (bobbing, states, etc.)
        m_enemySystem = std::make_unique<EnemySystem>(m_ecsSystem, m_levelManager.get(), m_projectileSystem.get());
        
        // Connect EnemySystem to BossSystem for checking boss death state
        if (m_enemySystem && m_bossSystem) {
            m_enemySystem->SetBossSystem(m_bossSystem.get());
        }
        
        // Create heart system for health display and management
        m_heartSystem = std::make_unique<HeartSystem>(m_ecsSystem, *m_platformDelegates);
        
        // Initialize enemy configuration registry BEFORE loading levels
        GameCore::EnemyConfigRegistry::Initialize();
        
        // Load the current level (ensure previous entities are torn down)
        m_levelManager->UnloadLevel();
        if (!m_levelManager->LoadLevel(m_currentLevelId)) {
            GN_LOG_ERROR("Failed to load level " + std::to_string(m_currentLevelId));
        } else {
            GN_LOG_INFO("Level " + std::to_string(m_currentLevelId) + " loaded successfully");
        }

        // Initialize boss system for level 6
        if (m_currentLevelId == 6 && m_bossSystem) {
            m_bossSystem->InitializeForLevel();
        }
        
        GN_LOG_INFO("Gameplay systems initialized successfully");
        
        // Setup platform-specific layout after systems are initialized
        SetupLayout();
    }

    void GameplayState::CacheScreenDimensions() {
        GN_LOG_INFO("Caching screen dimensions (this eliminates 40+ repeated GetScreenInfo() calls throughout gameplay)");

        // Cache screen dimensions once at startup instead of calling GetScreenInfo() repeatedly
        if (m_renderSystem) {
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            m_cachedScreenWidth = screenInfo.pixelWidth;
            m_cachedScreenHeight = screenInfo.pixelHeight;
            GN_LOG_INFO("✅ Cached screen dimensions: " + std::to_string((int)m_cachedScreenWidth) + "x" + std::to_string((int)m_cachedScreenHeight));

            // Update PauseSystem with new screen dimensions
            if (m_pauseSystem) {
                m_pauseSystem->UpdateScreenDimensions(m_cachedScreenWidth, m_cachedScreenHeight);
                GN_LOG_INFO("✅ PauseSystem updated with cached screen dimensions");
            }
        } else {
            GN_LOG_WARN("⚠️ RenderSystem not available, using fallback screen dimensions");
            // Keep the constructor defaults
        }

        GN_LOG_INFO("Screen dimensions cached - all future dimension access will use cached values instead of repeated GetScreenInfo() calls");

        // UI layout will be updated after CreateUI() is called
    }

    void GameplayState::RegisterScreenInfoCallback() {
        GN_LOG_INFO("Registering screen info update callback for orientation changes");

        // Store initial orientation state
        bool lastOrientation = IsLandscapeMode();

        // Register callback with ConfigManager to handle orientation changes
        auto& configManager = ConfigManager::Instance();
        configManager.SetScreenInfoUpdateCallback([this, lastOrientation, &configManager]() mutable {
            // Safety check: ensure this object and render system are still valid
            if (!this || !m_renderSystem) {
                GN_LOG_WARN("Screen info callback called on invalid GameplayState object");
                return;
            }

            // Get current screen info including orientation lock state
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            
            // CRITICAL: Ignore updates while orientation is still changing (before MTKView rotation completes)
            if (screenInfo.isOrientationChanging) {
                GN_LOG_INFO("🔒 Ignoring screen info update - orientation still changing (waiting for MTKView)");
                return;
            }

            // Get current orientation
            bool currentOrientation = IsLandscapeMode();

            // Only reposition if orientation actually changed AND rotation is complete
            if (currentOrientation != lastOrientation) {
                std::string fromOrient = lastOrientation ? "landscape" : "portrait";
                std::string toOrient = currentOrientation ? "landscape" : "portrait";
                GN_LOG_INFO("🎯 Orientation change COMPLETE: " + fromOrient + " → " + toOrient + " - repositioning UI");

                // Update cached screen dimensions
                m_cachedScreenWidth = screenInfo.pixelWidth;
                m_cachedScreenHeight = screenInfo.pixelHeight;

                GN_LOG_INFO("Updated cached screen dimensions: " +
                           std::to_string((int)m_cachedScreenWidth) + "x" +
                           std::to_string((int)m_cachedScreenHeight));

                // Reposition UI elements for new orientation (one-time operation)
                UpdateUILayoutForOrientation();

                // Update boss system if active
                if (m_bossSystem && m_currentLevelId == 6) { // Level 6 is boss level
                    m_bossSystem->UpdateScreenDimensions(m_cachedScreenWidth, m_cachedScreenHeight);
                    GN_LOG_INFO("Updated boss system screen dimensions");
                }

                // Update pause system
                if (m_pauseSystem) {
                    m_pauseSystem->UpdateScreenDimensions(m_cachedScreenWidth, m_cachedScreenHeight);
                    GN_LOG_INFO("Updated pause system screen dimensions");
                }

                // Update PlayerControllerSystem screen dimensions for shooting zone calculations
                if (m_playerControllerSystem) {
                    // This ensures shooting zone boundaries are recalculated with new dimensions
                    GN_LOG_INFO("Orientation change: PlayerControllerSystem should recalculate shooting zones");
                }

                // Update last orientation state
                lastOrientation = currentOrientation;
                GN_LOG_INFO("UI repositioning complete for new orientation");
            }
        });

        GN_LOG_INFO("Screen info update callback registered successfully");
    }

    void GameplayState::NormalizeCoordinates(float pixelX, float pixelY, float& outNormalizedX, float& outNormalizedY) {
        // CRITICAL FIX: Use real-time screen dimensions instead of cached ones
        // This ensures input coordinates are normalized correctly even during orientation changes
        if (m_renderSystem) {
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            outNormalizedX = pixelX / screenInfo.pixelWidth;
            outNormalizedY = pixelY / screenInfo.pixelHeight;

            // Update cached dimensions to stay in sync
            m_cachedScreenWidth = screenInfo.pixelWidth;
            m_cachedScreenHeight = screenInfo.pixelHeight;
        } else {
            // Fallback to cached dimensions if render system unavailable
            GN_LOG_WARN("NormalizeCoordinates: RenderSystem unavailable, using cached dimensions");
            outNormalizedX = pixelX / m_cachedScreenWidth;
            outNormalizedY = pixelY / m_cachedScreenHeight;
        }
    }

    // Additional coordinate conversion helpers
    void GameplayState::DenormalizeCoordinates(float normalizedX, float normalizedY, float& outPixelX, float& outPixelY) {
        // Convert normalized coordinates (0.0-1.0) back to pixel coordinates
        outPixelX = normalizedX * m_cachedScreenWidth;
        outPixelY = normalizedY * m_cachedScreenHeight;
    }

    Gnosis::GNVector2 GameplayState::CenterObjectAtPosition(float centerX, float centerY, float width, float height) {
        // Calculate top-left position for an object centered at the given coordinates
        float topLeftX = centerX - (width * 0.5f);
        float topLeftY = centerY - (height * 0.5f);
        return Gnosis::GNVector2(topLeftX, topLeftY);
    }

    void GameplayState::CreateGameEntities() {
        GN_LOG_INFO("Creating game entities");
        
        if (!m_ecsSystem) {
            GN_LOG_ERROR("ECS system is null!");
            return;
        }
        
        // Create player entity
        m_playerEntity = m_ecsSystem->CreateEntity();
        if (m_playerEntity != 0) {
            // Add basic components to player (positioned by PlayerControllerSystem)
            // Use level's base scale for consistent sizing
            float playerScale = m_currentLevelConfig.baseScale;

            // Position player based on level - boss level uses more spacing to avoid UI overlap, other levels center
            float playerX = (m_currentLevelId == 6) ? (m_cachedScreenWidth * 0.15f) : (m_cachedScreenWidth * 0.5f);
            Transform playerTransform(Gnosis::GNVector2(playerX, 639.0f), 0.0f, Gnosis::GNVector2(playerScale, playerScale));
            m_ecsSystem->AddComponent<Transform>(m_playerEntity, playerTransform);
            
            // Add sprite component with Turdlet idle animation (use existing playerScale)
            // TurdletIdle.png is 64x64 pixels, single frame
            Sprite playerSprite("TurdletIdle", 64.0f, 64.0f, 64, 64, 1, 0.1f);
            playerSprite.color = Gnosis::GNColor(255, 255, 255, 255);
            playerSprite.visible = true;
            playerSprite.layer = 6; // Player layer (above obstacles which are layers 3-5)

            // Set up sprite for animation support
            playerSprite.isAnimated = true;  // Enable animation support
            playerSprite.playing = true;     // Start playing
            playerSprite.loop = true;        // Loop animations by default
            playerSprite.currentFrame = 0;
            playerSprite.currentFrameTime = 0.0f;
            playerSprite.hasCompleted = false;

            GN_LOG_INFO("Player sprite created with texture: " + playerSprite.textureId + ", animated: " + std::to_string(playerSprite.isAnimated));
            m_ecsSystem->AddComponent<Sprite>(m_playerEntity, playerSprite);
            
            // Add physics component
            Physics playerPhysics;
            playerPhysics.useGravity = true;
            playerPhysics.mass = 1.0f;
            playerPhysics.drag = 0.95f;
            m_ecsSystem->AddComponent<Physics>(m_playerEntity, playerPhysics);
            
            // Add hitbox component (circle)
            Hitbox playerHitbox;
            playerHitbox.type = ColliderType::Circle;
            playerHitbox.radius = 12.0f;
            // Offsets are relative to the sprite CENTER in our collision/render math.
            // Keep centered by using zero offsets so pCenter = spriteTopLeft + (spriteHalfW/H).
            playerHitbox.offsetX = 0.0f;
            playerHitbox.offsetY = 0.0f;
            playerHitbox.isTrigger = false;
            m_ecsSystem->AddComponent<Hitbox>(m_playerEntity, playerHitbox);

            // Debug overlays OFF by default (can be toggled later if needed)
            // Only add debug draw if debug mode is enabled (check current level)
            bool enablePlayerDebug = (m_currentLevelId != 6); // Disable for boss level
            if (enablePlayerDebug) {
                DebugDraw playerDebug(false, false, Gnosis::GNColor(0, 255, 0, 255), Gnosis::GNColor(255, 0, 0, 255));
                playerDebug.alpha = 0.35f;
                m_ecsSystem->AddComponent<DebugDraw>(m_playerEntity, playerDebug);
                GN_LOG_DEBUG("Player debug draw enabled for level " + std::to_string(m_currentLevelId));
            } else {
                GN_LOG_DEBUG("Player debug draw disabled for boss level " + std::to_string(m_currentLevelId));
            }
            
            // Add player component with current coin count from game stats
            PlayerComponent playerData;

            // Initialize player with current total coins from game stats
            if (GameCore::GetGame()) {
                playerData.totalCoins = GameCore::GetGame()->GetGameStats().storedCoins;
                playerData.grossTotalCoins = GameCore::GetGame()->GetGameStats().totalCoinsCollected;
                GN_LOG_INFO("🎮 Player init - storedCoins: " + std::to_string(playerData.totalCoins) + ", totalCoinsCollected: " + std::to_string(playerData.grossTotalCoins));
            } else {
                GN_LOG_WARN("GameCore::GetGame() returned null - player initialized with 0 coins");
            }

            m_ecsSystem->AddComponent<PlayerComponent>(m_playerEntity, playerData);
            
            // Initialize player hearts based on current difficulty (but don't update UI yet)
            if (m_heartSystem) {
                Difficulty currentDifficulty = LevelManager::GetGlobalDifficulty();
                m_heartSystem->InitializePlayerHearts(m_playerEntity, currentDifficulty);
                GN_LOG_INFO("Player hearts initialized for difficulty: " + DifficultyToString(currentDifficulty));
            }
            
                    // Set up player controller
        if (m_playerControllerSystem) {
            m_playerControllerSystem->SetPlayerEntity(m_playerEntity);
            // Ensure player controller knows the player is alive
            m_playerControllerSystem->SetPlayerAlive(true);
        }

            // Note: Equipped hat is managed by HatsSystem and retrieved directly by PlayerControllerSystem
            // No need to set it in PlayerComponent - it's loaded from game save in HatsSystem constructor
            
            // Expose player to LevelManager for NPC/enemy behavior and triggers
            if (m_levelManager) {
                m_levelManager->SetPlayerEntity(m_playerEntity);
            }
            if (m_pickupSystem) {
                m_pickupSystem->SetPlayerEntity(m_playerEntity);
                m_pickupSystem->SetLevelConfig(&m_currentLevelConfig);
            }
            
            GN_LOG_INFO("Created player entity: " + std::to_string(m_playerEntity));
        }
        
        // Create camera entity with proper camera component
        m_cameraEntity = m_ecsSystem->CreateEntity();
        if (m_cameraEntity != 0) {
            Transform cameraTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_cameraEntity, cameraTransform);
            
            // Add camera component
            Camera cameraComponent;
            cameraComponent.zoom = 1.0f;
            cameraComponent.viewportSize = Gnosis::GNVector2(1179.0f, 1278.0f); // iPhone 16 screen size
            m_ecsSystem->AddComponent<Camera>(m_cameraEntity, cameraComponent);
            
            // Set as main camera for systems
            if (m_cameraSystem) {
                m_cameraSystem->SetMainCamera(m_cameraEntity);
                // Keep world scroll speed in sync with difficulty-scaled worldSpeed
                m_cameraSystem->SetWorldScrollSpeed(m_currentLevelConfig.worldSpeed);
            }
            
            if (m_renderSystem) {
                m_renderSystem->SetActiveCamera(m_cameraEntity);
            }
            
            GN_LOG_INFO("Created camera entity: " + std::to_string(m_cameraEntity));
        }
        
        // Background layers are now created by LevelManager in InitializeSystems
        GN_LOG_INFO("Background layers managed by LevelManager");
    }

    void GameplayState::CreateBackgroundLayers() {
        GN_LOG_INFO("Creating background layers for level: " + m_currentLevelConfig.levelName);
        
        if (!m_ecsSystem) {
            GN_LOG_ERROR("ECS system is null!");
            return;
        }
        
        // Clear existing background entities
        for (Gnosis::Entity entity : m_backgroundEntities) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_backgroundEntities.clear();
        
        GN_LOG_INFO("=== PHASE 1: Starting background layer creation ===");
        GN_LOG_INFO("Number of layer configs: " + std::to_string(m_currentLevelConfig.backgroundLayers.size()));
        
        int totalEntitiesCreated = 0;
        
        // Create background layers from level configuration
        for (size_t layerIdx = 0; layerIdx < m_currentLevelConfig.backgroundLayers.size(); layerIdx++) {
            const BackgroundLayer& layerConfig = m_currentLevelConfig.backgroundLayers[layerIdx];
            
            GN_LOG_INFO("--- Creating layer " + std::to_string(layerIdx) + ": '" + layerConfig.textureId + "' ---");
            
            // Calculate scaling and positioning
            float baseScale = m_currentLevelConfig.baseScale;
            
            // For backgrounds, calculate scale to fit screen height
            // Determine texture size based on specific texture ID
            float textureWidth, textureHeight;
            if (layerConfig.textureId.find("Front") != std::string::npos) {
                // FrontLayer backgrounds are 2048x480
                textureWidth = 2048.0f;
                textureHeight = 480.0f;
            } else if (layerConfig.textureId.find("Clouds") != std::string::npos) {
                // Cloud layers are 512x180
                textureWidth = 512.0f;
                textureHeight = 180.0f;
            } else if (layerConfig.textureId.find("BossLevelBackgroundMobile") != std::string::npos) {
                // Boss level background is 384x512 (portrait)
                textureWidth = 384.0f;
                textureHeight = 512.0f;
            } else {
                // Other background layers (Back, Mid) are 1024x480
                textureWidth = 1024.0f;
                textureHeight = 480.0f;
            }
            
            // Scale to fit screen height (using cached value)
            float screenHeight = m_cachedScreenHeight;
            float heightScale = screenHeight / textureHeight; // Scale to fill screen height
            float finalScale = heightScale * layerConfig.scaleMultiplier;
            
            GN_LOG_INFO("Texture '" + layerConfig.textureId + "': width=" + std::to_string(textureWidth) + 
                       ", height=" + std::to_string(textureHeight) + 
                       ", heightScale=" + std::to_string(heightScale) + 
                       ", scaleMultiplier=" + std::to_string(layerConfig.scaleMultiplier) +
                       ", finalScale=" + std::to_string(finalScale));
            
            // Scaled dimensions
            float scaledWidth = textureWidth * finalScale;
            float scaledHeight = textureHeight * finalScale;
            
            // Calculate number of instances needed for seamless wrapping
            // For static backgrounds (scrollSpeed = 0), only need 1 instance
            int numInstances;
            if (layerConfig.scrollSpeed == 0.0f) {
                numInstances = 1; // Static background - no need for multiple instances
                GN_LOG_INFO("Static background detected (scrollSpeed=0), using 1 instance");
            } else {
                numInstances = static_cast<int>(std::ceil(m_cachedScreenWidth / scaledWidth)) + 2;
                // Ensure minimum of 3 instances for proper wrapping on scrolling backgrounds
                numInstances = std::max(numInstances, 3);
                GN_LOG_INFO("Scrolling background detected, using " + std::to_string(numInstances) + " instances for seamless wrapping");
            }
            
            GN_LOG_INFO("Layer calculations: textureWidth=" + std::to_string(textureWidth) + 
                       ", finalScale=" + std::to_string(finalScale) + 
                       ", scaledWidth=" + std::to_string(scaledWidth) + 
                       ", numInstances=" + std::to_string(numInstances));
            
            float repeatWidth = layerConfig.repeatWidth > 0 ? layerConfig.repeatWidth : scaledWidth;
            
            // For initial positioning, we want instances to be placed touching each other
            // So use the actual scaled texture width for positioning
            float positionSpacing = scaledWidth;
            
            int layerEntitiesCreated = 0;
            
            // Create multiple instances for this layer
            for (int i = 0; i < numInstances; i++) {
                GN_LOG_INFO("Creating instance " + std::to_string(i) + " of " + std::to_string(numInstances));
                
                Gnosis::Entity bgEntity = m_ecsSystem->CreateEntity();
                if (bgEntity == 0) {
                    GN_LOG_ERROR("Failed to create entity for instance " + std::to_string(i));
                    continue;
                }
                
                GN_LOG_INFO("Successfully created entity " + std::to_string(bgEntity) + " for instance " + std::to_string(i));
                
                // Position instances side by side for seamless wrapping
                // Start first instance at x=0, others follow consecutively using texture width
                float xPos = i * positionSpacing;
                float yPos = 0.0f; // Position at top of screen for top-left rendering
                
                GN_LOG_INFO("Positioning entity " + std::to_string(bgEntity) + " at (" + std::to_string(xPos) + ", " + std::to_string(yPos) + ")");
                
                // Create and add Transform component
                // Apply the final scaling through Transform component
                Transform bgTransform(Gnosis::GNVector2(xPos, yPos), 0.0f, Gnosis::GNVector2(finalScale, finalScale));
                m_ecsSystem->AddComponent<Transform>(bgEntity, bgTransform);
                GN_LOG_INFO("Added Transform component to entity " + std::to_string(bgEntity) + " at (" + std::to_string(xPos) + ", " + std::to_string(yPos) + ") with scale=" + std::to_string(finalScale));
                
                // Create and add Sprite component
                // Use the original texture dimensions, scaling is handled by Transform
                Sprite bgSprite(layerConfig.textureId, textureWidth, textureHeight);
                bgSprite.color = Gnosis::GNColor(255, 255, 255, 255);
                bgSprite.visible = true;
                bgSprite.layer = layerConfig.renderLayer;
                m_ecsSystem->AddComponent<Sprite>(bgEntity, bgSprite);
                GN_LOG_INFO("Added Sprite component to entity " + std::to_string(bgEntity) + " with texture '" + layerConfig.textureId + "'" +
                           " textureSize=(" + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) + ")" +
                           " finalScale=" + std::to_string(finalScale) + " scaledSize=(" + std::to_string(scaledWidth) + "x" + std::to_string(scaledHeight) + ")");
                
                // Create and add Parallax component
                Parallax parallaxComponent;
                parallaxComponent.scrollSpeed = layerConfig.scrollSpeed;
                parallaxComponent.repeatWidth = repeatWidth;
                // For boss level (level 6), completely disable parallax scrolling
                if (m_currentLevelId == 6) {
                    // Don't add parallax component at all for boss level - static background
                    GN_LOG_INFO("Boss level: Skipping parallax component for static background - entity " +
                               std::to_string(bgEntity) + " for level " + std::to_string(m_currentLevelId));
                } else {
                    parallaxComponent.autoScroll = true;
                    parallaxComponent.scrollSpeed = layerConfig.scrollSpeed;
                    m_ecsSystem->AddComponent<Parallax>(bgEntity, parallaxComponent);
                    GN_LOG_INFO("Added Parallax component to entity " + std::to_string(bgEntity) +
                               " with scrollSpeed=" + std::to_string(parallaxComponent.scrollSpeed) +
                               " autoScroll=" + (parallaxComponent.autoScroll ? "true" : "false") +
                               " for level " + std::to_string(m_currentLevelId));
                }
                
                // Create and add ParallaxInstance component for better management
                ParallaxInstance instanceComponent(layerConfig.textureId, i, numInstances, scaledWidth);
                m_ecsSystem->AddComponent<ParallaxInstance>(bgEntity, instanceComponent);
                GN_LOG_INFO("Added ParallaxInstance component to entity " + std::to_string(bgEntity) + " (instance " + std::to_string(i) + " of " + std::to_string(numInstances) + ")");
                
                m_backgroundEntities.push_back(bgEntity);
                layerEntitiesCreated++;
                totalEntitiesCreated++;
                
                GN_LOG_INFO("✓ Successfully created background layer '" + layerConfig.textureId + 
                           "' instance " + std::to_string(i) + " (entity " + std::to_string(bgEntity) + ")" +
                           " at (" + std::to_string(xPos) + ", " + std::to_string(yPos) + ")" +
                           " with scale " + std::to_string(finalScale) +
                           " positionSpacing " + std::to_string(positionSpacing) +
                           " repeatWidth " + std::to_string(repeatWidth) +
                           " scaledWidth " + std::to_string(scaledWidth) +
                           " on render layer " + std::to_string(layerConfig.renderLayer));
            }
            
            GN_LOG_INFO("Created " + std::to_string(layerEntitiesCreated) + " entities for layer '" + layerConfig.textureId + "'");
        }
    }

    void GameplayState::DestroyGameEntities() {
        GN_LOG_INFO("Destroying game entities");
        
        if (!m_ecsSystem) {
            return;
        }
        
        // Destroy player
        if (m_playerEntity != 0) {
            m_ecsSystem->DestroyEntity(m_playerEntity);
            m_playerEntity = 0;
        }
        
        // Destroy camera
        if (m_cameraEntity != 0) {
            m_ecsSystem->DestroyEntity(m_cameraEntity);
            m_cameraEntity = 0;
        }
        
        // Destroy background entities
        for (Gnosis::Entity entity : m_backgroundEntities) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_backgroundEntities.clear();
        
        // Destroy obstacles
        for (Gnosis::Entity entity : m_obstacles) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_obstacles.clear();
        
        // Destroy pickups via system
        if (m_pickupSystem) {
            m_pickupSystem->ClearAll();
        }
        
        // Clear boss coin tracking (coins already destroyed above)
        m_activeBossCoins.clear();
        
        // Destroy projectiles via ProjectileSystem
        if (m_projectileSystem) {
            m_projectileSystem->Cleanup();
        }
        
        // Destroy enemies
        for (Gnosis::Entity entity : m_enemies) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_enemies.clear();
        
        // Destroy boss system entities (level 6 cleanup)
        if (m_bossSystem) {
            GN_LOG_INFO("Destroying boss system and Rat King entities");
            m_bossSystem.reset();  // This will call ~BossSystem() which cleans up arm entities
            m_bossHealthBar.reset();
        }
        
        // Destroy explosion system (level 6 cleanup)
        if (m_explosionSystem) {
            GN_LOG_INFO("Destroying explosion system");
            m_explosionSystem.reset();
        }
        
        // Destroy white fade overlay entity (level 6 cleanup)
        if (m_whiteFadeEntity != 0) {
            m_ecsSystem->DestroyEntity(m_whiteFadeEntity);
            m_whiteFadeEntity = 0;
            GN_LOG_INFO("Destroyed white fade overlay entity");
        }
    }

    void GameplayState::CreateUI() {
        GN_LOG_INFO("Creating UI elements - SIMPLIFIED for pipe counter only");
    
    if (!m_ecsSystem) {
        return;
    }
    
    // Re-enable coin counter UI with coin bag icon
    m_scoreTextEntity = 0;
    m_livesTextEntity = 0;
    m_coinsTextEntity = 0;
    m_coinBagEntity = 0;
    m_shootingZoneEntity = 0;

    // Create pipe counter text entity - centered under iPhone notch (top display)
    m_pipeCounterEntity = m_ecsSystem->CreateEntity();
    if (m_pipeCounterEntity != 0) {
        // Get safe area information for proper positioning under notch
        float safeLeft, safeTop, safeRight, safeBottom;
        if (m_uiSystem) {
            m_uiSystem->GetSafeArea(safeLeft, safeTop, safeRight, safeBottom);
        } else {
            // Fallback values for iPhone notch area
            safeLeft = 0.0f;
            safeTop = 44.0f;  // Standard notch height
            if (m_renderSystem) {
                const ScreenInfo& si = m_renderSystem->GetScreenInfo();
                safeRight = si.pixelWidth;
                safeBottom = si.pixelHeight;
            } else {
                safeRight = 1179.0f;  // Fallback iPhone 16 width
                safeBottom = 2556.0f; // Fallback iPhone 16 height
            }
        }
        
        // TOP placement (centered, 10% from top)
        float screenW = 1179.0f, screenH = 2556.0f;
        if (m_renderSystem) {
            const ScreenInfo& si2 = m_renderSystem->GetScreenInfo();
            screenW = si2.pixelWidth;
            screenH = si2.pixelHeight;
        }
        float centerX = screenW * 0.50f;        // centered horizontally
        float pipeCounterY = screenH * 0.10f;   // 10% from top
        GN_LOG_INFO("CreateUI: screenW=" + std::to_string(screenW) +
                     " screenH=" + std::to_string(screenH) +
                     " pipeCounter pos=(" + std::to_string(centerX) + "," + std::to_string(pipeCounterY) + ") top-center");
        
        Transform pipeTransform(Gnosis::GNVector2(centerX, pipeCounterY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_pipeCounterEntity, pipeTransform);
        GN_LOG_INFO("CreateUI: added Transform to pipe counter entity " + std::to_string(m_pipeCounterEntity) +
                    " pos=(" + std::to_string(centerX) + "," + std::to_string(pipeCounterY) + ")");
        
        // Create UIElement for pipe counter (HIGH PRIORITY LAYER for visibility)
        UIElement pipeCounter;
        pipeCounter.buttonText = "0";  // Just the number, no label
        pipeCounter.fontSize = 120.0f;  // Larger font for clear visibility
        pipeCounter.textOutlineWidth = 18.0f; // thicker outline for raster path
        // Enable outline for pipe counter via renderer delegates
        // RenderSystem will choose centered outlined path when both center flags are true
        pipeCounter.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White color
        pipeCounter.centerTextHorizontally = true;
        pipeCounter.centerTextVertically = true;
        pipeCounter.visible = true;
        pipeCounter.isEnabled = true;   // Must be enabled for UISystem to render
        pipeCounter.textLayer = 10;     // High UI layer to ensure on-top ordering
        pipeCounter.normalTextureId = "";  // Text-only (no background texture)
        m_ecsSystem->AddComponent<UIElement>(m_pipeCounterEntity, pipeCounter);
        GN_LOG_INFO("CreateUI: added UIElement to pipe counter entity fontSize=" + std::to_string(pipeCounter.fontSize));
        
        GN_LOG_INFO("Created pipe counter entity " + std::to_string(m_pipeCounterEntity) + " at (" + std::to_string(centerX) + ", " + std::to_string(pipeCounterY) + ") - top-center");
    } else {
        GN_LOG_ERROR("Failed to create pipe counter entity!");
    }
    
    // Create coin bag icon (32x32) at bottom-left - HIDE IN LEVEL 1
    if (m_currentLevelId != 1) {
        float screenW = 1179.0f, screenH = 2556.0f;
        if (m_renderSystem) {
            const ScreenInfo& si = m_renderSystem->GetScreenInfo();
            screenW = si.pixelWidth;
            screenH = si.pixelHeight;
        }
                   const float iconX = screenW * 0.01f; // Move coin bag from 2% to 1% from left edge
        const float iconY = screenH * 0.87f; // 87% from top (13% from bottom)
        const float bagScale = 8.0f;    // Scale 32x32 coin bag to 256x256
        
        // Store initial positions for reset consistency (DO NOT let these be overwritten later)
        m_coinBagInitialX = iconX;
        m_coinBagInitialY = iconY;
        
        m_coinBagEntity = m_ecsSystem->CreateEntity();
        if (m_coinBagEntity != 0) {
            Transform tr(Gnosis::GNVector2(m_coinBagInitialX, m_coinBagInitialY), 0.0f, Gnosis::GNVector2(bagScale, bagScale));
            m_ecsSystem->AddComponent<Transform>(m_coinBagEntity, tr);
            UIElement bag;
            bag.normalTextureId = "CoinBag";
            bag.visible = true;
            bag.isEnabled = true;
            bag.textLayer = 10;
            m_ecsSystem->AddComponent<UIElement>(m_coinBagEntity, bag);
            GN_LOG_INFO("Created coin bag icon at (" + std::to_string(iconX) + "," + std::to_string(iconY) + ")");
        }
        // Coin number to the right of the bag, vertically centered to it
        m_coinsTextEntity = m_ecsSystem->CreateEntity();
        if (m_coinsTextEntity != 0) {
            const float textX = m_coinBagInitialX + (32.0f * bagScale) + 8.0f; // bag width + small gap
            const float textY = m_coinBagInitialY + (32.0f * bagScale * 0.5f) + 24.0f; // nudge down a bit more
            
            // Store initial coin text position for reset consistency
            m_coinsTextInitialX = textX;
            m_coinsTextInitialY = textY;
            
            Transform tr(Gnosis::GNVector2(m_coinsTextInitialX, m_coinsTextInitialY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_coinsTextEntity, tr);
            // Use UIElement text rendering path
            UIElement ui;
            ui.buttonText = "0";
            ui.fontSize = 64.0f; // match pipe counter visual weight but can tune
            ui.textOutlineWidth = 10.0f;
            ui.textColor = Gnosis::GNColor(255, 215, 0, 255); // gold
            ui.centerTextHorizontally = false;
            ui.centerTextVertically = true; // center on icon
            ui.visible = true;
            ui.isEnabled = true;
            ui.textLayer = 10;
            m_ecsSystem->AddComponent<UIElement>(m_coinsTextEntity, ui);
            GN_LOG_INFO("Created coins text at (" + std::to_string(textX) + "," + std::to_string(textY) + ")");
        }
    } else {
        GN_LOG_INFO("Skipped coin counter for Level 1 (coins disabled)");
    }

    // Create shooting zone visual indicator (always created, visibility controlled by level)
    m_shootingZoneEntity = m_ecsSystem->CreateEntity();
    if (m_shootingZoneEntity != 0) {
            // Calculate shooting zone position relative to coin bag and counter (using cached screen dimensions)
            GN_LOG_INFO("Shooting zone: Using cached screen dimensions: " + std::to_string((int)m_cachedScreenWidth) + "x" + std::to_string((int)m_cachedScreenHeight));
            float coinBagX = m_cachedScreenWidth * 0.01f; // 1% from left (same as coin bag)
            float coinBagY = m_cachedScreenHeight * 0.85f; // 15% from bottom (same as coin bag)
        float coinBagScale = 8.0f; // Same scale as coin bag
        float coinBagHeight = 32.0f * coinBagScale; // 256.0f
        float coinBagWidth = 32.0f * coinBagScale; // 256.0f (coin bag width, not height)

        // Coin counter position: right of coin bag + 8px gap
        float coinCounterX = coinBagX + coinBagWidth + 8.0f;
        // Estimate coin counter width (rough estimate based on "Coins: 999" text at font size 64)
        float estimatedCoinCounterWidth = 200.0f; // Rough estimate for coin counter text width
        float coinCounterRightX = coinCounterX + estimatedCoinCounterWidth;

        // Position shooting zone: percentage-based positioning (80%-95% from top) - MUST MATCH PlayerControllerSystem
        float shootingZoneTopY = m_cachedScreenHeight * 0.80f; // 80% from top
        float shootingZoneBottomY = m_cachedScreenHeight * 0.95f; // 95% from top
        float shootingZoneHeight = shootingZoneBottomY - shootingZoneTopY;

        // Position to the right of coin counter (not coin bag)
        float shootingZoneLeftX = coinCounterRightX + 8.0f; // Same 8px gap from coin counter
            float shootingZoneRightX = m_cachedScreenWidth * 0.95f; // 5% from right edge

        // Calculate final dimensions and position
        float shootZoneWidth = shootingZoneRightX - shootingZoneLeftX;
        float shootZoneHeight = shootingZoneHeight; // Already calculated above
        float shootZoneStartX = shootingZoneLeftX;
        float shootZoneStartY = shootingZoneTopY;

        GN_LOG_INFO("Shooting zone positioned above coinbag:");
        GN_LOG_INFO("  Coin bag at: (" + std::to_string(coinBagX) + ", " + std::to_string(coinBagY) + ") height: " + std::to_string(coinBagHeight));
        GN_LOG_INFO("  Coin counter at: " + std::to_string(coinCounterX));
        GN_LOG_INFO("  Shooting zone: (" + std::to_string(shootZoneStartX) + ", " + std::to_string(shootZoneStartY) + ") size (" + std::to_string(shootZoneWidth) + "x" + std::to_string(shootZoneHeight) + ")");

        // Create transform component (position in pixel screen space)
        Transform shootingZoneTransform;
        shootingZoneTransform.position = Gnosis::GNVector2(shootZoneStartX, shootZoneStartY);
        shootingZoneTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
        shootingZoneTransform.rotation = 0.0f;
        m_ecsSystem->AddComponent<Transform>(m_shootingZoneEntity, shootingZoneTransform);

        // Create UIElement for the visual indicator (same pattern as coin bag)
        UIElement shootingZoneIndicator;
        shootingZoneIndicator.buttonText = "";  // No text, just visual
        shootingZoneIndicator.fontSize = 1.0f;  // Minimal
        shootingZoneIndicator.textColor = Gnosis::GNColor(128, 128, 128, 64); // Semi-transparent gray
        shootingZoneIndicator.centerTextHorizontally = false;

        shootingZoneIndicator.centerTextVertically = false;
        shootingZoneIndicator.visible = false; // Will be set correctly below
        shootingZoneIndicator.isEnabled = true;
        shootingZoneIndicator.textLayer = 10; // Same layer as coin bag and pipe counter
        shootingZoneIndicator.normalTextureId = ""; // No texture, just use background color
        m_ecsSystem->AddComponent<UIElement>(m_shootingZoneEntity, shootingZoneIndicator);

        // Create UIShape component for the actual rectangle rendering
        UIShape shootingZoneShape;
        shootingZoneShape.width = shootZoneWidth;  // Pixel width
        shootingZoneShape.height = shootZoneHeight; // Pixel height
        shootingZoneShape.color = Gnosis::GNColor(128, 128, 128, 64); // Semi-transparent gray
        shootingZoneShape.layer = 10; // Same layer as coin bag and pipe counter (top UI layer)
        shootingZoneShape.visible = false; // Will be set correctly below
        m_ecsSystem->AddComponent<UIShape>(m_shootingZoneEntity, shootingZoneShape);


        GN_LOG_INFO("Created shooting zone entity " + std::to_string(m_shootingZoneEntity) + " with UIElement and UIShape");

        // Immediately apply the current level's shooting zone visibility
        if (m_shootingZoneEntity != 0 && m_ecsSystem) {
            auto uiElement = m_ecsSystem->GetComponent<UIElement>(m_shootingZoneEntity);
            auto uiShape = m_ecsSystem->GetComponent<UIShape>(m_shootingZoneEntity);
            bool shouldBeVisible = m_currentLevelConfig.shootingEnabled;

            GN_LOG_INFO("CreateUI: Applying shooting zone visibility for level " + std::to_string(m_currentLevelId) + " - shootingEnabled=" + std::to_string(shouldBeVisible));

            if (uiElement) {
                uiElement->visible = shouldBeVisible;
                GN_LOG_INFO("CreateUI: Set UIElement visible = " + std::to_string(shouldBeVisible));
            } else {
                GN_LOG_ERROR("CreateUI: UIElement component not found!");
            }
            if (uiShape) {
                uiShape->visible = shouldBeVisible;
                GN_LOG_INFO("CreateUI: Set UIShape visible = " + std::to_string(shouldBeVisible));
            } else {
                GN_LOG_ERROR("CreateUI: UIShape component not found!");
            }
        }

        // Create debug rectangle for shooting zone
        // CreateDebugShootingZoneRectangle(); // Disabled for production
    }

    // Create settings button for pause menu
    CreateSettingsButton();

    // Create debug rectangle for shooting zone (after shooting zone is created in CreateUI)
    
    // Create pause menu system
    if (m_pauseSystem && !m_pauseSystem->IsVisible()) {
        m_pauseSystem->Initialize();
    }
    
    // Create heart UI - only if it doesn't already exist (for initial game start)
    if (m_heartSystem && m_heartUIEntity == 0) {
        // Position hearts to hug left side of screen, just below pipe counter
            float heartX = m_cachedScreenWidth * 0.01f;   // 1% from left edge to hug the left side
            float heartY = m_cachedScreenHeight * 0.12f;  // 12% from top (just below pipe counter)
        m_heartUIEntity = m_heartSystem->CreateHeartUI(heartX, heartY);
        if (m_heartUIEntity != Gnosis::INVALID_ENTITY) {
            GN_LOG_INFO("Created heart UI at (" + std::to_string(heartX) + ", " + std::to_string(heartY) + ")");
            // Set initial heart count and visibility based on difficulty (one-time setup)
            Difficulty currentDifficulty = LevelManager::GetGlobalDifficulty();
            m_heartSystem->UpdateHeartCountForDifficulty(m_playerEntity, currentDifficulty);
            GN_LOG_INFO("Set initial heart count for difficulty");
        } else {
            GN_LOG_ERROR("Failed to create heart UI");
        }
    } else if (m_heartSystem && m_heartUIEntity != 0) {
        GN_LOG_INFO("Heart UI already exists - skipping creation for restart scenario");
    }
}


 
void GameplayState::DestroyUI() {
    GN_LOG_INFO("Destroying UI elements");

    if (!m_ecsSystem) {
        return;
    }
    
    // Reset initial position trackers when destroying UI
    m_coinBagInitialX = 0.0f;
    m_coinBagInitialY = 0.0f;
    m_coinsTextInitialX = 0.0f;
    m_coinsTextInitialY = 0.0f;

    if (m_scoreTextEntity != 0) {
        m_ecsSystem->DestroyEntity(m_scoreTextEntity);
        m_scoreTextEntity = 0;
    }

    if (m_livesTextEntity != 0) {
        m_ecsSystem->DestroyEntity(m_livesTextEntity);
        m_livesTextEntity = 0;
    }

    if (m_coinsTextEntity != 0) {
        m_ecsSystem->DestroyEntity(m_coinsTextEntity);
        m_coinsTextEntity = 0;
    }
    if (m_coinBagEntity != 0) {
        m_ecsSystem->DestroyEntity(m_coinBagEntity);
        m_coinBagEntity = 0;
    }

    if (m_pipeCounterEntity != 0) {
        m_ecsSystem->DestroyEntity(m_pipeCounterEntity);
        m_pipeCounterEntity = 0;
    }

    if (m_shootingZoneEntity != 0) {
        m_ecsSystem->DestroyEntity(m_shootingZoneEntity);
        m_shootingZoneEntity = 0;
    }

    // Destroy pause menu system
    if (m_pauseSystem) {
        m_pauseSystem->Cleanup();
    }

    if (m_settingsButtonEntity != 0) {
        m_ecsSystem->DestroyEntity(m_settingsButtonEntity);
        m_settingsButtonEntity = 0;
    }

    if (m_heartUIEntity != 0) {
        m_ecsSystem->DestroyEntity(m_heartUIEntity);
        m_heartUIEntity = 0;
    }

    // Clean up all heart system entities
    if (m_heartSystem) {
        m_heartSystem->DestroyAllHeartUI();
    }
}

void GameplayState::UpdateGameLogic(float deltaTime) {
        // Update invulnerability timer
        if (m_invulnerabilityTimer > 0.0f) {
            m_invulnerabilityTimer -= deltaTime;
        }
        
        // Update pipe increment cooldown timer
        if (m_pipeIncrementCooldown > 0.0f) {
            m_pipeIncrementCooldown -= deltaTime;
        }
        
        // Update UI text
        if (m_scoreTextEntity != 0) {
            Text* scoreText = m_ecsSystem->GetComponent<Text>(m_scoreTextEntity);
            if (scoreText) {
                scoreText->text = "Score: " + std::to_string(m_currentScore);
            }
        }
        
        if (m_livesTextEntity != 0) {
            Text* livesText = m_ecsSystem->GetComponent<Text>(m_livesTextEntity);
            if (livesText) {
                livesText->text = "Lives: " + std::to_string(m_currentLives);
            }
        }
        
        if (m_coinsTextEntity != 0) {
            Text* coinsText = m_ecsSystem->GetComponent<Text>(m_coinsTextEntity);
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            if (coinsText && player) {
                coinsText->text = "Coins: " + std::to_string(player->sessionCoins);
            }
        }

        // One-shot UI reposition once actual PIXEL screen info is available (avoid 800x600 fallback)
        if (!m_uiPositionsSynced && m_renderSystem) {
            const ScreenInfo& si = m_renderSystem->GetScreenInfo();
            // Heuristic: real iPhone pixel widths are well above 800
            if (si.pixelWidth >= 1000.0f && si.pixelHeight >= 1000.0f) {
                // Pipe counter top-center placement (10% from top) - HIDE in boss level (6)
                float centerX = si.pixelWidth * 0.50f;
                float pipeCounterY = si.pixelHeight * 0.10f;

                if (m_currentLevelId != 6) { // Show pipe counter in non-boss levels
                    if (m_pipeCounterEntity != 0) {
                        Transform* t = m_ecsSystem->GetComponent<Transform>(m_pipeCounterEntity);
                        if (t) {
                            t->position.x = centerX;
                            t->position.y = pipeCounterY;
                        }
                    }
                } else {
                    // Hide pipe counter completely in boss level by moving it off-screen
                    if (m_pipeCounterEntity != 0) {
                        Transform* t = m_ecsSystem->GetComponent<Transform>(m_pipeCounterEntity);
                        if (t) {
                            t->position.x = -1000.0f; // Move off-screen
                            t->position.y = -1000.0f;
                        }
                    }
                }

                // DO NOT reposition coin bag here - let orientation-specific repositioning handle it
                // UpdateUILayoutForOrientation() is called when orientation changes
                // Resetting to initial values here breaks landscape positioning
                if (m_coinBagEntity != 0) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(m_coinBagEntity);
                    if (t) {
                        // Only maintain scale, don't override position
                        t->scale.x = 8.0f;
                        t->scale.y = 8.0f;
                    }
                }
                // Coin text position is relative to coin bag, so don't override it either

                // Settings button: 85% width, 5% height (moved left to avoid clipping)
                float menuX = si.pixelWidth * 0.85f;
                float menuY = si.pixelHeight * 0.05f;
                if (m_settingsButtonEntity != 0) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);
                    if (t) {
                        t->position.x = menuX;
                        t->position.y = menuY;
                        t->scale.x = 8.0f; // Keep 8x scale after sync
                        t->scale.y = 8.0f;
                    }
                }

                GN_LOG_INFO("UI Repositioned with PIXELS: screenW=" + std::to_string(si.pixelWidth) +
                            " screenH=" + std::to_string(si.pixelHeight) +
                            " pipeCounter=(" + std::to_string(centerX) + "," + std::to_string(pipeCounterY) + ") top-center" +
                            " menuPos=(" + std::to_string(menuX) + "," + std::to_string(menuY) + ")");
                m_uiPositionsSynced = true;
            }
        }
        
        // Periodically attempt to update screen info from iOS to get real iPhone dimensions
        static float screenInfoUpdateTimer = 0.0f;
        screenInfoUpdateTimer += deltaTime;
        if (screenInfoUpdateTimer >= 0.5f && m_renderSystem) { // Check every 500ms
            screenInfoUpdateTimer = 0.0f;
            const ScreenInfo& siBeforeUpdate = m_renderSystem->GetScreenInfo();
            m_renderSystem->UpdateScreenInfo(); // Force async screen info update
            const ScreenInfo& siAfterUpdate = m_renderSystem->GetScreenInfo();
            
            // Log if dimensions actually changed (async update completed)
            if (siBeforeUpdate.pixelWidth != siAfterUpdate.pixelWidth || siBeforeUpdate.pixelHeight != siAfterUpdate.pixelHeight) {
                GN_LOG_INFO("PAUSE MENU DEBUG: Screen dimensions updated from " + 
                           std::to_string((int)siBeforeUpdate.pixelWidth) + "x" + std::to_string((int)siBeforeUpdate.pixelHeight) + 
                           " to " + std::to_string((int)siAfterUpdate.pixelWidth) + "x" + std::to_string((int)siAfterUpdate.pixelHeight));
            }
        }
        
        // Attempt pause menu creation if not yet created and we have valid screen dimensions
        if (m_pauseSystem && !m_pauseSystem->IsVisible() && m_renderSystem && m_currentSubState == GameplaySubState::Paused) {
            const ScreenInfo& si = m_renderSystem->GetScreenInfo();
            // Check if we have valid iPhone screen dimensions (not fallback values)
            if (si.pixelWidth >= 1000.0f && si.pixelHeight >= 1000.0f) {
                GN_LOG_INFO("PAUSE MENU DEBUG: Valid screen dimensions now available, initializing pause menu");
                if (!m_pauseSystem->IsVisible()) {
                    m_pauseSystem->Initialize();
                }
            } else {
                // Log current dimensions for debugging
                static float debugTimer = 0.0f;
                debugTimer += deltaTime;
                if (debugTimer >= 2.0f) { // Log every 2 seconds while waiting
                    debugTimer = 0.0f;
                    GN_LOG_WARN("PAUSE MENU DEBUG: Still waiting for valid dimensions. Current: " + 
                               std::to_string((int)si.pixelWidth) + "x" + std::to_string((int)si.pixelHeight) + 
                               " (Expected: iPhone portrait like 1179x2556)");
                }
            }
        }
    }

    void GameplayState::UpdateObjectPools(float deltaTime) {
        // Update pooled entities (obstacles, enemies, NPCs) via LevelManager
        if (m_levelManager && m_cameraSystem) {
            // Get world scroll distance for wrapping calculations
            // Camera stays at (0,0), only world objects move, so use world scroll distance directly
            float worldScrollDistance = m_cameraSystem->GetWorldPosition();
            
            // Update obstacles using new ObstacleSystem (no redundant calls)
            m_levelManager->UpdateObstacleSystem(deltaTime, worldScrollDistance);
            
            // Update other pooling systems (non-obstacle)
            m_levelManager->UpdateEnemyPooling(deltaTime, worldScrollDistance);
            // NOTE: EnemySystem::Update() is now called AFTER ProjectileSystem::Update() 
            // (see line 286-289) to ensure collision detection has access to current projectile list

            m_levelManager->UpdateNPCPooling(deltaTime, worldScrollDistance);
            
            // Pickup logic handled centrally in Update()
            
            // States
            m_levelManager->UpdateNPCStates(deltaTime);
        }
    }

    void GameplayState::UpdateDifficulty(float deltaTime) {
        m_difficultyTimer += deltaTime;
        if (m_difficultyTimer >= DIFFICULTY_INCREASE_INTERVAL) {
            m_difficultyLevel = std::min(m_difficultyLevel + 0.5f, MAX_DIFFICULTY_LEVEL);
            m_difficultyTimer = 0.0f;
            GN_LOG_INFO("Difficulty increased to: " + std::to_string(m_difficultyLevel));
        }
    }

    void GameplayState::HandleGameEvents() {
        // Event handling will be implemented in Phase 2
    }

    void GameplayState::CleanupOffscreenEntities() {
        // Use LevelManager for cleanup
        if (m_levelManager) {
            m_levelManager->CleanupOffscreenEntities(-100.0f); // Left boundary
        }
    }

    void GameplayState::CheckLevelCompletion() {
        // Level completion logic will be implemented in Phase 3
        // For now, just check if player is still alive
        if (!m_playerAlive && m_currentLives <= 0) {
            GameOver();
        }
    }

    void GameplayState::SaveGameProgress() {
        GN_LOG_INFO("Saving game progress");
        // Save logic will be implemented in Phase 4
    }

    // Event handler implementations (will be expanded in Phase 2)
    void GameplayState::OnPlayerJump() {
        GN_LOG_INFO("Player jumped");
    }

    void GameplayState::OnPlayerShoot() {
        GN_LOG_INFO("Player shot");
    }

    void GameplayState::OnPlayerHurt(int damage) {
        GN_LOG_INFO("Player hurt with damage: " + std::to_string(damage));
        m_invulnerabilityTimer = 2.0f; // keep invulnerability to avoid spam
        
        // Damage the heart system
        if (m_heartSystem && m_playerEntity != 0) {
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            if (player) {
                // Remove heart slices based on damage (typically 1 slice per damage)
                int slicesToRemove = damage;
                m_heartSystem->RemoveHeartSlices(m_playerEntity, slicesToRemove);
                
                GN_LOG_INFO("Removed " + std::to_string(slicesToRemove) + " heart slices. Current slices: " + std::to_string(player->liveSlices));
                
                // Check if player is dead - try coin safety net first
                GN_LOG_INFO("Player damage check - live slices: " + std::to_string(player->liveSlices) +
                           ", ghost slices: " + std::to_string(player->ghostSlices) +
                           ", total hearts: " + std::to_string(player->hearts) +
                           ", heart mode: " + std::to_string(static_cast<int>(player->heartMode)));

                if (player->liveSlices <= 0) {
                    GN_LOG_INFO("Player reached 0 live slices - attempting coin safety net");
                    // Try to activate coin safety net if available
                    if (m_skillSystem && m_skillSystem->TryActivateCoinSafetyNet(m_playerEntity)) {
                        GN_LOG_INFO("Coin safety net activated! Player saved from death");
                        // Update the coin counter UI to reflect that all coins were spent
                        UpdateCoinCounterUI();
                        return; // Don't process death
                    } else {
                        GN_LOG_INFO("Coin safety net failed or not available - player will die");
                    }
                }
            }
        }
    }

    void GameplayState::OnPlayerDeath() {
        GN_LOG_INFO("Player died");
        m_playerAlive = false;
        // Death is now handled by the heart system and UpdateSubState
    }

    void GameplayState::OnCoinCollected(int value) {
        GN_LOG_INFO("Coin collected: " + std::to_string(value));

        // Log current coin counts BEFORE collection
        int beforePlayerCoins = GameCore::GetGame()->GetPlayerCoins();
        GN_LOG_INFO("💰 BEFORE coin collection - m_playerCoins: " + std::to_string(beforePlayerCoins));

        // Update session coin counter
        m_sessionCoinsCollected += value;

        // Update coin counter UI (HUD)
        UpdateCoinCounterUI();

        // Update PauseSystem with new stats data
        if (m_pauseSystem) {
            // Get total spendable coins (stored + session)
            int totalSpendableCoins = 0;
            int storedCoins = 0;
            if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
                storedCoins = player->totalCoins;
                totalSpendableCoins = player->totalCoins + player->sessionCoins;
            }

            // Get gross total coins from GameStats (authoritative source)
            int grossTotalCoins = GameCore::GetGame()->GetGameStats().totalCoinsCollected;
            GN_LOG_INFO("💰 Stats update - totalSpendable: " + std::to_string(totalSpendableCoins) + ", storedCoins: " + std::to_string(storedCoins) + ", grossTotal: " + std::to_string(grossTotalCoins) + ", sessionCoins: " + std::to_string(m_sessionCoinsCollected));

            m_pauseSystem->UpdateStatsData(m_pipesCleared, m_sessionCoinsCollected, totalSpendableCoins, grossTotalCoins, GameCore::GetGame()->GetGameStats().totalDeaths, GameCore::GetGame()->GetGameStats().totalEnemiesKilled, GameCore::GetGame()->GetGameStats().totalPipesCleared);
        }

        // Update player session coins and gross total
        if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
            player->sessionCoins += value;
            // Keep grossTotalCoins in sync with GameStats
            player->grossTotalCoins = GameCore::GetGame()->GetGameStats().totalCoinsCollected;
            GN_LOG_INFO("💰 Updated PlayerComponent::sessionCoins to: " + std::to_string(player->sessionCoins) +
                       ", grossTotalCoins to: " + std::to_string(player->grossTotalCoins));
        }

        // Update lifetime total coins in GameStats
        GameCore::FloppyTurdGame::GameStats gameStats = GameCore::GetGame()->GetGameStats();
        gameStats.totalCoinsCollected += value;
        GameCore::GetGame()->UpdateGameStats(gameStats);
    }

    void GameplayState::OnHeartCollected(int healAmount) {
        GN_LOG_INFO("Heart collected: " + std::to_string(healAmount) + " slices");

        // Heal the player using the heart system
        if (m_heartSystem && m_playerEntity != 0) {
            GN_LOG_INFO("Adding " + std::to_string(healAmount) + " heart slices via HeartSystem");
            m_heartSystem->AddHeartSlices(m_playerEntity, healAmount);

            // Log current heart state after healing
            auto* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            if (playerComp) {
                int currentSlices = m_heartSystem->GetCurrentSlices(m_playerEntity);
                int maxSlices = m_heartSystem->GetMaxSlices(m_playerEntity);
                GN_LOG_INFO("Heart slices after healing: " + std::to_string(currentSlices) + "/" + std::to_string(maxSlices));
            }
        } else {
            GN_LOG_ERROR("Heart collected but HeartSystem not available or player entity invalid");
        }
    }

    void GameplayState::OnPickupCollected() {
        GN_LOG_INFO("Pickup collected");
        m_currentScore += 50;
    }

    void GameplayState::OnObstacleHit() {
        GN_LOG_INFO("Obstacle hit");
        if (m_invulnerabilityTimer <= 0.0f) {
            OnPlayerHurt(1);
        }
    }

    void GameplayState::OnEnemyDefeated() {
        GN_LOG_INFO("Enemy defeated!");
        // Handle enemy defeat logic
    }
    
    // Menu navigation functions
    void GameplayState::ReturnToMainMenu() {
        GN_LOG_INFO("Returning to main menu from gameplay");
        
        // Update level high score with current progress (even if not completed)
        if (GameCore::GetGame()) {
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            int sessionCoins = player ? player->sessionCoins : 0;
            
            // Update level stats with current pipes cleared and coins
            GameCore::GetGame()->UpdateLevelHighScore(m_currentLevelId, m_pipesCleared, sessionCoins);
            GN_LOG_INFO("💾 Updated level " + std::to_string(m_currentLevelId) + " stats on menu return: " + std::to_string(m_pipesCleared) + " pipes, " + std::to_string(sessionCoins) + " coins");
        }
        
        // FINALITY EVENT: Transfer session coins to stored coins when returning to menu
        // (Will be 0 if coming from game over, preventing double-adding)
        PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
        if (player && player->sessionCoins > 0) {
            // Convert session coins to stored coins (don't use AddCoins as that would double-count gross total)
            if (GameCore::GetGame()) {
                GameCore::FloppyTurdGame::GameStats gameStats = GameCore::GetGame()->GetGameStats();
                gameStats.storedCoins += player->sessionCoins;
                // DON'T add to totalCoinsCollected - they were already counted when collected
                GameCore::GetGame()->UpdateGameStats(gameStats);
                GameCore::GetGame()->SaveGameData();

                // SYNC PlayerComponent with updated stored coins
                player->totalCoins = gameStats.storedCoins;
                GN_LOG_INFO("💰 PlayerComponent synced - totalCoins updated to: " + std::to_string(player->totalCoins));

                GN_LOG_INFO("💰 Menu return finality: Converted " + std::to_string(player->sessionCoins) + " session coins to stored coins (gross total unchanged: " + std::to_string(gameStats.totalCoinsCollected) + ")");
            }
        }
        
        m_finished = true;  // This will trigger state transition back to main menu
    }

    void GameplayState::CreateDebugButtonRectangle() {
        GN_LOG_INFO("Creating debug button rectangle");

        // Create a red rectangle to visualize button collision bounds
        m_debugButtonRect = m_ecsSystem->CreateEntity();
        if (m_debugButtonRect != 0) {
            // Initial position will be updated when bounds are calculated
            Transform rectTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_debugButtonRect, rectTransform);

            // Create red rectangle shape
            UIShape rectShape(UIShapeType::Rectangle, 128.0f, 128.0f, Gnosis::GNColor(255, 0, 0, 128)); // Semi-transparent red
            rectShape.layer = 20; // High layer to appear on top
            rectShape.visible = true;
            m_ecsSystem->AddComponent<UIShape>(m_debugButtonRect, rectShape);

            UIElement rectUI;
            rectUI.visible = true;
            rectUI.textLayer = 20;
            m_ecsSystem->AddComponent<UIElement>(m_debugButtonRect, rectUI);

            GN_LOG_INFO("Created debug button rectangle entity " + std::to_string(m_debugButtonRect));
        }
    }

    void GameplayState::UpdateDebugButtonRectangle(float left, float top, float right, float bottom) {
        if (m_debugButtonRect == 0 || !m_ecsSystem) return;

        // Position at the button bounds center
        float rectCenterX = (left + right) / 2.0f;
        float rectCenterY = (top + bottom) / 2.0f;
        float rectWidth = right - left;
        float rectHeight = bottom - top;

        // Update transform
        auto rectTransform = m_ecsSystem->GetComponent<Transform>(m_debugButtonRect);
        if (rectTransform) {
            rectTransform->position = Gnosis::GNVector2(rectCenterX, rectCenterY);
        }

        // Update shape dimensions
        auto uiShape = m_ecsSystem->GetComponent<UIShape>(m_debugButtonRect);
        if (uiShape) {
            uiShape->width = rectWidth;
            uiShape->height = rectHeight;
        }

        GN_LOG_INFO("Updated debug button rectangle: center(" + std::to_string(rectCenterX) + "," + std::to_string(rectCenterY) +
                   ") size(" + std::to_string(rectWidth) + "x" + std::to_string(rectHeight) + ")");
    }

    void GameplayState::CreateDebugShootingZoneRectangle() {
        GN_LOG_INFO("Creating debug shooting zone rectangle");

        // Create a blue rectangle to visualize shooting zone collision bounds
        m_debugShootingZoneRect = m_ecsSystem->CreateEntity();
        if (m_debugShootingZoneRect != 0) {
            // Initial position will be updated when bounds are calculated
            Transform rectTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_debugShootingZoneRect, rectTransform);

            // Create blue rectangle shape
            UIShape rectShape(UIShapeType::Rectangle, 100.0f, 100.0f, Gnosis::GNColor(0, 0, 255, 128)); // Semi-transparent blue
            rectShape.layer = 19; // High layer to appear on top but below button debug
            rectShape.visible = true;
            m_ecsSystem->AddComponent<UIShape>(m_debugShootingZoneRect, rectShape);

            UIElement rectUI;
            rectUI.visible = true;
            rectUI.textLayer = 19;
            m_ecsSystem->AddComponent<UIElement>(m_debugShootingZoneRect, rectUI);

            GN_LOG_INFO("Created debug shooting zone rectangle entity " + std::to_string(m_debugShootingZoneRect));
        }
    }

    void GameplayState::UpdateDebugShootingZoneRectangle(float left, float top, float right, float bottom) {
        if (m_debugShootingZoneRect == 0 || !m_ecsSystem) return;

        // Position at the shooting zone bounds center
        float zoneCenterX = (left + right) / 2.0f;
        float zoneCenterY = (top + bottom) / 2.0f;
        float zoneWidth = right - left;
        float zoneHeight = bottom - top;

        // Update transform
        auto zoneTransform = m_ecsSystem->GetComponent<Transform>(m_debugShootingZoneRect);
        if (zoneTransform) {
            zoneTransform->position = Gnosis::GNVector2(zoneCenterX, zoneCenterY);
        }

        // Update shape dimensions
        auto uiShape = m_ecsSystem->GetComponent<UIShape>(m_debugShootingZoneRect);
        if (uiShape) {
            uiShape->width = zoneWidth;
            uiShape->height = zoneHeight;
        }

        GN_LOG_INFO("Updated debug shooting zone rectangle: center(" + std::to_string(zoneCenterX) + "," + std::to_string(zoneCenterY) +
                   ") size(" + std::to_string(zoneWidth) + "x" + std::to_string(zoneHeight) + ")");
    }

    void GameplayState::CreateSettingsButton() {
        GN_LOG_INFO("Creating settings button");

        m_settingsButtonEntity = m_ecsSystem->CreateEntity();
        if (m_settingsButtonEntity != 0) {
            // Position in top-right corner in screen space, accounting for orientation
            bool isLandscape = IsLandscapeMode();
            float buttonX, buttonY;

            if (isLandscape) {
                // Use landscape-specific positioning constants
                buttonX = m_cachedScreenWidth * LANDSCAPE_SETTINGS_X;
                buttonY = m_cachedScreenHeight * LANDSCAPE_SETTINGS_Y;
            } else {
                // Portrait mode: top-right corner
                buttonX = m_cachedScreenWidth * PORTRAIT_SETTINGS_X;
                buttonY = m_cachedScreenHeight * PORTRAIT_SETTINGS_Y;
            }

            GN_LOG_INFO("Settings button position - landscape: " + std::to_string(isLandscape) +
                       ", pos: (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")" +
                       ", screen: " + std::to_string((int)m_cachedScreenWidth) + "x" + std::to_string((int)m_cachedScreenHeight) +
                       ", constants: X=" + std::to_string(isLandscape ? LANDSCAPE_SETTINGS_X : PORTRAIT_SETTINGS_X) +
                       ", Y=" + std::to_string(isLandscape ? LANDSCAPE_SETTINGS_Y : PORTRAIT_SETTINGS_Y));

            // Scale the button to 8x like other UI elements
            float buttonScale = 8.0f;

            // Position at fixed screen coordinates (this will be updated when UI is repositioned)
            Transform buttonTransform(Gnosis::GNVector2(buttonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            m_ecsSystem->AddComponent<Transform>(m_settingsButtonEntity, buttonTransform);

            // Create Sprite component with actual texture dimensions
            Sprite buttonSprite;
            buttonSprite.textureId = "settingsbutton";
            buttonSprite.width = 16.0f;  // Actual texture width
            buttonSprite.height = 16.0f; // Actual texture height
            m_ecsSystem->AddComponent<Sprite>(m_settingsButtonEntity, buttonSprite);

            // Create UIElement using settingsbutton.png - this keeps it fixed on screen like coin bag
            UIElement buttonUI;
            buttonUI.normalTextureId = "settingsbutton";
            buttonUI.visible = true;
            buttonUI.isEnabled = true;
            buttonUI.textLayer = 10; // Same layer as other UI elements
            m_ecsSystem->AddComponent<UIElement>(m_settingsButtonEntity, buttonUI);

            // Create bounds component matching the scaled sprite dimensions for proper hitbox detection
            float scaledWidth = buttonSprite.width * buttonScale;
            float scaledHeight = buttonSprite.height * buttonScale;
            Bounds buttonBounds(scaledWidth, scaledHeight, 0.0f, 0.0f, false); // Centered bounds
            m_ecsSystem->AddComponent<Bounds>(m_settingsButtonEntity, buttonBounds);

            GN_LOG_INFO("Created settings button at (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ") with scale " + std::to_string(buttonScale) +
                       " and bounds " + std::to_string(scaledWidth) + "x" + std::to_string(scaledHeight));

            // FORCE repositioning after creation to ensure correct positioning
            GN_LOG_INFO("Forcing settings button repositioning after creation");
            UpdateUILayoutForOrientation();

            // DEBUG: Create a red rectangle to visualize button collision bounds
            // CreateDebugButtonRectangle(); // Disabled for production
        }
    }

    void GameplayState::IncrementDeathCounter() {
        GN_LOG_INFO("Incrementing death counter");

        // Update the game's death counter
        if (GameCore::GetGame()) {
            // Get current stats, increment death counter, and update
            GameCore::FloppyTurdGame::GameStats stats = GameCore::GetGame()->GetGameStats();
            stats.totalDeaths++;
            GameCore::GetGame()->UpdateGameStats(stats);
            GN_LOG_INFO("Death counter incremented in game stats: " + std::to_string(stats.totalDeaths));
        } else {
            GN_LOG_WARN("GameCore::GetGame() returned null - cannot increment death counter");
        }
    }

    void GameplayState::StartLevelMusic() {
        GN_LOG_INFO("Starting level music for level: " + std::to_string(m_currentLevelId));
        
        if (!m_platformDelegates || !m_platformDelegates->audio.playMusic) {
            GN_LOG_WARN("Audio delegate not available - cannot play level music");
            return;
        }
        
        // Get the level config from LevelManager which has difficulty applied
        const LevelConfig* levelConfig = nullptr;
        if (m_levelManager && m_levelManager->IsLevelLoaded()) {
            levelConfig = &m_levelManager->GetCurrentLevelConfig();
        } else {
            // Fallback to our local config if LevelManager isn't available
            levelConfig = &m_currentLevelConfig;
        }
        
        // Get the appropriate music file for the current difficulty
        std::string musicFile = levelConfig->GetMusicForDifficulty();
        
        if (musicFile.empty()) {
            GN_LOG_WARN("No music configured for level " + std::to_string(m_currentLevelId));
            return;
        }
        
        GN_LOG_INFO("Playing music: " + musicFile + " (Difficulty: " + DifficultyToString(levelConfig->currentDifficulty) + ")");
        
        // Play music with loop (-1 for infinite loop) and appropriate volume
        float musicVolume = 0.7f; // 70% volume for gameplay music
        m_platformDelegates->audio.playMusic(musicFile.c_str(), musicVolume, -1);
        
        GN_LOG_INFO("Level music started: " + musicFile);
    }

    void GameplayState::StopLevelMusic() {
        GN_LOG_INFO("Stopping level music");
        
        if (!m_platformDelegates || !m_platformDelegates->audio.stopMusic) {
            GN_LOG_WARN("Audio delegate not available - cannot stop level music");
            return;
        }
        
        m_platformDelegates->audio.stopMusic();
        GN_LOG_INFO("Level music stopped");
    }
    
    // Platform-specific layout implementation
    void GameplayState::SetupLayout() {
#ifdef PLATFORM_IOS
        SetupIOSLayout();
#else
        SetupDesktopLayout();
#endif
    }
    
    void GameplayState::SetupIOSLayout() {
        GN_LOG_INFO("GameplayState: Setting up iOS layout");
        
        // Get dynamic screen info from render system
        if (m_renderSystem) {
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            float dynamicScale = m_renderSystem->GetDynamicScale();
            
            GN_LOG_INFO("GameplayState: iOS layout - Screen: " + 
                       std::to_string((int)screenInfo.logicalWidth) + "x" + 
                       std::to_string((int)screenInfo.logicalHeight) + 
                       ", Scale: " + std::to_string(dynamicScale));
            
            // iOS-specific gameplay layout adjustments can go here
            // e.g., adjusting spawn positions, UI element positions, etc.
        }
    }
    
    void GameplayState::SetupDesktopLayout() {
        GN_LOG_INFO("GameplayState: Setting up desktop layout");

        // Get dynamic screen info from render system
        if (m_renderSystem) {
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            float dynamicScale = m_renderSystem->GetDynamicScale();

            GN_LOG_INFO("GameplayState: Desktop layout - Screen: " +
                       std::to_string((int)screenInfo.logicalWidth) + "x" +
                       std::to_string((int)screenInfo.logicalHeight) +
                       ", Scale: " + std::to_string(dynamicScale));

            // Desktop-specific gameplay layout adjustments can go here
        }
    }

    // Orientation-specific UI management implementation
    bool GameplayState::IsLandscapeMode() const {
        if (!m_renderSystem) {
            return false; // Default to portrait if no render system
        }
        const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
        return !screenInfo.isPortrait;
    }

    void GameplayState::UpdateUILayoutForOrientation() {
        GN_LOG_INFO("GameplayState: Updating UI layout for orientation change");

        // Get current screen info for debugging
        if (m_renderSystem) {
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            GN_LOG_INFO("GameplayState: Current screen dimensions: " + std::to_string(screenInfo.pixelWidth) + "x" +
                       std::to_string(screenInfo.pixelHeight) + ", isPortrait: " + std::to_string(screenInfo.isPortrait));
        }

        if (IsLandscapeMode()) {
            GN_LOG_INFO("GameplayState: Repositioning UI for LANDSCAPE mode");
            RepositionUIElementsLandscape();
        } else {
            GN_LOG_INFO("GameplayState: Repositioning UI for PORTRAIT mode");
            RepositionUIElementsPortrait();
        }

        std::string orientationStr = IsLandscapeMode() ? "landscape" : "portrait";
        GN_LOG_INFO("GameplayState: UI layout updated for " + orientationStr + " mode");
    }

    void GameplayState::RepositionUIElementsLandscape() {
        GN_LOG_INFO("GameplayState: Repositioning UI elements for landscape mode");

        if (!m_renderSystem) {
            return;
        }

        const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
        float screenW = screenInfo.pixelWidth;
        float screenH = screenInfo.pixelHeight;
        
        // Rescale and reposition backgrounds for landscape orientation
        RescaleBackgroundsForOrientation(true, screenW, screenH);

        // Reposition settings button
        if (m_settingsButtonEntity != 0 && m_ecsSystem) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);
            if (transform) {
                float buttonX = screenW * LANDSCAPE_SETTINGS_X;
                float buttonY = screenH * LANDSCAPE_SETTINGS_Y;
                GN_LOG_INFO("Repositioning settings button - screen: " + std::to_string((int)screenW) + "x" + std::to_string((int)screenH) +
                           ", constants: X=" + std::to_string(LANDSCAPE_SETTINGS_X) + ", Y=" + std::to_string(LANDSCAPE_SETTINGS_Y) +
                           ", calculated pos: (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")" +
                           ", current pos: (" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ")");
                transform->position = Gnosis::GNVector2(buttonX, buttonY);
                GN_LOG_INFO("Repositioned settings button to landscape position: (" +
                           std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")");
            }
        }

        // Reposition coin bag and coins text using stored initial positions to ensure consistency
    if (m_coinBagEntity != 0 && m_ecsSystem) {
        Transform* bagTransform = m_ecsSystem->GetComponent<Transform>(m_coinBagEntity);
        if (bagTransform) {
            float bagX = screenW * LANDSCAPE_COINBAG_X;
            float bagY = screenH * LANDSCAPE_COINBAG_Y;
                
            // DO NOT update stored positions - keep original from CreateUI
            // Just reposition the entity
                
            bagTransform->position = Gnosis::GNVector2(bagX, bagY);
                GN_LOG_INFO("Repositioned coin bag to landscape position: (" +
                           std::to_string(bagX) + ", " + std::to_string(bagY) + ")");

                // Reposition coins text relative to coin bag
                if (m_coinsTextEntity != 0) {
                    Transform* textTransform = m_ecsSystem->GetComponent<Transform>(m_coinsTextEntity);
                    if (textTransform) {
                        const float bagScale = 8.0f;
                        float textX = bagX + (32.0f * bagScale) + 8.0f;
                        float textY = bagY + (32.0f * bagScale * 0.5f) + 24.0f;
                        
                        // DO NOT update stored positions - keep original from CreateUI
                        
                        textTransform->position = Gnosis::GNVector2(textX, textY);
                        GN_LOG_INFO("Repositioned coins text to landscape position: (" +
                                   std::to_string(textX) + ", " + std::to_string(textY) + ")");
                    }
                }
            }
        }

        // Reposition pipe counter
        if (m_pipeCounterEntity != 0 && m_ecsSystem) {
            Transform* pipeTransform = m_ecsSystem->GetComponent<Transform>(m_pipeCounterEntity);
            if (pipeTransform) {
                float centerX = screenW * 0.50f;
                float pipeY = screenH * LANDSCAPE_PIPE_Y;
                pipeTransform->position = Gnosis::GNVector2(centerX, pipeY);
                GN_LOG_INFO("Repositioned pipe counter to landscape position: (" +
                           std::to_string(centerX) + ", " + std::to_string(pipeY) + ")");
            }
        }

        // Reposition shooting zone
        if (m_shootingZoneEntity != 0 && m_ecsSystem) {
            // Recalculate shooting zone position for landscape
            float coinBagX = screenW * LANDSCAPE_COINBAG_X;
            float coinBagY = screenH * LANDSCAPE_COINBAG_Y;
            const float bagScale = 8.0f;
            float coinBagHeight = 32.0f * bagScale;
            float coinBagWidth = 32.0f * bagScale;

            // Coin counter position: right of coin bag + 8px gap
            float coinCounterX = coinBagX + coinBagWidth + 8.0f;
            // Estimate coin counter width
            float estimatedCoinCounterWidth = 200.0f;
            float coinCounterRightX = coinCounterX + estimatedCoinCounterWidth;

            // Shooting zone position - percentage-based positioning for landscape (80%-95%) - MUST MATCH PlayerControllerSystem
            float shootingZoneTopY = screenH * 0.80f; // 80% from top in landscape
            float shootingZoneBottomY = screenH * 0.95f; // 95% from top in landscape
            float shootingZoneHeight = shootingZoneBottomY - shootingZoneTopY;
            float shootingZoneLeftX = coinCounterRightX + 8.0f;
            float shootingZoneRightX = screenW * 0.95f;
            float shootZoneWidth = shootingZoneRightX - shootingZoneLeftX;
            float shootZoneStartX = shootingZoneLeftX;
            float shootZoneStartY = shootingZoneTopY;

            Transform* zoneTransform = m_ecsSystem->GetComponent<Transform>(m_shootingZoneEntity);
            if (zoneTransform) {
                zoneTransform->position = Gnosis::GNVector2(shootZoneStartX, shootZoneStartY);
            }

            // Update UIShape dimensions
            UIShape* uiShape = m_ecsSystem->GetComponent<UIShape>(m_shootingZoneEntity);
            if (uiShape) {
                uiShape->width = shootZoneWidth;
                uiShape->height = shootingZoneHeight;
            }

            GN_LOG_INFO("Repositioned shooting zone for landscape: (" +
                       std::to_string(shootZoneStartX) + ", " + std::to_string(shootZoneStartY) +
                       ") size (" + std::to_string(shootZoneWidth) + "x" + std::to_string(shootingZoneHeight) + ")");

            // Update debug shooting zone rectangle
            // UpdateDebugShootingZoneRectangle(shootingZoneLeftX, shootingZoneTopY, shootingZoneRightX, shootingZoneBottomY); // Disabled for production
        }

        // Update heart system positioning
        if (m_heartSystem && m_heartUIEntity != Gnosis::INVALID_ENTITY) {
            float heartX = screenW * LANDSCAPE_COINBAG_X; // Same X as coin bag
            float heartY = screenH * LANDSCAPE_SETTINGS_Y; // Start from settings button level
            m_heartSystem->UpdateHeartUIPositioning(m_heartUIEntity, heartX, heartY);
        }
    }

    void GameplayState::RepositionSettingsButtonForPauseMenu(bool isPauseMenuActive) {
        GN_LOG_INFO("GameplayState: Repositioning settings button for pause menu - active: " + std::to_string(isPauseMenuActive));

        if (m_settingsButtonEntity == 0 || !m_ecsSystem) {
            GN_LOG_WARN("RepositionSettingsButtonForPauseMenu: Settings button entity not available");
            return;
        }

        Transform* transform = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);
        if (!transform) {
            GN_LOG_WARN("RepositionSettingsButtonForPauseMenu: Settings button transform not found");
            return;
        }

        float buttonScale = 8.0f;
        float buttonWidth = 16.0f * buttonScale;
        float buttonHeight = 16.0f * buttonScale;

        float buttonX, buttonY;

        if (isPauseMenuActive) {
            // Move settings button to bottom right when pause menu is active (in iPhone curve area)
            buttonX = m_cachedScreenWidth - buttonWidth - 10.0f;  // 10px from right edge (further right)
            buttonY = m_cachedScreenHeight - buttonHeight - 10.0f; // 10px from bottom edge (in curve area)
            GN_LOG_INFO("RepositionSettingsButtonForPauseMenu: Moving to bottom right (" +
                       std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")");
        } else {
            // Restore to normal gameplay position
            if (IsLandscapeMode()) {
                buttonX = m_cachedScreenWidth * LANDSCAPE_SETTINGS_X;
                buttonY = m_cachedScreenHeight * LANDSCAPE_SETTINGS_Y;
            } else {
                buttonX = m_cachedScreenWidth * PORTRAIT_SETTINGS_X;
                buttonY = m_cachedScreenHeight * PORTRAIT_SETTINGS_Y;
            }
            GN_LOG_INFO("RepositionSettingsButtonForPauseMenu: Restoring to normal position (" +
                       std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")");
        }

        // Use CenterObjectAtPosition to properly center the button
        GNVector2 buttonPosition = CenterObjectAtPosition(buttonX, buttonY, buttonWidth, buttonHeight);
        transform->position = buttonPosition;

        // Update bounds for collision detection
        Bounds* bounds = m_ecsSystem->GetComponent<Bounds>(m_settingsButtonEntity);
        if (bounds) {
            bounds->width = buttonWidth;
            bounds->height = buttonHeight;
        }

        GN_LOG_INFO("RepositionSettingsButtonForPauseMenu: Settings button repositioned to (" +
                   std::to_string(buttonPosition.x) + ", " + std::to_string(buttonPosition.y) + ")");

        // Update debug rectangle if it exists
        // UpdateDebugButtonRectangle(buttonPosition.x, buttonPosition.y, buttonPosition.x + buttonWidth, buttonPosition.y + buttonHeight); // Disabled for production
    }

    void GameplayState::RepositionUIElementsPortrait() {
        GN_LOG_INFO("GameplayState: Repositioning UI elements for portrait mode");

        if (!m_renderSystem) {
            return;
        }

        const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
        float screenW = screenInfo.pixelWidth;
        float screenH = screenInfo.pixelHeight;
        
        // Rescale and reposition backgrounds for portrait orientation
        RescaleBackgroundsForOrientation(false, screenW, screenH);

        // Reposition settings button to portrait position
        if (m_settingsButtonEntity != 0 && m_ecsSystem) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);
            if (transform) {
                float buttonX = screenW * PORTRAIT_SETTINGS_X;
                float buttonY = screenH * PORTRAIT_SETTINGS_Y;
                GN_LOG_INFO("Repositioning settings button to portrait - screen: " + std::to_string((int)screenW) + "x" + std::to_string((int)screenH) +
                           ", constants: X=" + std::to_string(PORTRAIT_SETTINGS_X) + ", Y=" + std::to_string(PORTRAIT_SETTINGS_Y) +
                           ", calculated pos: (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")" +
                           ", current pos: (" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ")");
                transform->position = Gnosis::GNVector2(buttonX, buttonY);
                GN_LOG_INFO("Repositioned settings button to portrait position: (" +
                           std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")");
            }
        }

        // Reposition coin bag and coins text to portrait position
        // Reposition coin bag and coins text using stored initial positions to ensure consistency
    if (m_coinBagEntity != 0 && m_ecsSystem) {
        Transform* bagTransform = m_ecsSystem->GetComponent<Transform>(m_coinBagEntity);
        if (bagTransform) {
            float bagX = screenW * PORTRAIT_COINBAG_X;
            float bagY = screenH * PORTRAIT_COINBAG_Y;
                
            // DO NOT update stored positions - keep original from CreateUI
            // Just reposition the entity
                
            bagTransform->position = Gnosis::GNVector2(bagX, bagY);
                GN_LOG_INFO("Repositioned coin bag to portrait position: (" +
                           std::to_string(bagX) + ", " + std::to_string(bagY) + ")");

                // Reposition coins text relative to coin bag
                if (m_coinsTextEntity != 0) {
                    Transform* textTransform = m_ecsSystem->GetComponent<Transform>(m_coinsTextEntity);
                    if (textTransform) {
                        const float bagScale = 8.0f;
                        float textX = bagX + (32.0f * bagScale) + 8.0f;
                        float textY = bagY + (32.0f * bagScale * 0.5f) + 24.0f;
                        
                        // DO NOT update stored positions - keep original from CreateUI
                        
                        textTransform->position = Gnosis::GNVector2(textX, textY);
                        GN_LOG_INFO("Repositioned coins text to portrait position: (" +
                                   std::to_string(textX) + ", " + std::to_string(textY) + ")");
                    }
                }
            }
        }

        // Reposition pipe counter to portrait position
        if (m_pipeCounterEntity != 0 && m_ecsSystem) {
            Transform* pipeTransform = m_ecsSystem->GetComponent<Transform>(m_pipeCounterEntity);
            if (pipeTransform) {
                float centerX = screenW * 0.50f;
                float pipeY = screenH * PORTRAIT_PIPE_Y;
                pipeTransform->position = Gnosis::GNVector2(centerX, pipeY);
                GN_LOG_INFO("Repositioned pipe counter to portrait position: (" +
                           std::to_string(centerX) + ", " + std::to_string(pipeY) + ")");
            }
        }

        // Reposition shooting zone to portrait position
        if (m_shootingZoneEntity != 0 && m_ecsSystem) {
            // Recalculate shooting zone position for portrait
            float coinBagX = screenW * PORTRAIT_COINBAG_X;
            float coinBagY = screenH * PORTRAIT_COINBAG_Y;
            const float bagScale = 8.0f;
            float coinBagHeight = 32.0f * bagScale;
            float coinBagWidth = 32.0f * bagScale;

            // Coin counter position: right of coin bag + 8px gap
            float coinCounterX = coinBagX + coinBagWidth + 8.0f;
            // Estimate coin counter width
            float estimatedCoinCounterWidth = 200.0f;
            float coinCounterRightX = coinCounterX + estimatedCoinCounterWidth;

            // Shooting zone position - percentage-based positioning for portrait (80%-95%) - MUST MATCH PlayerControllerSystem
            float shootingZoneTopY = screenH * 0.80f; // 80% from top in portrait
            float shootingZoneBottomY = screenH * 0.95f; // 95% from top in portrait
            float shootingZoneHeight = shootingZoneBottomY - shootingZoneTopY;
            float shootingZoneLeftX = coinCounterRightX + 8.0f;
            float shootingZoneRightX = screenW * 0.95f;
            float shootingZoneWidth = shootingZoneRightX - shootingZoneLeftX;
            float shootingZoneStartX = shootingZoneLeftX;
            float shootingZoneStartY = shootingZoneTopY;

            Transform* zoneTransform = m_ecsSystem->GetComponent<Transform>(m_shootingZoneEntity);
            if (zoneTransform) {
                zoneTransform->position = Gnosis::GNVector2(shootingZoneStartX, shootingZoneStartY);
            }

            // Update UIShape dimensions
            UIShape* uiShape = m_ecsSystem->GetComponent<UIShape>(m_shootingZoneEntity);
            if (uiShape) {
                uiShape->width = shootingZoneWidth;
                uiShape->height = shootingZoneHeight;
            }

            GN_LOG_INFO("Repositioned shooting zone for portrait: (" +
                       std::to_string(shootingZoneStartX) + ", " + std::to_string(shootingZoneStartY) +
                       ") size (" + std::to_string(shootingZoneWidth) + "x" + std::to_string(shootingZoneHeight) + ")");

            // Update debug shooting zone rectangle
            // UpdateDebugShootingZoneRectangle(shootingZoneLeftX, shootingZoneTopY, shootingZoneRightX, shootingZoneBottomY); // Disabled for production
        }

        // Update heart system positioning
        if (m_heartSystem && m_heartUIEntity != Gnosis::INVALID_ENTITY) {
            float heartX = screenW * PORTRAIT_COINBAG_X; // Same X as coin bag
            float heartY = screenH * PORTRAIT_SETTINGS_Y; // Start from settings button level
            m_heartSystem->UpdateHeartUIPositioning(m_heartUIEntity, heartX, heartY);
        }
    }
    
    void GameplayState::CheckToiletCollisions() {
        if (!m_playerAlive || !m_ecsSystem || !m_levelManager) {
            return;
        }
        
        // Get player transform, hitbox, and sprite
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        Hitbox* playerHitbox = m_ecsSystem->GetComponent<Hitbox>(m_playerEntity);
        Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        
        // Check if player fell off screen and trigger damage
        if (playerTransform && playerSprite && m_playerAlive) {
            float playerHeight = playerSprite->height * playerTransform->scale.y;
            float resetThreshold = m_cachedScreenHeight + (playerHeight * 0.75f);
            
            // If player is below threshold and invulnerability is off, trigger damage and reset
            if (playerTransform->position.y > resetThreshold && m_invulnerabilityTimer <= 0.0f) {
                GN_LOG_INFO("💔 Player fell off screen - triggering damage and reset!");
                
                // Play hurt sound effect
                if (m_platformDelegates && m_platformDelegates->audio.playSound) {
                    m_platformDelegates->audio.playSound("hurt.mp3", 0.8f);
                }
                
                // Reset player position immediately
                playerTransform->position.y = 50.0f; // TOP_SPAWN_Y
                Physics* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
                if (physics) {
                    physics->velocity.y = 0.0f;
                }
                
                // Trigger damage (will handle hurt animation and invulnerability)
                OnPlayerHurt(1); // Take 1 damage for falling off
                return; // Skip other collision checks this frame
            }
        }
        if (!playerTransform || !playerHitbox || !playerSprite) {
            return;
        }
        // Player circle collision (center-based): transform position is top-left; add sprite half-dimensions
        float pHalfW = playerSprite ? (playerSprite->width * playerTransform->scale.x * 0.5f) : 0.0f;
        float pHalfH = playerSprite ? (playerSprite->height * playerTransform->scale.y * 0.5f) : 0.0f;
        float pCenterX = playerTransform->position.x + pHalfW + (playerHitbox->offsetX * playerTransform->scale.x);
        float pCenterY = playerTransform->position.y + pHalfH + (playerHitbox->offsetY * playerTransform->scale.y);
        float pRadius  = playerHitbox->radius * ((playerTransform->scale.x + playerTransform->scale.y) * 0.5f);
        
        // Performance: Disabled per-frame player hitbox logging
        // GN_LOG_DEBUG("Player hitbox: position=(" + std::to_string(playerTransform->position.x) + "," + std::to_string(playerTransform->position.y) + ") " +
        //             "center=(" + std::to_string(pCenterX) + "," + std::to_string(pCenterY) + ") " +
        //             "radius=" + std::to_string(pRadius));
        
        // Check collision with all active obstacles (toilets) via ObstacleSystem
        const auto& activeObstacles = m_levelManager->GetActiveObstacles();
        // Performance: Disabled per-frame obstacle count logging
        // GN_LOG_DEBUG("Checking collisions with " + std::to_string(activeObstacles.size()) + " active obstacles");
        
        bool playerHitThisFrame = false; // Track if player was hit this frame
        
        for (Gnosis::Entity obstacleEntity : activeObstacles) {
            Transform* obstacleTransform = m_ecsSystem->GetComponent<Transform>(obstacleEntity);
            Sprite* obstacleSprite = m_ecsSystem->GetComponent<Sprite>(obstacleEntity);
            Obstacle* obstacle = m_ecsSystem->GetComponent<Obstacle>(obstacleEntity);
            Hitbox* obstacleHitbox = m_ecsSystem->GetComponent<Hitbox>(obstacleEntity);
            
            if (!obstacleTransform || !obstacleSprite || !obstacle || !obstacleHitbox) {
                // Performance: Disabled per-obstacle component checking logs
                // if (!obstacleTransform) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Transform component");
                // else if (!obstacleSprite) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Sprite component");
                // else if (!obstacle) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Obstacle component");
                // else if (!obstacleHitbox) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Hitbox component");
                continue;
            }
            // Calculate rectangle bounds (needed for both collision and pipe clearing)
            float rectW = obstacleHitbox->width * obstacleTransform->scale.x;
            float rectH = obstacleHitbox->height * obstacleTransform->scale.y;
            float spriteHalfW = obstacleSprite ? (obstacleSprite->width * obstacleTransform->scale.x * 0.5f) : 0.0f;
            float spriteHalfH = obstacleSprite ? (obstacleSprite->height * obstacleTransform->scale.y * 0.5f) : 0.0f;
            float rectCenterX = obstacleTransform->position.x + spriteHalfW + (obstacleHitbox->offsetX * obstacleTransform->scale.x);
            float rectCenterY = obstacleTransform->position.y + spriteHalfH + (obstacleHitbox->offsetY * obstacleTransform->scale.y);
            float rectX = rectCenterX - (rectW * 0.5f);
            float rectY = rectCenterY - (rectH * 0.5f);
            
            // Check collision based on obstacle hitbox type
            // Performance: Disabled per-frame collision logging
            // GN_LOG_INFO("COLLISION DEBUG: Processing obstacle " + std::to_string(obstacleEntity) + " (" + obstacle->obstacleType + ") hitboxType=" + std::to_string(static_cast<int>(obstacleHitbox->type)));
            
            bool collided = false;
            
            if (obstacleHitbox->type == ColliderType::Circle) {
                // Circle-Circle collision (e.g., player vs spike ball)
                float oCenterX, oCenterY;
                
                // Special handling for SpikeBalls - use BASE entity for positioning
                if (obstacle->type == ObstacleType::SpikeBall && m_levelManager && m_levelManager->GetObstacleSystem()) {
                    Gnosis::Entity baseEntity = m_levelManager->GetObstacleSystem()->GetSpikeBallBaseEntity(obstacleEntity);
                    if (baseEntity != 0) {
                        Transform* baseTransform = m_ecsSystem->GetComponent<Transform>(baseEntity);
                        if (baseTransform) {
                            // Calculate BASE center (base uses top-left positioning)
                            float baseCenterX = baseTransform->position.x + (5.0f * baseTransform->scale.x);
                            float baseCenterY = baseTransform->position.y + (5.0f * baseTransform->scale.y);
                            
                            // Apply hitbox offset (which contains rotation calculation)
                            oCenterX = baseCenterX + (obstacleHitbox->offsetX * baseTransform->scale.x);
                            oCenterY = baseCenterY + (obstacleHitbox->offsetY * baseTransform->scale.y);
                            
                            GN_LOG_INFO("SPIKEBALL BASE COLLISION: baseCenter=(" + std::to_string(baseCenterX) + "," + std::to_string(baseCenterY) + ") " +
                                       "hitboxOffset=(" + std::to_string(obstacleHitbox->offsetX) + "," + std::to_string(obstacleHitbox->offsetY) + ") " +
                                       "finalCenter=(" + std::to_string(oCenterX) + "," + std::to_string(oCenterY) + ")");
                        } else {
                            // Fallback to normal calculation if base transform not found
                            oCenterX = rectCenterX;
                            oCenterY = rectCenterY;
                            GN_LOG_INFO("SpikeBall base transform not found, using fallback positioning");
                        }
                    } else {
                        // Fallback to normal calculation if base entity not found  
                        oCenterX = rectCenterX;
                        oCenterY = rectCenterY;
                        GN_LOG_INFO("SpikeBall base entity not found, using fallback positioning");
                    }
                } else {
                    // Normal circle entities use standard center calculation
                    oCenterX = rectCenterX;
                    oCenterY = rectCenterY;
                }
                
                float oRadius = obstacleHitbox->radius * ((obstacleTransform->scale.x + obstacleTransform->scale.y) * 0.5f);
                
                float dx = pCenterX - oCenterX;
                float dy = pCenterY - oCenterY;
                float distanceSquared = dx * dx + dy * dy;
                float radiusSum = pRadius + oRadius;
                collided = distanceSquared <= (radiusSum * radiusSum);
                
                GN_LOG_INFO("SPIKE BALL COLLISION: pCenter=(" + std::to_string(pCenterX) + "," + std::to_string(pCenterY) + ") " +
                           "oCenter=(" + std::to_string(oCenterX) + "," + std::to_string(oCenterY) + ") " +
                           "pRadius=" + std::to_string(pRadius) + " oRadius=" + std::to_string(oRadius) + " " +
                           "distance=" + std::to_string(std::sqrt(distanceSquared)) + " radiusSum=" + std::to_string(radiusSum) + " collided=" + std::to_string(collided));
            } else {
                // Circle-Rectangle collision (e.g., player vs toilet)
                float closestX = std::max(rectX, std::min(pCenterX, rectX + rectW));
                float closestY = std::max(rectY, std::min(pCenterY, rectY + rectH));
                float dx = pCenterX - closestX;
                float dy = pCenterY - closestY;
                collided = (dx * dx + dy * dy) <= (pRadius * pRadius);
                
                // Performance: Disabled per-frame collision logging
                // GN_LOG_DEBUG("Circle-Rectangle: rect=(" + std::to_string(rectX) + "," + std::to_string(rectY) + "," + std::to_string(rectW) + "," + std::to_string(rectH) + ") " +
                //            "closest=(" + std::to_string(closestX) + "," + std::to_string(closestY) + ") collided=" + std::to_string(collided));
            }
            
            // Enhanced debug logging
            if (collided) {
                GN_LOG_INFO("COLLISION DETECTED! Entity: " + std::to_string(obstacleEntity) + 
                           " Type: " + std::to_string(static_cast<int>(obstacle->type)) + 
                           " HitboxType: " + std::to_string(static_cast<int>(obstacleHitbox->type)) + 
                           " Invulnerable: " + std::to_string(m_invulnerabilityTimer > 0.0f) + 
                           " Timer: " + std::to_string(m_invulnerabilityTimer));
            } else {
                // Performance: Disabled per-frame collision logging
                // GN_LOG_DEBUG("No collision with obstacle " + std::to_string(obstacleEntity) + 
                //            " (" + obstacle->obstacleType + ") type=" + std::to_string(static_cast<int>(obstacleHitbox->type)));
            }
            
            if (collided && m_invulnerabilityTimer <= 0.0f) {
                // Collision detected and player is not invulnerable!
                GN_LOG_INFO("*** COLLISION DETECTED! *** Entity: " + std::to_string(obstacleEntity) + " Type: " + std::to_string(static_cast<int>(obstacle->type)) + " HitboxType: " + std::to_string(static_cast<int>(obstacleHitbox->type)));
                playerHitThisFrame = true; // Mark that player was hit, but continue processing other obstacles
            } else if (collided) {
                GN_LOG_INFO("Collision detected but player invulnerable. Entity: " + std::to_string(obstacleEntity) + " Type: " + std::to_string(static_cast<int>(obstacle->type)) + " Timer: " + std::to_string(m_invulnerabilityTimer));
            }
            
            // Performance: Disabled per-frame pipe clearing logging
            // GN_LOG_DEBUG("Pipe clearing check for obstacle " + std::to_string(obstacleEntity) + 
            //             " (" + obstacle->obstacleType + ") - pipeCleared=" + std::to_string(obstacle->pipeCleared));
            
            // Calculate pipe clearing variables for this obstacle
            float playerRight = pCenterX + pRadius;
            float pipeCenterX = rectX + rectW * 0.5f;
            
            // Increment once per column (X-range) as the player passes its center.
            // This prevents double count when top and bottom exist at similar X.
            if (!obstacle->pipeCleared) {
                // Performance: Disabled per-frame pipe clearing logging
                // GN_LOG_DEBUG("Pipe clearing check: obstacle " + std::to_string(obstacleEntity) + 
                //             " (" + obstacle->obstacleType + "), playerRight=" + std::to_string(playerRight) + 
                //             ", pipeCenterX=" + std::to_string(pipeCenterX));
                
                if (playerRight > pipeCenterX) {
                    // Mark this obstacle as cleared
                    if (obstacle->type != ObstacleType::BrickWall) {
                        obstacle->pipeCleared = true;
                        
                        // Also mark paired entity if it exists (top/bottom toilet pair)
                        if (obstacle->pairedEntity != 0) {
                            if (Obstacle* pairedObstacle = m_ecsSystem->GetComponent<Obstacle>(obstacle->pairedEntity)) {
                                pairedObstacle->pipeCleared = true;
                            }
                        }
                        
                        // ANTI-CHEESE: Don't award pipe clear if player is invulnerable (hurt phase)
                        // This prevents players from cheesing pipes by taking damage to get through
                        if (m_invulnerabilityTimer > 0.0f) {
                            GN_LOG_INFO("Pipe crossed while invulnerable - NO SCORE (anti-cheese) at X=" + std::to_string(pipeCenterX));
                        } else {
                            // Use cooldown-based approach: only increment if cooldown expired
                            // This prevents stacked sewer pipes from counting multiple times (100ms = 0.1s cooldown)
                            if (m_pipeIncrementCooldown <= 0.0f) {
                                OnPipeCleared();
                                m_pipeIncrementCooldown = 0.1f; // 100ms cooldown before next increment allowed
                                GN_LOG_INFO("Pipe cleared at X=" + std::to_string(pipeCenterX) + " (cooldown reset)");
                            } else {
                                GN_LOG_INFO("Pipe cleared at X=" + std::to_string(pipeCenterX) + " (cooldown active, skipping increment)");
                            }
                        }
                    }
                }
            } else {
                // Performance: Disabled pipe clearing logging
                // GN_LOG_DEBUG("Player hasn't passed pipe center yet: playerRight=" + std::to_string(playerRight) + 
                //            ", pipeCenterX=" + std::to_string(pipeCenterX));
            }
        }
        
        // Performance: Disabled per-frame collision loop summary logging
        // GN_LOG_DEBUG("Finished collision detection loop for " + std::to_string(activeObstacles.size()) + " obstacles");
        
        // Check snowball/TP collisions with player (circle vs circle)
        // CRITICAL: Skip projectile collisions if boss death sequence is active (use m_bossDying flag)
        if (m_projectileSystem && !playerHitThisFrame && m_invulnerabilityTimer <= 0.0f && !m_bossDying) {
            const auto& enemyProjectiles = m_projectileSystem->GetActiveEnemyProjectiles();
            
            // Debug counter for periodic logging
            static int tpCollisionCheckCounter = 0;
            bool shouldLogThisFrame = (++tpCollisionCheckCounter % 30 == 0); // Log every 30 frames
            
            if (playerTransform && playerHitbox && playerSprite) {
                // Calculate player center (reuse from above)
                
                if (shouldLogThisFrame && enemyProjectiles.size() > 0) {
                    GN_LOG_INFO("[TP_CHECK] Checking " + std::to_string(enemyProjectiles.size()) + " enemy projectiles vs player at (" + 
                               std::to_string(pCenterX) + "," + std::to_string(pCenterY) + ") radius=" + std::to_string(pRadius));
                }
                
                for (Entity projectile : enemyProjectiles) {
                    Transform* projTransform = m_ecsSystem->GetComponent<Transform>(projectile);
                    Hitbox* projHitbox = m_ecsSystem->GetComponent<Hitbox>(projectile);
                    Projectile* projData = m_ecsSystem->GetComponent<Projectile>(projectile);
                    
                    if (projTransform && projHitbox && projData && projData->isActive) {
                        // Calculate projectile center - CONSISTENT with player/enemy calculations
                        // Position is top-left, so: center = position + spriteHalfDimensions + (offset * scale)
                        Sprite* projSprite = m_ecsSystem->GetComponent<Sprite>(projectile);
                        float projHalfW = projSprite ? (projSprite->width * projTransform->scale.x * 0.5f) : 0.0f;
                        float projHalfH = projSprite ? (projSprite->height * projTransform->scale.y * 0.5f) : 0.0f;
                        float projCenterX = projTransform->position.x + projHalfW + (projHitbox->offsetX * projTransform->scale.x);
                        float projCenterY = projTransform->position.y + projHalfH + (projHitbox->offsetY * projTransform->scale.y);
                        
                        // Calculate radius with scale applied
                        float projRadius = projHitbox->radius * ((projTransform->scale.x + projTransform->scale.y) * 0.5f);
                        
                        // Circle-circle collision
                        float dx = pCenterX - projCenterX;
                        float dy = pCenterY - projCenterY;
                        float distanceSquared = dx * dx + dy * dy;
                        float distance = std::sqrt(distanceSquared);
                        float combinedRadius = pRadius + projRadius;
                        
                        // Log detailed collision info periodically
                        if (shouldLogThisFrame && distance < 300.0f) { // Only log if within 300px
                            GN_LOG_INFO("[TP_COLLISION_DETAIL] Proj@(" + std::to_string(projTransform->position.x) + "," + std::to_string(projTransform->position.y) + 
                                       ") sprite=" + std::to_string(projSprite ? projSprite->width : 0) + "x" + std::to_string(projSprite ? projSprite->height : 0) +
                                       " scale=" + std::to_string(projTransform->scale.x) + 
                                       " offset=(" + std::to_string(projHitbox->offsetX) + "," + std::to_string(projHitbox->offsetY) + ")" +
                                       " radius=" + std::to_string(projHitbox->radius) + " (unscaled)");
                            GN_LOG_INFO("[TP_COLLISION_CALC] projHalfW=" + std::to_string(projHalfW) + " projHalfH=" + std::to_string(projHalfH) +
                                       " projCenter=(" + std::to_string(projCenterX) + "," + std::to_string(projCenterY) + ")" +
                                       " projRadius=" + std::to_string(projRadius) + " (scaled)");
                            GN_LOG_INFO("[TP_COLLISION_TEST] distance=" + std::to_string(distance) + 
                                       " combinedRadius=" + std::to_string(combinedRadius) + 
                                       " collision=" + std::to_string(distanceSquared <= (combinedRadius * combinedRadius)));
                        }
                        
                        if (distanceSquared <= (combinedRadius * combinedRadius)) {
                            GN_LOG_INFO("*** TP COLLISION DETECTED! *** distance=" + std::to_string(distance) + 
                                       ", combinedRadius=" + std::to_string(combinedRadius) +
                                       ", playerCenter=(" + std::to_string(pCenterX) + "," + std::to_string(pCenterY) + ")" +
                                       ", projCenter=(" + std::to_string(projCenterX) + "," + std::to_string(projCenterY) + ")" +
                                       ", playerRadius=" + std::to_string(pRadius) + ", projRadius=" + std::to_string(projRadius));
                            playerHitThisFrame = true;
                            
                            // Mark projectile as inactive (ProjectileSystem will clean it up)
                            projData->isActive = false;
                            break; // Only one hit per frame
                        }
                    }
                }
            }
        }
        
        // Check enemy collisions with player (circle vs circle) - exclude snowmen
        // CRITICAL: Skip all enemy collisions if boss death sequence is active (use m_bossDying flag)
        if (m_levelManager && !playerHitThisFrame && m_invulnerabilityTimer <= 0.0f && !m_bossDying) {
            const auto& activeEnemies = m_levelManager->GetActiveEnemies();
            
            // DEBUG: Log collision check start
            static int collisionCheckCounter = 0;
            if (collisionCheckCounter++ % 60 == 0) {
                GN_LOG_INFO("[ENEMY_COLLISION_CHECK] Checking " + std::to_string(activeEnemies.size()) + 
                           " enemies, player at (" + std::to_string(pCenterX) + "," + std::to_string(pCenterY) + 
                           "), radius=" + std::to_string(pRadius));
            }
            
            if (playerTransform && playerHitbox && playerSprite) {
                int enemyIndex = 0;
                for (Entity enemy : activeEnemies) {
                    Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
                    Transform* enemyTransform = m_ecsSystem->GetComponent<Transform>(enemy);
                    Hitbox* enemyHitbox = m_ecsSystem->GetComponent<Hitbox>(enemy);
                    Sprite* enemySprite = m_ecsSystem->GetComponent<Sprite>(enemy);
                    
                    if (!enemyComp || !enemyTransform || !enemyHitbox || !enemySprite) {
                        GN_LOG_WARN("[ENEMY_COLLISION_CHECK] Enemy " + std::to_string(enemyIndex) + " missing components");
                        enemyIndex++;
                        continue;
                    }
                    if (!enemyComp->isActive) {
                        enemyIndex++;
                        continue;
                    }
                    
                    // Skip ALL snowmen - they don't hurt on contact and can't be killed by player touch
                    // (only SnowmanThrower's snowballs hurt, decorative snowmen are just scenery)
                    bool isSnowman = (enemyComp->enemyType.find("SnowMan") != std::string::npos || 
                                     enemyComp->enemyType.find("Snowman") != std::string::npos);
                    if (isSnowman) {
                        enemyIndex++;
                        continue;
                    }
                    
                    // Skip Rat King boss - player shouldn't take damage from touching the boss itself
                    // (boss projectiles will still damage the player)
                    bool isRatKing = (enemyComp->enemyType == "Ratking");
                    if (isRatKing) {
                        enemyIndex++;
                        continue;
                    }
                    
                    // Calculate enemy center position
                    float enemyHalfW = enemySprite->width * enemyTransform->scale.x * 0.5f;
                    float enemyHalfH = enemySprite->height * enemyTransform->scale.y * 0.5f;
                    float enemyCenterX = enemyTransform->position.x + enemyHalfW + (enemyHitbox->offsetX * enemyTransform->scale.x);
                    float enemyCenterY = enemyTransform->position.y + enemyHalfH + (enemyHitbox->offsetY * enemyTransform->scale.y);
                    float enemyRadius = enemyHitbox->radius * ((enemyTransform->scale.x + enemyTransform->scale.y) * 0.5f);
                    
                    // Circle-circle collision
                    float dx = pCenterX - enemyCenterX;
                    float dy = pCenterY - enemyCenterY;
                    float distance = std::sqrt(dx * dx + dy * dy);
                    float distanceSquared = dx * dx + dy * dy;
                    float combinedRadius = pRadius + enemyRadius;
                    
                    // DEBUG: Log near misses (within 200px)
                    if (distance < 200.0f && collisionCheckCounter % 30 == 0) {
                        GN_LOG_INFO("[ENEMY_COLLISION_CHECK] Near miss: Enemy " + enemyComp->enemyType + 
                                   " at (" + std::to_string(enemyCenterX) + "," + std::to_string(enemyCenterY) + 
                                   "), distance=" + std::to_string(distance) + 
                                   ", combinedRadius=" + std::to_string(combinedRadius) + 
                                   ", enemyRadius=" + std::to_string(enemyRadius));
                    }
                    
                    if (distanceSquared <= (combinedRadius * combinedRadius)) {
                        GN_LOG_INFO("*** ENEMY COLLISION DETECTED! *** Enemy: " + enemyComp->enemyType + 
                                   " Pattern: " + enemyComp->movementPattern + 
                                   " distance=" + std::to_string(distance) + 
                                   ", combinedRadius=" + std::to_string(combinedRadius) +
                                   ", playerCenter=(" + std::to_string(pCenterX) + "," + std::to_string(pCenterY) + ")" +
                                   ", enemyCenter=(" + std::to_string(enemyCenterX) + "," + std::to_string(enemyCenterY) + ")");
                        playerHitThisFrame = true;
                        
                        // Damage the enemy (they also get hurt when touching player)
                        enemyComp->health -= 1;
                        
                        if (enemyComp->health <= 0) {
                            // Enemy defeated - mark as hurt, will be cleaned up by EnemySystem
                            enemyComp->currentState = EnemyState::Hurt;
                            enemyComp->hurtTimer = 0.6f;
                            
                            GN_LOG_INFO("[PLAYER_ENEMY_COLLISION] Enemy " + enemyComp->enemyType + " defeated");
                        }
                        
                        break; // Only one hit per frame
                    }
                    
                    enemyIndex++;
                }
            }
        }
        
        // Handle hurt effects after processing ALL collisions (obstacles + snowballs + enemies)
        if (playerHitThisFrame) {
            GN_LOG_INFO("Player hurt this frame! Applying hurt effects after processing all collisions");
            
            // Set invulnerability timer to prevent repeated hits
            m_invulnerabilityTimer = 1.0f;  // 1 second invulnerability
            
            // Play hurt sound effect
            if (m_platformDelegates && m_platformDelegates->audio.playSound) {
                m_platformDelegates->audio.playSound("hurt.mp3", 0.8f);  // 80% volume
            }
            
            // Trigger hurt state through PlayerControllerSystem - it will handle the animation and return to idle automatically
            if (m_playerControllerSystem) {
                m_playerControllerSystem->PlayHurtAnimation();
            }
            
            // Call the existing hurt handler for consistency
            OnPlayerHurt(1);
            
            GN_LOG_INFO("Player hurt - invulnerable for 1 second, PlayerControllerSystem managing hurt animation");
        }
    }
    
    void GameplayState::UpdatePipeCounterUI() {
        if (m_pipeCounterEntity == 0 || !m_ecsSystem) {
            return;
        }
        
        UIElement* pipeCounter = m_ecsSystem->GetComponent<UIElement>(m_pipeCounterEntity);
        if (pipeCounter) {
            pipeCounter->buttonText = std::to_string(m_pipesCleared);  // Just the number
        }
    }

    void GameplayState::UpdateCoinCounterUI() {
        if (m_coinsTextEntity == 0 || !m_ecsSystem) {
            return;
        }
        UIElement* coinsUi = m_ecsSystem->GetComponent<UIElement>(m_coinsTextEntity);
        if (coinsUi) {
            // Read from PlayerComponent::sessionCoins instead of m_sessionCoinsCollected
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            int coinCount = player ? player->sessionCoins : 0;
            coinsUi->buttonText = std::to_string(coinCount);
        }
    }
    
    void GameplayState::OnPipeCleared() {
        m_pipesCleared++;
        GN_LOG_INFO("Pipe cleared! Total pipes: " + std::to_string(m_pipesCleared));

        // Update lifetime total pipes cleared in GameStats
        if (GameCore::GetGame()) {
            GameCore::FloppyTurdGame::GameStats gameStats = GameCore::GetGame()->GetGameStats();
            gameStats.totalPipesCleared++;
            GameCore::GetGame()->UpdateGameStats(gameStats);
            GN_LOG_INFO("Updated lifetime total pipes cleared to: " + std::to_string(gameStats.totalPipesCleared));
        }

        // Update PauseSystem with new stats data (pipes changed!)
        if (m_pauseSystem) {
            // Get total spendable coins (stored + session)
            int totalSpendableCoins = 0;
            if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
                totalSpendableCoins = player->totalCoins + player->sessionCoins;
            }

            // Get gross total coins from GameStats (authoritative source)
            int grossTotalCoins = GameCore::GetGame()->GetGameStats().totalCoinsCollected;
            GN_LOG_INFO("📊 Pipe stats update - totalSpendable: " + std::to_string(totalSpendableCoins) + ", grossTotal: " + std::to_string(grossTotalCoins) + ", sessionCoins: " + std::to_string(m_sessionCoinsCollected) + ", pipes: " + std::to_string(m_pipesCleared));

            m_pauseSystem->UpdateStatsData(m_pipesCleared, m_sessionCoinsCollected, totalSpendableCoins, grossTotalCoins, GameCore::GetGame()->GetGameStats().totalDeaths, GameCore::GetGame()->GetGameStats().totalEnemiesKilled, GameCore::GetGame()->GetGameStats().totalPipesCleared);
        }

        // Update score (optional)
        m_currentScore += 10; // 10 points per pipe

        // Play bubble pop sound when clearing a pipe
        if (m_platformDelegates && m_platformDelegates->audio.playSound) {
            // Use dedicated bubble sound from environment sfx
            m_platformDelegates->audio.playSound("bubble.mp3", 0.7f);
        }
    }

    void GameplayState::DrawDebugRectangles() {
        GN_LOG_INFO("DrawDebugRectangles: Called");
        if (!m_platformDelegates || !m_platformDelegates->renderer.drawRectangle) {
            GN_LOG_ERROR("DrawDebugRectangles: Platform delegates or drawRectangle not available");
            return;
        }
        if (!m_ecsSystem) {
            GN_LOG_ERROR("DrawDebugRectangles: ECS system not available");
            return;
        }

        // Camera X for world->screen conversion
        float camX = 0.0f;
        Camera* camera = m_ecsSystem->GetComponent<Camera>(m_cameraEntity);
        if (camera) {
            camX = camera->position.x;
        } else {
            Transform* camTransform = m_ecsSystem->GetComponent<Transform>(m_cameraEntity);
            if (camTransform) camX = camTransform->position.x;
        }

        // Debug rectangles are now handled by RenderSystem via DebugDraw components.
        // Legacy manual rectangles removed to prevent duplicates and mismatches.
    }

    void GameplayState::TriggerGameOver() {
        if (m_currentSubState != GameplaySubState::Playing) {
            return; // Already in game over or paused
        }

        GN_LOG_INFO("Game Over triggered - player reached 0 hearts, starting fall sequence");
        
        // Trigger haptic feedback for game over
        if (m_platformDelegates) {
            HapticHelpers::TriggerGameOver(*m_platformDelegates);
        }

        // Update level high score with current session progress before death
        if (GameCore::GetGame()) {
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            int sessionCoins = player ? player->sessionCoins : 0;

            GameCore::GetGame()->UpdateLevelHighScore(m_currentLevelId, m_pipesCleared, sessionCoins);
            GN_LOG_INFO("Updated level " + std::to_string(m_currentLevelId) + " high score: " + std::to_string(m_pipesCleared) + " pipes, " + std::to_string(sessionCoins) + " coins");
            
            // FINALITY EVENT: Transfer session coins to stored coins on death
            if (sessionCoins > 0) {
                // Update GameStats stored coins (session coins were already counted in gross total when collected)
                GameCore::FloppyTurdGame::GameStats gameStats = GameCore::GetGame()->GetGameStats();
                gameStats.storedCoins += sessionCoins;
                // DON'T add to totalCoinsCollected again - they were already counted when collected
                GameCore::GetGame()->UpdateGameStats(gameStats);
                GameCore::GetGame()->SaveGameData();

                // SYNC PlayerComponent with updated stored coins
                if (PlayerComponent* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
                    playerComp->totalCoins = gameStats.storedCoins;
                    GN_LOG_INFO("💰 PlayerComponent synced - totalCoins updated to: " + std::to_string(playerComp->totalCoins));
                }

                GN_LOG_INFO("💰 Death finality: Added " + std::to_string(sessionCoins) + " session coins to stored coins. New storedCoins: " + std::to_string(gameStats.storedCoins) + " (gross total unchanged: " + std::to_string(gameStats.totalCoinsCollected) + ")");
            }
        }

        // Clear all active projectiles to prevent hits during game over screen
        if (m_projectileSystem) {
            m_projectileSystem->ResetForNewGame();
            GN_LOG_INFO("Cleared all projectiles on game over");
        }

        // Increment death counter for stats tracking
        IncrementDeathCounter();
        
        // NOTE: Ad will be triggered when game over UI is created (after player falls off screen)
        // This gives player time to see their death animation before showing the ad
        
        m_currentSubState = GameplaySubState::GameOver;
        m_gameOverTimer = 0.0f;
        m_morteFloatOffset = 0.0f;
        
        // Set player as not alive in PlayerControllerSystem to prevent position reset
        if (m_playerControllerSystem) {
            m_playerControllerSystem->SetPlayerAlive(false);
        }
        
        // Give dead player initial downward velocity so they can actually fall
        if (m_playerEntity != 0 && m_ecsSystem) {
            Physics* playerPhysics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            if (playerPhysics) {
                playerPhysics->velocity.y = 300.0f; // Tripled initial downward velocity for faster fall to game over screen
                GN_LOG_INFO("Set dead player initial downward velocity: " + std::to_string(playerPhysics->velocity.y));
            }
        }
        
        // Immediately hide hearts when player dies
        if (m_heartSystem && m_playerEntity != 0) {
            m_heartSystem->UpdateHeartVisibility(m_playerEntity);
        }
        
        // Hide settings button during game over
        if (m_settingsButtonEntity != 0 && m_ecsSystem) {
            UIElement* settingsUI = m_ecsSystem->GetComponent<UIElement>(m_settingsButtonEntity);
            if (settingsUI) {
                settingsUI->visible = false;
            }
        }
        
        // Play gameover stinger immediately
        if (m_platformDelegates && m_platformDelegates->audio.playSound) {
            m_platformDelegates->audio.playSound("gameover.mp3", 1.0f);
        }
        
        // Player will fall naturally due to physics, input will be locked
        // UI will be shown when player falls completely off screen
        GN_LOG_INFO("Player falling... Input locked. UI will appear when player falls off screen");
    }

    void GameplayState::TriggerPause() {
        if (m_currentSubState == GameplaySubState::Playing) {
            m_currentSubState = GameplaySubState::Paused;
            GN_LOG_INFO("Game paused");
        }
    }

    void GameplayState::TriggerResume() {
        if (m_currentSubState == GameplaySubState::Paused) {
            m_currentSubState = GameplaySubState::Playing;
            GN_LOG_INFO("Game resumed");
        }
    }

    void GameplayState::CreateGameOverUI() {
        if (!m_ecsSystem) {
            return;
        }
        
        // Explicitly hide boss health bar during game over
        if (m_bossHealthBar) {
            m_bossHealthBar->SetVisible(false);
            GN_LOG_INFO("Boss health bar explicitly hidden for game over screen");
        }
        
        // Check if landscape mode
        bool isLandscape = IsLandscapeMode();
        
        if (isLandscape) {
            // LANDSCAPE LAYOUT: Dead turd on left, scoreboard + buttons on right
            
            // Create game over background - LEFT HALF ONLY (starts from left edge)
            m_gameOverBackgroundEntity = m_ecsSystem->CreateEntity();
            if (m_gameOverBackgroundEntity != Gnosis::INVALID_ENTITY) {
                // Scale to cover left half of screen, positioned from left edge
                float backgroundScale = (m_cachedScreenWidth * 0.5f) / 64.0f;
                float bgX = 0.0f;  // Start from left edge
                Transform bgTransform(Gnosis::GNVector2(bgX, 0.0f), 0.0f, Gnosis::GNVector2(backgroundScale, backgroundScale));
                m_ecsSystem->AddComponent<Transform>(m_gameOverBackgroundEntity, bgTransform);
                
                Sprite bgSprite("GameOverBackground", 64, 64);
                bgSprite.layer = 100;
                bgSprite.visible = true;
                m_ecsSystem->AddComponent<Sprite>(m_gameOverBackgroundEntity, bgSprite);
            }
            
            // Create morte sprite (floating on LEFT side)
            m_morteEntity = m_ecsSystem->CreateEntity();
            if (m_morteEntity != Gnosis::INVALID_ENTITY) {
                float morteScale = 6.0f;
                float morteWidth = 64.0f * morteScale;
                float morteHeight = 64.0f * morteScale;
                
                // Position morte on left side, vertically centered
                Gnosis::GNVector2 mortePosition = CenterObjectAtPosition(m_cachedScreenWidth * 0.25f, m_cachedScreenHeight * 0.5f, morteWidth, morteHeight);
                
                Transform morteTransform(Gnosis::GNVector2(mortePosition.x, mortePosition.y), 0.0f, Gnosis::GNVector2(morteScale, morteScale));
                m_ecsSystem->AddComponent<Transform>(m_morteEntity, morteTransform);
                
                Sprite morteSprite("FloppyTurdMorte", 64, 64);
                morteSprite.layer = 102;
                morteSprite.visible = true;
                m_ecsSystem->AddComponent<Sprite>(m_morteEntity, morteSprite);
                
                GN_LOG_INFO("Created morte sprite (landscape) at: (" + std::to_string(mortePosition.x) + ", " + std::to_string(mortePosition.y) + ")");
            }
            
            // Create score display background on RIGHT side
            m_gameOverScoreEntity = m_ecsSystem->CreateEntity();
            if (m_gameOverScoreEntity != Gnosis::INVALID_ENTITY) {
                float scoreScale = (m_cachedScreenWidth * 0.35f) / 64.0f; // Smaller in landscape
                float scoreWidth = 64.0f * scoreScale;
                float scoreHeight = 32.0f * scoreScale;
                
                // Position on right side, upper area
                Gnosis::GNVector2 scorePosition = CenterObjectAtPosition(m_cachedScreenWidth * 0.70f, m_cachedScreenHeight * 0.30f, scoreWidth, scoreHeight);
                
                Transform scoreTransform(Gnosis::GNVector2(scorePosition.x, scorePosition.y), 0.0f, Gnosis::GNVector2(scoreScale, scoreScale));
                m_ecsSystem->AddComponent<Transform>(m_gameOverScoreEntity, scoreTransform);
                
                UIElement scoreUI;
                scoreUI.buttonText = "";
                scoreUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
                scoreUI.visible = true;
                scoreUI.isEnabled = true;
                scoreUI.textLayer = 101;
                scoreUI.normalTextureId = "GameOverScore";
                scoreUI.fontSize = 60.0f; // Smaller font for landscape
                scoreUI.centerTextHorizontally = true;
                scoreUI.centerTextVertically = true;
                m_ecsSystem->AddComponent<UIElement>(m_gameOverScoreEntity, scoreUI);
                
                GN_LOG_INFO("Created game over score (landscape) at: (" + std::to_string(scorePosition.x) + ", " + std::to_string(scorePosition.y) + ")");
            }
            
            // Pipes label on right side (moved up 16px)
            m_pipesLabelEntity = m_ecsSystem->CreateEntity();
            if (m_pipesLabelEntity != Gnosis::INVALID_ENTITY) {
                float pipesX = m_cachedScreenWidth * 0.70f;
                float pipesY = m_cachedScreenHeight * 0.30f - 76.0f; // Moved up 16px
                Transform pipesTransform(Gnosis::GNVector2(pipesX, pipesY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                m_ecsSystem->AddComponent<Transform>(m_pipesLabelEntity, pipesTransform);
                
                UIElement pipesUI;
                pipesUI.buttonText = "Pipes: " + std::to_string(m_pipesCleared);
                pipesUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
                pipesUI.visible = true;
                pipesUI.isEnabled = true;
                pipesUI.textLayer = 102;
                pipesUI.fontSize = 60.0f;
                pipesUI.centerTextHorizontally = true;
                pipesUI.textOutlineWidth = 6.0f;
                pipesUI.normalTextureId = "";
                m_ecsSystem->AddComponent<UIElement>(m_pipesLabelEntity, pipesUI);
            }
            
            // Coins label on right side (moved down 16px)
            m_coinsLabelEntity = m_ecsSystem->CreateEntity();
            if (m_coinsLabelEntity != Gnosis::INVALID_ENTITY) {
                PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
                int totalCoins = player ? player->sessionCoins : 0;
                
                float coinsX = m_cachedScreenWidth * 0.70f;
                float coinsY = m_cachedScreenHeight * 0.30f + 56.0f; // Moved down 16px
                Transform coinsTransform(Gnosis::GNVector2(coinsX, coinsY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                m_ecsSystem->AddComponent<Transform>(m_coinsLabelEntity, coinsTransform);
                
                UIElement coinsUI;
                coinsUI.buttonText = "Coins: " + std::to_string(totalCoins);
                coinsUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
                coinsUI.visible = true;
                coinsUI.isEnabled = true;
                coinsUI.textLayer = 102;
                coinsUI.fontSize = 60.0f;
                coinsUI.centerTextHorizontally = true;
                coinsUI.textOutlineWidth = 6.0f;
                coinsUI.normalTextureId = "";
                m_ecsSystem->AddComponent<UIElement>(m_coinsLabelEntity, coinsUI);
            }
            
            // Death message at top LEFT (above morte)
            m_deathMessageEntity = m_ecsSystem->CreateEntity();
            if (m_deathMessageEntity != Gnosis::INVALID_ENTITY) {
                float messageX = m_cachedScreenWidth * 0.25f; // Left side with morte
                float messageY = m_cachedScreenHeight * 0.15f;
                Transform messageTransform(Gnosis::GNVector2(messageX, messageY), 0.0f, Gnosis::GNVector2(6.0f, 6.0f));
                m_ecsSystem->AddComponent<Transform>(m_deathMessageEntity, messageTransform);
                
                UIElement messageUI;
                messageUI.buttonText = GetRandomDeathMessage();
                messageUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
                messageUI.visible = true;
                messageUI.isEnabled = true;
                messageUI.textLayer = 104;
                messageUI.fontSize = 60.0f;
                messageUI.centerTextHorizontally = true;
                messageUI.centerTextVertically = true;
                messageUI.textOutlineWidth = 6.0f;
                messageUI.normalTextureId = "";
                m_ecsSystem->AddComponent<UIElement>(m_deathMessageEntity, messageUI);
            }
            
            // Try Again button (RIGHT side, bigger buttons)
            m_tryAgainButtonEntity = m_ecsSystem->CreateEntity();
            if (m_tryAgainButtonEntity != Gnosis::INVALID_ENTITY) {
                float buttonScale = 7.0f; // Even bigger
                float buttonWidth = 90.0f * buttonScale;
                float buttonHeight = 16.0f * buttonScale;
                
                Gnosis::GNVector2 tryAgainPosition = CenterObjectAtPosition(
                    m_cachedScreenWidth * 0.70f, 
                    m_cachedScreenHeight * 0.65f, 
                    buttonWidth, 
                    buttonHeight
                );
                
                Transform tryAgainTransform(tryAgainPosition, 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
                m_ecsSystem->AddComponent<Transform>(m_tryAgainButtonEntity, tryAgainTransform);
                
                Sprite tryAgainSprite("FloppyButtonBlue", 90, 16);
                tryAgainSprite.layer = 103;
                tryAgainSprite.visible = true;
                m_ecsSystem->AddComponent<Sprite>(m_tryAgainButtonEntity, tryAgainSprite);
                
                UIElement tryAgainUI;
                tryAgainUI.buttonText = "Try Again";
                tryAgainUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
                tryAgainUI.visible = true;
                tryAgainUI.isEnabled = true;
                tryAgainUI.textLayer = 104;
                tryAgainUI.normalTextureId = "FloppyButtonBlue";
                tryAgainUI.fontSize = 60.0f;
                tryAgainUI.centerTextHorizontally = true;
                tryAgainUI.centerTextVertically = true;
                tryAgainUI.isHovered = false;
                tryAgainUI.isPressed = false;
                m_ecsSystem->AddComponent<UIElement>(m_tryAgainButtonEntity, tryAgainUI);
                
                Hitbox buttonHitbox;
                buttonHitbox.width = buttonWidth;
                buttonHitbox.height = buttonHeight;
                buttonHitbox.offsetX = 0.0f;
                buttonHitbox.offsetY = 0.0f;
                m_ecsSystem->AddComponent<Hitbox>(m_tryAgainButtonEntity, buttonHitbox);
                
                GN_LOG_INFO("Created Try Again button (landscape) at: (" + std::to_string(tryAgainPosition.x) + ", " + std::to_string(tryAgainPosition.y) + ")");
            }
            
            // Quit button (RIGHT side, bigger buttons)
            m_quitButtonEntity = m_ecsSystem->CreateEntity();
            if (m_quitButtonEntity != Gnosis::INVALID_ENTITY) {
                float buttonScale = 7.0f; // Even bigger
                float buttonWidth = 90.0f * buttonScale;
                float buttonHeight = 16.0f * buttonScale;
                
                Gnosis::GNVector2 quitPosition = CenterObjectAtPosition(
                    m_cachedScreenWidth * 0.70f, 
                    m_cachedScreenHeight * 0.80f, 
                    buttonWidth, 
                    buttonHeight
                );
                
                Transform quitTransform(quitPosition, 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
                m_ecsSystem->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
                
                Sprite quitSprite("FloppyButtonBlue", 90, 16);
                quitSprite.layer = 103;
                quitSprite.visible = true;
                m_ecsSystem->AddComponent<Sprite>(m_quitButtonEntity, quitSprite);
                
                UIElement quitUI;
                quitUI.buttonText = "Quit";
                quitUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
                quitUI.visible = true;
                quitUI.isEnabled = true;
                quitUI.textLayer = 104;
                quitUI.normalTextureId = "FloppyButtonBlue";
                quitUI.fontSize = 60.0f;
                quitUI.centerTextHorizontally = true;
                quitUI.centerTextVertically = true;
                quitUI.isHovered = false;
                quitUI.isPressed = false;
                m_ecsSystem->AddComponent<UIElement>(m_quitButtonEntity, quitUI);
                
                Hitbox buttonHitbox;
                buttonHitbox.width = buttonWidth;
                buttonHitbox.height = buttonHeight;
                buttonHitbox.offsetX = 0.0f;
                buttonHitbox.offsetY = 0.0f;
                m_ecsSystem->AddComponent<Hitbox>(m_quitButtonEntity, buttonHitbox);
                
                GN_LOG_INFO("Created Quit button (landscape) at: (" + std::to_string(quitPosition.x) + ", " + std::to_string(quitPosition.y) + ")");
            }
            
        } else {
            // PORTRAIT LAYOUT (original)
            
            // Create game over background (light from heaven) - start at top and stretch to full width
            m_gameOverBackgroundEntity = m_ecsSystem->CreateEntity();
            if (m_gameOverBackgroundEntity != Gnosis::INVALID_ENTITY) {
                // Calculate scale to stretch width to screen width while maintaining aspect ratio
                float backgroundScale = m_cachedScreenWidth / 64.0f; // 64 is actual texture width
                
                // Position at top-left corner (0,0) and stretch to full width
                // The sprite's origin is at center, so we need to offset by half the scaled width
                float scaledWidth = 64.0f * backgroundScale;
                Transform bgTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(backgroundScale, backgroundScale));
                m_ecsSystem->AddComponent<Transform>(m_gameOverBackgroundEntity, bgTransform);
                
                // Create background sprite (like main menu) - THIS IS THE KEY DIFFERENCE!
                Sprite bgSprite("GameOverBackground", 64, 64); // Use actual texture dimensions
                bgSprite.layer = 100; // Background layer (lowest priority)
                bgSprite.visible = true;
                m_ecsSystem->AddComponent<Sprite>(m_gameOverBackgroundEntity, bgSprite);
                
                GN_LOG_INFO("Created game over background at top with scale: " + std::to_string(backgroundScale) + ", position: (" + std::to_string(m_cachedScreenWidth * 0.5f) + ", 0.0f), scaled width: " + std::to_string(scaledWidth));
            }
            
            // Create morte sprite (floating above score) - use proper centering
            m_morteEntity = m_ecsSystem->CreateEntity();
            if (m_morteEntity != Gnosis::INVALID_ENTITY) {
                float morteScale = 6.0f;
                float morteWidth = 64.0f * morteScale;
                float morteHeight = 64.0f * morteScale;
                
                // Position morte sprite at 30% from top
                Gnosis::GNVector2 mortePosition = CenterObjectAtPosition(m_cachedScreenWidth * 0.5f, m_cachedScreenHeight * 0.30f, morteWidth, morteHeight);
                
                Transform morteTransform(Gnosis::GNVector2(mortePosition.x, mortePosition.y), 0.0f, Gnosis::GNVector2(morteScale, morteScale));
                m_ecsSystem->AddComponent<Transform>(m_morteEntity, morteTransform);
                
                // Create morte sprite (like main menu) - THIS IS THE KEY DIFFERENCE!
                Sprite morteSprite("FloppyTurdMorte", 64, 64); // Use actual texture dimensions
                morteSprite.layer = 102; // Above background, below text
                morteSprite.visible = true;
                m_ecsSystem->AddComponent<Sprite>(m_morteEntity, morteSprite);
                
                GN_LOG_INFO("Created morte sprite at centered position: (" + std::to_string(mortePosition.x) + ", " + std::to_string(mortePosition.y) + ")");
            }
            
            // Create score display background - use proper centering
            m_gameOverScoreEntity = m_ecsSystem->CreateEntity();
            if (m_gameOverScoreEntity != Gnosis::INVALID_ENTITY) {
                // Calculate scale to make scoreboard 80% of screen width while maintaining 1:1 pixel ratio
                float scoreScale = (m_cachedScreenWidth * 0.8f) / 64.0f; // 80% of screen width / texture width
                float scoreWidth = 64.0f * scoreScale; // This will be 80% of screen width
                float scoreHeight = 32.0f * scoreScale; // Height scales proportionally
                
                // Position score display lower on screen (around 60% from top)
                Gnosis::GNVector2 scorePosition = CenterObjectAtPosition(m_cachedScreenWidth * 0.5f, m_cachedScreenHeight * 0.6f, scoreWidth, scoreHeight);
                
                Transform scoreTransform(Gnosis::GNVector2(scorePosition.x, scorePosition.y), 0.0f, Gnosis::GNVector2(scoreScale, scoreScale));
                m_ecsSystem->AddComponent<Transform>(m_gameOverScoreEntity, scoreTransform);
                
                UIElement scoreUI;
                scoreUI.buttonText = ""; // No text on background
                scoreUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
                scoreUI.visible = true;
                scoreUI.isEnabled = true;
                scoreUI.textLayer = 101; // Lower than morte, higher than background
                scoreUI.normalTextureId = "GameOverScore"; // Use correct asset catalog name
                scoreUI.fontSize = 80.0f;
                scoreUI.centerTextHorizontally = true;
                scoreUI.centerTextVertically = true;
                m_ecsSystem->AddComponent<UIElement>(m_gameOverScoreEntity, scoreUI);
                
                GN_LOG_INFO("Created game over score background at centered position: (" + std::to_string(scorePosition.x) + ", " + std::to_string(scorePosition.y) + ") with scale: " + std::to_string(scoreScale) + ", dimensions: " + std::to_string(scoreWidth) + "x" + std::to_string(scoreHeight));
            }
            
            // Create separate "Pipes:" label entity
            m_pipesLabelEntity = m_ecsSystem->CreateEntity();
            if (m_pipesLabelEntity != Gnosis::INVALID_ENTITY) {
                // Get player stats
                PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
                
                // Position above center of scoreboard, shifted 32px to the right
                float pipesX = (m_cachedScreenWidth * 0.5f) + 32.0f;
                float pipesY = m_cachedScreenHeight * 0.6f - 100.0f; // 100px above center
                Transform pipesTransform(Gnosis::GNVector2(pipesX, pipesY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                m_ecsSystem->AddComponent<Transform>(m_pipesLabelEntity, pipesTransform);
                
                UIElement pipesUI;
                pipesUI.buttonText = "Pipes: " + std::to_string(m_pipesCleared);
                pipesUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
                pipesUI.visible = true;
                pipesUI.isEnabled = true;
                pipesUI.textLayer = 102; // Above scoreboard background
                pipesUI.fontSize = 80.0f;
                pipesUI.centerTextHorizontally = true;
                pipesUI.textOutlineWidth = 8.0f; // Add outline for visibility
                pipesUI.normalTextureId = ""; // Text-only
                m_ecsSystem->AddComponent<UIElement>(m_pipesLabelEntity, pipesUI);
                
                GN_LOG_INFO("Created Pipes label at: (" + std::to_string(pipesX) + ", " + std::to_string(pipesY) + ")");
            }
            
            // Create separate "Coins:" label entity
            m_coinsLabelEntity = m_ecsSystem->CreateEntity();
            if (m_coinsLabelEntity != Gnosis::INVALID_ENTITY) {
                // Get player stats
                PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
                int totalCoins = player ? player->sessionCoins : 0;
                
                // Position below center of scoreboard, shifted 32px right and 48px lower (was 64, moved up 16px)
                float coinsX = (m_cachedScreenWidth * 0.5f) + 32.0f;
                float coinsY = (m_cachedScreenHeight * 0.6f + 50.0f) + 48.0f; // 50px below center + 48px lower
                Transform coinsTransform(Gnosis::GNVector2(coinsX, coinsY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                m_ecsSystem->AddComponent<Transform>(m_coinsLabelEntity, coinsTransform);
                
                UIElement coinsUI;
                coinsUI.buttonText = "Coins: " + std::to_string(totalCoins);
                coinsUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
                coinsUI.visible = true;
                coinsUI.isEnabled = true;
                coinsUI.textLayer = 102; // Above scoreboard background
                coinsUI.fontSize = 80.0f;
                coinsUI.centerTextHorizontally = true;
                coinsUI.textOutlineWidth = 8.0f; // Add outline for visibility
                coinsUI.normalTextureId = ""; // Text-only
                m_ecsSystem->AddComponent<UIElement>(m_coinsLabelEntity, coinsUI);
                
                GN_LOG_INFO("Created Coins label at: (" + std::to_string(coinsX) + ", " + std::to_string(coinsY) + ")");
            }
            
            // Create death message (centered at top)
            m_deathMessageEntity = m_ecsSystem->CreateEntity();
            if (m_deathMessageEntity != Gnosis::INVALID_ENTITY) {
                // Position death message below the morte sprite (around 25% from top)
                float messageX = m_cachedScreenWidth * 0.5f;  // Center horizontally
                float messageY = m_cachedScreenHeight * 0.25f; // 25% from top (below morte sprite)
                Transform messageTransform(Gnosis::GNVector2(messageX, messageY), 0.0f, Gnosis::GNVector2(8.0f, 8.0f));
                m_ecsSystem->AddComponent<Transform>(m_deathMessageEntity, messageTransform);
                
                UIElement messageUI;
                messageUI.buttonText = GetRandomDeathMessage();
                messageUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
                messageUI.visible = true;
                messageUI.isEnabled = true;
                messageUI.textLayer = 104; // Highest priority for death message
                messageUI.fontSize = 80.0f; // Use raw font size value (not scaled)
                messageUI.centerTextHorizontally = true;
                messageUI.centerTextVertically = true;
                messageUI.textOutlineWidth = 8.0f; // Add outline for better visibility
                messageUI.normalTextureId = ""; // Text-only, no background texture
                m_ecsSystem->AddComponent<UIElement>(m_deathMessageEntity, messageUI);
                
                GN_LOG_INFO("Created death message: " + messageUI.buttonText + " at (" + std::to_string(messageX) + ", " + std::to_string(messageY) + ") with font size: " + std::to_string(messageUI.fontSize));
            }
            
            // Create Try Again button (PORTRAIT) - use proper centering and main menu font size
            m_tryAgainButtonEntity = m_ecsSystem->CreateEntity();
        if (m_tryAgainButtonEntity != Gnosis::INVALID_ENTITY) {
            float buttonScale = 10.0f; // Match main menu button scale
            float buttonWidth = 90.0f * buttonScale; // 90 is texture width
            float buttonHeight = 16.0f * buttonScale; // 16 is texture height
            
            // Use CenterObjectAtPosition like main menu for proper centering
            Gnosis::GNVector2 tryAgainPosition = CenterObjectAtPosition(m_cachedScreenWidth * 0.5f, m_cachedScreenHeight * 0.8f, buttonWidth, buttonHeight);
            
            Transform tryAgainTransform(Gnosis::GNVector2(tryAgainPosition.x, tryAgainPosition.y), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            m_ecsSystem->AddComponent<Transform>(m_tryAgainButtonEntity, tryAgainTransform);
            
            // Create button sprite (like main menu) - THIS IS THE KEY DIFFERENCE!
            Sprite tryAgainSprite("FloppyButtonBlue", 90, 16); // Use actual texture dimensions
            tryAgainSprite.layer = 103; // Button layer (higher priority)
            tryAgainSprite.visible = true;
            m_ecsSystem->AddComponent<Sprite>(m_tryAgainButtonEntity, tryAgainSprite);
            
            UIElement tryAgainUI;
            tryAgainUI.buttonText = "Try Again";
            tryAgainUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
            tryAgainUI.visible = true;
            tryAgainUI.isEnabled = true;
            tryAgainUI.textLayer = 104; // Text layer (higher than sprite)
            tryAgainUI.normalTextureId = "FloppyButtonBlue"; // Use the asset catalog name
            tryAgainUI.fontSize = 80.0f; // Much larger font size for better visibility
            tryAgainUI.centerTextHorizontally = true;
            tryAgainUI.centerTextVertically = true;
            // Ensure the button texture is properly set
            tryAgainUI.isHovered = false;
            tryAgainUI.isPressed = false;
            m_ecsSystem->AddComponent<UIElement>(m_tryAgainButtonEntity, tryAgainUI);
            
            // Add hitbox for accurate input detection covering the entire button area
            Hitbox buttonHitbox;
            buttonHitbox.width = buttonWidth;  // This is already scaled (buttonWidth = 90.0f * buttonScale)
            buttonHitbox.height = buttonHeight; // This is already scaled (buttonHeight = 16.0f * buttonScale)
            buttonHitbox.offsetX = 0.0f;
            buttonHitbox.offsetY = 0.0f;
            m_ecsSystem->AddComponent<Hitbox>(m_tryAgainButtonEntity, buttonHitbox);
            
            GN_LOG_INFO("Created Try Again button at centered position: (" + std::to_string(tryAgainPosition.x) + ", " + std::to_string(tryAgainPosition.y) + ") with scale: " + std::to_string(buttonScale));
            }
            
            // Create Quit button (PORTRAIT) - use proper centering and main menu font size
            m_quitButtonEntity = m_ecsSystem->CreateEntity();
        if (m_quitButtonEntity != Gnosis::INVALID_ENTITY) {
            float buttonScale = 10.0f; // Match main menu button scale
            float buttonWidth = 90.0f * buttonScale; // 90 is texture width
            float buttonHeight = 16.0f * buttonScale; // 16 is texture height
            
            // Use CenterObjectAtPosition like main menu for proper centering
            Gnosis::GNVector2 quitPosition = CenterObjectAtPosition(m_cachedScreenWidth * 0.5f, m_cachedScreenHeight * 0.9f, buttonWidth, buttonHeight);
            
            Transform quitTransform(Gnosis::GNVector2(quitPosition.x, quitPosition.y), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            m_ecsSystem->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
            
            // Create button sprite (like main menu) - THIS IS THE KEY DIFFERENCE!
            Sprite quitSprite("FloppyButtonBlue", 90, 16); // Use actual texture dimensions
            quitSprite.layer = 103; // Button layer (higher priority)
            quitSprite.visible = true;
            m_ecsSystem->AddComponent<Sprite>(m_quitButtonEntity, quitSprite);
            
            UIElement quitUI;
            quitUI.buttonText = "Quit";
            quitUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
            quitUI.visible = true;
            quitUI.isEnabled = true;
            quitUI.textLayer = 104; // Text layer (higher than sprite)
            quitUI.normalTextureId = "FloppyButtonBlue"; // Use the asset catalog name
            quitUI.fontSize = 80.0f; // Much larger font size for better visibility
            quitUI.centerTextHorizontally = true;
            quitUI.centerTextVertically = true;
            // Ensure the button texture is properly set
            quitUI.isHovered = false;
            quitUI.isPressed = false;
            m_ecsSystem->AddComponent<UIElement>(m_quitButtonEntity, quitUI);
            
            // Add hitbox for accurate input detection covering the entire button area
            Hitbox buttonHitbox;
            buttonHitbox.width = buttonWidth;  // This is already scaled (buttonWidth = 90.0f * buttonScale)
            buttonHitbox.height = buttonHeight; // This is already scaled (buttonHeight = 16.0f * buttonScale)
            buttonHitbox.offsetX = 0.0f;
            buttonHitbox.offsetY = 0.0f;
            m_ecsSystem->AddComponent<Hitbox>(m_quitButtonEntity, buttonHitbox);
            
            GN_LOG_INFO("Created Quit button at centered position: (" + std::to_string(quitPosition.x) + ", " + std::to_string(quitPosition.y) + ") with scale: " + std::to_string(buttonScale));
            }
        }
    }

    void GameplayState::DestroyGameOverUI() {
        if (!m_ecsSystem) {
            return;
        }
        
        if (m_gameOverBackgroundEntity != 0) {
            m_ecsSystem->DestroyEntity(m_gameOverBackgroundEntity);
            m_gameOverBackgroundEntity = 0;
        }
        
        if (m_morteEntity != 0) {
            m_ecsSystem->DestroyEntity(m_morteEntity);
            m_morteEntity = 0;
        }
        
        if (m_gameOverScoreEntity != 0) {
            m_ecsSystem->DestroyEntity(m_gameOverScoreEntity);
            m_gameOverScoreEntity = 0;
        }
    
        if (m_pipesLabelEntity != 0) {
            m_ecsSystem->DestroyEntity(m_pipesLabelEntity);
            m_pipesLabelEntity = 0;
        }
    
        if (m_coinsLabelEntity != 0) {
            m_ecsSystem->DestroyEntity(m_coinsLabelEntity);
            m_coinsLabelEntity = 0;
        }
    
        if (m_deathMessageEntity != 0) {
            m_ecsSystem->DestroyEntity(m_deathMessageEntity);
            m_deathMessageEntity = 0;
        }
        
        if (m_tryAgainButtonEntity != 0) {
            m_ecsSystem->DestroyEntity(m_tryAgainButtonEntity);
            m_tryAgainButtonEntity = 0;
        }
        
        if (m_quitButtonEntity != 0) {
            m_ecsSystem->DestroyEntity(m_quitButtonEntity);
            m_quitButtonEntity = 0;
        }
        
        GN_LOG_INFO("Game over UI destroyed");
    }

    void GameplayState::UpdateMorteFloating(float deltaTime) {
        if (m_morteEntity == 0 || !m_ecsSystem) {
            return;
        }
        
        // Gentle hovering animation in place
        m_morteFloatOffset += deltaTime * 1.5f; // Slower, gentler movement
        float floatY = sin(m_morteFloatOffset) * 8.0f; // Smaller movement range (8 pixels up/down)
        
        Transform* morteTransform = m_ecsSystem->GetComponent<Transform>(m_morteEntity);
        if (morteTransform) {
            // Get screen info for base position - use render system instead of creating local objects
            float baseY = 1022.4f; // Default iPhone 16 height * 0.30f (30% from top)
            
            // Try to get actual screen info from render system if available
            if (m_renderSystem) {
                const GameCore::ScreenInfo& renderScreenInfo = m_renderSystem->GetScreenInfo();
                baseY = static_cast<float>(renderScreenInfo.pixelHeight) * 0.30f; // 30% from top as intended
            }
            
            morteTransform->position.y = baseY + floatY;
        }
    }

    void GameplayState::HandleGameOverInput() {
        // Check for touch input on game over buttons
        if (!m_platformDelegates || !m_platformDelegates->input.isPrimaryInputJustPressed) {
            return;
        }
        
        // Check if primary input was just pressed
        if (m_platformDelegates->input.isPrimaryInputJustPressed()) {
            float touchX, touchY;
            m_platformDelegates->input.getPrimaryInputPosition(&touchX, &touchY);
            
            // Check Try Again button
            if (m_tryAgainButtonEntity != 0) {
                Transform* tryAgainTransform = m_ecsSystem->GetComponent<Transform>(m_tryAgainButtonEntity);
                if (tryAgainTransform) {
                    // Use actual button dimensions with 80% scaling for better touch detection (like main menu)
                    float buttonWidth = 90.0f * tryAgainTransform->scale.x * 0.8f;  // 80% of actual button texture size
                    float buttonHeight = 16.0f * tryAgainTransform->scale.y * 0.8f; // 80% of actual button texture size
                    
                    float buttonLeft = tryAgainTransform->position.x;
                    float buttonRight = tryAgainTransform->position.x + buttonWidth;
                    float buttonTop = tryAgainTransform->position.y;
                    float buttonBottom = tryAgainTransform->position.y + buttonHeight;
                    
                    GN_LOG_INFO("Try Again button hit area: (" + std::to_string(buttonLeft) + ", " + std::to_string(buttonTop) + 
                               ") to (" + std::to_string(buttonRight) + ", " + std::to_string(buttonBottom) + ")");
                    GN_LOG_INFO("Touch position: (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");
                    
                    if (touchX >= buttonLeft && touchX <= buttonRight && 
                        touchY >= buttonTop && touchY <= buttonBottom) {
                        GN_LOG_INFO("Try Again button clicked!");
                        TryAgain();
                        return;
                    }
                }
            }
            
            // Check Quit button
            if (m_quitButtonEntity != 0) {
                Transform* quitTransform = m_ecsSystem->GetComponent<Transform>(m_quitButtonEntity);
                if (quitTransform) {
                    // Use actual button dimensions with 80% scaling for better touch detection (like main menu)
                    float buttonWidth = 90.0f * quitTransform->scale.x * 0.8f;  // 80% of actual button texture size
                    float buttonHeight = 16.0f * quitTransform->scale.y * 0.8f; // 80% of actual button texture size
                    
                    float buttonLeft = quitTransform->position.x;
                    float buttonRight = quitTransform->position.x + buttonWidth;
                    float buttonTop = quitTransform->position.y;
                    float buttonBottom = quitTransform->position.y + buttonHeight;
                    
                    GN_LOG_INFO("Quit button hit area: (" + std::to_string(buttonLeft) + ", " + std::to_string(buttonTop) + 
                               ") to (" + std::to_string(buttonRight) + ", " + std::to_string(buttonBottom) + ")");
                    GN_LOG_INFO("Touch position: (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");
                    
                    if (touchX >= buttonLeft && touchX <= buttonRight && 
                        touchY >= buttonTop && touchY <= buttonBottom) {
                        GN_LOG_INFO("Quit button clicked!");
                        QuitToMainMenu();
                        return;
                    }
                }
            }
        }
    }

    void GameplayState::TryAgain() {
        GN_LOG_INFO("Player chose to try again - completely reloading level");
        
        // Clean up game over UI first
        DestroyGameOverUI();
        
        // Stop the stinger sound effect to prevent it from bleeding/stacking
        if (m_platformDelegates && m_platformDelegates->audio.stopSound) {
            m_platformDelegates->audio.stopSound("gameover.mp3");
            GN_LOG_INFO("Stopped specific stinger sound effect: gameover.mp3");
        }
        
        // Clear pickups FIRST before respawning groups (otherwise new coins get deleted)
        if (m_pickupSystem) {
            m_pickupSystem->ClearAll();
            GN_LOG_INFO("[RESET] Cleared all pickups");
        }
        
        // Clear obstacles and other entities, but KEEP the player entity
        if (m_levelManager) {
            // Clean up obstacle system without destroying the player
            if (m_levelManager->GetObstacleSystem()) {
                m_levelManager->GetObstacleSystem()->Cleanup();
                // Reinitialize the obstacle system for the current level
                m_levelManager->GetObstacleSystem()->InitializeForLevel(m_levelManager->GetCurrentLevelId(), m_levelManager->GetCurrentLevelConfig());
            }
            
            // Clear and respawn obstacle groups using orchestrator pattern
            m_levelManager->ClearGroupManifests();
            m_levelManager->SpawnInitialGroups(m_levelManager->GetCurrentLevelId());
            GN_LOG_INFO("[RESET] Group manifests cleared and initial groups respawned");
        }

        // Reset enemies for retry (skips Rat King boss, removes rat minions)
        if (m_levelManager) {
            m_levelManager->ResetEnemiesForRetry();
            GN_LOG_INFO("[RESET] Enemies reset for level retry");
        }
        
        // Reset boss system and health bar for level 6
        if (m_currentLevelId == 6 && m_bossSystem) {
            // Reset boss health to full
            m_bossSystem->Reset();
            GN_LOG_INFO("[RESET] Boss system reset - health reset to full");
            
            // Reset boss health bar display
            if (m_bossHealthBar) {
                m_bossHealthBar->Reset();
                GN_LOG_INFO("[RESET] Boss health bar reset to full");
            }
        }

        // Reset projectile system
        if (m_projectileSystem) {
            m_projectileSystem->ResetForNewGame();
        }

        // Reset player input delay to prevent accidental shooting
        if (m_playerControllerSystem) {
            m_playerControllerSystem->ResetInputDelay(0.4f);
        }

        // Reset game state variables
        m_currentScore = 0;
        m_pipesCleared = 0;
        m_sessionCoinsCollected = 0; // Reset session coins for new attempt
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        m_obstacleSpawnTimer = 0.0f;
        m_pickupSpawnTimer = 0.0f;
        m_enemySpawnTimer = 0.0f;
        m_inputDelayTimer = 0.0f;
        
        // Keep the existing player entity - don't reset to 0
        // m_playerEntity stays the same
        
        // Reset camera system to ensure proper scroll speed and position for fresh level
        if (m_cameraSystem) {
            // Reset both world position and scroll speed for complete reset
            m_cameraSystem->ResetForNewGame();
            m_cameraSystem->SetWorldScrollSpeed(m_currentLevelConfig.worldSpeed);
            GN_LOG_INFO("Reset camera scroll speed to: " + std::to_string(m_currentLevelConfig.worldSpeed));
        }

        // Reset background positions to initial state
        if (m_levelManager) {
            m_levelManager->ResetBackgroundPositions();
        }
        
        // Reset the existing player entity in place (don't recreate)
        if (m_playerEntity != 0) {
            ResetPlayerEntity();
            GN_LOG_INFO("TryAgain: Player entity reset in place: " + std::to_string(m_playerEntity));
        }
        
                        // Background layers are managed by LevelManager - no need to recreate here
                GN_LOG_INFO("TryAgain: Background layers managed by LevelManager");
        
        // Reset UI counter values without destroying/recreating entities
        UpdatePipeCounterUI();
        UpdateCoinCounterUI();
        
        // Restore coin bag using orientation-aware positioning (NOT hardcoded 0.87f)
        if (m_coinBagEntity != 0 && m_ecsSystem && m_renderSystem) {
            const ScreenInfo& si = m_renderSystem->GetScreenInfo();
            bool isLandscape = !si.isPortrait;
            
            float bagX = si.pixelWidth * (isLandscape ? LANDSCAPE_COINBAG_X : PORTRAIT_COINBAG_X);
            float bagY = si.pixelHeight * (isLandscape ? LANDSCAPE_COINBAG_Y : PORTRAIT_COINBAG_Y);
            
            Transform* bagTransform = m_ecsSystem->GetComponent<Transform>(m_coinBagEntity);
            if (bagTransform) {
                bagTransform->position = Gnosis::GNVector2(bagX, bagY);
                GN_LOG_INFO("TryAgain: Reset coin bag to orientation-aware position: (" + 
                           std::to_string(bagX) + ", " + std::to_string(bagY) + ") landscape=" + std::to_string(isLandscape));
            }
            
            // Reset coin text relative to coin bag position
            if (m_coinsTextEntity != 0) {
                const float bagScale = 8.0f;
                float textX = bagX + (32.0f * bagScale) + 8.0f;
                float textY = bagY + (32.0f * bagScale * 0.5f) + 24.0f;
                Transform* textTransform = m_ecsSystem->GetComponent<Transform>(m_coinsTextEntity);
                if (textTransform) {
                    textTransform->position = Gnosis::GNVector2(textX, textY);
                    GN_LOG_INFO("TryAgain: Reset coins text to correct position: (" + 
                               std::to_string(textX) + ", " + std::to_string(textY) + ")");
                }
            }
        }
        GN_LOG_INFO("TryAgain: UI values and positions reset");
        
        // Heart system was already reset in ResetPlayerEntity() - no need to reset again
        
        // Start level music again
        StartLevelMusic();
        
        // Show regular UI and return to playing state
        ShowRegularUI();
        m_currentSubState = GameplaySubState::Playing;

        // Update PauseSystem with reset stats data (everything should be 0 now!)
        if (m_pauseSystem) {
            // Get total spendable coins (stored + session, but session is now 0)
            int totalSpendableCoins = 0;
            if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
                totalSpendableCoins = player->totalCoins + player->sessionCoins; // sessionCoins is 0 after reset
            }

            // Get gross total coins from GameStats (should not reset)
            int grossTotalCoins = GameCore::GetGame()->GetGameStats().totalCoinsCollected;
            GN_LOG_INFO("🔄 TryAgain stats reset - totalSpendable: " + std::to_string(totalSpendableCoins) + ", grossTotal: " + std::to_string(grossTotalCoins) + ", sessionCoins: " + std::to_string(m_sessionCoinsCollected) + ", pipes: " + std::to_string(m_pipesCleared));

            m_pauseSystem->UpdateStatsData(m_pipesCleared, m_sessionCoinsCollected, totalSpendableCoins, grossTotalCoins, GameCore::GetGame()->GetGameStats().totalDeaths, GameCore::GetGame()->GetGameStats().totalEnemiesKilled, GameCore::GetGame()->GetGameStats().totalPipesCleared);
        }

        GN_LOG_INFO("Level completely reloaded successfully");
    }

    void GameplayState::ResetPlayerEntity() {
        if (m_playerEntity == 0 || !m_ecsSystem) {
            GN_LOG_ERROR("ResetPlayerEntity: No valid player entity or ECS system");
            return;
        }
        
        GN_LOG_INFO("Resetting player entity " + std::to_string(m_playerEntity) + " in place");
        
        // Reset player transform to starting position
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        if (playerTransform) {
            // Set player starting position based on level and orientation
            float startX;
            if (m_currentLevelId == 6) {
                // Boss level: use percentage-based positioning to avoid UI overlap
                startX = m_cachedScreenWidth * BOSS_PLAYER_X_PERCENT;
            } else {
                // Other levels: use existing positioning logic
                startX = IsLandscapeMode() ?
                    (PORTRAIT_PLAYER_START_X + LANDSCAPE_PLAYER_OFFSET_X) : PORTRAIT_PLAYER_START_X;
            }
            float startY = PORTRAIT_PLAYER_START_Y;
            playerTransform->position = Gnosis::GNVector2(startX, startY);
            playerTransform->rotation = 0.0f;
            playerTransform->scale = Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale);
            GN_LOG_INFO("Reset player transform to starting position");
        }
        
        // Reset player physics
        Physics* playerPhysics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
        if (playerPhysics) {
            playerPhysics->velocity = Gnosis::GNVector2(0.0f, 0.0f);
            playerPhysics->useGravity = true;
            playerPhysics->mass = 1.0f;
            playerPhysics->drag = 0.98f;
            GN_LOG_INFO("Reset player physics");
        }
        
        // Reset player component (health, hearts, etc.)
        PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
        if (player) {
            // Reset to default state - only reset SESSION values, keep STORED coins
            player->sessionCoins = 0;  // ✅ Reset session coins for new attempt
            // player->totalCoins stays as-is (stored coins persist)
            player->score = 0;
            player->invulnerabilityTimer = 0.0f;
            player->shootCooldown = 0.0f;
            
            // Reset heart-related fields to default values to force UpdateHeartCountForDifficulty to work
            player->ghostSlices = 0;
            player->hollowTurds = false;
            player->heartMode = GameCore::HeartMode::WHOLE;
            player->hearts = 0;        // Force to 0 so UpdateHeartCountForDifficulty doesn't early return
            player->liveSlices = 0;    // Force to 0 so it gets properly reset
            player->maxHealth = 0;     // Force to 0 so it gets properly reset
            player->health = 0;        // Force to 0 so it gets properly reset
            
            GN_LOG_INFO("Reset player component including heart fields");
        }
        
        // Reset player controller system
        if (m_playerControllerSystem) {
            m_playerControllerSystem->SetPlayerAlive(true);
            GN_LOG_INFO("Reset player controller system");
        }
        
        // Reset heart system for the existing player entity
        if (m_heartSystem) {
            Difficulty currentDifficulty = LevelManager::GetGlobalDifficulty();
            m_heartSystem->UpdateHeartCountForDifficulty(m_playerEntity, currentDifficulty);
            GN_LOG_INFO("Reset heart system for existing player");
        }
        
        GN_LOG_INFO("Player entity reset successfully");
    }

    void GameplayState::QuitToMainMenu() {
        GN_LOG_INFO("Player chose to quit to main menu");
        
        // Stop the stinger sound effect to prevent it from bleeding/stacking
        if (m_platformDelegates && m_platformDelegates->audio.stopSound) {
            m_platformDelegates->audio.stopSound("gameover.mp3");
            GN_LOG_INFO("Stopped specific stinger sound effect: gameover.mp3");
        }
        
        // Clear sessionCoins before calling ReturnToMainMenu to prevent double-adding
        // (coins were already added to total in TriggerGameOver)
        PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
        if (player) {
            player->sessionCoins = 0;
            GN_LOG_INFO("💰 Cleared sessionCoins to prevent double-adding when quitting to main menu");
        }
        
        DestroyGameOverUI();
        ReturnToMainMenu();
    }

    std::string GameplayState::GetRandomDeathMessage() {
        static const std::vector<std::string> deathMessages = {
            "You got flushed!",
            "Down the drain!",
            "That was crappy!",
            "Toilet trouble!",
            "Plumber needed!",
            "What a stinker!",
            "Sewage overflow!",
            "Pipe dream ended!",
            "Flushed with failure!",
            "Oh poop!",
            "Turd's in Trouble!",
            "Bummer!",
            "What a fiasco!",
            "Holy crap!",
            "Clogged up!",
        };
        
        // Simple random selection (not cryptographically secure, but fine for game)
        int randomIndex = rand() % deathMessages.size();
        return deathMessages[randomIndex];
    }

    bool GameplayState::HasPlayerFallenOffScreen() {
        if (m_playerEntity == 0 || !m_ecsSystem) {
            GN_LOG_INFO("HasPlayerFallenOffScreen: No player entity or ECS system");
            return true; // If no player, consider fallen
        }
        
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        if (!playerTransform) {
            GN_LOG_INFO("HasPlayerFallenOffScreen: No player transform component");
            return true; // If no transform, consider fallen
        }
        
        // Get screen dimensions using the render system instead of platform delegates
        float screenBottom = 2556.0f; // Default fallback
        if (m_renderSystem) {
            const GameCore::ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            screenBottom = static_cast<float>(screenInfo.pixelHeight);
        }
        
        // Player sprite is 64x64 pixels, but we need to account for the actual scale
        // Get the player's actual scale from the transform
        float playerHeight = 64.0f * playerTransform->scale.y; // Account for actual scale
        
        // Player has fallen off screen if their bottom edge is below screen bottom
        // Bottom edge = center Y + half height
        bool hasFallen = (playerTransform->position.y + (playerHeight / 2.0f)) > screenBottom;
        
        // TEMPORARY DEBUG: Force player to actually fall off screen
        // Only consider fallen if player is significantly below screen (not just at edge)
        hasFallen = (playerTransform->position.y + (playerHeight / 2.0f)) > (screenBottom + 100.0f);
        
        GN_LOG_INFO("HasPlayerFallenOffScreen: Player Y=" + std::to_string(playerTransform->position.y) + 
                   ", Scale=" + std::to_string(playerTransform->scale.y) +
                   ", Height=" + std::to_string(playerHeight) +
                   ", Bottom edge=" + std::to_string(playerTransform->position.y + (playerHeight / 2.0f)) +
                   ", Screen height=" + std::to_string(screenBottom) +
                   ", Threshold=" + std::to_string(screenBottom + 100.0f) +
                   ", Has fallen=" + std::string(hasFallen ? "YES" : "NO"));
        
        if (hasFallen) {
            GN_LOG_INFO("HasPlayerFallenOffScreen: Player has fallen off screen!");
        } else {
            GN_LOG_INFO("HasPlayerFallenOffScreen: Player still on screen, waiting for fall...");
        }
        
        return hasFallen;
    }

    void GameplayState::HideRegularUI() {
        // Hide all regular gameplay UI elements (except settings button - it should stay visible)
        std::vector<Gnosis::Entity*> uiElements = {
            &m_scoreTextEntity,
            &m_livesTextEntity,
            &m_coinsTextEntity,
            &m_coinBagEntity,
            &m_pipeCounterEntity,
            &m_heartUIEntity,
            &m_shootingZoneEntity
        };
        
        for (Gnosis::Entity* entityPtr : uiElements) {
            if (entityPtr && *entityPtr != 0 && m_ecsSystem) {
                UIElement* ui = m_ecsSystem->GetComponent<UIElement>(*entityPtr);
                if (ui) {
                    ui->visible = false;
                }
            }
        }
        
        // Also hide all individual heart entities
        if (m_heartSystem) {
            // Call the heart system to hide all heart entities
            m_heartSystem->HideAllHearts();
        }

        // Hide boss health bar if it exists
        if (m_bossHealthBar) {
            m_bossHealthBar->SetVisible(false);
        }

        GN_LOG_INFO("Hidden all regular UI elements for pause menu");
    }

    void GameplayState::ShowRegularUI() {
        // Show all regular gameplay UI elements including settings button
        std::vector<Gnosis::Entity*> uiElements = {
            &m_scoreTextEntity,
            &m_livesTextEntity,
            &m_coinsTextEntity,
            &m_coinBagEntity,
            &m_pipeCounterEntity,
            &m_heartUIEntity,
            &m_settingsButtonEntity
        };

        // Handle shooting zone separately - only show if enabled for current level
        if (m_shootingZoneEntity != 0 && m_ecsSystem) {
            auto uiElement = m_ecsSystem->GetComponent<UIElement>(m_shootingZoneEntity);
            auto uiShape = m_ecsSystem->GetComponent<UIShape>(m_shootingZoneEntity);
            bool shouldBeVisible = m_currentLevelConfig.shootingEnabled;

            if (uiElement) {
                uiElement->visible = shouldBeVisible;
            }
            if (uiShape) {
                uiShape->visible = shouldBeVisible;
            }

            GN_LOG_INFO("ShowRegularUI: Shooting zone visibility set to " + std::to_string(shouldBeVisible) + " for level " + std::to_string(m_currentLevelId));
        }
        
        for (Gnosis::Entity* entityPtr : uiElements) {
            if (entityPtr && *entityPtr != 0 && m_ecsSystem) {
                UIElement* ui = m_ecsSystem->GetComponent<UIElement>(*entityPtr);
                if (ui) {
                    ui->visible = true;
                }
            }
        }
        
        // Also show all individual heart entities
        if (m_heartSystem) {
            // Call the heart system to show current active heart entities
            m_heartSystem->ShowAllHearts();
        }

        // Show boss health bar if it exists and we're in boss level
        if (m_bossHealthBar && m_currentLevelId == 6) {
            m_bossHealthBar->SetVisible(true);
        }

        GN_LOG_INFO("Shown all regular UI elements");
    }

    bool GameplayState::CheckSettingsButtonClick(float touchX, float touchY) {
        GN_LOG_INFO("CheckSettingsButtonClick: touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        if (m_settingsButtonEntity == 0) {
            GN_LOG_WARN("CheckSettingsButtonClick: No settings button exists");
            return false; // No settings button exists
        }

        if (!m_ecsSystem) {
            GN_LOG_WARN("CheckSettingsButtonClick: ECS system is null");
            return false;
        }

        // Get button transform for collision detection
        auto transform = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);

        if (!transform) {
            GN_LOG_WARN("CheckSettingsButtonClick: No transform component found for settings button");
            return false;
        }

        GN_LOG_INFO("CheckSettingsButtonClick: Entity " + std::to_string(m_settingsButtonEntity) + " transform at (" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ") scale (" + std::to_string(transform->scale.x) + ", " + std::to_string(transform->scale.y) + ") - should be at (2300.4, 94.32) for landscape");

        // Use Bounds component for proper centered collision detection
        auto bounds = m_ecsSystem->GetComponent<Bounds>(m_settingsButtonEntity);

        float buttonWidth, buttonHeight;
        if (bounds) {
            buttonWidth = bounds->width;
            buttonHeight = bounds->height;
            GN_LOG_INFO("Using Bounds component: width=" + std::to_string(buttonWidth) + ", height=" + std::to_string(buttonHeight));
        } else {
            // Fallback: use sprite dimensions with scale
            auto sprite = m_ecsSystem->GetComponent<Sprite>(m_settingsButtonEntity);
            float buttonScale = transform->scale.x;
            if (sprite) {
                buttonWidth = sprite->width * buttonScale;
                buttonHeight = sprite->height * buttonScale;
                GN_LOG_INFO("Using Sprite fallback: sprite.width=" + std::to_string(sprite->width) + ", scale=" + std::to_string(buttonScale) + ", calculated width=" + std::to_string(buttonWidth));
            } else {
                buttonWidth = buttonHeight = 16.0f * buttonScale;
                GN_LOG_INFO("Using hardcoded fallback: width=" + std::to_string(buttonWidth));
            }
        }

        // Settings button uses top-left positioning (transform.position is top-left corner)
        float buttonLeft = transform->position.x;
        float buttonTop = transform->position.y;
        float buttonRight = buttonLeft + buttonWidth;
        float buttonBottom = buttonTop + buttonHeight;

        GN_LOG_INFO("Calculated bounds: center=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) +
                   ") size=(" + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + ") -> bounds=(" +
                   std::to_string(buttonLeft) + "," + std::to_string(buttonTop) + "," + std::to_string(buttonRight) + "," + std::to_string(buttonBottom) + ")");

        // Update debug rectangle position to match current collision bounds
        // UpdateDebugButtonRectangle(buttonLeft, buttonTop, buttonRight, buttonBottom); // Disabled for production

        GN_LOG_INFO("Settings button collision check: touch(" + std::to_string(touchX) + "," + std::to_string(touchY) +
                   ") vs button bounds(" + std::to_string(buttonLeft) + "," + std::to_string(buttonTop) + "," +
                   std::to_string(buttonRight) + "," + std::to_string(buttonBottom) + ") [16x16 texture at " +
                   std::to_string(transform->scale.x) + "x scale] landscape=" + std::to_string(IsLandscapeMode()) + " boss=" + std::to_string(m_currentLevelId == 6));

        // Debug: Check if touch is close to button for landscape mode debugging
        if (IsLandscapeMode() && m_currentLevelId == 6) {
            float distanceX = std::abs(touchX - transform->position.x);
            float distanceY = std::abs(touchY - transform->position.y);
            GN_LOG_INFO("Landscape boss level - Touch distance from button center: " + std::to_string(distanceX) + "," + std::to_string(distanceY) +
                       " (button size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + ")");
        }

        if (touchX >= buttonLeft && touchX <= buttonRight &&
            touchY >= buttonTop && touchY <= buttonBottom) {

            // Check debounce timer to prevent rapid clicking
            if (m_lastSettingsButtonPressTime < m_settingsButtonDebounceDelay) {
                GN_LOG_INFO("Settings button debounced - too soon since last press");
                return false;
            }

        if (m_currentSubState == GameplaySubState::Playing) {
            GN_LOG_INFO("Settings button clicked! Opening pause menu.");

            TriggerPause();
            // Hide regular UI before showing pause menu
            HideRegularUI();

            // Update pause system with latest stats before showing
            if (m_pauseSystem) {
                GN_LOG_INFO("🎮 PauseSystem exists, updating stats and showing menu");

                // Get total spendable coins (stored + session)
                int totalSpendableCoins = 0;
                if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
                    totalSpendableCoins = player->totalCoins + player->sessionCoins;
                }

                // Get gross total coins
                int grossTotalCoins = GameCore::GetGame()->GetGameStats().totalCoinsCollected;

                GN_LOG_INFO("📊 Updating pause system stats before show - totalSpendable: " + std::to_string(totalSpendableCoins) + ", grossTotal: " + std::to_string(grossTotalCoins) + ", sessionCoins: " + std::to_string(m_sessionCoinsCollected) + ", pipes: " + std::to_string(m_pipesCleared));

                m_pauseSystem->UpdateStatsData(m_pipesCleared, m_sessionCoinsCollected, totalSpendableCoins, grossTotalCoins, GameCore::GetGame()->GetGameStats().totalDeaths, GameCore::GetGame()->GetGameStats().totalEnemiesKilled, GameCore::GetGame()->GetGameStats().totalPipesCleared);

                GN_LOG_INFO("🎮 Calling PauseSystem::Show()");
                m_pauseSystem->Show();
                GN_LOG_INFO("🎮 PauseSystem::Show() completed");

                // Move settings button to bottom right AFTER pause menu is shown
                RepositionSettingsButtonForPauseMenu(true);
                GN_LOG_INFO("🎮 Settings button repositioned for pause menu");
            } else {
                GN_LOG_ERROR("🎮 PauseSystem is null!");
            }
            // Reset debounce timer
            m_lastSettingsButtonPressTime = 0.0f;
            return true;
        } else if (m_currentSubState == GameplaySubState::Paused) {
            GN_LOG_INFO("Settings button clicked! Closing pause menu.");
            if (m_pauseSystem) {
                m_pauseSystem->Hide();
            }
            // Restore settings button to original position
            RepositionSettingsButtonForPauseMenu(false);
            // Show regular UI after hiding pause menu
            ShowRegularUI();
            TriggerResume();
            // Reset debounce timer
            m_lastSettingsButtonPressTime = 0.0f;
            return true;
        }

        return false; // Button not clicked
    } // End of CheckSettingsButtonClick


} // Class GameplayState

// Function definitions outside the class
bool GameplayState::IsTapInSettingsButtonArea(float touchX, float touchY) {
    // Get the actual settings button position from its Transform component
    // This ensures we check the correct area regardless of orientation
    if (m_settingsButtonEntity != 0 && m_ecsSystem) {
        Transform* t = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);
        if (t) {
            // Use Bounds component for proper centered collision detection (same as CheckSettingsButtonClick)
            auto bounds = m_ecsSystem->GetComponent<Bounds>(m_settingsButtonEntity);

            float buttonWidth, buttonHeight;
            if (bounds) {
                buttonWidth = bounds->width;
                buttonHeight = bounds->height;
                GN_LOG_INFO("IsTapInSettingsButtonArea: Using Bounds component: width=" + std::to_string(buttonWidth) + ", height=" + std::to_string(buttonHeight));
            } else {
                // Fallback: use sprite dimensions with scale
                auto sprite = m_ecsSystem->GetComponent<Sprite>(m_settingsButtonEntity);
                float buttonScale = t->scale.x;
                if (sprite) {
                    buttonWidth = sprite->width * buttonScale;
                    buttonHeight = sprite->height * buttonScale;
                    GN_LOG_INFO("IsTapInSettingsButtonArea: Using Sprite fallback: sprite.width=" + std::to_string(sprite->width) + ", scale=" + std::to_string(buttonScale) + ", calculated width=" + std::to_string(buttonWidth));
                } else {
                    // Fallback to assumed dimensions if sprite not found
                    buttonWidth = buttonHeight = 16.0f * buttonScale;
                    GN_LOG_INFO("IsTapInSettingsButtonArea: Using hardcoded fallback: width=" + std::to_string(buttonWidth));
                }
            }

            // Settings button uses top-left positioning (transform.position is top-left corner)
            float buttonLeft = t->position.x;
            float buttonTop = t->position.y;
            float buttonRight = buttonLeft + buttonWidth;
            float buttonBottom = buttonTop + buttonHeight;

            GN_LOG_INFO("Settings button area check: touch(" + std::to_string(touchX) + "," + std::to_string(touchY) +
                       ") vs button(" + std::to_string(buttonLeft) + "," + std::to_string(buttonTop) + "," +
                       std::to_string(buttonRight) + "," + std::to_string(buttonBottom) +
                       "), sprite size=" + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight));

            return (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom);
        } else {
            GN_LOG_WARN("IsTapInSettingsButtonArea: No Transform component found for settings button");
        }
    } else {
        GN_LOG_WARN("IsTapInSettingsButtonArea: Settings button entity not available");
    }
    return false;
}

bool GameplayState::IsTapOutsideMenuArea(float touchX, float touchY) {
    // Get screen dimensions
    float screenWidth = 1179.0f;  // Default iPhone 16 width
    float screenHeight = 2556.0f; // Default iPhone 16 height

    if (m_renderSystem) {
        const ScreenInfo& si = m_renderSystem->GetScreenInfo();
        screenWidth = static_cast<float>(si.pixelWidth);
        screenHeight = static_cast<float>(si.pixelHeight);
    }

    // Define menu area bounds to match the actual pause menu background dimensions
    // Background: 160x300 scaled 6x = 960x1800 pixels, centered on screen
    float originalWidth = 160.0f;   // Original texture width
    float originalHeight = 300.0f;  // Original texture height
    float bgScale = 6.0f;           // Same scale used in PauseSystem

    // Calculate scaled dimensions and center position
    float scaledWidth = originalWidth * bgScale;   // 160 * 6 = 960
    float scaledHeight = originalHeight * bgScale; // 300 * 6 = 1800

    // Center the menu on screen
    float centerX = screenWidth * 0.5f;
    float centerY = screenHeight * 0.5f;

    // Calculate menu bounds
    float menuLeft = centerX - (scaledWidth * 0.5f);
    float menuRight = centerX + (scaledWidth * 0.5f);
    float menuTop = centerY - (scaledHeight * 0.5f);
    float menuBottom = centerY + (scaledHeight * 0.5f);

    // DISABLED: No tap outside logic - only settings button can close menu
    // This prevents accidental closes while allowing deliberate settings button closes
    bool outsideMenu = false;

    // EXCLUDE the settings button area from "outside menu" check
    if (IsTapInSettingsButtonArea(touchX, touchY)) {
        GN_LOG_INFO("Tap is in settings button area - not outside menu");
        return false;
    }

    GN_LOG_INFO("Tap outside menu check: touch(" + std::to_string(touchX) + "," + std::to_string(touchY) +
               ") menu_bounds(L:" + std::to_string(menuLeft) + " R:" + std::to_string(menuRight) +
               " T:" + std::to_string(menuTop) + " B:" + std::to_string(menuBottom) + ") outside:" +
               std::string(outsideMenu ? "YES" : "NO"));

    return outsideMenu;
}

void GameplayState::RescaleBackgroundsForOrientation(bool isLandscape, float screenWidth, float screenHeight) {
    GN_LOG_INFO("RescaleBackgroundsForOrientation: " + std::string(isLandscape ? "landscape" : "portrait") + 
               " (" + std::to_string((int)screenWidth) + "x" + std::to_string((int)screenHeight) + ")");
    
    if (!m_ecsSystem || !m_levelManager) {
        GN_LOG_WARN("RescaleBackgroundsForOrientation: Missing ECS system or level manager");
        return;
    }
    
    // 🎯 DISABLED: This rescaling breaks LevelManager's pixel-perfect positioning
    // LevelManager already calculates optimal scale using actual texture dimensions
    // This code was using incorrect hardcoded dimensions and overriding the correct scale
    
    GN_LOG_INFO("⚠️ Background rescaling DISABLED - LevelManager handles pixel-perfect scaling");
    return;
    
    /* LEGACY CODE - DO NOT USE
    // Get all background entities with parallax components
    auto backgroundEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite, Parallax>();
    
    for (Gnosis::Entity entity : backgroundEntities) {
        auto transform = m_ecsSystem->GetComponent<Transform>(entity);
        auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);
        
        if (!transform || !sprite) continue;
        
        // Determine original texture dimensions based on texture ID
        float textureWidth, textureHeight;
        if (sprite->textureId.find("BossLevelBackgroundMobile") != std::string::npos) {
            // Boss level background is 384x512 (portrait)
            textureWidth = 384.0f;
            textureHeight = 512.0f;
        } else if (sprite->textureId.find("Front") != std::string::npos) {
            // FrontLayer backgrounds are 2048x480
            textureWidth = 2048.0f;
            textureHeight = 480.0f;
        } else if (sprite->textureId.find("Clouds") != std::string::npos) {
            // Cloud layers are 512x180
            textureWidth = 512.0f;
            textureHeight = 180.0f;
        } else {
            // Other background layers (Back, Mid) are 1024x480
            textureWidth = 1024.0f;
            textureHeight = 480.0f;
        }
        
        // Calculate new scale to fit screen
        float heightScale = screenHeight / textureHeight;
        float widthScale = screenWidth / textureWidth;
        
        // For boss level, use different scaling strategy for landscape vs portrait
        if (sprite->textureId.find("BossLevelBackgroundMobile") != std::string::npos) {
            if (isLandscape) {
                // In landscape, scale to fill width and crop height if necessary
                float scale = widthScale;
                transform->scale = Gnosis::GNVector2(scale, scale);
                // Center vertically
                float scaledHeight = textureHeight * scale;
                transform->position.y = (screenHeight - scaledHeight) * 0.5f;
                transform->position.x = 0.0f;
            } else {
                // In portrait, scale to fit height
                float scale = heightScale;
                transform->scale = Gnosis::GNVector2(scale, scale);
                // Center horizontally  
                float scaledWidth = textureWidth * scale;
                transform->position.x = (screenWidth - scaledWidth) * 0.5f;
                transform->position.y = 0.0f;
            }
        } else {
            // For other backgrounds, always scale to fit height
            float scale = heightScale;
            transform->scale = Gnosis::GNVector2(scale, scale);
            
            // Update parallax repeat width for new scaling
            auto parallax = m_ecsSystem->GetComponent<Parallax>(entity);
            if (parallax) {
                parallax->repeatWidth = textureWidth * scale;
            }
        }
        
        GN_LOG_INFO("Rescaled background '" + sprite->textureId + "' to scale " + 
                   std::to_string(transform->scale.x) + " at position (" + 
                   std::to_string(transform->position.x) + ", " + 
                   std::to_string(transform->position.y) + ")");
    }
    
    // Also handle static boss background entities (without parallax)
    auto staticBackgrounds = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite>();
    for (Gnosis::Entity entity : staticBackgrounds) {
        // Skip if this entity has parallax (already handled above)
        if (m_ecsSystem->HasComponent<Parallax>(entity)) continue;
        
        auto transform = m_ecsSystem->GetComponent<Transform>(entity);
        auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);
        
        if (!transform || !sprite) continue;
        
        // Only process boss level background
        if (sprite->textureId.find("BossLevelBackgroundMobile") != std::string::npos) {
            float textureWidth = 384.0f;
            float textureHeight = 512.0f;
            
            float heightScale = screenHeight / textureHeight;
            float widthScale = screenWidth / textureWidth;
            
            if (isLandscape) {
                // Scale to fill width in landscape
                float scale = widthScale;
                transform->scale = Gnosis::GNVector2(scale, scale);
                float scaledHeight = textureHeight * scale;
                transform->position.y = (screenHeight - scaledHeight) * 0.5f;
                transform->position.x = 0.0f;
            } else {
                // Scale to fit height in portrait
                float scale = heightScale;
                transform->scale = Gnosis::GNVector2(scale, scale);
                float scaledWidth = textureWidth * scale;
                transform->position.x = (screenWidth - scaledWidth) * 0.5f;
                transform->position.y = 0.0f;
            }
            
            GN_LOG_INFO("Rescaled boss static background to scale " + std::to_string(transform->scale.x));
        }
    }
    END LEGACY CODE */
}

} // namespace GameCore