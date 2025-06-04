#pragma once
#include "raylib.h"
#include "Boss.h"

class BossHealthBar
{
public:
    BossHealthBar(Boss* boss, const char* bossName);
    ~BossHealthBar();

    void Draw();
    void Update(float deltaTime);

private:
    const char* _bossName;
    Boss* boss;
    Vector2 position;
    int originalWidth;  // Original pixel width from images
    int originalHeight; // Original pixel height from images
    Texture2D frameTexture;
    Texture2D healthTexture;
    Texture2D hurtTexture;
    float currentHealthPercent;
    float shadowHealthPercent;  // For the hurt effect
    float hurtFadeTimer;       // Timer for fading hurt effect
    const float HURT_FADE_DURATION = 0.75f;  // Increased to 1 second for slower fade
};