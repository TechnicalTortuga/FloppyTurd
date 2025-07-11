#ifndef PLATFORM_SPECIFIC_H
#define PLATFORM_SPECIFIC_H

#pragma once

#include "RaylibCompat.h"
#include <string>
#include <vector>

/**
 * PlatformSpecific - Base class for platform-specific implementations
 * 
 * This class defines the interface that all platform implementations must follow.
 * Each platform (iOS, Desktop, Android) will provide its own implementation
 * of these virtual methods.
 */
class PlatformSpecific {
public:
    virtual ~PlatformSpecific() = default;

    // ============================================================================
    // INITIALIZATION AND LIFECYCLE
    // ============================================================================
    
    /**
     * Initialize the platform-specific implementation
     * @param nativeView Platform-specific view pointer (UIView on iOS, etc.)
     */
    virtual void Initialize(void* nativeView) = 0;
    
    /**
     * Initialize the platform-specific implementation without native view
     */
    virtual void Initialize() = 0;
    
    /**
     * Shutdown the platform-specific implementation and cleanup resources
     */
    virtual void Shutdown() = 0;

    // ============================================================================
    // RENDERING FUNCTIONS (Raylib-compatible)
    // ============================================================================
    
    // Drawing primitives
    virtual void DrawRectangle(int x, int y, int width, int height, Color color) = 0;
    virtual void DrawRectangleRec(Rectangle rec, Color color) = 0;
    virtual void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) = 0;
    virtual void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) = 0;
    virtual void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) = 0;
    
    virtual void DrawCircle(float centerX, float centerY, float radius, Color color) = 0;
    virtual void DrawCircleV(Vector2 center, float radius, Color color) = 0;
    
    virtual void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color) = 0;
    virtual void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) = 0;
    virtual void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) = 0;
    
    // Texture drawing
    virtual void DrawTexture(Texture2D texture, int posX, int posY, Color tint) = 0;
    virtual void DrawTextureV(Texture2D texture, Vector2 position, Color tint) = 0;
    virtual void DrawTextureRec(Texture2D texture, Rectangle source, Rectangle dest, Color tint) = 0;
    virtual void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) = 0;
    
    // Text rendering
    virtual void DrawText(const char* text, int posX, int posY, int fontSize, Color color) = 0;
    virtual void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) = 0;
    
    // Render texture operations
    virtual void BeginDrawing(void* renderTexture) = 0;
    virtual void EndDrawing(void* renderTexture) = 0;
    virtual void* LoadRenderTexture(int width, int height) = 0;
    virtual void UnloadRenderTexture(void* renderTexture) = 0;

    // ============================================================================
    // TEXTURE AND IMAGE FUNCTIONS
    // ============================================================================
    
    virtual Texture2D LoadTexture(const char* fileName) = 0;
    virtual void UnloadTexture(Texture2D texture) = 0;
    virtual void* LoadTextureFromImage(void* imageData, int width, int height, int format) = 0;
    virtual void* CreateTextureFromImage(void* image, int* width, int* height) = 0;
    
    virtual Image LoadImage(const char* fileName) = 0;
    virtual void UnloadImage(Image image) = 0;
    virtual void* CreateSolidColorImage(int width, int height, Color color) = 0;

    // ============================================================================
    // AUDIO FUNCTIONS
    // ============================================================================
    
    virtual void InitializeAudio() = 0;
    virtual void ShutdownAudio() = 0;
    virtual void* LoadSound(const char* fileName) = 0;
    virtual void UnloadSound(void* sound) = 0;
    virtual void PlaySound(void* sound) = 0;
    virtual void SetSoundVolume(void* sound, float volume) = 0;
    
    virtual void* LoadMusic(const char* fileName) = 0;
    virtual void UnloadMusic(void* music) = 0;
    virtual void PlayMusic(void* music) = 0;
    virtual void StopMusic(void* music) = 0;
    virtual void UpdateMusic(void* music) = 0;
    virtual bool IsMusicPlaying(void* music) = 0;
    virtual void SetMusicVolume(void* music, float volume) = 0;
    virtual void PauseMusic(void* music) = 0;
    virtual void ResumeMusic(void* music) = 0;
    virtual void SetMusicLooping(void* music, bool looping) = 0;

    // ============================================================================
    // FONT AND TEXT FUNCTIONS
    // ============================================================================
    
    virtual void* LoadFont(const char* fileName, int size) = 0;
    virtual void UnloadFont(void* font) = 0;
    virtual Vector2 MeasureText(const char* text, void* font, float fontSize, float spacing) = 0;
    virtual void DrawText(const char* text, float x, float y, float fontSize, Color color, void* font) = 0;

    // ============================================================================
    // INPUT FUNCTIONS
    // ============================================================================
    
    // Primary input (mouse on desktop, primary touch on mobile)
    virtual bool IsPrimaryInputDown() = 0;
    virtual bool IsPrimaryInputPressed() = 0;
    virtual bool IsPrimaryInputReleased() = 0;
    virtual Vector2 GetPrimaryInputPosition() = 0;
    
    // Secondary input
    virtual bool IsSecondaryInputDown() = 0;
    virtual bool IsSecondaryInputPressed() = 0;
    virtual bool IsSecondaryInputReleased() = 0;
    
    // Touch-specific (mobile)
    virtual bool IsTouchSupported() = 0;
    virtual int GetTouchCount() = 0;
    virtual Vector2 GetTouchPosition(int index) = 0;
    virtual std::vector<Vector2> GetTouchPoints() = 0;

    // ============================================================================
    // SCREEN AND WINDOW FUNCTIONS
    // ============================================================================
    
    virtual int GetScreenWidth() = 0;
    virtual int GetScreenHeight() = 0;
    virtual Vector2 GetScreenSize() = 0;
    virtual float GetScreenDensity() = 0;
    virtual float GetScreenScale() = 0;
    virtual Rectangle GetSafeArea() = 0;
    virtual bool IsWindowFullscreen() = 0;
    virtual void ToggleFullscreen() = 0;
    virtual void SetWindowTitle(const std::string& title) = 0;
    virtual void SetWindowSize(int width, int height) = 0;
    virtual bool SupportsFullscreen() = 0;

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    virtual double GetCurrentTime() = 0;
    virtual float GetLastFrameTime() = 0;
    virtual int GetLastFPS() = 0;
    virtual std::string GetResourcePath(const std::string& relativePath) = 0;
    virtual std::string GetSavePath(const std::string& filename) = 0;
    virtual std::string GetPlatformResourcePath(const std::string& relativePath) = 0;
    virtual bool PreferLowPowerMode() = 0;
    virtual int GetRecommendedTextureSize() = 0;
    virtual void SetOrientation(bool landscape) = 0;
    virtual void ShowVirtualKeyboard(bool show) = 0;
    virtual bool IsVirtualKeyboardShown() = 0;
    virtual void Vibrate(int milliseconds) = 0;

    // ============================================================================
    // APP LIFECYCLE
    // ============================================================================
    
    virtual void OnAppWillResignActive() = 0;
    virtual void OnAppDidBecomeActive() = 0;

    // ============================================================================
    // PLATFORM DETECTION
    // ============================================================================
    
    virtual bool IsMobilePlatform() = 0;
    virtual bool IsTouchSupported() const = 0;

protected:
    // Protected constructor for base class
    PlatformSpecific() = default;
};

#endif // PLATFORM_SPECIFIC_H 