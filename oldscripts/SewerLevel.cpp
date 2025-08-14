#include "SewerLevel.h"
#include "Coin.h"
#include "PoopHeart.h"
#include <raymath.h>
#include "LevelManager.h"
#include "Resources.h"
#include "GameSettings.h"

SewerLevel::SewerLevel()
{
    using namespace Resources;
    using namespace GameSettings;
    cameraSystem = new CameraSystem();

    cameraSystem->AddLayer(new ParallaxLayer(
        { BackWallVarA, BackWallVarB, BackWallVarC, BackWallVarD },
        80.0f,
        1.0f
    ));

    levelMusicSlow = new AudioClip(LevelTwoSlow);
    levelMusicRegular = new AudioClip(LevelTwo);
    levelMusicFast = new AudioClip(LevelTwoFast);
    currentMusic = levelMusicRegular;

    InitObstacles();
    SpawnCoins();

    janitorIdle = new Sprite(Janitor, 1, 1.0f, 1.0f);
    janitorSweep = new Sprite(JanitorSweep, 4, 0.2f, 1.0f);
    janitorShock = new Sprite(JanitorSurprise, 8, 0.12f, 1.0f);
    ResetJanitor();
}

SewerLevel::~SewerLevel()
{
    delete janitorIdle;
    delete janitorSweep;
    delete janitorShock;
}

void SewerLevel::Draw() const
{
    cameraSystem->Draw();
    DrawJanitor();

    for (const auto& obstacle : obstacles)
        obstacle->Draw();

    for (auto& pickup : pickups)
        if (!pickup->IsCollected())
            pickup->Draw();
}

void SewerLevel::ResetJanitor()
{
    janitorVisible = false;
    janitorTimer = 0.0f;
    janitorNextAppearance = GetRandomValue(8, 12);
}

void SewerLevel::DrawJanitor() const
{
    if (!janitorVisible) return;
    janitorCurrent->Draw(janitorX, 100.0f);
}

void SewerLevel::UpdateJanitor(float deltaTime)
{
    if (!janitorVisible)
    {
        janitorTimer += deltaTime;
        if (janitorTimer >= janitorNextAppearance)
        {
            janitorVisible = true;
            janitorX = 330.0f;
            janitorIsShocked = false;
            janitorCurrent = janitorSweep;
            janitorCurrent->ResetAnimation();
        }
    }

    if (janitorVisible)
    {
        janitorX -= pickupPanSpeed * deltaTime; // Use pickupPanSpeed for janitor

        if (!janitorIsShocked && janitorCurrent == janitorSweep)
        {
            Vector2 playerPos = Vector2{ 0, 0 };
            if (auto* manager = dynamic_cast<LevelManager*>(LevelManager::GetInstance()))
                playerPos = manager->GetPlayerPosition();

            if (std::abs(janitorX - playerPos.x) <= 30.0f && GetRandomValue(0, 100) < 50)
            {
                janitorIsShocked = true;
                janitorCurrent = janitorShock;
                janitorCurrent->ResetAnimation();
            }
        }

        janitorCurrent->Update(deltaTime);

        static float lingerTimer = 0.0f;

        if (janitorIsShocked && janitorCurrent->hasLoopedOnce())
        {
            janitorCurrent->SetFrameFrozen(7);
            janitorIsShocked = false;
            lingerTimer = 1.5f;
        }

        if (!janitorIsShocked && janitorCurrent == janitorIdle)
        {
            lingerTimer -= deltaTime;
            if (lingerTimer <= 0.0f && janitorX < -64)
                ResetJanitor();
        }
    }
}

void SewerLevel::SpawnCoins() {
    for (const auto& pipe : pipes) {
        SpawnPickups(pipe);
    }
}

void SewerLevel::SpawnPickups(std::shared_ptr<SewerPipe> pipe)
{
    const int count = 5;
    int pattern = GetRandomValue(0, 4);

    float pipeX = pipe->GetX();
    float pipeW = pipe->GetWidth();
    float centerX = pipeX + pipeW * 0.5f;

    bool isTop = (pipe->GetPipeType() == PipeType::TopOrange || pipe->GetPipeType() == PipeType::TopBlue);
    float baseY = pipe->GetEdge();
    float yLow, yHigh;

    if (isTop) {
        yLow = baseY + 16.0f;
        yHigh = yLow + 28.0f;
    }
    else {
        yHigh = baseY - 16.0f;
        yLow = yHigh - 28.0f;
    }

    for (int i = 0; i < count; ++i)
    {
        float t = (count == 1 ? 0.0f : (float)i / (count - 1));
        float x = centerX - 64.0f + i * 32.0f;
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

        int roll = GetRandomValue(1, 100);
        std::shared_ptr<PickUp> pickup;
        if (roll <= 10)
            pickup = std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL);
        else
            pickup = std::make_shared<Coin>(pos, CoinType::GOLDCOIN);

        pickup->SetPanSpeed(pickupPanSpeed);
        pickups.push_back(pickup);
    }
}

void SewerLevel::SetPanSpeedMultiplier(float multiplier, float duration) {
    panSpeedMultiplier = multiplier;
    panSpeedTimer = duration;
}

float SewerLevel::GetPipePanSpeed() const {
    return pickupPanSpeed * panSpeedMultiplier;
}

void SewerLevel::Update(float deltaTime)
{
    cameraSystem->Update(deltaTime);
    if (!currentMusic->IsPlaying()) currentMusic->Play();
    currentMusic->Update();

    if (panSpeedTimer > 0.0f) {
        panSpeedTimer -= deltaTime;
        if (panSpeedTimer <= 0.0f)
            panSpeedMultiplier = 1.0f;
    }

    //  Let pickups handle their own movement via PickUp::Update()
    for (auto it = pickups.begin(); it != pickups.end(); ) {
        (*it)->Update(deltaTime);  // This already applies panSpeed internally
        if ((*it)->ShouldBeRemoved())
            it = pickups.erase(it);
        else
            ++it;
    }

    for (int i = 0; i < pipes.size(); i++) {
        auto& pipe = pipes[i];
        pipe->Update(deltaTime);
        if (pipe->pos.x + 200 < 0) {
            int lastIndex = (i - 1 < 0) ? (pipes.size() - 1) : (i - 1);
            pipe->pos.x = pipes[lastIndex]->pos.x + spacing;
            pipesPassed++;
            pipe->resetScore();
            SpawnPickups(pipe);
        }
    }

    UpdateJanitor(deltaTime);
}

void SewerLevel::InitObstacles()
{
    using namespace GameSettings;

    float currentX = 320.0f;

    for (int i = 0; i < 5; ++i)
    {
        auto pipe = std::make_shared<SewerPipe>(currentX, i % 2 == 0 ? 0 : 1);

        if (i % 2 == 0) {
            if (pipe->GetPipeType() == PipeType::BottomOrange)
                pipe->SetPipeType(PipeType::TopOrange);
            else if (pipe->GetPipeType() == PipeType::BottomBlue)
                pipe->SetPipeType(PipeType::TopBlue);
        }
        else {
            if (pipe->GetPipeType() == PipeType::TopOrange)
                pipe->SetPipeType(PipeType::BottomOrange);
            else if (pipe->GetPipeType() == PipeType::TopBlue)
                pipe->SetPipeType(PipeType::BottomBlue);
        }

        pipes.push_back(pipe);
        obstacles.push_back(pipe);
        currentX += spacing;
    }
}

const std::vector<std::shared_ptr<Obstacle>>& SewerLevel::getObjLoc()
{
    return obstacles;
}

bool SewerLevel::checkForCollisions(Vector2 circleCenter, float circleRadius)
{
    for (auto& pipe : pipes)
    {
        if (CheckCollisionCircleRec(circleCenter, circleRadius, pipe->GetHitbox()))
            return true;
    }
    return false;
}

bool SewerLevel::checkForPointGain(Vector2 circleCenter, float circleRadius)
{
    float playerX = circleCenter.x;
    for (auto& pipe : pipes)
    {
        if (!pipe->hasScored && playerX > pipe->GetHitbox().x)
        {
            pipe->hasScored = true;
            return true;
        }
    }
    return false;
}

void SewerLevel::SetDifficulty(int difficultyIndex)
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

void SewerLevel::SetPanSpeed(float speed)
{
    pickupPanSpeed = speed;
    for (auto& pipe : pipes) {
        pipe->SetPanSpeed(speed);
    }
    for (auto& pickup : pickups) {
        pickup->SetPanSpeed(speed);
    }
    // Enemies are updated via LevelManager
}