#pragma once
#include <raylib.h>

// Creates the window either in true-fullscreen at the user’s monitor size
// or in a fixed border-less 1920×1080 fallback.
//      • Pass  fullScreen = true   → native-resolution fullscreen
//      • Pass  fullScreen = false  → 1920×1080 “border-less window”
class Window
{
public:
    explicit Window(bool fullScreen = true,
        int fallbackWidth = 1920,
        int fallbackHeight = 1080);
    ~Window();

    // Optional helper if you want a quick UI toggle:
    void ToggleMode() { ::ToggleFullscreen(); }
};