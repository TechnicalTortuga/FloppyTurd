#include "ToiletPaper.h"
#include <raymath.h>
#include "Resources.h"
#include <iostream>
#include <raylib.h>

ToiletPaper::ToiletPaper(Vector2 spawnPos, float panspeed)
    : pos(spawnPos), speed(panspeed), isHurt(false), hurtTimer(0.0f)
{
    using namespace Resources;

    tpSpriteIdle = new Sprite(ToiletPaperIdle, 8, 0.2f);
    tpSpriteHurt = new Sprite(ToiletPaperHurt, 2, 0.08f);
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
    if (isHurt) {
        hitbox = { 0, 0, 0, 0 };
        return;
    }
    
    float spriteWidth = (float)currentSprite->GetWidth();
    float spriteHeight = (float)currentSprite->GetHeight();
    float hitboxWidth = 12.0f;
    float hitboxHeight = 8.0f;
    
    // Center the hitbox on the sprite
    float xOffset = (spriteWidth - hitboxWidth) / 2.0f;
    float yOffset = (spriteHeight - hitboxHeight) / 2.0f;
    
    hitbox = { pos.x + xOffset, pos.y + yOffset, hitboxWidth, hitboxHeight };
    
    // Debug logging (log once every 60 frames to avoid spam)
    static int logCounter = 0;
    if (logCounter++ % 60 == 0) {
        TraceLog(LOG_INFO, "[ToiletPaper] Sprite: %.0fx%.0f, Hitbox: %.1f,%.1f,%.1fx%.1f (offset: %.1f,%.1f)", 
            spriteWidth, spriteHeight, 
            hitbox.x, hitbox.y, hitbox.width, hitbox.height,
            xOffset, yOffset);
    }
}

void ToiletPaper::SetSpeed(float spd)
{
    speed = spd;
}