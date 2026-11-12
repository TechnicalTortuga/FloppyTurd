#pragma once
#include "raylib.h"
#include "Sprite.h"
#include <memory>
#include <vector>
class Enemy;
class Boss;

class Projectile {
public:
    Projectile(Vector2 position, Vector2 direction, float speed, float scale, const char* spriteFilePath, int homingRank=0);
    ~Projectile();

    void Update(float deltaTime);
    void Draw();
    Rectangle GetHitbox() const;
    bool IsOffScreen() const;
    void AcquireTarget(const std::vector<const void*>& excluded = {});
    const void* TargetId() const;

private:
    Vector2 position;
    Vector2 direction;
    float speed;
    float scale;
    Sprite* sprite;
    Rectangle hitbox;
    int homingRank = 0;
    std::weak_ptr<Enemy> targetEnemy;
    std::weak_ptr<Boss> targetBoss;
    bool GetTargetPoint(Vector2& point) const;
};
