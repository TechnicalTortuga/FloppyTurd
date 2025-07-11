#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

#pragma once

#include "RaylibCompat.h"
#include <string>
#include <vector>

// Forward declarations for Metal rendering classes (iOS)
class MetalRenderer;
class MetalTextRenderer;

// Forward declaration for PlatformLayerDelegate (iOS) - DEPRECATED
#ifdef PLATFORM_IOS
#ifdef __OBJC__
@class PlatformLayerDelegate; // Will be removed in next phase
#endif
#endif

#ifdef PLATFORM_MOBILE
#include "TouchControls.h"
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

// Platform detection macros
#if defined(__ANDROID__)
    #define PLATFORM_MOBILE
    #define PLATFORM_ANDROID
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
#ifndef PLATFORM_IOS
#define PLATFORM_IOS
#endif
        #define PLATFORM_MOBILE
        // #define PLATFORM_IOS  // Commented out to avoid redefinition warning
    #endif
#endif

class PlatformLayer {
public:
    static PlatformLayer& GetInstance();

    // Deleted copy constructor and assignment operator for singleton
    PlatformLayer(const PlatformLayer&) = delete;
    PlatformLayer& operator=(const PlatformLayer&) = delete;

    // Platform-specific features
    void Initialize(void* nativeView);
    // Overload for initialization when native view is not yet available (e.g., desktop or high-level bootstrap)
    void Initialize();
    // Overload for iOS that accepts GameViewController and MetalRenderer for touch event forwarding
#ifdef PLATFORM_IOS
    void Initialize(void* nativeView, void* gameViewController, void* metalRenderer);
#endif
    void Shutdown();

    // App lifecycle events
    void OnAppWillResignActive();
    void OnAppDidBecomeActive();

    // File system helpers
    std::string GetResourcePath(const std::string& relativePath);
    std::string GetSavePath(const std::string& filename);

    // Input handling
    bool IsTouchSupported() const;
    Vector2 GetPrimaryInputPosition() const; // Mouse on desktop, primary touch on mobile
    bool IsPrimaryInputDown() const;
    bool IsPrimaryInputPressed() const;
    bool IsPrimaryInputReleased() const;
    bool IsSecondaryInputPressed() const;
    bool IsSecondaryInputDown() const;
    bool IsSecondaryInputReleased() const;

    // Touch-specific helpers (iOS)
    int GetTouchCount() const;
    Vector2 GetTouchPosition(int index) const;

    // Get all active touch points (for multi-touch support)
    std::vector<Vector2> GetTouchPoints() const;

    // Screen/Window management
    float GetScreenDensity() const;
    void SetOrientation(bool landscape);
    bool SupportsFullscreen() const;
    int GetScreenWidth() const;
    int GetScreenHeight() const;
    double GetCurrentTime() const;

#ifdef PLATFORM_IOS
    // DEPRECATED: Metal rendering interface - Will be removed in next phase
    // These methods are now handled by GameView and MetalRenderer directly
    MetalRenderer* GetMetalRenderer() const;
    float GetLastFrameTime() const;
    int GetLastFPS() const;

    // Texture and image handling - BRIDGE FUNCTIONS (keep these!)
    void* LoadTexture(const char* fileName, int* width, int* height);
    void UnloadTexture(void* texture);
    void* CreateTextureFromImage(void* image, int* width, int* height);

    // Image handling - BRIDGE FUNCTIONS (keep these!)
    void* LoadImage(const char* fileName, int* width, int* height);
    void UnloadImage(void* image);
    void* CreateSolidColorImage(int width, int height, Color color);

    // Audio management - BRIDGE FUNCTIONS (keep these!)
    void InitializeAudio();
    void ShutdownAudio();
    void* LoadSound(const char* fileName);
    void UnloadSound(void* sound);
    void PlaySound(void* sound);
    void SetSoundVolume(void* sound, float volume);

    // Music streaming - BRIDGE FUNCTIONS (keep these!)
    void* LoadMusic(const char* fileName);
    void UnloadMusic(void* music);
    void PlayMusic(void* music);
    void StopMusic(void* music);
    void UpdateMusic(void* music);
    bool IsMusicPlaying(void* music);
    void SetMusicVolume(void* music, float volume);

    // Text rendering via MetalTextRenderer - BRIDGE FUNCTIONS (keep these!)
    void* LoadFont(const char* fileName, int size);
    void UnloadFont(void* font);
    Vector2 MeasureText(const char* text, void* font, float fontSize, float spacing);
    void DrawText(const char* text, float x, float y, float fontSize, Color color, void* font);

    // Rendering integration - BRIDGE FUNCTIONS (keep these!)
    void* LoadRenderTexture(int width, int height);
    void UnloadRenderTexture(void* texture);
    void BeginDrawing(void* renderTexture);
    void EndDrawing(void* renderTexture);
    void DrawRectangle(int posX, int posY, int width, int height, unsigned int color);
    void DrawLineEx(float x1, float y1, float x2, float y2, float thickness, Color color);
    void DrawRectangleRoundedLines(float x, float y, float width, float height, float roundness, int segments, float lineThick, Color color);
    void* LoadTextureFromImage(void* imageData, int width, int height, int format);
    void DrawTexture(void* texture, float x, float y, float width, float height, Color tint);
    void EnqueueDrawCommand(void* vertexBuffer, void* texture, size_t vertexCount);
#endif

    // Platform-specific features
    void ShowVirtualKeyboard(bool show);
    bool IsVirtualKeyboardShown() const;
    void Vibrate(int milliseconds);

    // Delegate access (iOS) - DEPRECATED: Will be removed in next phase
    void* GetDelegate() const;
    
    // Platform-agnostic resource path helper
    // On iOS: returns path as-is (for asset catalogs)
    // On other platforms: prepends "resources/" for file system access
    std::string GetPlatformResourcePath(const std::string& relativePath) const;

    // Safe area handling (for notched devices)
    Rectangle GetSafeArea() const;

    // Performance hints
    bool PreferLowPowerMode() const;
    int GetRecommendedTextureSize() const;

    // UI and System
    void SetWindowTitle(const std::string& title);
    void SetWindowSize(int width, int height);
    bool IsWindowFullscreen() const;
    void ToggleFullscreen();
    float GetScreenScale() const;

    bool IsMobilePlatform() const;
    void UpdateTouchState();
    void ClearTouchStatesAfterRender(); // Clear touch states after render phase
    
    // Static touch state management functions (delegate to TouchControls)
    static void SetTouchState(bool pressed, float x, float y);
    static void ClearAllTouchStates();

#ifdef PLATFORM_MOBILE
    TouchControls* GetTouchControls() { return &m_TouchControls; }
#endif

private:
    // Private constructor for singleton
    PlatformLayer();
    ~PlatformLayer();

    // Forward-declared implementation class (PIMPL)
    class PlatformLayerImpl;
    PlatformLayerImpl* m_pImpl;

    void* m_View; // Pointer to UIView
    void* m_Delegate; // Pointer to PlatformLayerDelegate (Objective-C) - DEPRECATED
    std::vector<Vector2> m_TouchPoints;
    bool m_PrimaryInputDown;
    bool m_PrimaryInputPressed;
    bool m_PrimaryInputReleased;
    bool m_SecondaryInputDown;
    bool m_SecondaryInputPressed;
    bool m_SecondaryInputReleased;

#ifdef PLATFORM_MOBILE
    TouchControls m_TouchControls;
#endif

#ifdef PLATFORM_MOBILE
    bool virtualKeyboardShown = false;
#endif
};

#endif // PLATFORM_LAYER_H