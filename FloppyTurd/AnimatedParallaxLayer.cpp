#include "AnimatedParallaxLayer.h"
#include "GameSettings.h"

AnimatedParallaxLayer::AnimatedParallaxLayer(const std::string& spriteSheetPath,
    int frameCount,
    float frameTime,
    float speed,
    float scale)
    : sprite(spriteSheetPath, frameCount, frameTime, scale)
    , speed(speed)
    , scale(scale)
{
    // We loaded the sprite’s texture in the constructor above
    spriteWidth = sprite.GetScaledWidth();
    spriteHeight = sprite.GetScaledHeight();

    // Example: fill the entire screen width plus a couple extra segments
    float screenWidth = GameSettings::GameWidth;
    numSegments = static_cast<int>(screenWidth / spriteWidth) + 2;

    positionsX.resize(numSegments);
    for (int i = 0; i < numSegments; i++) {
        positionsX[i] = i * spriteWidth;
    }
}

AnimatedParallaxLayer::~AnimatedParallaxLayer()
{
}

void AnimatedParallaxLayer::Update(float deltaTime) {
    // 1) Advance animation (so frames move)
    sprite.Update(deltaTime);

    // 2) Move segments horizontally
    for (int i = 0; i < numSegments; i++) {
        positionsX[i] -= speed * deltaTime;
        if (positionsX[i] <= -spriteWidth) {
            positionsX[i] += spriteWidth * numSegments;
        }
    }
}

void AnimatedParallaxLayer::Draw() {
    // Draw each horizontal segment
    for (int i = 0; i < numSegments; i++) {
        // Move the sprite to that segment’s X position
        sprite.SetPosition(positionsX[i], 0.0f);

        // Then draw it
        sprite.Draw();
    }
}