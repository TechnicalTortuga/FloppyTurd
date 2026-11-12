#include "SoundManager.h"
#include "raylib.h"
#include "AudioManager.h"

void SoundManager::PlaySoundClip(Sound soundClip) {
    // Set volume for this sound before playing.
    const auto& audio=AudioManager::GetInstance();
    if (audio.IsSoundMuted() || !soundClip.stream.buffer) return;
    SetSoundVolume(soundClip, volume*audio.GetSoundVolume()/10.0f);
    PlaySound(soundClip);
}

Sound SoundManager::LoadSoundClip(const std::string& filePath) {
    return LoadSound(filePath.c_str());
}

void SoundManager::UnloadSoundClip(Sound soundClip) {
    UnloadSound(soundClip);
}
