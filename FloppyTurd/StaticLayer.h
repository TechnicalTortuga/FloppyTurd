// StaticLayer.h
#pragma once

#include "Layer.h"
#include "Sprite.h"
#include <string>
#include "PlatformAPI.h"

class StaticLayer : public Layer {
public:
    // If you have only a single-frame sprite, just pass frameCount=1
    StaticLayer(const std::string& filePath, Vector2 position, float scale);
    void Update(float deltaTime) override;
    void Draw() override;

private:
    Sprite sprite;
};
