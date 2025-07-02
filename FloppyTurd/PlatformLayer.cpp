#include "PlatformLayer.h"
#include <filesystem>
#include <algorithm>

#ifdef PLATFORM_ANDROID
#include <android/native_activity.h>
#include <jni.h>
#endif

void PlatformLayer::Initialize() {
    // Platform-specific initialization
#ifdef PLATFORM_MOBILE
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#else
    // Removed FLAG_WINDOW_HIGHDPI to fix mouse coordinate issues on macOS
    // Let the game handle scaling manually for better control
#endif
}

void PlatformLayer::Shutdown() {
    // Platform-specific cleanup
}

std::string PlatformLayer::GetResourcePath(const std::string& relativePath) {
#ifdef PLATFORM_ANDROID
    // On Android, resources are in the APK
    return relativePath;
#elif defined(PLATFORM_IOS)
    // On iOS, resources are in the app bundle
    return GetApplicationDirectory() + relativePath;
#else
    // On desktop, resources are in the "resources" subdirectory
    return "resources/" + relativePath;
#endif
}

std::string PlatformLayer::GetSavePath(const std::string& filename) {
#ifdef PLATFORM_ANDROID
    // Android: Use internal storage
    return std::string(GetAndroidApp()->activity->internalDataPath) + "/" + filename;
#elif defined(PLATFORM_IOS)
    // iOS: Use documents directory
    return GetApplicationDirectory() + "../Documents/" + filename;
#else
    // Desktop: Use current directory
    return filename;
#endif
}

bool PlatformLayer::IsTouchSupported() const {
#ifdef PLATFORM_MOBILE
    return true;
#else
    return false;
#endif
}

Vector2 PlatformLayer::GetPrimaryInputPosition() const {
#ifdef PLATFORM_MOBILE
    if (GetTouchPointCount() > 0) {
        return GetTouchPosition(0);
    }
    return { -1, -1 };
#else
    return GetMousePosition();
#endif
}

bool PlatformLayer::IsPrimaryInputPressed() const {
#ifdef PLATFORM_MOBILE
    return GetTouchPointCount() > 0 && IsGestureDetected(GESTURE_TAP);
#else
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
#endif
}

bool PlatformLayer::IsPrimaryInputDown() const {
#ifdef PLATFORM_MOBILE
    return GetTouchPointCount() > 0;
#else
    return IsMouseButtonDown(MOUSE_LEFT_BUTTON);
#endif
}

bool PlatformLayer::IsPrimaryInputReleased() const {
#ifdef PLATFORM_MOBILE
    static int previousTouchCount = 0;
    int currentTouchCount = GetTouchPointCount();
    bool released = (previousTouchCount > 0 && currentTouchCount == 0);
    previousTouchCount = currentTouchCount;
    return released;
#else
    return IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
#endif
}

std::vector<Vector2> PlatformLayer::GetTouchPoints() const {
    std::vector<Vector2> points;
#ifdef PLATFORM_MOBILE
    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        points.push_back(GetTouchPosition(i));
    }
#else
    // On desktop, simulate single touch with mouse
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        points.push_back(GetMousePosition());
    }
#endif
    return points;
}

float PlatformLayer::GetScreenDensity() const {
#ifdef PLATFORM_ANDROID
    // Get DPI from Android
    return 2.0f; // Default to 2x for now, should query actual DPI
#elif defined(PLATFORM_IOS)
    // iOS devices typically have 2x or 3x displays
    return GetWindowScaleDPI().x;
#else
    return 1.0f;
#endif
}

void PlatformLayer::SetOrientation(bool landscape) {
#ifdef PLATFORM_MOBILE
    // This would need platform-specific implementation
    // For now, just log
    TraceLog(LOG_INFO, "SetOrientation: %s", landscape ? "Landscape" : "Portrait");
#else
    (void)landscape; // Unused on desktop
#endif
}

bool PlatformLayer::SupportsFullscreen() const {
#ifdef PLATFORM_MOBILE
    return false; // Mobile apps are always "fullscreen"
#else
    return true;
#endif
}

void PlatformLayer::ShowVirtualKeyboard(bool show) {
#ifdef PLATFORM_ANDROID
    // Would need JNI calls to show/hide keyboard
    virtualKeyboardShown = show;
#elif defined(PLATFORM_IOS)
    // Would need Objective-C calls
    virtualKeyboardShown = show;
#else
    (void)show; // Unused on desktop
#endif
}

bool PlatformLayer::IsVirtualKeyboardShown() const {
#ifdef PLATFORM_MOBILE
    return virtualKeyboardShown;
#else
    return false;
#endif
}

void PlatformLayer::Vibrate(int milliseconds) {
#ifdef PLATFORM_ANDROID
    // Would need JNI calls to vibrate
    TraceLog(LOG_INFO, "Vibrate for %d ms", milliseconds);
#elif defined(PLATFORM_IOS)
    // Would need haptic feedback API
    TraceLog(LOG_INFO, "Vibrate for %d ms", milliseconds);
#else
    (void)milliseconds; // Unused on desktop
#endif
}

Rectangle PlatformLayer::GetSafeArea() const {
    // Get screen dimensions
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    
#ifdef PLATFORM_IOS
    // iOS safe area insets (simplified - would need actual API calls)
    float topInset = 44.0f;    // Status bar + notch
    float bottomInset = 34.0f; // Home indicator
    return {
        0, topInset,
        screenWidth, screenHeight - topInset - bottomInset
    };
#else
    // No safe area on other platforms
    return { 0, 0, screenWidth, screenHeight };
#endif
}

bool PlatformLayer::PreferLowPowerMode() const {
#ifdef PLATFORM_MOBILE
    // Could check battery level and thermal state
    return false;
#else
    return false;
#endif
}

int PlatformLayer::GetRecommendedTextureSize() const {
#ifdef PLATFORM_MOBILE
    // Mobile devices may have texture size limits
    return 2048;
#else
    return 4096;
#endif
} 