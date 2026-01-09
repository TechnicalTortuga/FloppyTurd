#include "EnemyConfigs.h"
#include "../../Engine/Core/GNLog.h"

namespace GameCore {

    std::unordered_map<std::string, EnemyConfig> EnemyConfigRegistry::s_enemyConfigs;
    bool EnemyConfigRegistry::s_initialized = false;

    void EnemyConfigRegistry::Initialize() {
        if (s_initialized) return;
        
        try {
            GN_LOG_DEBUG("EnemyConfigRegistry: Starting initialization...");
            // Register all enemy configurations
            s_enemyConfigs.emplace("BirdIdle", CreateBirdConfig());
            GN_LOG_DEBUG("EnemyConfigRegistry: Added BirdIdle config");
            
            auto toiletPaperConfig = CreateToiletPaperConfig();
            GN_LOG_INFO("EnemyConfigRegistry: ToiletPaper config created - useStateAnimation=" + std::to_string(toiletPaperConfig.useStateAnimation) + 
                       " animationStates.size=" + std::to_string(toiletPaperConfig.animationStates.size()));
            s_enemyConfigs.emplace("ToiletPaperFlap", toiletPaperConfig);
            GN_LOG_DEBUG("EnemyConfigRegistry: Added ToiletPaperFlap config");
            s_enemyConfigs.emplace("SnowManChill", CreateSnowManChillConfig());
            GN_LOG_DEBUG("EnemyConfigRegistry: Added SnowManChill config");
            s_enemyConfigs.emplace("SnowManGreen", CreateSnowManGreenConfig());
            GN_LOG_DEBUG("EnemyConfigRegistry: Added SnowManGreen config");
            s_enemyConfigs.emplace("SnowManChad", CreateSnowManChadConfig());
            GN_LOG_DEBUG("EnemyConfigRegistry: Added SnowManChad config");
            s_enemyConfigs.emplace("SnowManIdle", CreateSnowManThrowerConfig());
            GN_LOG_DEBUG("EnemyConfigRegistry: Added SnowManIdle config");
            s_enemyConfigs.emplace("RatCopterIdle", CreateRatCopterConfig());
            GN_LOG_DEBUG("EnemyConfigRegistry: Added RatCopterIdle config");
            s_enemyConfigs.emplace("Ratking", CreateRatKingConfig());
            GN_LOG_DEBUG("EnemyConfigRegistry: Added Ratking config");
            
            s_initialized = true;
            GN_LOG_INFO("EnemyConfigRegistry: Initialized " + std::to_string(s_enemyConfigs.size()) + " enemy configurations");
        } catch (const std::exception& e) {
            GN_LOG_ERROR("EnemyConfigRegistry: Exception during initialization: " + std::string(e.what()));
            s_initialized = false;
        } catch (...) {
            GN_LOG_ERROR("EnemyConfigRegistry: Unknown exception during initialization");
            s_initialized = false;
        }
    }

    const EnemyConfig& EnemyConfigRegistry::GetConfig(const std::string& textureId) {
        try {
            Initialize(); // Ensure initialized
            
            auto it = s_enemyConfigs.find(textureId);
            if (it != s_enemyConfigs.end()) {
                return it->second;
            }
            
            GN_LOG_ERROR("EnemyConfigRegistry: Configuration not found for enemy: " + textureId);
            static EnemyConfig defaultConfig("", 64.0f, 64.0f, 150.0f, 3.0f, 1, 64, 64, 1, 0.16f, true, "horizontal");
            return defaultConfig;
        } catch (const std::exception& e) {
            GN_LOG_ERROR("EnemyConfigRegistry: Exception in GetConfig: " + std::string(e.what()));
            static EnemyConfig defaultConfig("", 64.0f, 64.0f, 150.0f, 3.0f, 1, 64, 64, 1, 0.16f, true, "horizontal");
            return defaultConfig;
        } catch (...) {
            GN_LOG_ERROR("EnemyConfigRegistry: Unknown exception in GetConfig");
            static EnemyConfig defaultConfig("", 64.0f, 64.0f, 150.0f, 3.0f, 1, 64, 64, 1, 0.16f, true, "horizontal");
            return defaultConfig;
        }
    }

    bool EnemyConfigRegistry::HasConfig(const std::string& textureId) {
        try {
            Initialize();
            return s_enemyConfigs.find(textureId) != s_enemyConfigs.end();
        } catch (const std::exception& e) {
            GN_LOG_ERROR("EnemyConfigRegistry: Exception in HasConfig: " + std::string(e.what()));
            return false;
        } catch (...) {
            GN_LOG_ERROR("EnemyConfigRegistry: Unknown exception in HasConfig");
            return false;
        }
    }

    std::vector<EnemyConfig> EnemyConfigRegistry::GetConfigsForLevel(int levelId) {
        Initialize();
        std::vector<EnemyConfig> configs;
        
        try {
            switch (levelId) {
                case 2: // Sewer level
                    if (HasConfig("ToiletPaperFlap")) {
                        configs.push_back(GetConfig("ToiletPaperFlap"));
                    }
                    break;
                case 3: // Desert level
                    if (HasConfig("BirdIdle")) {
                        configs.push_back(GetConfig("BirdIdle"));
                    }
                    break;
                case 4: // Snow level
                    if (HasConfig("SnowManChill")) {
                        configs.push_back(GetConfig("SnowManChill"));
                    }
                    if (HasConfig("SnowManGreen")) {
                        configs.push_back(GetConfig("SnowManGreen"));
                    }
                    if (HasConfig("SnowManChad")) {
                        configs.push_back(GetConfig("SnowManChad"));
                    }
                    if (HasConfig("SnowManIdle")) {
                        configs.push_back(GetConfig("SnowManIdle"));
                    }
                    break;
                case 5: // Castle level
                    if (HasConfig("RatCopterIdle")) {
                        configs.push_back(GetConfig("RatCopterIdle"));
                    }
                    break;
                default:
                    // Other levels may not have enemies or use default configurations
                    break;
            }
        } catch (const std::exception& e) {
            GN_LOG_ERROR("EnemyConfigRegistry: Exception in GetConfigsForLevel: " + std::string(e.what()));
        } catch (...) {
            GN_LOG_ERROR("EnemyConfigRegistry: Unknown exception in GetConfigsForLevel");
        }
        
        return configs;
    }

    EnemyConfig EnemyConfigRegistry::CreateBirdConfig() {
        // Birds have 4 frames of 32x32 in horizontal spritesheet (128x32 total)
        // Use the individual frame size (32x32) for the sprite, not the total spritesheet width
        // ECHELON: Use "echelon" movement pattern for formation flying
        EnemyConfig config("BirdIdle", 32.0f, 32.0f, 6.0f, 150.0f, 3.0f, 1, 32, 32, 4, 0.16f, true, "echelon");

        // Enable StateAnimation for idle/hurt states
        config.useStateAnimation = true;
        config.initialState = "idle";

        // Configure animation states
        // Idle is 4 frames, hurt is 4 frames (0.4 seconds total)
        AnimationClip idleClip("BirdIdle", 32, 32, 4, 0.16f, true);
        AnimationClip hurtClip("BirdHurt", 32, 32, 4, 0.1f, false);

        config.animationStates.push_back({"idle", idleClip});
        config.animationStates.push_back({"hurt", hurtClip});

        // Subtle hovering behavior: 70% chance to hover, 30% static for echelon formation
        config.bobbingConfig.enabled = false; // Will be enabled probabilistically in spawn
        config.bobbingConfig.chanceToHover = 70.0f;
        config.bobbingConfig.baseSpeed = 1.2f;
        config.bobbingConfig.speedJitter = 0.05f; // -0.5 to +0.5 range
        config.bobbingConfig.amplitudeMin = 15.0f;
        config.bobbingConfig.amplitudeMax = 30.0f;
        
        // HITBOX OFFSET: Lower by 4 pixels for better centering
        config.hitboxOffsetY = 4.0f;
        
        // Custom hitbox radius - 30% smaller than default (12.8*0.7 = ~9)
        config.hitboxRadius = 9.0f;

        return config;
    }

    EnemyConfig EnemyConfigRegistry::CreateToiletPaperConfig() {
        // 4-frame flying animation at 64x64 (ToiletPaperFlap sheet has 4 frames)
        // GALAGA: Use "galaga" movement pattern
        EnemyConfig config("ToiletPaperFlap", 64.0f, 64.0f, 6.0f, 25.0f, 3.0f, 1, 64, 64, 4, 0.30f, true, "galaga");

        // Custom tight hitbox - 25% smaller than before (12 * 0.75 = 9)
        // Bird/RatCopter have ~12.8px radius. Reduced for tighter collisions.
        config.hitboxRadius = 9.0f;
        config.hitboxOffsetY = 8.0f; // Shift down to center on the roll

        // Enable StateAnimation for idle/hurt states
        config.useStateAnimation = true;
        config.initialState = "idle";

        // Configure animation states - separate spritesheets for idle and hurt
        AnimationClip idleClip("ToiletPaperFlap", 64, 64, 4, 0.30f, true);   // 4 frames for idle (300ms per frame - SLOWER)
        AnimationClip hurtClip("ToiletPaperHit", 64, 64, 4, 0.15f, false);   // 4 frames for hurt (150ms per frame = 0.6s total, fast but visible)
        config.animationStates.push_back({"idle", idleClip});
        config.animationStates.push_back({"hurt", hurtClip});

        // Constrained bobbing for top portion of screen (1/8 to 1/2)
        // Centered at 5/16 (31.25%) so amplitude of 3/16 (18.75%) reaches 1/8 min and 1/2 max
        config.bobbingConfig.enabled = true;
        config.bobbingConfig.baseSpeed = 1.0f;      // Slower wave movement
        config.bobbingConfig.speedJitter = 0.3f;     // Increased variation (0.7-1.3 range)
        config.bobbingConfig.amplitudeMin = 0.1875f; // 18.75% of screen height (center at 31.25% ±18.75% = 1/8 to 1/2 screen)
        config.bobbingConfig.amplitudeMax = 0.1875f; // Fixed amplitude for consistent range

        return config;
    }

    EnemyConfig EnemyConfigRegistry::CreateSnowManChillConfig() {
        // Static decorative snowman - NO bobbing
        EnemyConfig config("SnowManChill", 64.0f, 64.0f, 6.0f, 150.0f, 3.0f, 1, 64, 64, 1, 0.16f, true, "decorative");
        
        // Explicitly disable bobbing for static snowmen
        config.bobbingConfig.enabled = false;
        config.bobbingConfig.baseSpeed = 0.0f;
        config.bobbingConfig.amplitudeMin = 0.0f;
        config.bobbingConfig.amplitudeMax = 0.0f;
        
        return config;
    }

    EnemyConfig EnemyConfigRegistry::CreateSnowManGreenConfig() {
        // Static decorative snowman - NO bobbing
        EnemyConfig config("SnowManGreen", 64.0f, 64.0f, 6.0f, 150.0f, 3.0f, 1, 64, 64, 1, 0.16f, true, "decorative");
        
        // Explicitly disable bobbing for static snowmen
        config.bobbingConfig.enabled = false;
        config.bobbingConfig.baseSpeed = 0.0f;
        config.bobbingConfig.amplitudeMin = 0.0f;
        config.bobbingConfig.amplitudeMax = 0.0f;
        
        return config;
    }

    EnemyConfig EnemyConfigRegistry::CreateSnowManChadConfig() {
        // Static decorative snowman - NO bobbing
        EnemyConfig config("SnowManChad", 64.0f, 64.0f, 6.0f, 150.0f, 3.0f, 1, 64, 64, 1, 0.16f, true, "decorative");
        
        // Explicitly disable bobbing for static snowmen
        config.bobbingConfig.enabled = false;
        config.bobbingConfig.baseSpeed = 0.0f;
        config.bobbingConfig.amplitudeMin = 0.0f;
        config.bobbingConfig.amplitudeMax = 0.0f;
        
        return config;
    }

    EnemyConfig EnemyConfigRegistry::CreateSnowManThrowerConfig() {
        // Snowman thrower with StateAnimation (idle/throw states)
        EnemyConfig config("SnowManIdle", 64.0f, 64.0f, 6.0f, 150.0f, 3.0f, 1, 64, 64, 1, 0.25f, true, "snowman_thrower");
        
        // Enable StateAnimation
        config.useStateAnimation = true;
        config.initialState = "idle";
        
        // Configure animation states
        // Idle is just 1 frame (static), throw is 6 frames
        AnimationClip idleClip("SnowManIdle", 64, 64, 1, 0.25f, true);
        AnimationClip throwClip("SnowManThrow", 64, 64, 6, 0.15f, false);
        
        config.animationStates.push_back({"idle", idleClip});
        config.animationStates.push_back({"throw", throwClip});
        
        return config;
    }

    EnemyConfig EnemyConfigRegistry::CreateRatCopterConfig() {
        // RatCopter flying enemy - 32x32 sprite with 6 frames, flying behavior
        // RAT SWARM: Use "rat_swarm" movement pattern for staggered attacks
        EnemyConfig config("RatCopterIdle", 32.0f, 32.0f, 6.0f, 120.0f, 4.0f, 1, 32, 32, 6, 0.20f, true, "rat_swarm");
        
        // Set render layer to 10 (in front of boss which uses layers 7-9)
        config.renderLayer = 10;

        // Enable StateAnimation for idle/hurt states
        config.useStateAnimation = true;
        config.initialState = "idle";

        // Configure animation states
        // Idle is 6 frames, hurt is 6 frames (0.5 seconds total) - 32x32 frames
        AnimationClip idleClip("RatCopterIdle", 32, 32, 6, 0.20f, true);
        AnimationClip hurtClip("RatCopterHurt", 32, 32, 6, 0.083f, false);

        config.animationStates.push_back({"idle", idleClip});
        config.animationStates.push_back({"hurt", hurtClip});

        // Disable bobbing - rats have animated rotors in sprite, movement should be horizontal only
        config.bobbingConfig.enabled = false;
        config.bobbingConfig.baseSpeed = 0.0f;
        config.bobbingConfig.speedJitter = 0.0f;
        config.bobbingConfig.amplitudeMin = 0.0f;
        config.bobbingConfig.amplitudeMax = 0.0f;

        return config;
    }

    EnemyConfig EnemyConfigRegistry::CreateRatKingConfig() {
            // Rat King Boss - 128x128 sprite with complex boss behavior
    EnemyConfig config("Ratking", 128.0f, 128.0f, 8.0f, 100.0f, 1.0f, 20, 128, 128, 1, 0.16f, true, "boss_idle");

        // Boss-specific properties
        config.hitPoints = 20;  // Boss has 20 HP

    
        // Animation states for Rat King
        AnimationClip idleClip("Ratking", 128, 128, 1, 0.25f, true);                    // Single frame idle
        AnimationClip walkClip("RatkingWalk", 128, 128, 8, 0.12f, true);               // 8 frame walking
        AnimationClip hurtClip("RatkingHurt", 128, 128, 6, 0.15f, false);              // 6 frame hurt
        AnimationClip deathClip("RatkingDeath", 128, 128, 6, 0.20f, false);            // 6 frame death
        AnimationClip torsoClip("RatkingTorsoOnly", 128, 128, 7, 0.16f, false);        // 7 frame torso aiming
        AnimationClip backArmClip("RatkingBackArmOnly", 128, 128, 7, 0.16f, false);   // 7 frame back arm aiming
        AnimationClip tossArmClip("RatkingTossArmOnly", 128, 128, 7, 0.16f, false);   // 7 frame toss arm

        config.animationStates.push_back({"idle", idleClip});
        config.animationStates.push_back({"walking", walkClip});
        config.animationStates.push_back({"hurt", hurtClip});
        config.animationStates.push_back({"death", deathClip});
        config.animationStates.push_back({"aiming_torso", torsoClip});
        config.animationStates.push_back({"aiming_back_arm", backArmClip});
        config.animationStates.push_back({"throwing", tossArmClip});

        return config;
    }

} // namespace GameCore