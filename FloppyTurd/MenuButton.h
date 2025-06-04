#pragma once
#include "raylib.h"
class MenuButton
{
public:
	virtual ~MenuButton() = default;

	virtual void Draw() = 0;
	virtual void Update() = 0;

private:
	Rectangle hitbox;
};

