#pragma once
#include "Obstacle.h"
#include "GameSettings.h"
#include "Resources.h"
#include <vector>
#include "raylib.h"

class Outhouse : public Obstacle
{
public:
    Outhouse(int xPos, int yPos);
    ~Outhouse();

    void Draw();
    void ResetScore();
    void Update(float deltaTime);
    Rectangle GetOuthouseHitbox();
    Rectangle GetOuthouseToiletHitbox();
    std::vector<Rectangle> GetHitboxes();
    // Implement the new method from Obstacle
    void SetCollisionEnabled(bool enabled) override;

    bool hasScored = false;

    float outhouseWidthScaled;
    float outhouseHeightScaled;

private:
    Texture2D _Outhouse;
    Texture2D _OuthouseToilet;

    Rectangle _outhouseHitbox;
    Rectangle _outhouseToiletHitbox;

    float yOffset;
    float panSpeed;
    float objectScale;

    void UpdateHitbox();
};