#pragma once
#include "Obstacle.h"
#include "Sprite.h"
#include "Resources.h"
#include <vector>

enum class CactusVariant {
    A, B, C, D, E, BUSH,
    DANCING_SMALL,        // 64 × 64  · 8 frames
    DANCING_BIG,          // 64 × 90  · 8 frames
    DANCING_COWBOY        // 64 × 90  · 8 frames + hat
};

class Cactus : public Obstacle
{
public:
    Cactus(Vector2 spawnPos, CactusVariant variant);
    ~Cactus();

    void Update(float deltaTime) override;
    void Draw()   override;
    std::vector<Rectangle> GetHitboxes() override;

    void SetCollisionEnabled(bool enabled) override;   // inherits from Obstacle
    float GetWidth() const;

    CactusVariant GetVariant() const { return type; }

private:
    void   UpdateHitbox();

    CactusVariant type;
    Sprite* sprite;
    Rectangle     hitbox{};          // unused for dancing variants
    float         moveSpeed;         // 80 or 100 px / s
    bool          collisionEnabled;
};
