#include "FloppyTurdGame.h"
#include "../States/LoadingState.h"
#include "../States/MainMenuState.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#ifdef PLATFORM_IOS
#include "../../iOS/Threading/ThreadingProxy.h"
#endif
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <memory>
#include <algorithm>
#include <map>

#ifdef PLATFORM_IOS
#include "../../Engine/Platform/iOSPlatformImpl.h"
#else
#include "../../Engine/Platform/RaylibPlatformImpl.h"
#endif

namespace GameCore {

    // Global game instance
    FloppyTurdGame* g_Game = nullptr;
    
    // Static const member definitions
    const float FloppyTurdGame::TARGET_FPS = 60.0f;
    const char* FloppyTurdGame::SAVE_FILE_NAME = "floppyturd_save.dat";
    const char* FloppyTurdGame::SETTINGS_FILE_NAME = "floppyturd_settings.cfg";

    FloppyTurdGame::FloppyTurdGame()
        : m_ecsSystem(nullptr)
        , m_stateManager(nullptr)
        , m_initialized(false)
        , m_running(false)
        , m_paused(false)
        , m_highScore(0)
        , m_playerCoins(0)
        , m_musicVolume(0.7f)
        , m_sfxVolume(0.8f)
        , m_frameTime(0.0f)
        , m_targetFrameTime(1.0f / TARGET_FPS)
        , m_frameCount(0)
        , m_fpsTimer(0.0f)
        , m_currentFPS(0.0f)
        , m_showDebugInfo(false)
        , m_levelUnlockSoundTimer(0.0f)
        , m_pendingPartyHorn(false)
    {
        // Initialize game stats
        m_gameStats = {0, 0, 0, 0, 0, 0, 0, 0.0f, 0, 0};
        
        GN_LOG_INFO("Game instance created");
    }

    FloppyTurdGame::~FloppyTurdGame() {
        if (m_initialized) {
            Shutdown();
        }
        GN_LOG_INFO("Game instance destroyed");
    }

    bool FloppyTurdGame::Initialize() {
        if (m_initialized) {
            GN_LOG_WARN("Game already initialized");
            return true;
        }
        
        GN_LOG_INFO("Initializing Floppy Turd Game...");
        
        // Initialize platform using existing implementations
        #ifdef PLATFORM_IOS
        iOSPlatform::SetupDelegates(m_platformDelegates);
        GN_LOG_INFO("iOS platform delegates configured");
        #else
        RaylibPlatform::SetupDelegates(m_platformDelegates);
        if (!RaylibPlatform::Initialize(800, 600, "Floppy Turd")) {
            GN_LOG_ERROR("Failed to initialize Raylib platform");
            return false;
        }
        GN_LOG_INFO("Desktop platform initialized");
        #endif
        
        // Validate delegates
        if (!m_platformDelegates.IsValid()) {
            GN_LOG_ERROR("Platform delegates not properly configured");
            return false;
        }

        // Initialize core systems
        if (!InitializeECS()) {
            GN_LOG_ERROR("Failed to initialize ECS system");
            return false;
        }

        // Platform-specific system initialization (using delegates)
        #ifdef PLATFORM_IOS
        // iOS: Systems are handled by Swift components
        GN_LOG_INFO("iOS systems managed by Swift components");
        #else
        // Desktop: Initialize Raylib systems
        if (!InitializeAudio()) {
            GN_LOG_ERROR("Failed to initialize audio");
            return false;
        }
        
        if (!InitializeGraphics()) {
            GN_LOG_ERROR("Failed to initialize graphics");
            return false;
        }
        #endif

        // Initialize game states
        InitializeGameStates();

        // Load game data and settings
        LoadSettings();
        LoadGameData();
        LoadGameResources();

        // Set global game instance for access from other parts of the code
        SetGame(this);
        
        m_initialized = true;
        GN_LOG_INFO("Game initialization complete!");
        return true;
    }

    void FloppyTurdGame::Shutdown() {
        if (!m_initialized) {
            return;
        }

        GN_LOG_INFO("Shutting down game...");

        // Stop the game if running
        if (m_running) {
            m_running = false;
        }

        // Save game data
        SaveGameData();
        SaveSettings();

        // Unload resources
        UnloadGameResources();

        // Shutdown systems
        StopBackgroundMusic();
        
        if (m_stateManager) {
            m_stateManager.reset();
        }

        if (m_ecsSystem) {
            m_ecsSystem.reset();
        }

        m_initialized = false;
        
        GN_LOG_INFO("Game shutdown complete");
    }

    void FloppyTurdGame::Run() {
        if (!m_initialized) {
            GN_LOG_ERROR("Cannot run game - not initialized");
            return;
        }

        m_running = true;
        GN_LOG_INFO("Entering game loop...");

        auto lastTime = std::chrono::high_resolution_clock::now();

        while (m_running) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            // Handle input
            HandleInput();

            // Update game
            if (!m_paused) {
                Update(deltaTime);
            }

            // Render
            Render();

            // Update performance stats
            UpdatePerformanceStats(deltaTime);

            // Limit frame rate
            LimitFrameRate();
        }

        GN_LOG_INFO("Game loop ended");
    }

    void FloppyTurdGame::Update(float deltaTime) {
        if (!m_initialized || !m_running || m_paused) {
            return;
        }

        // 1. Update current game state FIRST (states manage entities)
        if (m_stateManager) {
            m_stateManager->Update(deltaTime);  // ✅ ENABLED
        }

        // 2. Check for state transitions
        if (m_stateManager && !m_stateManager->IsEmpty()) {
            GameState* currentState = m_stateManager->GetCurrentState();
            if (currentState && currentState->IsFinished()) {
                HandleStateTransition(currentState);
            }
        }

        // 3. Update ECS systems (process entities created by states)
        if (m_ecsSystem) {
            m_ecsSystem->Update(deltaTime);
        }

        // 4. Update debug info
        if (m_showDebugInfo) {
            UpdateDebugInfo(deltaTime);
        }

        // 5. Update level unlock sound timer
        if (m_pendingPartyHorn) {
            m_levelUnlockSoundTimer += deltaTime;
            GN_LOG_DEBUG("⏰ Party horn timer: " + std::to_string(m_levelUnlockSoundTimer) + "s / 1.0s");

            if (m_levelUnlockSoundTimer >= 1.0f) { // 1 second delay
                GN_LOG_INFO("🎊 Timer reached 1 second - playing partyhorn!");
                PlaySFX("partyhorn");
                m_pendingPartyHorn = false;
                m_levelUnlockSoundTimer = 0.0f;
                GN_LOG_INFO("✅ Party horn sequence completed");
            }
        }
    }

    void FloppyTurdGame::Render() {
        if (!m_initialized || !m_running) {
            return;
        }

        // Begin frame using platform delegates
        if (m_platformDelegates.renderer.beginFrame) {
            m_platformDelegates.renderer.beginFrame();
        }

        // Clear screen using platform delegates
        if (m_platformDelegates.renderer.clearScreen) {
            m_platformDelegates.renderer.clearScreen(0.0f, 0.0f, 0.0f, 1.0f); // Black background
        }

        // 1. Render current game state (creates/manages entities for rendering)
        if (m_stateManager) {
            m_stateManager->Render();  // ✅ ENABLED
        }

        // 2. Render ECS systems (render entities managed by states)
        if (m_ecsSystem) {
            m_ecsSystem->Render();  // ✅ ENABLED
        }

        // End frame using platform delegates
        if (m_platformDelegates.renderer.endFrame) {
            m_platformDelegates.renderer.endFrame();
        }

        // Present frame using platform delegates
        if (m_platformDelegates.renderer.present) {
            m_platformDelegates.renderer.present();
        }
    }

    void FloppyTurdGame::HandleInput() {
        if (!m_initialized) {
            return;
        }

        // Handle platform input using delegates
        // Input handling is done through the input delegate functions as needed
        // No general handleInput function in the delegate structure

        // Pass input to state manager
        if (m_stateManager) {
            m_stateManager->HandleInput();  // ✅ ENABLED
        }
    }

    // Game state management
    void FloppyTurdGame::StartGame() {
        if (!m_initialized) {
            GN_LOG_WARN("Cannot start game - not initialized");
            return;
        }
        
        GN_LOG_INFO("Starting new game...");
        m_running = true;
        m_paused = false;
        
        // Reset game stats for new game
        m_gameStats.totalGamesPlayed++;
        
        PlayBackgroundMusic();
    }

    void FloppyTurdGame::PauseGame() {
        if (m_running && !m_paused) {
            GN_LOG_INFO("Game paused");
            m_paused = true;
        }
    }

    void FloppyTurdGame::ResumeGame() {
        if (m_running && m_paused) {
            GN_LOG_INFO("Game resumed");
            m_paused = false;
        }
    }

    void FloppyTurdGame::EndGame() {
        GN_LOG_INFO("Ending game...");
        m_running = false;
        m_paused = false;
        StopBackgroundMusic();
    }

    void FloppyTurdGame::RestartGame() {
        GN_LOG_INFO("Restarting game...");
        EndGame();
        StartGame();
    }

    void FloppyTurdGame::ShowMainMenu() {
        GN_LOG_INFO("Showing main menu");
        
        // Start playing background music for the main menu
        PlayBackgroundMusic();
        
        // TODO: Implement main menu state
    }

    void FloppyTurdGame::ShowShop() {
        GN_LOG_INFO("Showing shop");
        // TODO: Implement shop state
    }

    void FloppyTurdGame::ShowGameOver(int score, int coins) {
        GN_LOG_INFO("Game Over - Score: " + std::to_string(score) + ", Coins: " + std::to_string(coins));
        
        // Update high score if needed
        if (score > m_highScore) {
            m_highScore = score;
            GN_LOG_INFO("New high score: " + std::to_string(m_highScore));
        }
        
        // Add coins
        AddCoins(coins);
        
        // Update game stats
        m_gameStats.totalScore += score;
        m_gameStats.totalCoinsCollected += coins;
        
        // TODO: Implement game over state
    }

    // Data management
    void FloppyTurdGame::SaveGameData() {
        GN_LOG_INFO("💰 Saving game data... Player coins: " + std::to_string(m_playerCoins));

        std::ofstream file(SAVE_FILE_NAME, std::ios::binary);
        if (file.is_open()) {
            // Save basic game data
            file.write(reinterpret_cast<const char*>(&m_highScore), sizeof(m_highScore));
            file.write(reinterpret_cast<const char*>(&m_playerCoins), sizeof(m_playerCoins));
            file.write(reinterpret_cast<const char*>(&m_gameStats), sizeof(m_gameStats));

            // Save only essential level data (no requirements)
            for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
                LevelSaveData saveData;
                saveData.highScore = m_levelStats[levelId].highScore;
                saveData.bestCoins = m_levelStats[levelId].bestCoins;
                saveData.unlocked = m_levelStats[levelId].unlocked;
                file.write(reinterpret_cast<const char*>(&saveData), sizeof(LevelSaveData));
            }

            file.close();
            GN_LOG_INFO("Game data saved successfully (requirements not saved)");
        } else {
            GN_LOG_ERROR("Failed to save game data");
        }
    }

    void FloppyTurdGame::LoadGameData() {
        GN_LOG_INFO("Loading game data...");

        // Log initial level 2 stats before loading
        GN_LOG_INFO("📊 BEFORE LoadGameData - Level 2 stats: unlockReq=" + std::to_string(m_levelStats[2].unlockRequirement) +
                   ", coinReq=" + std::to_string(m_levelStats[2].coinRequirement) + ", unlocked=" + std::to_string(m_levelStats[2].unlocked));

        std::ifstream file(SAVE_FILE_NAME, std::ios::binary);
        if (file.is_open()) {
            // Load basic game data
            file.read(reinterpret_cast<char*>(&m_highScore), sizeof(m_highScore));
            int oldCoins = m_playerCoins; // Store old value for logging
            file.read(reinterpret_cast<char*>(&m_playerCoins), sizeof(m_playerCoins));
            file.read(reinterpret_cast<char*>(&m_gameStats), sizeof(m_gameStats));
            GN_LOG_INFO("💰 LOADED COINS: " + std::to_string(oldCoins) + " → " + std::to_string(m_playerCoins));

            // Initialize level stats with default requirements first
            for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
                SetDefaultUnlockRequirements(levelId, m_levelStats[levelId]);
            }

            // Log level 2 stats after setting defaults
            GN_LOG_INFO("📊 AFTER setting defaults - Level 2 stats: unlockReq=" + std::to_string(m_levelStats[2].unlockRequirement) +
                       ", coinReq=" + std::to_string(m_levelStats[2].coinRequirement) + ", unlocked=" + std::to_string(m_levelStats[2].unlocked));

            // Load only essential level data (override with saved progress)
            for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
                LevelSaveData saveData;
                file.read(reinterpret_cast<char*>(&saveData), sizeof(LevelSaveData));

                // Keep the requirements set above, only load essential data
                m_levelStats[levelId].highScore = saveData.highScore;
                m_levelStats[levelId].bestCoins = saveData.bestCoins;
                m_levelStats[levelId].unlocked = saveData.unlocked;
            }

            file.close();
            GN_LOG_INFO("Game data loaded successfully - requirements set at runtime");

            // Log final level 2 stats after loading from save file
            GN_LOG_INFO("📊 FINAL after loading from save file - Level 2 stats: unlockReq=" + std::to_string(m_levelStats[2].unlockRequirement) +
                       ", coinReq=" + std::to_string(m_levelStats[2].coinRequirement) + ", unlocked=" + std::to_string(m_levelStats[2].unlocked));
        } else {
            GN_LOG_INFO("No save file found, using defaults");
            ResetGameData();

            // Log final level 2 stats after reset
            GN_LOG_INFO("📊 FINAL after reset - Level 2 stats: unlockReq=" + std::to_string(m_levelStats[2].unlockRequirement) +
                       ", coinReq=" + std::to_string(m_levelStats[2].coinRequirement) + ", unlocked=" + std::to_string(m_levelStats[2].unlocked));
        }
    }

    void FloppyTurdGame::SetMusicVolume(float volume) {
        m_musicVolume = std::max(0.0f, std::min(1.0f, volume));
        GN_LOG_INFO("Music volume set to: " + std::to_string(m_musicVolume));
        if (m_platformDelegates.audio.setMusicVolume) {
            m_platformDelegates.audio.setMusicVolume(m_masterVolume * m_musicVolume);
        }
    }

    void FloppyTurdGame::SetSFXVolume(float volume) {
        m_sfxVolume = std::max(0.0f, std::min(1.0f, volume));
        GN_LOG_INFO("SFX volume set to: " + std::to_string(m_sfxVolume));
        if (m_platformDelegates.audio.setSFXVolume) {
            m_platformDelegates.audio.setSFXVolume(m_masterVolume * m_sfxVolume);
        }
    }

    void FloppyTurdGame::SetMasterVolume(float volume) {
        m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
        GN_LOG_INFO("Master volume set to: " + std::to_string(m_masterVolume));
        // Re-apply child volumes to platform
        if (m_platformDelegates.audio.setMusicVolume) {
            m_platformDelegates.audio.setMusicVolume(m_masterVolume * m_musicVolume);
        }
        if (m_platformDelegates.audio.setSFXVolume) {
            m_platformDelegates.audio.setSFXVolume(m_masterVolume * m_sfxVolume);
        }
    }

    void FloppyTurdGame::UpdateGameStats(const GameStats& stats) {
        m_gameStats = stats;
        GN_LOG_DEBUG("Game stats updated");
    }

    // Private helper methods
    bool FloppyTurdGame::InitializeECS() {
        GN_LOG_INFO("Initializing ECS system...");
        
        m_ecsSystem = std::unique_ptr<Gnosis::ECS>(new Gnosis::ECS());
        if (!m_ecsSystem) {
            return false;
        }

        // Initialize ECS with platform delegates for integrated rendering
        m_ecsSystem->Initialize(m_platformDelegates);
        
        GN_LOG_INFO("ECS system initialized with integrated sprite rendering");
        return true;
    }

    bool FloppyTurdGame::InitializePlatform() {
        GN_LOG_INFO("Initializing platform...");
        
        // Platform delegates are already initialized in Initialize() method
        if (!m_platformDelegates.IsValid()) {
            GN_LOG_ERROR("Platform delegates not properly configured");
            return false;
        }
        
        GN_LOG_INFO("Platform initialized");
        return true;
    }

    bool FloppyTurdGame::InitializeAudio() {
        GN_LOG_INFO("Initializing audio...");
        
        // TODO: Initialize audio system
        
        GN_LOG_INFO("Audio initialized");
        return true;
    }

    bool FloppyTurdGame::InitializeGraphics() {
        GN_LOG_INFO("Initializing graphics...");
        
        // TODO: Initialize graphics system
        
        GN_LOG_INFO("Graphics initialized");
        return true;
    }

    void FloppyTurdGame::InitializeGameStates() {
        GN_LOG_INFO("Initializing game states...");
        
        // Initialize state manager
        m_stateManager = std::make_unique<GameStateManager>();
        
        // Start with loading state using shared ECS coordinator
        auto loadingState = std::make_unique<LoadingState>(m_ecsSystem.get());
        m_stateManager->PushState(std::move(loadingState));
        
        GN_LOG_INFO("Game states initialized - starting with LoadingState");
    }

    void FloppyTurdGame::HandleStateTransition(GameState* finishedState) {
        if (!finishedState) {
            return;
        }
        
        const char* stateName = finishedState->GetStateName();
        GN_LOG_INFO("Handling state transition from: %s", stateName);
        
        if (strcmp(stateName, "Loading") == 0) {
            // Transition from loading to main menu
            auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
            m_stateManager->ChangeState(std::move(mainMenuState));
            GN_LOG_INFO("Transitioned to MainMenuState");
        }
        else if (strcmp(stateName, "MainMenu") == 0) {
            // Handle main menu selections - check if a level was selected
            MainMenuState* mainMenu = dynamic_cast<MainMenuState*>(finishedState);
            if (mainMenu && mainMenu->GetSelectedLevelIndex() >= 0) {
                // Transition to gameplay with selected level
                int selectedLevel = mainMenu->GetSelectedLevelIndex();
                auto gameplayState = std::make_unique<GameplayState>(m_ecsSystem.get(), &m_platformDelegates, selectedLevel);
                m_stateManager->ChangeState(std::move(gameplayState));
                GN_LOG_INFO("Transitioned to GameplayState with level: " + std::to_string(selectedLevel));
            } else {
                // Return to main menu (no level selected)
                auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
                m_stateManager->ChangeState(std::move(mainMenuState));
                GN_LOG_INFO("Returned to MainMenuState");
            }
        }
        else if (strcmp(stateName, "Gameplay") == 0) {
            // Handle gameplay state transitions (game over, level complete, etc.)
            // Save game data when returning from gameplay to ensure coins are persisted
            GN_LOG_INFO("Saving game data before transitioning from Gameplay to MainMenu");
            SaveGameData();

            auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
            m_stateManager->ChangeState(std::move(mainMenuState));
            GN_LOG_INFO("Gameplay finished - returned to MainMenuState");
        }
        else {
            GN_LOG_WARN("Unknown state transition from: %s", stateName);
        }
    }

    void FloppyTurdGame::UpdatePerformanceStats(float deltaTime) {
        m_frameTime = deltaTime;
        m_frameCount++;
        m_fpsTimer += deltaTime;
        
        if (m_fpsTimer >= 1.0f) {
            m_currentFPS = static_cast<float>(m_frameCount) / m_fpsTimer;
            m_frameCount = 0;
            m_fpsTimer = 0.0f;
        }
    }

    void FloppyTurdGame::LimitFrameRate() {
        // Simple frame rate limiting
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<float>(frameEnd - frameEnd).count();
        
        if (frameDuration < m_targetFrameTime) {
            auto sleepTime = m_targetFrameTime - frameDuration;
            std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        }
    }

    void FloppyTurdGame::LoadSettings() {
        GN_LOG_INFO("Loading settings...");
        
        std::ifstream file(SETTINGS_FILE_NAME);
        if (file.is_open()) {
            // Try new format: master music sfx debug
            file >> m_masterVolume >> m_musicVolume >> m_sfxVolume >> m_showDebugInfo;
            if (!file.fail()) {
                file.close();
                GN_LOG_INFO("Settings loaded successfully (v2)");
            } else {
                // Fallback to legacy format: music sfx debug
                file.clear();
                file.seekg(0);
                if (file >> m_musicVolume >> m_sfxVolume >> m_showDebugInfo) {
                    m_masterVolume = 1.0f;
                    GN_LOG_INFO("Settings loaded successfully (legacy v1), defaulting masterVolume=1.0");
                } else {
                    GN_LOG_WARN("Failed to parse settings file, using defaults");
                }
                file.close();
            }
        } else {
            GN_LOG_INFO("No settings file found, using defaults");
        }
    }

    void FloppyTurdGame::SaveSettings() {
        GN_LOG_INFO("Saving settings...");
        
        std::ofstream file(SETTINGS_FILE_NAME);
        if (file.is_open()) {
            file << m_masterVolume << " " << m_musicVolume << " " << m_sfxVolume << " " << m_showDebugInfo;
            file.close();
            GN_LOG_INFO("Settings saved successfully");
        } else {
            GN_LOG_ERROR("Failed to save settings");
        }
    }

    void FloppyTurdGame::ResetGameData() {
        GN_LOG_INFO("🔄 ResetGameData called - resetting to defaults");
        m_highScore = 0;
        m_playerCoins = 0;
        m_gameStats = {0, 0, 0, 0, 0, 0, 0, 0.0f, 0, 0};

        // Initialize level stats with default values
        for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
            LevelStats& stats = m_levelStats[levelId];
            stats.highScore = 0;
            stats.bestCoins = 0;
            stats.unlocked = (levelId == 1); // Only first level unlocked by default
            SetDefaultUnlockRequirements(levelId, stats);
        }

        // Explicitly ensure level 2 has 0 requirements for debug testing
        m_levelStats[2].unlockRequirement = 0;
        m_levelStats[2].coinRequirement = 0;

        GN_LOG_INFO("Game data reset to defaults");
    }

    void FloppyTurdGame::PlayBackgroundMusic() {
        GN_LOG_INFO("Starting background music via delegates");
        
        // Use platform audio delegate consistently for all platforms
        if (m_platformDelegates.audio.playMusic) {
            m_platformDelegates.audio.playMusic("FloppyTurdMenu.mp3", m_masterVolume * m_musicVolume, -1);  // Use stored volume, loop infinitely
            GN_LOG_INFO("Background music started via platform delegate");
        } else {
            GN_LOG_WARN("Audio delegate not available - cannot play background music");
        }
    }

    void FloppyTurdGame::StopBackgroundMusic() {
        GN_LOG_INFO("Stopping background music");
        
        // Use platform audio delegate to stop music
        if (m_platformDelegates.audio.stopMusic) {
            m_platformDelegates.audio.stopMusic();
            GN_LOG_INFO("Background music stopped via platform delegate");
        } else {
            GN_LOG_WARN("Audio delegate not available - cannot stop background music");
        }
    }

    void FloppyTurdGame::PlaySFX(const std::string& soundName) {
        GN_LOG_INFO("🎵 PlaySFX called with: '" + soundName + "'");

        // Use platform audio delegate to play sound effect
        if (m_platformDelegates.audio.playSound) {
            GN_LOG_INFO("🔊 Calling platform delegate to play: '" + soundName + "'");
            m_platformDelegates.audio.playSound(soundName.c_str(), m_masterVolume * m_sfxVolume);
            GN_LOG_INFO("✅ SFX request sent to platform: '" + soundName + "'");
        } else {
            GN_LOG_ERROR("❌ Audio delegate not available - cannot play SFX: '" + soundName + "'");
        }
    }

    void FloppyTurdGame::LoadGameResources() {
        GN_LOG_INFO("Loading game resources...");
        // TODO: Load textures, sounds, etc.
        GN_LOG_INFO("Game resources loaded");
    }

    void FloppyTurdGame::UnloadGameResources() {
        GN_LOG_INFO("Unloading game resources...");
        // TODO: Unload resources
        GN_LOG_INFO("Game resources unloaded");
    }

    void FloppyTurdGame::UpdateDebugInfo(float deltaTime) {
        // Update debug information
    }

    void FloppyTurdGame::RenderDebugInfo() {
        // Render debug overlay
    }

    // SetSwiftComponents removed - Swift components managed entirely on Swift side

    // Level-based high scores and unlocking system
    const FloppyTurdGame::LevelStats& FloppyTurdGame::GetLevelStats(int levelId) const {
        static const LevelStats defaultStats = {0, 0, false, 0, 0};
        if (levelId >= 1 && levelId <= MAX_LEVELS) {
            // FORCE level 2 to always have 0 requirements for debug testing
            if (levelId == 2) {
                const_cast<LevelStats&>(m_levelStats[2]).unlockRequirement = 0;
                const_cast<LevelStats&>(m_levelStats[2]).coinRequirement = 0;
            }

            return m_levelStats[levelId];
        }
        return defaultStats;
    }

    void FloppyTurdGame::UpdateLevelStats(int levelId, const LevelStats& stats) {
        if (levelId >= 1 && levelId <= MAX_LEVELS) {
            m_levelStats[levelId] = stats;
            SaveGameData();
            GN_LOG_INFO("Updated level " + std::to_string(levelId) + " stats");
        }
    }

    bool FloppyTurdGame::IsLevelUnlocked(int levelId) const {
        if (levelId == 1) return true; // First level always unlocked
        if (levelId >= 2 && levelId <= MAX_LEVELS) {
            return m_levelStats[levelId].unlocked;
        }
        return false;
    }

    void FloppyTurdGame::UnlockLevel(int levelId) {
        GN_LOG_INFO("🏆 UnlockLevel called for level " + std::to_string(levelId));

        if (levelId >= 1 && levelId <= MAX_LEVELS) {
            m_levelStats[levelId].unlocked = true;
            SaveGameData();

            // Play level unlock sound effects
            GN_LOG_INFO("🎉 Unlocked level " + std::to_string(levelId) + " - playing sound effects!");
            GN_LOG_INFO("🔊 Playing balloonpop sound...");
            PlaySFX("balloonpop");
            GN_LOG_INFO("✅ PlaySFX(balloonpop) called");

            // Schedule party horn sound to play after 1 second delay
            GN_LOG_INFO("⏰ Scheduling partyhorn sound to play in 1 second...");
            m_pendingPartyHorn = true;
            m_levelUnlockSoundTimer = 0.0f;
            GN_LOG_INFO("✅ Party horn scheduled - m_pendingPartyHorn=true, timer=0.0f");

            GN_LOG_INFO("✅ Level " + std::to_string(levelId) + " unlocked successfully!");
        } else {
            GN_LOG_WARN("⚠️ Invalid level ID: " + std::to_string(levelId));
        }
    }

    int FloppyTurdGame::GetLevelHighScore(int levelId) const {
        if (levelId >= 1 && levelId <= MAX_LEVELS) {
            return m_levelStats[levelId].highScore;
        }
        return 0;
    }

    void FloppyTurdGame::UpdateLevelHighScore(int levelId, int score, int coins) {
        if (levelId >= 1 && levelId <= MAX_LEVELS) {
            // Update existing stats
            if (score > m_levelStats[levelId].highScore) {
                m_levelStats[levelId].highScore = score;
                GN_LOG_INFO("New high score for level " + std::to_string(levelId) + ": " + std::to_string(score));
            }
            if (coins > m_levelStats[levelId].bestCoins) {
                m_levelStats[levelId].bestCoins = coins;
            }

            // Check if this level completion unlocks the next level
            CheckLevelUnlock(levelId, score, coins);

            SaveGameData();
        }
    }

    void FloppyTurdGame::SetDefaultUnlockRequirements(int levelId, LevelStats& stats) {
        // Set unlock requirements based on level progression
        int oldUnlockReq = stats.unlockRequirement;
        int oldCoinReq = stats.coinRequirement;

        GN_LOG_INFO("🎯 SetDefaultUnlockRequirements CALLED for level " + std::to_string(levelId) +
                   " - BEFORE: unlockReq=" + std::to_string(oldUnlockReq) + ", coinReq=" + std::to_string(oldCoinReq));

        switch (levelId) {
            case 1: // Park - always unlocked
                stats.unlocked = true;
                stats.unlockRequirement = 0;
                stats.coinRequirement = 0;
                break;
            case 2: // Sewer - TEMPORARILY SET TO 0 PIPES FOR TESTING
                stats.unlockRequirement = 0; // pipes from Park (level 1) - TEMP: 0 for testing
                stats.coinRequirement = 0;
                GN_LOG_INFO("🎯 LEVEL 2: Setting unlockRequirement to 0 for debug testing!");
                break;
            case 3: // Desert - 50 pipes from Sewer + 100 coins
                stats.unlockRequirement = 50; // pipes from Sewer (level 2)
                stats.coinRequirement = 100;
                break;
            case 4: // Snow - 50 pipes from Desert + 250 coins
                stats.unlockRequirement = 50; // pipes from Desert (level 3)
                stats.coinRequirement = 250;
                break;
            case 5: // Castle - 50 pipes from Snow + 500 coins
                stats.unlockRequirement = 50; // pipes from Snow (level 4)
                stats.coinRequirement = 500;
                break;
            case 6: // Boss - 50 pipes from Castle + 1000 coins
                stats.unlockRequirement = 50; // pipes from Castle (level 5)
                stats.coinRequirement = 1000;
                break;
            default:
                stats.unlockRequirement = 0;
                stats.coinRequirement = 0;
                break;
        }

        if (oldUnlockReq != stats.unlockRequirement || oldCoinReq != stats.coinRequirement) {
            GN_LOG_INFO("🎯 SetDefaultUnlockRequirements: Level " + std::to_string(levelId) +
                       " requirements changed from (" + std::to_string(oldUnlockReq) + ", " + std::to_string(oldCoinReq) +
                       ") to (" + std::to_string(stats.unlockRequirement) + ", " + std::to_string(stats.coinRequirement) + ")");
        } else {
            GN_LOG_INFO("🎯 SetDefaultUnlockRequirements: Level " + std::to_string(levelId) +
                       " requirements unchanged (" + std::to_string(stats.unlockRequirement) + ", " + std::to_string(stats.coinRequirement) + ")");
        }
    }

    bool FloppyTurdGame::CanUnlockLevel(int levelId, std::string& failureMessage) {
        GN_LOG_INFO("🔍 CanUnlockLevel called for level " + std::to_string(levelId));

        if (levelId < 2 || levelId > MAX_LEVELS) {
            failureMessage = "Invalid level";
            GN_LOG_INFO("❌ Invalid level ID: " + std::to_string(levelId));
            return false;
        }

        const LevelStats& levelStats = m_levelStats[levelId];

        // FORCE level 2 to have 0 requirements for debug testing
        int effectiveUnlockRequirement = levelStats.unlockRequirement;
        int effectiveCoinRequirement = levelStats.coinRequirement;
        if (levelId == 2) {
            effectiveUnlockRequirement = 0;
            effectiveCoinRequirement = 0;
            GN_LOG_INFO("🎯 LEVEL 2 DEBUG: Original requirements were pipes=" + std::to_string(levelStats.unlockRequirement) + ", coins=" + std::to_string(levelStats.coinRequirement));
            GN_LOG_INFO("🎯 LEVEL 2 DEBUG: FORCED to effective requirements: pipes=0, coins=0");
        }

        GN_LOG_INFO("📊 Level " + std::to_string(levelId) + " requirements: pipes=" + std::to_string(effectiveUnlockRequirement) + ", coins=" + std::to_string(effectiveCoinRequirement));

        // Check pipe requirement if it exists
        GN_LOG_INFO("🔍 Checking pipe requirement: effectiveUnlockRequirement=" + std::to_string(effectiveUnlockRequirement) + " (level " + std::to_string(levelId) + ")");
        if (effectiveUnlockRequirement > 0) {
            GN_LOG_INFO("⚠️ Pipe requirement check triggered for level " + std::to_string(levelId));
            // Get the high score from the required previous level
            int requiredLevelId;
            if (levelId == 2) {
                requiredLevelId = 1; // Sewer requires pipes from Park (level 1)
            } else if (levelId == 3) {
                requiredLevelId = 2; // Desert requires pipes from Sewer (level 2)
            } else if (levelId == 4) {
                requiredLevelId = 3; // Snow requires pipes from Desert (level 3)
            } else if (levelId == 5) {
                requiredLevelId = 4; // Castle requires pipes from Snow (level 4)
            } else if (levelId == 6) {
                requiredLevelId = 5; // Boss requires pipes from Castle (level 5)
            } else {
                requiredLevelId = levelId - 1;
            }

            int prevLevelHighScore = GetLevelHighScore(requiredLevelId);
            GN_LOG_INFO("🎯 Checking pipes: need " + std::to_string(effectiveUnlockRequirement) + " from level " + std::to_string(requiredLevelId) + ", current high score: " + std::to_string(prevLevelHighScore));

            if (prevLevelHighScore < effectiveUnlockRequirement) {
                failureMessage = "Need " + std::to_string(effectiveUnlockRequirement) + " pipes from previous level";
                GN_LOG_INFO("❌ Pipe requirement not met: " + failureMessage);
                return false;
            }
            GN_LOG_INFO("✅ Pipe requirement met!");
        } else {
            GN_LOG_INFO("ℹ️ No pipe requirement for this level");
        }

        // Check coin requirement if it exists
        GN_LOG_INFO("🔍 Checking coin requirement: effectiveCoinRequirement=" + std::to_string(effectiveCoinRequirement) + " (level " + std::to_string(levelId) + ")");
        if (effectiveCoinRequirement > 0) {
            GN_LOG_INFO("⚠️ Coin requirement check triggered for level " + std::to_string(levelId));
            GN_LOG_INFO("💰 Checking coins: need " + std::to_string(effectiveCoinRequirement) + ", current coins: " + std::to_string(m_playerCoins));
            if (m_playerCoins < effectiveCoinRequirement) {
                failureMessage = "Need " + std::to_string(effectiveCoinRequirement) + " coins";
                GN_LOG_INFO("❌ Coin requirement not met: " + failureMessage);
                return false;
            }
            GN_LOG_INFO("✅ Coin requirement met!");
        } else {
            GN_LOG_INFO("ℹ️ No coin requirement for this level");
        }

        GN_LOG_INFO("🎉 All requirements met for level " + std::to_string(levelId));
        GN_LOG_INFO("✅ CanUnlockLevel returning TRUE for level " + std::to_string(levelId));
        return true;
    }

    bool FloppyTurdGame::TryUnlockLevel(int levelId) {
        GN_LOG_INFO("🔓 TryUnlockLevel called for level " + std::to_string(levelId));

        std::string failureMessage;
        if (!CanUnlockLevel(levelId, failureMessage)) {
            GN_LOG_INFO("❌ Cannot unlock level " + std::to_string(levelId) + ": " + failureMessage);
            return false;
        }

        GN_LOG_INFO("✅ Requirements met for level " + std::to_string(levelId));

        const LevelStats& levelStats = m_levelStats[levelId];

        // FORCE level 2 to have 0 requirements for debug testing
        int effectiveCoinRequirement = levelStats.coinRequirement;
        if (levelId == 2) {
            effectiveCoinRequirement = 0;
        }

        // Deduct coins if required
        if (effectiveCoinRequirement > 0) {
            SpendCoins(effectiveCoinRequirement);
            GN_LOG_INFO("💰 Spent " + std::to_string(effectiveCoinRequirement) + " coins to unlock level " + std::to_string(levelId));
        }

        // Unlock the level
        UnlockLevel(levelId);
        GN_LOG_INFO("🎉 Successfully unlocked level " + std::to_string(levelId) + " via manual unlock button!");
        return true;
    }

    void FloppyTurdGame::CheckLevelUnlock(int completedLevelId, int score, int coins) {
        // This method is now deprecated - levels are only unlocked manually via unlock button
        // Keeping the method for potential future use but removing automatic unlock logic
        GN_LOG_DEBUG("CheckLevelUnlock called but automatic unlocking is disabled - use manual unlock button instead");
    }

    // Global utility functions
    FloppyTurdGame* GetGame() {
        return g_Game;
    }

    void SetGame(FloppyTurdGame* game) {
        g_Game = game;
    }

} // namespace FloppyTurd