#include "SpikeBall.h"
#include <raymath.h>
#include "GameSettings.h"
#include "Resources.h"
#include "TextureCache.h"

#define SPIKEBALL_DEBUG 1

using namespace Resources;

SpikeBall::SpikeBall(Vector2 start)
	: basePos(start)
{
	baseTex = TextureCache::Get(SpikeBallBase);
	swingTex = TextureCache::Get(SpikeBallTexture);

	if (swingTex.id == 0 || baseTex.id == 0) {
		TraceLog(LOG_WARNING, "SpikeBall: Failed to load textures from cache. Visuals may be missing.");
	}

	panSpeed = 80.0f; // Default to Regular speed
	UpdateHitbox();
}

SpikeBall::~SpikeBall()
{
	// Texture unloading is managed by TextureCache::Clear()
}

void SpikeBall::Update(float dt)
{
	angle += angSpeed * dt;
	if (angle > 2 * PI) angle -= 2 * PI;

	basePos.x -= panSpeed * dt;

	if (basePos.x + swingTex.width < 0)
		basePos.x = 320.f + 120.f;

	UpdateHitbox();
}

void SpikeBall::UpdateHitbox()
{
	pivotPos = { basePos.x, basePos.y - BASE_CENTER_Y_OFFSET };
	drawPos = { pivotPos.x - origin.x + 32.f, pivotPos.y - origin.y };

	Vector2 dir = { -sinf(angle), cosf(angle) };
	Vector2 centre = Vector2Add(pivotPos, Vector2Scale(dir, PIVOT_LEN));
	ballHit = { centre.x - 12.f, centre.y - 12.f, 24.f, 24.f };
}

void SpikeBall::Draw()
{
	if (swingTex.id > 0) {
		DrawTexturePro(
			swingTex,
			{ 0, 0, (float)swingTex.width, (float)swingTex.height },
			{ drawPos.x, drawPos.y, (float)swingTex.width, (float)swingTex.height },
			origin,
			RAD2DEG * angle,
			WHITE
		);
	}

	if (baseTex.id > 0) {
		DrawTexturePro(
			baseTex,
			{ 0, 0, (float)baseTex.width, (float)baseTex.height },
			{ basePos.x - baseTex.width / 2.f, basePos.y - baseTex.height, (float)baseTex.width, (float)baseTex.height },
			{ 0, 0 },
			0.f,
			WHITE
		);
	}
}

std::vector<Rectangle> SpikeBall::GetHitboxes()
{
	if (!collisionEnabled) {
		return std::vector<Rectangle>();
	}
	return { ballHit };
}

Rectangle SpikeBall::GetHitbox() const
{
	return ballHit;
}

void SpikeBall::SetPosition(Vector2 newPos)
{
	basePos = newPos;
	UpdateHitbox();
}

void SpikeBall::SetCollisionEnabled(bool enabled)
{
	collisionEnabled = enabled;
}

void SpikeBall::SetPanSpeed(float speed)
{
	panSpeed = speed;
}