#include "GameplayState.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../Config/EnemyConfigs.h"
#include "../../Engine/Utility/Utils.h"
#include <algorithm>
#include <set>

namespace GameCore {

    GameplayState::GameplayState(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, int levelId)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_currentLevelId(levelId)
        , m_currentLevelConfig(LevelConfigFactory::GetLevelConfig(levelId))
        , m_currentScore(0)
        , m_currentLives(STARTING_LIVES)
        , m_gameTime(0.0f)
        , m_difficultyTimer(0.0f)
        , m_difficultyLevel(1.0f)
        , m_playerAlive(true)
        , m_invulnerabilityTimer(0.0f)
        , m_pipesCleared(0)
        , m_finished(false)
        , m_levelCompleted(false)
        , m_currentSubState(GameplaySubState::Playing)
        , m_gameOverTimer(0.0f)
        , m_morteFloatOffset(0.0f)
        , m_obstacleSpawnTimer(0.0f)
        , m_pickupSpawnTimer(0.0f)
        , m_enemySpawnTimer(0.0f)
        , m_inputDelayTimer(0.0f)
    {
        GN_LOG_INFO("GameplayState created for level: " + std::to_string(levelId) + " (" + m_currentLevelConfig.levelName + ")");
    }

    GameplayState::~GameplayState() {
        GN_LOG_INFO("GameplayState destroyed");
    }

    void GameplayState::Enter() {
        GN_LOG_INFO("Entering GameplayState for level: " + std::to_string(m_currentLevelId));
        
        // Initialize systems (will be implemented in Phase 2)
        InitializeSystems();
        
        // Create game entities
        CreateGameEntities();
        
        // Create UI
        CreateUI();
        
        // Reset game state
        m_currentScore = 0;
        m_currentLives = STARTING_LIVES;
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        m_pipesCleared = 0;
        m_finished = false;
        m_levelCompleted = false;

        
        // Reset spawn timers
        m_obstacleSpawnTimer = 0.0f;
        m_pickupSpawnTimer = 0.0f;
        m_enemySpawnTimer = 0.0f;
        
        // Reset input delay timer to prevent immediate input processing
        m_inputDelayTimer = 0.0f;
        
        // Clear any lingering input commands to prevent auto-shooting when entering level
        if (m_platformDelegates && m_platformDelegates->input.clearInputBuffer) {
            m_platformDelegates->input.clearInputBuffer();
            GN_LOG_INFO("GameplayState: Cleared input buffer to prevent lingering touch inputs");
        }
        
        // Start level music based on current difficulty
        StartLevelMusic();
        
        GN_LOG_INFO("GameplayState entered successfully");
    }

    void GameplayState::Exit() {
        GN_LOG_INFO("Exiting GameplayState");
        
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
        
        GN_LOG_INFO("GameplayState exited");
    }

    void GameplayState::Pause() {
        TriggerPause();
    }

    void GameplayState::Resume() {
        TriggerResume();
    }

    void GameplayState::Update(float deltaTime) {
        // Handle different sub-states
        UpdateSubState(deltaTime);
        
        // Only update game time when playing, but allow physics during game over for falling
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
        
            // Handle input only after delay period to prevent auto-shooting
            if (m_inputDelayTimer >= INPUT_DELAY_TIME) {
                HandleInput();
            }
            
        if (m_cameraSystem) {
            m_cameraSystem->Update(deltaTime);
        }
        
        // Update UI system for menu button rendering
        if (m_uiSystem) {
            m_uiSystem->Update(deltaTime);
        }
        
        // Update heart system for health display (only when playing)
        if (m_heartSystem && m_currentSubState == GameplaySubState::Playing) {
            m_heartSystem->Update(deltaTime);
        }
        
        // Update game logic
        UpdateGameLogic(deltaTime);
        
        // Update spawning FIRST (this calculates SpikeBall hitbox rotations)
        UpdateSpawning(deltaTime);
        
        // Check toilet collisions and pipe clearing (after hitbox updates)
        CheckToiletCollisions();
        
        // Handle pickups via PickupSystem
        if (m_pickupSystem) {
            m_pickupSystem->Update(deltaTime);
        }
        

        
        // Update pipe counter UI
        UpdatePipeCounterUI();
        
        // Update difficulty
        UpdateDifficulty(deltaTime);
        
        // Handle game events
        HandleGameEvents();
        
        // Clean up offscreen entities
        CleanupOffscreenEntities();
        
            // Check level completion
            CheckLevelCompletion();
        } // End of Playing sub-state
        
        // Update essential systems regardless of sub-state (needed for physics during falling)
        if (m_spriteSystem) {
            m_spriteSystem->Update(deltaTime);
        }
        
        // PlayerControllerSystem updates player physics - essential for falling during game over
        if (m_playerControllerSystem) {
            m_playerControllerSystem->Update(deltaTime);
        }
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
                // Game is paused, no updates needed
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
        
        // Draw debug rectangles overlay (after world/UI render so they appear on top)
        DrawDebugRectangles();
    }

    void GameplayState::HandleInput() {
        if (m_currentSubState == GameplaySubState::Paused) {
            // Handle pause menu input
            return;
        }
        
        if (m_currentSubState == GameplaySubState::GameOver) {
            // No input allowed during game over falling sequence
            // Input will be handled by HandleGameOverInput once UI is shown
            return;
        }
        
        // Debug: Check platform delegates
        if (!m_platformDelegates) {
            GN_LOG_ERROR("GameplayState: m_platformDelegates is NULL!");
            return;
        }
        
        GN_LOG_DEBUG("GameplayState: Platform delegates available, checking input functions...");
        
        // Handle gameplay input
        if (m_playerControllerSystem && m_platformDelegates) {
            // Check for touch input with proper state tracking
            if (m_platformDelegates->input.getTouchCount && m_platformDelegates->input.getTouchPosition &&
                m_platformDelegates->input.isTouchJustPressed && m_platformDelegates->input.isTouchJustReleased) {
                
                // Handle touch press events
                if (m_platformDelegates->input.isTouchJustPressed()) {
                    int touchCount = m_platformDelegates->input.getTouchCount();
                    GN_LOG_INFO("Touch PRESSED! Count: " + std::to_string(touchCount));
                    
                    for (int i = 0; i < touchCount; i++) {
                        float x, y;
                        m_platformDelegates->input.getTouchPosition(i, &x, &y);
                        
                        GN_LOG_INFO("Touch " + std::to_string(i) + " PRESSED at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                    
                        // Check for menu button click first
                        CheckMenuButtonClick(x, y);
                    
                        // Send touch press event
                        m_playerControllerSystem->HandleTouchInput(x, y, true);
                    }
                }
                
                // Handle touch release events
                if (m_platformDelegates->input.isTouchJustReleased()) {
                    GN_LOG_INFO("Touch RELEASED!");
                    
                    // Send touch release event with last known position
                    float x = 0.0f, y = 0.0f;
                    if (m_platformDelegates->input.getTouchCount() > 0) {
                        m_platformDelegates->input.getTouchPosition(0, &x, &y);
                    }
                    
                    GN_LOG_INFO("Touch RELEASED at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                    
                    // Send touch release event
                    m_playerControllerSystem->HandleTouchInput(x, y, false);
                }
            } else {
                GN_LOG_WARN("Touch input functions not available!");
                GN_LOG_DEBUG("GameplayState: getTouchCount = " + std::string(m_platformDelegates->input.getTouchCount ? "available" : "NULL"));
                GN_LOG_DEBUG("GameplayState: getTouchPosition = " + std::string(m_platformDelegates->input.getTouchPosition ? "available" : "NULL"));
                GN_LOG_DEBUG("GameplayState: isTouchJustPressed = " + std::string(m_platformDelegates->input.isTouchJustPressed ? "available" : "NULL"));
                GN_LOG_DEBUG("GameplayState: isTouchJustReleased = " + std::string(m_platformDelegates->input.isTouchJustReleased ? "available" : "NULL"));
            }
        } else {
            GN_LOG_WARN("PlayerControllerSystem or PlatformDelegates is null!");
            GN_LOG_DEBUG("GameplayState: m_playerControllerSystem = " + std::string(m_playerControllerSystem ? "available" : "NULL"));
            GN_LOG_DEBUG("GameplayState: m_platformDelegates = " + std::string(m_platformDelegates ? "available" : "NULL"));
        }
    }

    void GameplayState::SetLevel(int levelId) {
        GN_LOG_INFO("Setting level to: " + std::to_string(levelId));
        m_currentLevelId = levelId;
        
        // Load level configuration
        m_currentLevelConfig = LevelConfigFactory::GetLevelConfig(levelId);
        GN_LOG_INFO("Loaded configuration for: " + m_currentLevelConfig.levelName);
        
        // Reset game state for new level
        m_currentScore = 0;
        m_currentLives = STARTING_LIVES;
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        m_levelCompleted = false;

        
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
        SaveGameProgress();
    }

    void GameplayState::TogglePause() {
        if (m_currentSubState == GameplaySubState::Paused) {
            TriggerResume();
        } else if (m_currentSubState == GameplaySubState::Playing) {
            TriggerPause();
        }
    }

    // Private method implementations

    void GameplayState::InitializeSystems() {
        GN_LOG_INFO("Initializing gameplay systems");
        
        if (!m_platformDelegates) {
            GN_LOG_ERROR("Platform delegates is null!");
            return;
        }
        
        // Create sprite system
        m_spriteSystem = std::make_unique<SpriteSystem>(m_ecsSystem, *m_platformDelegates);
        
        // Create player controller system
        m_playerControllerSystem = std::make_unique<PlayerControllerSystem>(m_ecsSystem, m_platformDelegates, m_spriteSystem.get());
        
        // Create camera system
        m_cameraSystem = std::make_unique<CameraSystem>(m_ecsSystem);
        
        // Create unified render system (replaces individual sprite rendering)
        m_renderSystem = std::make_unique<RenderSystem>(m_ecsSystem, *m_platformDelegates);
        // Set texture base path for asset catalog via shared RenderSystem
        if (m_renderSystem) {
            m_renderSystem->SetTextureBasePath("turd/");
        }
        
        // Create UI system for text and button rendering
        m_uiSystem = std::make_unique<UISystem>(m_ecsSystem, *m_platformDelegates);
        
        // Create level manager system
        m_levelManager = std::make_unique<LevelManager>(m_ecsSystem);
        if (m_platformDelegates) {
            m_levelManager->SetPlatformDelegates(*m_platformDelegates);
        }
        // Create pickup system and pass dependencies
        m_pickupSystem = std::make_unique<PickupSystem>(m_ecsSystem, m_levelManager.get(), m_platformDelegates, &m_currentLevelConfig);
        // Create enemy system for behaviors (bobbing, states, etc.)
        m_enemySystem = std::make_unique<EnemySystem>(m_ecsSystem, m_levelManager.get());
        
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
        
        GN_LOG_INFO("Gameplay systems initialized successfully");
        
        // Setup platform-specific layout after systems are initialized
        SetupLayout();
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
            Transform playerTransform(Gnosis::GNVector2(400.0f, 639.0f), 0.0f, Gnosis::GNVector2(playerScale, playerScale));
            m_ecsSystem->AddComponent<Transform>(m_playerEntity, playerTransform);
            
            // Add sprite component with Turdlet idle animation (use existing playerScale)
            // TurdletIdle.png is 64x64 pixels, single frame
            Sprite playerSprite("TurdletIdle", 64.0f, 64.0f, 64, 64, 1, 0.1f);
            playerSprite.color = Gnosis::GNColor(255, 255, 255, 255);
            playerSprite.visible = true;
            playerSprite.layer = 4; // Player layer (above backgrounds, below effects)
            m_ecsSystem->AddComponent<Sprite>(m_playerEntity, playerSprite);
            
            // Add physics component
            Physics playerPhysics;
            playerPhysics.useGravity = true;
            playerPhysics.mass = 1.0f;
            playerPhysics.drag = 0.98f;
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
            DebugDraw playerDebug(false, false, Gnosis::GNColor(0, 255, 0, 255), Gnosis::GNColor(255, 0, 0, 255));
            playerDebug.alpha = 0.35f;
            // Debug hitboxes off for production visuals
            
            // Add player component
            PlayerComponent playerData;
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
            }

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
            } else {
                // Other background layers (Back, Mid) are 1024x480
                textureWidth = 1024.0f;
                textureHeight = 480.0f;
            }
            
            // Scale to fit iPhone 16 screen height in portrait mode (actual pixels)
            // iPhone 16 Portrait: 1179×2556 actual pixels
            float screenHeight = 2556.0f; // iPhone 16 portrait pixel height
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
            // Use screen width + 2 extra instances for smooth scrolling
            float screenWidth = 1179.0f; // iPhone 16 portrait pixel width
            int numInstances = static_cast<int>(std::ceil(screenWidth / scaledWidth)) + 2;
            
            // Ensure minimum of 3 instances for proper wrapping
            numInstances = std::max(numInstances, 3);
            
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
                parallaxComponent.autoScroll = true;
                m_ecsSystem->AddComponent<Parallax>(bgEntity, parallaxComponent);
                GN_LOG_INFO("Added Parallax component to entity " + std::to_string(bgEntity) + " with scrollSpeed=" + std::to_string(layerConfig.scrollSpeed));
                
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
        
        GN_LOG_INFO("=== PHASE 1 COMPLETE: Created " + std::to_string(totalEntitiesCreated) + " total background entities ===");
        GN_LOG_INFO("Expected entities: " + std::to_string(m_currentLevelConfig.backgroundLayers.size()) + " layers × 3+ instances = " + std::to_string(m_currentLevelConfig.backgroundLayers.size() * 3) + "+ entities");
        GN_LOG_INFO("Actual entities in vector: " + std::to_string(m_backgroundEntities.size()));
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
        
        // Destroy projectiles
        for (Gnosis::Entity entity : m_projectiles) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_projectiles.clear();
        
        // Destroy enemies
        for (Gnosis::Entity entity : m_enemies) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_enemies.clear();
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
    
    // Create coin bag icon (32x32) at bottom-left: 10% from left, 10% from bottom
    {
        float screenW = 1179.0f, screenH = 2556.0f;
        if (m_renderSystem) {
            const ScreenInfo& si = m_renderSystem->GetScreenInfo();
            screenW = si.pixelWidth;
            screenH = si.pixelHeight;
        }
        float iconX = screenW * 0.10f;
        float iconY = screenH * 0.85f; // 15% from bottom (pixel Y increases downward)
        const float bagScale = 8.0f;    // Scale 32x32 coin bag to 256x256
        m_coinBagEntity = m_ecsSystem->CreateEntity();
        if (m_coinBagEntity != 0) {
            Transform tr(Gnosis::GNVector2(iconX, iconY), 0.0f, Gnosis::GNVector2(bagScale, bagScale));
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
            float textX = iconX + (32.0f * bagScale) + 8.0f; // bag width + small gap
            float textY = iconY + (32.0f * bagScale * 0.5f) + 24.0f; // nudge down a bit more
            Transform tr(Gnosis::GNVector2(textX, textY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
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
    }

    // Create temporary menu button using proper UI system
    m_tempMenuButtonEntity = m_ecsSystem->CreateEntity();
    if (m_tempMenuButtonEntity != 0) {
        // Calculate proper top-right position using actual pixel dimensions from ScreenInfo
        float screenWidth = 1179.0f;
        float screenHeight = 2556.0f;
        if (m_renderSystem) {
            const ScreenInfo& si = m_renderSystem->GetScreenInfo();
            screenWidth = si.pixelWidth;
            screenHeight = si.pixelHeight;
        }
        
        // Get screen dimensions from render system for debugging
        if (m_renderSystem) {
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            GN_LOG_INFO("Render system dimensions: " + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight));
            GN_LOG_INFO("Using actual device dimensions: " + std::to_string(screenWidth) + "x" + std::to_string(screenHeight));
            
            // For now, use hardcoded iPhone 16 dimensions since render system gives scaled values
            // TODO: Get actual device dimensions from platform layer
        }
        
        // Position in top-right corner with safe margins
        // Move slightly left from the corner and a bit further up using pixel dimensions
        float menuX = screenWidth * 0.90f;      // 90% from left edge
        float menuY = screenHeight * 0.05f;     // 5% from top
        GN_LOG_INFO("CreateUI: menu button target pos=(" + std::to_string(menuX) + "," + std::to_string(menuY) + ")");
        
        Transform menuButtonTransform(Gnosis::GNVector2(menuX, menuY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_tempMenuButtonEntity, menuButtonTransform);
        GN_LOG_INFO("CreateUI: added Transform to menu button entity " + std::to_string(m_tempMenuButtonEntity) +
                    " pos=(" + std::to_string(menuX) + "," + std::to_string(menuY) + ")");
        
        // Create UIElement for proper text rendering
        UIElement menuButton;
        menuButton.buttonText = "[MENU]";
        menuButton.fontSize = 24.0f;
        menuButton.textColor = Gnosis::GNColor(255, 100, 100, 255);  // Red color for visibility
        menuButton.centerTextHorizontally = true;
        menuButton.centerTextVertically = true;
        menuButton.visible = true;
        menuButton.isEnabled = true;
        menuButton.normalTextureId = "";  // Text-only button (no background texture)
        m_ecsSystem->AddComponent<UIElement>(m_tempMenuButtonEntity, menuButton);
        
        GN_LOG_INFO("Created temporary menu button with UIElement at (" + std::to_string(menuX) + ", " + std::to_string(menuY) + ")");
    }
    
    // Create heart UI - positioned at same X as coin bag, starting from menu button Y position  
    if (m_heartSystem) {
        float screenWidth = 1179.0f;
        float screenHeight = 2556.0f;
        if (m_renderSystem) {
            const ScreenInfo& si = m_renderSystem->GetScreenInfo();
            screenWidth = si.pixelWidth;
            screenHeight = si.pixelHeight;
        }
        
        // Position hearts at same X as coin bag (10% from left), starting 25% from top
        float heartX = screenWidth * 0.10f;   // Same X as coin bag 
        float heartY = screenHeight * 0.25f;  // 25% from top (below pipe counter)
        
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
    }
}

void GameplayState::DestroyUI() {
    GN_LOG_INFO("Destroying UI elements");
    
    if (!m_ecsSystem) {
        return;
    }
    
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
    
    if (m_pauseMenuEntity != 0) {
        m_ecsSystem->DestroyEntity(m_pauseMenuEntity);
        m_pauseMenuEntity = 0;
    }
    
    if (m_tempMenuButtonEntity != 0) {
        m_ecsSystem->DestroyEntity(m_tempMenuButtonEntity);
        m_tempMenuButtonEntity = 0;
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
                // Pipe counter top-center placement (10% from top)
                float centerX = si.pixelWidth * 0.50f;
                float pipeCounterY = si.pixelHeight * 0.10f;
        if (m_pipeCounterEntity != 0) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(m_pipeCounterEntity);
                    if (t) {
                        t->position.x = centerX;
                        t->position.y = pipeCounterY;
                    }
                }

                // Reposition coin bag and coins text based on pixel screen size
                if (m_coinBagEntity != 0) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(m_coinBagEntity);
                    if (t) {
                        t->position.x = si.pixelWidth * 0.10f;
                        t->position.y = si.pixelHeight * 0.85f; // 15% from bottom
                        t->scale.x = 8.0f; // Keep 8x scale after sync
                        t->scale.y = 8.0f;
                    }
                }
                if (m_coinsTextEntity != 0) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(m_coinsTextEntity);
                    if (t) {
                        // Text to the right of the scaled bag, vertically centered
                        t->position.x = (si.pixelWidth * 0.10f) + (32.0f * 8.0f) + 8.0f;
                        t->position.y = (si.pixelHeight * 0.85f) + (32.0f * 8.0f * 0.5f) + 16.0f; // nudge down by 16px
                    }
                }

                // Menu button: 90% width, 5% height
                float menuX = si.pixelWidth * 0.90f;
                float menuY = si.pixelHeight * 0.05f;
                if (m_tempMenuButtonEntity != 0) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(m_tempMenuButtonEntity);
                    if (t) {
                        t->position.x = menuX;
                        t->position.y = menuY;
                    }
                }

                GN_LOG_INFO("UI Repositioned with PIXELS: screenW=" + std::to_string(si.pixelWidth) +
                            " screenH=" + std::to_string(si.pixelHeight) +
                            " pipeCounter=(" + std::to_string(centerX) + "," + std::to_string(pipeCounterY) + ") top-center" +
                            " menuPos=(" + std::to_string(menuX) + "," + std::to_string(menuY) + ")");
                m_uiPositionsSynced = true;
            }
        }
    }

    void GameplayState::UpdateSpawning(float deltaTime) {
        // Use new unified ObstacleSystem via LevelManager interface
        if (m_levelManager && m_cameraSystem) {
            // Get world scroll distance for wrapping calculations
            // Camera stays at (0,0), only world objects move, so use world scroll distance directly
            float worldScrollDistance = m_cameraSystem->GetWorldPosition();
            
            // Update obstacles using new ObstacleSystem (no redundant calls)
            m_levelManager->UpdateObstacleSystem(deltaTime, worldScrollDistance);
            
            // Update other pooling systems (non-obstacle)
            m_levelManager->UpdateEnemyPooling(deltaTime, worldScrollDistance);
            if (m_enemySystem) {
                m_enemySystem->Update(deltaTime);
            }
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
    
    // =======================================================================
    // NEW: Pickup Coordination Methods (moved from LevelManager)
    // =======================================================================
    
    // Pickup logic moved to PickupSystem

    void GameplayState::HandleGameEvents() {
        // Event handling will be implemented in Phase 2
    }

    void GameplayState::CleanupOffscreenEntities() {
        // Use LevelManager for cleanup
        if (m_levelManager) {
            m_levelManager->CleanupOffscreenEntities(-100.0f); // Left boundary
        }
    }

    void GameCore::GameplayState::CheckLevelCompletion() {
        // Level completion logic will be implemented in Phase 3
        // For now, just check if player is still alive
        if (!m_playerAlive && m_currentLives <= 0) {
            GameOver();
        }
    }

    void GameCore::GameplayState::SaveGameProgress() {
        GN_LOG_INFO("Saving game progress");
        // Save logic will be implemented in Phase 4
    }

    // Event handler implementations (will be expanded in Phase 2)
    void GameCore::GameplayState::OnPlayerJump() {
        GN_LOG_INFO("Player jumped");
    }

    void GameCore::GameplayState::OnPlayerShoot() {
        GN_LOG_INFO("Player shot");
    }

    void GameCore::GameplayState::OnPlayerHurt(int damage) {
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
                
                // Check if player is dead (this will be handled by UpdateSubState)
                if (player->liveSlices <= 0) {
                    GN_LOG_INFO("Player has no heart slices remaining - death will be handled by game over system");
                }
            }
        }
    }

    void GameCore::GameplayState::OnPlayerDeath() {
        GN_LOG_INFO("Player died");
        m_playerAlive = false;
        // Death is now handled by the heart system and UpdateSubState
    }

    void GameCore::GameplayState::OnCoinCollected(int value) {
        GN_LOG_INFO("Coin collected: " + std::to_string(value));
        // Update player session coins; totalCoins handled when persisting between levels
        if (PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity)) {
            player->sessionCoins += value;
        }
    }

    void GameCore::GameplayState::OnPickupCollected() {
        GN_LOG_INFO("Pickup collected");
        m_currentScore += 50;
    }

    void GameCore::GameplayState::OnObstacleHit() {
        GN_LOG_INFO("Obstacle hit");
        if (m_invulnerabilityTimer <= 0.0f) {
            OnPlayerHurt(1);
        }
    }

    void GameCore::GameplayState::OnEnemyDefeated() {
        GN_LOG_INFO("Enemy defeated!");
        // Handle enemy defeat logic
    }
    
    // Menu navigation functions
    void GameplayState::ReturnToMainMenu() {
        GN_LOG_INFO("Returning to main menu from gameplay");
        m_finished = true;  // This will trigger state transition back to main menu
    }
    
    void GameplayState::CheckMenuButtonClick(float touchX, float touchY) {
        if (m_tempMenuButtonEntity == 0) {
            return;  // No menu button exists
        }
        
        // Get button transform for simple text-based button
        auto transform = m_ecsSystem->GetComponent<Transform>(m_tempMenuButtonEntity);
        
        if (transform) {
            // Simple rectangular hit area around the text button (approximate size)
            float buttonWidth = 100.0f;  // Approximate width for "[MENU]" text
            float buttonHeight = 30.0f;  // Approximate height for text
            
            float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
            float buttonRight = transform->position.x + (buttonWidth / 2.0f);
            float buttonTop = transform->position.y - (buttonHeight / 2.0f);
            float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
            
            if (touchX >= buttonLeft && touchX <= buttonRight && 
                touchY >= buttonTop && touchY <= buttonBottom) {
                GN_LOG_INFO("Menu button clicked! Returning to main menu.");
                ReturnToMainMenu();
            }
        }
    }

    void GameCore::GameplayState::StartLevelMusic() {
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

    void GameCore::GameplayState::StopLevelMusic() {
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
    
    void GameplayState::CheckToiletCollisions() {
        if (!m_playerAlive || !m_ecsSystem || !m_levelManager) {
            return;
        }
        
        // Get player transform and hitbox
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        Hitbox* playerHitbox = m_ecsSystem->GetComponent<Hitbox>(m_playerEntity);
        if (!playerTransform || !playerHitbox) {
            return;
        }
        // Player circle collision (center-based): transform position is top-left; add sprite half-dimensions
        Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        float pHalfW = playerSprite ? (playerSprite->width * playerTransform->scale.x * 0.5f) : 0.0f;
        float pHalfH = playerSprite ? (playerSprite->height * playerTransform->scale.y * 0.5f) : 0.0f;
        float pCenterX = playerTransform->position.x + pHalfW + (playerHitbox->offsetX * playerTransform->scale.x);
        float pCenterY = playerTransform->position.y + pHalfH + (playerHitbox->offsetY * playerTransform->scale.y);
        float pRadius  = playerHitbox->radius * ((playerTransform->scale.x + playerTransform->scale.y) * 0.5f);
        
        GN_LOG_DEBUG("Player hitbox: position=(" + std::to_string(playerTransform->position.x) + "," + std::to_string(playerTransform->position.y) + ") " +
                    "center=(" + std::to_string(pCenterX) + "," + std::to_string(pCenterY) + ") " +
                    "radius=" + std::to_string(pRadius));
        
        // Check collision with all active obstacles (toilets) via ObstacleSystem
        const auto& activeObstacles = m_levelManager->GetActiveObstacles();
        GN_LOG_DEBUG("Checking collisions with " + std::to_string(activeObstacles.size()) + " active obstacles");
        
        bool playerHitThisFrame = false; // Track if player was hit this frame
        
        for (Gnosis::Entity obstacleEntity : activeObstacles) {
            Transform* obstacleTransform = m_ecsSystem->GetComponent<Transform>(obstacleEntity);
            Sprite* obstacleSprite = m_ecsSystem->GetComponent<Sprite>(obstacleEntity);
            Obstacle* obstacle = m_ecsSystem->GetComponent<Obstacle>(obstacleEntity);
            Hitbox* obstacleHitbox = m_ecsSystem->GetComponent<Hitbox>(obstacleEntity);
            
            if (!obstacleTransform || !obstacleSprite || !obstacle || !obstacleHitbox) {
                if (!obstacleTransform) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Transform component");
                else if (!obstacleSprite) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Sprite component");
                else if (!obstacle) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Obstacle component");
                else if (!obstacleHitbox) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(obstacleEntity) + " - no Hitbox component");
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
            GN_LOG_INFO("COLLISION DEBUG: Processing obstacle " + std::to_string(obstacleEntity) + " (" + obstacle->obstacleType + ") hitboxType=" + std::to_string(static_cast<int>(obstacleHitbox->type)));
            
            bool collided = false;
            
            if (obstacleHitbox->type == ColliderType::Circle) {
                // Circle-Circle collision (e.g., player vs spike ball)
                float oCenterX, oCenterY;
                
                // Special handling for SpikeBalls - use BASE entity for positioning
                if (obstacle->obstacleType == "SpikeBall" && m_levelManager && m_levelManager->GetObstacleSystem()) {
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
                
                GN_LOG_DEBUG("Circle-Rectangle: rect=(" + std::to_string(rectX) + "," + std::to_string(rectY) + "," + std::to_string(rectW) + "," + std::to_string(rectH) + ") " +
                           "closest=(" + std::to_string(closestX) + "," + std::to_string(closestY) + ") collided=" + std::to_string(collided));
            }
            
            // Enhanced debug logging
            if (collided) {
                GN_LOG_INFO("COLLISION DETECTED! Entity: " + std::to_string(obstacleEntity) + 
                           " Type: " + obstacle->obstacleType + 
                           " HitboxType: " + std::to_string(static_cast<int>(obstacleHitbox->type)) + 
                           " Invulnerable: " + std::to_string(m_invulnerabilityTimer > 0.0f) + 
                           " Timer: " + std::to_string(m_invulnerabilityTimer));
            } else {
                GN_LOG_DEBUG("No collision with obstacle " + std::to_string(obstacleEntity) + 
                           " (" + obstacle->obstacleType + ") type=" + std::to_string(static_cast<int>(obstacleHitbox->type)));
            }
            
            if (collided && m_invulnerabilityTimer <= 0.0f) {
                // Collision detected and player is not invulnerable!
                GN_LOG_INFO("*** COLLISION DETECTED! *** Entity: " + std::to_string(obstacleEntity) + " Type: " + obstacle->obstacleType + " HitboxType: " + std::to_string(static_cast<int>(obstacleHitbox->type)));
                playerHitThisFrame = true; // Mark that player was hit, but continue processing other obstacles
            } else if (collided) {
                GN_LOG_INFO("Collision detected but player invulnerable. Entity: " + std::to_string(obstacleEntity) + " Type: " + obstacle->obstacleType + " Timer: " + std::to_string(m_invulnerabilityTimer));
            }
            
            GN_LOG_DEBUG("Pipe clearing check for obstacle " + std::to_string(obstacleEntity) + 
                        " (" + obstacle->obstacleType + ") - pipeCleared=" + std::to_string(obstacle->pipeCleared));
            
            // Calculate pipe clearing variables for this obstacle
            float playerRight = pCenterX + pRadius;
            float pipeCenterX = rectX + rectW * 0.5f;
            
            // Increment once per column (X-range) as the player passes its center.
            // This prevents double count when top and bottom exist at similar X.
            if (!obstacle->pipeCleared) {
                GN_LOG_DEBUG("Pipe clearing check: obstacle " + std::to_string(obstacleEntity) + 
                            " (" + obstacle->obstacleType + "), playerRight=" + std::to_string(playerRight) + 
                            ", pipeCenterX=" + std::to_string(pipeCenterX));
                
                if (playerRight > pipeCenterX) {
                                            // Define a horizontal window for this column using half the pipe width (scaled)
                        const float columnHalfWidth = rectW * 0.5f;
                        const float windowMinX = pipeCenterX - columnHalfWidth;
                        const float windowMaxX = pipeCenterX + columnHalfWidth;
                        
                        GN_LOG_DEBUG("Column window calculation: pipeCenterX=" + std::to_string(pipeCenterX) + 
                                   ", columnHalfWidth=" + std::to_string(columnHalfWidth) + 
                                   ", window=[" + std::to_string(windowMinX) + "," + std::to_string(windowMaxX) + "]");

                    // Track if we've cleared any obstacles in this column to avoid double counting
                    bool clearedAnyInColumn = false;

                                            // Mark all obstacles within this column window as cleared to dedup
                        const auto& allObstacles = m_levelManager->GetActiveObstacles();
                        GN_LOG_DEBUG("Processing " + std::to_string(allObstacles.size()) + " obstacles for column clearing");
                        for (Gnosis::Entity e2 : allObstacles) {
                        Obstacle* o2 = m_ecsSystem->GetComponent<Obstacle>(e2);
                        Transform* t2 = m_ecsSystem->GetComponent<Transform>(e2);
                        Sprite* s2 = m_ecsSystem->GetComponent<Sprite>(e2);
                        Hitbox* hb2 = m_ecsSystem->GetComponent<Hitbox>(e2);
                                                    if (!o2 || !t2 || !s2 || !hb2 || o2->pipeCleared) {
                                if (!o2) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(e2) + " - no Obstacle component");
                                else if (!t2) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(e2) + " - no Transform component");
                                else if (!s2) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(e2) + " - no Sprite component");
                                else if (!hb2) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(e2) + " - no Hitbox component");
                                else if (o2->pipeCleared) GN_LOG_DEBUG("Skipping obstacle " + std::to_string(e2) + " - already cleared");
                                continue;
                            }
                                                    float rectW2 = hb2->width * t2->scale.x;
                            float spriteHalfW2 = s2->width * t2->scale.x * 0.5f;
                            float rectCenterX2 = t2->position.x + spriteHalfW2 + (hb2->offsetX * t2->scale.x);
                            GN_LOG_DEBUG("Obstacle " + std::to_string(e2) + " hitbox: rectW=" + std::to_string(rectW2) + 
                                        ", spriteHalfW=" + std::to_string(spriteHalfW2) + 
                                        ", rectCenterX=" + std::to_string(rectCenterX2) + 
                                        ", position.x=" + std::to_string(t2->position.x) + 
                                        ", offsetX=" + std::to_string(hb2->offsetX * t2->scale.x));
                        if (rectCenterX2 >= windowMinX && rectCenterX2 <= windowMaxX) {
                            // Only count actual pipes, not brick walls
                            if (o2->obstacleType != "BrickWall") {
                                o2->pipeCleared = true;
                                clearedAnyInColumn = true;
                                GN_LOG_DEBUG("Marked obstacle " + std::to_string(e2) + " (" + o2->obstacleType + 
                                            ") as cleared in column window [" + std::to_string(windowMinX) + "," + std::to_string(windowMaxX) + "]");
                                // Also clear its explicit pair if any
                                if (o2->pairedEntity != 0) {
                                    if (Obstacle* pairedObstacle2 = m_ecsSystem->GetComponent<Obstacle>(o2->pairedEntity)) {
                                        pairedObstacle2->pipeCleared = true;
                                        GN_LOG_DEBUG("Also marked paired obstacle " + std::to_string(o2->pairedEntity) + " as cleared");
                                    }
                                }
                            } else {
                                GN_LOG_DEBUG("Skipping brick wall " + std::to_string(e2) + " - not a pipe");
                            }
                        } else {
                            GN_LOG_DEBUG("Obstacle " + std::to_string(e2) + " (" + o2->obstacleType + 
                                        ") outside column window [" + std::to_string(windowMinX) + "," + std::to_string(windowMaxX) + 
                                        "], rectCenterX=" + std::to_string(rectCenterX2));
                        }
                    }

                                            // Only increment pipe counter once per column, not per obstacle
                        if (clearedAnyInColumn) {
                            OnPipeCleared();
                            GN_LOG_INFO("Pipe column cleared at X=" + std::to_string(pipeCenterX) + 
                                       ", incrementing counter to " + std::to_string(m_pipesCleared + 1));
                        } else {
                            GN_LOG_DEBUG("No obstacles cleared in column at X=" + std::to_string(pipeCenterX) + 
                                        ", pipe counter not incremented");
                        }
                        
                        GN_LOG_DEBUG("Column clearing complete for obstacle " + std::to_string(obstacleEntity) + 
                                   " at X=" + std::to_string(pipeCenterX) + 
                                   ", clearedAnyInColumn=" + std::to_string(clearedAnyInColumn));
                } else {
                    GN_LOG_DEBUG("Pipe clearing check skipped for obstacle " + std::to_string(obstacleEntity) + 
                               " (" + obstacle->obstacleType + ") - already cleared");
                }
            } else {
                GN_LOG_DEBUG("Player hasn't passed pipe center yet: playerRight=" + std::to_string(playerRight) + 
                           ", pipeCenterX=" + std::to_string(pipeCenterX));
            }
        }
        
        // Handle hurt effects after processing all obstacles (allows pipe clearing to complete first)
        if (playerHitThisFrame) {
            GN_LOG_INFO("Player hurt this frame! Applying hurt effects after processing all obstacles");
            
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
        
        GN_LOG_DEBUG("Finished collision detection loop for " + std::to_string(activeObstacles.size()) + " obstacles");
    }
    
    void GameplayState::UpdatePipeCounterUI() {
        if (m_pipeCounterEntity == 0 || !m_ecsSystem) {
            return;
        }
        
        UIElement* pipeCounter = m_ecsSystem->GetComponent<UIElement>(m_pipeCounterEntity);
        if (pipeCounter) {
            pipeCounter->buttonText = std::to_string(m_pipesCleared);  // Just the number
        }
        // Update coins UI from player sessionCoins
        if (m_coinsTextEntity != 0) {
            UIElement* coinsUi = m_ecsSystem->GetComponent<UIElement>(m_coinsTextEntity);
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            if (coinsUi && player) {
                coinsUi->buttonText = std::to_string(player->sessionCoins);
            }
        }
    }
    
    void GameplayState::OnPipeCleared() {
        m_pipesCleared++;
        GN_LOG_INFO("Pipe cleared! Total pipes: " + std::to_string(m_pipesCleared));
        
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
                playerPhysics->velocity.y = 100.0f; // Initial downward velocity to start falling
                GN_LOG_INFO("Set dead player initial downward velocity: " + std::to_string(playerPhysics->velocity.y));
            }
        }
        
        // Immediately hide hearts when player dies
        if (m_heartSystem && m_playerEntity != 0) {
            m_heartSystem->UpdateHeartVisibility(m_playerEntity);
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
        
        // Get screen dimensions - use render system instead of creating local ScreenInfo objects
        float screenWidth = 1179.0f;  // Default iPhone 16 width
        float screenHeight = 2556.0f; // Default iPhone 16 height
        
        // Try to get actual screen info from render system if available
        if (m_renderSystem) {
            const GameCore::ScreenInfo& renderScreenInfo = m_renderSystem->GetScreenInfo();
            screenWidth = static_cast<float>(renderScreenInfo.pixelWidth);
            screenHeight = static_cast<float>(renderScreenInfo.pixelHeight);
        }
        
        // Create game over background (light from heaven) - start at top and stretch to full width
        m_gameOverBackgroundEntity = m_ecsSystem->CreateEntity();
        if (m_gameOverBackgroundEntity != Gnosis::INVALID_ENTITY) {
            // Calculate scale to stretch width to screen width while maintaining aspect ratio
            float backgroundScale = screenWidth / 256.0f; // 256 is texture width
            
            // Position at top center of screen
            Transform bgTransform(Gnosis::GNVector2(screenWidth * 0.5f, 0.0f), 0.0f, Gnosis::GNVector2(backgroundScale, backgroundScale));
            m_ecsSystem->AddComponent<Transform>(m_gameOverBackgroundEntity, bgTransform);
            
            // Create background sprite (like main menu) - THIS IS THE KEY DIFFERENCE!
            Sprite bgSprite("GameOverBackground", 256, 256); // Use actual texture dimensions
            bgSprite.layer = 100; // Background layer (lowest priority)
            bgSprite.visible = true;
            m_ecsSystem->AddComponent<Sprite>(m_gameOverBackgroundEntity, bgSprite);
            
            GN_LOG_INFO("Created game over background at top with scale: " + std::to_string(backgroundScale));
        }
        
        // Create morte sprite (floating above score) - use proper centering
        m_morteEntity = m_ecsSystem->CreateEntity();
        if (m_morteEntity != Gnosis::INVALID_ENTITY) {
            float morteScale = 6.0f;
            float morteWidth = 64.0f * morteScale;
            float morteHeight = 64.0f * morteScale;
            
            // Use CenterObjectAtPosition like main menu for proper centering
            Gnosis::GNVector2 mortePosition = CenterObjectAtPosition(screenWidth * 0.5f, screenHeight * 0.35f, morteWidth, morteHeight);
            
            Transform morteTransform(Gnosis::GNVector2(mortePosition.x, mortePosition.y), 0.0f, Gnosis::GNVector2(morteScale, morteScale));
            m_ecsSystem->AddComponent<Transform>(m_morteEntity, morteTransform);
            
            // Create morte sprite (like main menu) - THIS IS THE KEY DIFFERENCE!
            Sprite morteSprite("FloppyTurdMorte", 64, 64); // Use actual texture dimensions
            morteSprite.layer = 102; // Above background, below text
            morteSprite.visible = true;
            m_ecsSystem->AddComponent<Sprite>(m_morteEntity, morteSprite);
            
            GN_LOG_INFO("Created morte sprite at centered position: (" + std::to_string(mortePosition.x) + ", " + std::to_string(mortePosition.y) + ")");
        }
        
        // Create score display with pipes and coins - use proper centering and fix newlines
        m_gameOverScoreEntity = m_ecsSystem->CreateEntity();
        if (m_gameOverScoreEntity != Gnosis::INVALID_ENTITY) {
            float scoreScale = 6.0f;
            float scoreWidth = 256.0f * scoreScale; // Approximate scoreboard width
            float scoreHeight = 128.0f * scoreScale; // Approximate scoreboard height
            
            // Use CenterObjectAtPosition like main menu for proper centering
            Gnosis::GNVector2 scorePosition = CenterObjectAtPosition(screenWidth * 0.5f, screenHeight * 0.55f, scoreWidth, scoreHeight);
            
            Transform scoreTransform(Gnosis::GNVector2(scorePosition.x, scorePosition.y), 0.0f, Gnosis::GNVector2(scoreScale, scoreScale));
            m_ecsSystem->AddComponent<Transform>(m_gameOverScoreEntity, scoreTransform);
            
            // Get player stats for comprehensive score display
            PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
            int totalCoins = player ? player->sessionCoins : 0;
            
            UIElement scoreUI;
            // Fix newlines - use \n not \\n for proper line breaks
            scoreUI.buttonText = "Score: " + std::to_string(m_currentScore) + 
                               "\nPipes: " + std::to_string(m_pipesCleared) + 
                               "\nCoins: " + std::to_string(totalCoins);
            scoreUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
            scoreUI.visible = true;
            scoreUI.isEnabled = true;
            scoreUI.textLayer = 101; // Lower than morte, higher than background
            scoreUI.normalTextureId = "GameOverScore"; // Use correct asset catalog name
            scoreUI.fontSize = 28.0f; // Larger text for better visibility
            scoreUI.centerTextHorizontally = true;
            scoreUI.centerTextVertically = true;
            m_ecsSystem->AddComponent<UIElement>(m_gameOverScoreEntity, scoreUI);
            
            GN_LOG_INFO("Created game over score at centered position: (" + std::to_string(scorePosition.x) + ", " + std::to_string(scorePosition.y) + ")");
        }
        
        // Create death message (centered at top)
        m_deathMessageEntity = m_ecsSystem->CreateEntity();
        if (m_deathMessageEntity != Gnosis::INVALID_ENTITY) {
            // Center the message properly like the pipe counter
            float messageX = screenWidth * 0.5f;  // Center horizontally
            float messageY = screenHeight * 0.20f; // 20% from top (raised higher)
            Transform messageTransform(Gnosis::GNVector2(messageX, messageY), 0.0f, Gnosis::GNVector2(8.0f, 8.0f));
            m_ecsSystem->AddComponent<Transform>(m_deathMessageEntity, messageTransform);
            
            UIElement messageUI;
            messageUI.buttonText = GetRandomDeathMessage();
            messageUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
            messageUI.visible = true;
            messageUI.isEnabled = true;
            messageUI.textLayer = 104; // Highest priority for death message
            messageUI.fontSize = 56.0f; // Increased font size for better visibility
            messageUI.centerTextHorizontally = true;
            messageUI.centerTextVertically = true;
            messageUI.textOutlineWidth = 8.0f; // Add outline for better visibility
            messageUI.normalTextureId = ""; // Text-only, no background texture
            m_ecsSystem->AddComponent<UIElement>(m_deathMessageEntity, messageUI);
            
            GN_LOG_INFO("Created death message: " + messageUI.buttonText + " at (" + std::to_string(messageX) + ", " + std::to_string(messageY) + ") with font size: " + std::to_string(messageUI.fontSize));
        }
        
        // Create Try Again button - use proper centering and main menu font size
        m_tryAgainButtonEntity = m_ecsSystem->CreateEntity();
        if (m_tryAgainButtonEntity != Gnosis::INVALID_ENTITY) {
            float buttonScale = 10.0f; // Match main menu button scale
            float buttonWidth = 90.0f * buttonScale; // 90 is texture width
            float buttonHeight = 16.0f * buttonScale; // 16 is texture height
            
            // Use CenterObjectAtPosition like main menu for proper centering
            Gnosis::GNVector2 tryAgainPosition = CenterObjectAtPosition(screenWidth * 0.5f, screenHeight * 0.8f, buttonWidth, buttonHeight);
            
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
            tryAgainUI.fontSize = 18.0f; // Match main menu button font size
            tryAgainUI.centerTextHorizontally = true;
            tryAgainUI.centerTextVertically = true;
            // Ensure the button texture is properly set
            tryAgainUI.isHovered = false;
            tryAgainUI.isPressed = false;
            m_ecsSystem->AddComponent<UIElement>(m_tryAgainButtonEntity, tryAgainUI);
            
            GN_LOG_INFO("Created Try Again button at centered position: (" + std::to_string(tryAgainPosition.x) + ", " + std::to_string(tryAgainPosition.y) + ") with scale: " + std::to_string(buttonScale));
        }
        
        // Create Quit button - use proper centering and main menu font size
        m_quitButtonEntity = m_ecsSystem->CreateEntity();
        if (m_quitButtonEntity != Gnosis::INVALID_ENTITY) {
            float buttonScale = 10.0f; // Match main menu button scale
            float buttonWidth = 90.0f * buttonScale; // 90 is texture width
            float buttonHeight = 16.0f * buttonScale; // 16 is texture height
            
            // Use CenterObjectAtPosition like main menu for proper centering
            Gnosis::GNVector2 quitPosition = CenterObjectAtPosition(screenWidth * 0.5f, screenHeight * 0.9f, buttonWidth, buttonHeight);
            
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
            quitUI.fontSize = 18.0f; // Match main menu button font size
            quitUI.centerTextHorizontally = true;
            quitUI.centerTextVertically = true;
            // Ensure the button texture is properly set
            quitUI.isHovered = false;
            quitUI.isPressed = false;
            m_ecsSystem->AddComponent<UIElement>(m_quitButtonEntity, quitUI);
            
            GN_LOG_INFO("Created Quit button at centered position: (" + std::to_string(quitPosition.x) + ", " + std::to_string(quitPosition.y) + ") with scale: " + std::to_string(buttonScale));
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
            float baseY = 1022.4f; // Default iPhone 16 height * 0.4f
            
            // Try to get actual screen info from render system if available
            if (m_renderSystem) {
                const GameCore::ScreenInfo& renderScreenInfo = m_renderSystem->GetScreenInfo();
                baseY = static_cast<float>(renderScreenInfo.pixelHeight) * 0.4f;
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
                    // Larger hit area for better touch detection
                    float buttonWidth = 300.0f;  // Larger width for button
                    float buttonHeight = 120.0f; // Larger height for button
                    
                    float buttonLeft = tryAgainTransform->position.x - (buttonWidth / 2.0f);
                    float buttonRight = tryAgainTransform->position.x + (buttonWidth / 2.0f);
                    float buttonTop = tryAgainTransform->position.y - (buttonHeight / 2.0f);
                    float buttonBottom = tryAgainTransform->position.y + (buttonHeight / 2.0f);
                    
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
                    // Larger hit area for better touch detection
                    float buttonWidth = 300.0f;  // Larger width for button
                    float buttonHeight = 120.0f; // Larger height for button
                    
                    float buttonLeft = quitTransform->position.x - (buttonWidth / 2.0f);
                    float buttonRight = quitTransform->position.x + (buttonWidth / 2.0f);
                    float buttonTop = quitTransform->position.y - (buttonHeight / 2.0f);
                    float buttonBottom = quitTransform->position.y + (buttonHeight / 2.0f);
                    
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
        GN_LOG_INFO("Player chose to try again");
        
        // Reset player state
        if (m_playerControllerSystem) {
            m_playerControllerSystem->SetPlayerAlive(true);
        }
        
        // Reset player health and hearts
        if (m_heartSystem && m_playerEntity != 0) {
            Difficulty currentDifficulty = LevelManager::GetGlobalDifficulty();
            m_heartSystem->UpdateHeartCountForDifficulty(m_playerEntity, currentDifficulty);
        }
        
        // Reset player position and physics
        if (m_playerEntity != 0) {
            Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
            Physics* playerPhysics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            
            if (playerTransform && playerPhysics) {
                // Reset to starting position
                playerTransform->position = Gnosis::GNVector2(400.0f, 639.0f);
                playerPhysics->velocity = Gnosis::GNVector2(0.0f, 0.0f);
                playerPhysics->acceleration = Gnosis::GNVector2(0.0f, 0.0f);
            }
        }
        
        // Reset game state
        m_currentScore = 0;
        m_pipesCleared = 0;
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        
        // Reset spawn timers
        m_obstacleSpawnTimer = 0.0f;
        m_pickupSpawnTimer = 0.0f;
        m_enemySpawnTimer = 0.0f;
        
        // Clear any existing obstacles/enemies/pickups
        if (m_levelManager) {
            m_levelManager->DestroyAllEntities();
        }
        
        // Clear pickups
        if (m_pickupSystem) {
            m_pickupSystem->ClearAll();
        }
        
        // Clear obstacles (ObstacleSystem cleanup is handled by LevelManager::DestroyAllEntities)
        
        // Reset camera world position (simple reset)
        if (m_cameraSystem) {
            // Reset world scroll position to 0
            // Note: CameraSystem doesn't have a ResetCamera method, so we'll just let it continue from current position
        }
        
        // Start level music again
        StartLevelMusic();
        
        // Clean up and show UI
        DestroyGameOverUI();
        ShowRegularUI();
        m_currentSubState = GameplaySubState::Playing;
        
        GN_LOG_INFO("Level restarted successfully");
    }

    void GameplayState::QuitToMainMenu() {
        GN_LOG_INFO("Player chose to quit to main menu");
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
            "Bog standard death!"
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
        // Hide all regular gameplay UI elements
        std::vector<Gnosis::Entity*> uiElements = {
            &m_scoreTextEntity,
            &m_livesTextEntity,
            &m_coinsTextEntity,
            &m_coinBagEntity,
            &m_pipeCounterEntity,
            &m_tempMenuButtonEntity,
            &m_heartUIEntity
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
            // The heart system should handle hiding its own entities
            // We could add a method to HeartSystem to hide hearts, but they should already be invisible when dead
        }
        
        GN_LOG_INFO("Hidden all regular UI elements for game over");
    }

    void GameplayState::ShowRegularUI() {
        // Show all regular gameplay UI elements
        std::vector<Gnosis::Entity*> uiElements = {
            &m_scoreTextEntity,
            &m_livesTextEntity,
            &m_coinsTextEntity,
            &m_coinBagEntity,
            &m_pipeCounterEntity,
            &m_tempMenuButtonEntity,
            &m_heartUIEntity
        };
        
        for (Gnosis::Entity* entityPtr : uiElements) {
            if (entityPtr && *entityPtr != 0 && m_ecsSystem) {
                UIElement* ui = m_ecsSystem->GetComponent<UIElement>(*entityPtr);
                if (ui) {
                    ui->visible = true;
                }
            }
        }
        
        GN_LOG_INFO("Shown all regular UI elements");
    }

} // namespace GameCore