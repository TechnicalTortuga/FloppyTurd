// Enemy.h
#pragma once
#include <raylib.h>

class Enemy {
public:
    virtual ~Enemy() = default;

    // Draw and update the enemy.
    virtual void Draw() const = 0;
    virtual void Update(float deltaTime) = 0;
    virtual Rectangle GetHitbox() const = 0;

    // NEW: Called when the enemy is hit by a projectile.
    virtual void TakeDamage() = 0;
    // NEW: Returns true if the enemy’s hurt animation is finished and it can be removed.
    virtual bool ShouldBeRemoved() const = 0;
};
