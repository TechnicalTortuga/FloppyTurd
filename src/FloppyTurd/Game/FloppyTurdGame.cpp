#include "FloppyTurdGame.h"
#include "../../Engine/Platform/SaveGameHelpers.h"
#include "../States/LoadingState.h"
#include "../States/MainMenuState.h"
#include "../States/LeaderboardState.h"
#include "../States/ScreenPromptState.h"
#include "../States/TransitionState.h"
#include "../States/CreditsState.h"
#include "../Config/LevelConfig.h"
#include "../Input/InputManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Configuration/ConfigManager.h"
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
        , m_isIOSPlatform(false)
        , m_isLoadingGameData(false)
        , m_pendingLandscapeLevelId(0)
        , m_enteredViaQuickplay(false)
        , m_lastPlayedLevelId(0)
    {
        // Detect platform at initialization
        #ifdef PLATFORM_IOS
        m_isIOSPlatform = true;
        #else
        m_isIOSPlatform = false;
        #endif

        // Initialize game stats (includes sessionEnemiesKilled)
        m_gameStats = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0, 0};

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

        // For iOS: Initialize InputManager early (after platform delegates but before ECS)
        // This ensures touch input is ready for Swift interop
        InputManager::InitializeInstance(nullptr, &m_platformDelegates);
        GN_LOG_INFO("InputManager singleton initialized (iOS - early)");
        #else
        RaylibPlatform::SetupDelegates(m_platformDelegates);
        if (!RaylibPlatform::Initialize(800, 600, "Floppy Turd")) {
            GN_LOG_ERROR("Failed to initialize Raylib platform");
            return false;
        }
        GN_LOG_INFO("Desktop platform initialized");

        // For Desktop: Initialize InputManager after Raylib is set up
        InputManager::InitializeInstance(nullptr, &m_platformDelegates);
        GN_LOG_INFO("InputManager singleton initialized (Desktop)");
        #endif

        // Validate delegates
        if (!m_platformDelegates.IsValid()) {
            GN_LOG_ERROR("Platform delegates not properly configured");
            return false;
        }

        // CRITICAL: Initialize ConfigManager immediately after delegates are set up
        // This ensures screen info is properly queried from the actual device
        auto& configManager = ConfigManager::Instance();
        configManager.Initialize(m_platformDelegates);
        GN_LOG_INFO("ConfigManager initialized with screen info: " + 
                   std::to_string(configManager.GetCurrentScreenInfo().pixelWidth) + "x" + 
                   std::to_string(configManager.GetCurrentScreenInfo().pixelHeight));

        // Initialize core systems
        if (!InitializeECS()) {
            GN_LOG_ERROR("Failed to initialize ECS system");
            return false;
        }

        // Update InputManager with ECS system now that it's available
        #ifdef PLATFORM_IOS
        // For iOS: Update the existing InputManager instance with ECS
        if (InputManager::GetInstance()) {
            InputManager::GetInstance()->SetECSSystem(m_ecsSystem.get());
            GN_LOG_INFO("InputManager singleton updated with ECS system (iOS)");
        }
        #else
        // For Desktop: Update the existing InputManager instance with ECS
        if (InputManager::GetInstance()) {
            InputManager::GetInstance()->SetECSSystem(m_ecsSystem.get());
            GN_LOG_INFO("InputManager singleton updated with ECS system (Desktop)");
        }
        #endif

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

        // Destroy InputManager singleton
        InputManager::DestroyInstance();
        GN_LOG_INFO("InputManager singleton destroyed");

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
        GN_LOG_INFO("💾 SaveGameData() CALLED - Stored coins: " + std::to_string(m_gameStats.storedCoins) + 
                   ", Loading flag: " + std::to_string(m_isLoadingGameData));
        
        // Skip saving if we're currently loading
        if (m_isLoadingGameData) {
            GN_LOG_WARN("⚠️ SaveGameData() skipped - currently loading game data");
            return;
        }

        // Serialize game data to JSON using helper
        std::string jsonData = GameCore::SaveGameHelpers::serializeGameData(*this);
        
        // Save via platform delegates (goes through ThreadingProxy command queue)
        if (m_platformDelegates.save.saveGameData) {
            bool success = m_platformDelegates.save.saveGameData(jsonData.c_str());
            if (success) {
                GN_LOG_INFO("✅ Game data save queued successfully (JSON format)");
            } else {
                GN_LOG_ERROR("❌ Failed to queue game data save");
            }
        } else {
            GN_LOG_ERROR("❌ Save delegate not configured");
        }
    }

    void FloppyTurdGame::LoadGameData() {
        GN_LOG_INFO("📖 Loading game data...");

        // Set loading flag to prevent saves during deserialization
        m_isLoadingGameData = true;

        // Load game data synchronously via platform delegates
        if (m_platformDelegates.save.loadGameData) {
            const char* jsonDataPtr = nullptr;
            bool loadSuccess = m_platformDelegates.save.loadGameData(&jsonDataPtr);
            
            if (loadSuccess && jsonDataPtr != nullptr) {
                // Deserialize the loaded JSON data
                bool deserializeSuccess = GameCore::SaveGameHelpers::deserializeGameData(*this, jsonDataPtr);
                if (deserializeSuccess) {
                    GN_LOG_INFO("✅ Game data loaded and deserialized successfully");
                    GN_LOG_INFO("📊 Loaded - Coins: " + std::to_string(m_gameStats.storedCoins) + 
                               ", High Score: " + std::to_string(m_highScore));
                    
                    // Set proper unlock requirements for all levels after loading
                    for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
                        SetDefaultUnlockRequirements(levelId, m_levelStats[levelId]);
                    }
                    GN_LOG_INFO("✅ Unlock requirements set for all levels after load");
                    
                    // Clear loading flag
                    m_isLoadingGameData = false;
                } else {
                    GN_LOG_ERROR("❌ Failed to deserialize game data, using defaults");
                    m_isLoadingGameData = false;
                    ResetGameData();
                }
            } else {
                GN_LOG_INFO("ℹ️ No save file found, using defaults");
                m_isLoadingGameData = false;
                ResetGameData();
            }
        } else {
            GN_LOG_WARN("⚠️ Save delegate not configured, using defaults");
            m_isLoadingGameData = false;
            ResetGameData();
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
            
            // Check if transitioning to leaderboard
            if (mainMenu && mainMenu->IsTransitioningToLeaderboard()) {
                GN_LOG_INFO("Transitioning to LeaderboardState");
                auto leaderboardState = std::make_unique<LeaderboardState>(m_ecsSystem.get(), &m_platformDelegates);
                m_stateManager->ChangeState(std::move(leaderboardState));
                return;
            }
            
            if (mainMenu && mainMenu->GetSelectedLevelIndex() >= 0) {
                // Transition to gameplay with selected level
                int selectedLevel = mainMenu->GetSelectedLevelIndex();
                
                // Store Quickplay flag and level for when we return
                m_enteredViaQuickplay = mainMenu->GetEnteredViaQuickplay();
                m_lastPlayedLevelId = selectedLevel;
                GN_LOG_INFO("Starting level " + std::to_string(selectedLevel) + " - Quickplay: " + std::string(m_enteredViaQuickplay ? "YES" : "NO"));

                // Check if level requires landscape mode
                LevelConfig levelConfig = LevelConfigFactory::GetLevelConfig(selectedLevel);
                if (levelConfig.forceLandscape) {
                    // Show screen prompt - this will be the active state until landscape is detected
                    auto screenPrompt = std::make_unique<ScreenPromptState>(m_ecsSystem.get(), &m_platformDelegates);
                    m_stateManager->ChangeState(std::move(screenPrompt));
                    GN_LOG_INFO("Showing screen prompt for landscape-required level: " + std::to_string(selectedLevel));

                    // Store the level info for when ScreenPromptState finishes
                    // The ScreenPromptState will create and push the GameplayState when ready
                    m_pendingLandscapeLevelId = selectedLevel;
                } else {
                    // Normal level - direct transition to gameplay
                    auto gameplayState = std::make_unique<GameplayState>(m_ecsSystem.get(), &m_platformDelegates, selectedLevel);
                    m_stateManager->ChangeState(std::move(gameplayState));
                    GN_LOG_INFO("Transitioned to GameplayState with level: " + std::to_string(selectedLevel));
                }
            } else {
                // Return to main menu (no level selected)
                auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
                m_stateManager->ChangeState(std::move(mainMenuState));
                GN_LOG_INFO("Returned to MainMenuState");
            }
        }
        else if (strcmp(stateName, "ScreenPrompt") == 0) {
            // Handle screen prompt state finishing
            if (m_pendingLandscapeLevelId > 0) {
                // ScreenPrompt finished - entering landscape mode for boss level
                GN_LOG_INFO("ScreenPrompt finished - IMMEDIATELY locking to landscape orientation for boss level: " + std::to_string(m_pendingLandscapeLevelId));

                // LOCK ORIENTATION IMMEDIATELY BEFORE CREATING GAMEPLAY STATE to prevent rotation during transition
                if (m_platformDelegates.renderer.lockToLandscape) {
                    m_platformDelegates.renderer.lockToLandscape();
                    GN_LOG_INFO("Orientation locked to landscape before GameplayState creation");
                }

                auto gameplayState = std::make_unique<GameplayState>(m_ecsSystem.get(), &m_platformDelegates, m_pendingLandscapeLevelId);
                m_stateManager->ChangeState(std::move(gameplayState));
                m_pendingLandscapeLevelId = 0; // Clear the pending level
            } else if (!m_pendingTransitionTarget.empty()) {
                // ScreenPrompt finished - exiting landscape mode, go directly to target state
                GN_LOG_INFO("ScreenPrompt finished - going directly to: " + m_pendingTransitionTarget);

                if (m_pendingTransitionTarget == "MainMenu") {
                    // Go directly to MainMenuState - ScreenPromptState already provides transition
                    // Check if we should return to level select or main menu (Quickplay)
                    GN_LOG_INFO("Creating MainMenuState (from landscape) - Quickplay: " + std::string(m_enteredViaQuickplay ? "YES" : "NO"));
                    auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
                    
                    if (m_enteredViaQuickplay) {
                        // Quickplay mode - return to main menu
                        GN_LOG_INFO("Returning to MAIN MENU from Quickplay (landscape level)");
                        // Leave mode as MAIN_MENU (default)
                    } else {
                        // Normal level select - use last played level ID
                        if (m_lastPlayedLevelId > 0) {
                            mainMenuState->SetStartingMenuMode(MainMenuState::MenuMode::LEVEL_SELECT);
                            mainMenuState->SetReturnToLevel(m_lastPlayedLevelId);
                            GN_LOG_INFO("Returned to LEVEL SELECT from landscape level " + std::to_string(m_lastPlayedLevelId));
                        }
                    }
                    
                    // Reset Quickplay flag
                    m_enteredViaQuickplay = false;
                    
                    m_stateManager->ChangeState(std::move(mainMenuState));
                }

                m_pendingTransitionTarget.clear(); // Clear the pending target
            } else {
                // No pending state, return to main menu
                GN_LOG_WARN("ScreenPrompt finished but no pending state found");
                auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
                m_stateManager->ChangeState(std::move(mainMenuState));
            }
        }
        else if (strcmp(stateName, "Gameplay") == 0) {
            // Handle gameplay state transitions (game over, level complete, etc.)
            // Save game data when returning from gameplay to ensure coins are persisted
            GN_LOG_INFO("Saving game data before transitioning from Gameplay");
            SaveGameData();

            // Check if we're exiting from a landscape level (boss level)
            GameplayState* gameplay = dynamic_cast<GameplayState*>(finishedState);
            if (gameplay) {
                int levelId = gameplay->GetCurrentLevelId();
                LevelConfig levelConfig = LevelConfigFactory::GetLevelConfig(levelId);
                
                if (levelConfig.forceLandscape) {
                    // Boss level completed - transition to Credits (stay in landscape)
                    GN_LOG_INFO("Boss level " + std::to_string(levelId) + " completed - transitioning to Credits");
                    auto creditsState = std::make_unique<CreditsState>(m_ecsSystem.get(), &m_platformDelegates);
                    m_stateManager->ChangeState(std::move(creditsState));
                    return;
                }
            }

            // Normal portrait level - check if Quickplay or normal level select
            GN_LOG_INFO("Creating MainMenuState (from portrait level) - Quickplay: " + std::string(m_enteredViaQuickplay ? "YES" : "NO"));
            auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
            
            // Get level info from gameplay
            if (gameplay) {
                int levelId = gameplay->GetCurrentLevelId();
                
                if (m_enteredViaQuickplay) {
                    // Quickplay mode - return to main menu (don't set level select mode)
                    GN_LOG_INFO("Returning to MAIN MENU from Quickplay");
                    // Leave mode as MAIN_MENU (default)
                } else {
                    // Normal level select - return to level select screen with the level we came from
                    mainMenuState->SetStartingMenuMode(MainMenuState::MenuMode::LEVEL_SELECT);
                    mainMenuState->SetReturnToLevel(levelId);
                    GN_LOG_INFO("Returning to LEVEL SELECT - level " + std::to_string(levelId));
                }
            }
            
            // Reset Quickplay flag after handling
            m_enteredViaQuickplay = false;
            
            m_stateManager->ChangeState(std::move(mainMenuState));
            GN_LOG_INFO("Gameplay finished - transitioning to menu");
        }
        else if (strcmp(stateName, "Leaderboard") == 0) {
            // Return to main menu from leaderboard
            GN_LOG_INFO("Leaderboard finished - returning to main menu");
            auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
            m_stateManager->ChangeState(std::move(mainMenuState));
        }
        else if (strcmp(stateName, "Credits") == 0) {
            // Credits finished - show ScreenPromptState to rotate back to portrait
            GN_LOG_INFO("Credits finished - transitioning to ScreenPromptState for portrait rotation");
            
            // Unlock orientation so user can rotate
            if (m_platformDelegates.renderer.unlockOrientation) {
                m_platformDelegates.renderer.unlockOrientation();
                GN_LOG_INFO("Orientation unlocked - user can now rotate to portrait");
            }
            
            // Show ScreenPromptState waiting for portrait
            auto screenPrompt = std::make_unique<ScreenPromptState>(m_ecsSystem.get(), &m_platformDelegates, false); // false = wait for portrait
            m_pendingTransitionTarget = "MainMenu";
            m_stateManager->ChangeState(std::move(screenPrompt));
        }
        else if (strcmp(stateName, "Transition") == 0) {
            // Handle transition state finishing
            TransitionState* transition = dynamic_cast<TransitionState*>(finishedState);
            if (transition) {
                const char* targetState = transition->GetTargetStateName();
                GN_LOG_INFO("TransitionState finished - transitioning to: " + std::string(targetState));
                
                if (strcmp(targetState, "MainMenu") == 0) {
                    // Return to level select when transitioning from gameplay
                    GN_LOG_INFO("Creating MainMenuState with LEVEL_SELECT mode (from transition)");
                    auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get(), &m_platformDelegates);
                    mainMenuState->SetStartingMenuMode(MainMenuState::MenuMode::LEVEL_SELECT);
                    m_stateManager->ChangeState(std::move(mainMenuState));
                    GN_LOG_INFO("Transitioned to level select after orientation change");
                }
            }
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
        GN_LOG_INFO("📖 Loading settings...");
        
        // Queue the load settings command
        if (m_platformDelegates.save.loadSettings) {
            m_platformDelegates.save.loadSettings(&m_masterVolume, &m_musicVolume, &m_sfxVolume, &m_showDebugInfo);
            GN_LOG_INFO("📖 Settings load queued");
        }
        
        GN_LOG_INFO("🔊 Master: " + std::to_string(m_masterVolume) + 
                   ", Music: " + std::to_string(m_musicVolume) + 
                   ", SFX: " + std::to_string(m_sfxVolume));
    }

    void FloppyTurdGame::SaveSettings() {
        GN_LOG_INFO("💾 Saving settings...");
        
        // Save via platform delegates
        if (m_platformDelegates.save.saveSettings) {
            m_platformDelegates.save.saveSettings(m_masterVolume, m_musicVolume, m_sfxVolume, m_showDebugInfo);
            GN_LOG_INFO("✅ Settings save queued");
        }
    }

    void FloppyTurdGame::ResetGameData() {
        GN_LOG_INFO("🔄 ResetGameData called - resetting to defaults");
        m_highScore = 0;
        m_playerCoins = 0;
        m_gameStats = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0, 0};

        // Initialize level stats with default values
        for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
            LevelStats& stats = m_levelStats[levelId];
            stats.highScore = 0;
            stats.bestCoins = 0;
            stats.bestBossTime = 0.0f;
            stats.unlocked = (levelId == 1); // Only first level unlocked by default
            SetDefaultUnlockRequirements(levelId, stats);
        }

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
            return m_levelStats[levelId];
        }
        return defaultStats;
    }

    void FloppyTurdGame::UpdateLevelStats(int levelId, const LevelStats& stats) {
        if (levelId >= 1 && levelId <= MAX_LEVELS) {
            m_levelStats[levelId] = stats;
            
            // Only save if we're not currently loading game data
            if (!m_isLoadingGameData) {
                SaveGameData();
            }
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
            
            // Only save if we're not currently loading game data
            if (!m_isLoadingGameData) {
                SaveGameData();
            }

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

    void FloppyTurdGame::UpdateLevelHighScore(int levelId, int score, int coins, float bossTime) {
        if (levelId >= 1 && levelId <= MAX_LEVELS) {
            // Update existing stats
            bool newHighScore = (score > m_levelStats[levelId].highScore);
            if (newHighScore) {
                m_levelStats[levelId].highScore = score;
                GN_LOG_INFO("New high score for level " + std::to_string(levelId) + ": " + std::to_string(score));
                
                // Submit to Game Center if authenticated
                #ifdef PLATFORM_IOS
                if (m_platformDelegates.gameCenter.submitScore && m_platformDelegates.gameCenter.isAuthenticated) {
                    bool isAuthenticated = m_platformDelegates.gameCenter.isAuthenticated();
                    if (isAuthenticated) {
                        // Get the Game Center leaderboard ID for this level
                        std::string leaderboardID;
                        switch (levelId) {
                            case 1: leaderboardID = "com.floppyturd.level1.park"; break;
                            case 2: leaderboardID = "com.floppyturd.level2.sewer"; break;
                            case 3: leaderboardID = "com.floppyturd.level3.desert"; break;
                            case 4: leaderboardID = "com.floppyturd.level4.snow"; break;
                            case 5: leaderboardID = "com.floppyturd.level5.castle"; break;
                            case 6: leaderboardID = "com.floppyturd.level6.boss"; break;
                            default: break;
                        }
                        
                        if (!leaderboardID.empty()) {
                            GN_LOG_INFO("📊 Submitting score to Game Center: " + leaderboardID + " = " + std::to_string(score));
                            m_platformDelegates.gameCenter.submitScore(leaderboardID.c_str(), score, nullptr);
                        }
                    } else {
                        GN_LOG_INFO("📊 Not submitting to Game Center - not authenticated");
                    }
                }
                #endif
            }
            if (coins > m_levelStats[levelId].bestCoins) {
                m_levelStats[levelId].bestCoins = coins;
            }
            
            // Update boss time for level 6 (only if valid and better than previous)
            if (levelId == 6 && bossTime > 0.0f) {
                bool newBestTime = (m_levelStats[levelId].bestBossTime == 0.0f || bossTime < m_levelStats[levelId].bestBossTime);
                if (newBestTime) {
                    m_levelStats[levelId].bestBossTime = bossTime;
                    GN_LOG_INFO("New best boss time for level 6: " + std::to_string(bossTime) + " seconds");
                    
                    // Submit boss time to Game Center (convert to milliseconds for leaderboard)
                    #ifdef PLATFORM_IOS
                    if (m_platformDelegates.gameCenter.submitScore && m_platformDelegates.gameCenter.isAuthenticated) {
                        bool isAuthenticated = m_platformDelegates.gameCenter.isAuthenticated();
                        if (isAuthenticated) {
                            int64_t timeInMs = static_cast<int64_t>(bossTime * 1000.0f);
                            GN_LOG_INFO("📊 Submitting boss time to Game Center: " + std::to_string(timeInMs) + "ms");
                            m_platformDelegates.gameCenter.submitScore("com.floppyturd.level6.boss.time", timeInMs, nullptr);
                        }
                    }
                    #endif
                }
            }

            // Check if this level completion unlocks the next level
            CheckLevelUnlock(levelId, score, coins);

            // Only save if we're not currently loading game data
            if (!m_isLoadingGameData) {
                SaveGameData();
            }
        }
    }
    
    void FloppyTurdGame::IncrementSessionEnemyKills() {
        m_gameStats.sessionEnemiesKilled++;
        m_gameStats.totalEnemiesKilled++;
        GN_LOG_INFO("Enemy defeated! Session: " + std::to_string(m_gameStats.sessionEnemiesKilled) + 
                   ", Total: " + std::to_string(m_gameStats.totalEnemiesKilled));
    }
    
    void FloppyTurdGame::ResetSessionEnemyKills() {
        m_gameStats.sessionEnemiesKilled = 0;
        GN_LOG_INFO("Session enemy kills reset");
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
            case 2: // Sewer - 50 pipes from Park
                stats.unlockRequirement = 50; // pipes from Park (level 1)
                stats.coinRequirement = 0;    // no coins required
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
                stats.coinRequirement = 1000; // coins
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

        GN_LOG_INFO("📊 Level " + std::to_string(levelId) + " requirements: pipes=" + std::to_string(levelStats.unlockRequirement) + ", coins=" + std::to_string(levelStats.coinRequirement));

        // Check pipe requirement if it exists
        GN_LOG_INFO("🔍 Checking pipe requirement: unlockRequirement=" + std::to_string(levelStats.unlockRequirement) + " (level " + std::to_string(levelId) + ")");
        if (levelStats.unlockRequirement > 0) {
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
            GN_LOG_INFO("🎯 Checking pipes: need " + std::to_string(levelStats.unlockRequirement) + " from level " + std::to_string(requiredLevelId) + ", current high score: " + std::to_string(prevLevelHighScore));

            if (prevLevelHighScore < levelStats.unlockRequirement) {
                failureMessage = "Need " + std::to_string(levelStats.unlockRequirement) + " pipes from previous level";
                GN_LOG_INFO("❌ Pipe requirement not met: " + failureMessage);
                return false;
            }
            GN_LOG_INFO("✅ Pipe requirement met!");
        } else {
            GN_LOG_INFO("ℹ️ No pipe requirement for this level");
        }

        // Check coin requirement if it exists
        GN_LOG_INFO("🔍 Checking coin requirement: coinRequirement=" + std::to_string(levelStats.coinRequirement) + " (level " + std::to_string(levelId) + ")");
        if (levelStats.coinRequirement > 0) {
            GN_LOG_INFO("⚠️ Coin requirement check triggered for level " + std::to_string(levelId));
            GN_LOG_INFO("💰 Checking stored coins: need " + std::to_string(levelStats.coinRequirement) + ", current stored coins: " + std::to_string(m_gameStats.storedCoins));
            if (m_gameStats.storedCoins < levelStats.coinRequirement) {
                failureMessage = "Need " + std::to_string(levelStats.coinRequirement) + " coins";
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

        // Deduct coins if required
        if (levelStats.coinRequirement > 0) {
            // Spend stored coins for level unlocking
            m_gameStats.storedCoins -= levelStats.coinRequirement;
            GN_LOG_INFO("💰 Spent " + std::to_string(levelStats.coinRequirement) + " stored coins to unlock level " + std::to_string(levelId));
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

    // MARK: - Screen Info Update for Swift Interop

    void FloppyTurdGame::UpdateScreenInfo(const ScreenInfo& screenInfo) {
        // Update the singleton ConfigManager with new screen information
        auto& configManager = ConfigManager::Instance();
        configManager.SetScreenInfoDirect(screenInfo);

        GN_LOG_INFO("ConfigManager screen info updated from Swift: " +
                   std::to_string(screenInfo.pixelWidth) + "x" +
                   std::to_string(screenInfo.pixelHeight) + ", portrait: " +
                   (screenInfo.isPortrait ? "true" : "false"));
    }

    // MARK: - Customization Data Methods

    void FloppyTurdGame::UpdateCustomizationData(const CustomizationData& data) {
        m_customizationData = data;
        GN_LOG_INFO("Customization data updated - Equipped: " + std::to_string(data.equippedHatIndex) + 
                   ", Selected: " + std::to_string(data.selectedHatIndex));
        
        // Only save if we're not currently loading game data
        if (!m_isLoadingGameData) {
            SaveGameData();
        }
    }

    void FloppyTurdGame::SetEquippedHatIndex(int index) {
        m_customizationData.equippedHatIndex = index;
        GN_LOG_INFO("Equipped hat index set to: " + std::to_string(index));
        
        // Only save if we're not currently loading game data
        if (!m_isLoadingGameData) {
            SaveGameData();
        }
    }

    void FloppyTurdGame::SetSelectedHatIndex(int index) {
        m_customizationData.selectedHatIndex = index;
        GN_LOG_INFO("Selected hat index set to: " + std::to_string(index));
        
        // Only save if we're not currently loading game data
        if (!m_isLoadingGameData) {
            SaveGameData();
        }
    }

    bool FloppyTurdGame::IsHatUnlocked(int index) const {
        if (index < 0 || index >= static_cast<int>(m_customizationData.unlockedHats.size())) {
            return false;
        }
        return m_customizationData.unlockedHats[index];
    }

    void FloppyTurdGame::UnlockHat(int index) {
        if (index >= 0 && index < static_cast<int>(m_customizationData.unlockedHats.size())) {
            m_customizationData.unlockedHats[index] = true;
            GN_LOG_INFO("Hat unlocked at index: " + std::to_string(index));
            SaveGameData();
        }
    }

    bool FloppyTurdGame::IsSkillUnlocked(int skillIndex) const {
        if (skillIndex < 0 || skillIndex >= static_cast<int>(m_customizationData.unlockedSkills.size())) {
            return false;
        }
        return m_customizationData.unlockedSkills[skillIndex];
    }

    void FloppyTurdGame::UnlockSkill(int skillIndex) {
        if (skillIndex >= 0 && skillIndex < static_cast<int>(m_customizationData.unlockedSkills.size())) {
            m_customizationData.unlockedSkills[skillIndex] = true;
            GN_LOG_INFO("Skill unlocked at index: " + std::to_string(skillIndex));
            SaveGameData();
        }
    }

    // Global utility functions
    FloppyTurdGame* GetGame() {
        return g_Game;
    }

    void SetGame(FloppyTurdGame* game) {
        g_Game = game;
    }

} // namespace FloppyTurd