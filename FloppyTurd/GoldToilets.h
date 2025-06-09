#pragma once
#include "raylib.h"
#include "GameSettings.h"
#include <vector>
#include "Obstacle.h"
#include "Resources.h"

class GoldToilets : public Obstacle {
public:
    GoldToilets(int xPos, int yPos);
    ~GoldToilets();

    void Draw();
    void Update(float deltaTime);
    void resetScore();
    Rectangle GetTopHitbox();
    Rectangle GetBottomHitbox();
    std::vector<Rectangle> GetHitboxes();
    void yOffsetRandomizer();
    void SetCollisionEnabled(bool enabled) override;
    void SetOscillationEnabled(bool enabled);
    void SetPanSpeed(float speed); // New: Set pan speed

    bool hasScored = false;

private:
    void UpdateHitbox();

    Texture2D _TopToilet;
    Texture2D _BottomToilet;

    float yOffset;
    float panSpeed;

    Rectangle hitboxTop;
    Rectangle hitboxBottom;

    float gapBetweenToilets{};
    float maxYOffset{ 60.0f };

    float oscillationTimer = 0.0f;
    bool isOscillating = true;
    bool defaultOscillating = true;
    float oscillationPhase = 0.0f;
    float oscillationDirection = 1.0f;
    float phaseOffset = 0.0f;
};