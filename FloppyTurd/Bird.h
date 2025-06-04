#pragma once
#include "Enemy.h"
#include "Sprite.h"

class Bird : public Enemy {
public:
    Bird(Vector2 startPos, float speed);
    ~Bird();
    void Update(float deltaTime) override;
    void Draw()   const override;
    void TakeDamage() override;
    Rectangle GetHitbox() const override;
    bool ShouldBeRemoved() const override;

private:
    void UpdateHitbox();

    Vector2 pos;
    float baseY;         // original Y around which we hover
    float hoverTimer;    // timer for sine wave hover
    float speed;         // horizontal speed

    Sprite* currentSprite;
    Sprite* flySprite;   // normal flying loop
    Sprite* hurtSprite;  // 4-frame hurt animation

    Rectangle hitbox;
    bool isHurt = false;
    bool hurtAnimFinished = false;
    float hurtTimer = 0.0f;  // total duration for hurt animation
};