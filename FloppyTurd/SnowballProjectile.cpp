#include "SnowballProjectile.h"
#include "Resources.h"

SnowballProjectile::SnowballProjectile(Vector2 pos, Vector2 vel)
    : position(pos), velocity(vel) {}

void SnowballProjectile::Activate(Vector2 pos, Vector2 vel) {
    position = pos;
    velocity = vel;
    active = true;
}

void SnowballProjectile::Deactivate() {
    active = false;
}

bool SnowballProjectile::IsActive() const {
    return active;
}

void SnowballProjectile::Update(float dt) {
    if (!active) return;
    velocity.y += gravity * dt;
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    if (ShouldBeRemoved())
        Deactivate();
}

void SnowballProjectile::Draw() const {
    if (active)
        DrawCircleV(position, radius, WHITE);
}

Vector2 SnowballProjectile::GetCenter() const {
    return position;
}

Rectangle SnowballProjectile::GetHitbox() const {
    return Rectangle{ position.x - radius, position.y - radius, radius * 2.0f, radius * 2.0f };
}

bool SnowballProjectile::ShouldBeRemoved() const {
    return (position.x + radius < 0 || position.x - radius > 320 ||
        position.y + radius < 0 || position.y - radius > 180);
}