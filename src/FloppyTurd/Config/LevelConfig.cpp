#include "LevelConfig.h"

namespace GameCore {

    LevelConfig LevelConfigFactory::CreateLevel1Config() {
        LevelConfig config(1, "A Flop in the Park");
        
        // Set level-specific properties
        config.musicTrack = "Park";        // Base name for difficulty variants
        config.worldSpeed = 200.0f;
        config.baseScale = 8.0f;  // Scale up from 320x180
        
        AddParkLevelLayers(config);
        AddParkObstacles(config);
        AddParkEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel2Config() {
        LevelConfig config(2, "Home Sweet Home");
        
        // Set level-specific properties  
        config.musicTrack = "Sewer";       // Base name for difficulty variants
        config.worldSpeed = 220.0f;  // Slightly faster
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 1.2f;
        
        AddSewerLevelLayers(config);
        AddSewerObstacles(config);
        AddSewerEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel3Config() {
        LevelConfig config(3, "The Good, The Bad, and the Stinky");
        
        // Set level-specific properties
        config.musicTrack = "Desert";      // Base name for difficulty variants
        config.worldSpeed = 240.0f;  // Faster pace
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 1.4f;
        
        AddDesertLevelLayers(config);
        AddDesertObstacles(config);
        AddDesertEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel4Config() {
        LevelConfig config(4, "Polar Pandemonium");
        
        // Set level-specific properties
        config.musicTrack = "Snow";        // Base name for difficulty variants
        config.worldSpeed = 260.0f;  // Even faster
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 1.6f;
        
        AddSnowLevelLayers(config);
        AddSnowObstacles(config);
        AddSnowEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel5Config() {
        LevelConfig config(5, "Dung in the Dungeon");
        
        // Set level-specific properties
        config.musicTrack = "Castle";      // Base name for difficulty variants
        config.worldSpeed = 280.0f;  // High speed
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 1.8f;
        
        AddCastleLevelLayers(config);
        AddCastleObstacles(config);
        AddCastleEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel6Config() {
        LevelConfig config(6, "Curtains for Crap");
        
        // Set boss level properties
        config.musicTrack = "Boss";        // Base name for difficulty variants
        config.worldSpeed = 150.0f;  // Slower for boss fight
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 2.0f;
        
        AddBossLevelLayers(config);
        AddBossObstacles(config);
        AddBossEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::GetLevelConfig(int levelId) {
        switch (levelId) {
            case 1: return CreateLevel1Config();
            case 2: return CreateLevel2Config();
            case 3: return CreateLevel3Config();
            case 4: return CreateLevel4Config();
            case 5: return CreateLevel5Config();
            case 6: return CreateLevel6Config();
            default: return CreateLevel1Config(); // Default fallback
        }
    }

    std::string LevelConfigFactory::GetLevelName(int levelId) {
        switch (levelId) {
            case 1: return "A Flop in the Park";
            case 2: return "Home Sweet Home";
            case 3: return "The Good, The Bad, and the Stinky";
            case 4: return "Polar Pandemonium";
            case 5: return "Dung in the Dungeon";
            case 6: return "Curtains for Crap";
            default: return "Unknown Level";
        }
    }

    void LevelConfigFactory::AddParkLevelLayers(LevelConfig& config) {
        // Background layers from back to front based on original 320x180 design
        // Each layer has different scroll speeds for parallax effect
        
        // Back layer - slowest moving (furthest back)
        config.backgroundLayers.emplace_back("Level1BackLayerBackground", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        // Mid layer - medium speed
        config.backgroundLayers.emplace_back("Level1MidLayerBackground", 100.0f, 0.3f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        // Clouds - independent movement
        config.backgroundLayers.emplace_back("Level1Clouds", 75.0f, 0.2f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        // Front layer - fastest moving (closest to player, behind game objects)
        config.backgroundLayers.emplace_back("Level1FrontLayerBackground", 150.0f, 0.5f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
    }

    void LevelConfigFactory::AddSewerLevelLayers(LevelConfig& config) {
        // Sewer level layers - darker, underground feel
        config.backgroundLayers.emplace_back("SewerLevelBackground", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("SewerLevelPipes", 75.0f, 0.2f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("SewerLevelWater", 100.0f, 0.3f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
    }

    void LevelConfigFactory::AddDesertLevelLayers(LevelConfig& config) {
        // Desert level layers - hot, sandy environment
        config.backgroundLayers.emplace_back("DesertLevelSky", 40.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("DesertLevelMountains", 60.0f, 0.2f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("DesertLevelDunes", 90.0f, 0.3f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("DesertLevelCacti", 120.0f, 0.4f, 3);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
    }

    void LevelConfigFactory::AddSnowLevelLayers(LevelConfig& config) {
        // Snow level layers
        config.backgroundLayers.emplace_back("SnowLevelBackground", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("SnowLevelMountains", 75.0f, 0.2f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("SnowLevelBackTrees", 100.0f, 0.3f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("SnowLevelFrontTrees", 150.0f, 0.5f, 3);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        // Add snow particles layer
        config.backgroundLayers.emplace_back("Snowfall", 25.0f, 0.15f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
    }

    void LevelConfigFactory::AddCastleLevelLayers(LevelConfig& config) {
        // Castle level layers - more gothic/dungeon feel
        config.backgroundLayers.emplace_back("castlelevelbackgroundwall", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        config.backgroundLayers.emplace_back("castlelevelfloorceiling", 100.0f, 0.3f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
        
        // Add atmospheric elements
        config.backgroundLayers.emplace_back("curtains", 75.0f, 0.2f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
    }

    void LevelConfigFactory::AddBossLevelLayers(LevelConfig& config) {
        // Boss level uses enhanced castle layers
        AddCastleLevelLayers(config);
        
        // Add dramatic boss-specific elements
        config.backgroundLayers.emplace_back("BossBackground", 30.0f, 0.05f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.2f; // Slightly larger for dramatic effect
        config.backgroundLayers.back().repeatWidth = 320.0f * config.baseScale;
    }

    // OBSTACLE CONFIGURATIONS
    void LevelConfigFactory::AddParkObstacles(LevelConfig& config) {
        // Simple pipes - classic Flappy Bird style
        config.obstacles.emplace_back("PipeTop", 64.0f, 200.0f, 180.0f, 3.0f, config.worldSpeed, true);
        config.obstacles.emplace_back("PipeBottom", 64.0f, 200.0f, 180.0f, 3.0f, config.worldSpeed, true);
    }

    void LevelConfigFactory::AddSewerObstacles(LevelConfig& config) {
        // Toilets and plumbing fixtures
        config.obstacles.emplace_back("ToiletObstacle", 80.0f, 150.0f, 160.0f, 2.8f, config.worldSpeed, false);
        config.obstacles.emplace_back("SewerPipe", 70.0f, 180.0f, 170.0f, 3.2f, config.worldSpeed, true);
    }

    void LevelConfigFactory::AddDesertObstacles(LevelConfig& config) {
        // Outhouses and desert structures
        config.obstacles.emplace_back("Outhouse", 90.0f, 160.0f, 150.0f, 2.5f, config.worldSpeed, false);
        config.obstacles.emplace_back("CactiA", 60.0f, 120.0f, 200.0f, 3.5f, config.worldSpeed, false);
        config.obstacles.emplace_back("CactiB", 70.0f, 140.0f, 180.0f, 3.0f, config.worldSpeed, false);
    }

    void LevelConfigFactory::AddSnowObstacles(LevelConfig& config) {
        // Ice formations and snow structures
        config.obstacles.emplace_back("IcePipe", 65.0f, 190.0f, 170.0f, 2.8f, config.worldSpeed, true);
        config.obstacles.emplace_back("Snowball", 80.0f, 80.0f, 220.0f, 4.0f, config.worldSpeed, false);
    }

    void LevelConfigFactory::AddCastleObstacles(LevelConfig& config) {
        // Castle/dungeon obstacles
        config.obstacles.emplace_back("BrickWall", 100.0f, 180.0f, 140.0f, 2.2f, config.worldSpeed, true);
        config.obstacles.emplace_back("SpikeBall", 70.0f, 70.0f, 250.0f, 3.5f, config.worldSpeed, false);
    }

    void LevelConfigFactory::AddBossObstacles(LevelConfig& config) {
        // Boss-specific obstacles - fewer but more challenging
        config.obstacles.emplace_back("BossWall", 120.0f, 200.0f, 120.0f, 4.0f, config.worldSpeed * 0.8f, true);
    }

    // ENEMY CONFIGURATIONS
    void LevelConfigFactory::AddParkEnemies(LevelConfig& config) {
        // Simple flying birds
        config.enemies.emplace_back("BirdIdle", 40.0f, 30.0f, 150.0f, 5.0f, 1, "horizontal");
    }

    void LevelConfigFactory::AddSewerEnemies(LevelConfig& config) {
        // Sewer creatures
        config.enemies.emplace_back("RatCopterIdle", 50.0f, 40.0f, 120.0f, 4.5f, 2, "swoop");
        config.enemies.emplace_back("ToiletPaperFlap", 35.0f, 35.0f, 180.0f, 6.0f, 1, "vertical");
    }

    void LevelConfigFactory::AddDesertEnemies(LevelConfig& config) {
        // Desert wildlife
        config.enemies.emplace_back("BirdIdle", 40.0f, 30.0f, 160.0f, 4.8f, 1, "horizontal");
        // Could add desert-specific enemies like vultures or scorpions
    }

    void LevelConfigFactory::AddSnowEnemies(LevelConfig& config) {
        // Snow creatures
        config.enemies.emplace_back("SnowManIdle", 60.0f, 80.0f, 100.0f, 4.0f, 3, "horizontal");
        config.enemies.emplace_back("SnowManThrow", 60.0f, 80.0f, 130.0f, 3.5f, 2, "circular");
    }

    void LevelConfigFactory::AddCastleEnemies(LevelConfig& config) {
        // Castle/dungeon enemies
        config.enemies.emplace_back("BirdIdle", 40.0f, 30.0f, 140.0f, 3.8f, 2, "swoop");
        config.enemies.emplace_back("ToiletPaperFlap", 35.0f, 35.0f, 200.0f, 4.2f, 1, "vertical");
    }

    void LevelConfigFactory::AddBossEnemies(LevelConfig& config) {
        // Boss enemy - Rat King
        config.enemies.emplace_back("Ratking", 120.0f, 150.0f, 80.0f, 8.0f, 20, "boss_pattern");
        config.enemies.emplace_back("RatkingDeath", 120.0f, 150.0f, 0.0f, 0.0f, 1, "death");
    }

} // namespace GameCore
