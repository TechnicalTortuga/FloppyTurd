#pragma once
#include "Enemy.h"
#include "Sprite.h"

enum class Mode { FLY_IN, HOVER, PULLBACK, BEELINE };

class RatCopter : public Enemy
{
public:
    RatCopter(Vector2 spawnPos, float panspeed);
    ~RatCopter();

    void Draw() const override;
    void Update(float deltaTime) override;
    Rectangle GetHitbox() const override;

    void TakeDamage() override;
    bool ShouldBeRemoved() const override;
    void SetTarget(Vector2 target);
    void SetSpeed(float speed) override; // New: Set movement speed

private:
    void UpdateHitbox();

    Vector2 pos;

    Sprite* currentSprite;
    Sprite* ratSpriteIdle;
    Sprite* ratSpriteHurt;

    Rectangle hitbox;
    float speed;

    bool isHurt;
    float hurtTimer;

    Mode currentMode = Mode::FLY_IN;
    float hoverTimer = 0.0f;
    Vector2 direction = { -1.0f, 0.0f };
    bool hasLockedDirection = false;

    Vector2 targetPos;

    Vector2 pullbackVector = { 0.0f, 0.0f };
    float pullbackTimer = 0.25f;
};