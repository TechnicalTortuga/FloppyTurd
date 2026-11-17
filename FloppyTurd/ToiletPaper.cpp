#include "ToiletPaper.h"
#include <raymath.h>
#include "ResourceCompat.h"
#include <iostream>
#include "GameSfx.h"
#include "GameSettings.h"
#include "EnemyEntry.h"

ToiletPaper::ToiletPaper(Vector2 spawnPos, float panspeed, float extra)
    : pos(spawnPos), speed(panspeed), extraSpeed(extra), isHurt(false), hurtTimer(0.0f)
{
    using namespace Resources;
    pos.x=EnemyEntry::OffscreenX(GameSettings::GameWidth,pos.x);
    tpSpriteIdle = new Sprite(ToiletPaperIdle, 8, 0.2f);
    tpSpriteHurt = new Sprite(ToiletPaperHurt, 4, 0.1f);
    currentSprite = tpSpriteIdle;

    startY = spawnPos.y;
    phaseOffset = GetRandomValue(0, 628) / 100.0f;

    UpdateHitbox();
}

ToiletPaper::~ToiletPaper()
{
    delete tpSpriteIdle;
    delete tpSpriteHurt;
}

void ToiletPaper::Update(float deltaTime)
{
    // Defeated rolls still drift and finish their animation before removal.
    if (isHurt) {
        hurtTimer -= deltaTime;
        tpSpriteHurt->Update(deltaTime);
        if (tpSpriteHurt->hasLoopedOnce() && !tpSpriteHurt->IsFrozen()) tpSpriteHurt->SetFrameFrozen(3);
        pos.x -= speed*deltaTime;
        UpdateHitbox();
        return;
    }
    pos=EnemyEntry::AdvanceRoll(pos,startY,waveDistance,speed+extraSpeed,phaseOffset,inverted,deltaTime);

    tpSpriteIdle->Update(deltaTime);

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
        GameSfx::Play("tpkill");
        hurtTimer = 0.5f;
        tpSpriteHurt->ResetAnimation();
        UpdateHitbox();
    }
}

bool ToiletPaper::ShouldBeRemoved() const
{
    return isHurt && hurtTimer <= 0.0f;
}

void ToiletPaper::UpdateHitbox() {
	if (isHurt) {
		hitbox = { 0, 0, 0, 0 }; // Disable hitbox when hurt
	}
	else {
		hitbox = { pos.x + 8, pos.y + 24, (float)currentSprite->GetWidth() - 24, (float)currentSprite->GetHeight() - 36 };
	}
}

void ToiletPaper::SetSpeed(float spd)
{
    speed = spd;
}
