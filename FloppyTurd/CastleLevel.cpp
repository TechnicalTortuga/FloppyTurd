#include "CastleLevel.h"
#include "Resources.h"
#include "GameSettings.h"
#include "Coin.h"
#include "PoopHeart.h"
#include <raymath.h>
#include <algorithm>

using namespace Resources;
using namespace GameSettings;

CastleLevel::CastleLevel()
{
    camera = new CameraSystem();
    music = new AudioClip(LevelFour);

    // background ---------------------------------------------------------- 
    camera->AddLayer(new ParallaxLayer({ CastleBackgroundWall }, 80.f, 1.f));
    camera->AddLayer(new ParallaxLayer({ CastleBackgroundBars }, 80.f, 1.f));

    InitDecoration();
    InitObstacles();
    InitPickups();

    hasPassedFirstToilet = false;
    hasSpawnedFirstGapPickups = false;
}

CastleLevel::~CastleLevel()
{
    delete music;
    delete camera;
}

// ──────────────────────────────────────────────────────────────────────────
//  INITIALISATION
// ──────────────────────────────────────────────────────────────────────────
void CastleLevel::InitDecoration()
{
    // Initialize multiple paintings --------------------------------------- 
    const char* paints[] = { PaintingA, PaintingB, PaintingC, PaintingD };
    paintings.resize(GAP_COUNT); // One painting per gap
    for (size_t i = 0; i < paintings.size(); ++i) {
        paintings[i] = std::make_shared<Sprite>(
            paints[std::uniform_int_distribution<int>(0, 3)(rng)], 1);
        float x = i * spacing; // Start paintings at x = 0, spaced by 300.f
        paintings[i]->SetPosition({ x - 16.f, 48.f }); // Shift 16 pixels left
    }

    // Initialize curtains (one per toilet, will be positioned in InitObstacles) --- 
    curtains.resize(GAP_COUNT); // One curtain per toilet
    for (size_t i = 0; i < curtains.size(); ++i) {
        curtains[i] = std::make_shared<Sprite>(Curtains, 1);
    }
}

void CastleLevel::InitObstacles()
{
    float x = 0.f; // Start toilets at x = 0 to align with earlier floor torches
    toilets.reserve(GAP_COUNT);
    spikes.resize(GAP_COUNT);
    pillars.resize(GAP_COUNT);
    chandeliers.resize(GAP_COUNT);
    floorTorches.reserve(GAP_COUNT * 2);

    const float torchY = GameHeight - 64.f - 21.f;   // floor torches sit on floor
    const float spikeY = 90.f;                       // vertical centre for spikes
    const float toiletWidth = 45.f;                  // Correct toilet width

    for (int i = 0; i < GAP_COUNT; ++i, x += spacing)
    {
        // Gold toilets ---------------------------------------------------- 
        auto loo = std::make_shared<GoldToilets>(static_cast<int>(x), 0);
        toilets.push_back(loo);
        obstacles.push_back(loo);

        // Center curtain on this toilet, shifted 32 pixels left ----------- 
        curtains[i]->SetPosition({ x - 32.f, 21.f });

        // floor torches (foreground), adjusted for 45-pixel-wide toilet --- 
        auto torchL = std::make_shared<Sprite>(FloorTorch, 4, 0.1f, 1.f);
        auto torchR = std::make_shared<Sprite>(FloorTorch, 4, 0.1f, 1.f);
        torchL->SetPosition({ x - 37.f + (80.f - toiletWidth) / 2.f, torchY });
        torchR->SetPosition({ x + 85.f - (80.f - toiletWidth) / 2.f, torchY });
        floorTorches.push_back(torchL);
        floorTorches.push_back(torchR);

        foreground.push_back(torchL);
        foreground.push_back(torchR);

        // chandelier (mid-ground, center of gap) ------------------------- 
        float chandelierPos = x + (spacing - toiletWidth) / 2.f + toiletWidth - 16.f; // Center in gap, shift 16 pixels left
        auto cndl = std::make_shared<Sprite>(Chandelier, 4, 0.12f, 1.f);
        cndl->SetPosition({ chandelierPos, 21.f });
        chandeliers[i] = cndl;

        // spike OR pillar (centered behind toilet) ----------------------- 
        float toiletCentre = x + (spacing + toiletWidth) / 2.f - 16.f;
        if (i % 2 == 1)   // odd index ⇒ spike-ball (ensures first is TorchPillar)
        {
            auto spk = std::make_shared<SpikeBall>(Vector2{ toiletCentre, spikeY });
            spikes[i] = spk;
            obstacles.push_back(spk);
        }
        else              // even index ⇒ decorative pillar
        {
            auto pil = std::make_shared<Sprite>(TorchPillar, 4, 0.15f, 1.f);
            pil->SetPosition({ toiletCentre, 21.f });
            pillars[i] = pil;
        }
    }
}

void CastleLevel::InitPickups()
{
    float currentX = 0.f;
    // Start from index 1 to skip the first gap (between toilets 0 and 1)
    for (int i = 2; i < toilets.size(); ++i) {
        float xStart = toilets[i - 1]->pos.x + 45.f; // Right edge of previous toilet
        float xEnd = toilets[i]->pos.x; // Left edge of current toilet
        SpawnPickupsBetween(xStart, xEnd);
    }
}

void CastleLevel::SpawnPickupsBetween(float xStart, float xEnd)
{
    const int count = 5;
    const float yOffset = -40.0f; // Shift patterns upward, matching SnowLevel
    const float yLow = 60.0f + yOffset;
    const float yHigh = 120.0f + yOffset;
    const float topY = 10.0f; // Hug top of screen
    const float bottomY = GameHeight - 20.0f; // Hug bottom of screen
    const float xShift = -10.0f; // Shift pattern slightly left to center, matching SnowLevel

    // Randomly select one of seven pickup patterns (added top and bottom patterns)
    int pattern = GetRandomValue(0, 6); // 0 = straight, 1 = diagonal up, 2 = diagonal down, 3 = V, 4 = U, 5 = top, 6 = bottom

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
        case 5: // Top row (hugging top)
            y = topY;
            break;
        case 6: // Bottom row (hugging bottom)
            y = bottomY;
            break;
        default:
            y = (yLow + yHigh) * 0.5f;
            break;
        }

        Vector2 pos{ x, y };
        SpawnPickup(pos);
    }
}

void CastleLevel::SpawnPickup(Vector2 pos)
{
    int roll = GetRandomValue(1, 1000);
    if (roll <= 10) { // 1% chance for big heart
        auto heart = std::make_shared<PoopHeart>(pos, PoopHeartType::BIG);
        heart->SetPanSpeed(80.0f);
        pickups.push_back(heart);
    }
    else if (roll <= 40) { // 3% chance for small heart
        auto heart = std::make_shared<PoopHeart>(pos, PoopHeartType::SMALL);
        heart->SetPanSpeed(80.0f);
        pickups.push_back(heart);
    }
    else if (roll <= 100) { // 6% chance for red coin
        auto coin = std::make_shared<Coin>(pos, CoinType::REDCOIN);
        coin->SetPanSpeed(80.0f);
        pickups.push_back(coin);
    }
    else if (roll <= 250) { // 15% chance for blue coin
        auto coin = std::make_shared<Coin>(pos, CoinType::BLUECOIN);
        coin->SetPanSpeed(80.0f);
        pickups.push_back(coin);
    }
    else { // 75% chance for gold coin
        auto coin = std::make_shared<Coin>(pos, CoinType::GOLDCOIN);
        coin->SetPanSpeed(80.0f);
        pickups.push_back(coin);
    }
}

// ──────────────────────────────────────────────────────────────────────────
//  PER-FRAME
// ──────────────────────────────────────────────────────────────────────────
void CastleLevel::ScrollSprite(std::shared_ptr<Sprite>& s, float dt) const
{
    if (!s) return;
    Vector2 pos = s->GetPosition();
    pos.x -= 80.f * dt;
    s->SetPosition(pos);
    s->Update(dt);
}

void CastleLevel::ScrollSpriteVec(std::vector<std::shared_ptr<Sprite>>& v, float dt) const
{
    for (auto& s : v) {
        if (!s) continue;
        ScrollSprite(s, dt);
    }
}

void CastleLevel::Update(float dt)
{
    const float torchY = 180 - 64.f - 21.f;
    const float toiletWidth = 45.f; // Correct toilet width
    static float swingTimer = 0.0f; // Timer for spike ball swing
    const float swingFrequency = 1.0f; // Reduced frequency for slower swing
    const float swingAmplitude = 20.0f; // Reduced amplitude for smaller swing

    swingTimer += dt * swingFrequency;

    if (!music->IsPlaying()) music->Play();
    music->Update();
    camera->Update(dt);

    // Check if player has passed the first toilet
    if (!hasPassedFirstToilet && lastPlayerPosition.x > toilets[0]->pos.x + toiletWidth) {
        hasPassedFirstToilet = true;
    }

    // If player has passed the first toilet and we haven't spawned pickups in the first gap yet
    if (hasPassedFirstToilet && !hasSpawnedFirstGapPickups) {
        float xStart = toilets[0]->pos.x + 45.f; // Right edge of first toilet
        float xEnd = toilets[1]->pos.x; // Left edge of second toilet
        SpawnPickupsBetween(xStart, xEnd);
        hasSpawnedFirstGapPickups = true;
    }

    // Update toilets & associated objects --------------------------------- 
    for (size_t i = 0; i < toilets.size(); ++i)
    {
        auto& loo = toilets[i];
        loo->pos.x -= 80.f * dt;

        // Update positions of associated objects to match toilet
        // Alternate between curtains and paintings based on i % 2
        if (i % 2 == 0) { // even index (TorchPillar) => show curtain
            curtains[i]->SetPosition({ loo->pos.x - 32.f, 21.f });
            paintings[i]->SetPosition({ loo->pos.x - 16.f, 48.f });
        }
        else { // odd index (SpikeBall) => show painting
            paintings[i]->SetPosition({ loo->pos.x - 16.f, 48.f });
            curtains[i]->SetPosition({ loo->pos.x - 32.f, 21.f });
        }

        floorTorches[i * 2]->SetPosition({ loo->pos.x - 37.f + (80.f - toiletWidth) / 2.f, torchY });
        floorTorches[i * 2 + 1]->SetPosition({ loo->pos.x + 85.f - (80.f - toiletWidth) / 2.f, torchY });

        float centre = loo->pos.x + (spacing + toiletWidth) / 2.f - 16.f;
        if (chandeliers[i]) {
            float chandelierPos = loo->pos.x + (spacing - toiletWidth) / 2.f + toiletWidth - 16.f; // Center in gap, shift 16 pixels left
            chandeliers[i]->SetPosition({ chandelierPos, 21.f });
        }
        if (spikes[i]) {
            // Apply smoother, slower swinging motion directly
            float baseY = 90.f; // Center Y position for spike ball
            float swingOffset = sinf(swingTimer) * swingAmplitude; // Calculate swing offset
            spikes[i]->SetPosition({ centre, baseY + swingOffset });
            spikes[i]->Update(dt); // Still call Update for animations, but position is controlled here
        }
        else if (pillars[i]) {
            pillars[i]->SetPosition({ centre, 21.f });
        }

        // wrapped completely past left edge with a buffer?
        if (loo->pos.x + toiletWidth < -spacing / 2.f)
        {
            // previous toilet (with wrap-around)
            size_t prev = (i == 0 ? toilets.size() - 1 : i - 1);
            loo->pos.x = toilets[prev]->pos.x + spacing;
            loo->resetScore();
            // Spawn new pickups between the right edge of the previous toilet and new position
            float xStart = toilets[prev]->pos.x + 45.f;
            float xEnd = loo->pos.x;
            SpawnPickupsBetween(xStart, xEnd);
        }

        // Update animations
        loo->Update(dt);
        curtains[i]->Update(dt);
        paintings[i]->Update(dt);
        floorTorches[i * 2]->Update(dt);
        floorTorches[i * 2 + 1]->Update(dt);
        if (chandeliers[i]) chandeliers[i]->Update(dt);
        if (spikes[i]) spikes[i]->Update(dt);
        else if (pillars[i]) pillars[i]->Update(dt);
    }

    // Update pickups
    for (auto& pickup : pickups) {
        if (!pickup->IsCollected()) {
            pickup->Update(dt); // Let the pickup handle its own panning (set to 80.0f)
        }
    }

    // Sort floor torches by x-position to render leftmost first
    std::sort(floorTorches.begin(), floorTorches.end(),
        [](const auto& a, const auto& b) {
            return a->GetPosition().x < b->GetPosition().x;
        });
}

void CastleLevel::Draw() const
{
    camera->Draw();

    // mid-ground (curtains, paintings, chandeliers, pillars) ------------- 
    for (size_t i = 0; i < curtains.size(); ++i) {
        if (i % 2 == 0) curtains[i]->Draw(); // Show curtains on even indices (TorchPillar gaps)
    }
    for (size_t i = 0; i < paintings.size(); ++i) {
        if (i % 2 == 1) paintings[i]->Draw(); // Show paintings on odd indices (SpikeBall gaps)
    }
    for (const auto& s : pillars)   if (s) s->Draw();
    for (const auto& s : chandeliers) s->Draw();

    // foreground (floor torches) ---------------------------------------- 
    for (const auto& s : foreground) s->Draw();

    // toilets & spike-balls (obstacles) --------------------------------- 
    for (const auto& o : obstacles) o->Draw();

    // Draw pickups
    for (const auto& pickup : pickups) {
        if (!pickup->IsCollected())
            pickup->Draw();
    }
}

// ──────────────────────────────────────────────────────────────────────────
//  COLLISION / SCORE
// ──────────────────────────────────────────────────────────────────────────
bool CastleLevel::checkForCollisions(Vector2 c, float r)
{
    for (auto& loo : toilets)
        if (CheckCollisionCircleRec(c, r, loo->GetTopHitbox()) ||
            CheckCollisionCircleRec(c, r, loo->GetBottomHitbox()))
            return true;
    for (auto& spk : spikes)
        if (spk && CheckCollisionCircleRec(c, r, spk->GetHitbox()))
            return true;
    return false;
}

bool CastleLevel::checkForPointGain(Vector2 c, float /*r*/)
{
    float px = c.x;
    for (auto& loo : toilets)
    {
        float tx = loo->GetTopHitbox().x;
        if (px > tx && !loo->hasScored)
        {
            loo->hasScored = true; return true;
        }
    }
    return false;
}

const std::vector<std::shared_ptr<Obstacle>>& CastleLevel::getObjLoc()
{
    return obstacles;
}

std::vector<std::shared_ptr<PickUp>>& CastleLevel::GetPickUps()
{
    return pickups;
}