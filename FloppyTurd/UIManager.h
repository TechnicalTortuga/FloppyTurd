#pragma once

#include "UIAnchor.h"
#include "PlatformAPI.h"
#include <vector>
#include <string>

class UIManager
{
public:
    // Singleton access
    static UIManager& GetInstance();

    // Deleted copy constructor and assignment operator to prevent duplication
    UIManager(const UIManager&) = delete;
    void operator=(const UIManager&) = delete;

    // Initialization (new: supports both points and pixels)
    void Initialize(float screenWidthPoints, float screenHeightPoints,
                    float screenWidthPixels, float screenHeightPixels,
                    Rectangle safeAreaPoints, Rectangle safeAreaPixels);

    // Layout calculation
    // If usePixels is true, returns position in pixel coordinates, else in points
    Vector2 GetPosition(UIAnchor anchor, Vector2 offset = {0, 0}, bool usePixels = true) const;
    Rectangle GetSafeArea(bool usePixels = true) const;
    float GetScaleFactor(bool usePixels = true) const;

    // Conversion helpers
    float PointsToPixels(float value) const;
    float PixelsToPoints(float value) const;
    Vector2 PointsToPixels(Vector2 pt) const;
    Vector2 PixelsToPoints(Vector2 px) const;
    Rectangle PointsToPixels(Rectangle r) const;
    Rectangle PixelsToPoints(Rectangle r) const;

    float GetScreenWidth(bool usePixels = true) const { return usePixels ? screenWidthPixels : screenWidthPoints; }
    float GetScreenHeight(bool usePixels = true) const { return usePixels ? screenHeightPixels : screenHeightPoints; }

private:
    // Private constructor for singleton pattern
    UIManager() = default;

    // Screen and safe area in points and pixels
    float screenWidthPoints = 0.0f;
    float screenHeightPoints = 0.0f;
    float screenWidthPixels = 0.0f;
    float screenHeightPixels = 0.0f;
    Rectangle safeAreaPoints = {0, 0, 0, 0};
    Rectangle safeAreaPixels = {0, 0, 0, 0};
    float nativeScale = 1.0f;
    // Base resolution for scaling calculations
    const float baseWidth = 320.0f;
    const float baseHeight = 180.0f;
}; 