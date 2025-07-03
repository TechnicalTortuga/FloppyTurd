#include <algorithm>
#include "SnowmanEnemy.h"
#include "ResourceCompat.h"
#include "SnowballProjectile.h"
#include <iostream>
#include <iomanip>

#include "LevelManager.h"

SnowmanEnemy::SnowmanEnemy(Vector2 spawnPos, SnowmanType type)
    : pos(spawnPos), type(type), isHurt(false), hurtTimer(0.0f), hasThrown(false), isThrowing(false), speed(80.0f) {
    using namespace Resources;

    switch (type) {
    case SnowmanType::sRed:
        idleSprite = new Sprite("resources/enemies/SnowManIdle.png", 1, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        throwSprite = new Sprite("resources/enemies/SnowManThrow.png", 6, 0.12f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        currentSprite = idleSprite;
        break;
    case SnowmanType::sGREEN:
        idleSprite = new Sprite("resources/enemies/SnowManGreen.png", 1, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        throwSprite = new Sprite("resources/enemies/SnowManThrow.png", 6, 0.12f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        currentSprite = idleSprite;
        break;
    case SnowmanType::sBLUE:
        idleSprite = new Sprite("resources/enemies/SnowManChill.png", 1, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        throwSprite = new Sprite("resources/enemies/SnowManThrow.png", 6, 0.12f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        currentSprite = idleSprite;
        break;
    case SnowmanType::sCHAD:
        idleSprite = new Sprite("resources/enemies/SnowManChad.png", 1, 0.1f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        throwSprite = new Sprite("resources/enemies/SnowManThrow.png", 6, 0.12f, 1.0f, pos, AtlasCategory::ENEMY_SPRITES);
        currentSprite = idleSprite;
        break;
    }
    UpdateHitbox();
    // Default cooldown for the hardest difficulty
    throwCooldown = 3.2f;
    // Initial delay before the first throw
    throwTimer = 1.5f;
}

SnowmanEnemy::SnowmanEnemy(Vector2 spawnPos, SnowmanType type, float speed, int difficulty)
    : SnowmanEnemy(spawnPos, type) { // Delegate to base constructor
    SetSpeed(speed);
    // Adjust throwCooldown based on difficulty using the original formula
    throwCooldown = 1.0f + (difficulty == 0 ? 1.0f : (difficulty == 1 ? 0.5f : 0.0f));
}

SnowmanEnemy::~SnowmanEnemy() {
    delete idleSprite;
    delete throwSprite;
}

void SnowmanEnemy::TryThrowSnowball()
{
    // — only throw once per animation cycle, at frame 4 —
    if (!hasThrown
        && currentSprite == throwSprite
        && throwSprite->GetFrameIndex() == 4)
    {
        Vector2 playerPos = LevelManager::GetInstance()->GetPlayerPosition();
        Rectangle hb = GetHitbox();
        Vector2 start = { hb.x + 4, hb.y + 6 };
        Vector2 delta = Vector2Subtract(playerPos, start);
        float   dist = Vector2Length(delta);
        if (dist < 0.01f) return;

        // skip overly-vertical throws (made more permissive)
        if (fabsf(delta.x) < fabsf(delta.y) * 0.1f)
            return;

        // normalize and fire
        Vector2 vel = Vector2Scale(Vector2Normalize(delta), 220.0f);
        SnowballProjectile* sb = GetInactiveSnowball();
        sb->Activate(start, vel);

        hasThrown = true;
        //TraceLog(LOG_INFO, "[Snowman] Threw snowball from (%.1f, %.1f) toward player at (%.1f, %.1f)", start.x, start.y, playerPos.x, playerPos.y);
    }
}

void SnowmanEnemy::Update(float deltaTime) {
    if (throwTimer > 0.0f)
        throwTimer -= deltaTime;

    Vector2 playerPos = LevelManager::GetInstance()->GetPlayerPosition();
    isFlipped = (playerPos.x > pos.x && type == SnowmanType::sRed);

    pos.x -= speed * deltaTime;

    if (type == SnowmanType::sRed) {
        // Always update both sprites' positions
        idleSprite->SetPosition(pos);
        throwSprite->SetPosition(pos);

        // Begin throw when on-screen AND the cooldown is over
        if (!isThrowing && throwTimer <= 0.0f && pos.x + 48 <= 320) {
            throwSprite->ResetAnimation();
            isThrowing = true;
            hasThrown = false;
        }

        // Flip around if to the left of player and hasn't already flipped
        if (!hasFlipped && !queuedSecondThrow && pos.x + hitbox.width < LevelManager::GetInstance()->GetPlayerPosition().x)
        {
            isFlipped = true;
            queuedSecondThrow = true;
            hasFlipped = true;
        }

        if (isThrowing) {
            throwSprite->Update(deltaTime);
            currentSprite = throwSprite;
            TryThrowSnowball();

            // When the animation completes, reset state and start the cooldown
            if (throwSprite->hasLoopedOnce()) {
                isThrowing = false;
                throwSprite->ResetAnimation();
                currentSprite = idleSprite;
                throwTimer = throwCooldown; // Reset timer for the next throw
            }
        }
        else {
            idleSprite->Update(deltaTime);
            currentSprite = idleSprite;
        }
    }
    else {
        if (idleSprite) idleSprite->SetPosition(pos);
        if (idleSprite) idleSprite->Update(deltaTime);
        currentSprite = idleSprite;
    }

    if (isHurt) hurtTimer -= deltaTime;
    for (auto& sb : snowballPool) {
        if (sb->IsActive()) sb->Update(deltaTime);
    }
    UpdateHitbox();
}

void SnowmanEnemy::Draw() const {
    if (currentSprite) {
        if (isFlipped) {
            Rectangle src = currentSprite->GetSourceRect();
            src.width *= -1;  // flip horizontally

            DrawTexturePro(
                currentSprite->GetTexture(),
                src,
                Rectangle{ pos.x, pos.y, currentSprite->GetWidth(), currentSprite->GetHeight() },
                Vector2{ 0, 0 },
                0.0f,
                WHITE
            );
        }
        else {
            currentSprite->Draw(pos.x, pos.y);
        }
    }

    // DrawRectangleLines((int)hitbox.x, (int)hitbox.y, (int)hitbox.width, (int)hitbox.height, RED); // Debug
    for (auto& sb : snowballPool) {
        if (sb->IsActive()) sb->Draw();
    }
}

Rectangle SnowmanEnemy::GetHitbox() const {
    return hitbox;
}

void SnowmanEnemy::TakeDamage() {
    isHurt = true;
    hurtTimer = 0.4f;
}

bool SnowmanEnemy::ShouldBeRemoved() const {
    return isHurt && hurtTimer <= 0.0f && pos.x + hitbox.width < 0;
}

void SnowmanEnemy::UpdateHitbox() {
    hitbox = { pos.x + 8, pos.y + 8, 48, 48 };
}

std::vector<SnowballProjectile*> SnowmanEnemy::GetSnowballs() const {
    std::vector<SnowballProjectile*> out;
    for (const auto& sb : snowballPool) {
        if (sb->IsActive()) out.push_back(sb.get());
    }
    return out;
}

SnowballProjectile* SnowmanEnemy::GetInactiveSnowball()
{
    for (auto& sb : snowballPool) {
        if (!sb->IsActive()) return sb.get();
    }
    // none free → create one
    snowballPool.push_back(
        std::make_unique<SnowballProjectile>(Vector2{ 0,0 }, Vector2{ 0,0 })
    );
    return snowballPool.back().get();
}

void SnowmanEnemy::ResetSnowballs()
{
    for (auto& sb : snowballPool) {
        sb->Deactivate();
    }
}

void SnowmanEnemy::SetSpeed(float speed)
{
    this->speed = speed; // Update movement speed
}