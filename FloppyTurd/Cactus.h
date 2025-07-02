#pragma once
#include "Obstacle.h"
#include "Sprite.h"
#include "ResourceCompat.h"
#include <vector>

enum class CactusVariant {
    A, B, C, D, E, BUSH,
    DANCING_SMALL,
    DANCING_BIG,
    DANCING_COWBOY
};

class Cactus : public Obstacle
{
public:
    Cactus(Vector2 spawnPos, CactusVariant variant);
    ~Cactus();

    void Update(float deltaTime) override;
    void Draw() override;
    std::vector<Rectangle> GetHitboxes() override;
    void SetCollisionEnabled(bool enabled) override;
    void SetPanSpeed(float speed) override; // New: Set pan speed

    float GetWidth() const;
    CactusVariant GetVariant() const { return type; }

private:
    void UpdateHitbox();

    CactusVariant type;
    Sprite* sprite;
    Rectangle hitbox;
    float moveSpeed;
    bool collisionEnabled;
};