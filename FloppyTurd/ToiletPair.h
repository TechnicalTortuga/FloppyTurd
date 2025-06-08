#pragma once
#include "raylib.h"
#include "GameSettings.h"
#include <vector>
#include "Obstacle.h"
#include "Resources.h"

class ToiletPair : public Obstacle {
public:
    ToiletPair(int xPos, int yPos, bool isSnowy = false);
    ~ToiletPair();

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

    float toiletScale;
    bool hasScored = false;

private:
    void UpdateHitbox();
    bool isSnowyVariant = false;  // Store flag for possible future behavior tweaks
    Texture2D _TopToilet;
    Texture2D _BottomToilet;

    float yOffset;
    float panSpeed;

    Rectangle hitboxTop;
    Rectangle hitboxBottom;

    float gapBetweenToilets{};
    float maxYOffset{ 60.0f };    // How high or low we allow offset to go

    float oscillationTimer = 0.0f;
    bool isOscillating = false;  // Controlled by SetOscillationEnabled
    bool defaultOscillating = false; // Stores default state (true for snowy)

    float oscillationPhase = 0.0f;
    float oscillationDirection = 1.0f;
    float phaseOffset;
};