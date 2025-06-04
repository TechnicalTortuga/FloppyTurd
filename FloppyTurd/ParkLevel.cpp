#include "ParkLevel.h"

ParkLevel::ParkLevel()
{
    using namespace Resources;
    using namespace GameSettings;
    cameraSystem = new CameraSystem();

    // Create parallax layers with different speeds
    cameraSystem->AddLayer(new ParallaxLayer(
        {BackgroundBackLayer}, // Texture paths
        1.0f, // Speed
        1.0f // Scale
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { Clouds }, // Texture paths
        1.0f, // Speed
        1.0f // Scale
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        {BackgroundMidLayer},
        10.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        {BackgroundFrontLayer},
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

        // Move left
        oh->pos.x -= 80 * deltaTime;

        // Check if off screen
        if (oh->pos.x + 80 < 0)
        {
            
            int lastIndex = (i - 1 < 0) ? (toilets.size() - 1) : (i - 1);

            oh->pos.x = toilets[lastIndex]->pos.x + spacing;

            // Reset any scoring
            oh->resetScore();
        }

        // Now let do internal updates (hitboxes, etc.)
        oh->Update(deltaTime);
    }
}

void ParkLevel::InitObstacles()
{
    float currentX = static_cast<float>(320);

    for (int i = 0; i < 5; i++)
    {
        // Create ToiletPairs at currentX
        auto oh = std::make_shared<ToiletPair>(currentX, 0);
        toilets.push_back(oh);
        obstacles.push_back(static_cast<std::shared_ptr<Obstacle>>(oh));

        // Advance currentX
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
    // Grab the player's center X
    float playerCenterX = circleCenter.x;

    for (auto& t : toilets)
    {
        // Lets say t->GetX() returns the left x-position of the toilet pair (or the gaps center).
        float toiletX = t->GetTopHitbox().x; // Or whatever logic obtains the relevant x

        // If the player has passed the toilet's center, and we haven't scored yet:
        if (playerCenterX > toiletX && !t->hasScored)
        {
            t->hasScored = true;          // Mark that weve scored
            return true;                  // Indicate to the caller that we got a point
        }
    }

    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& ParkLevel::getObjLoc()
{
    return obstacles;
}
