#pragma once
#include "PlatformAPI.h"
#include <string>

class SoundManager {
public:
    // Singleton accessor
    static SoundManager& GetInstance() {
        static SoundManager instance;
        return instance;
    }

    // Get/set sound volume (range 0.0f to 1.0f)
    float GetVolume() const { return volume; }
    void SetVolume(float vol) {
        volume = (vol < 0.0f) ? 0.0f : (vol > 1.0f ? 1.0f : vol);
    }
    void IncreaseVolume() { SetVolume(volume + 0.1f); }
    void DecreaseVolume() { SetVolume(volume - 0.1f); }

    // Play a sound clip at the current sound volume.
    void PlaySoundClip(Sound soundClip);

    // Optionally, helper methods for loading/unloading sounds.
    Sound LoadSoundClip(const std::string& filePath);
    void UnloadSoundClip(Sound soundClip);

private:
    SoundManager() : volume(1.0f) { }
    ~SoundManager() { }
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    float volume;
};
