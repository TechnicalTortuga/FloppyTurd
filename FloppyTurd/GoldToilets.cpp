#include "GoldToilets.h"
#include <iostream>
#include <random>

GoldToilets::GoldToilets(int xPos, int yPos)
{
    using namespace GameSettings;
    using namespace Resources;

    // Load textures
    _TopToilet = LoadTexture(TopToiletGold);
    _BottomToilet = LoadTexture(BottomToiletGold);

    // Initialize oscillation state
    defaultOscillating = true;
    isOscillating = defaultOscillating;

    // Randomize oscillation parameters
    std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> phaseDist(0.0f, 2 * PI);
    std::uniform_int_distribution<int> dirDist(0, 1);

    oscillationPhase = phaseDist(engine);
    oscillationDirection = dirDist(engine) == 0 ? 1.0f : -1.0f;
    phaseOffset = GetRandomValue(0, 628) / 100.0f;

    // Initialize position
    pos.x = xPos;
    pos.y = yPos;

    // Initialize pan speed
    panSpeed = 320 * 0.002f;

    // The gap between toilets
    gapBetweenToilets = GameHeight / 5.0f;

    // Assign random offset
    yOffsetRandomizer();

    // Reset scoring
    hasScored = false;
}

GoldToilets::~GoldToilets()
{
    UnloadTexture(_BottomToilet);
    UnloadTexture(_TopToilet);
}

void GoldToilets::Update(float deltaTime)
{
    if (isOscillating)
    {
        oscillationTimer += deltaTime;
        float amplitude = 40.0f;
        float baseYOffset = 10.0f;
        yOffset = baseYOffset + sinf(oscillationTimer * 1.5f + oscillationPhase + phaseOffset) * amplitude * oscillationDirection;
    }

    UpdateHitbox();
}

void GoldToilets::resetScore()
{
    hasScored = false;
}

void GoldToilets::UpdateHitbox()
{
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

void GoldToilets::Draw()
{
    DrawTexturePro(
        _TopToilet,
        { 0, 0, (float)(_TopToilet.width), (float)(_TopToilet.height) },
        { pos.x,  (2 * -_TopToilet.height / 3) + yOffset, (float)(_TopToilet.width), (float)(_TopToilet.height) },
        { 0, 0 },
        0.0f,
        WHITE
    );

    DrawTexturePro(
        _BottomToilet,
        { 0, 0, (float)(_BottomToilet.width), (float)(_BottomToilet.height) },
        { pos.x, (_BottomToilet.height / 3) + yOffset + (_BottomToilet.height / 7), (float)(_BottomToilet.width), (float)(_BottomToilet.height) },
        { 0, 0 },
        0.0f,
        WHITE
    );
}

Rectangle GoldToilets::GetTopHitbox()
{
    return hitboxTop;
}

Rectangle GoldToilets::GetBottomHitbox()
{
    return hitboxBottom;
}

std::vector<Rectangle> GoldToilets::GetHitboxes()
{
    if (!collisionEnabled) {
        return std::vector<Rectangle>();
    }

    std::vector<Rectangle> hitboxes;
    hitboxes.push_back(hitboxTop);
    hitboxes.push_back(hitboxBottom);
    return hitboxes;
}

void GoldToilets::SetCollisionEnabled(bool enabled)
{
    collisionEnabled = enabled;
}

void GoldToilets::SetOscillationEnabled(bool enabled)
{
    isOscillating = enabled;
}

void GoldToilets::yOffsetRandomizer()
{
    static std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(-20.0f, maxYOffset);
    yOffset = dist(engine);
}