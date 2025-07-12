#pragma once
#include "Level.h"
#include "PlatformAPI.h"
#include "CameraSystem.h"
#include "Boss.h"
#include "Explosion.h"
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
    void InitLayers();

    void SetSwingingPipes(bool enable) override;
    void SetDifficulty(int difficultyIndex) override; // Set music based on difficulty
    void SetPanSpeed(float speed) override; // Set pan speed for pickups

    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    std::shared_ptr<Boss> GetBoss() { return boss; }

    void Reset();

    // Check if level is complete (for Playing to trigger credits)
    bool IsComplete() const { return isComplete; }

private:
    CameraSystem* cameraSystem;
    std::shared_ptr<Boss> boss;
    AudioClip* lowHealthMusic;

    double lowHealthTriggerTime = 0.0;
    double lowHealthPlayTime = 0.0;
    bool hasRecordedLowHealthPlay = false;
    std::vector<std::shared_ptr<PickUp>> pickups;

    float pickupSpawnTimer = 0.0f;
    float pickupSpawnInterval = 5.0f;
    std::mt19937 engine;
    std::uniform_int_distribution<int> pickupTypeDist;
    std::uniform_int_distribution<int> pickupCountDist;
    std::uniform_real_distribution<float> spawnIntervalDist;
    std::uniform_real_distribution<float> phaseDist;

    float waveAmplitude = 40.0f;
    float waveFrequency = 6.2832f;

    void SpawnPickupWave();
    std::shared_ptr<PickUp> GenerateRandomPickup(Vector2 pos);

    bool deathSequenceActive = false; // Flag for death sequence
    float deathSequenceTimer = 0.0f; // Timer for death sequence
    std::vector<std::shared_ptr<Explosion>> explosions; // Explosion effects
    bool isComplete = false; // Flag to signal level completion
};