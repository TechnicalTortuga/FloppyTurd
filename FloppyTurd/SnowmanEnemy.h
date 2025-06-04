#pragma once

#include "Enemy.h"
#include "Sprite.h"
#include <vector>
#include <memory>       // <— added
#include "raylib.h"

class SnowballProjectile;  // forward

enum class SnowmanType { sRed, sGREEN, sBLUE, sCHAD };

class SnowmanEnemy : public Enemy {
public:
    SnowmanEnemy(Vector2 spawnPos, SnowmanType type);
    ~SnowmanEnemy();

    void TryThrowSnowball();

    void Update(float deltaTime) override;
    void Draw() const override;
    Rectangle GetHitbox() const override;
    void TakeDamage() override;
    bool ShouldBeRemoved() const override;

    // Expose active projectiles
    std::vector<SnowballProjectile*> GetSnowballs() const;

private:
    void UpdateHitbox();
    void ResetSnowballs();                             // implemented below
    SnowballProjectile* GetInactiveSnowball();         // implemented below

    Vector2 pos;
    Rectangle hitbox;
    SnowmanType type;

    Sprite* currentSprite;
    Sprite* idleSprite;
    Sprite* throwSprite;

    bool isHurt;
    float hurtTimer;

    float throwTimer = 0.0f;
    bool hasThrown;
    bool isThrowing;

    float speed;

    // Our pool
    std::vector<std::unique_ptr<SnowballProjectile>> snowballPool;

    // Flip state
    bool hasFlipped;
    bool isFlipped;
    bool queuedSecondThrow;
};
