#pragma once
#include "Obstacle.h"
#include "GameSettings.h"
#include "ResourceCompat.h"
#include <vector>
#include "PlatformAPI.h"

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
    void SetCollisionEnabled(bool enabled) override;
    void SetPanSpeed(float speed); // New: Set pan speed

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