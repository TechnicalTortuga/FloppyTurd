#pragma once
#include "raylib.h"
#include "GameSettings.h"
#include <vector>
#include "Obstacle.h"
#include "Resources.h"

enum class PipeType {
    TopOrange,
    TopBlue,
    BottomOrange,
    BottomBlue
};

class SewerPipe : public Obstacle {
public:
    SewerPipe(int xPos, int yPos);
    ~SewerPipe();

    void Draw() override;
    void Update(float deltaTime) override;
    void resetScore();
    Rectangle GetHitbox() const { return hitbox; }
    std::vector<Rectangle> GetHitboxes() override;
    void SetCollisionEnabled(bool enabled) override;
    void SetPanSpeed(float speed); // New: Set pan speed

    float GetX() const { return pos.x; }
    float GetWidth() const { return static_cast<float>(chosenPipe.width); }
    PipeType GetPipeType() const { return pipeType; }
    float GetPanSpeed() const { return panSpeed; }

    float GetEdge() const;

    void yOffsetRandomizer();

    void SetPipeType(PipeType newType);

    bool hasScored = false;

    bool HasSpawnedPickups() const { return hasSpawnedPickups; }
    void SetSpawnedPickups(bool spawned) { hasSpawnedPickups = spawned; }

private:
    void UpdateHitbox();
    bool hasSpawnedPickups = false;
    Texture2D chosenPipe;
    PipeType pipeType;
    float panSpeed;
    float yOffset;
    Rectangle hitbox;
};