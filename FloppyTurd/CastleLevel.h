#pragma once
#include "Level.h"
#include "CameraSystem.h"
#include "ParallaxLayer.h"
#include "GoldToilets.h"
#include "SpikeBall.h"
#include "Sprite.h"
#include "Coin.h"
#include "PoopHeart.h"
#include <vector>
#include <memory>
#include <random>

class CastleLevel : public Level
{
public:
    CastleLevel();
    ~CastleLevel();

    void Draw()  const override;
    void Update(float dt) override;

    bool checkForCollisions(Vector2 c, float r)        override;
    bool checkForPointGain(Vector2 c, float r)        override;
    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override;

private:
    void InitDecoration();
    void InitObstacles();
    void InitPickups();
    void SpawnPickupsBetween(float xStart, float xEnd);
    void SpawnPickup(Vector2 pos);

    // helpers ------------------------------------------------------------- 
    void ScrollSprite(std::shared_ptr<Sprite>& s, float dt) const;
    void ScrollSpriteVec(std::vector<std::shared_ptr<Sprite>>& v, float dt) const;

    // -------------------------------------------------------------------- 
    CameraSystem* camera = nullptr;
    AudioClip* music = nullptr;

    static constexpr int    GAP_COUNT = 5;
    const    float          spacing = 300.f;

    std::vector<std::shared_ptr<GoldToilets>> toilets;        // 5
    std::vector<std::shared_ptr<SpikeBall>>   spikes;         // 5 (some may stay off-screen)
    std::vector<std::shared_ptr<Sprite>>      pillars;        // 5
    std::vector<std::shared_ptr<Sprite>>      chandeliers;    // 5
    std::vector<std::shared_ptr<Sprite>>      floorTorches;   // 10 (2 per toilet)

    std::vector<std::shared_ptr<Sprite>> curtains;  // One curtain per toilet
    std::vector<std::shared_ptr<Sprite>> paintings; // Multiple paintings

    std::vector<std::shared_ptr<Obstacle>> obstacles; // toilets + active spikes
    std::vector<std::shared_ptr<PickUp>> pickups;     // coins and hearts

    /* render helpers */
    std::vector<std::shared_ptr<Sprite>> midground;   // curtains, paintings, chandeliers, pillars
    std::vector<std::shared_ptr<Sprite>> foreground;  // floor torches

    std::mt19937 rng{ std::random_device{}() };

    // Flags for delayed spawning
    bool hasPassedFirstToilet = false;
    bool hasSpawnedFirstGapPickups = false;
    Vector2 lastPlayerPosition = { 150, 90 }; // Default player position, updated by LevelManager
};