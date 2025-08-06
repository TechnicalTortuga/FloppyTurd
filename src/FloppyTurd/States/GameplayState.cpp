#include "GameplayState.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include <algorithm>

namespace GameCore {

    GameplayState::GameplayState(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, int levelId)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_currentLevelId(levelId)
        , m_currentLevelConfig(LevelConfigFactory::GetLevelConfig(levelId))
        , m_currentScore(0)
        , m_currentCoins(0)
        , m_currentLives(STARTING_LIVES)
        , m_gameTime(0.0f)
        , m_difficultyTimer(0.0f)
        , m_difficultyLevel(1.0f)
        , m_playerAlive(true)
        , m_invulnerabilityTimer(0.0f)
        , m_finished(false)
        , m_isPaused(false)
        , m_levelCompleted(false)
        , m_gameOver(false)
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
        m_currentCoins = 0;
        m_currentLives = STARTING_LIVES;
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        m_finished = false;
        m_isPaused = false;
        m_levelCompleted = false;
        m_gameOver = false;
        
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
        
        // Clean up entities
        DestroyGameEntities();
        DestroyUI();
        
        GN_LOG_INFO("GameplayState exited");
    }

    void GameplayState::Pause() {
        GN_LOG_INFO("Pausing GameplayState");
        m_isPaused = true;
    }

    void GameplayState::Resume() {
        GN_LOG_INFO("Resuming GameplayState");
        m_isPaused = false;
    }

    void GameplayState::Update(float deltaTime) {
        if (m_isPaused) {
            return;
        }
        
        // Update game time
        m_gameTime += deltaTime;
        
        // Update input delay timer
        m_inputDelayTimer += deltaTime;
        
        // Handle input only after delay period to prevent auto-shooting
        if (m_inputDelayTimer >= INPUT_DELAY_TIME) {
            HandleInput();
        }
        
        // Update systems
        if (m_spriteSystem) {
            m_spriteSystem->Update(deltaTime);
        }
        
        if (m_playerControllerSystem) {
            m_playerControllerSystem->Update(deltaTime);
        }
        
        if (m_cameraSystem) {
            m_cameraSystem->Update(deltaTime);
        }
        
        // Update game logic
        UpdateGameLogic(deltaTime);
        
        // Update spawning
        UpdateSpawning(deltaTime);
        
        // Update difficulty
        UpdateDifficulty(deltaTime);
        
        // Handle game events
        HandleGameEvents();
        
        // Clean up offscreen entities
        CleanupOffscreenEntities();
        
        // Check level completion
        CheckLevelCompletion();
    }

    void GameplayState::Render() {
        // Use unified render system instead of individual sprite system
        if (m_renderSystem) {
            m_renderSystem->Render();
        }
    }

    void GameplayState::HandleInput() {
        if (m_isPaused) {
            // Handle pause menu input
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
        m_currentCoins = 0;
        m_currentLives = STARTING_LIVES;
        m_gameTime = 0.0f;
        m_difficultyTimer = 0.0f;
        m_difficultyLevel = 1.0f;
        m_playerAlive = true;
        m_invulnerabilityTimer = 0.0f;
        m_levelCompleted = false;
        m_gameOver = false;
        
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
        m_gameOver = true;
        m_playerAlive = false;
        SaveGameProgress();
    }

    void GameplayState::LevelComplete() {
        GN_LOG_INFO("Level Complete!");
        m_levelCompleted = true;
        SaveGameProgress();
    }

    void GameplayState::TogglePause() {
        if (m_isPaused) {
            Resume();
        } else {
            Pause();
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
        
        // Set texture base path for asset catalog
        m_spriteSystem->SetTextureBasePath("turd/");
        
        // Create player controller system
        m_playerControllerSystem = std::make_unique<PlayerControllerSystem>(m_ecsSystem, m_platformDelegates, m_spriteSystem.get());
        
        // Create camera system
        m_cameraSystem = std::make_unique<CameraSystem>(m_ecsSystem);
        
        // Create unified render system (replaces individual sprite rendering)
        m_renderSystem = std::make_unique<RenderSystem>(m_ecsSystem, *m_platformDelegates);
        
        // Create level manager system
        m_levelManager = std::make_unique<LevelManager>(m_ecsSystem);
        
        // Load the current level
        if (!m_levelManager->LoadLevel(m_currentLevelId)) {
            GN_LOG_ERROR("Failed to load level " + std::to_string(m_currentLevelId));
        } else {
            GN_LOG_INFO("Level " + std::to_string(m_currentLevelId) + " loaded successfully");
        }
        
        GN_LOG_INFO("Gameplay systems initialized successfully");
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
            
            // Add collider component (scaled to match sprite)
            Collider playerCollider;
            playerCollider.type = ColliderType::Rectangle;
            playerCollider.width = 64.0f * playerScale;
            playerCollider.height = 64.0f * playerScale;
            m_ecsSystem->AddComponent<Collider>(m_playerEntity, playerCollider);
            
            // Add player component
            PlayerComponent playerData;
            m_ecsSystem->AddComponent<PlayerComponent>(m_playerEntity, playerData);
            
            // Set up player controller
            if (m_playerControllerSystem) {
                m_playerControllerSystem->SetPlayerEntity(m_playerEntity);
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
        
        // Destroy pickups
        for (Gnosis::Entity entity : m_pickups) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_pickups.clear();
        
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
        GN_LOG_INFO("Creating UI elements");
        
        if (!m_ecsSystem) {
            return;
        }
        
        // Create score text entity
        m_scoreTextEntity = m_ecsSystem->CreateEntity();
        if (m_scoreTextEntity != 0) {
            Transform scoreTransform(Gnosis::GNVector2(50.0f, 50.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_scoreTextEntity, scoreTransform);
            
            Text scoreText("Score: 0", 24.0f, Gnosis::GNColor(255, 255, 255, 255), 10);
            m_ecsSystem->AddComponent<Text>(m_scoreTextEntity, scoreText);
        }
        
        // Create lives text entity
        m_livesTextEntity = m_ecsSystem->CreateEntity();
        if (m_livesTextEntity != 0) {
            Transform livesTransform(Gnosis::GNVector2(50.0f, 80.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_livesTextEntity, livesTransform);
            
            Text livesText("Lives: " + std::to_string(m_currentLives), 24.0f, Gnosis::GNColor(255, 255, 255, 255), 10);
            m_ecsSystem->AddComponent<Text>(m_livesTextEntity, livesText);
        }
        
        // Create coins text entity
        m_coinsTextEntity = m_ecsSystem->CreateEntity();
        if (m_coinsTextEntity != 0) {
            Transform coinsTransform(Gnosis::GNVector2(50.0f, 110.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_coinsTextEntity, coinsTransform);
            
            Text coinsText("Coins: " + std::to_string(m_currentCoins), 24.0f, Gnosis::GNColor(255, 255, 0, 255), 10);
            m_ecsSystem->AddComponent<Text>(m_coinsTextEntity, coinsText);
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
        
        if (m_pauseMenuEntity != 0) {
            m_ecsSystem->DestroyEntity(m_pauseMenuEntity);
            m_pauseMenuEntity = 0;
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
            if (coinsText) {
                coinsText->text = "Coins: " + std::to_string(m_currentCoins);
            }
        }
    }

    void GameplayState::UpdateSpawning(float deltaTime) {
        // Use LevelManager for all spawning
        if (m_levelManager) {
            m_levelManager->UpdateObstacleSpawning(deltaTime);
            m_levelManager->UpdateEnemySpawning(deltaTime);
            m_levelManager->UpdatePickupSpawning(deltaTime);
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

    void GameplayState::SpawnObstacle() {
        GN_LOG_INFO("Spawning obstacle");
        
        if (!m_ecsSystem) {
            return;
        }
        
        Gnosis::Entity obstacleEntity = m_ecsSystem->CreateEntity();
        if (obstacleEntity != 0) {
            // Position obstacle off-screen to the right
            Transform obstacleTransform(Gnosis::GNVector2(800.0f, 400.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(obstacleEntity, obstacleTransform);
            
            // Add sprite component (placeholder)
            Sprite obstacleSprite;
            obstacleSprite.textureId = ""; // Will be set when we load textures
            obstacleSprite.width = 64.0f;
            obstacleSprite.height = 64.0f;
            obstacleSprite.color = Gnosis::GNColor(255, 0, 0, 255);
            obstacleSprite.visible = true;
            obstacleSprite.layer = 1;
            m_ecsSystem->AddComponent<Sprite>(obstacleEntity, obstacleSprite);
            
            // Add collider component
            Collider obstacleCollider;
            obstacleCollider.type = ColliderType::Rectangle;
            obstacleCollider.width = 64.0f;
            obstacleCollider.height = 64.0f;
            m_ecsSystem->AddComponent<Collider>(obstacleEntity, obstacleCollider);
            
            m_obstacles.push_back(obstacleEntity);
        }
    }

    void GameplayState::SpawnPickup() {
        GN_LOG_INFO("Spawning pickup");
        
        if (!m_ecsSystem) {
            return;
        }
        
        Gnosis::Entity pickupEntity = m_ecsSystem->CreateEntity();
        if (pickupEntity != 0) {
            // Position pickup off-screen to the right
            Transform pickupTransform(Gnosis::GNVector2(800.0f, 300.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(pickupEntity, pickupTransform);
            
            // Add sprite component (placeholder)
            Sprite pickupSprite;
            pickupSprite.textureId = ""; // Will be set when we load textures
            pickupSprite.width = 32.0f;
            pickupSprite.height = 32.0f;
            pickupSprite.color = Gnosis::GNColor(255, 255, 0, 255);
            pickupSprite.visible = true;
            pickupSprite.layer = 1;
            m_ecsSystem->AddComponent<Sprite>(pickupEntity, pickupSprite);
            
            // Add collider component
            Collider pickupCollider;
            pickupCollider.type = ColliderType::Rectangle;
            pickupCollider.width = 32.0f;
            pickupCollider.height = 32.0f;
            m_ecsSystem->AddComponent<Collider>(pickupEntity, pickupCollider);
            
            m_pickups.push_back(pickupEntity);
        }
    }

    void GameplayState::SpawnEnemy() {
        GN_LOG_INFO("Spawning enemy");
        
        if (!m_ecsSystem) {
            return;
        }
        
        Gnosis::Entity enemyEntity = m_ecsSystem->CreateEntity();
        if (enemyEntity != 0) {
            // Position enemy off-screen to the right
            Transform enemyTransform(Gnosis::GNVector2(800.0f, 200.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(enemyEntity, enemyTransform);
            
            // Add sprite component (placeholder)
            Sprite enemySprite;
            enemySprite.textureId = ""; // Will be set when we load textures
            enemySprite.width = 48.0f;
            enemySprite.height = 48.0f;
            enemySprite.color = Gnosis::GNColor(0, 255, 0, 255);
            enemySprite.visible = true;
            enemySprite.layer = 1;
            m_ecsSystem->AddComponent<Sprite>(enemyEntity, enemySprite);
            
            // Add collider component
            Collider enemyCollider;
            enemyCollider.type = ColliderType::Rectangle;
            enemyCollider.width = 48.0f;
            enemyCollider.height = 48.0f;
            m_ecsSystem->AddComponent<Collider>(enemyEntity, enemyCollider);
            
            m_enemies.push_back(enemyEntity);
        }
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
        GN_LOG_INFO("Player hurt: " + std::to_string(damage));
        m_currentLives--;
        m_invulnerabilityTimer = 2.0f; // 2 seconds of invulnerability
        
        if (m_currentLives <= 0) {
            OnPlayerDeath();
        }
    }

    void GameCore::GameplayState::OnPlayerDeath() {
        GN_LOG_INFO("Player died");
        m_playerAlive = false;
        GameOver();
    }

    void GameCore::GameplayState::OnCoinCollected(int value) {
        GN_LOG_INFO("Coin collected: " + std::to_string(value));
        m_currentCoins += value;
        m_currentScore += value * 10;
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
        GN_LOG_INFO("Enemy defeated");
        m_currentScore += 100;
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

} // namespace GameCore 