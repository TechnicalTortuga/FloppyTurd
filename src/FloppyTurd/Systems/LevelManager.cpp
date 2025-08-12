#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Assets/TextureManager.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <deque>

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
        , m_npcSpawnTimer(0.0f)
        , m_lastObstacleX(1000.0f)   // Start obstacles off screen to the right
        , m_lastEnemyX(1200.0f)      // Start enemies further out
        , m_lastPickupX(800.0f)      // Start pickups closer
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
        m_pickupSpawnTimer = 0.0f;
        m_npcSpawnTimer = 0.0f;
        m_lastObstacleX = 1000.0f;
        m_lastEnemyX = 1200.0f;
        m_lastPickupX = 800.0f;
        
        // Set loaded flag BEFORE initializing obstacle pool
        m_isLoaded = true;
        
        // Initialize object pool instead of timer-based spawning
        m_poolInitialized = false;
        InitializeObstaclePool();
        m_enemyPoolInitialized = false;
        m_npcPoolInitialized = false;
        m_pickupPoolInitialized = false;
        m_projectilePoolInitialized = false;
        InitializeEnemyPool();
        InitializeNPCPool();
        InitializePickupPool();
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
        for (int i = 0; i < m_maxActiveEnemies; ++i) {
            float x = startX + i * m_enemySpacing;
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
        // Align to sewer background pixel band: 20px * 5x = 100px above bottom of image band
        float spawnY = screenH - janitorHeight - 100.0f;
        GN_LOG_INFO("NPC Janitor init: spawnX=" + std::to_string(spawnX) + ", spawnY=" + std::to_string(spawnY));
        m_janitorEntity = SpawnNPCJanitor(spawnX, spawnY);
        if (m_janitorEntity != 0) m_activeNPCs.push_back(m_janitorEntity);
        m_npcPoolInitialized = true;
    }

    void LevelManager::InitializePickupPool() {
        if (m_pickupPoolInitialized) return;
        if (!m_currentLevelConfig.enablePickups) { m_pickupPoolInitialized = true; return; }
        // Only create pickups for Level 2 (Sewer) here.
        if (m_currentLevelId != 2) { m_pickupPoolInitialized = true; return; }
        // Sewer level: only GoldCoin (+1) and small PooHeart (32x32) as a rare pickup
        const int coinPoolSize = 12; // enough for two 5-stacks + buffer
        const int heartPoolSize = 2; // rare
        float screenW = 1179.0f;
        float startX = screenW + 100.0f;
        for (int i = 0; i < coinPoolSize; ++i) {
            Gnosis::Entity e = SpawnPickup("GoldCoin", startX + i * 40.0f, 200.0f);
            if (e != 0) m_pickupPool.push_back(e);
        }
        for (int i = 0; i < heartPoolSize; ++i) {
            Gnosis::Entity e = SpawnPickup("PooHeart", startX + i * 60.0f, 300.0f);
            if (e != 0) m_pickupPool.push_back(e);
        }
        m_pickupPoolInitialized = true;
    }

    void LevelManager::InitializeProjectilePool() {
        if (m_projectilePoolInitialized) return;
        // Player projectiles pool (if/when used)
        // We will allocate placeholders when first shot to avoid cold-start allocation hiccup
        m_projectilePoolInitialized = true;
    }

    void LevelManager::UpdatePickupPooling(float, float) {
        if (!m_pickupPoolInitialized || !m_currentLevelConfig.enablePickups) return;
        // Ensure we maintain active coin patterns on screen. When a pattern exits, reposition as new pattern.
        float screenW = 1179.0f;
        // Build a pickup group on demand if queue is empty
        if (m_coinPatterns.empty()) {
            float startX = screenW + 200.0f;
            float midY = 500.0f;
            int count = 3 + (rand() % 3); // 3..5
            SpawnPickupGroup(startX, midY, count);
        }
        // For each active pattern, if it leaves screen, respawn to the right as a new pattern
        float rightmostX = screenW;
        for (Gnosis::Entity e : m_activePickups) {
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            if (t && t->position.x > rightmostX) rightmostX = t->position.x;
        }
        for (auto& group : m_coinPatterns) {
            bool groupOff = true;
            float groupLeft = 1e9f;
            for (Gnosis::Entity e : group) {
                Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                if (!t || !s) continue;
                float rightEdge = t->position.x + s->width * std::abs(t->scale.x);
                groupLeft = std::min(groupLeft, t->position.x);
                if (rightEdge >= 0.0f) { groupOff = false; }
            }
            if (groupOff) {
                // Move this group to the right and re-roll each entity coin/heart in-place
                float startX = rightmostX + 220.0f;
                float midY = 420.0f + static_cast<float>((rand()%300) - 150);
                int newCount = 3 + (rand()%3); // 3..5
                RerollPickupGroupInPlace(group, startX, midY, newCount);
                float spacing = 32.0f * m_currentLevelConfig.baseScale * 1.25f;
                rightmostX = startX + newCount * spacing;
            }
        }
    }

    void LevelManager::SpawnPickupGroup(float startX, float midY, int count) {
        const float scale = m_currentLevelConfig.baseScale;
        float spacing = 32.0f * scale * 1.25f;
        // Reserve a fixed-size group vector once and reuse
        std::vector<Gnosis::Entity> group;
        group.reserve(5);
        int used = 0;
        size_t poolIdx = 0;
        while (used < count && poolIdx < m_pickupPool.size()) {
            Gnosis::Entity e = m_pickupPool[poolIdx++];
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
            Pickup* p = m_ecsSystem->GetComponent<Pickup>(e);
            if (!t || !s || !p) continue;
            bool isHeart = (rand()%100) < 10;
            s->textureId = isHeart ? std::string("PooHeart") : std::string("GoldCoin");
            p->pickupType = s->textureId;
            p->value = (s->textureId == "GoldCoin") ? 1 : 0;
            s->visible = true;
            p->isActive = true;
            t->position.x = startX + used * spacing;
            t->position.y = midY + ((used % 2 == 1) ? -spacing * 0.3f : 0.0f);
            group.push_back(e);
            if (std::find(m_activePickups.begin(), m_activePickups.end(), e) == m_activePickups.end()) m_activePickups.push_back(e);
            used++;
        }
        if (!group.empty()) m_coinPatterns.push_back(group);
    }

    void LevelManager::RerollPickupGroupInPlace(std::vector<Gnosis::Entity>& group, float startX, float midY, int count) {
        const float scale = m_currentLevelConfig.baseScale;
        float spacing = 32.0f * scale * 1.25f;
        // Ensure group has capacity up to 5
        if (group.capacity() < 5) group.reserve(5);
        // If group has fewer entities than needed, top up with pool entities
        size_t poolIdx = 0;
        while (group.size() < static_cast<size_t>(count) && poolIdx < m_pickupPool.size()) {
            Gnosis::Entity e = m_pickupPool[poolIdx++];
            if (std::find(group.begin(), group.end(), e) == group.end()) {
                group.push_back(e);
            }
        }
        // Configure first 'count' entities; hide the rest if any
        for (size_t i = 0; i < group.size(); ++i) {
            Gnosis::Entity e = group[i];
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
            Pickup* p = m_ecsSystem->GetComponent<Pickup>(e);
            if (!t || !s || !p) continue;
            if (i < static_cast<size_t>(count)) {
                bool isHeart = (rand()%100) < 10;
                s->textureId = isHeart ? std::string("PooHeart") : std::string("GoldCoin");
                p->pickupType = s->textureId;
                p->value = (s->textureId == "GoldCoin") ? 1 : 0;
                s->visible = true;
                p->isActive = true;
                t->position.x = startX + static_cast<float>(i) * spacing;
                t->position.y = midY + ((i % 2 == 1) ? -spacing * 0.3f : 0.0f);
                if (std::find(m_activePickups.begin(), m_activePickups.end(), e) == m_activePickups.end()) m_activePickups.push_back(e);
            } else {
                // Hide unused tail
                s->visible = false;
                p->isActive = false;
            }
        }
    }

    // MaybeQueueHeart removed: we now place hearts directly from the pool inline where needed

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
                t->position.x = rightmostX + m_enemySpacing;
                float baseY = 900.0f + static_cast<float>((rand()%300) - 150);
                t->position.y = baseY;
                m_enemyBaseY[e] = baseY;
                rightmostX = t->position.x;
                GN_LOG_DEBUG("Enemy wrap: newX=" + std::to_string(t->position.x) + ", baseY=" + std::to_string(baseY));
            }
            // Sinusoidal motion around base Y
            auto it = m_enemyBaseY.find(e);
            if (it != m_enemyBaseY.end()) {
                float time = worldScrollDistance * 0.0035f; // tie to scroll for deterministic path
                float amplitude = 110.0f;
                t->position.y = it->second + std::sin(time + (t->position.x * 0.004f)) * amplitude;
                // Log occasionally to verify sinusoid (lightweight)
                if (((int)worldScrollDistance % 2000) == 0) {
                    GN_LOG_DEBUG("Enemy sinusoid: x=" + std::to_string(t->position.x) + ", y=" + std::to_string(t->position.y));
                }
            }
        }
    }

    void LevelManager::UpdateNPCPooling(float, float) {
        if (!m_npcPoolInitialized || m_janitorEntity == 0) return;
        // If Janitor goes off-screen left, move him to the right again at ground Y
        Transform* t = m_ecsSystem->GetComponent<Transform>(m_janitorEntity);
        Sprite* s = m_ecsSystem->GetComponent<Sprite>(m_janitorEntity);
        if (!t || !s) return;
        float screenW = 1179.0f, screenH = 2556.0f;
        float rightEdge = t->position.x + s->width * std::abs(t->scale.x);
        if (rightEdge < 0.0f) {
            float janitorHeight = 64.0f * m_currentLevelConfig.baseScale;
            t->position.x = screenW + 220.0f;
            // Align using parallax back layer's scale to match pixel band: 20px * backgroundScale
            float backScale = (m_currentLevelConfig.backgroundLayers.size() > 1) ? m_currentLevelConfig.backgroundLayers[1].scaleMultiplier : 1.0f;
            float offsetPx = 20.0f * 5.0f; // Sewer backgrounds scaled by ~5x to screen height
            t->position.y = screenH - janitorHeight - offsetPx;
            GN_LOG_INFO("NPC Janitor wrap: newX=" + std::to_string(t->position.x) + ", newY=" + std::to_string(t->position.y));
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
        sp.frameTime = 0.08f; // faster sweep
        sp.currentFrame = 0;
        // Match parallax back band speed from config (first back layer if available)
        // Use EXACT back parallax scroll speed (Layer 1 index) so he drifts slower than pipes
        float janitorSpeed = 60.0f;
        if (m_currentLevelConfig.backgroundLayers.size() > 1) {
            janitorSpeed = m_currentLevelConfig.backgroundLayers[1].scrollSpeed;
        }
        Physics ph; ph.velocity.x = -janitorSpeed; ph.useGravity = false;
        Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = 48.0f; hb.height = 48.0f; hb.isTrigger = true; hb.tag = "NPC";
        NPC npcComp; npcComp.type = "Janitor"; npcComp.state = 0; npcComp.timer = 0.0f; npcComp.triggered = false;

        m_ecsSystem->AddComponent<Transform>(npc, tr);
        m_ecsSystem->AddComponent<Sprite>(npc, sp);
        // Attach declarative animation set for state-driven control
        StateAnimation sa;
        {
            StateAnimation::Clip sweep; sweep.textureId = "JanitorSweep"; sweep.frameWidth = 64; sweep.frameHeight = 64; sweep.frameCount = 4; sweep.frameTime = 0.08f; sweep.loop = true;
            StateAnimation::Clip surprise; surprise.textureId = "JanitorSurprise"; surprise.frameWidth = 64; surprise.frameHeight = 64; surprise.frameCount = 8; surprise.frameTime = 0.09f; surprise.loop = false;
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

    void LevelManager::UpdatePickupSpawning(float deltaTime) {
        if (!m_isLoaded) {
            return;
        }
        
        // Skip pickup spawning if spawn rate is 0 (disabled for this level)
        if (m_currentLevelConfig.pickupSpawnRate <= 0.0f) {
            // Log active pickup count to help debug
            GN_LOG_DEBUG("Pickup spawning disabled for this level (spawn rate: " + std::to_string(m_currentLevelConfig.pickupSpawnRate) + "), active pickups: " + std::to_string(m_activePickups.size()));
            return;
        }
        
        GN_LOG_DEBUG("Pickup spawning active (spawn rate: " + std::to_string(m_currentLevelConfig.pickupSpawnRate) + ")");
        
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
            sprite.frameTime = 0.12f; // ~8.3 fps
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
        
        // Sewer: only GoldCoin (counts +1) and small PooHeart 32x32
        if (type == "GoldCoin") {
            // Gold coin animation: keep as 10 frames of 16x16 if spritesheet implies; render scaled by baseScale
            width = height = 16.0f;
            sprite.width = width; sprite.height = height;
            sprite.isAnimated = true;
            sprite.frameWidth = 16; sprite.frameHeight = 16; 
            sprite.frameCount = 10; sprite.frameTime = 0.1f;
            sprite.currentFrame = 0;
            sprite.currentFrameTime = 0.0f;
            sprite.playing = true;
            sprite.loop = true; // Coins loop infinitely
            sprite.hasCompleted = false;
            
            GN_LOG_DEBUG("LevelManager: Created animated gold coin with 10 frames of 16x16");
        } else if (type == "PooHeart") {
            width = height = 32.0f; // Small heart 32x32
            sprite.width = width; sprite.height = height;
            sprite.isAnimated = false; // Static heart
        } else {
            // Default pickup configuration
            sprite.isAnimated = false;
        }
        
        // Create physics
        Physics physics;
        physics.velocity.x = -m_currentLevelConfig.worldSpeed; // Move left with world
        
        // Create hitbox  
        Hitbox collider;
        collider.isStatic = false;
        collider.width = width;
        collider.height = height;
        collider.tag = "Pickup";
        
        // Create pickup component
        Pickup pickupComp;
        pickupComp.pickupType = type;
        pickupComp.value = (type == "GoldCoin") ? 1 : 0; // Only coins add to coin counter
        pickupComp.isActive = true;
        
        // Add components
        m_ecsSystem->AddComponent<Transform>(pickup, transform);
        m_ecsSystem->AddComponent<Sprite>(pickup, sprite);
        m_ecsSystem->AddComponent<Physics>(pickup, physics);
        m_ecsSystem->AddComponent<Hitbox>(pickup, collider);
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
        
        // Clean up pickups that have moved off screen
        // FIXED: Use right edge of entity for proper cleanup, like toilet logic
        for (auto it = m_activePickups.begin(); it != m_activePickups.end();) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(*it);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(*it);
            if (transform && sprite) {
                // Calculate right edge of pickup for proper cleanup
                float scaledWidth = sprite->width * std::abs(transform->scale.x);
                float rightEdge = transform->position.x + scaledWidth;
                if (rightEdge < leftBoundary) {
                    m_ecsSystem->DestroyEntity(*it);
                    it = m_activePickups.erase(it);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }

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
        
        // Create initial pool: place patterns back-to-back using each group's actual width, not a fixed spacing
        float screenWidth = 1179.0f;
        float initialCameraX = 0.0f;
        float nextWorldX = screenWidth + 100.0f + initialCameraX;
        const float safeGap = 160.0f; // slight breathing room between groups
        
        for (int i = 0; i < OBSTACLE_POOL_SIZE; i++) {
            GN_LOG_DEBUG("Spawning pooled pattern " + std::to_string(i) + " at world X: " + std::to_string(nextWorldX));
            // Capture the next group id before spawn (spawners increment it once per group)
            int groupIdBefore = m_nextGroupId;
            if (m_currentLevelId == 2) {
                int pattern = rand() % 5;
                switch (pattern) {
                    case 0: SpawnSewerPattern_TopOnly(nextWorldX); break;
                    case 1: SpawnSewerPattern_BottomOnly(nextWorldX); break;
                    case 2: SpawnSewerPattern_TopAndBottom(nextWorldX); break;
                    case 3: SpawnSewerPattern_Pyramid3(nextWorldX); break;
                    default: SpawnSewerPattern_PyramidTop3(nextWorldX); break;
                }
            } else {
            if (baseConfig.spawnAsPair && !baseConfig.bottomTextureId.empty()) {
                    SpawnToiletPairWithGap(baseConfig, nextWorldX, 0.0f, 0.0f);
            } else {
                    float spawnY = 500.0f;
                    SpawnObstacle(baseConfig, nextWorldX, spawnY);
                }
            }
            // Determine spawned group's width from the leader's Group component
            int justSpawnedGroupId = groupIdBefore; // spawners incremented m_nextGroupId
            float spawnedGroupWidth = 600.0f; // fallback
            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->id == justSpawnedGroupId && g->isLeader) {
                    spawnedGroupWidth = g->groupWidth;
                    break;
                }
            }
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
        const float screenCenterX = 1179.0f * 0.35f; // trigger slightly left of center so it's visible
        for (Gnosis::Entity e : m_activeNPCs) {
            NPC* npc = m_ecsSystem->GetComponent<NPC>(e);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(e);
            Transform* tr = m_ecsSystem->GetComponent<Transform>(e);
            if (!npc || !sprite || !tr) continue;

            if (npc->state == 0) {
                // sweeping -> surprised when the player has presumably passed
                if (!npc->triggered && tr->position.x < screenCenterX) {
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
        Gnosis::Entity leaderEntity = 0;

        for (Gnosis::Entity e : m_activeObstacles) {
            Group* g = m_ecsSystem->GetComponent<Group>(e);
            if (g && g->id == groupId && g->isLeader) {
                leaderTransform = m_ecsSystem->GetComponent<Transform>(e);
                leaderSprite = m_ecsSystem->GetComponent<Sprite>(e);
                leaderEntity = e;
                groupWidth = g->groupWidth;
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
            // Find rightmost leader
            float rightmostX = screenWidth;
            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->isLeader) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    if (t && t->position.x > rightmostX) rightmostX = t->position.x;
                }
            }

            // Compute actual group width (leader's groupWidth if present), and place next using that width
            float groupSpan = groupWidth > 0.0f ? groupWidth : (scaledWidth + 40.0f);
            float newX = rightmostX + groupSpan;
            // Move entire group preserving offsets
            float originY = leaderTransform->position.y; // keep same Y baseline
            for (Gnosis::Entity e : m_activeObstacles) {
                Group* g = m_ecsSystem->GetComponent<Group>(e);
                if (g && g->id == groupId) {
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    if (t) {
                        t->position.x = newX + (g->isLeader ? 0.0f : g->offsetX);
                        t->position.y = originY + g->offsetY;
                    }
                    Obstacle* o = m_ecsSystem->GetComponent<Obstacle>(e);
                    if (o) o->pipeCleared = false;
                }
            }
        }
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
        const float spacingX = pipeW * scale * 1.6f; // more separation

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
        Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position;
        Group gp; gp.id = groupId; gp.isLeader = true; gp.offsetX = 0.0f; gp.offsetY = 0.0f; gp.groupWidth = pipeW * scale * 1.6f;
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
        const float spacingX = pipeW * scale * 1.6f; // more separation

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
        Group gp2; gp2.id = groupId; gp2.isLeader = true; gp2.offsetX = 0.0f; gp2.offsetY = 0.0f; gp2.groupWidth = pipeW * scale * 1.6f;
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
            Group gp; gp.id = groupId; gp.isLeader = true; gp.offsetX = 0.0f; gp.offsetY = 0.0f; gp.groupWidth = pipeW * scale * 1.6f;
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
            Group gp; gp.id = groupId; gp.isLeader = false; gp.offsetX = 0.0f; gp.offsetY = 0.0f; gp.groupWidth = pipeW * scale * 1.6f;
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
        const float startYOffset = spacingY * 0.1f;           // start near top of bottom row
        const float startXOffset = spacingX * 0.5f;           // offset to the right a bit
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
            Group gp; gp.id = groupId; gp.isLeader = (idx == 0); gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - baseY; gp.groupWidth = 4 * spacingX;
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
    }

    // Inverted top-origin 3-high pyramid: top row 3 (TopPipe), then 2, then 1, stacking downward
    void LevelManager::SpawnSewerPattern_PyramidTop3(float startX) {
        const float scale = m_currentLevelConfig.baseScale;
        int metaW = 192, metaH = 64;
        const float pipeW = static_cast<float>(metaW);
        const float pipeH = static_cast<float>(metaH);
        const float spacingX = pipeW * scale * 1.15f;
        const float spacingY = pipeH * scale * 0.6f;
        const float row1LiftTop = 12.0f * scale;
        const float row2LiftTop = 24.0f * scale;
        const float topY = 0.0f; // origin at top
        int groupId = m_nextGroupId++;

        auto spawnTop = [&](float col, int row, int idx){
            // Use only top variants for top-origin stacks
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float x = startX + col * spacingX;
            float lift = (row == 1 ? row1LiftTop : (row == 2 ? row2LiftTop : 0.0f));
            float y = topY + row * spacingY - lift; // nudge rows into the pipes visually
            GN_LOG_DEBUG(std::string("Spawn PyramidTop3 groupId=") + std::to_string(groupId) + " idx=" + std::to_string(idx) + 
                         " tex=" + tex + " x=" + std::to_string(x) + " y=" + std::to_string(y));
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            Transform tr(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite sp(tex, pipeW, pipeH); sp.layer = 3; sp.visible = true;
            Physics ph; ph.velocity.x = -m_currentLevelConfig.worldSpeed; ph.useGravity = false;
            Hitbox hb; hb.type = ColliderType::Rectangle; hb.width = pipeW; hb.height = pipeH; hb.isStatic = false; hb.tag = "obstacle";
            Obstacle ob; ob.obstacleType = tex; ob.damage = 1; ob.basePosition = tr.position; ob.isTopPart = true;
            Group gp; gp.id = groupId; gp.isLeader = (idx == 0); gp.offsetX = tr.position.x - startX; gp.offsetY = tr.position.y - topY; gp.groupWidth = 4 * spacingX;
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
        // top row: 3
        for (int c = 0; c < 3; ++c) spawnTop(static_cast<float>(c), 0, idx++);
        // row 1: 2 centered under
        for (int c = 0; c < 2; ++c) spawnTop(c + 0.5f, 1, idx++);
        // row 2: 1 centered under
        spawnTop(1.0f, 2, idx++);
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
            
            // FIXED: Queue this obstacle after the rightmost one with proper spacing
            // The old scripts maintained consistent spacing between obstacle pairs
            float newX = rightmostX + m_obstacleSpacing;
        
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
