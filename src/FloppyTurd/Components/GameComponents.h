#ifndef FLOPPY_TURD_GAME_COMPONENTS_H
#define FLOPPY_TURD_GAME_COMPONENTS_H

// Use forward declarations and proper includes to avoid circular dependencies
#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Core/Component.h"

#include <string>

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
    struct PlayerComponent : public Gnosis::Component {
        int health;
        int maxHealth;
        int score;
        int coins;
        float invulnerabilityTimer;
        float shootCooldown;
        HatType equippedHat;
        bool canDoubleJump;
        bool hasUsedDoubleJump;
        
        PlayerComponent()
            : health(3)
            , maxHealth(3)
            , score(0)
            , coins(0)
            , invulnerabilityTimer(0.0f)
            , shootCooldown(0.0f)
            , equippedHat(HatType::None)
            , canDoubleJump(false)
            , hasUsedDoubleJump(false)
        {}
    };
    
    /**
     * Enemy component - enemy-specific data
     */
    struct Enemy : public Gnosis::Component {
        int health;
        int damage;
        float speed;
        std::string enemyType;
        bool isActive;
        
        Enemy()
            : health(1)
            , damage(1)
            , speed(100.0f)
            , isActive(true)
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
        
        Projectile()
            : damage(1)
            , speed(300.0f)
            , lifetime(3.0f)
            , currentLifetime(0.0f)
            , piercing(false)
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
        
        Pickup()
            : pickupType("BlueCoin")
            , value(10)
            , isActive(true)
            , bobbingSpeed(2.0f)
            , bobbingAmplitude(5.0f)
            , bobbingTimer(0.0f)
        {}
        
        Pickup(const std::string& type, int val)
            : pickupType(type)
            , value(val)
            , isActive(true)
            , bobbingSpeed(2.0f)
            , bobbingAmplitude(5.0f)
            , bobbingTimer(0.0f)
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
    using ColliderType = GameCore::ColliderType;
}

#endif // FLOPPY_TURD_GAME_COMPONENTS_H