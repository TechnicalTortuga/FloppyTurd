#pragma once
#include "Obstacle.h"  // Now inherits from Obstacle
#include "RaylibCompat.h"
#include <vector>
#include "Sprite.h"

class Boss : public Obstacle
{
public:
    virtual ~Boss() = default;

    void Draw() override = 0;
    void Update(float deltaTime) override = 0;  // Updated to accept deltaTime
    std::vector<Rectangle> GetHitboxes() override = 0;
    void SetCollisionEnabled(bool enabled) override = 0; // Implement the new method from Obstacle

    virtual void TakeDamage(int damage) = 0;
    virtual bool ShouldBeRemoved() = 0;  // For death state or removal

    // Common properties
    int health;
    float scale;
    bool isActive;
    virtual void SetPlayerPosition(Vector2 playerPos) { }
    virtual bool IsInLowHealthMode() = 0;
    virtual double GetLowHealthTriggerTime() const = 0;

    // Add method to access current state
    enum State { IDLE, WALKING, PREPARING_ATTACK, ATTACKING, HURT, DEATH };
    State GetCurrentState() const { return currentState; }
    Vector2 position;

protected:  // Changed from public to protected for inheritance
    Sprite* currentSprite;  // Current animation state
    State currentState;
};