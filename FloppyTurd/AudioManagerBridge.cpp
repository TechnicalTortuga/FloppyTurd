#include "AudioManagerBridge.h"
#include "AudioManager.h"
#include "SoundManager.h"
#include "SwiftTypes.h"

extern "C" {

AudioManagerRef AudioManager_getInstance() {
    return (AudioManagerRef)&AudioManager::GetInstance();
}

SoundManagerRef SoundManager_getInstance() {
    return (SoundManagerRef)&SoundManager::GetInstance();
}

void AudioManager_playMusic(AudioManagerRef mgr, const char* fileName) {
    if (!mgr) return;
    ((AudioManager*)mgr)->PlayMusic(fileName);
}

void AudioManager_stopMusic(AudioManagerRef mgr) {
    if (!mgr) return;
    ((AudioManager*)mgr)->StopMusic();
}

void AudioManager_pauseMusic(AudioManagerRef mgr) {
    if (!mgr) return;
    ((AudioManager*)mgr)->PauseMusic();
}

void AudioManager_resumeMusic(AudioManagerRef mgr) {
    if (!mgr) return;
    ((AudioManager*)mgr)->ResumeMusic();
}

void AudioManager_setMusicVolume(AudioManagerRef mgr, float volume) {
    if (!mgr) return;
    ((AudioManager*)mgr)->SetMusicVolume((int)(volume * 10.0f));
}

void AudioManager_setMusicMuted(AudioManagerRef mgr, bool muted) {
    if (!mgr) return;
    ((AudioManager*)mgr)->SetMusicMuted(muted);
}

void SoundManager_playSound(SoundManagerRef mgr, const char* fileName) {
    if (!mgr) return;
    ((SoundManager*)mgr)->PlaySound(fileName);
}

void SoundManager_playSoundAtPosition(SoundManagerRef mgr, const char* fileName, void* position) {
    if (!mgr || !position) return;
    ((SoundManager*)mgr)->PlaySoundAtPosition(fileName, *(Vector2*)position);
}

void SoundManager_stopAllSounds(SoundManagerRef mgr) {
    if (!mgr) return;
    ((SoundManager*)mgr)->StopAllSounds();
}

void SoundManager_setVolume(SoundManagerRef mgr, float volume) {
    if (!mgr) return;
    ((SoundManager*)mgr)->SetVolume((int)(volume * 10.0f));
}

void SoundManager_setMuted(SoundManagerRef mgr, bool muted) {
    if (!mgr) return;
    ((SoundManager*)mgr)->SetMuted(muted);
}

} // extern "C"
