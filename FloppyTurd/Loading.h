#pragma once
#include "RaylibCompat.h"
#include "Game.h"
#include <atomic>

class Loading {
public:
	Loading(Game* game);
	~Loading();

	void Initialize();
	void Update(float deltaTime);
	void Draw();
	bool IsComplete() const { return loadingComplete; }
	float GetProgress() const { return loadingProgress; }

private:
	void LoadResources();
	void UpdateLoadingProgress(float progress);
	void DrawLoadingProgress();

	Game* game;
	Texture2D poophat;
	float rotationAngle;
	float rotationTimer;
	float loadingProgress;
	bool loadingStarted;
	bool loadingComplete;
	bool poophatLoaded;
	std::atomic<bool> resourcesLoaded{false};
};