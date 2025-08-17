#include "ObstacleSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace GameCore {

    ObstacleSystem::ObstacleSystem(Gnosis::ECS* ecsSystem) 
        : m_ecsSystem(ecsSystem), m_initialized(false), m_currentLevelId(0), m_nextGroupId(1), m_baseScale(1.0f) {
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
        m_obstacleGroups.clear();
        m_wrappedGroups.clear();
        
        // Set level-specific parameters
        m_baseScale = config.baseScale;
        m_worldSpeed = config.worldSpeed;

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
                
            default:
                m_levelPatterns.push_back({PatternType::PARK_TOILET_PAIR, 1.0f, 0.0f});
                break;
        }

        // Create initial obstacle pool
        float nextWorldX = SCREEN_WIDTH + 100.0f;
        for (int i = 0; i < OBSTACLE_POOL_SIZE; i++) {
            int groupIdBefore = m_nextGroupId;
            
            SpawnRandomPatternForLevel(levelId, nextWorldX);
            
            // Calculate next position from group width
            float groupWidth = CalculateGroupWidth(groupIdBefore);
            if (groupWidth <= 0.0f) {
                groupWidth = 600.0f; // Fallback
            }
            nextWorldX += groupWidth;
        }

        m_initialized = true;
        GN_LOG_INFO("ObstacleSystem initialized with " + std::to_string(OBSTACLE_POOL_SIZE) + " obstacle groups");
    }

    void ObstacleSystem::Update(float deltaTime, float worldScrollDistance) {
        if (!m_initialized || m_activeObstacles.empty()) {
            return;
        }
        
        // Check if any group leader is off-screen and needs wrapping
        std::unordered_map<int, Gnosis::Entity> groupLeaders;
        for (Gnosis::Entity e : m_activeObstacles) {
            if (!m_ecsSystem->HasComponent<Group>(e) || !m_ecsSystem->HasComponent<Transform>(e) || !m_ecsSystem->HasComponent<Sprite>(e)) {
                continue;
            }
            
            auto* group = m_ecsSystem->GetComponent<Group>(e);
            auto* transform = m_ecsSystem->GetComponent<Transform>(e);
            auto* sprite = m_ecsSystem->GetComponent<Sprite>(e);
            
            if (group->isLeader) {
                groupLeaders[group->id] = e;
                
                // Check if group is off-screen using the actual group's rightmost member
                float lastMemberRight = transform->position.x + sprite->width * std::abs(transform->scale.x);
                for (Gnosis::Entity member : m_activeObstacles) {
                    auto* memberGroup = m_ecsSystem->GetComponent<Group>(member);
                    if (memberGroup && memberGroup->id == group->id) {
                        auto* memberTransform = m_ecsSystem->GetComponent<Transform>(member);
                        auto* memberSprite = m_ecsSystem->GetComponent<Sprite>(member);
                        if (memberTransform && memberSprite) {
                            float memberRight = memberTransform->position.x + memberSprite->width * std::abs(memberTransform->scale.x);
                            if (memberRight > lastMemberRight) {
                                lastMemberRight = memberRight;
                            }
                        }
                    }
                }
                
                if (lastMemberRight < 0.0f) {
                    WrapGroupAroundScreen(group->id, worldScrollDistance);
                }
            }
        }
        
        // Handle wrapped groups (for coin system coordination)
        // Note: Don't clear wrapped groups here - let PickupSystem consume them
        auto wrappedGroups = m_wrappedGroups; // Copy instead of clear
        for (int groupId : wrappedGroups) {
            WrapGroup(groupId, worldScrollDistance);
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

        m_activeObstacles.clear();
        m_obstacleGroups.clear();
        m_wrappedGroups.clear();
        m_levelPatterns.clear();
        
        m_nextGroupId = 1;
        m_initialized = false;
        
        GN_LOG_INFO("ObstacleSystem cleaned up");
    }

    void ObstacleSystem::SpawnPattern(PatternType type, float x) {
        switch (type) {
            case PatternType::PARK_TOILET_PAIR:
                SpawnParkPattern_ToiletPair(x);
                break;
            case PatternType::DESERT_OUTHOUSE:
                SpawnDesertPattern_Outhouse(x);
                break;
            // Temporarily removed cacti obstacles
            // case PatternType::DESERT_CACTUS:
            //     SpawnDesertPattern_Cactus(x);
            //     break;
            case PatternType::SEWER_TOP_ONLY:
                SpawnSewerPattern_TopOnly(x);
                break;
            case PatternType::SEWER_BOTTOM_ONLY:
                SpawnSewerPattern_BottomOnly(x);
                break;
            case PatternType::SEWER_TOP_AND_BOTTOM:
                SpawnSewerPattern_TopAndBottom(x);
                break;
            case PatternType::SEWER_PYRAMID_3:
                SpawnSewerPattern_Pyramid3(x);
                break;
            case PatternType::SEWER_PYRAMID_TOP_3:
                SpawnSewerPattern_PyramidTop3(x);
                break;
            case PatternType::SEWER_TWO_BY_TWO_FUNNEL:
                SpawnSewerPattern_TwoByTwoFunnel(x);
                break;
        }
    }

    void ObstacleSystem::SpawnRandomPatternForLevel(int levelId, float x) {
        if (m_levelPatterns.empty()) {
            GN_LOG_ERROR("No patterns configured for level " + std::to_string(levelId));
            return;
        }

        // Simple random selection (can be enhanced with weights later)
        int patternIndex = rand() % m_levelPatterns.size();
        SpawnPattern(m_levelPatterns[patternIndex].type, x);
    }

    // ============================================================================
    // Pattern Implementations
    // ============================================================================

    void ObstacleSystem::SpawnParkPattern_ToiletPair(float x) {
        // Use exact toilet positioning logic from old LevelManager SpawnToiletPairWithGap
        float screenHeight = 2556.0f; // iPhone 16 portrait height
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
        
        int groupId = m_nextGroupId++;
        
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
        // Center-based: trim 30px from bottom; offset center up by 15 so collider top aligns with sprite top
        const float TRIM_TOP = 30.0f;
        topCollider.width = 20.0f;
        topCollider.height = 190.0f - TRIM_TOP;
        topCollider.offsetX = 0.0f;
        topCollider.offsetY = -(TRIM_TOP * 0.5f);
        topCollider.isStatic = false;
        topCollider.isTrigger = false;
        topCollider.tag = "obstacle";
        
        Obstacle topObstacle;
        topObstacle.obstacleType = "TopToilet";
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
        // Center-based: start 30px down; offset center down by 15 so collider top = sprite top + 30
        const float TRIM_BOTTOM = 30.0f;
        bottomCollider.width = 20.0f;
        bottomCollider.height = 190.0f - TRIM_BOTTOM;
        bottomCollider.offsetX = 0.0f;
        bottomCollider.offsetY = +(TRIM_BOTTOM * 0.5f);
        bottomCollider.isStatic = false;
        bottomCollider.isTrigger = false;
        bottomCollider.tag = "obstacle";
        
        Obstacle bottomObstacle;
        bottomObstacle.obstacleType = "BottomToilet";
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
        
        // Add to group management - balanced horizontal spacing (happy medium between 65 and 130)
        AddEntityToGroup(topToilet, groupId, true, 0.0f, 0.0f, 95.0f * m_baseScale, GroupPattern::TopAndBottom);
        AddEntityToGroup(bottomToilet, groupId, false, 0.0f, bottomToiletY - topToiletY, 95.0f * m_baseScale, GroupPattern::TopAndBottom);
        
        // Add to tracking
        m_activeObstacles.push_back(topToilet);
        m_activeObstacles.push_back(bottomToilet);
        m_obstacleGroups[groupId] = {topToilet, bottomToilet};
        
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
    }


    void ObstacleSystem::SpawnDesertPattern_Outhouse(float x) {
        const float outhouseW = 90.0f;
        const float outhouseH = 160.0f;
        const float groundY = SCREEN_HEIGHT - (outhouseH * m_baseScale);
        
        int groupId = m_nextGroupId++;
        
        // Create outhouse entity (solid collision)
        Gnosis::Entity outhouse = CreateOuthouseEntity("Outhouse", x, groundY, m_baseScale, true);
        AddEntityToGroup(outhouse, groupId, true, 0.0f, 0.0f, (outhouseW * 2.0f) * m_baseScale, GroupPattern::Ground);
        
        // Create toilet entity (trigger only)
        Gnosis::Entity toilet = CreateOuthouseEntity("OuthouseToilet", x, groundY, m_baseScale, false);
        AddEntityToGroup(toilet, groupId, false, 0.0f, 0.0f, (outhouseW * 2.0f) * m_baseScale, GroupPattern::Ground);
        
        // Link entities
        LinkOuthousePair(outhouse, toilet);
        
        // Add to tracking
        m_activeObstacles.push_back(outhouse);
        m_activeObstacles.push_back(toilet);
        m_obstacleGroups[groupId] = {outhouse, toilet};
        
        GN_LOG_DEBUG("Spawned desert outhouse at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
        GN_LOG_DEBUG("Outhouse hitbox: solid collision, width=" + std::to_string((90.0f - 16.0f) * m_baseScale) + 
                     ", height=" + std::to_string((160.0f - 64.0f) * m_baseScale) + 
                     ", offsetX=0, offsetY=" + std::to_string(64.0f * m_baseScale));
        GN_LOG_DEBUG("Toilet hitbox: trigger only, width=" + std::to_string(((90.0f / 2) - 8.0f) * m_baseScale) + 
                     ", height=" + std::to_string(160.0f * m_baseScale) + 
                     ", offsetX=" + std::to_string((90.0f / 4 + 4.0f) * m_baseScale) + ", offsetY=0");
    }

    void ObstacleSystem::SpawnDesertPattern_Cactus(float x) {
        const std::vector<std::string> cactiTypes = {"CactiA", "CactiB", "CactiC"};
        const std::string cactusType = cactiTypes[rand() % cactiTypes.size()];
        
        const float cactusW = 64.0f;
        const float cactusH = 48.0f;
        const float groundY = SCREEN_HEIGHT - (cactusH * m_baseScale);
        
        int groupId = m_nextGroupId++;
        
        Gnosis::Entity cactus = CreateCactusEntity(cactusType, x, groundY, m_baseScale);
        AddEntityToGroup(cactus, groupId, true, 0.0f, 0.0f, cactusW * m_baseScale, GroupPattern::Ground);
        
        // Add to tracking
        m_activeObstacles.push_back(cactus);
        m_obstacleGroups[groupId] = {cactus};
        
        GN_LOG_DEBUG("Spawned desert cactus (" + cactusType + ") at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
    }

    void ObstacleSystem::SpawnSewerPattern_TopOnly(float x) {
        // Match old LevelManager's exact TopOnly logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float topY = 0.0f;
        
        int groupId = m_nextGroupId++;
        
        const std::string texture = (rand() % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
        Gnosis::Entity pipe = CreateSewerPipeEntity(texture, x, topY, m_baseScale, true);
        AddEntityToGroup(pipe, groupId, true, 0.0f, 0.0f, pipeW * m_baseScale, GroupPattern::TopOnly);
        
        m_activeObstacles.push_back(pipe);
        m_obstacleGroups[groupId] = {pipe};
        
        GN_LOG_DEBUG("Spawned sewer top-only pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
    }

    void ObstacleSystem::SpawnSewerPattern_BottomOnly(float x) {
        // Match old LevelManager's exact BottomOnly logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float bottomY = SCREEN_HEIGHT - (pipeH * m_baseScale);
        
        int groupId = m_nextGroupId++;
        
        const std::string texture = (rand() % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
        Gnosis::Entity pipe = CreateSewerPipeEntity(texture, x, bottomY, m_baseScale, false);
        AddEntityToGroup(pipe, groupId, true, 0.0f, 0.0f, pipeW * m_baseScale, GroupPattern::BottomOnly);
        
        m_activeObstacles.push_back(pipe);
        m_obstacleGroups[groupId] = {pipe};
        
        GN_LOG_DEBUG("Spawned sewer bottom-only pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
    }

    void ObstacleSystem::SpawnSewerPattern_TopAndBottom(float x) {
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float topY = 0.0f;
        const float bottomY = SCREEN_HEIGHT - (pipeH * m_baseScale);
        
        int groupId = m_nextGroupId++;
        
        // Create top pipe
        const std::string topTexture = (rand() % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
        Gnosis::Entity topPipe = CreateSewerPipeEntity(topTexture, x, topY, m_baseScale, true);
        AddEntityToGroup(topPipe, groupId, true, 0.0f, 0.0f, pipeW * m_baseScale, GroupPattern::TopAndBottom);
        
        // Create bottom pipe
        const std::string bottomTexture = (rand() % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
        Gnosis::Entity bottomPipe = CreateSewerPipeEntity(bottomTexture, x, bottomY, m_baseScale, false);
        AddEntityToGroup(bottomPipe, groupId, false, 0.0f, bottomY - topY, pipeW * m_baseScale, GroupPattern::TopAndBottom);
        
        m_activeObstacles.push_back(topPipe);
        m_activeObstacles.push_back(bottomPipe);
        m_obstacleGroups[groupId] = {topPipe, bottomPipe};
        
        GN_LOG_DEBUG("Spawned sewer top-and-bottom pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
    }

    void ObstacleSystem::SpawnSewerPattern_Pyramid3(float x) {
        // Match old LevelManager's exact pyramid logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float spacingX = pipeW * m_baseScale * 1.15f;
        const float spacingY = pipeH * m_baseScale * 0.6f;
        const float row1Lift = 12.0f * m_baseScale;
        const float row2Lift = 24.0f * m_baseScale;
        const float baseY = SCREEN_HEIGHT - (pipeH * m_baseScale);
        
        int groupId = m_nextGroupId++;
        std::vector<Gnosis::Entity> pipes;
        
        auto spawn = [&](float col, int row, int idx) {
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1Lift : (row == 2 ? row2Lift : 0.0f));
            float pipeY = (baseY - row * spacingY) - lift;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, false);
            float groupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);
            AddEntityToGroup(pipe, groupId, idx == 0, pipeX - x, pipeY - baseY, groupWidth, GroupPattern::PyramidBottom);
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
        
        // Complement: TOP row across the span (funnel)
        const float topY = 0.0f;
        for (int c = 0; c < 3; ++c) {
            const std::string tex = (c % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float pipeX = x + c * spacingX;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, topY, m_baseScale, true);
            float groupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);
            AddEntityToGroup(pipe, groupId, false, pipeX - x, topY - baseY, groupWidth, GroupPattern::PyramidBottom);
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        }
        
        m_obstacleGroups[groupId] = pipes;
        GN_LOG_DEBUG("Spawned sewer pyramid-3 pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
    }

    void ObstacleSystem::SpawnSewerPattern_TwoByTwoFunnel(float x) {
        // Match old LevelManager's exact TwoByTwoFunnel logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float spacingX = pipeW * m_baseScale * 1.15f;
        const float spacingY = pipeH * m_baseScale * 0.6f;
        const float row1LiftTop = 12.0f * m_baseScale;
        const float row1Lift = 12.0f * m_baseScale;
        const float bottomY = SCREEN_HEIGHT - (pipeH * m_baseScale);
        const float topY = 0.0f;
        
        int groupId = m_nextGroupId++;
        std::vector<Gnosis::Entity> pipes;
        
        // Helper: bottom rows
        auto spawnBottom = [&](float col, int row, int idx) {
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1Lift : 0.0f);
            float pipeY = bottomY - row * spacingY - lift;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, false);
            float groupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);
            AddEntityToGroup(pipe, groupId, idx == 0, pipeX - x, pipeY - bottomY, groupWidth, GroupPattern::TwoFunnel);
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        };
        
        // Helper: top rows
        auto spawnTop = [&](float col, int row) {
            const std::string tex = (static_cast<int>(col + row) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1LiftTop : 0.0f);
            float pipeY = topY + row * spacingY + lift;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, true);
            float groupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);
            AddEntityToGroup(pipe, groupId, false, pipeX - x, pipeY - topY, groupWidth, GroupPattern::TwoFunnel);
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
        
        m_obstacleGroups[groupId] = pipes;
        GN_LOG_DEBUG("Spawned sewer two-by-two-funnel pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
    }

    void ObstacleSystem::SpawnSewerPattern_PyramidTop3(float x) {
        // Match old LevelManager's exact PyramidTop3 logic
        const float pipeW = 192.0f;
        const float pipeH = 64.0f;
        const float spacingX = pipeW * m_baseScale * 1.15f;
        const float spacingY = pipeH * m_baseScale * 0.6f;
        const float row1LiftTop = 12.0f * m_baseScale;
        const float row2LiftTop = 24.0f * m_baseScale;
        const float topY = 0.0f;
        
        int groupId = m_nextGroupId++;
        std::vector<Gnosis::Entity> pipes;
        
        auto spawnTop = [&](float col, int row, int idx) {
            const std::string tex = ((row + static_cast<int>(col)) % 2 == 0) ? "TopPipeWide" : "TopPipeWideBlue";
            float pipeX = x + col * spacingX;
            float lift = (row == 1 ? row1LiftTop : (row == 2 ? row2LiftTop : 0.0f));
            float pipeY = topY + row * spacingY + lift;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, pipeY, m_baseScale, true);
            float groupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);
            bool makeLeader = (row == 0 && col == 0.0f);
            AddEntityToGroup(pipe, groupId, makeLeader, pipeX - x, pipeY - topY, groupWidth, GroupPattern::PyramidTop);
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
        
        // Complement: BOTTOM row across the span (funnel)
        const float bottomY = SCREEN_HEIGHT - (pipeH * m_baseScale);
        for (int c = 0; c < 3; ++c) {
            const std::string tex = (c % 2 == 0) ? "BottomPipeWide" : "BottomPipeWideBlue";
            float pipeX = x + c * spacingX;
            
            Gnosis::Entity pipe = CreateSewerPipeEntity(tex, pipeX, bottomY, m_baseScale, false);
            float groupWidth = (2.0f * spacingX) + (pipeW * m_baseScale);
            AddEntityToGroup(pipe, groupId, false, pipeX - x, bottomY - topY, groupWidth, GroupPattern::PyramidTop);
            pipes.push_back(pipe);
            m_activeObstacles.push_back(pipe);
        }
        
        m_obstacleGroups[groupId] = pipes;
        GN_LOG_DEBUG("Spawned sewer pyramid-top-3 pattern at x=" + std::to_string(x) + ", groupId=" + std::to_string(groupId));
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
        sprite.layer = 3;
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
        hitbox.height = 190.0f - 30.0f; // Trim 30px
        hitbox.offsetX = 0.0f;
        hitbox.offsetY = isTop ? -(30.0f * 0.5f) : +(30.0f * 0.5f);
        hitbox.isStatic = false;
        hitbox.isTrigger = false;
        hitbox.tag = "obstacle";
        m_ecsSystem->AddComponent<Hitbox>(entity, hitbox);
        
        // Obstacle
        Obstacle obstacle;
        obstacle.obstacleType = texture;
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
        sprite.layer = (texture == "Outhouse") ? 3 : 2; // Outhouse in front, toilet behind
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
        obstacle.obstacleType = texture;
        obstacle.damage = 1;
        obstacle.basePosition = Gnosis::GNVector2(x, y);
        obstacle.isTopPart = (texture == "Outhouse");
        m_ecsSystem->AddComponent<Obstacle>(entity, obstacle);
        
        // DebugDraw - show hitboxes visually
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
        
        return entity;
    }

    Gnosis::Entity ObstacleSystem::CreateSewerPipeEntity(const std::string& texture, float x, float y, float scale, bool isTop) {
        Gnosis::Entity entity = m_ecsSystem->CreateEntity();
        
        // Transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
        m_ecsSystem->AddComponent<Transform>(entity, transform);
        
        // Sprite
        Sprite sprite(texture, 192.0f, 64.0f);
        sprite.layer = 3;
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
        obstacle.obstacleType = texture;
        obstacle.damage = 1;
        obstacle.basePosition = Gnosis::GNVector2(x, y);
        obstacle.isTopPart = isTop;
        m_ecsSystem->AddComponent<Obstacle>(entity, obstacle);
        
        return entity;
    }

    Gnosis::Entity ObstacleSystem::CreateCactusEntity(const std::string& texture, float x, float y, float scale) {
        Gnosis::Entity entity = m_ecsSystem->CreateEntity();
        
        // Transform
        Transform transform(Gnosis::GNVector2(x, y), 0.0f, Gnosis::GNVector2(scale, scale));
        m_ecsSystem->AddComponent<Transform>(entity, transform);
        
        // Sprite
        Sprite sprite(texture, 64.0f, 48.0f);
        sprite.layer = 3;
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
        hitbox.width = 64.0f;
        hitbox.height = 48.0f;
        hitbox.offsetX = 0.0f;
        hitbox.offsetY = 0.0f;
        hitbox.isStatic = false;
        hitbox.isTrigger = false;
        hitbox.tag = "obstacle";
        m_ecsSystem->AddComponent<Hitbox>(entity, hitbox);
        
        // Obstacle
        Obstacle obstacle;
        obstacle.obstacleType = texture;
        obstacle.damage = 1;
        obstacle.basePosition = Gnosis::GNVector2(x, y);
        obstacle.isTopPart = false;
        m_ecsSystem->AddComponent<Obstacle>(entity, obstacle);
        
        // DebugDraw - show hitboxes visually
        DebugDraw debugDraw;
        debugDraw.showBounds = false;
        debugDraw.showCollider = true;
        debugDraw.colliderColor = Gnosis::GNColor(0, 255, 0, 128); // Green for cacti
        debugDraw.alpha = 0.6f;
        debugDraw.debugLayer = 20;
        m_ecsSystem->AddComponent<DebugDraw>(entity, debugDraw);
        
        return entity;
    }

    // ============================================================================
    // Group Management - Stubs for now
    // ============================================================================

    void ObstacleSystem::AddEntityToGroup(Gnosis::Entity entity, int groupId, bool isLeader, 
                                        float offsetX, float offsetY, float groupWidth, GroupPattern pattern) {
        Group group;
        group.id = groupId;
        group.isLeader = isLeader;
        group.offsetX = offsetX;
        group.offsetY = offsetY;
        group.groupWidth = groupWidth;
        group.pattern = pattern;
        
        m_ecsSystem->AddComponent<Group>(entity, group);
    }

    void ObstacleSystem::WrapGroup(int groupId, float worldScrollDistance) {
        auto it = m_obstacleGroups.find(groupId);
        if (it == m_obstacleGroups.end()) {
            return;
        }
        
        std::vector<Gnosis::Entity>& groupEntities = it->second;
        if (groupEntities.empty()) {
            return;
        }
        
        // Find the leader entity
        Gnosis::Entity leader = 0;
        for (Gnosis::Entity entity : groupEntities) {
            Group* group = m_ecsSystem->GetComponent<Group>(entity);
            if (group && group->isLeader) {
                leader = entity;
                break;
            }
        }
        
        if (leader == 0) {
            return;
        }
        
        Transform* leaderTransform = m_ecsSystem->GetComponent<Transform>(leader);
        if (!leaderTransform) {
            return;
        }
        
        // Check if leader is far enough left to wrap
        const float wrapThreshold = -400.0f;
        if (leaderTransform->position.x > wrapThreshold) {
            return;
        }
        
        // Calculate wrap distance
        const float totalLevelWidth = SCREEN_WIDTH + (OBSTACLE_POOL_SIZE * 600.0f);
        const float wrapDistance = totalLevelWidth;
        
        // Wrap all entities in the group
        for (Gnosis::Entity entity : groupEntities) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(entity);
            if (transform) {
                transform->position.x += wrapDistance;
                
                // Update base position in Obstacle component
                Obstacle* obstacle = m_ecsSystem->GetComponent<Obstacle>(entity);
                if (obstacle) {
                    obstacle->basePosition.x += wrapDistance;
                }
            }
        }
        
        // Mark this group as wrapped for coin repositioning
        m_wrappedGroups.push_back(groupId);
    }

    float ObstacleSystem::CalculateGroupWidth(int groupId) const {
    // Prefer exact groupWidth from the Group component's leader to match old LevelManager
    auto it = m_obstacleGroups.find(groupId);
    if (it != m_obstacleGroups.end()) {
        const std::vector<Gnosis::Entity>& groupEntities = it->second;
        // Leader first
        for (Gnosis::Entity entity : groupEntities) {
            Group* g = m_ecsSystem->GetComponent<Group>(entity);
            if (g && g->isLeader) {
                return g->groupWidth;
            }
        }
        // Any member fallback
        for (Gnosis::Entity entity : groupEntities) {
            Group* g = m_ecsSystem->GetComponent<Group>(entity);
            if (g) {
                return g->groupWidth;
            }
        }
    }
    // As a last resort, scan active obstacles for this group id
    for (Gnosis::Entity e : m_activeObstacles) {
        Group* g = m_ecsSystem->GetComponent<Group>(e);
        if (g && g->id == groupId) {
            return g->groupWidth;
        }
    }
    // Fallback if group not found
    return 600.0f;
}

std::vector<Gnosis::Entity> ObstacleSystem::GetGroupEntities(int groupId) const {
        auto it = m_obstacleGroups.find(groupId);
        if (it != m_obstacleGroups.end()) {
            return it->second;
        }
        return {};
    }

    std::vector<int> ObstacleSystem::GetAndClearWrappedGroups() {
        std::vector<int> result;
        result.swap(m_wrappedGroups);
        return result;
    }

    std::vector<int> ObstacleSystem::ConsumeWrappedGroups() {
        std::vector<int> result;
        result.swap(m_wrappedGroups);
        return result;
    }

    std::vector<Gnosis::GNVector2> ObstacleSystem::CalculateCoinPositionsForGroup(int groupId, GroupPattern pattern) const {
        std::vector<Gnosis::GNVector2> positions;
        
        // Check if level allows pickups using level config
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
            float spacing = (right - left) / static_cast<float>(coinsPerStripe);
            for (int i = 0; i < coinsPerStripe; ++i) {
                float x = left + spacing * (i + 0.5f);
                positions.emplace_back(x, y);
            }
            GN_LOG_DEBUG("ObstacleSystem::emitStripe y=" + std::to_string(y) + " left=" + std::to_string(left) + " right=" + std::to_string(right));
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
            GN_LOG_DEBUG("ObstacleSystem::emitStripeInRange y=" + std::to_string(y) + " left=" + std::to_string(left) + " right=" + std::to_string(right));
        };

        switch (pattern) {
            case GroupPattern::TopOnly: {
                float y = (topBandBottomEdge != std::numeric_limits<float>::max())
                    ? (topBandBottomEdge + verticalPad)
                    : (bottomBandTopEdge > std::numeric_limits<float>::lowest() ? bottomBandTopEdge - verticalPad : 400.0f);
                if (topMinX < topMaxX) emitStripeInRange(y, topMinX, topMaxX); else emitStripe(y);
                GN_LOG_DEBUG("ObstacleSystem::TopOnly stripe y=" + std::to_string(y) + " span=[" + std::to_string(topMinX) + "," + std::to_string(topMaxX) + "]");
                break;
            }
            case GroupPattern::BottomOnly: {
                float y = (bottomBandTopEdge != std::numeric_limits<float>::lowest())
                    ? (bottomBandTopEdge - verticalPad)
                    : (topBandBottomEdge < std::numeric_limits<float>::max() ? topBandBottomEdge + verticalPad : 400.0f);
                if (bottomMinX < bottomMaxX) emitStripeInRange(y, bottomMinX, bottomMaxX); else emitStripe(y);
                GN_LOG_DEBUG("ObstacleSystem::BottomOnly stripe y=" + std::to_string(y) + " span=[" + std::to_string(bottomMinX) + "," + std::to_string(bottomMaxX) + "]");
                break;
            }
            case GroupPattern::TopAndBottom: {
                // Two stripes: one below top band and one above bottom band, each within its own band span
                if (topBandBottomEdge != std::numeric_limits<float>::max()) {
                    if (topMinX < topMaxX) emitStripeInRange(topBandBottomEdge + verticalPad, topMinX, topMaxX);
                }
                if (bottomBandTopEdge != std::numeric_limits<float>::lowest()) {
                    if (bottomMinX < bottomMaxX) emitStripeInRange(bottomBandTopEdge - verticalPad, bottomMinX, bottomMaxX);
                }
                GN_LOG_DEBUG("ObstacleSystem::TopAndBottom top stripe below y=" + std::to_string(topBandBottomEdge + verticalPad) + " bottom stripe above y=" + std::to_string(bottomBandTopEdge - verticalPad));
                break;
            }
            case GroupPattern::Ground: {
                // Ground obstacles: place coins above the obstacle at a safe height
                float groundObstacleTop = std::numeric_limits<float>::max();
                for (Gnosis::Entity e : m_activeObstacles) {
                    Group* g = m_ecsSystem->GetComponent<Group>(e);
                    if (!g || g->id != groupId) continue;
                    Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                    if (!t) continue;
                    groundObstacleTop = std::min(groundObstacleTop, t->position.y);
                }
                if (groundObstacleTop != std::numeric_limits<float>::max()) {
                    float coinY = groundObstacleTop - verticalPad; // Place coins above ground obstacles
                    emitStripe(coinY);
                    GN_LOG_DEBUG("ObstacleSystem::Ground stripe y=" + std::to_string(coinY) + " above ground obstacle");
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
                    GN_LOG_DEBUG("ObstacleSystem::TwoFunnel clusters=" + std::to_string(clusters.size()) + " centerY=" + std::to_string(centerY));
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
                    GN_LOG_DEBUG("ObstacleSystem::PyramidBottom complement stripe span=[" + std::to_string(p.left) + "," + std::to_string(p.right) + "] y=" + std::to_string(y));
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
                    GN_LOG_DEBUG("ObstacleSystem::PyramidTop complement stripe span=[" + std::to_string(p.left) + "," + std::to_string(p.right) + "] y=" + std::to_string(y));
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


    void ObstacleSystem::WrapGroupAroundScreen(int groupId, float worldScrollDistance) {
        // Find all entities in this group and check if the leader needs wrapping
        Gnosis::Entity leaderEntity = 0;
        float leaderBaseOffsetX = 0.0f;
        bool needsWrapping = false;
        GroupPattern groupPattern = GroupPattern::TopAndBottom;
        
        for (Gnosis::Entity e : m_activeObstacles) {
            if (!m_ecsSystem->HasComponent<Group>(e) || !m_ecsSystem->HasComponent<Transform>(e)) {
                continue;
            }
            
            auto* group = m_ecsSystem->GetComponent<Group>(e);
            auto* transform = m_ecsSystem->GetComponent<Transform>(e);
            
            if (group->id == groupId && group->isLeader) {
                leaderEntity = e;
                leaderBaseOffsetX = group->offsetX;
                groupPattern = group->pattern;
                
                // Check if leader has moved off-screen (left side)
                if (transform->position.x < -200.0f) {
                    needsWrapping = true;
                }
                break;
            }
        }
        
        if (!needsWrapping || leaderEntity == 0) {
            return;
        }
        
        // Find rightmost group leader and use its Group component's groupWidth (OLD LEVELMANAGER LOGIC)
        float rightmostRightEdge = 0.0f;
        float rightmostOriginX = 0.0f;
        float rightmostGroupWidth = 0.0f;
        int rightmostGroupId = -1;
        
        for (Gnosis::Entity e : m_activeObstacles) {
            Group* g = m_ecsSystem->GetComponent<Group>(e);
            if (g && g->isLeader && g->id != groupId) {
                Transform* t = m_ecsSystem->GetComponent<Transform>(e);
                if (t) {
                    float r = t->position.x + g->groupWidth; // Use Group component's groupWidth
                    if (r > rightmostRightEdge) {
                        rightmostRightEdge = r;
                        rightmostGroupId = g->id;
                        rightmostOriginX = t->position.x;
                        rightmostGroupWidth = g->groupWidth;
                    }
                }
            }
        }
        
        // Position new group exactly after rightmost group with NO GAP (safeGap = 0.0f)
        float newX = rightmostOriginX + rightmostGroupWidth;
        
        // Handle level-specific positioning logic within the group
        if (m_currentLevelId == 1 && groupPattern == GroupPattern::TopAndBottom) {
            // Park level: Randomize toilet pair Y positions with proper gap
            float toiletHeight = 190.0f * m_baseScale;
            float minTopY = -toiletHeight * 0.8f;
            float maxTopY = -toiletHeight * 0.2f;
            float randomTopY = minTopY + (maxTopY - minTopY) * ((float)rand() / RAND_MAX);
            float fixedGapHeight = 800.0f; // Vertical gap within toilet pairs set to 800px
            
            for (Gnosis::Entity e : m_activeObstacles) {
                if (!m_ecsSystem->HasComponent<Group>(e) || !m_ecsSystem->HasComponent<Transform>(e) ||
                    !m_ecsSystem->HasComponent<Obstacle>(e)) {
                    continue;
                }
                
                auto* group = m_ecsSystem->GetComponent<Group>(e);
                auto* transform = m_ecsSystem->GetComponent<Transform>(e);
                auto* obstacle = m_ecsSystem->GetComponent<Obstacle>(e);
                
                if (group->id == groupId) {
                    // Calculate relative offset from leader
                    float relativeOffsetX = group->offsetX - leaderBaseOffsetX;
                    transform->position.x = newX + relativeOffsetX;
                    
                    // Set Y positions based on toilet type
                    if (obstacle->obstacleType == "TopToilet") {
                        transform->position.y = randomTopY;
                    } else if (obstacle->obstacleType == "BottomToilet") {
                        transform->position.y = randomTopY + toiletHeight + fixedGapHeight;
                    }
                    
                    // Reset obstacle state
                    obstacle->pipeCleared = false;
                }
            }
        } else {
            // Sewer and Desert levels: maintain original relative positioning
            for (Gnosis::Entity e : m_activeObstacles) {
                if (!m_ecsSystem->HasComponent<Group>(e) || !m_ecsSystem->HasComponent<Transform>(e)) {
                    continue;
                }
                
                auto* group = m_ecsSystem->GetComponent<Group>(e);
                auto* transform = m_ecsSystem->GetComponent<Transform>(e);
                
                if (group->id == groupId) {
                    // Calculate relative offset from leader
                    float relativeOffsetX = group->offsetX - leaderBaseOffsetX;
                    transform->position.x = newX + relativeOffsetX;
                    
                    // Y positioning is maintained from original spawning
                    // (Sewer pipes keep their top/bottom Y, Desert outhouses keep their stack Y)
                    
                    // Reset obstacle state
                    if (m_ecsSystem->HasComponent<Obstacle>(e)) {
                        auto* obstacle = m_ecsSystem->GetComponent<Obstacle>(e);
                        obstacle->pipeCleared = false;
                    }
                }
            }
        }
        
        GN_LOG_INFO("Wrapped group " + std::to_string(groupId) + " to x: " + std::to_string(newX) + 
                   " (positioned after group " + std::to_string(rightmostGroupId) + 
                   ", groupWidth: " + std::to_string(rightmostGroupWidth) + ")");
        
        // Mark this group as wrapped for coin repositioning
        m_wrappedGroups.push_back(groupId);
    }

    void ObstacleSystem::RenderDebugHitboxes() {
        if (!m_debugMode) {
            return;
        }

        GN_LOG_DEBUG("Rendering debug hitboxes for " + std::to_string(m_activeObstacles.size()) + " obstacles");
        
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

            // Calculate hitbox world position
            float hitboxX = transform->position.x + (hitbox->offsetX * transform->scale.x);
            float hitboxY = transform->position.y + (hitbox->offsetY * transform->scale.y);
            float hitboxW = hitbox->width * transform->scale.x;
            float hitboxH = hitbox->height * transform->scale.y;

            // Log hitbox information for debugging
            GN_LOG_DEBUG("Obstacle " + std::to_string(entity) + 
                        " (" + obstacle->obstacleType + "): " +
                        "pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ") " +
                        "hitbox=(" + std::to_string(hitboxX) + "," + std::to_string(hitboxY) + "," + 
                        std::to_string(hitboxW) + "," + std::to_string(hitboxH) + ") " +
                        "isTrigger=" + std::to_string(hitbox->isTrigger) + 
                        " pipeCleared=" + std::to_string(obstacle->pipeCleared));
        }
    }

} // namespace GameCore
