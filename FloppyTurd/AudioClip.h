// AudioClip.h
#pragma once
#include "RaylibCompat.h"
#include <string>
#include "AudioManager.h"

class AudioClip
{
public:
    AudioClip(const std::string& filePath) {
        music = LoadMusicStream(filePath.c_str());
        if (!music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        ) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
            music.music = nullptr; // Explicitly set to nullptr for safety
#else
            music.audioData = nullptr; // Explicitly set to nullptr for safety
#endif
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        if (music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        ) { // Only unload if valid
            UnloadMusicStream(music);
        }
    }

    void Play() {
        if (music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        ) PlayMusicStream(music);
    }

    void Stop() {
        if (music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        && IsMusicStreamPlaying(music)) { // Add playing check
            TraceLog(LOG_INFO, "Stopping music stream");
            StopMusicStream(music); // Line 31
        }
        else {
            TraceLog(LOG_WARNING, "Attempted to stop invalid or non-playing music stream");
        }
    }

    void Update() {
        if (music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        ) UpdateMusicStream(music);
    }

    bool IsPlaying() const {
        return music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        && IsMusicStreamPlaying(music);
    }

    // NEW: Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        if (music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        ) SetMusicVolume(music, vol);
    }

    // Corrected: Set looping state
    void SetLooping(bool loop) {
        if (music
#if defined(__APPLE__) && TARGET_OS_IPHONE
            .music
#else
            .audioData
#endif
        ) music.looping = loop;
    }

    Music music;
};