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

    // Inherited via Level
    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    // New method to enable swinging pipes
    void SetSwingingPipes(bool enable) { swingingPipes = enable; }

private:
    CameraSystem* cameraSystem;

    float gameScale{ 1.0f }; // Added GameScale variable
    float screenScale{ 1.0f }; // Additional scaling to fit screen

	std::vector<std::shared_ptr<ToiletPair>> toilets;
    std::vector<std::shared_ptr<Obstacle>> obstacles;

    std::vector<std::shared_ptr<PickUp>> pickups;

    float spacing = 200.0f; // For the obstacles

    void SpawnPickupsBetween(float xStart, float xEnd);

    // pan speed for pickups should match level scroll speed
    static constexpr float PickupPanSpeed = 80.0f;

    // Swinging pipes support
    bool swingingPipes = false;
    float swingTimer = 0.0f;
};

