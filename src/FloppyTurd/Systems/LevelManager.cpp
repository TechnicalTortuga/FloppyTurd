#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../Config/EnemyConfigs.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_map>
#include <deque>
#include <map>
#include <set>
#include <limits>

namespace GameCore {

    // Static member definition - default to Regular difficulty
    Difficulty LevelManager::s_globalDifficulty = Difficulty::Regular;

    LevelManager::LevelManager(Gnosis::ECS* ecsSystem)
        : m_ecsSystem(ecsSystem)
        , m_renderSystem(nullptr)  // 🎯 NEW: Initialize RenderSystem reference
        , m_isLoaded(false)
        , m_currentLevelId(0)
        , m_currentLevelConfig(0, "")
        , m_enemySpawnTimer(0.0f)
        , m_npcSpawnTimer(0.0f)
        , m_lastEnemyX(0.0f)
    {
        GN_LOG_INFO("LevelManager created");
        InitializeProgressionSystem();
        m_obstacleSystem = std::make_unique<ObstacleSystem>(ecsSystem);
    }

    LevelManager::~LevelManager() {
        UnloadLevel();
        GN_LOG_INFO("LevelManager destroyed");
    }

    bool LevelManager::LoadLevel(int levelId) {
        GN_LOG_DEBUG("[LevelManager] Loading level: " + std::to_string(levelId));
        
        if (!ValidateLevelId(levelId)) {
            GN_LOG_ERROR("Invalid level ID: " + std::to_string(levelId));
            return false;
        }
        
        // Get level configuration from factory
        m_currentLevelConfig = LevelConfigFactory::GetLevelConfig(levelId);
        m_currentLevelId = levelId;
        
        // Apply current difficulty settings
        m_currentLevelConfig.ApplyDifficulty(s_globalDifficulty);
        
        // Log level configuration
        GN_LOG_INFO("Loading level " + std::to_string(levelId) + ": " + m_currentLevelConfig.levelName);
        GN_LOG_INFO("Difficulty: " + DifficultyToString(s_globalDifficulty));
        GN_LOG_INFO("World speed: " + std::to_string(m_currentLevelConfig.worldSpeed));
        GN_LOG_INFO("Obstacle spawn rate: " + std::to_string(m_currentLevelConfig.obstacleSpawnRate));
        GN_LOG_INFO("Number of obstacles: " + std::to_string(m_currentLevelConfig.obstacles.size()));
        
        // Log all obstacles in this level
        for (size_t i = 0; i < m_currentLevelConfig.obstacles.size(); i++) {
            const auto& obs = m_currentLevelConfig.obstacles[i];
            GN_LOG_INFO("Obstacle " + std::to_string(i) + ": " + obs.textureId + 
                       " (bottom: " + obs.bottomTextureId + 
                       ", gap: " + std::to_string(obs.gapHeight) + 
                       ", speed: " + std::to_string(obs.speed) + ")");
        }
        
        // Unload current level if any
        if (m_isLoaded) {
            GN_LOG_INFO("Unloading current level before loading level " + std::to_string(levelId));
            UnloadLevel();
        }
        
        // Load new level configuration
        GN_LOG_INFO("Setting current level ID to " + std::to_string(levelId));
        m_currentLevelId = levelId;
        GN_LOG_INFO("Getting level config for level " + std::to_string(levelId));
        m_currentLevelConfig = LevelConfigFactory::GetLevelConfig(levelId);
        GN_LOG_INFO("Level config loaded: " + m_currentLevelConfig.levelName);
        
        // Apply current difficulty settings
        GN_LOG_INFO("Applying difficulty settings for level " + std::to_string(levelId));
        m_currentLevelConfig.ApplyDifficulty(s_globalDifficulty);
        
        // Create background layers
        GN_LOG_INFO("Creating background layers for level " + std::to_string(levelId));
        CreateBackgroundLayers();
        GN_LOG_INFO("Background layers created successfully for level " + std::to_string(levelId));
        
        // Reset spawn timers and positions
        GN_LOG_INFO("Resetting spawn timers and positions for level " + std::to_string(levelId));
        m_enemySpawnTimer = 0.0f;
        m_npcSpawnTimer = 0.0f;
        m_lastEnemyX = 0.0f;
        
        // Set loaded flag BEFORE initializing systems
        GN_LOG_INFO("Setting loaded flag for level " + std::to_string(levelId));
        m_isLoaded = true;
        
        // Initialize ObstacleSystem for this level
        GN_LOG_INFO("Initializing ObstacleSystem for level " + std::to_string(levelId));
        m_obstacleSystem->InitializeForLevel(levelId, m_currentLevelConfig);
        GN_LOG_INFO("ObstacleSystem initialized successfully for level " + std::to_string(levelId));


        
        // Initialize other entity pools
        m_enemyPoolInitialized = false;
        m_npcPoolInitialized = false;
        // Projectile system initialization is now handled by GameplayState
        
        GN_LOG_INFO("Initializing enemy pool for level " + std::to_string(levelId));
        InitializeEnemyPool();
        GN_LOG_INFO("Enemy pool initialized for level " + std::to_string(levelId));
        
        GN_LOG_INFO("Initializing NPC pool for level " + std::to_string(levelId));
        InitializeNPCPool();
        GN_LOG_INFO("NPC pool initialized for level " + std::to_string(levelId));
        
        GN_LOG_INFO("Projectile system ready for level " + std::to_string(levelId));
        // ProjectileSystem is now initialized separately in GameplayState
        
        // Pickups and projectiles are pooled but may be empty until used
        GN_LOG_INFO("Level " + std::to_string(levelId) + " (" + m_currentLevelConfig.levelName + ") loaded successfully");
        
        return true;
    }

    void LevelManager::UnloadLevel() {
        if (!m_isLoaded) {
            return;
        }
        
        GN_LOG_INFO("Unloading level " + std::to_string(m_currentLevelId));
        
        // Cleanup ObstacleSystem
        if (m_obstacleSystem) {
            m_obstacleSystem->Cleanup();
        }
        
        // Destroy all level entities
        DestroyAllEntities();
        
        // Reset state
        m_isLoaded = false;
        m_currentLevelId = 0;
        m_currentLevelConfig = LevelConfig(0, "");
        m_enemyPoolInitialized = false;
        m_npcPoolInitialized = false;
        // Projectile system initialization is now handled by GameplayState
        m_enemySpawnTimer = 0.0f;
        m_npcSpawnTimer = 0.0f;
        m_lastEnemyX = 0.0f;
        
        GN_LOG_INFO("Level unloaded successfully");
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

    void LevelManager::ResetBackgroundPositions() {
        if (!m_isLoaded) {
            return;
        }

        // Reset all parallax background positions to their initial state
        auto parallaxEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite, Parallax, ParallaxInstance>();
        int resetCount = 0;

        for (Gnosis::Entity entity : parallaxEntities) {
            auto transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto instance = m_ecsSystem->GetComponent<ParallaxInstance>(entity);
            auto sprite = m_ecsSystem->GetComponent<Sprite>(entity);

            if (transform && instance && sprite && instance->textureWidth > 0.0f) {
                // Reset to initial pixel-perfect position based on instance index
                // Use the same calculation as in CreateBackgroundLayers
                int pixelScaledWidth = static_cast<int>(std::round(instance->textureWidth));
                int pixelX = instance->instanceIndex * pixelScaledWidth;

                // Ensure pixel-perfect positioning with no sub-pixel artifacts
                transform->position.x = static_cast<float>(pixelX);
                transform->position.y = 0.0f; // Reset Y position as well (exact pixel boundary)

                // Also reset to initial texture variant if this is a sewer level
                if (sprite->textureId.find("Sewer") != std::string::npos && instance->instanceIndex == 0) {
                    sprite->textureId = "SewerLargeA"; // Reset to first variant
                }

                resetCount++;
                GN_LOG_INFO("Reset background entity " + std::to_string(entity) +
                           " (instance " + std::to_string(instance->instanceIndex) + ") to x=" +
                           std::to_string(transform->position.x));
            }
        }

        GN_LOG_INFO("Background positions reset for level " + std::to_string(m_currentLevelId) +
                   " - reset " + std::to_string(resetCount) + " entities");
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
        
        // DEBUG: ALL LEVELS UNLOCKED FOR TESTING
            return true;
        
        // Original unlock logic (commented out for testing):
        // Level 1 is always unlocked
        // if (levelId == 1) {
        //     return true;
        // }
        // 
        // // Check if previous level is completed
        // if (levelId > 1 && levelId <= GetMaxLevelId()) {
        //     return m_levelCompleted[levelId - 2]; // Previous level completed
        // }
        // 
        // return false;
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

    void LevelManager::UpdateObstacleSystem(float deltaTime, float worldScrollDistance) {
        if (m_obstacleSystem) {
            m_obstacleSystem->Update(deltaTime, worldScrollDistance);
        }
    }

    std::vector<int> LevelManager::ConsumeWrappedGroups() {
        if (m_obstacleSystem) {
            return m_obstacleSystem->ConsumeWrappedGroups();
        }
        return {};
    }

    std::vector<Gnosis::Entity> LevelManager::GetActiveObstacles() const {
        if (m_obstacleSystem) {
            return m_obstacleSystem->GetActiveObstacles();
        }
        return {};
    }

    void LevelManager::UpdateEnemySpawning(float) { /* disabled; using pool */ }

    // Spawn Janitor NPC periodically (Level 2 only)
    void LevelManager::UpdateNPCSpawning(float) { /* disabled; using pool */ }

    Gnosis::GNVector2 LevelManager::GetPlayerPosition() const {
        if (m_playerEntity != 0) {
            Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
            if (playerTransform) {
                return playerTransform->position;
            }
        }
        // Fallback: return center of screen if player not found
        const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        return Gnosis::GNVector2(screenInfo.pixelWidth * 0.5f, screenInfo.pixelHeight * 0.5f);
    }

    void LevelManager::InitializeEnemyPool() {
        if (m_enemyPoolInitialized) return;
        if (!m_isLoaded || !m_currentLevelConfig.enableEnemies || m_currentLevelConfig.enemies.empty()) return;

        GN_LOG_INFO("LevelManager: Initializing enemy pool with " + std::to_string(MAX_ENEMY_POOL_SIZE) + " enemies using level configs");

        // Create enemy pool with custom ratios for snow level
        int enemyIndex = 0;
        
        // Snow level (4) uses custom spawn ratios: 15% red thrower, 85% decoratives
        bool isSnowLevel = (m_currentLevelId == 4);
        std::unordered_map<std::string, int> snowRatios;
        if (isSnowLevel) {
            // Calculate counts for snow level: guarantee at least two throwers
            int throwerCount = std::max(2, static_cast<int>(MAX_ENEMY_POOL_SIZE * 0.15f));
            int decorativeTotal = MAX_ENEMY_POOL_SIZE - throwerCount;
            int decorativeEach = decorativeTotal / 3; // Split remaining among 3 decoratives
            
            snowRatios["SnowManIdle"] = throwerCount;  // Red thrower - rare!
            snowRatios["SnowManChill"] = decorativeEach;
            snowRatios["SnowManGreen"] = decorativeEach;
            snowRatios["SnowManChad"] = decorativeTotal - (decorativeEach * 2); // Get remainder
            
            GN_LOG_INFO("Snow level spawn ratios: Thrower=" + std::to_string(throwerCount) + 
                       ", Chill=" + std::to_string(snowRatios["SnowManChill"]) +
                       ", Green=" + std::to_string(snowRatios["SnowManGreen"]) +
                       ", Chad=" + std::to_string(snowRatios["SnowManChad"]));
        }
        
        // Default: equal distribution for non-snow levels
        int enemiesPerType = MAX_ENEMY_POOL_SIZE / m_currentLevelConfig.enemies.size();
        int remainder = MAX_ENEMY_POOL_SIZE % m_currentLevelConfig.enemies.size();

        for (size_t configIndex = 0; configIndex < m_currentLevelConfig.enemies.size(); ++configIndex) {
            const EnemyConfig& config = m_currentLevelConfig.enemies[configIndex];
            
            // Skip boss enemies - they're handled separately by BossSystem
            if (config.movementPattern == "boss_idle" || 
                config.textureId == "Ratking" || 
                config.textureId == "RatKing") {
                GN_LOG_INFO("LevelManager: Skipping boss enemy '" + config.textureId + "' - handled by BossSystem");
                continue;
            }
            
            // Determine count for this enemy type
            int countForThisType;
            if (isSnowLevel && snowRatios.find(config.textureId) != snowRatios.end()) {
                // Use custom ratio for snow level
                countForThisType = snowRatios[config.textureId];
            } else {
                // Default equal distribution
                countForThisType = enemiesPerType + (configIndex < remainder ? 1 : 0);
            }

            GN_LOG_DEBUG("LevelManager: Creating " + std::to_string(countForThisType) + " enemies of type " + config.textureId);

            GN_LOG_INFO("[POOL] Creating " + std::to_string(countForThisType) + " enemies of type '" + config.textureId + "'");
            
            for (int i = 0; i < countForThisType && enemyIndex < MAX_ENEMY_POOL_SIZE; ++i) {
                Gnosis::Entity enemy = m_ecsSystem->CreateEntity();

                // Add all required components but keep inactive
                Transform transform(Gnosis::GNVector2(-1000.0f, -1000.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                m_ecsSystem->AddComponent<Transform>(enemy, transform);

                // Add sprite using the actual enemy config - use animated constructor
                Sprite sprite(config.textureId, config.width, config.height, config.frameWidth, config.frameHeight, config.frameCount, config.frameTime);
                sprite.loop = config.loopAnimation;
                sprite.color = GNColor(255, 255, 255, 0); // Invisible initially
                sprite.visible = false; // Explicitly invisible for inactive enemies
                sprite.layer = 4; // Enemy layer (above pipes/obstacles layer 3, below player layer 5)
                m_ecsSystem->AddComponent<Sprite>(enemy, sprite);

                // Add enemy component (inactive)
                Enemy enemyComp;
                enemyComp.isActive = false;
                enemyComp.health = config.hitPoints;
                enemyComp.enemyType = config.textureId;
                enemyComp.movementPattern = config.movementPattern;
                
                // CRITICAL: Set isThrower flag for snowman throwers
                if (config.movementPattern == "snowman_thrower") {
                    enemyComp.isThrower = true;
                    GN_LOG_INFO("[POOL] Set isThrower=true for " + config.textureId);
                }
                
                m_ecsSystem->AddComponent<Enemy>(enemy, enemyComp);

                // Add physics component
                Physics physics;
                physics.velocity = Gnosis::GNVector2(0.0f, 0.0f);
                physics.useGravity = false;
                m_ecsSystem->AddComponent<Physics>(enemy, physics);

                // Add hitbox using enemy config dimensions
                Hitbox hitbox;
                hitbox.type = ColliderType::Circle; // Default to circle for now
                hitbox.radius = config.width * 0.4f; // Rough approximation based on sprite size
                m_ecsSystem->AddComponent<Hitbox>(enemy, hitbox);

                // Add StateAnimation if the config uses it
                GN_LOG_DEBUG("LevelManager: Config " + config.textureId + " useStateAnimation=" + std::to_string(config.useStateAnimation) + 
                            " animationStates.size=" + std::to_string(config.animationStates.size()));
                
                if (config.useStateAnimation) {
                    StateAnimation sa;
                    sa.currentState = config.initialState;

                    // Add all animation states from config
                    for (const auto& statePair : config.animationStates) {
                        const std::string& stateName = statePair.first;
                        const AnimationClip& clip = statePair.second;

                        StateAnimation::Clip saClip;
                        saClip.textureId = clip.textureId;
                        saClip.frameWidth = clip.frameWidth;
                        saClip.frameHeight = clip.frameHeight;
                        saClip.frameCount = clip.frameCount;
                        saClip.frameTime = clip.frameTime;
                        saClip.loop = clip.loop;

                        sa.clips.push_back({stateName, saClip});
                        GN_LOG_DEBUG("LevelManager: Added clip '" + stateName + "' -> '" + clip.textureId + "' with " + std::to_string(clip.frameCount) + " frames");
                    }

                    m_ecsSystem->AddComponent<StateAnimation>(enemy, sa);
                    GN_LOG_INFO("LevelManager: Added StateAnimation to pooled enemy " + config.textureId + " with " + std::to_string(sa.clips.size()) + " clips");
                } else {
                    GN_LOG_WARN("LevelManager: Config " + config.textureId + " has useStateAnimation=false, skipping StateAnimation component");
                }

                // Add to pool
                m_enemyPool.inactiveEnemies.push_back(enemy);
                enemyIndex++;
            }
        }
        
        // Boss level (level 6) spawning
        if (m_currentLevelId == 6) {
            // Find boss config
            const EnemyConfig* bossConfig = nullptr;
            for (const auto& config : m_currentLevelConfig.enemies) {
                if (config.movementPattern == "boss_idle" || 
                    config.textureId == "Ratking" || 
                    config.textureId == "RatKing") {
                    bossConfig = &config;
                    break;
                }
            }
            
            if (bossConfig) {
                // Get screen dimensions using ConfigManager (landscape mode for boss level)
                const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                
                // Spawn boss on the right side of the screen, GROUNDED
                // Position for 128x128 sprite at 8x scale (1024px total) in landscape
                // Shift left by 32 * scale (was too far left at 128, adjusting back)
                float bossX = (screenInfo.pixelWidth * 0.70f) - (32.0f * bossConfig->scale);
                
                // GROUND THE RAT KING: floor + his height up from bottom
                float ratKingHeight = 128.0f * bossConfig->scale; // 128px sprite * 8x scale = 1024px
                float bossY = screenInfo.pixelHeight - ratKingHeight; // Ground him!
                
                Gnosis::Entity bossEntity = SpawnBossEnemy(*bossConfig, bossX, bossY);
                GN_LOG_INFO("LevelManager: Spawned Rat King boss entity " + std::to_string(bossEntity) + 
                           " at (" + std::to_string(bossX) + ", " + std::to_string(bossY) + ")");
            } else {
                GN_LOG_ERROR("LevelManager: Boss level 6 has no boss config!");
            }
        } else {
            // CRITICAL: Shuffle enemy pool for variety (avoid always spawning same types)
            std::random_device rd;
            std::mt19937 rng(rd());

            if (isSnowLevel) {
                std::vector<Gnosis::Entity> throwerEntities;
                throwerEntities.reserve(m_enemyPool.inactiveEnemies.size());

                // Extract throwers so we can guarantee they spawn first
                for (auto it = m_enemyPool.inactiveEnemies.begin(); it != m_enemyPool.inactiveEnemies.end();) {
                    Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(*it);
                    bool isThrower = enemyComp && (enemyComp->enemyType == "SnowManIdle" || enemyComp->movementPattern == "snowman_thrower");
                    if (isThrower) {
                        throwerEntities.push_back(*it);
                        it = m_enemyPool.inactiveEnemies.erase(it);
                    } else {
                        ++it;
                    }
                }

                std::shuffle(m_enemyPool.inactiveEnemies.begin(), m_enemyPool.inactiveEnemies.end(), rng);

                // Append throwers to the end so pop_back() returns them first
                for (Gnosis::Entity thrower : throwerEntities) {
                    m_enemyPool.inactiveEnemies.push_back(thrower);
                }

                GN_LOG_INFO("LevelManager: Shuffled snow pool (" + std::to_string(m_enemyPool.inactiveEnemies.size()) +
                           ") and promoted " + std::to_string(throwerEntities.size()) + " throwers for guaranteed early spawns");
            } else {
                std::shuffle(m_enemyPool.inactiveEnemies.begin(), m_enemyPool.inactiveEnemies.end(), rng);
                GN_LOG_INFO("LevelManager: Shuffled enemy pool for variety");
            }
            
            // Spawn initial enemies based on level requirements (using pool for non-boss levels)
            SpawnInitialEnemies();
        }

        m_enemyPoolInitialized = true;
    }


    void LevelManager::InitializeNPCPool() {
        if (m_npcPoolInitialized) return;
        if (!m_isLoaded || !m_currentLevelConfig.enableNPCs) return;
        if (m_currentLevelId != 2) { m_npcPoolInitialized = true; return; }

        // Ensure single Janitor entity
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenW = screenInfo.pixelWidth, screenH = screenInfo.pixelHeight;
        // Clear any existing NPCs to enforce singleton Janitor
        for (Gnosis::Entity e : m_activeNPCs) {
            if (e != 0) { m_ecsSystem->DestroyEntity(e); }
        }
        m_activeNPCs.clear();

        float janitorHeight = 64.0f * m_currentLevelConfig.baseScale;
        float spawnX = screenW + 150.0f;
        // Align to sewer background pixel band: 20px * backgroundScale (Sewer backgrounds ~5x)
        float backgroundPixelScale = 5.0f; // same as AddSewerLevelLayers vertical scale
        float spawnY = screenH - janitorHeight - (20.0f * backgroundPixelScale);
        GN_LOG_INFO("NPC Janitor init: spawnX=" + std::to_string(spawnX) + ", spawnY=" + std::to_string(spawnY));
        m_janitorEntity = SpawnNPCJanitor(spawnX, spawnY);
        if (m_janitorEntity != 0) m_activeNPCs.push_back(m_janitorEntity);
        m_npcPoolInitialized = true;
    }

    // Pickups now handled in GameplayState; no pickup pool
    // void LevelManager::InitializePickupPool() {}

    void LevelManager::OnProjectileSystemReady() {
        GN_LOG_INFO("LevelManager notified that ProjectileSystem is ready");
        // ProjectileSystem handles all projectile pooling and management
        // LevelManager no longer needs to manage projectile pools directly
    }

    void LevelManager::UpdateEnemyPooling(float, float worldScrollDistance) {
        if (!m_enemyPoolInitialized) return;
        
        // CRITICAL FIX: Respawn enemies from inactive pool if active count is low
        // This handles enemies that were killed/returned to pool
        const int initialEnemyCount = m_currentLevelConfig.enemies.size() > 0 ? static_cast<int>(m_currentLevelConfig.enemies.size()) : 3;
        while (m_activeEnemies.size() < static_cast<size_t>(initialEnemyCount) && !m_enemyPool.inactiveEnemies.empty()) {
            // Get an inactive enemy from the pool
            Gnosis::Entity enemy = GetInactiveEnemy();
            if (!enemy) break;
            
            // Reactivate and reposition the enemy
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
            Transform* transform = m_ecsSystem->GetComponent<Transform>(enemy);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
            
            if (enemyComp && transform && sprite) {
                // Find rightmost active enemy position
                float rightmostX = ConfigManager::Instance().GetCurrentScreenInfo().pixelWidth;
                for (Gnosis::Entity e : m_activeEnemies) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    if (t && t->position.x > rightmostX) rightmostX = t->position.x;
                }
                
                // Position offscreen right with spacing
                transform->position.x = rightmostX + (m_enemySpacing * 1.25f);
                
                // Calculate Y position based on enemy type and level
                float baseY;
                if (m_currentLevelId == 2) { // Sewer - toilet paper
                    baseY = ConfigManager::Instance().GetCurrentScreenInfo().pixelHeight * 0.3125f;
                } else if (m_currentLevelId == 3) { // Desert - birds in top half
                    float minY = ConfigManager::Instance().GetCurrentScreenInfo().pixelHeight * 0.15f;
                    float maxY = ConfigManager::Instance().GetCurrentScreenInfo().pixelHeight * 0.45f;
                    baseY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                } else if (m_currentLevelId == 5 || m_currentLevelId == 6) { // Castle (5) or Boss (6) - RatCopters
                    // LANDSCAPE-AWARE: Boss level (6) is landscape, Castle level (5) is portrait
                    // Level 5 (Castle, Portrait): 30%-50% of 2556 = 766-1278 (middle band)
                    // Level 6 (Boss, Landscape): 50%-70% of 1179 = 589-825 (adjusted for lower height)
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float minY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.30f : 0.50f);
                    float maxY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.50f : 0.70f);
                    baseY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                    GN_LOG_INFO("[RAT_WRAP] RatCopter wrap L" + std::to_string(m_currentLevelId) + ": screenH=" + std::to_string(screenInfo.pixelHeight) + 
                               ", isPortrait=" + std::to_string(screenInfo.isPortrait) + 
                               ", Y=" + std::to_string(baseY) + " (" + std::to_string((baseY/screenInfo.pixelHeight)*100.0f) + "%)");
                } else if (enemyComp->isGrounded) { // Snowmen
                    float rawSpriteHeight = sprite->frameHeight;
                    float scaledSpriteHeight = sprite->height * std::abs(transform->scale.y);
                    baseY = ConfigManager::Instance().GetCurrentScreenInfo().pixelHeight - scaledSpriteHeight + rawSpriteHeight;
                } else {
                    baseY = 900.0f + static_cast<float>((rand()%300) - 150);
                }
                
                transform->position.y = baseY;
                enemyComp->baseY = baseY;
                enemyComp->hasInitializedBaseY = true;
                enemyComp->isActive = true;
                enemyComp->currentState = EnemyState::Idle;
                
                // Reset flying enemy state (RatCopters, Birds)
                if (enemyComp->movementPattern == "flying" || enemyComp->movementPattern == "horizontal") {
                    enemyComp->currentState = (enemyComp->movementPattern == "flying") ? EnemyState::FlyIn : EnemyState::Moving;
                    enemyComp->isGrounded = false; // CRITICAL: Ensure flying enemies don't get grounded
                    enemyComp->hoverTimer = 0.0f;
                    enemyComp->hasLockedDirection = false;
                    enemyComp->pullbackTimer = 0.0f;
                    enemyComp->targetDirection = Gnosis::GNVector2(0.0f, 0.0f);
                    enemyComp->pullbackVector = Gnosis::GNVector2(0.0f, 0.0f);
                    enemyComp->beelineSpeed = 0.0f;
                    
                    GN_LOG_INFO("[ENEMY_RESPAWN] Reset flying/horizontal enemy " + enemyComp->enemyType + " isGrounded=false, pattern=" + enemyComp->movementPattern);
                }
                
                // Make sprite visible and reset animation
                sprite->visible = true;
                sprite->color.a = 255;
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                sprite->hasCompleted = false;
                sprite->playing = true;
                
                // Reset to idle animation for StateAnimation enemies
                StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(enemy);
                if (sa && enemyComp) {
                    for (const auto& config : m_currentLevelConfig.enemies) {
                        if (config.textureId == enemyComp->enemyType && config.useStateAnimation) {
                            const StateAnimation::Clip* idleClip = sa->getClip(config.initialState);
                            if (idleClip) {
                                sprite->textureId = idleClip->textureId;
                                sprite->frameWidth = idleClip->frameWidth;
                                sprite->frameHeight = idleClip->frameHeight;
                                sprite->frameCount = idleClip->frameCount;
                                sprite->frameTime = idleClip->frameTime;
                                sprite->loop = idleClip->loop;
                            }
                            break;
                        }
                    }
                }
                
                // Add to active enemies
                m_activeEnemies.push_back(enemy);
                
                GN_LOG_INFO("[ENEMY_RESPAWN] Respawned " + enemyComp->enemyType + " from pool at X=" + std::to_string(transform->position.x) + ", Y=" + std::to_string(baseY));
            }
        }
        
        // Wrap enemies when off-screen left, reusing pool
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenW = screenInfo.pixelWidth;
        // Find rightmost enemy X
        float rightmostX = screenW;
        for (Gnosis::Entity e : m_activeEnemies) {
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            if (t && t->position.x > rightmostX) rightmostX = t->position.x;
        }
        for (Gnosis::Entity e : m_activeEnemies) {
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
            if (!t || !s) continue;
            float leftEdge = t->position.x;
            float widthPx = s->width * std::abs(t->scale.x);
            float rightEdge = leftEdge + widthPx;
            // Wrap when enemy goes off screen left (right edge goes negative)
            if (rightEdge < 0.0f) {
                t->position.x = rightmostX + (m_enemySpacing * 1.25f);
                
                // Get enemy component to check if it should be grounded
                Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(e);
                float baseY;
                
                // Check level FIRST before checking isGrounded to avoid applying ground offset to toilet paper
                if (m_currentLevelId == 2) { // Sewer level - toilet paper centered at 31.25% for 1/8 to 1/2 screen bobbing
                    // Center at 31.25% (5/16) of screen so amplitude of 18.75% reaches 1/8 min and 1/2 max
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    baseY = screenInfo.pixelHeight * 0.3125f;
                    GN_LOG_DEBUG("[WRAP_SEWER] Toilet paper wrapped to center Y=" + std::to_string(baseY) + " (screenH=" + std::to_string(screenInfo.pixelHeight) + ")");
                } else if (enemyComp && enemyComp->isGrounded) {
                    // CRITICAL: Use EXACT spawn formula - NO baseY manipulation!
                    // From initial spawn (line 1595-1602):
                    // y = screenInfo.pixelHeight - scaledSpriteHeight + rawSpriteHeight;
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    
                    float rawSpriteHeight = s->frameHeight;  // 64px
                    float scaledSpriteHeight = s->height * std::abs(t->scale.y);  // 384px for snowmen
                    
                    // EXACT SPAWN FORMULA - do NOT change!
                    baseY = screenInfo.pixelHeight - scaledSpriteHeight + rawSpriteHeight;
                    
                    GN_LOG_INFO("[WRAP_GROUND] Snowman " + std::to_string(e) + 
                               " using SPAWN FORMULA: Y=" + std::to_string(baseY) + 
                               " (screenH=" + std::to_string(screenInfo.pixelHeight) + 
                               " - scaledH=" + std::to_string(scaledSpriteHeight) + 
                               " + rawH=" + std::to_string(rawSpriteHeight) + ")");
                } else if (m_currentLevelId == 3) { // Desert level - birds only in top half
                    // Random Y within top half of screen (15% to 45% range) for birds
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float minY = screenInfo.pixelHeight * 0.15f;
                    float maxY = screenInfo.pixelHeight * 0.45f;
                    baseY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                } else if (m_currentLevelId == 5 || m_currentLevelId == 6) { // Castle (5) or Boss (6) - RatCopters
                    // LANDSCAPE-AWARE: Boss level (6) is landscape, Castle level (5) is portrait
                    // Level 5 (Castle, Portrait): 30%-50% of 2556 = 766-1278 (middle band)
                    // Level 6 (Boss, Landscape): 50%-70% of 1179 = 589-825 (adjusted for lower height)
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float minY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.30f : 0.50f);
                    float maxY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.50f : 0.70f);
                    baseY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                    GN_LOG_INFO("[RAT_WRAP_FULL] RatCopter " + std::to_string(e) + " wrapped L" + std::to_string(m_currentLevelId) + ": screenH=" + 
                               std::to_string(screenInfo.pixelHeight) + ", isPortrait=" + 
                               std::to_string(screenInfo.isPortrait) + ", Y=" + std::to_string(baseY) + 
                               " (" + std::to_string((baseY/screenInfo.pixelHeight)*100.0f) + "%)");
                } else { // Other levels - original logic for non-grounded enemies
                    baseY = 900.0f + static_cast<float>((rand()%300) - 150);
                }
                
                t->position.y = baseY;
                // Sync Enemy component's bobbing anchor with new wrap position
                if (enemyComp) {
                    enemyComp->baseY = baseY;
                    enemyComp->hasInitializedBaseY = true;
                    
                    // Randomize bobPhase on wrap for maximum variation
                    if (enemyComp->bobbingEnabled) {
                        enemyComp->bobPhase = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.28318530718f;
                    }
                    
                    // CRITICAL: Reset throw state when wrapping so snowmen can throw again
                    if (enemyComp->isThrower) {
                        enemyComp->hasThrownOnScreenEntry = false; // Allow throwing again
                        enemyComp->isThrowing = false;
                        enemyComp->hasSpawnedProjectile = false;
                        enemyComp->throwAnimationTimer = 0.0f;
                        enemyComp->currentThrowFrame = 0;
                        enemyComp->isOnScreen = false;
                        enemyComp->currentState = EnemyState::Idle;
                        
                        // Reset flip state to default (facing left)
                        t->scale.x = std::abs(t->scale.x);
                        enemyComp->isFacingRight = false;
                        
                        // CRITICAL: Ensure sprite is visible after wrap
                        s->visible = true;
                        s->color.a = 255; // Full opacity
                        
                        GN_LOG_INFO("[ENEMY_WRAP] Reset throw state for snowman " + std::to_string(e) + 
                                   ", scale.x=" + std::to_string(t->scale.x) + 
                                   ", visible=" + std::to_string(s->visible));
                        
                        // Reset to idle animation
                        StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(e);
                        if (sa) {
                            sa->currentState = "idle";
                            const StateAnimation::Clip* idleClip = sa->getClip("idle");
                            if (idleClip) {
                                s->textureId = idleClip->textureId;
                                s->frameWidth = idleClip->frameWidth;
                                s->frameHeight = idleClip->frameHeight;
                                s->frameCount = idleClip->frameCount;
                                s->frameTime = idleClip->frameTime;
                                s->loop = idleClip->loop;
                                s->isAnimated = (idleClip->frameCount > 1);
                                s->playing = true;
                                s->currentFrame = 0;
                                s->currentFrameTime = 0.0f;
                            }
                        }
                    }
                    
                    // CRITICAL: Reset flying enemy state when wrapping (RatCopters, Birds)
                    if (enemyComp->movementPattern == "flying" || enemyComp->movementPattern == "horizontal") {
                        enemyComp->currentState = (enemyComp->movementPattern == "flying") ? EnemyState::FlyIn : EnemyState::Moving;
                        enemyComp->isGrounded = false; // CRITICAL: Keep flying/horizontal enemies airborne
                        enemyComp->hoverTimer = 0.0f;
                        enemyComp->hasLockedDirection = false;
                        enemyComp->pullbackTimer = 0.0f;
                        enemyComp->targetDirection = Gnosis::GNVector2(0.0f, 0.0f);
                        enemyComp->pullbackVector = Gnosis::GNVector2(0.0f, 0.0f);
                        enemyComp->baseY = baseY;  // CRITICAL: Update baseY to new wrap position
                        enemyComp->beelineSpeed = 0.0f;  // Reset beeline speed
                        
                        // Reset sprite animation to idle
                        s->currentFrame = 0;
                        s->currentFrameTime = 0.0f;
                        s->hasCompleted = false;
                        s->playing = true;
                        
                        GN_LOG_INFO("[FLYING_ENEMY_WRAP] " + enemyComp->enemyType + " wrapped: isGrounded=false, pattern=" + enemyComp->movementPattern + ", pos=(" + std::to_string(t->position.x) + "," + std::to_string(baseY) + ")");
                    }
                }
                m_enemyBaseY[e] = baseY;
                rightmostX = t->position.x;
                GN_LOG_DEBUG("Enemy wrap: newX=" + std::to_string(t->position.x) + ", baseY=" + std::to_string(baseY) + ", level=" + std::to_string(m_currentLevelId));
            }
            // Y behavior moved to EnemySystem; LevelManager now only wraps enemies
        }
    }

    void LevelManager::UpdateNPCPooling(float deltaTime, float) {
        if (!m_npcPoolInitialized || m_janitorEntity == 0) return;
        // If Janitor goes off-screen left, move him to the right again at ground Y
        Transform* t = m_ecsSystem->GetComponent<Transform>(m_janitorEntity);
        Sprite* s = m_ecsSystem->GetComponent<Sprite>(m_janitorEntity);
        if (!t || !s) return;

        // Continuously match Janitor speed to the actual sewer background speed
        // Find the real sewer background speed dynamically
        float targetSpeed = 0.0f;
        for (const auto& layer : m_currentLevelConfig.backgroundLayers) {
            if (layer.textureId.find("Sewer") != std::string::npos) {
                targetSpeed = layer.scrollSpeed;
                break;
            }
        }
        // Fallback if no sewer layer found
        if (targetSpeed == 0.0f) {
            targetSpeed = m_currentLevelConfig.worldSpeed * 0.28f;
        }
        if (!m_ecsSystem->HasComponent<ScrollSpeed>(m_janitorEntity)) {
            m_ecsSystem->AddComponent<ScrollSpeed>(m_janitorEntity, ScrollSpeed(targetSpeed));
        } else {
            auto scr = m_ecsSystem->GetComponent<ScrollSpeed>(m_janitorEntity);
            scr->speed = targetSpeed;
        }
        // Remove direct per-frame velocity movement to avoid double scroll; CameraSystem now moves him.
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenW = screenInfo.pixelWidth, screenH = screenInfo.pixelHeight;
        float rightEdge = t->position.x + s->width * std::abs(t->scale.x);
        if (rightEdge < 0.0f) {
            float janitorHeight = 64.0f * m_currentLevelConfig.baseScale;
            t->position.x = screenW + 220.0f;
            // Keep Y aligned to sewer background band: 20 px at background pixel scale (~5x)
            float offsetPx = 20.0f * 5.0f;
            t->position.y = screenH - janitorHeight - offsetPx;
            GN_LOG_INFO("NPC Janitor wrap: newX=" + std::to_string(t->position.x) + ", newY=" + std::to_string(t->position.y));
            // Re-apply ScrollSpeed on wrap to match actual sewer background speed
            // Find the real sewer background speed dynamically
            float targetSpeed = 0.0f;
            for (const auto& layer : m_currentLevelConfig.backgroundLayers) {
                if (layer.textureId.find("Sewer") != std::string::npos) {
                    targetSpeed = layer.scrollSpeed;
                    break;
                }
            }
            // Fallback if no sewer layer found
            if (targetSpeed == 0.0f) {
                targetSpeed = m_currentLevelConfig.worldSpeed * 0.28f;
            }
            if (!m_ecsSystem->HasComponent<ScrollSpeed>(m_janitorEntity)) {
                m_ecsSystem->AddComponent<ScrollSpeed>(m_janitorEntity, ScrollSpeed(targetSpeed));
            } else {
                auto scr = m_ecsSystem->GetComponent<ScrollSpeed>(m_janitorEntity);
                scr->speed = targetSpeed;
            }
            // Reset NPC state on wrap and re-apply state clip
            NPC* npc = m_ecsSystem->GetComponent<NPC>(m_janitorEntity);
            if (npc) { npc->state = 0; npc->triggered = false; npc->timer = 0.0f; }
            StateAnimation* sa2 = m_ecsSystem->GetComponent<StateAnimation>(m_janitorEntity);
            Sprite* spr = m_ecsSystem->GetComponent<Sprite>(m_janitorEntity);
            if (sa2 && spr) {
                const StateAnimation::Clip* clip = sa2->getClip("sweep");
                if (clip) {
                    spr->textureId = clip->textureId;
                    spr->isAnimated = (clip->frameCount > 1);
                    spr->frameWidth = clip->frameWidth;
                    spr->frameHeight = clip->frameHeight;
                    spr->frameCount = clip->frameCount;
                    spr->frameTime = clip->frameTime;
                    spr->loop = clip->loop;
                    spr->currentFrame = 0;
                    spr->playing = true;
                }
                sa2->currentState = "sweep";
            }
        }
    }

    Gnosis::Entity LevelManager::SpawnNPCJanitor(float x, float y) {
        if (!m_ecsSystem) return 0;
        Gnosis::Entity npc = m_ecsSystem->CreateEntity();
        Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        // Start in sweeping loop; swap to surprise when player passes (state machine handled elsewhere)
        Sprite sp("JanitorSweep", 64.0f, 64.0f);
        sp.layer = 2; // Above background (layer 1) but behind sewer pipes (layers 3-5)
        sp.visible = true;
        sp.isAnimated = true;
        sp.frameWidth = 64; sp.frameHeight = 64;
        sp.frameCount = 4;
        sp.frameTime = 0.3f; // slower sweep from the start (300ms)
        sp.currentFrame = 0;
        sp.playing = true;
        // Attach ScrollSpeed so CameraSystem moves the Janitor with the sewer background speed
        // Find the actual sewer background speed (not assuming 0.28f multiplier)
        float janitorSpeed = 0.0f;
        for (const auto& layer : m_currentLevelConfig.backgroundLayers) {
            if (layer.textureId.find("Sewer") != std::string::npos) {
                janitorSpeed = layer.scrollSpeed;
                break; // Found sewer layer, use its speed
            }
        }
        // Fallback if no sewer layer found
        if (janitorSpeed == 0.0f) {
            janitorSpeed = m_currentLevelConfig.worldSpeed * 0.28f; // Original fallback
        }
        Physics ph; ph.velocity.x = 0.0f; ph.useGravity = false;
        Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = 48.0f; hb.height = 48.0f; hb.isTrigger = true; hb.tag = "NPC";
        NPC npcComp; npcComp.type = "Janitor"; npcComp.state = 0; npcComp.timer = 0.0f; npcComp.triggered = false;

        m_ecsSystem->AddComponent<Transform>(npc, tr);
        m_ecsSystem->AddComponent<Sprite>(npc, sp);
        // Attach declarative animation set for state-driven control
        StateAnimation sa;
        {
            StateAnimation::Clip sweep; sweep.textureId = "JanitorSweep"; sweep.frameWidth = 64; sweep.frameHeight = 64; sweep.frameCount = 4; sweep.frameTime = 0.3f; sweep.loop = true;
            StateAnimation::Clip surprise; surprise.textureId = "JanitorSurprise"; surprise.frameWidth = 64; surprise.frameHeight = 64; surprise.frameCount = 8; surprise.frameTime = 0.12f; surprise.loop = false;
            sa.clips.push_back({"sweep", sweep});
            sa.clips.push_back({"surprise", surprise});
            sa.currentState = "sweep";
        }
        m_ecsSystem->AddComponent<StateAnimation>(npc, sa);
        m_ecsSystem->AddComponent<Physics>(npc, ph);
        m_ecsSystem->AddComponent<Hitbox>(npc, hb);
        m_ecsSystem->AddComponent<NPC>(npc, npcComp);
        // Add ScrollSpeed component for CameraSystem to move the Janitor
        m_ecsSystem->AddComponent<ScrollSpeed>(npc, ScrollSpeed(janitorSpeed));
        m_activeNPCs.push_back(npc);
        return npc;
    }

    // REMOVED: UpdatePickupSpawning - replaced with GameplayState coordination

    // REMOVED: SpawnObstacle - now handled by ObstacleSystem
    
    // ============================================================================
    // Legacy Obstacle Functions - All Removed and Handled by ObstacleSystem
    // ============================================================================
    // REMOVED: SpawnToiletPair - now handled by ObstacleSystem
    // REMOVED: SpawnToiletPairWithGap - now handled by ObstacleSystem  
    // REMOVED: SpawnOuthouse - now handled by ObstacleSystem
    // REMOVED: SpawnSingleObstacle - now handled by ObstacleSystem

    // REMOVED: RemoveObstacle - now handled by ObstacleSystem

    Gnosis::Entity LevelManager::SpawnEnemy(const EnemyConfig& config, float x, float y) {
        if (!m_ecsSystem) {
            return 0;
        }

        // Ensure enemy config registry is initialized
        EnemyConfigRegistry::Initialize();

        // Get enhanced configuration from registry
        GN_LOG_DEBUG("LevelManager: Getting enhanced config for enemy: " + config.textureId);
        const EnemyConfig& enhancedConfig = EnemyConfigRegistry::GetConfig(config.textureId);
        GN_LOG_DEBUG("LevelManager: Got enhanced config: " + enhancedConfig.textureId + " (empty: " + (enhancedConfig.textureId.empty() ? "true" : "false") + ")");
        
        if (enhancedConfig.textureId.empty()) {
            GN_LOG_ERROR("No enhanced configuration found for enemy: " + config.textureId);
            return 0;
        }

        GN_LOG_INFO("Spawning enemy with texture: " + enhancedConfig.textureId);
        
        Gnosis::Entity enemy = m_ecsSystem->CreateEntity();
        
        // Create transform using enemy-specific scale
        Transform transform(Gnosis::GNVector2(x, y), 0.0f,
                          Gnosis::GNVector2(enhancedConfig.scale, enhancedConfig.scale));
        
        // Create sprite using enhanced configuration
        Sprite sprite(enhancedConfig.textureId, enhancedConfig.width, enhancedConfig.height);
        sprite.layer = enhancedConfig.renderLayer;
        sprite.visible = true;
        
        // Configure animation based on enhanced config
        sprite.isAnimated = enhancedConfig.isAnimated;
        sprite.frameWidth = enhancedConfig.frameWidth;
        sprite.frameHeight = enhancedConfig.frameHeight;
        sprite.frameCount = enhancedConfig.frameCount;
        sprite.frameTime = enhancedConfig.frameTime;
        sprite.currentFrame = 0;
        sprite.currentFrameTime = 0.0f;
        sprite.playing = enhancedConfig.isAnimated;
        sprite.loop = enhancedConfig.loopAnimation;
        sprite.hasCompleted = false;
        
        // Create physics
        Physics physics;
        physics.velocity.x = -enhancedConfig.speed; // Move left with world
        
        // Create hitbox - use CIRCLE for better precision and smaller hit area
        Hitbox collider;
        collider.type = ColliderType::Circle;
        collider.isStatic = false;
        // Use 40% of the smaller dimension for a tight, fair hitbox
        float smallerDim = std::min(enhancedConfig.width, enhancedConfig.height);
        collider.radius = smallerDim * 0.4f; // Tighter hitbox (40% of smaller dimension)
        collider.offsetX = 0.0f; // Centered
        collider.offsetY = 0.0f; // Centered
        collider.tag = "Enemy";
        
        // Create enemy component with bobbing behavior from config
        Enemy enemyComp;
        enemyComp.health = enhancedConfig.hitPoints;
        enemyComp.enemyType = enhancedConfig.textureId;
        enemyComp.movementPattern = enhancedConfig.movementPattern;
        enemyComp.isActive = true;
        
        // Configure bobbing behavior from enhanced config
        const BobbingConfig& bobConfig = enhancedConfig.bobbingConfig;
        if (bobConfig.enabled || bobConfig.chanceToHover > 0.0f) {
            bool enableBobbing = bobConfig.enabled;
            
            // Handle probabilistic hovering (like birds)
            if (!enableBobbing && bobConfig.chanceToHover > 0.0f) {
                enableBobbing = (rand() % 100) < static_cast<int>(bobConfig.chanceToHover);
            }
            
            if (enableBobbing) {
                enemyComp.bobbingEnabled = true;
                
                // Calculate speed with jitter
                float speedJitter = bobConfig.speedJitter * (static_cast<float>((rand() % 41) - 20)); // -20 to +20 range
                enemyComp.bobSpeed = std::max(0.8f, bobConfig.baseSpeed + speedJitter);
                
                // Calculate amplitude
                if (bobConfig.amplitudeMin > 0.0f && bobConfig.amplitudeMax > 0.0f) {
                    // Range-based amplitude (like birds: 15-30px)
                    float range = bobConfig.amplitudeMax - bobConfig.amplitudeMin;
                    enemyComp.bobAmplitude = bobConfig.amplitudeMin + (static_cast<float>(rand()) / RAND_MAX) * range;
                } else if (bobConfig.amplitudeMin < 1.0f && bobConfig.amplitudeMax < 1.0f) {
                    // Percentage-based amplitude (like toilet paper)
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float screenH = screenInfo.pixelHeight;
                    float ampFactor = bobConfig.amplitudeMin + (static_cast<float>((rand() % 21) - 10) * 0.005f);
                    enemyComp.bobAmplitude = screenH * ampFactor;
                } else {
                    enemyComp.bobAmplitude = bobConfig.amplitude;
                }
                
                // Random starting phase 0..2π with better distribution
                // Use full float range for maximum variation
                enemyComp.bobPhase = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.28318530718f;
                
                GN_LOG_DEBUG("LevelManager: Enabled bobbing for " + enhancedConfig.textureId + 
                           " amplitude=" + std::to_string(enemyComp.bobAmplitude) + "px");
            } else {
                enemyComp.bobbingEnabled = false;
                GN_LOG_DEBUG("LevelManager: Static " + enhancedConfig.textureId + " (no bobbing)");
            }
        }
        
        // Add components
        m_ecsSystem->AddComponent<Transform>(enemy, transform);
        m_ecsSystem->AddComponent<Sprite>(enemy, sprite);
        m_ecsSystem->AddComponent<Physics>(enemy, physics);
        m_ecsSystem->AddComponent<Hitbox>(enemy, collider);
        m_ecsSystem->AddComponent<Enemy>(enemy, enemyComp);
        
        // Add StateAnimation for enemies that use it
        if (enhancedConfig.useStateAnimation) {
            StateAnimation sa;
            sa.currentState = enhancedConfig.initialState;
            
            // Add all animation states from enhanced config
            for (const auto& statePair : enhancedConfig.animationStates) {
                const std::string& stateName = statePair.first;
                const AnimationClip& clip = statePair.second;
                
                StateAnimation::Clip saClip;
                saClip.textureId = clip.textureId;
                saClip.frameWidth = clip.frameWidth;
                saClip.frameHeight = clip.frameHeight;
                saClip.frameCount = clip.frameCount;
                saClip.frameTime = clip.frameTime;
                saClip.loop = clip.loop;
                
                sa.clips.push_back({stateName, saClip});
            }
            
            m_ecsSystem->AddComponent<StateAnimation>(enemy, sa);
            GN_LOG_DEBUG("LevelManager: Added StateAnimation to " + enhancedConfig.textureId + 
                        " with " + std::to_string(enhancedConfig.animationStates.size()) + " states");
        }
        
        // Track active enemy
        m_activeEnemies.push_back(enemy);
        
        GN_LOG_DEBUG("Spawned enemy: " + enhancedConfig.textureId + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        
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

    // Legacy pickup spawn removed; GameplayState creates coin entities directly

    // Legacy RemovePickup removed

    void LevelManager::CleanupOffscreenEntities(float leftBoundary) {
        // REMOVED: Legacy obstacle cleanup - now handled by ObstacleSystem
        
        // Clean up enemies that have moved off screen or are inactive
        // FIXED: Use right edge of entity for proper cleanup, like toilet logic
        for (auto it = m_activeEnemies.begin(); it != m_activeEnemies.end();) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(*it);
            Enemy* enemy = m_ecsSystem->GetComponent<Enemy>(*it);

            bool shouldRemove = false;

            // Check if enemy is inactive (defeated)
            if (enemy && !enemy->isActive) {
                shouldRemove = true;
                GN_LOG_DEBUG("LevelManager: Removing inactive enemy " + std::to_string(*it));
            }
            // Check if enemy has moved off screen
            else if (transform && sprite) {
                // Calculate right edge of enemy for proper cleanup
                float scaledWidth = sprite->width * std::abs(transform->scale.x);
                float rightEdge = transform->position.x + scaledWidth;
                if (rightEdge < leftBoundary) {
                    shouldRemove = true;
                    GN_LOG_DEBUG("LevelManager: Removing offscreen enemy " + std::to_string(*it) + " (rightEdge=" + std::to_string(rightEdge) + " < leftBoundary=" + std::to_string(leftBoundary) + ")");
                }
            }

            if (shouldRemove) {
                    m_ecsSystem->DestroyEntity(*it);
                    it = m_activeEnemies.erase(it);
            } else {
                ++it;
            }
        }
        
        // Pickups are managed by GameplayState; no cleanup here

        // Clean up NPCs
        for (auto it = m_activeNPCs.begin(); it != m_activeNPCs.end();) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(*it);
            if (transform && sprite) {
                float scaledWidth = sprite->width * std::abs(transform->scale.x);
                float rightEdge = transform->position.x + scaledWidth;
                if (rightEdge < leftBoundary) {
                    m_ecsSystem->DestroyEntity(*it);
                    it = m_activeNPCs.erase(it);
                } else {
                    ++it;
                }
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

        // DEBUG: Unlock all levels for testing
        for (int i = 0; i < maxLevels; ++i) {
            m_unlockedLevels[i] = true;
        }
        
        LoadProgression();
        
        GN_LOG_INFO("Progression system initialized for " + std::to_string(maxLevels) + " levels");
    }

    void LevelManager::CreateBackgroundLayers() {
        GN_LOG_INFO("🎨 CREATING BACKGROUND LAYERS WITH METADATA-DRIVEN SYSTEM");

        if (!m_ecsSystem) {
            GN_LOG_ERROR("❌ ECS system not available for background creation");
            return;
        }

        DestroyBackgroundLayers(); // Clean up any existing layers

        for (const auto& layerConfig : m_currentLevelConfig.backgroundLayers) {
            GN_LOG_INFO("📋 Processing layer: '" + layerConfig.textureId + "' (level: " + std::to_string(m_currentLevelId) + ")");

            // 🎯 STEP 1: Get actual texture dimensions from metadata system
            int textureWidth, textureHeight;
            if (!GetTextureDimensions(layerConfig.textureId, textureWidth, textureHeight)) {
                // 🚨 REAL FAILURE: Skip this layer entirely rather than guess
                GN_LOG_ERROR("💥 SKIPPING background layer due to metadata failure: " + layerConfig.textureId);
                GN_LOG_ERROR("💥 This indicates a critical asset management issue");
                continue; // Skip this layer - don't create broken backgrounds
            }

            GN_LOG_INFO("📐 Texture dimensions: " + std::to_string(textureWidth) + "x" + std::to_string(textureHeight));

            // 🎯 STEP 2: Calculate scaling using dynamic screen dimensions
            float scale;
            float effectiveScreenWidth, effectiveScreenHeight;

            // Get actual screen dimensions from render system
            if (m_renderSystem) {
                const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
                effectiveScreenWidth = screenInfo.pixelWidth;
                effectiveScreenHeight = screenInfo.pixelHeight;

                // Check if we're actually in landscape mode (width > height)
                bool isActuallyLandscape = (effectiveScreenWidth > effectiveScreenHeight);

                GN_LOG_INFO("📱 Screen dimensions: " + std::to_string((int)effectiveScreenWidth) + "x" + std::to_string((int)effectiveScreenHeight) +
                           " (orientation: " + (isActuallyLandscape ? "landscape" : "portrait") + ")");

                if ((m_currentLevelConfig.forceLandscape && isActuallyLandscape) || isActuallyLandscape) {
                    // Landscape mode with width priority scaling
                    scale = effectiveScreenWidth / static_cast<float>(textureWidth);
                    GN_LOG_INFO("🌅 Landscape scaling: screen=" + std::to_string(effectiveScreenWidth) +
                               " texture=" + std::to_string(textureWidth) + " scale=" + std::to_string(scale));
                } else {
                    // Standard portrait mode scaling
                    scale = effectiveScreenHeight / static_cast<float>(textureHeight);
                    GN_LOG_INFO("📱 Portrait scaling: screen=" + std::to_string(effectiveScreenHeight) +
                               " texture=" + std::to_string(textureHeight) + " scale=" + std::to_string(scale));
                }
            } else {
                // Fallback to hardcoded dimensions if render system not available
                GN_LOG_WARN("⚠️ Render system not available, using fallback dimensions");
                effectiveScreenWidth = 1179.0f;
                effectiveScreenHeight = 2556.0f;

                if (m_currentLevelConfig.forceLandscape && m_currentLevelConfig.widthPriorityScaling) {
                    scale = effectiveScreenWidth / m_currentLevelConfig.landscapeWidth;
                } else {
                    scale = effectiveScreenHeight / static_cast<float>(textureHeight);
                }
            }
            scale = std::round(scale * 100.0f) / 100.0f; // Pixel-perfect rounding

            float scaledWidth = static_cast<float>(textureWidth) * scale;
            float scaledHeight = static_cast<float>(textureHeight) * scale;

            GN_LOG_INFO("🔢 Scaling: screen=" + std::to_string(effectiveScreenWidth) + "x" + std::to_string(effectiveScreenHeight) +
                       " texture=" + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) +
                       " scale=" + std::to_string(scale) +
                       " result=" + std::to_string(scaledWidth) + "x" + std::to_string(scaledHeight));

            // 🎯 STEP 3: Calculate optimal instance count using the screen dimensions we already determined
            // Use the effectiveScreenWidth and effectiveScreenHeight from above

            // Calculate exactly how many instances needed for seamless wrapping
            // Add 2 extra instances for safety margin to prevent gaps during movement
            int instancesNeeded = static_cast<int>(std::ceil((effectiveScreenWidth * 2.0f) / scaledWidth));
            instancesNeeded = std::max(instancesNeeded, 5); // Minimum for seamless wrapping

            GN_LOG_INFO("🔄 Instances: screen=" + std::to_string(effectiveScreenWidth) +
                       " scaledWidth=" + std::to_string(scaledWidth) +
                       " needed=" + std::to_string(instancesNeeded));

            // 🎯 STEP 4: Create instances with PURE INTEGER positioning (no floating point errors)
            // Apply segment gap if configured (for castle curtains, etc.)
            int gapInt = static_cast<int>(std::round(layerConfig.segmentGap));
            
            for (int i = 0; i < instancesNeeded; ++i) {
                // Use integer arithmetic to prevent floating point precision errors
                // Convert scaledWidth to integer for pixel-perfect positioning
                int scaledWidthInt = static_cast<int>(std::round(scaledWidth));
                // Apply segment gap: each segment is positioned at (width + gap) * index
                int xPosInt = i * (scaledWidthInt + gapInt);
                float xPos = static_cast<float>(xPosInt);
                float yPos = 0.0f;

                GN_LOG_INFO("📍 Instance " + std::to_string(i) + ": x=" + std::to_string(xPos) +
                           " (int:" + std::to_string(xPosInt) + ", scaledWidthInt:" + std::to_string(scaledWidthInt) + ")");

                // Create entity with calculated position
                Gnosis::Entity bgEntity = m_ecsSystem->CreateEntity();
                if (bgEntity == 0) {
                    GN_LOG_ERROR("❌ Failed to create background entity");
                    continue;
                }

                // Special positioning for boss floor and screen curtains
                float finalXPos = xPos;
                float finalYPos = yPos;

                // Apply precise Y offset for boss floor to ensure visibility on screen
                if (layerConfig.textureId == "BossFloor.png") {
                    // Calculate offset to ensure floor is visible on screen
                    // Boss floor is 320x180 scaled like other backgrounds, but needs to be positioned properly

                    // Get screen dimensions for proper positioning
                    float screenHeight = effectiveScreenHeight;

                    // Position floor so it's visible at the bottom of the screen
                    // Subtract offset from screen height to move it up from the very bottom
                    float floorOffset = screenHeight * 0.15f + 300.0f; // 15% from bottom + 300px additional offset

                    finalYPos = screenHeight - floorOffset;

                    GN_LOG_INFO("Boss floor positioned for screen visibility: 15% offset + 300px = " +
                               std::to_string(floorOffset) + "px total, finalY=" + std::to_string(finalYPos) +
                               ", screenHeight=" + std::to_string(screenHeight));
                }

                if (layerConfig.textureId == "screenCurtains.png" && effectiveScreenWidth > effectiveScreenHeight) {
                    // In landscape mode, position screen curtains on left and right sides
                    if (i == 0) {
                        // Left curtain
                        finalXPos = -scaledWidth * 0.25f; // Position left curtain off-screen to the left
                    } else if (i == instancesNeeded - 1) {
                        // Right curtain
                        finalXPos = effectiveScreenWidth - scaledWidth * 0.75f; // Position right curtain off-screen to the right
                    } else {
                        // Skip middle instances for curtains
                        m_ecsSystem->DestroyEntity(bgEntity);
                        continue;
                    }
                    GN_LOG_INFO("🎭 Positioning screen curtain at: (" + std::to_string(finalXPos) + ", " + std::to_string(finalYPos) + ") for landscape mode");
                }

                // 🎯 PIXEL-PERFECT SCALE: Adjust scale so rendered width EXACTLY matches integer positioning
                // This prevents sub-pixel gaps/overlaps between segments
                float pixelPerfectScale = static_cast<float>(scaledWidthInt) / static_cast<float>(textureWidth);
                
                // Transform: Position and scale
                Transform transform(Gnosis::GNVector2(finalXPos, finalYPos), 0.0f, Gnosis::GNVector2(pixelPerfectScale, pixelPerfectScale));
                m_ecsSystem->AddComponent<Transform>(bgEntity, transform);

                // Sprite: Use actual texture dimensions
                Sprite sprite(layerConfig.textureId, static_cast<float>(textureWidth), static_cast<float>(textureHeight));
                sprite.layer = layerConfig.renderLayer;
                sprite.visible = true;
                m_ecsSystem->AddComponent<Sprite>(bgEntity, sprite);

                // Parallax: Use integer scaled width for pixel-perfect wrapping
                // Skip parallax for boss level (level 6) to keep background static
                if (m_currentLevelId != 6) {
                    Parallax parallax;
                    parallax.scrollSpeed = layerConfig.scrollSpeed;
                    parallax.repeatWidth = static_cast<float>(scaledWidthInt); // Integer-based for precision
                    parallax.autoScroll = true;
                    parallax.segmentGap = layerConfig.segmentGap; // Apply gap from config
                    m_ecsSystem->AddComponent<Parallax>(bgEntity, parallax);
                    GN_LOG_INFO("Added Parallax component to background entity for level " + std::to_string(m_currentLevelId) +
                               " with segmentGap=" + std::to_string(parallax.segmentGap));
                } else {
                    GN_LOG_INFO("Skipped Parallax component for boss level (static background) - level " + std::to_string(m_currentLevelId));
                }

                // ParallaxInstance: Track position in layer with integer dimensions
                ParallaxInstance instance(layerConfig.textureId, i, instancesNeeded, static_cast<float>(scaledWidthInt));
                m_ecsSystem->AddComponent<ParallaxInstance>(bgEntity, instance);

                // Variants: If this layer has texture variations
                if (!layerConfig.variantTextureIds.empty()) {
                    ParallaxVariants variants(layerConfig.variantTextureIds);
                    m_ecsSystem->AddComponent<ParallaxVariants>(bgEntity, variants);
                }

                m_backgroundEntities.push_back(bgEntity);                GN_LOG_INFO("✅ Created background entity " + std::to_string(bgEntity) +
                           " at (" + std::to_string(xPos) + ", " + std::to_string(yPos) + ")");
            }
        }

        GN_LOG_INFO("🎉 Background creation complete - " + std::to_string(m_backgroundEntities.size()) + " entities created");
    }

    // 🎯 NEW: Robust texture dimension retrieval with cache-first approach
    bool LevelManager::GetTextureDimensions(const std::string& textureId, int& width, int& height) {
        // 🎯 FIRST: Try synchronous cache (fastest, most reliable)
        if (m_renderSystem && m_renderSystem->GetCachedTextureInfo(textureId, width, height)) {
            return true; // ✅ Cache hit - instant success
        }

        // 🎯 SECOND: Try delegates as fallback (for non-preloaded textures)
        TextureMetadata meta;

        // Try renderer delegate first (most direct)
        if (m_platformDelegates.renderer.getTextureMetadata &&
            m_platformDelegates.renderer.getTextureMetadata(textureId.c_str(), &meta) &&
            meta.width > 0 && meta.height > 0) {
            width = meta.width;
            height = meta.height;
            GN_LOG_INFO("📊 Texture metadata (renderer fallback): " + textureId + " = " +
                       std::to_string(width) + "x" + std::to_string(height));
            return true;
        }

        // Try asset delegate (last resort)
        if (m_platformDelegates.asset.getTextureMetadata &&
            m_platformDelegates.asset.getTextureMetadata(textureId.c_str(), &meta) &&
            meta.width > 0 && meta.height > 0) {
            width = meta.width;
            height = meta.height;
            GN_LOG_INFO("📊 Texture metadata (asset fallback): " + textureId + " = " +
                       std::to_string(width) + "x" + std::to_string(height));
            return true;
        }

        // 🚨 FAILURE: No metadata available - this is a real problem
        GN_LOG_ERROR("❌ CRITICAL: No texture metadata available for: " + textureId);
        GN_LOG_ERROR("💥 This texture should have been preloaded in LoadingState!");
        GN_LOG_ERROR("💥 Cache miss - RenderSystem: " + std::string(m_renderSystem ? "AVAILABLE" : "NULL"));
        std::string rendererStatus = m_platformDelegates.renderer.getTextureMetadata ? "YES" : "NO";
        std::string assetStatus = m_platformDelegates.asset.getTextureMetadata ? "YES" : "NO";
        std::string delegateStatus = "💥 Delegates available - Renderer: " + rendererStatus +
                                    ", Asset: " + assetStatus;
        GN_LOG_ERROR(delegateStatus);

        width = height = 0;
        return false;
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
        
        // REMOVED: Legacy obstacle cleanup - now handled by ObstacleSystem
        
        // Destroy all active enemies
        for (Gnosis::Entity entity : m_activeEnemies) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_activeEnemies.clear();
        
        // Pickups managed by GameplayState

        // Destroy all active NPCs (including Janitor) and reset handle
        for (Gnosis::Entity entity : m_activeNPCs) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_activeNPCs.clear();
        m_janitorEntity = 0;
    }

    // REMOVED: CalculateNextObstaclePosition - now handled by ObstacleSystem

    float LevelManager::CalculateNextEnemyPosition() {
        // Space enemies differently than obstacles
        float baseSpacing = 600.0f;
        float spacing = baseSpacing / m_currentLevelConfig.difficultyMultiplier;
        
        m_lastEnemyX += spacing;
        return m_lastEnemyX;
    }

    // Pickup positions are not calculated here anymore

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

    // ============================================================================
    // Legacy Obstacle Pooling System - All Removed and Handled by ObstacleSystem
    // ============================================================================
    // REMOVED: InitializeObstaclePool - now handled by ObstacleSystem
    // REMOVED: UpdateObstaclePooling - now handled by ObstacleSystem
    // ============================================================================
    // ObstacleSystem Integration - Clean Interface
    // ============================================================================

    void LevelManager::UpdateNPCStates(float deltaTime) {
        // Simple state: switch to surprise when player passes (x less than player x)
        // We don't have a player reference here; approximate by switching once when the NPC passes center of screen.
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float screenCenterX = screenInfo.pixelWidth * 0.5f; // fallback if player transform unavailable
        for (Gnosis::Entity e : m_activeNPCs) {
            NPC* npc = m_ecsSystem->GetComponent<NPC>(e);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(e);
            Transform* tr = m_ecsSystem->GetComponent<Transform>(e);
            if (!npc || !sprite || !tr) continue;

            if (npc->state == 0) {
                // sweeping -> surprised when the player has presumably passed
                bool passed = false;
                // Prefer real player position if available
                if (m_playerEntity != 0) {
                    Transform* playerTr = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
                    if (playerTr) {
                        // Trigger when the NPC has crossed to the left of the player
                        passed = (tr->position.x < playerTr->position.x);
                    }
                }
                // Fallback to screen center if we couldn't read player position
                if (!passed) {
                    passed = (tr->position.x < screenCenterX);
                }
                if (!npc->triggered && passed) {
                    npc->triggered = true;
                    npc->state = 1;
                    npc->timer = 0.8f; // show surprise for ~0.8s
                    // apply surprise clip
                    StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(e);
                    if (sa) {
                        const StateAnimation::Clip* clip = sa->getClip("surprise");
                        if (clip) {
                            sprite->textureId = clip->textureId;
                            sprite->isAnimated = (clip->frameCount > 1);
                            sprite->frameWidth = clip->frameWidth;
                            sprite->frameHeight = clip->frameHeight;
                            sprite->frameCount = clip->frameCount;
                            sprite->frameTime = clip->frameTime;
                            sprite->loop = clip->loop;
                            sprite->currentFrame = 0;
                            sprite->currentFrameTime = 0.0f; // Reset frame timer
                            sprite->playing = true;
                            sprite->visible = true; // Ensure visible
                            GN_LOG_INFO("Janitor triggered surprise animation");
                        }
                        sa->currentState = "surprise";
                    }
                }
            } else if (npc->state == 1) {
                npc->timer -= deltaTime;
                if (npc->timer <= 0.0f) {
                    npc->state = 0;
                    StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(e);
                    if (sa) {
                        const StateAnimation::Clip* clip = sa->getClip("sweep");
                        if (clip) {
                            sprite->textureId = clip->textureId;
                            sprite->isAnimated = (clip->frameCount > 1);
                            sprite->frameWidth = clip->frameWidth;
                            sprite->frameHeight = clip->frameHeight;
                            sprite->frameCount = clip->frameCount;
                            sprite->frameTime = clip->frameTime;
                            sprite->loop = clip->loop;
                            sprite->currentFrame = 0;
                            sprite->currentFrameTime = 0.0f; // Reset frame timer
                            sprite->playing = true;
                            sprite->visible = true; // Ensure visible
                            GN_LOG_INFO("Janitor returned to sweep animation");
                        }
                        sa->currentState = "sweep";
                    }
                }
            }
        }
    }

    // REMOVED: WrapGroupAroundScreen - now handled by ObstacleSystem

    std::vector<int> LevelManager::GetAndClearWrappedGroups() {
        // Delegate to ObstacleSystem
        if (m_obstacleSystem) {
            return m_obstacleSystem->GetAndClearWrappedGroups();
        }
        return {};
    }
    
    // ============================================================================
    // New Unified Coin System Implementation
    // ============================================================================
    
    LevelManager::GroupPattern LevelManager::DetectGroupPattern(int groupId) const {
        // Delegate to ObstacleSystem
        if (m_obstacleSystem) {
            return m_obstacleSystem->DetectGroupPattern(groupId);
        }
        return GroupPattern::TopAndBottom; // fallback
    }
    
    // REMOVED: Coin positioning functions moved to ObstacleSystem

    // REMOVED: NextSewerColor helper - now handled by ObstacleSystem

    // REMOVED: SpawnSewerPattern_TopOnly - now handled by ObstacleSystem

    // REMOVED: SpawnSewerPattern_BottomOnly - now handled by ObstacleSystem

    // REMOVED: SpawnSewerPattern_TopAndBottom - now handled by ObstacleSystem

    // REMOVED: SpawnSewerPattern_Pyramid3 - now handled by ObstacleSystem

    // REMOVED: SpawnSewerPattern_PyramidTop3 - now handled by ObstacleSystem

    // REMOVED: SpawnSewerPattern_TwoByTwoFunnel - now handled by ObstacleSystem

    // REMOVED: SpawnSewerPattern_Pyramid4 - now handled by ObstacleSystem

    // REMOVED: WrapObstacleAroundScreen - now handled by ObstacleSystem

    // 🎯 NEW: Set RenderSystem reference for cache access
    void LevelManager::SetRenderSystem(RenderSystem* renderSystem) {
        m_renderSystem = renderSystem;
        GN_LOG_INFO("LevelManager: RenderSystem reference set for texture metadata cache access");
    }

    // Enemy pooling system implementation
    Gnosis::Entity LevelManager::GetInactiveEnemy() {
        if (m_enemyPool.inactiveEnemies.empty()) {
            GN_LOG_WARN("LevelManager: No inactive enemies available in pool");
            return 0;
        }

        Gnosis::Entity enemy = m_enemyPool.inactiveEnemies.back();
        m_enemyPool.inactiveEnemies.pop_back();
        return enemy;
    }

    void LevelManager::ReturnEnemyToPool(Gnosis::Entity enemy) {
        if (!enemy) return;

        // Reset enemy state
        Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
        if (enemyComp) {
            enemyComp->isActive = false;
            enemyComp->currentState = EnemyState::Idle;
            enemyComp->hurtTimer = 0.0f;
            enemyComp->stateTimer = 0.0f;
            enemyComp->stateDuration = 0.0f;

            // Reset health to config value
            for (const auto& config : m_currentLevelConfig.enemies) {
                if (config.textureId == enemyComp->enemyType) {
                    enemyComp->health = config.hitPoints;
                    break;
                }
            }
        }

        // Reset transform to offscreen
        Transform* transform = m_ecsSystem->GetComponent<Transform>(enemy);
        if (transform) {
            transform->position = Gnosis::GNVector2(-1000.0f, -1000.0f);
        }

        // Reset sprite visibility and animation state
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
        if (sprite) {
            sprite->color = GNColor(255, 255, 255, 0); // Invisible
            sprite->visible = false; // Explicitly invisible for inactive enemies
            // CRITICAL: Reset animation frame to prevent ghostly stuck animations
            sprite->currentFrame = 0;
            sprite->currentFrameTime = 0.0f;
            sprite->hasCompleted = false;
            sprite->playing = false; // Stop playing until respawned
        }

        // Reset physics
        Physics* physics = m_ecsSystem->GetComponent<Physics>(enemy);
        if (physics) {
            physics->velocity = Gnosis::GNVector2(0.0f, 0.0f);
        }

        // Reset StateAnimation if present - MUST reset sprite texture to idle animation
        StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(enemy);
        if (sa && sprite && enemyComp) {
            // Find the initial state from config
            for (const auto& config : m_currentLevelConfig.enemies) {
                if (config.textureId == enemyComp->enemyType && config.useStateAnimation) {
                    sa->currentState = config.initialState;
                    
                    // CRITICAL: Reset sprite to idle animation to prevent ghostly stuck frames
                    const StateAnimation::Clip* idleClip = sa->getClip(config.initialState);
                    if (idleClip) {
                        sprite->textureId = idleClip->textureId;
                        sprite->frameWidth = idleClip->frameWidth;
                        sprite->frameHeight = idleClip->frameHeight;
                        sprite->frameCount = idleClip->frameCount;
                        sprite->frameTime = idleClip->frameTime;
                        sprite->loop = idleClip->loop;
                        sprite->currentFrame = 0;
                        sprite->currentFrameTime = 0.0f;
                        sprite->hasCompleted = false;
                        GN_LOG_DEBUG("ReturnEnemyToPool: Reset " + enemyComp->enemyType + " to idle animation: " + idleClip->textureId);
                    }
                    break;
                }
            }
        }

        // Return to inactive pool (all enemies can be reused)
        m_enemyPool.inactiveEnemies.push_back(enemy);

        // Remove from active enemies list
        auto activeIt = std::find(m_activeEnemies.begin(), m_activeEnemies.end(), enemy);
        if (activeIt != m_activeEnemies.end()) {
            m_activeEnemies.erase(activeIt);
        }

        GN_LOG_DEBUG("LevelManager: Returned enemy " + std::to_string(enemy) + " to inactive pool");
    }


    void LevelManager::SpawnInitialEnemies() {
        GN_LOG_INFO("LevelManager: Spawning initial enemies from pool");

        if (m_enemyPool.inactiveEnemies.empty()) {
            GN_LOG_WARN("LevelManager: No inactive enemies available to spawn");
            return;
        }

        // Get screen dimensions using ConfigManager
        const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();

        // Spawn up to 5 enemies from the inactive pool (increased for better variety)
        int maxSpawns = std::min(5, (int)m_enemyPool.inactiveEnemies.size());
        
        GN_LOG_INFO("LevelManager: Will spawn " + std::to_string(maxSpawns) + " enemies from " + 
                   std::to_string(m_enemyPool.inactiveEnemies.size()) + " available");

        for (int i = 0; i < maxSpawns; ++i) {
            Gnosis::Entity enemy = GetInactiveEnemy();
            if (!enemy) break;

            // Get the enemy's config from its component
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
            if (!enemyComp) {
                GN_LOG_ERROR("LevelManager: Spawned enemy has no Enemy component!");
                continue;
            }

            // Find the matching config for this enemy type
            const EnemyConfig* matchingConfig = nullptr;
            for (const auto& config : m_currentLevelConfig.enemies) {
                if (config.textureId == enemyComp->enemyType) {
                    matchingConfig = &config;
                    break;
                }
            }

            if (!matchingConfig) {
                GN_LOG_ERROR("LevelManager: Could not find config for enemy type: " + enemyComp->enemyType);
                continue;
            }

            // Position enemies offscreen to the right with generous spacing
            float x = screenInfo.pixelWidth + 650.0f + (i * 1200.0f); // 1200px spacing - better visibility and separation
            
            // Calculate Y position based on enemy type
            float y;
            if (enemyComp->enemyType == "ToiletPaperFlap" || m_currentLevelId == 2) {
                // Toilet paper (sewer level) - spawn centered at 31.25% (5/16) for 1/8 to 1/2 bobbing
                y = screenInfo.pixelHeight * 0.3125f;
            } else if (enemyComp->enemyType == "BirdIdle" || enemyComp->enemyType.find("Bird") != std::string::npos) {
                // Birds spawn only in top half of screen (15% to 45% range) in echelon formation
                float minY = screenInfo.pixelHeight * 0.15f;
                float maxY = screenInfo.pixelHeight * 0.45f;
                y = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
            } else if (enemyComp->enemyType == "RatCopterIdle" || enemyComp->movementPattern == "flying") {
                // LANDSCAPE-AWARE: Boss level is landscape (2556x1179), adjust Y band accordingly
                // In portrait: 30%-50% of 2556 = 766-1278 (middle band)
                // In landscape: 50%-70% of 1179 = 589-825 (adjusted middle-lower band for landscape)
                float minY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.30f : 0.50f);
                float maxY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.50f : 0.70f);
                y = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                GN_LOG_INFO("[RATCOPTER_SPAWN] ScreenHeight=" + std::to_string(screenInfo.pixelHeight) + 
                           ", isPortrait=" + std::to_string(screenInfo.isPortrait) + 
                           ", MinY=" + std::to_string(minY) + " (" + std::to_string((minY/screenInfo.pixelHeight)*100.0f) + "%), " +
                           "MaxY=" + std::to_string(maxY) + " (" + std::to_string((maxY/screenInfo.pixelHeight)*100.0f) + "%)");
                GN_LOG_INFO("[RATCOPTER_SPAWN] Positioned at Y=" + std::to_string(y) + 
                           " (" + std::to_string((y/screenInfo.pixelHeight)*100.0f) + "% of screen)");
            } else if (enemyComp->enemyType.find("SnowMan") != std::string::npos ||
                       enemyComp->enemyType.find("Snowman") != std::string::npos) {
                // GROUND SNOWMEN: Position at EXACT ground level with bottom alignment
                // Snowmen are 64px tall sprites, scaled by matchingConfig->scale (6.0x = 384px)
                float rawSpriteHeight = matchingConfig->frameHeight;  // 64px
                float scaledSpriteHeight = rawSpriteHeight * matchingConfig->scale;  // 64 * 6.0 = 384px
                
                // Rendering anchor is offset by raw sprite height, so push the top-left further down by +rawSpriteHeight
                // This ensures the visual bottom edge touches the ground exactly.
                y = screenInfo.pixelHeight - scaledSpriteHeight + rawSpriteHeight;
                
                float visualBottomEdge = y + scaledSpriteHeight - rawSpriteHeight; // Actual rendered bottom considering anchor
                
                GN_LOG_INFO("[SNOWMAN_POS] Grounding '" + enemyComp->enemyType + "' at Y=" + std::to_string(y) + 
                           " | screenH=" + std::to_string(screenInfo.pixelHeight) + 
                           ", rawH=" + std::to_string(rawSpriteHeight) + "px" +
                           ", scale=" + std::to_string(matchingConfig->scale) + "x" +
                           ", scaledH=" + std::to_string(scaledSpriteHeight) + "px" +
                           ", visualBottom=" + std::to_string(visualBottomEdge) + "px (GROUND expectation)");
            } else {
                // Other flying enemies
                y = screenInfo.pixelHeight * 0.3f + (i * 200.0f);
            }

            // Activate and position the enemy with its matching config
            SpawnEnemyWithConfig(enemy, *matchingConfig, x, y);
            
            GN_LOG_INFO("LevelManager: Spawned enemy type '" + enemyComp->enemyType + "' at (" + 
                       std::to_string(x) + ", " + std::to_string(y) + ")");
        }
    }

    void LevelManager::SpawnEnemyWithConfig(Gnosis::Entity enemy, const EnemyConfig& config, float x, float y) {
        // Update transform
        Transform* transform = m_ecsSystem->GetComponent<Transform>(enemy);
        if (transform) {
            transform->position = Gnosis::GNVector2(x, y);
            transform->scale = Gnosis::GNVector2(config.scale, config.scale);
        }

        // Update sprite
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
        if (sprite) {
            sprite->textureId = config.textureId;
            sprite->width = config.width;
            sprite->height = config.height;
            sprite->frameWidth = config.frameWidth;
            sprite->frameHeight = config.frameHeight;
            sprite->frameCount = config.frameCount;
            sprite->frameTime = config.frameTime;
            sprite->loop = config.loopAnimation;
            sprite->isAnimated = (config.frameCount > 1);
            sprite->playing = true; // CRITICAL: Start playing animation (just like Janitor and Player)
            sprite->currentFrame = 0;
            sprite->currentFrameTime = 0.0f;
            sprite->color = GNColor(255, 255, 255, 255); // Visible
            sprite->visible = true; // Explicitly visible for active enemies
        }

        // Update enemy component
        Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
        if (enemyComp) {
            enemyComp->isActive = true;
            enemyComp->health = config.hitPoints;
            enemyComp->enemyType = config.textureId;
            enemyComp->movementPattern = config.movementPattern;
            enemyComp->currentState = EnemyState::Idle;
            enemyComp->speed = config.speed;

            bool isSnowmanType = (enemyComp->enemyType.find("SnowMan") != std::string::npos ||
                                  enemyComp->enemyType.find("Snowman") != std::string::npos);
            if (isSnowmanType) {
                // FIXED: Snowmen should NOT have additional speed - they move with world scroll only
                enemyComp->speed = 0.0f; // No additional movement speed
                GN_LOG_INFO("[SNOWMAN_SPEED] Set '" + enemyComp->enemyType + "' speed=0 (moves with world scroll only, worldSpeed=" + 
                           std::to_string(m_currentLevelConfig.worldSpeed) + ")");
                
                // CRITICAL: Initialize throw animation parameters for snowman throwers
                if (enemyComp->isThrower && enemyComp->movementPattern == "snowman_thrower") {
                    enemyComp->totalFrames = 6;  // 6-frame throw animation
                    enemyComp->frameDuration = 0.12f;  // 0.12s per frame (match old script line 17)
                    enemyComp->throwAnimationDuration = enemyComp->totalFrames * enemyComp->frameDuration; // 0.72s total
                    GN_LOG_INFO("[SNOWMAN_INIT] Set throw animation: totalFrames=6, frameDuration=0.12s, duration=0.72s");
                }
            }

            // Apply bobbing configuration from EnemyConfig
            if (config.bobbingConfig.enabled) {
                enemyComp->bobbingEnabled = true;
                enemyComp->bobSpeed = config.bobbingConfig.baseSpeed;
                // Calculate amplitude based on screen height percentage
                const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                float screenHeight = screenInfo.pixelHeight;
                enemyComp->bobAmplitude = screenHeight * config.bobbingConfig.amplitudeMin; // Use min for now
                // Random starting phase 0..2π with better distribution for variation
                enemyComp->bobPhase = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.28318530718f;
            }
            
            // RATCOPTER: Initialize with FlyIn state for proper behavior state machine
            if (config.movementPattern == "flying") {
                enemyComp->currentState = EnemyState::FlyIn;
                enemyComp->hoverTimer = 0.0f;
                enemyComp->hasLockedDirection = false;
                enemyComp->pullbackTimer = 0.0f;
                enemyComp->beelineSpeed = 150.0f;
                enemyComp->baseY = y;  // Initialize baseY
                enemyComp->bobPhase = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.28318530718f;  // Random phase
                GN_LOG_INFO("[RATCOPTER_INIT] ===== SPAWN =====");
                GN_LOG_INFO("[RATCOPTER_INIT] Pos=(" + std::to_string(x) + "," + std::to_string(y) + 
                           "), BaseY=" + std::to_string(y) + ", State=FLY_IN");
                GN_LOG_INFO("[RATCOPTER_INIT] Speed=" + std::to_string(config.speed) + 
                           ", BobAmplitude=" + std::to_string(enemyComp->bobAmplitude) + 
                           ", BobSpeed=" + std::to_string(enemyComp->bobSpeed));
                GN_LOG_INFO("[RATCOPTER_INIT] =================");
            }
        }

        // Update hitbox based on config
        Hitbox* hitbox = m_ecsSystem->GetComponent<Hitbox>(enemy);
        if (hitbox) {
            hitbox->radius = config.width * 0.4f; // Rough approximation
        }

        // Initialize StateAnimation if present and apply initial animation clip
        StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(enemy);
        if (sa && sprite) {
            sa->currentState = config.initialState;
            GN_LOG_INFO("LevelManager: Enemy " + std::to_string(enemy) + " has StateAnimation with " + 
                       std::to_string(sa->clips.size()) + " clips, initial state: '" + config.initialState + "'");
            
            // Apply the initial animation clip immediately
            const StateAnimation::Clip* initialClip = sa->getClip(sa->currentState);
            if (initialClip) {
                sprite->textureId = initialClip->textureId;
                sprite->frameWidth = initialClip->frameWidth;
                sprite->frameHeight = initialClip->frameHeight;
                sprite->frameCount = initialClip->frameCount;
                sprite->frameTime = initialClip->frameTime;
                sprite->loop = initialClip->loop;
                sprite->isAnimated = (initialClip->frameCount > 1);
                sprite->playing = true;
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                
                GN_LOG_INFO("LevelManager: Applied initial animation clip '" + initialClip->textureId + 
                           "' with " + std::to_string(initialClip->frameCount) + " frames to enemy " + std::to_string(enemy));
            } else {
                GN_LOG_ERROR("LevelManager: Failed to find clip for initial state '" + config.initialState + "' on enemy " + std::to_string(enemy));
            }
        } else {
            if (!sa) {
                GN_LOG_INFO("LevelManager: Enemy " + std::to_string(enemy) + " has NO StateAnimation component");
            }
        }

        // Add to active enemies
        m_activeEnemies.push_back(enemy);
        m_enemyBaseY[enemy] = y;
        
        // CRITICAL: Set enemy's baseY for movement system (enemyComp already defined above)
        if (enemyComp) {
            enemyComp->baseY = y;
            enemyComp->hasInitializedBaseY = true;
            GN_LOG_INFO("[SPAWN] Set enemy baseY=" + std::to_string(y) + " for " + config.textureId);
        }

        GN_LOG_INFO("LevelManager: Spawned enemy " + std::to_string(enemy) + " of type " + config.textureId + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    }

    Gnosis::Entity LevelManager::SpawnBossEnemy(const EnemyConfig& config, float x, float y) {
        GN_LOG_INFO("LevelManager: Spawning BOSS enemy '" + config.textureId + "' at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        
        // Create a NEW entity for the boss (not from pool!)
        Gnosis::Entity bossEntity = m_ecsSystem->CreateEntity();
        
        // Add Transform component
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(config.scale, config.scale));
        m_ecsSystem->AddComponent<Transform>(bossEntity, transform);
        
        // Add Sprite component with animation properties from config
        Sprite sprite(config.textureId, static_cast<float>(config.frameWidth), static_cast<float>(config.frameHeight));
        sprite.frameWidth = config.frameWidth;
        sprite.frameHeight = config.frameHeight;
        sprite.frameCount = config.frameCount;
        sprite.frameTime = config.frameTime;
        sprite.currentFrame = 0;
        sprite.currentFrameTime = 0.0f;
        sprite.loop = config.loopAnimation;
        sprite.isAnimated = (config.frameCount > 1);
        sprite.playing = true;
        sprite.visible = true;
        sprite.color = GNColor(255, 255, 255, 255);
        sprite.layer = 2; // Foreground layer
        m_ecsSystem->AddComponent<Sprite>(bossEntity, sprite);
        
        // Add Enemy component
        Enemy enemy;
        enemy.enemyType = config.textureId;
        enemy.health = config.hitPoints;
        enemy.isActive = true;
        enemy.currentState = EnemyState::Idle;
        enemy.hurtTimer = 0.0f;
        enemy.baseY = y;
        enemy.movementPattern = config.movementPattern;
        enemy.speed = config.speed;
        enemy.stateTimer = 0.0f;
        enemy.stateDuration = 2.0f;
        enemy.bobbingEnabled = false;
        enemy.bobSpeed = 0.0f;
        enemy.bobAmplitude = 0.0f;
        enemy.bobPhase = 0.0f;
        enemy.hasInitializedBaseY = true;
        enemy.isGrounded = false;
        m_ecsSystem->AddComponent<Enemy>(bossEntity, enemy);
        
        // Add Hitbox component (boss uses circle hitbox typically)
        Hitbox hitbox;
        hitbox.type = ColliderType::Circle;
        hitbox.radius = 64.0f * config.scale; // Reasonable boss hitbox size
        hitbox.offsetX = 0.0f;
        hitbox.offsetY = 0.0f;
        hitbox.isStatic = false;
        hitbox.isTrigger = false;
        hitbox.tag = "enemy";
        m_ecsSystem->AddComponent<Hitbox>(bossEntity, hitbox);
        
        // Don't use StateAnimation for boss - BossSystem handles its own states
        // Just use the basic sprite animation from config
        
        // Add to active enemies (so BossSystem can find it)
        m_activeEnemies.push_back(bossEntity);
        m_enemyBaseY[bossEntity] = y;
        
        GN_LOG_INFO("LevelManager: Boss enemy " + std::to_string(bossEntity) + " spawned successfully!");
        return bossEntity;
    }

    void LevelManager::ResetEnemiesForRetry() {
        GN_LOG_INFO("[RESET] Resetting all enemies for level retry...");

        // Get screen dimensions for positioning
        const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();

        // Reset all active enemies - keep them VISIBLE and scrolling like obstacles
        // CRITICAL: Space them out horizontally, do NOT stack them!
        const float ENEMY_SPACING = 800.0f; // Generous spacing between enemies
        int enemyIndex = 0;
        
        for (Entity enemy : m_activeEnemies) {
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
            Transform* transform = m_ecsSystem->GetComponent<Transform>(enemy);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);

            if (enemyComp && transform) {
                // Position offscreen to the right with proper spacing (like initial spawn)
                float offsetX = screenInfo.pixelWidth + 650.0f + (enemyIndex * ENEMY_SPACING);
                transform->position.x = offsetX;
                
                // CRITICAL: Recalculate Y position for flying enemies to prevent top-of-screen spawn
                // Grounded enemies can preserve Y, but flying enemies need proper Y range
                if (enemyComp->movementPattern == "flying") {
                    float newY;
                    if (m_currentLevelId == 5 || m_currentLevelId == 6) {
                        // LANDSCAPE-AWARE: Boss level (6) is landscape, Castle level (5) is portrait
                        // Level 5 (Castle, Portrait): 30%-50% of 2556 = 766-1278
                        // Level 6 (Boss, Landscape): 50%-70% of 1179 = 589-825
                        float minY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.30f : 0.50f);
                        float maxY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.50f : 0.70f);
                        newY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                        GN_LOG_INFO("[RESET] RatCopter " + std::to_string(enemy) + " repositioned L" + std::to_string(m_currentLevelId) + ": screenH=" + 
                                   std::to_string(screenInfo.pixelHeight) + ", isPortrait=" + 
                                   std::to_string(screenInfo.isPortrait) + ", Y=" + std::to_string(newY) + 
                                   " (" + std::to_string((newY/screenInfo.pixelHeight)*100.0f) + "% band)");
                    } else if (m_currentLevelId == 3) {
                        // Desert level - Birds in top half (15%-45%)
                        float minY = screenInfo.pixelHeight * 0.15f;
                        float maxY = screenInfo.pixelHeight * 0.45f;
                        newY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                        GN_LOG_INFO("[RESET] Bird " + std::to_string(enemy) + " repositioned to Y=" + std::to_string(newY) + " (15%-45% band)");
                    } else {
                        newY = 900.0f + static_cast<float>((rand()%300) - 150);
                    }
                    transform->position.y = newY;
                    enemyComp->baseY = newY;
                    enemyComp->isGrounded = false; // CRITICAL: Ensure flying enemies stay airborne
                }
                // Other enemy types preserve their Y position
                
                // Reset flip state to default (facing left)
                transform->scale.x = std::abs(transform->scale.x);
                enemyComp->isFacingRight = false;

                // Reset enemy state but KEEP ACTIVE AND VISIBLE
                enemyComp->isActive = true;
                enemyComp->throwTimer = 0.0f;
                enemyComp->isThrowing = false;
                enemyComp->hasSpawnedProjectile = false;
                enemyComp->isOnScreen = false;
                enemyComp->hasThrownOnScreenEntry = false;
                enemyComp->stateTimer = 0.0f;
                enemyComp->throwAnimationTimer = 0.0f;
                enemyComp->currentThrowFrame = 0;
                
                // CRITICAL: Reset movement pattern specific flags for flying enemies
                // Reset enemy state based on movement pattern
                if (enemyComp->movementPattern == "flying" || enemyComp->movementPattern == "horizontal") {
                    enemyComp->currentState = (enemyComp->movementPattern == "flying") ? EnemyState::FlyIn : EnemyState::Moving;
                    enemyComp->isGrounded = false; // CRITICAL: Ensure flying/horizontal enemies are NOT grounded!
                    enemyComp->hoverTimer = 0.0f;
                    enemyComp->hasLockedDirection = false;
                    enemyComp->pullbackTimer = 0.0f;
                    enemyComp->beelineSpeed = 0.0f;
                    enemyComp->targetDirection = Gnosis::GNVector2(0.0f, 0.0f);
                    enemyComp->pullbackVector = Gnosis::GNVector2(0.0f, 0.0f);
                    GN_LOG_INFO("[RESET] Flying/horizontal enemy " + enemyComp->enemyType + " " + std::to_string(enemy) + " reset, isGrounded=false, pattern=" + enemyComp->movementPattern);
                } else {
                    enemyComp->currentState = EnemyState::Idle;
                }

                GN_LOG_INFO("[RESET] Reset enemy " + std::to_string(enemy) + " #" + std::to_string(enemyIndex) +
                           " - positioned at (" + std::to_string(transform->position.x) + ", " + 
                           std::to_string(transform->position.y) + "), Y PRESERVED, VISIBLE and ACTIVE");
                
                enemyIndex++;
            }
            
            // Keep sprites VISIBLE and PLAYING - enemies scroll naturally like obstacles
            if (sprite) {
                sprite->visible = true; // Keep visible!
                sprite->color.a = 255; // Full opacity
                // Reset animation to idle state but keep playing
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                sprite->hasCompleted = false;
                sprite->playing = true; // Keep playing!
                
                // Reset to idle animation for StateAnimation enemies
                StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(enemy);
                if (sa && enemyComp) {
                    for (const auto& config : m_currentLevelConfig.enemies) {
                        if (config.textureId == enemyComp->enemyType && config.useStateAnimation) {
                            sa->currentState = config.initialState;
                            const StateAnimation::Clip* idleClip = sa->getClip(config.initialState);
                            if (idleClip) {
                                sprite->textureId = idleClip->textureId;
                                sprite->frameWidth = idleClip->frameWidth;
                                sprite->frameHeight = idleClip->frameHeight;
                                sprite->frameCount = idleClip->frameCount;
                                sprite->frameTime = idleClip->frameTime;
                                sprite->loop = idleClip->loop;
                            }
                            break;
                        }
                    }
                }
            }
        }
        
        int enemyCount = static_cast<int>(m_activeEnemies.size());

        GN_LOG_INFO("[RESET] All " + std::to_string(enemyCount) + " enemies reset with " + 
                   std::to_string(ENEMY_SPACING) + "px spacing - Y positions PRESERVED, kept VISIBLE and ACTIVE");
    }

} // namespace GameCore
