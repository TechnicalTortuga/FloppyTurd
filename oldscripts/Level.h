#pragma once
#include <raylib.h>
#ifndef LEVEL_H
#define LEVEL_H
#include "Obstacle.h"
#include "AudioClip.h"
#include <vector>
#include <memory>

class Level
{
public:
    virtual ~Level() = default;

    virtual void Draw() const = 0;
    virtual void Update(float deltaTime) = 0;
    virtual bool checkForCollisions(Vector2 circleCenter, float circleRadius) = 0;
    virtual bool checkForPointGain(Vector2 circleCenter, float circleRadius) = 0;
    virtual const std::vector<std::shared_ptr<Obstacle>>& getObjLoc() = 0;

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

};

#endif // LEVEL_H