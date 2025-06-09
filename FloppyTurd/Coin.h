#pragma once
#include "PickUp.h"
#include "CoinType.h"
#include "Sprite.h"
#include "AudioClip.h"

class Coin : public PickUp {
public:
	Coin(Vector2 pos, CoinType type, Vector2* playerPos = nullptr);
	~Coin();

	void Update(float deltaTime) override;
	void Draw() const override;
	Rectangle GetHitbox() const override;
	void OnPickup() override;

	int GetValue() const { return value; }
	void SetPanSpeed(float speed) override;
	void SetPlayerPosition(Vector2* pos) { playerPos = pos; } // Set player position for magnet

private:
	CoinType type;
	Sprite* sprite = nullptr;
	Rectangle hitbox;
	int value = 0;
	Vector2* playerPos = nullptr; // Pointer to player's position for magnet effect
	float panSpeed = 0.0f;
	void Init();
};