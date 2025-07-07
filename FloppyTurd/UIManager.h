#pragma once

#include "RaylibCompat.h"

// Enum for defining UI anchor points
enum class UIAnchor
{
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

class UIManager
{
public:
    // Singleton access
    static UIManager& GetInstance();

    // Deleted copy constructor and assignment operator to prevent duplication
    UIManager(const UIManager&) = delete;
    void operator=(const UIManager&) = delete;

    // Initialization
    void Initialize(float screenWidth, float screenHeight, Rectangle safeArea);

    // Layout calculation
    Vector2 GetPosition(UIAnchor anchor, Vector2 offset = {0, 0}) const;
    Rectangle GetSafeArea() const;
    float GetScaleFactor() const;

private:
    // Private constructor for singleton pattern
    UIManager() = default;

    float screenWidth = 0.0f;
    float screenHeight = 0.0f;
    Rectangle safeArea = {0, 0, 0, 0};
    
    // Base resolution for scaling calculations
    const float baseWidth = 320.0f;
    const float baseHeight = 180.0f;
}; 