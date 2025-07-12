#pragma once

#include "Layer.h"   
#include "Sprite.h"
#include <string>
#include "PlatformAPI.h"

class AnimatedLayer : public Layer {
public:
    AnimatedLayer(const std::string& spriteSheetPath,
        int frameCount,
        float frameTime,
        Vector2 startPosition,
        float scale);

    void Update(float deltaTime) override;
    void Draw() override;

private:
    Sprite sprite; // Now stores frame count, frame time, scale, position, etc.
};