#include "SnowLevel.h"
#include "Resources.h"
#include "Coin.h"
#include "PoopHeart.h"
#include <raymath.h>  // for Vector2, GetRandomValue

SnowLevel::SnowLevel()
{
    using namespace Resources;
    using namespace GameSettings;
    cameraSystem = new CameraSystem();

    // Create parallax layers for a snowy environment
    cameraSystem->AddLayer(new ParallaxLayer(
        { SnowBackground }, // Far background (slowest)
        1.0f, // Speed (slow)
        1.0f // Scale
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { SnowMountains }, // Midground (moderate speed)
        10.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { SnowBackTrees }, // Foreground (fastest)
        60.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { SnowTundra }, // Foreground (fastest)
        60.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { SnowFrontTrees }, // Foreground (fastest)
        60.0f,
        1.0f
    ));

    levelMusic = new AudioClip(SnowLevelMusic); // Use SnowLevel.mp3 as Level 4 music
    InitObstacles();
}

SnowLevel::~SnowLevel()
{
    delete cameraSystem;
}

void SnowLevel::Draw() const
{
    cameraSystem->Draw();

    // Draw all obstacles (e.g., snow-covered trees or drifts)
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
    if (!levelMusic->IsPlaying()) levelMusic->Play();
    levelMusic->Update();

    for (auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Update(deltaTime);
    }
    // Update all obstacles
    for (int i = 0; i < (int)toilets.size(); ++i)
    {
        auto& oh = toilets[i];
        oh->pos.x -= PickupPanSpeed * deltaTime;

        if (oh->pos.x + 80 < 0)  // off‐screen
        {
            int lastIndex = (i - 1 < 0) ? (toilets.size() - 1) : (i - 1);
            float oldX = oh->pos.x;
            oh->pos.x = toilets[lastIndex]->pos.x + spacing;
            oh->resetScore();

            // spawn pickups between the RIGHT edge of the last pipe and new position
            float xStart = toilets[lastIndex]->pos.x + 80.0f;
            float xEnd = oh->pos.x;
            SpawnPickupsBetween(xStart, xEnd);
        }
        oh->Update(deltaTime);
    }
}

void SnowLevel::SpawnPickupsBetween(float xStart, float xEnd)
{
    const int count = 5;

    // Randomly select one of five pickup patterns
    int pattern = GetRandomValue(0, 4); // 0 = straight, 1 = diagonal up, 2 = diagonal down, 3 = V, 4 = U

    // Raise all patterns upward more
    const float yOffset = -40.0f; // more negative = higher on screen
    float yLow = 60.0f + yOffset;
    float yHigh = 120.0f + yOffset;

    // Shift pattern slightly left to better center between pipes
    float xShift = -10.0f;

    for (int i = 0; i < count; ++i)
    {
        float t = (count == 1 ? 0.0f : (float)i / (count - 1));
        float x = xStart + t * (xEnd - xStart) + xShift;

        float y;
        switch (pattern)
        {
        case 0: // straight
            y = (yLow + yHigh) * 0.5f;
            break;
        case 1: // diagonal up
            y = yLow + t * (yHigh - yLow);
            break;
        case 2: // diagonal down
            y = yHigh + t * (yLow - yHigh);
            break;
        case 3: // V shape
            y = yHigh - fabsf(t - 0.5f) * (yHigh - yLow) * 2.0f;
            break;
        case 4: // U shape
            y = yLow + fabsf(t - 0.5f) * (yHigh - yLow) * 2.0f;
            break;
        default:
            y = (yLow + yHigh) * 0.5f;
            break;
        }

        Vector2 pos{ x, y };

        int roll = GetRandomValue(1, 1000);
        if (roll <= 5) { // 0.5% chance for big heart
            auto heart = std::make_shared<PoopHeart>(pos, PoopHeartType::BIG);
            heart->SetPanSpeed(PickupPanSpeed);
            pickups.push_back(heart);
        }
        else if (roll <= 30) { // 2.5% chance for small heart
            auto heart = std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL);
            heart->SetPanSpeed(PickupPanSpeed);
            pickups.push_back(heart);
        }
        else {
            auto coin = std::make_shared<Coin>(pos, CoinType::BLUECOIN);
            coin->SetPanSpeed(PickupPanSpeed);
            pickups.push_back(coin);
        }
    }
}



void SnowLevel::InitObstacles()
{
    float currentX = static_cast<float>(320);

    for (int i = 0; i < 5; i++)
    {
        auto oh = std::make_shared<ToiletPair>(currentX, 0, true);  // <-- true for snowy
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
        if (CheckCollisionCircleRec(circleCenter, circleRadius, t->GetTopHitbox()) || CheckCollisionCircleRec(circleCenter, circleRadius, t->GetBottomHitbox()))
            return true;
    }

    return false;
}

bool SnowLevel::checkForPointGain(Vector2 circleCenter, float circleRadius)
{
    // Grab the player's center X
    float playerCenterX = circleCenter.x;

    for (auto& t : toilets)
    {
        // Let’s say t->GetX() returns the left x-position of the toilet pair (or the gap’s center).
        float toiletX = t->GetTopHitbox().x; // Or whatever logic obtains the relevant x

        // If the player has passed the toilet's center, and we haven't scored yet:
        if (playerCenterX > toiletX && !t->hasScored)
        {
            t->hasScored = true;          // Mark that we’ve scored
            return true;                  // Indicate to the caller that we got a point
        }
    }

    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& SnowLevel::getObjLoc()
{
	return obstacles;
}
