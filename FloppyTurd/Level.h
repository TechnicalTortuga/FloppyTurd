#pragma once
#include <raylib.h>
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
        : levelMusic(nullptr)
    {
        // any other base initialization
    }
    virtual ~Level() {
        if (levelMusic) {
            delete levelMusic;
            levelMusic = nullptr; // Set to nullptr after deletion
        }
    }

    virtual void Draw() const = 0;
    virtual void Update(float deltaTime) = 0;
    virtual bool checkForCollisions(Vector2 circleCenter, float circleRadius) = 0;
    virtual bool checkForPointGain(Vector2 circleCenter, float circleRadius) = 0;
    virtual const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() = 0;

    virtual void AddPickUp(std::shared_ptr<PickUp> pickup);
    virtual std::vector<std::shared_ptr<PickUp>>& GetPickUps();

    // Music-related functions
    virtual AudioClip* GetAudioClip() const { return levelMusic; } // Return the associated music

    virtual void PlayMusic() {
        if (levelMusic) levelMusic->Play();
    }

    virtual void StopMusic() {
        if (levelMusic) levelMusic->Stop();
    }

    virtual void UpdateMusic() {
        if (levelMusic) levelMusic->Update();
    }

    AudioClip* levelMusic;

protected:
    std::vector<std::shared_ptr<PickUp>> pickups;
};

#endif // LEVEL_H