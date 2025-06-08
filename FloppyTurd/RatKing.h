#pragma once
#include "Boss.h"
#include "Sprite.h"
#include "ToiletPaperProjectile.h"
#include <vector>

class RatKing : public Boss
{
public:
    RatKing(Vector2 startPosition);
    ~RatKing();

    void Draw() override;
    void Update(float deltaTime) override;
    std::vector<Rectangle> GetHitboxes() override;
    void SetCollisionEnabled(bool enabled) override; // Implement the new method from Boss
    void TakeDamage(int damage) override;
    bool ShouldBeRemoved() override;
    void SetPlayerPosition(Vector2 playerPos);
    std::vector<ToiletPaperProjectile*>& GetProjectiles() { return projectiles; }
    bool IsInLowHealthMode() override;
    double GetLowHealthTriggerTime() const override;

private:
    void ChangeState(State newState);
    void HandleIdle(float deltaTime);
    void HandleWalking(float deltaTime);
    void HandlePreparingAttack(float deltaTime);
    void HandleAttacking(float deltaTime);
    void HandleHurt(float deltaTime);
    void HandleDeath(float deltaTime);

    void DrawNormal();
    void DrawAiming();
    void DrawLockOnIndicator();

    // Modified to accept angle offset
    void SpawnProjectile(float angleOverride = 0.0f);

    // Sprites
    Sprite* idleSprite{};
    Sprite* walkSprite{};
    Sprite* hurtSprite{};
    Sprite* deathSprite{};
    Sprite* torsoSprite{};
    Sprite* frontArmSprite{};
    Sprite* backArmSprite{};

    std::vector<ToiletPaperProjectile*> projectiles;

    // Aiming
    float aimingAngle{};
    float lockOnTimer{};
    float lockOnDuration{ 1.2f };
    bool hasLockedOn{ false };
    Vector2 shoulder;

    // Movement & States
    float walkRangeMin;
    float walkRangeMax;
    float walkSpeed;
    float hurtTimer;
    bool hasFlashedHurt;
    int hurtBuffer{};

    float attackTimer{};
    bool hasFiredProjectile{ false };

    Vector2 playerPosition;  // Last known player position
    float currentArmAngleDeg{};

    bool hasSpawnedMinions = false;  // NEW for later RatCopter trigger
    bool hasTriggeredLowHealthMusic = false;

    double lowHealthTriggeredAt = 0.0;
    int nextMinionHealthThreshold = 185;

    void SpawnMinionWave(int count);  // NEW
};