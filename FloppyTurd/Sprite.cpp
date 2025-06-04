#include "Sprite.h"
#include <raylib.h>

// Modern constructor: Initialize with texture, animation settings, and position
Sprite::Sprite(const std::string& filePath, int frameCount, float frameTime, float scale, Vector2 startPosition)
    : frameCount(frameCount), frameTime(frameTime), scale(scale), position(startPosition)
{
    image = LoadTexture(filePath.c_str());
    if (image.id == 0) {
        TraceLog(LOG_WARNING, "Sprite: Failed to load texture at path %s. Visuals will be missing.", filePath.c_str());
        frameRec = { 0, 0, 32, 32 }; // Fallback dimensions if texture fails to load
    }
    else {
        frameRec = { 0, 0, (float)image.width / frameCount, (float)image.height };
    }
}

// Legacy constructor: Initialize with specific dimensions and position
Sprite::Sprite(const std::string& filePath, float x, float y, float width, float height, float frameTime, float scale)
    : frameTime(frameTime), scale(scale)
{
    image = LoadTexture(filePath.c_str());
    position = { x, y };
    if (image.id == 0) {
        TraceLog(LOG_WARNING, "Sprite: Failed to load texture at path %s. Visuals will be missing.", filePath.c_str());
        frameRec = { 0, 0, width, height }; // Use provided dimensions as fallback
    }
    else {
        frameRec = { 0, 0, width, height }; // Override texture dimensions with provided ones
    }
}

// Destructor: Unload the texture to free memory
Sprite::~Sprite()
{
    UnloadTexture(image);
}

// Update: Advance the animation frame based on elapsed time
void Sprite::Update(float deltaTime)
{
    if (frozenFrame >= 0) {
        frameIndex = frozenFrame; // If frozen, stay on the specified frame
        return;
    }

    if (frameCount > 1) { // Only animate if there's more than one frame
        elapsedTime += deltaTime;
        while (elapsedTime >= frameTime) {
            elapsedTime -= frameTime;
            frameIndex++;
            if (frameIndex >= frameCount) {
                frameIndex = 0; // Loop back to the first frame
                loopedOnce = true; // Mark that we've completed one loop
                isAnimationFinished = true; // Mark animation as finished (though it loops)
            }
        }
    }
}

// Draw: Render the current frame at the stored position
void Sprite::Draw()
{
    if (image.id > 0) {
        Rectangle source = GetSourceRect();
        Rectangle dest = { position.x, position.y, frameRec.width * scale, frameRec.height * scale };
        DrawTexturePro(image, source, dest, { 0, 0 }, 0.f, WHITE);
    }
}

// Draw (legacy): Render the current frame at the specified position
void Sprite::Draw(float x, float y)
{
    if (image.id > 0) {
        Rectangle source = GetSourceRect();
        Rectangle dest = { x, y, frameRec.width * scale, frameRec.height * scale };
        DrawTexturePro(image, source, dest, { 0, 0 }, 0.f, WHITE);
    }
}

// SetPosition: Update the position using separate x, y coordinates
void Sprite::SetPosition(float x, float y)
{
    position = { x, y };
}

// SetPosition: Update the position using a Vector2
void Sprite::SetPosition(const Vector2& pos)
{
    position = pos;
}

// GetPosition: Return the current position
Vector2 Sprite::GetPosition() const
{
    return position;
}

// SetScale: Update the sprite's scale
void Sprite::SetScale(float s)
{
    scale = s;
}

// GetScale: Return the current scale
float Sprite::GetScale() const
{
    return scale;
}

// GetScaledWidth: Return the scaled width of the current frame
float Sprite::GetScaledWidth() const
{
    return frameRec.width * scale;
}

// GetScaledHeight: Return the scaled height of the current frame
float Sprite::GetScaledHeight() const
{
    return frameRec.height * scale;
}

// ResetAnimation: Reset the animation state
void Sprite::ResetAnimation()
{
    frameIndex = 0;
    elapsedTime = 0.f;
    isAnimationFinished = false;
    loopedOnce = false;
}

// EnablePixelPerfectRotation: Placeholder (no-op for now, as rotation isn't used)
void Sprite::EnablePixelPerfectRotation()
{
    // No-op: Sprite doesn't currently support rotation, but this could be implemented later
}

// SetFrameFrozen: Freeze the animation on a specific frame (-1 to unfreeze)
void Sprite::SetFrameFrozen(int index)
{
    if (index < -1 || index >= frameCount) {
        TraceLog(LOG_WARNING, "Sprite: Invalid frame index %d for freezing (frameCount: %d). Ignoring.", index, frameCount);
        return;
    }
    frozenFrame = index;
    if (frozenFrame >= 0) {
        frameIndex = frozenFrame; // Immediately set the frame
    }
}

// IsFrozen: Check if the animation is frozen on a specific frame
bool Sprite::IsFrozen() const
{
    return frozenFrame >= 0;
}