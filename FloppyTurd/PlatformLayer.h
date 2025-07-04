#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

#include <string>
#include <vector>
#include "raylib.h"

// Platform detection macros
#if defined(__ANDROID__)
    #define PLATFORM_MOBILE
    #define PLATFORM_ANDROID
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define PLATFORM_MOBILE
        #define PLATFORM_IOS
    #endif
#endif

class PlatformLayer {
public:
    static PlatformLayer& GetInstance();

    // Deleted copy constructor and assignment operator for singleton
    PlatformLayer(const PlatformLayer&) = delete;
    PlatformLayer& operator=(const PlatformLayer&) = delete;

    // Platform-specific features
    void Initialize();
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
    bool IsPrimaryInputPressed() const;
    bool IsPrimaryInputDown() const;
    bool IsPrimaryInputReleased() const;

    // Get all active touch points (for multi-touch support)
    std::vector<Vector2> GetTouchPoints() const;

    // Screen/Window management
    float GetScreenDensity() const;
    void SetOrientation(bool landscape);
    bool SupportsFullscreen() const;

    // Platform-specific features
    void ShowVirtualKeyboard(bool show);
    bool IsVirtualKeyboardShown() const;
    void Vibrate(int milliseconds);

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

private:
    // Private constructor for singleton
    PlatformLayer();
    ~PlatformLayer();

    // Forward-declared implementation class (PIMPL)
    class PlatformLayerImpl;
    PlatformLayerImpl* m_pImpl;

#ifdef PLATFORM_MOBILE
    bool virtualKeyboardShown = false;
#endif
};

#endif // PLATFORM_LAYER_H 