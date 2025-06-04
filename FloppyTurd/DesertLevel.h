#pragma once

#include "Level.h"
#include "raylib.h"
#include "GameSettings.h"
#include "Resources.h"
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

class DesertLevel : public Level
{
public:
    DesertLevel();
    ~DesertLevel();

    void Draw() const override;
    void Update(float deltaTime) override;

    bool IsBrickBetween(float leftX, float rightX) const;
    void SpawnPickupsBetween(float xStart, float xEnd);
    std::shared_ptr<PickUp> GenerateRandomPickup(Vector2 pos);

    void InitObstacles();
    void InitDecoration();

    bool checkForCollisions(Vector2 circleCenter, float circleRadius) override;
    bool checkForPointGain(Vector2 circleCenter, float circleRadius) override;

    // Inherited via Level:
    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override { return pickups; }

    // Horizontal padding so pickups don’t spawn flush against outhouse hitboxes:
    static constexpr float SpawnPadding = 16.0f;
    std::vector<std::shared_ptr<Outhouse>> outhouses;
    std::vector<std::shared_ptr<Obstacle>> obstacles;

    // NEW: keep track of the last gap‐center that spawned a Bird:
    float lastBirdGapCenter = FLT_MAX;

    // NEW: how far (in px) the gap must move left before allowing another Bird
    static constexpr float birdGapSpacing = 50.0f;

    float lastCoinSpawnX = FLT_MAX;

    static constexpr float coinGapSpacing = 64.0f;

private:
    CameraSystem* cameraSystem;

    float backParallax{ 0.25f };
    float midParallax{ 0.50f };
    float frontParallax{ 0.75f };

    float gameScale{ 4.0f };   // Added GameScale variable (not strictly used for pickups)
    float screenScale{ 1.5f }; // Additional scaling to fit screen



    std::vector<std::shared_ptr<Sprite>> midground;
    std::vector<std::shared_ptr<Sprite>> foreground;

    std::mt19937 engine;
    std::uniform_int_distribution<int> cactiCountDist;       // for 1..8 cacti
    std::uniform_int_distribution<int> cactusTypeDist;
    std::uniform_real_distribution<float> xSpacingDist;

    std::vector<std::shared_ptr<PickUp>> pickups;

    // Pan‐scroll speed for pickups (must match how the level scrolls)
    static constexpr float PickupPanSpeed = 80.0f;



    struct CactusInfo {
        const char* spriteId;  // which resource to use
        int width;             // sprite frame width
        int height;            // sprite frame height
    };

    inline static const CactusInfo CACTUS_NORMAL = { Resources::DancingCacti,       64, 90 };
    inline static const CactusInfo CACTUS_SMALL = { Resources::DancingCactiSmall,  64, 64 };
    inline static const CactusInfo CACTUS_COWBOY = { Resources::DancingCactiCowboy, 64, 90 };

    float spacing = 300.0f;

    // Track which pairs of outhouses have spawned pickups this pass
    std::unordered_set<int> spawnedPickupsIndices;
};
