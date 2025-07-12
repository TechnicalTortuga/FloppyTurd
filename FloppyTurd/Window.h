#pragma once
#include "PlatformAPI.h"

// Creates the window either in true-fullscreen or windowed mode using the monitor's native resolution.
// Automatically detects the current monitor's resolution and uses it as the default window size.
//      • Pass  fullScreen = true   → native-resolution fullscreen
//      • Pass  fullScreen = false  → native-resolution windowed mode
//      • fallbackWidth/Height = 0  → use native monitor resolution (default)
//      • fallbackWidth/Height > 0  → use specified size if smaller than monitor (prevents overflow)
class Window
{
public:
    explicit Window(bool fullScreen = true,
        int fallbackWidth = 0,
        int fallbackHeight = 0);
    ~Window();

    // Desktop window management
    void ToggleMode();
    void GetOptimalWindowedSize(int& width, int& height);
    void SetBorderlessFullscreen(bool enable);
    bool IsBorderlessFullscreen() const;

    // Mobile-specific screen management
    Rectangle GetSafeArea() const;
    float GetScreenDensity() const;
    bool IsLandscape() const;
    bool IsPortrait() const;
    void SetPreferredOrientation(bool landscape);
    
    // Universal screen utilities
    Vector2 GetScreenCenter() const;
    Vector2 GetRenderScale() const;  // For scaling UI elements based on screen density
    bool ShouldUseLargerTouchTargets() const;
    int GetRecommendedFontSize() const;

private:
    bool borderlessFullscreen = false;
    int detectedMonitorWidth = 0;
    int detectedMonitorHeight = 0;
    bool preferLandscape = true;  // Default preference for game
};