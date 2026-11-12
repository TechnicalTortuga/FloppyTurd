#include "PipeLayout.h"
#include "GoldToilets.h"
#include <iostream>
#include <random>
#include "ResourceCompat.h"
#include "GameSettings.h"
#include "TextureCache.h"

GoldToilets::GoldToilets(int xPos, int yPos)
{
	using namespace GameSettings;
	using namespace Resources;

	_TopToilet = TextureCache::Get(TopToiletGold);
	_BottomToilet = TextureCache::Get(BottomToiletGold);

	defaultOscillating = true;
	isOscillating = defaultOscillating;

	std::default_random_engine engine{ std::random_device{}() };
	std::uniform_real_distribution<float> phaseDist(0.0f, 2 * PI);
	std::uniform_int_distribution<int> dirDist(0, 1);

	oscillationPhase = phaseDist(engine);
	oscillationDirection = dirDist(engine) == 0 ? 1.0f : -1.0f;
	phaseOffset = GetRandomValue(0, 628) / 100.0f;

	pos.x = xPos;
	pos.y = yPos;

	panSpeed = 80.0f; // Default to Regular speed (adjusted by SetPanSpeed)

	gapBetweenToilets = GameSettings::GameHeight / 5.0f; // Use virtual resolution (180) from Playing.cpp

	yOffsetRandomizer();
	hasScored = false;

	UpdateHitbox();
}

GoldToilets::~GoldToilets()
{
	// Texture unloading is managed by TextureCache::Clear()
}

void GoldToilets::Update(float deltaTime)
{
	pos.x -= panSpeed * deltaTime;

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

void GoldToilets::UpdateHitbox() {
    const auto pair=PipeLayout::Calculate(pos.x,yOffset,_TopToilet.height,_BottomToilet.height);
    hitboxTop=pair.topHit; hitboxBottom=pair.bottomHit;
}

void GoldToilets::Draw() {
    PipeLayout::Draw(_TopToilet,_BottomToilet,pos.x,yOffset);
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

void GoldToilets::SetPanSpeed(float speed)
{
	panSpeed = speed;
}

void GoldToilets::yOffsetRandomizer()
{
	static std::default_random_engine engine{ std::random_device{}() };
	std::uniform_real_distribution<float> dist(-20.0f, maxYOffset);
	yOffset = dist(engine);
}