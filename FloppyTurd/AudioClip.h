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
        m_platformMusicHandle = PlatformAPI::LoadMusic(filePath.c_str());
        if (!m_platformMusicHandle) {
            GameLog::Log("[AUDIOCLIP] Failed to load music: %s", filePath.c_str());
        } else {
            GameLog::Log("[AUDIOCLIP] Successfully loaded music: %s, attempting to play", filePath.c_str());
            Play(); // Auto-play when loaded
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    // Constructor for pre-loaded music from ResourceManager
    AudioClip(void* platformMusicHandle) {
        m_platformMusicHandle = platformMusicHandle;
        if (!m_platformMusicHandle) {
            GameLog::Log("[AUDIOCLIP] AudioClip created with invalid music resource");
        }
        AudioManager::GetInstance().RegisterClip(this);
    }

    virtual ~AudioClip() {
        AudioManager::GetInstance().UnregisterClip(this);
        if (m_platformMusicHandle) { // Only unload if valid
            PlatformAPI::UnloadMusic(m_platformMusicHandle);
        }
    }

    void Play() {
        if (m_platformMusicHandle) PlatformAPI::PlayMusic(m_platformMusicHandle);
    }

    void PlayLoop() {
        if (m_platformMusicHandle) {
            PlatformAPI::PlayMusic(m_platformMusicHandle);
            // Note: Looping is handled by the platform implementation
        }
    }

    void Stop() {
        if (m_platformMusicHandle && IsPlaying()) { // Add playing check
            GameLog::Log("[AUDIOCLIP] Stopping music");
            PlatformAPI::StopMusic(m_platformMusicHandle);
        }
        else {
            GameLog::Log("[AUDIOCLIP] Attempted to stop invalid or non-playing music");
        }
    }

    void Pause() {
        if (m_platformMusicHandle) PlatformAPI::PauseMusic(m_platformMusicHandle);
    }

    void Resume() {
        if (m_platformMusicHandle) PlatformAPI::ResumeMusic(m_platformMusicHandle);
    }

    void Update() {
        if (m_platformMusicHandle) PlatformAPI::UpdateMusic(m_platformMusicHandle);
    }

    bool IsPlaying() const {
        return m_platformMusicHandle && PlatformAPI::IsMusicPlaying(m_platformMusicHandle);
    }

    // Set the music volume (expected range 0.0 to 1.0)
    void SetVolume(float vol) {
        if (m_platformMusicHandle) PlatformAPI::SetMusicVolume(m_platformMusicHandle, vol);
    }

    // Set looping state for the music
    void SetLooping(bool looping) {
        if (m_platformMusicHandle) PlatformAPI::SetMusicLooping(m_platformMusicHandle, looping);
    }

    // Getter for platform handle (for compatibility with existing code)
    void* GetPlatformHandle() const { return m_platformMusicHandle; }

private:
    void* m_platformMusicHandle; // Platform-specific music handle
};