#include "DesertLevel.h"
#include "BrickWall.h"
#include "Cactus.h"
#include "Coin.h"
#include "PoopHeart.h"
#include "Resources.h"
#include "GameSettings.h"

DesertLevel::DesertLevel()
    : engine(std::random_device{}())
    , cactiCountDist(1, 8)
    , cactusTypeDist(0, 9)
    , xSpacingDist(150.0f, 300.0f)
{
    using namespace Resources;
    using namespace GameSettings;

    cameraSystem = new CameraSystem();

    cameraSystem->AddLayer(new ParallaxLayer({ DesertBackgroundBackLayer }, 16.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ DesertBackgroundMidLayer }, 32.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ DesertBackgroundFrontLayer }, 64.0f, 1.0f));
    cameraSystem->AddLayer(new ParallaxLayer({ DesertBackgroundCactiLayer }, 80.0f, 1.0f));

    levelMusicSlow = new AudioClip(LevelThreeSlow);
    levelMusicRegular = new AudioClip(LevelThree);
    levelMusicFast = new AudioClip(LevelThreeFast);
    currentMusic = levelMusicRegular;

    InitObstacles();
    InitDecoration();
}

DesertLevel::~DesertLevel()
{
    delete cameraSystem;
}

void DesertLevel::Draw() const
{
    cameraSystem->Draw();

    for (auto& deco : midground) {
        deco->Draw(deco->position.x, deco->position.y);
    }

    for (const auto& obstacle : obstacles) {
        obstacle->Draw();
    }

    for (auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Draw();
    }

    for (auto& deco : foreground) {
        deco->Draw();
    }
}

void DesertLevel::Update(float deltaTime)
{
    cameraSystem->Update(deltaTime);
    if (!currentMusic->IsPlaying()) currentMusic->Play();
    currentMusic->Update();

    for (size_t i = 0; i < outhouses.size(); ++i)
    {
        auto& oh = outhouses[i];
        oh->Update(deltaTime);
        if (oh->pos.x + 80 < 0)
        {
            size_t prev = (i == 0) ? outhouses.size() - 1 : i - 1;
            oh->pos.x = outhouses[prev]->pos.x + spacing;
            oh->hasScored = false;
            spawnedPickupsIndices.erase(prev);
            spawnedPickupsIndices.erase(i);
        }
    }

    const float VirtualScreenW = 320.0f;

    for (size_t i = 0; i < outhouses.size(); ++i)
    {
        size_t j = (i + 1) % outhouses.size();
        Rectangle leftHit = outhouses[i]->GetOuthouseHitbox();
        Rectangle rightHit = outhouses[j]->GetOuthouseHitbox();

        if (leftHit.x <= VirtualScreenW) continue;
        if (spawnedPickupsIndices.count(i)) continue;

        float leftEdge = leftHit.x + leftHit.width + 10.0f;
        float rightEdge = rightHit.x - 10.0f;

        SpawnPickupsBetween(leftEdge, rightEdge);
        spawnedPickupsIndices.insert(i);
    }

    for (auto it = pickups.begin(); it != pickups.end(); )
    {
        (*it)->Update(deltaTime);
        if ((*it)->ShouldBeRemoved())
            it = pickups.erase(it);
        else
            ++it;
    }

    for (auto& o : obstacles)
        if (auto cactus = dynamic_cast<Cactus*>(o.get()))
        {
            if (cactus->pos.x + cactus->GetWidth() < 0)
            {
                float maxX = 0.0f;
                for (const auto& ob : obstacles)
                    if (auto c = dynamic_cast<Cactus*>(ob.get()))
                        maxX = std::max(maxX, c->pos.x);

                cactus->pos.x = maxX + GetRandomValue(48, 96);
            }
            cactus->Update(deltaTime);
        }

    for (auto& o : obstacles)
        if (auto brick = dynamic_cast<BrickWall*>(o.get()))
        {
            brick->Update(deltaTime);
            if (brick->pos.x + brick->GetWidth() < 0)
            {
                static size_t currentPair = 0;
                currentPair = (currentPair + 1) % outhouses.size();

                Vector2 left = outhouses[currentPair]->pos;
                Vector2 right = outhouses[(currentPair + 1) % outhouses.size()]->pos;

                Texture2D tex = LoadTexture(Resources::OuthouseSolo);
                float outhouseW = tex.width; UnloadTexture(tex);

                Texture2D wallTex = LoadTexture(Resources::BrickWallTexture);
                float wallW = wallTex.width; UnloadTexture(wallTex);

                float gapCentre = (left.x + outhouseW * 0.5f + right.x + outhouseW * 0.5f) * 0.5f;
                brick->pos.x = gapCentre - wallW * 0.5f;
                brick->pos.y = 0.0f;
            }
        }
}

bool DesertLevel::IsBrickBetween(float leftX, float rightX) const
{
    for (const auto& o : obstacles)
    {
        if (auto brick = dynamic_cast<BrickWall*>(o.get()))
        {
            float bx = brick->GetPosition().x;
            if (bx > leftX && bx < rightX)
                return true;
        }
    }
    return false;
}

void DesertLevel::SpawnPickupsBetween(float xStart, float xEnd)
{
    float paddedStart = xStart + SpawnPadding;
    float paddedEnd = xEnd - SpawnPadding;

    if (paddedEnd <= paddedStart)
        return;

    const int count = 5;
    bool hasBrickWall = IsBrickBetween(paddedStart, paddedEnd);

    int pattern;
    do {
        pattern = GetRandomValue(0, 4);
    } while (hasBrickWall && (pattern == 3 || pattern == 4));

    const float yOffset = -20.0f;
    float yLow = 60.0f + yOffset;
    float yHigh = 120.0f + yOffset;

    for (int i = 0; i < count; ++i)
    {
        float t = (count == 1 ? 0.0f : (float)i / (count - 1));
        float x = paddedStart + t * (paddedEnd - paddedStart);

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
        auto pickup = GenerateRandomPickup(pos);
        pickup->SetPanSpeed(pickupPanSpeed);
        pickups.push_back(pickup);
    }
}

std::shared_ptr<PickUp> DesertLevel::GenerateRandomPickup(Vector2 pos)
{
    int roll = GetRandomValue(0, 9);
    if (roll <= 7)
        return std::make_shared<Coin>(pos, CoinType::GOLDCOIN);
    else if (roll == 8)
        return std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL);
    else
        return std::make_shared<PoopHeart>(pos, PoopHeartType::BIG);
}

void DesertLevel::InitObstacles()
{
    outhouses.clear();
    obstacles.clear();
    spawnedPickupsIndices.clear();

    float x = 320;
    for (int i = 0; i < 12; ++i)
    {
        int y = 0;
        auto house = std::make_shared<Outhouse>((int)x, y);
        outhouses.push_back(house);
        obstacles.push_back(house);
        x += spacing;
    }

    float cactusSpacing = 100.0f;
    for (int i = 0; i < 6; ++i)
    {
        float x = 320 + i * cactusSpacing + GetRandomValue(-10, 10);
        float y = 0;

        int roll = GetRandomValue(0, 99);
        CactusVariant variant;
        if (roll < 2) variant = CactusVariant::DANCING_COWBOY;
        else if (roll < 10) variant = CactusVariant::DANCING_BIG;
        else if (roll < 20) variant = CactusVariant::DANCING_SMALL;
        else variant = static_cast<CactusVariant>(GetRandomValue(0, 5));
        obstacles.push_back(std::make_shared<Cactus>(Vector2{ x, y }, variant));
    }

    if (outhouses.size() >= 2)
    {
        Vector2 left = outhouses[0]->pos;
        Vector2 right = outhouses[1]->pos;

        Texture2D tex = LoadTexture(Resources::OuthouseSolo);
        float outhouseW = tex.width; UnloadTexture(tex);

        Texture2D wallTex = LoadTexture(Resources::BrickWallTexture);
        float wallX = (left.x + outhouseW * 0.5f + right.x + outhouseW * 0.5f) * 0.5f - wallTex.width * 0.5f;
        float wallY = 0.0f;
        UnloadTexture(wallTex);

        auto brick = std::make_shared<BrickWall>(Vector2{ wallX, wallY });
        obstacles.push_back(brick);
    }
}

void DesertLevel::InitDecoration()
{
    using namespace GameSettings;

    float currentX = 320.0f;
    const float minSpacing = 40.0f;
    const float maxSpacing = 100.0f;

    std::vector<CactusVariant> cactusPool;
    for (int i = 0; i <= static_cast<int>(CactusVariant::BUSH); ++i)
    {
        cactusPool.push_back(static_cast<CactusVariant>(i));
        cactusPool.push_back(static_cast<CactusVariant>(i));
    }
    std::shuffle(cactusPool.begin(), cactusPool.end(), engine);

    for (auto type : cactusPool)
    {
        obstacles.push_back(std::make_shared<Cactus>(Vector2{ currentX, 0.0f }, type));
        currentX += GetRandomValue(minSpacing, maxSpacing) * GameScale;
    }

    std::uniform_int_distribution<int> cactusRoll(0, 9);
    while (currentX < 1280.0f)
    {
        int roll = cactusRoll(engine);
        if (roll <= 5)
        {
            CactusVariant variant = static_cast<CactusVariant>(roll);
            obstacles.push_back(std::make_shared<Cactus>(Vector2{ currentX, 0.0f }, variant));
        }
        else
        {
            const CactusInfo* decoCactus = nullptr;
            if (roll == 6 || roll == 7) decoCactus = &CACTUS_NORMAL;
            else if (roll == 8) decoCactus = &CACTUS_SMALL;
            else decoCactus = &CACTUS_COWBOY;

            float y = 180.0f - decoCactus->height;
            foreground.push_back(std::make_shared<Sprite>(
                decoCactus->spriteId,
                8,
                0.1f,
                1.0f,
                Vector2{ currentX, y }
            ));
        }
        currentX += GetRandomValue(minSpacing, maxSpacing) * GameScale;
    }
}

bool DesertLevel::checkForCollisions(Vector2 circleCenter, float circleRadius)
{
    for (auto& o : outhouses)
    {
        if (CheckCollisionCircleRec(circleCenter, circleRadius, o->GetOuthouseHitbox()) ||
            CheckCollisionCircleRec(circleCenter, circleRadius, o->GetOuthouseToiletHitbox()))
        {
            return true;
        }
    }

    for (auto& o : obstacles)
    {
        if (dynamic_cast<BrickWall*>(o.get()))
        {
            for (auto& hitbox : o->GetHitboxes())
            {
                if (CheckCollisionCircleRec(circleCenter, circleRadius, hitbox))
                    return true;
            }
        }

        if (auto cactus = dynamic_cast<Cactus*>(o.get()))
        {
            if (cactus->GetVariant() == CactusVariant::BUSH)
                continue;
            for (auto& hitbox : cactus->GetHitboxes())
            {
                if (CheckCollisionCircleRec(circleCenter, circleRadius, hitbox))
                    return true;
            }
        }
    }
    return false;
}

bool DesertLevel::checkForPointGain(Vector2 circleCenter, float circleRadius)
{
    float playerCenterX = circleCenter.x;
    for (auto& t : outhouses)
    {
        float toiletX = t->GetOuthouseHitbox().x;
        if (playerCenterX > toiletX && !t->hasScored)
        {
            t->hasScored = true;
            return true;
        }
    }
    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& DesertLevel::getObjLoc()
{
    return obstacles;
}

void DesertLevel::SetDifficulty(int difficultyIndex)
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

void DesertLevel::SetPanSpeed(float speed)
{
    pickupPanSpeed = speed;
    for (auto& outhouse : outhouses) {
        outhouse->SetPanSpeed(speed);
    }
    for (auto& obstacle : obstacles) {
        if (auto cactus = dynamic_cast<Cactus*>(obstacle.get())) {
            cactus->SetPanSpeed(speed);
        }
        if (auto brick = dynamic_cast<BrickWall*>(obstacle.get())) {
            brick->SetPanSpeed(speed);
        }
    }
    for (auto& pickup : pickups) {
        pickup->SetPanSpeed(speed);
    }
    // Enemies are updated via LevelManager
}