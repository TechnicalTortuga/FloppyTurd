#include "SoundManager.h"
#include "raylib.h"

void SoundManager::PlaySoundClip(Sound soundClip) {
    // Set volume for this sound before playing.
    SetSoundVolume(soundClip, volume);
    PlaySound(soundClip);
}

Sound SoundManager::LoadSoundClip(const std::string& filePath) {
    return LoadSound(filePath.c_str());
}

void SoundManager::UnloadSoundClip(Sound soundClip) {
    UnloadSound(soundClip);
}
