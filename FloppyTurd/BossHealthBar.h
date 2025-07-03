#pragma once
#include "RaylibCompat.h"
#include "Boss.h"
#include <memory>

class BossHealthBar
{
public:
    BossHealthBar(std::shared_ptr<Boss> boss, const char* bossName);
    ~BossHealthBar();

    void Draw();
    void Update(float deltaTime);

private:
    const char* _bossName;
    std::weak_ptr<Boss> boss; // Use weak_ptr to avoid dangling pointer
    Vector2 position;
    int originalWidth;  // Original pixel width from images
    int originalHeight; // Original pixel height from images
    Texture2D frameTexture;
    Texture2D healthTexture;
    Texture2D hurtTexture;
    float currentHealthPercent;
    float shadowHealthPercent;  // For the hurt effect
    float hurtFadeTimer;       // Timer for fading hurt effect
    const float HURT_FADE_DURATION = 0.75f;  // Duration for hurt effect fade
};