#pragma once
#include "Level.h"
#include "GameSettings.h"
#include "Resources.h"
#include "ToiletPair.h"
#include <iostream>
#include <vector>
#include <memory>
#include "CameraSystem.h"
#include "ParallaxLayer.h"
#include "Obstacle.h"

class ParkLevel : public Level
{
public:
    ParkLevel();
    ~ParkLevel();

    void Draw() const override;
    void Update(float deltaTime) override;
    void InitObstacles();

    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    void SetSwingingPipes(bool enable) override;

    std::vector<std::shared_ptr<ToiletPair>> toilets{};
    std::vector<std::shared_ptr<Obstacle>> obstacles;

private:
    CameraSystem* cameraSystem;

    float backParallax{ 0.25f };
    float midParallax{ 0.50f };
    float frontParallax{ 0.75f };

    float gameScale{ 4.0f };
    float screenScale{ 1.5f };

    float xDistBackgroundBack{ 0 };
    float xDistBackgroundMid{ 0 };
    float xDistBackgroundFront{ 0 };

    Rectangle TopToiletHitbox{};
    Rectangle BottomToiletHitbox{};

    float xDist;
    float xDistBackground;

    std::vector<std::shared_ptr<PickUp>> pickups;

    int spacing{ 100 };
};