#pragma once

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../Config/LevelConfig.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace GameCore {

    enum class PatternType {
        PARK_TOILET_PAIR,
        DESERT_OUTHOUSE,
        DESERT_CACTUS,
        SEWER_TOP_ONLY,
        SEWER_BOTTOM_ONLY,
        SEWER_TOP_AND_BOTTOM,
        SEWER_PYRAMID_3,
        SEWER_PYRAMID_TOP_3,
        SEWER_TWO_BY_TWO_FUNNEL,
        SNOW_TOILET_PAIR,
        CASTLE_GOLD_TOILET_PAIR
    };

    struct PatternConfig {
        PatternType type;
        float weight = 1.0f;        // For random selection
        float minSpacing = 0.0f;    // Minimum distance from previous pattern
    };

    // ============================================================================
    // LAYERMANAGER - Centralized render layer assignment utility
    // ============================================================================
    
    enum class RenderLayer : int {
        Background = 1,             // Background decorations (curtains, furthest elements)
        Decoration = 2,             // Mid-layer decorations (torches, chandeliers)
        DecorationFront = 3,        // Front decorations (torch pillars, paintings)
        Obstacles = 4,              // Obstacles (toilets, pipes, spike balls)
        PickupsAndEffects = 5,      // Pickups (coins, hearts) and particle effects
        Player = 6,                 // Player character
        UI = 10                     // UI elements
    };
    
    class LayerManager {
    public:
        // Get layer for specific entity types
        static int GetLayerForEntity(Gnosis::ECS* ecs, Gnosis::Entity entity) {
            // Check component types to determine layer
            if (ecs->HasComponent<GameCore::Obstacle>(entity)) {
                return static_cast<int>(RenderLayer::Obstacles);
            }
            if (ecs->HasComponent<GameCore::Pickup>(entity)) {
                return static_cast<int>(RenderLayer::PickupsAndEffects);
            }
            if (ecs->HasComponent<GameCore::Decoration>(entity)) {
                auto* deco = ecs->GetComponent<GameCore::Decoration>(entity);
                return deco ? deco->renderLayer : static_cast<int>(RenderLayer::Decoration);
            }
            return static_cast<int>(RenderLayer::Obstacles);  // Default fallback
        }
        
        // Get layer for pickup types (for spawning)
        static int GetLayerForPickup(PickupType type) {
            return static_cast<int>(RenderLayer::PickupsAndEffects);  // All pickups on same layer
        }
        
        // Get layer for decoration types (for spawning)
        static int GetLayerForDecoration(DecorationType type) {
            switch (type) {
                case DecorationType::Curtain:
                    return static_cast<int>(RenderLayer::Background);  // Furthest back
                case DecorationType::FloorTorch:
                case DecorationType::Chandelier:
                    return static_cast<int>(RenderLayer::Decoration);  // Mid-layer
                case DecorationType::TorchPillar:
                case DecorationType::PaintingRabbitKnight:
                case DecorationType::PaintingRatBeach:
                case DecorationType::PaintingRiverWalk:
                case DecorationType::PaintingCabin:
                    return static_cast<int>(RenderLayer::DecorationFront);  // Front decorations
                default:
                    return static_cast<int>(RenderLayer::Decoration);
            }
        }
    };

    class ObstacleSystem {
    public:
        ObstacleSystem(Gnosis::ECS* ecsSystem);
        ~ObstacleSystem();

        // Lifecycle
        void InitializeForLevel(int levelId, const LevelConfig& config);
        void Update(float deltaTime, float worldScrollDistance);
        void Cleanup();

        // Pattern spawning
        // LEGACY DELETED: SpawnPattern() and SpawnRandomPatternForLevel()
        // Replaced by orchestrator pattern - LevelManager calls individual Spawn*Pattern_* functions with groupId

        // Coin positioning system (legacy - used by PickupSystem during migration)
        std::vector<Gnosis::GNVector2> CalculateCoinPositionsForGroup(int groupId, GroupPattern pattern, float gapWidth = 800.0f) const;
        bool IsGroupReadyForCoins(int groupId) const;
        GroupPattern DetectGroupPattern(int groupId) const;

        // State queries
        bool IsInitialized() const { return m_initialized; }
        size_t GetActiveObstacleCount() const { return m_activeObstacles.size(); }
        const std::vector<Gnosis::Entity>& GetActiveObstacles() const { return m_activeObstacles; }

        // SpikeBall utilities
        Gnosis::Entity GetSpikeBallBaseEntity(Gnosis::Entity spikeBallEntity) const;

        // Debug rendering
        void RenderDebugHitboxes();
        void RemoveAllDebugDraws();

        // Boss level decorations
        void AddBossLevelDecorations();

        // Pattern spawn functions (NEW: return created entities for manifest tracking)
        // PUBLIC: LevelManager orchestrator calls these to spawn obstacles
        std::pair<Gnosis::Entity, Gnosis::Entity> SpawnParkPattern_ToiletPair(float x, int groupId);
        Gnosis::Entity SpawnDesertPattern_Outhouse(float x, int groupId);
        Gnosis::Entity SpawnDesertPattern_Cactus(float x, int groupId);
        std::vector<Gnosis::Entity> SpawnSewerPattern_TopOnly(float x, int groupId);
        std::vector<Gnosis::Entity> SpawnSewerPattern_BottomOnly(float x, int groupId);
        std::vector<Gnosis::Entity> SpawnSewerPattern_TopAndBottom(float x, int groupId);
        std::vector<Gnosis::Entity> SpawnSewerPattern_Pyramid3(float x, int groupId);
        std::vector<Gnosis::Entity> SpawnSewerPattern_PyramidTop3(float x, int groupId);
        std::vector<Gnosis::Entity> SpawnSewerPattern_TwoByTwoFunnel(float x, int groupId);
        std::pair<Gnosis::Entity, Gnosis::Entity> SpawnSnowPattern_ToiletPair(float x, int groupId, float gapWidth);
        std::pair<Gnosis::Entity, Gnosis::Entity> SpawnCastlePattern_GoldToiletPair(float x, int groupId, float gapWidth);

    private:

        // Entity creation helpers
        Gnosis::Entity CreateToiletEntity(const std::string& texture, float x, float y, float scale, bool isTop);
        Gnosis::Entity CreateOuthouseEntity(const std::string& texture, float x, float y, float scale, bool isSolid);
        Gnosis::Entity CreateSewerPipeEntity(const std::string& texture, float x, float y, float scale, bool isTop, int layer = 4);
        Gnosis::Entity CreateCactusEntity(const std::string& texture, float x, float y, float scale, bool isDancing = false, float width = 64.0f, float height = 90.0f);
        
        // Castle decorative element helpers
        Gnosis::Entity SpawnCastleCurtain(float x, int groupId);
        Gnosis::Entity SpawnCastleTorchPillar(float x, int groupId, float offsetX);
        Gnosis::Entity SpawnCastleChandelier(float x, int groupId, float offsetX);
        Gnosis::Entity SpawnCastleFloorTorch(float x, int groupId, float offsetX);
        Gnosis::Entity SpawnCastleDecorativePainting(float x, int groupId, float offsetX);
        Gnosis::Entity SpawnCastleSpikeBall(float x, int groupId, float offsetX);
        
        // Cactus system
        void InitializeCactusSystem();
        void SpawnCactusPool(float startX);
        void UpdateCactusAnimation(float deltaTime);
        void WrapCactusPool(float worldScrollDistance);
        
        // Spike ball rotation system
        void UpdateSpikeBallRotations(float deltaTime);
        
        // Obstacle oscillation system
        void UpdateObstacleOscillation(float deltaTime);

        // REMOVED LEGACY GROUP MANAGEMENT (now in LevelManager):
        // - AddEntityToGroup() 
        // - WrapGroup()
        // - CalculateGroupWidth()

        // Utility
        void LinkToiletPair(Gnosis::Entity top, Gnosis::Entity bottom);
        void LinkOuthousePair(Gnosis::Entity outhouse, Gnosis::Entity toilet);
        // LEGACY DELETED: WrapGroupAroundScreen() - wrapping now handled by LevelManager::WrapGroup() + UpdateGroupMemberPositions()

        Gnosis::ECS* m_ecsSystem;
        std::vector<Gnosis::Entity> m_activeObstacles;
        
        // REMOVED LEGACY FIELDS:
        // - m_obstacleGroups (replaced by LevelManager::m_groupManifests)
        // - m_wrappedGroups (wrap detection now in LevelManager)

        int m_currentLevelId = 0;
        int m_nextGroupId = 1;  // LEGACY: Still used during spawn, will be removed when InitializeForLevel uses orchestrator
        bool m_initialized = false;
        float m_baseScale = 8.0f;
        float m_worldSpeed = 200.0f;
        
        // Level configuration reference
        const LevelConfig* m_levelConfig = nullptr;

        // Level-specific pattern configs
        std::vector<PatternConfig> m_levelPatterns;

        // Cactus system
        struct CactusType {
            std::string texture;
            float width;
            float height;
            float weight;
            bool isDancing;
            float animationSpeed;
        };
        std::vector<CactusType> m_cactusTypes;
        std::vector<Gnosis::Entity> m_cactusPool;
        std::map<Gnosis::Entity, const CactusType*> m_cactusTypeMap; // Track cactus types for proper positioning
        bool m_cactusPoolInitialized = false;

        // Spike ball rotation system data
        std::map<Gnosis::Entity, float> m_spikeBallRotationSpeeds;
        std::map<Gnosis::Entity, Gnosis::Entity> m_spikeBallToBase; // Track which base belongs to which spike ball
        
        // Debug system data
        bool m_debugMode = true;

        static constexpr int OBSTACLE_POOL_SIZE = 32;
        static constexpr int CACTUS_POOL_SIZE = 16;
        // Note: SCREEN_WIDTH and SCREEN_HEIGHT removed - use ConfigManager::Instance().GetCurrentScreenInfo() for dynamic resolution
    };

} // namespace GameCore
