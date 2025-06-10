#include "ToiletPaper.h"
#include <raymath.h>
#include "Resources.h"
#include <iostream>

ToiletPaper::ToiletPaper(Vector2 spawnPos, float panspeed, float extra)
    : pos(spawnPos), speed(panspeed), extraSpeed(extra), isHurt(false), hurtTimer(0.0f)
{
    using namespace Resources;
    tpSpriteIdle = new Sprite(ToiletPaperIdle, 8, 0.2f);
    tpSpriteHurt = new Sprite(ToiletPaperHurt, 4, 0.1f);
    currentSprite = tpSpriteIdle;

    startY = 70.0f + GetRandomValue(-10, 10);
    phaseOffset = GetRandomValue(0, 628) / 100.0f;

    idleTimer = 0.0f;
    idleDelay = GetRandomValue(4, 8);
    active = GetRandomValue(0, 100) < 75;

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

    pos.x -= (speed + extraSpeed) * deltaTime;
    float wave = sinf(pos.x * 0.015f + phaseOffset);
    pos.y = startY + 50.0f * (inverted ? -wave : wave);

    if (isHurt) {
        hurtTimer -= deltaTime;
        if (hurtTimer > 0.0f && !tpSpriteHurt->hasLoopedOnce()) {
            tpSpriteHurt->Update(deltaTime);
        }
        if (tpSpriteHurt->hasLoopedOnce() && !tpSpriteHurt->IsFrozen()) {
            tpSpriteHurt->SetFrameFrozen(3); // Freeze on last frame (index 3 for 4 frames)
        }
    }
    else {
        tpSpriteIdle->Update(deltaTime);
    }

    UpdateHitbox();
}

void ToiletPaper::Draw() const
{
    if (isHurt)
        tpSpriteHurt->Draw(pos.x, pos.y);
    else
        tpSpriteIdle->Draw(pos.x, pos.y);
}

Rectangle ToiletPaper::GetHitbox() const
{
    return hitbox; // Always return valid hitbox
}

void ToiletPaper::TakeDamage()
{
    if (!isHurt) {
        isHurt = true;
        hurtTimer = 0.5f;
        tpSpriteHurt->ResetAnimation();
    }
}

bool ToiletPaper::ShouldBeRemoved() const
{
    return isHurt && hurtTimer <= 0.0f;
}

void ToiletPaper::UpdateHitbox()
{
    hitbox = { pos.x + 8, pos.y + 24, (float)currentSprite->GetWidth() - 24, (float)currentSprite->GetHeight() - 36 };
}

void ToiletPaper::SetSpeed(float spd)
{
    speed = spd;
}