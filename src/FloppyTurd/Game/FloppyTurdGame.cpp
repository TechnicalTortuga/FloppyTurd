#include "FloppyTurdGame.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Systems/RenderSystem.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <memory>

namespace FloppyTurd {

    // Global game instance
    FloppyTurdGame* g_Game = nullptr;
    
    // Static const member definitions
    const float FloppyTurdGame::TARGET_FPS = 60.0f;
    const char* FloppyTurdGame::SAVE_FILE_NAME = "floppyturd_save.dat";
    const char* FloppyTurdGame::SETTINGS_FILE_NAME = "floppyturd_settings.cfg";

    FloppyTurdGame::FloppyTurdGame()
        : m_ecsSystem(nullptr)
        , m_stateManager(nullptr)
        , m_platform(nullptr)
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
    {
        // Initialize game stats
        m_gameStats = {0, 0, 0, 0, 0, 0.0f, 0, 0};
        
        GN_LOG_INFO("Game instance created");
    }

    FloppyTurdGame::~FloppyTurdGame() {
        if (m_initialized) {
            Shutdown();
        }
        GN_LOG_INFO("Game instance destroyed");
    }

    bool FloppyTurdGame::Initialize(Gnosis::IPlatform* platform) {
        if (m_initialized) {
            GN_LOG_WARN("Game already initialized");
            return true;
        }

        if (!platform) {
            GN_LOG_ERROR("Platform interface is null");
            return false;
        }

        m_platform = platform;
        
        GN_LOG_INFO("Initializing Floppy Turd Game...");

        // Initialize core systems
        if (!InitializeECS()) {
            GN_LOG_ERROR("Failed to initialize ECS system");
            return false;
        }

        if (!InitializePlatform()) {
            GN_LOG_ERROR("Failed to initialize platform");
            return false;
        }

        if (!InitializeAudio()) {
            GN_LOG_ERROR("Failed to initialize audio");
            return false;
        }

        if (!InitializeGraphics()) {
            GN_LOG_ERROR("Failed to initialize graphics");
            return false;
        }

        // Initialize game states
        InitializeGameStates();

        // Load game data and settings
        LoadSettings();
        LoadGameData();
        LoadGameResources();

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

        m_platform = nullptr;
        m_initialized = false;
        
        GN_LOG_INFO("Game shutdown complete");
    }

    void FloppyTurdGame::Run() {
        if (!m_initialized) {
            GN_LOG_ERROR("Cannot run game - not initialized");
            return;
        }

        m_running = true;
        GN_LOG_INFO("Starting game loop...");

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

        // Update ECS systems
        if (m_ecsSystem) {
            m_ecsSystem->Update(deltaTime);
        }

        // Update state manager
        if (m_stateManager) {
            // m_stateManager->Update(deltaTime);
        }

        // Update debug info
        if (m_showDebugInfo) {
            UpdateDebugInfo(deltaTime);
        }
    }

    void FloppyTurdGame::Render() {
        if (!m_initialized || !m_running) {
            return;
        }

        // Clear screen
        if (m_platform) {
            // m_platform->ClearScreen();
        }

        // Render ECS systems
        if (m_ecsSystem) {
            // Get render system and render
            // auto renderSystem = m_ecsSystem->GetSystem<Gnosis::RenderSystem>();
            // if (renderSystem) {
            //     renderSystem->Render();
            // }
        }

        // Render state manager
        if (m_stateManager) {
            // m_stateManager->Render();
        }

        // Render debug info
        if (m_showDebugInfo) {
            RenderDebugInfo();
        }

        // Present frame
        if (m_platform) {
            // m_platform->PresentFrame();
        }
    }

    void FloppyTurdGame::HandleInput() {
        if (!m_initialized || !m_platform) {
            return;
        }

        // Handle platform input
        // m_platform->HandleInput();

        // Pass input to state manager
        if (m_stateManager) {
            // m_stateManager->HandleInput();
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
        GN_LOG_INFO("Saving game data...");
        
        std::ofstream file(SAVE_FILE_NAME, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(&m_highScore), sizeof(m_highScore));
            file.write(reinterpret_cast<const char*>(&m_playerCoins), sizeof(m_playerCoins));
            file.write(reinterpret_cast<const char*>(&m_gameStats), sizeof(m_gameStats));
            file.close();
            GN_LOG_INFO("Game data saved successfully");
        } else {
            GN_LOG_ERROR("Failed to save game data");
        }
    }

    void FloppyTurdGame::LoadGameData() {
        GN_LOG_INFO("Loading game data...");
        
        std::ifstream file(SAVE_FILE_NAME, std::ios::binary);
        if (file.is_open()) {
            file.read(reinterpret_cast<char*>(&m_highScore), sizeof(m_highScore));
            file.read(reinterpret_cast<char*>(&m_playerCoins), sizeof(m_playerCoins));
            file.read(reinterpret_cast<char*>(&m_gameStats), sizeof(m_gameStats));
            file.close();
            GN_LOG_INFO("Game data loaded successfully");
        } else {
            GN_LOG_INFO("No save file found, using defaults");
            ResetGameData();
        }
    }

    void FloppyTurdGame::SetMusicVolume(float volume) {
        m_musicVolume = std::max(0.0f, std::min(1.0f, volume));
        GN_LOG_INFO("Music volume set to: " + std::to_string(m_musicVolume));
    }

    void FloppyTurdGame::SetSFXVolume(float volume) {
        m_sfxVolume = std::max(0.0f, std::min(1.0f, volume));
        GN_LOG_INFO("SFX volume set to: " + std::to_string(m_sfxVolume));
    }

    void FloppyTurdGame::UpdateGameStats(const GameStats& stats) {
        m_gameStats = stats;
        GN_LOG_DEBUG("Game stats updated");
    }

    // Private helper methods
    bool FloppyTurdGame::InitializeECS() {
        GN_LOG_INFO("Initializing ECS system...");
        
        m_ecsSystem = std::make_unique<Gnosis::ECS>();
        if (!m_ecsSystem) {
            return false;
        }

        // Register systems
        // m_ecsSystem->RegisterSystem<Gnosis::RenderSystem>();
        
        GN_LOG_INFO("ECS system initialized");
        return true;
    }

    bool FloppyTurdGame::InitializePlatform() {
        GN_LOG_INFO("Initializing platform...");
        
        if (!m_platform) {
            return false;
        }

        // Platform-specific initialization
        // return m_platform->Initialize();
        
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
        
        // TODO: Initialize state manager and states
        
        GN_LOG_INFO("Game states initialized");
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
            file >> m_musicVolume >> m_sfxVolume >> m_showDebugInfo;
            file.close();
            GN_LOG_INFO("Settings loaded successfully");
        } else {
            GN_LOG_INFO("No settings file found, using defaults");
        }
    }

    void FloppyTurdGame::SaveSettings() {
        GN_LOG_INFO("Saving settings...");
        
        std::ofstream file(SETTINGS_FILE_NAME);
        if (file.is_open()) {
            file << m_musicVolume << " " << m_sfxVolume << " " << m_showDebugInfo;
            file.close();
            GN_LOG_INFO("Settings saved successfully");
        } else {
            GN_LOG_ERROR("Failed to save settings");
        }
    }

    void FloppyTurdGame::ResetGameData() {
        m_highScore = 0;
        m_playerCoins = 0;
        m_gameStats = {0, 0, 0, 0, 0, 0.0f, 0, 0};
        GN_LOG_INFO("Game data reset to defaults");
    }

    void FloppyTurdGame::PlayBackgroundMusic() {
        GN_LOG_INFO("Starting background music");
        // TODO: Implement background music
    }

    void FloppyTurdGame::StopBackgroundMusic() {
        GN_LOG_INFO("Stopping background music");
        // TODO: Stop background music
    }

    void FloppyTurdGame::PlaySFX(const std::string& soundName) {
        GN_LOG_DEBUG("Playing SFX: " + soundName);
        // TODO: Implement SFX playback
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

    // Global utility functions
    FloppyTurdGame* GetGame() {
        return g_Game;
    }

    void SetGame(FloppyTurdGame* game) {
        g_Game = game;
    }

} // namespace FloppyTurd