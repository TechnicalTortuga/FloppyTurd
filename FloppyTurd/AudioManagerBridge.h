#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Opaque pointer types for Swift interop
typedef void* AudioManagerRef;
typedef void* SoundManagerRef;

// Singleton accessors
AudioManagerRef AudioManager_getInstance();
SoundManagerRef SoundManager_getInstance();

// Music controls
void AudioManager_playMusic(AudioManagerRef mgr, const char* fileName);
void AudioManager_stopMusic(AudioManagerRef mgr);
void AudioManager_pauseMusic(AudioManagerRef mgr);
void AudioManager_resumeMusic(AudioManagerRef mgr);
void AudioManager_setMusicVolume(AudioManagerRef mgr, float volume);
void AudioManager_setMusicMuted(AudioManagerRef mgr, bool muted);

// Sound controls
void SoundManager_playSound(SoundManagerRef mgr, const char* fileName);
void SoundManager_playSoundAtPosition(SoundManagerRef mgr, const char* fileName, void* position); // position is a pointer to Vector2
void SoundManager_stopAllSounds(SoundManagerRef mgr);
void SoundManager_setVolume(SoundManagerRef mgr, float volume);
void SoundManager_setMuted(SoundManagerRef mgr, bool muted);

#ifdef __cplusplus
}
#endif
