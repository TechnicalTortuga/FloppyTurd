// AudioClip.h
#pragma once
#include "raylib.h"
#include <string>
#include "AudioManager.h"
#include "WebAssets.h"

class AudioClip
{
public:
    AudioClip(const std::string& filePath) : path(filePath) {
#ifndef __EMSCRIPTEN__
        Load();
#endif
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        if (music.stream.buffer) { // Only unload if valid
            UnloadMusicStream(music);
        }
        WebAssets::Release(path.c_str());
    }

    void Play() {
        wantsPlay = true;
        if (Load()) PlayMusicStream(music);
    }

    void Stop() {
        wantsPlay = false;
        if (music.stream.buffer) StopMusicStream(music);
#ifdef __EMSCRIPTEN__
        // Keep compressed music on disk in the browser cache, not in WASM RAM.
        if (music.stream.buffer) UnloadMusicStream(music);
        music = {};
        WebAssets::Release(path.c_str());
#endif
    }

    void Update() {
        if (wantsPlay && !music.stream.buffer && Load()) PlayMusicStream(music);
        if (music.stream.buffer) UpdateMusicStream(music);
    }

    bool IsPlaying() const {
        return music.stream.buffer && IsMusicStreamPlaying(music);
    }
    bool IsPlaybackRequested() const { return wantsPlay; }

    // NEW: Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        volume = vol;
        if (music.stream.buffer) SetMusicVolume(music, vol);
    }

    // Corrected: Set looping state
    void SetLooping(bool loop) {
        looping = loop;
        if (music.stream.buffer) music.looping = loop;
    }

    Music music{};
private:
    std::string path;
    bool wantsPlay = false, looping = true;
    float volume = 1.0f;
    bool Load() {
        if (music.stream.buffer) return true;
        if (WebAssets::File(path.c_str()) != 1) return false;
        music = LoadMusicStream(path.c_str());
        if (!music.stream.buffer) return false;
        music.looping = looping;
        SetMusicVolume(music, volume);
        return true;
    }
};
