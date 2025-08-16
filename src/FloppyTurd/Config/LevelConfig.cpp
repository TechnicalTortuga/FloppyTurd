#include "LevelConfig.h"

namespace GameCore {

    LevelConfig LevelConfigFactory::CreateLevel1Config() {
        LevelConfig config(1, "A Flop in the Park");
        
        // Set level-specific properties
        config.musicTrack = "Park";        // Base name for difficulty variants
        config.worldSpeed = SpeedConstants::BASE_WORLD_SPEED;  // Use configurable speed constants
        config.baseScale = 8.0f;  // Keep player scale consistent
        
        // Level 1 feature gates: no enemies, NPCs, or pickups
        config.enableEnemies = false;
        config.enableNPCs = false;
        config.enablePickups = false;
        config.pickupSpawnRate = 0.0f;  // No pickups in Level 1
        
        AddParkLevelLayers(config);
        AddParkObstacles(config);
        // No enemies or pickups for Level 1 - focus on core mechanics
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel2Config() {
        LevelConfig config(2, "Home Sweet Home");
        
        // Set level-specific properties  
        config.musicTrack = "Sewer";       // Base name for difficulty variants
        config.worldSpeed = 220.0f;  // Slightly faster
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 1.2f;
        
        // Level 2 feature gates
        config.enableEnemies = true;
        config.enableNPCs = true;   // Janitor
        config.enablePickups = true;
        // Sewer: only GoldCoin (90%) and PooHeart (10%)
        config.pickupRatios.clear();
        config.pickupRatios.emplace_back("GoldCoin", 0.90f);
        config.pickupRatios.emplace_back("PooHeart", 0.10f);
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
        
        // Level 3 gates
        config.enableEnemies = true;
        config.enableNPCs = false;
        config.enablePickups = true;
        // Desert: GoldCoin (80%) and PooHeart (20%) - slightly more hearts for harder level
        config.pickupRatios.clear();
        config.pickupRatios.emplace_back("GoldCoin", 0.80f);
        config.pickupRatios.emplace_back("PooHeart", 0.20f);
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
        
        config.enableEnemies = true;
        config.enableNPCs = false;
        config.enablePickups = true;
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
        
        config.enableEnemies = true;
        config.enableNPCs = false;
        config.enablePickups = true;
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
        
        config.enableEnemies = true;
        config.enableNPCs = false;
        config.enablePickups = false; // Boss level: control pickups per design
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
        // Background layers from back to front using new 1024x512 assets
        // Each layer has different scroll speeds for parallax effect
        
        // Match sewer level behavior: 512px tall textures scaled to fill ~2556px iPhone height (~5x)
        // Our park textures are 1024x512; horizontally repeat, vertically scale ~5x via transform
        float backgroundScale = 5.0f;  // Approximately 2556/512
        
        // Back layer - slowest moving (furthest back)
        config.backgroundLayers.emplace_back("Level1BackLayerBackground", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        // Mid layer - medium speed
        config.backgroundLayers.emplace_back("Level1MidLayerBackground", 100.0f, 0.3f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        // Clouds - independent movement
        config.backgroundLayers.emplace_back("Level1Clouds", 75.0f, 0.2f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        // Front layer - fastest moving (closest to player, behind game objects)
        config.backgroundLayers.emplace_back("Level1FrontLayerBackground", 150.0f, 0.5f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
    }

    void LevelConfigFactory::AddSewerLevelLayers(LevelConfig& config) {
        // Sewer level layers - alternate between four large backgrounds as they wrap
        // SewerLarge textures are 512x512; scale to fill screen height (2556px)
        float backgroundScale = 5.0f;  // Approximately 2556/512

        // Single sewer band (512x512 variants cover the whole screen)
        float sewerScroll = config.worldSpeed * 0.28f; // slow band; Janitor will match this
        config.backgroundLayers.emplace_back("SewerLargeA", sewerScroll, 0.35f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 512.0f * backgroundScale;
        config.backgroundLayers.back().variantTextureIds = {"SewerLargeA", "SewerLargeB", "SewerLargeC", "SewerLargeD"};
    }

    void LevelConfigFactory::AddDesertLevelLayers(LevelConfig& config) {
        // Level 3 (Desert) background layers using actual asset IDs (1024x512)
        // Match sewer behavior: scale vertically to ~5x to fill iPhone height, tile horizontally
        float backgroundScale = 5.0f;  // Approximately 2556/512

        // Back layer - slowest
        config.backgroundLayers.emplace_back("Level3BackLayerBackground", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;

        // Mid layer - medium speed
        config.backgroundLayers.emplace_back("Level3MidLayerBackground", 90.0f, 0.3f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;

        // Front layer - fastest (still behind gameplay sprites)
        config.backgroundLayers.emplace_back("Level3FrontLayerBackground", 130.0f, 0.4f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
    }

    void LevelConfigFactory::AddSnowLevelLayers(LevelConfig& config) {
        // Snow level layers
        float backgroundScale = 2.66f;  // Separate scale for 1024x480 backgrounds
        
        config.backgroundLayers.emplace_back("SnowLevelBackground", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        config.backgroundLayers.emplace_back("SnowLevelMountains", 75.0f, 0.2f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        config.backgroundLayers.emplace_back("SnowLevelBackTrees", 100.0f, 0.3f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        config.backgroundLayers.emplace_back("SnowLevelFrontTrees", 150.0f, 0.5f, 3);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        // Add snow particles layer
        config.backgroundLayers.emplace_back("Snowfall", 25.0f, 0.15f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
    }

    void LevelConfigFactory::AddCastleLevelLayers(LevelConfig& config) {
        // Castle level layers - more gothic/dungeon feel
        float backgroundScale = 2.66f;  // Separate scale for 1024x480 backgrounds
        
        config.backgroundLayers.emplace_back("castlelevelbackgroundwall", 50.0f, 0.1f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        config.backgroundLayers.emplace_back("castlelevelfloorceiling", 100.0f, 0.3f, 1);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
        
        // Add atmospheric elements
        config.backgroundLayers.emplace_back("curtains", 75.0f, 0.2f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
    }

    void LevelConfigFactory::AddBossLevelLayers(LevelConfig& config) {
        // Boss level uses enhanced castle layers
        AddCastleLevelLayers(config);
        
        // Add dramatic boss-specific elements
        float backgroundScale = 2.66f;  // Separate scale for 1024x480 backgrounds
        config.backgroundLayers.emplace_back("BossBackground", 30.0f, 0.05f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.2f; // Slightly larger for dramatic effect
        config.backgroundLayers.back().repeatWidth = 1024.0f * backgroundScale;
    }

    // OBSTACLE CONFIGURATIONS
    void LevelConfigFactory::AddParkObstacles(LevelConfig& config) {
        // Level 1: Basic toilet pairs - simple static obstacles for park level
        // Using actual toilet assets with appropriate gap size for gameplay
        // Updated to match actual sprite dimensions: 64x256 pixels
        config.obstacles.emplace_back("TopToilet", "BottomToilet", 
                                     64.0f, 256.0f, 200.0f, 3.0f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
    }

    void LevelConfigFactory::AddSewerObstacles(LevelConfig& config) {
        // Level 2: Horizontal sewer segments (exact art size 192x64)
        config.obstacles.clear();
        config.obstacles.emplace_back("TopPipeWide", 192.0f, 64.0f, 0.0f, 2.8f, config.worldSpeed, false);
        config.obstacles.emplace_back("TopPipeWideBlue", 192.0f, 64.0f, 0.0f, 2.8f, config.worldSpeed, false);
        config.obstacles.emplace_back("BottomPipeWide", 192.0f, 64.0f, 0.0f, 2.8f, config.worldSpeed, false);
        config.obstacles.emplace_back("BottomPipeWideBlue", 192.0f, 64.0f, 0.0f, 2.8f, config.worldSpeed, false);
    }

    void LevelConfigFactory::AddDesertObstacles(LevelConfig& config) {
        // Level 3: Desert level with outhouses and ground hazards
        // Using actual texture dimensions: Outhouse=64x160, CactiA=64x48
        
        // Outhouse - single ground obstacle (actual size: 64x160)
        // Use small gap height to ensure proper spawning as single obstacle
        config.obstacles.emplace_back("Outhouse", "", 
                                     64.0f, 160.0f, 50.0f, 2.5f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
        
        // Ground cacti hazards - using actual texture sizes
        // Use small gap height to ensure proper spawning as single obstacle
        config.obstacles.emplace_back("CactiA", "", 
                                     64.0f, 48.0f, 50.0f, 3.5f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
        config.obstacles.emplace_back("CactiB", "", 
                                     64.0f, 48.0f, 50.0f, 3.0f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
        config.obstacles.emplace_back("CactiC", "", 
                                     64.0f, 48.0f, 50.0f, 3.2f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
    }

    void LevelConfigFactory::AddSnowObstacles(LevelConfig& config) {
        // Level 4: Snow level with ice toilets
        // Basic snow toilets for all difficulties
        config.obstacles.emplace_back("TopToiletSnow", "BottomToiletSnow", 
                                     65.0f, 190.0f, 170.0f, 2.8f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
        
        // Add oscillating gold toilets for "Rough" difficulty
        if (config.currentDifficulty == Difficulty::Rough) {
            config.obstacles.emplace_back("TopToiletGold", "BottomToiletGold", 
                                         65.0f, 190.0f, 160.0f, 3.5f, config.worldSpeed, 
                                         ToiletBehavior::OSCILLATE_VERTICAL, 1.5f, 50.0f);
        }
        
        // Single snowball obstacles for ground hazards
        config.obstacles.emplace_back("Snowball", "", 
                                     80.0f, 80.0f, 0.0f, 4.0f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
    }

    void LevelConfigFactory::AddCastleObstacles(LevelConfig& config) {
        // Level 5: Castle/dungeon obstacles
        // Standard brick wall pairs (like toilets but made of bricks)
        config.obstacles.emplace_back("TorchPillar", "", 
                                     100.0f, 180.0f, 0.0f, 2.2f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
        
        // Spike ball hazards - ground obstacles
        config.obstacles.emplace_back("SpikeBall", "", 
                                     70.0f, 70.0f, 0.0f, 3.5f, config.worldSpeed, 
                                     ToiletBehavior::STATIC, 0.0f, 0.0f);
                                     
        // Add oscillating gold toilets for variety and "Rough" difficulty
        if (config.currentDifficulty == Difficulty::Rough) {
            config.obstacles.emplace_back("TopToiletGold", "BottomToiletGold", 
                                         65.0f, 180.0f, 140.0f, 3.0f, config.worldSpeed, 
                                         ToiletBehavior::OSCILLATE_VERTICAL, 2.0f, 60.0f);
        }
    }

    void LevelConfigFactory::AddBossObstacles(LevelConfig& config) {
        // Boss-specific obstacles - fewer but more challenging
        config.obstacles.emplace_back("BossWall", 120.0f, 200.0f, 120.0f, 4.0f, config.worldSpeed * 0.8f, true);
    }

    // ENEMY CONFIGURATIONS
    void LevelConfigFactory::AddParkEnemies(LevelConfig& config) {
        // Level 1: No enemies or coins - simple introduction level
        // Player just needs to learn basic flying controls and navigate toilet obstacles
        // This keeps Level 1 focused on core mechanics without distractions
    }

    void LevelConfigFactory::AddSewerEnemies(LevelConfig& config) {
        // Level 2 should only spawn Toilet Paper enemies
        config.enemies.clear();
        // Make enemies spawn less frequently and animate a tad slower by default
        config.enemies.emplace_back("ToiletPaperFlap", 64.0f, 64.0f, 180.0f, 7.0f, 1, "horizontal");
    }

    void LevelConfigFactory::AddDesertEnemies(LevelConfig& config) {
        // Desert wildlife - birds with proper frame size
        config.enemies.emplace_back("BirdIdle", 32.0f, 32.0f, 160.0f, 4.8f, 1, "horizontal");
        // Could add desert-specific enemies like vultures or scorpions
    }

    void LevelConfigFactory::AddSnowEnemies(LevelConfig& config) {
        // Snow creatures
        config.enemies.emplace_back("SnowManIdle", 60.0f, 80.0f, 100.0f, 4.0f, 3, "horizontal");
        config.enemies.emplace_back("SnowManThrow", 60.0f, 80.0f, 130.0f, 3.5f, 2, "circular");
    }

    void LevelConfigFactory::AddCastleEnemies(LevelConfig& config) {
        // Castle/dungeon enemies - birds with proper frame size
        config.enemies.emplace_back("BirdIdle", 32.0f, 32.0f, 140.0f, 3.8f, 2, "swoop");
        config.enemies.emplace_back("ToiletPaperFlap", 35.0f, 35.0f, 200.0f, 4.2f, 1, "vertical");
    }

    void LevelConfigFactory::AddBossEnemies(LevelConfig& config) {
        // Boss enemy - Rat King
        config.enemies.emplace_back("Ratking", 120.0f, 150.0f, 80.0f, 8.0f, 20, "boss_pattern");
        config.enemies.emplace_back("RatkingDeath", 120.0f, 150.0f, 0.0f, 0.0f, 1, "death");
    }

} // namespace GameCore
