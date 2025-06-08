#include "ParkLevel.h"

ParkLevel::ParkLevel()
{
    using namespace Resources;
    using namespace GameSettings;
    cameraSystem = new CameraSystem();

    // Create parallax layers with different speeds
    cameraSystem->AddLayer(new ParallaxLayer(
        { BackgroundBackLayer }, // Texture paths
        1.0f, // Speed
        1.0f // Scale
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { Clouds }, // Texture paths
        1.0f, // Speed
        1.0f // Scale
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { BackgroundMidLayer },
        10.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { BackgroundFrontLayer },
        60.0f,
        1.0f
    ));

    levelMusic = new AudioClip(LevelOne);
    InitObstacles();
}

ParkLevel::~ParkLevel()
{
    delete cameraSystem;
}

void ParkLevel::Draw() const
{
    cameraSystem->Draw();

    // Draw all obstacles
    for (const auto& obstacle : obstacles) {
        obstacle->Draw();
    }

    for (auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Draw();
    }
}

void ParkLevel::Update(float deltaTime)
{
    cameraSystem->Update(deltaTime);
    if (!levelMusic->IsPlaying()) levelMusic->Play();
    levelMusic->Update();

    // Update all pickups
    for (auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Update(deltaTime);
    }

    // Update all obstacles
    for (int i = 0; i < toilets.size(); i++)
    {
        auto& oh = toilets[i];
        oh->pos.x -= 80 * deltaTime;

        if (oh->pos.x + 80 < 0)
        {
            int lastIndex = (i - 1 < 0) ? (toilets.size() - 1) : (i - 1);
            oh->pos.x = toilets[lastIndex]->pos.x + spacing;
            oh->resetScore();
        }

        oh->Update(deltaTime);
    }
}

void ParkLevel::InitObstacles()
{
    float currentX = static_cast<float>(320);

    for (int i = 0; i < 5; i++)
    {
        auto oh = std::make_shared<ToiletPair>(currentX, 0);
        toilets.push_back(oh);
        obstacles.push_back(static_cast<std::shared_ptr<Obstacle>>(oh));
        currentX += spacing;
    }
}

bool ParkLevel::checkForCollisions(Vector2 circleCenter, float circleRadius)
{
    for (auto& t : toilets)
    {
        if (CheckCollisionCircleRec(circleCenter, circleRadius, t->GetTopHitbox()) || CheckCollisionCircleRec(circleCenter, circleRadius, t->GetBottomHitbox()))
            return true;
    }
    return false;
}

bool ParkLevel::checkForPointGain(Vector2 circleCenter, float circleRadius)
{
    float playerCenterX = circleCenter.x;

    for (auto& t : toilets)
    {
        float toiletX = t->GetTopHitbox().x;
        if (playerCenterX > toiletX && !t->hasScored)
        {
            t->hasScored = true;
            return true;
        }
    }
    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& ParkLevel::getObjLoc()
{
    return obstacles;
}

void ParkLevel::SetSwingingPipes(bool enable)
{
    for (auto& toilet : toilets) {
        toilet->SetOscillationEnabled(enable);
    }
}