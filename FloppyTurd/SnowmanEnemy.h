#pragma once

#include "Enemy.h"
#include "Sprite.h"
#include <vector>
#include <memory>
#include "raylib.h"

class SnowballProjectile;

enum class SnowmanType { sRed, sGREEN, sBLUE, sCHAD };

class SnowmanEnemy : public Enemy {
public:
    SnowmanEnemy(Vector2 spawnPos, SnowmanType type);
    SnowmanEnemy(Vector2 spawnPos, SnowmanType type, float speed, int difficulty); // Updated constructor
    ~SnowmanEnemy();

    void TryThrowSnowball();

    void Update(float deltaTime) override;
    void Draw() const override;
    Rectangle GetHitbox() const override;
    void TakeDamage() override;
    bool ShouldBeRemoved() const override;
    void SetSpeed(float speed) override; // Inherited from Enemy

    std::vector<SnowballProjectile*> GetSnowballs() const;

private:
    void UpdateHitbox();
    void ResetSnowballs();
    SnowballProjectile* GetInactiveSnowball();

    Vector2 pos;
    Rectangle hitbox;
    SnowmanType type;

    Sprite* currentSprite;
    Sprite* idleSprite;
    Sprite* throwSprite;

    bool isHurt;
    float hurtTimer;

    float throwTimer = 0.0f;
    float throwCooldown;      // New: Stores the cooldown duration
    bool hasThrown;
    bool isThrowing;

    float speed;

    std::vector<std::unique_ptr<SnowballProjectile>> snowballPool;

    bool hasFlipped;
    bool isFlipped;
    bool queuedSecondThrow;
};