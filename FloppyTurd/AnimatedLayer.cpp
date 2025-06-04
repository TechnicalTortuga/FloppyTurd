#include "AnimatedLayer.h"

AnimatedLayer::AnimatedLayer(const std::string& spriteSheetPath,
    int frameCount,
    float frameTime,
    Vector2 startPosition,
    float scale)
    : sprite(spriteSheetPath,
        frameCount,    // how many frames in one row
        frameTime,
        scale,
        startPosition) // let the sprite hold the position
{
    // Nothing else needed here, because the sprite now "knows" its position & scale
}

void AnimatedLayer::Update(float deltaTime) {
    sprite.Update(deltaTime);
}

void AnimatedLayer::Draw() {
    sprite.Draw(); // No need to manually pass position or scale
}