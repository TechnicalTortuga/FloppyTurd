#pragma once
#include "PickUp.h"
#include "PoopHeartType.h"
#include "Sprite.h"

class PoopHeart : public PickUp {
public:
    PoopHeart(Vector2 pos, PoopHeartType type);
    ~PoopHeart();

    void Update(float deltaTime) override;
    void Draw() const override;
    Rectangle GetHitbox() const override;
    void OnPickup() override;
    void SetPlayerPosition(Vector2* pos) { playerPos = pos; } // Set player position for magnet

    int GetHealAmount() const;
    bool IsInvisible() const;
	float GetPanSpeed() const { return panSpeed; }
	void SetPanSpeed(float speed) { panSpeed = speed; }
    PoopHeartType GetType() const;

private:
    Vector2* playerPos = nullptr; // Pointer to player's position for magnet effect
    PoopHeartType type;
    Sprite* sprite = nullptr;
    Rectangle hitbox;
    float panSpeed = 0.0f;
};
