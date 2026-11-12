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
	RenderTexture2D target{};
	int lastWebUiState = -1;
	int warmupFrames = 2;
	bool touchPresentation = false;
	bool hostPaused = false;
	bool focusPaused = false;

	void Update();
	void Draw();
	void HandleInput();

	Font whackyJoe;
};