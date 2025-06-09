#include "CastleLevel.h"
#include "Resources.h"
#include "GameSettings.h"
#include "Coin.h"
#include "PoopHeart.h"
#include <raymath.h>
#include <algorithm>

using namespace Resources;
using namespace GameSettings;

CastleLevel::CastleLevel()
{
    camera = new CameraSystem();

    camera->AddLayer(new ParallaxLayer({ CastleBackgroundWall }, 80.f, 1.f));
    camera->AddLayer(new ParallaxLayer({ CastleBackgroundBars }, 80.f, 1.f));

    levelMusicSlow = new AudioClip(LevelFiveSlow);
    levelMusicRegular = new AudioClip(LevelFour);
    levelMusicFast = new AudioClip(LevelFiveFast);
    currentMusic = levelMusicRegular;

    InitDecoration();
    InitObstacles();
    InitPickups();

    hasPassedFirstToilet = false;
    hasSpawnedFirstGapPickups = false;
}

CastleLevel::~CastleLevel()
{
    delete camera;
}

void CastleLevel::InitDecoration()
{
    const char* paints[] = { PaintingA, PaintingB, PaintingC, PaintingD };
    paintings.resize(GAP_COUNT);
    for (size_t i = 0; i < paintings.size(); ++i) {
        paintings[i] = std::make_shared<Sprite>(
            paints[std::uniform_int_distribution<int>(0, 3)(rng)], 1);
        float x = i * spacing;
        paintings[i]->SetPosition({ x - 16.f, 48.f });
    }

    curtains.resize(GAP_COUNT);
    for (size_t i = 0; i < curtains.size(); ++i) {
        curtains[i] = std::make_shared<Sprite>(Curtains, 1);
    }
}

void CastleLevel::InitObstacles()
{
    float x = 0.f;
    toilets.reserve(GAP_COUNT);
    spikes.resize(GAP_COUNT);
    pillars.resize(GAP_COUNT);
    chandeliers.resize(GAP_COUNT);
    floorTorches.reserve(GAP_COUNT * 2);

    const float torchY = GameHeight - 64.f - 21.f;
    const float spikeY = 90.f;
    const float toiletWidth = 45.f;

    for (int i = 0; i < GAP_COUNT; ++i, x += spacing)
    {
        auto loo = std::make_shared<GoldToilets>(static_cast<int>(x), 0);
        toilets.push_back(loo);
        obstacles.push_back(loo);

        curtains[i]->SetPosition({ x - 32.f, 21.f });

        auto torchL = std::make_shared<Sprite>(FloorTorch, 4, 0.1f, 1.f);
        auto torchR = std::make_shared<Sprite>(FloorTorch, 4, 0.1f, 1.f);
        torchL->SetPosition({ x - 37.f + (80.f - toiletWidth) / 2.f, torchY });
        torchR->SetPosition({ x + 85.f - (80.f - toiletWidth) / 2.f, torchY });
        floorTorches.push_back(torchL);
        floorTorches.push_back(torchR);

        foreground.push_back(torchL);
        foreground.push_back(torchR);

        float chandelierPos = x + (spacing - toiletWidth) / 2.f + toiletWidth - 16.f;
        auto cndl = std::make_shared<Sprite>(Chandelier, 4, 0.12f, 1.f);
        cndl->SetPosition({ chandelierPos, 21.f });
        chandeliers[i] = cndl;

        float toiletCentre = x + (spacing + toiletWidth) / 2.f - 16.f;
        if (i % 2 == 1)
        {
            auto spk = std::make_shared<SpikeBall>(Vector2{ toiletCentre, spikeY });
            spikes[i] = spk;
            obstacles.push_back(spk);
        }
        else
        {
            auto pil = std::make_shared<Sprite>(TorchPillar, 4, 0.15f, 1.f);
            pil->SetPosition({ toiletCentre, 21.f });
            pillars[i] = pil;
        }
    }
}

void CastleLevel::InitPickups()
{
    float currentX = 0.f;
    for (int i = 2; i < toilets.size(); ++i) {
        float xStart = toilets[i - 1]->pos.x + 45.f;
        float xEnd = toilets[i]->pos.x;
        SpawnPickupsBetween(xStart, xEnd);
    }
}

void CastleLevel::SpawnPickupsBetween(float xStart, float xEnd)
{
    const int count = 5;
    const float yOffset = -40.0f;
    const float yLow = 60.0f + yOffset;
    const float yHigh = 120.0f + yOffset;
    const float topY = 10.0f;
    const float bottomY = GameHeight - 20.0f;
    const float xShift = -10.0f;

    int pattern = GetRandomValue(0, 6);

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
        case 5: y = topY; break;
        case 6: y = bottomY; break;
        default: y = (yLow + yHigh) * 0.5f; break;
        }

        Vector2 pos{ x, y };
        SpawnPickup(pos);
    }
}

void CastleLevel::SpawnPickup(Vector2 pos)
{
    int roll = GetRandomValue(1, 1000);
    std::shared_ptr<PickUp> pickup;

    if (roll <= 20) {
        pickup = std::make_shared<Coin>(pos, CoinType::REDCOIN); // 2%
    }
    else if (roll <= 50) {
        pickup = std::make_shared<PoopHeart>(pos, PoopHeartType::BIG); // 3%
    }
    else if (roll <= 150) {
        pickup = std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL); // 10%
    }
    else if (roll <= 400) {
        pickup = std::make_shared<Coin>(pos, CoinType::BLUECOIN); // 25%
    }
    else {
        pickup = std::make_shared<Coin>(pos, CoinType::GOLDCOIN); // 60%
    }

    pickup->SetPanSpeed(pickupPanSpeed);
    pickups.push_back(pickup);
}

void CastleLevel::ScrollSprite(std::shared_ptr<Sprite>& s, float dt) const
{
    if (!s) return;
    Vector2 pos = s->GetPosition();
    pos.x -= pickupPanSpeed * dt;
    s->SetPosition(pos);
    s->Update(dt);
}

void CastleLevel::ScrollSpriteVec(std::vector<std::shared_ptr<Sprite>>& v, float dt) const
{
    for (auto& s : v) {
        if (!s) continue;
        ScrollSprite(s, dt);
    }
}

void CastleLevel::Update(float dt)
{
    const float torchY = 180 - 64.f - 21.f;
    const float toiletWidth = 45.f;
    static float swingTimer = 0.0f;
    const float swingFrequency = 1.0f;
    const float swingAmplitude = 20.0f;

    swingTimer += dt * swingFrequency;

    if (!currentMusic->IsPlaying()) currentMusic->Play();
    currentMusic->Update();
    camera->Update(dt);

    if (!hasPassedFirstToilet && lastPlayerPosition.x > toilets[0]->pos.x + toiletWidth) {
        hasPassedFirstToilet = true;
    }

    if (hasPassedFirstToilet && !hasSpawnedFirstGapPickups) {
        float xStart = toilets[0]->pos.x + 45.f;
        float xEnd = toilets[1]->pos.x;
        SpawnPickupsBetween(xStart, xEnd);
        hasSpawnedFirstGapPickups = true;
    }

    for (size_t i = 0; i < toilets.size(); ++i)
    {
        auto& loo = toilets[i];
        loo->Update(dt);

        if (i % 2 == 0) {
            curtains[i]->SetPosition({ loo->pos.x - 32.f, 21.f });
            paintings[i]->SetPosition({ loo->pos.x - 16.f, 48.f });
        }
        else {
            paintings[i]->SetPosition({ loo->pos.x - 16.f, 48.f });
            curtains[i]->SetPosition({ loo->pos.x - 32.f, 21.f });
        }

        floorTorches[i * 2]->SetPosition({ loo->pos.x - 37.f + (80.f - toiletWidth) / 2.f, torchY });
        floorTorches[i * 2 + 1]->SetPosition({ loo->pos.x + 85.f - (80.f - toiletWidth) / 2.f, torchY });

        float centre = loo->pos.x + (spacing + toiletWidth) / 2.f - 16.f;
        if (chandeliers[i]) {
            float chandelierPos = loo->pos.x + (spacing - toiletWidth) / 2.f + toiletWidth - 16.f;
            chandeliers[i]->SetPosition({ chandelierPos, 21.f });
        }
        if (spikes[i]) {
            float baseY = 90.f;
            float swingOffset = sinf(swingTimer) * swingAmplitude;
            spikes[i]->SetPosition({ centre, baseY + swingOffset });
            spikes[i]->Update(dt);
        }
        else if (pillars[i]) {
            pillars[i]->SetPosition({ centre, 21.f });
        }

        if (loo->pos.x + toiletWidth < -spacing / 2.f)
        {
            size_t prev = (i == 0 ? toilets.size() - 1 : i - 1);
            loo->pos.x = toilets[prev]->pos.x + spacing;
            loo->resetScore();
            float xStart = toilets[prev]->pos.x + 45.f;
            float xEnd = loo->pos.x;
            SpawnPickupsBetween(xStart, xEnd);
        }

        curtains[i]->Update(dt);
        paintings[i]->Update(dt);
        floorTorches[i * 2]->Update(dt);
        floorTorches[i * 2 + 1]->Update(dt);
        if (chandeliers[i]) chandeliers[i]->Update(dt);
        if (spikes[i]) spikes[i]->Update(dt);
        else if (pillars[i]) pillars[i]->Update(dt);
    }

    for (auto& pickup : pickups) {
        if (!pickup->IsCollected()) {
            pickup->Update(dt);
        }
    }

    std::sort(floorTorches.begin(), floorTorches.end(),
        [](const auto& a, const auto& b) {
            return a->GetPosition().x < b->GetPosition().x;
        });
}

void CastleLevel::Draw() const
{
    camera->Draw();

    for (size_t i = 0; i < curtains.size(); ++i) {
        if (i % 2 == 0) curtains[i]->Draw();
    }
    for (size_t i = 0; i < paintings.size(); ++i) {
        if (i % 2 == 1) paintings[i]->Draw();
    }
    for (const auto& s : pillars) if (s) s->Draw();
    for (const auto& s : chandeliers) s->Draw();

    for (const auto& s : foreground) s->Draw();

    for (const auto& o : obstacles) o->Draw();

    for (const auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Draw();
    }
}

bool CastleLevel::checkForCollisions(Vector2 c, float r)
{
    for (auto& loo : toilets)
        if (CheckCollisionCircleRec(c, r, loo->GetTopHitbox()) ||
            CheckCollisionCircleRec(c, r, loo->GetBottomHitbox()))
            return true;
    for (auto& spk : spikes)
        if (spk && CheckCollisionCircleRec(c, r, spk->GetHitbox()))
            return true;
    return false;
}

bool CastleLevel::checkForPointGain(Vector2 c, float /*r*/)
{
    float px = c.x;

    for (auto& loo : toilets)
    {
        float tx = loo->GetTopHitbox().x;
        float txRight = tx + loo->GetTopHitbox().width;

        if (!loo->hasScored && px > txRight && px < txRight + 2.0f)
        {
            loo->hasScored = true;
            return true;
        }
    }

    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& CastleLevel::getObjLoc()
{
    return obstacles;
}

std::vector<std::shared_ptr<PickUp>>& CastleLevel::GetPickUps()
{
    return pickups;
}

void CastleLevel::SetSwingingPipes(bool enable)
{
    for (auto& toilet : toilets) {
        toilet->SetOscillationEnabled(enable);
    }
}

void CastleLevel::SetDifficulty(int difficultyIndex)
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

void CastleLevel::SetPanSpeed(float speed)
{
    pickupPanSpeed = speed;
    for (auto& toilet : toilets) {
        toilet->SetPanSpeed(speed);
    }
    for (auto& spike : spikes) {
        if (spike) {
            spike->SetPanSpeed(speed);
        }
    }
    for (auto& pickup : pickups) {
        pickup->SetPanSpeed(speed);
    }
    // Enemies are updated via LevelManager
}