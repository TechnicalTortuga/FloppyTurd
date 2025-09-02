#pragma once
#include "PickUp.h"
#include "CoinType.h"
#include "Sprite.h"
#include "AudioClip.h"

class Coin : public PickUp {
public:
    Coin(Vector2 pos, CoinType type);
    ~Coin();

    void Update(float deltaTime) override;
    void Draw() const override;
    Rectangle GetHitbox() const override;
    void OnPickup() override;

    int GetValue() const { return value; }

    // Shared audio loading/unloading
    void SetPanSpeed(float speed);

private:
    CoinType type;
    Sprite* sprite = nullptr;
    Rectangle hitbox;
    int value = 0;

    float panSpeed = 0.0f;
    void Init();
};