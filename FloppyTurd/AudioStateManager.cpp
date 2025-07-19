#include "AudioStateManager.h"
#include "ResourceManager.h"
#include "GameLog.h"
#include "PlatformAPI.h"
#include <memory>
#include <string>
#include <unordered_map>

AudioStateManager& AudioStateManager::GetInstance() {
    static AudioStateManager instance;
    return instance;
}

void AudioStateManager::Initialize() {
    GameLog::Log("[AUDIO] AudioStateManager::Initialize() - Starting initialization");
    InitializeTrackMapping();
    currentState = AUDIO_MAIN_MENU;
    targetState = AUDIO_MAIN_MENU;
    auto trackInfo = GetTrackInfoForAudioState(AUDIO_MAIN_MENU);
    LoadAndPlayTrack(trackInfo);
    GameLog::Log("[AUDIO] AudioStateManager::Initialize() - Initialization complete");
}

void AudioStateManager::InitializeTrackMapping() {
    // Map audio states to track info
    trackMapping[AUDIO_MAIN_MENU] = {"music/FloppyTurdMenu.mp3", true, 0.8f, 1.0f, 1.0f};
    trackMapping[AUDIO_LOADING] = {"music/MenuFast.ogg", true, 0.7f, 0.5f, 0.5f};
    trackMapping[AUDIO_PARK_LEVEL] = {"music/ParkLevel.mp3", true, 0.8f, 1.0f, 1.0f};
    trackMapping[AUDIO_SEWER_LEVEL] = {"music/SewerLevel.mp3", true, 0.8f, 1.0f, 1.0f};
    trackMapping[AUDIO_DESERT_LEVEL] = {"music/DesertLevel.mp3", true, 0.8f, 1.0f, 1.0f};
    trackMapping[AUDIO_SNOW_LEVEL] = {"music/SnowLevel.mp3", true, 0.8f, 1.0f, 1.0f};
    trackMapping[AUDIO_CASTLE_LEVEL] = {"music/CastleLevel.mp3", true, 0.8f, 1.0f, 1.0f};
    trackMapping[AUDIO_BOSS_FIGHT] = {"music/BossLevel.ogg", true, 0.9f, 1.5f, 1.5f};
    trackMapping[AUDIO_BOSS_LOW_HEALTH] = {"music/BossThemeLowHealth.mp3", true, 0.9f, 1.0f, 1.0f};
    trackMapping[AUDIO_GAME_OVER] = {"music/gameover.mp3", false, 0.7f, 0.5f, 2.0f};
    trackMapping[AUDIO_CREDITS] = {"music/EndTheme.ogg", false, 0.8f, 1.0f, 3.0f};
}

void AudioStateManager::TransitionToLevel(int levelNumber, Difficulty difficulty, bool fadeOut, float fadeTime) {
    currentLevel = levelNumber;
    currentDifficulty = difficulty;
    AudioState levelState;
    switch (levelNumber) {
        case 1: levelState = AUDIO_PARK_LEVEL; break;
        case 2: levelState = AUDIO_SEWER_LEVEL; break;
        case 3: levelState = AUDIO_DESERT_LEVEL; break;
        case 4: levelState = AUDIO_SNOW_LEVEL; break;
        case 5: levelState = AUDIO_CASTLE_LEVEL; break;
        case 6: levelState = AUDIO_BOSS_FIGHT; break;
        default: levelState = AUDIO_PARK_LEVEL; break;
    }
    TransitionToState(levelState, fadeOut, fadeTime);
}

AudioStateManager::AudioTrackInfo AudioStateManager::GetTrackInfoForLevel(int levelNumber, Difficulty difficulty, bool fartMode) const {
    std::string trackPath;
    
    switch (levelNumber) {
        case 1: // Park Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "music/ParkSlow.ogg"; break;
                case DIFFICULTY_NORMAL: trackPath = "music/ParkLevel.mp3"; break;
                case DIFFICULTY_HARD: trackPath = "music/ParkFast.ogg"; break;
            }
            break;
        case 2: // Sewer Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "music/SewerSlow.ogg"; break;
                case DIFFICULTY_NORMAL: trackPath = "music/SewerLevel.mp3"; break;
                case DIFFICULTY_HARD: trackPath = "music/SewerFast.ogg"; break;
            }
            break;
        case 3: // Desert Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "music/DesertSlow.ogg"; break;
                case DIFFICULTY_NORMAL: trackPath = "music/DesertLevel.mp3"; break;
                case DIFFICULTY_HARD: trackPath = "music/DesertFast.ogg"; break;
            }
            break;
        case 4: // Snow Level
            switch (difficulty) {
                case DIFFICULTY_EASY: trackPath = "music/SnowSlow.ogg"; break;
                case DIFFICULTY_NORMAL: trackPath = "music/SnowLevel.mp3"; break;
                case DIFFICULTY_HARD: trackPath = "music/SnowFast.ogg"; break;
            }
            break;
        case 5: // Castle Level
            if (fartMode && difficulty == DIFFICULTY_HARD) {
                trackPath = "music/CastleFastFart.ogg";
            } else {
                switch (difficulty) {
                    case DIFFICULTY_EASY: trackPath = "music/CastleSlow.ogg"; break;
                    case DIFFICULTY_NORMAL: trackPath = "music/CastleLevel.mp3"; break;
                    case DIFFICULTY_HARD: trackPath = "music/CastleFast.ogg"; break;
                }
            }
            break;
        case 6: // Boss Level (Rat King)
            trackPath = "music/BossLevel.ogg";  // Boss music
            break;
        default:
            GameLog::Log("[AUDIO] WARNING: Unknown level %d, using Park Level normal", levelNumber);
            trackPath = "music/ParkLevel.mp3";
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
        case MAINMENU: return AUDIO_MAIN_MENU;
        case LOADING: return AUDIO_LOADING;
        case PLAYING: return AUDIO_PARK_LEVEL; // Default, will be overridden
        case CREDITS: return AUDIO_CREDITS;
        case PAUSEMENU: return currentState;
        case SHUTDOWN: return AUDIO_MAIN_MENU;
        default: return AUDIO_MAIN_MENU;
    }
}

AudioStateManager::AudioTrackInfo AudioStateManager::GetTrackInfoForAudioState(AudioState audioState) const {
    auto it = trackMapping.find(audioState);
    if (it != trackMapping.end()) return it->second;
    return {"music/FloppyTurdMenu.mp3", true, 0.8f, 1.0f, 1.0f};
}

void AudioStateManager::TransitionToState(AudioState newState, bool fadeOut, float fadeTime) {
    if (currentState == newState) return;
    targetState = newState;
    auto trackInfo = GetTrackInfoForAudioState(newState);
    LoadAndPlayTrack(trackInfo);
    currentState = newState;
}

void AudioStateManager::Update(float deltaTime) {
    // Ensure music is playing if it should be
    if (currentMusicPlayer && !isPaused && !IsMusicPlayingPlatform(currentMusicPlayer)) {
        GameLog::Log("[AUDIO] Music stopped unexpectedly, restarting");
        PlayMusicPlatform(currentMusicPlayer);
    }
}

void AudioStateManager::SetMasterVolume(float volume) {
    // Not implemented; Raylib does not have a global master volume
}

void AudioStateManager::SetMusicVolume(float volume) {
    if (currentMusicPlayer) {
        SetMusicVolumePlatform(currentMusicPlayer, volume);
    }
}

void AudioStateManager::SetFartMode(bool enabled) {
    fartModeEnabled = enabled;
    // If in castle or main menu, reload music
    if (currentState == AUDIO_CASTLE_LEVEL || currentState == AUDIO_MAIN_MENU) {
        auto trackInfo = GetTrackInfoForAudioState(currentState);
        LoadAndPlayTrack(trackInfo);
    }
}

void AudioStateManager::StopMusic() {
    if (currentMusicPlayer) {
        StopMusicPlatform(currentMusicPlayer);
    }
    if (nextMusicPlayer) {
        StopMusicPlatform(nextMusicPlayer);
    }
}

void AudioStateManager::PauseMusic() {
    if (currentMusicPlayer) {
        PauseMusicPlatform(currentMusicPlayer);
        isPaused = true;
    }
}

void AudioStateManager::ResumeMusic() {
    if (currentMusicPlayer && isPaused) {
        ResumeMusicPlatform(currentMusicPlayer);
        isPaused = false;
    }
}

void AudioStateManager::PlayTrack(const std::string& trackName, bool loop) {
    GameLog::Log("[AUDIO] PlayTrack: %s (loop: %s)", trackName.c_str(), loop ? "true" : "false");
    
    // Stop current music if playing
    if (currentMusicPlayer) {
        StopMusicPlatform(currentMusicPlayer);
        UnloadMusicPlatform(currentMusicPlayer);
        currentMusicPlayer = nullptr;
    }
    
    // Load and play the track
    currentMusicPlayer = LoadMusicPlatform(trackName);
    if (currentMusicPlayer) {
        SetMusicLoopingPlatform(currentMusicPlayer, loop);
        PlayMusicPlatform(currentMusicPlayer);
    }
}

void AudioStateManager::TestLevelMusic(int levelNumber, Difficulty difficulty) {
    auto trackInfo = GetTrackInfoForLevel(levelNumber, difficulty, fartModeEnabled);
    LoadAndPlayTrack(trackInfo);
}

void AudioStateManager::LoadAndPlayTrack(const AudioTrackInfo& trackInfo) {
    GameLog::Log("[AUDIO] LoadAndPlayTrack: %s (loop: %s, volume: %.2f)", 
                 trackInfo.resourcePath.c_str(), 
                 trackInfo.shouldLoop ? "true" : "false", 
                 trackInfo.volume);
    
    // Stop current music if playing
    if (currentMusicPlayer) {
        StopMusicPlatform(currentMusicPlayer);
        UnloadMusicPlatform(currentMusicPlayer);
        currentMusicPlayer = nullptr;
    }
    
    // Load new music using platform-specific function
    currentMusicPlayer = LoadMusicPlatform(trackInfo.resourcePath);
    if (currentMusicPlayer) {
        // Set looping state
        SetMusicLoopingPlatform(currentMusicPlayer, trackInfo.shouldLoop);
        
        // Set volume
        SetMusicVolumePlatform(currentMusicPlayer, trackInfo.volume);
        
        // Start playing
        PlayMusicPlatform(currentMusicPlayer);
        
        GameLog::Log("[AUDIO] Successfully loaded and started playing track");
    } else {
        GameLog::Log("[AUDIO] ERROR: Failed to load music track: %s", trackInfo.resourcePath.c_str());
    }
}

bool AudioStateManager::IsMusicPlaying() const {
    return currentMusicPlayer && IsMusicPlayingPlatform(currentMusicPlayer);
}

void AudioStateManager::TestAllLevelMusic() {
    // Test each level with all difficulties
    for (int level = 1; level <= 6; level++) {
        if (level == 6) {
            TestLevelMusic(level, DIFFICULTY_NORMAL);
        } else {
            TestLevelMusic(level, DIFFICULTY_EASY);
            TestLevelMusic(level, DIFFICULTY_NORMAL);
            TestLevelMusic(level, DIFFICULTY_HARD);
        }
    }
}

void AudioStateManager::TestFartMode() {
    GameLog::Log("[AUDIO] Testing fart mode functionality");
    SetFartMode(!fartModeEnabled);
    GameLog::Log("[AUDIO] Fart mode is now %s", fartModeEnabled ? "ENABLED" : "DISABLED");
}

void AudioStateManager::PlayGameOverMusic() {
    StopMusic();
    auto trackInfo = GetTrackInfoForAudioState(AUDIO_GAME_OVER);
    trackInfo.shouldLoop = false;
    LoadAndPlayTrack(trackInfo);
    currentState = AUDIO_GAME_OVER;
}

void AudioStateManager::StopGameOverMusic() {
    if (currentState == AUDIO_GAME_OVER && currentMusicPlayer) {
        StopMusicPlatform(currentMusicPlayer);
    }
}

// ============================================================================
// PLATFORM-SPECIFIC AUDIO OPERATIONS (Delegates to PlatformAPI)
// ============================================================================

void* AudioStateManager::LoadMusicPlatform(const std::string& fileName) {
    GameLog::Log("[AUDIO] Loading music through PlatformAPI: %s", fileName.c_str());
    Music* music = new Music();
    *music = LoadMusic(fileName.c_str());
    return music;
}

void AudioStateManager::UnloadMusicPlatform(void* musicPlayer) {
    if (musicPlayer) {
        GameLog::Log("[AUDIO] Unloading music through PlatformAPI");
        Music* music = static_cast<Music*>(musicPlayer);
        UnloadMusic(*music);
        delete music;
    }
}

void AudioStateManager::PlayMusicPlatform(void* musicPlayer) {
    if (musicPlayer) {
        GameLog::Log("[AUDIO] Playing music through PlatformAPI");
        Music* music = static_cast<Music*>(musicPlayer);
        PlayMusic(*music);
    }
}

void AudioStateManager::StopMusicPlatform(void* musicPlayer) {
    if (musicPlayer) {
        GameLog::Log("[AUDIO] Stopping music through PlatformAPI");
        Music* music = static_cast<Music*>(musicPlayer);
        StopMusicStream(*music);
    }
}

void AudioStateManager::PauseMusicPlatform(void* musicPlayer) {
    if (musicPlayer) {
        GameLog::Log("[AUDIO] Pausing music through PlatformAPI");
        Music* music = static_cast<Music*>(musicPlayer);
        PauseMusicStream(*music);
    }
}

void AudioStateManager::ResumeMusicPlatform(void* musicPlayer) {
    if (musicPlayer) {
        GameLog::Log("[AUDIO] Resuming music through PlatformAPI");
        Music* music = static_cast<Music*>(musicPlayer);
        ResumeMusicStream(*music);
    }
}

void AudioStateManager::SetMusicVolumePlatform(void* musicPlayer, float volume) {
    if (musicPlayer) {
        GameLog::Log("[AUDIO] Setting music volume through PlatformAPI: %.2f", volume);
        Music* music = static_cast<Music*>(musicPlayer);
        SetMusicVolumeForId(*music, volume);
    }
}

void AudioStateManager::SetMusicLoopingPlatform(void* musicPlayer, bool looping) {
    if (musicPlayer) {
        GameLog::Log("[AUDIO] Setting music looping through PlatformAPI: %s", looping ? "true" : "false");
        Music* music = static_cast<Music*>(musicPlayer);
        SetMusicLooping(*music, looping);
    }
}

bool AudioStateManager::IsMusicPlayingPlatform(void* musicPlayer) const {
    if (musicPlayer) {
        Music* music = static_cast<Music*>(musicPlayer);
        bool isPlaying = IsMusicStreamPlaying(*music);
        GameLog::Log("[AUDIO] Checking music playing status through PlatformAPI: %s", isPlaying ? "true" : "false");
        return isPlaying;
    }
    return false;
} 