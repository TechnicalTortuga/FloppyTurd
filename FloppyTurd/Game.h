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
	void RunGame();

	void Update();
	void Draw();
	void HandleInput();

	Font whackyJoe;
};