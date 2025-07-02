#include "Bird.h"
#include "ResourceCompat.h"
#include <cmath>

Bird::Bird(Vector2 startPos, float spd)
    : pos(startPos), speed(spd), baseY(startPos.y), hoverTimer(0.0f)
{
    using namespace Resources;
    flySprite = new Sprite(BirdIdle, 4, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
    hurtSprite = new Sprite(BirdHurt, 4, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);

    currentSprite = flySprite;
    hitbox = { pos.x + 4, pos.y + 4, 24, 24 };
}

Bird::~Bird() {
    delete flySprite;
    delete hurtSprite;
}

void Bird::Update(float deltaTime) {
	pos.x -= speed * deltaTime;
	hoverTimer += deltaTime;
	pos.y = baseY + std::sinf(hoverTimer * 2.0f) * 4.0f;

	currentSprite->SetPosition(pos);

	if (isHurt) {
		hurtTimer -= deltaTime;
		currentSprite->Update(deltaTime);
		if (currentSprite->hasLoopedOnce() && !currentSprite->IsFrozen()) {
			currentSprite->SetFrameFrozen(3); // Freeze on last frame
			hurtAnimFinished = true;
		}
	}
	else {
		currentSprite->Update(deltaTime);
	}

	UpdateHitbox();
}

void Bird::Draw() const {
    currentSprite->Draw(pos.x, pos.y);
}

Rectangle Bird::GetHitbox() const {
    return hitbox;
}

void Bird::TakeDamage() {
	if (!isHurt) {
		isHurt = true;
		currentSprite = hurtSprite;
		hurtSprite->ResetAnimation();
		hurtTimer = 0.4f; // Duration of hurt animation
		hurtAnimFinished = false;
	}
}

bool Bird::ShouldBeRemoved() const {
	return isHurt && hurtAnimFinished;
}

void Bird::UpdateHitbox() {
    if (isHurt) {
        hitbox = { 0, 0, 0, 0 };
        return;
    }
    hitbox = { pos.x + 4, pos.y + 10, 24, 8 };
}

void Bird::SetSpeed(float spd) {
    speed = spd;
}