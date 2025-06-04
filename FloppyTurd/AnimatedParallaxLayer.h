#pragma once
#include "raylib.h"
#include "Layer.h"
#include "Sprite.h"
#include <vector>
#include <string>

class AnimatedParallaxLayer : public Layer {
public:
    AnimatedParallaxLayer(
        const std::string& spriteSheetPath,
        int frameCount,
        float frameTime,
        float speed,
        float scale
    );
    ~AnimatedParallaxLayer();

    void Update(float deltaTime) override;
    void Draw() override;

private:
    // A single sprite to display, but repeated horizontally
    Sprite sprite;

    float speed;
    float scale;

    std::vector<float> positionsX;
    int numSegments;

    float spriteWidth;
    float spriteHeight;
};