#pragma once
#include <raylib.h>
#include <string>

class Sprite {
public:
    // “Modern” constructor
    Sprite(const std::string& filePath,
        int frameCount,
        float frameTime = 0.1f,
        float scale = 1.0f,
        Vector2 startPosition = { 0,0 });

    // “Legacy” constructor
    Sprite(const std::string& filePath,
        float x, float y,
        float width, float height,
        float frameTime = 0.1f,
        float scale = 1.0f);

    ~Sprite();

    // Update/Draw that rely on internally stored position
    void Update(float deltaTime);
    void Draw();

    // A helper so old code that calls “Draw(x,y)” continues to work
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

    int GetFrameIndex(){ return frameIndex; }
    void SetFrameIndex(int index)
    {
        frameIndex = index;
    }

private:
    Texture2D image;
    // For animation
    int   frameCount = 1;
    int   frameIndex = 0;
    float frameTime = 0.1f; // seconds per frame
    float elapsedTime = 0.0f;


    // For drawing
    float scale = 1.0f;
    Rectangle frameRec = { 0,0,0,0 };
};