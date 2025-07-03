#pragma once
#include "RaylibCompat.h"
#ifndef LEVEL_H
#define LEVEL_H
#include "Obstacle.h"
#include "AudioClip.h"
#include <vector>
#include <memory>
#include "PickUp.h"

class Level
{
public:
    Level()
        : levelMusicSlow(nullptr), levelMusicRegular(nullptr), levelMusicFast(nullptr), currentMusic(nullptr)
    {
    }
    virtual ~Level() {
        if (levelMusicSlow) {
            delete levelMusicSlow;
            levelMusicSlow = nullptr;
        }
        if (levelMusicRegular) {
            delete levelMusicRegular;
            levelMusicRegular = nullptr;
        }
        if (levelMusicFast) {
            delete levelMusicFast;
            levelMusicFast = nullptr;
        }
        // Do not delete currentMusic here, as it may alias one of the above pointers
        currentMusic = nullptr; // Just nullify it to avoid dangling pointer
    }

    virtual void Draw() const = 0;
    virtual void Update(float deltaTime) = 0;
    virtual bool checkForCollisions(Vector2 circleCenter, float circleRadius) = 0;
    virtual bool checkForPointGain(Vector2 circleCenter, float circleRadius) = 0;
    virtual const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() = 0;
    virtual void SetSwingingPipes(bool enable) = 0;
    virtual void SetDifficulty(int difficultyIndex) = 0; // New: Set difficulty (0=Runny, 1=Regular, 2=Rough)
    virtual void SetPanSpeed(float speed) = 0; // New: Set pan speed for obstacles and enemies

    virtual void AddPickUp(std::shared_ptr<PickUp> pickup);
    virtual std::vector<std::shared_ptr<PickUp>>& GetPickUps();

    // Music-related functions with nullptr guards
    virtual AudioClip* GetAudioClip() const { return currentMusic; }

    virtual void PlayMusic() {
        if (currentMusic) currentMusic->Play();
    }

    virtual void StopMusic() {
        if (currentMusic) currentMusic->Stop();
    }

    virtual void UpdateMusic() {
        if (currentMusic) currentMusic->Update();
    }

protected:
    AudioClip* levelMusicSlow; // Slow theme (Runny)
    AudioClip* levelMusicRegular; // Regular theme (Regular)
    AudioClip* levelMusicFast; // Fast theme (Rough)
    AudioClip* currentMusic; // Currently selected music
    std::vector<std::shared_ptr<PickUp>> pickups;
};

#endif // LEVEL_H