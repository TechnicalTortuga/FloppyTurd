#pragma once
#include <iostream>
#include <raylib.h>

#include "Window.h"
#include "MainMenu.h"
#include "Playing.h"
#include "GameSettings.h"

class Window;
class MainMenu;
class PauseMenu;
class Playing;

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
        SHUTDOWN
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

    void InitClasses();
    void RunGame();

    void Update();
    void Draw();
    void HandleInput();

    Font whackyJoe;
};