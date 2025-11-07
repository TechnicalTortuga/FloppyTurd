#ifndef FLOPPY_TURD_GAME_H
#define FLOPPY_TURD_GAME_H

#ifdef PLATFORM_IOS
#include <TargetConditionals.h>
#endif

// Include necessary engine headers first
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"

// Forward declarations to avoid circular dependencies
namespace GameCore {
    class GameStateManager;
    class EventManager;
}

#include "../States/GameState.h"
#include "../States/GameplayState.h"
#include "../Systems/AdSystem.h"
#include <memory>

namespace GameCore {
    class GameStateManager;
}

// Swift 5.9+ Native C++ Interop Support
#if __has_include(<swift/bridging>)
#include <swift/bridging>
#endif

namespace GameCore {

    /**
     * @brief Main game class for Floppy Turd
     * 
     * This class orchestrates the entire game, managing the ECS system,
     * platform interfaces, game states, and core game loop.
     */
    class FloppyTurdGame {
    public:
        // Game version (single source of truth)
        static constexpr const char* GetVersion() { return "1.0.0"; }
        
        FloppyTurdGame();
        ~FloppyTurdGame();
        
        // Swift 5.9+ C++ Interop: Allow copy/move for Swift compatibility
        FloppyTurdGame(const FloppyTurdGame& other) = delete;
        FloppyTurdGame& operator=(const FloppyTurdGame& other) = delete;
        FloppyTurdGame(FloppyTurdGame&& other) = default;
        FloppyTurdGame& operator=(FloppyTurdGame&& other) = default;

        // Game lifecycle - Single point of initialization
        bool Initialize();
        void Shutdown();
        void Run();
        
        // Platform-specific component setters (iOS only)
        // Note: Always declared for Swift C++ interop compatibility
        // SetSwiftComponents removed - Swift components managed entirely on Swift side

        // Game loop
        void Update(float deltaTime);
        void Render();
        void HandleInput();

        // Game state management
        void StartGame();
        void PauseGame();
        void ResumeGame();
        void EndGame();
        void RestartGame();
        void ShowMainMenu();
        void ShowShop();
        void ShowGameOver(int score, int coins);

        // Game data
        void SaveGameData();
        void LoadGameData();
        int GetHighScore() const { return m_highScore; }
        int GetPlayerCoins() const { return m_gameStats.storedCoins; } // Return stored coins instead of legacy value
        void AddCoins(int amount) {
            m_gameStats.storedCoins += amount;
            m_gameStats.totalCoinsCollected += amount; // Also update gross total
        }
        void SpendCoins(int amount) {
            m_gameStats.storedCoins -= amount;
            // Note: gross total doesn't decrease when spending
        }

        // System access
        Gnosis::ECS* GetECS() const { return m_ecsSystem.get(); }
        const PlatformDelegates& GetPlatformDelegates() const { return m_platformDelegates; }

        // Screen info update for Swift interop
        void UpdateScreenInfo(const ScreenInfo& screenInfo);

        // Landscape mode support
        int GetPendingLandscapeLevelId() const { return m_pendingLandscapeLevelId; }

        // Game settings
        void SetMusicVolume(float volume);
        void SetSFXVolume(float volume);
        void SetMasterVolume(float volume);
        float GetMasterVolume() const { return m_masterVolume; }
        float GetMusicVolume() const { return m_musicVolume; }
        float GetSFXVolume() const { return m_sfxVolume; }
        bool IsMusicPlaying() const { return !m_currentMusicTrack.empty(); }
        const std::string& GetCurrentMusicTrack() const { return m_currentMusicTrack; }
        void SetCurrentMusicTrack(const std::string& track) { m_currentMusicTrack = track; }
        
        // Vibration settings
        void SetVibrationsEnabled(bool enabled);
        bool GetVibrationsEnabled() const { return m_vibrationsEnabled; }
        
        // Debug mode
        bool GetDebugMode() const { return m_showDebugInfo; }
        void SetDebugMode(bool enabled) { m_showDebugInfo = enabled; }
        
        // Settings persistence
        void LoadSettings();
        void SaveSettings();

        // Game statistics
        struct GameStats {
            int totalGamesPlayed;
            int totalScore;
            int totalCoinsCollected;
            int storedCoins;            // Coins stored between sessions (spendable)
            int totalDeaths;            // Total number of deaths/flops
            int totalPipesCleared;      // Total pipes cleared across all sessions
            int totalJumps;
            int totalEnemiesKilled;     // Total enemies defeated (all types)
            int sessionEnemiesKilled;   // Enemies killed in current session (reset on level start)
            float totalPlayTime;
            int currentStreak;
            int bestStreak;
        };

        // Customization data
        struct CustomizationData {
            int equippedHatIndex;       // Currently equipped hat (-1 = none)
            int selectedHatIndex;       // Currently selected hat in menu (-1 = none)
            std::vector<bool> unlockedHats; // Which hats are unlocked
            std::vector<bool> unlockedSkills; // Which skills are unlocked (5 skills total)
            
            CustomizationData() : equippedHatIndex(-1), selectedHatIndex(-1) {
                unlockedHats.resize(15, false);
                // First 4 hats are unlocked by default (Cowboy, Flower, Doorag, Ballcap)
                unlockedHats[0] = true;
                unlockedHats[1] = true;
                unlockedHats[2] = true;
                unlockedHats[3] = true;
                unlockedSkills.resize(5, false); // 5 skills: HalfHearts, ThirdHearts, CoinMagnet, HeartMagnet, CoinSafetyNet
            }
        };

        // Level-based high scores and unlock requirements
        struct LevelStats {
            int highScore;              // Best pipes cleared for this level
            int bestCoins;              // Best coins collected for this level
            float bestBossTime;         // Best boss completion time (Level 6 only, in seconds)
            bool unlocked;              // Whether this level is unlocked
            int unlockRequirement;      // Pipes required to unlock next level (runtime only)
            int coinRequirement;        // Coins required to unlock next level (runtime only)
        };

        // Save data structure - only essential persistent data
        struct LevelSaveData {
            int highScore;              // Best pipes cleared for this level
            int bestCoins;              // Best coins collected for this level
            float bestBossTime;         // Best boss completion time (Level 6 only, in seconds)
            bool unlocked;              // Whether this level is unlocked
        };
        
        const GameStats& GetGameStats() const { return m_gameStats; }
        void UpdateGameStats(const GameStats& stats);

        // Level-based high scores and unlocking
        const LevelStats& GetLevelStats(int levelId) const;
        void UpdateLevelStats(int levelId, const LevelStats& stats);
        bool IsLevelUnlocked(int levelId) const;
        void UnlockLevel(int levelId);
        int GetLevelHighScore(int levelId) const;
        void UpdateLevelHighScore(int levelId, int score, int coins, float bossTime = 0.0f);
        bool CanUnlockLevel(int levelId, std::string& failureMessage);
        bool TryUnlockLevel(int levelId);
        void PlaySFX(const std::string& soundName);

        // Customization data access
        const CustomizationData& GetCustomizationData() const { return m_customizationData; }
        void UpdateCustomizationData(const CustomizationData& data);
        int GetEquippedHatIndex() const { return m_customizationData.equippedHatIndex; }
        void SetEquippedHatIndex(int index);
        int GetSelectedHatIndex() const { return m_customizationData.selectedHatIndex; }
        void SetSelectedHatIndex(int index);
        bool IsHatUnlocked(int index) const;
        void UnlockHat(int index);
        
        // Skill unlock tracking
        bool IsSkillUnlocked(int skillIndex) const;
        void UnlockSkill(int skillIndex);
        
        // Enemy kill tracking
        void IncrementSessionEnemyKills();
        void ResetSessionEnemyKills();
        int GetSessionEnemiesKilled() const { return m_gameStats.sessionEnemiesKilled; }

        // ConfigManager access for Swift interop
        void UpdateConfigManagerScreenInfo(const ScreenInfo& screenInfo);

        // Platform detection
        bool IsIOSPlatform() const { return m_isIOSPlatform; }

        // Ad system access
        void TriggerGameOverAd();
        FloppyTurd::AdSystem* GetAdSystem() { return m_adSystem.get(); }

        // Debug unlock flags
        bool GetDebugLevelsUnlocked() const { return m_debugLevelsUnlocked; }
        void SetDebugLevelsUnlocked(bool unlocked) { m_debugLevelsUnlocked = unlocked; }
        bool GetDebugHatsUnlocked() const { return m_debugHatsUnlocked; }
        void SetDebugHatsUnlocked(bool unlocked) { m_debugHatsUnlocked = unlocked; }

    private:
        // Helper methods for level system
        void SetDefaultUnlockRequirements(int levelId, LevelStats& stats);
        void CheckLevelUnlock(int completedLevelId, int score, int coins);
        // Core systems
        std::unique_ptr<Gnosis::ECS> m_ecsSystem;
        std::unique_ptr<GameStateManager> m_stateManager;
        std::unique_ptr<FloppyTurd::AdSystem> m_adSystem;
        
        // Platform abstraction
        PlatformDelegates m_platformDelegates;
        
        // Platform-specific components removed - using delegates only

        // Game state
        bool m_initialized;
        bool m_running;
        bool m_paused;
        
        // Game data
        int m_highScore;
        int m_playerCoins;
        float m_musicVolume;
        float m_sfxVolume;
        float m_masterVolume = 1.0f;
        bool m_vibrationsEnabled = true; // Default enabled
        std::string m_currentMusicTrack = ""; // Track which music is currently playing
        GameStats m_gameStats;

        // Level-based data (fixed array for levels 1-6)
        static const int MAX_LEVELS = 6;
        LevelStats m_levelStats[MAX_LEVELS + 1]; // Index 1-6 for levels

        // Customization data
        CustomizationData m_customizationData;

        // Level unlock sound effect timer
        float m_levelUnlockSoundTimer;
        bool m_pendingPartyHorn;

        // Platform detection (set at initialization)
        bool m_isIOSPlatform;
        
        // Loading flag to prevent saves during deserialization
        bool m_isLoadingGameData;

        // Landscape mode support
        int m_pendingLandscapeLevelId;
        std::string m_pendingTransitionTarget;
        bool m_enteredViaQuickplay;  // Track if user entered via Quickplay
        int m_lastPlayedLevelId;     // Track the last level played (for return navigation)
        
        // Performance tracking
        float m_frameTime;
        float m_targetFrameTime;
        int m_frameCount;
        float m_fpsTimer;
        float m_currentFPS;
        
        // Game constants
        static const float TARGET_FPS;
        static const char* SAVE_FILE_NAME;
        static const char* SETTINGS_FILE_NAME;
        
        // Initialization helpers
        bool InitializeECS();
        bool InitializePlatform();
        bool InitializeAudio();
        bool InitializeGraphics();
        void InitializeGameStates();
        
        // Game loop helpers
        void UpdatePerformanceStats(float deltaTime);
        void LimitFrameRate();
        
        // State management helpers
        void HandleStateTransition(GameState* finishedState);
        
        // Data management
        void ResetGameData();
        
        // Audio management
        void PlayBackgroundMusic();
        void StopBackgroundMusic();
        
        // Resource management
        void LoadGameResources();
        void UnloadGameResources();
        
        // Debug and development
        void UpdateDebugInfo(float deltaTime);
        void RenderDebugInfo();
        bool m_showDebugInfo;
        bool m_debugLevelsUnlocked;
        bool m_debugHatsUnlocked;
    };

    /**
     * @brief Game Configuration
     * 
     * Holds configuration settings for the game
     */
    struct GameConfig {
        // Display settings
        int windowWidth;
        int windowHeight;
        bool fullscreen;
        bool vsync;
        
        // Audio settings
        float masterVolume;
        float musicVolume;
        float sfxVolume;
        
        // Gameplay settings
        float gameSpeed;
        int startingLives;
        bool enableParticles;
        bool enableScreenShake;
        
        // Debug settings
        bool showFPS;
        bool showColliders;
        bool enableLogging;
        
        GameConfig()
            : windowWidth(800)
            , windowHeight(600)
            , fullscreen(false)
            , vsync(true)
            , masterVolume(1.0f)
            , musicVolume(0.7f)
            , sfxVolume(0.8f)
            , gameSpeed(1.0f)
            , startingLives(3)
            , enableParticles(true)
            , enableScreenShake(true)
            , showFPS(false)
            , showColliders(false)
            , enableLogging(true)
        {}
    };

    // Global game instance access
    extern FloppyTurdGame* g_Game;
    
    // Utility functions
    FloppyTurdGame* GetGame();
    void SetGame(FloppyTurdGame* game);
    
    // Swift 5.9+ C++ Interop: FloppyTurdGame is now directly accessible as a Swift reference type

} // namespace GameCore

#endif // FLOPPY_TURD_GAME_H