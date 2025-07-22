#pragma once
#include "PlatformTypes.h"
#include "PlatformAPI.h"
#include <string>

class SoundEffect {
public:
    SoundEffect(const std::string& path) {
        sound = LoadSound(path.c_str());
    }

    ~SoundEffect() {
        UnloadSound(sound);
    }

    void Play(float volume = 1.0f) {
        SetSoundVolume(sound, volume);
        PlaySound(sound);
    }

private:
    Sound sound;
};
