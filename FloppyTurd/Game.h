#pragma once
#include <iostream>
#include <atomic>
#include "RaylibCompat.h"
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

	// Platform-agnostic game interface
	bool Initialize();           // One-time setup (platform-independent) - returns success
	void UpdateFrame(float deltaTime); // Single frame update (platform-independent)
	void RenderFrame();          // Single frame render (platform-independent)
	void Shutdown();             // Resource cleanup (platform-independent)
	
	// Platform-specific runners
	void RunGameDesktop();       // Traditional loop for desktop platforms
	
	// iOS lifecycle hooks
	void OnPause();              // App going to background
	void OnResume();             // App returning to foreground
	
	// Accessors for iOS integration
	bool IsInitialized() const { 
    bool value = initialized.load();
    std::cout << "[DEBUG] IsInitialized() called, returning: " << (value ? "true" : "false") << std::endl;
    std::cout << "[DEBUG] IsInitialized() - initialized variable address: " << &initialized << std::endl;
    std::cout << "[DEBUG] IsInitialized() - initialized variable value: " << value << std::endl;
    return value; 
}

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
	
	// Game loop state
	std::atomic<bool> initialized;
	RenderTexture2D renderTarget;
	
	// Letterbox calculation state (preserved between frames)
	float gameScale;
	float gameOffsetX;
	float gameOffsetY;
	float renderedWidth;
	float renderedHeight;
};