#pragma once
#include "Obstacle.h"
#include "raylib.h"
#include "Sprite.h"

class ToiletPaperProjectile : public Obstacle
{
public:
    ToiletPaperProjectile(Vector2 position, Vector2 direction, float speed, float scale);
    ~ToiletPaperProjectile();

    void Draw() override;
    void Update(float deltaTime) override;  // Updated to accept deltaTime
    std::vector<Rectangle> GetHitboxes() override;
    bool IsOffScreen() const { return position.x < -32.0f; };
	Rectangle GetHitbox() const { return hitbox; }

private:
    Vector2 position;
    Vector2 direction;
    float speed;
    float scale;
    Sprite* sprite;
    Rectangle hitbox;
};