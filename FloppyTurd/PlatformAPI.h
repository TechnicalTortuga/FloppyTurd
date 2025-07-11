#ifndef PLATFORM_API_H
#define PLATFORM_API_H

#pragma once

#include "RaylibCompat.h"
#include <string>
#include <vector>

// Forward declarations for platform-specific implementations
class PlatformSpecific;

// Platform detection macros
#if defined(__ANDROID__)
    #define PLATFORM_MOBILE
    #define PLATFORM_ANDROID
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define PLATFORM_IOS
        #define PLATFORM_MOBILE
    #endif
#endif

/**
 * PlatformAPI - Unified Raylib-compatible interface for all platforms
 * 
 * This class provides a single entry point for all Raylib function calls,
 * delegating to platform-specific implementations (iOS/Metal, Desktop/Raylib, etc.)
 * 
 * Usage: PlatformAPI::DrawRectangle(x, y, width, height, color);
 */
class PlatformAPI {
public:
    // Singleton access
    static PlatformAPI& GetInstance();
    
    // Deleted copy constructor and assignment operator for singleton
    PlatformAPI(const PlatformAPI&) = delete;
    PlatformAPI& operator=(const PlatformAPI&) = delete;

    // ============================================================================
    // INITIALIZATION AND LIFECYCLE
    // ============================================================================
    
    /**
     * Initialize the platform API with native view
     * @param nativeView Platform-specific view pointer (UIView on iOS, etc.)
     */
    void Initialize(void* nativeView);
    
    /**
     * Initialize the platform API without native view (for desktop)
     */
    void Initialize();
    
    /**
     * Shutdown the platform API and cleanup resources
     */
    void Shutdown();

    // ============================================================================
    // RENDERING FUNCTIONS (Raylib-compatible)
    // ============================================================================
    
    // Drawing primitives
    static void DrawRectangle(int x, int y, int width, int height, Color color);
    static void DrawRectangleRec(Rectangle rec, Color color);
    static void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
    static void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    static void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    
    static void DrawCircle(float centerX, float centerY, float radius, Color color);
    static void DrawCircleV(Vector2 center, float radius, Color color);
    
    static void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color);
    static void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
    static void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
    
    // Texture drawing
    static void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
    static void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
    static void DrawTextureRec(Texture2D texture, Rectangle source, Rectangle dest, Color tint);
    static void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
    
    // Text rendering
    static void DrawText(const char* text, int posX, int posY, int fontSize, Color color);
    static void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint);
    
    // Render texture operations
    static void BeginDrawing(void* renderTexture);
    static void EndDrawing(void* renderTexture);
    static void* LoadRenderTexture(int width, int height);
    static void UnloadRenderTexture(void* renderTexture);

    // ============================================================================
    // TEXTURE AND IMAGE FUNCTIONS
    // ============================================================================
    
    static Texture2D LoadTexture(const char* fileName);
    static void UnloadTexture(Texture2D texture);
    static void* LoadTextureFromImage(void* imageData, int width, int height, int format);
    static void* CreateTextureFromImage(void* image, int* width, int* height);
    
    static Image LoadImage(const char* fileName);
    static void UnloadImage(Image image);
    static void* CreateSolidColorImage(int width, int height, Color color);

    // ============================================================================
    // AUDIO FUNCTIONS
    // ============================================================================
    
    static void InitializeAudio();
    static void ShutdownAudio();
    static void* LoadSound(const char* fileName);
    static void UnloadSound(void* sound);
    static void PlaySound(void* sound);
    static void SetSoundVolume(void* sound, float volume);
    
    static void* LoadMusic(const char* fileName);
    static void UnloadMusic(void* music);
    static void PlayMusic(void* music);
    static void StopMusic(void* music);
    static void UpdateMusic(void* music);
    static bool IsMusicPlaying(void* music);
    static void SetMusicVolume(void* music, float volume);
    static void PauseMusic(void* music);
    static void ResumeMusic(void* music);
    static void SetMusicLooping(void* music, bool looping);
    
    // Advanced audio functions
    static void* PreloadNextTrack(const char* fileName);
    static void SwitchToNextTrack();
    static void ClearNextTrack();
    static void StartCrossfade(float duration);
    static void UpdateCrossfade(float deltaTime);
    static void CompleteCrossfade();
    static void FadeOutMusic(float duration);
    static void FadeInMusic(float duration);
    static void UpdateFade(float deltaTime);

    // ============================================================================
    // FONT AND TEXT FUNCTIONS
    // ============================================================================
    
    static void* LoadFont(const char* fileName, int size);
    static void UnloadFont(void* font);
    static Vector2 MeasureText(const char* text, void* font, float fontSize, float spacing);
    static void DrawText(const char* text, float x, float y, float fontSize, Color color, void* font);

    // ============================================================================
    // INPUT FUNCTIONS
    // ============================================================================
    
    // Primary input (mouse on desktop, primary touch on mobile)
    static bool IsPrimaryInputDown();
    static bool IsPrimaryInputPressed();
    static bool IsPrimaryInputReleased();
    static Vector2 GetPrimaryInputPosition();
    
    // Secondary input
    static bool IsSecondaryInputDown();
    static bool IsSecondaryInputPressed();
    static bool IsSecondaryInputReleased();
    
    // Touch-specific (mobile)
    static bool IsTouchSupported();
    static int GetTouchCount();
    static Vector2 GetTouchPosition(int index);
    static std::vector<Vector2> GetTouchPoints();

    // ============================================================================
    // SCREEN AND WINDOW FUNCTIONS
    // ============================================================================
    
    static int GetScreenWidth();
    static int GetScreenHeight();
    static Vector2 GetScreenSize();
    static float GetScreenDensity();
    static float GetScreenScale();
    static Rectangle GetSafeArea();
    static bool IsWindowFullscreen();
    static void ToggleFullscreen();
    static void SetWindowTitle(const std::string& title);
    static void SetWindowSize(int width, int height);
    static bool SupportsFullscreen();

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    static double GetCurrentTime();
    static float GetLastFrameTime();
    static int GetLastFPS();
    static std::string GetResourcePath(const std::string& relativePath);
    static std::string GetSavePath(const std::string& filename);
    static std::string GetPlatformResourcePath(const std::string& relativePath);
    static bool PreferLowPowerMode();
    static int GetRecommendedTextureSize();
    static void SetOrientation(bool landscape);
    static void ShowVirtualKeyboard(bool show);
    static bool IsVirtualKeyboardShown();
    static void Vibrate(int milliseconds);

    // ============================================================================
    // APP LIFECYCLE
    // ============================================================================
    
    static void OnAppWillResignActive();
    static void OnAppDidBecomeActive();

    // ============================================================================
    // PLATFORM DETECTION
    // ============================================================================
    
    static bool IsMobilePlatform();
    static bool IsTouchSupported() const;

private:
    // Private constructor for singleton
    PlatformAPI();
    ~PlatformAPI();
    
    // Get the platform-specific implementation
    static PlatformSpecific* GetPlatformImpl();
    
    // Platform-specific implementation instance
    PlatformSpecific* m_platformImpl;
    
    // Singleton instance
    static PlatformAPI* s_instance;
};

#endif // PLATFORM_API_H 