#include "UICoordinateSystem.h"

Rectangle UICoordinateSystem::GetPixelScreenRect() {
    // This is the fallback implementation for non-iOS platforms
    // iOS uses the .mm implementation instead
    return Rectangle{ 0, 0, 1920, 1080 }; // Default fallback
}

Rectangle UICoordinateSystem::GetPointScreenRect() {
    // This is the fallback implementation for non-iOS platforms
    // iOS uses the .mm implementation instead
    return Rectangle{ 0, 0, 1920, 1080 }; // Default fallback
}

Rectangle UICoordinateSystem::GetSafeAreaRect(bool inPixels) {
    // This is the fallback implementation for non-iOS platforms
    // iOS uses the .mm implementation instead
    Rectangle screenRect = inPixels ? GetPixelScreenRect() : GetPointScreenRect();
    return Rectangle{ 0, 0, screenRect.width, screenRect.height };
}

float UICoordinateSystem::PointsToPixels(float value) {
    // This is the fallback implementation for non-iOS platforms
    // iOS uses the .mm implementation instead
    return value; // 1:1 mapping for non-iOS platforms
}

Vector2 UICoordinateSystem::PointsToPixels(Vector2 pt) {
    return Vector2{ PointsToPixels(pt.x), PointsToPixels(pt.y) };
}

Rectangle UICoordinateSystem::PointsToPixels(Rectangle r) {
    return Rectangle{
        PointsToPixels(r.x),
        PointsToPixels(r.y),
        PointsToPixels(r.width),
        PointsToPixels(r.height)
    };
}

float UICoordinateSystem::PixelsToPoints(float value) {
    // This is the fallback implementation for non-iOS platforms
    // iOS uses the .mm implementation instead
    return value; // 1:1 mapping for non-iOS platforms
}

Vector2 UICoordinateSystem::PixelsToPoints(Vector2 px) {
    return Vector2{ PixelsToPoints(px.x), PixelsToPoints(px.y) };
}

Rectangle UICoordinateSystem::PixelsToPoints(Rectangle r) {
    return Rectangle{
        PixelsToPoints(r.x),
        PixelsToPoints(r.y),
        PixelsToPoints(r.width),
        PixelsToPoints(r.height)
    };
}

float UICoordinateSystem::GetNativeScale() {
    // This is the fallback implementation for non-iOS platforms
    // iOS uses the .mm implementation instead
    return 1.0f; // Standard scale for non-iOS platforms
}

Vector2 UICoordinateSystem::GetScreenCenterPixels() {
    Rectangle rect = GetPixelScreenRect();
    return Vector2{ rect.width / 2.0f, rect.height / 2.0f };
}

Vector2 UICoordinateSystem::GetScreenCenterPoints() {
    Rectangle rect = GetPointScreenRect();
    return Vector2{ rect.width / 2.0f, rect.height / 2.0f };
}

Vector2 UICoordinateSystem::GetPosition(UIAnchor anchor, Vector2 offset, bool usePixels) {
    Rectangle screenRect = usePixels ? GetPixelScreenRect() : GetPointScreenRect();
    Vector2 position = {0, 0};
    switch (anchor) {
        case UIAnchor::TOP_LEFT:
            position = {screenRect.x, screenRect.y};
            break;
        case UIAnchor::TOP_CENTER:
            position = {screenRect.x + screenRect.width / 2.0f, screenRect.y};
            break;
        case UIAnchor::TOP_RIGHT:
            position = {screenRect.x + screenRect.width, screenRect.y};
            break;
        case UIAnchor::CENTER_LEFT:
            position = {screenRect.x, screenRect.y + screenRect.height / 2.0f};
            break;
        case UIAnchor::CENTER:
            position = {screenRect.x + screenRect.width / 2.0f, screenRect.y + screenRect.height / 2.0f};
            break;
        case UIAnchor::CENTER_RIGHT:
            position = {screenRect.x + screenRect.width, screenRect.y + screenRect.height / 2.0f};
            break;
        case UIAnchor::BOTTOM_LEFT:
            position = {screenRect.x, screenRect.y + screenRect.height};
            break;
        case UIAnchor::BOTTOM_CENTER:
            position = {screenRect.x + screenRect.width / 2.0f, screenRect.y + screenRect.height};
            break;
        case UIAnchor::BOTTOM_RIGHT:
            position = {screenRect.x + screenRect.width, screenRect.y + screenRect.height};
            break;
    }
    // Apply offset
    position.x += offset.x;
    position.y += offset.y;
    return position;
}

float UICoordinateSystem::GetUIScaleFactor() {
    float scale = GetNativeScale();
    // Scale UI elements based on screen density
    if (scale >= 3.0f) return 1.5f;      // Super high DPI
    if (scale >= 2.0f) return 1.25f;     // High DPI
    if (scale >= 1.5f) return 1.1f;      // Medium DPI
    return 1.0f;                         // Standard DPI
}

float UICoordinateSystem::GetMinTouchSize() {
    // Minimum touch target size in pixels (44pt * scale)
    return 44.0f * GetNativeScale();
}

int UICoordinateSystem::GetRecommendedFontSize(int baseSize) {
    float scale = GetUIScaleFactor();
    return (int)(baseSize * scale);
}

// Helper functions are implemented in UICoordinateSystem.mm for iOS
// and not needed for the fallback implementation 