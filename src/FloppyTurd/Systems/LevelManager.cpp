#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../Config/EnemyConfigs.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include "../Game/FloppyTurdGame.h"
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
        , m_pickupSystem(nullptr)  // NEW: Initialize PickupSystem reference for orchestrator
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
        
        // Spawn initial obstacle groups using orchestrator pattern
        GN_LOG_INFO("Spawning initial groups for level " + std::to_string(levelId));
        SpawnInitialGroups(levelId);
        GN_LOG_INFO("Initial groups spawned successfully for level " + std::to_string(levelId));


        
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
        
        // Clear group manifests
        m_groupManifests.clear();
        m_nextGroupId = 1;  // Reset group ID counter
        GN_LOG_INFO("Cleared " + std::to_string(m_groupManifests.size()) + " group manifests");
        
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
    
    void LevelManager::ClearGroupManifests() {
        m_groupManifests.clear();
        m_nextGroupId = 1;
        GN_LOG_INFO("Cleared all group manifests, reset group ID counter to 1");
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
    
    // DEBUG: Toggle this flag to unlock all levels for testing
    // Set to 'true' to unlock all levels, 'false' for proper progression
    const bool DEBUG_UNLOCK_ALL_LEVELS = false;
    
    if (DEBUG_UNLOCK_ALL_LEVELS) {
        return true;
    }
    
    // Legacy Mode Logic:
    // Level 1 (Park) is "Legacy Mode" - Only unlocked if flag is set (or debug)
    if (levelId == 1) {
        return ConfigManager::Instance().IsLegacyModeUnlocked();
    }
    
    // Level 2 (Sewer) is the NEW Default Starting Level
    if (levelId == 2) {
        return true;
    }
    
    // Regular Progression (Levels 3+)
    // Check if previous level is completed
    if (levelId > 2 && levelId <= GetMaxLevelId()) {
        return m_levelCompleted[levelId - 2]; // Previous level completed (e.g. L3 needs L2 done)
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

    // Special Case: Beating Boss Level (Level 6) Unlocks Legacy Mode (Level 1)
    if (levelId == 6) {
        ConfigManager::Instance().SetLegacyModeUnlocked(true);
        ConfigManager::Instance().SaveConfiguration(); // Persist the unlock
        GN_LOG_INFO("BOSS DEFEATED! Legacy Mode (Level 1) Unlocked!");
    }
    
    SaveProgression();
    GN_LOG_INFO("Level " + std::to_string(levelId) + " completed with score: " + std::to_string(score));
    
    return true;
}
    
    void LevelManager::UpdateObstacleSystem(float deltaTime, float worldScrollDistance) {
        if (!m_obstacleSystem) return;
        
        // Step 1: Update obstacle animations (oscillations, spike balls, cactus)
        m_obstacleSystem->Update(deltaTime, worldScrollDistance);
        
        // Step 2: ORCHESTRATOR PATTERN - Check manifests for groups needing wrapping
        // LevelManager owns manifests and calculates wrap detection using rightmostMemberOffsetX
        for (auto& pair : m_groupManifests) {
            int groupId = pair.first;
            GroupManifest& manifest = pair.second;
            
            // Skip if no leader
            if (manifest.leaderEntity == 0) continue;
            
            // Get leader position
            Transform* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest.leaderEntity);
            if (!leaderTransform) continue;
            
            // Calculate rightmost member's world position
            float rightmostWorldX = leaderTransform->position.x + manifest.rightmostMemberOffsetX;
            
            // Determine wrap threshold based on gap width
            float wrapThreshold = -(manifest.gapWidth / 2.0f);
            
            // Wrap if rightmost member is off-screen
            if (rightmostWorldX < wrapThreshold) {
                GN_LOG_INFO("[LevelManager] Group " + std::to_string(groupId) + 
                           " needs wrap: rightmost=" + std::to_string(rightmostWorldX) + 
                           ", threshold=" + std::to_string(wrapThreshold));
                WrapGroup(groupId);
            }
        }
    }

    // REMOVED: ConsumeWrappedGroups() - wrap detection now handled directly in UpdateObstacleSystem()

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
                // Use custom hitboxRadius if set, otherwise calculate from width
                hitbox.radius = (config.hitboxRadius > 0.0f) ? config.hitboxRadius : (config.width * 0.4f);
                hitbox.offsetY = config.hitboxOffsetY; // Apply custom Y offset
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

    void LevelManager::UpdateEnemyPooling(float deltaTime, float worldScrollDistance, int pipesCleared) {
        if (!m_enemyPoolInitialized) return;
        
        // Throttling: Only look to spawn if timer is ready
        if (m_respawnTimer > 0.0f) {
            m_respawnTimer -= deltaTime;
            // Don't return, we still might need to do other pooling updates if any (currently none)
        }
        
        // CRITICAL FIX: Respawn enemies from inactive pool if active count is low
        // This handles enemies that were killed/returned to pool
        int initialEnemyCount = m_currentLevelConfig.enemies.size() > 0 ? static_cast<int>(m_currentLevelConfig.enemies.size()) : 3;
        
        // STRICT LIMIT: Level 2 (Galaga) capped based on PROGRESSIVE DIFFICULTY
        if (m_currentLevelId == 2) {
            if (pipesCleared < 25) {
                initialEnemyCount = 1;
            } else if (pipesCleared < 50) {
                initialEnemyCount = 2;
            } else {
                initialEnemyCount = 3;
            }
        } else if (m_currentLevelId == 5) {
            // Level 5 (Castle): Same progressive difficulty as Level 2
            // 1 rat until 25 pipes, 2 rats until 50 pipes, 3 rats after
            if (pipesCleared < 25) {
                initialEnemyCount = 1;
            } else if (pipesCleared < 50) {
                initialEnemyCount = 2;
            } else {
                initialEnemyCount = 3;
            }
        } else if (m_currentLevelId == 3) {
            // Level 3 (Desert): DO NOT respawn individual birds
            // Birds only come back when the group wraps together
            // Set initialEnemyCount to 0 to prevent respawn logic from triggering
            initialEnemyCount = 0; // Birds managed by group wrap, not individual respawn
        }
        
        // Change WHILE to IF to throttle - spawn one at a time with delay
        if (m_respawnTimer <= 0.0f && m_activeEnemies.size() < static_cast<size_t>(initialEnemyCount) && !m_enemyPool.inactiveEnemies.empty()) {
            // Get an inactive enemy from the pool
            Gnosis::Entity enemy = GetInactiveEnemy();
            if (enemy) {
            
            // Reactivate and reposition the enemy
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
            Transform* transform = m_ecsSystem->GetComponent<Transform>(enemy);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
            
            // DebugDraw DISABLED per user request
            // if (!m_ecsSystem->HasComponent<DebugDraw>(enemy)) {
            //     DebugDraw debugDraw;
            //     debugDraw.debugLayer = 100;
            //     debugDraw.showBounds = false;
            //     debugDraw.showCollider = true;
            //     debugDraw.colliderColor = {255, 0, 0, 255};
            //     debugDraw.alpha = 0.5f;
            //     m_ecsSystem->AddComponent<DebugDraw>(enemy, debugDraw);
            //     GN_LOG_INFO("[DEBUG_DRAW] Added hitbox visualizer to enemy " + std::to_string(enemy));
            // } else {
            //     DebugDraw* dd = m_ecsSystem->GetComponent<DebugDraw>(enemy);
            //     dd->showCollider = true;
            //     dd->colliderColor = {255, 0, 0, 255};
            // }
            
            if (enemyComp && transform && sprite) {
                // Find rightmost active enemy position
                float rightmostX = ConfigManager::Instance().GetCurrentScreenInfo().pixelWidth;
                for (Gnosis::Entity e : m_activeEnemies) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    if (t && t->position.x > rightmostX) rightmostX = t->position.x;
                }
                
                // Position offscreen right with spacing
                // Level 2 (Galaga) needs larger spacing
                float spacing = (m_currentLevelId == 2) ? 1400.0f : (m_enemySpacing * 1.25f);
                transform->position.x = rightmostX + spacing;
                
                // Calculate Y position based on enemy type and level
                float baseY;
                // Get player Y for centering enemies
                // Player sprite renders from top-left, so player center = player.y + (playerHeight/2)
                Gnosis::GNVector2 playerPos = GetPlayerPosition();
                // Assume player sprite height is ~64*6 = 384 (scaled), use 192 as half height
                float playerHalfHeight = 192.0f; // Half of player sprite height
                float playerCenterY = playerPos.y + playerHalfHeight;
                
                if (m_currentLevelId == 2) { // Sewer - toilet paper (center on player)
                    // Enemy also renders from top-left, so offset by half enemy height
                    float enemySpriteHeight = sprite->height * std::abs(transform->scale.y);
                    baseY = playerCenterY - (enemySpriteHeight * 0.5f);
                } else if (m_currentLevelId == 3) { // Desert - birds in echelon
                    // ECHELON FORMATION: Base at player height, offset down per bird
                    float enemySpriteHeight = sprite->height * std::abs(transform->scale.y);
                    int activeCount = static_cast<int>(m_activeEnemies.size());
                    baseY = playerCenterY - (enemySpriteHeight * 0.5f) + (activeCount * 150.0f);
                } else if (m_currentLevelId == 5 || m_currentLevelId == 6) { // Castle (5) or Boss (6) - RatCopters
                    // RAT_SWARM: Spawn at player center Y + formation offset
                    float enemySpriteHeight = sprite->height * std::abs(transform->scale.y);
                    baseY = playerCenterY - (enemySpriteHeight * 0.5f) + enemyComp->formationOffset.y;
                    GN_LOG_INFO("[RAT_SPAWN] RatCopter L" + std::to_string(m_currentLevelId) + 
                               ": playerCenterY=" + std::to_string(playerCenterY) + 
                               ", formationOffset=" + std::to_string(enemyComp->formationOffset.y) +
                               ", Y=" + std::to_string(baseY));
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
                
                // Reset flying enemy state (RatCopters, Birds, Toilet Paper)
                if (enemyComp->movementPattern == "flying" || enemyComp->movementPattern == "horizontal" || 
                    enemyComp->movementPattern == "galaga" || enemyComp->movementPattern == "echelon" || 
                    enemyComp->movementPattern == "rat_swarm") {
                    // Reset state based on movement pattern
                    if (enemyComp->movementPattern == "galaga") {
                        // GALAGA: Reset to Idle so state machine runs from beginning
                        enemyComp->currentState = EnemyState::Idle;
                        enemyComp->galagaHoverTimer = 0.0f;
                        enemyComp->galagaHoverComplete = false;
                        enemyComp->circleAngle = 0.0f;
                        enemyComp->circleLoopsRemaining = 3;
                        enemyComp->hasSetAnchorX = false;
                        GN_LOG_INFO("[ENEMY_RESPAWN] Reset GALAGA enemy " + enemyComp->enemyType + " to Idle state");
                    } else if (enemyComp->movementPattern == "echelon") {
                        // ECHELON: Reset to Moving state, preserve formation offset
                        enemyComp->currentState = EnemyState::Moving;
                        enemyComp->isGrounded = false;
                        GN_LOG_INFO("[ENEMY_RESPAWN] Reset ECHELON bird " + enemyComp->enemyType + " isGrounded=false");
                    } else if (enemyComp->movementPattern == "rat_swarm") {
                        // RAT_SWARM: Reset to FlyIn state for galaga-style approach
                        enemyComp->currentState = EnemyState::FlyIn;
                        enemyComp->isGrounded = false;
                        enemyComp->hoverTimer = 0.0f;
                        GN_LOG_INFO("[ENEMY_RESPAWN] Reset RAT_SWARM " + enemyComp->enemyType + " to FlyIn state");
                    } else {
                        enemyComp->currentState = (enemyComp->movementPattern == "flying") ? EnemyState::FlyIn : EnemyState::Moving;
                    }
                    enemyComp->isGrounded = false; // CRITICAL: Ensure flying enemies don't get grounded
                    enemyComp->hoverTimer = 0.0f;
                    enemyComp->hasLockedDirection = false;
                    enemyComp->pullbackTimer = 0.0f;
                    enemyComp->targetDirection = Gnosis::GNVector2(0.0f, 0.0f);
                    enemyComp->pullbackVector = Gnosis::GNVector2(0.0f, 0.0f);
                    enemyComp->beelineSpeed = 0.0f;
                    
                    if (enemyComp->movementPattern != "galaga" && enemyComp->movementPattern != "echelon") {
                        GN_LOG_INFO("[ENEMY_RESPAWN] Reset flying/horizontal enemy " + enemyComp->enemyType + " isGrounded=false, pattern=" + enemyComp->movementPattern);
                    }
                }
                
                // Make sprite visible and reset animation
                sprite->visible = true;
                sprite->color.a = 255;
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                sprite->hasCompleted = false;
                sprite->playing = true;
                
                // CRITICAL: Reset transform scale and enemy speed from config
                for (const auto& config : m_currentLevelConfig.enemies) {
                    if (config.textureId == enemyComp->enemyType) {
                        transform->scale = Gnosis::GNVector2(config.scale, config.scale);
                        enemyComp->speed = config.speed; // Fix slow enemy bug
                        GN_LOG_INFO("[RESPAWN_CONFIG] Reset " + enemyComp->enemyType + 
                                   " scale=" + std::to_string(config.scale) + 
                                   " speed=" + std::to_string(config.speed));
                        break;
                    }
                }
                
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
                
                // Reset timer to throttle next spawn
                // Level 2 (Galaga) wants slower pacing (8.0s), Level 5 (Castle) 6.0s for spacing
                if (m_currentLevelId == 2) {
                    m_respawnTimer = 8.0f;
                } else if (m_currentLevelId == 5) {
                    m_respawnTimer = 6.0f; // Slower spawn rate for castle rats
                } else {
                    m_respawnTimer = 2.0f;
                }
            }
            } // Close if(enemy)
        } // Close if(respawnTimer)
        
        // Request struct for respawning group members (defined locally)
        struct GroupRespawnRequest {
            int groupId;
            bool vPointsDown;
            float groupBaseX;
            float echelonBaseY;
            std::vector<int> missingPositions;
        };
        std::vector<GroupRespawnRequest> respawnRequests;

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
                } else if (enemyComp && enemyComp->movementPattern == "echelon") {
                    // ECHELON GROUP WRAP: Wait for the TRAILING bird (highest X offset) to go off-screen
                    // Find if there's any other bird in this group with a higher X offset
                    int groupId = enemyComp->groupId;
                    bool isTrailingBird = true;
                    
                    for (Gnosis::Entity other : m_activeEnemies) {
                        if (other == e) continue;
                        Enemy* otherComp = m_ecsSystem->GetComponent<Enemy>(other);
                        if (otherComp && otherComp->movementPattern == "echelon" && otherComp->groupId == groupId) {
                            // If another bird has higher X offset, this one isn't the trailing bird
                            if (otherComp->formationOffset.x > enemyComp->formationOffset.x + 1.0f) {
                                isTrailingBird = false;
                                break;
                            }
                        }
                    }
                    
                    if (isTrailingBird) {
                        // Trailing bird is off-screen! Wrap ALL echelon birds together
                        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                        float echelonBaseY = screenInfo.pixelHeight * 0.15f; // 15% - above toilets
                        
                        // Calculate new group base from rightmostX
                        // rightmostX is the rightmost bird's X position (calculated before any wraps)
                        // The rightmost bird in a full group has offset=300
                        // So: currentGroupBase = rightmostX - 300
                        // And: newGroupBase = currentGroupBase + 800
                        float currentGroupBaseX = rightmostX - 300.0f; // Always use max offset
                        float groupBaseX = currentGroupBaseX + 800.0f;
                        
                        GN_LOG_INFO("[ECHELON_GROUP_WRAP] Trailing bird (posInV=" + std::to_string(enemyComp->posInV) + 
                                   ") triggered wrap! RightmostX=" + std::to_string(rightmostX) +
                                   ", CurrentGroupBase=" + std::to_string(currentGroupBaseX) +
                                   ", NewGroupBase=" + std::to_string(groupBaseX));
                        
                        // Collect which positions are still present (alive birds)
                        bool present[5] = {false, false, false, false, false};
                        
                        // Wrap and resurrect ALL echelon birds of this group (including hidden/dead ones)
                        for (Gnosis::Entity birdEntity : m_activeEnemies) {
                            Enemy* birdComp = m_ecsSystem->GetComponent<Enemy>(birdEntity);
                            Transform* birdTransform = m_ecsSystem->GetComponent<Transform>(birdEntity);
                            Sprite* birdSprite = m_ecsSystem->GetComponent<Sprite>(birdEntity);

                            if (birdComp && birdTransform && birdComp->movementPattern == "echelon" && birdComp->groupId == groupId) {
                                // RECALCULATE offsets based on posInV for consistency
                                float xOffset = 0.0f;
                                float yOffset = 0.0f;
                                switch (birdComp->posInV) {
                                    case 0: xOffset = 0.0f;   yOffset = 0.0f; break;      // Tip (front center)
                                    case 1: xOffset = 150.0f; yOffset = -100.0f; break;   // Upper wing 1
                                    case 2: xOffset = 150.0f; yOffset = 100.0f; break;    // Lower wing 1
                                    case 3: xOffset = 300.0f; yOffset = -200.0f; break;   // Upper wing 2 (trailing)
                                    case 4: xOffset = 300.0f; yOffset = 200.0f; break;    // Lower wing 2 (trailing)
                                }
                                
                                // Update stored offset to match
                                birdComp->formationOffset = Gnosis::GNVector2(xOffset, yOffset);
                                
                                // Apply formation offset to base position
                                birdTransform->position.x = groupBaseX + xOffset;
                                birdTransform->position.y = echelonBaseY + yOffset;
                                birdComp->baseY = echelonBaseY;
                                birdComp->isGrounded = false;
                                birdComp->currentState = EnemyState::Moving;
                                
                                // Track this position for logging
                                if (birdComp->posInV >= 0 && birdComp->posInV < 5) {
                                    present[birdComp->posInV] = true;
                                }
                                
                                // RESURRECTION: Make all birds visible and healthy
                                if (birdSprite) {
                                    bool wasHidden = !birdSprite->visible || birdComp->health <= 0;
                                    birdSprite->visible = true;
                                    birdSprite->color.a = 255;
                                    birdSprite->color.r = 255;
                                    birdSprite->color.g = 255;
                                    birdSprite->color.b = 255;
                                    birdSprite->playing = true;
                                    birdComp->health = 1;
                                    
                                    if (wasHidden) {
                                        GN_LOG_INFO("[ECHELON_RESURRECT] Bird posInV=" + std::to_string(birdComp->posInV) + 
                                                   " resurrected at (" + std::to_string(birdTransform->position.x) + "," + 
                                                   std::to_string(birdTransform->position.y) + ")");
                                    }
                                }
                            }
                        }
                        
                        // No respawn queue needed - all birds stay in active list and get resurrected above
                        
                        // Update rightmostX to prevent overlap
                        // The new rightmost bird will be at groupBaseX + 300 (trailing bird offset)
                        rightmostX = groupBaseX + 300.0f;
                        
                        GN_LOG_INFO("[ECHELON_GROUP_WRAP] All birds wrapped and resurrected. New rightmostX=" + std::to_string(rightmostX));
                    }
                    // If not trailing bird, DON'T wrap yet - wait for the group
                    continue;
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
                } else if (m_currentLevelId == 3) { // Desert level - non-echelon enemies
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
                    
                    // CRITICAL: Reset flying enemy state when wrapping (RatCopters, Birds, Galaga, Rat Swarm)
                    if (enemyComp->movementPattern == "flying" || enemyComp->movementPattern == "horizontal" || 
                        enemyComp->movementPattern == "galaga" || enemyComp->movementPattern == "rat_swarm") {
                        if (enemyComp->movementPattern == "galaga") {
                            // GALAGA: Reset to Idle to restart full pattern
                            enemyComp->currentState = EnemyState::Idle;
                            enemyComp->galagaHoverTimer = 0.0f;
                            enemyComp->hasSetAnchorX = false;
                            enemyComp->circleAngle = 0.0f;
                            enemyComp->circleLoopsRemaining = 3;
                            GN_LOG_INFO("[GALAGA_WRAP] Reset to Idle, pos=(" + std::to_string(t->position.x) + "," + std::to_string(baseY) + ")");
                        } else if (enemyComp->movementPattern == "rat_swarm") {
                            // RAT_SWARM: Reset to FlyIn state for galaga-style approach
                            enemyComp->currentState = EnemyState::FlyIn;
                            enemyComp->hoverTimer = 0.0f;
                            enemyComp->isGrounded = false;
                            
                            // Center Y on player + formation offset
                            Gnosis::GNVector2 playerPos = GetPlayerPosition();
                            float playerHalfHeight = 192.0f;
                            float playerCenterY = playerPos.y + playerHalfHeight;
                            float enemySpriteHeight = s->height * std::abs(t->scale.y);
                            baseY = playerCenterY - (enemySpriteHeight * 0.5f) + enemyComp->formationOffset.y;
                            t->position.y = baseY;
                            
                            GN_LOG_INFO("[RAT_SWARM_WRAP] Reset to FlyIn, pos=(" + std::to_string(t->position.x) + "," + std::to_string(baseY) + 
                                       "), formationOffset=" + std::to_string(enemyComp->formationOffset.y));
                        } else {
                            enemyComp->currentState = (enemyComp->movementPattern == "flying") ? EnemyState::FlyIn : EnemyState::Moving;
                        }
                        
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
        
        // Process pending group respawns
        for (const auto& req : respawnRequests) {
            GN_LOG_INFO("[ECHELON_RESPAWN_DEBUG] Processing respawn request for group " + std::to_string(req.groupId) + 
                       ", missing positions: " + std::to_string(req.missingPositions.size()) +
                       ", pool size: " + std::to_string(m_enemyPool.inactiveEnemies.size()));
            
            for (int posInV : req.missingPositions) {
                // Get an inactive enemy from the pool
                if (m_enemyPool.inactiveEnemies.empty()) {
                    GN_LOG_WARN("[ECHELON_RESPAWN_DEBUG] Pool is empty! Cannot respawn bird at pos " + std::to_string(posInV));
                    break;
                }
                
                Gnosis::Entity enemy = GetInactiveEnemy();
                if (!enemy) {
                    GN_LOG_WARN("[ECHELON_RESPAWN_DEBUG] GetInactiveEnemy returned null!");
                    continue;
                }
                
                Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
                Transform* transform = m_ecsSystem->GetComponent<Transform>(enemy);
                Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
                
                GN_LOG_INFO("[ECHELON_RESPAWN_DEBUG] Got entity " + std::to_string(enemy) + 
                           ", enemyComp=" + (enemyComp ? "OK" : "NULL") +
                           ", transform=" + (transform ? "OK" : "NULL") +
                           ", sprite=" + (sprite ? "OK" : "NULL"));
                
                if (enemyComp && transform && sprite) {
                    // Reconstruct properties
                    enemyComp->groupId = req.groupId;
                    enemyComp->posInV = posInV;
                    enemyComp->vFormationPointsDown = req.vPointsDown;
                    enemyComp->currentState = EnemyState::Moving;
                    enemyComp->isGrounded = false;
                    enemyComp->movementPattern = "echelon";
                    enemyComp->enemyType = "BirdIdle"; // Assume standard bird
                    
                    // V-DOWN FORMATION (consistent with initial spawn):
                    // Bird 0: Center front (lowest X, middle Y - the "tip" pointing forward)
                    // Bird 1,2: Behind tip, spread up/down
                    // Bird 3,4: Furthest back, spread further up/down
                    // Result: V shape with tip at front, wings trailing behind
                    float xOffset = 0.0f;
                    float yOffset = 0.0f;
                    switch (posInV) {
                        case 0: xOffset = 0.0f;   yOffset = 0.0f; break;      // Tip (front center)
                        case 1: xOffset = 150.0f; yOffset = -100.0f; break;   // Upper wing 1
                        case 2: xOffset = 150.0f; yOffset = 100.0f; break;    // Lower wing 1  
                        case 3: xOffset = 300.0f; yOffset = -200.0f; break;   // Upper wing 2 (trailing)
                        case 4: xOffset = 300.0f; yOffset = 200.0f; break;    // Lower wing 2 (trailing)
                    }
                    enemyComp->formationOffset = Gnosis::GNVector2(xOffset, yOffset);
                    enemyComp->baseY = req.echelonBaseY;
                    
                    // Set position
                    transform->position.x = req.groupBaseX + xOffset;
                    transform->position.y = req.echelonBaseY + yOffset;
                    
                    GN_LOG_INFO("[ECHELON_RESPAWN_DEBUG] Set position: X=" + std::to_string(transform->position.x) +
                               ", Y=" + std::to_string(transform->position.y));
                    
                    // Config lookups for scale/speed and texture
                    bool foundConfig = false;
                     for (const auto& config : m_currentLevelConfig.enemies) {
                        if (config.textureId == enemyComp->enemyType) {
                            transform->scale = Gnosis::GNVector2(config.scale, config.scale);
                            enemyComp->speed = config.speed;
                            
                            // CRITICAL: Set sprite texture and dimensions
                            sprite->textureId = config.textureId;
                            sprite->width = config.width;
                            sprite->height = config.height;
                            sprite->frameWidth = config.frameWidth;
                            sprite->frameHeight = config.frameHeight;
                            sprite->frameCount = config.frameCount;
                            sprite->frameTime = config.frameTime;
                            sprite->currentFrame = 0;
                            sprite->currentFrameTime = 0.0f;
                            foundConfig = true;
                            GN_LOG_INFO("[ECHELON_RESPAWN_DEBUG] Applied config for BirdIdle");
                            break;
                        }
                    }
                    
                    if (!foundConfig) {
                        GN_LOG_WARN("[ECHELON_RESPAWN_DEBUG] Could not find BirdIdle config!");
                    }
                    
                    // Reset sprite and health
                    sprite->visible = true;
                    sprite->color.a = 255;
                    sprite->color.r = 255;
                    sprite->color.g = 255;
                    sprite->color.b = 255;
                    sprite->playing = true;
                    sprite->isAnimated = true;
                    enemyComp->health = 1; // Ensure bird has health
                    
                    // Activate!
                    m_activeEnemies.push_back(enemy);
                    
                    GN_LOG_INFO("[ECHELON_RESPAWN] Respawned bird pos=" + std::to_string(posInV) + 
                               " for group " + std::to_string(req.groupId) +
                               " at (" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ")" +
                               ", visible=" + std::to_string(sprite->visible) + ", activeCount=" + std::to_string(m_activeEnemies.size()));
                } else {
                    GN_LOG_WARN("[ECHELON_RESPAWN_DEBUG] Component retrieval failed for entity " + std::to_string(enemy));
                }
            }
        }
    }

    void LevelManager::UpdateNPCPooling(float deltaTime, float) {
        if (!m_npcPoolInitialized || m_janitorEntity == 0) return;
        // If Janitor goes off-screen left, move him to the right again at ground Y
        Transform* t = m_ecsSystem->GetComponent<Transform>(m_janitorEntity);
        Sprite* s = m_ecsSystem->GetComponent<Sprite>(m_janitorEntity);
        if (!t || !s) return;

        // Continuously match Janitor speed using SAME formula as ApplyDifficulty
        // Sewer layer depth is 0.35f, formula: BASE_BACKGROUND_SPEED * (0.3 + depth * 0.7) * diffMultiplier
        float sewerDepth = 0.35f;
        float parallaxMultiplier = 0.3f + (sewerDepth * 0.7f); // = 0.545
        float targetSpeed = SpeedConstants::BASE_BACKGROUND_SPEED * parallaxMultiplier * m_currentLevelConfig.difficultyMultiplier;
        
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
            // Re-apply ScrollSpeed on wrap (same formula as above)
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
        // Use the SAME formula as ApplyDifficulty to ensure perfect sync with sewer background
        // Sewer layer depth is 0.35f, formula: BASE_BACKGROUND_SPEED * (0.3 + depth * 0.7) * diffMultiplier
        float sewerDepth = 0.35f; // Same as AddSewerLevelLayers
        float parallaxMultiplier = 0.3f + (sewerDepth * 0.7f); // = 0.545
        float janitorSpeed = SpeedConstants::BASE_BACKGROUND_SPEED * parallaxMultiplier * m_currentLevelConfig.difficultyMultiplier;
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
        
        // Request struct for respawning group members (defined locally)
        // REMOVED bad insertion
        
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

    // LEGACY DELETED: GetAndClearWrappedGroups() - wrap detection now internal to LevelManager orchestrator
    
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

        // Increment enemy kill counter (this is called when enemy is defeated)
        if (auto* game = GameCore::GetGame()) {
            game->IncrementSessionEnemyKills();
        }

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
        // STRICT LIMIT: Level 2 (Galaga) and Level 5 (Castle) capped at 1 initially for progressive build-up
        int spawnLimit = (m_currentLevelId == 2 || m_currentLevelId == 5) ? 1 : 5;
        int maxSpawns = std::min(spawnLimit, (int)m_enemyPool.inactiveEnemies.size());
        
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

            // DebugDraw DISABLED per user request
            // if (!m_ecsSystem->HasComponent<DebugDraw>(enemy)) {
            //     DebugDraw debugDraw;
            //     debugDraw.debugLayer = 100;
            //     debugDraw.showBounds = false;
            //     debugDraw.showCollider = true;
            //     debugDraw.colliderColor = {255, 0, 0, 255};
            //     debugDraw.alpha = 0.5f;
            //     m_ecsSystem->AddComponent<DebugDraw>(enemy, debugDraw);
            //     GN_LOG_INFO("[DEBUG_DRAW] Added hitbox visualizer to initial enemy " + std::to_string(enemy));
            // } else {
            //     DebugDraw* dd = m_ecsSystem->GetComponent<DebugDraw>(enemy);
            //     dd->showCollider = true;
            //     dd->colliderColor = {255, 0, 0, 255};
            // }

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
            // Level 2 (Sewer/Galaga): Larger buffer and spacing to prevent swarming
            float initialBuffer = (m_currentLevelId == 2) ? 1500.0f : 650.0f;
            float spacing = (m_currentLevelId == 2) ? 1400.0f : 1200.0f;
            
            float x = screenInfo.pixelWidth + initialBuffer + (i * spacing);
            
            // Calculate Y position based on enemy type
            float y;
            if (enemyComp->enemyType == "ToiletPaperFlap" || m_currentLevelId == 2) {
                // Toilet paper (sewer level) - spawn centered at 31.25% (5/16) for 1/8 to 1/2 bobbing
                y = screenInfo.pixelHeight * 0.3125f;
            } else if (enemyComp->enemyType == "BirdIdle" || enemyComp->enemyType.find("Bird") != std::string::npos) {
                // ECHELON V-FORMATION: Birds spawn in symmetric V-shape above pipes
                // Base Y at 15% screen height (above toilets/obstacles)
                float echelonBaseY = screenInfo.pixelHeight * 0.15f;
                
                // SYMMETRIC V-FORMATION (Flying V pointing left):
                // Bird 0: Tip (front, center Y) - enters screen first
                // Bird 1: Upper wing 1 (behind tip, above)
                // Bird 2: Lower wing 1 (behind tip, below)
                // Bird 3: Upper wing 2 (furthest back, highest) - triggers wrap
                // Bird 4: Lower wing 2 (furthest back, lowest) - triggers wrap
                int posInV = i % 5;
                float xOffset = 0.0f;
                float yOffset = 0.0f;
                
                int groupIndex = i / 5;
                
                // CONSISTENT V-FORMATION: tip at front, wings trailing behind
                // WIDER SPREAD: Using larger Y offsets for visible formation
                switch (posInV) {
                    case 0: xOffset = 0.0f;   yOffset = 0.0f; break;      // Tip (front center)
                    case 1: xOffset = 150.0f; yOffset = -100.0f; break;   // Upper wing 1
                    case 2: xOffset = 150.0f; yOffset = 100.0f; break;    // Lower wing 1
                    case 3: xOffset = 300.0f; yOffset = -200.0f; break;   // Upper wing 2 (trailing)
                    case 4: xOffset = 300.0f; yOffset = 200.0f; break;    // Lower wing 2 (trailing)
                }
                
                // FINAL POSITION includes offset; STORED baseY is the RAW echelon center
                y = echelonBaseY + yOffset;
                
                // CRITICAL: Store the RAW echelonBaseY (without offset) for EnemySystem
                // EnemySystem will add the offset based on posInV
                enemyComp->baseY = echelonBaseY;
                enemyComp->hasInitializedBaseY = true;
                
                // X SPACING: Base 650px offscreen + bird-specific offset + group offset
                float groupSpacing = 800.0f; // Space between groups (matches wrap spacing)
                x = screenInfo.pixelWidth + 650.0f + xOffset + (groupIndex * groupSpacing);
                
                // Store formation offset and Group info
                enemyComp->formationOffset = Gnosis::GNVector2(xOffset, yOffset);
                enemyComp->groupId = groupIndex;
                enemyComp->posInV = posInV;
                enemyComp->vFormationPointsDown = true; // Always use V-down style
                
                GN_LOG_INFO("[ECHELON_SPAWN] Bird " + std::to_string(i) + " posInV=" + std::to_string(posInV) 
                           + " at X=" + std::to_string(x) + ", Y=" + std::to_string(y) 
                           + " (offset: " + std::to_string(xOffset) + ", " + std::to_string(yOffset) + ")"
                           + " baseY=" + std::to_string(echelonBaseY));
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
            // Use custom hitboxRadius if set, otherwise calculate from width
            hitbox->radius = (config.hitboxRadius > 0.0f) ? config.hitboxRadius : (config.width * 0.4f);
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
        // SKIP echelon birds - their baseY is already set to raw center in echelon spawn code
        if (enemyComp && enemyComp->movementPattern != "echelon") {
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

        // CRITICAL: For Level 2 (Galaga) and Level 5 (Castle), extra enemies spawned at pipe 25/50 should be removed
        // When player dies, they start at pipe 0, so only 1 enemy should be active
        int maxEnemiesForLevel = 99; // Default: keep all enemies
        if (m_currentLevelId == 2 || m_currentLevelId == 5) {
            maxEnemiesForLevel = 1; // Level 2 and 5 start with only 1 enemy
            GN_LOG_INFO("[RESET] Level " + std::to_string(m_currentLevelId) + " detected - reducing active enemies to " + std::to_string(maxEnemiesForLevel));
        }
        
        // Reset all active enemies - keep them VISIBLE and scrolling like obstacles
        // CRITICAL: Space them out horizontally, do NOT stack them!
        // EXCEPTION: Skip Rat King boss (managed by BossSystem), but REMOVE rat minions spawned during battle
        const float ENEMY_SPACING = 800.0f; // Generous spacing between enemies
        int enemyIndex = 0;
        
        std::vector<Entity> enemiesToRemove; // Track enemies to return to pool
        
        for (Entity enemy : m_activeEnemies) {
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
            Transform* transform = m_ecsSystem->GetComponent<Transform>(enemy);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);

            if (enemyComp && transform) {
                // Skip the Rat King boss - he's managed by BossSystem
                if (enemyComp->enemyType == "Ratking" || enemyComp->enemyType == "RatKing") {
                    GN_LOG_INFO("[RESET] Skipping Rat King boss entity " + std::to_string(enemy) + " (managed by BossSystem)");
                    continue;
                }
                
                // Remove rat minions spawned during boss battle (Rat enemy type with "grounded" pattern)
                // Also remove boss-spawned RatCopters (they have isBossMinion flag set)
                if ((enemyComp->enemyType == "Rat" && enemyComp->movementPattern == "grounded") ||
                    (enemyComp->isBossMinion && (enemyComp->enemyType == "RatCopterIdle" || 
                     enemyComp->movementPattern == "flying"))) {
                    GN_LOG_INFO("[RESET] Marking boss minion " + std::to_string(enemy) + " for removal (type=" + enemyComp->enemyType + ")");
                    enemiesToRemove.push_back(enemy);
                    continue;
                }
                
                // CRITICAL: Remove extra enemies beyond initial count for level
                // This handles pipe 25/50 spawned extras that shouldn't persist on death
                if (enemyIndex >= maxEnemiesForLevel) {
                    GN_LOG_INFO("[RESET] Marking extra enemy " + std::to_string(enemy) + " for removal (index " + 
                               std::to_string(enemyIndex) + " >= max " + std::to_string(maxEnemiesForLevel) + ")");
                    enemiesToRemove.push_back(enemy);
                    enemyIndex++;
                    continue;
                }
                
                // Position offscreen to the right with proper spacing (like initial spawn)
                float offsetX = screenInfo.pixelWidth + 650.0f + (enemyIndex * ENEMY_SPACING);
                transform->position.x = offsetX;
                
                // CRITICAL: Recalculate Y position based on movement pattern
                if (enemyComp->movementPattern == "echelon") {
                    // ECHELON BIRDS: Preserve V-formation by using proper offsets
                    // Position groups offscreen with correct formation spacing
                    int groupIndex = enemyComp->groupId;
                    int posInV = enemyComp->posInV;
                    
                    // Calculate echelon base Y and offsets
                    float echelonBaseY = screenInfo.pixelHeight * 0.15f;
                    float xOffset = 0.0f;
                    float yOffset = 0.0f;
                    switch (posInV) {
                        case 0: xOffset = 0.0f;   yOffset = 0.0f; break;
                        case 1: xOffset = 150.0f; yOffset = -100.0f; break;
                        case 2: xOffset = 150.0f; yOffset = 100.0f; break;
                        case 3: xOffset = 300.0f; yOffset = -200.0f; break;
                        case 4: xOffset = 300.0f; yOffset = 200.0f; break;
                    }
                    
                    // Position in formation: group base X + bird offset
                    float groupBaseX = screenInfo.pixelWidth + 650.0f + (groupIndex * 800.0f);
                    transform->position.x = groupBaseX + xOffset;
                    transform->position.y = echelonBaseY + yOffset;
                    enemyComp->baseY = echelonBaseY;
                    enemyComp->currentState = EnemyState::Moving;
                    enemyComp->isGrounded = false;
                    
                    // Make sure bird is visible
                    if (sprite) {
                        sprite->visible = true;
                        sprite->color.a = 255;
                        sprite->playing = true;
                    }
                    enemyComp->health = 1;
                    
                    GN_LOG_INFO("[RESET] Echelon bird " + std::to_string(enemy) + " group=" + std::to_string(groupIndex) + 
                               " posInV=" + std::to_string(posInV) + " reset to (" + std::to_string(transform->position.x) + 
                               "," + std::to_string(transform->position.y) + ")");
                    
                    // Skip enemyIndex increment - echelon birds are spaced by formation, not generic spacing
                    continue;
                } else if (enemyComp->movementPattern == "flying" || enemyComp->movementPattern == "rat_swarm") {
                    // OTHER FLYING ENEMIES (RatCopter, etc.) and RAT_SWARM: Randomize Y position
                    float newY;
                    if (m_currentLevelId == 5 || m_currentLevelId == 6) {
                        // LANDSCAPE-AWARE: Boss level (6) is landscape, Castle level (5) is portrait
                        // Level 5 (Castle, Portrait): 30%-50% of 2556 = 766-1278
                        // Level 6 (Boss, Landscape): 50%-70% of 1179 = 589-825
                        float minY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.30f : 0.50f);
                        float maxY = screenInfo.pixelHeight * (screenInfo.isPortrait ? 0.50f : 0.70f);
                        newY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                        GN_LOG_INFO("[RESET] RatCopter/RatSwarm " + std::to_string(enemy) + " repositioned L" + std::to_string(m_currentLevelId) + ": screenH=" + 
                                   std::to_string(screenInfo.pixelHeight) + ", isPortrait=" + 
                                   std::to_string(screenInfo.isPortrait) + ", Y=" + std::to_string(newY) + 
                                   " (" + std::to_string((newY/screenInfo.pixelHeight)*100.0f) + "% band)");
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
                if (enemyComp->movementPattern == "flying" || enemyComp->movementPattern == "horizontal" || enemyComp->movementPattern == "rat_swarm") {
                    // rat_swarm uses FlyIn state like flying pattern
                    enemyComp->currentState = (enemyComp->movementPattern == "horizontal") ? EnemyState::Moving : EnemyState::FlyIn;
                    enemyComp->isGrounded = false; // CRITICAL: Ensure flying/horizontal enemies are NOT grounded!
                    enemyComp->hoverTimer = 0.0f;
                    enemyComp->hasLockedDirection = false;
                    enemyComp->pullbackTimer = 0.0f;
                    enemyComp->beelineSpeed = 0.0f;
                    enemyComp->targetDirection = Gnosis::GNVector2(0.0f, 0.0f);
                    enemyComp->pullbackVector = Gnosis::GNVector2(0.0f, 0.0f);
                    GN_LOG_INFO("[RESET] Flying/horizontal/rat_swarm enemy " + enemyComp->enemyType + " " + std::to_string(enemy) + " reset, isGrounded=false, pattern=" + enemyComp->movementPattern);
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
        
        // Remove rat minions from active enemies
        for (Entity ratMinion : enemiesToRemove) {
            m_activeEnemies.erase(
                std::remove(m_activeEnemies.begin(), m_activeEnemies.end(), ratMinion),
                m_activeEnemies.end()
            );
            
            // Deactivate and hide the rat minion
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(ratMinion);
            if (enemyComp) {
                enemyComp->isActive = false;
            }
            
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(ratMinion);
            if (sprite) {
                sprite->visible = false;
            }
            
            // Move offscreen
            Transform* transform = m_ecsSystem->GetComponent<Transform>(ratMinion);
            if (transform) {
                transform->position.x = -5000.0f;
                transform->position.y = -5000.0f;
            }
            
            GN_LOG_INFO("[RESET] Removed rat minion entity " + std::to_string(ratMinion));
        }
        
        int enemyCount = static_cast<int>(m_activeEnemies.size());
        int removedCount = static_cast<int>(enemiesToRemove.size());

        GN_LOG_INFO("[RESET] " + std::to_string(enemyCount) + " enemies reset (removed " + 
                   std::to_string(removedCount) + " rat minions) with " + 
                   std::to_string(ENEMY_SPACING) + "px spacing - Y positions PRESERVED, kept VISIBLE and ACTIVE");
    }

    // ============================================================================
    // GROUP MANIFEST SYSTEM IMPLEMENTATION (NEW ORCHESTRATOR PATTERN)
    // ============================================================================
    
    GroupManifest* LevelManager::GetGroupManifest(int groupId) {
        auto it = m_groupManifests.find(groupId);
        if (it != m_groupManifests.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    
    const GroupManifest* LevelManager::GetGroupManifest(int groupId) const {
        auto it = m_groupManifests.find(groupId);
        if (it != m_groupManifests.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    
    GroupManifest* LevelManager::CreateGroupManifest(int groupId) {
        // Create or overwrite manifest for this group
        m_groupManifests[groupId] = GroupManifest();
        m_groupManifests[groupId].groupId = groupId;
        
        GN_LOG_DEBUG("[GroupManifest] Created manifest for group " + std::to_string(groupId));
        return &m_groupManifests[groupId];
    }
    
    void LevelManager::UpdateGroupMemberPositions(int groupId) {
        GroupManifest* manifest = GetGroupManifest(groupId);
        if (!manifest || manifest->leaderEntity == 0) {
            GN_LOG_WARN("[GroupManifest] Cannot update positions for group " + std::to_string(groupId) + " - invalid manifest");
            return;
        }
        
        // Get leader position
        Transform* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
        if (!leaderTransform) {
            GN_LOG_WARN("[GroupManifest] Cannot update positions for group " + std::to_string(groupId) + " - leader has no Transform");
            return;
        }
        
        float leaderX = leaderTransform->position.x;
        
        // Update all member positions based on their offsets
        for (const auto& member : manifest->allMembers) {
            if (member.entity == 0) continue;
            
            Transform* memberTransform = m_ecsSystem->GetComponent<Transform>(member.entity);
            if (memberTransform) {
                memberTransform->position.x = leaderX + member.offsetX;
                memberTransform->position.y = member.offsetY;  // Y is absolute, not offset from leader
            }
        }
        
        GN_LOG_DEBUG("[GroupManifest] Updated " + std::to_string(manifest->allMembers.size()) + 
                    " member positions for group " + std::to_string(groupId));
    }
    
    bool LevelManager::ValidateGroupIntegrity(int groupId) const {
        const GroupManifest* manifest = GetGroupManifest(groupId);
        if (!manifest) {
            GN_LOG_WARN("[GroupManifest] Group " + std::to_string(groupId) + " has no manifest");
            return false;
        }
        
        // Check leader exists
        if (manifest->leaderEntity == 0) {
            GN_LOG_WARN("[GroupManifest] Group " + std::to_string(groupId) + " has no leader");
            return false;
        }
        
        // Check all members exist
        int invalidMembers = 0;
        for (const auto& member : manifest->allMembers) {
            if (member.entity == 0) {
                invalidMembers++;
            }
        }
        
        if (invalidMembers > 0) {
            GN_LOG_WARN("[GroupManifest] Group " + std::to_string(groupId) + " has " + 
                          std::to_string(invalidMembers) + " invalid members");
        }
        
        return invalidMembers == 0;
    }
    
    float LevelManager::GetRightmostGroupPosition() const {
        float rightmost = 0.0f;
        
        for (const auto& pair : m_groupManifests) {
            const GroupManifest& manifest = pair.second;
            if (manifest.leaderEntity == 0) continue;
            
            Transform* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest.leaderEntity);
            if (leaderTransform) {
                float groupRightEdge = leaderTransform->position.x + manifest.rightmostMemberOffsetX;
                if (groupRightEdge > rightmost) {
                    rightmost = groupRightEdge;
                }
            }
        }
        
        return rightmost;
    }
    
    int LevelManager::GetNextGroupId() const {
        return m_nextGroupId;
    }
    
    // ============================================================================
    // ORCHESTRATOR METHODS (COMPOSITION PATTERN)
    // ============================================================================
    
    void LevelManager::SpawnGroup(int levelId, int groupId, float worldX, GroupPattern pattern) {
        GN_LOG_INFO("[Orchestrator] SpawnGroup: level=" + std::to_string(levelId) + 
                   ", groupId=" + std::to_string(groupId) + ", worldX=" + std::to_string(worldX));
        
        // Create manifest for this group
        GroupManifest* manifest = CreateGroupManifest(groupId);
        manifest->pattern = pattern;
        manifest->baseRenderLayer = 3;  // Default obstacle layer
        
        // Set level-specific gap widths
        // NOTE: Sewer level uses dynamic positioning based on group width, not fixed gap
        switch(levelId) {
            case 1: // Park
                manifest->gapWidth = 330.0f;  // Tighter spacing for park
                break;
            case 2: // Sewer - DYNAMIC: gap calculated from actual group width (see SpawnInitialGroups)
                manifest->gapWidth = 200.0f;  // Minimal spacing between varied-width patterns
                break;
            case 3: // Desert
                manifest->gapWidth = 928.0f;  // Increased by 128px for more coin/brick wall spacing
                break;
            case 4: // Snow
                manifest->gapWidth = 1000.0f;  // Balanced gap for coin spread without feeling too far apart
                break;
            case 5: // Castle
                manifest->gapWidth = 1400.0f;  // Increased from 1200 for centerpiece visibility
                break;
            default:
                manifest->gapWidth = 400.0f;
        }
        
        // Set toilet width
        manifest->toiletWidth = 64.0f * 8.0f;  // 64px sprite * 8.0f baseScale
        
        // Step 1: Spawn obstacles based on level
        std::vector<Gnosis::Entity> obstacles;
        
        switch (levelId) {
            case 1: { // Park
                auto toiletPair = m_obstacleSystem->SpawnParkPattern_ToiletPair(worldX, groupId);
                obstacles.push_back(toiletPair.first);
                obstacles.push_back(toiletPair.second);
                manifest->leaderEntity = toiletPair.first;
                break;
            }
            case 2: { // Sewer - random pattern
                int randomPattern = rand() % 6;
                switch (randomPattern) {
                    case 0:
                        obstacles = m_obstacleSystem->SpawnSewerPattern_TopOnly(worldX, groupId);
                        break;
                    case 1:
                        obstacles = m_obstacleSystem->SpawnSewerPattern_BottomOnly(worldX, groupId);
                        break;
                    case 2:
                        obstacles = m_obstacleSystem->SpawnSewerPattern_TopAndBottom(worldX, groupId);
                        break;
                    case 3:
                        obstacles = m_obstacleSystem->SpawnSewerPattern_Pyramid3(worldX, groupId);
                        break;
                    case 4:
                        obstacles = m_obstacleSystem->SpawnSewerPattern_PyramidTop3(worldX, groupId);
                        break;
                    case 5:
                        obstacles = m_obstacleSystem->SpawnSewerPattern_TwoByTwoFunnel(worldX, groupId);
                        break;
                }
                manifest->leaderEntity = obstacles.empty() ? 0 : obstacles[0];
                
                // CRITICAL FIX: Detect actual pattern from spawned obstacles (they set their own Group::pattern)
                // instead of using the hardcoded parameter, since sewer has varied patterns
                if (!obstacles.empty()) {
                    Group* leaderGroup = m_ecsSystem->GetComponent<Group>(obstacles[0]);
                    if (leaderGroup) {
                        pattern = leaderGroup->pattern;
                        manifest->pattern = pattern;
                        GN_LOG_INFO("[Orchestrator] Sewer group " + std::to_string(groupId) + " detected pattern: " + std::to_string(static_cast<int>(pattern)));
                    }
                }
                break;
            }
            case 3: { // Desert
                obstacles = m_obstacleSystem->SpawnDesertPattern_Outhouse(worldX, groupId);
                manifest->leaderEntity = obstacles.empty() ? 0 : obstacles[0];
                break;
            }
            case 4: { // Snow
                auto toiletPair = m_obstacleSystem->SpawnSnowPattern_ToiletPair(worldX, groupId, manifest->gapWidth);
                obstacles.push_back(toiletPair.first);
                obstacles.push_back(toiletPair.second);
                manifest->leaderEntity = toiletPair.first;
                break;
            }
            case 5: { // Castle
            std::vector<Gnosis::Entity> castleEntities = m_obstacleSystem->SpawnCastlePattern_GoldToiletPair(worldX, groupId, manifest->gapWidth);
            for (Gnosis::Entity ent : castleEntities) {
                obstacles.push_back(ent);
            }
            // First entity is always the top toilet -> leader
            manifest->leaderEntity = castleEntities.empty() ? 0 : castleEntities[0];
            break;
        }
            default:
                GN_LOG_ERROR("[Orchestrator] Unknown levelId: " + std::to_string(levelId));
                return;
        }
        
        // Step 2: Add obstacles to manifest with offsets
        Transform* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
        if (!leaderTransform) {
            GN_LOG_ERROR("[Orchestrator] Leader entity has no Transform");
            return;
        }
        
        float leaderX = leaderTransform->position.x;
        for (Gnosis::Entity obstacle : obstacles) {
            Transform* obstacleTransform = m_ecsSystem->GetComponent<Transform>(obstacle);
            if (obstacleTransform) {
                float offsetX = obstacleTransform->position.x - leaderX;
                float offsetY = obstacleTransform->position.y;
                manifest->allMembers.push_back(GroupMemberOffset(obstacle, offsetX, offsetY));
            }
        }
        manifest->obstacleCount = obstacles.size();
        
        // Step 3: Spawn coins if PickupSystem is available (pass gap width for accurate positioning)
        if (m_pickupSystem) {
            std::vector<Gnosis::Entity> coins = m_pickupSystem->SpawnCoinsForGroup(groupId, pattern, manifest->gapWidth);
            for (Gnosis::Entity coin : coins) {
                Transform* coinTransform = m_ecsSystem->GetComponent<Transform>(coin);
                if (coinTransform) {
                    float offsetX = coinTransform->position.x - leaderX;
                    float offsetY = coinTransform->position.y;
                    manifest->allMembers.push_back(GroupMemberOffset(coin, offsetX, offsetY));
                }
            }
            manifest->pickupCount = coins.size();
            GN_LOG_INFO("[Orchestrator] Spawned " + std::to_string(coins.size()) + " coins for groupId=" + std::to_string(groupId) + " with gapWidth=" + std::to_string(manifest->gapWidth));
        }
        
        // Step 4: Calculate bounds
        // CRITICAL: For sewer patterns, use Group component's groupWidth if available (matches old logic)
        // Otherwise calculate from actual positions
        manifest->boundsNeedRecalc = true;
        float leftmost = 0.0f;
        float rightmost = 0.0f;
        bool foundGroupWidth = false;
        float groupWidthFromComponent = 0.0f;
        
        // Try to get groupWidth from leader's Group component (sewer patterns set this)
        Group* leaderGroup = m_ecsSystem->GetComponent<Group>(manifest->leaderEntity);
        if (leaderGroup && leaderGroup->groupWidth > 0.0f) {
            groupWidthFromComponent = leaderGroup->groupWidth;
            foundGroupWidth = true;
            rightmost = groupWidthFromComponent;  // Group width is the span from leader to rightmost
            GN_LOG_INFO("[Orchestrator] Using Group component groupWidth=" + std::to_string(groupWidthFromComponent) + " for groupId=" + std::to_string(groupId));
        }
        
        // Also calculate from actual positions (for non-sewer patterns or verification)
        for (const auto& member : manifest->allMembers) {
            Transform* t = m_ecsSystem->GetComponent<Transform>(member.entity);
            Sprite* s = m_ecsSystem->GetComponent<Sprite>(member.entity);
            if (t && s) {
                float memberLeft = member.offsetX;
                float memberRight = member.offsetX + (s->width * std::abs(t->scale.x));
                
                if (memberLeft < leftmost) leftmost = memberLeft;
                if (!foundGroupWidth && memberRight > rightmost) {
                    rightmost = memberRight;
                }
            }
        }
        
        // If we didn't find groupWidth in component, use calculated value
        if (!foundGroupWidth) {
            rightmost = std::max(rightmost, 0.0f);
        }
        
        manifest->leftBound = leftmost;
        manifest->rightBound = rightmost;
        manifest->rightmostMemberOffsetX = rightmost;  // This is the RIGHT EDGE offset from leader
        manifest->boundsNeedRecalc = false;
        
        GN_LOG_INFO("[Orchestrator] SpawnGroup complete: groupId=" + std::to_string(groupId) + 
                   ", obstacles=" + std::to_string(manifest->obstacleCount) + 
                   ", pickups=" + std::to_string(manifest->pickupCount) + 
                   ", bounds=[" + std::to_string(leftmost) + ", " + std::to_string(rightmost) + "]");
    }
    
    void LevelManager::SpawnInitialGroups(int levelId) {
        GN_LOG_INFO("[Orchestrator] SpawnInitialGroups for level " + std::to_string(levelId));
        
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float nextWorldX = screenInfo.pixelWidth + 100.0f;
        const int INITIAL_GROUP_COUNT = 32;  // Match OBSTACLE_POOL_SIZE
        
        // Determine pattern for this level
        GroupPattern pattern = GroupPattern::TopAndBottom;  // Default
        switch (levelId) {
            case 1: pattern = GroupPattern::TopAndBottom; break;  // Park
            case 2: pattern = GroupPattern::TopAndBottom; break;  // Sewer (will vary)
            case 3: pattern = GroupPattern::Ground; break;         // Desert - ground-based
            case 4: pattern = GroupPattern::SnowScreenEdges; break;  // Snow - top/bottom screen strips
            case 5: pattern = GroupPattern::TopAndBottom; break;  // Castle
            case 6: return;  // Boss level - no obstacle groups
        }
        
        for (int i = 0; i < INITIAL_GROUP_COUNT; i++) {
            int groupId = GetNextGroupId();
            m_nextGroupId++;
            
            GN_LOG_INFO("[Orchestrator] Spawning initial group " + std::to_string(i) + 
                       " (groupId=" + std::to_string(groupId) + ") at x=" + std::to_string(nextWorldX));
            
            // Spawn the group using orchestrator
            SpawnGroup(levelId, groupId, nextWorldX, pattern);
            
            // Get the manifest to calculate next position
            GroupManifest* manifest = GetGroupManifest(groupId);
            if (manifest) {
                // rightmostMemberOffsetX is the right EDGE offset from leader (includes sprite width)
                // So if leader is at X, the right edge is at X + rightmostMemberOffsetX
                // Next group should start at: right edge + gap = X + rightmostMemberOffsetX + gap
                // Since nextWorldX is currently the leader X, we add rightmostMemberOffsetX + gap
                float rightEdgeOffset = manifest->rightmostMemberOffsetX;
                nextWorldX = nextWorldX + rightEdgeOffset + manifest->gapWidth;
                GN_LOG_INFO("[Orchestrator] Next group will spawn at x=" + std::to_string(nextWorldX) + 
                           " (rightEdgeOffset=" + std::to_string(rightEdgeOffset) + 
                           ", gap=" + std::to_string(manifest->gapWidth) + 
                           ", leaderWasAt=" + std::to_string(nextWorldX - rightEdgeOffset - manifest->gapWidth) + ")");
            } else {
                // Fallback
                nextWorldX += 800.0f;
                GN_LOG_ERROR("[Orchestrator] Manifest not found for groupId=" + std::to_string(groupId) + ", using fallback spacing");
            }
        }
        
        GN_LOG_INFO("[Orchestrator] SpawnInitialGroups complete: spawned " + std::to_string(INITIAL_GROUP_COUNT) + " groups");
    }
    
    void LevelManager::OnGroupOffScreen(int groupId) {
        GN_LOG_DEBUG("[Orchestrator] OnGroupOffScreen: groupId=" + std::to_string(groupId));
        
        // Notify that group needs wrapping
        WrapGroup(groupId);
    }
    
    void LevelManager::WrapGroup(int groupId) {
        GN_LOG_INFO("[Orchestrator] WrapGroup: groupId=" + std::to_string(groupId));
        
        GroupManifest* manifest = GetGroupManifest(groupId);
        if (!manifest) {
            GN_LOG_ERROR("[Orchestrator] Cannot wrap group " + std::to_string(groupId) + " - no manifest found");
            return;
        }
        
        if (manifest->leaderEntity == 0) {
            GN_LOG_ERROR("[Orchestrator] Cannot wrap group " + std::to_string(groupId) + " - no leader entity");
            return;
        }
        
        // Step 1: Find rightmost group position  
        float rightmost = GetRightmostGroupPosition();
        
        // Step 2: Calculate new position for this group
        float newX = rightmost + manifest->gapWidth;
        
        GN_LOG_INFO("[Orchestrator] Wrapping group " + std::to_string(groupId) + 
                   " to newX=" + std::to_string(newX) + 
                   " (rightmost=" + std::to_string(rightmost) + ", gap=" + std::to_string(manifest->gapWidth) + ")");
        
        // Step 3: Update leader position
        Transform* leaderTransform = m_ecsSystem->GetComponent<Transform>(manifest->leaderEntity);
        if (leaderTransform) {
            leaderTransform->position.x = newX;
            
            // Step 4: Update all member positions based on their offsets
            UpdateGroupMemberPositions(groupId);
            
            // Step 5: Reset obstacle state (pipeCleared, etc.) and respawn pickups
            // Destroy old pickups and spawn fresh ones for this group
            std::vector<Gnosis::Entity> oldPickups;
            for (const auto& member : manifest->allMembers) {
                Obstacle* obstacle = m_ecsSystem->GetComponent<Obstacle>(member.entity);
                if (obstacle) {
                    obstacle->pipeCleared = false;  // Reset for reuse
                }
                
                // Collect pickups to destroy
                Pickup* pickup = m_ecsSystem->GetComponent<Pickup>(member.entity);
                if (pickup) {
                    oldPickups.push_back(member.entity);
                }
            }
            
            // Destroy old pickups (they may have been collected)
            for (Gnosis::Entity pickup : oldPickups) {
                m_ecsSystem->DestroyEntity(pickup);
            }
            
            // Respawn fresh pickups for this group
            if (m_pickupSystem && manifest->pickupCount > 0) {
                std::vector<Gnosis::Entity> newPickups = m_pickupSystem->SpawnCoinsForGroup(
                    groupId, 
                    manifest->pattern, 
                    manifest->gapWidth
                );
                
                // Add new pickups to manifest
                for (Gnosis::Entity coin : newPickups) {
                    Transform* coinTransform = m_ecsSystem->GetComponent<Transform>(coin);
                    if (coinTransform) {
                        float offsetX = coinTransform->position.x - newX;
                        float offsetY = coinTransform->position.y;
                        manifest->allMembers.push_back(GroupMemberOffset(coin, offsetX, offsetY));
                    }
                }
                manifest->pickupCount = newPickups.size();
                
                GN_LOG_INFO("[Orchestrator] Respawned " + std::to_string(newPickups.size()) + 
                           " fresh pickups for group " + std::to_string(groupId));
            }
            
            // Step 6: Level-specific randomization on wrap
            if (m_currentLevelId == 2) {
                // SEWER LEVEL: Randomize pattern variation on wrap
                // Destroy old pipes and spawn new random pattern
                std::vector<Gnosis::Entity> oldPipes;
                for (const auto& member : manifest->allMembers) {
                    if (m_ecsSystem->HasComponent<Obstacle>(member.entity)) {
                        oldPipes.push_back(member.entity);
                    }
                }
                
                // Destroy old pipes
                for (Gnosis::Entity pipe : oldPipes) {
                    m_ecsSystem->DestroyEntity(pipe);
                }
                
                // Spawn new random sewer pattern at the new position
                // Random patterns: TopOnly, BottomOnly, TopAndBottom, Pyramid3, PyramidTop3, TwoByTwoFunnel
                int randomPattern = rand() % 6;
                std::vector<Gnosis::Entity> newPipes;
                
                switch (randomPattern) {
                    case 0:
                        newPipes = m_obstacleSystem->SpawnSewerPattern_TopOnly(newX, groupId);
                        manifest->pattern = GroupPattern::TopOnly;
                        break;
                    case 1:
                        newPipes = m_obstacleSystem->SpawnSewerPattern_BottomOnly(newX, groupId);
                        manifest->pattern = GroupPattern::BottomOnly;
                        break;
                    case 2:
                        newPipes = m_obstacleSystem->SpawnSewerPattern_TopAndBottom(newX, groupId);
                        manifest->pattern = GroupPattern::TopAndBottom;
                        break;
                    case 3:
                        newPipes = m_obstacleSystem->SpawnSewerPattern_Pyramid3(newX, groupId);
                        manifest->pattern = GroupPattern::PyramidBottom;
                        break;
                    case 4:
                        newPipes = m_obstacleSystem->SpawnSewerPattern_PyramidTop3(newX, groupId);
                        manifest->pattern = GroupPattern::PyramidTop;
                        break;
                    case 5:
                        newPipes = m_obstacleSystem->SpawnSewerPattern_TwoByTwoFunnel(newX, groupId);
                        manifest->pattern = GroupPattern::TwoFunnel;
                        break;
                }
                
                // Update manifest with new pipes
                manifest->allMembers.clear();
                manifest->leaderEntity = newPipes.empty() ? 0 : newPipes[0];
                for (Gnosis::Entity pipe : newPipes) {
                    Transform* pipeTransform = m_ecsSystem->GetComponent<Transform>(pipe);
                    if (pipeTransform) {
                        float offsetX = pipeTransform->position.x - newX;
                        float offsetY = pipeTransform->position.y;
                        manifest->allMembers.push_back(GroupMemberOffset(pipe, offsetX, offsetY));
                    }
                }
                manifest->obstacleCount = newPipes.size();
                manifest->boundsNeedRecalc = true;
                
                GN_LOG_INFO("[Orchestrator] Sewer group " + std::to_string(groupId) + 
                           " randomized to pattern " + std::to_string(static_cast<int>(manifest->pattern)));
            }
            else if (m_currentLevelId == 5) {
                // CASTLE LEVEL: Randomize centerpiece painting on wrap
                // Find painting decoration in manifest
                for (const auto& member : manifest->allMembers) {
                    Decoration* deco = m_ecsSystem->GetComponent<Decoration>(member.entity);
                    if (deco && deco->type >= DecorationType::PaintingRabbitKnight && 
                        deco->type <= DecorationType::PaintingCabin) {
                        // Randomize painting texture
                        const char* paintingTextures[] = {
                            "RabbitKnightPainting",
                            "RatBeachPainting",
                            "RiverWalkPainting",
                            "CabinPainting"
                        };
                        int randomPaintingIndex = rand() % 4;
                        
                        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(member.entity);
                        if (sprite) {
                            sprite->textureId = paintingTextures[randomPaintingIndex];
                            GN_LOG_INFO("[Orchestrator] Castle group " + std::to_string(groupId) + 
                                       " painting randomized to " + std::string(paintingTextures[randomPaintingIndex]));
                        }
                        break;  // Only one painting per group
                    }
                }
            }
            
            GN_LOG_INFO("[Orchestrator] Group " + std::to_string(groupId) + " wrapped successfully");
        } else {
            GN_LOG_ERROR("[Orchestrator] Leader entity " + std::to_string(manifest->leaderEntity) + " has no Transform");
        }
    }

} // namespace GameCore
