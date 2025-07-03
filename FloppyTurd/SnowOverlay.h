#pragma once
#include "RaylibCompat.h"

class SnowOverlay {
public:
    SnowOverlay(Texture2D texture, int frameCount, float frameDuration);
    void Update(float deltaTime);
    void Draw() const;

private:
    Texture2D snowTexture;
    int frameCount;
    float frameDuration;
    float currentTime;
    int currentFrame;

    static constexpr int tileSize = 32;
};
