#include "AudioManagerBridge.h"
#include "AudioManager.h"
#include "SoundManager.h"

extern "C" {

AudioManagerRef AudioManager_getInstance() {
    return (AudioManagerRef)&AudioManager::GetInstance();
}

SoundManagerRef SoundManager_getInstance() {
    return (SoundManagerRef)&SoundManager::GetInstance();
}

void AudioManager_playMusic(AudioManagerRef mgr, const char* fileName) {
    if (!mgr) return;
    // AudioManager doesn't have PlayMusic method, this is a no-op for compatibility
}

void AudioManager_stopMusic(AudioManagerRef mgr) {
    if (!mgr) return;
    ((AudioManager*)mgr)->StopMusic();
}

void AudioManager_pauseMusic(AudioManagerRef mgr) {
    if (!mgr) return;
    // AudioManager doesn't have PauseMusic method, this is a no-op for compatibility
}

void AudioManager_resumeMusic(AudioManagerRef mgr) {
    if (!mgr) return;
    // AudioManager doesn't have ResumeMusic method, this is a no-op for compatibility
}

void AudioManager_setMusicVolume(AudioManagerRef mgr, float volume) {
    if (!mgr) return;
    ((AudioManager*)mgr)->SetMusicVolume((int)(volume * 10.0f));
}

void AudioManager_setMusicMuted(AudioManagerRef mgr, bool muted) {
    if (!mgr) return;
    // AudioManager doesn't have SetMusicMuted method, use ToggleMusicMute if needed
    if (muted && !((AudioManager*)mgr)->IsMusicMuted()) {
        ((AudioManager*)mgr)->ToggleMusicMute();
    } else if (!muted && ((AudioManager*)mgr)->IsMusicMuted()) {
        ((AudioManager*)mgr)->ToggleMusicMute();
    }
}

void SoundManager_playSound(SoundManagerRef mgr, const char* fileName) {
    if (!mgr || !fileName) return;
    // SoundManager doesn't have PlaySound method with filename, use PlaySoundEffect through AudioManager
    AudioManager::GetInstance().PlaySoundEffect(fileName);
}

void SoundManager_playSoundAtPosition(SoundManagerRef mgr, const char* fileName, void* position) {
    if (!mgr || !position || !fileName) return;
    // SoundManager doesn't have PlaySoundAtPosition method, fallback to regular sound
    AudioManager::GetInstance().PlaySoundEffect(fileName);
}

void SoundManager_stopAllSounds(SoundManagerRef mgr) {
    if (!mgr) return;
    // SoundManager doesn't have StopAllSounds method, this is a no-op for compatibility
}

void SoundManager_setVolume(SoundManagerRef mgr, float volume) {
    if (!mgr) return;
    ((SoundManager*)mgr)->SetVolume(volume);
}

void SoundManager_setMuted(SoundManagerRef mgr, bool muted) {
    if (!mgr) return;
    // SoundManager doesn't have SetMuted method, set volume to 0 for mute
    if (muted) {
        ((SoundManager*)mgr)->SetVolume(0.0f);
    } else {
        ((SoundManager*)mgr)->SetVolume(1.0f);
    }
}

} // extern "C"
