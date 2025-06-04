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
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        UnloadMusicStream(music);
    }

    void Play() {
        PlayMusicStream(music);
    }

    void Stop() {
        StopMusicStream(music);
    }

    void Update() {
        UpdateMusicStream(music);
    }

    bool IsPlaying() const {
        return IsMusicStreamPlaying(music);
    }

    // NEW: Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        SetMusicVolume(music, vol);
    }

    void SetLooping(bool loop) {
        music.looping = loop;
    }

protected:
    Music music;
};