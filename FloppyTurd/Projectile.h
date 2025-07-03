#pragma once
#include "RaylibCompat.h"
#include "Sprite.h"

class Projectile {
public:
    Projectile(Vector2 position, Vector2 direction, float speed, float scale, const char* spriteFilePath);
    ~Projectile();

    void Update(float deltaTime);
    void Draw();
    Rectangle GetHitbox() const;
    bool IsOffScreen() const;

private:
    Vector2 position;
    Vector2 direction;
    float speed;
    float scale;
    Sprite* sprite;
    Rectangle hitbox;
};