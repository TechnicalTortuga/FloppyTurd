#pragma once
#include <raylib.h>
#include <iostream>
#include "Resources.h"
#include "Player.h"
#include "Game.h"
#include <vector>
#include "Level.h"
#include "ParkLevel.h"
#include "DesertLevel.h"
#include "SewerLevel.h"
#include "CastleLevel.h"
#include <memory>
#include "LevelManager.h"
#include "Hat.h"
#include "BossHealthBar.h"
#include "SnowOverlay.h"
#include "QuickplaySettings.h"
#include "MainMenu.h"
#include "GameStats.h" // Added for stats tracking

class MainMenu;
class Player;
class Game;

enum class Difficulty { RUNNY, REGULAR, ROUGH };

enum PauseMenuTab {
	SYSTEM,
	STATS,
	HATS,
	SKILLS
};

class Playing {
public:
	Playing(Game* game);
	~Playing();

	void InitializeSkillNodes();
	void InitializeHats();

	void DrawPauseMenu();
	void DrawGameOverScreen();
	void Update();
	void Draw();
	void PlayMusic(AudioClip* clip);
	void UpdateMusic();
	void HandleInput();
	void FadeOutMusic(float deltaTime);
	void SetCurrentLevel(int levelIndex);
	void PreLoadLevels();
	void UnlockSkill(int nodeIndex);
	void OutputHatMenu();
	void LoadSessionRecords();
	void UseHighDefFont(bool enable) { useHighDefFont = enable; }
	bool IsHighDefFont() const { return useHighDefFont; }

	void UpdateSessionRecord(int levelIndex, int pipesPassed);
	int GetSessionRecord(int levelIndex) const { return sessionRecords[levelIndex]; }
	int GetDifficultyIndex() const { return difficultyIndex; }

	// New public methods to access private data safely
	int GetTotalCoins();
	const Texture2D& GetCoinBagTexture() const { return _CoinBag; }

	// Stats access
	GameStats& GetStats() { return stats; }
	void IncrementEnemiesKilled() { stats.totalEnemiesKilled++; }
	void IncrementLevelTries() { stats.totalLevelTries++; }

	int SCORE = 0;
	int COINS = 0;
	int TOTALCOINS = 0;
	int TOTALSCORE = 0;
	Player* player{};
	void TriggerSaveIfPending();
	bool savePending = false; // Flag to debounce saves

private:
	bool PurchaseItem(int cost);
	Game* game;

	AudioClip* currentMusic = nullptr;
	AudioClip* gameOverMusic = nullptr;
	BossHealthBar* bossHealthBar = nullptr;

	void DrawUI();
	void UpdatePlayerPosition();

	float deltaTime{};
	std::unique_ptr<LevelManager> levelManager;
	std::vector<std::shared_ptr<Level>> levels;

	Texture2D gameOverBackground;
	Texture2D tryAgainBackground;
	Texture2D deadFloppy;
	Texture2D gameOverScore;
	float gameOverHoverTimer = 0.0f;
	bool GAMEOVER = false;

	QuickplaySettings quickplaySettings;

	Texture2D Scoreboard;
	Texture2D _TurdHeart;
	Texture2D _CoinBag;
	Texture2D floppyButtonBlue;
	Texture2D floppyButtonBlueHover;
	Sound ScoreSound;

	bool isPaused = false;
	PauseMenuTab currentTab{ SYSTEM };
	Texture2D pauseMenuBackground;
	Texture2D arrowLeft;
	Texture2D arrowRight;
	Texture2D arrowLeftHover;
	Texture2D arrowRightHover;

	Texture2D hatFrameNormal;
	Texture2D hatFrameHover;
	Texture2D hatFrameSelected;
	Texture2D hatFrameLocked;
	Texture2D hatFrameDenied;

	std::vector<Hat*> hats;
	Hat* currentSelectedHat;

	std::unique_ptr<SnowOverlay> snowOverlay;
	bool gameOverTriggered = false;
	bool turdHasFallenOffScreen = false;

	int savedLevelIndex = 0;
	enum class LastLevelType { NONE, PARK, DESERT, SEWER, SNOW, CASTLE, BOSS };
	LastLevelType lastLevelType = LastLevelType::NONE;

	bool useHighDefFont = false;

	int sessionRecords[6] = { 0, 0, 0, 0, 0, 0 };
	int difficultyIndex = 1;

	static constexpr int totalSkillNodes = 5;
	int selectedNode = 0;
	bool skillUnlocked[totalSkillNodes]{ false };
	const char* skillNames[totalSkillNodes]{
		"Turd Shot",
		"Heart Halves",
		"Coin Magnet",
		"Big Turd Form",
		"Heart Thirds"
	};
	const char* skillDescs[totalSkillNodes]{
		"Shoot poop with F (1 coin).",
		"Hearts break into halves.",
		"Coins pull toward you.",
		"Powerful form for 15s.",
		"Hearts break into thirds."
	};
	const int skillCosts[totalSkillNodes]{
		0,   // Turd Shot
		100, // Heart Halves
		100, // Coin Magnet
		150, // Big Turd Form
		100  // Heart Thirds
	};

	GameStats stats; // Persistent game statistics
};