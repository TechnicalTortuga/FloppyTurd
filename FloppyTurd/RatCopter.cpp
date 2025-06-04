#include "RatCopter.h"
#include "Resources.h"
#include <iostream>
#include <raymath.h>
#include "LevelManager.h"

RatCopter::RatCopter(Vector2 spawnPos, float panspeed) {
    using namespace Resources;
    pos.x = spawnPos.x;
    pos.y = spawnPos.y;
    speed = panspeed; // Set enemy speed to match level pan speed initially

    ratSpriteIdle = new Sprite(RatCopterIdle, 6, 0.1f, 1.0f, Vector2(pos.x, pos.y));
    ratSpriteHurt = new Sprite(RatCopterHurt, 6, 0.1f, 1.0f, Vector2(pos.x, pos.y));

    currentSprite = ratSpriteIdle;
    isHurt = false;
    hurtTimer = 0.0f;

    currentMode = Mode::FLY_IN;
    hoverTimer = 0.0f;
    direction = { -1.0f, 0.0f }; // Default left for FLY_IN
    hasLockedDirection = false;
    pullbackVector = { 0.0f, 0.0f };
    pullbackTimer = 0.25f;

    UpdateHitbox();
}

RatCopter::~RatCopter()
{
    delete ratSpriteIdle;
    delete ratSpriteHurt;
}

void RatCopter::Draw() const
{
    currentSprite->Draw(pos.x, pos.y);
}

void RatCopter::Update(float deltaTime)
{
    switch (currentMode)
    {
    case Mode::FLY_IN:
        pos.x -= speed * deltaTime;
        if (pos.x <= 240.0f)  // Once mostly on screen
        {
            currentMode = Mode::HOVER;
            hoverTimer = 0.75f + GetRandomValue(0, 25) / 100.0f;
        }
        break;

    case Mode::HOVER:
        hoverTimer -= deltaTime;
        if (hoverTimer <= 0.0f)
        {
            currentMode = Mode::PULLBACK;
            pullbackTimer = 0.25f;
            // Capture player's position at this moment and lock direction
            Vector2 liveTarget = LevelManager::GetInstance()->GetPlayerPosition();
            direction = Vector2Normalize(Vector2Subtract(liveTarget, pos));
            pullbackVector = Vector2Scale(direction, -20.0f);  // Pull back opposite direction
            hasLockedDirection = true; // Lock direction for BEELINE
            std::cout << "[RatCopter] PULLBACK - Locked Direction: (" << direction.x << ", " << direction.y
                << "), Target was at: (" << liveTarget.x << ", " << liveTarget.y << ")\n";
        }
        break;

    case Mode::PULLBACK:
        pos = Vector2Add(pos, Vector2Scale(pullbackVector, deltaTime * 4.0f)); // Quick pullback
        pullbackTimer -= deltaTime;
        if (pullbackTimer <= 0.0f)
        {
            currentMode = Mode::BEELINE;
            // Boost speed for intimidating charge
            speed = 150.0f; // Fast, scary beeline toward locked target
            std::cout << "[RatCopter] BEELINE - Speed: " << speed
                << ", Locked Direction: (" << direction.x << ", " << direction.y << ")\n";
        }
        break;

    case Mode::BEELINE:
        // Move in locked direction, passing through where player was
        pos = Vector2Add(pos, Vector2Scale(direction, speed * deltaTime));
        std::cout << "[RatCopter] BEELINE - Pos: (" << pos.x << ", " << pos.y
            << "), Heading in direction: (" << direction.x << ", " << direction.y
            << "), Speed: " << speed * deltaTime << "\n";
        // Remove if off-screen after passing target
        if (pos.x < -32.0f || pos.x > 320.0f + 32.0f || pos.y < -32.0f || pos.y > 180.0f + 32.0f)
        {
            isHurt = true; // Mark for removal
            hurtTimer = 0.0f; // Immediate removal
        }
        break;
    }

    // Update sprite animation
    currentSprite->Update(deltaTime);

    // Handle hurt state
    if (isHurt) {
        if (currentSprite != ratSpriteHurt)
            currentSprite = ratSpriteHurt;

        hurtTimer -= deltaTime;
        if (hurtTimer > 0.0f)
            currentSprite->Update(deltaTime);
    }
    else {
        currentSprite->Update(deltaTime);
    }
    UpdateHitbox();
}

Rectangle RatCopter::GetHitbox() const
{
    return hitbox;
}

void RatCopter::UpdateHitbox() {
    int xOffset = pos.x + 5;
    int yOffset = pos.y + 7;
    hitbox.x = xOffset;
    hitbox.y = yOffset;
    hitbox.width = 23;
    hitbox.height = 21;
}

void RatCopter::TakeDamage() {
    if (!isHurt) {
        isHurt = true;
        hurtTimer = 0.5f;
        currentSprite = ratSpriteHurt;
    }
}

bool RatCopter::ShouldBeRemoved() const {
    return isHurt && (hurtTimer <= 0.0f || currentSprite->hasLoopedOnce());
}

void RatCopter::SetTarget(Vector2 target)
{
    targetPos = target;
}