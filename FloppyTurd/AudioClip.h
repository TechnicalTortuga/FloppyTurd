// AudioClip.h
#pragma once
#include "RaylibCompat.h"
#include <string>
#include "AudioManager.h"
#include "GameLog.h"

class AudioClip
{
public:
    AudioClip(const std::string& filePath) {
        music = LoadMusic(filePath.c_str());
        if (!music.player) {
            GameLog::Log("[AUDIOCLIP] Failed to load music: %s", filePath.c_str());
        } else {
            GameLog::Log("[AUDIOCLIP] Successfully loaded music: %s, attempting to play", filePath.c_str());
            Play(); // Auto-play when loaded
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    // Constructor for pre-loaded music from ResourceManager
    AudioClip(Music preloadedMusic) {
        music = preloadedMusic;
        if (!music.player) {
            GameLog::Log("[AUDIOCLIP] AudioClip created with invalid music resource");
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
            GameLog::Log("[AUDIOCLIP] Stopping music");
            StopMusic(music);
        }
        else {
            GameLog::Log("[AUDIOCLIP] Attempted to stop invalid or non-playing music");
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