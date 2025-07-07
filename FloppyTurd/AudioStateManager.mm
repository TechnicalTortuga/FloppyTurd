#include "AudioStateManager.h"
#include "ResourceManager.h"
#include "GameLog.h"
#include <memory>
#include <string>
#include <unordered_map>

// iOS-specific includes
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

AudioStateManager& AudioStateManager::GetInstance() {
    static AudioStateManager instance;
    return instance;
}

void AudioStateManager::Initialize() {
    GameLog::Log("[AUDIO] AudioStateManager::Initialize() - Starting initialization");
    
    InitializeTrackMapping();
    
    // Start with main menu music
    currentState = AUDIO_MAIN_MENU;
    targetState = AUDIO_MAIN_MENU;
    
    // Load and play main menu music
    auto trackInfo = GetTrackInfoForAudioState(AUDIO_MAIN_MENU);
    LoadAndPlayTrack(trackInfo);
    
    GameLog::Log("[AUDIO] AudioStateManager::Initialize() - Initialization complete");
}

void AudioStateManager::InitializeTrackMapping() {
    GameLog::Log("[AUDIO] Initializing track mapping...");
    
    // Main menu and loading
    trackMapping[AUDIO_MAIN_MENU] = {
        "main_menu_music",  // Resource ID from ResourceManager
        true,   // loop
        0.8f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
    
    trackMapping[AUDIO_LOADING] = {
        "main_menu_music",  // Use main menu music for loading
        true,   // loop
        0.7f,   // volume
        0.5f,   // fade in time
        0.5f    // fade out time
    };
    
    // Level music - these will be overridden by difficulty-based selection
    trackMapping[AUDIO_PARK_LEVEL] = {
        "level1_music",   // Resource ID from ResourceManager
        true,   // loop
        0.8f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
    
    trackMapping[AUDIO_SEWER_LEVEL] = {
        "level2_music",  // Resource ID from ResourceManager
        true,   // loop
        0.8f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
    
    trackMapping[AUDIO_DESERT_LEVEL] = {
        "level3_music", // Resource ID from ResourceManager
        true,   // loop
        0.8f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
    
    trackMapping[AUDIO_SNOW_LEVEL] = {
        "level4_music",   // Resource ID from ResourceManager
        true,   // loop
        0.8f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
    
    trackMapping[AUDIO_CASTLE_LEVEL] = {
        "level5_music", // Resource ID from ResourceManager
        true,   // loop
        0.8f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
    
    // Boss music
    trackMapping[AUDIO_BOSS_FIGHT] = {
        "boss_music",
        true,   // loop
        0.9f,   // volume (slightly louder for boss fights)
        1.5f,   // fade in time
        1.5f    // fade out time
    };
    
    trackMapping[AUDIO_BOSS_LOW_HEALTH] = {
        "boss_low_health",
        true,   // loop
        0.9f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
    
    // Game over and credits
    trackMapping[AUDIO_GAME_OVER] = {
        "game_over_music",
        false,  // don't loop
        0.7f,   // volume
        0.5f,   // fade in time
        2.0f    // fade out time
    };
    
    trackMapping[AUDIO_CREDITS] = {
        "credits_music",
        false,  // don't loop
        0.8f,   // volume
        1.0f,   // fade in time
        3.0f    // fade out time
    };
    
    GameLog::Log("[AUDIO] Track mapping initialized with %zu tracks", trackMapping.size());
}

void AudioStateManager::TransitionToLevel(int levelNumber, Difficulty difficulty, bool fadeOut, float fadeTime) {
    GameLog::Log("[AUDIO] Transitioning to Level %d with difficulty %s", 
                 levelNumber, 
                 difficulty == DIFFICULTY_EASY ? "EASY" : 
                 difficulty == DIFFICULTY_NORMAL ? "NORMAL" : "HARD");
    
    currentLevel = levelNumber;
    currentDifficulty = difficulty;
    
    // Map level number to audio state
    AudioState levelState;
    switch (levelNumber) {
        case 1: levelState = AUDIO_PARK_LEVEL; break;      // Park
        case 2: levelState = AUDIO_SEWER_LEVEL; break;     // Sewer
        case 3: levelState = AUDIO_DESERT_LEVEL; break;    // Desert
        case 4: levelState = AUDIO_SNOW_LEVEL; break;      // Snow
        case 5: levelState = AUDIO_CASTLE_LEVEL; break;    // Castle
        case 6: levelState = AUDIO_BOSS_FIGHT; break;      // Rat King Boss
        default: 
            GameLog::Log("[AUDIO] WARNING: Unknown level %d, defaulting to Park Level", levelNumber);
            levelState = AUDIO_PARK_LEVEL;
            break;
    }
    
    // Transition to the level state with difficulty-based track selection
    TransitionToState(levelState, fadeOut, fadeTime);
}

AudioStateManager::AudioTrackInfo AudioStateManager::GetTrackInfoForLevel(int levelNumber, Difficulty difficulty, bool fartMode) const {
    std::string trackPath;
    
    switch (levelNumber) {
        case 1: // Park Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "level1_slow"; break;
                case DIFFICULTY_NORMAL: trackPath = "level1_music"; break;
                case DIFFICULTY_HARD: trackPath = "level1_fast"; break;
            }
            break;
        case 2: // Sewer Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "level2_slow"; break;
                case DIFFICULTY_NORMAL: trackPath = "level2_music"; break;
                case DIFFICULTY_HARD: trackPath = "level2_fast"; break;
            }
            break;
        case 3: // Desert Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "level3_slow"; break;
                case DIFFICULTY_NORMAL: trackPath = "level3_music"; break;
                case DIFFICULTY_HARD: trackPath = "level3_fast"; break;
            }
            break;
        case 4: // Snow Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "level4_slow"; break;
                case DIFFICULTY_NORMAL: trackPath = "level4_music"; break;
                case DIFFICULTY_HARD: trackPath = "level4_fast"; break;
            }
            break;
        case 5: // Castle Level
            if (fartMode && difficulty == DIFFICULTY_HARD) {
                trackPath = "level5_music"; // Use normal castle music for fart mode
            } else {
                switch (difficulty) {
                    case DIFFICULTY_EASY: trackPath = "level5_music"; break; // Castle only has one version
                    case DIFFICULTY_NORMAL: trackPath = "level5_music"; break;
                    case DIFFICULTY_HARD: trackPath = "level5_music"; break;
                }
            }
            break;
        case 6: // Boss Level (Rat King)
            trackPath = "boss_music";  // Boss music
            break;
        default:
            GameLog::Log("[AUDIO] WARNING: Unknown level %d, using Park Level normal", levelNumber);
            trackPath = "level1_music";
            break;
    }
    
    return {
        trackPath,
        true,   // loop
        0.8f,   // volume
        1.0f,   // fade in time
        1.0f    // fade out time
    };
}

AudioStateManager::AudioState AudioStateManager::GetAudioStateForGameState(GAMESTATE gameState) const {
    switch (gameState) {
        case MAINMENU:
            return AUDIO_MAIN_MENU;
        case LOADING:
            return AUDIO_LOADING;
        case PLAYING:
            // This will be handled by the Playing state based on current level
            return AUDIO_PARK_LEVEL; // Default to park level
        case CREDITS:
            return AUDIO_CREDITS;
        case PAUSEMENU:
            // Keep current music but pause it
            return currentState;
        case SHUTDOWN:
            return AUDIO_MAIN_MENU; // Default
        default:
            return AUDIO_MAIN_MENU;
    }
}

AudioStateManager::AudioTrackInfo AudioStateManager::GetTrackInfoForAudioState(AudioState audioState) const {
    auto it = trackMapping.find(audioState);
    if (it != trackMapping.end()) {
        return it->second;
    }
    
    // Return default track info if not found
    GameLog::Log("[AUDIO] WARNING: No track mapping found for audio state %d, using default", (int)audioState);
    return {
        "mainmenu/FloppyTurdMenu.mp3",
        true,
        0.8f,
        1.0f,
        1.0f
    };
}

void AudioStateManager::TransitionToState(AudioState newState, bool fadeOut, float fadeTime) {
    if (currentState == newState && !isTransitioning) {
        return; // Already in the target state
    }
    
    GameLog::Log("[AUDIO] Transitioning from state %d to %d (fadeOut: %s, fadeTime: %.2f)", 
                 (int)currentState, (int)newState, fadeOut ? "true" : "false", fadeTime);
    
    targetState = newState;
    
    if (fadeOut && currentMusic) {
        // Start fade out transition
        isTransitioning = true;
        transitionDuration = fadeTime;
        transitionTimer = 0.0f;
        fadeOutDuration = fadeTime;
        fadeOutTimer = 0.0f;
    } else {
        // Immediate transition
        LoadAndPlayTrack(GetTrackInfoForAudioState(newState));
        currentState = newState;
    }
}

void AudioStateManager::Update(float deltaTime) {
    if (isTransitioning) {
        transitionTimer += deltaTime;
        
        if (currentMusic && fadeOutTimer < fadeOutDuration) {
            fadeOutTimer += deltaTime;
            float fadeProgress = fadeOutTimer / fadeOutDuration;
            float currentVolume = musicVolume * (1.0f - fadeProgress);
            
            // Update current music volume using iOS AVAudioPlayer
            if (currentMusic && currentMusic->IsPlaying()) {
                currentMusic->SetVolume(currentVolume);
            }
        }
        
        if (transitionTimer >= transitionDuration) {
            // Transition complete
            GameLog::Log("[AUDIO] Transition complete, loading new track");
            
            // Stop current music
            if (currentMusic) {
                currentMusic->Stop();
                currentMusic.reset();
            }
            
            // Load and play new track
            LoadAndPlayTrack(GetTrackInfoForAudioState(targetState));
            currentState = targetState;
            
            // Reset transition state
            isTransitioning = false;
            transitionTimer = 0.0f;
            transitionDuration = 0.0f;
            fadeOutTimer = 0.0f;
            fadeOutDuration = 0.0f;
        }
    }
}

void AudioStateManager::LoadAndPlayTrack(const AudioTrackInfo& trackInfo) {
    GameLog::Log("[AUDIO] Loading track: %s", trackInfo.resourcePath.c_str());
    
    try {
        // For level states, use difficulty-based track selection
        std::string finalTrackPath = trackInfo.resourcePath;
        
        // Handle main menu fart mode
        if (currentState == AUDIO_MAIN_MENU && fartModeEnabled) {
            finalTrackPath = "FloppyTurdMenuAlt";  // Asset catalog name for iOS
            GameLog::Log("[AUDIO] Using main menu fart variant");
        }
        else if (currentState >= AUDIO_PARK_LEVEL && currentState <= AUDIO_CASTLE_LEVEL) {
            // Use difficulty-based track selection for levels
            auto levelTrackInfo = GetTrackInfoForLevel(currentLevel, currentDifficulty, fartModeEnabled);
            finalTrackPath = levelTrackInfo.resourcePath;
            GameLog::Log("[AUDIO] Using difficulty-based track: %s (Level %d, Difficulty: %s, Fart Mode: %s)", 
                        finalTrackPath.c_str(), currentLevel, 
                        currentDifficulty == DIFFICULTY_EASY ? "EASY" : 
                        currentDifficulty == DIFFICULTY_NORMAL ? "NORMAL" : "HARD",
                        fartModeEnabled ? "ON" : "OFF");
        }
        
        // Get the resolved path from ResourceManager
        std::string resolvedPath = ResourceManager::GetInstance().GetResourcePath(
            finalTrackPath, ResourceType::MUSIC);
        
        if (resolvedPath.empty()) {
            GameLog::Log("[AUDIO] ERROR: Failed to resolve path for %s", finalTrackPath.c_str());
            return;
        }
        
        // Create new audio clip
        nextMusic = std::make_unique<AudioClip>(resolvedPath);
        
        if (nextMusic && nextMusic->IsPlaying()) {
            // Set volume
            nextMusic->SetVolume(trackInfo.volume * musicVolume);
            
            // Set looping
            if (trackInfo.shouldLoop) {
                nextMusic->SetLooping(true);
            }
            
            // Play the music
            nextMusic->Play();
            
            // Swap current and next
            if (currentMusic) {
                currentMusic->Stop();
            }
            currentMusic = std::move(nextMusic);
            
            GameLog::Log("[AUDIO] Successfully loaded and started playing: %s", finalTrackPath.c_str());
        } else {
            GameLog::Log("[AUDIO] ERROR: Failed to create valid AudioClip for %s", finalTrackPath.c_str());
        }
        
    } catch (const std::exception& e) {
        GameLog::Log("[AUDIO] ERROR: Exception loading track %s: %s", trackInfo.resourcePath.c_str(), e.what());
    } catch (...) {
        GameLog::Log("[AUDIO] ERROR: Unknown exception loading track %s", trackInfo.resourcePath.c_str());
    }
}

void AudioStateManager::StopMusic() {
    GameLog::Log("[AUDIO] Stopping all music");
    
    if (currentMusic) {
        currentMusic->Stop();
        currentMusic.reset();
    }
    
    if (nextMusic) {
        nextMusic->Stop();
        nextMusic.reset();
    }
    
    isTransitioning = false;
    transitionTimer = 0.0f;
    transitionDuration = 0.0f;
}

void AudioStateManager::PauseMusic() {
    GameLog::Log("[AUDIO] Pausing music");
    
    if (currentMusic && !isPaused) {
        currentMusic->Pause();
        isPaused = true;
    }
}

void AudioStateManager::ResumeMusic() {
    GameLog::Log("[AUDIO] Resuming music");
    
    if (currentMusic && isPaused) {
        currentMusic->Resume();
        isPaused = false;
    }
}

void AudioStateManager::SetMasterVolume(float volume) {
    masterVolume = std::max(0.0f, std::min(1.0f, volume));
    GameLog::Log("[AUDIO] Master volume set to %.2f", masterVolume);
    
    // Update current music volume if playing
    if (currentMusic) {
        currentMusic->SetVolume(musicVolume * masterVolume);
    }
}

void AudioStateManager::SetMusicVolume(float volume) {
    musicVolume = std::max(0.0f, std::min(1.0f, volume));
    GameLog::Log("[AUDIO] Music volume set to %.2f", musicVolume);
    
    // Update current music volume if playing
    if (currentMusic) {
        currentMusic->SetVolume(musicVolume * masterVolume);
    }
}

bool AudioStateManager::IsMusicPlaying() const {
    return currentMusic && currentMusic->IsPlaying();
}

void AudioStateManager::SetFartMode(bool enabled) {
    if (fartModeEnabled != enabled) {
        fartModeEnabled = enabled;
        GameLog::Log("[AUDIO] Fart mode %s", fartModeEnabled ? "ENABLED" : "DISABLED");
        
        // If we're currently playing main menu music, reload it with the new mode
        if (currentState == AUDIO_MAIN_MENU && currentMusic) {
            auto trackInfo = GetTrackInfoForAudioState(AUDIO_MAIN_MENU);
            LoadAndPlayTrack(trackInfo);
        }
    }
}

void AudioStateManager::PlayTrack(const std::string& trackName, bool loop) {
    GameLog::Log("[AUDIO] Force playing track: %s (loop: %s)", trackName.c_str(), loop ? "true" : "false");
    
    try {
        std::string resolvedPath = ResourceManager::GetInstance().GetResourcePath(trackName, ResourceType::MUSIC);
        
        if (resolvedPath.empty()) {
            GameLog::Log("[AUDIO] ERROR: Failed to resolve path for %s", trackName.c_str());
            return;
        }
        
        // Stop current music
        if (currentMusic) {
            currentMusic->Stop();
        }
        
        // Create and play new track
        currentMusic = std::make_unique<AudioClip>(resolvedPath);
        if (currentMusic && currentMusic->IsPlaying()) {
            if (loop) {
                currentMusic->SetLooping(true);
            }
            currentMusic->Play();
            GameLog::Log("[AUDIO] Successfully started playing: %s", trackName.c_str());
        }
        
    } catch (const std::exception& e) {
        GameLog::Log("[AUDIO] ERROR: Exception playing track %s: %s", trackName.c_str(), e.what());
    }
}

void AudioStateManager::TestLevelMusic(int levelNumber, Difficulty difficulty) {
    GameLog::Log("[AUDIO] TEST: Testing Level %d %s music", 
                 levelNumber, difficulty == DIFFICULTY_EASY ? "EASY" : "HARD");
    
    auto trackInfo = GetTrackInfoForLevel(levelNumber, difficulty, fartModeEnabled);
    GameLog::Log("[AUDIO] TEST: Track path: %s", trackInfo.resourcePath.c_str());
    
    // Load and play the track
    LoadAndPlayTrack(trackInfo);
}

void AudioStateManager::TestAllLevelMusic() {
    GameLog::Log("[AUDIO] TEST: Testing all level music variations");
    
    // Test each level with all difficulties
    for (int level = 1; level <= 6; level++) {
        if (level == 6) {
            // Boss level only has one version
            TestLevelMusic(level, DIFFICULTY_NORMAL);
        } else {
            // Test all three difficulties for all other levels
            TestLevelMusic(level, DIFFICULTY_EASY);
            TestLevelMusic(level, DIFFICULTY_NORMAL);
            TestLevelMusic(level, DIFFICULTY_HARD);
        }
    }
}

void AudioStateManager::TestFartMode() {
    GameLog::Log("[AUDIO] TEST: Testing fart mode functionality");
    
    // Test main menu with fart mode
    GameLog::Log("[AUDIO] TEST: Main menu with fart mode OFF");
    SetFartMode(false);
    TransitionToState(AUDIO_MAIN_MENU, false, 0.5f);
    
    // Wait a bit, then test with fart mode ON
    GameLog::Log("[AUDIO] TEST: Main menu with fart mode ON");
    SetFartMode(true);
    TransitionToState(AUDIO_MAIN_MENU, false, 0.5f);
    
    // Test Castle Level with different difficulties and fart modes
    GameLog::Log("[AUDIO] TEST: Castle Level EASY (no fart mode)");
    SetFartMode(false);
    TransitionToLevel(5, DIFFICULTY_EASY, false, 0.5f);
    
    GameLog::Log("[AUDIO] TEST: Castle Level HARD without fart mode");
    SetFartMode(false);
    TransitionToLevel(5, DIFFICULTY_HARD, false, 0.5f);
    
    GameLog::Log("[AUDIO] TEST: Castle Level HARD with fart mode");
    SetFartMode(true);
    TransitionToLevel(5, DIFFICULTY_HARD, false, 0.5f);
}

void AudioStateManager::PlayGameOverMusic() {
    GameLog::Log("[AUDIO] Playing game over music");
    StopMusic();
    auto trackInfo = GetTrackInfoForAudioState(AUDIO_GAME_OVER);
    trackInfo.shouldLoop = false;
    LoadAndPlayTrack(trackInfo);
    currentState = AUDIO_GAME_OVER;
}

void AudioStateManager::StopGameOverMusic() {
    GameLog::Log("[AUDIO] Stopping game over music");
    if (currentState == AUDIO_GAME_OVER && currentMusic) {
        currentMusic->Stop();
    }
} 