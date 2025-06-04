// PickUp.h
#pragma once
#include "raylib.h"

class PickUp {
public:
    virtual ~PickUp() = default;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() const = 0;
    virtual Rectangle GetHitbox() const = 0;
    virtual void OnPickup() = 0;  // Called when player collects this
    bool IsCollected() const { return collected; }
	virtual void SetPanSpeed(float speed) { panSpeed = speed; }
	Vector2 GetPosition() const { return position; }
	void SetPosition(Vector2 pos) { position = pos; }
    virtual bool ShouldBeRemoved() const {
        return collected || (position.x + width < 0);
    }
protected:
    Vector2 position{};
    float width = 32.0f;
    bool collected = false;
    float panSpeed;
};

