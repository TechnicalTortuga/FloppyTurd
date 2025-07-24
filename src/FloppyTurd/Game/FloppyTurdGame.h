#ifndef FLOPPY_TURD_GAME_H
#define FLOPPY_TURD_GAME_H

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../States/GameState.h"
#include "../Entities/Player.h"
#include <memory>

namespace FloppyTurd {
    class GameStateManager;
}

// Swift 5.9+ Native C++ Interop Support
#if __has_include(<swift/bridging>)
#include <swift/bridging>
#endif

namespace FloppyTurd {

    /**
     * @brief Main game class for Floppy Turd
     * 
     * This class orchestrates the entire game, managing the ECS system,
     * platform interfaces, game states, and core game loop.
     */
    class FloppyTurdGame {
    public:
        FloppyTurdGame();
        ~FloppyTurdGame();
        
        // Swift 5.9+ C++ Interop: Allow copy/move for Swift compatibility
        FloppyTurdGame(const FloppyTurdGame& other) = default;
        FloppyTurdGame& operator=(const FloppyTurdGame& other) = default;
        FloppyTurdGame(FloppyTurdGame&& other) = default;
        FloppyTurdGame& operator=(FloppyTurdGame&& other) = default;

        // Game lifecycle - Single point of initialization
        bool Initialize();
        void Shutdown();
        void Run();
        
        // Platform-specific component setters (iOS only)
        // Note: Always declared for Swift C++ interop compatibility
        void SetSwiftComponents(void* metalRenderer, void* touchInputHandler, void* audioHandler);

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
        int GetPlayerCoins() const { return m_playerCoins; }
        void AddCoins(int amount) { m_playerCoins += amount; }
        void SpendCoins(int amount) { m_playerCoins -= amount; }

        // System access
        Gnosis::ECS* GetECS() const { return m_ecsSystem.get(); }

        // Game settings
        void SetMusicVolume(float volume);
        void SetSFXVolume(float volume);
        float GetMusicVolume() const { return m_musicVolume; }
        float GetSFXVolume() const { return m_sfxVolume; }

        // Game statistics
        struct GameStats {
            int totalGamesPlayed;
            int totalScore;
            int totalCoinsCollected;
            int totalJumps;
            int totalEnemiesKilled;
            float totalPlayTime;
            int currentStreak;
            int bestStreak;
        };
        
        const GameStats& GetGameStats() const { return m_gameStats; }
        void UpdateGameStats(const GameStats& stats);

    private:
        // Core systems
        std::unique_ptr<Gnosis::ECS> m_ecsSystem;
        std::unique_ptr<GameStateManager> m_stateManager;
        
        // Platform abstraction
        PlatformDelegates m_platformDelegates;
        
        // Platform-specific components
        #ifdef __APPLE__
        #if TARGET_OS_IPHONE
        // iOS: Direct Swift component references
        void* m_swiftMetalRenderer;
        void* m_swiftTouchInputHandler;
        void* m_swiftAudioHandler;
        #endif
        #endif

        // Game state
        bool m_initialized;
        bool m_running;
        bool m_paused;
        
        // Game data
        int m_highScore;
        int m_playerCoins;
        float m_musicVolume;
        float m_sfxVolume;
        GameStats m_gameStats;
        
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
        
        // Data management
        void LoadSettings();
        void SaveSettings();
        void ResetGameData();
        
        // Audio management
        void PlayBackgroundMusic();
        void StopBackgroundMusic();
        void PlaySFX(const std::string& soundName);
        
        // Resource management
        void LoadGameResources();
        void UnloadGameResources();
        
        // Debug and development
        void UpdateDebugInfo(float deltaTime);
        void RenderDebugInfo();
        bool m_showDebugInfo;
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

} // namespace FloppyTurd

#endif // FLOPPY_TURD_GAME_H