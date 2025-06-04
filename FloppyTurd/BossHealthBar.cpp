#include "BossHealthBar.h"
#include "Resources.h"
#include "GameSettings.h"
#include <raymath.h>

BossHealthBar::BossHealthBar(Boss* boss, const char* bossName)
	: boss(boss), position(Vector2(80, 140)), originalWidth(160), originalHeight(32),
	currentHealthPercent(1.0f), shadowHealthPercent(1.0f), hurtFadeTimer(0.0f), _bossName(bossName)
{
    using namespace Resources;
    frameTexture = LoadTexture(BossBarFrame);
    healthTexture = LoadTexture(BossBarHealth);
    hurtTexture = LoadTexture(BossBarHurt);
}

BossHealthBar::~BossHealthBar()
{
    UnloadTexture(frameTexture);
    UnloadTexture(healthTexture);
    UnloadTexture(hurtTexture);
}

void BossHealthBar::Draw()
{
    DrawTexture(frameTexture, (int)position.x, (int)position.y, WHITE);

    float healthPercent = (float)boss->health / 200.0f;
    int healthPixelWidth = (int)(originalWidth * healthPercent);

    Color barColor = RED;

    if (boss->IsInLowHealthMode())
    {
        float pulse = 0.5f + 0.5f * sinf(GetTime() * 6.0f);  // oscillates 0 to 1
        barColor = ColorLerp(MAROON, RED, pulse);
        barColor.a = 180 + (unsigned char)(pulse * 75);  // range 180–255 alpha
    }

    if (hurtFadeTimer > 0.0f && shadowHealthPercent > currentHealthPercent)
    {
        int fullShadowWidth = (int)(originalWidth * shadowHealthPercent);
        int currentWidth = (int)(originalWidth * currentHealthPercent);
        int hurtPixelWidth = fullShadowWidth - currentWidth;

        if (hurtPixelWidth > 0)
        {
            float alphaPulse = Clamp((hurtFadeTimer / HURT_FADE_DURATION), 0.0f, 1.0f);
            Color hurtColor = { 255, 255, 255, (unsigned char)(alphaPulse * 255) };

            DrawTextureRec(
                hurtTexture,
                { 0, 0, (float)fullShadowWidth, (float)originalHeight },
                { position.x, position.y },
                hurtColor
            );
        }
    }

    if (healthPixelWidth > 0)
    {
        DrawTextureRec(healthTexture,
            { 0, 0, (float)healthPixelWidth, (float)originalHeight },
            { position.x, position.y },
            barColor);
    }

    // Draw name label under the health bar
    DrawText(_bossName, (int)position.x + 10, (int)position.y + 25, 10, GOLD);
}

void BossHealthBar::Update(float deltaTime)
{
    if (boss)
    {
        float newHealthPercent = (float)boss->health / 200.0f;

        // Trigger hurt animation only if boss took damage
        if (newHealthPercent < currentHealthPercent)
        {
            shadowHealthPercent = currentHealthPercent;  // Store old health as the shadow
            hurtFadeTimer = HURT_FADE_DURATION;          // Reset fade
        }

        currentHealthPercent = newHealthPercent;

        if (hurtFadeTimer > 0.0f)
        {
            hurtFadeTimer -= deltaTime;
            if (hurtFadeTimer <= 0.0f)
            {
                hurtFadeTimer = 0.0f;
                shadowHealthPercent = currentHealthPercent;  // Sync them once done
            }
        }
    }
}