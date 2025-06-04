// Cactus.h
#pragma once
#include "Obstacle.h"
#include "Sprite.h"
#include "Resources.h"
#include <vector>

enum class CactusVariant {
    A, B, C, D, E, BUSH
};

class Cactus : public Obstacle {
public:
    Cactus(Vector2 spawnPos, CactusVariant variant);
    ~Cactus();

    void Draw() override;
    void Update(float deltaTime) override;
    std::vector<Rectangle> GetHitboxes() override;

    float GetWidth() const;
    Sprite* sprite;
    CactusVariant GetVariant() const { return type; }

private:
    CactusVariant type;
    Rectangle hitbox;

    void UpdateHitbox();
};
