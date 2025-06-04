#include "Projectile.h"
#include "GameSettings.h"

Projectile::Projectile(Vector2 position, Vector2 direction, float speed, float scale, const char* spriteFilePath)
    : position(position), direction(direction), speed(speed), scale(scale)
{
    float frameWidth = 16.0f;   // Width of each frame in the sprite sheet
    float frameHeight = 16.0f;  // Height of each frame in the sprite sheet

    // Initialize the sprite with scaling
    sprite = new Sprite(spriteFilePath, position.x, position.y, 16.0f, 16.0f, 0.1f, scale);

    // Initialize the hitbox based on the scaled sprite dimensions
    hitbox.width = sprite->GetScaledWidth();
    hitbox.height = sprite->GetScaledHeight();
    hitbox.x = position.x;
    hitbox.y = position.y;
}

Projectile::~Projectile() {
    delete sprite;
}

void Projectile::Update(float deltaTime) {
    position.x += direction.x * speed * deltaTime;
    position.y += direction.y * speed * deltaTime;

    // Update sprite position and animation
    sprite->SetPosition(position.x, position.y);
    sprite->Update(deltaTime);

    // Update hitbox position
    hitbox.x = position.x;
    hitbox.y = position.y;
}

void Projectile::Draw() {
    sprite->Draw(position.x, position.y);
    // Optional: Draw hitbox for debugging
    // DrawRectangleLines(hitbox.x, hitbox.y, hitbox.width, hitbox.height, RED);
}

Rectangle Projectile::GetHitbox() const {
    return hitbox;
}

bool Projectile::IsOffScreen() const {
    return position.x > GameSettings::GameWidth || position.x < 0 ||
        position.y > GameSettings::GameHeight || position.y < 0;
}
