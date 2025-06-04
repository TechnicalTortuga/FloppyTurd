#include "Bird.h"
#include "Resources.h"
#include <cmath>

Bird::Bird(Vector2 startPos, float spd)
    : pos(startPos),
    speed(spd),
    baseY(startPos.y),
    hoverTimer(0.0f)
{
    using namespace Resources;
    // Fly loop has, say, 4 frames (0→3) at 0.1f anim speed
    flySprite = new Sprite(Resources::BirdIdle, 4, 0.1f, 1.0f, pos);
    hurtSprite = new Sprite(Resources::BirdHurt, 4, 0.1f, 1.0f, pos);

    currentSprite = flySprite;
    // Initial hitbox matches a 24×24 region (offset by 4 px in each direction)
    hitbox = { pos.x + 4, pos.y + 4, 24, 24 };
}

Bird::~Bird() {
    delete flySprite;
    delete hurtSprite;
}

void Bird::Update(float deltaTime) {
    // 1) Move left at constant speed
    pos.x -= speed * deltaTime;

    // 2) Hover effect (vertical sine wave around baseY)
    hoverTimer += deltaTime;
    pos.y = baseY + std::sinf(hoverTimer * 2.0f) * 4.0f;

    // 3) Position whichever sprite we’re drawing now
    currentSprite->SetPosition(pos);

    // 4) If just hurt, advance its hurt animation until it finishes exactly 4 frames
    if (isHurt) {
        hurtTimer -= deltaTime;

        if (!hurtAnimFinished) {
            // Advance the hurt sprite frame-by-frame:
            currentSprite->Update(deltaTime);

            // Check if timer ≤ 0 && we’re at frame index 3 (last of 4 frames):
            if (hurtTimer <= 0.0f && currentSprite->GetFrameIndex() >= 3) {
                hurtAnimFinished = true;

                // Freeze on frame 3 so it never loops back to 0
                currentSprite->SetFrameFrozen(3);

                // Hitbox is already zeroed (isHurt == true means UpdateHitbox will keep it at zero)
            }
        }
        // Once hurtAnimFinished is true, skip currentSprite->Update() so the frame remains frozen
    }
    else {
        // 5) Normal (not hurt) animation:
        currentSprite->Update(deltaTime);
    }

    // 6) Update hitbox each frame: as soon as isHurt is true, hitbox = {0,0,0,0}
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

        // Immediately drop hitbox so no more collisions:
        hitbox = { 0, 0, 0, 0 };

        // Switch to hurt sprite, reset it, start at frame 0:
        currentSprite = hurtSprite;
        hurtSprite->ResetAnimation();
        hurtSprite->SetFrameIndex(0);

        // Give it just enough time to play 4 frames (0.1f speed × 4 frames = 0.4 seconds)
        hurtTimer = 0.4f;
    }
}

bool Bird::ShouldBeRemoved() const {
    return hurtAnimFinished;
}

void Bird::UpdateHitbox() {
    if (isHurt) {
        // Immediately keep it disabled
        hitbox = { 0, 0, 0, 0 };
        return;
    }
    // Only when not hurt do we give it its normal flying hitbox
    hitbox = { pos.x + 4, pos.y + 10, 24, 8 };
}
