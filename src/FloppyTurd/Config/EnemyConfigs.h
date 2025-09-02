#ifndef FLOPPY_TURD_ENEMY_CONFIGS_H
#define FLOPPY_TURD_ENEMY_CONFIGS_H

#include "LevelConfig.h"
#include <unordered_map>

namespace GameCore {

    /**
     * @brief Enemy Configuration Registry
     * 
     * Centralized configuration for all enemy types across all levels
     */
    class EnemyConfigRegistry {
    public:
        // Initialize all enemy configurations
        static void Initialize();
        
        // Get enemy configuration by texture ID
        static const EnemyConfig& GetConfig(const std::string& textureId);
        
        // Check if configuration exists
        static bool HasConfig(const std::string& textureId);
        
        // Get all configurations for a specific level
        static std::vector<EnemyConfig> GetConfigsForLevel(int levelId);
        
    private:
        static std::unordered_map<std::string, EnemyConfig> s_enemyConfigs;
        static bool s_initialized;
        
        // Configuration builders for each enemy type
        static EnemyConfig CreateBirdConfig();
        static EnemyConfig CreateToiletPaperConfig(); 
        static EnemyConfig CreateSnowManChillConfig();
        static EnemyConfig CreateSnowManGreenConfig();
        static EnemyConfig CreateSnowManChadConfig();
        static EnemyConfig CreateSnowManThrowerConfig();
        static EnemyConfig CreateRatCopterConfig();
        static EnemyConfig CreateRatKingConfig();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_ENEMY_CONFIGS_H