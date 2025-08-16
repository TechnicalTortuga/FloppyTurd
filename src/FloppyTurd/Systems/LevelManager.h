#ifndef FLOPPY_TURD_LEVEL_MANAGER_H
#define FLOPPY_TURD_LEVEL_MANAGER_H

#include "../Config/LevelConfig.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Core/ECS.h"
#include "ObstacleSystem.h"
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
        void UpdateEnemySpawning(float deltaTime);
        // REMOVED: UpdatePickupSpawning - now handled by GameplayState
        void UpdateNPCSpawning(float deltaTime);
        void UpdateNPCStates(float deltaTime);
        // Pool-based updates (no dynamic spawns during gameplay)
        void UpdateEnemyPooling(float deltaTime, float worldScrollDistance);
        void UpdateNPCPooling(float deltaTime, float worldScrollDistance);
        
        // Obstacle system delegation
        void UpdateObstacleSystem(float deltaTime, float worldScrollDistance);
        std::vector<int> ConsumeWrappedGroups();
        std::vector<Gnosis::Entity> GetActiveObstacles() const;
        
        // Obstacle system access
        ObstacleSystem* GetObstacleSystem() const { return m_obstacleSystem.get(); }
        std::vector<int> GetAndClearWrappedGroups();
        
        // Enemy management  
        Gnosis::Entity SpawnEnemy(const EnemyConfig& config, float x, float y);
        void RemoveEnemy(Gnosis::Entity enemy);
        std::vector<Gnosis::Entity> GetActiveEnemies() const { return m_activeEnemies; }
        void InitializeEnemyPool();
        
        // Pickup management moved to GameplayState (single source of truth)
        std::vector<Gnosis::Entity> GetActiveNPCs() const { return m_activeNPCs; }
        void InitializeNPCPool();
        // REMOVED: SpawnPickupGroup, RerollPickupGroupInPlace - moved to GameplayState
        void InitializeProjectilePool();
        // REMOVED: UpdatePickupPooling - now handled by GameplayState
        void UpdateProjectilePooling(float deltaTime, float worldScrollDistance);
        
        // Cleanup
        void CleanupOffscreenEntities(float leftBoundary);
        Gnosis::Entity SpawnNPCJanitor(float x, float y);
        
        // Use shared GroupPattern declared in GameComponents.h
        using GroupPattern = GameCore::GroupPattern;
        
        // REMOVED: Coin system helpers moved to ObstacleSystem
        GroupPattern DetectGroupPattern(int groupId) const;  // Legacy: read from group's pattern
        
        // Level validation
        static bool ValidateLevelId(int levelId);
        static int GetMaxLevelId() { return LevelConfigFactory::GetLevelCount(); }
        
    private:
        // Core systems
        Gnosis::ECS* m_ecsSystem;
        GameCore::PlatformDelegates m_platformDelegates;
        std::unique_ptr<ObstacleSystem> m_obstacleSystem;
        
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
        std::vector<Gnosis::Entity> m_activeEnemies;
        std::vector<Gnosis::Entity> m_projectilePool;
        std::vector<Gnosis::Entity> m_activeNPCs;
        std::vector<Gnosis::Entity> m_backgroundEntities;
        Gnosis::Entity m_playerEntity = 0;
        Gnosis::Entity m_janitorEntity = 0; // Ensure single Janitor in Level 2
        
        // Spawn timers
        float m_enemySpawnTimer;
        // REMOVED: m_pickupSpawnTimer - pickup spawning moved to GameplayState
        float m_npcSpawnTimer;
        int m_maxActiveEnemies = 4; // Limit active enemies (Level 2 uses 4)
        float m_enemySpacing = 450.0f; // Horizontal spacing used when wrapping enemy pool
        bool m_enemyPoolInitialized = false;
        bool m_npcPoolInitialized = false;
        bool m_projectilePoolInitialized = false;
        // REMOVED: m_attachCoinsToGroups - coin attachment moved to GameplayState
        
        // Spawn positions
        float m_lastEnemyX;
        // REMOVED: m_groupCoins - coin tracking moved to GameplayState
        std::unordered_map<Gnosis::Entity, float> m_enemyBaseY; // Enemy base Y positions
        
        // Legacy obstacle pooling removed - now handled by ObstacleSystem

        // Internal methods
        void InitializeProgressionSystem();
        void CreateBackgroundLayers();
        void DestroyBackgroundLayers();
        void DestroyAllEntities();
        
        // Spawning helpers
        float CalculateNextEnemyPosition();
        // REMOVED: CalculateNextPickupPosition - pickups handled in GameplayState
        
        // Save/load progression
        void SaveProgression();
        void LoadProgression();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_LEVEL_MANAGER_H
