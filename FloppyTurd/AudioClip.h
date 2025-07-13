// AudioClip.h - Platform-agnostic version
#pragma once
#include <string>
#include "AudioManager.h"
#include "GameLog.h"
#include "PlatformAPI.h"

class AudioClip
{
public:
    AudioClip(const std::string& filePath) {
        m_platformMusicHandle = LoadMusic(filePath.c_str());
        if (!m_platformMusicHandle.player) {
            GameLog::Log("[AUDIOCLIP] Failed to load music: %s", filePath.c_str());
        } else {
            GameLog::Log("[AUDIOCLIP] Successfully loaded music: %s, attempting to play", filePath.c_str());
            Play(); // Auto-play when loaded
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    // Constructor for pre-loaded music from ResourceManager
    AudioClip(void* platformMusicHandle) {
        m_platformMusicHandle.player = platformMusicHandle;
        m_platformMusicHandle.length = 0; // Will be set by platform implementation
        if (!m_platformMusicHandle.player) {
            GameLog::Log("[AUDIOCLIP] AudioClip created with invalid music resource");
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        if (m_platformMusicHandle.player) { // Only unload if valid
            UnloadMusic(m_platformMusicHandle);
        }
    }

    void Play() {
        if (m_platformMusicHandle.player) PlayMusic(m_platformMusicHandle);
    }

    void PlayLoop() {
        if (m_platformMusicHandle.player) {
            PlayMusic(m_platformMusicHandle);
            // Note: Looping is handled by the platform implementation
        }
    }

    void Stop() {
        if (m_platformMusicHandle.player && IsPlaying()) { // Add playing check
            GameLog::Log("[AUDIOCLIP] Stopping music");
            StopMusic(m_platformMusicHandle);
        }
        else {
            GameLog::Log("[AUDIOCLIP] Attempted to stop invalid or non-playing music");
        }
    }

    void Pause() {
        if (m_platformMusicHandle.player) PauseMusic(m_platformMusicHandle);
    }

    void Resume() {
        if (m_platformMusicHandle.player) ResumeMusic(m_platformMusicHandle);
    }

    void Update() {
        if (m_platformMusicHandle.player) UpdateMusic(m_platformMusicHandle);
    }

    bool IsPlaying() const {
        return m_platformMusicHandle.player && IsMusicPlaying(m_platformMusicHandle);
    }

    // Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        if (m_platformMusicHandle.player) SetMusicVolume(m_platformMusicHandle, vol);
    }

    // Set looping state for the music
    void SetLooping(bool looping) {
        if (m_platformMusicHandle.player) SetMusicLooping(m_platformMusicHandle, looping);
    }

    // Getter for platform handle (for compatibility with existing code)
    void* GetPlatformHandle() const { return m_platformMusicHandle.player; }
    
    // Getter for Music struct (for duration and other Music-specific operations)
    const Music& GetMusic() const { return m_platformMusicHandle; }

private:
    Music m_platformMusicHandle = { 0, 0 };  // Initialize with player=nullptr, length=0
};