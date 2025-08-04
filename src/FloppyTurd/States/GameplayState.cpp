#include "GameplayState.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include <algorithm>

namespace GameCore {

    GameplayState::GameplayState(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, int levelId)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_currentLevelId(levelId)
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
        GN_LOG_INFO("GameplayState created for level: " + std::to_string(levelId));
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
        
        GN_LOG_INFO("GameplayState entered successfully");
    }

    void GameplayState::Exit() {
        GN_LOG_INFO("Exiting GameplayState");
        
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
        // Render sprites
        if (m_spriteSystem) {
            m_spriteSystem->Render();
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
            // Check for touch input
            if (m_platformDelegates->input.getTouchCount && m_platformDelegates->input.getTouchPosition) {
                int touchCount = m_platformDelegates->input.getTouchCount();
                
                if (touchCount > 0) {
                    GN_LOG_INFO("Touch detected! Count: " + std::to_string(touchCount));
                }
                
                for (int i = 0; i < touchCount; i++) {
                    float x, y;
                    m_platformDelegates->input.getTouchPosition(i, &x, &y);
                    
                    GN_LOG_INFO("Touch " + std::to_string(i) + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                    
                    // For now, treat any touch as a press (we'll refine this later)
                    m_playerControllerSystem->HandleTouchInput(x, y, true);
                }
            } else {
                GN_LOG_WARN("Touch input functions not available!");
                GN_LOG_DEBUG("GameplayState: getTouchCount = " + std::string(m_platformDelegates->input.getTouchCount ? "available" : "NULL"));
                GN_LOG_DEBUG("GameplayState: getTouchPosition = " + std::string(m_platformDelegates->input.getTouchPosition ? "available" : "NULL"));
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
            // Add basic components to player (centered on screen)
            // iPhone 16 game coordinates are 1179x2556, so center at (589.5, 1278)
            Transform playerTransform(Gnosis::GNVector2(589.5f, 1278.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_playerEntity, playerTransform);
            
            // Add sprite component with Turdlet idle animation (16x scale)
            // TurdletIdle.png is 64x64 pixels, single frame
            Sprite playerSprite("TurdletIdle", 512.0f, 512.0f, 64, 64, 1, 0.1f); // 16x scale (64 * 8 = 512)
            playerSprite.color = Gnosis::GNColor(255, 255, 255, 255);
            playerSprite.visible = true;
            playerSprite.layer = 1;
            m_ecsSystem->AddComponent<Sprite>(m_playerEntity, playerSprite);
            
            // Add physics component
            Physics playerPhysics;
            playerPhysics.useGravity = true;
            playerPhysics.mass = 1.0f;
            playerPhysics.drag = 0.98f;
            m_ecsSystem->AddComponent<Physics>(m_playerEntity, playerPhysics);
            
            // Add collider component (16x scale)
            Collider playerCollider;
            playerCollider.type = ColliderType::Rectangle;
            playerCollider.width = 512.0f;
            playerCollider.height = 512.0f;
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
        
        // Create camera entity
        m_cameraEntity = m_ecsSystem->CreateEntity();
        if (m_cameraEntity != 0) {
            // Camera functionality will be implemented in Phase 2
            // For now, just add a transform component
            Transform cameraTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_cameraEntity, cameraTransform);
            
            GN_LOG_INFO("Created camera entity: " + std::to_string(m_cameraEntity));
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
        // Update obstacle spawning
        m_obstacleSpawnTimer += deltaTime;
        if (m_obstacleSpawnTimer >= OBSTACLE_SPAWN_INTERVAL) {
            SpawnObstacle();
            m_obstacleSpawnTimer = 0.0f;
        }
        
        // Update pickup spawning
        m_pickupSpawnTimer += deltaTime;
        if (m_pickupSpawnTimer >= PICKUP_SPAWN_INTERVAL) {
            SpawnPickup();
            m_pickupSpawnTimer = 0.0f;
        }
        
        // Update enemy spawning
        m_enemySpawnTimer += deltaTime;
        if (m_enemySpawnTimer >= ENEMY_SPAWN_INTERVAL) {
            SpawnEnemy();
            m_enemySpawnTimer = 0.0f;
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
        if (!m_ecsSystem) {
            return;
        }
        
        // Clean up obstacles that are off-screen to the left
        m_obstacles.erase(
            std::remove_if(m_obstacles.begin(), m_obstacles.end(),
                [this](Gnosis::Entity entity) {
                    if (entity == 0) return true;
                    
                    Transform* transform = m_ecsSystem->GetComponent<Transform>(entity);
                    if (transform && transform->position.x < -100.0f) {
                        m_ecsSystem->DestroyEntity(entity);
                        return true;
                    }
                    return false;
                }),
            m_obstacles.end()
        );
        
        // Clean up pickups that are off-screen to the left
        m_pickups.erase(
            std::remove_if(m_pickups.begin(), m_pickups.end(),
                [this](Gnosis::Entity entity) {
                    if (entity == 0) return true;
                    
                    Transform* transform = m_ecsSystem->GetComponent<Transform>(entity);
                    if (transform && transform->position.x < -100.0f) {
                        m_ecsSystem->DestroyEntity(entity);
                        return true;
                    }
                    return false;
                }),
            m_pickups.end()
        );
        
        // Clean up enemies that are off-screen to the left
        m_enemies.erase(
            std::remove_if(m_enemies.begin(), m_enemies.end(),
                [this](Gnosis::Entity entity) {
                    if (entity == 0) return true;
                    
                    Transform* transform = m_ecsSystem->GetComponent<Transform>(entity);
                    if (transform && transform->position.x < -100.0f) {
                        m_ecsSystem->DestroyEntity(entity);
                        return true;
                    }
                    return false;
                }),
            m_enemies.end()
        );
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
        GN_LOG_INFO("Player hurt: " + std::to_string(damage));
        m_currentLives--;
        m_invulnerabilityTimer = 2.0f; // 2 seconds of invulnerability
        
        if (m_currentLives <= 0) {
            OnPlayerDeath();
        }
    }

    void GameplayState::OnPlayerDeath() {
        GN_LOG_INFO("Player died");
        m_playerAlive = false;
        GameOver();
    }

    void GameplayState::OnCoinCollected(int value) {
        GN_LOG_INFO("Coin collected: " + std::to_string(value));
        m_currentCoins += value;
        m_currentScore += value * 10;
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
        GN_LOG_INFO("Enemy defeated");
        m_currentScore += 100;
    }

} // namespace GameCore 