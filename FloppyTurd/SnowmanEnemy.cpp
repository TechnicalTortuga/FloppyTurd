#include <algorithm>
#include "SnowmanEnemy.h"
#include "Resources.h"
#include "SnowballProjectile.h"
#include <iostream>
#include <iomanip>
#include <raymath.h>
#include "LevelManager.h"

SnowmanEnemy::SnowmanEnemy(Vector2 spawnPos, SnowmanType type)
    : pos(spawnPos), type(type), isHurt(false), hurtTimer(0.0f), throwTimer(0.0f), hasThrown(false), isThrowing(false), speed(80.0f) {

    using namespace Resources;

    switch (type) {
    case SnowmanType::sRed:
        idleSprite = new Sprite(SnowManRed, 1, 0.1f, 1.0f, pos);
        throwSprite = new Sprite(SnowManRedThrow, 6, 0.12f, 1.0f, pos);
        currentSprite = idleSprite;
        break;
    case SnowmanType::sGREEN:
        idleSprite = new Sprite(SnowManGreen, 1, 0.1f, 1.0f, pos);
        throwSprite = new Sprite(SnowManRedThrow, 6, 0.12f, 1.0f, pos);
        currentSprite = idleSprite;
        break;
    case SnowmanType::sBLUE:
        idleSprite = new Sprite(SnowManBlue, 1, 0.1f, 1.0f, pos);
        throwSprite = new Sprite(SnowManRedThrow, 6, 0.12f, 1.0f, pos);
        currentSprite = idleSprite;
        break;
    case SnowmanType::sCHAD:
        idleSprite = new Sprite(SnowManChad, 1, 0.1f, 1.0f, pos);
        throwSprite = new Sprite(SnowManRedThrow, 6, 0.12f, 1.0f, pos);
        currentSprite = idleSprite;
        break;
    }
    UpdateHitbox();
}

SnowmanEnemy::~SnowmanEnemy() {
    delete idleSprite;
    delete throwSprite;
}

void SnowmanEnemy::TryThrowSnowball()
{
    // — no fire while cooling down —
    if (throwTimer > 0.0f) return;

    // — only once per animation cycle, at frame 4 —
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

        // skip overly-vertical throws
        if (fabsf(delta.x) < fabsf(delta.y) * 0.3f)
            return;

        // normalize and fire
        Vector2 vel = Vector2Scale(Vector2Normalize(delta), 220.0f);
        SnowballProjectile* sb = GetInactiveSnowball();
        sb->Activate(start, vel);

        hasThrown = true;
        throwTimer = 1.2f;   // tweak for your desired delay
    }

    // — once the throw animation loops, reset for the next cycle —
    if (hasThrown
        && currentSprite == throwSprite
        && throwSprite->hasLoopedOnce())
    {
        // swap back to idle
        currentSprite = idleSprite;

        // reset for next throw
        hasThrown = false;

        // clear the loopedOnce flag & rewind the throw anim
        throwSprite->ResetAnimation();
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

        // Begin throw when fully on screen (left side fully passed edge)
        if (!isThrowing && pos.x + 48 <= 320) {
            throwSprite->ResetAnimation();
            isThrowing = true;
            hasThrown = false;
            throwTimer = 0.0f;
        }

        // Flip around if to the left of player and hasn�t already flipped
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

            if (throwSprite->hasLoopedOnce()) {
                isThrowing = false;
                throwSprite->ResetAnimation();
                currentSprite = idleSprite;
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
    hitbox = { pos.x + 8, pos.y + 8,48, 48 };
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