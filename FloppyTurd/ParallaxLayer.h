// ParallaxLayer.h
#pragma once
#include "PlatformAPI.h"
#include <vector>
#include <string>
#include "GameSettings.h"
#include "Layer.h"

class ParallaxLayer : public Layer {
public:
	ParallaxLayer(const std::vector<std::string>& texturePaths, float speed, float scale);
	~ParallaxLayer();

	void Update(float deltaTime) override;
	void Draw() override;

private:
	std::vector<Texture2D> textures;  // Managed by TextureCache
	float speed;
	float scale;

	// Use a single offset and a base texture index for the cycling pattern.
	float scrollOffset;     // The current horizontal offset in our native resolution
	int baseTextureIndex;   // Which texture starts the cycle

	int numSegments;        // Number of copies needed to cover the screen (native width)
};