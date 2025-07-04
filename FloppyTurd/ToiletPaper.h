#pragma once
#include "Sprite.h"
#include "RaylibCompat.h"
#include "Enemy.h"

class ToiletPaper : public Enemy
{
public:
    ToiletPaper(Vector2 spawnPos, float panspeed, float extra = 20.0f);
    ~ToiletPaper();

    void Update(float deltaTime) override;
    void Draw() const override;

    Rectangle GetHitbox() const override;
    void TakeDamage() override;
    bool ShouldBeRemoved() const override;
    void SetInverted(bool val) { inverted = val; }
    void SetSpeed(float speed) override;

private:
    Vector2 pos;
    float speed;
    float extraSpeed;
    float startY;
    float phaseOffset;
    bool inverted = false;

    bool isHurt;
    bool removeSoon = false;
    float hurtTimer;

    Sprite* tpSpriteIdle;
    Sprite* tpSpriteHurt;
    Sprite* currentSprite;

    Rectangle hitbox;

    void UpdateHitbox();

    bool active;
    float idleTimer;
    float idleDelay;
};
