#include "ToiletPaper.h"
#include <raymath.h>
#include "Resources.h"
#include <iostream>

ToiletPaper::ToiletPaper(Vector2 spawnPos, float panspeed)
    : pos(spawnPos), speed(panspeed), isHurt(false), hurtTimer(0.0f)
{
    using namespace Resources;

    tpSpriteIdle = new Sprite(ToiletPaperIdle, 8, 0.2f);
    tpSpriteHurt = new Sprite(ToiletPaperHurt, 2, 0.08f);
    currentSprite = tpSpriteIdle;

    startY = 70.0f + GetRandomValue(-10, 10);
    phaseOffset = GetRandomValue(0, 628) / 100.0f; // random phase 0 to ~2pi

    idleTimer = 0.0f;
    idleDelay = GetRandomValue(4, 8);
    active = GetRandomValue(0, 100) < 75; // 75% chance to be active

    UpdateHitbox();
}

ToiletPaper::~ToiletPaper()
{
    delete tpSpriteIdle;
    delete tpSpriteHurt;
}

void ToiletPaper::Update(float deltaTime)
{
    if (!active) {
        idleTimer += deltaTime;
        if (idleTimer >= idleDelay) {
            active = true;
            idleTimer = 0.0f;
            idleDelay = GetRandomValue(4, 8);
        }
        return;
    }

    pos.x -= speed * deltaTime;
    float wave = sinf(pos.x * 0.015f + phaseOffset);
    pos.y = startY + 50.0f * (inverted ? -wave : wave);

    if (isHurt)
    {
        hurtTimer -= deltaTime;
        if (hurtTimer <= 0.0f)
        {
            isHurt = false;
            currentSprite = tpSpriteIdle;
        }
    }

    currentSprite->Update(deltaTime);
    UpdateHitbox();
}

void ToiletPaper::Draw() const
{
    currentSprite->Draw(pos.x, pos.y);
}

Rectangle ToiletPaper::GetHitbox() const
{
    return hitbox;
}

void ToiletPaper::TakeDamage()
{
    isHurt = true;
    hurtTimer = 0.3f;
    currentSprite = tpSpriteHurt;
}

bool ToiletPaper::ShouldBeRemoved() const
{
    return pos.x + currentSprite->GetWidth() < 0;
}

void ToiletPaper::UpdateHitbox()
{
    hitbox = { pos.x + 8, pos.y + 24, (float)currentSprite->GetWidth() - 24, (float)currentSprite->GetHeight() - 36};
}
