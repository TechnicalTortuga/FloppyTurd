#pragma once
#include "Obstacle.h"
#include "raylib.h"
#include "Resources.h"
#include <vector>

class SpikeBall : public Obstacle
{
public:
    // `start` is the *centre-bottom* of the square anchor that the chain hangs from
    explicit SpikeBall(Vector2 start);
    ~SpikeBall();

    void Update(float dt) override;
    void Draw() override;
    std::vector<Rectangle> GetHitboxes() override;
    Rectangle GetHitbox() const;

    // Handy helpers
    Vector2 GetPosition() const { return basePos; }
    void SetPosition(Vector2 newPos);

private:
    void UpdateHitbox();

    Texture2D baseTex{};          // Little square “bracket” (10x10)
    Texture2D swingTex{};         // Chain + ball in one image (64x90)

    Vector2 basePos{};            // Centre-bottom of the anchor
    Vector2 pivotPos{};           // World-space position of the chain's pivot (center of anchor)
    Vector2 drawPos{};            // Top-left for DrawTexturePro()
    Vector2 origin{ 32.f, 0.f };    // Pivot: top-centre of swingTex

    Rectangle ballHit{};          // 24 × 24 AABB for the ball

    float angle = 0.f;            // Current rotation (rad)
    float angSpeed = 1.5f;        // Angular speed (rad/s)
    float linkLength = 70.f;      // Pivot → ball-centre
    float panSpeed = 80.f;        // World scroll speed (px/s)

    // Constants
    static constexpr float PIVOT_LEN = 70.f;  // Distance from pivot to ball center
    static constexpr float BASE_CENTER_Y_OFFSET = 5.f; // Center of the 10x10 anchor is 5px up from bottom
};