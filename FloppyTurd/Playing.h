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

class Playing
{
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
    Vector2 scrollOffset;

    // Feedback system for high-definition font
    void UseHighDefFont(bool enable) { useHighDefFont = enable; }
    bool IsHighDefFont() const { return useHighDefFont; }

    // Session records for level unlocks
    void UpdateSessionRecord(int levelIndex, int pipesPassed);
    int GetSessionRecord(int levelIndex) const { return sessionRecords[levelIndex]; }

    int SCORE = 0; // Session pipes passed
    int COINS = 0; // Session coins (unused, kept for compatibility)
    int TOTALCOINS = 0; // Lifetime coins
    int TOTALSCORE = 0; // Lifetime pipes passed

private:
    Game* game;
    Player* player{};
    AudioClip* currentMusic = nullptr;
    AudioClip* gameOverMusic = nullptr;
    BossHealthBar* bossHealthBar = nullptr;

    void DrawUI();
    void UpdatePlayerPositionInLevel();

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
    Texture2D _TurdPointMenu;
    Texture2D _TurdPointMenuBorder;
    Texture2D _TurdPointInfo;
    Texture2D hatIcons[15];

    Vector2 skillMenuScrollOffset = { 0, 0 };
    Texture2D skillNodeTextures[4];
    static constexpr int totalSkillNodes = 6;
    Vector2 skillNodePositions[totalSkillNodes];
    const char* skillNames[totalSkillNodes]{
       "Shoot",
       "Heart Halves",
       "Teenage Turd",
       "Hollow Turds",
       "Big Turd",
       "Heart Thirds"
    };
    const char* skillDescs[totalSkillNodes]{
       "Unlocks the ability to fire poop projectiles (F key).",
       "Each heart is split into two slices; one hit removes only half.",
       "Unlocks the teenage-turd player form (cosmetic & future buffs).",
       "Projectiles consume ghost slices first – real HP is safe!",
       "Unlocks the big-turd player form (cosmetic & future buffs).",
       "Further divides hearts into three slices each."
    };
    bool skillUnlocked[totalSkillNodes]{ false };
    int selectedNode = -1;
    int turdPoints = 3;

    Vector2 transformedMousePos;

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

    // Session records for each level (pipes passed in a single session)
    int sessionRecords[6] = { 0, 0, 0, 0, 0, 0 }; // Park, Sewer, Desert, Snow, Castle, Rat King
};