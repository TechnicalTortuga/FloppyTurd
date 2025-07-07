#include "UIManager.h"

UIManager& UIManager::GetInstance()
{
    static UIManager instance;
    return instance;
}

void UIManager::Initialize(float width, float height, Rectangle area)
{
    this->screenWidth = width;
    this->screenHeight = height;
    this->safeArea = area;
}

Rectangle UIManager::GetSafeArea() const
{
    return this->safeArea;
}

float UIManager::GetScaleFactor() const
{
    // Calculate scale factor based on height, assuming a landscape layout
    return this->screenHeight / this->baseHeight;
}

Vector2 UIManager::GetPosition(UIAnchor anchor, Vector2 offset) const
{
    Vector2 position = {0, 0};

    // Calculate base position from the safe area
    switch (anchor)
    {
        case UIAnchor::TOP_LEFT:
            position = { safeArea.x, safeArea.y };
            break;
        case UIAnchor::TOP_CENTER:
            position = { safeArea.x + safeArea.width / 2.0f, safeArea.y };
            break;
        case UIAnchor::TOP_RIGHT:
            position = { safeArea.x + safeArea.width, safeArea.y };
            break;
        case UIAnchor::CENTER_LEFT:
            position = { safeArea.x, safeArea.y + safeArea.height / 2.0f };
            break;
        case UIAnchor::CENTER:
            position = { safeArea.x + safeArea.width / 2.0f, safeArea.y + safeArea.height / 2.0f };
            break;
        case UIAnchor::CENTER_RIGHT:
            position = { safeArea.x + safeArea.width, safeArea.y + safeArea.height / 2.0f };
            break;
        case UIAnchor::BOTTOM_LEFT:
            position = { safeArea.x, safeArea.y + safeArea.height };
            break;
        case UIAnchor::BOTTOM_CENTER:
            position = { safeArea.x + safeArea.width / 2.0f, safeArea.y + safeArea.height };
            break;
        case UIAnchor::BOTTOM_RIGHT:
            position = { safeArea.x + safeArea.width, safeArea.y + safeArea.height };
            break;
    }

    // Apply offset, scaled for resolution independence
    float scale = GetScaleFactor();
    position.x += offset.x * scale;
    position.y += offset.y * scale;

    return position;
} 