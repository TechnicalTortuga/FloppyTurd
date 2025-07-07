#include "PlatformLayer.h"
#include <cstdarg>
#include <cstdlib>
#include "RaylibCompat.h"
#include "Game.h"
#include <cstdint>
#include <ctime>
#include <cmath>

// Global state - these are the only things that should be here
static bool g_shouldClose = false;
static int g_targetFPS = 60;
static uint32_t g_frameStartTime = 0;
static Vector2 g_mousePosition = {0, 0};
static Vector2 g_mouseDelta = {0, 0};
static bool g_mousePressed[3] = {false, false, false};
static bool g_mouseReleased[3] = {false, false, false};
static Font g_defaultFont = {nullptr, 16};
static bool g_audioInitialized = false;
static bool g_windowFullscreen = false;
static double g_startTime = 0.0;
static double g_lastFrameTime = 0.0;

// ========== TRULY NON-RAYLIB UTILITIES ==========

// Basic clamp function (not raylib-specific)
float Clamp(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// Basic random number generator (not raylib-specific)
int GetRandomValue(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    
    return min + (rand() % (max - min + 1));
}

// ========== APP LIFECYCLE ==========

void OnAppPause()
{
    // Non-iOS implementation will be handled separately
}

void OnAppResume()
{
    // Non-iOS implementation will be handled separately
}


