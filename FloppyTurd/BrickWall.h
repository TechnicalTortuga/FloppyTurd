// BrickWall.h
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

    float GetWidth() const;
    Vector2 GetPosition() const;
    void SetPosition(Vector2 newPos);

private:
    Texture2D texture;
    Rectangle hitbox;
    void UpdateHitbox();
};