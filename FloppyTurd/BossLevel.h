#pragma once
#include "Level.h"
#include "raylib.h"
#include "CameraSystem.h"
#include "Boss.h"
#include <memory>
#include <vector>
#include <random>

class BossLevel : public Level
{
public:
    BossLevel();
    ~BossLevel();

    void Draw() const override;
    void Update(float deltaTime) override;

    void InitCamera();
    void InitLayers();  // Renamed from InitDecoration to reflect camera integration

    void SetSwingingPipes(bool enable) override {};
    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    // Inherited via Level
    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    // New method to access the boss
    std::shared_ptr<Boss> GetBoss() { return boss; }

    // New method to reset boss and pickups
    void Reset();

private:
    CameraSystem* cameraSystem;
    std::shared_ptr<Boss> boss;  // RatKing or future bosses
    AudioClip* lowHealthMusic;

    double lowHealthTriggerTime = 0.0;
    double lowHealthPlayTime = 0.0;
    bool hasRecordedLowHealthPlay = false;
    std::vector<std::shared_ptr<PickUp>> pickups;

    // Pickup wave spawning
    float pickupSpawnTimer = 0.0f; // Timer for next wave
    float pickupSpawnInterval = 5.0f; // Initial interval (will be randomized)
    std::mt19937 engine; // Random number generator
    std::uniform_int_distribution<int> pickupTypeDist; // For pickup type (0-99)
    std::uniform_int_distribution<int> pickupCountDist; // For number of pickups (3-5)
    std::uniform_real_distribution<float> spawnIntervalDist; // For wave interval (5-8s)
    std::uniform_real_distribution<float> phaseDist; // For random phase (0-2π)

    // Standing wave parameters
    float waveAmplitude = 40.0f; // Wave height (±40 pixels from y=90)
    float waveFrequency = 6.2832f; // 2π radians/second (1 cycle/second)

    void SpawnPickupWave(); // Spawn a wave of pickups
    std::shared_ptr<PickUp> GenerateRandomPickup(Vector2 pos); // Generate a single pickup
};