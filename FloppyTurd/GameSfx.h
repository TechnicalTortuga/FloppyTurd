#pragma once
#include "AudioManager.h"
#include <string>

// These short clips come from the preserved iOS audio catalog. The shared
// AudioManager owns their cache and applies the game's SFX mute/volume.
namespace GameSfx {
inline void Play(const std::string& name) {
    auto& audio=AudioManager::GetInstance();
    if (audio.IsSoundMuted() || audio.GetSoundVolume()<=0) return;
    const std::string id="ios_"+name;
    audio.LoadSoundEffect(id,"resources/sounds/ios/"+name+".mp3");
    audio.PlaySoundEffect(id);
}
inline void Spit() { Play("spit"+std::to_string(GetRandomValue(1,6))); }
}
