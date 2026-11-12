#include "PipeLayout.h"
#include "ToiletPair.h"
#include <iostream>
#include <random>
#include "TextureCache.h"

ToiletPair::ToiletPair(int xPos, int yPos, bool isSnowy, bool creditsMode)
	: ToiletPair(xPos, yPos, isSnowy)
{
	if (creditsMode)
	{
		yOffset = 0.0f;
		isOscillating = false;
		defaultOscillating = false;
	}
}

ToiletPair::ToiletPair(int xPos, int yPos, bool isSnowy)
{
	using namespace GameSettings;
	using namespace Resources;

	_TopToilet = isSnowy ? TextureCache::Get(TopToiletSnow) : TextureCache::Get(TopToilet);
	_BottomToilet = isSnowy ? TextureCache::Get(BottomToiletSnow) : TextureCache::Get(BottomToilet);

	isSnowyVariant = isSnowy;
	defaultOscillating = isSnowy;
	isOscillating = defaultOscillating;

	std::default_random_engine engine{ std::random_device{}() };
	std::uniform_real_distribution<float> phaseDist(0.0f, 2 * PI);
	std::uniform_int_distribution<int> dirDist(0, 1);

	oscillationPhase = phaseDist(engine);
	oscillationDirection = dirDist(engine) == 0 ? 1.0f : -1.0f;
	phaseOffset = GetRandomValue(0, 628) / 100.0f;

	pos.x = xPos;
	pos.y = yPos;

	toiletScale = 1.0f;
	panSpeed = 80.0f;

	gapBetweenToilets = GameHeight / 5.0f;

	yOffsetRandomizer();

	hasScored = false;
    UpdateHitbox();
}

ToiletPair::~ToiletPair()
{
	// Texture unloading is managed by TextureCache::Clear()
}

void ToiletPair::Update(float deltaTime)
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

void ToiletPair::resetScore()
{
	hasScored = false;
}

void ToiletPair::UpdateHitbox() {
    const auto pair=PipeLayout::Calculate(pos.x,yOffset,_TopToilet.height,_BottomToilet.height);
    hitboxTop=pair.topHit; hitboxBottom=pair.bottomHit;
}

void ToiletPair::Draw() {
    PipeLayout::Draw(_TopToilet,_BottomToilet,pos.x,yOffset);
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

void ToiletPair::SetPanSpeed(float speed)
{
	panSpeed = speed;
}

void ToiletPair::yOffsetRandomizer()
{
	static std::default_random_engine engine{ std::random_device{}() };
	std::uniform_real_distribution<float> dist(-20.0f, maxYOffset);
	yOffset = dist(engine);
}