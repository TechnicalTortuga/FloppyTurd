#import "UICoordinateSystem.h"
#include "PlatformAPI.h"
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>
#include <algorithm>

#if defined(__APPLE__) && TARGET_OS_IOS

Rectangle UICoordinateSystem::GetPixelScreenRect() {
    UIScreen* screen = GetCurrentScreen();
    if (!screen) {
        TraceLog(LOG_ERROR, "[UICoordinateSystem] Failed to get current screen");
        return {0, 0, 320, 180}; // Fallback
    }
    
    CGRect nativeBounds = screen.nativeBounds;
    return {0, 0, (float)nativeBounds.size.width, (float)nativeBounds.size.height};
}

Rectangle UICoordinateSystem::GetPointScreenRect() {
    UIScreen* screen = GetCurrentScreen();
    if (!screen) {
        TraceLog(LOG_ERROR, "[UICoordinateSystem] Failed to get current screen");
        return {0, 0, 320, 180}; // Fallback
    }
    
    CGRect bounds = screen.bounds;
    return {0, 0, (float)bounds.size.width, (float)bounds.size.height};
}

Rectangle UICoordinateSystem::GetSafeAreaRect(bool inPixels) {
    UIWindow* window = GetCurrentWindow();
    if (!window) {
        TraceLog(LOG_ERROR, "[UICoordinateSystem] Failed to get current window");
        return GetPixelScreenRect(); // Fallback to full screen
    }
    
    UIEdgeInsets insets = window.safeAreaInsets;
    CGRect bounds = window.bounds;
    float scale = inPixels ? GetNativeScale() : 1.0f;
    
    return {
        static_cast<float>(insets.left * scale),
        static_cast<float>(insets.top * scale),
        static_cast<float>((bounds.size.width - insets.left - insets.right) * scale),
        static_cast<float>((bounds.size.height - insets.top - insets.bottom) * scale)
    };
}

float UICoordinateSystem::PointsToPixels(float value) {
    return value * GetNativeScale();
}

Vector2 UICoordinateSystem::PointsToPixels(Vector2 pt) {
    float scale = GetNativeScale();
    return {pt.x * scale, pt.y * scale};
}

Rectangle UICoordinateSystem::PointsToPixels(Rectangle r) {
    float scale = GetNativeScale();
    return {r.x * scale, r.y * scale, r.width * scale, r.height * scale};
}

float UICoordinateSystem::PixelsToPoints(float value) {
    return value / GetNativeScale();
}

Vector2 UICoordinateSystem::PixelsToPoints(Vector2 px) {
    float scale = GetNativeScale();
    return {px.x / scale, px.y / scale};
}

Rectangle UICoordinateSystem::PixelsToPoints(Rectangle r) {
    float scale = GetNativeScale();
    return {r.x / scale, r.y / scale, r.width / scale, r.height / scale};
}

float UICoordinateSystem::GetNativeScale() {
    UIScreen* screen = GetCurrentScreen();
    if (!screen) {
        TraceLog(LOG_ERROR, "[UICoordinateSystem] Failed to get current screen for scale");
        return 1.0f; // Fallback
    }
    return screen.nativeScale;
}

Vector2 UICoordinateSystem::GetScreenCenterPixels() {
    Rectangle pixelRect = GetPixelScreenRect();
    return {pixelRect.width / 2.0f, pixelRect.height / 2.0f};
}

Vector2 UICoordinateSystem::GetScreenCenterPoints() {
    Rectangle pointRect = GetPointScreenRect();
    return {pointRect.width / 2.0f, pointRect.height / 2.0f};
}

Vector2 UICoordinateSystem::GetPosition(UIAnchor anchor, Vector2 offset, bool usePixels) {
    Rectangle area = usePixels ? GetSafeAreaRect(true) : GetSafeAreaRect(false);
    Vector2 position = {0, 0};
    switch (anchor) {
        case UIAnchor::TOP_LEFT:
            position = {area.x, area.y};
            break;
        case UIAnchor::TOP_CENTER:
            position = {area.x + area.width / 2.0f, area.y};
            break;
        case UIAnchor::TOP_RIGHT:
            position = {area.x + area.width, area.y};
            break;
        case UIAnchor::CENTER_LEFT:
            position = {area.x, area.y + area.height / 2.0f};
            break;
        case UIAnchor::CENTER:
            position = {area.x + area.width / 2.0f, area.y + area.height / 2.0f};
            break;
        case UIAnchor::CENTER_RIGHT:
            position = {area.x + area.width, area.y + area.height / 2.0f};
            break;
        case UIAnchor::BOTTOM_LEFT:
            position = {area.x, area.y + area.height};
            break;
        case UIAnchor::BOTTOM_CENTER:
            position = {area.x + area.width / 2.0f, area.y + area.height};
            break;
        case UIAnchor::BOTTOM_RIGHT:
            position = {area.x + area.width, area.y + area.height};
            break;
    }
    // Apply offset
    position.x += offset.x;
    position.y += offset.y;
    return position;
}

float UICoordinateSystem::GetUIScaleFactor() {
    float nativeScale = GetNativeScale();
    
    // Cap UI scale to prevent oversized elements
    float uiScale = std::min(nativeScale * 0.8f, 2.0f);
    
    // Ensure minimum scale for readability
    uiScale = std::max(uiScale, 1.0f);
    
    return uiScale;
}

float UICoordinateSystem::GetMinTouchSize() {
    // Apple guidelines recommend 44pt minimum touch target
    return 44.0f * GetNativeScale();
}

int UICoordinateSystem::GetRecommendedFontSize(int baseSize) {
    float uiScale = GetUIScaleFactor();
    int scaledSize = (int)(baseSize * uiScale);
    
    // Ensure minimum readable size
    if (scaledSize < 16) scaledSize = 16;
    
    // Cap maximum size to prevent overly large text
    if (scaledSize > 48) scaledSize = 48;
    
    return scaledSize;
}

UIScreen* UICoordinateSystem::GetCurrentScreen() {
    return [UIScreen mainScreen];
}

UIWindow* UICoordinateSystem::GetCurrentWindow() {
    NSArray<UIWindow*>* windows = [UIApplication sharedApplication].windows;
    for (UIWindow* window in windows) {
        if (window.isKeyWindow) {
            return window;
        }
    }
    
    // Fallback to first window if no key window
    if (windows.count > 0) {
        return windows.firstObject;
    }
    
    return nil;
}

#endif // defined(__APPLE__) && TARGET_OS_IOS 