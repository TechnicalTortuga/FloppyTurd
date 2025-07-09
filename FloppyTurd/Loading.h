#pragma once
#include "RaylibCompat.h"
#include "Game.h"
#include "ResourceManager.h"
#include <atomic>
#include <thread>
#include <future>

class Loading {
public:
	Loading(Game* game);
	~Loading();

	void Initialize();
	void Update(float deltaTime);
	void Draw();
	void Shutdown();
	bool IsComplete() const { return loadingComplete; }
	float GetProgress() const { return loadingProgress; }

	void UpdateLoadingProgress(float progress);
	void DrawLoadingProgress();
	bool LoadFontsInBackground();

	Game* game;
	Texture2D poophat;
	float rotationAngle;
	float rotationTimer;
	float loadingProgress;
	bool loadingStarted;
	bool loadingComplete;
	bool poophatLoaded;
	// Removed resourcesLoaded and loadingThread
	// Only use m_fontLoadingFuture for background font loading

	// Background font loading
	std::future<bool> m_fontLoadingFuture;
	bool m_fontLoadingComplete;
};