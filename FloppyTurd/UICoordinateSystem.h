#pragma once

#include "RaylibCompat.h"
#include "UIAnchor.h"

// Forward declarations for iOS-specific types
#if defined(__APPLE__) && TARGET_OS_IOS
#ifdef __OBJC__
@class UIScreen, UIWindow;
#else
typedef struct UIScreen UIScreen;
typedef struct UIWindow UIWindow;
#endif
#endif

// Coordinate system class to bridge UIKit and Metal coordinate systems
// Handles point-to-pixel conversions and safe area calculations
class UICoordinateSystem {
public:
    // Get screen rectangle in pixels (native resolution)
    static Rectangle GetPixelScreenRect();
    
    // Get screen rectangle in points (UIKit coordinates)
    static Rectangle GetPointScreenRect();
    
    // Get safe area rectangle in either points or pixels
    static Rectangle GetSafeAreaRect(bool inPixels = true);
    
    // Convert points to pixels
    static float PointsToPixels(float value);
    static Vector2 PointsToPixels(Vector2 pt);
    static Rectangle PointsToPixels(Rectangle r);
    
    // Convert pixels to points
    static float PixelsToPoints(float value);
    static Vector2 PixelsToPoints(Vector2 px);
    static Rectangle PixelsToPoints(Rectangle r);
    
    // Get native scale factor
    static float GetNativeScale();
    
    // Get screen center in pixels
    static Vector2 GetScreenCenterPixels();
    
    // Get screen center in points
    static Vector2 GetScreenCenterPoints();
    
    // Position UI element with anchor and offset
    static Vector2 GetPosition(UIAnchor anchor, Vector2 offset = {0, 0}, bool usePixels = true);
    
    // Scale factor for UI elements based on screen density
    static float GetUIScaleFactor();
    
    // Minimum touch target size in pixels
    static float GetMinTouchSize();
    
    // Recommended font size based on screen density
    static int GetRecommendedFontSize(int baseSize = 16);

private:
    // Private constructor to prevent instantiation
    UICoordinateSystem() = delete;
    
    // Helper to get current screen
    static UIScreen* GetCurrentScreen();
    
    // Helper to get current window
    static UIWindow* GetCurrentWindow();
}; 