#include "RatCopter.h"
#include "ResourceCompat.h"
#include <iostream>
#include <raymath.h>
#include "LevelManager.h"

RatCopter::RatCopter(Vector2 spawnPos, float panspeed) {
    using namespace Resources;
    pos.x = spawnPos.x;
    pos.y = spawnPos.y;
    speed = panspeed;

    ratSpriteIdle = new Sprite(RatCopterIdle, 6, 0.1f, 1.0f, Vector2(pos.x, pos.y));
    ratSpriteHurt = new Sprite(RatCopterHurt, 6, 0.1f, 1.0f, Vector2(pos.x, pos.y));

    currentSprite = ratSpriteIdle;
    isHurt = false;
    hurtTimer = 0.0f;

    currentMode = Mode::FLY_IN;
    hoverTimer = 0.0f;
    direction = { -1.0f, 0.0f };
    hasLockedDirection = false;
    pullbackVector = { 0.0f, 0.0f };
    pullbackTimer = 0.25f;

    UpdateHitbox();
}

RatCopter::~RatCopter()
{
    delete ratSpriteIdle;
    delete ratSpriteHurt;
}

void RatCopter::Draw() const
{
    currentSprite->Draw(pos.x, pos.y);
}

void RatCopter::Update(float deltaTime)
{
    switch (currentMode)
    {
    case Mode::FLY_IN:
        pos.x -= speed * deltaTime;
        if (pos.x <= 240.0f)
        {
            currentMode = Mode::HOVER;
            hoverTimer = 0.75f + GetRandomValue(0, 25) / 100.0f;
        }
        break;

    case Mode::HOVER:
        hoverTimer -= deltaTime;
        if (hoverTimer <= 0.0f)
        {
            currentMode = Mode::PULLBACK;
            pullbackTimer = 0.25f;
            Vector2 liveTarget = LevelManager::GetInstance()->GetPlayerPosition();
            direction = Vector2Normalize(Vector2Subtract(liveTarget, pos));
            pullbackVector = Vector2Scale(direction, -20.0f);
            hasLockedDirection = true;
        }
        break;

    case Mode::PULLBACK:
        pos = Vector2Add(pos, Vector2Scale(pullbackVector, deltaTime * 4.0f));
        pullbackTimer -= deltaTime;
        if (pullbackTimer <= 0.0f)
        {
            currentMode = Mode::BEELINE;
            // Use a fixed speed for BEELINE, scaled by difficulty
            float baseBeelineSpeed = 150.0f;
            speed = baseBeelineSpeed * (speed / 80.0f); // Scale based on difficulty speed
        }
        break;

    case Mode::BEELINE:
        pos = Vector2Add(pos, Vector2Scale(direction, speed * deltaTime));
        if (pos.x < -32.0f || pos.x > 320.0f + 32.0f || pos.y < -32.0f || pos.y > 180.0f + 32.0f)
        {
            isHurt = true;
            hurtTimer = 0.0f;
        }
        break;
    }

    if (isHurt) {
        if (currentSprite != ratSpriteHurt)
            currentSprite = ratSpriteHurt;

        hurtTimer -= deltaTime;
        if (hurtTimer > 0.0f)
            currentSprite->Update(deltaTime);
    }
    else {
        currentSprite->Update(deltaTime);
    }
    UpdateHitbox();
}

Rectangle RatCopter::GetHitbox() const
{
    return hitbox;
}

void RatCopter::UpdateHitbox() {
    int xOffset = pos.x + 5;
    int yOffset = pos.y + 7;
    hitbox.x = xOffset;
    hitbox.y = yOffset;
    hitbox.width = 23;
    hitbox.height = 21;
}

void RatCopter::TakeDamage() {
    if (!isHurt) {
        isHurt = true;
        hurtTimer = 0.5f;
        currentSprite = ratSpriteHurt;
    }
}

bool RatCopter::ShouldBeRemoved() const {
    return isHurt && (hurtTimer <= 0.0f || currentSprite->hasLoopedOnce());
}

void RatCopter::SetTarget(Vector2 target)
{
    targetPos = target;
}

void RatCopter::SetSpeed(float spd)
{
    speed = spd;
}