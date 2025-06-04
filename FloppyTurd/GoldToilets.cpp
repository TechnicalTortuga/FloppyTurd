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

    // Initialize position
    pos.x = xPos;
    pos.y = yPos;

    // Initialize pan speed
    panSpeed = 320 * 0.002f;

    // The gap between toilets
    gapBetweenToilets = GameHeight / 5.0f;

    // Assign random offset the moment we construct this
    yOffsetRandomizer();

    // Reset any scoring or other stuff
    hasScored = false;

    std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> phaseDist(0.0f, 2 * PI);
    std::uniform_int_distribution<int> dirDist(0, 1);

    oscillationPhase = phaseDist(engine);
    oscillationDirection = dirDist(engine) == 0 ? 1.0f : -1.0f;
    phaseOffset = GetRandomValue(0, 628) / 100.0f;
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
    // Draw the top toilet
    DrawTexturePro(
        _TopToilet, // Toilet image
        { 0, 0, (float)(_TopToilet.width), (float)(_TopToilet.height) }, // How much of the image we want, x,y, width, height
        { pos.x,  (2 * -_TopToilet.height / 3) + yOffset, (float)(_TopToilet.width), (float)(_TopToilet.height) }, // Where the image is going, x, y, width, height
        { 0, 0 }, // Origin of image
        0.0f, // Rotation
        WHITE
    );

    // Draw the bottom toilet
    DrawTexturePro(
        _BottomToilet,
        { 0, 0, (float)(_BottomToilet.width), (float)(_BottomToilet.height) },
        { pos.x, (_BottomToilet.height / 3) + yOffset + (_BottomToilet.height / 7), (float)(_BottomToilet.width), (float)(_BottomToilet.height) },
        { 0, 0 }, // Origin of image
        0.0f, // Rotation
        WHITE
    );

    // DEBUG DRAWING HITBOXES
    //DrawRectangleLines(hitboxBottom.x, hitboxBottom.y, hitboxBottom.width, hitboxBottom.height, BLUE);
    //DrawRectangleLines(hitboxTop.x, hitboxTop.y, hitboxTop.width, hitboxTop.height, BLUE);
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
    std::vector<Rectangle> hitboxes;

    hitboxes.push_back(hitboxTop);
    hitboxes.push_back(hitboxBottom);

    return hitboxes;
}

void GoldToilets::yOffsetRandomizer()
{
    // -------- Option A: Using modern <random> (recommended) --------
    static std::default_random_engine engine{ std::random_device{}() };
    // If you want integer offsets, use uniform_int_distribution<int>.
    // For float offsets, do uniform_real_distribution<float>.
    std::uniform_real_distribution<float> dist(-20.0f, maxYOffset);

    yOffset = dist(engine);

    // -------- Option B: Old-school rand() approach --------
    // Make sure you called `srand()` once in main()
    //yOffset = static_cast<float>(rand() % static_cast<int>(maxYOffset));
}