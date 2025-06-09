// Credits.h: Header for the Credits scene in Floppy Turd, defining properties and behavior.
// Updated to include CameraSystem for background scrolling.

#pragma once
#include "raylib.h"
#include "Sprite.h"
#include <vector>
#include <string>
#include "AudioClip.h"
#include "Game.h"
#include "ToiletPair.h"
#include "CameraSystem.h"
#include "ParallaxLayer.h"

class Credits
{
public:
    Credits(Game* game);
    ~Credits();

    void Update(float deltaTime);
    void Draw() const;
    void HandleInput();
    bool IsComplete() const;
    AudioClip* GetMusic() const { return music; }
    void Reset();
    void UpdateMusic();

private:
    struct CreditEntry
    {
        std::string title;
        std::string name;
        float yOffset;
        CreditEntry(const std::string& t, const std::string& n, float offset) : title(t), name(n), yOffset(offset) {}
    };

    CameraSystem* cameraSystem; // For background scrolling
    Texture2D background; // Retained for potential future use
    AudioClip* music;
    float textScrollOffset;
    const float scrollSpeed = 40.0f;
    float totalScrollTime;
    float elapsedTime;

    std::shared_ptr<Sprite> turdletSprite;
    Vector2 turdletPos;
    float turdletBounce;

    Texture2D finLogoNormal;
    Texture2D finLogoHover;
    Vector2 finLogoPos;
    bool finLogoHovered;
    float finLogoScale;

    std::vector<CreditEntry> creditEntries;
    Game* game;
    Font font;
    std::vector<std::shared_ptr<ToiletPair>> pipes;
};