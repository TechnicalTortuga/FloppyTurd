#include "Window.h"
#include <cstdio>  // For printf

Window::Window(bool fullScreen, int fallbackW, int fallbackH)
{
#ifdef PLATFORM_MOBILE
    // Mobile platform initialization
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_MAXIMIZED);
    
    // Initialize with default mobile resolution, will be adjusted by platform
    InitWindow(1920, 1080, "Floppy Turd");
    
    // Get actual screen dimensions (may differ from initialized size)
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Ensure we're using the full screen real estate on mobile
    SetWindowSize(screenWidth, screenHeight);
    
    printf("Mobile platform initialized: %dx%d\n", screenWidth, screenHeight);
    
    // Store mobile screen dimensions for future reference
    detectedMonitorWidth = screenWidth;
    detectedMonitorHeight = screenHeight;
    
#else
    // ALWAYS start in windowed mode to avoid macOS coordinate issues
    // We'll switch to fullscreen after initialization if needed
    
    // Always use windowed mode flags initially
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    
    // Create a temporary small window to initialize raylib and detect monitor resolution
    InitWindow(800, 600, "Floppy Turd");
    
    // Now that raylib is initialized, detect the native monitor resolution
    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);
    printf("Detected monitor resolution: %dx%d\n", monitorWidth, monitorHeight);
    
    // Use monitor resolution as default, but respect fallback if specified and smaller (to avoid overflow)
    int windowWidth = (fallbackW > 0 && fallbackW < monitorWidth) ? fallbackW : monitorWidth;
    int windowHeight = (fallbackH > 0 && fallbackH < monitorHeight) ? fallbackH : monitorHeight;
    
    // If fallback values are 0, use native monitor resolution
    if (fallbackW == 0) windowWidth = monitorWidth;
    if (fallbackH == 0) windowHeight = monitorHeight;
    
    // Close the temporary window and recreate with proper size
    CloseWindow();
    
    // Reinitialize with the detected/calculated size
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(windowWidth, windowHeight, "Floppy Turd");
    SetExitKey(KEY_NULL);          // ESC will be handled by the game itself
    
    // Ensure window is properly focused on macOS
    #ifdef __APPLE__
    SetWindowFocused();
    #endif
    
    printf("Windowed mode: %dx%d\n", GetScreenWidth(), GetScreenHeight());
    printf("Using resolution: %dx%d (monitor: %dx%d)\n", windowWidth, windowHeight, monitorWidth, monitorHeight);
    
    // Now switch to fullscreen if requested
    // This ensures consistent coordinate handling across platforms
    if (fullScreen)
    {
        // Option 1: True fullscreen (limited resolution options)
        ToggleFullscreen();
        
        // After switching to fullscreen, re-check monitor resolution (sometimes more accurate)
        int fullscreenMonitor = GetCurrentMonitor();
        int fullscreenMonitorWidth = GetMonitorWidth(fullscreenMonitor);
        int fullscreenMonitorHeight = GetMonitorHeight(fullscreenMonitor);
        
        // Store the accurate monitor resolution for future windowed mode transitions
        detectedMonitorWidth = fullscreenMonitorWidth;
        detectedMonitorHeight = fullscreenMonitorHeight;
        
        printf("Switched to fullscreen mode: %dx%d\n", GetScreenWidth(), GetScreenHeight());
        printf("Fullscreen monitor detection: %dx%d (stored for windowed mode)\n", fullscreenMonitorWidth, fullscreenMonitorHeight);
        
        // Option 2: Borderless windowed fullscreen (more flexible - uncomment to use)
        // SetWindowSize(monitorWidth, monitorHeight);
        // SetWindowPosition(0, 0);
        // printf("Switched to borderless windowed fullscreen: %dx%d\n", GetScreenWidth(), GetScreenHeight());
    }
#endif

    // Common mobile/desktop setup
    SetExitKey(KEY_NULL);          // ESC will be handled by the game itself
}

Window::~Window()
{
    CloseWindow();
}

void Window::ToggleMode()
{
    if (IsWindowFullscreen()) {
        // Switching from fullscreen to windowed
        ::ToggleFullscreen();
        
        // Resize to appropriate windowed size
        int windowWidth, windowHeight;
        GetOptimalWindowedSize(windowWidth, windowHeight);
        SetWindowSize(windowWidth, windowHeight);
        SetWindowPosition(50, 50);
        
        printf("Switched to windowed mode: %dx%d\n", GetScreenWidth(), GetScreenHeight());
    } else {
        // Switching from windowed to fullscreen
        ::ToggleFullscreen();
        printf("Switched to fullscreen mode: %dx%d\n", GetScreenWidth(), GetScreenHeight());
    }
}

void Window::GetOptimalWindowedSize(int& width, int& height)
{
    // Use stored monitor resolution if available (most accurate)
    if (detectedMonitorWidth > 0 && detectedMonitorHeight > 0) {
        width = (int)(detectedMonitorWidth * 0.85f);   // 85% of monitor width
        height = (int)(detectedMonitorHeight * 0.85f); // 85% of monitor height
    } else {
        // Fallback: use current monitor detection
        int monitor = GetCurrentMonitor();
        int monitorWidth = GetMonitorWidth(monitor);
        int monitorHeight = GetMonitorHeight(monitor);
        width = (int)(monitorWidth * 0.85f);
        height = (int)(monitorHeight * 0.85f);
    }
    
    // Ensure minimum size
    if (width < 800) width = 800;
    if (height < 600) height = 600;
}

void Window::SetBorderlessFullscreen(bool enable)
{
    if (enable && !borderlessFullscreen) {
        // Switch to borderless windowed fullscreen
        if (IsWindowFullscreen()) {
            ToggleFullscreen(); // Exit true fullscreen first
        }
        
        int monitor = GetCurrentMonitor();
        int monitorWidth = GetMonitorWidth(monitor);
        int monitorHeight = GetMonitorHeight(monitor);
        
        SetWindowSize(monitorWidth, monitorHeight);
        SetWindowPosition(0, 0);
        borderlessFullscreen = true;
        
        printf("Switched to borderless windowed fullscreen: %dx%d\n", 
               GetScreenWidth(), GetScreenHeight());
    }
    else if (!enable && borderlessFullscreen) {
        // Switch back to windowed mode using monitor's native resolution
        int monitor = GetCurrentMonitor();
        int monitorWidth = GetMonitorWidth(monitor);
        int monitorHeight = GetMonitorHeight(monitor);
        
        // Use slightly smaller size to account for window decorations and taskbar
        int windowWidth = (int)(monitorWidth * 0.9f);
        int windowHeight = (int)(monitorHeight * 0.9f);
        
        SetWindowSize(windowWidth, windowHeight);
        SetWindowPosition(50, 50); // Center with some offset
        borderlessFullscreen = false;
        
        printf("Switched back to windowed mode: %dx%d (monitor: %dx%d)\n", 
               GetScreenWidth(), GetScreenHeight(), monitorWidth, monitorHeight);
    }
}

bool Window::IsBorderlessFullscreen() const
{
    return borderlessFullscreen;
}

// Mobile-specific screen management methods
Rectangle Window::GetSafeArea() const
{
    return PlatformLayer::GetInstance().GetSafeArea();
}

float Window::GetScreenDensity() const
{
    return PlatformLayer::GetInstance().GetScreenDensity();
}

bool Window::IsLandscape() const
{
    int width = GetScreenWidth();
    int height = GetScreenHeight();
    return width > height;
}

bool Window::IsPortrait() const
{
    return !IsLandscape();
}

void Window::SetPreferredOrientation(bool landscape)
{
    preferLandscape = landscape;
    PlatformLayer::GetInstance().SetOrientation(landscape);
}

// Universal screen utilities
Vector2 Window::GetScreenCenter() const
{
    return Vector2{ 
        (float)GetScreenWidth() / 2.0f, 
        (float)GetScreenHeight() / 2.0f 
    };
}

Vector2 Window::GetRenderScale() const
{
    float density = GetScreenDensity();
    
#ifdef PLATFORM_MOBILE
    // On mobile, scale based on screen density for crisp UI
    return Vector2{ density, density };
#else
    // On desktop, use 1:1 scaling unless high DPI
    if (density > 1.5f) {
        return Vector2{ density, density };
    }
    return Vector2{ 1.0f, 1.0f };
#endif
}

bool Window::ShouldUseLargerTouchTargets() const
{
#ifdef PLATFORM_MOBILE
    return true;  // Always use larger touch targets on mobile
#else
    return false; // Desktop can use smaller mouse targets
#endif
}

int Window::GetRecommendedFontSize() const
{
    float density = GetScreenDensity();
    int baseSize = 16;  // Base font size for desktop
    
#ifdef PLATFORM_MOBILE
    // Scale font size based on screen density for mobile readability
    int scaledSize = (int)(baseSize * density);
    
    // Ensure minimum readable size on mobile
    if (scaledSize < 18) scaledSize = 18;
    
    // Cap maximum size to prevent overly large text
    if (scaledSize > 32) scaledSize = 32;
    
    return scaledSize;
#else
    // Desktop: only scale for high DPI displays
    if (density > 1.5f) {
        return (int)(baseSize * density);
    }
    return baseSize;
#endif
}
