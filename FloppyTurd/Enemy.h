#pragma once
#include "RaylibCompat.h"

class Enemy {
public:
    virtual ~Enemy() = default;

    virtual void Draw() const = 0;
    virtual void Update(float deltaTime) = 0;
    virtual Rectangle GetHitbox() const = 0;
    virtual void TakeDamage() = 0;
    virtual bool ShouldBeRemoved() const = 0;
    virtual void SetSpeed(float speed) = 0; // New: Set movement speed
};