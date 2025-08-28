#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../Config/EnemyConfigs.h"
#include <algorithm>
#include <cmath>
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

    void LevelManager::InitializeEnemyPool() {
        if (m_enemyPoolInitialized) return;
        if (!m_isLoaded || !m_currentLevelConfig.enableEnemies || m_currentLevelConfig.enemies.empty()) return;

        // Spawn all enemy types for the level
        float screenW = 1179.0f; // Avoid async delegate to prevent dangling pointer crash
        float startX = screenW + 200.0f;
        
        // For snow level (level 4), spawn all snowman types
        if (m_currentLevelId == 4) {
            // Spawn decorative snowmen first - GROUND THEM at bottom of screen
            float screenHeight = 2556.0f; // iPhone 16 portrait height
            float snowmanHeight = 64.0f; // Snowman sprite height
            float groundY = screenHeight - snowmanHeight; // Ground level - snowmen bottom edge at screen bottom
            
            for (int i = 0; i < 3; ++i) { // Spawn 3 decorative snowmen
                const EnemyConfig& cfg = m_currentLevelConfig.enemies[i]; // SnowManChill, SnowManGreen, SnowManChad
                float x = startX + i * 300.0f; // Space them out horizontally
                float baseY = groundY; // Ground level for snowmen
                
                GN_LOG_DEBUG("LevelManager: Spawning decorative snowman " + std::to_string(i) + " at x=" + std::to_string(x) + " baseY=" + std::to_string(baseY) + " (groundY=" + std::to_string(groundY) + ")");
                
                Gnosis::Entity e = SpawnEnemy(cfg, x, baseY);
                if (e != 0) { 
                    m_activeEnemies.push_back(e); 
                    m_enemyBaseY[e] = baseY; 
                    GN_LOG_DEBUG("Snow level enemy init: " + cfg.textureId + " baseY=" + std::to_string(baseY) + ", x=" + std::to_string(x) + " entity=" + std::to_string(e)); 
                }
            }
            
            // Spawn the red snowman thrower
            const EnemyConfig& throwerCfg = m_currentLevelConfig.enemies[3]; // SnowManIdle
            float throwerX = startX + 900.0f; // Further to the right
            float throwerBaseY = groundY; // Same ground level
            
            GN_LOG_DEBUG("LevelManager: Spawning red snowman thrower at x=" + std::to_string(throwerX) + " baseY=" + std::to_string(throwerBaseY) + " (groundY=" + std::to_string(groundY) + ")");
            
            Gnosis::Entity thrower = SpawnEnemy(throwerCfg, throwerX, throwerBaseY);
            if (thrower != 0) { 
                m_activeEnemies.push_back(thrower); 
                m_enemyBaseY[thrower] = throwerBaseY; 
                GN_LOG_DEBUG("Snow level thrower init: " + throwerCfg.textureId + " baseY=" + std::to_string(throwerBaseY) + ", x=" + std::to_string(throwerX) + " entity=" + std::to_string(thrower)); 
            }
        } else {
            // Original logic for other levels
            // For Level 2 we only configured ToiletPaperFlap; still create up to 4 entities spaced to the right
            const EnemyConfig& cfg = m_currentLevelConfig.enemies[0];
            // Reduce concurrent enemies a bit for sewers
            int desired = (m_currentLevelId == 2 ? 3 : m_maxActiveEnemies);
            for (int i = 0; i < desired; ++i) {
                float x = startX + i * (m_enemySpacing * 1.25f);
                float baseY;
                
                if (m_currentLevelId == 3) { // Desert level - spread birds more vertically
                    // Spread birds from middle of screen to near top, avoiding the very top
                    float minY = 400.0f; // Middle of screen
                    float maxY = 1200.0f; // Near top but not at very top
                    float range = maxY - minY;
                    baseY = minY + (range * (i + 1)) / (desired + 1); // Even distribution
                } else if (m_currentLevelId == 5) { // Castle level - spread RatCopters across screen
                    // Spread RatCopters from middle to upper portion of screen
                    float minY = 600.0f; // Middle of screen
                    float maxY = 1400.0f; // Upper portion but not at very top
                    float range = maxY - minY;
                    baseY = minY + (range * (i + 1)) / (desired + 1); // Even distribution
                } else { // Other levels - original logic for non-grounded enemies
                    baseY = 900.0f + static_cast<float>((i%2==0? -1:1) * 150);
                }
                
                Gnosis::Entity e = SpawnEnemy(cfg, x, baseY);
                if (e != 0) { 
                    m_activeEnemies.push_back(e); 
                    m_enemyBaseY[e] = baseY; 
                    GN_LOG_DEBUG("Enemy init: " + cfg.textureId + " baseY=" + std::to_string(baseY) + ", x=" + std::to_string(x) + ", level=" + std::to_string(m_currentLevelId)); 
                }
            }
        }
        m_enemyPoolInitialized = true;
    }

    void LevelManager::InitializeNPCPool() {
        if (m_npcPoolInitialized) return;
        if (!m_isLoaded || !m_currentLevelConfig.enableNPCs) return;
        if (m_currentLevelId != 2) { m_npcPoolInitialized = true; return; }

        // Ensure single Janitor entity
        float screenW = 1179.0f, screenH = 2556.0f; // Use fixed iPhone 16 portrait metrics
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
        // Wrap enemies when off-screen left, reusing pool
        float screenW = 1179.0f;
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
            // Wait until enemy is completely off screen before wrapping (not just touching edge)
            if (rightEdge < -widthPx) {
                t->position.x = rightmostX + (m_enemySpacing * 1.25f);
                
                // Get enemy component to check if it should be grounded
                Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(e);
                float baseY;
                
                if (enemyComp && enemyComp->isGrounded) {
                    // For grounded enemies, calculate proper ground position
                    // Use the same logic as EnemySystem::GroundEnemy
                    const float screenHeight = 2556.0f; // iPhone 16 portrait screen height
                    float enemyHeight = 64.0f * std::abs(t->scale.y); // Snowman height with scale
                    baseY = screenHeight - enemyHeight;
                } else if (m_currentLevelId == 3) { // Desert level - maintain vertical spread
                    // Random Y within the desert bird range for flying enemies
                    float minY = 400.0f;
                    float maxY = 1200.0f;
                    baseY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                } else if (m_currentLevelId == 5) { // Castle level - maintain RatCopter vertical spread
                    // Maintain RatCopter vertical spread when wrapping
                    float minY = 600.0f;
                    float maxY = 1400.0f;
                    baseY = minY + static_cast<float>(rand() % static_cast<int>(maxY - minY));
                } else { // Other levels - original logic for non-grounded enemies
                    baseY = 900.0f + static_cast<float>((rand()%300) - 150);
                }
                
                t->position.y = baseY;
                // Sync Enemy component's bobbing anchor with new wrap position
                if (enemyComp) {
                    enemyComp->baseY = baseY;
                    enemyComp->hasInitializedBaseY = true;
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

        // Continuously match Janitor speed to the sewer background band speed
        // via CameraSystem-controlled ScrollSpeed component.
        float targetSpeed = m_currentLevelConfig.worldSpeed * 0.35f;
        for (const auto& layer : m_currentLevelConfig.backgroundLayers) {
            if (layer.scrollSpeed > targetSpeed) targetSpeed = layer.scrollSpeed;
        }
        if (!m_ecsSystem->HasComponent<ScrollSpeed>(m_janitorEntity)) {
            m_ecsSystem->AddComponent<ScrollSpeed>(m_janitorEntity, ScrollSpeed(targetSpeed));
        } else {
            auto scr = m_ecsSystem->GetComponent<ScrollSpeed>(m_janitorEntity);
            scr->speed = targetSpeed;
        }
        // Remove direct per-frame velocity movement to avoid double scroll; CameraSystem now moves him.
        float screenW = 1179.0f, screenH = 2556.0f;
        float rightEdge = t->position.x + s->width * std::abs(t->scale.x);
        if (rightEdge < 0.0f) {
            float janitorHeight = 64.0f * m_currentLevelConfig.baseScale;
            t->position.x = screenW + 220.0f;
            // Keep Y aligned to sewer background band: 20 px at background pixel scale (~5x)
            float offsetPx = 20.0f * 5.0f;
            t->position.y = screenH - janitorHeight - offsetPx;
            GN_LOG_INFO("NPC Janitor wrap: newX=" + std::to_string(t->position.x) + ", newY=" + std::to_string(t->position.y));
            // Re-apply speed on wrap in case difficulty changed; use fastest parallax speed
            float targetSpeed = m_currentLevelConfig.worldSpeed * 0.65f;
            for (const auto& layer : m_currentLevelConfig.backgroundLayers) {
                if (layer.scrollSpeed > targetSpeed) targetSpeed = layer.scrollSpeed;
            }
            Physics* phComp = m_ecsSystem->GetComponent<Physics>(m_janitorEntity);
            if (phComp) phComp->velocity.x = -targetSpeed;
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
        sp.layer = 1; // behind pipes for parallax feel
        sp.visible = true;
        sp.isAnimated = true;
        sp.frameWidth = 64; sp.frameHeight = 64;
        sp.frameCount = 4;
        sp.frameTime = 0.3f; // slower sweep from the start (300ms)
        sp.currentFrame = 0;
        sp.playing = true;
        // Attach ScrollSpeed so CameraSystem moves the Janitor with the sewer background speed
        float janitorSpeed = m_currentLevelConfig.worldSpeed * 0.35f;
        for (const auto& layer : m_currentLevelConfig.backgroundLayers) {
            if (layer.scrollSpeed > janitorSpeed) janitorSpeed = layer.scrollSpeed;
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
            StateAnimation::Clip surprise; surprise.textureId = "JanitorSurprise"; surprise.frameWidth = 64; surprise.frameHeight = 64; surprise.frameCount = 8; surprise.frameTime = 0.3f; surprise.loop = false;
            sa.clips.push_back({"sweep", sweep});
            sa.clips.push_back({"surprise", surprise});
            sa.currentState = "sweep";
        }
        m_ecsSystem->AddComponent<StateAnimation>(npc, sa);
        m_ecsSystem->AddComponent<Physics>(npc, ph);
        m_ecsSystem->AddComponent<Hitbox>(npc, hb);
        m_ecsSystem->AddComponent<NPC>(npc, npcComp);
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
        
        // Create transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, 
                          Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
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
        
        // Create hitbox
        Hitbox collider;
        collider.isStatic = false;
        collider.width = enhancedConfig.width;
        collider.height = enhancedConfig.height;
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
                    // Percentage-based amplitude (like toilet paper: 33-43% of screen)
                    float screenH = 2556.0f;
                    float ampFactor = bobConfig.amplitudeMin + (static_cast<float>((rand() % 21) - 10) * 0.005f);
                    enemyComp.bobAmplitude = screenH * ampFactor;
                } else {
                    enemyComp.bobAmplitude = bobConfig.amplitude;
                }
                
                // Random starting phase 0..2π
                enemyComp.bobPhase = static_cast<float>((rand() % 628)) / 100.0f;
                
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
        
        // Clean up enemies that have moved off screen
        // FIXED: Use right edge of entity for proper cleanup, like toilet logic
        for (auto it = m_activeEnemies.begin(); it != m_activeEnemies.end();) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(*it);
            if (transform && sprite) {
                // Calculate right edge of enemy for proper cleanup
                float scaledWidth = sprite->width * std::abs(transform->scale.x);
                float rightEdge = transform->position.x + scaledWidth;
                if (rightEdge < leftBoundary) {
                    m_ecsSystem->DestroyEntity(*it);
                    it = m_activeEnemies.erase(it);
                } else {
                    ++it;
                }
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
            // Determine texture size directly from platform delegates (no static fallbacks)
            int tw = 0, th = 0;
            // Prefer renderer metadata (Metal knows actual pixel dimensions), then asset manager.
            bool got = false;
            if (m_platformDelegates.renderer.getTextureMetadata) {
                // Create a persistent TextureMetadata object that won't go out of scope
                static GameCore::TextureMetadata rendererMeta;
                if (m_platformDelegates.renderer.getTextureMetadata(layerConfig.textureId.c_str(), &rendererMeta)) {
                    tw = rendererMeta.width;
                    th = rendererMeta.height;
                    got = (tw > 0 && th > 0);
                    if (got) {
                        GN_LOG_INFO(std::string("Texture metadata (renderer) for '") + layerConfig.textureId +
                                    "': " + std::to_string(tw) + "x" + std::to_string(th));
                    }
                }
            }
            if (!got && m_platformDelegates.asset.getTextureMetadata) {
                // Create a persistent TextureMetadata object that won't go out of scope
                static GameCore::TextureMetadata assetMeta;
                if (m_platformDelegates.asset.getTextureMetadata(layerConfig.textureId.c_str(), &assetMeta)) {
                    tw = assetMeta.width;
                    th = assetMeta.height;
                    got = (tw > 0 && th > 0);
                    if (got) {
                        GN_LOG_INFO(std::string("Texture metadata (asset) for '") + layerConfig.textureId +
                                    "': " + std::to_string(tw) + "x" + std::to_string(th));
                    }
                }
            }
            if (tw <= 0 || th <= 0) {
                // Default texture dimensions based on level type
                // Sewer levels use 512x512, all other levels use 1024x512
                bool isSewerLevel = (layerConfig.textureId.find("Sewer") != std::string::npos);
                if (isSewerLevel) {
                    tw = 512; th = 512;
                    GN_LOG_WARN(std::string("getTextureMetadata failed for '") + layerConfig.textureId +
                                "' via renderer and asset delegates; defaulting to sewer size 512x512");
                } else {
                    tw = 1024; th = 512;
                    GN_LOG_WARN(std::string("getTextureMetadata failed for '") + layerConfig.textureId +
                                "' via renderer and asset delegates; defaulting to standard size 1024x512");
                }
            }
            float textureWidth = static_cast<float>(tw);
            float textureHeight = static_cast<float>(th);
            
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
                // Add optional variant support so the texture can change on wrap
                if (!layerConfig.variantTextureIds.empty()) {
                    ParallaxVariants variants(layerConfig.variantTextureIds);
                    m_ecsSystem->AddComponent<ParallaxVariants>(bgEntity, variants);
                }
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
        const float screenCenterX = 1179.0f * 0.5f; // fallback if player transform unavailable
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
                            sprite->playing = true;
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
                        }
                        sa->currentState = "sweep";
                        // Ensure sweep resumes playing
                        sprite->playing = true;
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

} // namespace GameCore
