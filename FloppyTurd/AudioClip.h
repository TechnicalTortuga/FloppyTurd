// AudioClip.h
#pragma once
#include "RaylibCompat.h"
#include <string>
#include "AudioManager.h"

class AudioClip
{
public:
    AudioClip(const std::string& filePath) {
        music = LoadMusic(filePath.c_str());
        if (!music.player) {
            TraceLog(LOG_WARNING, "Failed to load music: %s", filePath.c_str());
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        if (music.player) { // Only unload if valid
            UnloadMusic(music);
        }
    }

    void Play() {
        if (music.player) PlayMusic(music);
    }

    void PlayLoop() {
        if (music.player) PlayMusicLoop(music);
    }

    void Stop() {
        if (music.player && IsMusicPlaying(music)) { // Add playing check
            TraceLog(LOG_INFO, "Stopping music");
            StopMusic(music);
        }
        else {
            TraceLog(LOG_WARNING, "Attempted to stop invalid or non-playing music");
        }
    }

    void Pause() {
        if (music.player) PauseMusic(music);
    }

    void Resume() {
        if (music.player) ResumeMusic(music);
    }

    void Update() {
        // No update needed for iOS AVAudioPlayer
    }

    bool IsPlaying() const {
        return music.player && IsMusicPlaying(music);
    }

    // NEW: Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        if (music.player) SetMusicVolume(music, vol);
    }

    // Note: Looping is handled by PlayLoop() method instead of a looping field

    // NEW: Set looping state for the music
    void SetLooping(bool looping) {
        if (music.player) ::SetLooping(music, looping);
    }

    Music music;
};