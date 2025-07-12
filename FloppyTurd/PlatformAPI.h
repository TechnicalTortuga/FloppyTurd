#ifndef PLATFORM_API_H
#define PLATFORM_API_H

#pragma once

#include <string>
#include <vector>
#include <TargetConditionals.h>

// Platform detection macros
#if !defined(PLATFORM_MOBILE) && defined(__ANDROID__)
    #define PLATFORM_MOBILE
    #define PLATFORM_ANDROID
#elif !defined(PLATFORM_IOS) && defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define PLATFORM_IOS
        #define PLATFORM_MOBILE
    #endif
#endif

// ============================================================================
// PLATFORM API - UNIFIED INTERFACE
// ============================================================================

/**
 * PlatformAPI - Unified Raylib-compatible interface for all platforms
 * 
 * This header provides a single entry point for all Raylib function calls.
 * - Desktop: Direct Raylib calls via raylib.h include
 * - iOS: Function redefinitions that route to Metal implementations
 * 
 * Usage: All game files include this header and get access to Raylib types
 * and functions automatically.
 */

#if defined(PLATFORM_IOS)
    // ============================================================================
    // iOS: REDEFINE ALL FUNCTIONS FOR METAL IMPLEMENTATION
    // ============================================================================
    
    // iOS-specific type definitions (compatible with Raylib)
    struct Vector2 { float x, y; };
    struct Color { unsigned char r, g, b, a; };
    struct Rectangle { float x, y, width, height; };
    struct Texture2D { unsigned int id; int width, height; };
    struct Image { void* data; int width, height; int mipmaps; int format; };
    struct Font { unsigned int baseSize; unsigned int glyphCount; unsigned int glyphPadding; Texture2D texture; Rectangle* recs; int* glyphs; };
    struct Sound { void* player; };
    struct Music { void* player; };
    
    // ============================================================================
    // COLOR CONSTANTS (iOS redefinitions)
    // ============================================================================
    
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
    #define BLANK           Color{0, 0, 0, 0}
    #define MAGENTA         Color{255, 0, 255, 255}
    #define GOLD            Color{255, 203, 0, 255}
    #define RAYWHITE        Color{245, 245, 245, 255}
    
    // ============================================================================
    // MATH CONSTANTS (iOS redefinitions)
    // ============================================================================
    
    #define PI              3.14159265358979323846f
    #define DEG2RAD         (PI / 180.0f)
    #define RAD2DEG         (180.0f / PI)
    
    // ============================================================================
    // WINDOW FLAGS (iOS redefinitions)
    // ============================================================================
    
    #define FLAG_WINDOW_RESIZABLE   0x00000004
    #define FLAG_VSYNC_HINT         0x00000040
    #define FLAG_WINDOW_MAXIMIZED   0x00000200
    
    // ============================================================================
    // LOGGING LEVELS (iOS redefinitions)
    // ============================================================================
    
    #define LOG_ALL     0
    #define LOG_TRACE   1
    #define LOG_DEBUG   2
    #define LOG_INFO    3
    #define LOG_WARNING 4
    #define LOG_ERROR   5
    #define LOG_FATAL   6
    #define LOG_NONE    7
    
    // ============================================================================
    // MOUSE BUTTONS (iOS redefinitions)
    // ============================================================================
    
    #define MOUSE_LEFT_BUTTON 0
    #define MOUSE_RIGHT_BUTTON 1
    #define MOUSE_MIDDLE_BUTTON 2
    #define MOUSE_BUTTON_LEFT 0
    #define MOUSE_BUTTON_RIGHT 1
    #define MOUSE_BUTTON_MIDDLE 2
    
    // ============================================================================
    // KEYBOARD KEYS (iOS redefinitions)
    // ============================================================================
    
    #define KEY_NULL            0
    #define KEY_SPACE           32
    #define KEY_ENTER           257
    #define KEY_ESCAPE          256
    #define KEY_A               65
    #define KEY_B               66
    #define KEY_C               67
    #define KEY_D               68
    #define KEY_E               69
    #define KEY_F               70
    #define KEY_G               71
    #define KEY_H               72
    #define KEY_I               73
    #define KEY_J               74
    #define KEY_K               75
    #define KEY_L               76
    #define KEY_M               77
    #define KEY_N               78
    #define KEY_O               79
    #define KEY_P               80
    #define KEY_Q               81
    #define KEY_R               82
    #define KEY_S               83
    #define KEY_T               84
    #define KEY_U               85
    #define KEY_V               86
    #define KEY_W               87
    #define KEY_X               88
    #define KEY_Y               89
    #define KEY_Z               90
    #define KEY_F1              290
    #define KEY_F2              291
    #define KEY_F3              292
    #define KEY_F4              293
    #define KEY_F5              294
    #define KEY_F6              295
    #define KEY_F7              296
    #define KEY_F8              297
    #define KEY_F9              298
    #define KEY_F10             299
    #define KEY_F11             300
    #define KEY_F12             301
    
    // ============================================================================
    // GESTURE DEFINITIONS (iOS redefinitions)
    // ============================================================================
    
    #define GESTURE_NONE        0
    #define GESTURE_TAP         1
    #define GESTURE_DOUBLETAP   2
    #define GESTURE_HOLD        4
    #define GESTURE_DRAG        8
    #define GESTURE_SWIPE_RIGHT 16
    #define GESTURE_SWIPE_LEFT  32
    #define GESTURE_SWIPE_UP    64
    #define GESTURE_SWIPE_DOWN  128
    #define GESTURE_PINCH_IN    256
    #define GESTURE_PINCH_OUT   512
    
    // ============================================================================
    // PIXEL FORMAT CONSTANTS (iOS redefinitions)
    // ============================================================================
    
    #define PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 7
    
    // ============================================================================
    // TEXTURE WRAP MODE CONSTANTS (iOS redefinitions)
    // ============================================================================
    
    #define TEXTURE_WRAP_REPEAT        0
    #define TEXTURE_WRAP_CLAMP         1
    #define TEXTURE_WRAP_MIRROR_REPEAT 2
    #define TEXTURE_WRAP_MIRROR_CLAMP  3
    
    // ============================================================================
    // UTILITY FUNCTIONS (iOS implementations)
    // ============================================================================
    
    inline float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    inline float Lerp(float start, float end, float amount) {
        return start + (end - start) * amount;
    }

    inline int Round(float value) {
        return static_cast<int>(value + 0.5f);
    }

    inline float Min(float a, float b) {
        return (a < b) ? a : b;
    }

    inline float Max(float a, float b) {
        return (a > b) ? a : b;
    }

    inline float Abs(float value) {
        return (value < 0) ? -value : value;
    }

    inline float Sqrt(float value) {
        return sqrtf(value);
    }

    inline float Sin(float angle) {
        return sinf(angle);
    }

    inline float Cos(float angle) {
        return cosf(angle);
    }

    inline float Tan(float angle) {
        return tanf(angle);
    }

    inline float Atan2(float y, float x) {
        return atan2f(y, x);
    }
    
    // Window management
    void InitWindow(int width, int height, const char* title);
    void CloseWindow(void);
    bool WindowShouldClose(void);
    void SetTargetFPS(int fps);
    int GetScreenWidth(void);
    int GetScreenHeight(void);
    void SetWindowSize(int width, int height);
    bool IsWindowFullscreen(void);
    void ToggleFullscreen(void);
    
    // Drawing functions
    void BeginDrawing(void);
    void EndDrawing(void);
    void ClearBackground(Color color);
    void DrawRectangle(int posX, int posY, int width, int height, Color color);
    void DrawRectangleRec(Rectangle rec, Color color);
    void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);
    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
    void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    void DrawCircle(int centerX, int centerY, float radius, Color color);
    void DrawCircleV(Vector2 center, float radius, Color color);
    void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
    
    // Texture functions
    Texture2D LoadTexture(const char* fileName);
    void UnloadTexture(Texture2D texture);
    void SetTextureWrap(Texture2D texture, int wrap);
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
    void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
    void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
    
    // Image functions
    Image LoadImage(const char* fileName);
    void UnloadImage(Image image);
    Image GenImageColor(int width, int height, Color color);
    void ImageResize(Image* image, int newWidth, int newHeight);
    void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
    Texture2D LoadTextureFromImage(Image image);
    
    // Audio functions
    void InitAudioDevice(void);
    void CloseAudioDevice(void);
    Sound LoadSound(const char* fileName);
    void UnloadSound(Sound sound);
    void PlaySound(Sound sound);
    void SetSoundVolume(Sound sound, float volume);
    Music LoadMusic(const char* fileName);
    void UnloadMusic(Music music);
    void PlayMusic(Music music);
    void StopMusic(Music music);
    void UpdateMusic(Music music);
    bool IsMusicPlaying(Music music);
    void SetMusicVolume(Music music, float volume);
    void PauseMusic(Music music);
    void ResumeMusic(Music music);
    void SetMusicLooping(Music music, bool looping);
    
    // Font functions
    Font LoadFont(const char* fileName);
    void UnloadFont(Font font);
    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing);
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint);
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color);
    int MeasureText(const char* text, int fontSize);
    
    // Scissor mode
    void BeginScissorMode(int x, int y, int width, int height);
    void EndScissorMode();
    
    // Input functions
    bool IsMouseButtonDown(int button);
    bool IsMouseButtonReleased(int button);
    bool IsKeyPressed(int key);
    bool IsKeyDown(int key);
    Vector2 GetMousePosition(void);
    Vector2 GetMouseDelta(void);
    
    // Time functions
    double GetTime(void);
    float GetFrameTime(void);
    int GetRandomValue(int min, int max);
    void TraceLog(int logLevel, const char* text, ...);
    
    // Collision functions
    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
    bool CheckCollisionPointRec(Vector2 point, Rectangle rec);
    bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
    
    // Vector math functions
    Vector2 Vector2Add(Vector2 v1, Vector2 v2);
    Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);
    Vector2 Vector2Scale(Vector2 v, float scale);
    float Vector2Distance(Vector2 v1, Vector2 v2);
    float Vector2Length(Vector2 v);
    Vector2 Vector2Normalize(Vector2 v);
    
    // Color functions
    Color ColorAlpha(Color color, float alpha);
    Color Fade(Color color, float alpha);
    Color ColorLerp(Color a, Color b, float t);
    
    // Utility functions
    float Clamp(float value, float min, float max);

#else
    // ============================================================================
    // DESKTOP: DIRECT RAYLIB USAGE
    // ============================================================================
    
    // Include raylib.h for desktop platforms - all functions available directly
    #include "raylib.h"
    
    // No redefinitions needed - just use Raylib directly
    // All game code can call Raylib functions as normal

#endif

#endif // PLATFORM_API_H 