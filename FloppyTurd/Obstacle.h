#pragma once
#include <vector>
#include "PlatformAPI.h"
#ifndef OBSTACLE_H
#define OBSTACLE_H

class Obstacle
{
public:
    virtual ~Obstacle() = default;

    virtual void Draw() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual std::vector<Rectangle> GetHitboxes() = 0;
    virtual void SetCollisionEnabled(bool enabled) = 0;
    virtual void SetPanSpeed(float speed) = 0; // New: Virtual method for pan speed

    Vector2 pos;
    bool collisionEnabled = true;

protected:
};

#endif // OBSTACLE_H