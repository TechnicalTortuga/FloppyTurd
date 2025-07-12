#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

#pragma once

#include <cmath>
#include <cstdint>

// ============================================================================
// COLOR CONSTANTS (only define if not already defined by Raylib)
// ============================================================================

#ifndef PLATFORM_COLORS_DEFINED
#define PLATFORM_COLORS_DEFINED
    #define WHITE           Color{255, 255, 255, 255}
    #define BLACK           Color{0, 0, 0, 255}
    #define RED             Color{255, 0, 0, 255}
    #define GREEN           Color{0, 255, 0, 255}
    #define BLUE            Color{0, 0, 255, 255}
    #define YELLOW          Color{255, 255, 0, 255}
    #define PURPLE          Color{255, 0, 255, 255}
    #define CYAN            Color{0, 255, 255, 255}
    #define ORANGE          Color{255, 165, 0, 255}
    #define PINK            Color{255, 192, 203, 255}
    #define BROWN           Color{165, 42, 42, 255}
    #define GRAY            Color{128, 128, 128, 255}
    #define LIGHTGRAY       Color{211, 211, 211, 255}
    #define DARKGRAY        Color{64, 64, 64, 255}
    #define MAROON          Color{128, 0, 0, 255}
    #define LIME            Color{0, 255, 0, 255}
    #define NAVY            Color{0, 0, 128, 255}
    #define OLIVE           Color{128, 128, 0, 255}
    #define TEAL            Color{0, 128, 128, 255}
    #define VIOLET          Color{128, 0, 128, 255}
#endif

// ============================================================================
// MATH CONSTANTS (only define if not already defined by Raylib)
// ============================================================================

#ifndef PLATFORM_MATH_CONSTANTS_DEFINED
#define PLATFORM_MATH_CONSTANTS_DEFINED
    #define PI              3.14159265358979323846f
    #define DEG2RAD         (PI / 180.0f)
    #define RAD2DEG         (180.0f / PI)
#endif

// ============================================================================
// UTILITY FUNCTIONS (only define if not already defined by Raylib)
// ============================================================================

#ifndef PLATFORM_UTILITY_FUNCTIONS_DEFINED
#define PLATFORM_UTILITY_FUNCTIONS_DEFINED
    inline float PlatformClamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    inline float PlatformLerp(float start, float end, float amount) {
        return start + (end - start) * amount;
    }

    inline int PlatformRound(float value) {
        return static_cast<int>(value + 0.5f);
    }

    inline float PlatformMin(float a, float b) {
        return (a < b) ? a : b;
    }

    inline float PlatformMax(float a, float b) {
        return (a > b) ? a : b;
    }

    inline float PlatformAbs(float value) {
        return (value < 0) ? -value : value;
    }

    inline float PlatformSqrt(float value) {
        return sqrtf(value);
    }

    inline float PlatformSin(float angle) {
        return sinf(angle);
    }

    inline float PlatformCos(float angle) {
        return cosf(angle);
    }

    inline float PlatformTan(float angle) {
        return tanf(angle);
    }

    inline float PlatformAtan2(float y, float x) {
        return atan2f(y, x);
    }
#endif

#endif // PLATFORM_TYPES_H 