#include "UIManager.h"

UIManager& UIManager::GetInstance()
{
    static UIManager instance;
    return instance;
}

void UIManager::Initialize(float screenWidthPoints, float screenHeightPoints,
                          float screenWidthPixels, float screenHeightPixels,
                          Rectangle safeAreaPoints, Rectangle safeAreaPixels)
{
    this->screenWidthPoints = screenWidthPoints;
    this->screenHeightPoints = screenHeightPoints;
    this->screenWidthPixels = screenWidthPixels;
    this->screenHeightPixels = screenHeightPixels;
    this->safeAreaPoints = safeAreaPoints;
    this->safeAreaPixels = safeAreaPixels;
    this->nativeScale = (screenWidthPoints > 0) ? (screenWidthPixels / screenWidthPoints) : 1.0f;
}

float UIManager::PointsToPixels(float value) const { return value * nativeScale; }
float UIManager::PixelsToPoints(float value) const { return value / nativeScale; }
Vector2 UIManager::PointsToPixels(Vector2 pt) const { return { pt.x * nativeScale, pt.y * nativeScale }; }
Vector2 UIManager::PixelsToPoints(Vector2 px) const { return { px.x / nativeScale, px.y / nativeScale }; }
Rectangle UIManager::PointsToPixels(Rectangle r) const { return { r.x * nativeScale, r.y * nativeScale, r.width * nativeScale, r.height * nativeScale }; }
Rectangle UIManager::PixelsToPoints(Rectangle r) const { return { r.x / nativeScale, r.y / nativeScale, r.width / nativeScale, r.height / nativeScale }; }

Rectangle UIManager::GetSafeArea(bool usePixels) const {
    return usePixels ? safeAreaPixels : safeAreaPoints;
}

float UIManager::GetScaleFactor(bool usePixels) const {
    float refHeight = usePixels ? screenHeightPixels : screenHeightPoints;
    return refHeight / baseHeight;
}

Vector2 UIManager::GetPosition(UIAnchor anchor, Vector2 offset, bool usePixels) const {
    const Rectangle& area = usePixels ? safeAreaPixels : safeAreaPoints;
    Vector2 position = {0, 0};
    switch (anchor)
    {
        case UIAnchor::TOP_LEFT:
            position = { area.x, area.y };
            break;
        case UIAnchor::TOP_CENTER:
            position = { area.x + area.width / 2.0f, area.y };
            break;
        case UIAnchor::TOP_RIGHT:
            position = { area.x + area.width, area.y };
            break;
        case UIAnchor::CENTER_LEFT:
            position = { area.x, area.y + area.height / 2.0f };
            break;
        case UIAnchor::CENTER:
            position = { area.x + area.width / 2.0f, area.y + area.height / 2.0f };
            break;
        case UIAnchor::CENTER_RIGHT:
            position = { area.x + area.width, area.y + area.height / 2.0f };
            break;
        case UIAnchor::BOTTOM_LEFT:
            position = { area.x, area.y + area.height };
            break;
        case UIAnchor::BOTTOM_CENTER:
            position = { area.x + area.width / 2.0f, area.y + area.height };
            break;
        case UIAnchor::BOTTOM_RIGHT:
            position = { area.x + area.width, area.y + area.height };
            break;
    }
    // Apply offset (already in correct units)
    position.x += offset.x;
    position.y += offset.y;
    return position;
} 