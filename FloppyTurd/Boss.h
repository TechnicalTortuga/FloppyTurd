#pragma once
#include "Obstacle.h"  // Now inherits from Obstacle
#include "raylib.h"
#include <vector>
#include "Sprite.h"

class Boss : public Obstacle
{
public:
    virtual ~Boss() = default;

    void Draw() override = 0;
    void Update(float deltaTime) override = 0;  // Updated to accept deltaTime
    std::vector<Rectangle> GetHitboxes() override = 0;

    virtual void TakeDamage(int damage) = 0;
    virtual bool ShouldBeRemoved() = 0;  // For death state or removal

    // Common properties
    int health;
    float scale;
    bool isActive;
    virtual void SetPlayerPosition(Vector2 playerPos) { }
    virtual bool IsInLowHealthMode() = 0;

protected:  // Changed from public to protected for inheritance
    Vector2 position;
    Sprite* currentSprite;  // Current animation state
    enum State { IDLE, WALKING, PREPARING_ATTACK, ATTACKING, HURT, DEATH };
    State currentState;
};