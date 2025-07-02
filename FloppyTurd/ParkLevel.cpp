#include "ParkLevel.h"
#include "ResourceCompat.h"
#include "GameSettings.h"

ParkLevel::ParkLevel()
{
    using namespace Resources;
    using namespace GameSettings;
    cameraSystem = new CameraSystem();

    // Create parallax layers with different speeds
    cameraSystem->AddLayer(new ParallaxLayer({ BackgroundBackLayer }, 1.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ Clouds }, 1.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ BackgroundMidLayer }, 10.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ BackgroundFrontLayer }, 60.0f, 1.0f));

    // Initialize music for all difficulties
    levelMusicSlow = new AudioClip(LevelOneSlow);
    levelMusicRegular = new AudioClip(LevelOne);
    levelMusicFast = new AudioClip(LevelOneFast);
    currentMusic = levelMusicRegular; // Default to Regular

    InitObstacles();
}

ParkLevel::~ParkLevel()
{
    delete cameraSystem;
}

void ParkLevel::Draw() const
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

void ParkLevel::Update(float deltaTime)
{
    cameraSystem->Update(deltaTime);
    if (!currentMusic->IsPlaying()) currentMusic->Play();
    currentMusic->Update();

    // Update all pickups
    for (auto& pickup : pickups) {
        if (!pickup->IsCollected()) {
            pickup->Update(deltaTime);
            // Use same speed as toilets for pickups
            pickup->SetPosition({ pickup->GetPosition().x - toilets[0]->GetPanSpeed() * deltaTime, pickup->GetPosition().y });
        }
    }

    // Update all obstacles
    for (int i = 0; i < toilets.size(); i++)
    {
        auto& toilet = toilets[i];
        toilet->Update(deltaTime); // Handles movement internally
        if (toilet->pos.x + 80 < 0)
        {
            int lastIndex = (i - 1 < 0) ? (toilets.size() - 1) : (i - 1);
            toilet->pos.x = toilets[lastIndex]->pos.x + spacing;
            toilet->resetScore();
        }
    }
}

void ParkLevel::InitObstacles()
{
    toilets.clear();   // Prevent double stacking
    obstacles.clear(); // Also clear shared obstacle list

    float currentX = 320.0f;

    for (int i = 0; i < 5; i++)
    {
        auto toilet = std::make_shared<ToiletPair>(currentX, 0);
        toilets.push_back(toilet);
        obstacles.push_back(toilet);
        currentX += spacing;
    }
}

bool ParkLevel::checkForCollisions(Vector2 circleCenter, float circleRadius)
{
    for (auto& toilet : toilets)
    {
        for (auto& hitbox : toilet->GetHitboxes()) {
            if (CheckCollisionCircleRec(circleCenter, circleRadius, hitbox))
                return true;
        }
    }
    return false;
}

bool ParkLevel::checkForPointGain(Vector2 centre, float)
{
    const float playerX = centre.x;
    bool scored = false;

    for (auto& toilet : toilets)
    {
        const Rectangle top = toilet->GetTopHitbox();
        const float passLine = top.x + top.width;

        // Only count score when player's X is *just* passing this pipe
        if (!toilet->hasScored && playerX > passLine && playerX < passLine + 2.0f)
        {
            toilet->hasScored = true;
            scored = true;
        }
    }

    return scored;
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

void ParkLevel::SetDifficulty(int difficultyIndex)
{
    switch (difficultyIndex) {
    case 0: currentMusic = levelMusicSlow; break; // Runny
    case 1: currentMusic = levelMusicRegular; break; // Regular
    case 2: currentMusic = levelMusicFast; break; // Rough
    default: currentMusic = levelMusicRegular; break;
    }
    if (currentMusic) {
        currentMusic->Stop();
        currentMusic->Play();
    }
}

void ParkLevel::SetPanSpeed(float speed)
{
    for (auto& toilet : toilets) {
        toilet->SetPanSpeed(speed);
    }
}