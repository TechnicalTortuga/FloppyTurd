#include "LevelConfig.h"
#include "EnemyConfigs.h"

namespace GameCore {

    LevelConfig LevelConfigFactory::CreateLevel1Config() {
        LevelConfig config(1, "A Flop in the Park");
        
        // Set level-specific properties
        config.musicTrack = "Park";        // Base name for difficulty variants
        config.worldSpeed = SpeedConstants::BASE_WORLD_SPEED;  // Use configurable speed constants
        config.baseScale = 8.0f;  // Keep player scale consistent
        
        // Level 1 feature gates: no enemies, NPCs, pickups, or shooting
        config.enableEnemies = false;
        config.enableNPCs = false;
        config.enablePickups = false;
        config.shootingEnabled = false;  // Disable shooting in Level 1 - whole screen is jump zone
        config.pickupSpawnRate = 0.0f;  // No pickups in Level 1
        
        AddParkLevelLayers(config);
        // No enemies or pickups for Level 1 - focus on core mechanics
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel2Config() {
        LevelConfig config(2, "Home Sweet Home");
        
        // Set level-specific properties  
        config.musicTrack = "Sewer";       // Base name for difficulty variants
        config.worldSpeed = SpeedConstants::BASE_WORLD_SPEED;  // Use configurable speed constants
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
        AddSewerEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel3Config() {
        LevelConfig config(3, "The Good, The Bad, and the Stinky");
        
        // Set level-specific properties
        config.musicTrack = "Desert";      // Base name for difficulty variants
        config.worldSpeed = SpeedConstants::BASE_WORLD_SPEED;  // Use configurable speed constants
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
        AddDesertEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel4Config() {
        LevelConfig config(4, "Polar Pandemonium");
        
        // Set level-specific properties
        config.musicTrack = "Snow";        // Base name for difficulty variants
        config.worldSpeed = SpeedConstants::BASE_WORLD_SPEED;  // Use configurable speed constants
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 1.6f;
        
        config.enableEnemies = true;
        config.enableNPCs = false;
        config.enablePickups = true;
        
        // Pickup ratios: 70% gold coins, 20% hearts, 10% blue coins
        config.pickupRatios.push_back(PickupRatio("GoldCoin", 0.7f));
        config.pickupRatios.push_back(PickupRatio("PooHeart", 0.2f));
        config.pickupRatios.push_back(PickupRatio("BlueCoin", 0.1f));
        
        AddSnowLevelLayers(config);
        AddSnowEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel5Config() {
        LevelConfig config(5, "Dung in the Dungeon");
        
        // Set level-specific properties
        config.musicTrack = "Castle";      // Base name for difficulty variants
        config.worldSpeed = SpeedConstants::BASE_WORLD_SPEED;  // Use configurable speed constants
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 1.8f;
        
        config.enableEnemies = true;
        config.enableNPCs = false;
        config.enablePickups = true;
        
        // Castle pickup mix: Gold, Blue, Red coins and Hearts for harder level
        config.pickupRatios.emplace_back("GoldCoin", 0.60f);   // 60% gold
        config.pickupRatios.emplace_back("BlueCoin", 0.20f);   // 20% blue
        config.pickupRatios.emplace_back("PooHeart", 0.15f);   // 15% hearts (important for harder level!)
        config.pickupRatios.emplace_back("RedCoin", 0.05f);    // 5% red (rare!)
        
        AddCastleLevelLayers(config);
        AddCastleEnemies(config);
        
        return config;
    }

    LevelConfig LevelConfigFactory::CreateLevel6Config() {
        LevelConfig config(6, "Curtains for Crap");

        // Boss level properties
        config.musicTrack = "Boss";        // Base name for difficulty variants
        config.worldSpeed = 0.0f;          // Boss level: static background, no world movement
        config.baseScale = 8.0f;
        config.difficultyMultiplier = 2.0f;

        // Enable landscape mode for boss level
        config.forceLandscape = true;      // Boss level requires landscape orientation
        config.landscapeWidth = 320.0f;    // 320x180 base dimensions
        config.landscapeHeight = 180.0f;
        config.widthPriorityScaling = true; // Width stretches to fit screen

        config.enableEnemies = true;
        config.enableNPCs = false;
        config.enablePickups = false; // Boss level: control pickups per design
        config.shootingEnabled = true; // Boss level should have shooting enabled
        AddBossLevelLayers(config);
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
        // Background layers from back to front using 1024x512 assets
        // Each layer has different scroll speeds for parallax effect

        // Consistent scaling: 1024x512 textures scaled to fill iPhone 16 height (2556px)
        // The LevelManager calculates heightScale = 2556/512 ≈ 5.0 automatically
        // scaleMultiplier should be 1.0 (no additional scaling needed)

        // 🎯 SIMPLIFIED: Background layers with automatic scaling and positioning
        // No more hardcoded scaleMultiplier or repeatWidth - these are calculated from actual texture dimensions

        // Back layer - slowest moving (furthest back)
        config.backgroundLayers.emplace_back("Level1BackLayerBackground.png", 50.0f, 0.1f, 0);

        // Mid layer - medium speed
        config.backgroundLayers.emplace_back("Level1MidLayerBackground.png", 100.0f, 0.3f, 1);

        // Clouds - independent movement (adjusted to layer 0 to avoid overlap with mid layer)
        config.backgroundLayers.emplace_back("Level1Clouds.png", 75.0f, 0.2f, 0);

        // Front layer - fastest moving (closest to player, behind game objects)
        config.backgroundLayers.emplace_back("Level1FrontLayerBackground.png", 150.0f, 0.5f, 2);
    }

    void LevelConfigFactory::AddSewerLevelLayers(LevelConfig& config) {
        // 🎯 SIMPLIFIED: Sewer level with automatic scaling and variant cycling
        // No more hardcoded values - everything calculated from actual texture dimensions

        float sewerScroll = config.worldSpeed * 0.28f; // slow band; Janitor will match this
        config.backgroundLayers.emplace_back("SewerLargeA.png", sewerScroll, 0.35f, 1);

        // Variants: Automatically cycle through different sewer backgrounds on wrap
        config.backgroundLayers.back().variantTextureIds = {"SewerLargeA.png", "SewerLargeB.png", "SewerLargeC.png", "SewerLargeD.png"};
    }

    void LevelConfigFactory::AddDesertLevelLayers(LevelConfig& config) {
        // 🎯 SIMPLIFIED: Desert level with automatic scaling and positioning
        // No more hardcoded values - everything calculated from actual texture dimensions

        // Back layer - slowest (layer 0)
        config.backgroundLayers.emplace_back("Level3BackLayerBackground.png", 50.0f, 0.1f, 0);

        // Mid layer - medium speed (layer 1) 
        config.backgroundLayers.emplace_back("Level3MidLayerBackground.png", 90.0f, 0.3f, 1);

        // Front layer - fastest (layer 2) - behind obstacles (layers 3-5)
        config.backgroundLayers.emplace_back("Level3FrontLayerBackground.png", 130.0f, 0.4f, 2);
    }

        void LevelConfigFactory::AddSnowLevelLayers(LevelConfig& config) {
        // 🎯 SIMPLIFIED: Snow level with automatic scaling and positioning
        // No more hardcoded values - everything calculated from actual texture dimensions

        // Back layer - furthest background
        config.backgroundLayers.emplace_back("SnowLevelBackLayerBackground.png", 50.0f, 0.1f, 0);

        // Mid layer - mountains and middle ground
        config.backgroundLayers.emplace_back("SnowLevelMidLayerBackground.png", 75.0f, 0.2f, 1);

        // Front layer - trees and foreground elements  
        config.backgroundLayers.emplace_back("SnowLevelFrontLayerBackground.png", 150.0f, 0.3f, 2);

        // Front trees layer - behind pipes/obstacles (layer 3), same layer as front background
        config.backgroundLayers.emplace_back("SnowLevelFrontLayerTrees.png", 150.0f, 0.5f, 2);
    }

    void LevelConfigFactory::AddCastleLevelLayers(LevelConfig& config) {
        // 🎯 SIMPLIFIED: Castle level with automatic scaling and positioning
        // No more hardcoded values - everything calculated from actual texture dimensions

        // Back layer - castle background
        config.backgroundLayers.emplace_back("castlebacklayerbackground.png", 200.0f, 1.0f, 0);

        // Note: Curtains are now spawned as decorative obstacles positioned with toilet pairs
        // Note: Paintings, chandeliers, floor torches, and torch pillars are now handled by ObstacleSystem
        // as decorative obstacles rather than background layers to support proper positioning and animation
    }

    void LevelConfigFactory::AddBossLevelLayers(LevelConfig& config) {
        // Boss level with static background - layered from bottom up as specified:
        // 1. Boss Background (bottom layer)
        // 2. Boss floor / boss walls / pillar (middle layer)
        // 3. Screen curtains (top layer)

        // Layer 0: Boss Background (bottom)
        // Use mobile-specific background for better landscape support
        config.backgroundLayers.emplace_back("BossLevelBackgroundMobile.png", 0.0f, 0.0f, 0);
        config.backgroundLayers.back().scaleMultiplier = 1.0f; // Will be scaled to fit landscape mode

        // Layer 1: Boss Floor (middle) - precise positioning handled by LevelManager
        config.backgroundLayers.emplace_back("BossFloor.png", 0.0f, 0.0f, 1); // Precise offset calculated by LevelManager
        config.backgroundLayers.back().scaleMultiplier = 1.0f;

        // Layer 2: Boss Walls (middle)
        config.backgroundLayers.emplace_back("BossWalls.png", 0.0f, 0.0f, 2);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;

        // Layer 3: Screen Curtains (top) - position on sides for landscape
        config.backgroundLayers.emplace_back("screenCurtains.png", 0.0f, 0.0f, 3);
        config.backgroundLayers.back().scaleMultiplier = 1.0f;

        // Note: Animated pillar (15 frames) will be handled as a separate animated entity
        // since it needs proper animation support beyond static background layers
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
        // Use EnemyConfigRegistry to get properly configured enemies
        if (EnemyConfigRegistry::HasConfig("ToiletPaperFlap")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("ToiletPaperFlap"));
        }
    }

    void LevelConfigFactory::AddDesertEnemies(LevelConfig& config) {
        // Desert wildlife - birds with proper frame size
        config.enemies.clear();
        // Use EnemyConfigRegistry to get properly configured enemies
        if (EnemyConfigRegistry::HasConfig("BirdIdle")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("BirdIdle"));
        }
        // Could add desert-specific enemies like vultures or scorpions
    }

    void LevelConfigFactory::AddSnowEnemies(LevelConfig& config) {
        // Snow creatures - regular snowmen are decorative (64x64 sprites)
        config.enemies.clear();
        // Use EnemyConfigRegistry to get properly configured enemies
        if (EnemyConfigRegistry::HasConfig("SnowManChill")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("SnowManChill"));
        }
        if (EnemyConfigRegistry::HasConfig("SnowManGreen")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("SnowManGreen"));
        }
        if (EnemyConfigRegistry::HasConfig("SnowManChad")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("SnowManChad"));
        }
        // Red snowman is the actual enemy - throws snowballs (64x64 sprite, 6-frame throw animation)
        if (EnemyConfigRegistry::HasConfig("SnowManIdle")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("SnowManIdle"));
        }
    }

    void LevelConfigFactory::AddCastleEnemies(LevelConfig& config) {
        // Castle/dungeon enemies - only RatCopters with better positioning
        config.enemies.clear();
        // Use EnemyConfigRegistry to get properly configured enemies
        if (EnemyConfigRegistry::HasConfig("RatCopterIdle")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("RatCopterIdle"));
        }
    }

    void LevelConfigFactory::AddBossEnemies(LevelConfig& config) {
        // Boss enemy - Rat King (128x128 frame dimensions, 8x scale to match player)
        config.enemies.clear();
        // Use EnemyConfigRegistry to get properly configured enemies
        if (EnemyConfigRegistry::HasConfig("Ratking")) {
            config.enemies.push_back(EnemyConfigRegistry::GetConfig("Ratking"));
        }
        // Note: RatkingDeath is a state animation, not a separate enemy entity

        // Note: Pillar is now handled as a decorative obstacle in ObstacleSystem::AddBossLevelDecorations()
    }

} // namespace GameCore
