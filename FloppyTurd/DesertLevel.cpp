#include "DesertLevel.h"
#include "BrickWall.h"
#include "Cactus.h"
#include "Coin.h"
#include "PoopHeart.h"

DesertLevel::DesertLevel()
    : engine(std::random_device{}())
    , cactiCountDist(1, 8)
    , cactusTypeDist(0, 9)
    , xSpacingDist(150.0f, 300.0f)
{
    using namespace Resources;
    using namespace GameSettings;

    cameraSystem = new CameraSystem();

    // Create parallax layers with different speeds
    cameraSystem->AddLayer(new ParallaxLayer(
        { DesertBackgroundBackLayer },
        16.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { DesertBackgroundMidLayer },
        32.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { DesertBackgroundFrontLayer },
        64.0f,
        1.0f
    ));

    cameraSystem->AddLayer(new ParallaxLayer(
        { DesertBackgroundCactiLayer },
        80.0f,
        1.0f
    ));

    levelMusic = new AudioClip(LevelThree);

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

    // Draw midground decorations
    for (auto& deco : midground) {
        deco->Draw(deco->position.x, deco->position.y);
    }

    // Draw obstacles (outhouses, cacti, brick walls)
    for (const auto& obstacle : obstacles) {
        obstacle->Draw();
    }

    // Draw pickups if not collected
    for (auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Draw();
    }

    // Draw foreground decorations
    for (auto& deco : foreground) {
        deco->Draw();
    }
}

void DesertLevel::Update(float deltaTime)
{
    /* 0 ─ Camera & music ─────────────────────────────────────────── */
    cameraSystem->Update(deltaTime);
    if (!levelMusic->IsPlaying()) levelMusic->Play();
    levelMusic->Update();

    /* 1 ─ Scroll & recycle outhouses ───────────────────────────── */
    for (size_t i = 0; i < outhouses.size(); ++i)
    {
        auto& oh = outhouses[i];
        oh->pos.x -= 80.0f * deltaTime;

        if (oh->pos.x + 80 < 0)                // wrapped off the left
        {
            size_t prev = (i == 0) ? outhouses.size() - 1 : i - 1;
            oh->pos.x = outhouses[prev]->pos.x + spacing;
            oh->hasScored = false;

            /* ←── bring these two lines back: they free the two gaps     */
            spawnedPickupsIndices.erase(prev);  // gap (prev , i)
            spawnedPickupsIndices.erase(i);     // gap (i    , i+1)
        }
        oh->Update(deltaTime);
    }

    /* 2 ─ Spawn pick-ups only while the gap is still off the 320-px view ─ */
    const float VirtualScreenW = 320.0f;

    for (size_t i = 0; i < outhouses.size(); ++i)            // 🔄 0 .. 11
    {
        size_t j = (i + 1) % outhouses.size();      // next outhouse (wraps to 0)
        Rectangle leftHit = outhouses[i]->GetOuthouseHitbox();
        Rectangle rightHit = outhouses[j]->GetOuthouseHitbox();

        if (leftHit.x <= VirtualScreenW)  continue;          // still visible? skip
        if (spawnedPickupsIndices.count(i)) continue;        // gap already filled

        float leftEdge = leftHit.x + leftHit.width + 10.0f;
        float rightEdge = rightHit.x - 10.0f;

        SpawnPickupsBetween(leftEdge, rightEdge);
        spawnedPickupsIndices.insert(i);                     // mark this gap
    }

    // ─── 3. Update & cull pick-ups ───────────────────────────────
    for (auto it = pickups.begin(); it != pickups.end(); )
    {
        (*it)->Update(deltaTime);               // advance animation / pan

        if ((*it)->ShouldBeRemoved())           // collected or off-screen?
            it = pickups.erase(it);             // erase and get next iterator
        else
            ++it;                               // just advance
    }

    /* 4 ─ Cactus scroll / recycle ───────────────────────────────── */
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

    /* 5 ─ Brick-wall scroll / recycle (unchanged) ──────────────── */
    for (auto& o : obstacles)
        if (auto brick = dynamic_cast<BrickWall*>(o.get()))
        {
            brick->pos.x -= 80.0f * deltaTime;
            if (brick->pos.x + brick->GetWidth() < 0)
            {
                static size_t currentPair = 0;
                currentPair = (currentPair + 1) % outhouses.size();

                Vector2 left = outhouses[currentPair]->pos;
                Vector2 right = outhouses[(currentPair + 1) % outhouses.size()]->pos;

                Texture2D tex = LoadTexture(Resources::OuthouseSolo);
                float outhouseW = tex.width;  UnloadTexture(tex);

                Texture2D wallTex = LoadTexture(Resources::BrickWallTexture);
                float wallW = wallTex.width;  UnloadTexture(wallTex);

                float gapCentre = (left.x + outhouseW * 0.5f +
                    right.x + outhouseW * 0.5f) * 0.5f;
                brick->pos.x = gapCentre - wallW * 0.5f;
                brick->pos.y = 0.0f;
            }
            brick->Update(deltaTime);
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
    // We apply SpawnPadding so pickups aren’t flush against the outhouse:
    float paddedStart = xStart + SpawnPadding;
    float paddedEnd = xEnd - SpawnPadding;

    // If, after padding, the range is too small, skip spawning:
    if (paddedEnd <= paddedStart)
        return;

    const int count = 5;

    // Brick wall awareness
    bool hasBrickWall = IsBrickBetween(paddedStart, paddedEnd);

    int pattern;
    do {
        pattern = GetRandomValue(0, 4);  // 0: straight, 1: diag ↑, 2: diag ↓, 3: V, 4: U
    } while (hasBrickWall && (pattern == 3 || pattern == 4));  // Avoid V and U if brick is present

    // Lower everything by 10px for extra player space
    const float yOffset = -20.0f;  // Lower on screen by 10 pixels
    float yLow = 60.0f + yOffset;
    float yHigh = 120.0f + yOffset;

    for (int i = 0; i < count; ++i)
    {
        float t = (count == 1 ? 0.0f : (float)i / (count - 1));
        float x = paddedStart + t * (paddedEnd - paddedStart);

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
        auto pickup = GenerateRandomPickup(pos);
        pickup->SetPanSpeed(PickupPanSpeed);
        pickups.push_back(pickup);
    }
}

std::shared_ptr<PickUp> DesertLevel::GenerateRandomPickup(Vector2 pos)
{
    int roll = GetRandomValue(0, 9); // 0–7 = coin, 8 = small heart, 9 = big heart

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

    // 0) Initialize outhouses at evenly spaced intervals
    float x = 320;
    for (int i = 0; i < 12; ++i)
    {
        int y = 0;
        auto house = std::make_shared<Outhouse>((int)x, y);
        outhouses.push_back(house);
        obstacles.push_back(house);
        x += spacing;
    }

    // 1) Place cactus obstacles at regular intervals
    float cactusSpacing = 100.0f;
    for (int i = 0; i < 6; ++i)
    {
        float x = 320 + i * cactusSpacing + GetRandomValue(-10, 10);
        float y = 0;

        int roll = GetRandomValue(0, 99);

        CactusVariant variant;
        if (roll < 2) variant = CactusVariant::DANCING_COWBOY;   // 2 
        else if (roll < 10) variant = CactusVariant::DANCING_BIG;      // 8 
        else if (roll < 20) variant = CactusVariant::DANCING_SMALL;    // 10 
        else                 variant = static_cast<CactusVariant>(GetRandomValue(0, 5));
        obstacles.push_back(std::make_shared<Cactus>(Vector2{ x, y }, variant));
    }

    // 2) Insert a single brick wall between the first pair of outhouses (0 and 1)
    if (outhouses.size() >= 2) // Ensure we have at least two outhouses
    {
        Vector2 left = outhouses[0]->pos;
        Vector2 right = outhouses[1]->pos;

        // Load the outhouse texture to get its width
        Texture2D tex = LoadTexture(Resources::OuthouseSolo);
        float outhouseWidth = tex.width;
        UnloadTexture(tex);

        // Calculate the center between the two outhouses
        float leftCenter = left.x + outhouseWidth / 2.0f;
        float rightCenter = right.x + outhouseWidth / 2.0f;
        float gapCenter = (leftCenter + rightCenter) / 2.0f;

        // Load the brick wall texture to get its width
        Texture2D wallTex = LoadTexture(Resources::BrickWallTexture);
        float wallX = gapCenter - wallTex.width / 2.0f; // Center the brick wall
        float wallY = 0.0f;
        UnloadTexture(wallTex);

        // Add the single brick wall to obstacles
        auto brick = std::make_shared<BrickWall>(Vector2{ wallX, wallY });
        obstacles.push_back(brick);

        std::cout << "Initial brick wall between outhouses 0 and 1 at {" << wallX << ", " << wallY << "}\n";
    }
}

void DesertLevel::InitDecoration()
{
    using namespace GameSettings;

    float currentX = 320.0f;
    const float minSpacing = 40.0f;
    const float maxSpacing = 100.0f;

    // 1) Build a pool of 2 of each cactus variant
    std::vector<CactusVariant> cactusPool;
    for (int i = 0; i <= static_cast<int>(CactusVariant::BUSH); ++i)
    {
        cactusPool.push_back(static_cast<CactusVariant>(i));
        cactusPool.push_back(static_cast<CactusVariant>(i));
    }
    std::shuffle(cactusPool.begin(), cactusPool.end(), engine);

    // 2) Spawn the guaranteed pool of cacti as obstacles
    for (auto type : cactusPool)
    {
        obstacles.push_back(std::make_shared<Cactus>(Vector2{ currentX, 0.0f }, type));
        currentX += GetRandomValue(minSpacing, maxSpacing) * GameScale;
    }

    // 3) Fill the remainder with random cacti or decorative sprites
    std::uniform_int_distribution<int> cactusRoll(0, 9);
    while (currentX < 1280.0f)
    {
        int roll = cactusRoll(engine);
        if (roll <= 5)  // Obstacle cactus
        {
            CactusVariant variant = static_cast<CactusVariant>(roll);
            obstacles.push_back(std::make_shared<Cactus>(Vector2{ currentX, 0.0f }, variant));
        }
        else  // Decorative dancing cacti in foreground
        {
            const CactusInfo* decoCactus = nullptr;
            if (roll == 6 || roll == 7) decoCactus = &CACTUS_NORMAL;
            else if (roll == 8)         decoCactus = &CACTUS_SMALL;
            else                        decoCactus = &CACTUS_COWBOY;

            float y = 180.0f - decoCactus->height;
            foreground.push_back(std::make_shared<Sprite>(
                decoCactus->spriteId,
                8,      // 8 frames
                0.1f,   // animation speed
                1.0f,   // scale
                Vector2{ currentX, y }
            ));
        }

        currentX += GetRandomValue(minSpacing, maxSpacing) * GameScale;
    }
}

bool DesertLevel::checkForCollisions(Vector2 circleCenter, float circleRadius)
{
    // Check against each outhouse’s two hitboxes
    for (auto& o : outhouses)
    {
        if (CheckCollisionCircleRec(circleCenter, circleRadius, o->GetOuthouseHitbox()) ||
            CheckCollisionCircleRec(circleCenter, circleRadius, o->GetOuthouseToiletHitbox()))
        {
            return true;
        }
    }

    // Check against brick walls and cacti
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