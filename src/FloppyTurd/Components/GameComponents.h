#ifndef FLOPPY_TURD_GAME_COMPONENTS_H
#define FLOPPY_TURD_GAME_COMPONENTS_H

// Use forward declarations and proper includes to avoid circular dependencies
#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Core/Component.h"
#include "../Config/LevelConfig.h"

#include <string>
#include <vector>

namespace GameCore {

    // ============================================================================
    // Game-Specific Enums
    // ============================================================================
    
    enum class HatType {
        None = 0,
        TopHat,
        Beanie,
        Crown,
        Helmet,
        Cowboy,
        Wizard
    };
    
    enum class SkillType {
        DoubleJump = 0,
        Shield,
        SpeedBoost,
        RapidFire,
        Magnet,
        TimeSlowdown
    };
    
    enum class ColliderType {
        Circle = 0,
        Rectangle,
        Polygon
    };
    
    // ============================================================================
    // Core Components
    // ============================================================================
    
    /**
     * Transform component - position, rotation, scale
     */
    struct Transform : public Gnosis::Component {
        Gnosis::GNVector2 position;
        float rotation;
        Gnosis::GNVector2 scale;
        
        Transform() 
            : position(0.0f, 0.0f)
            , rotation(0.0f)
            , scale(1.0f, 1.0f) 
        {}
        
        Transform(const Gnosis::GNVector2& pos, float rot = 0.0f, const Gnosis::GNVector2& scl = Gnosis::GNVector2(1.0f, 1.0f))
            : position(pos)
            , rotation(rot)
            , scale(scl)
        {}
        
        ~Transform() noexcept = default;
    };
    
    /**
     * Physics component - velocity, acceleration, mass
     */
    struct Physics : public Gnosis::Component {
        Gnosis::GNVector2 velocity;
        Gnosis::GNVector2 acceleration;
        float mass;
        float drag;
        bool useGravity;
        
        Physics()
            : velocity(0.0f, 0.0f)
            , acceleration(0.0f, 0.0f)
            , mass(1.0f)
            , drag(0.98f)
            , useGravity(true)
        {}
    };
    
    /**
     * Sprite component - visual representation (static or animated)
     */
    struct Sprite : public Gnosis::Component {
        std::string textureId;
        float width;
        float height;
        Gnosis::GNColor color;
        bool visible;
        int layer;
        
        // Animation support
        bool isAnimated;
        int frameWidth;          // Width of each frame in the sprite sheet
        int frameHeight;         // Height of each frame in the sprite sheet
        int frameCount;          // Total number of frames
        int currentFrame;        // Current frame index (0-based)
        float frameTime;         // Time per frame in seconds
        float currentFrameTime;  // Accumulated time for current frame
        bool loop;               // Should animation loop?
        bool playing;            // Is animation currently playing?
        bool hasCompleted;       // Has animation completed at least once?
        
        // Static sprite constructor
        Sprite()
            : width(32.0f)
            , height(32.0f)
            , color(255, 255, 255, 255)
            , visible(true)
            , layer(0)
            , isAnimated(false)
            , frameWidth(32)
            , frameHeight(32)
            , frameCount(1)
            , currentFrame(0)
            , frameTime(0.1f)
            , currentFrameTime(0.0f)
            , loop(true)
            , playing(false)
            , hasCompleted(false)
        {}
        
        // Static sprite constructor with texture
        Sprite(const std::string& texId, float w, float h)
            : textureId(texId)
            , width(w)
            , height(h)
            , color(255, 255, 255, 255)
            , visible(true)
            , layer(0)
            , isAnimated(false)
            , frameWidth(static_cast<int>(w))
            , frameHeight(static_cast<int>(h))
            , frameCount(1)
            , currentFrame(0)
            , frameTime(0.1f)
            , currentFrameTime(0.0f)
            , loop(true)
            , playing(false)
            , hasCompleted(false)
        {}
        
        // Animated sprite constructor
        Sprite(const std::string& texId, float w, float h, int fWidth, int fHeight, int fCount, float fTime = 0.1f)
            : textureId(texId)
            , width(w)
            , height(h)
            , color(255, 255, 255, 255)
            , visible(true)
            , layer(0)
            , isAnimated(true)
            , frameWidth(fWidth)
            , frameHeight(fHeight)
            , frameCount(fCount)
            , currentFrame(0)
            , frameTime(fTime)
            , currentFrameTime(0.0f)
            , loop(true)
            , playing(true)
            , hasCompleted(false)
        {}
        
        // Animation control methods
        void Play() { playing = true; }
        void Pause() { playing = false; }
        void Stop() { playing = false; currentFrame = 0; currentFrameTime = 0.0f; }
        void Reset() { playing = false; currentFrame = 0; currentFrameTime = 0.0f; hasCompleted = false; }
        void SetFrame(int frame) { currentFrame = (frame >= 0 && frame < frameCount) ? frame : 0; }
    };
    
    /**
     * Hitbox component - unified collision and debug bounds
     */
    struct Hitbox : public Gnosis::Component {
        ColliderType type;
        // Circle
        float radius; 
        // Rectangle
        float width;
        float height;
        // Offset from entity transform position (sprite center)
        float offsetX;
        float offsetY;
        // Behavior
        bool isTrigger;
        bool isStatic;
        std::string tag;

        Hitbox()
            : type(ColliderType::Circle)
            , radius(16.0f)
            , width(32.0f)
            , height(32.0f)
            , offsetX(0.0f)
            , offsetY(0.0f)
            , isTrigger(false)
            , isStatic(false)
        {}
    };

    
    /**
     * Animation component - sprite animation
     */
    struct Animation : public Gnosis::Component {
        std::string currentAnimation;
        float frameTime;
        float currentFrameTime;
        int currentFrame;
        int frameCount;
        bool loop;
        bool playing;
        
        Animation()
            : frameTime(0.1f)
            , currentFrameTime(0.0f)
            , currentFrame(0)
            , frameCount(1)
            , loop(true)
            , playing(false)
        {}
    };
    
    // ============================================================================
    // Game-Specific Components
    // ============================================================================
    
    /**
     * Player component - player-specific data
     */
    /**
     * Heart modes for different slice granularity
     */
    enum class HeartMode {
        WHOLE = 1,   // Each heart = 1 slice (normal hearts)
        HALVES = 2,  // Each heart = 2 slices (heart halves)
        THIRDS = 3   // Each heart = 3 slices (heart thirds)
    };



    struct PlayerComponent : public Gnosis::Component {
        int health;                    // Current health slices
        int maxHealth;                 // Maximum health slices
        int score;
        int totalCoins;               // lifetime/account coins
        int sessionCoins;             // coins collected in current level attempt
        float invulnerabilityTimer;
        float shootCooldown;
        HatType equippedHat;
        bool canDoubleJump;
        bool hasUsedDoubleJump;
        
        // Heart system
        int hearts;                   // Number of heart containers (visual)
        HeartMode heartMode;          // How many slices per heart
        int liveSlices;               // Current living slices
        int ghostSlices;              // "Borrowed" slices (for hollow turds skill)
        bool hollowTurds;             // Hollow turds skill enabled
        GameCore::Difficulty difficulty;        // Current difficulty setting
        
        PlayerComponent()
            : health(3)
            , maxHealth(3)
            , score(0)
            , totalCoins(0)
            , sessionCoins(0)
            , invulnerabilityTimer(0.0f)
            , shootCooldown(0.0f)
            , equippedHat(HatType::None)
            , canDoubleJump(false)
            , hasUsedDoubleJump(false)
            , hearts(2)
            , heartMode(HeartMode::WHOLE)
            , liveSlices(2)
            , ghostSlices(0)
            , hollowTurds(false)
            , difficulty(GameCore::Difficulty::Regular)
        {}
    };
    
    /**
     * Heart UI component - displays player hearts in a vertical column
     */
    struct HeartUI : public Gnosis::Component {
        float x, y;                   // Base position for heart column
        float heartSpacing;           // Vertical spacing between hearts
        float scale;                  // UI scale factor
        bool visible;                 // Whether to render hearts
        
        HeartUI()
            : x(0.0f)
            , y(0.0f)
            , heartSpacing(40.0f)     // 40 pixels between hearts
            , scale(1.0f)
            , visible(true)
        {}
    };
    
    /**
     * Game Over UI component - manages game over screen state and animations
     */
    struct GameOverUI : public Gnosis::Component {
        bool isActive;                // Whether game over screen is showing
        bool playerHasFallen;         // Whether player has finished falling
        float fallingTimer;           // Timer for player falling animation
        float hoverTimer;             // Timer for morte sprite hovering
        float stingerTimer;           // Timer for game over stinger delay
        bool stingerPlayed;           // Whether stinger has been played
        std::string deathMessage;     // Random death message to display
        
        GameOverUI()
            : isActive(false)
            , playerHasFallen(false)
            , fallingTimer(0.0f)
            , hoverTimer(0.0f)
            , stingerTimer(0.0f)
            , stingerPlayed(false)
            , deathMessage("")
        {}
    };
    
    /**
     * Enemy component - enemy-specific data
     */
    /**
     * Enemy states for different behaviors
     */
    enum class EnemyState {
        Idle = 0,           // Default state
        Moving,              // Moving around
        Attacking,           // Attacking/Throwing
        Hurt,                // Taking damage
        Dead,                // Dead/Inactive
        Decorative           // Just for show, no behavior
    };
    
    struct Enemy : public Gnosis::Component {
        int health;
        int damage;
        float speed;
        std::string enemyType;
        std::string movementPattern;  // "horizontal", "vertical", "circular", "swoop", "snowman_thrower", "decorative"
        bool isActive;
        
        // State management
        EnemyState currentState;
        float stateTimer;           // Time in current state
        float stateDuration;        // How long to stay in current state
        
        // Bobbing & behavior state
        bool bobbingEnabled;
        float bobSpeed;       // radians per second
        float bobAmplitude;   // pixels
        float bobPhase;       // initial phase offset
        float baseY;          // anchor Y around which to bob
        bool hasInitializedBaseY;
        
        // Movement and positioning
        Gnosis::GNVector2 spawnPosition;
        bool isGrounded;           // Whether enemy should be grounded at screen bottom
        float groundOffset;        // Offset from ground (for enemies that float slightly above)
        
        // Snowman thrower specific properties
        bool isThrower;
        float throwCooldown;      // Time between throws
        float throwTimer;         // Current cooldown timer
        float throwRange;         // Distance at which to start throwing
        bool isOnScreen;          // Whether enemy is visible on screen
        bool isThrowing;          // Currently in throw animation
        float throwAnimationTimer; // Timer for throw animation
        float throwAnimationDuration; // Duration of throw animation (6 frames)
        int currentThrowFrame;    // Current frame of throw animation
        bool hasSpawnedProjectile; // Whether projectile was spawned this throw cycle
        
        // Animation support
        bool isAnimated;
        int totalFrames;           // Total animation frames
        float frameDuration;       // Time per frame
        float animationTimer;      // Current animation time
        int currentFrame;          // Current animation frame
        
        Enemy()
            : health(1)
            , damage(1)
            , speed(100.0f)
            , enemyType("")
            , movementPattern("horizontal")
            , isActive(true)
            , currentState(EnemyState::Idle)
            , stateTimer(0.0f)
            , stateDuration(0.0f)
            , bobbingEnabled(false)
            , bobSpeed(2.0f)
            , bobAmplitude(90.0f)
            , bobPhase(0.0f)
            , baseY(0.0f)
            , hasInitializedBaseY(false)
            , spawnPosition(0.0f, 0.0f)
            , isGrounded(true)
            , groundOffset(0.0f)
            , isThrower(false)
            , throwCooldown(2.0f)
            , throwTimer(0.0f)
            , throwRange(400.0f)
            , isOnScreen(false)
            , isThrowing(false)
            , throwAnimationTimer(0.0f)
            , throwAnimationDuration(0.5f)
            , currentThrowFrame(0)
            , hasSpawnedProjectile(false)
            , isAnimated(false)
            , totalFrames(1)
            , frameDuration(0.1f)
            , animationTimer(0.0f)
            , currentFrame(0)
        {}
    };
    
    /**
     * Projectile component - projectile behavior
     */
    struct Projectile : public Gnosis::Component {
        int damage;
        float speed;
        float lifetime;
        float currentLifetime;
        std::string ownerTag;
        bool piercing;
        
        // Enemy projectile specific properties
        bool isEnemyProjectile;
        Gnosis::GNVector2 direction;
        float gravity;
        bool affectedByGravity;
        
        Projectile()
            : damage(1)
            , speed(300.0f)
            , lifetime(3.0f)
            , currentLifetime(0.0f)
            , piercing(false)
            , isEnemyProjectile(false)
            , direction(0.0f, 0.0f)
            , gravity(0.0f)
            , affectedByGravity(false)
        {}
    };
    
    /**
     * PowerUp component - collectible items
     */
    struct PowerUp : public Gnosis::Component {
        Gnosis::PowerUpType type;
        int value;
        bool collected;
        float bobOffset;
        float bobSpeed;
        
        PowerUp()
            : type(Gnosis::PowerUpType::COIN)
            , value(1)
            , collected(false)
            , bobOffset(0.0f)
            , bobSpeed(2.0f)
        {}
    };
    
    /**
     * Obstacle component - static obstacles
     */
    struct Obstacle : public Gnosis::Component {
        int damage;
        bool isDestructible;
        int health;
        std::string obstacleType;
        
        // Toilet/pipe specific properties
        int behavior;               // ToiletBehavior as int (0=STATIC, 1=OSCILLATE_VERTICAL, 2=OSCILLATE_HORIZONTAL)
        float oscillationSpeed;     // Speed of oscillation (radians per second)
        float oscillationRange;     // Range of oscillation (pixels)
        float oscillationTimer;     // Current oscillation time
        Gnosis::GNVector2 basePosition; // Original spawn position for oscillation
        Gnosis::Entity pairedEntity; // For toilet pairs (top/bottom linked)
        bool isTopPart;             // True if this is the top part of a pair
        bool pipeCleared;           // True if player has passed through this pipe
        
        Obstacle()
            : damage(1)
            , isDestructible(false)
            , health(1)
            , behavior(0) // STATIC = 0
            , oscillationSpeed(0.0f)
            , oscillationRange(0.0f)
            , oscillationTimer(0.0f)
            , basePosition(0.0f, 0.0f)
            , pairedEntity(0)
            , isTopPart(false)
            , pipeCleared(false)
        {}
    };

    // Shared group pattern enum so spawners can mark pattern at creation time
    enum class GroupPattern {
        TopOnly = 0,
        BottomOnly,
        TopAndBottom,
        Ground,           // For desert ground-based obstacles (Outhouse, Cactus)
        PyramidBottom,
        PyramidTop,
        TwoFunnel,
        Decorative        // For castle decorative elements (torch pillars, chandeliers, floor torches)
    };
    
    /**
     * Group component - allows treating multiple entities as a single logical group
     * for spawning, spacing, and recycling (wrapping) purposes.
     */
    struct Group : public Gnosis::Component {
        int id;                 // Unique group identifier
        bool isLeader;          // True for the first entity in the group (drives wrapping)
        float offsetX;          // X offset from group origin where this entity should be placed
        float offsetY;          // Y offset from group origin where this entity should be placed
        float groupWidth;       // Total width of the group (valid on leader only)
        GroupPattern pattern;   // Spawned pattern type for simplified logic

        Group()
            : id(0)
            , isLeader(false)
            , offsetX(0.0f)
            , offsetY(0.0f)
            , groupWidth(0.0f)
            , pattern(GroupPattern::TopOnly) {}
    };

    /**
     * GroupGap component - modular spacing system for obstacle groups
     * This component allows flexible gap management without hardcoded spacing
     */
    struct GroupGap : public Gnosis::Component {
        float gapDistance;      // Extra spacing this group adds (in pixels)
        bool isSpacingGroup;    // True if this group exists purely for spacing
        std::string gapType;    // Type of gap: "toilet_spacing", "outhouse_spacing", etc.
        
        GroupGap()
            : gapDistance(0.0f)
            , isSpacingGroup(false)
            , gapType("default")
        {}
        
        GroupGap(float distance, bool isSpacing = false, const std::string& type = "default")
            : gapDistance(distance)
            , isSpacingGroup(isSpacing)
            , gapType(type)
        {}
    };

    /**
     * Camera component - world view and scrolling
     */
    struct Camera : public Gnosis::Component {
        Gnosis::GNVector2 position;
        float zoom;
        Gnosis::GNVector2 viewportSize;
        bool followTarget;
        Gnosis::Entity targetEntity;
        Gnosis::GNVector2 offset;
        
        Camera()
            : position(0.0f, 0.0f)
            , zoom(1.0f)
            , viewportSize(1179.0f, 1278.0f)
            , followTarget(false)
            , targetEntity(0)
            , offset(0.0f, 0.0f)
        {}
    };
    
    /**
     * Parallax component - background scrolling
     */
    struct Parallax : public Gnosis::Component {
        float scrollSpeed;
        float repeatWidth;
        bool autoScroll;
        
        Parallax()
            : scrollSpeed(50.0f)
            , repeatWidth(800.0f)
            , autoScroll(true)
        {}
    };

    /**
     * ParallaxInstance component - manages multiple instances of parallax layers
     */
    struct ParallaxInstance : public Gnosis::Component {
        std::string layerId;        // Unique identifier for the layer type
        int instanceIndex;          // Which instance this is (0, 1, 2, 3)
        int totalInstances;         // Total number of instances for this layer
        float textureWidth;         // Width of the texture for wrapping calculations
        
        ParallaxInstance()
            : instanceIndex(0)
            , totalInstances(1)
            , textureWidth(1024.0f)
        {}
        
        ParallaxInstance(const std::string& id, int index, int total, float width)
            : layerId(id)
            , instanceIndex(index)
            , totalInstances(total)
            , textureWidth(width)
        {}
    };

    /**
     * ScrollSpeed component - opt-in per-entity horizontal scroll speed handled by CameraSystem.
     * If present, CameraSystem will use this absolute pixels-per-second speed for X scrolling
     * instead of the global world scroll speed. Useful for NPCs or props that should match a
     * specific parallax band.
     */
    struct ScrollSpeed : public Gnosis::Component {
        float speed;
        explicit ScrollSpeed(float s = 0.0f) : speed(s) {}
    };

    /**
     * ParallaxVariants - optional component for background instances that can
     * swap between multiple textures each time they wrap.
     */
    struct ParallaxVariants : public Gnosis::Component {
        std::vector<std::string> textureIds; // e.g., {"SewerLargeA","SewerLargeB","SewerLargeC","SewerLargeD"}

        ParallaxVariants() = default;
        explicit ParallaxVariants(const std::vector<std::string>& ids) : textureIds(ids) {}
    };

    /**
     * NPC component - lightweight state for simple NPC behaviors (e.g., Janitor).
     */
    struct NPC : public Gnosis::Component {
        std::string type;   // "Janitor"
        int state;          // 0 = Idle/Sweep, 1 = Surprised
        float timer;        // state timer (e.g., surprise duration)
        bool triggered;     // has surprise been triggered by player passing?

        NPC() : state(0), timer(0.0f), triggered(false) {}
    };

    /**
     * StateAnimation component - declarative animation set per logical state.
     * Attach alongside Sprite; systems can switch stateName to drive Sprite fields.
     */
    struct StateAnimation : public Gnosis::Component {
        struct Clip {
            std::string textureId;
            int frameWidth = 0;
            int frameHeight = 0;
            int frameCount = 1;
            float frameTime = 0.1f;
            bool loop = true;
        };

        // Mapping of state name -> animation clip
        std::vector<std::pair<std::string, Clip>> clips;
        std::string currentState;

        // Returns pointer to clip by state or nullptr
        const Clip* getClip(const std::string& state) const {
            for (const auto& kv : clips) {
                if (kv.first == state) return &kv.second;
            }
            return nullptr;
        }
    };
    
    /**
     * Lifetime component - auto-destroy after time
     */
    struct Lifetime : public Gnosis::Component {
        float maxLifetime;
        float currentLifetime;
        
        Lifetime(float lifetime = 5.0f)
            : maxLifetime(lifetime)
            , currentLifetime(0.0f)
        {}
    };
    
    /**
     * Audio component - sound effects
     */
    struct AudioSource : public Gnosis::Component {
        std::string soundId;
        float volume;
        bool loop;
        bool playOnStart;
        bool isPlaying;
        
        AudioSource()
            : volume(1.0f)
            , loop(false)
            , playOnStart(false)
            , isPlaying(false)
        {}
    };

    /**
     * UIElement component - combines button and text functionality
     */
    struct UIElement : public Gnosis::Component {
        // Button properties
        std::string buttonText;
        std::string normalTextureId;
        std::string hoverTextureId;
        std::string pressedTextureId;
        bool isHovered;
        bool isPressed;
        bool isEnabled;
        bool visible;  // Visibility control
        
        // Text properties
        float fontSize;
        Gnosis::GNColor textColor;
        Gnosis::GNColor textHoverColor;
        int textLayer;  // Layer for text rendering
        float textOutlineWidth; // Outline width in pixels for raster text
        
        // Text centering properties
        bool centerTextHorizontally;  // Center text horizontally within button bounds
        bool centerTextVertically;    // Center text vertically within button bounds
        float textOffsetX;            // Manual text offset X (applied after centering)
        float textOffsetY;            // Manual text offset Y (applied after centering)
        
        UIElement()
            : isHovered(false)
            , isPressed(false)
            , isEnabled(true)
            , visible(true)
            , fontSize(24.0f)
            , textColor(0, 0, 0, 255)
            , textHoverColor(255, 255, 0, 255)
            , textLayer(10)  // Default to high layer for UI text
            , textOutlineWidth(0.0f)
            , centerTextHorizontally(true)  // Default to centered text
            , centerTextVertically(true)
            , textOffsetX(0.0f)
            , textOffsetY(0.0f)
        {}
        
        UIElement(const std::string& text, const std::string& normalTex, const std::string& hoverTex = "", const std::string& pressedTex = "")
            : buttonText(text)
            , normalTextureId(normalTex)
            , hoverTextureId(hoverTex.empty() ? normalTex : hoverTex)
            , pressedTextureId(pressedTex.empty() ? normalTex : pressedTex)
            , isHovered(false)
            , isPressed(false)
            , isEnabled(true)
            , visible(true)
            , fontSize(24.0f)
            , textColor(0, 0, 0, 255)
            , textHoverColor(255, 255, 0, 255)
            , textLayer(10)
            , textOutlineWidth(0.0f)
            , centerTextHorizontally(true)  // Default to centered text
            , centerTextVertically(true)
            , textOffsetX(0.0f)
            , textOffsetY(0.0f)
        {}
    };

    /**
     * Text component - for rendering text
     */
    struct Text : public Gnosis::Component {
        std::string text;
        float fontSize;
        Gnosis::GNColor color;
        bool visible;
        int layer;
        
        Text()
            : fontSize(24.0f)
            , color(255, 255, 255, 255)
            , visible(true)
            , layer(10)  // Default to high layer for UI text
        {}
        
        Text(const std::string& text, float fontSize = 24.0f, const Gnosis::GNColor& color = Gnosis::GNColor(255, 255, 255, 255), int layer = 10)
            : text(text)
            , fontSize(fontSize)
            , color(color)
            , visible(true)
            , layer(layer)
        {}
    };

    /**
     * UIShape component - draws simple shapes in screen space on UI layers
     * Supports rectangles and lines using platform delegate drawRectangle
     */
    enum class UIShapeType {
        Rectangle = 0,
        Line = 1
    };

    struct UIShape : public Gnosis::Component {
        UIShapeType type;
        float width;            // For Rectangle: width in pixels; For Line: length in pixels
        float height;           // For Rectangle: height; For Line: thickness
        Gnosis::GNColor color;  // RGBA color
        int layer;              // UI layer ordering
        bool visible;           // Visibility flag

        UIShape()
            : type(UIShapeType::Rectangle)
            , width(0.0f)
            , height(0.0f)
            , color(40, 40, 70, 230)
            , layer(20)
            , visible(true)
        {}

        UIShape(UIShapeType t, float w, float h, const Gnosis::GNColor& c, int l = 20, bool v = true)
            : type(t)
            , width(w)
            , height(h)
            , color(c)
            , layer(l)
            , visible(v)
        {}
    };

    struct Bounds {
        float width;
        float height;
        float offsetX = 0.0f;  // Offset from sprite center
        float offsetY = 0.0f;
        bool useTextureSize = false;  // If true, use actual texture size instead of sprite size
        
        Bounds() : width(0.0f), height(0.0f) {}
        Bounds(float w, float h) : width(w), height(h) {}
        Bounds(float w, float h, float ox, float oy) : width(w), height(h), offsetX(ox), offsetY(oy) {}
        Bounds(float w, float h, float ox, float oy, bool useTex) : width(w), height(h), offsetX(ox), offsetY(oy), useTextureSize(useTex) {}
    };

    /**
     * DebugDraw component - indicates an entity should show debug overlays (hitboxes, bounds)
     */
    struct DebugDraw : public Gnosis::Component {
        bool showBounds = true;         // Show bounding box rectangle
        bool showCollider = true;       // Show collider rectangle  
        Gnosis::GNColor boundsColor;            // Color for bounds rectangle
        Gnosis::GNColor colliderColor;          // Color for collider rectangle
        float alpha = 0.3f;             // Transparency for debug overlays
        int debugLayer = 18;            // Layer for debug overlays (high priority)
        
        DebugDraw() 
            : boundsColor(Gnosis::GNColor(0, 255, 0, 255))      // Green for bounds
            , colliderColor(Gnosis::GNColor(255, 0, 0, 255))    // Red for colliders
        {}
        
        DebugDraw(bool bounds, bool collider, Gnosis::GNColor bColor, Gnosis::GNColor cColor, float a = 0.3f)
            : showBounds(bounds), showCollider(collider), boundsColor(bColor), colliderColor(cColor), alpha(a)
        {}
    };

    /**
     * Pickup component - collectible items like coins and power-ups
     */
    struct Pickup : public Gnosis::Component {
        std::string pickupType;
        int value;
        bool isActive;
        float bobbingSpeed;
        float bobbingAmplitude;
        float bobbingTimer;
        float bobbingBaseY;
        
        Pickup()
            : pickupType("BlueCoin")
            , value(10)
            , isActive(true)
            , bobbingSpeed(2.0f)
            , bobbingAmplitude(5.0f)
            , bobbingTimer(0.0f)
            , bobbingBaseY(0.0f)
        {}
        
        Pickup(const std::string& type, int val)
            : pickupType(type)
            , value(val)
            , isActive(true)
            , bobbingSpeed(2.0f)
            , bobbingAmplitude(5.0f)
            , bobbingTimer(0.0f)
            , bobbingBaseY(0.0f)
        {}
    };

    /**
     * RotationRenderer component - marks sprites that should use centered rendering for rotation
     * This component tells the SpriteSystem to use centered positioning instead of top-left
     * for sprites that need to rotate properly (like the poophat)
     */
    struct RotationRenderer : public Gnosis::Component {
        bool enabled;
        
        RotationRenderer(bool isEnabled = true)
            : enabled(isEnabled)
        {}
    };

    /**
     * PivotRotationRenderer component - supports rotation around custom pivot points
     * This allows sprites to rotate around specific points (like spike ball rotating from its base)
     */
    struct PivotRotationRenderer : public Gnosis::Component {
        bool enabled;
        float pivotX;        // Pivot point X relative to sprite center (in pixels)
        float pivotY;        // Pivot point Y relative to sprite center (in pixels)
        float rotationSpeed; // Rotation speed in degrees per second
        
        PivotRotationRenderer(bool isEnabled = true, float px = 0.0f, float py = 0.0f, float speed = 0.0f)
            : enabled(isEnabled)
            , pivotX(px)
            , pivotY(py)
            , rotationSpeed(speed)
        {}
    };

} // namespace GameCore

// Bring GameCore components into Gnosis namespace for easier access
namespace Gnosis {
    using Transform = GameCore::Transform;
    using Physics = GameCore::Physics;
    using Sprite = GameCore::Sprite;
    using Hitbox = GameCore::Hitbox;
    using Animation = GameCore::Animation;
    using PlayerComponent = GameCore::PlayerComponent;
    using Enemy = GameCore::Enemy;
    using Projectile = GameCore::Projectile;
    using PowerUp = GameCore::PowerUp;
    using Obstacle = GameCore::Obstacle;
    using Parallax = GameCore::Parallax;
    using Lifetime = GameCore::Lifetime;
    using AudioSource = GameCore::AudioSource;
    using UIElement = GameCore::UIElement;
    using Text = GameCore::Text;
    using UIShape = GameCore::UIShape;
    using Pickup = GameCore::Pickup;
    using RotationRenderer = GameCore::RotationRenderer;
    using PivotRotationRenderer = GameCore::PivotRotationRenderer;
    using Group = GameCore::Group;
    using GroupGap = GameCore::GroupGap;
    using ColliderType = GameCore::ColliderType;
}

#endif // FLOPPY_TURD_GAME_COMPONENTS_H