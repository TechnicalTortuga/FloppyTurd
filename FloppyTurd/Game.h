#pragma once
#include <iostream>
#include <atomic>
#include "RaylibCompat.h"
#include <iostream>
#include <mutex>

#include "Window.h"
#include "MainMenu.h"
#include "Playing.h"
#include "GameSettings.h"
#include "Credits.h"
#include "Loading.h" // New include
#include "GameLog.h"
#include "GameState.h"
#include "AudioStateManager.h"

// Cross-platform logging macro
#if defined(__OBJC__) && defined(__APPLE__) && TARGET_OS_IPHONE
    #import <Foundation/Foundation.h>
    #define GAME_LOG(fmt, ...) NSLog((@"[GAME] " fmt), ##__VA_ARGS__)
#else
    #define GAME_LOG(fmt, ...) printf("[GAME] " fmt "\n", ##__VA_ARGS__)
#endif

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

	// Game state enum moved to GameState.h to avoid circular dependencies

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
	
	// Audio state management
	void UpdateAudioState();     // Update audio based on current game state
	void SetLevelAudio(int levelNumber, AudioStateManager::Difficulty difficulty); // Set level audio with difficulty
	
	// Accessors for iOS integration
	bool IsInitialized() const { 
    std::lock_guard<std::mutex> lock(initializedMutex);
    bool value = initialized;
    GameLog::Log("[DEBUG] IsInitialized() called, returning: %s", value ? "true" : "false");
    GameLog::Log("[DEBUG] IsInitialized() - initialized variable address: %p", &initialized);
    GameLog::Log("[DEBUG] IsInitialized() - initialized variable value: %d", (int)value);
    
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

	void Update(float deltaTime = 0.0f);
	void Draw();
	void HandleInput();

	Font whackyJoe;
	
	// Game loop state
	bool initialized;
	mutable std::mutex initializedMutex;
	RenderTexture2D renderTarget;
	
	// Letterbox calculation state (preserved between frames)
	float gameScale;
	float gameOffsetX;
	float gameOffsetY;
	float renderedWidth;
	float renderedHeight;
};