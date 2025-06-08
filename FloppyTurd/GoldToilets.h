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
    // New method to toggle oscillation for swinging pipes
    void SetOscillationEnabled(bool enabled);

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
    float maxYOffset{ 60.0f };    // How high or low we allow offset to go

    float oscillationTimer = 0.0f;
    bool isOscillating = true;   // Controlled by SetOscillationEnabled
    bool defaultOscillating = true; // GoldToilets oscillate by default
    float oscillationPhase = 0.0f;
    float oscillationDirection = 1.0f;
    float phaseOffset = 0.0f;
};