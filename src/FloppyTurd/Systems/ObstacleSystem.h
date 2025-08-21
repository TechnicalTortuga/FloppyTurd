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

    class ObstacleSystem {
    public:
        ObstacleSystem(Gnosis::ECS* ecsSystem);
        ~ObstacleSystem();

        // Lifecycle
        void InitializeForLevel(int levelId, const LevelConfig& config);
        void Update(float deltaTime, float worldScrollDistance);
        void Cleanup();

        // Pattern spawning
        void SpawnPattern(PatternType type, float x);
        void SpawnRandomPatternForLevel(int levelId, float x);

        // Group management
        std::vector<int> ConsumeWrappedGroups();
        
        // Coin positioning system
        std::vector<Gnosis::GNVector2> CalculateCoinPositionsForGroup(int groupId, GroupPattern pattern) const;
        bool IsGroupReadyForCoins(int groupId) const;
        GroupPattern DetectGroupPattern(int groupId) const;
        
        std::vector<Gnosis::Entity> GetGroupEntities(int groupId) const;

        // State queries
        bool IsInitialized() const { return m_initialized; }
        size_t GetActiveObstacleCount() const { return m_activeObstacles.size(); }
        const std::vector<Gnosis::Entity>& GetActiveObstacles() const { return m_activeObstacles; }
        std::vector<int> GetAndClearWrappedGroups();

        // SpikeBall utilities
        Gnosis::Entity GetSpikeBallBaseEntity(Gnosis::Entity spikeBallEntity) const;

        // Debug rendering
        void RenderDebugHitboxes();
        void RemoveAllDebugDraws();

    private:
        // Pattern implementations
        void SpawnParkPattern_ToiletPair(float x);
        void SpawnDesertPattern_Outhouse(float x);
        void SpawnDesertPattern_Cactus(float x);
        void SpawnSewerPattern_TopOnly(float x);
        void SpawnSewerPattern_BottomOnly(float x);
        void SpawnSewerPattern_TopAndBottom(float x);
        void SpawnSewerPattern_Pyramid3(float x);
        void SpawnSewerPattern_PyramidTop3(float x);
        void SpawnSewerPattern_TwoByTwoFunnel(float x);
        void SpawnSnowPattern_ToiletPair(float x);
        void SpawnCastlePattern_GoldToiletPair(float x);

        // Entity creation helpers
        Gnosis::Entity CreateToiletEntity(const std::string& texture, float x, float y, float scale, bool isTop);
        Gnosis::Entity CreateOuthouseEntity(const std::string& texture, float x, float y, float scale, bool isSolid);
        Gnosis::Entity CreateSewerPipeEntity(const std::string& texture, float x, float y, float scale, bool isTop);
        Gnosis::Entity CreateCactusEntity(const std::string& texture, float x, float y, float scale, bool isDancing = false, float width = 64.0f, float height = 90.0f);
        
        // Castle decorative element helpers
        void SpawnCastleTorchPillar(float x, int groupId, float offsetX);
        void SpawnCastleChandelier(float x, int groupId, float offsetX);
        void SpawnCastleFloorTorch(float x, int groupId, float offsetX);
        void SpawnCastleDecorativePainting(float x, int groupId, float offsetX);
        void SpawnCastleSpikeBall(float x, int groupId, float offsetX);
        
        // Cactus system
        void InitializeCactusSystem();
        void SpawnCactusPool(float startX);
        void UpdateCactusAnimation(float deltaTime);
        void WrapCactusPool(float worldScrollDistance);
        
        // Spike ball rotation system
        void UpdateSpikeBallRotations(float deltaTime);
        
        // Obstacle oscillation system
        void UpdateObstacleOscillation(float deltaTime);

        // Group management
        void AddEntityToGroup(Gnosis::Entity entity, int groupId, bool isLeader, 
                            float offsetX, float offsetY, float groupWidth, GroupPattern pattern);
        void WrapGroup(int groupId, float worldScrollDistance);
        float CalculateGroupWidth(int groupId) const;

        // Utility
        void LinkToiletPair(Gnosis::Entity top, Gnosis::Entity bottom);
        void LinkOuthousePair(Gnosis::Entity outhouse, Gnosis::Entity toilet);
        void WrapGroupAroundScreen(int groupId, float worldScrollDistance);

        Gnosis::ECS* m_ecsSystem;
        std::vector<Gnosis::Entity> m_activeObstacles;
        std::unordered_map<int, std::vector<Gnosis::Entity>> m_obstacleGroups;
        std::vector<int> m_wrappedGroups;

        int m_currentLevelId = 0;
        int m_nextGroupId = 1;
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
        static constexpr float SCREEN_WIDTH = 1179.0f;
        static constexpr float SCREEN_HEIGHT = 2556.0f;
    };

} // namespace GameCore
