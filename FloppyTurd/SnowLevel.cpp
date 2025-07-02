#include "SnowLevel.h"
#include "ResourceCompat.h"
#include "Coin.h"
#include "PoopHeart.h"
#include <raymath.h>
#include "GameSettings.h"

SnowLevel::SnowLevel()
{
    using namespace Resources;
    using namespace GameSettings;
    cameraSystem = new CameraSystem();

    cameraSystem->AddLayer(new ParallaxLayer({ SnowBackground }, 1.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ SnowMountains }, 10.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ SnowBackTrees }, 60.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ SnowTundra }, 60.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ SnowFrontTrees }, 60.0f, 1.0f));

    levelMusicSlow = new AudioClip(LevelFourSlow);
    levelMusicRegular = new AudioClip(SnowLevelMusic);
    levelMusicFast = new AudioClip(LevelFourFast);
    currentMusic = levelMusicRegular;

    InitObstacles();
}

SnowLevel::~SnowLevel()
{
    delete cameraSystem;
}

void SnowLevel::Draw() const
{
    cameraSystem->Draw();

    for (const auto& obstacle : obstacles) {
        obstacle->Draw();
    }

    for (auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Draw();
    }
}

void SnowLevel::Update(float deltaTime)
{
    cameraSystem->Update(deltaTime);
    if (!currentMusic->IsPlaying()) currentMusic->Play();
    currentMusic->Update();

    // Update obstacles (toilets)
    for (int i = 0; i < (int)toilets.size(); ++i)
    {
        auto& oh = toilets[i];
        oh->Update(deltaTime);
        if (oh->pos.x + 80 < 0)
        {
            int lastIndex = (i - 1 < 0) ? (toilets.size() - 1) : (i - 1);
            oh->pos.x = toilets[lastIndex]->pos.x + spacing;
            oh->resetScore();

            float xStart = toilets[lastIndex]->pos.x + 80.0f;
            float xEnd = oh->pos.x;
            SpawnPickupsBetween(xStart, xEnd);
        }
    }

    // Move and update pickups every frame
    for (auto& pickup : pickups) {
        if (!pickup->IsCollected()) {
            pickup->Update(deltaTime);
        }
    }
}

void SnowLevel::SpawnPickupsBetween(float xStart, float xEnd)
{
    const int count = 5;
    const float yOffset = -40.0f;
    float yLow = 60.0f + yOffset;
    float yHigh = 120.0f + yOffset;
    float xShift = -10.0f;

    int pattern = GetRandomValue(0, 4);

    for (int i = 0; i < count; ++i)
    {
        float t = (count == 1 ? 0.0f : (float)i / (count - 1));
        float x = xStart + t * (xEnd - xStart) + xShift;

        float y;
        switch (pattern)
        {
        case 0: y = (yLow + yHigh) * 0.5f; break;
        case 1: y = yLow + t * (yHigh - yLow); break;
        case 2: y = yHigh + t * (yLow - yHigh); break;
        case 3: y = yHigh - fabsf(t - 0.5f) * (yHigh - yLow) * 2.0f; break;
        case 4: y = yLow + fabsf(t - 0.5f) * (yHigh - yLow) * 2.0f; break;
        default: y = (yLow + yHigh) * 0.5f; break;
        }

        Vector2 pos{ x, y };

        int roll = GetRandomValue(1, 1000);
        std::shared_ptr<PickUp> pickup;

        if (roll <= 10) {
            pickup = std::make_shared<PoopHeart>(pos, PoopHeartType::BIG); // 1%
        }
        else if (roll <= 110) {
            pickup = std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL); // 10%
        }
        else if (roll <= 360) {
            pickup = std::make_shared<Coin>(pos, CoinType::BLUECOIN); // 25%
        }
        else {
            pickup = std::make_shared<Coin>(pos, CoinType::GOLDCOIN); // 64%
        }

        pickup->SetPanSpeed(pickupPanSpeed);
        pickups.push_back(pickup);
    }
}

void SnowLevel::InitObstacles()
{
    toilets.clear();
    obstacles.clear();

    float currentX = static_cast<float>(320);

    for (int i = 0; i < 5; i++)
    {
        auto oh = std::make_shared<ToiletPair>(currentX, 0, true);
        toilets.push_back(oh);
        obstacles.push_back(std::static_pointer_cast<Obstacle>(oh));
        currentX += spacing;
    }

    for (int i = 1; i < toilets.size(); ++i) {
        float xStart = toilets[i - 1]->pos.x + 80.0f;
        float xEnd = toilets[i]->pos.x;
        SpawnPickupsBetween(xStart, xEnd);
    }
}

bool SnowLevel::checkForCollisions(Vector2 circleCenter, float circleRadius)
{
    for (auto& t : toilets)
    {
        for (auto& hitbox : t->GetHitboxes()) {
            if (CheckCollisionCircleRec(circleCenter, circleRadius, hitbox))
                return true;
        }
    }
    return false;
}

bool SnowLevel::checkForPointGain(Vector2 circleCenter, float circleRadius)
{
    float playerCenterX = circleCenter.x;
    bool scored = false;

    for (auto& t : toilets)
    {
        float passLine = t->GetTopHitbox().x + t->GetTopHitbox().width;
        if (!t->hasScored && playerCenterX > passLine && playerCenterX < passLine + 2.0f)
        {
            t->hasScored = true;
            scored = true;
        }
    }

    return scored;
}

const std::vector<std::shared_ptr<Obstacle>>& SnowLevel::getObjLoc()
{
    return obstacles;
}

void SnowLevel::SetSwingingPipes(bool enable)
{
    for (auto& toilet : toilets) {
        toilet->SetOscillationEnabled(enable);
    }
}

void SnowLevel::SetDifficulty(int difficultyIndex)
{
    switch (difficultyIndex) {
    case 0: currentMusic = levelMusicSlow; break;
    case 1: currentMusic = levelMusicRegular; break;
    case 2: currentMusic = levelMusicFast; break;
    default: currentMusic = levelMusicRegular; break;
    }
    if (currentMusic) {
        currentMusic->Stop();
        currentMusic->Play();
    }
}

void SnowLevel::SetPanSpeed(float speed)
{
    pickupPanSpeed = speed;
    for (auto& toilet : toilets) {
        toilet->SetPanSpeed(speed);
    }
    for (auto& pickup : pickups) {
        pickup->SetPanSpeed(speed);
    }
}
