#pragma once
#include "PlatformTypes.h"
#include "PlatformAPI.h"
#include <vector>
#include <map>
#include "SoundEffect.h"

class AudioClip;  //forward declaration

//AudioManager a Singleton to hold and sync audio settings across menus
class AudioManager {
public:
    // Get the singleton instance
    static AudioManager& GetInstance() {
        static AudioManager instance;
        return instance;
    }


    void StopMusic();

    // Volume range: 0-10
    int GetMusicVolume() const { return musicVolume; }
    int GetSoundVolume() const { return soundVolume; }
    bool IsMusicMuted() const { return musicMuted; }
    bool IsSoundMuted() const { return soundMuted; }

    // Setters (clamped)
    void SetMusicVolume(int vol);
    void SetSoundVolume(int vol);
    void IncreaseMusicVolume();
    void DecreaseMusicVolume();
    void IncreaseSoundVolume();
    void DecreaseSoundVolume();
    void ToggleMusicMute();
    void ToggleSoundMute();

    // Load/unload UI textures
    void LoadTextures();
    void UnloadTextures();

    // Draw the audio options UI
    void DrawAudioOptions(float posX, float posY);

    void RegisterClip(AudioClip* clip);
    void UnregisterClip(AudioClip* clip);
    void Update();  // Call this once per frame (in Game or Playing)

    void LoadSoundEffect(const std::string& name, const std::string& path);
    void PlaySoundEffect(const std::string& name, float volume = 1.0f);
private:
    // Private constructor/destructor for Singleton
    AudioManager();
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    std::vector<AudioClip*> activeClips;

    // Internal helper to draw a 3-state button
    bool DrawButton3States(
        Texture2D normalTex,
        Texture2D hoverTex,
        Texture2D clickedTex,
        float x, float y,
        float width, float height
    );

    // Volume/mute data
    int musicVolume{7}; // 0-10
    int soundVolume; // 0-10
    bool musicMuted;
    bool soundMuted;

    // UI textures for volume meters
    Texture2D volumemeterEmpty;
    Texture2D volumemeterFull;

    // + Button
    Texture2D plusButtonNormal;
    Texture2D plusButtonHover;
    Texture2D plusButtonClicked;

    // - Button
    Texture2D minusButtonNormal;
    Texture2D minusButtonHover;
    Texture2D minusButtonClicked;

    // Mute Button (normal)
    Texture2D muteButtonNormal;
    Texture2D muteButtonHover;
    Texture2D muteButtonClicked;

    // Mute Button (locked)
    Texture2D muteButtonLockedNormal;
    Texture2D muteButtonLockedHover;
    Texture2D muteButtonLockedClicked;

    std::map<std::string, SoundEffect*> soundEffects;
};
