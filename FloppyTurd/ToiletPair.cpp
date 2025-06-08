#include "ToiletPair.h"
#include <iostream>
#include <random>

ToiletPair::ToiletPair(int xPos, int yPos, bool isSnowy)
{
    using namespace GameSettings;
    using namespace Resources;

    // Load textures
    _TopToilet = isSnowy ? LoadTexture(TopToiletSnow) : LoadTexture(TopToilet);
    _BottomToilet = isSnowy ? LoadTexture(BottomToiletSnow) : LoadTexture(BottomToilet);

    // Set snowy variant and default oscillation state
    isSnowyVariant = isSnowy;
    defaultOscillating = isSnowy; // Only snowy pipes oscillate by default
    isOscillating = defaultOscillating;

    // Randomize oscillation parameters
    std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> phaseDist(0.0f, 2 * PI);
    std::uniform_int_distribution<int> dirDist(0, 1);

    oscillationPhase = phaseDist(engine);
    oscillationDirection = dirDist(engine) == 0 ? 1.0f : -1.0f;
    phaseOffset = GetRandomValue(0, 628) / 100.0f; // 0 to 2π

    // Initialize position
    pos.x = xPos;
    pos.y = yPos;

    // Initialize scaling and pan speed
    toiletScale = 1.0f;
    panSpeed = 320 * 0.002f;

    // The gap between toilets
    gapBetweenToilets = GameHeight / 5.0f;

    // Assign random offset
    yOffsetRandomizer();

    // Reset scoring
    hasScored = false;
}

ToiletPair::~ToiletPair()
{
    UnloadTexture(_BottomToilet);
    UnloadTexture(_TopToilet);
}

void ToiletPair::Update(float deltaTime)
{
    if (isOscillating)
    {
        oscillationTimer += deltaTime;
        float amplitude = 40.0f;  // Consistent with GoldToilets
        float baseYOffset = 10.0f; // Prevent going too low
        yOffset = baseYOffset + sinf(oscillationTimer * 1.5f + oscillationPhase + phaseOffset) * amplitude * oscillationDirection;
    }

    UpdateHitbox();
}

void ToiletPair::resetScore()
{
    hasScored = false;
}

void ToiletPair::UpdateHitbox()
{
    using namespace GameSettings;

    float topWidthScaled = _TopToilet.width;
    float topHeightScaled = _TopToilet.height;
    float bottomWidthScaled = _BottomToilet.width;
    float bottomHeightScaled = _BottomToilet.height;

    // Top toilet hitbox
    hitboxTop.x = pos.x + 18;
    hitboxTop.y = (2 * -topHeightScaled / 3) + yOffset;
    hitboxTop.width = 30;
    hitboxTop.height = (6 * topHeightScaled / 7);

    // Bottom toilet hitbox
    hitboxBottom.x = pos.x + 18;
    hitboxBottom.y = (bottomHeightScaled / 3) + yOffset + (2 * bottomHeightScaled / 7);
    hitboxBottom.width = 30;
    hitboxBottom.height = bottomHeightScaled;
}

void ToiletPair::Draw()
{
    // Draw the top toilet
    DrawTexturePro(
        _TopToilet,
        { 0, 0, (float)(_TopToilet.width), (float)(_TopToilet.height) },
        { pos.x,  (2 * -_TopToilet.height / 3) + yOffset, (float)(_TopToilet.width), (float)(_TopToilet.height) },
        { 0, 0 },
        0.0f,
        WHITE
    );

    // Draw the bottom toilet
    DrawTexturePro(
        _BottomToilet,
        { 0, 0, (float)(_BottomToilet.width), (float)(_BottomToilet.height) },
        { pos.x, (_BottomToilet.height / 3) + yOffset + (_BottomToilet.height / 7), (float)(_BottomToilet.width), (float)(_BottomToilet.height) },
        { 0, 0 },
        0.0f,
        WHITE
    );
}

Rectangle ToiletPair::GetTopHitbox()
{
    return hitboxTop;
}

Rectangle ToiletPair::GetBottomHitbox()
{
    return hitboxBottom;
}

std::vector<Rectangle> ToiletPair::GetHitboxes()
{
    if (!collisionEnabled) {
        return std::vector<Rectangle>();
    }

    std::vector<Rectangle> hitboxes;
    hitboxes.push_back(hitboxTop);
    hitboxes.push_back(hitboxBottom);
    return hitboxes;
}

void ToiletPair::SetCollisionEnabled(bool enabled)
{
    collisionEnabled = enabled;
}

void ToiletPair::SetOscillationEnabled(bool enabled)
{
    isOscillating = enabled;
}

void ToiletPair::yOffsetRandomizer()
{
    static std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(-20.0f, maxYOffset);
    yOffset = dist(engine);
}