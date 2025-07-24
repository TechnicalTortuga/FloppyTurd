#ifndef FLOPPY_TURD_GAME_COMPONENTS_H
#define FLOPPY_TURD_GAME_COMPONENTS_H

#include "../../Engine/Core/Component.h"
#include "../../Engine/Core/GnosisTypes.h"
#include <string>

namespace FloppyTurd {

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
     * Sprite component - visual representation
     */
    struct Sprite : public Gnosis::Component {
        std::string textureId;
        float width;
        float height;
        Gnosis::GNColor color;
        bool visible;
        int layer;
        
        Sprite()
            : width(32.0f)
            , height(32.0f)
            , color(255, 255, 255, 255)
            , visible(true)
            , layer(0)
        {}
        
        Sprite(const std::string& texId, float w, float h)
            : textureId(texId)
            , width(w)
            , height(h)
            , color(255, 255, 255, 255)
            , visible(true)
            , layer(0)
        {}
    };
    
    /**
     * Collider component - collision detection
     */
    struct Collider : public Gnosis::Component {
        ColliderType type;
        float radius;           // For circle colliders
        float width, height;    // For rectangle colliders
        bool isTrigger;
        bool isStatic;
        std::string tag;
        
        Collider()
            : type(ColliderType::Circle)
            , radius(16.0f)
            , width(32.0f)
            , height(32.0f)
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
        
        Obstacle()
            : damage(1)
            , isDestructible(false)
            , health(1)
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

} // namespace FloppyTurd

// Bring FloppyTurd components into Gnosis namespace for easier access
namespace Gnosis {
    using Transform = FloppyTurd::Transform;
    using Physics = FloppyTurd::Physics;
    using Sprite = FloppyTurd::Sprite;
    using Collider = FloppyTurd::Collider;
    using Animation = FloppyTurd::Animation;
    using PlayerComponent = FloppyTurd::PlayerComponent;
    using Enemy = FloppyTurd::Enemy;
    using Projectile = FloppyTurd::Projectile;
    using PowerUp = FloppyTurd::PowerUp;
    using Obstacle = FloppyTurd::Obstacle;
    using Parallax = FloppyTurd::Parallax;
    using Lifetime = FloppyTurd::Lifetime;
    using AudioSource = FloppyTurd::AudioSource;
    using ColliderType = FloppyTurd::ColliderType;
}

#endif // FLOPPY_TURD_GAME_COMPONENTS_H