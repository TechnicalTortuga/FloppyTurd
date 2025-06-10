#pragma once
#include "Obstacle.h"
#include "raylib.h"

class BrickWall : public Obstacle {
public:
    BrickWall(Vector2 pos);
    ~BrickWall();

    void Draw() override;
    void Update(float deltaTime) override;
    std::vector<Rectangle> GetHitboxes() override;
    void SetCollisionEnabled(bool enabled) override;
    void SetPanSpeed(float speed) override; // Set pan speed

    float GetWidth() const;
    Vector2 GetPosition() const;
    void SetPosition(Vector2 newPos);

private:
    Texture2D texture;
    Rectangle hitbox;
    float panSpeed = 80.0f; // Store pan speed
    void UpdateHitbox();
};