#pragma once
#include "Level.h"
#include "raylib.h"
#include "CameraSystem.h"
#include "Boss.h"
#include <memory>

class BossLevel : public Level
{
public:
    BossLevel();
    ~BossLevel();

    void Draw() const override;
    void Update(float deltaTime) override;

    void InitCamera();
    void InitLayers();  // Renamed from InitDecoration to reflect camera integration

    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    // Inherited via Level
    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    // New method to access the boss
    std::shared_ptr<Boss> GetBoss() { return boss; }

private:
    CameraSystem* cameraSystem;
    std::shared_ptr<Boss> boss;  // RatKing or future bosses
    AudioClip* lowHealthMusic;

    double lowHealthTriggerTime = 0.0;
    double lowHealthPlayTime = 0.0;
    bool hasRecordedLowHealthPlay = false;
    std::vector<std::shared_ptr<PickUp>> pickups;

};