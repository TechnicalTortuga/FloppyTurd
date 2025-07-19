#ifndef PLATFORM_API_H
#define PLATFORM_API_H

// Forward declarations
class Game;  // Forward declaration for Game class

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdarg>  // For variadic functions
#include <cstdio>   // For vsnprintf
#include <TargetConditionals.h>
#include "PlatformTypes.h"

// Platform detection macros
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    #ifndef PLATFORM_IOS
        #define PLATFORM_IOS
    #endif
#else
    #ifndef PLATFORM_DESKTOP
        #define PLATFORM_DESKTOP
    #endif
#endif

// Conditional includes based on platform
#ifdef PLATFORM_IOS
    // Include Core Graphics for CGSize and related types in C++
    #include <CoreGraphics/CoreGraphics.h>
    
    // Forward declare UIKit types for C++ compatibility
    #ifdef __OBJC__
        #include <UIKit/UIKit.h>
    #else
        // C++ forward declarations for UIKit types
        typedef struct UIEdgeInsets {
            double top, left, bottom, right;
        } UIEdgeInsets;
    #endif
    
    // NOTE: Swift-generated header will be included when C++ interop is enabled
    // The bridge provides thread-safe access to Swift functions from C++
    // Don't include during bridging header compilation to avoid circular dependencies
    #if !defined(FLOPPYTURD_BRIDGING_HEADER_H) && !defined(SWIFT_PACKAGE) && !defined(__SWIFT_CLANG_MODULE_BUILD__)
        #include "GameEngine-Swift.h"  // Generated Swift C++ interop header
    #endif
#else
    #include "raylib.h"
#endif

// ============================================================================
// PLATFORM API CLASS - Simple wrapper around bridge functions
// ============================================================================

class PlatformAPI {
public:
    static PlatformAPI& GetInstance() {
        static PlatformAPI instance;
        return instance;
    }
    
    // Constructor/Destructor
    PlatformAPI() = default;
    ~PlatformAPI() = default;
    
    // Prevent copying
    PlatformAPI(const PlatformAPI&) = delete;
    PlatformAPI& operator=(const PlatformAPI&) = delete;
    
    // ============================================================================
    // FUNCTION DECLARATIONS & IMPLEMENTATIONS
    // All functions with raylib-compatible signatures
    // Implementations are inlined and platform-specific
    // ============================================================================

#if defined(PLATFORM_IOS)
    // ============================================================================
    // iOS IMPLEMENTATIONS (via Swift C++ interop bridge)
    // ============================================================================

    // Platform Functions
    void Initialize(void* nativeView = nullptr) { 
        FloppyTurd::getCppInteropBridge().initializeEngine();
    }
    void Shutdown() { 
        FloppyTurd::getCppInteropBridge().shutdownEngine();
    }
    void InitializePlatform() { 
        FloppyTurd::getCppInteropBridge().initializePlatform();
    }
    void ShutdownPlatform() { 
        FloppyTurd::getCppInteropBridge().shutdownPlatform();
    }
    
    // Window and Screen Functions
    void InitWindow(int width, int height, const char* title) { 
        FloppyTurd::getCppInteropBridge().initWindow();
    }
    void CloseWindow() { 
        FloppyTurd::getCppInteropBridge().closeWindow();
    }
    bool WindowShouldClose() { 
        return FloppyTurd::getCppInteropBridge().windowShouldClose();
    }
    int GetScreenWidth() { 
        return static_cast<int>(FloppyTurd::getCppInteropBridge().getScreenWidth());
    }
    int GetScreenHeight() { 
        return static_cast<int>(FloppyTurd::getCppInteropBridge().getScreenHeight());
    }
    float GetScreenScale() { 
        return FloppyTurd::getCppInteropBridge().getScreenScale();
    }
    void SetTargetFPS(int fps) { 
        FloppyTurd::getCppInteropBridge().setTargetFPS(static_cast<int32_t>(fps));
    }
    int GetCurrentFPS() { 
        return static_cast<int>(FloppyTurd::getCppInteropBridge().getCurrentFPS());
    }
    float GetCurrentFrameTime() { 
        return FloppyTurd::getCppInteropBridge().getCurrentFrameTime();
    }
    void SetWindowSize(int width, int height) { 
        // iOS: Window size is managed by the system
        // This is a no-op for iOS but provided for compatibility
    }
    void ToggleFullscreen() { 
        // iOS: Fullscreen is managed by the system  
        // This is a no-op for iOS but provided for compatibility
    }
    bool IsWindowFullscreen() { 
        // iOS: Apps are always fullscreen
        return true;
    }
    Vector2 GetScreenCenter() { 
        return {(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};
    }
    Rectangle GetSafeArea() { 
        // iOS: Return the full screen as safe area for compatibility
        return Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
    }
    float GetScreenDensity() {
        // iOS: Get screen density from bridge
        return FloppyTurd::getCppInteropBridge().getScreenScale();
    }
    
    // Input Functions
    bool IsKeyPressed(int key) { return FloppyTurd::getCppInteropBridge().isKeyPressed(key); }
    bool IsKeyDown(int key) { return FloppyTurd::getCppInteropBridge().isKeyDown(key); }
    bool IsKeyReleased(int key) { return FloppyTurd::getCppInteropBridge().isKeyReleased(key); }
    bool IsMouseButtonPressed(int button) { return FloppyTurd::getCppInteropBridge().isMouseButtonPressed(button); }
    bool IsMouseButtonDown(int button) { return FloppyTurd::getCppInteropBridge().isMouseButtonDown(button); }
    bool IsMouseButtonReleased(int button) { return FloppyTurd::getCppInteropBridge().isMouseButtonReleased(button); }
    Vector2 GetMousePosition() { 
        float x = FloppyTurd::getCppInteropBridge().getMousePositionX();
        float y = FloppyTurd::getCppInteropBridge().getMousePositionY();
        return Vector2{x, y}; 
    }
    Vector2 GetMouseDelta() { 
        float x = FloppyTurd::getCppInteropBridge().getMouseDeltaX();
        float y = FloppyTurd::getCppInteropBridge().getMouseDeltaY();
        return Vector2{x, y}; 
    }
    Vector2 GetTouchPosition(int index) { 
        float x = FloppyTurd::getCppInteropBridge().getTouchPositionX(index);
        float y = FloppyTurd::getCppInteropBridge().getTouchPositionY(index);
        return Vector2{x, y}; 
    }
    bool IsPrimaryInputPressed() { return FloppyTurd::getCppInteropBridge().isPrimaryInputPressed(); }
    bool IsPrimaryInputDown() { return FloppyTurd::getCppInteropBridge().isPrimaryInputDown(); }
    bool IsPrimaryInputReleased() { return FloppyTurd::getCppInteropBridge().isPrimaryInputReleased(); }
    Vector2 GetPrimaryInputPosition() { 
        float x = FloppyTurd::getCppInteropBridge().getPrimaryInputPositionX();
        float y = FloppyTurd::getCppInteropBridge().getPrimaryInputPositionY();
        return Vector2{x, y}; 
    }
    
    // Rendering Functions
    void BeginDrawing() { FloppyTurd::getCppInteropBridge().beginDrawing(); }
    void EndDrawing() { FloppyTurd::getCppInteropBridge().endDrawing(); }
    void ClearBackground(Color color) { FloppyTurd::getCppInteropBridge().clearBackground(color.r, color.g, color.b, color.a); }
    void DrawRectangle(int posX, int posY, int width, int height, Color color) { FloppyTurd::getCppInteropBridge().drawRectangle(posX, posY, width, height, color.r, color.g, color.b, color.a); }
    void DrawRectangleRec(Rectangle rec, Color color) { FloppyTurd::getCppInteropBridge().drawRectangleRec(rec.x, rec.y, rec.width, rec.height, color.r, color.g, color.b, color.a); }
    void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) { FloppyTurd::getCppInteropBridge().drawRectangleLinesEx(rec.x, rec.y, rec.width, rec.height, lineThick, color.r, color.g, color.b, color.a); }
    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) { FloppyTurd::getCppInteropBridge().drawRectangleRounded(rec.x, rec.y, rec.width, rec.height, roundness, segments, color.r, color.g, color.b, color.a); }
    void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) { 
        // iOS: Fallback to regular rectangle lines since drawRectangleRoundedLinesEx is not available in bridge
        FloppyTurd::getCppInteropBridge().drawRectangleLinesEx(rec.x, rec.y, rec.width, rec.height, lineThick, color.r, color.g, color.b, color.a); 
    }
    void DrawCircle(int centerX, int centerY, float radius, Color color) { FloppyTurd::getCppInteropBridge().drawCircle(centerX, centerY, radius, color.r, color.g, color.b, color.a); }
    void DrawCircleV(Vector2 center, float radius, Color color) { FloppyTurd::getCppInteropBridge().drawCircleV(center.x, center.y, radius, color.r, color.g, color.b, color.a); }
    void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) { FloppyTurd::getCppInteropBridge().drawLine(startPosX, startPosY, endPosX, endPosY, color.r, color.g, color.b, color.a); }
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) { FloppyTurd::getCppInteropBridge().drawLineV(startPos.x, startPos.y, endPos.x, endPos.y, color.r, color.g, color.b, color.a); }
    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) { FloppyTurd::getCppInteropBridge().drawLineEx(startPos.x, startPos.y, endPos.x, endPos.y, thick, color.r, color.g, color.b, color.a); }
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color) { FloppyTurd::getCppInteropBridge().drawText(text, posX, posY, fontSize, color.r, color.g, color.b, color.a); }
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) { FloppyTurd::getCppInteropBridge().drawTextEx(font.id, text, position.x, position.y, fontSize, spacing, tint.r, tint.g, tint.b, tint.a); }
    void BeginScissorMode(int x, int y, int width, int height) { FloppyTurd::getCppInteropBridge().beginScissorMode(x, y, width, height); }
    void EndScissorMode() { FloppyTurd::getCppInteropBridge().endScissorMode(); }
    void DrawFPS(int posX, int posY) { FloppyTurd::getCppInteropBridge().drawFPS(posX, posY); }
    
    // Texture Functions
    Texture2D LoadTexture(const char* fileName) { return Texture2D{FloppyTurd::getCppInteropBridge().loadTexture(fileName)}; }
    void UnloadTexture(Texture2D texture) { FloppyTurd::getCppInteropBridge().unloadTexture(texture.id); }
    Image LoadImage(const char* fileName) { return Image{FloppyTurd::getCppInteropBridge().loadImage(fileName)}; }
    void UnloadImage(Image image) { FloppyTurd::getCppInteropBridge().unloadImage(image.data); }
    void SetTextureWrap(Texture2D texture, int wrap) { FloppyTurd::getCppInteropBridge().setTextureWrap(texture.id, wrap); }
    Texture2D LoadTextureFromImage(Image image) { return Texture2D{FloppyTurd::getCppInteropBridge().loadTextureFromImage(image.data)}; }
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint) { FloppyTurd::getCppInteropBridge().drawTexture(texture.id, posX, posY, tint.r, tint.g, tint.b, tint.a); }
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint) { FloppyTurd::getCppInteropBridge().drawTextureV(texture.id, position.x, position.y, tint.r, tint.g, tint.b, tint.a); }
    void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) { FloppyTurd::getCppInteropBridge().drawTextureRec(texture.id, source.x, source.y, source.width, source.height, position.x, position.y, tint.r, tint.g, tint.b, tint.a); }
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) { FloppyTurd::getCppInteropBridge().drawTexturePro(texture.id, source.x, source.y, source.width, source.height, dest.x, dest.y, dest.width, dest.height, origin.x, origin.y, rotation, tint.r, tint.g, tint.b, tint.a); }
    void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) { FloppyTurd::getCppInteropBridge().drawTextureEx(texture.id, position.x, position.y, rotation, scale, tint.r, tint.g, tint.b, tint.a); }
    void SetTextureFilter(Texture2D texture, int filter) { FloppyTurd::getCppInteropBridge().setTextureFilter(texture.id, filter); }
    
    // Font Functions
    Font LoadFont(const char* fileName) { return Font{FloppyTurd::getCppInteropBridge().loadFont(fileName)}; }
    Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount) { return Font{FloppyTurd::getCppInteropBridge().loadFontEx(fileName, fontSize, fontChars, glyphCount)}; }
    void UnloadFont(Font font) { FloppyTurd::getCppInteropBridge().unloadFont(font.id); }
    int MeasureText(const char* text, int fontSize) { return FloppyTurd::getCppInteropBridge().measureText(text, fontSize); }
    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) { 
        float width = FloppyTurd::getCppInteropBridge().measureTextExWidth(font.id, text, fontSize, spacing);
        float height = FloppyTurd::getCppInteropBridge().measureTextExHeight(font.id, text, fontSize, spacing);
        return Vector2{width, height}; 
    }
    Font GetFontDefault() { return Font{FloppyTurd::getCppInteropBridge().getFontDefault()}; }
    
    // Audio Functions
    void InitAudioDevice() { FloppyTurd::getCppInteropBridge().initAudioDevice(); }
    void CloseAudioDevice() { FloppyTurd::getCppInteropBridge().closeAudioDevice(); }
    bool IsAudioDeviceReady() { return FloppyTurd::getCppInteropBridge().isAudioDeviceReady(); }
    void SetMasterVolume(float volume) { FloppyTurd::getCppInteropBridge().setMasterVolume(volume); }
    Sound LoadSound(const char* fileName) { return Sound{FloppyTurd::getCppInteropBridge().loadSoundFile(fileName)}; }
    void PlaySound(Sound sound) { FloppyTurd::getCppInteropBridge().playSoundById(sound.id); }
    void StopSound(Sound sound) { FloppyTurd::getCppInteropBridge().stopSoundById(sound.id); }
    void PauseSound(Sound sound) { FloppyTurd::getCppInteropBridge().pauseSound(sound.id); }
    void ResumeSound(Sound sound) { FloppyTurd::getCppInteropBridge().resumeSound(sound.id); }
    void SetSoundVolume(Sound sound, float volume) { FloppyTurd::getCppInteropBridge().setSoundVolume(sound.id, volume); }
    void SetSoundPitch(Sound sound, float pitch) { FloppyTurd::getCppInteropBridge().setSoundPitch(sound.id, pitch); }
    void SetSoundPan(Sound sound, float pan) { FloppyTurd::getCppInteropBridge().setSoundPan(sound.id, pan); }
    bool IsSoundPlaying(Sound sound) { return FloppyTurd::getCppInteropBridge().isSoundPlaying(sound.id); }
    void UnloadSound(Sound sound) { FloppyTurd::getCppInteropBridge().unloadSound(sound.id); }
    Music LoadMusicStream(const char* fileName) { return Music{FloppyTurd::getCppInteropBridge().loadMusicStream(fileName)}; }
    Music LoadMusic(const char* fileName) { return Music{FloppyTurd::getCppInteropBridge().loadMusic(fileName)}; }
    void PlayMusicStream(Music music) { FloppyTurd::getCppInteropBridge().playMusicStream(music.id); }
    void PlayMusic(Music music) { FloppyTurd::getCppInteropBridge().playMusic(music.id); }
    void StopMusicStream(Music music) { FloppyTurd::getCppInteropBridge().stopMusicStream(music.id); }
    void StopMusic() { FloppyTurd::getCppInteropBridge().stopMusic(); }
    void PauseMusicStream(Music music) { FloppyTurd::getCppInteropBridge().pauseMusicStream(music.id); }
    void ResumeMusicStream(Music music) { FloppyTurd::getCppInteropBridge().resumeMusicStream(music.id); }
    void UpdateMusicStream(Music music) { FloppyTurd::getCppInteropBridge().updateMusicStream(music.id); }
    void SetMusicVolume(float volume) { FloppyTurd::getCppInteropBridge().setMusicVolume(volume); }
    void SetMusicVolumeForId(Music music, float volume) { FloppyTurd::getCppInteropBridge().setMusicVolumeForId(music.id, volume); }
    bool IsMusicStreamPlaying(Music music) { return FloppyTurd::getCppInteropBridge().isMusicStreamPlaying(music.id); }
    bool IsMusicPlaying() { return FloppyTurd::getCppInteropBridge().isMusicPlaying(); }
    void SetMusicLooping(Music music, bool looping) { FloppyTurd::getCppInteropBridge().setMusicLooping(music.id, looping); }
    float GetMusicTimeLength(Music music) { return FloppyTurd::getCppInteropBridge().getMusicTimeLength(music.id); }
    float GetMusicTimePlayed(Music music) { return FloppyTurd::getCppInteropBridge().getMusicTimePlayed(music.id); }
    float GetMusicDuration(Music music) { return FloppyTurd::getCppInteropBridge().getMusicTimeLength(music.id); } // Alias for compatibility
    void UnloadMusicStream(Music music) { FloppyTurd::getCppInteropBridge().unloadMusicStream(music.id); }
    void UnloadMusic(Music music) { FloppyTurd::getCppInteropBridge().unloadMusic(music.id); }
    
    // Time Functions
    double GetTime() { return FloppyTurd::getCppInteropBridge().getTime(); }
    float GetFrameTime() { return FloppyTurd::getCppInteropBridge().getFrameTime(); }
    
    // Math and Utility Functions
    int GetRandomValue(int min, int max) { return FloppyTurd::getCppInteropBridge().getRandomValue(min, max); }
    float GetRandomFloat(float min, float max) { return FloppyTurd::getCppInteropBridge().getRandomFloat(min, max); }
    void SetRandomSeed(unsigned int seed) { FloppyTurd::getCppInteropBridge().setRandomSeed(seed); }
    Vector2 GetRandomVector2(Vector2 min, Vector2 max) { 
        float x = FloppyTurd::getCppInteropBridge().getRandomVector2X(min.x, min.y, max.x, max.y);
        float y = FloppyTurd::getCppInteropBridge().getRandomVector2Y(min.x, min.y, max.x, max.y);
        return Vector2{x, y};
    }
    Color GetRandomColor() { 
        auto result = FloppyTurd::getCppInteropBridge().getRandomColor();
        return Color{result.getR(), result.getG(), result.getB(), result.getA()};
    }
    void TraceLog(int logLevel, const char* text, ...) { FloppyTurd::getCppInteropBridge().traceLog(logLevel, text); }
    void SetTraceLogLevel(int logLevel) { FloppyTurd::getCppInteropBridge().setTraceLogLevel(logLevel); }
    void SetConfigFlags(unsigned int flags) { FloppyTurd::getCppInteropBridge().setConfigFlags(flags); }
    
    // Vector Math Functions
    float Vector2Length(Vector2 v) { return FloppyTurd::getCppInteropBridge().vector2Length(v.x, v.y); }
    Vector2 Vector2Normalize(Vector2 v) { 
        float x = FloppyTurd::getCppInteropBridge().vector2NormalizeX(v.x, v.y);
        float y = FloppyTurd::getCppInteropBridge().vector2NormalizeY(v.x, v.y);
        return Vector2{x, y};
    }
    Vector2 Vector2Add(Vector2 v1, Vector2 v2) { 
        float x = FloppyTurd::getCppInteropBridge().vector2AddX(v1.x, v1.y, v2.x, v2.y);
        float y = FloppyTurd::getCppInteropBridge().vector2AddY(v1.x, v1.y, v2.x, v2.y);
        return Vector2{x, y};
    }
    Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) { 
        float x = FloppyTurd::getCppInteropBridge().vector2SubtractX(v1.x, v1.y, v2.x, v2.y);
        float y = FloppyTurd::getCppInteropBridge().vector2SubtractY(v1.x, v1.y, v2.x, v2.y);
        return Vector2{x, y};
    }
    Vector2 Vector2Scale(Vector2 v, float scale) { 
        float x = FloppyTurd::getCppInteropBridge().vector2ScaleX(v.x, v.y, scale);
        float y = FloppyTurd::getCppInteropBridge().vector2ScaleY(v.x, v.y, scale);
        return Vector2{x, y};
    }
    float Vector2Distance(Vector2 v1, Vector2 v2) { return FloppyTurd::getCppInteropBridge().vector2Distance(v1.x, v1.y, v2.x, v2.y); }
    
    // Collision Detection Functions
    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) { return FloppyTurd::getCppInteropBridge().checkCollisionRecs(rec1.x, rec1.y, rec1.width, rec1.height, rec2.x, rec2.y, rec2.width, rec2.height); }
    bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) { return FloppyTurd::getCppInteropBridge().checkCollisionCircleRec(center.x, center.y, radius, rec.x, rec.y, rec.width, rec.height); }
    bool CheckCollisionPointRec(Vector2 point, Rectangle rec) { return FloppyTurd::getCppInteropBridge().checkCollisionPointRec(point.x, point.y, rec.x, rec.y, rec.width, rec.height); }
    
    // Color Functions
    Color ColorAlpha(Color color, float alpha) { 
        auto result = FloppyTurd::getCppInteropBridge().colorAlpha(color.r, color.g, color.b, color.a, alpha);
        return Color{result.getR(), result.getG(), result.getB(), result.getA()};
    }
    Color Fade(Color color, float alpha) { 
        auto result = FloppyTurd::getCppInteropBridge().fade(color.r, color.g, color.b, color.a, alpha);
        return Color{result.getR(), result.getG(), result.getB(), result.getA()};
    }
    Color ColorLerp(Color color1, Color color2, float amount) { 
        auto result = FloppyTurd::getCppInteropBridge().colorLerp(color1.r, color1.g, color1.b, color1.a, color2.r, color2.g, color2.b, color2.a, amount);
        return Color{result.getR(), result.getG(), result.getB(), result.getA()};
    }
    
    // Math Utility Functions
    float Clamp(float value, float min, float max) { return FloppyTurd::getCppInteropBridge().clamp(value, min, max); }
    float Lerp(float start, float end, float amount) { return FloppyTurd::getCppInteropBridge().lerp(start, end, amount); }
    
    // Text Formatting Functions
    const char* TextFormat(const char* text, ...) {
        static char buffer[1024];
        va_list args;
        va_start(args, text);
        vsnprintf(buffer, sizeof(buffer), text, args);
        va_end(args);
        return buffer;
    }
    
    // Rectangle Utility Functions
    Rectangle RectangleNew(float x, float y, float width, float height) { 
        auto result = FloppyTurd::getCppInteropBridge().rectangleNew(x, y, width, height);
        return Rectangle{result.getX(), result.getY(), result.getWidth(), result.getHeight()};
    }
    
    // Platform-Specific Functions
    void SetOrientation(bool landscape) { FloppyTurd::getCppInteropBridge().setOrientation(landscape); }
    void ShowVirtualKeyboard(bool show) { FloppyTurd::getCppInteropBridge().showVirtualKeyboard(show); }
    void Vibrate(int milliseconds) { FloppyTurd::getCppInteropBridge().vibrate(milliseconds); }
    const char* GetResourcePath(const char* resourceName) { 
        auto result = FloppyTurd::getCppInteropBridge().getResourcePath(resourceName);
        static std::string converted = std::string(result);
        return converted.c_str();
    }
    const char* GetSaveDataPath(const char* filename) { 
        auto result = FloppyTurd::getCppInteropBridge().getSaveDataPath(filename);
        static std::string converted = std::string(result);
        return converted.c_str();
    }
    bool IsMobilePlatform() { return FloppyTurd::getCppInteropBridge().isMobilePlatform(); }
    bool PreferLowPowerMode() { return FloppyTurd::getCppInteropBridge().preferLowPowerMode(); }

    // Monitor Functions (iOS implementation - no multi-monitor support)
    int GetCurrentMonitor() { return 0; } // Always return 0 on iOS
    int GetMonitorWidth(int monitor) { return GetScreenWidth(); } // Return screen width
    int GetMonitorHeight(int monitor) { return GetScreenHeight(); } // Return screen height

    // Render Texture Functions
    void UnloadRenderTexture(RenderTexture2D target) { 
        // iOS: Render texture cleanup handled by Metal renderer
        // This is a no-op for iOS but provided for compatibility
    }
    int GetRecommendedTextureSize() { return FloppyTurd::getCppInteropBridge().getRecommendedTextureSize(); }

#else
    // ============================================================================
    // DESKTOP IMPLEMENTATIONS (via raylib)
    // ============================================================================

    // Platform Functions
    void Initialize(void* nativeView = nullptr) { ::InitWindow(800, 600, "Game"); }
    void Shutdown() { ::CloseWindow(); }
    void InitializePlatform() { /* No-op on desktop */ }
    void ShutdownPlatform() { /* No-op on desktop */ }

    // Window and Screen Functions
    void InitWindow(int width, int height, const char* title) { ::InitWindow(width, height, title); }
    void CloseWindow() { ::CloseWindow(); }
    bool WindowShouldClose() { return ::WindowShouldClose(); }
    int GetScreenWidth() { return ::GetScreenWidth(); }
    int GetScreenHeight() { return ::GetScreenHeight(); }
    float GetScreenScale() { return 1.0f; } // No equivalent in raylib, return default
    void SetTargetFPS(int fps) { ::SetTargetFPS(fps); }
    int GetCurrentFPS() { return ::GetFPS(); }
    float GetCurrentFrameTime() { return ::GetFrameTime(); }
    void SetWindowSize(int width, int height) { ::SetWindowSize(width, height); }
    void ToggleFullscreen() { ::ToggleFullscreen(); }
    bool IsWindowFullscreen() { return ::IsWindowFullscreen(); }
    Vector2 GetScreenCenter() { return {(float)::GetScreenWidth() / 2.0f, (float)::GetScreenHeight() / 2.0f}; }
    Vector2 GetRenderScale() { return {1.0f, 1.0f}; } // No equivalent
    Rectangle GetSafeArea() { return {0, 0, (float)::GetScreenWidth(), (float)::GetScreenHeight()}; } // No equivalent
    float GetScreenDensity() { return 1.0f; } // No equivalent
    bool IsLandscape() { return ::GetScreenWidth() > ::GetScreenHeight(); }
    bool IsPortrait() { return ::GetScreenWidth() <= ::GetScreenHeight(); }
    void SetPreferredOrientation(bool landscape) { /* No-op on desktop */ }
    bool ShouldUseLargerTouchTargets() { return false; }
    int GetRecommendedFontSize() { return 20; }
    void UpdateSafeAreaInsets(float top, float right, float bottom, float left) { /* No-op on desktop */ }

    // Monitor Functions
    int GetCurrentMonitor() { return ::GetCurrentMonitor(); }
    int GetMonitorWidth(int monitor) { return ::GetMonitorWidth(monitor); }
    int GetMonitorHeight(int monitor) { return ::GetMonitorHeight(monitor); }

    // Render Texture Functions
    void UnloadRenderTexture(RenderTexture2D target) { ::UnloadRenderTexture(target); }

    // Input Functions
    bool IsKeyPressed(int key) { return ::IsKeyPressed(key); }
    bool IsKeyDown(int key) { return ::IsKeyDown(key); }
    bool IsKeyReleased(int key) { return ::IsKeyReleased(key); }
    bool IsMouseButtonPressed(int button) { return ::IsMouseButtonPressed(button); }
    bool IsMouseButtonDown(int button) { return ::IsMouseButtonDown(button); }
    bool IsMouseButtonReleased(int button) { return ::IsMouseButtonReleased(button); }
    Vector2 GetMousePosition() { return ::GetMousePosition(); }
    Vector2 GetMouseDelta() { return ::GetMouseDelta(); }
    Vector2 GetTouchPosition(int index) { return ::GetMousePosition(); } // Fallback to mouse
    bool IsPrimaryInputPressed() { return ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }
    bool IsPrimaryInputDown() { return ::IsMouseButtonDown(MOUSE_BUTTON_LEFT); }
    bool IsPrimaryInputReleased() { return ::IsMouseButtonReleased(MOUSE_BUTTON_LEFT); }
    Vector2 GetPrimaryInputPosition() { return ::GetMousePosition(); }

    // Rendering Functions
    void BeginDrawing() { ::BeginDrawing(); }
    void EndDrawing() { ::EndDrawing(); }
    void ClearBackground(Color color) { ::ClearBackground(color); }
    void DrawRectangle(int posX, int posY, int width, int height, Color color) { ::DrawRectangle(posX, posY, width, height, color); }
    void DrawRectangleRec(Rectangle rec, Color color) { ::DrawRectangleRec(rec, color); }
    void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) { ::DrawRectangleLinesEx(rec, lineThick, color); }
    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) { ::DrawRectangleRounded(rec, roundness, segments, color); }
    void DrawCircle(int centerX, int centerY, float radius, Color color) { ::DrawCircle(centerX, centerY, radius, color); }
    void DrawCircleV(Vector2 center, float radius, Color color) { ::DrawCircleV(center, radius, color); }
    void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) { ::DrawLine(startPosX, startPosY, endPosX, endPosY, color); }
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) { ::DrawLineV(startPos, endPos, color); }
    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) { ::DrawLineEx(startPos, endPos, thick, color); }
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color) { ::DrawText(text, posX, posY, fontSize, color); }
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) { ::DrawTextEx(font, text, position, fontSize, spacing, tint); }
    void BeginScissorMode(int x, int y, int width, int height) { ::BeginScissorMode(x, y, width, height); }
    void EndScissorMode() { ::EndScissorMode(); }
    void DrawFPS(int posX, int posY) { ::DrawFPS(posX, posY); }

    // ============================================================================
    // TEXTURE FUNCTIONS
    // ============================================================================
    
    Texture2D LoadTexture(const char* fileName) { return ::LoadTexture(fileName); }
    void UnloadTexture(Texture2D texture) { ::UnloadTexture(texture); }
    Image LoadImage(const char* fileName) { return ::LoadImage(fileName); }
    void UnloadImage(Image image) { ::UnloadImage(image); }
    void SetTextureWrap(Texture2D texture, int wrap) { ::SetTextureWrap(texture, wrap); }
    Texture2D LoadTextureFromImage(Image image) { return ::LoadTextureFromImage(image); }
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint) { ::DrawTexture(texture, posX, posY, tint); }
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint) { ::DrawTextureV(texture, position, tint); }
    void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) { ::DrawTextureRec(texture, source, position, tint); }
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) { ::DrawTexturePro(texture, source, dest, origin, rotation, tint); }
    void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) { ::DrawTextureEx(texture, position, rotation, scale, tint); }
    void SetTextureFilter(Texture2D texture, int filter) { ::SetTextureFilter(texture, filter); }
    
    // ============================================================================
    // FONT FUNCTIONS
    // ============================================================================
    
    Font LoadFont(const char* fileName) { return ::LoadFont(fileName); }
    Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount) { return ::LoadFontEx(fileName, fontSize, fontChars, glyphCount); }
    void UnloadFont(Font font) { ::UnloadFont(font); }
    int MeasureText(const char* text, int fontSize) { return ::MeasureText(text, fontSize); }
    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) { return ::MeasureTextEx(font, text, fontSize, spacing); }
    Font GetFontDefault() { return ::GetFontDefault(); }
    
    // ============================================================================
    // AUDIO FUNCTIONS
    // ============================================================================
    
    void InitAudioDevice() { ::InitAudioDevice(); }
    void CloseAudioDevice() { ::CloseAudioDevice(); }
    bool IsAudioDeviceReady() { return ::IsAudioDeviceReady(); }
    void SetMasterVolume(float volume) { ::SetMasterVolume(volume); }
    Sound LoadSound(const char* fileName) { return ::LoadSound(fileName); }
    void PlaySound(Sound sound) { ::PlaySound(sound); }
    void StopSound(Sound sound) { ::StopSound(sound); }
    void PauseSound(Sound sound) { ::PauseSound(sound); }
    void ResumeSound(Sound sound) { ::ResumeSound(sound); }
    void SetSoundVolume(Sound sound, float volume) { ::SetSoundVolume(sound, volume); }
    void SetSoundPitch(Sound sound, float pitch) { ::SetSoundPitch(sound, pitch); }
    void SetSoundPan(Sound sound, float pan) { ::SetSoundPan(sound, pan); }
    bool IsSoundPlaying(Sound sound) { return ::IsSoundPlaying(sound); }
    void UnloadSound(Sound sound) { ::UnloadSound(sound); }
    Music LoadMusicStream(const char* fileName) { return ::LoadMusicStream(fileName); }
    Music LoadMusic(const char* fileName) { return ::LoadMusic(fileName); }
    void PlayMusicStream(Music music) { ::PlayMusicStream(music); }
    void PlayMusic(Music music) { ::PlayMusic(music); }
    void StopMusicStream(Music music) { ::StopMusicStream(music); }
    void StopMusic() { ::StopMusic(); }
    void PauseMusicStream(Music music) { ::PauseMusicStream(music); }
    void ResumeMusicStream(Music music) { ::ResumeMusicStream(music); }
    void UpdateMusicStream(Music music) { ::UpdateMusicStream(music); }
    void SetMusicVolume(float volume) { ::SetMusicVolume(volume); }
    void SetMusicVolumeForId(Music music, float volume) { ::SetMusicVolumeForId(music, volume); }
    bool IsMusicStreamPlaying(Music music) { return ::IsMusicStreamPlaying(music); }
    bool IsMusicPlaying() { return ::IsMusicPlaying(); }
    void SetMusicLooping(Music music, bool looping) { ::SetMusicLooping(music, looping); }
    float GetMusicTimeLength(Music music) { return ::GetMusicTimeLength(music); }
    float GetMusicTimePlayed(Music music) { return ::GetMusicTimePlayed(music); }
    float GetMusicDuration(Music music) { return ::GetMusicTimeLength(music); } // Alias for compatibility
    void UnloadMusicStream(Music music) { ::UnloadMusicStream(music); }
    void UnloadMusic(Music music) { ::UnloadMusic(music); }
    
    // ============================================================================
    // TIME FUNCTIONS
    // ============================================================================
    
    double GetTime() { return ::GetTime(); }
    float GetFrameTime() { return ::GetFrameTime(); }
    
    // ============================================================================
    // MATH AND UTILITY FUNCTIONS
    // ============================================================================
    
    int GetRandomValue(int min, int max) { return ::GetRandomValue(min, max); }
    float GetRandomFloat(float min, float max) { return (float)GetRandomValue(min * 1000, max * 1000) / 1000.0f; }
    void SetRandomSeed(unsigned int seed) { ::SetRandomSeed(seed); }
    Vector2 GetRandomVector2(Vector2 min, Vector2 max) { return {(float)GetRandomValue((int)min.x, (int)max.x), (float)GetRandomValue((int)min.y, (int)max.y)}; }
    Color GetRandomColor() { return {(unsigned char)::GetRandomValue(0, 255), (unsigned char)::GetRandomValue(0, 255), (unsigned char)::GetRandomValue(0, 255), 255}; }
    void TraceLog(int logLevel, const char* text, ...) { ::TraceLog(logLevel, text); }
    void SetTraceLogLevel(int logLevel) { ::SetTraceLogLevel(logLevel); }
    void SetConfigFlags(unsigned int flags) { ::SetConfigFlags(flags); }

    // ============================================================================
    // VECTOR MATH FUNCTIONS  
    // ============================================================================
    
    float Vector2Length(Vector2 v) { return ::Vector2Length(v); }
    Vector2 Vector2Normalize(Vector2 v) { return ::Vector2Normalize(v); }
    Vector2 Vector2Add(Vector2 v1, Vector2 v2) { return ::Vector2Add(v1, v2); }
    Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) { return ::Vector2Subtract(v1, v2); }
    Vector2 Vector2Scale(Vector2 v, float scale) { return ::Vector2Scale(v, scale); }
    float Vector2Distance(Vector2 v1, Vector2 v2) { return ::Vector2Distance(v1, v2); }

    // ============================================================================
    // COLLISION DETECTION FUNCTIONS
    // ============================================================================
    
    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) { return ::CheckCollisionRecs(rec1, rec2); }
    bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) { return ::CheckCollisionCircleRec(center, radius, rec); }
    bool CheckCollisionPointRec(Vector2 point, Rectangle rec) { return ::CheckCollisionPointRec(point, rec); }

    // ============================================================================
    // COLOR FUNCTIONS
    // ============================================================================
    
    Color ColorAlpha(Color color, float alpha) { return ::ColorAlpha(color, alpha); }
    Color Fade(Color color, float alpha) { return ::Fade(color, alpha); }
    Color ColorLerp(Color color1, Color color2, float amount) { return ::ColorLerp(color1, color2, amount); }

    // ============================================================================
    // MATH UTILITY FUNCTIONS
    // ============================================================================
    
    float Clamp(float value, float min, float max) { return ::Clamp(value, min, max); }
    float Lerp(float start, float end, float amount) { return ::Lerp(start, end, amount); }
    
    // ============================================================================
    // TEXT FORMATTING FUNCTIONS
    // ============================================================================
    
    const char* TextFormat(const char* text, ...) { 
        static char buffer[1024];
        va_list args;
        va_start(args, text);
        vsnprintf(buffer, sizeof(buffer), text, args);
        va_end(args);
        return buffer;
    }
    float Lerp(float start, float end, float amount) { return ::Lerp(start, end, amount); }

    // ============================================================================
    // RECTANGLE UTILITY FUNCTIONS
    // ============================================================================
    
    Rectangle RectangleNew(float x, float y, float width, float height) { return {x, y, width, height}; }

    // Platform-Specific Functions
    void SetOrientation(bool landscape) { /* No-op on desktop */ }
    void ShowVirtualKeyboard(bool show) { /* No-op on desktop */ }
    void Vibrate(int milliseconds) { /* No-op on desktop */ }
    const char* GetResourcePath(const char* resourceName) { return resourceName; }
    const char* GetSaveDataPath(const char* filename) { return filename; }
    bool IsMobilePlatform() { return false; }
    bool PreferLowPowerMode() { return false; }
    int GetRecommendedTextureSize() { return 2048; }

#endif
};

// ============================================================================
// GLOBAL TYPEDEF FOR EASY ACCESS
// ============================================================================

using CurrentPlatformAPI = PlatformAPI;

// ============================================================================
// GLOBAL STANDALONE FUNCTIONS (for compatibility)
// ============================================================================

// These are declared as extern C functions and implemented by the bridge
// No need to redeclare them here since they're already declared above

// ============================================================================
// GLOBAL STANDALONE FUNCTIONS (for compatibility)
// ============================================================================

// Math and Utility Functions
inline int GetRandomValue(int min, int max) { return PlatformAPI::GetInstance().GetRandomValue(min, max); }
inline float GetRandomFloat(float min, float max) { return PlatformAPI::GetInstance().GetRandomFloat(min, max); }
inline void SetRandomSeed(unsigned int seed) { PlatformAPI::GetInstance().SetRandomSeed(seed); }
inline Vector2 GetRandomVector2(Vector2 min, Vector2 max) { return PlatformAPI::GetInstance().GetRandomVector2(min, max); }
inline Color GetRandomColor() { return PlatformAPI::GetInstance().GetRandomColor(); }
inline void TraceLog(int logLevel, const char* text, ...) { 
    va_list args;
    va_start(args, text);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    PlatformAPI::GetInstance().TraceLog(logLevel, buffer);
}

// Texture Functions  
inline Texture2D LoadTexture(const char* fileName) { return PlatformAPI::GetInstance().LoadTexture(fileName); }
inline void UnloadTexture(Texture2D texture) { PlatformAPI::GetInstance().UnloadTexture(texture); }
inline Image LoadImage(const char* fileName) { return PlatformAPI::GetInstance().LoadImage(fileName); }
inline void UnloadImage(Image image) { PlatformAPI::GetInstance().UnloadImage(image); }
inline void SetTextureWrap(Texture2D texture, int wrap) { PlatformAPI::GetInstance().SetTextureWrap(texture, wrap); }
inline void SetTextureFilter(Texture2D texture, int filter) { PlatformAPI::GetInstance().SetTextureFilter(texture, filter); }

// Audio Functions
inline Sound LoadSound(const char* fileName) { return PlatformAPI::GetInstance().LoadSound(fileName); }
inline void PlaySound(Sound sound) { PlatformAPI::GetInstance().PlaySound(sound); }
inline void StopSound(Sound sound) { PlatformAPI::GetInstance().StopSound(sound); }
inline void UnloadSound(Sound sound) { PlatformAPI::GetInstance().UnloadSound(sound); }
inline void SetSoundVolume(Sound sound, float volume) { PlatformAPI::GetInstance().SetSoundVolume(sound, volume); }
inline Music LoadMusic(const char* fileName) { return PlatformAPI::GetInstance().LoadMusic(fileName); }
inline void PlayMusic(Music music) { PlatformAPI::GetInstance().PlayMusic(music); }
inline void StopMusic() { PlatformAPI::GetInstance().StopMusic(); }
inline void StopMusic(Music music) { PlatformAPI::GetInstance().StopMusicStream(music); }
inline void UnloadMusic(Music music) { PlatformAPI::GetInstance().UnloadMusic(music); }
inline void PauseMusic(Music music) { PlatformAPI::GetInstance().PauseMusicStream(music); }
inline void ResumeMusic(Music music) { PlatformAPI::GetInstance().ResumeMusicStream(music); }
inline void UpdateMusic(Music music) { PlatformAPI::GetInstance().UpdateMusicStream(music); }
inline bool IsMusicPlaying() { return PlatformAPI::GetInstance().IsMusicPlaying(); }
inline bool IsMusicPlaying(Music music) { return PlatformAPI::GetInstance().IsMusicStreamPlaying(music); }
inline void SetMusicVolume(float volume) { PlatformAPI::GetInstance().SetMusicVolume(volume); }
inline void SetMusicVolume(Music music, float volume) { PlatformAPI::GetInstance().SetMusicVolumeForId(music, volume); }
inline void SetMusicLooping(Music music, bool looping) { PlatformAPI::GetInstance().SetMusicLooping(music, looping); }

// Math and Utility Functions  
inline float Clamp(float value, float min, float max) { return PlatformAPI::GetInstance().Clamp(value, min, max); }
inline float Lerp(float start, float end, float amount) { return PlatformAPI::GetInstance().Lerp(start, end, amount); }
inline Image GenImageColor(int width, int height, Color color) { 
    // For now, return an invalid image - this function needs proper implementation in PlatformAPI
    return Image{0}; 
}
inline Texture2D LoadTextureFromImage(Image image) { return PlatformAPI::GetInstance().LoadTextureFromImage(image); }

// Additional functions
inline Color ColorLerp(Color color1, Color color2, float amount) { return PlatformAPI::GetInstance().ColorLerp(color1, color2, amount); }
inline Color ColorAlpha(Color color, float alpha) { return PlatformAPI::GetInstance().ColorAlpha(color, alpha); }
inline Color Fade(Color color, float alpha) { return PlatformAPI::GetInstance().Fade(color, alpha); }
inline void BeginScissorMode(int x, int y, int width, int height) { PlatformAPI::GetInstance().BeginScissorMode(x, y, width, height); }
inline void EndScissorMode() { PlatformAPI::GetInstance().EndScissorMode(); }
inline Vector2 GetMouseDelta() { return PlatformAPI::GetInstance().GetMouseDelta(); }
inline bool IsMobilePlatform() { return PlatformAPI::GetInstance().IsMobilePlatform(); }

// Constants that are missing
#ifndef MOUSE_LEFT_BUTTON
#define MOUSE_LEFT_BUTTON 0
#endif

// Font Functions
inline Font LoadFont(const char* fileName) { return PlatformAPI::GetInstance().LoadFont(fileName); }
inline void UnloadFont(Font font) { PlatformAPI::GetInstance().UnloadFont(font); }
inline int MeasureText(const char* text, int fontSize) { return PlatformAPI::GetInstance().MeasureText(text, fontSize); }
inline Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) { return PlatformAPI::GetInstance().MeasureTextEx(font, text, fontSize, spacing); }
inline const char* TextFormat(const char* text, ...) { 
    static char buffer[1024];
    va_list args;
    va_start(args, text);
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    return buffer;
}

// Rendering Functions
inline void BeginDrawing() { PlatformAPI::GetInstance().BeginDrawing(); }
inline void EndDrawing() { PlatformAPI::GetInstance().EndDrawing(); }
inline void ClearBackground(Color color) { PlatformAPI::GetInstance().ClearBackground(color); }
inline void DrawRectangle(int posX, int posY, int width, int height, Color color) { PlatformAPI::GetInstance().DrawRectangle(posX, posY, width, height, color); }
inline void DrawRectangleRec(Rectangle rec, Color color) { PlatformAPI::GetInstance().DrawRectangleRec(rec, color); }
inline void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) { PlatformAPI::GetInstance().DrawRectangleRounded(rec, roundness, segments, color); }
inline void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) { PlatformAPI::GetInstance().DrawRectangleLinesEx(rec, lineThick, color); }
inline void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) { PlatformAPI::GetInstance().DrawLine(startPosX, startPosY, endPosX, endPosY, color); }
inline void DrawTexture(Texture2D texture, int posX, int posY, Color tint) { PlatformAPI::GetInstance().DrawTexture(texture, posX, posY, tint); }
inline void DrawTextureV(Texture2D texture, Vector2 position, Color tint) { PlatformAPI::GetInstance().DrawTextureV(texture, position, tint); }
inline void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) { PlatformAPI::GetInstance().DrawTextureEx(texture, position, rotation, scale, tint); }
inline void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) { PlatformAPI::GetInstance().DrawTextureRec(texture, source, position, tint); }
inline void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) { PlatformAPI::GetInstance().DrawTexturePro(texture, source, dest, origin, rotation, tint); }
inline void DrawText(const char* text, int posX, int posY, int fontSize, Color color) { PlatformAPI::GetInstance().DrawText(text, posX, posY, fontSize, color); }
inline void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) { PlatformAPI::GetInstance().DrawTextEx(font, text, position, fontSize, spacing, tint); }
inline void DrawCircle(int centerX, int centerY, float radius, Color color) { PlatformAPI::GetInstance().DrawCircle(centerX, centerY, radius, color); }
inline void DrawCircleV(Vector2 center, float radius, Color color) { PlatformAPI::GetInstance().DrawCircleV(center, radius, color); }

// Window and Screen Functions
inline void InitWindow(int width, int height, const char* title) { PlatformAPI::GetInstance().InitWindow(width, height, title); }
inline void CloseWindow() { PlatformAPI::GetInstance().CloseWindow(); }
inline bool WindowShouldClose() { return PlatformAPI::GetInstance().WindowShouldClose(); }
inline int GetScreenWidth() { return PlatformAPI::GetInstance().GetScreenWidth(); }
inline int GetScreenHeight() { return PlatformAPI::GetInstance().GetScreenHeight(); }
inline void SetTargetFPS(int fps) { PlatformAPI::GetInstance().SetTargetFPS(fps); }
inline void SetWindowSize(int width, int height) { PlatformAPI::GetInstance().SetWindowSize(width, height); }
inline void ToggleFullscreen() { PlatformAPI::GetInstance().ToggleFullscreen(); }

// Input Functions
inline bool IsKeyPressed(int key) { return PlatformAPI::GetInstance().IsKeyPressed(key); }
inline bool IsKeyDown(int key) { return PlatformAPI::GetInstance().IsKeyDown(key); }
inline bool IsMouseButtonPressed(int button) { return PlatformAPI::GetInstance().IsMouseButtonPressed(button); }
inline bool IsMouseButtonDown(int button) { return PlatformAPI::GetInstance().IsMouseButtonDown(button); }
inline bool IsMouseButtonReleased(int button) { return PlatformAPI::GetInstance().IsMouseButtonReleased(button); }
inline bool IsPrimaryInputPressed() { return PlatformAPI::GetInstance().IsPrimaryInputPressed(); }
inline bool IsPrimaryInputReleased() { return PlatformAPI::GetInstance().IsPrimaryInputReleased(); }
inline Vector2 GetMousePosition() { return PlatformAPI::GetInstance().GetMousePosition(); }
inline Vector2 GetTouchPosition(int index) { return PlatformAPI::GetInstance().GetTouchPosition(index); }

// Time Functions
inline double GetTime() { return PlatformAPI::GetInstance().GetTime(); }
inline float GetFrameTime() { return PlatformAPI::GetInstance().GetFrameTime(); }
inline int GetCurrentFPS() { return PlatformAPI::GetInstance().GetCurrentFPS(); }
inline float GetCurrentFrameTime() { return PlatformAPI::GetInstance().GetCurrentFrameTime(); }

// Vector Math Functions
inline float Vector2Length(Vector2 v) { return PlatformAPI::GetInstance().Vector2Length(v); }
inline Vector2 Vector2Normalize(Vector2 v) { return PlatformAPI::GetInstance().Vector2Normalize(v); }
inline Vector2 Vector2Add(Vector2 v1, Vector2 v2) { return PlatformAPI::GetInstance().Vector2Add(v1, v2); }
inline Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) { return PlatformAPI::GetInstance().Vector2Subtract(v1, v2); }
inline Vector2 Vector2Scale(Vector2 v, float scale) { return PlatformAPI::GetInstance().Vector2Scale(v, scale); }
inline float Vector2Distance(Vector2 v1, Vector2 v2) { return PlatformAPI::GetInstance().Vector2Distance(v1, v2); }

// Collision Detection Functions
inline bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) { return PlatformAPI::GetInstance().CheckCollisionRecs(rec1, rec2); }
inline bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) { return PlatformAPI::GetInstance().CheckCollisionCircleRec(center, radius, rec); }
inline bool CheckCollisionPointRec(Vector2 point, Rectangle rec) { return PlatformAPI::GetInstance().CheckCollisionPointRec(point, rec); }

// File and Storage Functions
inline const char* GetSaveDataPath(const char* filename) { return PlatformAPI::GetInstance().GetSaveDataPath(filename); }
inline const char* GetResourcePath(const char* resourceName) { return PlatformAPI::GetInstance().GetResourcePath(resourceName); }

// Audio Functions
inline void InitAudioDevice() { PlatformAPI::GetInstance().InitAudioDevice(); }
inline void CloseAudioDevice() { PlatformAPI::GetInstance().CloseAudioDevice(); }
inline float GetMusicDuration(Music music) { return PlatformAPI::GetInstance().GetMusicDuration(music); }
inline void StopMusicStream(Music music) { PlatformAPI::GetInstance().StopMusicStream(music); }
inline void PauseMusicStream(Music music) { PlatformAPI::GetInstance().PauseMusicStream(music); }
inline void ResumeMusicStream(Music music) { PlatformAPI::GetInstance().ResumeMusicStream(music); }
inline void SetMusicVolumeForId(Music music, float volume) { PlatformAPI::GetInstance().SetMusicVolumeForId(music, volume); }
inline bool IsMusicStreamPlaying(Music music) { return PlatformAPI::GetInstance().IsMusicStreamPlaying(music); }

// Font Functions
inline Font GetFontDefault() { return PlatformAPI::GetInstance().GetFontDefault(); }

// Screen and Monitor Functions
inline float GetScreenScale() { return PlatformAPI::GetInstance().GetScreenScale(); }
inline bool IsWindowFullscreen() { return PlatformAPI::GetInstance().IsWindowFullscreen(); }
inline Vector2 GetScreenCenter() { return PlatformAPI::GetInstance().GetScreenCenter(); }
inline Rectangle GetSafeArea() { return PlatformAPI::GetInstance().GetSafeArea(); }
inline int GetCurrentMonitor() { return PlatformAPI::GetInstance().GetCurrentMonitor(); }
inline int GetMonitorWidth(int monitor) { return PlatformAPI::GetInstance().GetMonitorWidth(monitor); }
inline int GetMonitorHeight(int monitor) { return PlatformAPI::GetInstance().GetMonitorHeight(monitor); }
inline float GetScreenDensity() { return PlatformAPI::GetInstance().GetScreenDensity(); }

// Input Functions
inline bool IsPrimaryInputDown() { return PlatformAPI::GetInstance().IsPrimaryInputDown(); }
inline Vector2 GetPrimaryInputPosition() { return PlatformAPI::GetInstance().GetPrimaryInputPosition(); }

// Platform-Specific Functions
inline void SetOrientation(bool landscape) { PlatformAPI::GetInstance().SetOrientation(landscape); }

// Render Texture Functions
inline void UnloadRenderTexture(RenderTexture2D target) { PlatformAPI::GetInstance().UnloadRenderTexture(target); }

// Drawing Functions
inline void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) { PlatformAPI::GetInstance().DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color); }

// ============================================================================
// GLOBAL GAME INSTANCE MANAGEMENT (for iOS integration)
// ============================================================================

extern "C" {
    // Get the current game instance (returns nullptr if not set)
    Game* GetGameInstance();
    
    // Set the current game instance (call from Game constructor)
    void SetGameInstance(Game* game);
    
    // App lifecycle functions
    void OnAppPause();
    void OnAppResume();
    
    // Global game view management for iOS
    void SetGlobalGameView(void* gameView);
    void* GetGlobalGameView();
    
    // Game main function
    int game_main(int argc, char* argv[]);
}

#endif // PLATFORM_API_H
