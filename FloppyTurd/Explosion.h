#pragma once
#include "raylib.h"
#include "Sprite.h"
#include <memory>

// Explosion class to manage visual effects for BlastSmall and BlastBig
class Explosion
{
public:
    // Constructor: Initialize with texture path, position, and scale
    Explosion(const char* texturePath, Vector2 position, float scale = 1.0f)
        : sprite(std::make_shared<Sprite>(texturePath, 8, 0.3f, scale, position, AtlasCategory::PARTICLES)), position(position), scale(scale) {}
    ~Explosion();

    // Update explosion animation
    void Update(float deltaTime);

    // Draw explosion sprite
    void Draw() const;

    // Check if animation has completed
    bool IsComplete() const { return sprite->hasLoopedOnce(); }

private:
    std::shared_ptr<Sprite> sprite; // Sprite for animation
    Vector2 position;               // Explosion position
    float scale;                    // Scale factor
};