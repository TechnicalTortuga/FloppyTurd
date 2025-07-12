#pragma once
#include "PlatformAPI.h"

class SnowballProjectile {
public:
    SnowballProjectile(Vector2 pos, Vector2 vel);
    void Update(float dt);
    void Draw() const;
    Vector2 GetCenter() const;
    Rectangle GetHitbox() const;
    bool ShouldBeRemoved() const;
    void Activate(Vector2 pos, Vector2 vel);
    void Deactivate();
    bool IsActive() const;
private:
    Vector2 position;
    Vector2 velocity;
    const float gravity = 40.0f;
    const float radius = 3.0f;
    bool active = false;
};
