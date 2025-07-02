#pragma once

#include "Level.h"
#include "raylib.h"
#include "GameSettings.h"
#include "ResourceCompat.h"
#include <iostream>
#include <vector>
#include <memory>
#include "CameraSystem.h"
#include "Obstacle.h"
#include "Outhouse.h"
#include "Sprite.h"
#include <random>
#include <ctime>
#include "ParallaxLayer.h"
#include "AnimatedParallaxLayer.h"
#include <unordered_set>
#include <cfloat>

class DesertLevel : public Level
{
public:
    DesertLevel();
    ~DesertLevel();

    void Draw() const override;
    void Update(float deltaTime) override;

    void SetSwingingPipes(bool enable) override {};
    void SetDifficulty(int difficultyIndex) override; // Set music based on difficulty
    void SetPanSpeed(float speed) override; // Set pan speed for obstacles, enemies, pickups
    bool IsBrickBetween(float leftX, float rightX) const;
    void SpawnPickupsBetween(float xStart, float xEnd);
    std::shared_ptr<PickUp> GenerateRandomPickup(Vector2 pos);

    void InitObstacles();
    void InitDecoration();

    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    static constexpr float SpawnPadding = 16.0f;
    std::vector<std::shared_ptr<Outhouse>> outhouses;
    std::vector<std::shared_ptr<Obstacle>> obstacles;

    float lastBirdGapCenter = FLT_MAX;
    static constexpr float birdGapSpacing = 50.0f;

    float lastCoinSpawnX = FLT_MAX;
    static constexpr float coinGapSpacing = 64.0f;

private:
    CameraSystem* cameraSystem;

    float backParallax{ 0.25f };
    float midParallax{ 0.50f };
    float frontParallax{ 0.75f };

    float gameScale{ 4.0f };
    float screenScale{ 1.5f };

    std::vector<std::shared_ptr<Sprite>> midground;
    std::vector<std::shared_ptr<Sprite>> foreground;

    std::mt19937 engine;
    std::uniform_int_distribution<int> cactiCountDist;
    std::uniform_int_distribution<int> cactusTypeDist;
    std::uniform_real_distribution<float> xSpacingDist;

    std::vector<std::shared_ptr<PickUp>> pickups;

    float pickupPanSpeed = 80.0f; // Dynamic pickup speed

    struct CactusInfo {
        const char* spriteId;
        int width;
        int height;
    };

    inline static const CactusInfo CACTUS_NORMAL = { Resources::DancingCacti, 64, 90 };
    inline static const CactusInfo CACTUS_SMALL = { Resources::DancingCactiSmall, 64, 64 };
    inline static const CactusInfo CACTUS_COWBOY = { Resources::DancingCactiCowboy, 64, 90 };

    float spacing = 300.0f;

    std::unordered_set<int> spawnedPickupsIndices;
};