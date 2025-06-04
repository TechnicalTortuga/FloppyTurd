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
	Vector2 pos;
};

#endif // OBSTACLE_H

