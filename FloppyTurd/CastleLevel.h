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

    void Draw() const override;
    void Update(float dt) override;

    bool checkForCollisions(Vector2 c, float r) override;
    bool checkForPointGain(Vector2 c, float r) override;
    const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() override;
    std::vector<std::shared_ptr<PickUp>>& GetPickUps() override;

    void SetSwingingPipes(bool enable) override;

private:
    void InitDecoration();
    void InitObstacles();
    void InitPickups();
    void SpawnPickupsBetween(float xStart, float xEnd);
    void SpawnPickup(Vector2 pos);

    void ScrollSprite(std::shared_ptr<Sprite>& s, float dt) const;
    void ScrollSpriteVec(std::vector<std::shared_ptr<Sprite>>& v, float dt) const;

    CameraSystem* camera = nullptr;
    AudioClip* music = nullptr;

    static constexpr int GAP_COUNT = 5;
    const float spacing = 300.f;

    std::vector<std::shared_ptr<GoldToilets>> toilets;
    std::vector<std::shared_ptr<SpikeBall>> spikes;
    std::vector<std::shared_ptr<Sprite>> pillars;
    std::vector<std::shared_ptr<Sprite>> chandeliers;
    std::vector<std::shared_ptr<Sprite>> floorTorches;

    std::vector<std::shared_ptr<Sprite>> curtains;
    std::vector<std::shared_ptr<Sprite>> paintings;

    std::vector<std::shared_ptr<Obstacle>> obstacles;
    std::vector<std::shared_ptr<PickUp>> pickups;

    std::vector<std::shared_ptr<Sprite>> midground;
    std::vector<std::shared_ptr<Sprite>> foreground;

    std::mt19937 rng{ std::random_device{}() };

    bool hasPassedFirstToilet = false;
    bool hasSpawnedFirstGapPickups = false;
    Vector2 lastPlayerPosition = { 150, 90 };
};