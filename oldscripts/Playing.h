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
#include "LevelManager.h"  // New!
#include "Hat.h"

class Player;
class Game;

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

private:
    Game* game;
    Player* player{};
    AudioClip* currentMusic = nullptr;

    void DrawUI();

    void UpdatePlayerPositionInLevel();

    float deltaTime{};
    // Remove direct currentLevel pointer. Instead, we now use the LevelManager.
    std::unique_ptr<LevelManager> levelManager;
    std::vector<std::shared_ptr<Level>> levels;

    int SCORE;
    Texture2D Scoreboard;
    Texture2D _TurdHeart;
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
    static constexpr int totalSkillNodes = 6;
    Vector2 skillNodePositions[totalSkillNodes];
    Texture2D skillNodeTextures[4];
    bool skillNodeStates[totalSkillNodes] = { false };
    bool isSkillMenuActive = false;
    int selectedNode = -1;

    Vector2 transformedMousePos;

    Texture2D hatFrameNormal;
    Texture2D hatFrameHover;
    Texture2D hatFrameSelected;
    Texture2D hatFrameLocked;
    Texture2D hatFrameDenied;  // For denied confirmation

    std::vector<Hat*> hats;
    Hat* currentSelectedHat;
};
