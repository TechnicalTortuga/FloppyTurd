#pragma once
#include <raylib.h>
#include <iostream>
#include "ResourceCompat.h"
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
#include "GameStats.h"
#include "MenuButton.h"
#include "TouchControls.h"

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
    void TouchAction(int action, bool down);
    void CancelTouchInput();
    bool IsPaused() const { return isPaused; }
    bool IsGameOver() const { return GAMEOVER; }
    void FadeOutMusic(float deltaTime);
    void SetCurrentLevel(int levelIndex);
    bool IsLoadingLevel() const { return pendingLevel >= 0; }
    int MobileOrientationMask() const { return mobileRun ? (savedLevelIndex==5 ? 32 : 16) : 0; }
    void UnlockSkill(int skillIndex);
    void OutputHatMenu();
    void LoadSessionRecords();
    void UseHighDefFont(bool enable) { useHighDefFont = enable; }
    bool IsHighDefFont() const { return useHighDefFont; }

    void UpdateSessionRecord(int levelIndex, int pipesPassed);
    int GetSessionRecord(int levelIndex) const { return sessionRecords[levelIndex]; }
    int GetDifficultyIndex() const { return difficultyIndex; }

    int GetTotalCoins();
    const Texture2D& GetCoinBagTexture() const { return _CoinBag; }

    GameStats& GetStats() { return stats; }
    void IncrementEnemiesKilled() { stats.totalEnemiesKilled++; }
    void IncrementLevelTries() { stats.totalLevelTries++; }

    int SCORE = 0;
    int COINS = 0;
    int TOTALCOINS = 0;
    int TOTALSCORE = 0;
    Player* player{};
    void TriggerSaveIfPending();
    bool savePending = false;

private:
    bool PurchaseItem(int cost);
    Game* game;

    AudioClip* currentMusic = nullptr;
    AudioClip* gameOverMusic = nullptr;
    BossHealthBar* bossHealthBar = nullptr;

    void DrawUI();
    void DrawMobileHUD();
    void ApplyPlayerSpawn();
    bool mobileRun = false;
    void DrawMobilePause();
    void DrawMobileGameOver();
    void ReturnToMenu();
    void RetryLevel();
    void LeaveGameOver();
    int mobileSelectedHat = 0;
    void UpdatePlayerPosition();

    float deltaTime{};
    std::unique_ptr<LevelManager> levelManager;
    int pendingLevel = -1;
    bool levelLoadFailed = false;
    void CompleteLevelLoad(int levelIndex);
    void DrawLevelLoading();

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

    bool pendingTouchJump = false;
    bool pendingTouchPause = false;
    bool heldTouchShoot = false;
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
    Hat* currentSelectedHat{ nullptr };

    std::unique_ptr<SnowOverlay> snowOverlay;
    bool gameOverTriggered = false;
    bool turdHasFallenOffScreen = false;

    int savedLevelIndex = 0;
    enum class LastLevelType { NONE, PARK, DESERT, SEWER, SNOW, CASTLE, BOSS };
    LastLevelType lastLevelType = LastLevelType::NONE;

    bool useHighDefFont = false;

    int sessionRecords[6] = { 0, 0, 0, 0, 0, 0 };
    int difficultyIndex = 1;

    static constexpr int totalSkills = 8;
    int SkillRank(int index) const;
    int SkillCost(int index) const;
    bool SkillPrerequisite(int index) const;
    const char* SkillTitle(int index) const;
    const char* SkillPurchaseLabel(int index) const;
    int selectedSkill = 0; // Renamed from selectedNode for clarity
    bool skillUnlocked[totalSkills]{ false };
    const char* skillNames[totalSkills]{
        "Turd Shot",
        "Half a Heart",
        "Coin Magnet",
        "Heart Magnet",
        "Split my heart in 3 Pieces",
        "Coin Shield",
        "Homing Shots",
        "Split Shot"
    };
    const char* skillDescs[totalSkills]{
        "Shoot poop with S (1 coin).",
        "Hearts break into halves.",
        "Coins pull toward you.",
        "Hearts pull toward you.",
        "Upgrade half hearts to thirds.",
        "Survive last hit by dropping coins.",
        "Seek enemies. Rank II: stronger.",
        "2 shots, then 3. Costs 1 coin."
    };
    const int skillCosts[totalSkills]{
        50,   // Turd Shot
        150,  // Heart Halves
        150,  // Coin Magnet
        150,  // Heart Magnet
        750,  // Heart Thirds (upgraded cost)
        500,  // Coin Shield
        300,  // Homing rank I; rank II costs 700
        200   // Split rank I; rank II costs 400
    };

    GameStats stats;

    MenuButton* tryAgainYes;
    MenuButton* tryAgainNo;
    
    // Touch controls for mobile
    TouchControls* touchControls = nullptr;
};
