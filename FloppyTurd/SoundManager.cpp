#include "SoundManager.h"
#include "PlatformAPI.h"

void SoundManager::PlaySoundClip(Sound soundClip) {
    // Set volume for this sound before playing.
    SetSoundVolume(soundClip.player, volume);
    PlaySound(soundClip.player);
}

Sound SoundManager::LoadSoundClip(const std::string& filePath) {
    Sound sound;
    sound = LoadSound(filePath.c_str());
    sound.length = 0; // Length will be set by platform implementation
    return sound;
}

void SoundManager::UnloadSoundClip(Sound soundClip) {
    UnloadSound(soundClip.player);
}
