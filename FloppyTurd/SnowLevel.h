#pragma once
#include "Level.h"
#include "raylib.h"
#include <vector>
#include <iostream>
#include "CameraSystem.h"
#include "ParallaxLayer.h"
#include "ToiletPair.h"

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

private:
    CameraSystem* cameraSystem;

    float gameScale{ 1.0f };
    float screenScale{ 1.0f };

    std::vector<std::shared_ptr<ToiletPair>> toilets;
    std::vector<std::shared_ptr<Obstacle>> obstacles;

    std::vector<std::shared_ptr<PickUp>> pickups;

    float spacing = 200.0f;

    void SpawnPickupsBetween(float xStart, float xEnd);

    static constexpr float PickupPanSpeed = 80.0f;
};