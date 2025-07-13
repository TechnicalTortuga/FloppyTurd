#pragma once
#include "PlatformAPI.h"
#include <string>
#include "TextureAtlas.h"

class Sprite {
public:
	// Modern constructor
	Sprite(const std::string& filePath,
		int frameCount,
		float frameTime = 0.1f,
		float scale = 1.0f,
		Vector2 startPosition = { 0,0 },
		AtlasCategory atlasCategory = AtlasCategory::PLAYER_SPRITES);

	// Legacy constructor
	Sprite(const std::string& filePath,
		float x, float y,
		float width, float height,
		float frameTime = 0.1f,
		float scale = 1.0f);

	~Sprite();

	// Update/Draw that rely on internally stored position
	void Update(float deltaTime);
	void Draw();

	// A helper so old code that calls Draw(x,y) continues to work
	void Draw(float x, float y);

	// Accessors/mutators
	void SetPosition(float x, float y);
	void SetPosition(const Vector2& pos);
	Vector2 GetPosition() const;

	void SetScale(float s);
	float GetScale() const;

	float GetScaledWidth() const;
	float GetScaledHeight() const;

	Vector2 position = { 0,0 };

	bool isAnimationFinished = false;
	bool IsAnimationFinished() { return isAnimationFinished; }
	bool loopedOnce = false;
	bool hasLoopedOnce() { return loopedOnce; };
	void ResetAnimation();

	void EnablePixelPerfectRotation();

	void SetFrameFrozen(int index);   // -1 to unfreeze
	bool IsFrozen() const;

	Texture2D GetTexture() const { return image; }  // Optional: expose for raw drawing

	int GetFrameIndex() { return frameIndex; }
	void SetFrameIndex(int index)
	{
		frameIndex = index;
	}
	bool HasPlayedFrames(int compareIndex) {
		return frameIndex == compareIndex;
	}
	Rectangle GetSourceRect() const {
		Rectangle src = frameRec;
		src.x = frameRec.width * frameIndex;
		return src;
	}

	float GetWidth() const { return frameRec.width * scale; }
	float GetHeight() const { return frameRec.height * scale; }

	float GetFrameWidth()  const { return frameRec.width; }
	float GetFrameHeight() const { return frameRec.height; }

private:
	Texture2D image;  // Now managed by TextureCache, but stored for convenience
	// For animation
	int   frameCount = 1;
	int   frameIndex = 0;
	float frameTime = 0.1f; // seconds per frame
	float elapsedTime = 0.0f;

	int frozenFrame = -1;  // -1 = not frozen

	// For drawing
	float scale = 1.0f;
	Rectangle frameRec = { 0,0,0,0 };

	// --- TextureAtlas support ---
	bool isAtlased = false;
	Texture2D atlasTexture = { 0, 0, 0, 0, 0, nullptr }; // Initialize with 6 values: id, width, height, mipmaps, format, texture
	Rectangle atlasRegion = { 0,0,0,0 }; // The region in the atlas for this sprite
	AtlasCategory atlasCategory = AtlasCategory::PLAYER_SPRITES;
};