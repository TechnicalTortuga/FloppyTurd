#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Assets/TextureManager.h"
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
        , m_obstacleSpawnTimer(0.0f)
        , m_enemySpawnTimer(0.0f)
        // REMOVED: m_pickupSpawnTimer initialization
        , m_npcSpawnTimer(0.0f)
        , m_lastObstacleX(1000.0f)   // Start obstacles off screen to the right
        , m_lastEnemyX(1200.0f)      // Start enemies further out
        , m_obstacleSpacing(400.0f)  // Default spacing between obstacles
        , m_poolInitialized(false)
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
        m_npcSpawnTimer = 0.0f;
        m_lastObstacleX = 1000.0f;
        m_lastEnemyX = 1200.0f;
        // Set loaded flag BEFORE initializing pools
        m_isLoaded = true;
        
        // Reset group id
        m_nextGroupId = 1;
        
        // Initialize object pool instead of timer-based spawning
        m_poolInitialized = false;
        
        // Pickups managed by GameplayState; no pickup pool
        
        InitializeObstaclePool();
        // Coin attachment for Level 2 is now handled dynamically in UpdatePickupPooling
        m_enemyPoolInitialized = false;
        m_npcPoolInitialized = false;
        m_projectilePoolInitialized = false;
        InitializeEnemyPool();
        InitializeNPCPool();
        InitializeProjectilePool();
        // Pickups and projectiles are pooled but may be empty until used
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
        m_poolInitialized = false;
        m_enemyPoolInitialized = false;
        m_npcPoolInitialized = false;
        
        m_projectilePoolInitialized = false;
        
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

    void LevelManager::UpdateEnemySpawning(float) { /* disabled; using pool */ }

    // Spawn Janitor NPC periodically (Level 2 only)
    void LevelManager::UpdateNPCSpawning(float) { /* disabled; using pool */ }

    void LevelManager::InitializeEnemyPool() {
        if (m_enemyPoolInitialized) return;
        if (!m_isLoaded || !m_currentLevelConfig.enableEnemies || m_currentLevelConfig.enemies.empty()) return;

        // Limit pool to m_maxActiveEnemies
        int count = std::min(m_maxActiveEnemies, static_cast<int>(m_currentLevelConfig.enemies.size() * m_maxActiveEnemies));
        // For Level 2 we only configured ToiletPaperFlap; still create up to 4 entities spaced to the right
        const EnemyConfig& cfg = m_currentLevelConfig.enemies[0];
        float screenW = 1179.0f; // Avoid async delegate to prevent dangling pointer crash
        float startX = screenW + 200.0f;
        // Reduce concurrent enemies a bit for sewers
        int desired = (m_currentLevelId == 2 ? 3 : m_maxActiveEnemies);
        for (int i = 0; i < desired; ++i) {
            float x = startX + i * (m_enemySpacing * 1.25f);
            float baseY = 900.0f + static_cast<float>((i%2==0? -1:1) * 150);
            Gnosis::Entity e = SpawnEnemy(cfg, x, baseY);
            if (e != 0) { m_activeEnemies.push_back(e); m_enemyBaseY[e] = baseY; GN_LOG_DEBUG("Enemy init: ToiletPaper baseY=" + std::to_string(baseY) + ", x=" + std::to_string(x)); }
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

    void LevelManager::InitializeProjectilePool() {
        if (m_projectilePoolInitialized) return;
        // Player projectiles pool (if/when used)
        // We will allocate placeholders when first shot to avoid cold-start allocation hiccup
        m_projectilePoolInitialized = true;
    }


    void LevelManager::UpdateProjectilePooling(float, float) {
        if (!m_projectilePoolInitialized) return;
        // Projectiles will be recycled by PlayerController when lifetime expires; nothing to do here yet
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
            if (rightEdge < 0.0f) {
                t->position.x = rightmostX + (m_enemySpacing * 1.25f);
                float baseY = 900.0f + static_cast<float>((rand()%300) - 150);
                t->position.y = baseY;
                // Sync Enemy component's bobbing anchor with new wrap position
                Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(e);
                if (enemyComp) {
                    enemyComp->baseY = baseY;
                    enemyComp->hasInitializedBaseY = true;
                }
                m_enemyBaseY[e] = baseY;
                rightmostX = t->position.x;
                GN_LOG_DEBUG("Enemy wrap: newX=" + std::to_string(t->position.x) + ", baseY=" + std::to_string(baseY));
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

    Gnosis::Entity LevelManager::SpawnObstacle(const ObstacleConfig& config, float x, float y) {
        if (!m_ecsSystem) {
            return 0;
        }
        
        // Handle toilet pairs vs single obstacles
        if (config.spawnAsPair && !config.bottomTextureId.empty()) {
            return SpawnToiletPair(config, x, y);
        } else {
            return SpawnSingleObstacle(config, x, y);
        }
    }
    
    Gnosis::Entity LevelManager::SpawnToiletPair(const ObstacleConfig& config, float x, float y) {
        if (!m_ecsSystem) {
            return 0;
        }
        
        // Calculate positions for top and bottom toilets with gap, interpreting Transform.position as top-left
        float gapCenter = y; // y is the center of the gap in world pixels
        float topToiletY = gapCenter - (config.gapHeight * 0.5f) - config.height;      // top sprite top-left
        float bottomToiletY = gapCenter + (config.gapHeight * 0.5f);                    // bottom sprite top-left
        
        // Create top toilet
        Gnosis::Entity topToilet = m_ecsSystem->CreateEntity();
        
        Transform topTransform(Gnosis::GNVector2(x, topToiletY), 0.0f, 
                              Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
        Sprite topSprite(config.textureId, config.width, config.height);
        topSprite.layer = 3;
        topSprite.visible = true;
        
        GN_LOG_DEBUG("Creating toilet pair: topTexture=" + config.textureId + " bottomTexture=" + config.bottomTextureId);
        GN_LOG_DEBUG("Toilet dimensions: width=" + std::to_string(config.width) + " height=" + std::to_string(config.height));
        GN_LOG_DEBUG("Toilet position: x=" + std::to_string(x) + " topY=" + std::to_string(topToiletY) + " bottomY=" + std::to_string(bottomToiletY));
        GN_LOG_DEBUG("Toilet speed: " + std::to_string(config.speed) + " layer: " + std::to_string(topSprite.layer));
        
        Physics topPhysics;
        topPhysics.velocity.x = -config.speed;
        topPhysics.useGravity = false;
        
        Hitbox topCollider;
        topCollider.type = ColliderType::Rectangle;
        // Trim a fixed 30px from bottom; bias center up by half trim so collider top aligns with sprite top
        const float TRIM_PX = 30.0f;
        topCollider.width = 20.0f;
        topCollider.height = config.height - TRIM_PX;
        topCollider.offsetX = 0.0f; // centered horizontally
        topCollider.offsetY = -(TRIM_PX * 0.5f);
        topCollider.isStatic = false;
        topCollider.isTrigger = false;
        topCollider.tag = "obstacle";
        
        Obstacle topObstacle;
        topObstacle.obstacleType = config.textureId;
        topObstacle.damage = 1;
        topObstacle.behavior = static_cast<int>(config.behavior);
        topObstacle.oscillationSpeed = config.oscillationSpeed;
        topObstacle.oscillationRange = config.oscillationRange;
        topObstacle.oscillationTimer = 0.0f;
        topObstacle.basePosition = Gnosis::GNVector2(x, topToiletY);
        topObstacle.isTopPart = true;

        // Verbose debug: exact sprite and hitbox world metrics for TOP toilet
        {
            const float scale = m_currentLevelConfig.baseScale;
            const float spriteWidth = config.width * scale;
            const float spriteHeight = config.height * scale;
            const float centerX = x + spriteWidth * 0.5f;
            const float centerY = topToiletY + spriteHeight * 0.5f;
            const float hbWidth = topCollider.width * scale;
            const float hbHeight = topCollider.height * scale;
            const float hbLeft = centerX + (topCollider.offsetX * scale) - (hbWidth * 0.5f);
            const float hbRight = hbLeft + hbWidth;
            const float hbTop = centerY + (topCollider.offsetY * scale) - (hbHeight * 0.5f);
            const float hbBottom = hbTop + hbHeight;
            const float spriteTop = topToiletY;
            const float spriteBottom = topToiletY + spriteHeight;
            GN_LOG_DEBUG(std::string("TOP spriteTL=(") + std::to_string(x) + "," + std::to_string(topToiletY) + ") size=(" +
                          std::to_string(config.width) + "x" + std::to_string(config.height) + ") scale=" + std::to_string(scale) +
                          " worldSize=(" + std::to_string(spriteWidth) + "x" + std::to_string(spriteHeight) + ")");
            GN_LOG_DEBUG(std::string("TOP hitbox LRTB=") +
                          "L=" + std::to_string(hbLeft) +
                          " R=" + std::to_string(hbRight) +
                          " T=" + std::to_string(hbTop) +
                          " B=" + std::to_string(hbBottom) +
                          " size=(" + std::to_string(hbWidth) + "x" + std::to_string(hbHeight) + ")");
            GN_LOG_DEBUG(std::string("TOP spriteTop=") + std::to_string(spriteTop) +
                          " spriteBottom=" + std::to_string(spriteBottom));
        }
        
        // Create bottom toilet
        Gnosis::Entity bottomToilet = m_ecsSystem->CreateEntity();
        
        Transform bottomTransform(Gnosis::GNVector2(x, bottomToiletY), 0.0f, 
                                 Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
        Sprite bottomSprite(config.bottomTextureId, config.width, config.height);
        bottomSprite.layer = 3;
        bottomSprite.visible = true;
        
        Physics bottomPhysics;
        bottomPhysics.velocity.x = -config.speed;
        bottomPhysics.useGravity = false;
        
        Hitbox bottomCollider;
        bottomCollider.type = ColliderType::Rectangle;
        // Start 30px down; bias center down by half trim so collider top = sprite top + 30
        bottomCollider.width = 20.0f;
        bottomCollider.height = config.height - TRIM_PX;
        bottomCollider.offsetX = 0.0f; // centered horizontally
        bottomCollider.offsetY = +(TRIM_PX * 0.5f);
        bottomCollider.isStatic = false;
        bottomCollider.isTrigger = false;
        bottomCollider.tag = "obstacle";
        
        Obstacle bottomObstacle;
        bottomObstacle.obstacleType = config.bottomTextureId;
        bottomObstacle.damage = 1;
        bottomObstacle.behavior = static_cast<int>(config.behavior);
        bottomObstacle.oscillationSpeed = config.oscillationSpeed;
        bottomObstacle.oscillationRange = config.oscillationRange;
        bottomObstacle.oscillationTimer = 0.0f;
        bottomObstacle.basePosition = Gnosis::GNVector2(x, bottomToiletY);
        bottomObstacle.isTopPart = false;
        
        // Link the pair
        topObstacle.pairedEntity = bottomToilet;
        bottomObstacle.pairedEntity = topToilet;
        
        // Add components to both entities
        m_ecsSystem->AddComponent<Transform>(topToilet, topTransform);
        m_ecsSystem->AddComponent<Sprite>(topToilet, topSprite);
        m_ecsSystem->AddComponent<Physics>(topToilet, topPhysics);
        m_ecsSystem->AddComponent<Hitbox>(topToilet, topCollider);
        m_ecsSystem->AddComponent<Obstacle>(topToilet, topObstacle);

        // Debug overlays for top toilet (uses Hitbox for dimensions)
        DebugDraw topDebug(false, false, Gnosis::GNColor(0, 255, 0, 255), Gnosis::GNColor(255, 0, 0, 255));
        m_ecsSystem->AddComponent<DebugDraw>(topToilet, topDebug);
        
        m_ecsSystem->AddComponent<Transform>(bottomToilet, bottomTransform);
        m_ecsSystem->AddComponent<Sprite>(bottomToilet, bottomSprite);
        m_ecsSystem->AddComponent<Physics>(bottomToilet, bottomPhysics);
        m_ecsSystem->AddComponent<Hitbox>(bottomToilet, bottomCollider);
        m_ecsSystem->AddComponent<Obstacle>(bottomToilet, bottomObstacle);

        // Debug overlays for bottom toilet (uses Hitbox for dimensions)
        DebugDraw bottomDebug(false, false, Gnosis::GNColor(0, 255, 0, 255), Gnosis::GNColor(255, 0, 0, 255));
        m_ecsSystem->AddComponent<DebugDraw>(bottomToilet, bottomDebug);
        
        // Track both active obstacles
        m_activeObstacles.push_back(topToilet);
        m_activeObstacles.push_back(bottomToilet);

        // Verbose debug: exact sprite and hitbox world metrics for BOTTOM toilet (+ gap analysis)
        {
            const float scale = m_currentLevelConfig.baseScale;
            const float spriteWidth = config.width * scale;
            const float spriteHeight = config.height * scale;
            const float centerX = x + spriteWidth * 0.5f;
            const float centerY = bottomToiletY + spriteHeight * 0.5f;
            const float hbWidth = bottomCollider.width * scale;
            const float hbHeight = bottomCollider.height * scale;
            const float hbLeft = centerX + (bottomCollider.offsetX * scale) - (hbWidth * 0.5f);
            const float hbRight = hbLeft + hbWidth;
            const float hbTop = centerY + (bottomCollider.offsetY * scale) - (hbHeight * 0.5f);
            const float hbBottom = hbTop + hbHeight;
            const float spriteTop = bottomToiletY;
            const float spriteBottom = bottomToiletY + spriteHeight;

            // Compute gaps between TOP/BOTTOM sprites and colliders for cross-check
            const float scaleTop = m_currentLevelConfig.baseScale;
            const float topSpriteHeight = config.height * scaleTop;
            const float topSpriteBottom = topToiletY + topSpriteHeight;
            const float spriteGap = bottomToiletY - topSpriteBottom; // should equal config.gapHeight (unscaled)
            const float topColliderBottom = (topToiletY + topSpriteHeight) - (30.0f * scaleTop);
            const float bottomColliderTop = bottomToiletY + (30.0f * scale);
            const float colliderGap = bottomColliderTop - topColliderBottom; // should be fixedGapHeight + 60*scale

            GN_LOG_DEBUG(std::string("BOTTOM spriteTL=(") + std::to_string(x) + "," + std::to_string(bottomToiletY) + ") size=(" +
                          std::to_string(config.width) + "x" + std::to_string(config.height) + ") scale=" + std::to_string(scale) +
                          " worldSize=(" + std::to_string(spriteWidth) + "x" + std::to_string(spriteHeight) + ")");
            GN_LOG_DEBUG(std::string("BOTTOM hitbox LRTB=") +
                          "L=" + std::to_string(hbLeft) +
                          " R=" + std::to_string(hbRight) +
                          " T=" + std::to_string(hbTop) +
                          " B=" + std::to_string(hbBottom) +
                          " size=(" + std::to_string(hbWidth) + "x" + std::to_string(hbHeight) + ")");
            GN_LOG_DEBUG(std::string("GAP check: spriteGap=") + std::to_string(spriteGap) +
                          " (expected=" + std::to_string(config.gapHeight) + ") spriteGapScaled=" + std::to_string(spriteGap * scale) +
                          " (expectedScaled=" + std::to_string(config.gapHeight * scale) + ") colliderGap=" + std::to_string(colliderGap) +
                          " (expectedColliderGapScaled=" + std::to_string(config.gapHeight * scale + 60.0f * scale) + ")");
        }
        
            // Summary line to confirm initial spawn metrics
            GN_LOG_DEBUG(std::string("SPAWN SUMMARY: x=") + std::to_string(x) +
                         " topY=" + std::to_string(topToiletY) +
                         " bottomY=" + std::to_string(bottomToiletY) +
                         " gap(unscaled)=" + std::to_string(config.gapHeight) +
                         " scale=" + std::to_string(m_currentLevelConfig.baseScale));
        
        return topToilet; // Return top toilet as primary entity
    }
    
    Gnosis::Entity LevelManager::SpawnToiletPairWithGap(const ObstacleConfig& config, float x, float gapCenterY, float gapHeight) {
        if (!m_ecsSystem) {
            return 0;
        }
        
        // NEW TOILET POSITIONING GROUND RULES:
        // 1. Top toilet ALWAYS positioned above screen (negative Y values)
        // 2. Much larger gap between toilets for better gameplay
        // 3. Bottom toilet position calculated from top toilet + large fixed gap
        
        float screenHeight = 2556.0f; // iPhone 16 portrait height
        float toiletHeight = config.height * m_currentLevelConfig.baseScale; // Scaled toilet height
        
        // Top toilet should be above screen but not too far - UPDATED for better center gap positioning
        float minTopY = -toiletHeight * 0.8f; // Closer to screen top (less negative)
        float maxTopY = -toiletHeight * 0.2f; // Even closer to screen top
        
        // Generate random position for top toilet within allowed range (all negative Y)
        float randomTopY = minTopY + (maxTopY - minTopY) * ((float)rand() / RAND_MAX);
        
        // Reduce the gap further (another ~25%): 412 -> ~309
        float fixedGapHeight = 309.0f;
        
        // Calculate bottom toilet position: top toilet bottom + large fixed gap
        float bottomToiletY = randomTopY + toiletHeight + fixedGapHeight;
        
        GN_LOG_DEBUG("Toilet positioning: screenHeight=" + std::to_string(screenHeight) + 
                    ", toiletHeight=" + std::to_string(toiletHeight) + 
                    ", minTopY=" + std::to_string(minTopY) + 
                    ", maxTopY=" + std::to_string(maxTopY) + 
                    ", topY=" + std::to_string(randomTopY) + 
                    ", bottomY=" + std::to_string(bottomToiletY) + 
                    ", gap=" + std::to_string(fixedGapHeight) + 
                    " (improved positioning: lower top, larger gaps)");
        
        // Create top toilet
        Gnosis::Entity topToilet = m_ecsSystem->CreateEntity();
        
        Transform topTransform(Gnosis::GNVector2(x, randomTopY), 0.0f, 
                              Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
        Sprite topSprite(config.textureId, config.width, config.height);
        topSprite.layer = 3;
        topSprite.visible = true;
        
        Physics topPhysics;
        topPhysics.velocity.x = -config.speed;
        topPhysics.useGravity = false;
        
        Hitbox topCollider;
        topCollider.type = ColliderType::Rectangle;
        // Center-based: trim 30px from bottom; offset center up by 15 so collider top aligns with sprite top
        const float TRIM_TOP = 30.0f;
        topCollider.width = 20.0f;
        topCollider.height = config.height - TRIM_TOP;
        topCollider.offsetX = 0.0f;
        topCollider.offsetY = -(TRIM_TOP * 0.5f);
        topCollider.isStatic = false;
        topCollider.isTrigger = false;
        topCollider.tag = "obstacle";
        
        Obstacle topObstacle;
        topObstacle.obstacleType = config.textureId;
        topObstacle.damage = 1;
        topObstacle.behavior = static_cast<int>(config.behavior);
        topObstacle.oscillationSpeed = config.oscillationSpeed;
        topObstacle.oscillationRange = config.oscillationRange;
        topObstacle.oscillationTimer = 0.0f;
        topObstacle.basePosition = Gnosis::GNVector2(x, randomTopY);
        topObstacle.isTopPart = true;
        
        // Create bottom toilet
        Gnosis::Entity bottomToilet = m_ecsSystem->CreateEntity();
        
        Transform bottomTransform(Gnosis::GNVector2(x, bottomToiletY), 0.0f, 
                                 Gnosis::GNVector2(m_currentLevelConfig.baseScale, m_currentLevelConfig.baseScale));
        
        Sprite bottomSprite(config.bottomTextureId, config.width, config.height);
        bottomSprite.layer = 3;
        bottomSprite.visible = true;
        
        Physics bottomPhysics;
        bottomPhysics.velocity.x = -config.speed;
        bottomPhysics.useGravity = false;
        
        Hitbox bottomCollider;
        bottomCollider.type = ColliderType::Rectangle;
        // Center-based: start 30px down; offset center down by 15 so collider top = sprite top + 30
        const float TRIM_BOTTOM = 30.0f;
        bottomCollider.width = 20.0f;
        bottomCollider.height = config.height - TRIM_BOTTOM;
        bottomCollider.offsetX = 0.0f;
        bottomCollider.offsetY = +(TRIM_BOTTOM * 0.5f);
        bottomCollider.isStatic = false;
        bottomCollider.isTrigger = false;
        bottomCollider.tag = "obstacle";
        
        Obstacle bottomObstacle;
        bottomObstacle.obstacleType = config.bottomTextureId;
        bottomObstacle.damage = 1;
        bottomObstacle.behavior = static_cast<int>(config.behavior);
        bottomObstacle.oscillationSpeed = config.oscillationSpeed;
        bottomObstacle.oscillationRange = config.oscillationRange;
        bottomObstacle.oscillationTimer = 0.0f;
        bottomObstacle.basePosition = Gnosis::GNVector2(x, bottomToiletY);
        bottomObstacle.isTopPart = false;
        
        // Link the pair
        topObstacle.pairedEntity = bottomToilet;
        bottomObstacle.pairedEntity = topToilet;
        
        // Add components to both entities
        m_ecsSystem->AddComponent<Transform>(topToilet, topTransform);
        m_ecsSystem->AddComponent<Sprite>(topToilet, topSprite);
        m_ecsSystem->AddComponent<Physics>(topToilet, topPhysics);
        m_ecsSystem->AddComponent<Hitbox>(topToilet, topCollider);
        m_ecsSystem->AddComponent<Obstacle>(topToilet, topObstacle);
        // Debug overlays for top toilet (uses Hitbox for dimensions)
        DebugDraw topDebug2(false, false, Gnosis::GNColor(0, 255, 0, 255), Gnosis::GNColor(255, 0, 0, 255));
        m_ecsSystem->AddComponent<DebugDraw>(topToilet, topDebug2);
        
        m_ecsSystem->AddComponent<Transform>(bottomToilet, bottomTransform);
        m_ecsSystem->AddComponent<Sprite>(bottomToilet, bottomSprite);
        m_ecsSystem->AddComponent<Physics>(bottomToilet, bottomPhysics);
        m_ecsSystem->AddComponent<Hitbox>(bottomToilet, bottomCollider);
        m_ecsSystem->AddComponent<Obstacle>(bottomToilet, bottomObstacle);
        // Debug overlays for bottom toilet (uses Hitbox for dimensions)
        DebugDraw bottomDebug2(false, false, Gnosis::GNColor(0, 255, 0, 255), Gnosis::GNColor(255, 0, 0, 255));
        m_ecsSystem->AddComponent<DebugDraw>(bottomToilet, bottomDebug2);
        
        // Track both active obstacles
        m_activeObstacles.push_back(topToilet);
        m_activeObstacles.push_back(bottomToilet);

        // Verbose debug: exact sprite and hitbox world metrics for TOP and BOTTOM (pool spawn variant)
        {
            const float scale = m_currentLevelConfig.baseScale;
            const float spriteWidth = config.width * scale;
            const float spriteHeight = config.height * scale;
            const float centerXTop = x + spriteWidth * 0.5f;
            const float centerYTop = randomTopY + spriteHeight * 0.5f;
            const float topHbW = topCollider.width * scale;
            const float topHbH = topCollider.height * scale;
            const float topHbL = centerXTop + (topCollider.offsetX * scale) - (topHbW * 0.5f);
            const float topHbT = centerYTop + (topCollider.offsetY * scale) - (topHbH * 0.5f);
            const float topHbR = topHbL + topHbW;
            const float topHbB = topHbT + topHbH;

            GN_LOG_DEBUG(std::string("POOL TOP spriteTL=(") + std::to_string(x) + "," + std::to_string(randomTopY) + ") size=(" +
                          std::to_string(config.width) + "x" + std::to_string(config.height) + ") scale=" + std::to_string(scale) +
                          " worldSize=(" + std::to_string(spriteWidth) + "x" + std::to_string(spriteHeight) + ")");
            GN_LOG_DEBUG(std::string("POOL TOP hitbox LRTB=") +
                          "L=" + std::to_string(topHbL) +
                          " R=" + std::to_string(topHbR) +
                          " T=" + std::to_string(topHbT) +
                          " B=" + std::to_string(topHbB) +
                          " size=(" + std::to_string(topHbW) + "x" + std::to_string(topHbH) + ")");

            const float centerXBot = x + spriteWidth * 0.5f;
            const float centerYBot = bottomToiletY + spriteHeight * 0.5f;
            const float botHbW = bottomCollider.width * scale;
            const float botHbH = bottomCollider.height * scale;
            const float botHbL = centerXBot + (bottomCollider.offsetX * scale) - (botHbW * 0.5f);
            const float botHbT = centerYBot + (bottomCollider.offsetY * scale) - (botHbH * 0.5f);
            const float botHbR = botHbL + botHbW;
            const float botHbB = botHbT + botHbH;

            GN_LOG_DEBUG(std::string("POOL BOTTOM spriteTL=(") + std::to_string(x) + "," + std::to_string(bottomToiletY) + ") size=(" +
                          std::to_string(config.width) + "x" + std::to_string(config.height) + ") scale=" + std::to_string(scale) +
                          " worldSize=(" + std::to_string(spriteWidth) + "x" + std::to_string(spriteHeight) + ")");
            GN_LOG_DEBUG(std::string("POOL BOTTOM hitbox LRTB=") +
                          "L=" + std::to_string(botHbL) +
                          " R=" + std::to_string(botHbR) +
                          " T=" + std::to_string(botHbT) +
                          " B=" + std::to_string(botHbB) +
                          " size=(" + std::to_string(botHbW) + "x" + std::to_string(botHbH) + ")");

            // Gap diagnostics
            const float topSpriteBottom = randomTopY + spriteHeight;
            const float spriteGap = bottomToiletY - topSpriteBottom; // unscaled world px
            const float topColliderBottom = (randomTopY + spriteHeight) - (30.0f * scale);
            const float bottomColliderTop = bottomToiletY + (30.0f * scale);
            const float colliderGap = bottomColliderTop - topColliderBottom; // world px

            GN_LOG_DEBUG(std::string("POOL GAP check: spriteGap=") + std::to_string(spriteGap) +
                          " (expected=" + std::to_string(fixedGapHeight) + ") spriteGapScaled=" + std::to_string(spriteGap * 1.0f) +
                          " colliderGap=" + std::to_string(colliderGap) +
                          " (expectedColliderGapScaled=" + std::to_string(fixedGapHeight + 60.0f * scale) + ")");
        }

        GN_LOG_DEBUG("Spawned toilet pair with gap: " + config.textureId + "/" + config.bottomTextureId +
                    " at x=" + std::to_string(x) + ", gap center=" + std::to_string(gapCenterY) +
                    ", gap height=" + std::to_string(gapHeight));
        
        return topToilet; // Return top toilet as primary entity
    }
    
    Gnosis::Entity LevelManager::SpawnSingleObstacle(const ObstacleConfig& config, float x, float y) {
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
        
        // Create hitbox component
        Hitbox collider;
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
        obstacleComp.behavior = static_cast<int>(config.behavior);
        obstacleComp.oscillationSpeed = config.oscillationSpeed;
        obstacleComp.oscillationRange = config.oscillationRange;
        obstacleComp.oscillationTimer = 0.0f;
        obstacleComp.basePosition = Gnosis::GNVector2(x, y);
        obstacleComp.pairedEntity = 0;
        obstacleComp.isTopPart = false;
        
        // Add components
        m_ecsSystem->AddComponent<Transform>(obstacle, transform);
        m_ecsSystem->AddComponent<Sprite>(obstacle, sprite);
        m_ecsSystem->AddComponent<Physics>(obstacle, physics);
        m_ecsSystem->AddComponent<Hitbox>(obstacle, collider);
        m_ecsSystem->AddComponent<Obstacle>(obstacle, obstacleComp);
        
        // Track active obstacle
        m_activeObstacles.push_back(obstacle);
        
        GN_LOG_DEBUG("Spawned single obstacle: " + config.textureId + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        
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
        } else if (config.textureId == "ToiletPaperFlap") {
            // Other animated enemies can be configured similarly
            sprite.isAnimated = true;
            // 8-frame flying animation at 64x64
            sprite.frameWidth = 64;
            sprite.frameHeight = 64;
            sprite.frameCount = 8;
            sprite.frameTime = 0.18f; // slow flapping a tad
            sprite.currentFrame = 0;
            sprite.currentFrameTime = 0.0f;
            sprite.playing = true;
            sprite.loop = true;
            sprite.hasCompleted = false;
            // Note: hurt animation (4 frames) can be switched by damage handling code later
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
        
        // Create hitbox
        Hitbox collider;
        collider.isStatic = false;
        collider.width = config.width;
        collider.height = config.height;
        collider.tag = "Enemy";
        
        // Create enemy component
        Enemy enemyComp;
        enemyComp.health = config.hitPoints;
        enemyComp.enemyType = config.movementPattern;
        enemyComp.isActive = true;
        // Enable bobbing for horizontal flyers (ToiletPaper)
        if (config.textureId == "ToiletPaperFlap") {
            enemyComp.bobbingEnabled = true;
            // Randomize speed slightly per enemy for desynchronization
            float speedBase = 1.8f;
            float speedJitter = (static_cast<float>((rand() % 41) - 20) * 0.02f); // -0.4 .. +0.4
            enemyComp.bobSpeed = std::max(0.8f, speedBase + speedJitter); // clamp min speed

            // Large amplitude: ~35-43% of screen height so they traverse most of the screen
            float screenH = 2556.0f;
            float ampFactor = 0.38f + (static_cast<float>((rand() % 21) - 10) * 0.005f); // 0.33..0.43
            enemyComp.bobAmplitude = screenH * ampFactor;

            // Random starting phase 0..2π
            enemyComp.bobPhase = static_cast<float>((rand() % 628)) / 100.0f;
        }
        
        // Add components
        m_ecsSystem->AddComponent<Transform>(enemy, transform);
        m_ecsSystem->AddComponent<Sprite>(enemy, sprite);
        m_ecsSystem->AddComponent<Physics>(enemy, physics);
        m_ecsSystem->AddComponent<Hitbox>(enemy, collider);
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

    // Legacy pickup spawn removed; GameplayState creates coin entities directly

    // Legacy RemovePickup removed

    void LevelManager::CleanupOffscreenEntities(float leftBoundary) {
        // CRITICAL FIX: Don't cleanup obstacles when pooling system is active!
        // The pooling system handles obstacle reuse via wrapping, not destruction
        if (!m_poolInitialized) {
            // Only clean up obstacles if pooling is NOT active (legacy behavior)
            for (auto it = m_activeObstacles.begin(); it != m_activeObstacles.end();) {
                Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
                if (transform && transform->position.x < leftBoundary) {
                    m_ecsSystem->DestroyEntity(*it);
                    it = m_activeObstacles.erase(it);
                } else {
                    ++it;
                }
            }
        }
        // Note: When pooling is active, obstacles are managed by UpdateObstaclePooling() instead
        
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
            if (m_platformDelegates.asset.getTextureMetadata) {
                GameCore::TextureMetadata meta{};
                if (m_platformDelegates.asset.getTextureMetadata(layerConfig.textureId.c_str(), &meta)) {
                    tw = meta.width;
                    th = meta.height;
                }
            }
            if (tw <= 0 || th <= 0) {
                GN_LOG_WARN(std::string("getTextureMetadata unavailable for '") + layerConfig.textureId + "', defaulting 512x512");
                tw = 512; th = 512;
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
        
        // Pickups managed by GameplayState

        // Destroy all active NPCs (including Janitor) and reset handle
        for (Gnosis::Entity entity : m_activeNPCs) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_activeNPCs.clear();
        m_janitorEntity = 0;
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
    // Object Pooling System Implementation
    // ============================================================================

    void LevelManager::InitializeObstaclePool() {
        GN_LOG_INFO("=== OBSTACLE POOL INITIALIZATION CHECK ===");
        GN_LOG_INFO("Pool initialized: " + std::to_string(m_poolInitialized));
        GN_LOG_INFO("Level loaded: " + std::to_string(m_isLoaded));
        GN_LOG_INFO("Current level ID: " + std::to_string(m_currentLevelId));
        GN_LOG_INFO("Obstacles count: " + std::to_string(m_currentLevelConfig.obstacles.size()));
        
        if (m_currentLevelConfig.obstacles.size() > 0) {
            GN_LOG_INFO("First obstacle: top='" + m_currentLevelConfig.obstacles[0].textureId + 
                       "', bottom='" + m_currentLevelConfig.obstacles[0].bottomTextureId + 
                       "', spawnAsPair=" + std::to_string(m_currentLevelConfig.obstacles[0].spawnAsPair));
        }
        
        if (m_poolInitialized || !m_isLoaded || m_currentLevelConfig.obstacles.empty()) {
            GN_LOG_ERROR("Skipping pool initialization - already initialized or no data");
            GN_LOG_ERROR("Reason: poolInit=" + std::to_string(m_poolInitialized) + 
                          ", levelLoaded=" + std::to_string(m_isLoaded) + 
                          ", obstaclesEmpty=" + std::to_string(m_currentLevelConfig.obstacles.empty()));
            return;
        }

        GN_LOG_INFO("Initializing obstacle pool for level " + std::to_string(m_currentLevelId));

        // Level 2 patterns can be several tiles wide; use wider base spacing to prevent initial overlaps
        m_obstacleSpacing = 5200.0f;
        
        // Choose the first obstacle type for the pool (can randomize later)
        const ObstacleConfig& baseConfig = m_currentLevelConfig.obstacles[0];
        
        // Create initial pool: place patterns back-to-back using each group's actual width
        float screenWidth = 1179.0f;
        float initialCameraX = 0.0f;
        float nextWorldX = screenWidth + 100.0f + initialCameraX;
        const float safeGap = 0.0f; // no extra gap; pack by exact group width
        
        for (int i = 0; i < OBSTACLE_POOL_SIZE; i++) {
            GN_LOG_DEBUG("Spawning pooled pattern " + std::to_string(i) + " at world X: " + std::to_string(nextWorldX));
            // Capture the next group id before spawn (spawners increment it once per group)
            int groupIdBefore = m_nextGroupId;
            if (m_currentLevelId == 2) {
                int pattern = rand() % 6;
                switch (pattern) {
                    case 0: SpawnSewerPattern_TopOnly(nextWorldX); break;
                    case 1: SpawnSewerPattern_BottomOnly(nextWorldX); break;
                    case 2: SpawnSewerPattern_TopAndBottom(nextWorldX); break;
                    case 3: SpawnSewerPattern_Pyramid3(nextWorldX); break;
                    case 4: SpawnSewerPattern_PyramidTop3(nextWorldX); break;
                    default: SpawnSewerPattern_TwoByTwoFunnel(nextWorldX); break;
                }
            } else {
            if (baseConfig.spawnAsPair && !baseConfig.bottomTextureId.empty()) {
                    SpawnToiletPairWithGap(baseConfig, nextWorldX, 0.0f, 0.0f);
            } else {
                    float spawnY = 500.0f;
                    SpawnObstacle(baseConfig, nextWorldX, spawnY);
                }
            }
            // Determine spawned group's width strictly from the leader's Group component
            int justSpawnedGroupId = groupIdBefore; // spawners incremented m_nextGroupId
            float spawnedGroupWidth = 600.0f; // fallback
            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->id == justSpawnedGroupId && g->isLeader) {
                    spawnedGroupWidth = g->groupWidth;
                    break;
                }
            }
            // Ensure groupWidth is at least the visual width of the leader sprite if not set
            if (spawnedGroupWidth <= 0.0f) {
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (g && g->id == justSpawnedGroupId && g->isLeader) {
                        Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                        Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                        if (t && s) spawnedGroupWidth = s->width * std::abs(t->scale.x);
                        break;
                    }
                }
            }
            // Anchor next group exactly after this group's width; no pattern-specific extra offset
            nextWorldX += spawnedGroupWidth + safeGap;
        }

        m_poolInitialized = true;
        GN_LOG_INFO("Obstacle pool initialized with " + std::to_string(OBSTACLE_POOL_SIZE) + " obstacles");
    }

    void LevelManager::UpdateObstaclePooling(float deltaTime, float worldScrollDistance) {
        if (!m_poolInitialized || m_activeObstacles.empty()) {
            GN_LOG_DEBUG("UpdateObstaclePooling skipped: poolInit=" + std::to_string(m_poolInitialized) + ", obstacles=" + std::to_string(m_activeObstacles.size()));
            return;
        }

        // Debug: Log obstacle positions periodically
        static float debugTimer = 0.0f;
        debugTimer += deltaTime;
        if (debugTimer >= 2.0f) { // Every 2 seconds
            debugTimer = 0.0f;
            GN_LOG_DEBUG("=== OBSTACLE POSITIONS DEBUG ===");
            GN_LOG_DEBUG("World scroll distance: " + std::to_string(worldScrollDistance));
            for (size_t i = 0; i < m_activeObstacles.size() && i < 4; i++) { // Log first 4 obstacles
                Transform* t = m_ecsSystem->GetComponent<Transform>(m_activeObstacles[i]);
                if (t) {
                    GN_LOG_DEBUG("Obstacle " + std::to_string(i) + " X: " + std::to_string(t->position.x));
                }
            }
        }

        // If we are on Level 2 and using grouped patterns, wrap by group id
        if (m_currentLevelId == 2) {
            // Gather leaders by group id
            std::unordered_map<int, Gnosis::Entity> groupLeaders;
            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->isLeader) {
                    groupLeaders[g->id] = e;
                }
            }
            for (const auto& [gid, leader] : groupLeaders) {
                WrapGroupAroundScreen(gid, worldScrollDistance);
                // Do not queue wrap here; WrapGroupAroundScreen() pushes only when a real wrap occurs
            }
        } else {
            // Legacy per-entity wrapping
        for (Gnosis::Entity obstacle : m_activeObstacles) {
            WrapObstacleAroundScreen(obstacle, worldScrollDistance);
            }
        }
    }

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

    void LevelManager::WrapGroupAroundScreen(int groupId, float worldScrollDistance) {
        if (!m_ecsSystem) return;

        // Find leader and compute group bounds
        Transform* leaderTransform = nullptr;
        Sprite* leaderSprite = nullptr;
        float groupWidth = 0.0f;
        float leaderBaseOffsetX = 0.0f;
        Gnosis::Entity leaderEntity = 0;

        for (Gnosis::Entity e : m_activeObstacles) {
            Group* g = m_ecsSystem->GetComponent<Group>(e);
            if (g && g->id == groupId && g->isLeader) {
                leaderTransform = m_ecsSystem->GetComponent<Transform>(e);
                leaderSprite = m_ecsSystem->GetComponent<Sprite>(e);
                leaderEntity = e;
                groupWidth = g->groupWidth;
                leaderBaseOffsetX = g->offsetX;
                break;
            }
        }
        if (!leaderTransform || !leaderSprite) return;

        float scaledWidth = leaderSprite->width * std::abs(leaderTransform->scale.x);
        // Compute last-member right edge by scanning members of this group
        float lastMemberRight = leaderTransform->position.x + scaledWidth;
        for (Gnosis::Entity e : m_activeObstacles) {
            Group* g = m_ecsSystem->GetComponent<Group>(e);
            if (g && g->id == groupId) {
                Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                if (t && s) {
                    float w = s->width * std::abs(t->scale.x);
                    float r = t->position.x + w;
                    if (r > lastMemberRight) lastMemberRight = r;
                }
            }
        }
        float screenWidth = 1179.0f;

        if (lastMemberRight < 0.0f) {
            // Find rightmost group's right edge
            float rightmostRightEdge = 0.0f;
            int rightmostGroupId = -1;
            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->isLeader) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                    if (t && s) {
                        float w = s->width * std::abs(t->scale.x);
                        float r = t->position.x + w;
                        if (r > rightmostRightEdge) {
                            rightmostRightEdge = r;
                            rightmostGroupId = g->id;
                        }
                    }
                }
            }
            // Measure the true span of the rightmost group (max member right edge − leader origin)
            float rightmostOriginX = 0.0f;
            float measuredRightEdge = rightmostRightEdge;
            if (rightmostGroupId >= 0) {
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (g && g->id == rightmostGroupId) {
                        Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                        Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                        if (t && s) {
                            if (g->isLeader) rightmostOriginX = t->position.x;
                            float w = s->width * std::abs(t->scale.x);
                            float r = t->position.x + w;
                            if (r > measuredRightEdge) measuredRightEdge = r;
                        }
                    }
                }
            }
            float measuredSpan = measuredRightEdge - rightmostOriginX;
            if (measuredSpan <= 0.0f) measuredSpan = scaledWidth; // fallback
            float newX = rightmostOriginX + measuredSpan;
            GN_LOG_DEBUG("WRAP groupId=" + std::to_string(groupId) +
                         " rightmostGroupId=" + std::to_string(rightmostGroupId) +
                         " rightmostRightEdge=" + std::to_string(rightmostRightEdge) +
                         " rightmostOriginX=" + std::to_string(rightmostOriginX) +
                         " leaderGroupWidth=" + std::to_string(groupWidth) +
                         " measuredSpan=" + std::to_string(measuredSpan) +
                         " leaderBaseOffsetX=" + std::to_string(leaderBaseOffsetX) +
                         " newX=" + std::to_string(newX));
            // Move entire group preserving offsets. For stacked pyramids, keep vertical offsets.
            const float screenH = 2556.0f;
            const float scale = std::abs(leaderTransform->scale.y);
            const float pipeH = leaderSprite->height;
            const float bottomY = screenH - pipeH * scale;

            // Detect group composition
            bool hasTopMember = false;
            bool hasBottomMember = false;
            bool hasStackOffsets = false; // any member has non-zero offsetY relative to its band
            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->id == groupId) {
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                    Obstacle* o = m_ecsSystem->GetComponent<Obstacle>(e);
                    if (o && s) {
                        bool isTop = o->isTopPart || (s->textureId.find("TopPipe") != std::string::npos);
                        if (isTop) hasTopMember = true; else hasBottomMember = true;
                    }
                    if (std::abs(g->offsetY) > 0.01f) {
                        hasStackOffsets = true;
                    }
                }
            }

            // Anchor baseline at the leader's band for single-band groups
            float leaderBandY = bottomY;
            {
                Obstacle* leaderObstacle = m_ecsSystem->GetComponent<Obstacle>(leaderEntity);
                Sprite* leaderSpriteComp = m_ecsSystem->GetComponent<Sprite>(leaderEntity);
                bool leaderIsTop = false;
                if (leaderObstacle && leaderObstacle->isTopPart) leaderIsTop = true;
                else if (leaderSpriteComp && leaderSpriteComp->textureId.find("TopPipe") != std::string::npos) leaderIsTop = true;
                leaderBandY = leaderIsTop ? 0.0f : bottomY;
            }

            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->id == groupId) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                    Obstacle* o = m_ecsSystem->GetComponent<Obstacle>(e);
                    if (t) {
                        float relOffsetX = g->offsetX - leaderBaseOffsetX;
                        t->position.x = newX + relOffsetX;

                        float newY = leaderBandY + g->offsetY; // default: single-band anchored + relative offset
                        if (hasTopMember && hasBottomMember) {
                            // Mixed band group (e.g., Top+Bottom pair): anchor per member band
                            bool isTop = false;
                            if (o && s) {
                                isTop = o->isTopPart || (s->textureId.find("TopPipe") != std::string::npos);
                            }
                            newY = (isTop ? 0.0f : bottomY) + g->offsetY;
                        } else if (!hasStackOffsets) {
                            // Single, non-stacked piece: force exact band anchor (offsetY typically 0)
                            bool isTop = false;
                            if (o && s) {
                                isTop = o->isTopPart || (s->textureId.find("TopPipe") != std::string::npos);
                            }
                            newY = isTop ? 0.0f : bottomY;
                        }

                        t->position.y = newY;
                    }
                    if (o) o->pipeCleared = false;
                }
            }
            // Queue wrap event; GameplayState will reposition coins
            m_wrappedGroups.push_back(groupId);
        }
    }

    std::vector<int> LevelManager::ConsumeWrappedGroups() {
        std::vector<int> out;
        out.swap(m_wrappedGroups);
        return out;
    }
    
    // ============================================================================
    // New Unified Coin System Implementation
    // ============================================================================
    
    LevelManager::GroupPattern LevelManager::DetectGroupPattern(int groupId) const {
        // Simplified: read the pattern from any member's Group component (leader preferred)
        GroupPattern found = GroupPattern::TopOnly;
        for (Gnosis::Entity e : m_activeObstacles) {
            Group* g = m_ecsSystem->GetComponent<Group>(e);
            if (!g || g->id != groupId) continue;
            if (g->isLeader) return g->pattern;
            found = g->pattern; // fallback if no leader found
        }
        return found;
    }
    
    // =======================================================================
    // NEW: Helper functions for GameplayState coin coordination
    // =======================================================================
    
    std::vector<Gnosis::GNVector2> LevelManager::CalculateCoinPositionsForGroup(int groupId, GroupPattern pattern) const {
        std::vector<Gnosis::GNVector2> positions;
        const int coinsPerStripe = 5;
        // Horizontal padding from band edges for spreading coins within the pipe span
        const float horizontalMargin = 32.0f;
        // Coin visual half-size (16x16 sprite scaled 8x => 128x128, half = 64)
        const float coinHalf = 64.0f;
        // Vertical clearance from band edges so coins are reachable and not intersecting pipes
        const float verticalPad = coinHalf + 96.0f; // stronger offset per feedback (128 + 96 = 224px from band edge)

        // Compute group horizontal bounds and vertical anchors from obstacle sprites
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float topBandBottomEdge = std::numeric_limits<float>::max();
        float bottomBandTopEdge = std::numeric_limits<float>::lowest();
        // Track per-band horizontal spans to allow per-stripe width and intersections
        float topMinX = std::numeric_limits<float>::max();
        float topMaxX = std::numeric_limits<float>::lowest();
        float bottomMinX = std::numeric_limits<float>::max();
        float bottomMaxX = std::numeric_limits<float>::lowest();

                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (!g || g->id != groupId) continue;
                    
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
            Obstacle* o = m_ecsSystem->GetComponent<Obstacle>(e);
            if (!t || !s || !o) continue;
                    
            const float scaledW = s->width * std::abs(t->scale.x);
            const float scaledH = s->height * std::abs(t->scale.y);
                    minX = std::min(minX, t->position.x);
            maxX = std::max(maxX, t->position.x + scaledW);
            if (o->isTopPart) {
                // bottom edge of the top band
                topBandBottomEdge = std::min(topBandBottomEdge, t->position.y + scaledH);
                topMinX = std::min(topMinX, t->position.x);
                topMaxX = std::max(topMaxX, t->position.x + scaledW);
            } else {
                // top edge of the bottom band
                bottomBandTopEdge = std::max(bottomBandTopEdge, t->position.y);
                bottomMinX = std::min(bottomMinX, t->position.x);
                bottomMaxX = std::max(bottomMaxX, t->position.x + scaledW);
            }
        }

        if (!(minX < maxX)) {
            // Fallback spread if we failed to detect bounds
            float startX = 500.0f;
                float spacing = 40.0f;
            float y = 400.0f;
            for (int i = 0; i < coinsPerStripe; ++i) positions.emplace_back(startX + i * spacing, y);
            return positions;
        }
                
        auto emitStripe = [&](float y) {
            float left = minX + horizontalMargin;
            float right = maxX - horizontalMargin;
            if (!(left < right)) return;
            float spacing = (right - left) / static_cast<float>(coinsPerStripe);
                for (int i = 0; i < coinsPerStripe; ++i) {
                float x = left + spacing * (i + 0.5f);
                positions.emplace_back(x, y);
            }
            GN_LOG_DEBUG("LM::emitStripe y=" + std::to_string(y) + " left=" + std::to_string(left) + " right=" + std::to_string(right));
        };

        auto emitStripeInRange = [&](float y, float rangeMin, float rangeMax) {
            float left = rangeMin + horizontalMargin;
            float right = rangeMax - horizontalMargin;
            if (!(left < right)) return;
            float spacing = (right - left) / static_cast<float>(coinsPerStripe);
                for (int i = 0; i < coinsPerStripe; ++i) {
                float x = left + spacing * (i + 0.5f);
                positions.emplace_back(x, y);
            }
            GN_LOG_DEBUG("LM::emitStripeInRange y=" + std::to_string(y) + " left=" + std::to_string(left) + " right=" + std::to_string(right));
        };

        switch (pattern) {
            case GroupPattern::TopOnly: {
                float y = (topBandBottomEdge != std::numeric_limits<float>::max())
                    ? (topBandBottomEdge + verticalPad)
                    : (bottomBandTopEdge > std::numeric_limits<float>::lowest() ? bottomBandTopEdge - verticalPad : 400.0f);
                if (topMinX < topMaxX) emitStripeInRange(y, topMinX, topMaxX); else emitStripe(y);
                GN_LOG_DEBUG("LM::TopOnly stripe y=" + std::to_string(y) + " span=[" + std::to_string(topMinX) + "," + std::to_string(topMaxX) + "]");
                break;
        }
            case GroupPattern::BottomOnly: {
                float y = (bottomBandTopEdge != std::numeric_limits<float>::lowest())
                    ? (bottomBandTopEdge - verticalPad)
                    : (topBandBottomEdge < std::numeric_limits<float>::max() ? topBandBottomEdge + verticalPad : 400.0f);
                if (bottomMinX < bottomMaxX) emitStripeInRange(y, bottomMinX, bottomMaxX); else emitStripe(y);
                GN_LOG_DEBUG("LM::BottomOnly stripe y=" + std::to_string(y) + " span=[" + std::to_string(bottomMinX) + "," + std::to_string(bottomMaxX) + "]");
                break;
            }
            case GroupPattern::TopAndBottom: {
                // Two stripes: one below top band and one above bottom band, each within its own band span
                if (topBandBottomEdge != std::numeric_limits<float>::max()) {
                    if (topMinX < topMaxX) emitStripeInRange(topBandBottomEdge + verticalPad, topMinX, topMaxX);
                    else emitStripe(topBandBottomEdge + verticalPad);
                }
                if (bottomBandTopEdge != std::numeric_limits<float>::lowest()) {
                    if (bottomMinX < bottomMaxX) emitStripeInRange(bottomBandTopEdge - verticalPad, bottomMinX, bottomMaxX);
                    else emitStripe(bottomBandTopEdge - verticalPad);
                }
                break;
            }
            case GroupPattern::TwoFunnel: {
                // Three segments: spread 5 coins across each segment (left, middle, right) at center Y
                struct Span { float left; float right; float center; };
                std::vector<Span> segs;
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (!g || g->id != groupId) continue;
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                    if (!t || !s) continue;
                    float left = t->position.x;
                    float right = t->position.x + s->width * std::abs(t->scale.x);
                    float center = (left + right) * 0.5f;
                    segs.push_back({left, right, center});
                }
                if (!segs.empty()) {
                    std::sort(segs.begin(), segs.end(), [](const Span& a, const Span& b){ return a.center < b.center; });
                    // Collapse near-duplicate spans (top/bottom overlap) by simple greedy clustering
                    std::vector<Span> clusters;
                    const float mergeThreshold = 32.0f; // pixels between centers to consider same segment
                    for (const auto& s : segs) {
                        if (clusters.empty() || std::abs(s.center - clusters.back().center) > mergeThreshold) {
                            clusters.push_back(s);
                        } else {
                            // expand cluster bounds
                            clusters.back().left = std::min(clusters.back().left, s.left);
                            clusters.back().right = std::max(clusters.back().right, s.right);
                            clusters.back().center = (clusters.back().left + clusters.back().right) * 0.5f;
                        }
                    }
                    float centerY = (topBandBottomEdge != std::numeric_limits<float>::max() && bottomBandTopEdge != std::numeric_limits<float>::lowest())
                        ? ((topBandBottomEdge + bottomBandTopEdge) * 0.5f)
                        : 2556.0f * 0.5f;
                    GN_LOG_DEBUG("LM::TwoFunnel clusters=" + std::to_string(clusters.size()) + " centerY=" + std::to_string(centerY));
                    // Emit for left/middle/right if available
                    if (clusters.size() >= 1) emitStripeInRange(centerY, clusters.front().left, clusters.front().right);
                    if (clusters.size() >= 3) emitStripeInRange(centerY, clusters[clusters.size()/2].left, clusters[clusters.size()/2].right);
                    if (clusters.size() >= 2) emitStripeInRange(centerY, clusters.back().left, clusters.back().right);
                } else {
                    float centerY = (topBandBottomEdge != std::numeric_limits<float>::max() && bottomBandTopEdge != std::numeric_limits<float>::lowest())
                        ? ((topBandBottomEdge + bottomBandTopEdge) * 0.5f)
                        : 2556.0f * 0.5f;
                    emitStripe(centerY);
                }
                break;
            }
            case GroupPattern::PyramidBottom: {
                // Place 5 coins per complementary TOP pipe segment using TopOnly stripe Y offset (hugging top pipes)
                struct TSpan { float left; float right; float yBottom; };
                std::vector<TSpan> topPipes;
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (!g || g->id != groupId) continue;
                    Obstacle* o = m_ecsSystem->GetComponent<Obstacle>(e);
                    if (!o || !o->isTopPart) continue; // top pipes only
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                    if (!t || !s) continue;
                    float left = t->position.x;
                    float right = t->position.x + s->width * std::abs(t->scale.x);
                    float yBottom = t->position.y + s->height * std::abs(t->scale.y);
                    topPipes.push_back({left, right, yBottom});
                }
                std::sort(topPipes.begin(), topPipes.end(), [](const TSpan& a, const TSpan& b){ return a.left < b.left; });
                for (const auto& p : topPipes) {
                    // Same offset rule as TopOnly rows
                    float y = p.yBottom + verticalPad;
                    GN_LOG_DEBUG("LM::PyramidBottom complement stripe span=[" + std::to_string(p.left) + "," + std::to_string(p.right) + "] y=" + std::to_string(y));
                    emitStripeInRange(y, p.left, p.right);
                }
                break;
            }
            case GroupPattern::PyramidTop: {
                // Place 5 coins per complementary BOTTOM pipe segment using BottomOnly stripe Y offset (hugging bottom pipes)
                struct BSpan { float left; float right; float yTop; };
                std::vector<BSpan> bottomPipes;
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (!g || g->id != groupId) continue;
                    Obstacle* o = m_ecsSystem->GetComponent<Obstacle>(e);
                    if (!o || o->isTopPart) continue; // bottom pipes only
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                    if (!t || !s) continue;
                    float left = t->position.x;
                    float right = t->position.x + s->width * std::abs(t->scale.x);
                    float yTop = t->position.y; // top edge of bottom pipe
                    bottomPipes.push_back({left, right, yTop});
                }
                std::sort(bottomPipes.begin(), bottomPipes.end(), [](const BSpan& a, const BSpan& b){ return a.left < b.left; });
                for (const auto& p : bottomPipes) {
                    // Same offset rule as BottomOnly rows
                    float y = p.yTop - verticalPad;
                    GN_LOG_DEBUG("LM::PyramidTop complement stripe span=[" + std::to_string(p.left) + "," + std::to_string(p.right) + "] y=" + std::to_string(y));
                    emitStripeInRange(y, p.left, p.right);
                }
                break;
            }
            default: {
                float y = (topBandBottomEdge != std::numeric_limits<float>::max() && bottomBandTopEdge != std::numeric_limits<float>::lowest())
                    ? ((topBandBottomEdge + bottomBandTopEdge) * 0.5f)
                    : 400.0f;
                emitStripe(y);
                break;
            }
        }
        
        GN_LOG_DEBUG("CalculateCoinPositionsForGroup: groupId=" + std::to_string(groupId) + 
                     " pattern=" + std::to_string(static_cast<int>(pattern)) + 
                     " positions=" + std::to_string(positions.size()) +
                     " minX=" + std::to_string(minX) + " maxX=" + std::to_string(maxX));
        return positions;
    }
    
    bool LevelManager::IsGroupReadyForCoins(int groupId) const {
        // Check if a group is ready to have coins attached
        if (!m_currentLevelConfig.enablePickups) {
            return false;
        }
        
        // Count group members
        int memberCount = 0;
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
            if (g && g->id == groupId) {
                memberCount++;
            }
        }
        
        // Group needs at least 1 member to be ready
        bool ready = (memberCount > 0);
        
        GN_LOG_DEBUG("IsGroupReadyForCoins: groupId=" + std::to_string(groupId) + 
                     " members=" + std::to_string(memberCount) + 
                     " ready=" + std::string(ready ? "true" : "false"));
        
        return ready;
    }

    // --- Sewer patterns: single-piece formations ---
    static inline std::string NextSewerColor(int index) {
        return (index % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue"; // we will override bottom flag
    }

    void LevelManager::SpawnSewerPattern_TopOnly(float startX) {
        const float scale = m_currentLevelConfig.baseScale;
        // Exact art size 192x64
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        const float verticalTopY = 0.0f; // top-aligned
        const float spacingX = pipeW * scale; // groupWidth should be actual width; no extra separation

        int groupId = m_nextGroupId++;
        // Enforce top-only variants
        const std::string tex = (rand() % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
        GN_LOG_DEBUG(std::string("Spawn TopOnly groupId=") + std::to_string(groupId) + 
                     " tex=" + tex + " startX=" + std::to_string(startX) + " topY=0");
        Gnosis::Entity e = m_ecsSystem->CreateEntity();
        Transform tr(Gnosis::GNVector2(startX, verticalTopY), 0.0f, Gnosis::GNVector2(scale, scale));
        Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
        Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
        // 6 px inset hitbox
        Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW - 12.0f; hb.height = pipeH - 12.0f; hb.offsetX = 0.0f; hb.offsetY = 0.0f; hb.isStatic = false; hb.tag = "obstacle";
        Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = true;
        Group gp; gp.id = groupId; gp.isLeader = true; gp.offsetX = 0.0f; gp.offsetY = 0.0f; gp.groupWidth = pipeW * scale; gp.pattern = GroupPattern::TopOnly;
        m_ecsSystem->AddComponent<Transform>(e, tr);
        m_ecsSystem->AddComponent<Sprite>(e, sp);
        m_ecsSystem->AddComponent<Physics>(e, ph);
        m_ecsSystem->AddComponent<Hitbox>(e, hb);
        m_ecsSystem->AddComponent<Obstacle>(e, ob);
        // Enable debug collider/bounds visualization for pipes
        // Debug hitboxes off for production visuals
        m_ecsSystem->AddComponent<Group>(e, gp);
        m_activeObstacles.push_back(e);
    }

    void LevelManager::SpawnSewerPattern_BottomOnly(float startX) {
        const float scale = m_currentLevelConfig.baseScale;
        // Exact art size 192x64
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        // Anchor from bottom of the screen up
        float screenH = 2556.0f;
        const float groundY = screenH - pipeH * scale;
        const float spacingX = pipeW * scale; // actual width only

        int groupId = m_nextGroupId++;
        // Enforce bottom-only variants
        const std::string texB = (rand() % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
        GN_LOG_DEBUG(std::string("Spawn BottomOnly groupId=") + std::to_string(groupId) +
                     " tex=" + texB + " startX=" + std::to_string(startX) + " bottomY=" + std::to_string(groundY));
        Gnosis::Entity e2 = m_ecsSystem->CreateEntity();
        Transform tr2(Gnosis::GNVector2(startX, groundY), 0.0f, Gnosis::GNVector2(scale, scale));
        Sprite sp2(texB, pipeW, pipeH); sp2.layer = 3; sp2.visible = true;
        Physics ph2; ph2.velocity.x = -m_currentLevelConfig.worldSpeed; ph2.useGravity = false;
        Hitbox hb2; hb2.type = ColliderType::Rectangle; hb2.width = pipeW - 12.0f; hb2.height = pipeH - 12.0f; hb2.offsetX = 0.0f; hb2.offsetY = 0.0f; hb2.isStatic = false; hb2.tag = "obstacle";
        Obstacle ob2; ob2.obstacleType = texB; ob2.damage = 1; ob2.basePosition = tr2.position;
        Group gp2; gp2.id = groupId; gp2.isLeader = true; gp2.offsetX = 0.0f; gp2.offsetY = 0.0f; gp2.groupWidth = pipeW * scale; gp2.pattern = GroupPattern::BottomOnly;
        m_ecsSystem->AddComponent<Transform>(e2, tr2);
        m_ecsSystem->AddComponent<Sprite>(e2, sp2);
        m_ecsSystem->AddComponent<Physics>(e2, ph2);
        m_ecsSystem->AddComponent<Hitbox>(e2, hb2);
        m_ecsSystem->AddComponent<Obstacle>(e2, ob2);
        // Debug hitboxes off for production visuals
        m_ecsSystem->AddComponent<Group>(e2, gp2);
        m_activeObstacles.push_back(e2);
    }

    void LevelManager::SpawnSewerPattern_TopAndBottom(float startX) {
        const float scale = m_currentLevelConfig.baseScale;
        // Exact art size 192x64
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        const float topY = 0.0f; // top-aligned
        float screenH = 2556.0f;
        const float bottomY = screenH - pipeH * scale;
        int groupId = m_nextGroupId++;

        // Top
        {
            const std::string texTop = (rand() % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue"; // top-only
            GN_LOG_DEBUG(std::string("Spawn TopAndBottom TOP groupId=") + std::to_string(groupId) + 
                         " tex=" + texTop + " x=" + std::to_string(startX) + " y=" + std::to_string(topY));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(startX, topY), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(texTop, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW - 12.0f; hb.height = pipeH - 12.0f; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = texTop; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = true;
            Group gp; gp.id = groupId; gp.isLeader = true; gp.offsetX = 0.0f; gp.offsetY = 0.0f; gp.groupWidth = pipeW * scale; gp.pattern = GroupPattern::TopAndBottom;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            // Debug hitboxes off for production visuals
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        }

        // Bottom
        {
            const std::string texBottom = (rand() % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue"; // bottom-only
            GN_LOG_DEBUG(std::string("Spawn TopAndBottom BOTTOM groupId=") + std::to_string(groupId) + 
                         " tex=" + texBottom + " x=" + std::to_string(startX) + " y=" + std::to_string(bottomY));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(startX, bottomY), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(texBottom, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW - 12.0f; hb.height = pipeH - 12.0f; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = texBottom; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = false;
            Group gp; gp.id = groupId; gp.isLeader = false; gp.offsetX = 0.0f; gp.offsetY = 0.0f; gp.groupWidth = pipeW * scale; gp.pattern = GroupPattern::TopAndBottom;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            // Debug hitboxes off for production visuals
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        }
    }

    void LevelManager::SpawnSewerPattern_Pyramid3(float startX) {
        // 3-high pyramid: bottom row 3, then 2, then 1; offset slightly right and start near top of bottom row
        const float scale = m_currentLevelConfig.baseScale;
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        const float spacingX = pipeW * scale * 1.15f;
        // Vertical stacking offsets: lift inner rows so stacked pipes sit "inside" the row below
        const float spacingY = pipeH * scale * 0.6f;
        const float row1LiftTop = 12.0f * scale;
        const float row2LiftTop = 24.0f * scale;
        const float row1Lift = 12.0f * scale;  // second row +12 px
        const float row2Lift = 24.0f * scale;  // third row +24 px
        float screenH = 2556.0f;
        const float baseY = screenH - pipeH * scale;         // bottom pipes sit on floor
        const float startYOffset = 0.0f;                       // base row sits at exact bottom
        const float startXOffset = 0.0f;                       // start exactly at startX (no half-spacing gap)
        int groupId = m_nextGroupId++;

        auto spawn = [&](float col, int row, int idx){
            const bool bottom = true;
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? (bottom ? "BottomPipeWide" : "TopPipeWide")
                                                                              : (bottom ? "BottomPipeWideBlue" : "TopPipeWideBlue");
            float x = startX + startXOffset + col * spacingX;
            float lift = (row == 1 ? row1Lift : (row == 2 ? row2Lift : 0.0f));
            float y = (baseY - row * spacingY) + startYOffset - lift;
            if (!bottom) { GN_LOG_ERROR("Pyramid3 spawn using TOP variant on bottom pyramid! idx=" + std::to_string(idx)); }
            GN_LOG_DEBUG(std::string("Spawn Pyramid3 groupId=") + std::to_string(groupId) + " idx=" + std::to_string(idx) + 
                         " tex=" + tex + " x=" + std::to_string(x) + " y=" + std::to_string(y));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW; hb.height = pipeH; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position;
            // Exact bottom row visual width = 2*spacingX + pipeW*scale
            Group gp; gp.id = groupId; gp.isLeader = (idx == 0); gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - baseY; gp.groupWidth = (2.0f * spacingX) + (pipeW * scale); gp.pattern = GroupPattern::PyramidBottom;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            // Debug hitboxes off for production visuals
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        };

        int idx = 0;
        for (int c = 0; c < 3; ++c) spawn(static_cast<float>(c), 0, idx++);     // bottom row 3
        for (int c = 0; c < 2; ++c) spawn(c + 0.5f, 1, idx++);                  // middle 2 centered
        spawn(1.0f, 2, idx++);                                                  // top 1 centered

        // Complement: single TOP row across the span to create a funnel
        const float topY = 0.0f;
        auto spawnTopRow = [&](float col){
            const std::string tex = (static_cast<int>(col) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float x = startX + startXOffset + col * spacingX;
            float y = topY; // anchored at top
            GN_LOG_DEBUG(std::string("Spawn Pyramid3 TOP complement groupId=") + std::to_string(groupId) +
                         " tex=" + tex + " x=" + std::to_string(x) + " y=" + std::to_string(y));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW; hb.height = pipeH; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = true;
            Group gp; gp.id = groupId; gp.isLeader = false; gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - topY; gp.groupWidth = (2.0f * spacingX) + (pipeW * scale); gp.pattern = GroupPattern::PyramidBottom;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        };
        for (int c = 0; c < 3; ++c) spawnTopRow(static_cast<float>(c));
    }

    // Inverted top-origin 3-high pyramid: top row 3 (TopPipe), then 2, then 1, stacking downward
    void LevelManager::SpawnSewerPattern_PyramidTop3(float startX) {
        const float scale = m_currentLevelConfig.baseScale;
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        const float spacingX = pipeW * scale * 1.15f;
        const float spacingY = pipeH * scale * 0.6f;
        const float row1LiftTop = 12.0f * scale;  // push downward to nest inside above row
        const float row2LiftTop = 24.0f * scale;
        const float topY = 0.0f; // origin at top
        int groupId = m_nextGroupId++;

        auto spawnTop = [&](float col, int row, int idx){
            // Use only top variants for top-origin stacks; center exactly on startX grid
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float x = startX + col * spacingX;
            float lift = (row == 1 ? row1LiftTop : (row == 2 ? row2LiftTop : 0.0f));
            float y = topY + row * spacingY + lift; // inverted: push rows downward inside
            GN_LOG_DEBUG(std::string("Spawn PyramidTop3 groupId=") + std::to_string(groupId) + " idx=" + std::to_string(idx) + 
                         " tex=" + tex + " x=" + std::to_string(x) + " y=" + std::to_string(y));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW; hb.height = pipeH; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = true;
            // Exact top row visual width = 2*spacingX + pipeW*scale
            bool makeLeader = (row == 0 && col == 0.0f);
            Group gp; gp.id = groupId; gp.isLeader = makeLeader; gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - topY; gp.groupWidth = (2.0f * spacingX) + (pipeW * scale); gp.pattern = GroupPattern::PyramidTop;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            // Debug hitboxes off for production visuals
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        };

        int idx = 0;
        // Construct bottom-most of the inverted stack first so the TOP row renders last
        // row 2: 1 centered under
        spawnTop(1.0f, 2, idx++);
        // row 1: 2 centered under
        for (int c = 0; c < 2; ++c) spawnTop(c + 0.5f, 1, idx++);
        // top row: 3 (rendered last for correct layering)
        for (int c = 0; c < 3; ++c) spawnTop(static_cast<float>(c), 0, idx++);

        // Complement: single BOTTOM row across the span to create a funnel
        float screenH2 = 2556.0f;
        const float bottomY2 = screenH2 - pipeH * scale;
        auto spawnBottomRow = [&](float col){
            const std::string tex = (static_cast<int>(col) % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float x = startX + col * spacingX;
            float y = bottomY2;
            GN_LOG_DEBUG(std::string("Spawn PyramidTop3 BOTTOM complement groupId=") + std::to_string(groupId) +
                         " tex=" + tex + " x=" + std::to_string(x) + " y=" + std::to_string(y));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW; hb.height = pipeH; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = false;
            Group gp; gp.id = groupId; gp.isLeader = false; gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - bottomY2; gp.groupWidth = (2.0f * spacingX) + (pipeW * scale); gp.pattern = GroupPattern::PyramidTop;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        };
        for (int c = 0; c < 3; ++c) spawnBottomRow(static_cast<float>(c));
    }

    void LevelManager::SpawnSewerPattern_TwoByTwoFunnel(float startX) {
        const float scale = m_currentLevelConfig.baseScale;
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        const float spacingX = pipeW * scale * 1.15f;
        const float spacingY = pipeH * scale * 0.6f;
        const float row1LiftTop = 12.0f * scale;
        const float row1Lift = 12.0f * scale;
        float screenH = 2556.0f;
        const float bottomY = screenH - pipeH * scale;
        const float topY = 0.0f;
        int groupId = m_nextGroupId++;

        // Helper: bottom rows
        auto spawnBottom = [&](float col, int row, int idx){
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float x = startX + col * spacingX;
            float lift = (row == 1 ? row1Lift : 0.0f);
            float y = bottomY - row * spacingY - lift;
            GN_LOG_DEBUG(std::string("Spawn TwoByTwoFunnel BOTTOM groupId=") + std::to_string(groupId) + " idx=" + std::to_string(idx) +
                         " tex=" + tex + " x=" + std::to_string(x) + " y=" + std::to_string(y));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW; hb.height = pipeH; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = false;
            Group gp; gp.id = groupId; gp.isLeader = (idx == 0); gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - bottomY; gp.groupWidth = (2.0f * spacingX) + (pipeW * scale); gp.pattern = GroupPattern::TwoFunnel;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        };
        // Helper: top rows
        auto spawnTop = [&](float col, int row){
            const std::string tex = (static_cast<int>(col + row) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float x = startX + col * spacingX;
            float lift = (row == 1 ? row1LiftTop : 0.0f);
            float y = topY + row * spacingY + lift;
            GN_LOG_DEBUG(std::string("Spawn TwoByTwoFunnel TOP groupId=") + std::to_string(groupId) +
                         " tex=" + tex + " x=" + std::to_string(x) + " y=" + std::to_string(y));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW; hb.height = pipeH; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = true;
            Group gp; gp.id = groupId; gp.isLeader = false; gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - topY; gp.groupWidth = (2.0f * spacingX) + (pipeW * scale);
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        };

        int idx = 0;
        // Bottom two rows: 3 then 2
        for (int c = 0; c < 3; ++c) spawnBottom(static_cast<float>(c), 0, idx++);
        for (int c = 0; c < 2; ++c) spawnBottom(c + 0.5f, 1, idx++);
        // Top two rows: 2-high inverted: row 1 (two) then row 0 (three) last for layering
        for (int c = 0; c < 2; ++c) spawnTop(c + 0.5f, 1);
        for (int c = 0; c < 3; ++c) spawnTop(static_cast<float>(c), 0);
    }

    void LevelManager::SpawnSewerPattern_Pyramid4(float startX) {
        // 4-high pyramid (4,3,2,1)
        const float scale = m_currentLevelConfig.baseScale;
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        const float spacingX = pipeW * scale * 1.1f;
        const float spacingY = pipeH * scale * 0.55f;
        float screenH = 2556.0f;
        const float baseY = screenH - pipeH * scale;
        int groupId = m_nextGroupId++;

        auto spawn = [&](float col, int row, int idx){
            const bool bottom = row == 0; // bottom row uses bottom variant primarily
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? (bottom ? "BottomPipeWide" : "TopPipeWide")
                                                                             : (bottom ? "BottomPipeWideBlue" : "TopPipeWideBlue");
            float x = startX + col * spacingX;
            float y = baseY - row * spacingY;
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW - 12.0f; hb.height = pipeH - 12.0f; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position;
            Group gp; gp.id = groupId; gp.isLeader = (idx == 0); gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - baseY; gp.groupWidth = 5 * spacingX;
            m_ecsSystem->AddComponent<Transform>(e, tr);
            m_ecsSystem->AddComponent<Sprite>(e, sp);
            m_ecsSystem->AddComponent<Physics>(e, ph);
            m_ecsSystem->AddComponent<Hitbox>(e, hb);
            m_ecsSystem->AddComponent<Obstacle>(e, ob);
            DebugDraw dbg(true, true, Gnosis::GNColor(0,255,0,255), Gnosis::GNColor(255,0,0,255));
            dbg.alpha = 0.5f; dbg.debugLayer = 18; m_ecsSystem->AddComponent<DebugDraw>(e, dbg);
            m_ecsSystem->AddComponent<Group>(e, gp);
            m_activeObstacles.push_back(e);
        };

        int idx = 0;
        for (int c = 0; c < 4; ++c) spawn(static_cast<float>(c), 0, idx++);
        for (int c = 0; c < 3; ++c) spawn(c + 0.5f, 1, idx++);
        for (int c = 0; c < 2; ++c) spawn(c + 1.0f, 2, idx++);
        spawn(1.5f, 3, idx++);
    }

    void LevelManager::WrapObstacleAroundScreen(Gnosis::Entity obstacle, float worldScrollDistance) {
        if (!m_ecsSystem || obstacle == 0) {
            return;
        }

        Transform* transform = m_ecsSystem->GetComponent<Transform>(obstacle);
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(obstacle);
        Obstacle* obstacleComp = m_ecsSystem->GetComponent<Obstacle>(obstacle);
        
        if (!transform || !sprite || !obstacleComp) {
            return;
        }

        // Check if obstacle is off screen to the left for wrapping
        // With stationary camera at (0,0), screen coordinates are absolute world coordinates
        float scaledWidth = sprite->width * std::abs(transform->scale.x);
        float leftEdge = transform->position.x;
        float rightEdge = leftEdge + scaledWidth;
        float screenWidth = 1179.0f; // iPhone 16 portrait width

        // FIXED: Wrap when obstacle starts going off the left side of screen
        // The user reports they disappear when touching the left edge, so trigger earlier
        // Use a more lenient condition that matches what the user observes
        float wrapBuffer = 0.0f; // No buffer - wrap as soon as right edge hits left screen edge
        bool shouldWrap = (rightEdge <= 0.0f);

        GN_LOG_DEBUG("Obstacle wrap check: leftEdge=" + std::to_string(leftEdge) +
                     ", rightEdge=" + std::to_string(rightEdge) +
                     ", scaledWidth=" + std::to_string(scaledWidth) +
                     ", shouldWrap=" + std::to_string(shouldWrap));
        
        if (shouldWrap) {
            // FIXED QUEUE SYSTEM: Find the rightmost obstacle position to queue this one behind it
            // The old scripts used a simple circular queue - replicate that logic
            float rightmostX = screenWidth; // Start from right edge of screen as minimum
            int totalObstacles = 0;
            int obstaclesChecked = 0;
        
            GN_LOG_DEBUG("=== WRAPPING DEBUG: Finding rightmost obstacle ===");
            GN_LOG_DEBUG("World Scroll Distance: " + std::to_string(worldScrollDistance));
            GN_LOG_DEBUG("Screen Width: " + std::to_string(screenWidth));
            GN_LOG_DEBUG("Starting rightmost search from: " + std::to_string(rightmostX));
            GN_LOG_DEBUG("Current obstacle X: " + std::to_string(transform->position.x));
            GN_LOG_DEBUG("Total active obstacles: " + std::to_string(m_activeObstacles.size()));
        
            // Find the actual rightmost obstacle position (excluding the one being wrapped)
            for (Gnosis::Entity otherObstacle : m_activeObstacles) {
                totalObstacles++;
                if (otherObstacle != obstacle) {
                    obstaclesChecked++;
                    Transform* otherTransform = m_ecsSystem->GetComponent<Transform>(otherObstacle);
                    Sprite* otherSprite = m_ecsSystem->GetComponent<Sprite>(otherObstacle);
                    if (otherTransform && otherSprite) {
                        // FIXED: Find rightmost LEFT EDGE to match initialization spacing logic
                        // Initialization uses leftEdge + spacing, so wrapping should too
                        float otherLeftEdge = otherTransform->position.x;
                    
                        GN_LOG_DEBUG("Obstacle " + std::to_string(obstaclesChecked) + " leftEdge: " + std::to_string(otherLeftEdge));
                    
                        // Use the rightmost left edge to match initialization spacing
                        if (otherLeftEdge > rightmostX) {
                            float previousRightmost = rightmostX;
                            rightmostX = otherLeftEdge;
                            GN_LOG_DEBUG("New rightmost leftEdge: " + std::to_string(rightmostX) + " (was: " + std::to_string(previousRightmost) + ")");
                        }
                    }
                }
            }
            
            // Place this single obstacle after the rightmost group's width
            float newX = rightmostX + (sprite->width * std::abs(transform->scale.x));
        
            // Ensure minimum distance from screen edge to prevent immediate re-wrapping
            float minDistanceFromScreen = screenWidth + 100.0f;
            if (newX < minDistanceFromScreen) {
                newX = minDistanceFromScreen;
                GN_LOG_DEBUG("Adjusted newX to minimum distance: " + std::to_string(newX));
            }
            
            GN_LOG_DEBUG("=== WRAPPING RESULT ===");
            GN_LOG_DEBUG("Rightmost X found: " + std::to_string(rightmostX));
            GN_LOG_DEBUG("Obstacle spacing: " + std::to_string(m_obstacleSpacing));
            GN_LOG_DEBUG("New X position: " + std::to_string(newX));
            
            transform->position.x = newX;
        
        // CRITICAL FIX: Reset collision state when wrapping to prevent collision detection from breaking
        obstacleComp->pipeCleared = false;  // Reset pipe cleared flag so collision works again
        
        // UPDATED TOILET POSITIONING - use same improved ground rules as spawning
        if (obstacleComp->pairedEntity != 0) {
                Transform* pairedTransform = m_ecsSystem->GetComponent<Transform>(obstacleComp->pairedEntity);
                Obstacle* pairedObstacle = m_ecsSystem->GetComponent<Obstacle>(obstacleComp->pairedEntity);
                
                if (pairedTransform && pairedObstacle) {
                    float screenHeight = 2556.0f; // TODO: Get from platform delegates  
                    float toiletHeight = sprite->height * transform->scale.y; // Scaled toilet height
                    
                    // Use same improved positioning: top toilet closer to screen, larger gap - UPDATED
                    float minTopY = -toiletHeight * 0.8f; // Closer to screen top (less negative)
                    float maxTopY = -toiletHeight * 0.2f; // Even closer to screen top
                    
                    // Generate new random position for top toilet within allowed range (all negative Y)
                    float randomTopY = minTopY + (maxTopY - minTopY) * ((float)rand() / RAND_MAX);
                    
                    // Reduced gap height for tighter gameplay: match initial spawn (309)
                    float fixedGapHeight = 309.0f;
                    
                    // Calculate bottom toilet position: top toilet bottom + fixed gap
                    float bottomToiletY = randomTopY + toiletHeight + fixedGapHeight;
                    
                    if (obstacleComp->isTopPart) {
                        // This is top toilet, position it with new random position
                        transform->position.y = randomTopY;
                        // Position bottom toilet consistently
                        pairedTransform->position.x = newX;
                        pairedTransform->position.y = bottomToiletY;
                    } else {
                        // This is bottom toilet, calculate from top toilet position
                        transform->position.y = bottomToiletY;
                        // Position top toilet consistently  
                        pairedTransform->position.x = newX;
                        pairedTransform->position.y = randomTopY;
                    }
                    
                    // CRITICAL FIX: Reset collision state for BOTH toilets in the pair
                    pairedObstacle->pipeCleared = false;
                    
                    // Update both base positions
                    obstacleComp->basePosition = transform->position;
                    pairedObstacle->basePosition = pairedTransform->position;
                    pairedObstacle->oscillationTimer = 0.0f;
                    
                    GN_LOG_DEBUG("Wrapped toilet pair to x=" + std::to_string(newX) + 
                                ", topY=" + std::to_string(randomTopY) + 
                                ", bottomY=" + std::to_string(bottomToiletY) + 
                                ", gap=" + std::to_string(fixedGapHeight) + 
                                " (improved positioning: lower top, larger gaps)");
                }
            } else {
                // Single obstacle - update base position for oscillation calculations
                obstacleComp->basePosition = Gnosis::GNVector2(newX, transform->position.y);
                obstacleComp->oscillationTimer = 0.0f;
                
                GN_LOG_DEBUG("Wrapped single obstacle to x=" + std::to_string(newX));
            }
        }
    }

} // namespace GameCore
