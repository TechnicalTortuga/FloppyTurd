#pragma once
#include "RaylibCompat.h"
#include "Game.h"

class Loading {
public:
	Loading(Game* game);
	~Loading();

	void Update();
	void Draw();
	bool IsComplete() const { return rotationComplete; }

private:
	Game* game;
	Texture2D poophat;
	float rotationAngle;
	float rotationTimer;
	bool rotationComplete;
};