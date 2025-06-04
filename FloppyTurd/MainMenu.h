// MainMenu.h
#pragma once
#include <raylib.h>
#include <iostream>
#include "Game.h"
#include "AudioClip.h"
#include "QuickplaySettings.h" // Include new header

class Game;

enum MenuState
{
    MAIN_MENU,
    LEVEL_SELECT,
    OPTIONS_MENU,
    QUICKPLAY_SETTINGS
};

class MainMenu
{
public:
    MainMenu(Game* game);
    ~MainMenu();

    void Update();
    void PlayMusic(AudioClip* clip);
    void UpdateMusic();
    void Draw();
    void HandleInput();

    void ResetMusic();
    AudioClip* GetAudioClip() const { return currentMusic; }
    const QuickplaySettings& GetQuickplaySettings() const { return quickplaySettings; }

private:
    void PlayRandomFartSound();
    void ToggleFartMusic();

    Game* game;
    MenuState currentMenu{ MAIN_MENU };
    Texture2D _MenuBackground;
    Texture2D _FloppyLogo;
    Texture2D emptyPainting;
    int currentLevelIndex{ 0 };
    bool levelSelectMode = false;
    bool levelsUnlocked[6] = { true, true, true, true, true, true };

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
};