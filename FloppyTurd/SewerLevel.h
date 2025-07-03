#pragma once
#include "Level.h"
#include "RaylibCompat.h"
#include <vector>
#include <iostream>
#include "CameraSystem.h"
#include "ParallaxLayer.h"
#include "SewerPipe.h"
#include "Sprite.h"

class SewerLevel : public Level
{
public:
    SewerLevel();
    ~SewerLevel();

    void Draw() const override;

    void SpawnCoins();
    void SpawnPickups(std::shared_ptr<SewerPipe> pipe);

    void Update(float deltaTime) override;

    void SetSwingingPipes(bool enable) override {};
    void SetDifficulty(int difficultyIndex) override; // New: Set music based on difficulty
    void SetPanSpeed(float speed) override; // New: Set pan speed for obstacles, enemies, pickups
    void SpawnPickupsBetween(float xStart, float xEnd);

    float GetCurrentPipePairX() const;
    float GetCurrentPipeGapCenter() const;

    void InitObstacles();

    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    Vector2 FindSafeSpawnForSinglePipe(int enemyWidth, int enemyHeight) const;
    float GetPipePanSpeed() const;

    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }
    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;

    std::vector<std::shared_ptr<SewerPipe>>& GetPipes() { return pipes; }

    void SetPanSpeedMultiplier(float multiplier, float duration);
    int pipesPassed = 0;

private:
    CameraSystem* cameraSystem;

    float backParallax{ 0.25f };
    float midParallax{ 0.50f };
    float frontParallax{ 0.75f };

    float gameScale{ 1.0f };
    float screenScale{ 1.0f };

    std::vector<std::shared_ptr<SewerPipe>> pipes{};
    std::vector<std::shared_ptr<Obstacle>> obstacles;
    std::vector<std::shared_ptr<PickUp>> pickups;

    float spacing = 200.0f;
    float pickupPanSpeed = 80.0f; // New: Dynamic pickup speed

    float panSpeedMultiplier = 1.0f;
    float panSpeedTimer = 0.0f;

    float EstimateOppositeEdge(std::shared_ptr<SewerPipe> pipe) const;

    Sprite* janitorIdle = nullptr;
    Sprite* janitorSweep = nullptr;
    Sprite* janitorShock = nullptr;

    float janitorX = 400.0f;
    float janitorTimer = 0.0f;
    float janitorNextAppearance = 10.0f;
    bool janitorVisible = false;
    bool janitorIsShocked = false;
    Sprite* janitorCurrent = nullptr;

    void UpdateJanitor(float deltaTime);
    void DrawJanitor() const;
    void ResetJanitor();
};