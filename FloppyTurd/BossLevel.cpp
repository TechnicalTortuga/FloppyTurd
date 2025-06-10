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
#include "AudioManager.h"

BossLevel::BossLevel()
    : engine(std::random_device{}()),
    pickupTypeDist(0, 99),
    pickupCountDist(3, 5),
    spawnIntervalDist(5.0f, 8.0f),
    phaseDist(0.0f, 6.2832f)
{
    using namespace Resources;
    using namespace GameSettings;

    cameraSystem = new CameraSystem();
    InitCamera();
    InitLayers();

    boss = std::make_shared<RatKing>(Vector2{ 160.0f, 40.0f });
    TraceLog(LOG_INFO, "[BossLevel] Created new RatKing at (160, 40)");

    levelMusicSlow = new AudioClip(BossLevelSlow);
    levelMusicRegular = new AudioClip(LevelFive);
    levelMusicFast = new AudioClip(BossLevelFast);
    currentMusic = levelMusicRegular;

    lowHealthMusic = new AudioClip(BossLowHealth);
    lowHealthMusic->SetVolume(0.0f);
    lowHealthMusic->Play();
    lowHealthMusic->Update();
    lowHealthMusic->Stop();
    lowHealthMusic->SetVolume(1.0f);

    // Preload BossBeat sound
    AudioManager::GetInstance().LoadSoundEffect("BossBeat", BossBeat);

    pickupSpawnInterval = spawnIntervalDist(engine);
    SetPanSpeed(80.0f); // Default pan speed for pickups
}

BossLevel::~BossLevel()
{
    TraceLog(LOG_INFO, "[BossLevel] Destroying BossLevel, boss ref count: %d", boss.use_count());
    boss.reset();
    delete cameraSystem;
    delete lowHealthMusic;
    explosions.clear(); // Clean up explosions
}

void BossLevel::Reset()
{
    TraceLog(LOG_INFO, "[BossLevel] Resetting BossLevel, old boss ref count: %d", boss.use_count());
    boss.reset();
    boss = std::make_shared<RatKing>(Vector2{ 160.0f, 40.0f });
    TraceLog(LOG_INFO, "[BossLevel] Created new RatKing at (160, 40)");
    pickups.clear();
    pickupSpawnTimer = 0.0f;
    pickupSpawnInterval = spawnIntervalDist(engine);
    lowHealthTriggerTime = 0.0;
    lowHealthPlayTime = 0.0;
    hasRecordedLowHealthPlay = false;
    SetPanSpeed(80.0f);
    deathSequenceActive = false;
    deathSequenceTimer = 0.0f;
    explosions.clear();
    isComplete = false;
}

void BossLevel::Draw() const
{
    if (cameraSystem) cameraSystem->Draw();
    if (boss && (boss->isActive || deathSequenceActive)) boss->Draw(); // Keep RatKing visible during death sequence

    for (const auto& pickup : pickups)
    {
        if (!pickup->IsCollected())
            pickup->Draw();
    }

    // Draw explosions on top of RatKing
    for (const auto& explosion : explosions)
    {
        explosion->Draw();
    }

    // Draw fade-to-white effect over everything (player and UI included)
    if (deathSequenceActive && deathSequenceTimer > 1.5f)
    {
        float alpha = (deathSequenceTimer - 1.5f) / 1.0f; // 1-second fade
        alpha = Clamp(alpha, 0.0f, 1.0f);
        DrawRectangle(0, 0, 320, 180, Fade(WHITE, alpha));
    }
}

void BossLevel::Update(float deltaTime)
{
    if (deathSequenceActive)
    {
        // Update explosions and timer during death sequence
        for (auto it = explosions.begin(); it != explosions.end();)
        {
            (*it)->Update(deltaTime);
            if ((*it)->IsComplete())
                it = explosions.erase(it);
            else
                ++it;
        }
        deathSequenceTimer += deltaTime;

        // Start fade after 1.5 seconds, complete level after 2.5 seconds
        if (deathSequenceTimer >= 2.5f)
        {
            isComplete = true;
            if (std::shared_ptr<RatKing> rk = std::dynamic_pointer_cast<RatKing>(boss))
            {
                rk->isActive = false; // Deactivate RatKing only after sequence
            }
        }
        return; // Skip other updates during death sequence
    }

    if (cameraSystem) cameraSystem->Update(deltaTime);

    if (boss)
    {
        boss->Update(deltaTime);

        // Check for Boss death to start sequence
        if (boss->GetCurrentState() == Boss::DEATH && !deathSequenceActive)
        {
            deathSequenceActive = true;
            deathSequenceTimer = 0.0f;
            TraceLog(LOG_INFO, "[BossLevel] Starting death sequence for Boss");

            // Play BossBeat sound
            AudioManager::GetInstance().PlaySoundEffect("BossBeat", 1.0f);

            // Spawn explosions at random positions within 40-pixel radius
            Vector2 bossCenter = { boss->position.x + 64, boss->position.y + 64 }; // Center of 128x128 sprite
            std::uniform_real_distribution<float> radiusDist(0.0f, 40.0f);
            std::uniform_real_distribution<float> angleDist(0.0f, 2 * PI);

            // Explosion 1: BlastSmall at 0.0s
            float r = radiusDist(engine);
            float a = angleDist(engine);
            Vector2 pos1 = { bossCenter.x + r * cosf(a), bossCenter.y + r * sinf(a) };
            explosions.push_back(std::make_shared<Explosion>(Resources::BlastSmall, pos1, 1.0f));

            // Explosion 2: BlastSmall at 0.29s (second beat)
            r = radiusDist(engine);
            a = angleDist(engine);
            Vector2 pos2 = { bossCenter.x + r * cosf(a), bossCenter.y + r * sinf(a) };
            explosions.push_back(std::make_shared<Explosion>(Resources::BlastSmall, pos2, 1.0f));

            // Explosion 3: BlastBig at 0.58s (third beat)
            r = radiusDist(engine);
            a = angleDist(engine);
            Vector2 pos3 = { bossCenter.x + r * cosf(a), bossCenter.y + r * sinf(a) };
            explosions.push_back(std::make_shared<Explosion>(Resources::BlastBig, pos3, 1.0f));
        }

        // Handle low health music logic
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
                if (currentMusic->IsPlaying()) currentMusic->Stop();

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

                    for (int i = 0; i < 3; ++i)
                        lowHealthMusic->Update();
                }

                lowHealthMusic->Update();
            }
            else
            {
                if (!currentMusic->IsPlaying()) currentMusic->Play();
                currentMusic->Update();
            }
        }
    }

    pickupSpawnTimer += deltaTime;
    if (pickupSpawnTimer >= pickupSpawnInterval)
    {
        SpawnPickupWave();
        pickupSpawnTimer = 0.0f;
        pickupSpawnInterval = spawnIntervalDist(engine);
    }

    for (auto it = pickups.begin(); it != pickups.end(); )
    {
        auto& pickup = *it;
        pickup->Update(deltaTime);

        float timeAlive = pickup->GetTimeAlive();
        float phase = pickup->GetWavePhase();
        float y = 90.0f + waveAmplitude * sinf(waveFrequency * timeAlive + phase);
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

void BossLevel::InitCamera() {}

void BossLevel::InitLayers()
{
    using namespace Resources;
    using namespace GameSettings;

    cameraSystem->AddLayer(new StaticLayer(BossBackground, Vector2{ 0, 0 }, 1.0f));
    cameraSystem->AddLayer(new StaticLayer(BossDarkClouds, Vector2{ 0, 0 }, 1.0f));
    cameraSystem->AddLayer(new StaticLayer(BossFloor, Vector2{ 0, 0 }, 1.0f));
    cameraSystem->AddLayer(new StaticLayer(BossCurtains, Vector2{ 0, 0 }, 1.0f));
    cameraSystem->AddLayer(new StaticLayer(BossWalls, Vector2{ 0, 0 }, 1.0f));
    cameraSystem->AddLayer(new AnimatedLayer(BossPillar, 15, 0.2f, Vector2{ 0, 0 }, 1.0f));
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
    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& BossLevel::getObjLoc()
{
    static std::vector<std::shared_ptr<Obstacle>> empty;
    return empty;
}

void BossLevel::SpawnPickupWave()
{
    const int count = pickupCountDist(engine);
    const float xSpacing = 32.0f;
    float spawnXBase = 300.0f;

    for (int i = 0; i < count; ++i)
    {
        float x = spawnXBase + i * xSpacing;
        float y = 90.0f;
        Vector2 pos{ x, y };
        float phase = phaseDist(engine);

        auto pickup = GenerateRandomPickup(pos);
        pickup->SetPanSpeed(80.0f); // Enable leftward motion
        pickup->SetWavePhase(phase);
        pickups.push_back(pickup);
    }
}

std::shared_ptr<PickUp> BossLevel::GenerateRandomPickup(Vector2 pos)
{
    int roll = pickupTypeDist(engine);
    if (roll < 40) return std::make_shared<Coin>(pos, CoinType::GOLDCOIN);
    else if (roll < 60) return std::make_shared<Coin>(pos, CoinType::BLUECOIN);
    else if (roll < 70) return std::make_shared<Coin>(pos, CoinType::REDCOIN);
    else if (roll < 95) return std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL);
    else if (roll < 99) return std::make_shared<PoopHeart>(pos, PoopHeartType::BIG);
    else return std::make_shared<PoopHeart>(pos, PoopHeartType::INVISIBLE);
}

void BossLevel::SetSwingingPipes(bool) {}

void BossLevel::SetDifficulty(int difficultyIndex)
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

void BossLevel::SetPanSpeed(float speed)
{
    for (auto& pickup : pickups) {
        pickup->SetPanSpeed(speed);
    }
}