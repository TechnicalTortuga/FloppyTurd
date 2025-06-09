// ToiletPair.h: Header for the ToiletPair obstacle class in Floppy Turd, defining toilet pair behavior and properties.
// Updated to add gapSize constant for consistent pipe spacing.

#pragma once
#include "raylib.h"
#include "GameSettings.h"
#include <vector>
#include "Obstacle.h"
#include "Resources.h"

class ToiletPair : public Obstacle {
public:
    ToiletPair(int xPos, int yPos, bool isSnowy = false);
    ToiletPair(int xPos, int yPos, bool isSnowy, bool creditsMode);
    ~ToiletPair();

    void Draw();
    void Update(float deltaTime);
    void resetScore();
    Rectangle GetTopHitbox();
    Rectangle GetBottomHitbox();
    std::vector<Rectangle> GetHitboxes();
    void yOffsetRandomizer();
    void SetCollisionEnabled(bool enabled) override;
    void SetOscillationEnabled(bool enabled);
    void SetPanSpeed(float speed);
    float GetPanSpeed() const { return panSpeed; }

    float toiletScale;
    bool hasScored = false;

private:
    void UpdateHitbox();
    bool isSnowyVariant = false;
    Texture2D _TopToilet;
    Texture2D _BottomToilet;

    float yOffset;
    float panSpeed;

    Rectangle hitboxTop;
    Rectangle hitboxBottom;

    float gapBetweenToilets{};
    float maxYOffset{ 60.0f };
    const float gapSize{ 120.0f }; // Consistent horizontal gap between pipes

    float oscillationTimer = 0.0f;
    bool isOscillating = false;
    bool defaultOscillating = false;

    float oscillationPhase = 0.0f;
    float oscillationDirection = 1.0f;
    float phaseOffset;
};