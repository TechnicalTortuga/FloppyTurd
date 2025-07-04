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
        if (!music.ctxData) {
            music.ctxData = nullptr; // Explicitly set to nullptr for safety
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        if (music.ctxData) { // Only unload if valid
            UnloadMusicStream(music);
        }
    }

    void Play() {
        if (music.ctxData) PlayMusicStream(music);
    }

    void Stop() {
        if (music.ctxData && IsMusicStreamPlaying(music)) { // Add playing check
            TraceLog(LOG_INFO, "Stopping music stream");
            StopMusicStream(music); // Line 31
        }
        else {
            TraceLog(LOG_WARNING, "Attempted to stop invalid or non-playing music stream");
        }
    }

    void Update() {
        if (music.ctxData) UpdateMusicStream(music);
    }

    bool IsPlaying() const {
        return music.ctxData && IsMusicStreamPlaying(music);
    }

    // NEW: Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        if (music.ctxData) SetMusicVolume(music, vol);
    }

    // Corrected: Set looping state
    void SetLooping(bool loop) {
        if (music.ctxData) music.looping = loop;
    }

    Music music;
};