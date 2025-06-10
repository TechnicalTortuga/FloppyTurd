#include "ParallaxLayer.h"
#include <algorithm>
#include "TextureCache.h"

ParallaxLayer::ParallaxLayer(const std::vector<std::string>& texturePaths, float speed, float scale)
	: speed(speed), scale(scale), scrollOffset(0.0f), baseTextureIndex(0)
{
	using namespace GameSettings;

	// Load all textures via TextureCache
	for (const auto& path : texturePaths) {
		Texture2D tex = TextureCache::Get(path.c_str());
		if (tex.id == 0) {
			TraceLog(LOG_WARNING, "ParallaxLayer: Failed to load texture from cache at path %s.", path.c_str());
		}
		textures.push_back(tex);
	}

	// Determine how many segments are needed based on the first texture's width
	float textureWidth = textures[0].width * scale;
	const float screenWidth = GameWidth;  // e.g., 320
	numSegments = static_cast<int>(screenWidth / textureWidth) + 2;
}

ParallaxLayer::~ParallaxLayer() {
	// Texture unloading is managed by TextureCache::Clear()
}

void ParallaxLayer::Update(float deltaTime) {
	float textureWidth = textures[0].width * scale;

	// Move the scroll offset leftward.
	scrollOffset -= speed * deltaTime;

	// When we've scrolled a full texture width, wrap around.
	if (scrollOffset <= -textureWidth) {
		scrollOffset += textureWidth;

		// If there are multiple textures, move to the next in the cycle.
		if (textures.size() > 1) {
			baseTextureIndex = (baseTextureIndex + 1) % textures.size();
		}
	}
}

void ParallaxLayer::Draw() {
	float textureWidth = textures[0].width * scale;
	float textureHeight = textures[0].height * scale;

	// Calculate how many copies are needed to cover the native screen width.
	int copies = numSegments;  // Already computed in constructor

	// Snap the scrollOffset to an integer value for crisp rendering.
	float startX = roundf(scrollOffset);

	for (int i = 0; i < copies; i++) {
		// Determine the texture index in the cycle.
		int textureIndex = (baseTextureIndex + i) % textures.size();
		Texture2D& currentTexture = textures[textureIndex];

		// Compute the x position for this segment.
		float x = startX + i * textureWidth;

		Rectangle destRec = { x, 0.0f, textureWidth, textureHeight };
		Rectangle sourceRec = { 0.0f, 0.0f, (float)currentTexture.width, (float)currentTexture.height };
		Vector2 origin = { 0.0f, 0.0f };

		DrawTexturePro(currentTexture, sourceRec, destRec, origin, 0.0f, WHITE);
	}
}