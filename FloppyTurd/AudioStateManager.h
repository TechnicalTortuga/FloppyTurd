#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include "AudioClip.h"
#include "GameState.h"

class AudioStateManager {
public:
    enum AudioState {
        AUDIO_MAIN_MENU,
        AUDIO_LOADING,
        AUDIO_PARK_LEVEL,      // Level 1 - Park
        AUDIO_SEWER_LEVEL,     // Level 2 - Sewer
        AUDIO_DESERT_LEVEL,    // Level 3 - Desert
        AUDIO_SNOW_LEVEL,      // Level 4 - Snow
        AUDIO_CASTLE_LEVEL,    // Level 5 - Castle
        AUDIO_BOSS_FIGHT,      // Level 6 - Rat King Boss
        AUDIO_BOSS_LOW_HEALTH,
        AUDIO_GAME_OVER,
        AUDIO_CREDITS
    };
    
    enum Difficulty {
        DIFFICULTY_EASY,       // Slow music (Runny)
        DIFFICULTY_NORMAL,     // Regular music (Regular)
        DIFFICULTY_HARD        // Fast music (Rough)
    };

    static AudioStateManager& GetInstance();

    // Initialize the audio state manager
    void Initialize();

    // Transition to a new audio state
    void TransitionToState(AudioState newState, bool fadeOut = true, float fadeTime = 1.0f);
    
    // Transition to a level with specific difficulty
    void TransitionToLevel(int levelNumber, Difficulty difficulty, bool fadeOut = true, float fadeTime = 1.0f);

    // Update audio state (called every frame)
    void Update(float deltaTime);

    // Stop all music
    void StopMusic();

    // Pause/resume music
    void PauseMusic();
    void ResumeMusic();

    // Volume control
    void SetMasterVolume(float volume);
    void SetMusicVolume(float volume);
    float GetMusicVolume() const { return musicVolume; }

    // Get current state
    AudioState GetCurrentState() const { return currentState; }
    
    // Get current difficulty
    Difficulty GetCurrentDifficulty() const { return currentDifficulty; }
    
    // Fart mode management
    void SetFartMode(bool enabled);
    bool IsFartModeEnabled() const { return fartModeEnabled; }

    // Check if music is playing
    bool IsMusicPlaying() const;

    // Force play a specific track (for testing)
    void PlayTrack(const std::string& trackName, bool loop = true);
    
    // Test methods for difficulty-based music
    void TestLevelMusic(int levelNumber, Difficulty difficulty);
    void TestAllLevelMusic();
    void TestFartMode();  // Test fart mode functionality

    // Map game states to audio states
    AudioState GetAudioStateForGameState(GAMESTATE gameState) const;

    void PlayGameOverMusic();
    void StopGameOverMusic();

private:
    AudioStateManager() = default;
    ~AudioStateManager() = default;
    AudioStateManager(const AudioStateManager&) = delete;
    AudioStateManager& operator=(const AudioStateManager&) = delete;

    // Audio state mapping
    struct AudioTrackInfo {
        std::string resourcePath;
        bool shouldLoop;
        float volume;
        float fadeInTime;
        float fadeOutTime;
    };

    // Map audio states to track information
    AudioTrackInfo GetTrackInfoForAudioState(AudioState audioState) const;
    
    // Get track info for level with difficulty
    AudioTrackInfo GetTrackInfoForLevel(int levelNumber, Difficulty difficulty, bool fartMode = false) const;

    // Internal state management
    void LoadAndPlayTrack(const AudioTrackInfo& trackInfo);
    void FadeOutCurrentTrack(float fadeTime);
    void FadeInNewTrack(float fadeTime);

    // Member variables
    AudioState currentState = AUDIO_MAIN_MENU;
    AudioState targetState = AUDIO_MAIN_MENU;
    Difficulty currentDifficulty = DIFFICULTY_NORMAL;
    int currentLevel = 1;
    
    // Audio clip objects (managed by AudioStateManager)
    std::unique_ptr<AudioClip> currentMusic;
    std::unique_ptr<AudioClip> nextMusic;
    
    float musicVolume = 1.0f;
    float masterVolume = 1.0f;
    
    bool isTransitioning = false;
    float transitionTimer = 0.0f;
    float transitionDuration = 0.0f;
    float fadeOutTimer = 0.0f;
    float fadeOutDuration = 0.0f;
    
    bool isPaused = false;
    bool fartModeEnabled = false;
    
    // Track mapping
    std::unordered_map<AudioState, AudioTrackInfo> trackMapping;
    
    // Initialize track mapping
    void InitializeTrackMapping();
    

}; 