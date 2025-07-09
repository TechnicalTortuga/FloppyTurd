#pragma once
#include "RaylibCompat.h"
#include <iostream>
#include "Game.h"
#include "AudioClip.h"
#include "QuickplaySettings.h"

class Game;

enum MenuState
{
	MAIN_MENU,
	LEVEL_SELECT,
	OPTIONS_MENU,
	QUICKPLAY_SETTINGS,
	MOBILE_OPTIONS_MENU
};

class MainMenu
{
public:
	MainMenu(Game* game);
	~MainMenu();

	void Update();
	void PlayMusic(AudioClip* clip);
	void UpdateMusic();
	bool CanPurchaseLevel(int levelIndex, int totalCoins, const int sessionRecords[6]);
	void Draw();
	void HandleInput();

	void ResetMusic();
	AudioClip* GetAudioClip() const { return currentMusic; }
	const QuickplaySettings& GetQuickplaySettings() const { return quickplaySettings; }
	int GetDifficultyIndex() const { return difficultyIndex; } // New getter for difficulty

	// Feedback system for high-definition font
	void UseHighDefFont(bool enable) { useHighDefFont = enable; }
	bool IsHighDefFont() const { return useHighDefFont; }

	// Level unlock management
	bool IsLevelUnlocked(int levelIndex) const { return levelsUnlocked[levelIndex]; }
	bool PurchaseLevel(int levelIndex);
	void UpdateLevelUnlocks(int totalCoins, const int sessionRecords[6]);
	bool levelsUnlocked[6] = { true, false, false, false, false, false }; // Park unlocked, Sewer requires 50 pipes, others depend on progress

private:
	void PlayRandomFartSound();
	void ToggleFartMusic();
	void DrawDesktopUI();
	void DrawMobileUI();
	void DrawMobileLevelSelect();
	void DrawMobileOptionsMenu();

	Game* game;
	MenuState currentMenu{ MAIN_MENU };
	Texture2D _MenuBackground;
	Texture2D _FloppyLogo;
	Texture2D emptyPainting;
	int currentLevelIndex{ 0 };
	bool levelSelectMode = false;
	
	// Mobile level select variables
	float levelSelectScrollOffset;
	float lastTouchX;
	bool isDragging;

	Texture2D levelPaintings[6];
	Texture2D lockedPainting;

	float paintingHoverScale = 1.0f;
	Texture2D finLogo;
	float fHoverScale = 1.0f;
	int fClickCount = 0;
	float fClickCooldown = 0.0f;
	bool fartModeEnabled = false;

	AudioClip* currentMusic = nullptr;

	QuickplaySettings quickplaySettings;

	bool useHighDefFont = false; // Flag for high-definition font

	int difficultyIndex = 1; // Moved from .cpp to ensure accessibility
	const char* difficultyLevels[3] = { "Runny", "Regular", "Rough" }; // Moved for clarity
};