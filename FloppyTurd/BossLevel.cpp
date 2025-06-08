#include "BossLevel.h"
#include "Resources.h"
#include "RatKing.h"
#include "GameSettings.h"
#include "ParallaxLayer.h"
#include "StaticLayer.h"
#include "AnimatedLayer.h"
#include "Coin.h"
#include "PoopHeart.h"
#include <raymath.h>

BossLevel::BossLevel()
    : engine(std::random_device{}())
    , pickupTypeDist(0, 99)
    , pickupCountDist(3, 5)
    , spawnIntervalDist(5.0f, 8.0f)
    , phaseDist(0.0f, 6.2832f) // 0 to 2π
{
    using namespace Resources;
    using namespace GameSettings;

    cameraSystem = new CameraSystem();
    InitCamera();
    InitLayers();

    boss = std::make_shared<RatKing>(Vector2{ 160.0f, 40.0f });
    TraceLog(LOG_INFO, "[BossLevel] Created new RatKing at (160, 40)");
    levelMusic = new AudioClip(LevelFive);  // Use BossLevel.mp3
    lowHealthMusic = new AudioClip(BossLowHealth);

    // Prime the stream silently
    lowHealthMusic->SetVolume(0.0f);
    lowHealthMusic->Play();
    lowHealthMusic->Update();  // This primes the stream buffers
    lowHealthMusic->Stop();
    lowHealthMusic->SetVolume(1.0f);  // Reset for actual use

    // Set initial pickup spawn interval
    pickupSpawnInterval = spawnIntervalDist(engine);
}

BossLevel::~BossLevel()
{
    TraceLog(LOG_INFO, "[BossLevel] Destroying BossLevel, boss ref count: %d", boss.use_count());
    boss.reset(); // Explicitly clear boss
    delete cameraSystem;
}

void BossLevel::Reset()
{
    TraceLog(LOG_INFO, "[BossLevel] Resetting BossLevel, old boss ref count: %d", boss.use_count());
    boss.reset(); // Clear old boss
    boss = std::make_shared<RatKing>(Vector2{ 160.0f, 40.0f });
    TraceLog(LOG_INFO, "[BossLevel] Created new RatKing at (160, 40)");
    pickups.clear(); // Clear pickups
    pickupSpawnTimer = 0.0f; // Reset wave timer
    pickupSpawnInterval = spawnIntervalDist(engine); // Randomize next wave
    lowHealthTriggerTime = 0.0;
    lowHealthPlayTime = 0.0;
    hasRecordedLowHealthPlay = false;
}

void BossLevel::Draw() const
{
    if (cameraSystem) cameraSystem->Draw();
    if (boss && boss->isActive) boss->Draw();

    // Draw pickups if not collected
    for (const auto& pickup : pickups)
    {
        if (!pickup->IsCollected())
            pickup->Draw();
    }
}

void BossLevel::Update(float deltaTime)
{
    if (!lowHealthMusic->IsPlaying())
    {
        if (!levelMusic->IsPlaying()) levelMusic->Play();
        levelMusic->Update();
    }

    if (cameraSystem) cameraSystem->Update(deltaTime);
    if (boss)
    {
        boss->Update(deltaTime);

        if (std::shared_ptr<RatKing> rk = std::dynamic_pointer_cast<RatKing>(boss))
        {
            double rkTriggerTime = rk->GetLowHealthTriggerTime();
            if (rkTriggerTime > 0.0 && lowHealthTriggerTime == 0.0)
            {
                lowHealthTriggerTime = rkTriggerTime;
                TraceLog(LOG_INFO, "RatKing low health trigger from RK: %.5f", lowHealthTriggerTime);
            }

            if (rk->IsInLowHealthMode())
            {
                if (levelMusic->IsPlaying())
                {
                    levelMusic->Stop();  // Stop normal music only once
                }

                if (!lowHealthMusic->IsPlaying())
                {
                    TraceLog(LOG_INFO, "PLAYING BOSS LOW HEALTH MUSIC NOW!");
                    lowHealthMusic->Play();

                    if (!hasRecordedLowHealthPlay && lowHealthMusic->IsPlaying())
                    {
                        lowHealthPlayTime = GetTime();
                        hasRecordedLowHealthPlay = true;

                        TraceLog(LOG_INFO, "Low health music PLAYED at %.5f (delay: %.5f seconds)",
                            lowHealthPlayTime, lowHealthPlayTime - lowHealthTriggerTime);
                    }

                    // Immediately update a few times to fill the stream buffer
                    for (int i = 0; i < 3; ++i)
                        lowHealthMusic->Update();
                }

                lowHealthMusic->Update();  // Keep streaming
            }
            else
            {
                levelMusic->Update();  // Still in normal mode
            }
        }
    }

    lowHealthMusic->Update();  // Keep streaming

    // Update pickup wave spawning
    pickupSpawnTimer += deltaTime;
    if (pickupSpawnTimer >= pickupSpawnInterval)
    {
        SpawnPickupWave();
        pickupSpawnTimer = 0.0f;
        pickupSpawnInterval = spawnIntervalDist(engine); // Randomize next interval
    }

    // Update pickups with standing wave motion and remove if off-screen or collected
    for (auto it = pickups.begin(); it != pickups.end(); )
    {
        auto& pickup = *it;
        pickup->Update(deltaTime);

        // Apply standing wave motion
        float timeAlive = pickup->GetTimeAlive();
        float phase = pickup->GetWavePhase();
        float y = 90.0f + waveAmplitude * sinf(waveFrequency * timeAlive + phase);
        // Clamp y to safe bounds (40-140)
        y = std::max(40.0f, std::min(140.0f, y));
        Vector2 pos = pickup->GetPosition();
        pos.y = y;
        pickup->SetPosition(pos);

        if (pickup->ShouldBeRemoved())
            it = pickups.erase(it);
        else
            ++it;
    }
}

void BossLevel::InitCamera()
{
    // No need for initialization beyond creation, as layers will be added in InitLayers
}

void BossLevel::InitLayers()
{
    using namespace Resources;
    using namespace GameSettings;

    // Add layers to CameraSystem with appropriate speeds
    cameraSystem->AddLayer(new StaticLayer(
        BossBackground,  // Background (slowest)
        Vector2{ 0,0 },  // Position
        1.0f   // Scale
    ));

    cameraSystem->AddLayer(new StaticLayer(
        BossDarkClouds,  // Clouds (moderate speed)
        Vector2{ 0,0 },  // Position
        1.0f
    ));

    cameraSystem->AddLayer(new StaticLayer(
        BossFloor,  // Floor and walls (faster)
        Vector2{ 0,0 },  // Position
        1.0f
    ));

    cameraSystem->AddLayer(new StaticLayer(
        BossCurtains,  // Curtains (fastest)
        Vector2{ 0,0 },
        1.0f
    ));

    cameraSystem->AddLayer(new StaticLayer(
        BossWalls,
        Vector2{ 0,0 },  // Position
        1.0f
    ));

    // Pillar (animated, treated as a fast-moving layer for effect)
    cameraSystem->AddLayer(new AnimatedLayer(
        BossPillar,  // Animated pillar (4 frames for rotation)
        15,
        0.2f,
        Vector2{ 0,0 },  // Position
        1.0f
    ));
}

bool BossLevel::checkForCollisions(Vector2 circleCenter, float circleRadius)
{
    if (boss && boss->isActive)
    {
        for (const auto& hitbox : boss->GetHitboxes())
        {
            if (CheckCollisionCircleRec(circleCenter, circleRadius, hitbox))
                return true;
        }
    }
    return false;
}

bool BossLevel::checkForPointGain(Vector2 circleCenter, float circleRadius)
{
    // No point gain in boss level (focus on defeating boss)
    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& BossLevel::getObjLoc()
{
    static std::vector<std::shared_ptr<Obstacle>> empty;
    return empty;  // No obstacles, only boss
}

void BossLevel::SpawnPickupWave()
{
    const int count = pickupCountDist(engine); // 3-5 pickups
    const float xSpacing = 32.0f; // Horizontal spacing between pickups

    // Spawn area: right side of screen, safe from boss
    float spawnXBase = 300.0f; // Just off-screen right

    for (int i = 0; i < count; ++i)
    {
        float x = spawnXBase + i * xSpacing;
        float y = 90.0f; // Start at center (will oscillate)
        Vector2 pos{ x, y };
        float phase = phaseDist(engine); // Random phase for wave effect

        auto pickup = GenerateRandomPickup(pos);
        pickup->SetPanSpeed(80.0f); // Match level scroll speed
        pickup->SetWavePhase(phase); // Store phase for wave motion
        pickups.push_back(pickup);
    }
}

std::shared_ptr<PickUp> BossLevel::GenerateRandomPickup(Vector2 pos)
{
    int roll = pickupTypeDist(engine);
    if (roll < 40) // 40%: Blue Coin
        return std::make_shared<Coin>(pos, CoinType::BLUECOIN);
    else if (roll < 70) // 30%: Red Coin
        return std::make_shared<Coin>(pos, CoinType::REDCOIN);
    else if (roll < 90) // 20%: Small Heart
        return std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL);
    else if (roll < 98) // 8%: Big Heart
        return std::make_shared<PoopHeart>(pos, PoopHeartType::BIG);
    else // 2%: Invisible Heart
        return std::make_shared<PoopHeart>(pos, PoopHeartType::INVISIBLE);
}