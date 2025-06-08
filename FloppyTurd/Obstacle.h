#pragma once
#include <vector>
#include "raylib.h"
#ifndef OBSTACLE_H
#define OBSTACLE_H

class Obstacle
{
public:
    virtual ~Obstacle() = default;

    virtual void Draw() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual std::vector<Rectangle> GetHitboxes() = 0;
    // Add method to toggle collision
    virtual void SetCollisionEnabled(bool enabled) = 0;

    Vector2 pos;
    bool collisionEnabled = true; // Default to enabled

protected:

};

#endif // OBSTACLE_H