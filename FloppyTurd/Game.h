#pragma once
#include <iostream>
#include <raylib.h>
#include <iostream>

#include "Window.h"
#include "MainMenu.h"
#include "Playing.h"
#include "GameSettings.h"
#include "Credits.h"
#include "Loading.h" // New include

class Window;
class MainMenu;
class PauseMenu;
class Playing;
class Credits;
class Loading;

class Game
{
public:
	Game();
	void RunGame();
	void Tick();
	void SetHostPaused(bool paused);
	void TouchAction(int action, bool down);
    void PointerInput(int phase, float x, float y);
    void SetInlinePresentation(bool enabled);
    void SetAudioActivated(bool enabled);
    void SetSafeArea(float top, float right, float bottom, float left);
    void SetOrientationWaiting(bool waiting);
    bool IsTouchPresentation() const { return touchPresentation; }
	~Game();

	enum GAMESTATE
	{
		MAINMENU,
		PLAYING,
		PAUSEMENU,
		SHUTDOWN,
		CREDITS,
		LOADING // New loading state
	};

	void SetGameState(GAMESTATE newState);
	Playing* playing;
	MainMenu* mainMenu;

	// Add method to get a scaled font instance
	Font GetScaledFont(float scaleFactor);

private:
	Window* window;
	GAMESTATE gamestate;
	//PauseMenu* pauseMenu;
	Credits* credits;
	Loading* loading; // New loading instance

	void InitClasses();
    void RefreshLayout();
	RenderTexture2D target{};
	int lastWebUiState = -1;
	int warmupFrames = 2;
	bool hostManagedVisibility = false;
	bool inlinePresentation = false;
	bool audioActivated = true;
	bool touchPresentation = false;
	bool hostPaused = false;
    bool orientationWaiting = false;
	bool focusPaused = false;

	void Update();
	void Draw();
	void HandleInput();

	Font whackyJoe;
};
