#include "AudioStateManager.h"
#include "ResourceManager.h"
#include "GameLog.h"
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
    // Raylib does not support crossfade natively; stub out fade/crossfade
    // Just ensure music is playing
    if (currentMusic && !currentMusic->IsPlaying()) {
        currentMusic->Play();
    }
}

void AudioStateManager::SetMasterVolume(float volume) {
    // Not implemented; Raylib does not have a global master volume
}

void AudioStateManager::SetMusicVolume(float volume) {
    if (currentMusic) currentMusic->SetVolume(volume);
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
    if (currentMusic) currentMusic->Stop();
    if (nextMusic) nextMusic->Stop();
}

void AudioStateManager::PauseMusic() {
    if (currentMusic) currentMusic->Pause();
    isPaused = true;
}

void AudioStateManager::ResumeMusic() {
    if (currentMusic && isPaused) {
        currentMusic->Resume();
        isPaused = false;
    }
}

void AudioStateManager::PlayTrack(const std::string& trackName, bool loop) {
    if (currentMusic) currentMusic->Stop();
    currentMusic = std::make_unique<AudioClip>(trackName);
    if (currentMusic) {
        if (loop) currentMusic->PlayLoop();
        else currentMusic->Play();
    }
}

void AudioStateManager::TestLevelMusic(int levelNumber, Difficulty difficulty) {
    auto trackInfo = GetTrackInfoForLevel(levelNumber, difficulty, fartModeEnabled);
    LoadAndPlayTrack(trackInfo);
}

void AudioStateManager::LoadAndPlayTrack(const AudioTrackInfo& trackInfo) {
    if (currentMusic) currentMusic->Stop();
    currentMusic = std::make_unique<AudioClip>(trackInfo.resourcePath);
    if (currentMusic) {
        if (trackInfo.shouldLoop) currentMusic->PlayLoop();
        else currentMusic->Play();
        currentMusic->SetVolume(trackInfo.volume);
    }
}

bool AudioStateManager::IsMusicPlaying() const {
    return currentMusic && currentMusic->IsPlaying();
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
    if (currentState == AUDIO_GAME_OVER && currentMusic) {
        currentMusic->Stop();
    }
} 