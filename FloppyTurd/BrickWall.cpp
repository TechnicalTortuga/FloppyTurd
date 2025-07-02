#include "BrickWall.h"
#include "ResourceCompat.h"
#include "TextureCache.h"

BrickWall::BrickWall(Vector2 pos)
{
	using namespace Resources;
	this->pos = pos;
	texture = TextureCache::Get(BrickWallTexture);
	panSpeed = 80.0f; // Default to Regular speed
	UpdateHitbox();
}

BrickWall::~BrickWall()
{
	// Texture unloading is managed by TextureCache::Clear()
}

void BrickWall::Draw()
{
	DrawTextureV(texture, pos, WHITE);
	// Uncomment for debug: DrawRectangleLines(hitbox.x, hitbox.y, hitbox.width, hitbox.height, RED);
}

void BrickWall::Update(float deltaTime)
{
	// Move left based on panSpeed and deltaTime
	pos.x -= panSpeed * deltaTime;
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