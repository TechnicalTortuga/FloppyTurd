#include "StaticLayer.h"

StaticLayer::StaticLayer(const std::string& filePath, Vector2 position, float scale)
    : sprite(filePath, /*frameCount=*/1, /*frameTime=*/0.0f, scale, position, AtlasCategory::ENVIRONMENT)
{
    // This sprite has only 1 frame, so it's effectively a static image.
}

void StaticLayer::Update(float deltaTime) {
    // No need to animate or scroll
    (void)deltaTime;
}

void StaticLayer::Draw() {
    sprite.Draw();
}