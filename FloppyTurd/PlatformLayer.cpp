#include "PlatformLayer.h"
#include <filesystem>
#include <algorithm>

#ifdef PLATFORM_ANDROID
#include <android/native_activity.h>
#include <jni.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS && defined(USE_METAL_RENDERER)
#import "RaylibCompat.h"
#endif

#ifdef PLATFORM_IOS
#import "RaylibCompat.h"
#import "HapticsManager.h"
#endif

// PIMPL Implementation
class PlatformLayer::PlatformLayerImpl {
public:
    void Initialize() {
        // Platform-specific initialization
#ifdef PLATFORM_MOBILE
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#else
        // Removed FLAG_WINDOW_HIGHDPI to fix mouse coordinate issues on macOS
        // Let the game handle scaling manually for better control
#endif
    }

    void Shutdown() {
        // Platform-specific cleanup
    }

    void OnAppWillResignActive() {
        TraceLog(LOG_INFO, "App will resign active.");
        // Pause game logic, music, etc. here
    }

    void OnAppDidBecomeActive() {
        TraceLog(LOG_INFO, "App did become active.");
        // Resume game logic, music, etc. here
    }

    std::string GetResourcePath(const std::string& relativePath) {
#ifdef PLATFORM_ANDROID
        // On Android, resources are in the APK
        return relativePath;
#elif defined(PLATFORM_IOS)
        // On iOS, resources are in the app bundle
        return GetApplicationDirectory() + relativePath;
#else
        // On desktop, resources are in the "resources" subdirectory
        return "" + relativePath;
#endif
    }

    std::string GetSavePath(const std::string& filename) {
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

    bool IsTouchSupported() const {
#ifdef PLATFORM_MOBILE
        return true;
#else
        return false;
#endif
    }

    Vector2 GetPrimaryInputPosition() const {
#ifdef PLATFORM_MOBILE
        return GetMousePosition(); // Mapped to first touch
#else
        return GetMousePosition();
#endif
    }

    bool IsPrimaryInputPressed() const {
#ifdef PLATFORM_MOBILE
        return IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
#else
        return IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
#endif
    }

    bool IsPrimaryInputDown() const {
#ifdef PLATFORM_MOBILE
        return IsMouseButtonDown(MOUSE_LEFT_BUTTON);
#else
        return IsMouseButtonDown(MOUSE_LEFT_BUTTON);
#endif
    }

    bool IsPrimaryInputReleased() const {
#ifdef PLATFORM_MOBILE
        return IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
#else
        return IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
#endif
    }

    std::vector<Vector2> GetTouchPoints() const {
        std::vector<Vector2> points;
#ifdef PLATFORM_MOBILE
        int touchCount = GetTouchCount();
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

    float GetScreenDensity() const {
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

    void SetOrientation(bool landscape) {
#ifdef PLATFORM_MOBILE
        // This would need platform-specific implementation
        // For now, just log
        TraceLog(LOG_INFO, "SetOrientation: %s", landscape ? "Landscape" : "Portrait");
#else
        (void)landscape; // Unused on desktop
#endif
    }

    bool SupportsFullscreen() const {
#ifdef PLATFORM_MOBILE
        return false; // Mobile apps are always "fullscreen"
#else
        return true;
#endif
    }

    void ShowVirtualKeyboard(bool show) {
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

    bool IsVirtualKeyboardShown() const {
#ifdef PLATFORM_MOBILE
        return virtualKeyboardShown;
#else
        return false;
#endif
    }

    void Vibrate(int milliseconds) {
#ifdef PLATFORM_ANDROID
        // Would need JNI calls to vibrate
        TraceLog(LOG_INFO, "Vibrate for %d ms", milliseconds);
#elif defined(PLATFORM_IOS)
        // Use the haptics manager
        [[HapticsManager sharedManager] playVibration];
#else
        (void)milliseconds; // Unused on desktop
#endif
    }

    Rectangle GetSafeArea() const {
#ifdef PLATFORM_IOS
        // Get screen dimensions
        float screenWidth = (float)GetScreenWidth();
        float screenHeight = (float)GetScreenHeight();
        float scale = GetWindowScaleDPI().x;

        // The g_safeAreaInsets are in points, convert to pixels
        extern Rectangle g_safeAreaInsets;
        return {
            g_safeAreaInsets.x * scale,
            g_safeAreaInsets.y * scale,
            screenWidth - (g_safeAreaInsets.x + g_safeAreaInsets.width) * scale,
            screenHeight - (g_safeAreaInsets.y + g_safeAreaInsets.height) * scale
        };
#else
        // No safe area on other platforms
        float screenWidth = (float)GetScreenWidth();
        float screenHeight = (float)GetScreenHeight();
        return { 0, 0, screenWidth, screenHeight };
#endif
    }

    bool PreferLowPowerMode() const {
#ifdef PLATFORM_MOBILE
        // Could check battery level and thermal state
        return false;
#else
        return false;
#endif
    }

    int GetRecommendedTextureSize() const {
#ifdef PLATFORM_MOBILE
        // Mobile devices may have texture size limits
        return 2048;
#else
        return 4096;
#endif
    }

    float GetScreenScale() const
    {
        return GetWindowScaleDPI().x;
    }
    
    // Static touch state management functions
    static void SetTouchState(bool pressed, float x, float y) {
        // This is implemented in PlatformLayer.mm for iOS
        // For desktop, we can implement basic mouse simulation here
#ifdef PLATFORM_MOBILE
        // iOS implementation is in PlatformLayer.mm
#else
        // Desktop implementation - simulate touch with mouse
        if (pressed) {
            // Simulate mouse press
            // This would need to be implemented based on the desktop input system
        }
#endif
    }
    
    static void ClearAllTouchStates() {
        // This is implemented in PlatformLayer.mm for iOS
        // For desktop, we can implement basic mouse simulation here
#ifdef PLATFORM_MOBILE
        // iOS implementation is in PlatformLayer.mm
#else
        // Desktop implementation - clear mouse states
        // This would need to be implemented based on the desktop input system
#endif
    }
};

// --- PlatformLayer methods forwarding to PIMPL ---

PlatformLayer::PlatformLayer() : m_pImpl(new PlatformLayerImpl()) {}
PlatformLayer::~PlatformLayer() { delete m_pImpl; }

PlatformLayer& PlatformLayer::GetInstance() {
    static PlatformLayer instance;
    return instance;
}

void PlatformLayer::Initialize() { m_pImpl->Initialize(); }
void PlatformLayer::Shutdown() { m_pImpl->Shutdown(); }
void PlatformLayer::OnAppWillResignActive() { m_pImpl->OnAppWillResignActive(); }
void PlatformLayer::OnAppDidBecomeActive() { m_pImpl->OnAppDidBecomeActive(); }
std::string PlatformLayer::GetResourcePath(const std::string& relativePath) { return m_pImpl->GetResourcePath(relativePath); }
std::string PlatformLayer::GetSavePath(const std::string& filename) { return m_pImpl->GetSavePath(filename); }
bool PlatformLayer::IsTouchSupported() const { return m_pImpl->IsTouchSupported(); }
Vector2 PlatformLayer::GetPrimaryInputPosition() const { return m_pImpl->GetPrimaryInputPosition(); }
bool PlatformLayer::IsPrimaryInputPressed() const { return m_pImpl->IsPrimaryInputPressed(); }
bool PlatformLayer::IsPrimaryInputDown() const { return m_pImpl->IsPrimaryInputDown(); }
bool PlatformLayer::IsPrimaryInputReleased() const { return m_pImpl->IsPrimaryInputReleased(); }
std::vector<Vector2> PlatformLayer::GetTouchPoints() const { return m_pImpl->GetTouchPoints(); }
float PlatformLayer::GetScreenDensity() const { return m_pImpl->GetScreenDensity(); }
void PlatformLayer::SetOrientation(bool landscape) { m_pImpl->SetOrientation(landscape); }
bool PlatformLayer::SupportsFullscreen() const { return m_pImpl->SupportsFullscreen(); }
void PlatformLayer::ShowVirtualKeyboard(bool show) { m_pImpl->ShowVirtualKeyboard(show); }
bool PlatformLayer::IsVirtualKeyboardShown() const { return m_pImpl->IsVirtualKeyboardShown(); }
void PlatformLayer::Vibrate(int milliseconds) { m_pImpl->Vibrate(milliseconds); }
Rectangle PlatformLayer::GetSafeArea() const { return m_pImpl->GetSafeArea(); }
bool PlatformLayer::PreferLowPowerMode() const { return m_pImpl->PreferLowPowerMode(); }
int PlatformLayer::GetRecommendedTextureSize() const { return m_pImpl->GetRecommendedTextureSize(); }
float PlatformLayer::GetScreenScale() const { return m_pImpl->GetScreenScale(); }

// Dummy implementations for functions that were in the header before
void PlatformLayer::SetWindowTitle(const std::string& title) { (void)title; }
void PlatformLayer::SetWindowSize(int width, int height) { (void)width; (void)height; }
bool PlatformLayer::IsWindowFullscreen() const { return false; }
void PlatformLayer::ToggleFullscreen() {}

std::string PlatformLayer::GetPlatformResourcePath(const std::string& relativePath) const
{
#ifdef PLATFORM_IOS
    // On iOS, return path as-is for asset catalog access
    return relativePath;
#else
    // On other platforms, prepend "resources/" for file system access
    return "resources/" + relativePath;
#endif
}

// Static touch state management functions
void PlatformLayer::SetTouchState(bool pressed, float x, float y) {
    m_pImpl->SetTouchState(pressed, x, y);
}

void PlatformLayer::ClearAllTouchStates() {
    m_pImpl->ClearAllTouchStates();
} 