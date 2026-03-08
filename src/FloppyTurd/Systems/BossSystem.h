#pragma once

#include "../Components/GameComponents.h"
#include "../Systems/LevelManager.h"
#include "../Systems/ProjectileSystem.h"
#include "../Systems/ExplosionSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <vector>
#include <memory>

namespace GameCore {

enum class RatKingState {
    IDLE,           // 2 second timer, single frame idle
    WALKING_LEFT,   // Walking left within bounds
    WALKING_RIGHT,  // Walking right within bounds
    AIMING,         // 1.2-2.0s aiming with arm rotation
    THROWING,       // 7-frame throwing animation
    HURT,           // 6-frame hurt animation
    DEATH           // 6-frame death animation
};

// Multi-sprite animation data
struct RatKingSprites {
    Sprite* idleSprite = nullptr;        // Single frame idle
    Sprite* walkSprite = nullptr;        // 8-frame walking
    Sprite* torsoSprite = nullptr;       // 7-frame aiming torso
    Sprite* backArmSprite = nullptr;     // 7-frame back arm rotation
    Sprite* frontArmSprite = nullptr;    // 7-frame front arm rotation
    Sprite* hurtSprite = nullptr;        // 6-frame hurt
    Sprite* deathSprite = nullptr;       // 6-frame death
};

// Lock-on dot data for visual indicator
struct LockOnDot {
    GNVector2 position;
    float progress;  // 0.0 to 1.0 for color interpolation
};

// Aiming system data
struct AimingData {
    float aimDuration = 1.2f;           // Base aiming time (health scales this)
    float aimTimer = 0.0f;
    GNVector2 playerPosition = {0, 0};
    float currentArmAngle = 180.0f;     // Start facing down
    GNVector2 shoulderPivot = {0, 0};     // Rotation point
    bool hasLockedOn = false;
    float lockOnAngle = 0.0f;           // Final locked angle
    std::vector<LockOnDot> lockOnDots;  // Visual indicator dots
    std::vector<Entity> dotEntities;    // Entities for rendering dots
};

class BossSystem {
public:
    BossSystem(Gnosis::ECS* ecsSystem, LevelManager* levelManager, ProjectileSystem* projectileSystem, ExplosionSystem* explosionSystem = nullptr, PlatformDelegates* platformDelegates = nullptr);
    ~BossSystem();

    void InitializeForLevel();
    void Update(float deltaTime);
    void HandleDamage(int damage);
    void UpdateScreenDimensions(float width, float height);

    // State management
    RatKingState GetCurrentState() const { return currentState; }
    void ChangeState(RatKingState newState);
    
    // Reset boss to initial state (for Try Again)
    void Reset();

    // Boss properties
    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    bool IsActive() const { return isActive; }
    float GetScale() const { return scale; }
    bool IsLowHealthMode() const { return health <= maxHealth * 0.4f; }

    // Position and movement
    GNVector2 GetPosition() const { return position; }
    void SetPlayerPosition(GNVector2 playerPos);

    // Visual effects data
    const std::vector<LockOnDot>& GetLockOnDots() const { return aimingData.lockOnDots; }
    bool IsHurtFlashing() const { return currentState == RatKingState::HURT && hurtFlashTimer < 0.6f; }
    
    // Death sequence query
    bool IsDeathSequenceComplete() const { return m_deathSequenceComplete; }
    float GetWhiteFadeAlpha() const { return m_whiteFadeAlpha; }
    
private:
    // Core systems
    Gnosis::ECS* m_ecsSystem = nullptr;
    LevelManager* m_levelManager = nullptr;
    ProjectileSystem* m_projectileSystem = nullptr;
    ExplosionSystem* m_explosionSystem = nullptr;
    PlatformDelegates* m_platformDelegates = nullptr;

    // Boss entities and state
    Entity bossEntity = 0;        // Main torso entity
    Entity backArmEntity = 0;     // Back arm entity
    Entity frontArmEntity = 0;    // Front arm entity
    RatKingState currentState = RatKingState::IDLE;
    RatKingSprites sprites;
    AimingData aimingData;

    // Boss position
    GNVector2 position = {0, 0};

    // Boss properties
    int health = 200;
    int maxHealth = 200;
    bool isActive = false;
    float scale = 8.0f;  // 8.0x scale for 128px -> 1024px (matches player scale)

    // Movement boundaries - will be set dynamically based on screen dimensions
    float walkRangeMin = 100.0f;
    float walkRangeMax = 220.0f;
    float walkSpeed = 200.0f;  // Increased for broader movement
    float screenWidth = 1179.0f;  // iPhone 16 width, will be updated dynamically
    float screenHeight = 2556.0f; // iPhone 16 height, will be updated dynamically

    // State timers
    float idleTimer = 0.0f;
    float walkTimer = 0.0f;
    float hurtTimer = 0.0f;
    float deathTimer = 0.0f;

    // Animation state
    bool hasFiredProjectile = false;
    int hurtBuffer = 0;
    bool hasFlashedHurt = false;
    float hurtFlashTimer = 0.0f;

    // Walking destination for purposeful movement
    float m_walkDestination = 0.0f;
    float m_walkCooldown = 0.0f;  // Cooldown timer to prevent consecutive walks

    // Minion spawning
    int nextMinionHealthThreshold = 185;  // Start at 185 HP (first threshold at ~90%)
    bool hasTriggeredLowHealthMusic = false;
    
    // Death sequence state
    bool m_deathSequenceStarted = false;
    bool m_deathSequenceComplete = false;
    float m_deathSequenceTimer = 0.0f;
    float m_whiteFadeAlpha = 0.0f;
    bool m_hasPlayedScreech = false;
    bool m_hasPlayedBossKill = false;
    int m_explosionIndex = 0;

    // State handlers
    void HandleIdle(float deltaTime);
    void HandleWalking(float deltaTime);
    void HandleAiming(float deltaTime);
    void HandleThrowing(float deltaTime);
    void HandleHurt(float deltaTime);
    void HandleDeath(float deltaTime);

    // Helper functions
    void CreateBodyPartEntities();
    void UpdateSprites(float deltaTime);
    void UpdateArmRotations();
    void SpawnProjectile();
    void SpawnMinionWave(int count);
    void UpdateLockOnIndicator();
    void CreateLockOnDotEntities();
    void DestroyLockOnDotEntities();
    void ChangeMusic(const std::string& musicFile);
    void SetArmSpriteVisibility(bool showBackArm, bool showFrontArm);
    Gnosis::GNVector2 GetShoulderPosition() const;
    
    // Death sequence helpers
    void StartDeathSequence();
    void UpdateDeathSequence(float deltaTime);
    void SpawnExplosionAtRandomPosition();

    // Sprite management
    void LoadSprites();
    void UnloadSprites();
    void SetCurrentSprite(Sprite* sprite);
};

} // namespace GameCore
