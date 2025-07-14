#ifndef PLATFORM_TRAITS_H
#define PLATFORM_TRAITS_H

#include "PlatformTypes.h"  // For types like Color, Vector2
#include "GlobalStateManager.h"  // For cross-platform state management
#include <cmath>
#include <cstdlib>
#include <ctime>

// ============================================================================
// SHARED CONSTANTS AND UTILITIES
// ============================================================================

// Math constants (from raylib)
#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

// Utility functions (from raylib)
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

#ifdef PLATFORM_IOS
// Forward declarations for MetalRenderer and MetalTextRenderer
class MetalRenderer;
class MetalTextRenderer;
extern MetalRenderer* g_metalRenderer;
extern MetalTextRenderer* g_textRenderer;

// Only include Objective-C headers in Objective-C++ files
#ifdef __OBJC__
// iOS-specific includes and externs
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#endif

#else
#include "raylib.h"
#endif

struct IOSTraits {
    static constexpr bool IsIOS = true;

    // ============================================================================
    // RENDERING FUNCTIONS
    // ============================================================================
    
    static void DrawRectangle(float x, float y, float width, float height, Color color);
    static void DrawRectangleRec(Rectangle rec, Color color);
    static void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);
    static void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
    static void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    static void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    static void DrawCircle(float centerX, float centerY, float radius, Color color);
    static void DrawCircleV(Vector2 center, float radius, Color color);
    static void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color);
    static void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
    static void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
    static void DrawTexture(Texture2D texture, float x, float y, Color tint);
    static void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
    static void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
    static void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
    static void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
    static void DrawText(const char* text, float x, float y, float fontSize, Color color);
    static void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint);
    static int MeasureText(const char* text, int fontSize);
    static const char* TextFormat(const char* text, va_list args);

    // ============================================================================
    // INPUT FUNCTIONS
    // ============================================================================
    
    static bool IsKeyPressed(int key);
    static bool IsKeyDown(int key);
    static bool IsKeyReleased(int key);
    static bool IsMouseButtonDown(int button);
    static bool IsMouseButtonPressed(int button);
    static bool IsMouseButtonReleased(int button);
    static Vector2 GetMousePosition();
    static Vector2 GetTouchPosition(int index);

    // ============================================================================
    // AUDIO FUNCTIONS
    // ============================================================================
    
    static Sound LoadSound(const char* fileName);
    static void PlaySound(Sound sound);
    static void StopSound(Sound sound);
    static void PauseSound(Sound sound);
    static void ResumeSound(Sound sound);
    static void SetSoundVolume(Sound sound, float volume);
    static bool IsSoundPlaying(Sound sound);
    static void UnloadSound(Sound sound);
    static Music LoadMusicStream(const char* fileName);
    static Music LoadMusic(const char* fileName);
    static void PlayMusicStream(Music music);
    static void PlayMusic(Music music);
    static void StopMusicStream(Music music);
    static void StopMusic(Music music);
    static void PauseMusicStream(Music music);
    static void PauseMusic(Music music);
    static void ResumeMusicStream(Music music);
    static void ResumeMusic(Music music);
    static void UpdateMusicStream(Music music);
    static void UpdateMusic(Music music);
    static void SetMusicVolume(Music music, float volume);
    static bool IsMusicStreamPlaying(Music music);
    static bool IsMusicPlaying(Music music);
    static void SetMusicLooping(Music music, bool looping);
    static void UnloadMusicStream(Music music);
    static void UnloadMusic(Music music);
    static float GetMusicDuration(Music music);

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    static double GetTime();
    static void TraceLog(int logLevel, const char* text, ...);
    static int GetRandomValue(int min, int max);
    static float GetRandomFloat(float min, float max);

    // ============================================================================
    // TEXTURE FUNCTIONS (Declarations only - implemented in .mm file)
    // ============================================================================
    
    static Texture2D LoadTexture(const char* fileName);
    static void UnloadTexture(Texture2D texture);
    static Image LoadImage(const char* fileName);
    static void UnloadImage(Image image);
    static void SetTextureWrap(Texture2D texture, int wrap);
    static void* CreateTextureFromImage(void* image, int* width, int* height);
    static Texture2D LoadTextureFromImage(Image image);
    static Image LoadImageFromTexture(Texture2D texture);
    static Rectangle GetTextureRec(Texture2D texture);

    // ============================================================================
    // FONT FUNCTIONS (Declarations only - implemented in .mm file)
    // ============================================================================
    
    static Font LoadFont(const char* fileName);
    static Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount);
    static void UnloadFont(Font font);
    static Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing);

    // ============================================================================
    // RENDERING FUNCTIONS (Declarations only - implemented in .mm file)
    // ============================================================================
    
    static void BeginScissorMode(int x, int y, int width, int height);
    static void EndScissorMode();
    static void DrawFPS(int posX, int posY);

    // ============================================================================
    // AUDIO DEVICE FUNCTIONS (Declarations only - implemented in .mm file)
    // ============================================================================
    
    static void InitAudioDevice();
    static void InitializeAudio();
    static void CloseAudioDevice();
    static void ShutdownAudio();
    static bool IsAudioDeviceReady();

    // ============================================================================
    // RENDER TEXTURE FUNCTIONS (Declarations only - implemented in .mm file)
    // ============================================================================
    
    static RenderTexture2D LoadRenderTexture(int width, int height);
    static void UnloadRenderTexture(RenderTexture2D target);
    static void BeginTextureMode(RenderTexture2D target);
    static void EndTextureMode();

    // ============================================================================
    // ADDITIONAL UTILITY FUNCTIONS
    // ============================================================================
    
    static void SetTraceLogLevel(int logLevel);
    static void SetConfigFlags(unsigned int flags);
    static void InitWindow(int width, int height, const char* title);
    static void SetWindowSize(int width, int height);
    static void ToggleFullscreen();
    static void CloseWindow();
    static void SetRandomSeed(unsigned int seed);
    static Vector2 GetRandomVector2(Vector2 min, Vector2 max);
    static Color GetRandomColor();
    static Color ColorAlphaBlend(Color dst, Color src, Color tint);

    // ============================================================================
    // ADDITIONAL INPUT FUNCTIONS
    // ============================================================================
    
    static Vector2 GetMouseDelta();
    static bool IsPrimaryInputPressed();
    static bool IsPrimaryInputDown();
    static bool IsPrimaryInputReleased();
    static Vector2 GetPrimaryInputPosition();

    // ============================================================================
    // WINDOW/SCREEN FUNCTIONS
    // ============================================================================
    
    static int GetScreenWidth();
    static int GetScreenHeight();
    static float GetScreenScale();
    static Vector2 GetScreenCenter();
    static Vector2 GetRenderScale();
    static Rectangle GetSafeArea();
    static float GetScreenDensity();
    static bool IsLandscape();
    static bool IsPortrait();

    static void SetPreferredOrientation(bool landscape);
    static bool ShouldUseLargerTouchTargets();
    static int GetRecommendedFontSize();

    static void SetTargetFPS(int fps);
    static void SetScreenSize(int width, int height);
    static void SetScreenScale(float scale);
    static void UpdateSafeAreaInsets(float top, float right, float bottom, float left);
    static bool WindowShouldClose();

    // ============================================================================
    // VECTOR MATH FUNCTIONS
    // ============================================================================
    
    static float Vector2Length(Vector2 v);
    static Vector2 Vector2Normalize(Vector2 v);
    static Vector2 Vector2Add(Vector2 v1, Vector2 v2);
    static Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);
static Vector2 Vector2Scale(Vector2 v, float scale);
static float Vector2Distance(Vector2 v1, Vector2 v2);

    // ============================================================================
    // COLLISION DETECTION FUNCTIONS
    // ============================================================================
    
    static bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
    static bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
    static bool CheckCollisionPointRec(Vector2 point, Rectangle rec);

    // ============================================================================
    // COLOR FUNCTIONS
    // ============================================================================
    
    static Color ColorAlpha(Color color, float alpha);
    static Color ColorLerp(Color color1, Color color2, float amount);
    static Color Fade(Color color, float alpha);  // Equivalent to raylib's Fade

    // ============================================================================
    // KEY CONSTANTS
    // ============================================================================
    
    // Key codes are defined in PlatformTypes.h
    
    // ============================================================================
    // GESTURE CONSTANTS
    // ============================================================================
    
    // Gesture types are defined in PlatformTypes.h

    // ============================================================================
    // MATH UTILITY FUNCTIONS (MUST MATCH IMPLEMENTATIONS IN .mm)
    // =========================================================================
    static float Clamp(float value, float min, float max);
    static float Lerp(float start, float end, float amount);
    static float Min(float a, float b);
    static float Max(float a, float b);
    static float Abs(float value);
    static float Sin(float angle);
    static float Cos(float angle);
    static float Atan2(float y, float x);
    static float Sqrt(float value);
    static float Pow(float base, float exponent);

    // ============================================================================
    // TIME AND PERFORMANCE FUNCTIONS
    // ============================================================================
    
    static float GetFrameTime();
    static int GetCurrentFPS();
    static float GetCurrentFrameTime();

    // ============================================================================
    // ADDITIONAL VECTOR MATH FUNCTIONS
    // ============================================================================
    


    // ============================================================================
    // RECTANGLE UTILITY FUNCTIONS
    // ============================================================================
    
    static Rectangle RectangleNew(float x, float y, float width, float height);
    static Rectangle RectangleFromVector2(Vector2 position, Vector2 size);



    // ============================================================================
    // PLATFORM-SPECIFIC UTILITY FUNCTIONS
    // ============================================================================
    
    static std::string GetResourcePath(const char* resourceName);
    static std::string GetSaveDataPath(const char* filename);
    static bool PreferLowPowerMode();
    static int GetRecommendedTextureSize();

    static bool IsMobilePlatform();

    // ============================================================================
    // INITIALIZATION AND LIFECYCLE FUNCTIONS
    // ============================================================================
    
    static void Initialize(void* nativeView);
    static void Initialize();
    static void Shutdown();

    static void BeginDrawing();
    static void EndDrawing();
    static void ClearBackground(Color color);

    // ============================================================================
    // COMPLEX AUDIO FUNCTIONS
    // ============================================================================
    
    static void StartCrossfade(float duration);
    static void UpdateCrossfade(float deltaTime);
    static void FadeOutMusic(float duration);
    static void FadeInMusic(float duration);
    static void UpdateFade(float deltaTime);

    // ============================================================================
    // COMPLEX IMAGE FUNCTIONS
    // ============================================================================
    
    static void ImageResize(Image* image, int newWidth, int newHeight);
    static void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
    static Image ImageCopy(Image image);
    static Image ImageFromImage(Image image, Rectangle rec);
    static void ImageFlipVertical(Image* image);
    static void ImageFlipHorizontal(Image* image);
    


    // ============================================================================
    // PLATFORM-SPECIFIC FUNCTIONS
    // ============================================================================
    
    static void SetOrientation(bool landscape);
    static void ShowVirtualKeyboard(bool show);
    static void Vibrate(int milliseconds);
    static void SetTextureFilter(Texture2D texture, int filter);
    static Image GenImageColor(int width, int height, Color color);
    static Font GetFontDefault();
};

// ============================================================================
// RAYLIB TRAITS
// ============================================================================

struct RaylibTraits {
    static constexpr bool IsIOS = false;

    // ============================================================================
    // RAYLIB FUNCTIONS - All available directly from raylib.h
    // ============================================================================
    
    // All raylib functions are available directly, no need to redeclare them
    // The traits just provide a consistent interface that matches IOSTraits
    
    // ============================================================================
    // COLOR FUNCTIONS
    // ============================================================================
    
    // ============================================================================
    // KEY CONSTANTS
    // ============================================================================
    
    // Key codes (from raylib)
    
    // ============================================================================
    // GESTURE CONSTANTS
    // ============================================================================
    
    // Gesture types (from raylib)
    
    // ============================================================================
    // ADDITIONAL FUNCTIONS NOT IN RAYLIB
    // ============================================================================
    
    // These functions are not in raylib and need to be implemented
    static Vector2 GetTouchPosition(int index);
    static Music LoadMusic(const char* fileName);
    static void PlayMusic(Music music);
    static void* CreateTextureFromImage(void* image, int* width, int* height);
    static void* CreateSolidColorImage(int width, int height, Color color);
    static Texture2D LoadTextureFromData(void* data, int width, int height, int format);
    static void SetPreferredOrientation(bool landscape);
    static bool ShouldUseLargerTouchTargets();
    static int GetRecommendedFontSize();
    static Vector2 GetScreenToWorld2D(Vector2 screen, Camera2D camera);
    static Vector2 GetWorldToScreen2D(Vector2 world, Camera2D camera);
    static Vector2 GetRenderScale();
    static Rectangle GetSafeArea();
    static float GetScreenDensity();
    static bool IsLandscape();
    static bool IsPortrait();
    static void StartCrossfade(float duration);
    static void UpdateCrossfade(float deltaTime);
    static void FadeOutMusic(float duration);
    static void FadeInMusic(float duration);
    static void UpdateFade(float deltaTime);
    static void SetOrientation(bool landscape);
    static void ShowVirtualKeyboard(bool show);
    static void Vibrate(int milliseconds);
    static std::string GetResourcePath(const char* resourceName);
    static std::string GetSaveDataPath(const char* filename);
    static bool PreferLowPowerMode();
    static int GetRecommendedTextureSize();
    static bool IsMobilePlatform();
    static void Initialize(void* nativeView);
    static void SetScreenScale(float scale);
    static void SetTextureFilter(Texture2D texture, int filter);
    static Image GenImageColor(int width, int height, Color color);
    static Font GetFontDefault();
};

// ============================================================================
// RAYLIB TRAITS INLINE IMPLEMENTATIONS
// ============================================================================

// Simple desktop implementation - just return the filename as-is since desktop doesn't have sandboxing
inline std::string RaylibTraits::GetSaveDataPath(const char* filename) {
    return std::string(filename);
}

// Simple desktop implementation - return filename as-is for now
inline std::string RaylibTraits::GetResourcePath(const char* resourceName) {
    return std::string(resourceName);
}

#ifdef PLATFORM_IOS
using CurrentTraits = IOSTraits;
#else
using CurrentTraits = RaylibTraits;
#endif



#endif // PLATFORM_TRAITS_H