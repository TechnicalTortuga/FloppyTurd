#pragma once
#include <iostream>
#include <raylib.h>

#include "Window.h"
#include "MainMenu.h"
#include "Playing.h"
#include "GameSettings.h"
#include "Credits.h" // Include Credits header

class Window;
class MainMenu;
class PauseMenu;
class Playing;
class Credits;

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
        CREDITS
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
    Credits* credits; // Add Credits instance

    void InitClasses();
    void RunGame();

    void Update();
    void Draw();
    void HandleInput();

    Font whackyJoe;
};