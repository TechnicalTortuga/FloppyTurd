#pragma once
#include "Level.h"
#include "RaylibCompat.h"
#include <vector>
#include <iostream>
#include "CameraSystem.h"
#include "ParallaxLayer.h"
#include "ToiletPair.h"
#include "SnowmanEnemy.h"

class SnowLevel : public Level
{
public:
    SnowLevel();
    ~SnowLevel();

    void Draw() const override;
    void Update(float deltaTime) override;
    void InitObstacles();

    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    void SetSwingingPipes(bool enable) override;
    void SetDifficulty(int difficultyIndex) override;
    void SetPanSpeed(float speed) override;
    float pickupPanSpeed = 80.0f; // New: Dynamic pickup speed

private:
    CameraSystem* cameraSystem;

    float gameScale{ 1.0f };
    float screenScale{ 1.0f };

    std::vector<std::shared_ptr<ToiletPair>> toilets;
    std::vector<std::shared_ptr<Obstacle>> obstacles;
    std::vector<std::shared_ptr<SnowmanEnemy>> enemies;

    std::vector<std::shared_ptr<PickUp>> pickups;

    float spacing = 200.0f;


    void SpawnPickupsBetween(float xStart, float xEnd);
};