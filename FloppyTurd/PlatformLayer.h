#pragma once
#include "RaylibCompat.h"
#include <string>
#include <vector>

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
    static PlatformLayer& GetInstance() {
        static PlatformLayer instance;
        return instance;
    }

    // Initialize platform-specific features
    void Initialize();
    void Shutdown();

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

private:
    PlatformLayer() = default;
    ~PlatformLayer() = default;
    PlatformLayer(const PlatformLayer&) = delete;
    PlatformLayer& operator=(const PlatformLayer&) = delete;

#ifdef PLATFORM_MOBILE
    bool virtualKeyboardShown = false;
#endif
}; 