#include "Sprite.h"
#include <stdexcept>
#include <iostream>

Sprite::Sprite(const std::string& filePath,
    int frameCount,
    float frameTime,
    float scale,
    Vector2 startPosition)
    : frameCount(frameCount),
    frameTime(frameTime),
    scale(scale),
    position(startPosition)
{
    image = LoadTexture(filePath.c_str());
    if (image.width == 0 || image.height == 0) {
        throw std::runtime_error("Failed to load sprite: " + filePath);
    }

    // For a single-row sprite sheet: each frame is (image.width / frameCount)
    float frameW = static_cast<float>(image.width) / frameCount;
    float frameH = static_cast<float>(image.height);

    frameRec = { 0.0f, 0.0f, frameW, frameH };
}

// --------------------------------------------------

Sprite::Sprite(const std::string& filePath,
    float x, float y,
    float width, float height,
    float frameTime,
    float scale)
    : frameCount(1),          // just 1 frame
    frameIndex(0),
    frameTime(frameTime),
    elapsedTime(0.0f),
    isAnimationFinished(false),
    scale(scale),
    position({ x, y })
{
    image = LoadTexture(filePath.c_str());
    if (image.width == 0 || image.height == 0) {
        throw std::runtime_error("Failed to load sprite: " + filePath);
    }

    // The “legacy” code gave us width/height it expects. 
    // We'll treat that as the slice of the texture we use.
    // If the entire image is exactly that big, fine. Otherwise you can adapt as needed.
    frameRec = {
        0.0f,
        0.0f,
        width,
        height
    };
}

Sprite::~Sprite() {
    UnloadTexture(image);
}

void Sprite::Update(float deltaTime) {
    if (frameCount > 1) {
        elapsedTime += deltaTime;

        if (elapsedTime >= frameTime) {
            elapsedTime = 0.0f;
            frameIndex++;

            if (frameIndex >= frameCount) {
                loopedOnce = true;
                frameIndex = 0;  //  Loop back to the first frame
                isAnimationFinished = false;
            }
        }
    }
}

void Sprite::Draw() {
    // Correctly determine the current frame’s source rectangle without modifying the original frameRec
    Rectangle sourceRec = {
        frameRec.width * frameIndex, // Move the X position per frame
        0.0f,
        frameRec.width,
        frameRec.height
    };

    // Destination rectangle = scaled
    Rectangle destRec = {
        position.x,
        position.y,
        frameRec.width * scale,
        frameRec.height * scale
    };

    Vector2 origin = { 0, 0 };
    DrawTexturePro(image, sourceRec, destRec, origin, 0.0f, WHITE);
}

// “Bridge” draw method that old code can call with x,y each time
void Sprite::Draw(float x, float y) {
    SetPosition(x, y);
    Draw();
}

// Set/Get
void Sprite::SetPosition(float x, float y) {
    position = { x, y };
}

void Sprite::SetPosition(const Vector2& pos) {
    position = pos;
}

Vector2 Sprite::GetPosition() const {
    return position;
}

void Sprite::SetScale(float s) {
    scale = s;
}
float Sprite::GetScale() const {
    return scale;
}

float Sprite::GetScaledWidth() const {
    return frameRec.width * scale;
}
float Sprite::GetScaledHeight() const {
    return frameRec.height * scale;
}

void Sprite::ResetAnimation()
{
    frameIndex = 0;
    isAnimationFinished = false;
}
