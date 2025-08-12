#ifndef FLOPPY_TURD_LEVEL_MANAGER_H
#define FLOPPY_TURD_LEVEL_MANAGER_H

#include "../Config/LevelConfig.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Core/ECS.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace GameCore {

    /**
     * @brief Level Manager System
     * 
     * Manages level lifecycle, progression, and content spawning
     * Provides modular, extensible level management
     */
    class LevelManager {
    public:
        explicit LevelManager(Gnosis::ECS* ecsSystem);
        void SetPlatformDelegates(const GameCore::PlatformDelegates& delegates) { m_platformDelegates = delegates; }
        const GameCore::PlatformDelegates& GetPlatformDelegates() const { return m_platformDelegates; }
        ~LevelManager();

        // Player reference (used by NPC/enemy behaviors)
        void SetPlayerEntity(Gnosis::Entity playerEntity) { m_playerEntity = playerEntity; }
        Gnosis::Entity GetPlayerEntity() const { return m_playerEntity; }

        // Level lifecycle
        bool LoadLevel(int levelId);
        void UnloadLevel();
        void ResetLevel();
        bool IsLevelLoaded() const { return m_isLoaded; }
        
        // Current level info
        int GetCurrentLevelId() const { return m_currentLevelId; }
        const LevelConfig& GetCurrentLevelConfig() const { return m_currentLevelConfig; }
        std::string GetCurrentLevelName() const;
        
        // Level progression
        bool IsLevelUnlocked(int levelId) const;
        void UnlockLevel(int levelId);
        bool CompleteLevel(int levelId, int score, int coinsCollected);
        
        // Difficulty management
        static void SetGlobalDifficulty(Difficulty difficulty) { s_globalDifficulty = difficulty; }
        static Difficulty GetGlobalDifficulty() { return s_globalDifficulty; }
        static std::string GetDifficultyName() { return DifficultyToString(s_globalDifficulty); }
        
        // Content spawning (called by systems)
        void UpdateObstacleSpawning(float deltaTime);
        void UpdateEnemySpawning(float deltaTime);
        void UpdatePickupSpawning(float deltaTime);
        void UpdateNPCSpawning(float deltaTime);
        void UpdateNPCStates(float deltaTime);
        // Pool-based updates (no dynamic spawns during gameplay)
        void UpdateEnemyPooling(float deltaTime, float worldScrollDistance);
        void UpdateNPCPooling(float deltaTime, float worldScrollDistance);
        
        // New object pooling system
        void UpdateObstaclePooling(float deltaTime, float worldScrollDistance);
        void InitializeObstaclePool();
        void WrapObstacleAroundScreen(Gnosis::Entity obstacle, float worldScrollDistance);
        void WrapGroupAroundScreen(int groupId, float worldScrollDistance);
        
        // Obstacle management
        Gnosis::Entity SpawnObstacle(const ObstacleConfig& config, float x, float y);
        Gnosis::Entity SpawnToiletPair(const ObstacleConfig& config, float x, float y); // For toilet pairs
        Gnosis::Entity SpawnToiletPairWithGap(const ObstacleConfig& config, float x, float gapCenterY, float gapHeight); // For toilet pairs with custom gap
        Gnosis::Entity SpawnSingleObstacle(const ObstacleConfig& config, float x, float y); // For single obstacles
        // Sewer group spawns (single-piece patterns)
        void SpawnSewerPattern_TopOnly(float startX);
        void SpawnSewerPattern_BottomOnly(float startX);
        void SpawnSewerPattern_TopAndBottom(float startX);
        void SpawnSewerPattern_Pyramid3(float startX);
        void SpawnSewerPattern_PyramidTop3(float startX);
        void SpawnSewerPattern_Pyramid4(float startX);
        void RemoveObstacle(Gnosis::Entity obstacle);
        std::vector<Gnosis::Entity> GetActiveObstacles() const { return m_activeObstacles; }
        
        // Enemy management  
        Gnosis::Entity SpawnEnemy(const EnemyConfig& config, float x, float y);
        void RemoveEnemy(Gnosis::Entity enemy);
        std::vector<Gnosis::Entity> GetActiveEnemies() const { return m_activeEnemies; }
        void InitializeEnemyPool();
        
        // Pickup management
        Gnosis::Entity SpawnPickup(const std::string& type, float x, float y);
        void RemovePickup(Gnosis::Entity pickup);
        std::vector<Gnosis::Entity> GetActivePickups() const { return m_activePickups; }
        std::vector<Gnosis::Entity> GetActiveNPCs() const { return m_activeNPCs; }
        void InitializeNPCPool();
        // Sewer pickup patterns
        void SpawnPickupGroup(float startX, float midY, int count);
        void RerollPickupGroupInPlace(std::vector<Gnosis::Entity>& group, float startX, float midY, int count);
        void InitializePickupPool();
        void InitializeProjectilePool();
        void UpdatePickupPooling(float deltaTime, float worldScrollDistance);
        void UpdateProjectilePooling(float deltaTime, float worldScrollDistance);
        
        // Cleanup
        void CleanupOffscreenEntities(float leftBoundary);
        Gnosis::Entity SpawnNPCJanitor(float x, float y);
        
        // Level validation
        static bool ValidateLevelId(int levelId);
        static int GetMaxLevelId() { return LevelConfigFactory::GetLevelCount(); }
        
    private:
        // Core systems
        Gnosis::ECS* m_ecsSystem;
        GameCore::PlatformDelegates m_platformDelegates;
        
        // Current level state
        bool m_isLoaded;
        int m_currentLevelId;
        LevelConfig m_currentLevelConfig;
        
        // Level progression tracking
        std::vector<bool> m_unlockedLevels;
        std::vector<int> m_levelScores;
        std::vector<bool> m_levelCompleted;
        
        // Global difficulty setting
        static Difficulty s_globalDifficulty;
        
        // Entity tracking
        std::vector<Gnosis::Entity> m_activeObstacles;
        std::vector<Gnosis::Entity> m_activeEnemies;
        std::vector<Gnosis::Entity> m_activePickups;
        std::vector<Gnosis::Entity> m_pickupPool;
        std::vector<Gnosis::Entity> m_projectilePool;
        std::vector<Gnosis::Entity> m_activeNPCs;
        std::vector<Gnosis::Entity> m_backgroundEntities;
        Gnosis::Entity m_playerEntity = 0;
        Gnosis::Entity m_janitorEntity = 0; // Ensure single Janitor in Level 2
        
        // Spawn timers
        float m_obstacleSpawnTimer;
        float m_enemySpawnTimer;
        float m_pickupSpawnTimer;
        float m_npcSpawnTimer;
        int m_maxActiveEnemies = 4; // Limit active enemies (Level 2 uses 4)
        float m_enemySpacing = 450.0f; // Horizontal spacing used when wrapping enemy pool
        bool m_enemyPoolInitialized = false;
        bool m_npcPoolInitialized = false;
        bool m_pickupPoolInitialized = false;
        bool m_projectilePoolInitialized = false;
        
        // Spawn positions
        float m_lastObstacleX;
        float m_lastEnemyX;
        float m_lastPickupX;
        std::deque<std::vector<Gnosis::Entity>> m_coinPatterns; // queued coin groups (3 or 5)
        std::unordered_map<Gnosis::Entity, float> m_enemyBaseY;
        std::unordered_map<int, bool> m_groupTopCounted;
        std::unordered_map<int, bool> m_groupBottomCounted;
        float m_lastPlayerX = 0.0f;
        std::unordered_set<Gnosis::Entity> m_countedPipes;
        int m_pipeCounter = 0;
        
        // Object pooling system
        static const int OBSTACLE_POOL_SIZE = 6;  // Pool of 6 obstacles (like old ParkLevel had 5+1)
        float m_obstacleSpacing;                   // Distance between obstacles
        bool m_poolInitialized;
        int m_nextGroupId = 1;                     // Incremental group id for formations
        
        // Internal methods
        void InitializeProgressionSystem();
        void CreateBackgroundLayers();
        void DestroyBackgroundLayers();
        void DestroyAllEntities();
        
        // Spawning helpers
        float CalculateNextObstaclePosition();
        float CalculateNextEnemyPosition();
        float CalculateNextPickupPosition();
        
        // Save/load progression
        void SaveProgression();
        void LoadProgression();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_LEVEL_MANAGER_H
