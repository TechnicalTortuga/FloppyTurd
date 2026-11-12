#include "GameSettings.h"
#include "SnowOverlay.h"

SnowOverlay::SnowOverlay(Texture2D texture, int frameCount, float frameDuration)
    : snowTexture(texture), frameCount(frameCount), frameDuration(frameDuration),
    currentTime(0.0f), currentFrame(0) {}

void SnowOverlay::Update(float deltaTime) {
    currentTime += deltaTime;
    if (currentTime >= frameDuration) {
        currentTime = 0.0f;
        currentFrame = (currentFrame + 1) % frameCount;
    }
}

void SnowOverlay::Draw() const {
    const int screenWidth = (int)GameSettings::GameWidth;
    const int screenHeight = (int)GameSettings::GameHeight;

    Rectangle source = {
        static_cast<float>(currentFrame * tileSize),
        0,
        static_cast<float>(tileSize),
        static_cast<float>(tileSize)
    };

    for (int y = 0; y < screenHeight; y += tileSize) {
        for (int x = 0; x < screenWidth; x += tileSize) {
            DrawTextureRec(snowTexture, source, Vector2{ (float)x, (float)y }, WHITE);
        }
    }
}
