#ifndef FLOPPY_TURD_LEVEL_MANAGER_H
#define FLOPPY_TURD_LEVEL_MANAGER_H

#include "../Config/LevelConfig.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Core/ECS.h"
#include <memory>
#include <vector>

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
        ~LevelManager();

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
        
        // Obstacle management
        Gnosis::Entity SpawnObstacle(const ObstacleConfig& config, float x, float y);
        void RemoveObstacle(Gnosis::Entity obstacle);
        std::vector<Gnosis::Entity> GetActiveObstacles() const { return m_activeObstacles; }
        
        // Enemy management  
        Gnosis::Entity SpawnEnemy(const EnemyConfig& config, float x, float y);
        void RemoveEnemy(Gnosis::Entity enemy);
        std::vector<Gnosis::Entity> GetActiveEnemies() const { return m_activeEnemies; }
        
        // Pickup management
        Gnosis::Entity SpawnPickup(const std::string& type, float x, float y);
        void RemovePickup(Gnosis::Entity pickup);
        std::vector<Gnosis::Entity> GetActivePickups() const { return m_activePickups; }
        
        // Cleanup
        void CleanupOffscreenEntities(float leftBoundary);
        
        // Level validation
        static bool ValidateLevelId(int levelId);
        static int GetMaxLevelId() { return LevelConfigFactory::GetLevelCount(); }
        
    private:
        // Core systems
        Gnosis::ECS* m_ecsSystem;
        
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
        std::vector<Gnosis::Entity> m_backgroundEntities;
        
        // Spawn timers
        float m_obstacleSpawnTimer;
        float m_enemySpawnTimer;
        float m_pickupSpawnTimer;
        
        // Spawn positions
        float m_lastObstacleX;
        float m_lastEnemyX;
        float m_lastPickupX;
        
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
