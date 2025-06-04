#include "ToiletPaperProjectile.h"
#include "Resources.h"
#include "GameSettings.h"

ToiletPaperProjectile::ToiletPaperProjectile(Vector2 position, Vector2 direction, float speed, float scale)
    : position(position), direction(direction), speed(speed), scale(scale)
{
    using namespace Resources;
    // Use 4-frame 32x32 sprite sheet for toilet paper
    sprite = new Sprite(ToiletPaperProjectilePic, 4, 0.1f, scale, position);  // 4 frames, 0.1s per frame

    // Initialize hitbox based on scaled sprite dimensions (32x32 per frame)
    hitbox.width = sprite->GetScaledWidth();  // 32 * scale
    hitbox.height = sprite->GetScaledHeight();  // 32 * scale
    hitbox.x = position.x;
    hitbox.y = position.y;
}

ToiletPaperProjectile::~ToiletPaperProjectile()
{
    delete sprite;
}

void ToiletPaperProjectile::Draw()
{
    sprite->Draw(position.x, position.y);
    // Optional: Draw hitbox for debugging
    // DrawRectangleLines(hitbox.x, hitbox.y, hitbox.width, hitbox.height, RED);
}

void ToiletPaperProjectile::Update(float deltaTime)
{
    position.x += direction.x * speed * deltaTime;
    position.y += direction.y * speed * deltaTime;

    // Update sprite position and animation
    sprite->SetPosition(position.x, position.y);
    sprite->Update(deltaTime);

    // Update hitbox position
    hitbox.x = position.x;
    hitbox.y = position.y;
}

std::vector<Rectangle> ToiletPaperProjectile::GetHitboxes()
{
    return { hitbox };
}