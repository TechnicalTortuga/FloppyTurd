#include "ObstacleSystem.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace GameCore {

    ObstacleSystem::ObstacleSystem(Gnosis::ECS* ecsSystem) 
        : m_ecsSystem(ecsSystem), m_initialized(false), m_currentLevelId(0), m_nextGroupId(1), m_baseScale(1.0f), m_debugMode(true) {
        GN_LOG_INFO("ObstacleSystem created");
    }

    ObstacleSystem::~ObstacleSystem() {
        Cleanup();
        GN_LOG_INFO("ObstacleSystem destroyed");
    }

    void ObstacleSystem::InitializeForLevel(int levelId, const LevelConfig& config) {
        m_currentLevelId = levelId;
        m_levelConfig = &config;  // Store reference to level config
        m_initialized = true;
        m_nextGroupId = 1;
        
        // Clear previous state
        m_activeObstacles.clear();
        
        // Set level-specific parameters
        m_baseScale = config.baseScale;
        m_worldSpeed = config.worldSpeed;

        // Disable debug mode for castle and boss levels to remove unwanted debug rectangles
        if (levelId == 5 || levelId == 6) {
            m_debugMode = false;
            // Force remove ALL DebugDraw components for castle/boss levels
            RemoveAllDebugDraws();
            GN_LOG_INFO("Level " + std::to_string(levelId) + ": Debug mode disabled and all DebugDraw components removed");
        }

        GN_LOG_INFO("Initializing ObstacleSystem for level " + std::to_string(levelId));

        // Setup level-specific patterns
        m_levelPatterns.clear();
        switch (levelId) {
            case 1: // Park
                m_levelPatterns.push_back({PatternType::PARK_TOILET_PAIR, 1.0f, 0.0f});
                break;
                
            case 2: // Sewer
                m_levelPatterns.push_back({PatternType::SEWER_TOP_ONLY, 1.0f, 0.0f});
                m_levelPatterns.push_back({PatternType::SEWER_BOTTOM_ONLY, 1.0f, 0.0f});
                m_levelPatterns.push_back({PatternType::SEWER_TOP_AND_BOTTOM, 1.0f, 0.0f});
                m_levelPatterns.push_back({PatternType::SEWER_PYRAMID_3, 1.0f, 0.0f});
                m_levelPatterns.push_back({PatternType::SEWER_PYRAMID_TOP_3, 1.0f, 0.0f});
                m_levelPatterns.push_back({PatternType::SEWER_TWO_BY_TWO_FUNNEL, 1.0f, 0.0f});
                break;
                
            case 3: // Desert
                m_levelPatterns.push_back({PatternType::DESERT_OUTHOUSE, 1.0f, 0.0f});
                // Temporarily removed cacti obstacles
                // m_levelPatterns.push_back({PatternType::DESERT_CACTUS, 0.3f, 0.0f});
                break;
                
            case 4: // Snow
                // HORIZONTAL spacing between toilet pairs: 1800px for ample coin room
                m_levelPatterns.push_back({PatternType::SNOW_TOILET_PAIR, 1.0f, 1800.0f});
                break;
                
            case 5: // Castle
                m_levelPatterns.push_back({PatternType::CASTLE_GOLD_TOILET_PAIR, 1.0f, 2000.0f});
                // Note: Decorative elements are now spawned as part of toilet group spawning
                break;

            case 6: // Boss level - no obstacles
                // Boss level has no procedural obstacle patterns
                AddBossLevelDecorations();
                break;

            default:
                m_levelPatterns.push_back({PatternType::PARK_TOILET_PAIR, 1.0f, 0.0f});
                break;
        }

        // Initialize cactus system for desert level
        if (levelId == 3) { // Desert level
            InitializeCactusSystem();
        }

        // Remove any existing debug components if debug mode is disabled
        if (!m_debugMode) {
            RemoveAllDebugDraws();
        }

        // NOTE: Initial obstacle spawning now handled by LevelManager using orchestrator
        // ObstacleSystem only sets up system parameters (scale, speed, patterns, cactus)
        // LevelManager will call SpawnInitialGroups() after this initialization

        m_initialized = true;
        GN_LOG_INFO("ObstacleSystem initialized for level " + std::to_string(levelId) + " (initial groups will be spawned by LevelManager orchestrator)");
    }

    void ObstacleSystem::Update(float deltaTime, float worldScrollDistance) {
        if (!m_initialized) {
            return;
        }
        
        // ORCHESTRATOR PATTERN: Wrap detection now handled by LevelManager
        // ObstacleSystem only updates animations and visual effects

        // Update obstacle oscillation (for snow toilets and gold toilets)
        UpdateObstacleOscillation(deltaTime);
        
        // Update spike ball rotations for castle level
        if (m_currentLevelId == 5 && !m_spikeBallRotationSpeeds.empty()) {
            UpdateSpikeBallRotations(deltaTime);
        }

        // Update cactus system for desert level
        if (m_currentLevelId == 3 && m_cactusPoolInitialized) {
            UpdateCactusAnimation(deltaTime);
            WrapCactusPool(worldScrollDistance);
        }

        // Debug rendering for hitboxes
        if (m_debugMode) {
            RenderDebugHitboxes();
        }
    }

    void ObstacleSystem::Cleanup() {
        if (!m_ecsSystem) {
            return;
        }

        for (Gnosis::Entity entity : m_activeObstacles) {
            m_ecsSystem->DestroyEntity(entity);
        }

        // Clean up cactus pool
        for (Gnosis::Entity entity : m_cactusPool) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_cactusTypeMap.clear();

        m_activeObstacles.clear();
        m_levelPatterns.clear();
        m_cactusPool.clear();
        m_cactusTypes.clear();
        m_spikeBallRotationSpeeds.clear();
        
        m_nextGroupId = 1;
        m_initialized = false;
        m_cactusPoolInitialized = false;
        
        GN_LOG_INFO("ObstacleSystem cleaned up");
    }

    // LEGACY DELETED: SpawnPattern() and SpawnRandomPatternForLevel()
    // These methods called spawn functions without groupId parameter
    // Replaced by orchestrator pattern where LevelManager directly calls individual
    // Spawn*Pattern_* functions with groupId and builds GroupManifests

    // ============================================================================
    // Pattern Implementations
    // ============================================================================

    std::pair<Gnosis::Entity, Gnosis::Entity> ObstacleSystem::SpawnParkPattern_ToiletPair(float x, int groupId) {
        // Use exact toilet positioning logic from old LevelManager SpawnToiletPairWithGap
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenHeight = screenInfo.pixelHeight;
        float toiletHeight = 190.0f * m_baseScale; // Scaled toilet height
        
        // Top toilet should be above screen but not too far - UPDATED for better center gap positioning
        float minTopY = -toiletHeight * 0.8f; // Closer to screen top (less negative)
        float maxTopY = -toiletHeight * 0.2f; // Even closer to screen top
        
        // Generate random position for top toilet within allowed range (all negative Y)
        float randomTopY = minTopY + (maxTopY - minTopY) * ((float)rand() / RAND_MAX);
        
        // Vertical gap within toilet pairs set to 800px
        float fixedGapHeight = 800.0f;
        
        // Calculate bottom toilet position: top toilet bottom + large fixed gap
        float topToiletY = randomTopY;
        float bottomToiletY = randomTopY + toiletHeight + fixedGapHeight;
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        // Create top toilet with exact old LevelManager logic
        Gnosis::Entity topToilet = m_ecsSystem->CreateEntity();
        
        Transform topTransform(Gnosis::GNVector2(x, topToiletY), 0.0f, 
                              Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite topSprite("TopToilet", 65.0f, 190.0f);
        topSprite.layer = 3;
        topSprite.visible = true;
        
        Physics topPhysics;
        topPhysics.velocity.x = -m_worldSpeed;
        topPhysics.useGravity = false;
        
        Hitbox topCollider;
        topCollider.type = ColliderType::Rectangle;
        // Consistent hitbox configuration: 20px width, 226px height, starts 15px down from top (visually lower)
        topCollider.width = 20.0f;
        topCollider.height = 226.0f;
        topCollider.offsetX = 0.0f;
        topCollider.offsetY = 15.0f; // Visually lower from 25.0f to 15.0f (added 5px back)
        topCollider.isStatic = false;
        topCollider.isTrigger = false;
        topCollider.tag = "obstacle";
        
        Obstacle topObstacle;
        topObstacle.type = ObstacleType::TopToilet;  // NEW: Use enum
        topObstacle.damage = 1;
        topObstacle.basePosition = Gnosis::GNVector2(x, topToiletY);
        topObstacle.isTopPart = true;
        
        // Create bottom toilet with exact old LevelManager logic
        Gnosis::Entity bottomToilet = m_ecsSystem->CreateEntity();
        
        Transform bottomTransform(Gnosis::GNVector2(x, bottomToiletY), 0.0f, 
                                 Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite bottomSprite("BottomToilet", 65.0f, 190.0f);
        bottomSprite.layer = 3;
        bottomSprite.visible = true;
        
        Physics bottomPhysics;
        bottomPhysics.velocity.x = -m_worldSpeed;
        bottomPhysics.useGravity = false;
        
        Hitbox bottomCollider;
        bottomCollider.type = ColliderType::Rectangle;
        // Consistent hitbox configuration with other toilet types
        bottomCollider.width = 20.0f;
        bottomCollider.height = 226.0f; // Fixed height: from 30px down to bottom (256px)
        bottomCollider.offsetX = 0.0f;
        bottomCollider.offsetY = 55.0f; // Start 55px from the top (scaled to 440px at scale 8)
        bottomCollider.isStatic = false;
        bottomCollider.isTrigger = false;
        bottomCollider.tag = "obstacle";
        
        Obstacle bottomObstacle;
        bottomObstacle.type = ObstacleType::BottomToilet;  // NEW: Use enum
        bottomObstacle.damage = 1;
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
        

        
        m_ecsSystem->AddComponent<Transform>(bottomToilet, bottomTransform);
        m_ecsSystem->AddComponent<Sprite>(bottomToilet, bottomSprite);
        m_ecsSystem->AddComponent<Physics>(bottomToilet, bottomPhysics);
        m_ecsSystem->AddComponent<Hitbox>(bottomToilet, bottomCollider);
        m_ecsSystem->AddComponent<Obstacle>(bottomToilet, bottomObstacle);
        
        // Debug overlays for bottom toilet (uses Hitbox for dimensions)
        if (m_debugMode) {
            DebugDraw bottomDebug(false, false, Gnosis::GNColor(0, 255, 0, 255), Gnosis::GNColor(255, 0, 0, 255));
            m_ecsSystem->AddComponent<DebugDraw>(bottomToilet, bottomDebug);
        }
        
        // Add Group components for coin calculation (needed by CalculateCoinPositionsForGroup)
        Group topGroup;
        topGroup.id = groupId;
        topGroup.isLeader = true;
        topGroup.offsetX = 0.0f;
        topGroup.offsetY = 0.0f;
        topGroup.groupWidth = 64.0f * m_baseScale;
        topGroup.pattern = GroupPattern::TopAndBottom;
        
        Group bottomGroup;
        bottomGroup.id = groupId;
        bottomGroup.isLeader = false;
        bottomGroup.offsetX = 0.0f;
        bottomGroup.offsetY = fixedGapHeight + toiletHeight;
        bottomGroup.groupWidth = 64.0f * m_baseScale;
        bottomGroup.pattern = GroupPattern::TopAndBottom;
        
        m_ecsSystem->AddComponent<Group>(topToilet, topGroup);
        m_ecsSystem->AddComponent<Group>(bottomToilet, bottomGroup);
        
        // Add to legacy tracking (still needed by ObstacleSystem::Update for now)
        m_activeObstacles.push_back(topToilet);
        m_activeObstacles.push_back(bottomToilet);
        
        // Verbose debug logging like old LevelManager
        GN_LOG_DEBUG("Toilet positioning: screenHeight=" + std::to_string(screenHeight) + 
                    ", toiletHeight=" + std::to_string(toiletHeight) + 
                    ", minTopY=" + std::to_string(minTopY) + 
                    ", maxTopY=" + std::to_string(maxTopY) + 
                    ", topY=" + std::to_string(topToiletY) + 
                    ", bottomY=" + std::to_string(bottomToiletY) + 
                    ", gap=" + std::to_string(fixedGapHeight) + 
                    " (improved positioning: lower top, larger gaps)");
        
        // Verbose debug: exact sprite and hitbox world metrics for TOP and BOTTOM
        {
            const float scale = m_baseScale;
            const float spriteWidth = 65.0f * scale;
            const float spriteHeight = 190.0f * scale;
            const float centerXTop = x + spriteWidth * 0.5f;
            const float centerYTop = topToiletY + spriteHeight * 0.5f;
            const float topHbW = topCollider.width * scale;
            const float topHbH = topCollider.height * scale;
            const float topHbL = centerXTop + (topCollider.offsetX * scale) - (topHbW * 0.5f);
            const float topHbT = centerYTop + (topCollider.offsetY * scale) - (topHbH * 0.5f);
            const float topHbR = topHbL + topHbW;
            const float topHbB = topHbT + topHbH;

            GN_LOG_DEBUG(std::string("PARK TOP spriteTL=(") + std::to_string(x) + "," + std::to_string(topToiletY) + ") size=(" +
                          std::to_string(65.0f) + "x" + std::to_string(190.0f) + ") scale=" + std::to_string(scale) +
                          " worldSize=(" + std::to_string(spriteWidth) + "x" + std::to_string(spriteHeight) + ")");
            GN_LOG_DEBUG(std::string("PARK TOP hitbox LRTB=") +
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

            GN_LOG_DEBUG(std::string("PARK BOTTOM spriteTL=(") + std::to_string(x) + "," + std::to_string(bottomToiletY) + ") size=(" +
                          std::to_string(65.0f) + "x" + std::to_string(190.0f) + ") scale=" + std::to_string(scale) +
                          " worldSize=(" + std::to_string(spriteWidth) + "x" + std::to_string(spriteHeight) + ")");
            GN_LOG_DEBUG(std::string("PARK BOTTOM hitbox LRTB=") +
                          "L=" + std::to_string(botHbL) +
                          " R=" + std::to_string(botHbR) +
                          " T=" + std::to_string(botHbT) +
                          " B=" + std::to_string(botHbB) +
                          " size=(" + std::to_string(botHbW) + "x" + std::to_string(botHbH) + ")");

            // Gap diagnostics
            const float topSpriteBottom = topToiletY + spriteHeight;
            const float spriteGap = bottomToiletY - topSpriteBottom; // unscaled world px
            const float topColliderBottom = (topToiletY + spriteHeight) - (30.0f * scale);
            const float bottomColliderTop = bottomToiletY + (30.0f * scale);
            const float colliderGap = bottomColliderTop - topColliderBottom; // world px

            GN_LOG_DEBUG(std::string("PARK GAP check: spriteGap=") + std::to_string(spriteGap) +
                          " (expected=" + std::to_string(fixedGapHeight) + ") spriteGapScaled=" + std::to_string(spriteGap * 1.0f) +
                          " colliderGap=" + std::to_string(colliderGap) +
                          " (expectedColliderGapScaled=" + std::to_string(fixedGapHeight + 60.0f * scale) + ")");
        }
        
        GN_LOG_DEBUG("Spawned Park toilet pair at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        // Return entities for orchestrator to add to GroupManifest
        return std::make_pair(topToilet, bottomToilet);
    }


    std::vector<Gnosis::Entity> ObstacleSystem::SpawnDesertPattern_Outhouse(float x, int groupId) {
        std::vector<Gnosis::Entity> entities;
        
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float outhouseW = 90.0f;
        const float outhouseH = 160.0f;
        const float groundY = screenInfo.pixelHeight - (outhouseH * m_baseScale);
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        // Create outhouse entity (solid collision) - always at ground level
        Gnosis::Entity outhouse = CreateOuthouseEntity("Outhouse", x, groundY, m_baseScale, true);
        
        // Create toilet entity (trigger only) - varies within 1/4 screen range from bottom
        // Random Y within range: groundY (bottom/max down) to groundY - (screenHeight * 0.25) (top/more exposed)
        float toiletYVariance = screenInfo.pixelHeight * 0.25f; // 1/4 screen range
        float toiletY = groundY - (static_cast<float>(rand() % static_cast<int>(toiletYVariance)));
        Gnosis::Entity toilet = CreateOuthouseEntity("OuthouseToilet", x, toiletY, m_baseScale, false);
        
        // Add Group components for coin calculation
        Group outhouseGroup;
        outhouseGroup.id = groupId;
        outhouseGroup.isLeader = true;
        outhouseGroup.offsetX = 0.0f;
        outhouseGroup.offsetY = groundY;
        outhouseGroup.groupWidth = outhouseW * m_baseScale;
        outhouseGroup.pattern = GroupPattern::Ground;
        m_ecsSystem->AddComponent<Group>(outhouse, outhouseGroup);
        
        Group toiletGroup;
        toiletGroup.id = groupId;
        toiletGroup.isLeader = false;
        toiletGroup.offsetX = 0.0f;
        toiletGroup.offsetY = toiletY;
        toiletGroup.groupWidth = outhouseW * m_baseScale;
        toiletGroup.pattern = GroupPattern::Ground;
        m_ecsSystem->AddComponent<Group>(toilet, toiletGroup);
        
        // Create brick wall between outhouse and toilet for duck-under challenge
        // Position it in the middle of the gap, extending from top of screen
        const float brickWallW = 18.0f; // Brick wall hitbox width - narrow for precise collision
        const float brickWallH = 128.0f; // Brick wall height - extends from top
        
        // Calculate the center position in the gap FOLLOWING this outhouse
        // The outhouse is at x, and we want the brick wall centered in the gap that follows
        // Gap width is managed by LevelManager manifest (928px for desert)
        // We'll use a local reasonable estimate that matches typical desert gaps
        const float outhouseRightEdge = x + (outhouseW * m_baseScale);
        const float estimatedGapWidth = 928.0f; // Matches LevelManager desert gapWidth
        const float brickWallSpriteWidth = 64.0f; // Visual sprite width
        // Center the brick wall in the gap following this outhouse
        const float brickWallX = outhouseRightEdge + (estimatedGapWidth * 0.5f) - (brickWallSpriteWidth * m_baseScale * 0.5f);
        const float brickWallY = 0.0f; // Start from top of screen
        
        Gnosis::Entity brickWall = m_ecsSystem->CreateEntity();
        if (brickWall != 0) {
            Transform brickTransform(Gnosis::GNVector2(brickWallX, brickWallY), 0.0f, 
                                   Gnosis::GNVector2(m_baseScale, m_baseScale));
            m_ecsSystem->AddComponent<Transform>(brickWall, brickTransform);
            
            Sprite brickSprite("BrickWall", 64.0f, brickWallH); // Visual sprite width 64, height from hitbox
            brickSprite.layer = 3;
            brickSprite.visible = true;
            m_ecsSystem->AddComponent<Sprite>(brickWall, brickSprite);
            
            Physics brickPhysics;
            brickPhysics.velocity.x = -m_worldSpeed;
            brickPhysics.useGravity = false;
            m_ecsSystem->AddComponent<Physics>(brickWall, brickPhysics);
            
            Hitbox brickCollider;
            brickCollider.type = ColliderType::Rectangle;
            brickCollider.width = brickWallW;
            brickCollider.height = brickWallH;
            // Position the 18-pixel hitbox on the actual brick wall texture
            // Need negative offset to move hitbox left onto the texture
            brickCollider.offsetX = -18.0f; // Move hitbox left to align with texture
            brickCollider.offsetY = 0.0f;
            brickCollider.isStatic = false;
            brickCollider.isTrigger = false;
            brickCollider.tag = "obstacle";
            m_ecsSystem->AddComponent<Hitbox>(brickWall, brickCollider);
            
            Obstacle brickObstacle;
            brickObstacle.type = ObstacleType::BrickWall;  // NEW: Use enum
            brickObstacle.damage = 1;
            brickObstacle.basePosition = Gnosis::GNVector2(brickWallX, brickWallY);
            m_ecsSystem->AddComponent<Obstacle>(brickWall, brickObstacle);
            
            // Add Group component so brick wall is part of the group and wraps with outhouse
            Group brickGroup;
            brickGroup.id = groupId;
            brickGroup.isLeader = false;
            brickGroup.offsetX = brickWallX - x;  // Offset from outhouse position
            brickGroup.offsetY = brickWallY;
            brickGroup.groupWidth = brickWallSpriteWidth * m_baseScale;
            brickGroup.pattern = GroupPattern::Ground;
            m_ecsSystem->AddComponent<Group>(brickWall, brickGroup);
            
            // DebugDraw DISABLED for brick walls per user request
            // if (m_debugMode) {
            //     DebugDraw debugDraw;
            //     debugDraw.showBounds = false;
            //     debugDraw.showCollider = true;
            //     debugDraw.colliderColor = Gnosis::GNColor(255, 0, 255, 255); // Magenta for brick wall
            //     debugDraw.alpha = 0.5f;
            //     m_ecsSystem->AddComponent<DebugDraw>(brickWall, debugDraw);
            // }
            
            // NOTE: Group management now handled by LevelManager orchestrator
            // Add brick wall to active tracking
            m_activeObstacles.push_back(brickWall);
            
            // Add brick wall to entities vector so it's included in the group manifest
            entities.push_back(brickWall);
            
            GN_LOG_DEBUG("Spawned brick wall at x=" + std::to_string(brickWallX) + ", y=" + std::to_string(brickWallY));
        }
        
        // Link entities
        LinkOuthousePair(outhouse, toilet);
        
        // Add to tracking
        m_activeObstacles.push_back(outhouse);
        m_activeObstacles.push_back(toilet);
        
        // Add outhouse and toilet to entities vector (brick wall already added above)
        entities.push_back(outhouse);
        entities.push_back(toilet);
        
        GN_LOG_DEBUG("Spawned desert outhouse at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        GN_LOG_DEBUG("Outhouse hitbox: solid collision, width=" + std::to_string((90.0f - 16.0f) * m_baseScale) + 
                     ", height=" + std::to_string((160.0f - 64.0f) * m_baseScale) + 
                     ", offsetX=0, offsetY=" + std::to_string(64.0f * m_baseScale));
        GN_LOG_DEBUG("Toilet hitbox: trigger only, width=" + std::to_string(((90.0f / 2) - 8.0f) * m_baseScale) + 
                     ", height=" + std::to_string(160.0f * m_baseScale) + 
                     ", offsetX=" + std::to_string((90.0f / 4 + 4.0f) * m_baseScale) + ", offsetY=0");
        
        // Return all entities (outhouse, toilet, and brick wall) for manifest tracking
        return entities;
    }

    Gnosis::Entity ObstacleSystem::SpawnDesertPattern_Cactus(float x, int groupId) {
        const std::vector<std::string> cactiTypes = {"CactiA", "CactiB", "CactiC"};
        const std::string cactusType = cactiTypes[rand() % cactiTypes.size()];
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float cactusW = 64.0f;
        const float cactusH = 48.0f;
        const float groundY = screenInfo.pixelHeight - (cactusH * m_baseScale);
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        Gnosis::Entity cactus = CreateCactusEntity(cactusType, x, groundY, m_baseScale, false);
        
        // NOTE: Group management now handled by LevelManager orchestrator
        // Add to tracking
        m_activeObstacles.push_back(cactus);
        
        GN_LOG_DEBUG("Spawned desert cactus (" + cactusType + ") at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        return cactus;  // Return entity for manifest tracking
    }

    std::vector<Gnosis::Entity> ObstacleSystem::SpawnSewerPattern_TopOnly(float x, int groupId) {
        // Match old LevelManager's exact TopOnly logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float topY = 0.0f;
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        const std::string texture = (rand() % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
        Gnosis::Entity pipe = CreateSewerPipeEntity(texture, x, topY, m_baseScale, true);
        
        // Add Group component for coin calculation
        // CRITICAL: groupWidth must be the actual span width (not just pipe width)
        // For single pipe patterns, width is just the pipe width
        Group pipeGroup;
        pipeGroup.id = groupId;
        pipeGroup.isLeader = true;
        pipeGroup.offsetX = 0.0f;
        pipeGroup.offsetY = 0.0f;
        pipeGroup.groupWidth = pipeW * m_baseScale;  // Single pipe = pipe width
        pipeGroup.pattern = GroupPattern::TopOnly;  // SEWER: TopOnly pattern for coins
        m_ecsSystem->AddComponent<Group>(pipe, pipeGroup);
        
        m_activeObstacles.push_back(pipe);
        
        GN_LOG_DEBUG("Spawned sewer top-only pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        return {pipe};  // Return entity for manifest tracking
    }

    std::vector<Gnosis::Entity> ObstacleSystem::SpawnSewerPattern_BottomOnly(float x, int groupId) {
        // Match old LevelManager's exact BottomOnly logic
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float bottomY = screenInfo.pixelHeight - (pipeH * m_baseScale);
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        const std::string texture = (rand() % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
        Gnosis::Entity pipe = CreateSewerPipeEntity(texture, x, bottomY, m_baseScale, false);
        
        // Add Group component for coin calculation
        // CRITICAL: groupWidth must be the actual span width (not just pipe width)
        // For single pipe patterns, width is just the pipe width
        Group pipeGroup;
        pipeGroup.id = groupId;
        pipeGroup.isLeader = true;
        pipeGroup.offsetX = 0.0f;
        pipeGroup.offsetY = bottomY;
        pipeGroup.groupWidth = pipeW * m_baseScale;  // Single pipe = pipe width
        pipeGroup.pattern = GroupPattern::BottomOnly;  // SEWER: BottomOnly pattern for coins
        m_ecsSystem->AddComponent<Group>(pipe, pipeGroup);
        
        m_activeObstacles.push_back(pipe);
        
        GN_LOG_DEBUG("Spawned sewer bottom-only pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        return {pipe};  // Return entity for manifest tracking
    }

    std::vector<Gnosis::Entity> ObstacleSystem::SpawnSewerPattern_TopAndBottom(float x, int groupId) {
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float topY = 0.0f;
        const float bottomY = screenInfo.pixelHeight - (pipeH * m_baseScale);
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        // Create top pipe
        const std::string topTexture = (rand() % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
        Gnosis::Entity topPipe = CreateSewerPipeEntity(topTexture, x, topY, m_baseScale, true);
        
        // Create bottom pipe
        const std::string bottomTexture = (rand() % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
        Gnosis::Entity bottomPipe = CreateSewerPipeEntity(bottomTexture, x, bottomY, m_baseScale, false);
        
        // Add Group components for coin calculation
        // CRITICAL: groupWidth must be the actual span width (not just pipe width)
        // For TopAndBottom, width is just the pipe width (pipes are vertically stacked, same X)
        Group topGroup;
        topGroup.id = groupId;
        topGroup.isLeader = true;
        topGroup.offsetX = 0.0f;
        topGroup.offsetY = topY;
        topGroup.groupWidth = pipeW * m_baseScale;  // Single pipe width (pipes stacked vertically)
        topGroup.pattern = GroupPattern::TopAndBottom;  // SEWER: TopAndBottom pattern for coins
        m_ecsSystem->AddComponent<Group>(topPipe, topGroup);
        
        Group bottomGroup;
        bottomGroup.id = groupId;
        bottomGroup.isLeader = false;
        bottomGroup.offsetX = 0.0f;
        bottomGroup.offsetY = bottomY;
        bottomGroup.groupWidth = pipeW * m_baseScale;  // Single pipe width (pipes stacked vertically)
        bottomGroup.pattern = GroupPattern::TopAndBottom;  // SEWER: TopAndBottom pattern for coins
        m_ecsSystem->AddComponent<Group>(bottomPipe, bottomGroup);
        
        m_activeObstacles.push_back(topPipe);
        m_activeObstacles.push_back(bottomPipe);
        
        GN_LOG_DEBUG("Spawned sewer top-and-bottom pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        return {topPipe, bottomPipe};  // Return entities for manifest tracking
    }

    std::vector<Gnosis::Entity> ObstacleSystem::SpawnSewerPattern_Pyramid3(float x, int groupId) {
        // Match old LevelManager's exact pyramid logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float spacingX = pipeW * m_baseScale * 1.15f;
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float spacingY = pipeH * m_baseScale * 0.6f;
        const float row1Lift = 12.0f * m_baseScale;
        const float row2Lift = 24.0f * m_baseScale;
        const float baseY = screenInfo.pixelHeight - (pipeH * m_baseScale);
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        std::vector<Gnosis::Entity> pipes;
        
        auto spawn = [&](float col, int row, int idx) {
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1Lift : (row == 2 ? row2Lift : 0.0f));
            float pipeY = (baseY - row * spacingY) - lift;
            // REVERSED: Bottom row (row 0) = layer 5 (front), ascending rows go backwards (4, 3)
            // This simulates pipes coming out of each other
            int layer = 5 - row;
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, false, layer);
            // NOTE: Group management now handled by LevelManager orchestrator
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        };
        
        int idx = 0;
        // Bottom row (3 pipes)
        for (int c = 0; c < 3; ++c) spawn(static_cast<float>(c), 0, idx++);
        // Middle row (2 pipes centered)
        for (int c = 0; c < 2; ++c) spawn(c + 0.5f, 1, idx++);
        // Top row (1 pipe centered)
        spawn(1.0f, 2, idx++);
        
        // Complement: TOP row across the span (funnel) - keep on layer 5 (front) - correct as-is
        const float topY = 0.0f;
        for (int c = 0; c < 3; ++c) {
            const std::string tex = (c % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float pipeX = x + c * spacingX;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, topY, m_baseScale, true, 5);
            // NOTE: Group management now handled by LevelManager orchestrator
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        }
        
        // Add Group components for coin calculation - pyramid pattern (use PyramidBottom, not Pyramid3)
        // CRITICAL: Calculate proper offsets relative to leader (first pipe at x)
        // Group width should be the actual span: (3 pipes - 1) * spacingX + pipeW = 2 * spacingX + pipeW
        const float actualGroupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);  // Matches old code!
        for (size_t i = 0; i < pipes.size(); ++i) {
            Transform* t = m_ecsSystem->GetComponent<Transform>(pipes[i]);
            Group pipeGroup;
            pipeGroup.id = groupId;
            pipeGroup.isLeader = (i == 0);
            pipeGroup.offsetX = t ? (t->position.x - x) : 0.0f;  // Offset from leader position
            pipeGroup.offsetY = 0.0f;
            pipeGroup.groupWidth = actualGroupWidth;  // Use calculated width matching old code
            pipeGroup.pattern = GroupPattern::PyramidBottom;  // SEWER: Bottom pyramid pattern for coins
            m_ecsSystem->AddComponent<Group>(pipes[i], pipeGroup);
        }
        
        GN_LOG_DEBUG("Spawned sewer pyramid-3 pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        return pipes;  // Return all entities for manifest tracking
    }

    std::vector<Gnosis::Entity> ObstacleSystem::SpawnSewerPattern_TwoByTwoFunnel(float x, int groupId) {
        // Match old LevelManager's exact TwoByTwoFunnel logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float spacingX = pipeW * m_baseScale * 1.15f;
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float spacingY = pipeH * m_baseScale * 0.6f;
        const float row1LiftTop = 12.0f * m_baseScale;
        const float row1Lift = 12.0f * m_baseScale;
        const float bottomY = screenInfo.pixelHeight - (pipeH * m_baseScale);
        const float topY = 0.0f;
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        std::vector<Gnosis::Entity> pipes;
        
        // Helper: bottom rows
        auto spawnBottom = [&](float col, int row, int idx) {
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1Lift : 0.0f);
            float pipeY = bottomY - row * spacingY - lift;
            // REVERSED: Bottom row (row 0) = layer 5 (front), row 1 = layer 4
            int layer = 5 - row;
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, false, layer);
            // NOTE: Group management now handled by LevelManager orchestrator
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        };
        
        // Helper: top rows
        auto spawnTop = [&](float col, int row) {
            const std::string tex = (static_cast<int>(col + row) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1LiftTop : 0.0f);
            float pipeY = topY + row * spacingY + lift;
            // Layer depth for top pipes: row 0 = layer 5, row 1 = layer 4
            int layer = (row == 0) ? 5 : 4;
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, true, layer);
            // NOTE: Group management now handled by LevelManager orchestrator
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        };
        
        int idx = 0;
        // Bottom two rows: 3 then 2
        for (int c = 0; c < 3; ++c) spawnBottom(static_cast<float>(c), 0, idx++);
        for (int c = 0; c < 2; ++c) spawnBottom(c + 0.5f, 1, idx++);
        // Top two rows: 2-high inverted: row 1 (two) then row 0 (three) last for layering
        for (int c = 0; c < 2; ++c) spawnTop(c + 0.5f, 1);
        for (int c = 0; c < 3; ++c) spawnTop(static_cast<float>(c), 0);
        
        // Add Group components for coin calculation - two-by-two-funnel pattern
        // CRITICAL: Calculate proper offsets relative to leader (first pipe at x)
        // Group width should be the actual span: (3 pipes - 1) * spacingX + pipeW = 2 * spacingX + pipeW
        const float actualGroupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);  // Matches old code!
        for (size_t i = 0; i < pipes.size(); ++i) {
            Transform* t = m_ecsSystem->GetComponent<Transform>(pipes[i]);
            Group pipeGroup;
            pipeGroup.id = groupId;
            pipeGroup.isLeader = (i == 0);
            pipeGroup.offsetX = t ? (t->position.x - x) : 0.0f;  // Offset from leader position
            pipeGroup.offsetY = 0.0f;
            pipeGroup.groupWidth = actualGroupWidth;  // Use calculated width matching old code
            pipeGroup.pattern = GroupPattern::TwoFunnel;  // SEWER: TwoFunnel pattern for coins
            m_ecsSystem->AddComponent<Group>(pipes[i], pipeGroup);
        }
        
        GN_LOG_DEBUG("Spawned sewer two-by-two-funnel pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        return pipes;  // Return all entities for manifest tracking
    }

    std::vector<Gnosis::Entity> ObstacleSystem::SpawnSewerPattern_PyramidTop3(float x, int groupId) {
        // Match old LevelManager's exact PyramidTop3 logic
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float spacingX = pipeW * m_baseScale * 1.15f;
        const float spacingY = pipeH * m_baseScale * 0.6f;
        const float row1LiftTop = 12.0f * m_baseScale;
        const float row2LiftTop = 24.0f * m_baseScale;
        const float topY = 0.0f;
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        std::vector<Gnosis::Entity> pipes;
        
        auto spawnTop = [&](float col, int row, int idx) {
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1LiftTop : (row == 2 ? row2LiftTop : 0.0f));
            float pipeY = topY + row * spacingY + lift;
            // Layer depth: row 0 (front) = layer 5, row 1 = layer 4, row 2 (back) = layer 3
            int layer = 5 - row;
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, true, layer);
            // NOTE: Group management now handled by LevelManager orchestrator
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        };
        
        int idx = 0;
        // Construct bottom-most of the inverted stack first so the TOP row renders last
        // row 2: 1 centered under
        spawnTop(1.0f, 2, idx++);
        // row 1: 2 centered under
        for (int c = 0; c < 2; ++c) spawnTop(c + 0.5f, 1, idx++);
        // top row: 3 (rendered last for correct layering)
        for (int c = 0; c < 3; ++c) spawnTop(static_cast<float>(c), 0, idx++);
        
        // Complement: BOTTOM row across the span (funnel) - keep on layer 5 (front) - correct as-is
        const float bottomY = screenInfo.pixelHeight - (pipeH * m_baseScale);
        for (int c = 0; c < 3; ++c) {
            const std::string tex = (c % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float pipeX = x + c * spacingX;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, bottomY, m_baseScale, false, 5);
            // NOTE: Group management now handled by LevelManager orchestrator
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        }
        
        // Add Group components for coin calculation - pyramid top pattern (use PyramidTop, not PyramidTop3)
        // CRITICAL: Calculate proper offsets relative to leader (first pipe at x)
        // Group width should be the actual span: (3 pipes - 1) * spacingX + pipeW = 2 * spacingX + pipeW
        const float actualGroupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);  // Matches old code!
        for (size_t i = 0; i < pipes.size(); ++i) {
            Transform* t = m_ecsSystem->GetComponent<Transform>(pipes[i]);
            Group pipeGroup;
            pipeGroup.id = groupId;
            pipeGroup.isLeader = (i == 0);
            pipeGroup.offsetX = t ? (t->position.x - x) : 0.0f;  // Offset from leader position
            pipeGroup.offsetY = 0.0f;
            pipeGroup.groupWidth = actualGroupWidth;  // Use calculated width matching old code
            pipeGroup.pattern = GroupPattern::PyramidTop;  // SEWER: Top pyramid pattern for coins
            m_ecsSystem->AddComponent<Group>(pipes[i], pipeGroup);
        }
        
        GN_LOG_DEBUG("Spawned sewer pyramid-top-3 pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        
        return pipes;  // Return all entities for manifest tracking
    }

    // ============================================================================
    // Entity Creation Helpers 
    // ============================================================================

    void ObstacleSystem::LinkToiletPair(Gnosis::Entity top, Gnosis::Entity bottom) {
        Obstacle* topObstacle = m_ecsSystem->GetComponent<Obstacle>(top);
        Obstacle* bottomObstacle = m_ecsSystem->GetComponent<Obstacle>(bottom);
        
        if (topObstacle && bottomObstacle) {
            topObstacle->pairedEntity = bottom;
            bottomObstacle->pairedEntity = top;
        }
    }

    void ObstacleSystem::LinkOuthousePair(Gnosis::Entity outhouse, Gnosis::Entity toilet) {
        Obstacle* outhouseObstacle = m_ecsSystem->GetComponent<Obstacle>(outhouse);
        Obstacle* toiletObstacle = m_ecsSystem->GetComponent<Obstacle>(toilet);
        
        if (outhouseObstacle && toiletObstacle) {
            outhouseObstacle->pairedEntity = toilet;
            toiletObstacle->pairedEntity = outhouse;
        }
    }

    Gnosis::Entity ObstacleSystem::CreateToiletEntity(const std::string& texture, float x, float y, float scale, bool isTop) {
        Gnosis::Entity entity = m_ecsSystem->CreateEntity();
        
        // Transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
        m_ecsSystem->AddComponent<Transform>(entity, transform);
        
        // Sprite
        Sprite sprite(texture, 65.0f, 190.0f);
        // Use layer 4 (mid obstacle layer) for regular toilets
        sprite.layer = 4;
        sprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(entity, sprite);
        
        // Physics
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        m_ecsSystem->AddComponent<Physics>(entity, physics);
        
        // Hitbox
        Hitbox hitbox;
        hitbox.type = ColliderType::Rectangle;
        hitbox.width = 20.0f;
        hitbox.height = 226.0f; // Fixed height: consistent with other toilet types
        hitbox.offsetX = 0.0f;
        hitbox.offsetY = isTop ? (25.0f - 8.0f * scale) : 55.0f; // Top toilet hitbox moved up by 8px*scale, bottom=55px
        hitbox.isStatic = false;
        hitbox.isTrigger = false;
        hitbox.tag = "obstacle";
        m_ecsSystem->AddComponent<Hitbox>(entity, hitbox);
        
        // Obstacle
        Obstacle obstacle;
        obstacle.type = isTop ? ObstacleType::TopToilet : ObstacleType::BottomToilet;  // Snow uses same toilet types as Park
        obstacle.damage = 1;
        obstacle.basePosition = Gnosis::GNVector2(x, y);
        obstacle.isTopPart = isTop;
        m_ecsSystem->AddComponent<Obstacle>(entity, obstacle);
        
        return entity;
    }

    Gnosis::Entity ObstacleSystem::CreateOuthouseEntity(const std::string& texture, float x, float y, float scale, bool isSolid) {
        Gnosis::Entity entity = m_ecsSystem->CreateEntity();
        
        // Transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
        m_ecsSystem->AddComponent<Transform>(entity, transform);
        
        // Sprite
        Sprite sprite(texture, 90.0f, 160.0f);
        // Obstacle layering: 3=back, 4=mid, 5=front
        // Outhouse structure on layer 4 (mid), toilet on layer 3 (back) so toilet renders behind
        sprite.layer = (texture == "Outhouse") ? 4 : 3;
        sprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(entity, sprite);
        
        // Physics
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        m_ecsSystem->AddComponent<Physics>(entity, physics);
        
        // Hitbox
        Hitbox hitbox;
        hitbox.type = ColliderType::Rectangle;
        if (isSolid) {
            // Outhouse - solid collision (texture is 64x160, not 90x160)
            hitbox.width = (64.0f - 16.0f);  // 48 pixels wide (64 - 16 for margins)
            hitbox.height = (160.0f - 32.0f);  // 128 pixels tall (160 - 32 for margins)
            hitbox.offsetX = -12.0f;  // Moved 4px to the right from previous position (was -16, now -12)
            hitbox.offsetY = 48.0f;  // Moved down another 32px (was 16, now 48)
            hitbox.isTrigger = false;
        } else {
            // Toilet - trigger only, properly centered with the toilet opening
            hitbox.width = 20.0f;  // Fixed 20px width for the split
            hitbox.height = 160.0f;  // Full height of texture
            hitbox.offsetX = -12.0f;  // Moved 4px to the left from previous position (was -8, now -12)
            hitbox.offsetY = 40.0f;  // Lowered by 40px as requested
            hitbox.isTrigger = true;
        }
        hitbox.isStatic = false;
        hitbox.tag = "obstacle";
        m_ecsSystem->AddComponent<Hitbox>(entity, hitbox);
        
        // Obstacle
        Obstacle obstacle;
        obstacle.type = ObstacleType::Outhouse;  // Desert level - both Outhouse and OuthouseToilet use same type
        obstacle.damage = 1;
        obstacle.basePosition = Gnosis::GNVector2(x, y);
        obstacle.isTopPart = (texture == "Outhouse");
        m_ecsSystem->AddComponent<Obstacle>(entity, obstacle);
        
                        // DebugDraw - show hitboxes visually (disabled for desert and castle levels)
                if (m_debugMode && m_currentLevelId != 3 && m_currentLevelId != 5) { // Disable debug drawing for desert and castle levels
                    DebugDraw debugDraw;
                    debugDraw.showBounds = false;  // Don't show sprite bounds
                    debugDraw.showCollider = true; // Show hitbox colliders
                    if (isSolid) {
                        debugDraw.colliderColor = Gnosis::GNColor(255, 0, 0, 128);  // Red for solid obstacles
                    } else {
                        debugDraw.colliderColor = Gnosis::GNColor(0, 255, 255, 128); // Cyan for trigger obstacles
                    }
                    debugDraw.alpha = 0.6f;
                    debugDraw.debugLayer = 20; // High priority layer
                    m_ecsSystem->AddComponent<DebugDraw>(entity, debugDraw);
                }
        
        return entity;
    }

    Gnosis::Entity ObstacleSystem::CreateSewerPipeEntity(const std::string& texture, float x, float y, float scale, bool isTop, int layer) {
        Gnosis::Entity entity = m_ecsSystem->CreateEntity();
        
        // Transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
        m_ecsSystem->AddComponent<Transform>(entity, transform);
        
        // Sprite
        Sprite sprite(texture, 192.0f, 64.0f);
        // Use the layer parameter (3=back, 4=mid, 5=front) for proper depth in stacked pipes
        sprite.layer = layer;
        sprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(entity, sprite);
        
        // Physics
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        m_ecsSystem->AddComponent<Physics>(entity, physics);
        
        // Hitbox
        Hitbox hitbox;
        hitbox.type = ColliderType::Rectangle;
        hitbox.width = 192.0f - 12.0f; // 6px inset
        hitbox.height = 64.0f - 12.0f;
        hitbox.offsetX = 0.0f;
        hitbox.offsetY = 0.0f;
        hitbox.isStatic = false;
        hitbox.isTrigger = false;
        hitbox.tag = "obstacle";
        m_ecsSystem->AddComponent<Hitbox>(entity, hitbox);
        
        // Obstacle
        Obstacle obstacle;
        obstacle.type = ObstacleType::Pipe;  // Sewer level - all pipes use the same type
        obstacle.damage = 1;
        obstacle.basePosition = Gnosis::GNVector2(x, y);
        obstacle.isTopPart = isTop;
        m_ecsSystem->AddComponent<Obstacle>(entity, obstacle);
        
        return entity;
    }

    Gnosis::Entity ObstacleSystem::CreateCactusEntity(const std::string& texture, float x, float y, float scale, bool isDancing, float width, float height) {
        Gnosis::Entity entity = m_ecsSystem->CreateEntity();
        
        // Use the dimensions passed from the cactus type
        // No need to hardcode dimensions anymore
        
        // Transform - use the y parameter that was calculated correctly in SpawnCactusPool
        // Use the proper scale (8.0f) for pixel art visibility on screen
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
        m_ecsSystem->AddComponent<Transform>(entity, transform);
        
        GN_LOG_DEBUG("Cactus Transform: x=" + std::to_string(x) + ", y=" + std::to_string(y) + ", scale=" + std::to_string(scale));
        
        // Sprite
        Sprite sprite(texture, width, height);
        sprite.layer = 5; // Higher layer than outhouses for forward positioning
        sprite.visible = true;
        
        // Set up animation for dancing cacti
        if (isDancing) {
            sprite.isAnimated = true;
            sprite.frameCount = 8; // 8-frame animation
            sprite.frameWidth = static_cast<int>(width); // Each frame is the full width (64 pixels)
            sprite.frameHeight = static_cast<int>(height); // Height stays the same
            sprite.currentFrame = 0;
            sprite.currentFrameTime = 0.0f; // Initialize frame time
            sprite.frameTime = 0.4f; // Slower animation to match music better (0.4s per frame)
            sprite.playing = true;
            sprite.loop = true;
            
            GN_LOG_DEBUG("Created dancing cactus: " + texture + " with " + std::to_string(sprite.frameCount) + " frames, frameWidth=" + std::to_string(sprite.frameWidth) + ", frameHeight=" + std::to_string(sprite.frameHeight) + ", total width=" + std::to_string(width) + ", height=" + std::to_string(height));
        }
        
        m_ecsSystem->AddComponent<Sprite>(entity, sprite);
        
        // Physics
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        m_ecsSystem->AddComponent<Physics>(entity, physics);
        
        // Hitbox - smaller than visual for better gameplay
        Hitbox hitbox;
        hitbox.type = ColliderType::Rectangle;
        hitbox.width = width * 0.6f; // 60% of visual width
        hitbox.height = height * 0.7f; // 70% of visual height
        hitbox.offsetX = (width - hitbox.width) * 0.5f; // Center hitbox
        hitbox.offsetY = (height - hitbox.height) * 0.5f; // Center hitbox
        hitbox.isStatic = false;
        hitbox.isTrigger = false;
        hitbox.tag = "obstacle";
        m_ecsSystem->AddComponent<Hitbox>(entity, hitbox);
        
        // Cacti are decorative and don't count as obstacles/pipes
        // No Obstacle component needed
        
        // DebugDraw DISABLED for cacti per user request
        // if (m_debugMode) {
        //     DebugDraw debugDraw;
        //     debugDraw.showBounds = false;
        //     debugDraw.showCollider = true;
        //     debugDraw.colliderColor = Gnosis::GNColor(0, 255, 0, 128); // Green for cacti
        //     debugDraw.alpha = 0.6f;
        //     debugDraw.debugLayer = 20;
        //     m_ecsSystem->AddComponent<DebugDraw>(entity, debugDraw);
        // }
        
        return entity;
    }

    // ============================================================================
    // REMOVED LEGACY GROUP MANAGEMENT METHODS (now in LevelManager)
    // ============================================================================
    // - AddEntityToGroup()
    // - WrapGroup()  
    // - CalculateGroupWidth()
    // - GetGroupEntities()
    // - GetAndClearWrappedGroups()
    // - ConsumeWrappedGroups()



    std::vector<Gnosis::GNVector2> ObstacleSystem::CalculateCoinPositionsForGroup(int groupId, GroupPattern pattern, float gapWidth) const {
        std::vector<Gnosis::GNVector2> positions;

        // CRITICAL DEBUG: Log which pattern we're processing
        std::string patternName = "UNKNOWN";
        switch(pattern) {
            case GroupPattern::TopOnly: patternName = "TopOnly"; break;
            case GroupPattern::BottomOnly: patternName = "BottomOnly"; break;
            case GroupPattern::TopAndBottom: patternName = "TopAndBottom"; break;
            case GroupPattern::Ground: patternName = "Ground"; break;
            case GroupPattern::PyramidBottom: patternName = "PyramidBottom"; break;
            case GroupPattern::PyramidTop: patternName = "PyramidTop"; break;
            case GroupPattern::TwoFunnel: patternName = "TwoFunnel"; break;
            case GroupPattern::Decorative: patternName = "Decorative"; break;
            case GroupPattern::SnowScreenEdges: patternName = "SnowScreenEdges"; break;
        }
        GN_LOG_INFO("🪙 CalculateCoinPositionsForGroup: groupId=" + std::to_string(groupId) + 
                    " pattern=" + patternName + " (" + std::to_string(static_cast<int>(pattern)) + ")" +
                    " gapWidth=" + std::to_string(gapWidth));
        
        // Check if level allows pickups using level config</parameter>
        if (!m_levelConfig || !m_levelConfig->enablePickups) {
            GN_LOG_DEBUG("Coins disabled for this level - enablePickups = false");
            return positions;
        }
        
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
            
            // Ensure minimum spacing between coins (widened for better spread without reducing coin count)
            const float preferredMinSpacing = 32.0f * m_baseScale; // Widened from 8.0f to 24.0f (192px at 8x scale)
            const float absoluteMinSpacing = 8.0f * m_baseScale;   // Fallback minimum to prevent overlap
            float spacing = (right - left) / static_cast<float>(coinsPerStripe);
            int actualCoinsToPlace = coinsPerStripe;

            if (spacing < preferredMinSpacing) {
                // Instead of reducing coin count, allow tighter spacing down to absolute minimum
                // This maintains coin count while still preventing severe overlap
                spacing = std::max(absoluteMinSpacing, spacing);
                actualCoinsToPlace = coinsPerStripe; // Keep all coins, just tighter spacing if needed
                // Performance: Disabled coin spawning debug logging
                // GN_LOG_DEBUG("ObstacleSystem::emitStripe using tighter spacing (" + std::to_string(spacing) + ") to maintain " + std::to_string(actualCoinsToPlace) + " coins");
            }
            
            for (int i = 0; i < actualCoinsToPlace; ++i) {
                float x = left + spacing * (i + 0.5f);
                positions.emplace_back(x, y);
            }
            // Performance: Disabled coin spawning debug logging
            // GN_LOG_DEBUG("ObstacleSystem::emitStripe y=" + std::to_string(y) + " left=" + std::to_string(left) + " right=" + std::to_string(right) + " actualCoins=" + std::to_string(actualCoinsToPlace));
        };

        auto emitStripeInRange = [&](float y, float rangeMin, float rangeMax) {
            float left = rangeMin + horizontalMargin;
            float right = rangeMax - horizontalMargin;
            if (!(left < right)) return;
            
            // Ensure minimum spacing between coins (widened for better spread without reducing coin count)
            const float preferredMinSpacing = 48.0f * m_baseScale; // Increased from 32.0f to 48.0f (384px at 8x scale)
            const float absoluteMinSpacing = 16.0f * m_baseScale;  // Increased from 8.0f to 16.0f (128px at 8x scale)
            float spacing = (right - left) / static_cast<float>(coinsPerStripe);
            int actualCoinsToPlace = coinsPerStripe;

            if (spacing < preferredMinSpacing) {
                // Instead of reducing coin count, allow tighter spacing down to absolute minimum
                // This maintains coin count while still preventing severe overlap
                spacing = std::max(absoluteMinSpacing, spacing);
                actualCoinsToPlace = coinsPerStripe; // Keep all coins, just tighter spacing if needed
                // Performance: Disabled coin spawning debug logging
                // GN_LOG_DEBUG("ObstacleSystem::emitStripeInRange using tighter spacing (" + std::to_string(spacing) + ") to maintain " + std::to_string(actualCoinsToPlace) + " coins");
            }
            
            // Performance: Disabled coin spawning debug logging
            // GN_LOG_DEBUG("ObstacleSystem::emitStripeInRange calculation: rangeMin=" + std::to_string(rangeMin) + ", rangeMax=" + std::to_string(rangeMax) + ", horizontalMargin=" + std::to_string(horizontalMargin) + ", left=" + std::to_string(left) + ", right=" + std::to_string(right) + ", spacing=" + std::to_string(spacing) + ", actualCoins=" + std::to_string(actualCoinsToPlace));
            for (int i = 0; i < actualCoinsToPlace; ++i) {
                float x = left + spacing * (i + 0.5f);
                positions.emplace_back(x, y);
                // Performance: Disabled per-coin debug logging
                // GN_LOG_DEBUG("ObstacleSystem::emitStripeInRange coin " + std::to_string(i) + " at x=" + std::to_string(x) + ", y=" + std::to_string(y));
            }
            // Performance: Disabled coin spawning debug logging
            // GN_LOG_DEBUG("ObstacleSystem::emitStripeInRange y=" + std::to_string(y) + " left=" + std::to_string(left) + " right=" + std::to_string(right));
        };
        
        // Special emit function for Ground pattern that doesn't add horizontal margin
        auto emitGroundStripe = [&](float y, float rangeMin, float rangeMax) {
            float left = rangeMin;  // No horizontal margin - already included in range calculation
            float right = rangeMax;
            if (!(left < right)) return;
            
            // Ensure minimum spacing between coins (widened for better spread without reducing coin count)
            const float preferredMinSpacing = 32.0f * m_baseScale; // Widened from 8.0f to 24.0f (192px at 8x scale)
            const float absoluteMinSpacing = 8.0f * m_baseScale;   // Fallback minimum to prevent overlap
            float spacing = (right - left) / static_cast<float>(coinsPerStripe);
            int actualCoinsToPlace = coinsPerStripe;

            if (spacing < preferredMinSpacing) {
                // Instead of reducing coin count, allow tighter spacing down to absolute minimum
                // This maintains coin count while still preventing severe overlap
                spacing = std::max(absoluteMinSpacing, spacing);
                actualCoinsToPlace = coinsPerStripe; // Keep all coins, just tighter spacing if needed
                // Performance: Disabled coin spawning debug logging
                // GN_LOG_DEBUG("ObstacleSystem::emitGroundStripe using tighter spacing (" + std::to_string(spacing) + ") to maintain " + std::to_string(actualCoinsToPlace) + " coins");
            }
            
            // Performance: Disabled coin spawning debug logging
            // GN_LOG_DEBUG("ObstacleSystem::emitGroundStripe calculation: rangeMin=" + std::to_string(rangeMin) + ", rangeMax=" + std::to_string(rangeMax) + ", left=" + std::to_string(left) + ", right=" + std::to_string(right) + ", spacing=" + std::to_string(spacing) + ", actualCoins=" + std::to_string(actualCoinsToPlace));
            for (int i = 0; i < actualCoinsToPlace; ++i) {
                float x = left + spacing * (i + 0.5f);
                positions.emplace_back(x, y);
                // Performance: Disabled per-coin debug logging
                // GN_LOG_DEBUG("ObstacleSystem::emitGroundStripe coin " + std::to_string(i) + " at x=" + std::to_string(x) + ", y=" + std::to_string(y));
            }
            // Performance: Disabled coin spawning debug logging
            // GN_LOG_DEBUG("ObstacleSystem::emitGroundStripe y=" + std::to_string(y) + " left=" + std::to_string(left) + " right=" + std::to_string(right));
        };

        switch (pattern) {
            case GroupPattern::TopOnly: {
                float y = (topBandBottomEdge != std::numeric_limits<float>::max())
                    ? (topBandBottomEdge + verticalPad)
                    : (bottomBandTopEdge > std::numeric_limits<float>::lowest() ? bottomBandTopEdge - verticalPad : 400.0f);
                if (topMinX < topMaxX) emitStripeInRange(y, topMinX, topMaxX); else emitStripe(y);
                // Performance: Disabled coin spawning debug logging
                // GN_LOG_DEBUG("ObstacleSystem::TopOnly stripe y=" + std::to_string(y) + " span=[" + std::to_string(topMinX) + "," + std::to_string(topMaxX) + "]");
                break;
            }
            case GroupPattern::BottomOnly: {
                float y = (bottomBandTopEdge != std::numeric_limits<float>::lowest())
                    ? (bottomBandTopEdge - verticalPad)
                    : (topBandBottomEdge < std::numeric_limits<float>::max() ? topBandBottomEdge + verticalPad : 400.0f);
                if (bottomMinX < bottomMaxX) emitStripeInRange(y, bottomMinX, bottomMaxX); else emitStripe(y);
                // Performance: Disabled coin spawning debug logging
                // GN_LOG_DEBUG("ObstacleSystem::BottomOnly stripe y=" + std::to_string(y) + " span=[" + std::to_string(bottomMinX) + "," + std::to_string(bottomMaxX) + "]");
                break;
            }
            case GroupPattern::TopAndBottom: {
                // Two stripes: one below top band and one above bottom band, each within its own band span
                // Used by: Park (Level 1), Sewer (Level 2), Snow (Level 4), Castle (Level 5)
                float topStripeY = 0.0f;
                float bottomStripeY = 0.0f;
                
                // Only enable verbose logging for Snow and Castle levels (wide gaps)
                bool verboseLogging = (m_currentLevelId == 4 || m_currentLevelId == 5);
                
                // CASTLE & SNOW LEVELS: Coins at screen edges, centered in the gap BEFORE toilet pairs
                if (m_currentLevelId == 5 || m_currentLevelId == 4) {
                    // Coins at top and bottom screen edges, spread evenly between toilet pairs
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    const float screenHeight = screenInfo.pixelHeight;
                    const float topScreenY = 200.0f;  // Near top of screen (with safe area padding)
                    const float bottomScreenY = screenHeight - 250.0f;  // Near bottom of screen
                    
                    // Use the ACTUAL gap width from the manifest (passed as parameter)
                    // Calculate the TRUE center of the gap between toilet pairs
                    // Current toilet LEFT edge is at minX
                    // Previous toilet RIGHT edge was at: minX - gapWidth
                    // Gap center is simply: minX - (gapWidth / 2)
                    float gapCenterX = minX - (gapWidth / 2.0f);
                    
                    // Spread coins across full gap width for Castle (need more spread)
                    float coinSpreadWidth = gapWidth * 0.95f;
                    float coinHalfWidth = coinSpreadWidth / 2.0f;
                    float coinMinX = gapCenterX - coinHalfWidth;
                    float coinMaxX = gapCenterX + coinHalfWidth;
                    
                    // Spawn coins at screen edges
                    emitStripeInRange(topScreenY, coinMinX, coinMaxX);       // Top screen edge
                    emitStripeInRange(bottomScreenY, coinMinX, coinMaxX);    // Bottom screen edge
                    
                    std::string levelName = (m_currentLevelId == 4) ? "Snow" : "Castle";
                    GN_LOG_INFO("ObstacleSystem::" + levelName + " [SCREEN EDGES] TOP row at Y=" + std::to_string(topScreenY) + 
                               ", BOTTOM row at Y=" + std::to_string(bottomScreenY) + 
                               ", gapCenterX=" + std::to_string(gapCenterX) + 
                               " (minX=" + std::to_string(minX) + ", gapWidth=" + std::to_string(gapWidth) + ")" +
                               ", X span=[" + std::to_string(coinMinX) + " to " + std::to_string(coinMaxX) + 
                               "], total coins=" + std::to_string(positions.size()));
                } else {
                    // Other levels: use standard obstacle bounds
                    if (topBandBottomEdge != std::numeric_limits<float>::max()) {
                        topStripeY = topBandBottomEdge + verticalPad;
                        if (topMinX < topMaxX) {
                            emitStripeInRange(topStripeY, topMinX, topMaxX);
                            if (verboseLogging) {
                                GN_LOG_INFO("ObstacleSystem::TopAndBottom [Level " + std::to_string(m_currentLevelId) + "] TOP coin row at Y=" + std::to_string(topStripeY) + 
                                           " (topBandBottom=" + std::to_string(topBandBottomEdge) + " + verticalPad=" + std::to_string(verticalPad) + 
                                           "), X span=[" + std::to_string(topMinX) + " to " + std::to_string(topMaxX) + "]");
                            }
                        }
                    }
                    if (bottomBandTopEdge != std::numeric_limits<float>::lowest()) {
                        bottomStripeY = bottomBandTopEdge - verticalPad;
                        if (bottomMinX < bottomMaxX) {
                            emitStripeInRange(bottomStripeY, bottomMinX, bottomMaxX);
                            if (verboseLogging) {
                                GN_LOG_INFO("ObstacleSystem::TopAndBottom [Level " + std::to_string(m_currentLevelId) + "] BOTTOM coin row at Y=" + std::to_string(bottomStripeY) + 
                                           " (bottomBandTop=" + std::to_string(bottomBandTopEdge) + " - verticalPad=" + std::to_string(verticalPad) + 
                                           "), X span=[" + std::to_string(bottomMinX) + " to " + std::to_string(bottomMaxX) + "]");
                            }
                        }
                    }
                    
                    if (verboseLogging) {
                        GN_LOG_INFO("ObstacleSystem::TopAndBottom [Level " + std::to_string(m_currentLevelId) + "] SUMMARY for groupId=" + std::to_string(groupId) + 
                                   ": TOP row Y=" + std::to_string(topStripeY) + ", BOTTOM row Y=" + std::to_string(bottomStripeY) + 
                                   ", GAP=" + std::to_string(bottomStripeY - topStripeY) + "px, total coins=" + std::to_string(positions.size()));
                    }
                }
                break;
            }
            case GroupPattern::Ground: {
                // Ground obstacles: place coins starting from the right edge of the outhouse, 
                // spreading evenly across the gap until the next group starts
                std::pair<float, float> currentGroupBounds; // (leftEdge, rightEdge)
                float groundObstacleTop = std::numeric_limits<float>::max();
                bool foundCurrentGroup = false;
                
                // Performance: Disabled coin spawning debug logging
                // GN_LOG_DEBUG("ObstacleSystem::Ground coin spawning - m_activeObstacles.size=" + std::to_string(m_activeObstacles.size()) + ", groupId=" + std::to_string(groupId));
                
                // First, find the current group's outhouse bounds
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (!g || g->id != groupId) continue;
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
                    if (!t || !s) continue;
                    
                    // Performance: Disabled coin spawning debug logging
                    // GN_LOG_DEBUG("ObstacleSystem::Ground found obstacle in group " + std::to_string(groupId) + " at x=" + std::to_string(t->position.x) + ", y=" + std::to_string(t->position.y) + ", sprite=" + s->textureId);
                    
                    // Only consider outhouses for coin positioning, not brick walls
                    // Brick walls are at Y=0, outhouses are at Y=1276
                    if (s->textureId == "Outhouse" || s->textureId == "OuthouseToilet") {
                        const float scaledW = s->width * std::abs(t->scale.x);
                        currentGroupBounds = std::make_pair(t->position.x, t->position.x + scaledW);
                        groundObstacleTop = std::min(groundObstacleTop, t->position.y);
                        foundCurrentGroup = true;
                        break; // Only need one outhouse per group
                    }
                }
                
                if (foundCurrentGroup && groundObstacleTop != std::numeric_limits<float>::max()) {
                    // Start coins 72px from the left edge of the outhouse (64px outhouse width + 8px margin)
                    float coinStartX = currentGroupBounds.first + (72.0f * m_baseScale);
                    
                    // Use the ACTUAL gap width passed from LevelManager manifest
                    float coinEndX = coinStartX + gapWidth;
                    
                    // Performance: Disabled coin spawning debug logging
                    // GN_LOG_DEBUG("ObstacleSystem::Ground using CONSISTENT standardGap=" + std::to_string(standardGap) + " (increased to 800.0f)");
                    
                    float coinRange = coinEndX - coinStartX;
                    
                    // Performance: Disabled coin spawning debug logging
                    // GN_LOG_DEBUG("ObstacleSystem::Ground coin range: start=" + std::to_string(coinStartX) + ", end=" + std::to_string(coinEndX) + ", range=" + std::to_string(coinRange) + ", groupId=" + std::to_string(groupId));
                    // GN_LOG_DEBUG("ObstacleSystem::Ground coin calculation details: outhouseLeftEdge=" + std::to_string(currentGroupBounds.first) + ", using CONSISTENT standardGap, 72px offset applied");
                    
                    if (coinRange > 100.0f) { // Always true with standardGap, but keeping safety check
                        // Place coins in this range, evenly spread
                        float coinY = groundObstacleTop + 600.0f; // Place coins halfway down the outhouse (higher Y = lower on screen)
                        // Performance: Disabled coin spawning debug logging
                        // GN_LOG_DEBUG("ObstacleSystem::Ground spawning coins at y=" + std::to_string(coinY) + " in range [" + std::to_string(coinStartX) + "," + std::to_string(coinEndX) + "] with range=" + std::to_string(coinRange));
                        emitGroundStripe(coinY, coinStartX, coinEndX);
                    } else {
                        // Performance: Disabled coin spawning debug logging
                        // GN_LOG_DEBUG("ObstacleSystem::Ground coin range too small: " + std::to_string(coinRange));
                    }
                } else {
                    // Performance: Disabled coin spawning debug logging
                    // GN_LOG_DEBUG("ObstacleSystem::Ground no coins spawned: groundObstacleTop=" + std::to_string(groundObstacleTop) + ", foundCurrentGroup=" + std::to_string(foundCurrentGroup));
                }
                break;
            }
            case GroupPattern::SnowScreenEdges: {
                // SNOW LEVEL UNIQUE PATTERN: Coins at TOP and BOTTOM of screen, horizontally aligned with pipe obstacles
                // This creates two horizontal coin rows: one hugging the top screen edge, one hugging the bottom
                const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                const float screenHeight = screenInfo.pixelHeight;
                const float topScreenY = 200.0f;  // Near top of screen (with safe area padding)
                const float bottomScreenY = screenHeight - 250.0f;  // Near bottom of screen (Y=2306, closer to bottom)
                
                // SNOW LEVEL: Coins centered in the gap BETWEEN toilet pairs
                // Use the actual gapWidth parameter passed from LevelManager
                // Current toilet LEFT edge is at minX
                // Previous toilet RIGHT edge was at: minX - gapWidth
                // Gap center is simply: minX - (gapWidth / 2)
                float gapCenterX = minX - (gapWidth / 2.0f);
                
                // DEBUG: Log the obstacle bounds and center calculation
                GN_LOG_INFO("Snow coin positioning: minX=" + std::to_string(minX) + ", maxX=" + std::to_string(maxX) + 
                           ", gapWidth=" + std::to_string(gapWidth) + 
                           ", gapCenterX=" + std::to_string(gapCenterX));
                
                // Spread coins across full gap width for Snow (need more spread)
                float coinSpreadWidth = gapWidth * 0.95f;
                float coinHalfWidth = coinSpreadWidth / 2.0f;
                float coinMinX = gapCenterX - coinHalfWidth;
                float coinMaxX = gapCenterX + coinHalfWidth;
                
                if (coinMinX < coinMaxX) {
                    GN_LOG_INFO("ObstacleSystem::SnowScreenEdges [Snow Level] 🪙 TOP row at Y=" + std::to_string(topScreenY) + 
                               ", BOTTOM row at Y=" + std::to_string(bottomScreenY) + 
                               ", X span CENTERED+WIDENED=[" + std::to_string(coinMinX) + " to " + std::to_string(coinMaxX) + 
                               "], total width=" + std::to_string(coinMaxX - coinMinX) + "px");
                    
                    emitStripeInRange(topScreenY, coinMinX, coinMaxX);       // Top screen edge
                    emitStripeInRange(bottomScreenY, coinMinX, coinMaxX);    // Bottom screen edge
                    
                    GN_LOG_INFO("ObstacleSystem::SnowScreenEdges spawned " + std::to_string(positions.size()) + 
                               " total coins (2 rows of 5) for groupId=" + std::to_string(groupId));
                }
                break;
            }
            case GroupPattern::TwoFunnel: {
                GN_LOG_INFO("🪙 ENTERING TwoFunnel case for groupId=" + std::to_string(groupId));
                // TwoFunnel pattern: Cluster pipes into vertical segments, spread 5 coins across each segment at center Y
                // Pattern has 3 vertical columns: left (3 pipes stacked), middle (2 pipes stacked), right (3 pipes stacked)
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
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float centerY = (topBandBottomEdge != std::numeric_limits<float>::max() && bottomBandTopEdge != std::numeric_limits<float>::lowest())
                        ? ((topBandBottomEdge + bottomBandTopEdge) * 0.5f)
                        : screenInfo.pixelHeight * 0.5f;
                    GN_LOG_DEBUG("ObstacleSystem::TwoFunnel clusters=" + std::to_string(clusters.size()) + " centerY=" + std::to_string(centerY));
                    // Emit for left/middle/right if available (should have 3 clusters for TwoByTwoFunnel)
                    if (clusters.size() >= 1) emitStripeInRange(centerY, clusters.front().left, clusters.front().right);
                    if (clusters.size() >= 3) emitStripeInRange(centerY, clusters[clusters.size()/2].left, clusters[clusters.size()/2].right);
                    if (clusters.size() >= 2) emitStripeInRange(centerY, clusters.back().left, clusters.back().right);
                } else {
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float centerY = (topBandBottomEdge != std::numeric_limits<float>::max() && bottomBandTopEdge != std::numeric_limits<float>::lowest())
                        ? ((topBandBottomEdge + bottomBandTopEdge) * 0.5f)
                        : screenInfo.pixelHeight * 0.5f;
                    emitStripe(centerY);
                }
                break;
            }
            case GroupPattern::PyramidBottom: {
                GN_LOG_INFO("🪙 ENTERING PyramidBottom case for groupId=" + std::to_string(groupId));
                // SEWER LEVEL: PyramidBottom - place 5 coins per TOP funnel pipe (complementary pipes)
                // Collect all top pipes, sort by X position, and emit 5 coins across each pipe's span
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
                    GN_LOG_DEBUG("ObstacleSystem::PyramidBottom complement stripe span=[" + std::to_string(p.left) + "," + std::to_string(p.right) + "] y=" + std::to_string(y));
                    emitStripeInRange(y, p.left, p.right);
                }
                break;
            }
            case GroupPattern::PyramidTop: {
                GN_LOG_INFO("🪙 ENTERING PyramidTop case for groupId=" + std::to_string(groupId));
                // SEWER LEVEL: PyramidTop - place 5 coins per BOTTOM funnel pipe (complementary pipes)
                // Collect all bottom pipes, sort by X position, and emit 5 coins across each pipe's span
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
                    GN_LOG_DEBUG("ObstacleSystem::PyramidTop complement stripe span=[" + std::to_string(p.left) + "," + std::to_string(p.right) + "] y=" + std::to_string(y));
                    emitStripeInRange(y, p.left, p.right);
                }
                break;
            }
            default: {
                GN_LOG_WARN("🪙 ENTERING DEFAULT case for groupId=" + std::to_string(groupId) + " pattern=" + std::to_string(static_cast<int>(pattern)));
                float y = (topBandBottomEdge != std::numeric_limits<float>::max() && bottomBandTopEdge != std::numeric_limits<float>::lowest())
                    ? ((topBandBottomEdge + bottomBandTopEdge) * 0.5f)
                    : 400.0f;
                emitStripe(y);
                break;
            }
        }
        
        GN_LOG_DEBUG("ObstacleSystem::CalculateCoinPositionsForGroup: groupId=" + std::to_string(groupId) + 
                     " positions=" + std::to_string(positions.size()) +
                     " minX=" + std::to_string(minX) + " maxX=" + std::to_string(maxX));
        return positions;
    }
    
    bool ObstacleSystem::IsGroupReadyForCoins(int groupId) const {
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
        
        GN_LOG_DEBUG("ObstacleSystem::IsGroupReadyForCoins: groupId=" + std::to_string(groupId) + 
                     " members=" + std::to_string(memberCount) + 
                     " ready=" + std::string(ready ? "true" : "false"));
        
        return ready;
    }
    
    GroupPattern ObstacleSystem::DetectGroupPattern(int groupId) const {
        // Find any group member and return its pattern
        for (Gnosis::Entity e : m_activeObstacles) {
            Group* g = m_ecsSystem->GetComponent<Group>(e);
            if (g && g->id == groupId) {
                return g->pattern;
            }
        }
        return GroupPattern::TopAndBottom; // fallback
    }

    // LEGACY DELETED: WrapGroupAroundScreen() - ~340 lines of complex wrapping logic
    // Replaced by LevelManager::WrapGroup() + UpdateGroupMemberPositions() orchestrator pattern
    // All wrapping, repositioning, and state reset now handled centrally by LevelManager

    void ObstacleSystem::RenderDebugHitboxes() {
        if (!m_debugMode) {
            // Performance: Disabled per-frame debug logging
            // GN_LOG_DEBUG("RenderDebugHitboxes: Debug mode disabled, skipping");
            return;
        }

        // Performance: Disabled per-frame debug logging
        // GN_LOG_DEBUG("RenderDebugHitboxes: Debug mode enabled, processing " + std::to_string(m_activeObstacles.size()) + " obstacles");
        
        int debugEntitiesFound = 0;
        for (Gnosis::Entity entity : m_activeObstacles) {
            if (!m_ecsSystem->HasComponent<Transform>(entity) || 
                !m_ecsSystem->HasComponent<Hitbox>(entity) || 
                !m_ecsSystem->HasComponent<Obstacle>(entity)) {
                continue;
            }

            auto* transform = m_ecsSystem->GetComponent<Transform>(entity);
            auto* hitbox = m_ecsSystem->GetComponent<Hitbox>(entity);
            auto* obstacle = m_ecsSystem->GetComponent<Obstacle>(entity);
            auto* sprite = m_ecsSystem->GetComponent<Sprite>(entity);

            if (!transform || !hitbox || !obstacle) {
                continue;
            }

            // Check if this entity has DebugDraw component
            if (m_ecsSystem->HasComponent<DebugDraw>(entity)) {
                debugEntitiesFound++;
                // Performance: Disabled per-obstacle debug logging
                // GN_LOG_DEBUG("RenderDebugHitboxes: Found debug entity " + std::to_string(entity) + " (" + obstacle->obstacleType + ")");
            }

            // Calculate hitbox world position
            float hitboxX, hitboxY;
            
            // Check if this is a spike ball (has PivotRotationRenderer) - use base center positioning
            if (m_ecsSystem->HasComponent<PivotRotationRenderer>(entity)) {
                // For spike balls, calculate world position relative to base center
                auto baseIt = m_spikeBallToBase.find(entity);
                if (baseIt != m_spikeBallToBase.end()) {
                    Transform* baseTransform = m_ecsSystem->GetComponent<Transform>(baseIt->second);
                                            if (baseTransform) {
                            // Use base center + hitbox offset for world position
                            float baseCenterX = baseTransform->position.x + (5.0f * baseTransform->scale.x);
                            float baseCenterY = baseTransform->position.y + (5.0f * baseTransform->scale.y);
                        hitboxX = baseCenterX + hitbox->offsetX;
                        hitboxY = baseCenterY + hitbox->offsetY;
                    } else {
                        // Fallback to spike ball transform if base not found
                        hitboxX = transform->position.x + hitbox->offsetX;
                        hitboxY = transform->position.y + hitbox->offsetY;
                    }
                } else {
                    // Fallback to spike ball transform if no base relationship found
                    hitboxX = transform->position.x + hitbox->offsetX;
                    hitboxY = transform->position.y + hitbox->offsetY;
                }
            } else {
                // Normal entities use standard top-left based positioning
                hitboxX = transform->position.x + (hitbox->offsetX * transform->scale.x);
                hitboxY = transform->position.y + (hitbox->offsetY * transform->scale.y);
            }
            
            float hitboxW = hitbox->width * transform->scale.x;
            float hitboxH = hitbox->height * transform->scale.y;

            // Performance: CRITICAL - Disabled per-obstacle per-frame debug logging (was creating 6000-9000 logs/sec!)
            // GN_LOG_DEBUG("Obstacle " + std::to_string(entity) + 
            //             " (" + obstacle->obstacleType + "): " +
            //             "pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ") " +
            //             "hitbox=(" + std::to_string(hitboxX) + "," + std::to_string(hitboxY) + "," + 
            //             std::to_string(hitboxW) + "," + std::to_string(hitboxY) + ") " +
            //             "isTrigger=" + std::to_string(hitbox->isTrigger) + 
            //             " pipeCleared=" + std::to_string(obstacle->pipeCleared));
        }
        
        // Performance: Disabled per-frame debug logging
        // GN_LOG_DEBUG("RenderDebugHitboxes: Found " + std::to_string(debugEntitiesFound) + " entities with DebugDraw components");
    }

    void ObstacleSystem::RemoveAllDebugDraws() {
        if (!m_ecsSystem) {
            return;
        }

        // Remove DebugDraw components from all active obstacles
        for (Gnosis::Entity entity : m_activeObstacles) {
            if (m_ecsSystem->HasComponent<DebugDraw>(entity)) {
                m_ecsSystem->RemoveComponent<DebugDraw>(entity);
            }
        }

        // Remove DebugDraw components from cactus pool
        for (Gnosis::Entity entity : m_cactusPool) {
            if (m_ecsSystem->HasComponent<DebugDraw>(entity)) {
                m_ecsSystem->RemoveComponent<DebugDraw>(entity);
            }
        }

        GN_LOG_INFO("Removed all DebugDraw components");
    }

    void ObstacleSystem::InitializeCactusSystem() {
        if (m_cactusPoolInitialized) {
            return;
        }

        // Clear any existing cactus pool
        for (Gnosis::Entity entity : m_cactusPool) {
            if (m_ecsSystem->HasComponent<Transform>(entity)) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_cactusPool.clear();

        // Define cactus types with correct dimensions and weights
        m_cactusTypes.clear();
        
        // Dancing cacti (these are sprite sheets with 8 frames each)
        // Use individual frame dimensions (64x90, 64x64) instead of full sprite sheet dimensions
        GN_LOG_DEBUG("Adding dancing cactus type: dancingcacti (64x90, 8 frames)");
        m_cactusTypes.push_back({"dancingcacti", 64.0f, 90.0f, 3.0f, true, 0.5f}); // Regular dancing cactus - individual frame is 64x90
        
        GN_LOG_DEBUG("Adding dancing cactus type: dancingcacticowboy (64x90, 8 frames)");
        m_cactusTypes.push_back({"dancingcacticowboy", 64.0f, 90.0f, 1.0f, true, 0.4f}); // Rare cowboy cactus - individual frame is 64x90
        
        GN_LOG_DEBUG("Adding dancing cactus type: dancingcactismall (64x64, 8 frames)");
        m_cactusTypes.push_back({"dancingcactismall", 64.0f, 64.0f, 2.0f, true, 0.6f}); // Small dancing cactus - individual frame is 64x64
        
        // Individual cacti - now active alongside dancing cacti
        m_cactusTypes.push_back({"CactiA", 64.0f, 48.0f, 2.5f, false, 0.0f}); // 64x48
        m_cactusTypes.push_back({"CactiB", 32.0f, 32.0f, 2.5f, false, 0.0f}); // 32x32
        m_cactusTypes.push_back({"CactiC", 64.0f, 80.0f, 2.5f, false, 0.0f}); // 64x80
        m_cactusTypes.push_back({"CactiD", 64.0f, 80.0f, 2.0f, false, 0.0f}); // 64x80
        m_cactusTypes.push_back({"CactiE", 64.0f, 80.0f, 2.0f, false, 0.0f}); // 64x80
        m_cactusTypes.push_back({"CactiBush", 16.0f, 16.0f, 1.5f, false, 0.0f}); // 16x16 - small decorative

        // Create the cactus pool
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        SpawnCactusPool(screenInfo.pixelWidth + 100.0f);
        
        m_cactusPoolInitialized = true;
        GN_LOG_INFO("Cactus system initialized with " + std::to_string(m_cactusPool.size()) + " cacti");
    }

    void ObstacleSystem::SpawnCactusPool(float startX) {
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float currentX = startX;
        float groundY = screenInfo.pixelHeight - 200.0f; // Will be calculated per cactus type
        
        // Create 16 cacti in a pool
        for (int i = 0; i < CACTUS_POOL_SIZE; i++) {
            // Select random cactus type based on weights
            float totalWeight = 0.0f;
            for (const auto& cactusType : m_cactusTypes) {
                totalWeight += cactusType.weight;
            }
            
            float randomValue = static_cast<float>(rand()) / RAND_MAX * totalWeight;
            float currentWeight = 0.0f;
            const CactusType* selectedType = nullptr;
            
            for (const auto& cactusType : m_cactusTypes) {
                currentWeight += cactusType.weight;
                if (randomValue <= currentWeight) {
                    selectedType = &cactusType;
                    break;
                }
            }
            
            if (!selectedType) {
                selectedType = &m_cactusTypes[0]; // Fallback
            }

            // Calculate position with increased spacing and less overlap
            float spacing = 200.0f + (rand() % 300); // 200-500px spacing (increased from 100-300)
            float overlap = (rand() % 2) * 10.0f; // 0 or 10px overlap (reduced from 0, 20, or 40)
            currentX += spacing - overlap;
            
            // Cacti are always grounded at the same level
            // Calculate ground position: screen bottom - (cactus height * m_baseScale)
            float finalY = screenInfo.pixelHeight - (selectedType->height * m_baseScale);
            
            GN_LOG_DEBUG("Cactus positioning: height=" + std::to_string(selectedType->height) + ", m_baseScale=" + std::to_string(m_baseScale) + ", finalY=" + std::to_string(finalY) + ", screenHeight=" + std::to_string(screenInfo.pixelHeight));
            
            // Create cactus entity
            Gnosis::Entity cactus = CreateCactusEntity(
                selectedType->texture, 
                currentX, 
                finalY, 
                m_baseScale, // Use the proper 8.0f scale for pixel art visibility
                selectedType->isDancing,
                selectedType->width,  // Pass actual width
                selectedType->height  // Pass actual height
            );
            
            GN_LOG_DEBUG("Spawned cactus: " + selectedType->texture + " at x=" + std::to_string(currentX) + ", y=" + std::to_string(finalY) + ", scale=" + std::to_string(m_baseScale) + ", isDancing=" + (selectedType->isDancing ? "true" : "false"));
            
            m_cactusPool.push_back(cactus);
            m_cactusTypeMap[cactus] = selectedType; // Store cactus type for proper positioning
            
            // Add to active obstacles for rendering
            m_activeObstacles.push_back(cactus);
        }
    }

    void ObstacleSystem::UpdateCactusAnimation(float deltaTime) {
        static int frameCount = 0;
        frameCount++;
        
        for (Gnosis::Entity cactus : m_cactusPool) {
            if (!m_ecsSystem->HasComponent<Sprite>(cactus)) {
                continue;
            }
            
            auto* sprite = m_ecsSystem->GetComponent<Sprite>(cactus);
            
            // Update animation for dancing cacti
            if (sprite->isAnimated && sprite->playing) {
                sprite->currentFrameTime += deltaTime;
                if (sprite->currentFrameTime >= sprite->frameTime) {
                    sprite->currentFrameTime = 0.0f;
                    sprite->currentFrame++;
                    if (sprite->currentFrame >= sprite->frameCount) {
                        if (sprite->loop) {
                            sprite->currentFrame = 0;
                        } else {
                            sprite->playing = false;
                            sprite->hasCompleted = true;
                        }
                    }
                    
                    // Log animation updates every 60 frames (about once per second)
                    if (frameCount % 60 == 0) {
                        GN_LOG_DEBUG("Dancing cactus animation: frame=" + std::to_string(sprite->currentFrame) + ", currentFrameTime=" + std::to_string(sprite->currentFrameTime));
                    }
                }
            }
        }
    }

    void ObstacleSystem::WrapCactusPool(float worldScrollDistance) {
        // Check if any cacti are off-screen and need wrapping
        for (Gnosis::Entity cactus : m_cactusPool) {
            if (!m_ecsSystem->HasComponent<Transform>(cactus) || !m_ecsSystem->HasComponent<Sprite>(cactus)) {
                continue;
            }
            
            auto* transform = m_ecsSystem->GetComponent<Transform>(cactus);
            auto* sprite = m_ecsSystem->GetComponent<Sprite>(cactus);
            
            // If cactus is off-screen to the left, wrap it to the right
            if (transform->position.x + sprite->width * transform->scale.x < -100.0f) {
                // Find the rightmost cactus
                float rightmostX = -std::numeric_limits<float>::max();
                for (Gnosis::Entity otherCactus : m_cactusPool) {
                    if (m_ecsSystem->HasComponent<Transform>(otherCactus) && m_ecsSystem->HasComponent<Sprite>(otherCactus)) {
                        auto* otherTransform = m_ecsSystem->GetComponent<Transform>(otherCactus);
                        auto* otherSprite = m_ecsSystem->GetComponent<Sprite>(otherCactus);
                        float otherRight = otherTransform->position.x + otherSprite->width * otherTransform->scale.x;
                        if (otherRight > rightmostX) {
                            rightmostX = otherRight;
                        }
                    }
                }
                
                // Position this cactus to the right of the rightmost one
                float spacing = 100.0f + (rand() % 200); // 100-300px spacing
                transform->position.x = rightmostX + spacing;
                
                // Cacti are always grounded at the same level
                // Use the stored cactus type for proper ground positioning
                auto it = m_cactusTypeMap.find(cactus);
                if (it != m_cactusTypeMap.end()) {
                    const CactusType* cactusType = it->second;
                    // Calculate proper ground position: screen bottom - (cactus height * scale)
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float groundY = screenInfo.pixelHeight - (cactusType->height * m_baseScale);
                    transform->position.y = groundY;
                } else {
                    // Fallback to default positioning - use the smallest cactus height as default
                    const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                    float groundY = screenInfo.pixelHeight - (64.0f * m_baseScale); // Smallest cactus height
                    transform->position.y = groundY;
                }
            }
        }
    }

    std::pair<Gnosis::Entity, Gnosis::Entity> ObstacleSystem::SpawnSnowPattern_ToiletPair(float x, int groupId, float gapWidth) {
        // Create oscillating snow toilet pair with vertical movement for timing challenge
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenHeight = screenInfo.pixelHeight;
        float toiletHeight = 288.0f * m_baseScale; // Scaled toilet height (updated from 190 to 288)
        
        // Position top toilet to match Park level's effective gap range
        // Park: 190px toilets with minTopY=-152, maxTopY=-38 creates gap range y=38 to y=952
        // Snow: 288px toilets need minTopY=-250, maxTopY=-136 to match same gap range
        float minTopY = -250.0f * m_baseScale; // Top toilet bottom edge starts at y=38 (matches Park)
        float maxTopY = -136.0f * m_baseScale; // Top toilet bottom edge starts at y=152 (matches Park)
        
        // Generate random position for top toilet within allowed range
        float randomTopY = minTopY + (maxTopY - minTopY) * ((float)rand() / RAND_MAX);
        
        // Fixed gap height between toilets (maintained during oscillation)
        // Reduced from 800px to compensate for taller 288px toilets (vs Park's 190px)
        // 256px keeps proper visual gap while accounting for extra toilet height
        float fixedGapHeight = 256.0f;
        
        // Calculate bottom toilet position
        float topToiletY = randomTopY;
        float bottomToiletY = randomTopY + toiletHeight + fixedGapHeight;
        
        // DEBUG: Log exact positioning values and effective gap range
        GN_LOG_INFO("🚽 SNOW TOILET POSITIONING (matches Park effective gap range):");
        GN_LOG_INFO("  toiletHeight: " + std::to_string(toiletHeight) + "px (288 * " + std::to_string(m_baseScale) + ")");
        GN_LOG_INFO("  minTopY: " + std::to_string(minTopY) + "px → gap starts at y=" + std::to_string(minTopY + toiletHeight));
        GN_LOG_INFO("  maxTopY: " + std::to_string(maxTopY) + "px → gap starts at y=" + std::to_string(maxTopY + toiletHeight));
        GN_LOG_INFO("  topToiletY: " + std::to_string(topToiletY) + "px");
        GN_LOG_INFO("  bottomToiletY: " + std::to_string(bottomToiletY) + "px");
        GN_LOG_INFO("  fixedGapHeight: " + std::to_string(fixedGapHeight) + "px (reduced from 800px for taller toilets)");
        GN_LOG_INFO("  Effective gap range: y=" + std::to_string(topToiletY + toiletHeight) + " to y=" + std::to_string(bottomToiletY));
        GN_LOG_INFO("  Bottom toilet bottom edge: y=" + std::to_string(bottomToiletY + toiletHeight));
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        // Create top snow toilet with oscillation
        Gnosis::Entity topToilet = m_ecsSystem->CreateEntity();
        
        Transform topTransform(Gnosis::GNVector2(x, topToiletY), 0.0f, 
                              Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite topSprite("TopToiletSnow", 65.0f, 288.0f);  // Use snow texture! (height updated to 288)
        topSprite.layer = 3;
        topSprite.visible = true;
        
        Physics topPhysics;
        topPhysics.velocity.x = -m_worldSpeed;
        topPhysics.useGravity = false;
        
        Hitbox topCollider;
        topCollider.type = ColliderType::Rectangle;
        // Hitbox configuration adjusted for 288px tall sprite (vs Park's 190px)
        topCollider.width = 20.0f;
        topCollider.height = 226.0f; // Fixed height: 226px hitbox
        topCollider.offsetY = -1.0f; // Start 30px down from sprite top (sprite center + offsetY - halfHeight = 144 + (-1) - 113 = 30)
        topCollider.isStatic = false;
        topCollider.tag = "Obstacle";
        
        // Add oscillation behavior - this is the key snow level feature!
        Obstacle topObstacle;
        topObstacle.type = ObstacleType::SnowToiletTop;  // NEW: Use enum
        topObstacle.damage = 1;
        topObstacle.isDestructible = false;
        topObstacle.health = 1;
        topObstacle.behavior = static_cast<int>(ToiletBehavior::OSCILLATE_VERTICAL);
        topObstacle.oscillationSpeed = 1.8f;  // Moderate speed for timing challenge  
        topObstacle.oscillationRange = 80.0f; // Good range for player timing
        topObstacle.oscillationTimer = 0.0f;
        topObstacle.basePosition = Gnosis::GNVector2(x, topToiletY);
        topObstacle.isTopPart = true;
        
        m_ecsSystem->AddComponent<Transform>(topToilet, topTransform);
        m_ecsSystem->AddComponent<Sprite>(topToilet, topSprite);
        m_ecsSystem->AddComponent<Physics>(topToilet, topPhysics);
        m_ecsSystem->AddComponent<Hitbox>(topToilet, topCollider);
        m_ecsSystem->AddComponent<Obstacle>(topToilet, topObstacle);



        // Create bottom snow toilet with same oscillation (linked movement)
        Gnosis::Entity bottomToilet = m_ecsSystem->CreateEntity();
        
        Transform bottomTransform(Gnosis::GNVector2(x, bottomToiletY), 0.0f, 
                                 Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite bottomSprite("BottomToiletSnow", 65.0f, 288.0f);  // Use snow texture! (height updated to 288)
        bottomSprite.layer = 3;
        bottomSprite.visible = true;
        
        Physics bottomPhysics;
        bottomPhysics.velocity.x = -m_worldSpeed;
        bottomPhysics.useGravity = false;
        
        Hitbox bottomCollider;
        bottomCollider.type = ColliderType::Rectangle;
        // Hitbox configuration adjusted for 288px tall sprite (vs Park's 190px)
        bottomCollider.width = 20.0f;
        bottomCollider.height = 226.0f; // Fixed height: 226px hitbox
        bottomCollider.offsetY = 1.0f; // End 30px up from sprite bottom (sprite center + offsetY + halfHeight = 144 + 1 + 113 = 258, sprite ends at 288)
        bottomCollider.isStatic = false;
        bottomCollider.tag = "Obstacle";
        
        // Add same oscillation behavior - pair moves together
        Obstacle bottomObstacle;
        bottomObstacle.type = ObstacleType::SnowToiletBottom;  // NEW: Use enum
        bottomObstacle.damage = 1;
        bottomObstacle.isDestructible = false;
        bottomObstacle.health = 1;
        bottomObstacle.behavior = static_cast<int>(ToiletBehavior::OSCILLATE_VERTICAL);
        bottomObstacle.oscillationSpeed = 1.8f;  // Same speed as top
        bottomObstacle.oscillationRange = 80.0f; // Same range as top
        bottomObstacle.oscillationTimer = 0.0f;  // Same starting phase
        bottomObstacle.basePosition = Gnosis::GNVector2(x, bottomToiletY);
        bottomObstacle.isTopPart = false;
        bottomObstacle.pairedEntity = topToilet; // Link to top toilet
        
        m_ecsSystem->AddComponent<Transform>(bottomToilet, bottomTransform);
        m_ecsSystem->AddComponent<Sprite>(bottomToilet, bottomSprite);
        m_ecsSystem->AddComponent<Physics>(bottomToilet, bottomPhysics);
        m_ecsSystem->AddComponent<Hitbox>(bottomToilet, bottomCollider);
        m_ecsSystem->AddComponent<Obstacle>(bottomToilet, bottomObstacle);
        
        // Link the pair for synchronized movement
        topObstacle.pairedEntity = bottomToilet;
        m_ecsSystem->GetComponent<Obstacle>(topToilet)->pairedEntity = bottomToilet;
        
        // Add Group components for coin calculation
        Group topGroup;
        topGroup.id = groupId;
        topGroup.isLeader = true;
        topGroup.offsetX = 0.0f;
        topGroup.offsetY = 0.0f;
        topGroup.groupWidth = 64.0f * m_baseScale;
        topGroup.pattern = GroupPattern::SnowScreenEdges;  // SNOW: Screen edge pattern for coins
        
        Group bottomGroup;
        bottomGroup.id = groupId;
        bottomGroup.isLeader = false;
        bottomGroup.offsetX = 0.0f;
        bottomGroup.offsetY = fixedGapHeight + toiletHeight;
        bottomGroup.groupWidth = 64.0f * m_baseScale;
        bottomGroup.pattern = GroupPattern::SnowScreenEdges;  // SNOW: Screen edge pattern for coins
        
        m_ecsSystem->AddComponent<Group>(topToilet, topGroup);
        m_ecsSystem->AddComponent<Group>(bottomToilet, bottomGroup);
        
        // Track obstacles for management
        m_activeObstacles.push_back(topToilet);
        m_activeObstacles.push_back(bottomToilet);
        
        GN_LOG_INFO("Spawned oscillating snow toilet pair at x=" + std::to_string(x) + " with group " + std::to_string(groupId) + 
                   " - VERTICAL gap=" + std::to_string(fixedGapHeight) + "px, topY=" + std::to_string(topToiletY) + ", bottomY=" + std::to_string(bottomToiletY));
        
        // Return entities for orchestrator to add to GroupManifest
        return std::make_pair(topToilet, bottomToilet);
    }

    void ObstacleSystem::UpdateObstacleOscillation(float deltaTime) {
        if (!m_ecsSystem) return;
        
        // Update oscillation for all obstacles with oscillation behavior
        for (Gnosis::Entity entity : m_activeObstacles) {
            if (!m_ecsSystem->HasComponent<Obstacle>(entity) || !m_ecsSystem->HasComponent<Transform>(entity)) {
                continue;
            }
            
            Obstacle* obstacle = m_ecsSystem->GetComponent<Obstacle>(entity);
            Transform* transform = m_ecsSystem->GetComponent<Transform>(entity);
            
            if (!obstacle || !transform) continue;
            
            // Only update oscillating obstacles
            if (obstacle->behavior == static_cast<int>(ToiletBehavior::OSCILLATE_VERTICAL)) {
                // Update oscillation timer
                obstacle->oscillationTimer += deltaTime;
                
                // Calculate vertical offset using sine wave
                float offsetY = std::sin(obstacle->oscillationTimer * obstacle->oscillationSpeed) * obstacle->oscillationRange;
                
                // Apply oscillation to position (basePosition.y + offset)
                transform->position.y = obstacle->basePosition.y + offsetY;
                
                // For toilet pairs, sync the paired entity's oscillation
                if (obstacle->pairedEntity != 0 && m_ecsSystem->IsEntityValid(obstacle->pairedEntity)) {
                    Obstacle* pairedObstacle = m_ecsSystem->GetComponent<Obstacle>(obstacle->pairedEntity);
                    Transform* pairedTransform = m_ecsSystem->GetComponent<Transform>(obstacle->pairedEntity);
                    
                    if (pairedObstacle && pairedTransform) {
                        // Sync oscillation timer for synchronized movement
                        pairedObstacle->oscillationTimer = obstacle->oscillationTimer;
                        
                        // Apply same oscillation to paired entity
                        float pairedOffsetY = std::sin(pairedObstacle->oscillationTimer * pairedObstacle->oscillationSpeed) * pairedObstacle->oscillationRange;
                        pairedTransform->position.y = pairedObstacle->basePosition.y + pairedOffsetY;
                    }
                }
            }
            else if (obstacle->behavior == static_cast<int>(ToiletBehavior::OSCILLATE_HORIZONTAL)) {
                // Update horizontal oscillation for sewer pipes (future enhancement)
                obstacle->oscillationTimer += deltaTime;
                float offsetX = std::sin(obstacle->oscillationTimer * obstacle->oscillationSpeed) * obstacle->oscillationRange;
                transform->position.x = obstacle->basePosition.x + offsetX;
                
                // Sync paired entity if exists
                if (obstacle->pairedEntity != 0 && m_ecsSystem->IsEntityValid(obstacle->pairedEntity)) {
                    Obstacle* pairedObstacle = m_ecsSystem->GetComponent<Obstacle>(obstacle->pairedEntity);
                    Transform* pairedTransform = m_ecsSystem->GetComponent<Transform>(obstacle->pairedEntity);
                    
                    if (pairedObstacle && pairedTransform) {
                        pairedObstacle->oscillationTimer = obstacle->oscillationTimer;
                        float pairedOffsetX = std::sin(pairedObstacle->oscillationTimer * pairedObstacle->oscillationSpeed) * pairedObstacle->oscillationRange;
                        pairedTransform->position.x = pairedObstacle->basePosition.x + pairedOffsetX;
                    }
                }
            }
        }
    }

    std::vector<Gnosis::Entity> ObstacleSystem::SpawnCastlePattern_GoldToiletPair(float x, int groupId, float gapWidth) {
        std::vector<Gnosis::Entity> spawnedEntities;
        
        // Create oscillating gold toilet pair with vertical movement for castle level
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenHeight = screenInfo.pixelHeight;
        float toiletHeight = 288.0f * m_baseScale; // Scaled toilet height (updated from 180 to 288)
        
        // Position top toilet to match Park level's effective gap range
        // Park: 190px toilets with minTopY=-152, maxTopY=-38 creates gap range y=38 to y=952
        // Castle: 288px toilets need minTopY=-250, maxTopY=-136 to match same gap range
        float minTopY = -250.0f * m_baseScale; // Top toilet bottom edge starts at y=38 (matches Park)
        float maxTopY = -136.0f * m_baseScale; // Top toilet bottom edge starts at y=152 (matches Park)
        
        // Generate random position for top toilet within allowed range
        float randomTopY = minTopY + (maxTopY - minTopY) * ((float)rand() / RAND_MAX);
        
        // Fixed gap height between toilets (maintained during oscillation)
        // Reduced from 800px to compensate for taller 288px toilets (vs Park's 190px)
        // 256px keeps proper visual gap while accounting for extra toilet height
        float fixedGapHeight = 256.0f;
        
        // Calculate bottom toilet position
        float topToiletY = randomTopY;
        float bottomToiletY = randomTopY + toiletHeight + fixedGapHeight;
        
        // NOTE: groupId now passed as parameter (managed by LevelManager orchestrator)
        
        // Create top gold toilet with oscillation
        Gnosis::Entity topToilet = m_ecsSystem->CreateEntity();
        spawnedEntities.push_back(topToilet);
        
        Transform topTransform(Gnosis::GNVector2(x, topToiletY), 0.0f, 
                              Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite topSprite("TopToiletGold", 65.0f, 288.0f);  // Use gold texture! (height updated to 288)
        topSprite.layer = 3;
        topSprite.visible = true;
        
        Physics topPhysics;
        topPhysics.velocity.x = -m_worldSpeed;
        topPhysics.useGravity = false;
        
        Hitbox topCollider;
        topCollider.type = ColliderType::Rectangle;
        // Hitbox configuration adjusted for 288px tall sprite (vs Park's 190px)
        topCollider.width = 20.0f;
        topCollider.height = 226.0f; // Fixed height: 226px hitbox
        topCollider.offsetY = -1.0f; // Start 30px down from sprite top (sprite center + offsetY - halfHeight = 144 + (-1) - 113 = 30)
        topCollider.isStatic = false;
        topCollider.tag = "Obstacle";
        
        // Add oscillation behavior for castle level challenge
        Obstacle topObstacle;
        topObstacle.type = ObstacleType::GoldToiletTop;  // NEW: Use enum
        topObstacle.damage = 1;
        topObstacle.isDestructible = false;
        topObstacle.health = 1;
        topObstacle.behavior = static_cast<int>(ToiletBehavior::OSCILLATE_VERTICAL);
        topObstacle.oscillationSpeed = 2.0f;  // Castle level speed
        topObstacle.oscillationRange = 60.0f; // Castle level range
        topObstacle.oscillationTimer = 0.0f;
        topObstacle.basePosition = Gnosis::GNVector2(x, topToiletY);
        topObstacle.isTopPart = true;
        
        m_ecsSystem->AddComponent<Transform>(topToilet, topTransform);
        m_ecsSystem->AddComponent<Sprite>(topToilet, topSprite);
        m_ecsSystem->AddComponent<Physics>(topToilet, topPhysics);
        m_ecsSystem->AddComponent<Hitbox>(topToilet, topCollider);
        m_ecsSystem->AddComponent<Obstacle>(topToilet, topObstacle);
        
        // Create bottom gold toilet with same oscillation (linked movement)
        Gnosis::Entity bottomToilet = m_ecsSystem->CreateEntity();
        spawnedEntities.push_back(bottomToilet);
        
        Transform bottomTransform(Gnosis::GNVector2(x, bottomToiletY), 0.0f, 
                                 Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite bottomSprite("BottomToiletGold", 65.0f, 288.0f);  // Use gold texture! (height updated to 288)
        bottomSprite.layer = 3;
        bottomSprite.visible = true;
        
        Physics bottomPhysics;
        bottomPhysics.velocity.x = -m_worldSpeed;
        bottomPhysics.useGravity = false;
        
        Hitbox bottomCollider;
        bottomCollider.type = ColliderType::Rectangle;
        // Hitbox configuration adjusted for 288px tall sprite (vs Park's 190px)
        bottomCollider.width = 20.0f;
        bottomCollider.height = 226.0f; // Fixed height: 226px hitbox
        bottomCollider.offsetY = 1.0f; // End 30px up from sprite bottom (sprite center + offsetY + halfHeight = 144 + 1 + 113 = 258, sprite ends at 288)
        bottomCollider.isStatic = false;
        bottomCollider.tag = "Obstacle";
        
        // Add same oscillation behavior - pair moves together
        Obstacle bottomObstacle;
        bottomObstacle.type = ObstacleType::GoldToiletBottom;  // NEW: Use enum
        bottomObstacle.damage = 1;
        bottomObstacle.isDestructible = false;
        bottomObstacle.health = 1;
        bottomObstacle.behavior = static_cast<int>(ToiletBehavior::OSCILLATE_VERTICAL);
        bottomObstacle.oscillationSpeed = 2.0f;  // Same speed as top
        bottomObstacle.oscillationRange = 60.0f; // Same range as top
        bottomObstacle.oscillationTimer = 0.0f;  // Same starting phase
        bottomObstacle.basePosition = Gnosis::GNVector2(x, bottomToiletY);
        bottomObstacle.isTopPart = false;
        bottomObstacle.pairedEntity = topToilet; // Link to top toilet
        
        m_ecsSystem->AddComponent<Transform>(bottomToilet, bottomTransform);
        m_ecsSystem->AddComponent<Sprite>(bottomToilet, bottomSprite);
        m_ecsSystem->AddComponent<Physics>(bottomToilet, bottomPhysics);
        m_ecsSystem->AddComponent<Hitbox>(bottomToilet, bottomCollider);
        m_ecsSystem->AddComponent<Obstacle>(bottomToilet, bottomObstacle);
        
        // Debug mode disabled for Castle level - no debug rectangles needed
        
        // Link the pair for synchronized movement
        topObstacle.pairedEntity = bottomToilet;
        m_ecsSystem->GetComponent<Obstacle>(topToilet)->pairedEntity = bottomToilet;
        
        // NOTE: Group components added here for backward compatibility with legacy coin positioning
        // LevelManager orchestrator will build the GroupManifest from these entities
        Group topGroup;
        topGroup.id = groupId;
        topGroup.isLeader = true;
        topGroup.offsetX = 0.0f;
        topGroup.offsetY = 0.0f;
        topGroup.groupWidth = 64.0f * m_baseScale;  // CORRECTED: 64px sprite width, not 65px
        topGroup.pattern = GroupPattern::TopAndBottom;
        
        Group bottomGroup;
        bottomGroup.id = groupId;
        bottomGroup.isLeader = false;
        bottomGroup.offsetX = 0.0f;
        bottomGroup.offsetY = fixedGapHeight + toiletHeight;
        bottomGroup.groupWidth = 64.0f * m_baseScale;  // CORRECTED: 64px sprite width
        bottomGroup.pattern = GroupPattern::TopAndBottom;
        
        m_ecsSystem->AddComponent<Group>(topToilet, topGroup);
        m_ecsSystem->AddComponent<Group>(bottomToilet, bottomGroup);
        
        // Add GroupGap component to ensure proper spacing between groups
        // Use the actual gapWidth passed from LevelManager (1400.0f for Castle)
        GroupGap groupGap(gapWidth, false, "castle_toilet_spacing");
        m_ecsSystem->AddComponent<GroupGap>(topToilet, groupGap);
        
        // Track obstacles for management
        m_activeObstacles.push_back(topToilet);
        m_activeObstacles.push_back(bottomToilet);
        
        // NOTE: GroupManifest will be built by LevelManager::SpawnGroup()
        // Decorations are spawned inline below and WILL be collected by orchestrator
        
        // FIXED: Include decorations IN THE SAME GROUP so they wrap with toilets
        // Decorations don't affect coin positioning because they lack Obstacle component (coin calc requires Obstacle)
        
        // Spawn curtain
        Gnosis::Entity curtain = SpawnCastleCurtain(x, groupId);
        spawnedEntities.push_back(curtain);
        
        // Spawn decorative elements positioned relative to this toilet group
        // Floor torches: positioned to left and right of toilet group
        // Account for actual texture content (remove 16px padding from sprite dimensions)
        float toiletWidthForTorchSpacing = (64.0f - 16.0f) * m_baseScale; // Actual toilet width without padding (64px, not 65px)
        float leftTorchX = x - 80.0f; // Push left torch further left
        float rightTorchX = x + toiletWidthForTorchSpacing + 80.0f; // Push right torch further right
        
        // Spawn floor torches
        float leftTorchOffsetX = leftTorchX - x;
        float rightTorchOffsetX = rightTorchX - x;
        Gnosis::Entity leftTorch = SpawnCastleFloorTorch(leftTorchX, groupId, leftTorchOffsetX);
        Gnosis::Entity rightTorch = SpawnCastleFloorTorch(rightTorchX, groupId, rightTorchOffsetX);
        spawnedEntities.push_back(leftTorch);
        spawnedEntities.push_back(rightTorch);
        
        // CASTLE CENTERPIECE POSITIONING: Calculate based on CURTAIN positions, not toilet positions
        // Curtains are 256px wide at screen-based scale (curtainScale = screenHeight / 512)
        // screenInfo and screenHeight already defined at top of function
        float curtainScale = screenHeight / 512.0f;  // Screen-based scale for curtains
        float curtainWidth = 256.0f * curtainScale;
        
        // FIXED: Calculate gap center using actual toiletWidth from manifest
        // Current toilet LEFT edge is at x
        // Previous toilet LEFT edge was at: x - toiletWidth - gapWidth
        // Previous toilet RIGHT edge was at: x - gapWidth
        // Current toilet LEFT edge is at: x
        // Gap center is at: (x - gapWidth + x) / 2 = x - (gapWidth / 2)
        float toiletWidth = 64.0f * m_baseScale; // Actual toilet sprite width
        float gapCenterX = x - (gapWidth / 2.0f);
        
        GN_LOG_INFO("[CASTLE_DECORATIONS] Toilet at X=" + std::to_string(x));
        GN_LOG_INFO("[CASTLE_DECORATIONS] Gap center X=" + std::to_string(gapCenterX) + " (gapWidth=" + std::to_string(gapWidth) + ")");
        
        // Chandeliers: positioned at top-left and top-right corners of curtains
        // Curtain actual position: x - (curtainWidth / 2) + (toiletWidth / 2)
        // This centers the curtain on the toilet center
        const float chandelierWidth = 32.0f * m_baseScale; // 256px at scale 8
        // toiletWidth already defined above at line 2297
        
        // Calculate actual curtain position (it's centered on toilet center)
        float curtainX = x - (curtainWidth / 2.0f) + (toiletWidth / 2.0f);
        float curtainLeftEdge = curtainX;
        float curtainRightEdge = curtainX + curtainWidth;
        
        // Position chandeliers at curtain corners
        // Chandelier sprite origin is top-left
        // Left chandelier: at left edge (sprite starts at curtain left)
        // Right chandelier: at right edge minus chandelier width (so sprite ends at curtain right)
        float leftChandelierX = curtainLeftEdge;
        float rightChandelierX = curtainRightEdge - chandelierWidth;
        
        float leftChandelierOffsetX = leftChandelierX - x;
        float rightChandelierOffsetX = rightChandelierX - x;
        
        Gnosis::Entity leftChandelier = SpawnCastleChandelier(leftChandelierX, groupId, leftChandelierOffsetX);
        Gnosis::Entity rightChandelier = SpawnCastleChandelier(rightChandelierX, groupId, rightChandelierOffsetX);
        spawnedEntities.push_back(leftChandelier);
        spawnedEntities.push_back(rightChandelier);
        
        GN_LOG_INFO("[CASTLE_CHANDELIER] Left at X=" + std::to_string(leftChandelierX) + ", Right at X=" + std::to_string(rightChandelierX) + " (gap center=" + std::to_string(gapCenterX) + ")");
        
        // Spawn centerpiece in center of the gap - randomly choose between:
        // 1. Torch pillar, 2. Decorative painting, 3. Spike ball obstacle
        static int centerCounter = 0;
        int centerpieceType = centerCounter % 3; // 3 different centerpiece types
        
        // Centerpiece positioned at gap center (matching coin center)
        float centerOffsetX = gapCenterX - x;
        Gnosis::Entity centerpiece = 0;
        switch (centerpieceType) {
            case 0:
                centerpiece = SpawnCastleTorchPillar(gapCenterX, groupId, centerOffsetX);
                break;
            case 1:
                centerpiece = SpawnCastleDecorativePainting(gapCenterX, groupId, centerOffsetX);
                break;
            case 2:
                // Spike ball (and its base) need to be added
                centerpiece = SpawnCastleSpikeBall(gapCenterX, groupId, centerOffsetX);
                // Also need to find and add the BASE since SpawnCastleSpikeBall returns the ball
                // The base is stored in m_spikeBallToBase map
                if (m_spikeBallToBase.find(centerpiece) != m_spikeBallToBase.end()) {
                    spawnedEntities.push_back(m_spikeBallToBase[centerpiece]); // Add base
                }
                break;
        }
        if (centerpiece != 0) {
            spawnedEntities.push_back(centerpiece);
        }
        centerCounter++;
        
        GN_LOG_INFO("[CASTLE_CENTERPIECE] Type=" + std::to_string(centerpieceType) + " at X=" + std::to_string(gapCenterX) + ", offset from toilet=" + std::to_string(centerOffsetX));
        
        GN_LOG_INFO("[CASTLE_TOILET] Spawned oscillating gold toilet pair at x=" + std::to_string(x) + " with group " + std::to_string(groupId));
        GN_LOG_INFO("[CASTLE_TOILET] Returned " + std::to_string(spawnedEntities.size()) + " entities to manifest");
        
        // Return ALL entities for manifest tracking
        return spawnedEntities;
    }

    Gnosis::Entity ObstacleSystem::SpawnCastleCurtain(float x, int groupId /* -1 = no group */) {
        // Spawn curtain centered on toilet pair, spanning full screen height
        // Curtains are 256x512 - use screen-based scaling like sewer backgrounds (not baseScale)
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenHeight = screenInfo.pixelHeight;
        
        // Calculate scale based on screen height divided by texture height (like sewer backgrounds)
        // This makes it span the screen height properly without being gigantic
        float curtainTextureHeight = 512.0f;
        float curtainScale = screenHeight / curtainTextureHeight; // e.g., 2556 / 512 = ~5.0
        
        float curtainWidth = 256.0f * curtainScale;
        float curtainHeight = 512.0f * curtainScale;
        
        // Center curtain X-wise on the toilet pair
        // Toilet is positioned at x, curtain should be centered on it
        float curtainX = x - (curtainWidth * 0.5f) + (64.0f * m_baseScale * 0.5f); // Center on toilet (64px toilet width)
        
        // Position curtain at top of screen, it will span downward
        float curtainY = 0.0f;
        
        Gnosis::Entity curtain = m_ecsSystem->CreateEntity();
        
        Transform transform(Gnosis::GNVector2(curtainX, curtainY), 0.0f, 
                           Gnosis::GNVector2(curtainScale, curtainScale));
        
        Sprite sprite("curtains", 256.0f, 512.0f);
        sprite.layer = 2; // Behind game objects, in front of background
        sprite.visible = true;
        sprite.isAnimated = false;
        
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        
        // No hitbox - purely decorative
        m_ecsSystem->AddComponent<Transform>(curtain, transform);
        m_ecsSystem->AddComponent<Sprite>(curtain, sprite);
        m_ecsSystem->AddComponent<Physics>(curtain, physics);
        
        // NOTE: No Group component needed - LevelManager will add to manifest
        
        // Track for management
        m_activeObstacles.push_back(curtain);
        
        GN_LOG_INFO("Spawned castle curtain at x=" + std::to_string(curtainX) + " (scale=" + std::to_string(curtainScale) + ", centered on toilet at " + std::to_string(x) + ") with group " + std::to_string(groupId));
        
        return curtain;
    }

    Gnosis::Entity ObstacleSystem::SpawnCastleTorchPillar(float x, int groupId /* -1 = no group */, float offsetX) {
        // Create animated torch pillar (4-frame spritesheet, 96x512 frames)
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        float screenHeight = screenInfo.pixelHeight;
        float pillarHeight = 512.0f * m_baseScale; // Full height spritesheet
        
        // Position the torch pillar at the passed X position (TRUE center of gap)
        // X is already the center, but pillar sprite origin is top-left, so offset by half width
        float pillarWidth = 96.0f * m_baseScale;
        float pillarX = x - (pillarWidth * 0.5f); // Center horizontally (top-left anchor)
        float centerY = screenHeight / 2.0f; // Center vertically
        float pillarY = centerY - (pillarHeight / 2.0f); // Position pillar centered
        
        Gnosis::Entity torchPillar = m_ecsSystem->CreateEntity();
        
        Transform transform(Gnosis::GNVector2(pillarX, pillarY), 0.0f,
                           Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        // Animated sprite with 4 frames - properly configured for spritesheet
        Sprite sprite("TorchPillar", 96.0f, 512.0f);
        sprite.layer = 3; // In front of curtains (layer 2), behind player
        sprite.visible = true;
        sprite.isAnimated = true;
        sprite.frameCount = 4; // 4-frame animation
        sprite.frameWidth = 96; // Each frame is 96px wide
        sprite.frameHeight = 512; // Full height
        sprite.currentFrame = 0;
        sprite.currentFrameTime = 0.0f;
        sprite.frameTime = 0.15f; // Faster animation to avoid flickering
        sprite.playing = true;
        sprite.loop = true;
        
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        
        // No hitbox - purely decorative
        // No Obstacle component - purely decorative
        
        m_ecsSystem->AddComponent<Transform>(torchPillar, transform);
        m_ecsSystem->AddComponent<Sprite>(torchPillar, sprite);
        m_ecsSystem->AddComponent<Physics>(torchPillar, physics);
        
        // Only add to group if groupId is valid (not -1)
        // NOTE: No Group component needed - LevelManager will add to manifest
        
        // Track for management
        m_activeObstacles.push_back(torchPillar);
        
        GN_LOG_INFO("[CASTLE_PILLAR] Spawned at X=" + std::to_string(pillarX) + ", Y=" + std::to_string(centerY) + " (center=" + std::to_string(x) + ", pillarWidth=" + std::to_string(96.0f * m_baseScale) + "), group=" + std::to_string(groupId));
        
        return torchPillar;
    }

    Gnosis::Entity ObstacleSystem::SpawnCastleChandelier(float x, int groupId /* -1 = no group */, float offsetX) {
        // Create animated chandelier (32x34 sprite) - positioned to touch top of screen
        float chandelierY = 0.0f; // Touch top of screen
        
        Gnosis::Entity chandelier = m_ecsSystem->CreateEntity();
        
        Transform transform(Gnosis::GNVector2(x, chandelierY), 0.0f, 
                           Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        // Animated sprite
        Sprite sprite("castlelevelchandelier", 32.0f, 34.0f);
        sprite.layer = 3; // Above game objects
        sprite.visible = true;
        sprite.isAnimated = true;
        sprite.frameCount = 4; // Assume 4-frame animation
        sprite.frameWidth = 32; // Each frame is 32px wide
        sprite.frameHeight = 34; // Height
        sprite.currentFrame = 0;
        sprite.currentFrameTime = 0.0f;
        sprite.frameTime = 0.4f; // Animation speed
        sprite.playing = true;
        sprite.loop = true;
        
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        
        // No hitbox - purely decorative
        // No Obstacle component - purely decorative
        
        m_ecsSystem->AddComponent<Transform>(chandelier, transform);
        m_ecsSystem->AddComponent<Sprite>(chandelier, sprite);
        m_ecsSystem->AddComponent<Physics>(chandelier, physics);
        
        // Only add to group if groupId is valid (not -1)
        // NOTE: No Group component needed - LevelManager will add to manifest
        
        // Track for management
        m_activeObstacles.push_back(chandelier);
        
        GN_LOG_INFO("[CASTLE_CHANDELIER] Spawned at X=" + std::to_string(x) + ", Y=0 (top), width=" + std::to_string(32.0f * m_baseScale) + ", group=" + std::to_string(groupId));
        
        return chandelier;
    }

    Gnosis::Entity ObstacleSystem::SpawnCastleFloorTorch(float x, int groupId /* -1 = no group */, float offsetX) {
        // Create animated floor torch (20x64 sprite)
        float screenHeight = 2556.0f; // iPhone 16 portrait height
        float torchHeight = 64.0f * m_baseScale;
        float torchY = screenHeight - torchHeight; // Ground at bottom
        
        Gnosis::Entity floorTorch = m_ecsSystem->CreateEntity();
        
        Transform transform(Gnosis::GNVector2(x, torchY), 0.0f, 
                           Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        // Animated sprite
        Sprite sprite("castlelevelfloortorch", 20.0f, 64.0f);
        sprite.layer = 3; // Above game objects
        sprite.visible = true;
        sprite.isAnimated = true;
        sprite.frameCount = 4; // Assume 4-frame animation
        sprite.frameWidth = 20; // Each frame is 20px wide
        sprite.frameHeight = 64; // Height
        sprite.currentFrame = 0;
        sprite.currentFrameTime = 0.0f;
        sprite.frameTime = 0.3f; // Animation speed
        sprite.playing = true;
        sprite.loop = true;
        
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        
        // No hitbox - purely decorative
        // No Obstacle component - purely decorative
        
        m_ecsSystem->AddComponent<Transform>(floorTorch, transform);
        m_ecsSystem->AddComponent<Sprite>(floorTorch, sprite);
        m_ecsSystem->AddComponent<Physics>(floorTorch, physics);
        
        // Only add to group if groupId is valid (not -1)
        // NOTE: No Group component needed - LevelManager will add to manifest
        
        // Track for management
        m_activeObstacles.push_back(floorTorch);
        
        GN_LOG_INFO("[CASTLE_FLOORTORCH] Spawned at X=" + std::to_string(x) + ", Y=" + std::to_string(torchY) + ", group=" + std::to_string(groupId));
        
        return floorTorch;
    }

    Gnosis::Entity ObstacleSystem::SpawnCastleDecorativePainting(float x, int groupId /* -1 = no group */, float offsetX) {
        // Create decorative painting (96x96 sprite) - positioned in center between toilet groups
        float screenHeight = 2556.0f; // iPhone 16 portrait height
        float paintingWidth = 96.0f * 6.0f; // 6x scaling to make paintings prominent
        float paintingHeight = 96.0f * 6.0f; // 6x scaling to make paintings prominent
        float paintingY = (screenHeight - paintingHeight) / 2.0f; // Center vertically
        
        // X parameter is the CENTER position - calculate top-left for sprite positioning
        float paintingX = x - (paintingWidth / 2.0f);
        
        Gnosis::Entity painting = m_ecsSystem->CreateEntity();
        
        Transform transform(Gnosis::GNVector2(paintingX, paintingY), 0.0f, 
                           Gnosis::GNVector2(6.0f, 6.0f)); // 6x scaling to make paintings prominent
        
        // NEW: Randomize painting type on each spawn/wrap
        const char* paintingTextures[] = {
            "RabbitKnightPainting",  // DecorationType::PaintingRabbitKnight
            "RatBeachPainting",      // DecorationType::PaintingRatBeach
            "RiverWalkPainting",     // DecorationType::PaintingRiverWalk
            "CabinPainting"          // DecorationType::PaintingCabin
        };
        int randomIndex = rand() % 4;
        const char* selectedPainting = paintingTextures[randomIndex];
        
        // Static sprite (paintings don't animate)
        Sprite sprite(selectedPainting, 96.0f, 96.0f); // Use actual 96x96 dimensions
        sprite.layer = 2; // Behind game objects
        sprite.visible = true;
        sprite.isAnimated = false;
        
        Physics physics;
        physics.velocity.x = -m_worldSpeed;
        physics.useGravity = false;
        
        // No hitbox - purely decorative
        // No Obstacle component - purely decorative
        
        m_ecsSystem->AddComponent<Transform>(painting, transform);
        m_ecsSystem->AddComponent<Sprite>(painting, sprite);
        m_ecsSystem->AddComponent<Physics>(painting, physics);
        
        // NOTE: No Group component needed - LevelManager will add to manifest
        
        // Track for management
        m_activeObstacles.push_back(painting);
        
        GN_LOG_INFO("[CASTLE_PAINTING] Spawned CENTERED at X=" + std::to_string(x) + " (sprite top-left=" + std::to_string(paintingX) + 
                   ", width=" + std::to_string(paintingWidth) + "), Y=" + std::to_string(paintingY) + ", group=" + std::to_string(groupId));
        
        return painting;
    }
    
    Gnosis::Entity ObstacleSystem::SpawnCastleSpikeBall(float x, int groupId /* -1 = no group */, float offsetX) {
        // Create rotating spike ball obstacle with base - positioned in gap between toilet groups
        float screenHeight = 2556.0f; // iPhone 16 portrait height
        float spikeBallHeight = 90.0f * m_baseScale; // 90px spike ball height
        float baseHeight = 10.0f * m_baseScale; // Base is actually 10x10 pixels
        
        // Position the spike ball at the passed X position (TRUE center of gap)
        // X is already the center - position base so its CENTER is at X
        float centerY = screenHeight / 2.0f; // Vertical center
        float baseY = centerY; // Base at center Y
        
        // Base is 10x10, sprite origin is top-left
        // To center the base at X, offset top-left by half the base width
        float baseWidth = 10.0f * m_baseScale;
        float baseTopLeftX = x - (baseWidth * 0.5f); // Top-left position to center base at X
        float baseCenterX = x; // The actual center position we want
        float spikeBallX = baseCenterX; 
        float spikeBallY = baseY;
        GN_LOG_INFO("Spike ball positioning - screenHeight: " + std::to_string(screenHeight) + 
                   ", centerY: " + std::to_string(centerY) + 
                   ", baseY: " + std::to_string(baseY) + 
                   ", spikeBallY: " + std::to_string(spikeBallY) + 
                   ", baseCenterX: " + std::to_string(baseCenterX) + 
                   ", spikeBallX: " + std::to_string(spikeBallX) + 
                   ", m_baseScale: " + std::to_string(m_baseScale) + 
                   ", base sprite dimensions: 10x10, spike ball sprite dimensions: 64x90");
        
        // Create the base first
        Gnosis::Entity base = m_ecsSystem->CreateEntity();
        
        // Base sprite origin is top-left, so we need to position it half-width and half-height offset
        float baseTopLeftY = baseY - (5.0f * m_baseScale);       // Half height offset for vertical centering
        Transform baseTransform(Gnosis::GNVector2(baseTopLeftX, baseTopLeftY), 0.0f, 
                              Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite baseSprite("SpikeBallBase", 10.0f, 10.0f); // Base is actually 10x10 pixels
        baseSprite.layer = 2; // Behind game objects
        baseSprite.visible = true;
        baseSprite.isAnimated = false;
        
        Physics basePhysics;
        basePhysics.velocity.x = -m_worldSpeed;
        basePhysics.useGravity = false;
        
        // Base has no hitbox - just decorative
        m_ecsSystem->AddComponent<Transform>(base, baseTransform);
        m_ecsSystem->AddComponent<Sprite>(base, baseSprite);
        m_ecsSystem->AddComponent<Physics>(base, basePhysics);
        
        GN_LOG_INFO("DEBUG TEST: Base top-left at (" + std::to_string(baseTopLeftX) + ", " + std::to_string(baseTopLeftY) + ")");
        GN_LOG_INFO("DEBUG TEST: Base center should be at (" + std::to_string(baseCenterX) + ", " + std::to_string(baseY) + ")");
        GN_LOG_INFO("Created base entity at position (" + std::to_string(baseTransform.position.x) + ", " + std::to_string(baseTransform.position.y) + ") with scale " + std::to_string(baseTransform.scale.x));
        
        // Now create the rotating spike ball
        Gnosis::Entity spikeBall = m_ecsSystem->CreateEntity();
        
        Transform spikeBallTransform(Gnosis::GNVector2(spikeBallX, spikeBallY), 0.0f, 
                                   Gnosis::GNVector2(m_baseScale, m_baseScale));
        
        Sprite spikeBallSprite("SpikeBall", 64.0f, 90.0f); // Use actual 64x90 dimensions
        spikeBallSprite.layer = 3; // Above game objects
        spikeBallSprite.visible = true;
        spikeBallSprite.isAnimated = false;
        
        Physics spikeBallPhysics;
        spikeBallPhysics.velocity.x = -m_worldSpeed;
        spikeBallPhysics.useGravity = false;
        
        // Spike ball has hitbox and obstacle component for damage
        Hitbox hitbox;
        hitbox.type = ColliderType::Circle; // Use circular hitbox to match spike ball shape
        hitbox.radius = 1.0f * m_baseScale; // 1px radius = 8px diameter when scaled (actual spike ball collision size)
        
        // Set hitbox to rotate around base center using relative offset from spikeball
        // Initial position: spikeball is 70 pixels above base, so hitbox needs -70Y offset to reach base
        float hitboxPivotX = 0.0f;   // No horizontal offset (centered)
        float hitboxPivotY = -70.0f; // 70 pixels down from spikeball to base center
        
        // Apply initial rotation with 180-degree phase shift (entity starts at some rotation angle)
        float initialRotation = 0.0f; // Will be set by the update function immediately
        float rotationRadians = (initialRotation + 180.0f) * (M_PI / 180.0f);
        float rotatedOffsetX = hitboxPivotX * cos(rotationRadians) + hitboxPivotY * sin(rotationRadians);
        float rotatedOffsetY = -hitboxPivotX * sin(rotationRadians) + hitboxPivotY * cos(rotationRadians);
        
        hitbox.offsetX = rotatedOffsetX;  // Initial rotated offset
        hitbox.offsetY = rotatedOffsetY;  // Initial rotated offset
        
        GN_LOG_INFO("SpikeBall created with rotating hitbox: initial_offset=(" + std::to_string(rotatedOffsetX) + ", " + std::to_string(rotatedOffsetY) + ")");
        
        Obstacle obstacle;
        obstacle.type = ObstacleType::SpikeBall;  // Castle level rotating obstacle
        obstacle.damage = 1;
        obstacle.isDestructible = false;
        obstacle.behavior = 0; // STATIC = 0 (no oscillation)
        obstacle.oscillationSpeed = 0.0f; // No oscillation
        obstacle.oscillationRange = 0.0f; // No oscillation
        obstacle.oscillationTimer = 0.0f; // No oscillation
        obstacle.basePosition = Gnosis::GNVector2(spikeBallX, spikeBallY); // Store original position
        
        m_ecsSystem->AddComponent<Transform>(spikeBall, spikeBallTransform);
        m_ecsSystem->AddComponent<Sprite>(spikeBall, spikeBallSprite);
        m_ecsSystem->AddComponent<Physics>(spikeBall, spikeBallPhysics);
        m_ecsSystem->AddComponent<Hitbox>(spikeBall, hitbox);
        m_ecsSystem->AddComponent<Obstacle>(spikeBall, obstacle);
        
        // Debug drawing for spike ball hitbox disabled (RotSprite working perfectly!)
        // DebugDraw debugDraw;
        // debugDraw.showBounds = true;  
        // debugDraw.showCollider = true;
        // debugDraw.colliderColor = Gnosis::GNColor(255, 255, 0, 255); // Yellow for spike ball
        // debugDraw.boundsColor = Gnosis::GNColor(0, 255, 0, 255);     // Green for bounds
        // debugDraw.alpha = 0.8f;
        // debugDraw.debugLayer = 18; 
        // m_ecsSystem->AddComponent<DebugDraw>(spikeBall, debugDraw);
        GN_LOG_INFO("Spike ball created without debug hitboxes (RotSprite system working perfectly!)");
        
        // Verify all components were added correctly
        GN_LOG_INFO("DEBUG: Spike ball " + std::to_string(spikeBall) + " components:");
        GN_LOG_INFO("  - Transform: " + std::string(m_ecsSystem->HasComponent<Transform>(spikeBall) ? "YES" : "NO"));
        GN_LOG_INFO("  - Sprite: " + std::string(m_ecsSystem->HasComponent<Sprite>(spikeBall) ? "YES" : "NO"));
        GN_LOG_INFO("  - Hitbox: " + std::string(m_ecsSystem->HasComponent<Hitbox>(spikeBall) ? "YES" : "NO"));
        GN_LOG_INFO("  - Obstacle: " + std::string(m_ecsSystem->HasComponent<Obstacle>(spikeBall) ? "YES" : "NO"));
        GN_LOG_INFO("  - DebugDraw: DISABLED (No longer needed - RotSprite perfected!)");
        GN_LOG_INFO("  - PivotRotationRenderer: " + std::string(m_ecsSystem->HasComponent<PivotRotationRenderer>(spikeBall) ? "YES" : "NO"));
        
        GN_LOG_INFO("DEBUG TEST: Spike ball center at (" + std::to_string(spikeBallX) + ", " + std::to_string(spikeBallY) + ")");
        GN_LOG_INFO("DEBUG TEST: Both base center and spike ball center should be at SAME coordinates!");
        
        // Calculate and log actual hitbox world position
        float hitboxWorldX = spikeBallX + hitbox.offsetX;  // offsetX is already scaled
        float hitboxWorldY = spikeBallY + hitbox.offsetY;  // offsetY is already scaled
        GN_LOG_INFO("DEBUG HITBOX: Spike ball hitbox center at (" + std::to_string(hitboxWorldX) + ", " + std::to_string(hitboxWorldY) + ")");
        GN_LOG_INFO("DEBUG HITBOX: Hitbox radius: " + std::to_string(hitbox.radius) + " (should be 64 scaled pixels)");
        
        GN_LOG_INFO("Created spike ball entity at position (" + std::to_string(spikeBallTransform.position.x) + ", " + std::to_string(spikeBallTransform.position.y) + ") with scale " + std::to_string(spikeBallTransform.scale.x));
        
                    // Add pivot rotation renderer for spinning animation around base center
            // The spikeball sprite is 64x90, chain connection is at (32,0) in sprite coordinates
                    // Sprite center is at (32,45), so pivot offset from center is (0, -45)  
        // This positions the chain connection (top of sprite) at the base center
        PivotRotationRenderer pivotRenderer(true, 0.0f, -45.0f, 180.0f);
        
        // CRITICAL: Add component and verify it was added
        m_ecsSystem->AddComponent<PivotRotationRenderer>(spikeBall, pivotRenderer);
        
        // Verify the component was actually added
        if (m_ecsSystem->HasComponent<PivotRotationRenderer>(spikeBall)) {
            GN_LOG_INFO("SUCCESS: PivotRotationRenderer added to spike ball " + std::to_string(spikeBall));
        } else {
            GN_LOG_ERROR("CRITICAL ERROR: PivotRotationRenderer component missing from spike ball " + std::to_string(spikeBall));
        }
        
        // Remove the old rotation renderer since we're using pivot-based rotation now
        // RotationRenderer rotationRenderer(true);
        // m_ecsSystem->AddComponent<RotationRenderer>(spikeBall, rotationRenderer);
        
        // Add both to group for management - different offsetX values and patterns
        float groupWidth = 64.0f * m_baseScale; // Use spike ball width for group
        // NOTE: No Group components needed - LevelManager will add to manifest
        
        // Track for management
        m_activeObstacles.push_back(base);
        m_activeObstacles.push_back(spikeBall);
        
        // Store rotation speed for animation (will be updated in UpdateObstacles)
        m_spikeBallRotationSpeeds[spikeBall] = 180.0f; // Slower: 180 degrees per second (1/2 rotation per second)
        
        // Store the relationship between spike ball and base for hitbox positioning
        m_spikeBallToBase[spikeBall] = base;
        
        GN_LOG_INFO("Spawned castle spike ball obstacle CENTERED at x=" + std::to_string(x) + " y=" + std::to_string(centerY) + " with group " + std::to_string(groupId));
        
        return spikeBall; // Return the spike ball (obstacle), not the base (decorative)
    }

    void ObstacleSystem::UpdateSpikeBallRotations(float deltaTime) {
        // Update rotation for all spike balls using PivotRotationRenderer
        for (auto it = m_spikeBallRotationSpeeds.begin(); it != m_spikeBallRotationSpeeds.end(); ++it) {
            Gnosis::Entity entity = it->first;
            float rotationSpeed = it->second;
            Transform* transform = m_ecsSystem->GetComponent<Transform>(entity);
            PivotRotationRenderer* pivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(entity);
            Hitbox* hitbox = m_ecsSystem->GetComponent<Hitbox>(entity);
            
            if (transform && pivotRenderer && hitbox) {
                // Use the rotation speed from the PivotRotationRenderer component
                float speed = pivotRenderer->rotationSpeed;
                
                // Convert rotation speed from degrees per second to degrees per frame
                float rotationDelta = speed * deltaTime;
                float oldRotation = transform->rotation;
                transform->rotation += rotationDelta;
                
                // Keep rotation in reasonable bounds (like loading guy does)
                if (transform->rotation >= 360.0f) {
                    transform->rotation -= 360.0f;
                }
                
                // Get the actual base position from the stored relationship
                auto baseIt = m_spikeBallToBase.find(entity);
                if (baseIt != m_spikeBallToBase.end()) {
                    Gnosis::Entity baseEntity = baseIt->second;
                    Transform* baseTransform = m_ecsSystem->GetComponent<Transform>(baseEntity);
                    
                    if (baseTransform) {
                        // Base center is our origin (0, 0) for the circle
                        // Base Transform.position is top-left, so we need to calculate the center
                        float baseCenterX = baseTransform->position.x + (5.0f * baseTransform->scale.x);
                        float baseCenterY = baseTransform->position.y + (5.0f * baseTransform->scale.y);
                        
                        // REFACTORED APPROACH: Use absolute positioning mode with rotation
                        // Calculate where the hitbox should be positioned relative to the spikeball center
                        // The spikeball rotates around the base, so we need to calculate the inverse offset
                        
                        // Spikeball is 70 pixels above the base center (in sprite coordinates)
                        // Hitbox should be 70 pixels below the spikeball center to reach the base
                        float pivotOffsetX = 0.0f;   // No horizontal offset (centered)
                        float pivotOffsetY = -70.0f; // 70 pixels down from spikeball to base center
                        
                        // Apply rotation to the offset vector with 180-degree phase shift
                        float rotationRadians = (transform->rotation + 180.0f) * (M_PI / 180.0f);
                        float rotatedOffsetX = pivotOffsetX * cos(rotationRadians) + pivotOffsetY * sin(rotationRadians);
                        float rotatedOffsetY = -pivotOffsetX * sin(rotationRadians) + pivotOffsetY * cos(rotationRadians);
                        
                        hitbox->offsetX = rotatedOffsetX;  // Rotated offset from spikeball to hitbox
                        hitbox->offsetY = rotatedOffsetY;  // Rotated offset from spikeball to hitbox
                        
                        // === COMPREHENSIVE COORDINATE DEBUGGING ===
                        GN_LOG_INFO("=== SPIKEBALL COORDINATE ANALYSIS ===");

                        // 1. Log actual base transform details
                        GN_LOG_INFO("BASE TRANSFORM: position=(" + std::to_string(baseTransform->position.x) + ", " + std::to_string(baseTransform->position.y) + "), scale=(" + std::to_string(baseTransform->scale.x) + ", " + std::to_string(baseTransform->scale.y) + ")");

                        // 2. Calculate what SHOULD be the center if position is top-left
                        float calculatedCenterX = baseTransform->position.x + (5.0f * baseTransform->scale.x);
                        float calculatedCenterY = baseTransform->position.y + (5.0f * baseTransform->scale.y);
                        GN_LOG_INFO("CALCULATED CENTER (if top-left): (" + std::to_string(calculatedCenterX) + ", " + std::to_string(calculatedCenterY) + ")");

                        // 3. Log what we're currently using as center
                        GN_LOG_INFO("CURRENT CENTER (what code uses): (" + std::to_string(baseCenterX) + ", " + std::to_string(baseCenterY) + ")");

                        // 4. Log spike ball transform details
                        GN_LOG_INFO("SPIKEBALL TRANSFORM: position=(" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + "), scale=(" + std::to_string(transform->scale.x) + ", " + std::to_string(transform->scale.y) + "), rotation=" + std::to_string(transform->rotation));

                        // 5. Log PivotRotationRenderer details
                        GN_LOG_INFO("PIVOT RENDERER: pivotX=" + std::to_string(pivotRenderer->pivotX) + ", pivotY=" + std::to_string(pivotRenderer->pivotY) + ", speed=" + std::to_string(pivotRenderer->rotationSpeed));

                        // 6. Log the calculated visual pivot point (where SpikeBall should visually rotate)
                        float visualPivotX = transform->position.x + (pivotRenderer->pivotX * transform->scale.x);
                        float visualPivotY = transform->position.y + (pivotRenderer->pivotY * transform->scale.y);
                        GN_LOG_INFO("VISUAL PIVOT POINT: (" + std::to_string(visualPivotX) + ", " + std::to_string(visualPivotY) + ")");

                        // 7. Log hitbox calculation step by step
                        GN_LOG_INFO("HITBOX CALCULATION (WITH ROTATION + 180° SHIFT):");
                        GN_LOG_INFO("  - Spikeball rotation: " + std::to_string(transform->rotation) + "° (+ 180° phase shift)");
                        GN_LOG_INFO("  - Base pivot offset: (" + std::to_string(pivotOffsetX) + ", " + std::to_string(pivotOffsetY) + ")");
                        GN_LOG_INFO("  - Rotated hitbox offset: (" + std::to_string(rotatedOffsetX) + ", " + std::to_string(rotatedOffsetY) + ")");
                        GN_LOG_INFO("  - Base center: (" + std::to_string(baseCenterX) + ", " + std::to_string(baseCenterY) + ")");
                        GN_LOG_INFO("  - Using absolute positioning mode in renderer");
                    }
                }
                
                // Debug logging for hitbox positioning
                // Calculate the actual world position where the hitbox will be rendered
                // This should be base center + hitbox offset, since we're calculating the hitbox purely from base coordinates
                if (baseIt != m_spikeBallToBase.end()) {
                    Transform* baseTransform = m_ecsSystem->GetComponent<Transform>(baseIt->second);
                    if (baseTransform) {
                        // Calculate the actual base center (not top-left position)
                        float baseCenterX = baseTransform->position.x + (5.0f * baseTransform->scale.x);
                        float baseCenterY = baseTransform->position.y + (5.0f * baseTransform->scale.y);
                        
                        // Hitbox position in absolute mode is spikeball position + offset
                        float hitboxWorldX = transform->position.x + hitbox->offsetX;
                        float hitboxWorldY = transform->position.y + hitbox->offsetY;
                        
                        // 8. Log final hitbox world position
                        GN_LOG_INFO("FINAL HITBOX WORLD POS: (" + std::to_string(hitboxWorldX) + ", " + std::to_string(hitboxWorldY) + ")");
                        GN_LOG_INFO("HITBOX SHOULD NOW ROTATE AROUND BASE CENTER");

                        // 9. Calculate distance from base to hitbox (should be 0)
                        float distanceFromBaseToHitbox = sqrt(pow(hitboxWorldX - baseCenterX, 2) + pow(hitboxWorldY - baseCenterY, 2));
                        GN_LOG_INFO("DISTANCE BASE-TO-HITBOX: " + std::to_string(distanceFromBaseToHitbox));
                        GN_LOG_INFO("EXPECTED DISTANCE: 0.0 pixels (hitbox at base center)");
                        
                        // Also calculate distance from visual pivot (need to recalculate since we're in different scope)
                        Transform* spikeBallTransform = m_ecsSystem->GetComponent<Transform>(entity);
                        PivotRotationRenderer* spikeBallPivot = m_ecsSystem->GetComponent<PivotRotationRenderer>(entity);
                        if (spikeBallTransform && spikeBallPivot) {
                            float visualPivotX = spikeBallTransform->position.x + (spikeBallPivot->pivotX * spikeBallTransform->scale.x);
                            float visualPivotY = spikeBallTransform->position.y + (spikeBallPivot->pivotY * spikeBallTransform->scale.y);
                            float distanceFromVisualPivotToHitbox = sqrt(pow(hitboxWorldX - visualPivotX, 2) + pow(hitboxWorldY - visualPivotY, 2));
                            GN_LOG_INFO("DISTANCE VISUAL-PIVOT-TO-HITBOX: " + std::to_string(distanceFromVisualPivotToHitbox));
                        }

                        GN_LOG_INFO("=== END COORDINATE ANALYSIS ===");
                    } else {
                        GN_LOG_INFO("SpikeBall Hitbox: Base transform not found for entity " + std::to_string(entity));
                    }
                } else {
                    GN_LOG_INFO("SpikeBall Hitbox: No base relationship found for entity " + std::to_string(entity));
                }
            }
        }
    }

    Gnosis::Entity ObstacleSystem::GetSpikeBallBaseEntity(Gnosis::Entity spikeBallEntity) const {
        auto it = m_spikeBallToBase.find(spikeBallEntity);
        if (it != m_spikeBallToBase.end()) {
            return it->second;
        }
        return 0; // Return 0 if not found (invalid entity)
    }

void ObstacleSystem::AddBossLevelDecorations() {
    // Create the animated pillar in the center of the boss level
    // The pillar is 320x180 pixels with 15-frame animation (landscape mode)
    // Scale to fit landscape dimensions with width priority
    float screenWidth = 2556.0f; // Landscape width (portrait height becomes landscape width)
    float pillarScale = screenWidth / 320.0f; // Scale to fit 320px base width to full screen width
    float pillarWidth = 320.0f * pillarScale;
    float pillarX = (screenWidth - pillarWidth) / 2.0f; // Center horizontally
    float pillarY = 0.0f; // Start at y=0 as requested

    GN_LOG_INFO("Creating boss level pillar with 15-frame animation at x=" + std::to_string(pillarX) + ", y=" + std::to_string(pillarY) + ", scale=" + std::to_string(pillarScale));

    // Create pillar entity with proper 15-frame animation setup
    Gnosis::Entity pillar = m_ecsSystem->CreateEntity();

    Transform pillarTransform(Gnosis::GNVector2(pillarX, pillarY), 0.0f,
                             Gnosis::GNVector2(pillarScale, pillarScale));
    m_ecsSystem->AddComponent<Transform>(pillar, pillarTransform);

    // Create sprite for 15-frame pillar animation
    Sprite pillarSprite("bosspillar", 320.0f, 180.0f); // Individual frame dimensions (320x180)
    pillarSprite.layer = 2; // Background layer (between boss walls and screen curtains)
    pillarSprite.visible = true;

    // Set up animation for pillar (15-frame animation)
    pillarSprite.isAnimated = true;
    pillarSprite.frameCount = 15; // 15-frame animation
    pillarSprite.frameWidth = static_cast<int>(320.0f); // Each frame is 320 pixels wide
    pillarSprite.frameHeight = static_cast<int>(180.0f); // Each frame is 180 pixels tall
    pillarSprite.currentFrame = 0;
    pillarSprite.currentFrameTime = 0.0f; // Initialize frame time
    pillarSprite.frameTime = 0.12f; // Animation speed (slightly faster than old 7-frame)
    pillarSprite.playing = true;
    pillarSprite.loop = true;

    GN_LOG_DEBUG("Created boss pillar: bosspillar with " + std::to_string(pillarSprite.frameCount) + " frames, frameWidth=" + std::to_string(pillarSprite.frameWidth) + ", frameHeight=" + std::to_string(pillarSprite.frameHeight));

    m_ecsSystem->AddComponent<Sprite>(pillar, pillarSprite);

    // Physics (static, no movement for boss level pillar)
    Physics pillarPhysics;
    pillarPhysics.velocity.x = 0.0f; // Static - no movement like dancing cacti
    pillarPhysics.velocity.y = 0.0f; // No vertical movement
    pillarPhysics.useGravity = false;
    pillarPhysics.mass = 0.0f; // Static object
    pillarPhysics.drag = 1.0f; // No drag for static object
    m_ecsSystem->AddComponent<Physics>(pillar, pillarPhysics);

    // No hitbox for decorative pillar - it should not interact with anything

    // NOTE: Boss level pillars are purely decorative - no Obstacle component needed
    // They have no collision, no damage, and no gameplay interaction
    // Removed Obstacle component as pillars are not gameplay obstacles

    if (pillar != 0) {
        m_activeObstacles.push_back(pillar);

        // Remove any DebugDraw components that might have been added automatically
        if (m_ecsSystem->HasComponent<DebugDraw>(pillar)) {
            m_ecsSystem->RemoveComponent<DebugDraw>(pillar);
            GN_LOG_INFO("Removed DebugDraw component from boss pillar entity " + std::to_string(pillar));
        }

        // Verify the sprite was created correctly
        Sprite* pillarSprite = m_ecsSystem->GetComponent<Sprite>(pillar);
        if (pillarSprite) {
            GN_LOG_INFO("Boss level pillar created successfully - entity=" + std::to_string(pillar) +
                       ", textureId=" + pillarSprite->textureId +
                       ", visible=" + std::to_string(pillarSprite->visible) +
                       ", isAnimated=" + std::to_string(pillarSprite->isAnimated) +
                       ", frameCount=" + std::to_string(pillarSprite->frameCount));
        } else {
            GN_LOG_ERROR("Boss level pillar sprite component not found after creation!");
        }

        GN_LOG_INFO("Boss level pillar created at x=" + std::to_string(pillarX) + ", y=" + std::to_string(pillarY) +
                   " with 7-frame animation and scale " + std::to_string(pillarScale));
    } else {
        GN_LOG_ERROR("Failed to create boss level pillar entity!");
    }
}

} // namespace GameCore
