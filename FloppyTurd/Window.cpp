#include "Window.h"

Window::Window(bool fullScreen, int fallbackW, int fallbackH)
{
    // Decide the target size & flags up-front
    int width = fallbackW;
    int height = fallbackH;

    if (fullScreen)
    {
        int mon = GetCurrentMonitor();
        width = GetMonitorWidth(mon);
        height = GetMonitorHeight(mon);
        SetConfigFlags(FLAG_FULLSCREEN_MODE | FLAG_VSYNC_HINT);
    }
    else
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    }

    InitWindow(width, height, "Floppy Turd");
    SetExitKey(KEY_NULL);          // ESC will be handled by your own code
}

Window::~Window()
{
    CloseWindow();
}
