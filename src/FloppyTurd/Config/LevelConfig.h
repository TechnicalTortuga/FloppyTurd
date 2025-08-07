#ifndef FLOPPY_TURD_LEVEL_CONFIG_H
#define FLOPPY_TURD_LEVEL_CONFIG_H

#include <string>
#include <vector>

namespace GameCore {

    /**
     * @brief Game difficulty levels with poop puns!
     */
    enum class Difficulty {
        Runny = 0,      // Easy - "Runny" poop is easier to pass 
        Regular = 1,    // Normal - Standard difficulty
        Rough = 2       // Hard - "Rough" poop is harder to pass
    };
    
    /**
     * @brief Helper functions for difficulty
     */
    inline std::string DifficultyToString(Difficulty diff) {
        switch (diff) {
            case Difficulty::Runny: return "Runny";
            case Difficulty::Regular: return "Regular"; 
            case Difficulty::Rough: return "Rough";
            default: return "Regular";
        }
    }
    
    // CONFIGURABLE SPEED CONSTANTS - Easy to adjust from header
    namespace SpeedConstants {
        // Base world speeds for normal difficulty (user requested higher defaults)
        static constexpr float BASE_WORLD_SPEED = 180.0f;        // Increased from 100.0f
        static constexpr float BASE_BACKGROUND_SPEED = 120.0f;    // Background scrolling
        static constexpr float BASE_OBSTACLE_SPEED = 180.0f;     // Obstacle movement
        static constexpr float BASE_ENEMY_SPEED = 150.0f;        // Enemy movement
        
        // Difficulty multipliers (user requested: current speed perfect for easy)
        static constexpr float RUNNY_MULTIPLIER = 0.8f;    // 80% speed (slightly slower than current)
        static constexpr float REGULAR_MULTIPLIER = 1.0f;  // 100% speed (new higher base)
        static constexpr float ROUGH_MULTIPLIER = 1.5f;    // 150% speed (really fast)
    }
    
    inline float GetDifficultyMultiplier(Difficulty diff) {
        switch (diff) {
            case Difficulty::Runny: return SpeedConstants::RUNNY_MULTIPLIER;
            case Difficulty::Regular: return SpeedConstants::REGULAR_MULTIPLIER;
            case Difficulty::Rough: return SpeedConstants::ROUGH_MULTIPLIER;
            default: return SpeedConstants::REGULAR_MULTIPLIER;
        }
    }

    /**
     * @brief Background Layer Configuration
     * 
     * Defines a single parallax background layer with its texture and scrolling properties
     */
    struct BackgroundLayer {
        std::string textureId;
        float scrollSpeed;
        float depth;            // Layer depth (0.0 = furthest back, 1.0 = closest to player)
        int renderLayer;        // Render layer for sorting (0 = back, higher = front)
        float scaleMultiplier;  // Scale relative to base scale
        bool repeating;         // Whether this layer wraps around
        float repeatWidth;      // Width for wrapping (0 = auto-calculate from texture)
        
        BackgroundLayer(const std::string& texture, float speed, float layerDepth, int layer = 0)
            : textureId(texture)
            , scrollSpeed(speed)
            , depth(layerDepth)
            , renderLayer(layer)
            , scaleMultiplier(1.0f)
            , repeating(true)
            , repeatWidth(0.0f)
        {}
    };

    /**
     * @brief Toilet/Pipe behavior types for obstacles
     */
    enum class ToiletBehavior {
        STATIC,                 // Basic toilets - fixed position
        OSCILLATE_VERTICAL,     // Gold/Snow toilets - move up/down
        OSCILLATE_HORIZONTAL    // Sewer pipes - move left/right
    };

    /**
     * @brief Obstacle Configuration for each level
     */
    struct ObstacleConfig {
        std::string textureId;      // Base texture ID (for single obstacles) or top texture ID (for pairs)
        std::string bottomTextureId; // Bottom texture ID for toilet pairs (empty for single obstacles)
        float width;
        float height;
        float gapHeight;            // Height of gap for player to pass through
        float spawnRate;            // How often to spawn (seconds between spawns)
        float speed;                // Movement speed
        bool hasTopAndBottom;       // Whether obstacle has both top and bottom parts (like pipes)
        
        // Toilet-specific properties
        ToiletBehavior behavior;    // How this obstacle moves
        float oscillationSpeed;     // Speed of oscillation (radians per second)
        float oscillationRange;     // Range of oscillation (pixels)
        bool spawnAsPair;          // Whether to spawn as top/bottom pair
        
        ObstacleConfig(const std::string& texture, float w, float h, float gap, float rate, float spd, bool topBottom = true)
            : textureId(texture)
            , bottomTextureId("")
            , width(w), height(h), gapHeight(gap), spawnRate(rate), speed(spd), hasTopAndBottom(topBottom)
            , behavior(ToiletBehavior::STATIC)
            , oscillationSpeed(0.0f)
            , oscillationRange(0.0f)
            , spawnAsPair(topBottom)
        {}
        
        // Toilet pair constructor
        ObstacleConfig(const std::string& topTexture, const std::string& bottomTexture, 
                      float w, float h, float gap, float rate, float spd, 
                      ToiletBehavior behav = ToiletBehavior::STATIC, 
                      float oscSpeed = 0.0f, float oscRange = 0.0f)
            : textureId(topTexture)
            , bottomTextureId(bottomTexture)
            , width(w), height(h), gapHeight(gap), spawnRate(rate), speed(spd)
            , hasTopAndBottom(!bottomTexture.empty() && gap > 0.0f) // Only pairs if bottom texture and gap exist
            , behavior(behav)
            , oscillationSpeed(oscSpeed)
            , oscillationRange(oscRange)
            , spawnAsPair(!bottomTexture.empty() && gap > 0.0f) // Same logic as hasTopAndBottom
        {}
    };

    /**
     * @brief Enemy Configuration for each level
     */
    struct EnemyConfig {
        std::string textureId;
        float width;
        float height;
        float speed;
        float spawnRate;
        int hitPoints;
        std::string movementPattern;    // "horizontal", "vertical", "circular", "swoop"
        
        EnemyConfig(const std::string& texture, float w, float h, float spd, float rate, int hp, const std::string& pattern = "horizontal")
            : textureId(texture), width(w), height(h), speed(spd), spawnRate(rate), hitPoints(hp), movementPattern(pattern) {}
    };

    /**
     * @brief Level Configuration
     * 
     * Data-driven level configuration instead of inheritance-based levels
     */
    struct LevelConfig {
        int levelId;
        std::string levelName;
        std::string musicTrack;         // Base music track name (without difficulty suffix)
        Difficulty currentDifficulty;   // Current game difficulty
        
        // Background layers (ordered from back to front)
        std::vector<BackgroundLayer> backgroundLayers;
        
        // Gameplay elements
        std::vector<ObstacleConfig> obstacles;
        std::vector<EnemyConfig> enemies;
        
        // Scaling and camera settings
        float baseScale;            // Base scale factor (originally designed for 320x180)
        float worldSpeed;           // Speed at which world moves
        float cameraFollowSpeed;    // Camera follow smoothing
        
        // Level-specific settings
        float gravity;
        float terminalVelocity;
        float jumpForce;
        
        // Spawn rates and difficulty
        float obstacleSpawnRate;
        float enemySpawnRate;
        float pickupSpawnRate;
        float difficultyMultiplier;
        
        LevelConfig(int id, const std::string& name)
            : levelId(id)
            , levelName(name)
            , musicTrack("")
            , currentDifficulty(Difficulty::Regular)
            , baseScale(8.0f)        // Scale up from 320x180 to modern resolution
            , worldSpeed(200.0f)
            , cameraFollowSpeed(1.0f)
            , gravity(980.0f)
            , terminalVelocity(500.0f)
            , jumpForce(300.0f)
            , obstacleSpawnRate(2.0f)
            , enemySpawnRate(3.0f)
            , pickupSpawnRate(5.0f)
            , difficultyMultiplier(1.0f)
        {}
        
        /**
         * @brief Get the music file path based on current difficulty
         */
        std::string GetMusicForDifficulty() const {
            if (musicTrack.empty()) {
                return "";
            }
            
            switch (currentDifficulty) {
                case Difficulty::Runny:
                    return musicTrack + "Slow.mp3";     // Easy = Slow music
                case Difficulty::Regular:
                    return musicTrack + "Level.mp3";    // Normal = Regular music 
                case Difficulty::Rough:
                    return musicTrack + "Fast.mp3";     // Hard = Fast music
                default:
                    return musicTrack + "Level.mp3";
            }
        }
        
        /**
         * @brief Apply difficulty settings to this level config
         */
        void ApplyDifficulty(Difficulty difficulty) {
            currentDifficulty = difficulty;
            float diffMultiplier = GetDifficultyMultiplier(difficulty);
            
            // UPDATED: Use new configurable speed constants with higher base speeds
            worldSpeed = SpeedConstants::BASE_WORLD_SPEED * diffMultiplier;
            obstacleSpawnRate = 2.0f / diffMultiplier;  // Faster spawning = harder
            enemySpawnRate = 3.0f / diffMultiplier;
            pickupSpawnRate = 5.0f / diffMultiplier;
            difficultyMultiplier = diffMultiplier;
            
            // FIXED: Update background layer scroll speeds with proper parallax multipliers
            // Preserve the original parallax effect by using depth-based multipliers
            for (auto& layer : backgroundLayers) {
                // Calculate parallax multiplier based on layer depth (0.0 = furthest back, 1.0 = closest)
                // Further layers move slower, closer layers move faster
                float parallaxMultiplier = 0.3f + (layer.depth * 0.7f); // Range: 0.3x to 1.0x
                layer.scrollSpeed = SpeedConstants::BASE_BACKGROUND_SPEED * parallaxMultiplier * diffMultiplier;
            }
            
            // Update obstacle and enemy speeds with proper base speeds
            for (auto& obstacle : obstacles) {
                obstacle.speed = SpeedConstants::BASE_OBSTACLE_SPEED * diffMultiplier;
            }
            for (auto& enemy : enemies) {
                enemy.speed = SpeedConstants::BASE_ENEMY_SPEED * diffMultiplier;
            }
        }
    };

    /**
     * @brief Level Configuration Factory
     * 
     * Creates level configurations for all 6 levels plus boss
     */
    class LevelConfigFactory {
    public:
        // All 6 regular levels
        static LevelConfig CreateLevel1Config();   // Park - "A Flop in the Park"
        static LevelConfig CreateLevel2Config();   // Sewer - "Home Sweet Home"  
        static LevelConfig CreateLevel3Config();   // Desert - "The Good, The Bad, and the Stinky"
        static LevelConfig CreateLevel4Config();   // Snow - "Polar Pandemonium"
        static LevelConfig CreateLevel5Config();   // Castle - "Dung in the Dungeon"
        static LevelConfig CreateLevel6Config();   // Boss - "Curtains for Crap"
        
        // Get config by ID (1-6)
        static LevelConfig GetLevelConfig(int levelId);
        
        // Get level count
        static int GetLevelCount() { return 6; }
        
        // Get level name by ID
        static std::string GetLevelName(int levelId);
        
    private:
        // Background layer helpers
        static void AddParkLevelLayers(LevelConfig& config);
        static void AddSewerLevelLayers(LevelConfig& config);
        static void AddDesertLevelLayers(LevelConfig& config);
        static void AddSnowLevelLayers(LevelConfig& config);
        static void AddCastleLevelLayers(LevelConfig& config);
        static void AddBossLevelLayers(LevelConfig& config);
        
        // Obstacle configuration helpers
        static void AddParkObstacles(LevelConfig& config);
        static void AddSewerObstacles(LevelConfig& config);
        static void AddDesertObstacles(LevelConfig& config);
        static void AddSnowObstacles(LevelConfig& config);
        static void AddCastleObstacles(LevelConfig& config);
        static void AddBossObstacles(LevelConfig& config);
        
        // Enemy configuration helpers
        static void AddParkEnemies(LevelConfig& config);
        static void AddSewerEnemies(LevelConfig& config);
        static void AddDesertEnemies(LevelConfig& config);
        static void AddSnowEnemies(LevelConfig& config);
        static void AddCastleEnemies(LevelConfig& config);
        static void AddBossEnemies(LevelConfig& config);
    };

} // namespace GameCore

#endif // FLOPPY_TURD_LEVEL_CONFIG_H
