#pragma once
#include "Sprite.h"
#include <raylib.h>
#include "Enemy.h"

class ToiletPaper : public Enemy
{
public:
    ToiletPaper(Vector2 spawnPos, float panspeed);
    ~ToiletPaper();

    void Update(float deltaTime);
    void Draw() const;

    Rectangle GetHitbox() const;
    void TakeDamage();
    bool ShouldBeRemoved() const;
    void SetInverted(bool val) { inverted = val; }
    void SetSpeed(float speed) override; // New: Set movement speed

private:
    Vector2 pos;
    float speed;
    float startY;
    float phaseOffset;
    bool inverted = false;

    bool isHurt;
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