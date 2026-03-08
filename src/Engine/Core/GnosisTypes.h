#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace Gnosis {

    // ============================================================================
    // Core Math Types
    // ============================================================================
    
    struct GNVector2 {
        float x, y;
        
        GNVector2() : x(0.0f), y(0.0f) {}
        GNVector2(float x, float y) : x(x), y(y) {}
        
        GNVector2 operator+(const GNVector2& other) const {
            return GNVector2(x + other.x, y + other.y);
        }
        
        GNVector2 operator-(const GNVector2& other) const {
            return GNVector2(x - other.x, y - other.y);
        }
        
        GNVector2 operator*(float scalar) const {
            return GNVector2(x * scalar, y * scalar);
        }
        
        GNVector2& operator+=(const GNVector2& other) {
            x += other.x;
            y += other.y;
            return *this;
        }
        
        float Length() const {
            return std::sqrt(x * x + y * y);
        }
        
        GNVector2 Normalized() const {
            float len = Length();
            if (len > 0.0f) {
                return GNVector2(x / len, y / len);
            }
            return GNVector2(0.0f, 0.0f);
        }
    };

    // ============================================================================
    // Math Utility Functions
    // ============================================================================

    inline GNVector2 Vector2Subtract(const GNVector2& a, const GNVector2& b) {
        return GNVector2(a.x - b.x, a.y - b.y);
    }

    inline GNVector2 Vector2Add(const GNVector2& a, const GNVector2& b) {
        return GNVector2(a.x + b.x, a.y + b.y);
    }

    inline GNVector2 Vector2Scale(const GNVector2& v, float scale) {
        return GNVector2(v.x * scale, v.y * scale);
    }

    inline float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    inline int Clamp(int value, int min, int max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    inline float SmoothAngleLerp(float current, float target, float amount) {
        float diff = std::fmod((target - current + 540.0f), 360.0f) - 180.0f;
        return current + diff * amount;
    }
    
    struct GNColor {
        uint8_t r, g, b, a;
        
        GNColor() : r(255), g(255), b(255), a(255) {}
        GNColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
        
        // Common colors
        static const GNColor WHITE;
        static const GNColor BLACK;
        static const GNColor RED;
        static const GNColor GREEN;
        static const GNColor BLUE;
        static const GNColor YELLOW;
        static const GNColor TRANSPARENT;
    };
    
    struct GNRectangle {
        float x, y, width, height;
        
        GNRectangle() : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}
        GNRectangle(float x, float y, float width, float height) 
            : x(x), y(y), width(width), height(height) {}
        
        bool Contains(const GNVector2& point) const {
            return point.x >= x && point.x <= x + width &&
                   point.y >= y && point.y <= y + height;
        }
        
        bool Intersects(const GNRectangle& other) const {
            return x < other.x + other.width &&
                   x + width > other.x &&
                   y < other.y + other.height &&
                   y + height > other.y;
        }
        
        GNVector2 Center() const {
            return GNVector2(x + width * 0.5f, y + height * 0.5f);
        }
    };
    
    // ============================================================================
    // Resource Handle Types
    // ============================================================================
    
    using GNTextureHandle = uint32_t;
    using GNAudioHandle = uint32_t;
    using GNFontHandle = uint32_t;
    using GNShaderHandle = uint32_t;
    using GNRenderTargetHandle = uint32_t;
    using GNSoundHandle = uint32_t;
    using GNMusicHandle = uint32_t;
    
    // Invalid handle constants
    const GNTextureHandle INVALID_TEXTURE_HANDLE = 0;
    const GNAudioHandle INVALID_AUDIO_HANDLE = 0;
    const GNFontHandle INVALID_FONT_HANDLE = 0;
    const GNShaderHandle INVALID_SHADER_HANDLE = 0;
    const GNRenderTargetHandle INVALID_RENDER_TARGET_HANDLE = 0;
    const GNSoundHandle INVALID_SOUND_HANDLE = 0;
    const GNMusicHandle INVALID_MUSIC_HANDLE = 0;
    
    // ============================================================================
    // Game-Specific Types
    // ============================================================================
    
    using HatID = uint16_t;
    using SkillID = uint16_t;
    using Entity = uint32_t;
    
    const Entity INVALID_ENTITY = 0;
    
    struct Hat {
        HatID id;
        std::string name;
        int price;
        GNTextureHandle texture;
        
        Hat() : id(0), price(0), texture(INVALID_TEXTURE_HANDLE) {}
        Hat(HatID id, const std::string& name, int price, GNTextureHandle texture)
            : id(id), name(name), price(price), texture(texture) {}
    };
    
    struct Skill {
        SkillID id;
        std::string name;
        std::string description;
        
        Skill() : id(0) {}
        Skill(SkillID id, const std::string& name, const std::string& description)
            : id(id), name(name), description(description) {}
    };
    
    // ============================================================================
    // Enumerations
    // ============================================================================
    
    enum class InputAction {
        JUMP,
        SHOOT,
        PAUSE,
        MENU_CONFIRM,
        MENU_BACK,
        MENU_UP,
        MENU_DOWN,
        MENU_LEFT,
        MENU_RIGHT
    };
    
    enum class EventType {
        COLLISION,
        SCORE_INCREASED,
        PLAYER_DIED,
        LEVEL_COMPLETE,
        JUMP_PERFORMED,
        ENEMY_KILLED,
        PICKUP_COLLECTED,
        SKILL_ACTIVATED,
        HAT_EQUIPPED
    };
    
    enum class GameEvent {
        LEVEL_START,
        DEATH,
        PURCHASE,
        SKILL_USE,
        AD_WATCHED,
        ACHIEVEMENT_UNLOCKED
    };
    
    enum class AssetPriority {
        CRITICAL,
        HIGH,
        MEDIUM,
        LOW
    };
    
    enum class LayerType {
        BACKGROUND,
        MIDGROUND,
        FOREGROUND,
        EFFECTS,
        POST_PROCESSING,
        UI
    };
    
    enum class PowerUpType {
        COIN,
        HEART,
        SKILL_POINT,
        TEMPORARY_SHIELD
    };
    
    enum class PickupEffectType {
        HEAL,
        SCORE_BOOST,
        COIN_MAGNET,
        INVINCIBILITY
    };
    
    enum class PipeType {
        VERTICAL_TOILET_PAIR,
        HORIZONTAL_SEWER,
        SINGLE_OUTHOUSE,
        SNOW_TOILET_PAIR,
        GOLD_TOILET_PAIR
    };
    
    enum class SkillType {
        PASSIVE,
        ACTIVE
    };
    
    enum class SkillID_Enum : SkillID {
        COIN_MAGNET = 1,
        SCORE_MULTIPLIER = 2,
        DAMAGE_BOOST = 3,
        RAPID_FIRE = 4,
        SHIELD_BURST = 5
    };
    
    enum class EnemyType {
        RATCOPTER,
        SNOWMAN,
        SPIKEBALL,
        BOSS,
        BIRD,
        TOILET_PAPER,
        RAT_KING
    };
    
    enum class HatType {
        NONE,
        COWBOY,
        PIRATE,
        WIZARD,
        CROWN,
        SANTA
    };
    
    enum class ProjectileType {
        TOILET_PAPER,
        SNOWBALL,
        POOP_BALL,
        LARGE_POOP_BALL  // For big turd form
    };
    
    enum class PlayerState {
        IDLE,
        JUMPING,
        SHOOTING,
        HURT,
        DEAD
    };
    
    enum class BossState {
        IDLE,
        WALKING,
        PREPARING_ATTACK,
        ATTACKING,
        HURT,
        DEATH
    };
    
    enum class EnemyAIState {
        IDLE,
        MOVING,
        ATTACKING,
        FLEEING
    };
    
    enum class KeyCode {
        SPACE = 32,
        ENTER = 257,
        TAB = 258,
        BACKSPACE = 259,
        ESCAPE = 256,
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
    };
    
    enum class MouseButton {
        LEFT = 0,
        RIGHT = 1,
        MIDDLE = 2
    };
    
    enum class GamepadButton {
        A = 0,
        B = 1,
        X = 2,
        Y = 3,
        LEFT_BUMPER = 4,
        RIGHT_BUMPER = 5,
        BACK = 6,
        START = 7,
        LEFT_STICK = 8,
        RIGHT_STICK = 9,
        DPAD_UP = 10,
        DPAD_DOWN = 11,
        DPAD_LEFT = 12,
        DPAD_RIGHT = 13
    };
    
    enum class GamepadAxis {
        LEFT_X = 0,
        LEFT_Y = 1,
        RIGHT_X = 2,
        RIGHT_Y = 3,
        LEFT_TRIGGER = 4,
        RIGHT_TRIGGER = 5
    };
    
    enum class GestureType {
        TAP = 1,
        DOUBLE_TAP = 2,
        HOLD = 4,
        DRAG = 8,
        SWIPE_RIGHT = 16,
        SWIPE_LEFT = 32,
        SWIPE_UP = 64,
        SWIPE_DOWN = 128,
        PINCH_IN = 256,
        PINCH_OUT = 512
    };
    
    enum class AudioEffect {
        REVERB,
        ECHO,
        DISTORTION,
        CHORUS,
        FLANGER,
        LOW_PASS_FILTER,
        HIGH_PASS_FILTER
    };
    
    // ============================================================================
    // Configuration Structures
    // ============================================================================
    
    struct LevelConfig {
        uint32_t seed;
        float rateVarianceMin;
        float rateVarianceMax;
        float distanceVarianceMin;
        float distanceVarianceMax;
        
        LevelConfig() 
            : seed(0), rateVarianceMin(0.8f), rateVarianceMax(1.2f),
              distanceVarianceMin(0.9f), distanceVarianceMax(1.1f) {}
    };
    
    struct GameSaveData {
        // Player progression
        int coins;
        int hearts;
        int skillPoints;
        std::vector<Hat> unlockedHats;
        std::vector<Skill> unlockedSkills;
        
        // Statistics
        uint64_t totalJumps;
        uint64_t totalDeaths;
        uint64_t pipesCleared;
        uint64_t enemiesKilled;
        
        // Level progress
        std::vector<int> unlockedLevels;
        std::vector<std::string> achievements;
        
        GameSaveData() 
            : coins(0), hearts(3), skillPoints(0),
              totalJumps(0), totalDeaths(0), pipesCleared(0), enemiesKilled(0) {}
    };
    
} // namespace Gnosis

// Common constants
namespace Gnosis {
    const float PI = 3.14159265359f;
    const float DEG_TO_RAD = PI / 180.0f;
    const float RAD_TO_DEG = 180.0f / PI;
    const float DEG2RAD = DEG_TO_RAD;
    const float RAD2DEG = RAD_TO_DEG;
}