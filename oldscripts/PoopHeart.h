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

    int GetHealAmount() const;
    bool IsInvisible() const;
	float GetPanSpeed() const { return panSpeed; }
	void SetPanSpeed(float speed) { panSpeed = speed; }
    PoopHeartType GetType() const;

private:
    PoopHeartType type;
    Sprite* sprite = nullptr;
    Rectangle hitbox;
    float panSpeed = 0.0f;
};
