#include "Bird.h"
#include "ResourceCompat.h"
#include <cmath>
#include "GameSfx.h"

Bird::Bird(Vector2 startPos, float spd)
    : pos(startPos), speed(spd), baseY(startPos.y), hoverTimer(0.0f)
{
    using namespace Resources;
    flySprite = new Sprite(BirdIdle, 4, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
    hurtSprite = new Sprite(BirdHurt, 4, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);

    currentSprite = flySprite;
    // iOS Bird config: 70% hover probability, slight frequency variation,
    // and 15-30 native-pixel amplitude normalized from sprite scale eight.
    hoverSpeed=1.2f+GetRandomValue(-5,5)/100.0f;
    hoverAmplitude=GetRandomValue(0,99)<70 ? GetRandomValue(150,300)/80.0f : 0;
    hoverPhase=GetRandomValue(0,628)/100.0f;
    hitbox = { pos.x + 4, pos.y + 4, 24, 24 };
}

Bird::~Bird() {
    delete flySprite;
    delete hurtSprite;
}

void Bird::Update(float deltaTime) {
	pos.x -= speed * deltaTime;
	hoverTimer += deltaTime;
	if (!isHurt) pos.y = baseY + std::sin(hoverTimer*hoverSpeed+hoverPhase)*hoverAmplitude;

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
        GameSfx::Play("birdkill");
		currentSprite = hurtSprite;
		hurtSprite->ResetAnimation();
		hurtTimer = 0.4f; // Duration of hurt animation
		hurtAnimFinished = false;
		UpdateHitbox();
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
