#pragma once
#include "Obstacle.h"
#include "raylib.h"
#include "Resources.h"
#include <vector>

class SpikeBall : public Obstacle
{
public:
    explicit SpikeBall(Vector2 start);
    ~SpikeBall();

    void Update(float dt) override;
    void Draw() override;
    std::vector<Rectangle> GetHitboxes() override;
    void SetCollisionEnabled(bool enabled) override;
    void SetPanSpeed(float speed) override; // New: Set pan speed
    Rectangle GetHitbox() const;

    Vector2 GetPosition() const { return basePos; }
    void SetPosition(Vector2 newPos);

private:
    void UpdateHitbox();

    Texture2D baseTex;
    Texture2D swingTex;

    Vector2 basePos;
    Vector2 pivotPos;
    Vector2 drawPos;
    Vector2 origin{ 32.f, 0.f };

    Rectangle ballHit;

    float angle = 0.f;
    float angSpeed = 1.5f;
    float linkLength = 70.f;
    float panSpeed = 80.f;

    static constexpr float PIVOT_LEN = 70.f;
    static constexpr float BASE_CENTER_Y_OFFSET = 5.f;
};