#pragma once
#include "Enemy.h"
#include "Sprite.h"

enum class Mode { FLY_IN, HOVER, PULLBACK, BEELINE };

class RatCopter : public Enemy
{
public:
	RatCopter(Vector2 spawnPos, float panspeed);
	~RatCopter();

	void Draw() const;
	void Update(float deltaTime);
	Rectangle GetHitbox() const;

	void TakeDamage() override;
	bool ShouldBeRemoved() const override;
	void SetTarget(Vector2 target);

private:
	void UpdateHitbox();

	Vector2 pos;

	Sprite* currentSprite;
	Sprite* ratSpriteIdle;
	Sprite* ratSpriteHurt;

	Rectangle hitbox;
	float speed; // enemy speed

	bool isHurt;
	float hurtTimer;

	Mode currentMode = Mode::FLY_IN;
	float hoverTimer = 0.0f;
	Vector2 direction = { -1.0f, 0.0f };  // Default left
	bool hasLockedDirection = false;
	
	Vector2 targetPos;

	Vector2 pullbackVector = { 0.0f, 0.0f };
	float pullbackTimer = 0.25f;
};

