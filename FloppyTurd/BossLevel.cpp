#include "BossLevel.h"
#include "Resources.h"
#include "RatKing.h"
#include "GameSettings.h"
#include "ParallaxLayer.h"
#include "StaticLayer.h"
#include "AnimatedLayer.h"

BossLevel::BossLevel()
{
    using namespace Resources;
    using namespace GameSettings;

    cameraSystem = new CameraSystem();
    InitCamera();
    InitLayers();

    boss = std::make_shared<RatKing>(Vector2{ 160.0f, 40.0f });
    levelMusic = new AudioClip(LevelFive);  // Use BossLevel.mp3 (corrected from LevelFive)
    lowHealthMusic = new AudioClip(BossLowHealth);

    // Prime the stream silently
    lowHealthMusic->SetVolume(0.0f);
    lowHealthMusic->Play();
    lowHealthMusic->Update();  // This primes the stream buffers
    lowHealthMusic->Stop();
    lowHealthMusic->SetVolume(1.0f);  // Reset for actual use
}

BossLevel::~BossLevel()
{
    delete cameraSystem;
}

void BossLevel::Draw() const
{
    if (cameraSystem) cameraSystem->Draw();
    if (boss && boss->isActive) boss->Draw();
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
}

void BossLevel::InitCamera()
{
    // No need for initialization beyond creation, as layers will be added in InitLayers
}

void BossLevel::InitLayers()
{
    using namespace Resources;
    using namespace GameSettings;

    // Add layers to CameraSystem with appropriate speeds (similar to parallax layers)
    cameraSystem->AddLayer(new StaticLayer(
         BossBackground ,  // Background (slowest)
        Vector2{0,0},  // Position
        1.0f   // Scale
    ));

    cameraSystem->AddLayer(new StaticLayer(
         BossDarkClouds ,  // Clouds (moderate speed)
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
        BossWalls ,  
        Vector2{ 0,0 },  // Position
        1.0f
    ));

    // Pillar (animated, treated as a fast-moving layer for effect)
    cameraSystem->AddLayer(new AnimatedLayer(
        BossPillar ,  // Animated pillar (4 frames for rotation)
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