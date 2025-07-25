#include "FloppyTurdGame.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <memory>
#include <algorithm>

#ifdef __APPLE__
#if TARGET_OS_IPHONE
#include "../../Engine/Platform/iOSPlatformImpl.h"
#else
#include "../../Engine/Platform/RaylibPlatformImpl.h"
#endif
#else
#include "../../Engine/Platform/RaylibPlatformImpl.h"
#endif

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
#ifdef __APPLE__
#if TARGET_OS_IPHONE
        , m_swiftMetalRenderer(nullptr)
        , m_swiftTouchInputHandler(nullptr)
        , m_swiftAudioHandler(nullptr)
#endif
#endif
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

    bool FloppyTurdGame::Initialize() {
        if (m_initialized) {
            GN_LOG_WARN("Game already initialized");
            return true;
        }
        
        GN_LOG_INFO("Initializing Floppy Turd Game...");
        
        // Platform-specific initialization using delegates
        #ifdef __APPLE__
        #if TARGET_OS_IPHONE
        // iOS: Ensure Swift components are set and setup delegates
        GN_LOG_INFO("Checking Swift components...");
        GN_LOG_INFO("m_swiftMetalRenderer: %p", m_swiftMetalRenderer);
        GN_LOG_INFO("m_swiftTouchInputHandler: %p", m_swiftTouchInputHandler);
        GN_LOG_INFO("m_swiftAudioHandler: %p", m_swiftAudioHandler);
        
        if (!m_swiftMetalRenderer || !m_swiftTouchInputHandler || !m_swiftAudioHandler) {
            GN_LOG_ERROR("iOS Swift components not set. Call SetSwiftComponents() first.");
            return false;
        }
        GN_LOG_INFO("All Swift components are valid, proceeding with delegate setup...");
        // Setup iOS platform delegates
        iOSPlatform::SetSwiftComponents(
            static_cast<FloppyTurd::MetalRenderer*>(m_swiftMetalRenderer),
            static_cast<FloppyTurd::TouchInputHandler*>(m_swiftTouchInputHandler),
            static_cast<FloppyTurd::AudioManagerSwift*>(m_swiftAudioHandler)
        );
        iOSPlatform::SetupDelegates(m_platformDelegates);
        GN_LOG_INFO("iOS platform delegates configured");
        #else
        // macOS: Setup Raylib delegates
        RaylibPlatform::SetupDelegates(m_platformDelegates);
        if (!RaylibPlatform::Initialize(800, 600, "Floppy Turd")) {
            GN_LOG_ERROR("Failed to initialize Raylib platform");
            return false;
        }
        GN_LOG_INFO("macOS Raylib platform initialized");
        #endif
        #else
        // Desktop: Setup Raylib delegates
        RaylibPlatform::SetupDelegates(m_platformDelegates);
        if (!RaylibPlatform::Initialize(800, 600, "Floppy Turd")) {
            GN_LOG_ERROR("Failed to initialize Raylib platform");
            return false;
        }
        GN_LOG_INFO("Desktop Raylib platform initialized");
        #endif

        // Initialize core systems
        if (!InitializeECS()) {
            GN_LOG_ERROR("Failed to initialize ECS system");
            return false;
        }

        // Platform-specific system initialization
        #ifdef __APPLE__
        #if TARGET_OS_IPHONE
        // iOS: Systems are handled by Swift components
        GN_LOG_INFO("iOS systems managed by Swift components");
        #else
        // macOS: Initialize Raylib systems (TODO)
        if (!InitializeAudio()) {
            GN_LOG_ERROR("Failed to initialize audio");
            return false;
        }
        
        if (!InitializeGraphics()) {
            GN_LOG_ERROR("Failed to initialize graphics");
            return false;
        }
        #endif
        #else
        // Desktop: Initialize Raylib systems (TODO)
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

        // Begin frame and clear screen using platform delegates
        if (m_platformDelegates.renderer.beginFrame) {
            m_platformDelegates.renderer.beginFrame();
        }
        if (m_platformDelegates.renderer.clearScreen) {
            m_platformDelegates.renderer.clearScreen(0.2f, 0.3f, 0.3f, 1.0f); // Dark blue-gray background
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

        // End frame using platform delegates
        if (m_platformDelegates.renderer.endFrame) {
            m_platformDelegates.renderer.endFrame();
        }
        
        // Present the rendered frame
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
        
        m_ecsSystem = std::unique_ptr<Gnosis::ECS>(new Gnosis::ECS());
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

    void FloppyTurdGame::SetSwiftComponents(void* metalRenderer, void* touchInputHandler, void* audioHandler) {
        GN_LOG_INFO("Setting Swift components...");
        GN_LOG_INFO("MetalRenderer pointer: %p", metalRenderer);
        GN_LOG_INFO("TouchInputHandler pointer: %p", touchInputHandler);
        GN_LOG_INFO("AudioHandler pointer: %p", audioHandler);
        
        if (!metalRenderer || !touchInputHandler || !audioHandler) {
            GN_LOG_ERROR("One or more Swift components are null");
            return;
        }
        
#ifdef __APPLE__
#if TARGET_OS_IPHONE
        m_swiftMetalRenderer = metalRenderer;
        m_swiftTouchInputHandler = touchInputHandler;
        m_swiftAudioHandler = audioHandler;
        
        GN_LOG_INFO("Swift components set successfully - iOS platform detected");
#else
        GN_LOG_WARN("SetSwiftComponents called on non-iOS platform - ignoring");
#endif
#else
        GN_LOG_WARN("SetSwiftComponents called on non-Apple platform - ignoring");
#endif
    }

    // Global utility functions
    FloppyTurdGame* GetGame() {
        return g_Game;
    }

    void SetGame(FloppyTurdGame* game) {
        g_Game = game;
    }

} // namespace FloppyTurd