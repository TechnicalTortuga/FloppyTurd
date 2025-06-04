#include "SpikeBall.h"
#include <raymath.h>
#include "GameSettings.h"

// Comment-out to hide the red hit-box helper
#define SPIKEBALL_DEBUG 1

using namespace Resources;

SpikeBall::SpikeBall(Vector2 start)
    : basePos(start)
{
    baseTex = LoadTexture(SpikeBallBase);
    swingTex = LoadTexture(SpikeBallTexture);

    if (swingTex.id == 0 || baseTex.id == 0) {
        TraceLog(LOG_WARNING, "SpikeBall: Failed to load textures. Visuals may be missing.");
    }

    UpdateHitbox(); // Initializes pivotPos and drawPos
}

SpikeBall::~SpikeBall()
{
    UnloadTexture(swingTex);
    UnloadTexture(baseTex);
}

void SpikeBall::Update(float dt)
{
    // Update the swing angle
    angle += angSpeed * dt;
    if (angle > 2 * PI) angle -= 2 * PI;

    // Scroll with the level
    basePos.x -= panSpeed * dt;

    // Wrap to right-hand side once fully off-screen
    if (basePos.x + swingTex.width < 0)
        basePos.x = 320.f + 120.f; // 320 = virtual screen width

    UpdateHitbox();
}

void SpikeBall::UpdateHitbox()
{
    // Calculate the world-space position of the pivot (center of the anchor)
    pivotPos = { basePos.x, basePos.y - BASE_CENTER_Y_OFFSET };

    // Update drawPos to align swingTex's top-center with the anchor's center
    // Add 32px to the right to correct the ball's visual offset in the texture
    drawPos = { pivotPos.x - origin.x + 32.f, pivotPos.y - origin.y };

    // Unit vector from pivot → ball center
    Vector2 dir = { -sinf(angle), cosf(angle) };

    // Calculate the ball's center position
    Vector2 centre = Vector2Add(
        pivotPos,
        Vector2Scale(dir, PIVOT_LEN) // Distance from pivot to ball center
    );

    // Update the hitbox (24x24 AABB centered on the ball)
    ballHit = { centre.x - 12.f, centre.y - 12.f, 24.f, 24.f };
}

void SpikeBall::Draw()
{
    // 1) Swinging chain + ball
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

    // 2) Anchor bracket (centre-bottom sits at basePos)
    if (baseTex.id > 0) {
        DrawTexturePro(
            baseTex,
            { 0, 0, (float)baseTex.width, (float)baseTex.height },
            { basePos.x - baseTex.width / 2.f,
              basePos.y - baseTex.height,
              (float)baseTex.width,
              (float)baseTex.height },
            { 0, 0 },
            0.f,
            WHITE
        );
    }
}

std::vector<Rectangle> SpikeBall::GetHitboxes()
{
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