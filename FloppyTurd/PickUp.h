// PickUp.h
#pragma once
#include "PlatformAPI.h"

class PickUp {
public:
    PickUp() : timeAlive(0.0f), wavePhase(0.0f) {}
    virtual ~PickUp() = default;
    virtual void Update(float deltaTime)
    {
        timeAlive += deltaTime;
        position.x -= panSpeed * deltaTime; // Default panning
    }
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
    float GetTimeAlive() const { return timeAlive; }
    float GetWavePhase() const { return wavePhase; }
    void SetWavePhase(float phase) { wavePhase = phase; }
protected:
    Vector2 position{};
    float width = 32.0f;
    bool collected = false;
    float panSpeed;
    float wavePhase = 0.0f; // Phase for standing wave motion
    float timeAlive = 0.0f; // Track time since spawn
};

