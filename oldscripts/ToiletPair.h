#pragma once
#include "raylib.h"
#include "GameSettings.h"
#include <vector>
#include "Obstacle.h"
#include "Resources.h"

class ToiletPair : public Obstacle {
public:
    ToiletPair(int xPos, int yPos);
    ~ToiletPair();

    void Draw();
    void Update(float deltaTime);
    void resetScore();
    Rectangle GetTopHitbox();
    Rectangle GetBottomHitbox();
    std::vector<Rectangle> GetHitboxes();
    void yOffsetRandomizer();

    Vector2 pos;
    float toiletScale;
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
    float maxYOffset{60.0f};    // <--- how high or low we allow offset to go
};