#include "BrickWall.h"
#include "Resources.h"

BrickWall::BrickWall(Vector2 pos)
{
    using namespace Resources;
    this->pos = pos;
    texture = LoadTexture(BrickWallTexture);
    panSpeed = 80.0f; // Default to Regular speed
    UpdateHitbox();
}

BrickWall::~BrickWall()
{
    UnloadTexture(texture);
}

void BrickWall::Draw()
{
    DrawTextureV(texture, pos, WHITE);
    // Uncomment for debug: DrawRectangleLines(hitbox.x, hitbox.y, hitbox.width, hitbox.height, RED);
}

void BrickWall::Update(float deltaTime)
{
    // Movement handled by DesertLevel::Update(), but update hitbox
    UpdateHitbox();
}

std::vector<Rectangle> BrickWall::GetHitboxes()
{
    if (!collisionEnabled) {
        return std::vector<Rectangle>();
    }
    return { hitbox };
}

void BrickWall::UpdateHitbox()
{
    hitbox = { pos.x + 2, pos.y + 2, (float)texture.width - 4, (float)texture.height - 4 };
}

float BrickWall::GetWidth() const
{
    return static_cast<float>(texture.width);
}

Vector2 BrickWall::GetPosition() const
{
    return pos;
}

void BrickWall::SetPosition(Vector2 newPos)
{
    pos = newPos;
    UpdateHitbox();
}

void BrickWall::SetCollisionEnabled(bool enabled)
{
    collisionEnabled = enabled;
}

void BrickWall::SetPanSpeed(float speed)
{
    panSpeed = speed;
}