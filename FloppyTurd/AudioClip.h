// AudioClip.h
#pragma once
#include "raylib.h"
#include <string>
#include "AudioManager.h"

class AudioClip
{
public:
    AudioClip(const std::string& filePath) {
        music = LoadMusicStream(filePath.c_str());
        if (!music.stream.buffer) {
            TraceLog(LOG_WARNING, "Failed to load music stream: %s", filePath.c_str());
            music.stream.buffer = nullptr; // Explicitly set to nullptr for safety
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        if (music.stream.buffer) { // Only unload if valid
            UnloadMusicStream(music);
        }
    }

    void Play() {
        if (music.stream.buffer) PlayMusicStream(music);
    }

    void Stop() {
        if (music.stream.buffer && IsMusicStreamPlaying(music)) { // Add playing check
            TraceLog(LOG_INFO, "Stopping music stream");
            StopMusicStream(music); // Line 31
        }
        else {
            TraceLog(LOG_WARNING, "Attempted to stop invalid or non-playing music stream");
        }
    }

    void Update() {
        if (music.stream.buffer) UpdateMusicStream(music);
    }

    bool IsPlaying() const {
        return music.stream.buffer && IsMusicStreamPlaying(music);
    }

    // NEW: Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        if (music.stream.buffer) SetMusicVolume(music, vol);
    }

    // Corrected: Set looping state
    void SetLooping(bool loop) {
        if (music.stream.buffer) music.looping = loop;
    }

    Music music;
};