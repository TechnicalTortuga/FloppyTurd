#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include <algorithm>
#include <cmath>

namespace GameCore {

    // Static member definition - default to Regular difficulty
    Difficulty LevelManager::s_globalDifficulty = Difficulty::Regular;

    LevelManager::LevelManager(Gnosis::ECS* ecsSystem)
        : m_ecsSystem(ecsSystem)
        , m_isLoaded(false)
        , m_currentLevelId(0)
        , m_currentLevelConfig(0, "")
        , m_obstacleSpawnTimer(0.0f)
        , m_enemySpawnTimer(0.0f)
        , m_pickupSpawnTimer(0.0f)
        , m_lastObstacleX(1000.0f)   // Start obstacles off screen to the right
        , m_lastEnemyX(1200.0f)      // Start enemies further out
        , m_lastPickupX(800.0f)      // Start pickups closer
    {
        GN_LOG_INFO("LevelManager created");
        InitializeProgressionSystem();
    }

    LevelManager::~LevelManager() {
        UnloadLevel();
        GN_LOG_INFO("LevelManager destroyed");
    }

    bool LevelManager::LoadLevel(int levelId) {
        GN_LOG_INFO("Loading level: " + std::to_string(levelId));
        
        if (!ValidateLevelId(levelId)) {
            GN_LOG_ERROR("Invalid level ID: " + std::to_string(levelId));
            return false;
        }
        
        // Unload current level if any
        if (m_isLoaded) {
            UnloadLevel();
        }
        
        // Load new level configuration
        m_currentLevelId = levelId;
        m_currentLevelConfig = LevelConfigFactory::GetLevelConfig(levelId);
        
        // Apply current difficulty settings
        m_currentLevelConfig.ApplyDifficulty(s_globalDifficulty);
        
        // Create background layers
        CreateBackgroundLayers();
        
        // Reset spawn timers and positions
        m_obstacleSpawnTimer = 0.0f;
        m_enemySpawnTimer = 0.0f;
        m_pickupSpawnTimer = 0.0f;
        m_lastObstacleX = 1000.0f;
        m_lastEnemyX = 1200.0f;
        m_lastPickupX = 800.0f;
        
        m_isLoaded = true;
        GN_LOG_INFO("Level " + std::to_string(levelId) + " (" + m_currentLevelConfig.levelName + ") loaded successfully");
        
        return true;
    }

    void LevelManager::UnloadLevel() {
        if (!m_isLoaded) {
            return;
        }
        
        GN_LOG_INFO("Unloading level: " + std::to_string(m_currentLevelId));
        
        // Destroy all level entities
        DestroyAllEntities();
        
        // Reset state
        m_isLoaded = false;
        m_currentLevelId = 0;
        m_currentLevelConfig = LevelConfig(0, "");
        
        GN_LOG_INFO("Level unloaded");
    }

    void LevelManager::ResetLevel() {
        if (!m_isLoaded) {
            return;
        }
        
        int levelId = m_currentLevelId;
        UnloadLevel();
        LoadLevel(levelId);
        
        GN_LOG_INFO("Level " + std::to_string(levelId) + " reset");
    }

    std::string LevelManager::GetCurrentLevelName() const {
        if (!m_isLoaded) {
            return "No Level Loaded";
        }
        return m_currentLevelConfig.levelName;
    }

    bool LevelManager::IsLevelUnlocked(int levelId) const {
        if (!ValidateLevelId(levelId)) {
            return false;
        }
        
        // Level 1 is always unlocked
        if (levelId == 1) {
            return true;
        }
        
        // Check if previous level is completed
        if (levelId > 1 && levelId <= GetMaxLevelId()) {
            return m_levelCompleted[levelId - 2]; // Previous level completed
        }
        
        return false;
    }

    void LevelManager::UnlockLevel(int levelId) {
        if (ValidateLevelId(levelId) && levelId <= static_cast<int>(m_unlockedLevels.size())) {
            m_unlockedLevels[levelId - 1] = true;
            SaveProgression();
            GN_LOG_INFO("Level " + std::to_string(levelId) + " unlocked");
        }
    }

    bool LevelManager::CompleteLevel(int levelId, int score, int coinsCollected) {
        if (!ValidateLevelId(levelId)) {
            return false;
        }
        
        // Mark level as completed
        m_levelCompleted[levelId - 1] = true;
        
        // Update high score if better
        if (score > m_levelScores[levelId - 1]) {
            m_levelScores[levelId - 1] = score;
        }
        
        // Unlock next level if it exists
        if (levelId < GetMaxLevelId()) {
            UnlockLevel(levelId + 1);
        }
        
        SaveProgression();
        GN_LOG_INFO("Level " + std::to_string(levelId) + " completed with score: " + std::to_string(score));
        
        return true;
    }

    void LevelManager::UpdateObstacleSpawning(float deltaTime) {
        if (!m_isLoaded || m_currentLevelConfig.obstacles.empty()) {
            return;
        }
        
        m_obstacleSpawnTimer += deltaTime;
        
        // Check if it's time to spawn a new obstacle
        if (m_obstacleSpawnTimer >= m_currentLevelConfig.obstacleSpawnRate) {
            // Choose a random obstacle configuration
            int obstacleIndex = rand() % m_currentLevelConfig.obstacles.size();
            const ObstacleConfig& config = m_currentLevelConfig.obstacles[obstacleIndex];
            
            // Calculate spawn position
            float spawnX = CalculateNextObstaclePosition();
            float spawnY = 300.0f; // Middle of screen height, adjust as needed
            
            // Spawn the obstacle
            SpawnObstacle(config, spawnX, spawnY);
            
            // Reset timer
            m_obstacleSpawnTimer = 0.0f;
        }
    }

    void LevelManager::UpdateEnemySpawning(float deltaTime) {
        if (!m_isLoaded || m_currentLevelConfig.enemies.empty()) {
            return;
        }
        
        m_enemySpawnTimer += deltaTime;
        
        // Check if it's time to spawn a new enemy
        if (m_enemySpawnTimer >= m_currentLevelConfig.enemySpawnRate) {
            // Choose a random enemy configuration
            int enemyIndex = rand() % m_currentLevelConfig.enemies.size();
            const EnemyConfig& config = m_currentLevelConfig.enemies[enemyIndex];
            
            // Calculate spawn position
            float spawnX = CalculateNextEnemyPosition();
            float spawnY = 250.0f; // Adjust based on enemy type
            
            // Spawn the enemy
            SpawnEnemy(config, spawnX, spawnY);
            
            // Reset timer with some randomness
            m_enemySpawnTimer = 0.0f;
        }
    }

    void LevelManager::UpdatePickupSpawning(float deltaTime) {
        if (!m_isLoaded) {
            return;
        }
        
        m_pickupSpawnTimer += deltaTime;
        
        // Check if it's time to spawn a new pickup
        if (m_pickupSpawnTimer >= m_currentLevelConfig.pickupSpawnRate) {
            // Choose pickup type (coin, power-up, etc.)
            std::vector<std::string> pickupTypes = {"BlueCoin", "GoldCoin", "RedCoin", "PooHeart"};
            std::string pickupType = pickupTypes[rand() % pickupTypes.size()];
            
            // Calculate spawn position
            float spawnX = CalculateNextPickupPosition();
            float spawnY = 200.0f + (rand() % 200); // Random height
            
            // Spawn the pickup
            SpawnPickup(pickupType, spawnX, spawnY);
            
            // Reset timer
            m_pickupSpawnTimer = 0.0f;
        }
    }

    Gnosis::Entity LevelManager::SpawnObstacle(const ObstacleConfig& config, float x, float y) {
        if (!m_ecsSystem) {
            return 0;
        }
        
        Gnosis::Entity obstacle = m_ecsSystem->CreateEntity();
        
        // Create transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, 
                          Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
        // Create sprite
        Sprite sprite(config.textureId, config.width, config.height);
        sprite.layer = 3; // Obstacle layer
        sprite.visible = true;
        
        // Create physics (if needed)
        Physics physics;
        physics.velocity.x = -config.speed; // Move left with world
        physics.useGravity = false; // Obstacles don't use gravity
        
        // Create collider component
        Collider collider;
        collider.type = ColliderType::Rectangle;
        collider.width = config.width;
        collider.height = config.height;
        collider.isStatic = false; // Moves with world
        collider.isTrigger = false;
        collider.tag = "obstacle";
        
        // Create obstacle component
        Obstacle obstacleComp;
        obstacleComp.obstacleType = config.textureId;
        obstacleComp.damage = 1;
        
        // Add components
        m_ecsSystem->AddComponent<Transform>(obstacle, transform);
        m_ecsSystem->AddComponent<Sprite>(obstacle, sprite);
        m_ecsSystem->AddComponent<Physics>(obstacle, physics);
        m_ecsSystem->AddComponent<Collider>(obstacle, collider);
        m_ecsSystem->AddComponent<Obstacle>(obstacle, obstacleComp);
        
        // Track active obstacle
        m_activeObstacles.push_back(obstacle);
        
        GN_LOG_DEBUG("Spawned obstacle: " + config.textureId + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        
        return obstacle;
    }

    void LevelManager::RemoveObstacle(Gnosis::Entity obstacle) {
        if (!m_ecsSystem) {
            return;
        }
        
        // Remove from tracking
        auto it = std::find(m_activeObstacles.begin(), m_activeObstacles.end(), obstacle);
        if (it != m_activeObstacles.end()) {
            m_activeObstacles.erase(it);
        }
        
        // Destroy entity
        m_ecsSystem->DestroyEntity(obstacle);
    }

    Gnosis::Entity LevelManager::SpawnEnemy(const EnemyConfig& config, float x, float y) {
        if (!m_ecsSystem) {
            return 0;
        }
        
        Gnosis::Entity enemy = m_ecsSystem->CreateEntity();
        
        // Create transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, 
                          Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
        // Create sprite with animation configuration
        Sprite sprite(config.textureId, config.width, config.height);
        sprite.layer = 4; // Enemy layer
        sprite.visible = true;
        
        // Configure animation for specific enemy types
        if (config.textureId == "BirdIdle") {
            // Birds have 4 frames of 32x32 in horizontal spritesheet (128x32 total)
            sprite.isAnimated = true;
            sprite.frameWidth = 32;
            sprite.frameHeight = 32;
            sprite.frameCount = 4; // 4 frames for bird flapping animation
            sprite.frameTime = 0.16f; // 160ms per frame for slower bird flapping
            sprite.currentFrame = 0;
            sprite.currentFrameTime = 0.0f;
            sprite.playing = true;
            sprite.loop = true; // Birds loop infinitely
            sprite.hasCompleted = false;
            
            GN_LOG_DEBUG("LevelManager: Created animated bird enemy with 4 frames of 32x32");
        } else if (config.textureId == "RatCopterIdle" || config.textureId == "ToiletPaperFlap") {
            // Other animated enemies can be configured similarly
            sprite.isAnimated = true;
            sprite.frameWidth = static_cast<int>(config.width);
            sprite.frameHeight = static_cast<int>(config.height);
            sprite.frameCount = 4; // Default frame count
            sprite.frameTime = 0.15f; // Default frame time
            sprite.currentFrame = 0;
            sprite.currentFrameTime = 0.0f;
            sprite.playing = true;
            sprite.loop = true;
            sprite.hasCompleted = false;
        } else {
            // Static enemies
            sprite.isAnimated = false;
            sprite.frameWidth = static_cast<int>(config.width);
            sprite.frameHeight = static_cast<int>(config.height);
            sprite.frameCount = 1;
        }
        
        // Create physics
        Physics physics;
        physics.velocity.x = -config.speed; // Move left with world
        
        // Create collider
        Collider collider;
        collider.isStatic = false;
        collider.width = config.width;
        collider.height = config.height;
        collider.tag = "Enemy";
        
        // Create enemy component
        Enemy enemyComp;
        enemyComp.health = config.hitPoints;
        enemyComp.enemyType = config.movementPattern;
        enemyComp.isActive = true;
        
        // Add components
        m_ecsSystem->AddComponent<Transform>(enemy, transform);
        m_ecsSystem->AddComponent<Sprite>(enemy, sprite);
        m_ecsSystem->AddComponent<Physics>(enemy, physics);
        m_ecsSystem->AddComponent<Collider>(enemy, collider);
        m_ecsSystem->AddComponent<Enemy>(enemy, enemyComp);
        
        // Track active enemy
        m_activeEnemies.push_back(enemy);
        
        GN_LOG_DEBUG("Spawned enemy: " + config.textureId + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        
        return enemy;
    }

    void LevelManager::RemoveEnemy(Gnosis::Entity enemy) {
        if (!m_ecsSystem) {
            return;
        }
        
        // Remove from tracking
        auto it = std::find(m_activeEnemies.begin(), m_activeEnemies.end(), enemy);
        if (it != m_activeEnemies.end()) {
            m_activeEnemies.erase(it);
        }
        
        // Destroy entity
        m_ecsSystem->DestroyEntity(enemy);
    }

    Gnosis::Entity LevelManager::SpawnPickup(const std::string& type, float x, float y) {
        if (!m_ecsSystem) {
            return 0;
        }
        
        Gnosis::Entity pickup = m_ecsSystem->CreateEntity();
        
        // Create transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, 
                          Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
        // Create sprite (dimensions and animation based on pickup type)
        float width = 32.0f, height = 32.0f;
        Sprite sprite(type, width, height);
        sprite.layer = 5; // Pickup layer
        sprite.visible = true;
        
        // Configure animation for coins - they have 10 frames of 16x16 in horizontal spritesheet (160x16 total)
        if (type == "BlueCoin" || type == "GoldCoin" || type == "RedCoin") {
            width = height = 16.0f; // Coins are 16x16
            sprite.width = width;
            sprite.height = height;
            sprite.isAnimated = true;
            sprite.frameWidth = 16;
            sprite.frameHeight = 16; 
            sprite.frameCount = 10; // 10 frames for coin animation
            sprite.frameTime = 0.12f; // 120ms per frame for slower, more pleasant coin spinning
            sprite.currentFrame = 0;
            sprite.currentFrameTime = 0.0f;
            sprite.playing = true;
            sprite.loop = true; // Coins loop infinitely
            sprite.hasCompleted = false;
            
            GN_LOG_DEBUG("LevelManager: Created animated coin " + type + " with 10 frames of 16x16");
        } else if (type == "PooHeart") {
            width = height = 48.0f; // Hearts are bigger
            sprite.width = width;
            sprite.height = height;
            // PooHeart could be animated too if it has multiple frames
            sprite.isAnimated = false; // For now, keep hearts static
        } else {
            // Default pickup configuration
            sprite.isAnimated = false;
        }
        
        // Create physics
        Physics physics;
        physics.velocity.x = -m_currentLevelConfig.worldSpeed; // Move left with world
        
        // Create collider  
        Collider collider;
        collider.isStatic = false;
        collider.width = width;
        collider.height = height;
        collider.tag = "Pickup";
        
        // Create pickup component
        Pickup pickupComp;
        pickupComp.pickupType = type;
        pickupComp.value = (type.find("Coin") != std::string::npos) ? 10 : 1; // Coins worth 10, hearts worth 1
        pickupComp.isActive = true;
        
        // Add components
        m_ecsSystem->AddComponent<Transform>(pickup, transform);
        m_ecsSystem->AddComponent<Sprite>(pickup, sprite);
        m_ecsSystem->AddComponent<Physics>(pickup, physics);
        m_ecsSystem->AddComponent<Collider>(pickup, collider);
        m_ecsSystem->AddComponent<Pickup>(pickup, pickupComp);
        
        // Track active pickup
        m_activePickups.push_back(pickup);
        
        GN_LOG_DEBUG("Spawned pickup: " + type + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        
        return pickup;
    }

    void LevelManager::RemovePickup(Gnosis::Entity pickup) {
        if (!m_ecsSystem) {
            return;
        }
        
        // Remove from tracking
        auto it = std::find(m_activePickups.begin(), m_activePickups.end(), pickup);
        if (it != m_activePickups.end()) {
            m_activePickups.erase(it);
        }
        
        // Destroy entity
        m_ecsSystem->DestroyEntity(pickup);
    }

    void LevelManager::CleanupOffscreenEntities(float leftBoundary) {
        // Clean up obstacles that have moved off screen
        for (auto it = m_activeObstacles.begin(); it != m_activeObstacles.end();) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
            if (transform && transform->position.x < leftBoundary) {
                m_ecsSystem->DestroyEntity(*it);
                it = m_activeObstacles.erase(it);
            } else {
                ++it;
            }
        }
        
        // Clean up enemies that have moved off screen
        for (auto it = m_activeEnemies.begin(); it != m_activeEnemies.end();) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
            if (transform && transform->position.x < leftBoundary) {
                m_ecsSystem->DestroyEntity(*it);
                it = m_activeEnemies.erase(it);
            } else {
                ++it;
            }
        }
        
        // Clean up pickups that have moved off screen
        for (auto it = m_activePickups.begin(); it != m_activePickups.end();) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
            if (transform && transform->position.x < leftBoundary) {
                m_ecsSystem->DestroyEntity(*it);
                it = m_activePickups.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool LevelManager::ValidateLevelId(int levelId) {
        return levelId >= 1 && levelId <= GetMaxLevelId();
    }

    void LevelManager::InitializeProgressionSystem() {
        int maxLevels = GetMaxLevelId();
        m_unlockedLevels.resize(maxLevels, false);
        m_levelScores.resize(maxLevels, 0);
        m_levelCompleted.resize(maxLevels, false);
        
        // Level 1 is always unlocked
        m_unlockedLevels[0] = true;
        
        LoadProgression();
        
        GN_LOG_INFO("Progression system initialized for " + std::to_string(maxLevels) + " levels");
    }

    void LevelManager::CreateBackgroundLayers() {
        if (!m_ecsSystem) {
            return;
        }
        
        DestroyBackgroundLayers(); // Clean up any existing layers
        
        GN_LOG_INFO("=== LEVELMANAGER: Starting background layer creation ===");
        GN_LOG_INFO("Number of layer configs: " + std::to_string(m_currentLevelConfig.backgroundLayers.size()));
        
        int totalEntitiesCreated = 0;
        
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
            
            // Scaled dimensions (actual rendered size after Transform scaling)
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
                       ", scaledHeight=" + std::to_string(scaledHeight) + 
                       ", numInstances=" + std::to_string(numInstances));
            
            float repeatWidth = layerConfig.repeatWidth > 0 ? layerConfig.repeatWidth : scaledWidth;
            
            // For initial positioning, we want instances to be placed touching each other
            // Use the actual scaled texture width for positioning
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
                GN_LOG_INFO("Added ParallaxInstance component to entity " + std::to_string(bgEntity) + " (instance " + std::to_string(i) + " of " + std::to_string(numInstances) + ")" +
                           " scaledWidth=" + std::to_string(scaledWidth));
                
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
        
        GN_LOG_INFO("=== LEVELMANAGER COMPLETE: Created " + std::to_string(totalEntitiesCreated) + " total background entities ===");
        GN_LOG_INFO("Expected entities: " + std::to_string(m_currentLevelConfig.backgroundLayers.size()) + " layers × 3+ instances = " + std::to_string(m_currentLevelConfig.backgroundLayers.size() * 3) + "+ entities");
        GN_LOG_INFO("Actual entities in vector: " + std::to_string(m_backgroundEntities.size()));
    }

    void LevelManager::DestroyBackgroundLayers() {
        if (!m_ecsSystem) {
            return;
        }
        
        for (Gnosis::Entity entity : m_backgroundEntities) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_backgroundEntities.clear();
    }

    void LevelManager::DestroyAllEntities() {
        // Destroy background layers
        DestroyBackgroundLayers();
        
        // Destroy all active obstacles
        for (Gnosis::Entity entity : m_activeObstacles) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_activeObstacles.clear();
        
        // Destroy all active enemies
        for (Gnosis::Entity entity : m_activeEnemies) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_activeEnemies.clear();
        
        // Destroy all active pickups
        for (Gnosis::Entity entity : m_activePickups) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_activePickups.clear();
    }

    float LevelManager::CalculateNextObstaclePosition() {
        // Space obstacles based on difficulty and level config
        float baseSpacing = 800.0f; // Base spacing between obstacles
        float spacing = baseSpacing / m_currentLevelConfig.difficultyMultiplier;
        
        m_lastObstacleX += spacing;
        return m_lastObstacleX;
    }

    float LevelManager::CalculateNextEnemyPosition() {
        // Space enemies differently than obstacles
        float baseSpacing = 600.0f;
        float spacing = baseSpacing / m_currentLevelConfig.difficultyMultiplier;
        
        m_lastEnemyX += spacing;
        return m_lastEnemyX;
    }

    float LevelManager::CalculateNextPickupPosition() {
        // Space pickups more frequently
        float baseSpacing = 400.0f;
        float spacing = baseSpacing;
        
        m_lastPickupX += spacing;
        return m_lastPickupX;
    }

    void LevelManager::SaveProgression() {
        // TODO: Implement save system (file I/O or platform-specific storage)
        // For now, just log the action
        GN_LOG_DEBUG("Progression saved");
    }

    void LevelManager::LoadProgression() {
        // TODO: Implement load system (file I/O or platform-specific storage)
        // For now, just log the action
        GN_LOG_DEBUG("Progression loaded");
    }

} // namespace GameCore
