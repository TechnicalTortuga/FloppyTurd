#include "MainMenuState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Systems/LevelManager.h"
#include <cstdio>
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include "../../Engine/Utility/Utils.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include "../Input/InputManager.h"
#include <iostream>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
// Access shared systems via ECS SystemManager and RenderSystem APIs
#include "../../Engine/Core/SystemManager.h"
#include "../Systems/RenderSystem.h"

namespace GameCore {
    MainMenuState::MainMenuState(Gnosis::ECS* ecsCoordinator, PlatformDelegates* platformDelegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_platformDelegates(platformDelegates)
        , m_renderSystem(nullptr)
        , m_finished(false)
        , m_selectedOption(0)
        , m_animationTimer(0.0f)
        , m_isMobile(false)
        , m_currentMode(MenuMode::MAIN_MENU)
        , m_screenWidth(800.0f)
        , m_screenHeight(600.0f)
        , m_backgroundEntity(0)
        , m_logoEntity(0)
        , m_fButtonEntity(0)
        , m_playButtonEntity(0)
        , m_optionsButtonEntity(0)
        , m_quickPlayButtonEntity(0)
        , m_leaderboardButtonEntity(0)
        , m_currentLevelIndex(0)
        , m_selectedLevelIndex(-1)
        , m_backButtonEntity(0)
        , m_leftArrowButtonEntity(0)
        , m_rightArrowButtonEntity(0)
        , m_levelPlayButtonEntity(0)
        , m_isSwiping(false)
        , m_swipeThreshold(50.0f)
        , m_swipeAnimationTimer(0.0f)
        , m_swipeAnimationDuration(0.3f)
        , m_swipeDirection(SwipeDirection::NONE)
        , m_targetOffsetX(0.0f)
        , m_currentOffsetX(0.0f)
        , m_lastArrowPressTime(0.0f)
        , m_arrowDebounceDelay(0.3f)  // 300ms debounce delay
        , m_lastUnlockPressTime(0.0f)
        , m_unlockDebounceDelay(1.0f)  // 1 second debounce delay for unlock buttons
        , m_inputDebounceTimer(0.0f)
        , m_fartButtonDebounceTimer(0.0f)
        , m_lastMenuButtonPressTime(0.0f)
        , m_fontLoaded(false)
        , m_assetsLoaded(false) {
        
        // Cache RenderSystem reference at construction time (only once)
        if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager()) {
            m_renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
        }

        // Cache global game pointer once to avoid repeated extern lookups
        extern FloppyTurdGame* g_Game;
        m_game = g_Game;

        // Detect if we're on a mobile platform
        m_isMobile = IsMobilePlatform();

        // No local systems; use shared RenderSystem via ECS SystemManager throughout
        if (!m_platformDelegates) {
            GN_LOG_ERROR("MainMenuState: PlatformDelegates is null in constructor!");
        }
        
        // Initialize level data
        InitializeLevels();
    }

    MainMenuState::~MainMenuState() {
        // UI cleanup will be handled by platform-specific rendering system
    }

    void MainMenuState::Enter() {
        GN_LOG_INFO("Entering Main Menu State - currentMode = " + std::to_string(static_cast<int>(m_currentMode)));
        m_finished = false;
        
        // Reset input debounce timer to prevent accidental clicks from gameplay state
        m_inputDebounceTimer = INPUT_DEBOUNCE_DURATION;
        GN_LOG_INFO("Input debounce activated for " + std::to_string(INPUT_DEBOUNCE_DURATION) + " seconds");
        
        // Reset menu button debounce to prevent clicks when entering
        m_lastMenuButtonPressTime = 0.0f;
        
        m_selectedOption = 0;
        m_animationTimer = 0.0f;
        m_assetsLoaded = false;
        
        // Load vibration preference from game
        if (m_game) {
            m_vibrationsEnabled = m_game->GetVibrationsEnabled();
        }

        // Read current screen dimensions - ensure RenderSystem is up to date first
        if (m_renderSystem) {
            // Ensure RenderSystem has latest screen info from ConfigManager
            m_renderSystem->UpdateScreenInfo();

            ScreenInfo si = m_renderSystem->GetScreenInfo();
            m_screenWidth = static_cast<float>(si.pixelWidth);
            m_screenHeight = static_cast<float>(si.pixelHeight);
            GN_LOG_INFO("Main Menu - initial screen dimensions: " +
                       std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight) +
                       " (portrait: " + std::string(si.isPortrait ? "true" : "false") + ")");
        } else {
            // No render system available - this is a critical error
            GN_LOG_ERROR("Main Menu - no render system available! Cannot initialize UI without screen dimensions");
            m_screenWidth = 0.0f;
            m_screenHeight = 0.0f;
            return; // Don't initialize UI
        }

        // Lock to portrait mode when entering main menu
        if (m_platformDelegates && m_platformDelegates->renderer.lockToPortrait) {
            GN_LOG_INFO("Main Menu - locking orientation to portrait");
            m_platformDelegates->renderer.lockToPortrait();
        }

        // InputManager singleton should be initialized by FloppyTurdGame
        
        // Start playing main menu music only if it's not already playing
        if (m_game) {
            const std::string& currentTrack = m_game->GetCurrentMusicTrack();
            
            // Only start music if we're not already playing the main menu track
            if (currentTrack != "FloppyTurdMenu") {
                const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
                // Check if music is cached using new delegate
                if (delegates.asset.isCached) {
                    bool cached = delegates.asset.isCached("FloppyTurdMenu", 1); // 1 = audio type
                    GN_LOG_INFO("FloppyTurdMenu cached status: " + std::string(cached ? "true" : "false"));
                }
                // Play music using existing delegate
                if (delegates.audio.playMusic) {
                    delegates.audio.playMusic("FloppyTurdMenu", 0.7f, -1); // -1 = infinite loop
                    GN_LOG_INFO("Started main menu music: FloppyTurdMenu.mp3");
                }
            } else {
                GN_LOG_INFO("Main menu music already playing - not restarting");
            }
        }
        
        // Load the Whacky Joe font for text rendering
        if (m_game) {
            const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
            if (delegates.asset.loadFont) {
                // For now, just load the font without callback to test
                delegates.asset.loadFont("fonts/Whacky_Joe", 32, nullptr, nullptr);
                GN_LOG_INFO("Requested Whacky Joe font loading with path: fonts/Whacky_Joe");
                
                // Set font as loaded after a short delay to allow loading
                m_fontLoaded = true;
            } else {
                GN_LOG_INFO("❌ Font loading delegate not available");
            }
        } else {
            GN_LOG_INFO("❌ Game instance not available for font loading");
        }
        
        // Fetch screen dimensions (RenderSystem cached in constructor)
        if (m_renderSystem) {
            ScreenInfo si = m_renderSystem->GetScreenInfo();
            m_screenWidth = static_cast<float>(si.pixelWidth);
            m_screenHeight = static_cast<float>(si.pixelHeight);
            GN_LOG_INFO("MainMenuState: ScreenInfo (pixels) = " + std::to_string(si.pixelWidth) + "x" + std::to_string(si.pixelHeight));
        } else {
            GN_LOG_WARN("MainMenuState: Could not access RenderSystem; using default screen size " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
        }
        
        // Decide UI scale and create layout
        GN_LOG_INFO("Before layout - currentMode = " + std::to_string(static_cast<int>(m_currentMode)));
        m_uiScale = m_isMobile ? 8.0f : 1.0f;
        if (m_isMobile) CreateMobileLayout(); else CreateDesktopLayout();
        GN_LOG_INFO("After layout - currentMode = " + std::to_string(static_cast<int>(m_currentMode)));
        
        // Create UI elements (buttons with integrated text)
        CreateUIElements();
        GN_LOG_INFO("After CreateUIElements - currentMode = " + std::to_string(static_cast<int>(m_currentMode)));
        
        // Create level select layout (hidden initially)
        CreateLevelSelectLayout();
        GN_LOG_INFO("After CreateLevelSelectLayout - currentMode = " + std::to_string(static_cast<int>(m_currentMode)));

        m_assetsLoaded = true;
        m_uiInitialized = true;
        
        // Check if we should start in level select mode (e.g., returning from gameplay)
        GN_LOG_INFO("MainMenuState Enter() complete - currentMode = " + std::to_string(static_cast<int>(m_currentMode)) + " (0=MAIN_MENU, 1=LEVEL_SELECT, 2=OPTIONS)");
        if (m_currentMode == MenuMode::LEVEL_SELECT) {
            GN_LOG_INFO("✅ Starting in LEVEL_SELECT mode - calling ShowLevelSelect()");
            ShowLevelSelect();
            
            // If we should return to a specific level, navigate to it
            if (m_shouldReturnToSpecificLevel && m_returnToLevelNumber > 0) {
                // Find the level index for this level number
                for (size_t i = 0; i < m_levels.size(); i++) {
                    if (m_levels[i].levelNumber == m_returnToLevelNumber) {
                        m_currentLevelIndex = static_cast<int>(i);
                        GN_LOG_INFO("🎯 Returning to level " + std::to_string(m_returnToLevelNumber) + " (index " + std::to_string(i) + ")");
                        UpdateLevelVisibility();
                        break;
                    }
                }
                // Reset the flag
                m_shouldReturnToSpecificLevel = false;
                m_returnToLevelNumber = -1;
            }
        } else {
            GN_LOG_INFO("❌ Starting in MAIN_MENU mode - not showing level select");
        }
        
        GN_LOG_INFO("Main Menu State fully initialized");
    }

    void MainMenuState::Exit() {
        GN_LOG_INFO("Exiting Main Menu State");
        
        // Don't unlock orientation - keep portrait locked, GameplayState will handle its own orientation
        GN_LOG_INFO("Main Menu Exit - keeping portrait orientation locked (GameplayState will manage its own)");
        
        // Only stop music if NOT transitioning to leaderboard (leaderboard should keep main menu music playing)
        if (!m_transitioningToLeaderboard && m_game) {
            const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
            if (delegates.audio.stopMusic) {
                delegates.audio.stopMusic();
                GN_LOG_INFO("Stopped main menu music (not transitioning to leaderboard)");
            }
        } else if (m_transitioningToLeaderboard) {
            GN_LOG_INFO("NOT stopping music - transitioning to leaderboard (music should continue playing)");
        }
        
        // Cleanup UI entities
        if (m_ecsCoordinator && m_assetsLoaded) {
            if (m_backgroundEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_backgroundEntity);
            }
            if (m_logoEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_logoEntity);
            }
            if (m_fButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_fButtonEntity);
            }
            if (m_playButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_playButtonEntity);
            }
            if (m_optionsButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_optionsButtonEntity);
            }
            if (m_quickPlayButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_quickPlayButtonEntity);
            }
            if (m_leaderboardButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_leaderboardButtonEntity);
            }
            
            // Cleanup main menu UI - ad controls button and version text
            if (m_adControlsButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_adControlsButtonEntity);
                m_adControlsButtonEntity = 0;
            }
            if (m_versionTextEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_versionTextEntity);
                m_versionTextEntity = 0;
            }
            
            // Cleanup options menu entities
            if (m_optionsLeftArrowEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_optionsLeftArrowEntity);
                m_optionsLeftArrowEntity = 0;
            }
            if (m_optionsRightArrowEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_optionsRightArrowEntity);
                m_optionsRightArrowEntity = 0;
            }
            if (m_masterKnobEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_masterKnobEntity);
                m_masterKnobEntity = 0;
            }
            if (m_musicKnobEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_musicKnobEntity);
                m_musicKnobEntity = 0;
            }
            if (m_sfxKnobEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_sfxKnobEntity);
                m_sfxKnobEntity = 0;
            }
            if (m_optionsBackButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_optionsBackButtonEntity);
                m_optionsBackButtonEntity = 0;
            }
            if (m_masterTrackEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_masterTrackEntity);
                m_masterTrackEntity = 0;
            }
            if (m_musicTrackEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_musicTrackEntity);
                m_musicTrackEntity = 0;
            }
            if (m_sfxTrackEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_sfxTrackEntity);
                m_sfxTrackEntity = 0;
            }
            if (m_masterLabelEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_masterLabelEntity);
                m_masterLabelEntity = 0;
            }
            if (m_musicLabelEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_musicLabelEntity);
                m_musicLabelEntity = 0;
            }
            if (m_sfxLabelEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_sfxLabelEntity);
                m_sfxLabelEntity = 0;
            }
            if (m_optionsTitleEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_optionsTitleEntity);
                m_optionsTitleEntity = 0;
            }
            if (m_difficultyTextEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_difficultyTextEntity);
                m_difficultyTextEntity = 0;
            }
            if (m_difficultyValueEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_difficultyValueEntity);
                m_difficultyValueEntity = 0;
            }
            if (m_vibrationLabelEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_vibrationLabelEntity);
                m_vibrationLabelEntity = 0;
            }
            if (m_vibrationToggleEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_vibrationToggleEntity);
                m_vibrationToggleEntity = 0;
            }
            if (m_optionsOverlayEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_optionsOverlayEntity);
                m_optionsOverlayEntity = 0;
            }
            
            // Cleanup ad controls menu entities
            if (m_adControlsTitleEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_adControlsTitleEntity);
                m_adControlsTitleEntity = 0;
            }
            if (m_adControlsBackButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_adControlsBackButtonEntity);
                m_adControlsBackButtonEntity = 0;
            }
            if (m_removeAdsLabelEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_removeAdsLabelEntity);
                m_removeAdsLabelEntity = 0;
            }
            if (m_removeAdsPriceButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_removeAdsPriceButtonEntity);
                m_removeAdsPriceButtonEntity = 0;
            }
            if (m_adControlsOverlayEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_adControlsOverlayEntity);
                m_adControlsOverlayEntity = 0;
            }
            
            // Cleanup level select entities
            if (m_backButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_backButtonEntity);
            }
            if (m_levelPlayButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_levelPlayButtonEntity);
            }
            if (m_leftArrowButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_leftArrowButtonEntity);
            }
            if (m_rightArrowButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_rightArrowButtonEntity);
            }
            
            for (Gnosis::Entity entity : m_levelPaintingEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }
            
            for (Gnosis::Entity entity : m_levelFrameEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }
            
            for (Gnosis::Entity entity : m_levelTextEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }

            for (Gnosis::Entity entity : m_unlockButtonEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }

            for (Gnosis::Entity entity : m_requirementTextEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }

            if (m_lockedIndicatorEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_lockedIndicatorEntity);
            }
            
            GN_LOG_INFO("Cleaned up all main menu, options, ad controls, and level select UI entities");
        }
    }

    void MainMenuState::Pause() {
        // Main menu can be paused if needed
        GN_LOG_INFO("Main Menu State paused");
    }

    void MainMenuState::Resume() {
        // Resume main menu
        GN_LOG_INFO("Main Menu State resumed");
        
        // Set debounce timer to prevent accidental clicks when returning from other states
        m_lastMenuButtonPressTime = m_animationTimer;
        GN_LOG_INFO("Set menu button debounce timer on resume");
        
        // Reset transitioningToLeaderboard flag if we're returning from leaderboard
        if (m_transitioningToLeaderboard) {
            GN_LOG_INFO("Keeping main menu music playing (returned from leaderboard)");
            m_transitioningToLeaderboard = false; // Reset flag
        }

        // Refresh level display to update coin counts and unlock status after returning from gameplay
        if (m_currentMode == MenuMode::LEVEL_SELECT) {
            RefreshLevelDisplay();
            GN_LOG_INFO("🔄 Refreshed level display on resume - Level 2 should now show 0/0 requirements");
        }
    }

    void MainMenuState::Update(float deltaTime) {
        // Update input debounce timer
        if (m_inputDebounceTimer > 0.0f) {
            m_inputDebounceTimer -= deltaTime;
            if (m_inputDebounceTimer <= 0.0f) {
                m_inputDebounceTimer = 0.0f;
                GN_LOG_INFO("Input debounce expired - input now enabled");
            }
        }
        
        // Update fart button debounce timer
        if (m_fartButtonDebounceTimer > 0.0f) {
            m_fartButtonDebounceTimer -= deltaTime;
            if (m_fartButtonDebounceTimer < 0.0f) {
                m_fartButtonDebounceTimer = 0.0f;
            }
        }
        
        // Update InputManager singleton
        InputManager* inputManager = InputManager::GetInstance();
        if (inputManager) {
            inputManager->Update(deltaTime);
        }

        // SAFETY NET: Check if screen dimensions changed and recreate layout if needed
        // This catches cases where orientation changed after UI was created
        if (m_renderSystem) {
            ScreenInfo currentScreenInfo = m_renderSystem->GetScreenInfo();
            float currentWidth = static_cast<float>(currentScreenInfo.pixelWidth);
            float currentHeight = static_cast<float>(currentScreenInfo.pixelHeight);

            // Check if dimensions changed significantly (not just by a few pixels)
            // Also handle the initial update from Enter()'s default values
            bool dimensionsChanged = std::abs(currentWidth - m_screenWidth) > 10.0f ||
                                   std::abs(currentHeight - m_screenHeight) > 10.0f;

            if (dimensionsChanged || !m_uiInitialized) {
                if (dimensionsChanged) {
                    GN_LOG_INFO("🔄 Main Menu: Screen dimensions changed during runtime!");
                    GN_LOG_INFO("   Old: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));
                } else {
                    GN_LOG_INFO("📐 Main Menu: Reading final screen dimensions in Update()");
                }
                GN_LOG_INFO("   New: " + std::to_string((int)currentWidth) + "x" + std::to_string((int)currentHeight) +
                           " (portrait: " + std::string(currentScreenInfo.isPortrait ? "true" : "false") + ")");

                // Update stored dimensions
                m_screenWidth = currentWidth;
                m_screenHeight = currentHeight;

                // Recreate mobile layout
                if (m_isMobile) {
                    CreateMobileLayout();
                } else {
                    CreateDesktopLayout();
                }
                
                // Recreate UI elements with new layout
                CreateUIElements();
                
                // Recreate level select layout
                CreateLevelSelectLayout();
                
                GN_LOG_INFO("✅ Main Menu: UI recreated for new screen dimensions");
            }
        }

        m_animationTimer += deltaTime;
        
        // Update arrow button debounce timer
        m_lastArrowPressTime += deltaTime;

        // Update unlock button debounce timer
        m_lastUnlockPressTime += deltaTime;

        // Check for orientation changes and recreate layout if needed
        CheckForOrientationChange();

        // Update menu animations (logo bobbing, button highlights, etc.)
        UpdateMenuAnimations(deltaTime);
        
        // Handle menu selection changes
        UpdateMenuSelection();
        
        // Update swipe animation if in level select mode
        if (m_currentMode == MenuMode::LEVEL_SELECT) {
            AnimateSwipe(deltaTime);
            UpdatePanSnapAnimation(deltaTime);
        }
        
        // Update ECS systems
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Update(deltaTime);
        }
    }

    void MainMenuState::CheckForOrientationChange() {
        // Check if screen dimensions have changed (orientation change)
        if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager() && m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            auto* renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
            ScreenInfo currentScreenInfo = renderSystem->GetScreenInfo();
            float currentWidth = static_cast<float>(currentScreenInfo.pixelWidth);
            float currentHeight = static_cast<float>(currentScreenInfo.pixelHeight);

            // Check if screen dimensions have changed (orientation change)
            if (currentWidth != m_screenWidth || currentHeight != m_screenHeight) {
                GN_LOG_INFO("MainMenuState: Screen dimensions changed from " +
                           std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight) + " to " +
                           std::to_string((int)currentWidth) + "x" + std::to_string((int)currentHeight) +
                           " - recreating layout");

                // Update stored dimensions
                m_screenWidth = currentWidth;
                m_screenHeight = currentHeight;

                // Recreate the entire layout for the new orientation
                if (m_isMobile) {
                    CreateMobileLayout();
                } else {
                    CreateDesktopLayout();
                }

                // Recreate UI elements
                CreateUIElements();
            }
        }
    }

    void MainMenuState::Render() {
        // Don't render until all assets are loaded to prevent partial frame flashing
        if (!m_assetsLoaded || !m_uiInitialized) {
            return;
        }
        
        // Render through ECS system (sprites, buttons, text, etc.)
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Render();
        }
        
        // Debug rectangles removed for production build
        // if (m_game && m_game->GetPlatformDelegates().renderer.drawRectangle) {
        //     // Draw debug rectangles for each button
        //     DrawButtonDebugRectangles();
        //     
        //     // Draw level select debug info
        //     if (m_currentMode == MenuMode::LEVEL_SELECT) {
        //         DrawLevelSelectDebugInfo();
        //     }
        // }

        // Render Options overlay UI when active
        if (m_currentMode == MenuMode::OPTIONS) {
            RenderOptionsMenu();
        }
    }

    void MainMenuState::HandleInput() {
        // Block all input during debounce period
        if (m_inputDebounceTimer > 0.0f) {
            return;
        }
        
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }

        // Check if InputManager singleton is available
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) {
            GN_LOG_ERROR("🎮 MainMenuState: InputManager singleton is NULL!");
            return;
        }

        GN_LOG_INFO("🎮 MainMenuState: HandleInput() called - frame " + std::to_string(inputManager->GetCurrentFrameNumber()) +
                    " mode=" + std::to_string((int)m_currentMode));

        // Handle input based on current mode
        if (m_currentMode == MenuMode::MAIN_MENU) {
            HandleMainMenuInput();
        } else if (m_currentMode == MenuMode::LEVEL_SELECT) {
            HandleLevelSelectInput();
        } else if (m_currentMode == MenuMode::OPTIONS) {
            HandleOptionsInput();
        } else if (m_currentMode == MenuMode::AD_CONTROLS) {
            HandleAdControlsInput();
        }
    }

    void MainMenuState::ShowOptionsMenu() {
        m_currentMode = MenuMode::OPTIONS;
        GN_LOG_INFO("Options menu shown");
        
        // Sync vibration state from game
        if (m_game) {
            m_vibrationsEnabled = m_game->GetVibrationsEnabled();
            GN_LOG_INFO("Synced vibration state: " + std::string(m_vibrationsEnabled ? "ON" : "OFF"));
        }
        
        // Initialize cached overlay and slider geometry once, in pixels
        m_optionsOverlayX = m_screenWidth * 0.10f;
        m_optionsOverlayY = m_screenHeight * 0.10f; // 10% from top per request
        m_optionsOverlayW = m_screenWidth * 0.80f;
        m_optionsOverlayH = m_screenHeight * 0.80f;
        
        // Create overlay background (same as leaderboard and ad controls)
        if (m_optionsOverlayEntity == 0) {
            m_optionsOverlayEntity = m_ecsCoordinator->CreateEntity();
        }
        
        float overlayTextureWidth = 160.0f;
        float overlayTextureHeight = 300.0f;
        float overlayScale = 7.0f;
        
        Gnosis::GNVector2 overlayPosition(
            (m_screenWidth - overlayTextureWidth * overlayScale) * 0.5f,
            (m_screenHeight - overlayTextureHeight * overlayScale) * 0.5f
        );
        
        Transform overlayTransform(overlayPosition, 0.0f, Gnosis::GNVector2(overlayScale, overlayScale));
        
        if (!m_ecsCoordinator->HasComponent<Transform>(m_optionsOverlayEntity)) {
            m_ecsCoordinator->AddComponent<Transform>(m_optionsOverlayEntity, overlayTransform);
        } else {
            *m_ecsCoordinator->GetComponent<Transform>(m_optionsOverlayEntity) = overlayTransform;
        }
        
        Sprite overlaySprite("PauseMenuBackgroundMobile", (int)overlayTextureWidth, (int)overlayTextureHeight);
        overlaySprite.layer = 5;
        overlaySprite.visible = true;
        
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_optionsOverlayEntity)) {
            m_ecsCoordinator->AddComponent<Sprite>(m_optionsOverlayEntity, overlaySprite);
        } else {
            *m_ecsCoordinator->GetComponent<Sprite>(m_optionsOverlayEntity) = overlaySprite;
        }
        m_optionsSliderX = m_optionsOverlayX + 0.08f * m_optionsOverlayW;
        // Start first slider soon after title; use small fixed spacing from overlay top so rows are consistent
        // First slider block begins a fixed distance below the title to align rows
        m_optionsSliderY = m_screenHeight * 0.15f;
        m_optionsSliderW = m_optionsOverlayW - 0.16f * m_optionsOverlayW;
        // Use pixel dimensions (no arbitrary fractional multipliers)
        // Keep slider height modest and spacing consistent with rest of menu
        m_optionsSliderH = m_isMobile ? 20.0f : 10.0f;
        // Space per group: label (one line) + track + gap
        m_optionsSliderSpacing = m_isMobile ? 120.0f : 60.0f;
        // Note: Arrows will be positioned dynamically in CreateOptionsTracksAndLabels based on difficulty section
        // Hide main menu, show options elements
        SetMainMenuVisible(false);
        SetOptionsVisible(true);

        // Ensure tracks/labels and knobs exist and reset dragging
        CreateOptionsTracksAndLabels();
        m_draggingMaster = m_draggingMusic = m_draggingSFX = false;
        m_activeDragKnob = -1;  // Reset drag state
        CreateOptionsKnobs();
    }

    void MainMenuState::HideOptionsMenu() {
        GN_LOG_INFO("Options menu hidden - returning to main menu");
        m_lastMenuButtonPressTime = m_animationTimer; // Set debounce timer
        m_currentMode = MenuMode::MAIN_MENU;
        
        // Hide overlay background
        if (m_optionsOverlayEntity != 0) {
            auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_optionsOverlayEntity);
            if (sprite) sprite->visible = false;
        }
        
        // Hide options UI; show main menu buttons
        DestroyOptionsUI();
        SetMainMenuVisible(true);
    }

    void MainMenuState::SetMainMenuVisible(bool visible) {
        std::vector<Gnosis::Entity> mainButtons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_leaderboardButtonEntity};
        for (Gnosis::Entity entity : mainButtons) {
            if (entity != 0) {
                if (auto s = m_ecsCoordinator->GetComponent<Sprite>(entity)) s->visible = visible;
                if (auto ui = m_ecsCoordinator->GetComponent<UIElement>(entity)) ui->visible = visible;
            }
        }
        if (m_logoEntity != 0) {
            if (auto s = m_ecsCoordinator->GetComponent<Sprite>(m_logoEntity)) s->visible = visible;
        }
        if (m_fButtonEntity != 0) {
            if (auto s = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity)) s->visible = visible;
        }
        // Ad controls button and version text - ONLY show on main main menu
        if (m_adControlsButtonEntity != 0) {
            if (auto s = m_ecsCoordinator->GetComponent<Sprite>(m_adControlsButtonEntity)) {
                s->visible = visible && (m_currentMode == MenuMode::MAIN_MENU);
                GN_LOG_INFO("Ad controls button sprite visibility: " + std::string(s->visible ? "true" : "false"));
            }
            if (auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsButtonEntity)) {
                ui->visible = visible && (m_currentMode == MenuMode::MAIN_MENU);
                GN_LOG_INFO("Ad controls button UI visibility: " + std::string(ui->visible ? "true" : "false"));
            }
        }
        if (m_versionTextEntity != 0) {
            if (auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_versionTextEntity)) ui->visible = visible && (m_currentMode == MenuMode::MAIN_MENU);
        }
    }

    void MainMenuState::SetOptionsVisible(bool visible) {
        // ATOMIC RENDERING FIX: Collect all entities and components, then set visibility all at once
        // This prevents the "domino effect" where elements appear sequentially across frames
        std::vector<Gnosis::Entity> entitiesToUpdate;
        
        // Collect all option menu entities
        entitiesToUpdate.push_back(m_optionsLeftArrowEntity);
        entitiesToUpdate.push_back(m_optionsRightArrowEntity);
        entitiesToUpdate.push_back(m_masterKnobEntity);
        entitiesToUpdate.push_back(m_musicKnobEntity);
        entitiesToUpdate.push_back(m_sfxKnobEntity);
        entitiesToUpdate.push_back(m_optionsBackButtonEntity);
        entitiesToUpdate.push_back(m_masterTrackEntity);
        entitiesToUpdate.push_back(m_musicTrackEntity);
        entitiesToUpdate.push_back(m_sfxTrackEntity);
        entitiesToUpdate.push_back(m_optionsTitleEntity);
        entitiesToUpdate.push_back(m_difficultyTextEntity);
        entitiesToUpdate.push_back(m_difficultyValueEntity);
        entitiesToUpdate.push_back(m_masterLabelEntity);
        entitiesToUpdate.push_back(m_musicLabelEntity);
        entitiesToUpdate.push_back(m_sfxLabelEntity);
        entitiesToUpdate.push_back(m_vibrationLabelEntity);
        entitiesToUpdate.push_back(m_vibrationToggleEntity);
        
        // Now atomically set ALL entities to the same visibility at once
        for (Gnosis::Entity e : entitiesToUpdate) {
            if (e == 0) continue;
            if (auto s = m_ecsCoordinator->GetComponent<Sprite>(e)) s->visible = visible;
            if (auto ui = m_ecsCoordinator->GetComponent<UIElement>(e)) ui->visible = visible;
            if (auto shape = m_ecsCoordinator->GetComponent<UIShape>(e)) shape->visible = visible;
        }
    }

    void MainMenuState::CreateOptionsArrows(float overlayX, float overlayY, float overlayW, float overlayH, float diffY) {
        if (!m_ecsCoordinator) return;
        float scale = m_isMobile ? 10.0f : 5.0f;
        
        GN_LOG_INFO("DEBUG: CreateOptionsArrows - screenWidth=" + std::to_string(m_screenWidth) + ", screenHeight=" + std::to_string(m_screenHeight));
        
        // Left arrow
        if (m_optionsLeftArrowEntity == 0)
            m_optionsLeftArrowEntity = m_ecsCoordinator->CreateEntity();
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("LeftArrow");
        }
        int lwi = 0, lhi = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            if (!rs->GetTextureSize("LeftArrow", lwi, lhi)) { lwi = 16; lhi = 16; }
        }
        float lw = static_cast<float>(lwi);
        float lh = static_cast<float>(lhi);
        // Symmetric margin from screen edges
        float edgeMargin = m_screenWidth * 0.05f;  // 5% from each edge
        float leftX = edgeMargin;                   // left button starts at left margin
        float leftY = diffY - (lh * scale) * 0.5f; // center arrow vertically on baseline
        Transform lt(Gnosis::GNVector2(leftX, leftY), 0.0f, Gnosis::GNVector2(scale, scale));
        Sprite ls("LeftArrow", lw, lh); ls.layer = 20; ls.visible = true;
        UIElement le("", "LeftArrow", "LeftArrowHover");
        GN_LOG_INFO("DEBUG: Left arrow - edgeMargin=" + std::to_string(edgeMargin) + 
                    ", scaledWidth=" + std::to_string(lw * scale) + 
                    ", finalLeftX=" + std::to_string(leftX) + 
                    ", finalLeftY=" + std::to_string(leftY));
        if (!m_ecsCoordinator->HasComponent<Transform>(m_optionsLeftArrowEntity)) m_ecsCoordinator->AddComponent<Transform>(m_optionsLeftArrowEntity, lt); else *m_ecsCoordinator->GetComponent<Transform>(m_optionsLeftArrowEntity) = lt;
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_optionsLeftArrowEntity)) m_ecsCoordinator->AddComponent<Sprite>(m_optionsLeftArrowEntity, ls); else *m_ecsCoordinator->GetComponent<Sprite>(m_optionsLeftArrowEntity) = ls;
        if (!m_ecsCoordinator->HasComponent<UIElement>(m_optionsLeftArrowEntity)) m_ecsCoordinator->AddComponent<UIElement>(m_optionsLeftArrowEntity, le); else *m_ecsCoordinator->GetComponent<UIElement>(m_optionsLeftArrowEntity) = le;
        
        // Right arrow
        if (m_optionsRightArrowEntity == 0)
            m_optionsRightArrowEntity = m_ecsCoordinator->CreateEntity();
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("RightArrow");
        }
        int rwi = 0, rhi = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            if (!rs->GetTextureSize("RightArrow", rwi, rhi)) { rwi = 16; rhi = 16; }
        }
        float rw = static_cast<float>(rwi);
        float rh = static_cast<float>(rhi);
        
        // Right arrow positioned so its RIGHT EDGE is at (screenWidth - edgeMargin)
        float scaledRW = rw * scale;
        float rightX = m_screenWidth - edgeMargin - scaledRW; // place left coordinate so right edge matches margin
        float rightY = diffY - (rh * scale) * 0.5f;           // center arrow vertically on baseline
        Transform rt(Gnosis::GNVector2(rightX, rightY), 0.0f, Gnosis::GNVector2(scale, scale));
        Sprite rs("RightArrow", rw, rh); rs.layer = 20; rs.visible = true;
        UIElement re("", "RightArrow", "RightArrowHover");
        
        GN_LOG_INFO("DEBUG: Right arrow - edgeMargin=" + std::to_string(edgeMargin) +
                    ", scaledWidth=" + std::to_string(scaledRW) + ", finalRightX=" + std::to_string(rightX) +
                    ", finalRightY=" + std::to_string(rightY));
        
        if (!m_ecsCoordinator->HasComponent<Transform>(m_optionsRightArrowEntity)) m_ecsCoordinator->AddComponent<Transform>(m_optionsRightArrowEntity, rt); else *m_ecsCoordinator->GetComponent<Transform>(m_optionsRightArrowEntity) = rt;
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_optionsRightArrowEntity)) m_ecsCoordinator->AddComponent<Sprite>(m_optionsRightArrowEntity, rs); else *m_ecsCoordinator->GetComponent<Sprite>(m_optionsRightArrowEntity) = rs;
        if (!m_ecsCoordinator->HasComponent<UIElement>(m_optionsRightArrowEntity)) m_ecsCoordinator->AddComponent<UIElement>(m_optionsRightArrowEntity, re); else *m_ecsCoordinator->GetComponent<UIElement>(m_optionsRightArrowEntity) = re;
    }

    void MainMenuState::DestroyOptionsUI() {
        // Convert to hide-only to avoid dynamic create/destroy during runtime
        auto hideEntity = [&](Gnosis::Entity e){
            if (e == 0) return;
            if (auto s = m_ecsCoordinator->GetComponent<Sprite>(e)) s->visible = false;
            if (auto ui = m_ecsCoordinator->GetComponent<UIElement>(e)) ui->visible = false;
            if (auto shape = m_ecsCoordinator->GetComponent<UIShape>(e)) shape->visible = false;
        };
        hideEntity(m_optionsLeftArrowEntity);
        hideEntity(m_optionsRightArrowEntity);
        hideEntity(m_masterKnobEntity);
        hideEntity(m_musicKnobEntity);
        hideEntity(m_sfxKnobEntity);
        hideEntity(m_optionsBackButtonEntity);
        hideEntity(m_masterTrackEntity);
        hideEntity(m_musicTrackEntity);
        hideEntity(m_sfxTrackEntity);
        hideEntity(m_optionsTitleEntity);
        hideEntity(m_difficultyTextEntity);
        hideEntity(m_difficultyValueEntity);
        hideEntity(m_masterLabelEntity);
        hideEntity(m_musicLabelEntity);
        hideEntity(m_sfxLabelEntity);
        hideEntity(m_vibrationLabelEntity);
        hideEntity(m_vibrationToggleEntity);
    }

    void MainMenuState::CreateOptionsKnobs() {
        if (!m_ecsCoordinator) return;
        // Use poophat texture as knob; base size 16x16 px scaled by global ui scale ONLY
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("poophat");
        }
        int kwi = 0, khi = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            if (!rs->GetTextureSize("poophat", kwi, khi)) { kwi = 16; khi = 16; }
        }
        float kw = static_cast<float>(kwi);
        float kh = static_cast<float>(khi);
        float scale = m_uiScale;

        auto createKnob = [&](int index, Gnosis::Entity& outEntity) {
            if (outEntity == 0) outEntity = m_ecsCoordinator->CreateEntity();
            Transform t(Gnosis::GNVector2(m_optionsSliderX, m_optionsSliderY + index * m_optionsSliderSpacing - (kh * scale - m_optionsSliderH) * 0.5f), 0.0f, Gnosis::GNVector2(scale, scale));
            Sprite s("poophat", kw, kh); s.layer = 21; s.visible = true;
            UIElement ui("", "poophat", "poophat");
            if (!m_ecsCoordinator->HasComponent<Transform>(outEntity)) m_ecsCoordinator->AddComponent<Transform>(outEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(outEntity) = t;
            if (!m_ecsCoordinator->HasComponent<Sprite>(outEntity)) m_ecsCoordinator->AddComponent<Sprite>(outEntity, s); else *m_ecsCoordinator->GetComponent<Sprite>(outEntity) = s;
            if (!m_ecsCoordinator->HasComponent<UIElement>(outEntity)) m_ecsCoordinator->AddComponent<UIElement>(outEntity, ui); else *m_ecsCoordinator->GetComponent<UIElement>(outEntity) = ui;
        };

        createKnob(0, m_masterKnobEntity);
        createKnob(1, m_musicKnobEntity);
        createKnob(2, m_sfxKnobEntity);
        UpdateOptionsKnobPositions();

        // Create back button on FloppyButtonBlue if not existing
        if (m_optionsBackButtonEntity == 0) {
            m_optionsBackButtonEntity = m_ecsCoordinator->CreateEntity();
            if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) { rs->PreloadTexture("FloppyButtonBlue"); }
            int twi = 0, thi = 0;
            if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                if (!rs->GetTextureSize("FloppyButtonBlue", twi, thi)) { twi = 90; thi = 16; }
            }
            float tw = static_cast<float>(twi);
            float th = static_cast<float>(thi);
            float buttonScale = (m_isMobile ? 10.0f : 5.0f); // keep standard button size
            auto scaled = GetScaledDimensions(tw, th, buttonScale);
            float backW = scaled.first;
            float backH = scaled.second;
            float backX = m_optionsOverlayX + m_optionsOverlayW * 0.5f - backW * 0.5f;
            // Move back button further down near the bottom of the options overlay
            // Lower back button further, consistent with level select layout
            float backY = m_optionsOverlayY + m_optionsOverlayH - backH - 4.0f * m_uiScale;
            Transform t(Gnosis::GNVector2(backX, backY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            Sprite s("FloppyButtonBlue", tw, th); s.layer = 22; s.visible = true;
            UIElement ui("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
            ui.fontSize = m_isMobile ? 80.0f : 32.0f; // Increased font size
            ui.textColor = Gnosis::GNColor(255,255,255,255);
            ui.centerTextHorizontally = true;
            ui.centerTextVertically = true;
            ui.visible = true;
            m_ecsCoordinator->AddComponent<Transform>(m_optionsBackButtonEntity, t);
            m_ecsCoordinator->AddComponent<Sprite>(m_optionsBackButtonEntity, s);
            m_ecsCoordinator->AddComponent<UIElement>(m_optionsBackButtonEntity, ui);
        } else {
            // ensure visibility when options open
            if (auto s = m_ecsCoordinator->GetComponent<Sprite>(m_optionsBackButtonEntity)) s->visible = true;
            if (auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_optionsBackButtonEntity)) ui->visible = true;
        }
    }

    void MainMenuState::UpdateOptionsKnobPositions() {
        if (!m_ecsCoordinator) return;
        
        // Use same spacing values as CreateOptionsTracksAndLabels
        float currentY = m_screenHeight * 0.18f;
        float labelTrackGap = 15.0f * m_uiScale;
        float sectionGap = 25.0f * m_uiScale;
        
        auto setKnobPos = [&](Gnosis::Entity knobEntity, float pct, float trackY) {
            if (knobEntity == 0) return;
            Transform* t = m_ecsCoordinator->GetComponent<Transform>(knobEntity);
            Sprite* s = m_ecsCoordinator->GetComponent<Sprite>(knobEntity);
            if (!t || !s) return;
            float x = m_optionsSliderX + pct * m_optionsSliderW - (s->width * t->scale.x) * 0.5f;
            float y = trackY + m_optionsSliderH * 0.5f - (s->height * t->scale.y) * 0.5f;  // Center on track
            t->position.x = x;
            t->position.y = y;
        };
        
        // Calculate Y positions for each track using same logic as CreateOptionsTracksAndLabels
        float masterTrackY = currentY + labelTrackGap;
        currentY = masterTrackY + m_optionsSliderH + sectionGap;
        
        float musicTrackY = currentY + labelTrackGap;
        currentY = musicTrackY + m_optionsSliderH + sectionGap;
        
        float sfxTrackY = currentY + labelTrackGap;

        float master = GameCore::GetGame()->GetMasterVolume();
        float music = GameCore::GetGame()->GetMusicVolume();
        float sfx = GameCore::GetGame()->GetSFXVolume();
        
        setKnobPos(m_masterKnobEntity, master, masterTrackY);
        setKnobPos(m_musicKnobEntity, music, musicTrackY);
        setKnobPos(m_sfxKnobEntity, sfx, sfxTrackY);
    }
    
    void MainMenuState::HandleMainMenuInput() {
        // Get InputManager singleton
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) {
            GN_LOG_ERROR("🎮 MainMenuState: InputManager singleton is NULL!");
            return;
        }

        // Get active touches from InputManager
        auto touches = inputManager->GetActiveTouches();
        GN_LOG_INFO("🎮 MainMenuState: Processing " + std::to_string(touches.size()) + " touches");

        for (const auto& touch : touches) {
            GN_LOG_INFO("🎯 MainMenuState: Touch " + std::to_string(touch.touchId) + 
                        " state=" + std::to_string((int)touch.state) + 
                        " norm(" + std::to_string(touch.x) + ", " + std::to_string(touch.y) + 
                        ") pixel(" + std::to_string(touch.rawX) + ", " + std::to_string(touch.rawY) + ")");

            if (touch.state == TouchState::PRESSED || touch.state == TouchState::RELEASED) {
                // TouchData.rawX/rawY are already in pixel coordinates
                float pixelX = touch.rawX;
                float pixelY = touch.rawY;

                std::string stateStr = (touch.state == TouchState::PRESSED) ? "PRESSED" : "RELEASED";
                GN_LOG_INFO("🎮 MainMenuState: Processing " + stateStr + " touch at pixel(" +
                           std::to_string(pixelX) + ", " + std::to_string(pixelY) + ")");

                // Check if touch is within F button bounds
                if (m_fButtonEntity != 0) {
                    Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
                    Sprite* fButtonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);

                    if (fButtonTransform && fButtonSprite) {
                        // F button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = fButtonSprite->width * fButtonTransform->scale.x;
                        float buttonHeight = fButtonSprite->height * fButtonTransform->scale.y;
                        float buttonLeft = fButtonTransform->position.x;
                        float buttonRight = fButtonTransform->position.x + buttonWidth;
                        float buttonTop = fButtonTransform->position.y;
                        float buttonBottom = fButtonTransform->position.y + buttonHeight;

                        // Get current screen dimensions for logging
                        float currentScreenHeight = m_screenHeight;
                        if (auto* renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                            ScreenInfo screenInfo = renderSystem->GetScreenInfo();
                            currentScreenHeight = static_cast<float>(screenInfo.pixelHeight);
                        }

                        bool xInBounds = pixelX >= buttonLeft && pixelX <= buttonRight;
                        bool yInBounds = pixelY >= buttonTop && pixelY <= buttonBottom;

                        GN_LOG_INFO("🎯 F Button bounds check: xInBounds=" + std::string(xInBounds ? "true" : "false") +
                                   " yInBounds=" + std::string(yInBounds ? "true" : "false"));

                        if (xInBounds && yInBounds) {
                            if (touch.state == TouchState::PRESSED) {
                                GN_LOG_INFO("🎉 MainMenuState: F BUTTON PRESSED - setting visual state");
                                // Just set visual state on press, action on release
                                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_fButtonEntity);
                                if (uiElement) {
                                    uiElement->isPressed = true;
                                    uiElement->isHovered = true;
                                    UpdateButtonSprite(m_fButtonEntity, *uiElement);
                                }
                            } else if (touch.state == TouchState::RELEASED) {
                                // Check debounce timer before allowing fart
                                if (m_fartButtonDebounceTimer <= 0.0f) {
                                    GN_LOG_INFO("🎉 MainMenuState: F BUTTON RELEASED - playing fart sound!");
                                    OnFButtonPressed();
                                    m_fartButtonDebounceTimer = FART_BUTTON_DEBOUNCE; // Reset debounce timer
                                } else {
                                    GN_LOG_INFO("⏱️ MainMenuState: F BUTTON DEBOUNCED - " + std::to_string(m_fartButtonDebounceTimer) + "s remaining");
                                }
                                // Reset visual state regardless
                                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_fButtonEntity);
                                if (uiElement) {
                                    uiElement->isPressed = false;
                                    uiElement->isHovered = false;
                                    UpdateButtonSprite(m_fButtonEntity, *uiElement);
                                }
                            }
                        } else {
                            GN_LOG_INFO("❌ MainMenuState: Touch missed F button");
                            // Only check menu buttons if touch is released (to avoid triggering on press)
                            if (touch.state == TouchState::RELEASED) {
                                CheckMenuButtonClicks(pixelX, pixelY);
                            }
                        }
                    }
                }
            }
        }

        // TODO: Handle menu navigation (up/down arrows) for desktop
        // TODO: Handle selection (enter/space) for menu options
    }

    static void DrawLabeledSlider(const PlatformDelegates& delegates,
                                  const char* label,
                                  float x, float y,
                                  float width, float height,
                                  float value01,
                                  float* outHandleLeft,
                                  float* outHandleRight) {
        // Background bar
        if (delegates.renderer.drawRectangle) {
            delegates.renderer.drawRectangle(x, y, width, height, 0.2f, 0.2f, 0.2f, 0.8f);
            // Fill amount
            float fillWidth = width * value01;
            delegates.renderer.drawRectangle(x, y, fillWidth, height, 0.1f, 0.6f, 0.2f, 0.9f);
        }
        // Label and value
        if (delegates.renderer.drawText) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%s: %d%%", label, (int)(value01 * 100.0f));
            delegates.renderer.drawText(buffer, x, y - 28.0f, 24.0f, 1, 1, 1, 1);
        }
        if (outHandleLeft) *outHandleLeft = x;
        if (outHandleRight) *outHandleRight = x + width;
    }

    void MainMenuState::RenderOptionsMenu() {
        // Draw overlay and slider tracks using rectangles to ensure consistent look
        const auto& renderer = m_platformDelegates->renderer;
        if (!renderer.drawRectangle) return;

        // Dimmed background overlay
        renderer.drawRectangle(m_optionsOverlayX, m_optionsOverlayY,
                               m_optionsOverlayW, m_optionsOverlayH,
                               0.05f, 0.05f, 0.10f, 0.85f);

        // Slider tracks
        const float trackR = 0.10f, trackG = 0.10f, trackB = 0.30f, trackA = 0.95f;
        for (int i = 0; i < 3; ++i) {
            float y = m_optionsSliderY + i * m_optionsSliderSpacing;
            renderer.drawRectangle(m_optionsSliderX, y, m_optionsSliderW, m_optionsSliderH,
                                   trackR, trackG, trackB, trackA);
        }
    }

    // Position all level paintings horizontally according to current pan offset, allowing partial on-screen visibility.
    void MainMenuState::UpdateLevelPanPositions() {
        float centerX = m_screenWidth / 2.0f;
        float centerY = m_screenHeight / 2.0f;
        // Use same dynamic scaling as UpdateLevelVisibility for each painting
        for (size_t i = 0; i < m_levelPaintingEntities.size(); ++i) {
            if (m_levelPaintingEntities[i] == 0) continue;
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPaintingEntities[i]);
            if (!transform || !sprite || i >= m_levels.size()) continue;
            int pwi = 0, phi = 0;
            if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                rs->PreloadTexture(m_levels[i].paintingTexture);
                if (!rs->GetTextureSize(m_levels[i].paintingTexture, pwi, phi)) { pwi = 96; phi = 96; }
            }
            float paintingTextureWidth = static_cast<float>(pwi);
            float paintingTextureHeight = static_cast<float>(phi);
            float maxWidth = m_screenWidth * 0.8f;
            float maxHeight = m_screenHeight * 0.4f;
            float scaleByWidth = maxWidth / paintingTextureWidth;
            float scaleByHeight = maxHeight / paintingTextureHeight;
            float dynamicScale = std::min(scaleByWidth, scaleByHeight);
            auto scaledDimensions = GetScaledDimensions(paintingTextureWidth, paintingTextureHeight, dynamicScale);
            float paintingWidth = scaledDimensions.first;
            float paintingHeight = scaledDimensions.second;

            // Compute target center for each index relative to current level index
            float indexDelta = static_cast<float>(static_cast<int>(i) - m_currentLevelIndex);
            float targetCenterX = centerX + indexDelta * m_levelSpacing + m_currentOffsetX;
            float topLeftX = targetCenterX - (paintingWidth * 0.5f);
            float topLeftY = centerY - (paintingHeight * 0.5f);

            transform->position.x = topLeftX;
            transform->position.y = topLeftY;
            transform->scale.x = dynamicScale;
            transform->scale.y = dynamicScale;
            sprite->textureId = m_levels[i].paintingTexture;

            // Ensure neighbor paintings are visible while panning/snapping and within a small off-screen buffer
            if (m_isPanning || m_isSnapping) {
                // Visible if any part of the painting is within a 20% screen-width buffer on either side
                bool withinBufferedView = (topLeftX < m_screenWidth * 1.2f) && ((topLeftX + paintingWidth) > -m_screenWidth * 0.2f);
                sprite->visible = withinBufferedView;
            }

            // Frames follow painting
            if (i < m_levelFrameEntities.size() && m_levelFrameEntities[i] != 0) {
                if (auto frameT = m_ecsCoordinator->GetComponent<Transform>(m_levelFrameEntities[i])) {
                    if (auto frameS = m_ecsCoordinator->GetComponent<Sprite>(m_levelFrameEntities[i])) {
                        frameT->position = transform->position;
                        frameT->scale = transform->scale;
                        // Mirror painting visibility and only show frame when locked
                        frameS->visible = sprite->visible && !m_levels[i].isUnlocked;
                    }
                }
            }
            // Text stays centered above the current/target level
            if (i < m_levelTextEntities.size() && m_levelTextEntities[i] != 0) {
                if (auto textT = m_ecsCoordinator->GetComponent<Transform>(m_levelTextEntities[i])) {
                    if (auto textUI = m_ecsCoordinator->GetComponent<UIElement>(m_levelTextEntities[i])) {
                        textT->position.x = centerX;
                        textT->position.y = m_screenHeight * 0.15f;
                        // During active pan, show neighbor titles when substantially on screen to aid snap preview
                        if (m_isPanning) {
                            float visibilityThresholdLeft = 0.1f * m_screenWidth;
                            float visibilityThresholdRight = 0.9f * m_screenWidth;
                            float centerLeft = topLeftX + paintingWidth * 0.5f;
                            bool largelyOnScreen = centerLeft >= visibilityThresholdLeft && centerLeft <= visibilityThresholdRight;
                            textUI->visible = largelyOnScreen;
                            if (i < m_levels.size()) {
                                textUI->buttonText = m_levels[i].name;
                            }
                        } else if (m_isSnapping) {
                            // While snapping, lock the title to the decided target level to prevent flicker
                            int targetIndex = std::max(0, std::min(m_currentLevelIndex + m_pendingIndexDelta, (int)m_levels.size() - 1));
                            textUI->visible = (i == static_cast<size_t>(targetIndex));
                            if (i == static_cast<size_t>(targetIndex) && i < m_levels.size()) {
                                textUI->buttonText = m_levels[i].name;
                            }
                        } else {
                            // Idle (not panning or snapping): only the current level's title
                            textUI->visible = (i == static_cast<size_t>(m_currentLevelIndex));
                            if (i == static_cast<size_t>(m_currentLevelIndex) && i < m_levels.size()) {
                                textUI->buttonText = m_levels[i].name;
                            }
                        }
                    }
                }
            }
        }
    }

    // Smoothly animate snapping to the target index after release, including seeing neighbor during slide-in
    void MainMenuState::UpdatePanSnapAnimation(float deltaTime) {
        if (!m_isSnapping) return;
        m_snapElapsed += deltaTime;
        float t = std::min(m_snapElapsed / std::max(m_snapDuration, 0.0001f), 1.0f);
        // Ease-out cubic
        float eased = 1.0f - std::pow(1.0f - t, 3.0f);
        float targetOffset = m_pendingIndexDelta * -m_levelSpacing; // slide content opposite swipe to bring new center in
        m_currentOffsetX = m_snapStartOffsetX + (targetOffset - m_snapStartOffsetX) * eased;
        UpdateLevelPanPositions();
        if (t >= 1.0f) {
            // Commit index change
            m_currentLevelIndex = std::max(0, std::min(m_currentLevelIndex + m_pendingIndexDelta, (int)m_levels.size() - 1));
            m_pendingIndexDelta = 0;
            m_isSnapping = false;
            m_currentOffsetX = 0.0f;
            UpdateLevelVisibility();
        }
    }

    void MainMenuState::HandleOptionsInput() {
        if (!m_platformDelegates) return;

        // Read input state every frame; do not early-return on non-justPressed so dragging works
        bool justPressed = m_platformDelegates->input.isPrimaryInputJustPressed ? m_platformDelegates->input.isPrimaryInputJustPressed() : false;
        bool inputDown   = m_platformDelegates->input.isPrimaryInputDown        ? m_platformDelegates->input.isPrimaryInputDown()        : false;
        bool justReleased= m_platformDelegates->input.isPrimaryInputJustReleased? m_platformDelegates->input.isPrimaryInputJustReleased(): false;
        float touchX = 0.0f, touchY = 0.0f;
        if (m_platformDelegates->input.getPrimaryInputPosition) {
            m_platformDelegates->input.getPrimaryInputPosition(&touchX, &touchY);
        }

        auto clamp01 = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
        
        // Calculate track positions using same logic as UpdateOptionsKnobPositions  
        float currentY = m_screenHeight * 0.18f;
        float labelTrackGap = 15.0f * m_uiScale;
        float sectionGap = 25.0f * m_uiScale;
        
        float masterTrackY = currentY + labelTrackGap;
        currentY = masterTrackY + m_optionsSliderH + sectionGap;
        float musicTrackY = currentY + labelTrackGap;
        currentY = musicTrackY + m_optionsSliderH + sectionGap;
        float sfxTrackY = currentY + labelTrackGap;
        
        // Precise hitboxes - ONLY where the actual track + knob images are
        auto applySliderAtTrackY = [&](int index, float& valueRef, bool& draggingFlag, Gnosis::Entity knobEntity, float trackY) {
            // Precise hitbox - only the track area + small margin for knob
            float knobSize = 16.0f * m_uiScale;  // Knob is 16px scaled
            float hitTop = trackY - knobSize * 0.5f;  // Account for knob height above track
            float hitBottom = trackY + m_optionsSliderH + knobSize * 0.5f;  // Account for knob height below track
            float hitLeft = m_optionsSliderX - knobSize * 0.5f;  // Account for knob width left of track
            float hitRight = m_optionsSliderX + m_optionsSliderW + knobSize * 0.5f;  // Account for knob width right of track
            
            // Debug hitbox info
            if (justPressed) {
                GN_LOG_INFO("DEBUG: Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ") - Knob " + std::to_string(index) + " hitbox: (" + std::to_string(hitLeft) + "-" + std::to_string(hitRight) + ", " + std::to_string(hitTop) + "-" + std::to_string(hitBottom) + ")");
            }
            
            // State: BEGAN - Initialize drag operation (snap knob to touch position immediately)
            if (justPressed && touchY >= hitTop && touchY <= hitBottom && touchX >= hitLeft && touchX <= hitRight) {
                if (m_activeDragKnob == -1) {  // Only start if no active drag (single-touch)
                    m_activeDragKnob = index;
                    draggingFlag = true;
                    // Absolute mapping: set value directly from touch X within slider bounds
                    float pressedValue = clamp01((touchX - m_optionsSliderX) / m_optionsSliderW);
                    valueRef = pressedValue;
                    // Immediate visual feedback at press
                    if (knobEntity != 0) {
                        if (auto t = m_ecsCoordinator->GetComponent<Transform>(knobEntity)) {
                            if (auto s = m_ecsCoordinator->GetComponent<Sprite>(knobEntity)) {
                                float kx = m_optionsSliderX + pressedValue * m_optionsSliderW - (s->width * t->scale.x) * 0.5f;
                                float ky = trackY + m_optionsSliderH * 0.5f - (s->height * t->scale.y) * 0.5f;
                                t->position.x = kx;
                                t->position.y = ky;
                                GN_LOG_INFO("DEBUG: Pressed knob " + std::to_string(index) + " -> value=" + std::to_string(pressedValue) + 
                                            ", pos=(" + std::to_string(kx) + "," + std::to_string(ky) + ")");
                            }
                        }
                    }
                }
            }
            
            // State: CHANGED - Apply continuous updates (absolute mapping to touch X)
            if (draggingFlag && m_activeDragKnob == index && inputDown) {
                // Map touch X to slider value directly
                float newValue = clamp01((touchX - m_optionsSliderX) / m_optionsSliderW);
                valueRef = newValue;
                float newKnobCenterX = m_optionsSliderX + newValue * m_optionsSliderW;
                GN_LOG_INFO("DEBUG: Dragging knob " + std::to_string(index) + " - touchX=" + std::to_string(touchX) + 
                            ", newKnobCenterX=" + std::to_string(newKnobCenterX) + ", newValue=" + std::to_string(newValue));
                
                // Immediate visual feedback
                if (knobEntity != 0) {
                    if (auto t = m_ecsCoordinator->GetComponent<Transform>(knobEntity)) {
                        if (auto s = m_ecsCoordinator->GetComponent<Sprite>(knobEntity)) {
                            float kx = newKnobCenterX - (s->width * t->scale.x) * 0.5f;
                            float ky = trackY + m_optionsSliderH * 0.5f - (s->height * t->scale.y) * 0.5f;
                            t->position.x = kx;
                            t->position.y = ky;
                            GN_LOG_INFO("DEBUG: Updated knob " + std::to_string(index) + " position to (" + std::to_string(kx) + ", " + std::to_string(ky) + ")");
                        }
                    }
                }
            }
            
            // State: ENDED - Clean up drag operation  
            if (draggingFlag && justReleased && m_activeDragKnob == index) {
                draggingFlag = false;
                m_activeDragKnob = -1;  // Reset for next interaction
            }
        };

        float master = GameCore::GetGame()->GetMasterVolume();
        float music = GameCore::GetGame()->GetMusicVolume();
        float sfx = GameCore::GetGame()->GetSFXVolume();
        applySliderAtTrackY(0, master, m_draggingMaster, m_masterKnobEntity, masterTrackY);
        applySliderAtTrackY(1, music, m_draggingMusic, m_musicKnobEntity, musicTrackY);
        applySliderAtTrackY(2, sfx, m_draggingSFX, m_sfxKnobEntity, sfxTrackY);
        GameCore::GetGame()->SetMasterVolume(master);
        GameCore::GetGame()->SetMusicVolume(music);
        GameCore::GetGame()->SetSFXVolume(sfx);
        UpdateOptionsKnobPositions();

        // Difficulty arrows and back button hit-tests using cached geometry
        auto within = [&](float x, float y, float w, float h) {
            return touchX >= x && touchX <= x + w && touchY >= y && touchY <= y + h;
        };

        using GameCore::Difficulty;
        Difficulty current = GameCore::LevelManager::GetGlobalDifficulty();

        // Use ONLY actual arrow entity bounds - no fallback hitboxes to avoid overlap
        bool leftHit = false, rightHit = false;
        if (m_optionsLeftArrowEntity != 0) {
            Transform* t = m_ecsCoordinator->GetComponent<Transform>(m_optionsLeftArrowEntity);
            Sprite* s = m_ecsCoordinator->GetComponent<Sprite>(m_optionsLeftArrowEntity);
            // Hit test EXACTLY the image bounds
            if (t && s && within(t->position.x, t->position.y, s->width * t->scale.x, s->height * t->scale.y)) {
                leftHit = true;
            }
        }
        if (m_optionsRightArrowEntity != 0) {
            Transform* t = m_ecsCoordinator->GetComponent<Transform>(m_optionsRightArrowEntity);
            Sprite* s = m_ecsCoordinator->GetComponent<Sprite>(m_optionsRightArrowEntity);
            // Hit test EXACTLY the image bounds  
            if (t && s && within(t->position.x, t->position.y, s->width * t->scale.x, s->height * t->scale.y)) {
                rightHit = true;
            }
        }
        // NO FALLBACK - only use exact entity bounds to prevent overlap

        // Debounce difficulty arrow presses using the same timer as level select
        if (justPressed && (leftHit || rightHit)) {
            if (m_lastArrowPressTime >= m_arrowDebounceDelay) {
                if (leftHit) {
                    int d = static_cast<int>(current); d = std::max(0, d - 1);
                    GameCore::LevelManager::SetGlobalDifficulty(static_cast<Difficulty>(d));
                    m_game->SaveGameData(); // Consistent with how other data is persisted
                } else if (rightHit) {
                    int d = static_cast<int>(current); d = std::min(2, d + 1);
                    GameCore::LevelManager::SetGlobalDifficulty(static_cast<Difficulty>(d));
                    m_game->SaveGameData(); // Consistent with how other data is persisted
                }
                m_lastArrowPressTime = 0.0f; // reset shared debounce timer
            }
        }

        // Update centered difficulty value text after change
        if (m_difficultyValueEntity != 0) {
            if (auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_difficultyValueEntity)) {
                ui->buttonText = GameCore::LevelManager::GetDifficultyName();
            }
        }

        // Vibration toggle click detection with debouncing
        if (justPressed && m_vibrationToggleEntity != 0 && (m_animationTimer - m_lastMenuButtonPressTime) >= MENU_BUTTON_DEBOUNCE) {
            auto transform = m_ecsCoordinator->GetComponent<Transform>(m_vibrationToggleEntity);
            auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_vibrationToggleEntity);
            auto uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_vibrationToggleEntity);
            
            if (transform && sprite && uiElement && uiElement->isEnabled && uiElement->visible) {
                float toggleW = sprite->width * transform->scale.x;
                float toggleH = sprite->height * transform->scale.y;
                
                if (touchX >= transform->position.x && touchX <= transform->position.x + toggleW &&
                    touchY >= transform->position.y && touchY <= transform->position.y + toggleH) {
                    // Set debounce timer
                    m_lastMenuButtonPressTime = m_animationTimer;
                    
                    // Toggle vibration state
                    if (m_game) {
                        m_vibrationsEnabled = !m_vibrationsEnabled;
                        m_game->SetVibrationsEnabled(m_vibrationsEnabled);
                        
                        GN_LOG_INFO("Vibration toggled: " + std::string(m_vibrationsEnabled ? "ON" : "OFF"));
                        
                        // Use UISystem to atomically update the toggle button
                        if (auto uiSystem = m_ecsCoordinator->GetSystemManager()->GetUISystem()) {
                            uiSystem->SetToggleState(m_vibrationToggleEntity, m_vibrationsEnabled);
                        }
                        
                        // Save settings
                        m_game->SaveSettings();
                        
                        // Play haptic feedback for the toggle itself (if enabled)
                        if (m_vibrationsEnabled && m_platformDelegates && m_platformDelegates->haptic.triggerImpact) {
                            m_platformDelegates->haptic.triggerImpact(HapticStyle::LIGHT, 0.5f);
                        }
                    }
                }
            }
        }
        
        // Back button entity hit test
        bool backHit = false;
        if (m_optionsBackButtonEntity != 0) {
            if (auto t = m_ecsCoordinator->GetComponent<Transform>(m_optionsBackButtonEntity)) {
                if (auto s = m_ecsCoordinator->GetComponent<Sprite>(m_optionsBackButtonEntity)) {
                    if (within(t->position.x, t->position.y, s->width * t->scale.x, s->height * t->scale.y)) backHit = true;
                }
            }
        }
        if (backHit) {
            HideOptionsMenu();
        }
    }

    void MainMenuState::CreateDesktopLayout() {
        GN_LOG_INFO("Creating desktop main menu layout");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateDesktopLayout");
            return;
        }
        
        // Get enhanced screen information from shared RenderSystem
        ScreenInfo screenInfo;
        if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager() && m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            auto* renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
            screenInfo = renderSystem->GetScreenInfo();
            GN_LOG_INFO("Desktop enhanced screen info: logical=" + 
                       std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                       ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                       ", scale=" + std::to_string(screenInfo.scaleFactor));
        } else {
            // Fallback to platform delegates if render system not available
            if (m_game) {
                const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
                if (delegates.renderer.getScreenInfo) {
                    delegates.renderer.getScreenInfo(&screenInfo);
                    GN_LOG_INFO("Desktop enhanced screen info (delegate): logical=" + 
                               std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                               ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                               ", scale=" + std::to_string(screenInfo.scaleFactor));
                } else {
                    // Fallback to legacy screen size if enhanced info not available
                    if (delegates.renderer.getScreenSize) {
                        delegates.renderer.getScreenSize(&screenInfo.logicalWidth, &screenInfo.logicalHeight);
                        screenInfo.pixelWidth = screenInfo.logicalWidth;
                        screenInfo.pixelHeight = screenInfo.logicalHeight;
                        screenInfo.scaleFactor = 1.0f;
                        GN_LOG_INFO("Desktop fallback screen dimensions: " + std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight));
                    } else {
                        // Ultimate fallback
                        screenInfo.logicalWidth = 800.0f;
                        screenInfo.logicalHeight = 600.0f;
                        screenInfo.pixelWidth = 800.0f;
                        screenInfo.pixelHeight = 600.0f;
                        screenInfo.scaleFactor = 1.0f;
                    }
                }
        
                // Vibration toggle is handled in HandleOptionsInput, not here
            }
        }
        
        // Store screen dimensions for consistent use across the class - USE PIXEL DIMENSIONS for proper scaling
        m_screenWidth = screenInfo.pixelWidth;
        m_screenHeight = screenInfo.pixelHeight;
        
        float centerX = screenInfo.pixelWidth / 2.0f;
        float centerY = screenInfo.pixelHeight / 2.0f;
        
        // 1. Create Background Entity (MainMenu.png) - FULL SCREEN SCALING
        m_backgroundEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions using shared RenderSystem
        int textureW = 0, textureH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("MainMenu");
            if (!rs->GetTextureSize("MainMenu", textureW, textureH)) {
                textureW = 320; textureH = 180;
                GN_LOG_INFO("RenderSystem: size unavailable for 'MainMenu' yet; using fallback 320x180");
            }
        }
        float textureWidth = static_cast<float>(textureW);
        float textureHeight = static_cast<float>(textureH);
        
        // Calculate scale to fill screen - USE PIXEL DIMENSIONS
        float scaleX = screenInfo.pixelWidth / textureWidth;
        float scaleY = screenInfo.pixelHeight / textureHeight;
        
        // Position at top-left (0,0) since we now render from top-left
        Transform bgTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(scaleX, scaleY));
        
        // Create sprite with actual texture dimensions, scale will be applied by transform
        Sprite bgSprite("MainMenu", textureWidth, textureHeight);
        bgSprite.layer = 0; // Background layer
        bgSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backgroundEntity, bgTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        GN_LOG_INFO("Created full-screen background entity: MainMenu.png (texture: " + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) + ", scale: " + std::to_string(scaleX) + "x" + std::to_string(scaleY) + ", screen: " + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + ")");
        
        // 2. Create Logo Entity (FloppyLogo.png) - Desktop scaling
        m_logoEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions via RenderSystem
        int logoW = 0, logoH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyLogo");
            if (!rs->GetTextureSize("FloppyLogo", logoW, logoH)) {
                logoW = 112; logoH = 80;
                GN_LOG_INFO("RenderSystem: size unavailable for 'FloppyLogo'; using fallback 112x80");
            }
        }
        float logoWidth = static_cast<float>(logoW);
        float logoHeight = static_cast<float>(logoH);
        float logoScale = 2.0f; // 2x scale for desktop
        
        // Calculate logo position - use same x,y for both logo and F button - USE PIXEL DIMENSIONS
        float logoX = centerX - 150.0f; // Position to the left of center
        float logoY = screenInfo.pixelHeight * 0.35f; // 35% down from top
        
        // Calculate scaled dimensions using helper
        auto logoScaledDimensions = GetScaledDimensions(logoWidth, logoHeight, logoScale);
        float scaledLogoWidth = logoScaledDimensions.first;
        float scaledLogoHeight = logoScaledDimensions.second;
        
        // Logo position is already top-left based
        float logoTopLeftX = logoX;  // Already top-left for logo
        float logoTopLeftY = logoY;  // Already top-left for logo
        
        Transform logoTransform(Gnosis::GNVector2(logoTopLeftX, logoTopLeftY), 0.0f, Gnosis::GNVector2(logoScale, logoScale));
        Sprite logoSprite("FloppyLogo", logoWidth, logoHeight); // Use actual texture dimensions
        logoSprite.layer = 1; // Logo layer
        logoSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_logoEntity, logoSprite);
        GN_LOG_INFO("Created scaled logo entity: FloppyLogo.png (2x scale)");
        
        // 3. Create Interactive F Button Entity (F.png) - Desktop scaling
        m_fButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions via RenderSystem
        int fW = 0, fH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("F");
            if (!rs->GetTextureSize("F", fW, fH)) {
                fW = 28; fH = 40;
                GN_LOG_INFO("RenderSystem: size unavailable for 'F'; using fallback 28x40");
            }
        }
        float fButtonTextureWidth = static_cast<float>(fW);
        float fButtonTextureHeight = static_cast<float>(fH);
        
        // Position F button using SAME x,y coordinates as logo for perfect alignment
        float fButtonX = logoX; // Use same X as logo
        float fButtonY = logoY; // Use same Y as logo
        
        // Scale F button to match logo scale
        float fButtonScale = logoScale; // Use same scale as logo
        auto fButtonScaledDimensions = GetScaledDimensions(fButtonTextureWidth, fButtonTextureHeight, fButtonScale);
        float fButtonWidth = fButtonScaledDimensions.first;
        float fButtonHeight = fButtonScaledDimensions.second;
        
        // Position F button at same top-left coordinates as logo
        float fButtonTopLeftX = fButtonX;
        float fButtonTopLeftY = fButtonY;
        
        // Update logo transform to match F button exactly 
        logoTransform = Transform(Gnosis::GNVector2(logoTopLeftX, logoTopLeftY), 0.0f, Gnosis::GNVector2(logoScale, logoScale));
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        
        Transform fButtonTransform(Gnosis::GNVector2(fButtonTopLeftX, fButtonTopLeftY), 0.0f, Gnosis::GNVector2(fButtonScale, fButtonScale));
        Sprite fButtonSprite("F", fButtonTextureWidth, fButtonTextureHeight); // Use actual texture dimensions
        fButtonSprite.layer = 2; // F button layer
        fButtonSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_fButtonEntity, fButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_fButtonEntity, fButtonSprite);
        GN_LOG_INFO("Created F button entity: F.png (" + std::to_string(fButtonScale) + "x scale, actual size: " + std::to_string(fButtonTextureWidth) + "x" + std::to_string(fButtonTextureHeight) + ")");
        
        // Create menu buttons for desktop
        CreateMenuButtons();
        
        GN_LOG_INFO("Desktop layout created: Background, Logo, F Button, and Menu Button entities");
    }

    void MainMenuState::CreateMobileLayout() {
        GN_LOG_INFO("Creating mobile main menu layout");

        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMobileLayout");
            return;
        }

        // Setup platform-specific layout first
        SetupLayout();

        // Clear any existing button entities to prevent duplicates
        if (m_playButtonEntity != 0) m_ecsCoordinator->DestroyEntity(m_playButtonEntity);
        if (m_optionsButtonEntity != 0) m_ecsCoordinator->DestroyEntity(m_optionsButtonEntity);
        if (m_quickPlayButtonEntity != 0) m_ecsCoordinator->DestroyEntity(m_quickPlayButtonEntity);
        if (m_leaderboardButtonEntity != 0) m_ecsCoordinator->DestroyEntity(m_leaderboardButtonEntity);

        // Reset entity IDs
        m_playButtonEntity = 0;
        m_optionsButtonEntity = 0;
        m_quickPlayButtonEntity = 0;
        m_leaderboardButtonEntity = 0;

        // Get enhanced screen information from shared RenderSystem (like GameplayState)
        ScreenInfo screenInfo;
        if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager() && m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            auto* renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
            screenInfo = renderSystem->GetScreenInfo();
            GN_LOG_INFO("Mobile enhanced screen info: logical=" + 
                       std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                       ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                       ", scale=" + std::to_string(screenInfo.scaleFactor));
        } else {
            // Fallback to platform delegates if render system not available
            if (m_game) {
                const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
                if (delegates.renderer.getScreenInfo) {
                    delegates.renderer.getScreenInfo(&screenInfo);
                    GN_LOG_INFO("Mobile enhanced screen info (delegate): logical=" + 
                               std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                               ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                               ", scale=" + std::to_string(screenInfo.scaleFactor));
                } else {
                    // Fallback to legacy screen size if enhanced info not available
                    if (delegates.renderer.getScreenSize) {
                        delegates.renderer.getScreenSize(&screenInfo.logicalWidth, &screenInfo.logicalHeight);
                        screenInfo.pixelWidth = screenInfo.logicalWidth;
                        screenInfo.pixelHeight = screenInfo.logicalHeight;
                        screenInfo.scaleFactor = 1.0f;
                        GN_LOG_INFO("Mobile fallback screen dimensions: " + std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight));
                    } else {
                        // Ultimate fallback to iPhone 16 logical dimensions
                        screenInfo.logicalWidth = 393.0f;
                        screenInfo.logicalHeight = 852.0f;
                        screenInfo.pixelWidth = 1179.0f;
                        screenInfo.pixelHeight = 2556.0f;
                        screenInfo.scaleFactor = 3.0f;
                    }
                }
            }
        }
        
        // Store screen dimensions for consistent use across the class - USE PIXEL DIMENSIONS for proper scaling
        m_screenWidth = screenInfo.pixelWidth;
        m_screenHeight = screenInfo.pixelHeight;
        
        // === SIMPLIFIED MOBILE LAYOUT POSITIONING === //
        
        // 1. Create Background Entity (MainMenuMobile.png) - FULL SCREEN SCALING
        m_backgroundEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions via RenderSystem
        int bgW = 0, bgH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("MainMenuMobile");
            if (!rs->GetTextureSize("MainMenuMobile", bgW, bgH)) {
                bgW = 393; bgH = 852;
                GN_LOG_INFO("RenderSystem: size unavailable for 'MainMenuMobile'; using fallback 393x852");
            }
        }
        float bgTextureWidth = static_cast<float>(bgW);
        float bgTextureHeight = static_cast<float>(bgH);
        
        // Calculate scale to fill screen - USE PIXEL DIMENSIONS
        float bgScaleX = screenInfo.pixelWidth / bgTextureWidth;
        float bgScaleY = screenInfo.pixelHeight / bgTextureHeight;
        
        // Background positioned at (0,0) top-left
        Transform bgTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(bgScaleX, bgScaleY));
        Sprite bgSprite("MainMenuMobile", bgTextureWidth, bgTextureHeight);
        bgSprite.layer = 0;
        bgSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backgroundEntity, bgTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        GN_LOG_INFO("✅ Created background: MainMenuMobile.png at (0,0) with scale (" + std::to_string(bgScaleX) + "x" + std::to_string(bgScaleY) + ")");
        
        // 2. Create Logo Entity - SIMPLE POSITIONING
        m_logoEntity = m_ecsCoordinator->CreateEntity();
        
        // Load logo texture dimensions via RenderSystem
        int mLogoW = 0, mLogoH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyLogo");
            if (!rs->GetTextureSize("FloppyLogo", mLogoW, mLogoH)) { mLogoW = 112; mLogoH = 80; }
        }
        float logoTextureWidth = static_cast<float>(mLogoW);
        float logoTextureHeight = static_cast<float>(mLogoH);
        
        // Logo positioning: Center horizontally, 20% down from top
        float logoScale = 8.0f;  // Fixed scale for mobile
        auto logoScaledDimensions = GetScaledDimensions(logoTextureWidth, logoTextureHeight, logoScale);
        float logoScaledWidth = logoScaledDimensions.first;
        float logoScaledHeight = logoScaledDimensions.second;
        
        // For top-left rendering, position logo so it's centered on screen but accounting for its size
        // Calculate position so logo appears centered but renders from top-left - USE PIXEL DIMENSIONS
        float logoX = (screenInfo.pixelWidth - logoScaledWidth) / 2.0f;  // Center horizontally with top-left rendering
        float logoY = screenInfo.pixelHeight * 0.15f;  // 15% from top for top-left rendering
        
        Transform logoTransform(Gnosis::GNVector2(logoX, logoY), 0.0f, Gnosis::GNVector2(logoScale, logoScale));
        Sprite logoSprite("FloppyLogo", logoTextureWidth, logoTextureHeight);
        logoSprite.layer = 1;
        logoSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_logoEntity, logoSprite);
        GN_LOG_INFO("✅ Created logo: FloppyLogo.png at (" + std::to_string(logoX) + "," + std::to_string(logoY) + ") with scale " + std::to_string(logoScale) + "x" + std::to_string(logoScale));
        GN_LOG_INFO("🎯 LOGO DEBUG: logoTextureWidth=" + std::to_string(logoTextureWidth) + ", logoTextureHeight=" + std::to_string(logoTextureHeight) + ", logoScaledWidth=" + std::to_string(logoScaledWidth) + ", logoScaledHeight=" + std::to_string(logoScaledHeight));
        
        // 3. Create F Button Entity - OVERLAID ON LOGO
        m_fButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Load F button texture dimensions via RenderSystem
        int mFW = 0, mFH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("F");
            if (!rs->GetTextureSize("F", mFW, mFH)) { mFW = 28; mFH = 40; }
        }
        float fButtonTextureWidth = static_cast<float>(mFW);
        float fButtonTextureHeight = static_cast<float>(mFH);
        
        // F button uses SAME position and scale as logo for perfect overlay
        float fButtonScale = logoScale;  // Match logo scale exactly
        
        Transform fButtonTransform(Gnosis::GNVector2(logoX, logoY), 0.0f, Gnosis::GNVector2(fButtonScale, fButtonScale));
        Sprite fButtonSprite("F", fButtonTextureWidth, fButtonTextureHeight);
        fButtonSprite.layer = 2; // Above logo
        fButtonSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_fButtonEntity, fButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_fButtonEntity, fButtonSprite);
        GN_LOG_INFO("✅ Created F button: F.png at (" + std::to_string(logoX) + "," + std::to_string(logoY) + ") with scale " + std::to_string(fButtonScale) + "x" + std::to_string(fButtonScale) + " (overlaid on logo)");
        GN_LOG_INFO("🎯 F BUTTON DEBUG: fButtonTextureWidth=" + std::to_string(fButtonTextureWidth) + ", fButtonTextureHeight=" + std::to_string(fButtonTextureHeight) + ", using EXACT same coordinates as logo");
        
        // 4. Create menu buttons for mobile (includes Ad Controls button)
        CreateMobileMenuButtons();
        
        // 5. Create Version Number text (bottom right, properly aligned to stay on screen)
        if (m_versionTextEntity == 0) {
            m_versionTextEntity = m_ecsCoordinator->CreateEntity();
        }
        
        // Position bottom right with appropriate padding
        // Estimate text width: ~8-10px per character at this font size, so "v0.8" is roughly 40px
        float versionFontSize = 42.0f;
        float estimatedTextWidth = 50.0f; // Conservative estimate for "v0.8"
        float versionPaddingRight = 150.0f; // More padding from right edge to move further left
        float versionPaddingBottom = 60.0f; // Padding from bottom

        float versionX = m_screenWidth - versionPaddingRight - estimatedTextWidth;
        float versionY = m_screenHeight - versionPaddingBottom;
        
        Transform versionTransform(Gnosis::GNVector2(versionX, versionY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        Sprite versionSprite;
        versionSprite.visible = false; // Text only
        versionSprite.layer = 5;
        
        std::string versionText = "v0.8";
        UIElement versionUI(versionText, "", "");
        versionUI.fontSize = versionFontSize;
        versionUI.textColor = Gnosis::GNColor(255, 255, 255, 255); // White
        versionUI.centerTextHorizontally = false;
        versionUI.centerTextVertically = true;
        versionUI.visible = true;
        versionUI.isEnabled = false;
        
        // Use HasComponent checks to prevent duplicate component addition
        if (!m_ecsCoordinator->HasComponent<Transform>(m_versionTextEntity)) {
            m_ecsCoordinator->AddComponent<Transform>(m_versionTextEntity, versionTransform);
        } else {
            *m_ecsCoordinator->GetComponent<Transform>(m_versionTextEntity) = versionTransform;
        }
        
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_versionTextEntity)) {
            m_ecsCoordinator->AddComponent<Sprite>(m_versionTextEntity, versionSprite);
        } else {
            *m_ecsCoordinator->GetComponent<Sprite>(m_versionTextEntity) = versionSprite;
        }
        
        if (!m_ecsCoordinator->HasComponent<UIElement>(m_versionTextEntity)) {
            m_ecsCoordinator->AddComponent<UIElement>(m_versionTextEntity, versionUI);
        } else {
            *m_ecsCoordinator->GetComponent<UIElement>(m_versionTextEntity) = versionUI;
        }
        
        GN_LOG_INFO("✅ Set version text at (" + std::to_string(versionX) + "," + std::to_string(versionY) + ") - entity " + std::to_string(m_versionTextEntity));
        
        GN_LOG_INFO("✅ Mobile layout created successfully with simplified positioning");
    }

    void MainMenuState::UpdateMenuSelection() {
        // TODO: Handle menu selection logic
        // - Update selected option based on input
        // - Update visual highlights
        // - Handle selection confirmation
        
        // Placeholder for menu selection updates
    }

    void MainMenuState::UpdateMenuAnimations(float deltaTime) {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }
        
        // Logo floating animation
        if (m_logoEntity != 0) {
            Transform* logoTransform = m_ecsCoordinator->GetComponent<Transform>(m_logoEntity);
            if (logoTransform) {
                // Create a gentle floating effect
                float logoFloat = sin(m_animationTimer * 1.5f) * 8.0f; // 8 pixel float amplitude
                
                // Get current screen dimensions from render system (not cached values)
                float currentScreenHeight = m_screenHeight;
                if (auto* renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                    ScreenInfo screenInfo = renderSystem->GetScreenInfo();
                    currentScreenHeight = static_cast<float>(screenInfo.pixelHeight);
                }

                // Update logo Y position (preserve original Y + float offset)
                // Use the SAME calculation as in CreateMobileLayout for consistency
                float originalLogoY = currentScreenHeight * 0.15f; // 15% from top (matches CreateMobileLayout)
                logoTransform->position.y = originalLogoY + logoFloat;
            }
        }
        
        // F Button follows logo animation perfectly - same position + gentle pulsing
        if (m_fButtonEntity != 0) {
            Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
            if (fButtonTransform) {
                // Make F button follow logo's floating animation exactly
                float logoFloat = sin(m_animationTimer * 1.5f) * 8.0f; // Same float as logo

                // Get current screen dimensions from render system (not cached values)
                float currentScreenHeight = m_screenHeight;
                if (auto* renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                    ScreenInfo screenInfo = renderSystem->GetScreenInfo();
                    currentScreenHeight = static_cast<float>(screenInfo.pixelHeight);
                }

                float originalLogoY = currentScreenHeight * 0.15f; // Same Y calculation as logo
                fButtonTransform->position.y = originalLogoY + logoFloat; // Follow logo's Y position exactly
                
                // Create a subtle pulsing scale effect - use consistent 8x scale for mobile
                float baseScale = m_isMobile ? 8.0f : 2.5f; // Fixed mobile scale to 8.0f
                float pulseScale = baseScale + sin(m_animationTimer * 2.5f) * 0.3f;
                fButtonTransform->scale.x = pulseScale;
                fButtonTransform->scale.y = pulseScale;
            }
        }
    }

    void MainMenuState::OnMenuOptionSelected(MenuOption option) {
        GN_LOG_INFO("Menu option selected: " + std::to_string(static_cast<int>(option)));
        
        switch (option) {
            case MenuOption::PLAYING:
                GN_LOG_INFO("Starting game...");
                // TODO: Transition to game state
                m_finished = true;
                break;
                
            case MenuOption::OPTIONS:
                GN_LOG_INFO("Opening options menu...");
                ShowOptionsMenu();
                break;
                
            case MenuOption::QUICK_PLAY:
                GN_LOG_INFO("Starting quick play - Level 1...");
                // Set selected level NUMBER (not index) - matches OnLevelSelected logic
                m_selectedLevelIndex = 1;  // Level 1 (number, not 0-indexed)
                m_enteredViaQuickplay = true;  // Mark that we entered via Quickplay
                m_finished = true;
                break;
                
            case MenuOption::LEADERBOARD:
                GN_LOG_INFO("Opening leaderboards...");
                m_transitioningToLeaderboard = true;
                m_finished = true;
                break;
                
            default:
                GN_LOG_WARN("Unknown menu option selected");
                break;
        }
    }

    void MainMenuState::OnFButtonPressed() {
        GN_LOG_INFO("F button pressed - playing random fart sound!");
        
        // Play random fart sound (fart1.mp3 through fart11.mp3)
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(1, 11);
        
        int randomFartNumber = dis(gen);
        std::string fartSoundName = "fart" + std::to_string(randomFartNumber);
        
        // Use delegate system to play sound via cached game pointer
        if (m_game) {
            const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
            if (delegates.audio.playSound) {
                delegates.audio.playSound(fartSoundName.c_str(), 0.8f); // 80% volume
                GN_LOG_INFO("Playing fart sound: %s.mp3", fartSoundName.c_str());
            }
        }
        
        // Add visual feedback - make F button briefly larger
        if (m_fButtonEntity != 0 && m_ecsCoordinator) {
            Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
            if (fButtonTransform) {
                // Temporarily scale up the F button for feedback - based on 8x scale for mobile
                float feedbackScale = m_isMobile ? 9.0f : 3.0f; // Bigger feedback for mobile 8x base scale
                fButtonTransform->scale.x = feedbackScale;
                fButtonTransform->scale.y = feedbackScale;
                // Note: This will be smoothed back by the pulsing animation
            }
        }
    }

    const char* MainMenuState::GetMenuOptionText(int optionIndex) const {
        switch (static_cast<MenuOption>(optionIndex)) {
            case MenuOption::PLAYING:    return "Playing";
            case MenuOption::OPTIONS:    return "Options";
            case MenuOption::QUICK_PLAY: return "Quick Play";
            case MenuOption::LEADERBOARD: return "Leaderboard";
            default:                     return "Unknown";
        }
    }

    int MainMenuState::GetMenuOptionCount() const {
        return static_cast<int>(MenuOption::COUNT);
    }

    bool MainMenuState::IsMobilePlatform() const {
        // Use platform detection from game instance (set at initialization)
        return m_game ? m_game->IsIOSPlatform() : false;
    }

    void MainMenuState::CreateMenuButtons() {
        GN_LOG_INFO("Creating desktop menu buttons");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMenuButtons");
            return;
        }
        
        // Load button texture to get actual dimensions via RenderSystem
        int btnW = 0, btnH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyButtonBlue");
            if (!rs->GetTextureSize("FloppyButtonBlue", btnW, btnH)) { btnW = 90; btnH = 16; }
        }
        float buttonTextureWidth = static_cast<float>(btnW);
        float buttonTextureHeight = static_cast<float>(btnH);
        
        GN_LOG_INFO("Button texture dimensions: " + std::to_string(buttonTextureWidth) + "x" + std::to_string(buttonTextureHeight));
        
        float centerX = m_screenWidth / 2.0f;
        float buttonY = m_screenHeight * 0.55f; // Position buttons higher up
        float buttonSpacing = 150.0f; // Much more spacing between buttons
        float buttonScale = 10.0f; // Keep sprite scale at 10x for mobile visuals
        m_menuButtonScale = buttonScale;
        // Text size globally controlled; do not derive from sprite scale
        m_globalUIFontSize = m_isMobile ? 80.0f : (m_buttonFontSize * 5.0f);
        
        // Create Play Button
        m_playButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto playButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float playButtonWidth = playButtonScaledDimensions.first;
        float playButtonHeight = playButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        Gnosis::GNVector2 playButtonPosition = CenterObjectAtPosition(centerX, buttonY, playButtonWidth, playButtonHeight);
        float playButtonTopLeftX = playButtonPosition.x;
        float playButtonTopLeftY = playButtonPosition.y;
        
        Transform playTransform(Gnosis::GNVector2(playButtonTopLeftX, playButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite playSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        playSprite.layer = 2; // Button layer (lower than text)
        playSprite.visible = true;
        UIElement playButton("PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = m_buttonFontSize;
        playButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Play button with text: '%s' (length: %zu)", playButton.buttonText.c_str(), playButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_playButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_playButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_playButtonEntity, playButton);
        
        // Create Options Button
        m_optionsButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto optionsButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float optionsButtonWidth = optionsButtonScaledDimensions.first;
        float optionsButtonHeight = optionsButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        float optionsButtonCenterY = buttonY + buttonSpacing;
        Gnosis::GNVector2 optionsButtonPosition = CenterObjectAtPosition(centerX, optionsButtonCenterY, optionsButtonWidth, optionsButtonHeight);
        float optionsButtonTopLeftX = optionsButtonPosition.x;
        float optionsButtonTopLeftY = optionsButtonPosition.y;
        
        Transform optionsTransform(Gnosis::GNVector2(optionsButtonTopLeftX, optionsButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite optionsSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        optionsSprite.layer = 2; // Button layer (lower than text)
        optionsSprite.visible = true;
        UIElement optionsButton("OPTIONS", "FloppyButtonBlue", "FloppyButtonBlueHover");
        optionsButton.fontSize = m_buttonFontSize;
        optionsButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Options button with text: '%s' (length: %zu)", optionsButton.buttonText.c_str(), optionsButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_optionsButtonEntity, optionsTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_optionsButtonEntity, optionsSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_optionsButtonEntity, optionsButton);
        
        // Create Quick Play Button
        m_quickPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto quickPlayButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float quickPlayButtonWidth = quickPlayButtonScaledDimensions.first;
        float quickPlayButtonHeight = quickPlayButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        float quickPlayButtonCenterY = buttonY + buttonSpacing * 2;
        Gnosis::GNVector2 quickPlayButtonPosition = CenterObjectAtPosition(centerX, quickPlayButtonCenterY, quickPlayButtonWidth, quickPlayButtonHeight);
        float quickPlayButtonTopLeftX = quickPlayButtonPosition.x;
        float quickPlayButtonTopLeftY = quickPlayButtonPosition.y;
        
        Transform quickPlayTransform(Gnosis::GNVector2(quickPlayButtonTopLeftX, quickPlayButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quickPlaySprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        quickPlaySprite.layer = 2; // Button layer (lower than text)
        quickPlaySprite.visible = true;
        UIElement quickPlayButton("QUICK PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quickPlayButton.fontSize = m_buttonFontSize;
        quickPlayButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Quick Play button with text: '%s' (length: %zu)", quickPlayButton.buttonText.c_str(), quickPlayButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quickPlayButtonEntity, quickPlayTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quickPlayButtonEntity, quickPlaySprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_quickPlayButtonEntity, quickPlayButton);
        
        // Create Leaderboard Button
        m_leaderboardButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto quitButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float quitButtonWidth = quitButtonScaledDimensions.first;
        float quitButtonHeight = quitButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        float quitButtonCenterY = buttonY + buttonSpacing * 3;
        Gnosis::GNVector2 quitButtonPosition = CenterObjectAtPosition(centerX, quitButtonCenterY, quitButtonWidth, quitButtonHeight);
        float quitButtonTopLeftX = quitButtonPosition.x;
        float quitButtonTopLeftY = quitButtonPosition.y;
        
        Transform quitTransform(Gnosis::GNVector2(quitButtonTopLeftX, quitButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quitSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        quitSprite.layer = 2; // Button layer (lower than text)
        quitSprite.visible = true;
        UIElement quitButton("QUIT", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quitButton.fontSize = m_buttonFontSize;
        quitButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Leaderboard button with text: '%s' (length: %zu)", quitButton.buttonText.c_str(), quitButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_leaderboardButtonEntity, quitTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_leaderboardButtonEntity, quitSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_leaderboardButtonEntity, quitButton);
        
        GN_LOG_INFO("Created desktop menu buttons: Play, Options, Quick Play, Leaderboard");
    }

    void MainMenuState::CreateMobileMenuButtons() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMobileMenuButtons");
            return;
        }
        
        // Load button texture dimensions once via RenderSystem
        int mBtnW = 0, mBtnH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyButtonBlue");
            if (!rs->GetTextureSize("FloppyButtonBlue", mBtnW, mBtnH)) { mBtnW = 90; mBtnH = 16; }
        }
        float buttonTextureWidth = static_cast<float>(mBtnW);
        float buttonTextureHeight = static_cast<float>(mBtnH);
        
        GN_LOG_INFO("Mobile button texture dimensions: " + std::to_string(buttonTextureWidth) + "x" + std::to_string(buttonTextureHeight));
        
        // === SIMPLIFIED MOBILE BUTTON POSITIONING === //

        float buttonScale = 10.0f;  // Keep sprite scale at 10x for mobile visuals
        m_menuButtonScale = buttonScale;
        // Text size globally controlled; do not derive from sprite scale
        m_globalUIFontSize = m_isMobile ? 80.0f : (m_buttonFontSize * 5.0f);
        auto buttonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float buttonScaledWidth = buttonScaledDimensions.first;
        float buttonScaledHeight = buttonScaledDimensions.second;

        // Determine orientation - buttons should remain vertically stacked and centered in both orientations
        bool isLandscape = (m_screenWidth > m_screenHeight);

        float startY, buttonSpacing;
        if (isLandscape) {
            // Landscape mode: same vertical stacking but adjust spacing for taller screen
            startY = m_screenHeight * 0.50f;  // Start at 50% down from top
            buttonSpacing = buttonScaledHeight + 60.0f;  // Slightly tighter vertical spacing for landscape
            GN_LOG_INFO("📱 Creating landscape mobile buttons (vertical stack): screen=" + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));
        } else {
            // Portrait mode: vertical stacking with standard spacing
            startY = m_screenHeight * 0.50f;  // Start at 50% down from top
            buttonSpacing = buttonScaledHeight + 80.0f;  // Vertical spacing
            GN_LOG_INFO("📱 Creating portrait mobile buttons: screen=" + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));
        }
        
        GN_LOG_INFO("📱 Creating mobile buttons: scale=" + std::to_string(buttonScale) + ", size=" + std::to_string(buttonScaledWidth) + "x" + std::to_string(buttonScaledHeight) + ", startY=" + std::to_string(startY) + ", spacing=" + std::to_string(buttonSpacing));
        
        // Helper lambda for creating buttons with consistent positioning
        auto createButton = [&](Gnosis::Entity& entity, const std::string& text, int buttonIndex) {
            entity = m_ecsCoordinator->CreateEntity();

            // Always use vertical stacking with horizontal centering for both orientations
            float buttonCenterX = m_screenWidth / 2.0f;  // Always center horizontally
            float buttonCenterY = startY + (buttonIndex * buttonSpacing);  // Vertical stacking

            // Use CenterObjectAtPosition to get the correct top-left coordinates
            auto buttonPosition = CenterObjectAtPosition(buttonCenterX, buttonCenterY, buttonScaledWidth, buttonScaledHeight);
            float buttonX = buttonPosition.x;
            float buttonY = buttonPosition.y;

            Transform transform(Gnosis::GNVector2(buttonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            Sprite sprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight);
            sprite.layer = 2;
            sprite.visible = true;

            UIElement uiElement("", "FloppyButtonBlue", "FloppyButtonBlueHover");
            uiElement.buttonText = text;
            uiElement.fontSize = m_globalUIFontSize; // unified text size (144 on mobile)
            uiElement.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White text
            uiElement.centerTextHorizontally = true;
            uiElement.centerTextVertically = true;
            // NO OFFSET - text should be perfectly centered as requested
            uiElement.textOffsetY = 0.0f;  // No offset for perfect centering

            m_ecsCoordinator->AddComponent<Transform>(entity, transform);
            m_ecsCoordinator->AddComponent<Sprite>(entity, sprite);
            m_ecsCoordinator->AddComponent<UIElement>(entity, uiElement);

            GN_LOG_INFO("✅ Created '" + text + "' button at (" + std::to_string(buttonX) + "," + std::to_string(buttonY) + ")");
        };
        
        // Create all buttons using consistent positioning
        createButton(m_playButtonEntity, "PLAY", 0);
        createButton(m_optionsButtonEntity, "OPTIONS", 1);
        createButton(m_quickPlayButtonEntity, "QUICK PLAY", 2);
        createButton(m_leaderboardButtonEntity, "LEADERBOARD", 3);
        
        // Create Ad Controls button - bottom left with padding (like settings button)
        if (m_adControlsButtonEntity == 0) {
            m_adControlsButtonEntity = m_ecsCoordinator->CreateEntity();
        }
        
        // Load settings button texture - actual texture is 16x16 (same as GameplayState)
        float adButtonTextureWidth = 16.0f;
        float adButtonTextureHeight = 16.0f;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("settingsbutton");
        }
        
        // Position bottom left with padding - USING SAME m_screenHeight as other buttons
        float adButtonScale = 6.0f;
        float adButtonPadding = 40.0f;
        float adButtonX = adButtonPadding;
        float adButtonY = m_screenHeight - (adButtonTextureHeight * adButtonScale) - adButtonPadding;
        
        Transform adButtonTransform(Gnosis::GNVector2(adButtonX, adButtonY), 0.0f, Gnosis::GNVector2(adButtonScale, adButtonScale));
        Sprite adButtonSprite;
        adButtonSprite.textureId = "settingsbutton";
        adButtonSprite.width = adButtonTextureWidth;
        adButtonSprite.height = adButtonTextureHeight;
        adButtonSprite.visible = true;
        adButtonSprite.layer = 5;
        
        UIElement adButtonUI("", "settingsbutton", "");
        adButtonUI.buttonText = "AD CONTROLS";
        adButtonUI.fontSize = 36.0f;
        adButtonUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        adButtonUI.centerTextHorizontally = true;
        adButtonUI.centerTextVertically = true;
        adButtonUI.textOffsetX = 220.0f;
        adButtonUI.textOffsetY = 0.0f;
        adButtonUI.visible = true;
        adButtonUI.isEnabled = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_adControlsButtonEntity, adButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_adControlsButtonEntity, adButtonSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_adControlsButtonEntity, adButtonUI);
        
        GN_LOG_INFO("✅ Created Ad Controls button at (" + std::to_string(adButtonX) + "," + std::to_string(adButtonY) + ") - m_screenHeight=" + std::to_string(m_screenHeight));
        GN_LOG_INFO("✅ Created mobile menu buttons with simplified positioning");
    }

    void MainMenuState::DrawButtonDebugRectangles() {
        if (!m_game || !m_ecsCoordinator) {
            GN_LOG_INFO("DrawButtonDebugRectangles: No game or ECS coordinator");
            return;
        }
        
        const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
        if (!delegates.renderer.drawRectangle) {
            GN_LOG_INFO("DrawButtonDebugRectangles: No drawRectangle delegate");
            return;
        }
        
        GN_LOG_INFO("DrawButtonDebugRectangles: Drawing debug rectangles...");
        
        // Draw debug rectangles for each button
        std::vector<Gnosis::Entity> buttons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_leaderboardButtonEntity};
        
        for (Gnosis::Entity buttonEntity : buttons) {
            if (buttonEntity == 0) continue;
            
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(buttonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(buttonEntity);
            
            if (transform && sprite) {
                // Calculate button bounds using actual scaled dimensions
                float buttonWidth = sprite->width;
                float buttonHeight = sprite->height;
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                
                GN_LOG_INFO("DrawButtonDebugRectangles: Drawing rectangle at (" + std::to_string(buttonLeft) + ", " + std::to_string(buttonTop) + ") size (" + std::to_string(buttonWidth) + ", " + std::to_string(buttonHeight) + ")");
                
                // Draw red outline rectangle
                delegates.renderer.drawRectangle(
                    buttonLeft, buttonTop, 
                    buttonWidth, buttonHeight, 
                    1.0f, 0.0f, 0.0f, 0.5f  // Red with 50% alpha
                );
            }
        }
    }

    void MainMenuState::OnPlayButtonPressed() {
        GN_LOG_INFO("Play button pressed - showing level select");
        m_lastMenuButtonPressTime = m_animationTimer;
        ShowLevelSelect();
    }

    void MainMenuState::OnOptionsButtonPressed() {
        GN_LOG_INFO("Options button pressed - transitioning to options menu");
        m_lastMenuButtonPressTime = m_animationTimer;
        ShowOptionsMenu();
    }

    void MainMenuState::OnQuickPlayButtonPressed() {
        GN_LOG_INFO("Quick Play button pressed - starting level 1");
        m_lastMenuButtonPressTime = m_animationTimer;
        OnMenuOptionSelected(MenuOption::QUICK_PLAY);
    }

    void MainMenuState::OnLeaderboardButtonPressed() {
        GN_LOG_INFO("Leaderboard button pressed - opening leaderboards");
        m_lastMenuButtonPressTime = m_animationTimer;
        OnMenuOptionSelected(MenuOption::LEADERBOARD);
    }

    void MainMenuState::OnAdControlsButtonPressed() {
        GN_LOG_INFO("Ad Controls button pressed - showing ad controls menu");
        m_lastMenuButtonPressTime = m_animationTimer;
        m_currentMode = MenuMode::AD_CONTROLS;
        SetMainMenuVisible(false);
        ShowAdControlsMenu();
    }

    void MainMenuState::CheckMenuButtonClicks(float touchX, float touchY) {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }
        
        // Check debounce timer - prevent clicks during transition
        float timeSinceLastPress = m_animationTimer - m_lastMenuButtonPressTime;
        if (timeSinceLastPress < MENU_BUTTON_DEBOUNCE) {
            return; // Still in debounce period
        }
        
        GN_LOG_INFO("🎯 CheckMenuButtonClicks: Touch at pixel(" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");
        
        // Reset all button states first
        ResetAllButtonStates();
        
        // Check Play Button
        if (m_playButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_playButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_playButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_playButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Use actual sprite dimensions instead of hardcoded values
                float buttonWidth = sprite->width * transform->scale.x;
                float buttonHeight = sprite->height * transform->scale.y;
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                GN_LOG_INFO("🎯 BEFORE COLLISION: PLAY Button - spriteSize(" + std::to_string(sprite->width) + "x" + std::to_string(sprite->height) + 
                           ") scale(" + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + 
                           ") pos(" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ")");
                
                GN_LOG_INFO("🎯 BEFORE COLLISION: PLAY Button bounds - L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " size=" + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight));
                
                bool xInBounds = (touchX >= buttonLeft && touchX <= buttonRight);
                bool yInBounds = (touchY >= buttonTop && touchY <= buttonBottom);
                
                GN_LOG_INFO("🎯 COLLISION TEST: PLAY Button - xInBounds=" + std::string(xInBounds ? "TRUE" : "FALSE") + 
                           " yInBounds=" + std::string(yInBounds ? "TRUE" : "FALSE"));
                
                if (xInBounds && yInBounds) {
                    GN_LOG_INFO("🎮 AFTER COLLISION: PLAY BUTTON HIT! Calling OnPlayButtonPressed()");
                    // Trigger action and immediately reset visual state
                    OnPlayButtonPressed();
                    uiElement->isPressed = false;
                    uiElement->isHovered = false;
                    UpdateButtonSprite(m_playButtonEntity, *uiElement);
                    return;
                } else {
                    GN_LOG_INFO("❌ AFTER COLLISION: PLAY Button missed");
                }
            }
        }
        
        // Check Options Button
        if (m_optionsButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_optionsButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_optionsButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_optionsButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Use actual sprite dimensions instead of hardcoded values
                float buttonWidth = sprite->width * transform->scale.x;
                float buttonHeight = sprite->height * transform->scale.y;
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                GN_LOG_INFO("🎯 BEFORE COLLISION: OPTIONS Button - spriteSize(" + std::to_string(sprite->width) + "x" + std::to_string(sprite->height) + 
                           ") scale(" + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + 
                           ") pos(" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ")");
                
                GN_LOG_INFO("🎯 BEFORE COLLISION: OPTIONS Button bounds - L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " size=" + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight));
                
                bool xInBounds = (touchX >= buttonLeft && touchX <= buttonRight);
                bool yInBounds = (touchY >= buttonTop && touchY <= buttonBottom);
                
                GN_LOG_INFO("🎯 COLLISION TEST: OPTIONS Button - xInBounds=" + std::string(xInBounds ? "TRUE" : "FALSE") + 
                           " yInBounds=" + std::string(yInBounds ? "TRUE" : "FALSE"));
                
                if (xInBounds && yInBounds) {
                    GN_LOG_INFO("🎮 AFTER COLLISION: OPTIONS BUTTON HIT! Calling OnOptionsButtonPressed()");
                    // Trigger action and immediately reset visual state
                    OnOptionsButtonPressed();
                    uiElement->isPressed = false;
                    uiElement->isHovered = false;
                    UpdateButtonSprite(m_optionsButtonEntity, *uiElement);
                    return;
                } else {
                    GN_LOG_INFO("❌ AFTER COLLISION: OPTIONS Button missed");
                }
            }
        }
        
        // Check Quick Play Button
        if (m_quickPlayButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_quickPlayButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_quickPlayButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_quickPlayButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Button is now positioned at top-left, so collision detection uses top-left based bounds
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 QUICK PLAY Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: QUICK PLAY BUTTON HIT!");
                    // Trigger action and immediately reset visual state
                    OnQuickPlayButtonPressed();
                    uiElement->isPressed = false;
                    uiElement->isHovered = false;
                    UpdateButtonSprite(m_quickPlayButtonEntity, *uiElement);
                    return;
                }
            }
        }
        
        // Check Leaderboard Button
        if (m_leaderboardButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_leaderboardButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_leaderboardButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_leaderboardButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Button is now positioned at top-left, so collision detection uses top-left based bounds
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 QUIT Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: LEADERBOARD BUTTON HIT!");
                    // Trigger action and immediately reset visual state
                    OnLeaderboardButtonPressed();
                    uiElement->isPressed = false;
                    uiElement->isHovered = false;
                    UpdateButtonSprite(m_leaderboardButtonEntity, *uiElement);
                    return;
                }
            }
        }
        
        // Check Ad Controls Button
        if (m_adControlsButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_adControlsButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_adControlsButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsButtonEntity);
            
            if (transform && sprite && uiElement) {
                float buttonWidth = sprite->width * transform->scale.x;
                float buttonHeight = sprite->height * transform->scale.y;
                float buttonLeft = transform->position.x;
                // Extend hitbox to the right to cover the "AD CONTROLS" text (textOffsetX is 220.0f)
                // Add extra 280px to cover the full text area
                float buttonRight = transform->position.x + buttonWidth + 280.0f;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                GN_LOG_INFO("🎯 AD CONTROLS Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + " (extended hitbox)");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: AD CONTROLS BUTTON HIT!");
                    OnAdControlsButtonPressed();
                    return;
                }
            }
        }
        
        GN_LOG_INFO("❌ MainMenuState: Touch missed all buttons");
    }

    void MainMenuState::ResetAllButtonStates() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // Reset all button states to normal
        std::vector<Gnosis::Entity> buttonEntities = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_leaderboardButtonEntity};
        
        for (Gnosis::Entity entity : buttonEntities) {
            if (entity != 0) {
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) {
                    uiElement->isPressed = false;
                    uiElement->isHovered = false;
                    UpdateButtonSprite(entity, *uiElement);
                }
            }
        }
    }

    void MainMenuState::UpdateButtonSprite(Gnosis::Entity entity, const UIElement& uiElement) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        if (!sprite) {
            return;
        }
        
        // Choose the appropriate texture based on button state
        std::string textureId;
        if (uiElement.isPressed) {
            textureId = uiElement.pressedTextureId;
        } else if (uiElement.isHovered) {
            textureId = uiElement.hoverTextureId;
        } else {
            textureId = uiElement.normalTextureId;
        }
        
        // Update the sprite texture
        sprite->textureId = textureId;
        GN_LOG_DEBUG("Updated button sprite to: " + textureId);
    }



    void MainMenuState::CreateUIElements() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("CreateUIElements: ECS coordinator is null");
            return;
        }
        
        GN_LOG_INFO("Creating UI elements (buttons with integrated text)...");
        
        // The UI elements are already created in CreateDesktopLayout() and CreateMobileMenuButtons()
        // This method is now just for any additional UI setup if needed in the future
        
        GN_LOG_INFO("UI elements created successfully");
    }

    // Level Select Implementation
    void MainMenuState::InitializeLevels() {
        // Debug: Show current coin count at startup
        if (GameCore::GetGame()) {
            int currentCoins = GameCore::GetGame()->GetPlayerCoins();
            GameCore::FloppyTurdGame::GameStats gameStats = GameCore::GetGame()->GetGameStats();
            GN_LOG_INFO("💰 MAIN MENU STARTUP - GetPlayerCoins(): " + std::to_string(currentCoins) + " coins");
            GN_LOG_INFO("💰 MAIN MENU STARTUP - GameStats.totalCoinsCollected: " + std::to_string(gameStats.totalCoinsCollected) + " coins");
        }

        m_levels.clear();

        // Add all levels with their painting textures and unlock status from game
        bool level1Unlocked = GameCore::GetGame() ? GameCore::GetGame()->IsLevelUnlocked(1) : true;
        bool level2Unlocked = GameCore::GetGame() ? GameCore::GetGame()->IsLevelUnlocked(2) : false;
        bool level3Unlocked = GameCore::GetGame() ? GameCore::GetGame()->IsLevelUnlocked(3) : false;
        bool level4Unlocked = GameCore::GetGame() ? GameCore::GetGame()->IsLevelUnlocked(4) : false;
        bool level5Unlocked = GameCore::GetGame() ? GameCore::GetGame()->IsLevelUnlocked(5) : false;
        bool level6Unlocked = GameCore::GetGame() ? GameCore::GetGame()->IsLevelUnlocked(6) : false;

        m_levels.push_back({"A Flop in the Park", "ParkLevelPainting", "LockedPainting", level1Unlocked, 1});
        m_levels.push_back({"Home Sweet Home", "SewerLevelPainting", "LockedPainting", level2Unlocked, 2});
        m_levels.push_back({"The Good, The Bad,\nand the Stinky", "DesertLevelPainting", "LockedPainting", level3Unlocked, 3});
        m_levels.push_back({"Polar Pandemonium", "SnowLevelPainting", "LockedPainting", level4Unlocked, 4});
        m_levels.push_back({"Dung in the Dungeon", "CastleLevelPainting", "LockedPainting", level5Unlocked, 5});
        m_levels.push_back({"Curtains for Crap", "RatKingPainting", "LockedPainting", level6Unlocked, 6});

        m_currentLevelIndex = 0;
        GN_LOG_INFO("Initialized " + std::to_string(m_levels.size()) + " levels with game unlock status");
    }

    void MainMenuState::CreateLevelSelectLayout() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("CreateLevelSelectLayout: ECS coordinator is null");
            return;
        }
        
        GN_LOG_INFO("Creating level select layout...");
        
        CreateLevelPaintings();
        CreateBackButton();
        CreateLevelPlayButton();
        CreateArrowButtons();
        CreateLockedIndicator();
        
        // Initially hide all level select elements
        // IMPORTANT: Save and restore current mode to avoid overwriting it when entering from gameplay
        MenuMode savedMode = m_currentMode;
        HideLevelSelect();
        m_currentMode = savedMode;  // Restore the mode that was set before Enter()
        
        GN_LOG_INFO("Level select layout created - mode preserved: " + std::to_string(static_cast<int>(m_currentMode)));
    }

    void MainMenuState::CreateArrowButtons() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // Create left arrow button
        m_leftArrowButtonEntity = m_ecsCoordinator->CreateEntity();
        float buttonY = m_screenHeight / 2.0f;     // Center vertically
        float buttonScale = m_isMobile ? 10.0f : 5.0f;  // Perfect mobile button size (scale factor fix applied)
        
        // Preload and query actual dimensions via shared RenderSystem
        int lwi = 0, lhi = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("LeftArrow");
            if (!rs->GetTextureSize("LeftArrow", lwi, lhi)) { lwi = 16; lhi = 16; }
        }
        float leftArrowWidth = static_cast<float>(lwi);  // Correct size 16x16
        float leftArrowHeight = static_cast<float>(lhi); // Correct size 16x16
        
        // Use positioning helper for left arrow
        auto leftButtonScaledDimensions = GetScaledDimensions(leftArrowWidth, leftArrowHeight, buttonScale);
        float leftButtonWidth = leftButtonScaledDimensions.first;
        float leftButtonHeight = leftButtonScaledDimensions.second;
        
        // Position left button with its LEFT EDGE closer to screen left edge  
        float leftButtonLeftEdge = m_screenWidth * 0.005f;  // 0.5% from left edge (closer)
        float leftButtonFinalX = leftButtonLeftEdge;  // Simple: top-left X is the left edge
        float leftButtonFinalY = buttonY - (leftButtonHeight / 2.0f);  // Center vertically around buttonY
        
        GN_LOG_INFO("⬅️ Left arrow: screenWidth=" + std::to_string(m_screenWidth) + 
                    ", leftEdge=" + std::to_string(leftButtonLeftEdge) + "(0.5%), buttonSize(" + 
                    std::to_string(leftButtonWidth) + "," + std::to_string(leftButtonHeight) + 
                    "), finalTopLeft(" + std::to_string(leftButtonFinalX) + "," + std::to_string(leftButtonFinalY) + ")");
        
        Transform leftTransform(Gnosis::GNVector2(leftButtonFinalX, leftButtonFinalY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite leftSprite("LeftArrow", leftArrowWidth, leftArrowHeight);
        leftSprite.layer = 5; // Top layer
        leftSprite.visible = false;
        UIElement leftButton("", "LeftArrow", "LeftArrowHover"); // Remove text, keep arrow sprite
        leftButton.fontSize = m_isMobile ? 42.0f : 21.0f;
        leftButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        leftButton.visible = false;
        m_ecsCoordinator->AddComponent<Transform>(m_leftArrowButtonEntity, leftTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_leftArrowButtonEntity, leftSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_leftArrowButtonEntity, leftButton);
        
        // Create right arrow button
        m_rightArrowButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Preload and query actual dimensions via shared RenderSystem
        int rwi = 0, rhi = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("RightArrow");
            if (!rs->GetTextureSize("RightArrow", rwi, rhi)) { rwi = 16; rhi = 16; }
        }
        float rightArrowWidth = static_cast<float>(rwi);  // Correct size 16x16
        float rightArrowHeight = static_cast<float>(rhi); // Correct size 16x16
        
        // Use positioning helper for right arrow - right edge at 2% from right screen edge
        auto rightButtonScaledDimensions = GetScaledDimensions(rightArrowWidth, rightArrowHeight, buttonScale);
        float rightButtonWidth = rightButtonScaledDimensions.first;
        float rightButtonHeight = rightButtonScaledDimensions.second;
        
        // Position right button with its RIGHT EDGE closer to screen right edge
        float rightButtonRightEdge = m_screenWidth * 0.995f;  // 99.5% from left = 0.5% from right (closer)
        float rightButtonFinalX = rightButtonRightEdge - rightButtonWidth;  // Subtract full width to get left edge
        float rightButtonFinalY = buttonY - (rightButtonHeight / 2.0f);  // Center vertically around buttonY
        
        GN_LOG_INFO("➡️ Right arrow: screenWidth=" + std::to_string(m_screenWidth) + 
                    ", rightEdge=" + std::to_string(rightButtonRightEdge) + "(99.5%), buttonSize(" + 
                    std::to_string(rightButtonWidth) + "," + std::to_string(rightButtonHeight) + 
                    "), finalTopLeft(" + std::to_string(rightButtonFinalX) + "," + std::to_string(rightButtonFinalY) + ")");
        
        Transform rightTransform(Gnosis::GNVector2(rightButtonFinalX, rightButtonFinalY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite rightSprite("RightArrow", rightArrowWidth, rightArrowHeight);
        rightSprite.layer = 5; // Top layer
        rightSprite.visible = false;
        UIElement rightButton("", "RightArrow", "RightArrowHover"); // Remove text, keep arrow sprite
        rightButton.fontSize = m_isMobile ? 42.0f : 21.0f;
        rightButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        rightButton.visible = false;
        m_ecsCoordinator->AddComponent<Transform>(m_rightArrowButtonEntity, rightTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_rightArrowButtonEntity, rightSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_rightArrowButtonEntity, rightButton);
        
        GN_LOG_INFO("Created arrow navigation buttons");
    }

    void MainMenuState::CreateLockedIndicator() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        float centerX = m_screenWidth / 2.0f;
        float centerY = m_screenHeight / 2.0f;
        float paintingScale = m_isMobile ? 6.0f : 2.0f;
        
        m_lockedIndicatorEntity = m_ecsCoordinator->CreateEntity();
        float yPos = centerY - (paintingScale * 100.0f); // Position above the painting
        
        Transform indicatorTransform(Gnosis::GNVector2(centerX, yPos), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        UIElement indicatorElement("[Locked!]", "", "");
        indicatorElement.fontSize = m_isMobile ? 48.0f : 24.0f; // Smaller to match new menu scale
        indicatorElement.textColor = Gnosis::GNColor(255, 0, 0, 255); // Red text
        indicatorElement.visible = false; // Initially hidden
        
        m_ecsCoordinator->AddComponent<Transform>(m_lockedIndicatorEntity, indicatorTransform);
        m_ecsCoordinator->AddComponent<UIElement>(m_lockedIndicatorEntity, indicatorElement);
        
        GN_LOG_INFO("🔒 Created single locked indicator");
    }

    void MainMenuState::CreateLevelPaintings() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // === SIMPLIFIED LEVEL PAINTING POSITIONING === //
        
        float screenCenterX = m_screenWidth / 2.0f;
        float screenCenterY = m_screenHeight / 2.0f;
        float paintingSpacing = m_isMobile ? 400.0f : (m_screenWidth * 0.8f); // Much smaller spacing for mobile
        float paintingScale = m_isMobile ? 6.0f : 2.0f;
        
        m_levelPaintingEntities.clear();
        m_levelFrameEntities.clear();
        m_levelTextEntities.clear();
        m_unlockButtonEntities.clear();
        m_requirementTextEntities.clear();
        
        // Pruned verbose creation log
        
        for (size_t i = 0; i < m_levels.size(); ++i) {
            // All paintings start at center position - UpdateLevelVisibility will handle positioning and visibility
            float paintingCenterX = screenCenterX;
            float paintingCenterY = screenCenterY;
            
            // === Create painting entity === //
            Gnosis::Entity paintingEntity = m_ecsCoordinator->CreateEntity();
            
            // Query texture size via shared RenderSystem
            int pW = 0, pH = 0;
            if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                // ensure texture was requested for preload (also done here for safety)
                rs->PreloadTexture(m_levels[i].paintingTexture);
                if (!rs->GetTextureSize(m_levels[i].paintingTexture, pW, pH)) { pW = 96; pH = 96; }
            }
            float paintingTextureWidth = static_cast<float>(pW);
            float paintingTextureHeight = static_cast<float>(pH);
            
            // Use SAME dynamic scaling logic as UpdateLevelVisibility to prevent decentering
            float maxWidth = m_screenWidth * 0.8f; // Use 80% of screen width
            float maxHeight = m_screenHeight * 0.4f; // Use 40% of screen height for paintings
            
            float scaleByWidth = maxWidth / paintingTextureWidth;
            float scaleByHeight = maxHeight / paintingTextureHeight;
            float dynamicScale = std::min(scaleByWidth, scaleByHeight); // Use smaller scale to fit both dimensions
            
            auto paintingScaledDimensions = GetScaledDimensions(paintingTextureWidth, paintingTextureHeight, dynamicScale);
            float paintingScaledWidth = paintingScaledDimensions.first;
            float paintingScaledHeight = paintingScaledDimensions.second;
            
            // Use SAME manual positioning logic as UpdateLevelVisibility to prevent decentering
            float manualTopLeftX = paintingCenterX - (paintingScaledWidth / 2.0f);
            float manualTopLeftY = paintingCenterY - (paintingScaledHeight / 2.0f);
            
            // Pruned placement debug logs
            
            Transform paintingTransform(Gnosis::GNVector2(manualTopLeftX, manualTopLeftY), 0.0f, Gnosis::GNVector2(dynamicScale, dynamicScale));
            Sprite paintingSprite(m_levels[i].paintingTexture, paintingTextureWidth, paintingTextureHeight);
            paintingSprite.layer = 3; // Above background, below UI
            paintingSprite.visible = false; // Initially hidden
            m_ecsCoordinator->AddComponent<Transform>(paintingEntity, paintingTransform);
            m_ecsCoordinator->AddComponent<Sprite>(paintingEntity, paintingSprite);
            m_levelPaintingEntities.push_back(paintingEntity);
            
            // Pruned per-painting creation summary
            
            // === Create frame entity for locked levels === //
            Gnosis::Entity frameEntity = m_ecsCoordinator->CreateEntity();
            
            // For perfect overlay, the locked frame should use EXACTLY the same dimensions and position as the painting
            // This ensures perfect synchronization between locked and unlocked paintings
            
            Transform frameTransform(Gnosis::GNVector2(manualTopLeftX, manualTopLeftY), 0.0f, Gnosis::GNVector2(dynamicScale, dynamicScale));
            std::string frameTexture = m_levels[i].isUnlocked ? "" : "LockedPainting";
            Sprite frameSprite(frameTexture, paintingTextureWidth, paintingTextureHeight); // Use SAME dimensions as painting
            frameSprite.layer = 4; // Above paintings
            frameSprite.visible = false;
            m_ecsCoordinator->AddComponent<Transform>(frameEntity, frameTransform);
            m_ecsCoordinator->AddComponent<Sprite>(frameEntity, frameSprite);
            m_levelFrameEntities.push_back(frameEntity);
            
            // === Create level name text entity === //
            Gnosis::Entity textEntity = m_ecsCoordinator->CreateEntity();

            // Position text at top of screen, centered horizontally on screen (not painting)
            float textY = m_screenHeight * 0.15f; // 15% down from top
            float textX = screenCenterX; // Use screen center for proper horizontal centering

            Transform textTransform(Gnosis::GNVector2(textX, textY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));

            // Create level display text with high score if available
            std::string displayText = m_levels[i].name;
            if (GameCore::GetGame()) {
                int highScore = GameCore::GetGame()->GetLevelHighScore(m_levels[i].levelNumber);
                if (highScore > 0) {
                    displayText += "\nBest: " + std::to_string(highScore) + " pipes";
                }
            }

            UIElement textElement(displayText, "", "");
            textElement.fontSize = m_isMobile ? 70.0f : (m_buttonFontSize * 4.0f); // Slightly smaller to fit high score
            textElement.textOutlineWidth = 8.0f; // consistent outline thickness
            textElement.textColor = Gnosis::GNColor(255, 255, 255, 255);
            textElement.centerTextHorizontally = true;
            textElement.centerTextVertically = true;
            textElement.visible = false;
            m_ecsCoordinator->AddComponent<Transform>(textEntity, textTransform);
            m_ecsCoordinator->AddComponent<UIElement>(textEntity, textElement);
            m_levelTextEntities.push_back(textEntity);

            // === Create unlock button and requirements for locked levels === //
            if (!m_levels[i].isUnlocked && GameCore::GetGame()) {
                // Create unlock button
                Gnosis::Entity unlockButtonEntity = m_ecsCoordinator->CreateEntity();

                // Position button directly below the current level painting using actual runtime dimensions
                float buttonScale = m_isMobile ? 8.0f : 6.0f; // Original button size

                // Get the actual current painting's position and dimensions
                float paintingBottomY = m_screenHeight / 2.0f; // Default fallback
                if (m_currentLevelIndex < m_levelPaintingEntities.size() && m_levelPaintingEntities[m_currentLevelIndex] != 0) {
                    if (auto* paintingTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[m_currentLevelIndex])) {
                        // Get the painting's actual dimensions by querying the texture size and scale
                        int pwi = 96, phi = 96; // Default fallback
                        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                            rs->GetTextureSize(m_levels[m_currentLevelIndex].paintingTexture, pwi, phi);
                        }
                        auto scaledDimensions = GetScaledDimensions(pwi, phi, paintingTransform->scale.x);
                        float paintingHeight = scaledDimensions.second;

                        // Calculate the bottom of the painting
                        paintingBottomY = paintingTransform->position.y + paintingHeight;
                    }
                }

                // Position button below the painting with proper spacing
                float buttonY = paintingBottomY + 89.0f; // 89px gap below painting (raised slightly)

                // Get button texture dimensions
                int btnW = 90, btnH = 16;
                if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                    rs->PreloadTexture("FloppyButtonBlue");
                    rs->GetTextureSize("FloppyButtonBlue", btnW, btnH);
                }

                auto buttonScaledDimensions = GetScaledDimensions(btnW, btnH, buttonScale);
                float buttonWidth = buttonScaledDimensions.first;
                float buttonHeight = buttonScaledDimensions.second;

                Gnosis::GNVector2 buttonPosition = CenterObjectAtPosition(textX, buttonY, buttonWidth, buttonHeight);

                Transform buttonTransform(Gnosis::GNVector2(buttonPosition.x, buttonPosition.y), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
                Sprite buttonSprite("FloppyButtonBlue", btnW, btnH);
                buttonSprite.layer = 3;
                buttonSprite.visible = false;

                UIElement buttonElement("UNLOCK", "FloppyButtonBlue", "FloppyButtonBlueHover");
                buttonElement.fontSize = m_globalUIFontSize; // Match main menu button font size (80.0f for mobile)
                buttonElement.textColor = Gnosis::GNColor(255, 255, 255, 255);
                buttonElement.centerTextHorizontally = true;
                buttonElement.centerTextVertically = true;
                buttonElement.visible = false;

                m_ecsCoordinator->AddComponent<Transform>(unlockButtonEntity, buttonTransform);
                m_ecsCoordinator->AddComponent<Sprite>(unlockButtonEntity, buttonSprite);
                m_ecsCoordinator->AddComponent<UIElement>(unlockButtonEntity, buttonElement);
                m_unlockButtonEntities.push_back(unlockButtonEntity);

                // Create requirements text below the button
                Gnosis::Entity reqTextEntity = m_ecsCoordinator->CreateEntity();

                float reqTextY = buttonY + buttonHeight / 2.0f + 99.0f; // Position below the button (drop full button height + 64px more spacing)
                Transform reqTextTransform(Gnosis::GNVector2(textX, reqTextY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));

                // Get level stats to show requirements
                const auto& levelStats = GameCore::GetGame()->GetLevelStats(m_levels[i].levelNumber);

                // Level 2 requirements are now permanently set to 0 in SetDefaultUnlockRequirements
                auto displayStats = levelStats; // Use the level stats as-is

                if (m_levels[i].levelNumber == 2) {  // Debug level 2 specifically
                    GN_LOG_INFO("🎨 MainMenuState: Level 2 stats from GetLevelStats - unlockRequirement=" + std::to_string(displayStats.unlockRequirement) +
                               ", coinRequirement=" + std::to_string(displayStats.coinRequirement) +
                               ", unlocked=" + std::to_string(displayStats.unlocked));
                }

                // Get current progress
                int currentCoins = GameCore::GetGame() ? GameCore::GetGame()->GetPlayerCoins() : 0;
                GN_LOG_INFO("💰 LEVEL " + std::to_string(i) + " REQUIREMENTS - currentCoins: " + std::to_string(currentCoins) + ", coinRequirement: " + std::to_string(displayStats.coinRequirement));
                if (currentCoins >= displayStats.coinRequirement && displayStats.coinRequirement > 0) {
                    GN_LOG_INFO("💰 LEVEL " + std::to_string(i) + " - COIN REQUIREMENT MET!");
                } else if (displayStats.coinRequirement > 0) {
                    GN_LOG_INFO("💰 LEVEL " + std::to_string(i) + " - COIN REQUIREMENT NOT MET (" + std::to_string(currentCoins) + "/" + std::to_string(displayStats.coinRequirement) + ")");
                }
                int prevLevelHighScore = 0;
                std::string prevLevelName = "Previous Level";

                if (displayStats.unlockRequirement > 0 && GameCore::GetGame()) {
                    // Get the high score from the required previous level
                    int requiredLevelId;
                    if (m_levels[i].levelNumber == 2) {
                        requiredLevelId = 1; // Sewer requires pipes from Park (level 1)
                    } else if (m_levels[i].levelNumber == 3) {
                        requiredLevelId = 2; // Desert requires pipes from Sewer (level 2)
                    } else if (m_levels[i].levelNumber == 4) {
                        requiredLevelId = 3; // Snow requires pipes from Desert (level 3)
                    } else if (m_levels[i].levelNumber == 5) {
                        requiredLevelId = 4; // Castle requires pipes from Snow (level 4)
                    } else if (m_levels[i].levelNumber == 6) {
                        requiredLevelId = 5; // Boss requires pipes from Castle (level 5)
                    } else {
                        requiredLevelId = m_levels[i].levelNumber - 1;
                    }

                    prevLevelHighScore = GameCore::GetGame()->GetLevelHighScore(requiredLevelId);

                    // Find the level name for the required level
                    for (size_t j = 0; j < m_levels.size(); ++j) {
                        if (m_levels[j].levelNumber == requiredLevelId) {
                            prevLevelName = m_levels[j].name;
                            break;
                        }
                    }
                }

                std::string requirementsText;

                if (displayStats.coinRequirement > 0 && displayStats.unlockRequirement > 0) {
                    // Both pipes and coins required (later levels)
                    requirementsText = std::to_string(prevLevelHighScore) + "/" + std::to_string(displayStats.unlockRequirement) + " pipes from\n'" + prevLevelName + "'\n" +
                                     std::to_string(currentCoins) + "/" + std::to_string(displayStats.coinRequirement) + " coins";
                } else if (displayStats.coinRequirement > 0) {
                    // Only coins required
                    requirementsText = std::to_string(currentCoins) + "/" + std::to_string(displayStats.coinRequirement) + " coins";
                } else if (displayStats.unlockRequirement > 0) {
                    // Only pipes required
                    requirementsText = std::to_string(prevLevelHighScore) + "/" + std::to_string(displayStats.unlockRequirement) + " pipes from\n'" + prevLevelName + "'";
                }

                UIElement reqTextElement(requirementsText, "", "");
                reqTextElement.fontSize = m_isMobile ? 40.0f : (m_buttonFontSize * 2.5f); // Lowered font size
                reqTextElement.textOutlineWidth = 6.0f;
                reqTextElement.textColor = Gnosis::GNColor(200, 200, 200, 255); // Light gray
                reqTextElement.centerTextHorizontally = true;
                reqTextElement.centerTextVertically = true;
                reqTextElement.visible = false;

                m_ecsCoordinator->AddComponent<Transform>(reqTextEntity, reqTextTransform);
                m_ecsCoordinator->AddComponent<UIElement>(reqTextEntity, reqTextElement);
                m_requirementTextEntities.push_back(reqTextEntity);
            } else {
                // For unlocked levels, add placeholder entities to maintain vector indices
                m_unlockButtonEntities.push_back(0);
                m_requirementTextEntities.push_back(0);
            }
        }
        
        GN_LOG_INFO("✅ Created %zu level paintings with simplified centered positioning", m_levels.size());
    }

    void MainMenuState::RefreshLevelDisplay() {
        GN_LOG_INFO("🚨🚨🚨 REFRESH_LEVEL_DISPLAY_CALLED 🚨🚨🚨");
        GN_LOG_INFO("🔄 RefreshLevelDisplay: STARTED - processing " + std::to_string(m_levels.size()) + " levels");
        GN_LOG_INFO("🔄 RefreshLevelDisplay: m_requirementTextEntities size = " + std::to_string(m_requirementTextEntities.size()));

        // Update level unlock status from game
        for (size_t i = 0; i < m_levels.size(); ++i) {
            bool currentUnlockStatus = m_levels[i].isUnlocked;
            bool gameUnlockStatus = GameCore::GetGame() ? GameCore::GetGame()->IsLevelUnlocked(m_levels[i].levelNumber) : false;

            // Update unlock status if it changed
            if (currentUnlockStatus != gameUnlockStatus) {
                m_levels[i].isUnlocked = gameUnlockStatus;

                // Update locked frame visibility
                if (i < m_levelFrameEntities.size()) {
                    Gnosis::Entity frameEntity = m_levelFrameEntities[i];
                    if (frameEntity != 0 && m_ecsCoordinator) {
                        Sprite* frameSprite = m_ecsCoordinator->GetComponent<Sprite>(frameEntity);
                        if (frameSprite) {
                            frameSprite->visible = !gameUnlockStatus;
                        }
                    }
                }

                // Update unlock button and requirement text visibility
                UpdateUnlockButtonVisibility(i, gameUnlockStatus);
            }

            // Update level text with current high score
            if (i < m_levelTextEntities.size()) {
                Gnosis::Entity textEntity = m_levelTextEntities[i];
                if (textEntity != 0 && m_ecsCoordinator) {
                    UIElement* textElement = m_ecsCoordinator->GetComponent<UIElement>(textEntity);
                    if (textElement) {
                        std::string displayText = m_levels[i].name;
                        if (GameCore::GetGame()) {
                            int highScore = GameCore::GetGame()->GetLevelHighScore(m_levels[i].levelNumber);
                            if (highScore > 0) {
                                displayText += "\nBest: " + std::to_string(highScore) + " pipes";
                            }
                        }
                        textElement->buttonText = displayText;
                    }
                }
            }

            // Update requirements text with current progress
            if (m_levels[i].levelNumber == 2) {  // Debug level 2 specifically
                GN_LOG_INFO("🔄 RefreshLevelDisplay: Level 2 - checking requirement text entity. Array size: " + std::to_string(m_requirementTextEntities.size()) +
                           ", index: " + std::to_string(i) + ", entity: " + std::to_string(i < m_requirementTextEntities.size() ? m_requirementTextEntities[i] : 0) +
                           ", isUnlocked: " + std::to_string(m_levels[i].isUnlocked));
            }

            if (i < m_requirementTextEntities.size() && m_requirementTextEntities[i] != 0) {
                Gnosis::Entity reqTextEntity = m_requirementTextEntities[i];
                if (reqTextEntity != 0 && m_ecsCoordinator && !m_levels[i].isUnlocked) {
                    UIElement* reqTextElement = m_ecsCoordinator->GetComponent<UIElement>(reqTextEntity);
                    if (reqTextElement && GameCore::GetGame()) {
                        const auto& levelStats = GameCore::GetGame()->GetLevelStats(m_levels[i].levelNumber);

                        // Level 2 requirements are now permanently set to 0 in SetDefaultUnlockRequirements
                        auto displayStats = levelStats; // Use the level stats as-is

                        if (m_levels[i].levelNumber == 2) {  // Debug level 2 specifically during refresh
                            GN_LOG_INFO("🔄 RefreshLevelDisplay: Level 2 stats from GetLevelStats - unlockRequirement=" + std::to_string(displayStats.unlockRequirement) +
                                       ", coinRequirement=" + std::to_string(displayStats.coinRequirement) +
                                       ", unlocked=" + std::to_string(displayStats.unlocked));
                        }

                        // Get current progress
                        int currentCoins = GameCore::GetGame()->GetPlayerCoins();
                        GN_LOG_INFO("🔄 REFRESH LEVEL " + std::to_string(i) + " - Current coins: " + std::to_string(currentCoins) + ", coinRequirement: " + std::to_string(displayStats.coinRequirement));
                        int prevLevelHighScore = 0;
                        std::string prevLevelName = "Previous Level";

                        if (displayStats.unlockRequirement > 0 && GameCore::GetGame()) {
                            // Get the high score from the required previous level
                            int requiredLevelId;
                            if (m_levels[i].levelNumber == 2) {
                                requiredLevelId = 1; // Sewer requires pipes from Park (level 1)
                            } else if (m_levels[i].levelNumber == 3) {
                                requiredLevelId = 2; // Desert requires pipes from Sewer (level 2)
                            } else if (m_levels[i].levelNumber == 4) {
                                requiredLevelId = 3; // Snow requires pipes from Desert (level 3)
                            } else if (m_levels[i].levelNumber == 5) {
                                requiredLevelId = 4; // Castle requires pipes from Snow (level 4)
                            } else if (m_levels[i].levelNumber == 6) {
                                requiredLevelId = 5; // Boss requires pipes from Castle (level 5)
                            } else {
                                requiredLevelId = m_levels[i].levelNumber - 1;
                            }

                            prevLevelHighScore = GameCore::GetGame()->GetLevelHighScore(requiredLevelId);

                            // Find the level name for the required level
                            for (size_t j = 0; j < m_levels.size(); ++j) {
                                if (m_levels[j].levelNumber == requiredLevelId) {
                                    prevLevelName = m_levels[j].name;
                                    break;
                                }
                            }
                        }

                        std::string requirementsText;

                        if (displayStats.coinRequirement > 0 && displayStats.unlockRequirement > 0) {
                            // Both pipes and coins required (later levels)
                            requirementsText = std::to_string(prevLevelHighScore) + "/" + std::to_string(displayStats.unlockRequirement) + " pipes from\n'" + prevLevelName + "'\n" +
                                             std::to_string(currentCoins) + "/" + std::to_string(displayStats.coinRequirement) + " coins";
                        } else if (displayStats.coinRequirement > 0) {
                            // Only coins required
                            requirementsText = std::to_string(currentCoins) + "/" + std::to_string(displayStats.coinRequirement) + " coins";
                        } else if (displayStats.unlockRequirement > 0) {
                            // Only pipes required
                            requirementsText = std::to_string(prevLevelHighScore) + "/" + std::to_string(displayStats.unlockRequirement) + " pipes from\n'" + prevLevelName + "'";
                        }

                        reqTextElement->buttonText = requirementsText;
                    }
                }
            }
        }

        GN_LOG_INFO("Refreshed level display with updated unlock status and high scores");
    }

    void MainMenuState::UpdateUnlockButtonVisibility(size_t levelIndex, bool isUnlocked) {
        // Hide/show unlock button and requirements text based on unlock status
        if (levelIndex < m_unlockButtonEntities.size() && m_unlockButtonEntities[levelIndex] != 0) {
            if (m_ecsCoordinator) {
                Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_unlockButtonEntities[levelIndex]);
                UIElement* buttonElement = m_ecsCoordinator->GetComponent<UIElement>(m_unlockButtonEntities[levelIndex]);
                if (buttonSprite) buttonSprite->visible = !isUnlocked;
                if (buttonElement) buttonElement->visible = !isUnlocked;
            }
        }

        if (levelIndex < m_requirementTextEntities.size() && m_requirementTextEntities[levelIndex] != 0) {
            if (m_ecsCoordinator) {
                UIElement* reqTextElement = m_ecsCoordinator->GetComponent<UIElement>(m_requirementTextEntities[levelIndex]);
                if (reqTextElement) reqTextElement->visible = !isUnlocked;
            }
        }
    }

    void MainMenuState::OnUnlockButtonPressed(int levelNumber) {
        GN_LOG_INFO("🎮 OnUnlockButtonPressed called for level " + std::to_string(levelNumber));

        // Check debounce timer
        if (m_lastUnlockPressTime < m_unlockDebounceDelay) {
            GN_LOG_INFO("🚫 Unlock button debounced - too soon since last press");
            return;
        }

        // Reset debounce timer
        m_lastUnlockPressTime = 0.0f;
        GN_LOG_INFO("✅ Unlock button debounce passed");

        if (!GameCore::GetGame()) {
            GN_LOG_ERROR("❌ GameCore::GetGame() returned null!");
            return;
        }

        GN_LOG_INFO("🔓 Attempting to unlock level " + std::to_string(levelNumber));

        // Try to unlock the level using the new unified unlock system
        bool unlockResult = GameCore::GetGame()->TryUnlockLevel(levelNumber);

        if (unlockResult) {
            GN_LOG_INFO("✅ TryUnlockLevel returned true for level " + std::to_string(levelNumber));

            // Find the level index and immediately update the frame sprite for visual feedback
            for (size_t i = 0; i < m_levels.size(); ++i) {
                if (m_levels[i].levelNumber == levelNumber) {
                    m_levels[i].isUnlocked = true;

                    // Immediately update the frame sprite to hide the locked overlay
                    if (i < m_levelFrameEntities.size() && m_levelFrameEntities[i] != 0 && m_ecsCoordinator) {
                        Sprite* frameSprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelFrameEntities[i]);
                        if (frameSprite) {
                            frameSprite->visible = false;
                            GN_LOG_INFO("🎨 Immediately updated frame sprite for level " + std::to_string(levelNumber));
                        }
                    }

                    UpdateUnlockButtonVisibility(i, true);
                    GN_LOG_INFO("📱 Updated UI for level " + std::to_string(levelNumber));
                    break;
                }
            }

            // Update the UI to reflect the change
            RefreshLevelDisplay();
            GN_LOG_INFO("🎨 Refreshed level display after unlock");

        } else {
            // SIMPLIFIED: Always play denied sound since OnUnlockButtonPressed 
            // is only called from visible button clicks
            GN_LOG_INFO("❌ TryUnlockLevel returned false for level " + std::to_string(levelNumber));
            GN_LOG_INFO("🔊 Playing denied sound effect (unlock failed)");
            
            if (GameCore::GetGame()) {
                GameCore::GetGame()->PlaySFX("denied");
            }
        }
    }

    void MainMenuState::CreateBackButton() {
        if (!m_ecsCoordinator) {
            return;
        }
        // Create back button entity
        m_backButtonEntity = m_ecsCoordinator->CreateEntity();
        float centerX = m_screenWidth * 0.5f;
        float buttonY = m_screenHeight * 0.93f;  // near bottom
        float buttonScale = m_isMobile ? 10.0f : 4.0f; // Match main menu button scale (10.0f for mobile)

        // Query texture via shared RenderSystem
        int bw = 0, bh = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyButtonBlue");
            if (!rs->GetTextureSize("FloppyButtonBlue", bw, bh)) { bw = 90; bh = 16; }
        }
        float buttonTexW = static_cast<float>(bw);
        float buttonTexH = static_cast<float>(bh);
        auto scaled = GetScaledDimensions(buttonTexW, buttonTexH, buttonScale);
        float btnW = scaled.first;
        float btnH = scaled.second;
        float topLeftX = centerX - btnW * 0.5f;
        float topLeftY = buttonY - btnH * 0.5f;

        Transform t(Gnosis::GNVector2(topLeftX, topLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite s("FloppyButtonBlue", buttonTexW, buttonTexH); s.layer = 5; s.visible = false;
        UIElement ui("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
        ui.fontSize = m_isMobile ? 80.0f : 20.0f; // Match main menu button font size (80.0f for mobile)
        ui.textColor = Gnosis::GNColor(255, 255, 255, 255);
        ui.centerTextHorizontally = true;
        ui.centerTextVertically = true;
        ui.visible = false;

        m_ecsCoordinator->AddComponent<Transform>(m_backButtonEntity, t);
        m_ecsCoordinator->AddComponent<Sprite>(m_backButtonEntity, s);
        m_ecsCoordinator->AddComponent<UIElement>(m_backButtonEntity, ui);

        GN_LOG_INFO("Created back button for level select");
    }

    void MainMenuState::CreateLevelPlayButton() {
        if (!m_ecsCoordinator) {
            return;
        }
        // Create or reuse entity
        if (m_levelPlayButtonEntity == 0) {
            m_levelPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        }

        float centerX = m_screenWidth * 0.5f;
        // Place slightly above the back button
        float buttonY = m_screenHeight * 0.86f;
        float buttonScale = m_isMobile ? 10.0f : 4.0f; // Match main menu button scale (10.0f for mobile)

        // Query texture via shared RenderSystem
        int bw = 0, bh = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyButtonBlue");
            if (!rs->GetTextureSize("FloppyButtonBlue", bw, bh)) { bw = 90; bh = 16; }
        }
        float texW = static_cast<float>(bw);
        float texH = static_cast<float>(bh);
        auto scaled = GetScaledDimensions(texW, texH, buttonScale);
        float w = scaled.first;
        float h = scaled.second;
        float topLeftX = centerX - w * 0.5f;
        float topLeftY = buttonY - h * 0.5f;

        Transform t(Gnosis::GNVector2(topLeftX, topLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite s("FloppyButtonBlue", texW, texH); s.layer = 5; s.visible = false;
        UIElement ui("PLAY LEVEL", "FloppyButtonBlue", "FloppyButtonBlueHover");
        ui.fontSize = m_isMobile ? 80.0f : 20.0f; // Match main menu button font size (80.0f for mobile)
        ui.textColor = Gnosis::GNColor(255, 255, 255, 255);
        ui.centerTextHorizontally = true;
        ui.centerTextVertically = true;
        ui.visible = false;

        if (!m_ecsCoordinator->HasComponent<Transform>(m_levelPlayButtonEntity)) m_ecsCoordinator->AddComponent<Transform>(m_levelPlayButtonEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(m_levelPlayButtonEntity) = t;
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_levelPlayButtonEntity)) m_ecsCoordinator->AddComponent<Sprite>(m_levelPlayButtonEntity, s); else *m_ecsCoordinator->GetComponent<Sprite>(m_levelPlayButtonEntity) = s;
        if (!m_ecsCoordinator->HasComponent<UIElement>(m_levelPlayButtonEntity)) m_ecsCoordinator->AddComponent<UIElement>(m_levelPlayButtonEntity, ui); else *m_ecsCoordinator->GetComponent<UIElement>(m_levelPlayButtonEntity) = ui;

        GN_LOG_INFO("Created level play button");
    }

    void MainMenuState::CreateOptionsTracksAndLabels() {
        if (!m_ecsCoordinator) return;

        // Title centered at top of overlay
        if (m_optionsTitleEntity == 0) m_optionsTitleEntity = m_ecsCoordinator->CreateEntity();
        {
            float titleX = m_optionsOverlayX + m_optionsOverlayW * 0.5f;
            float titleY = m_optionsOverlayY + m_optionsOverlayH * 0.06f;
            Transform t(Gnosis::GNVector2(titleX, titleY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            Sprite s; s.visible = false; s.layer = 4; // text-only
            UIElement ui("OPTIONS", "", "");
            ui.fontSize = m_isMobile ? 72.0f : 42.0f; // Increased font size
            ui.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
            ui.centerTextHorizontally = true;
            ui.centerTextVertically = true;
            ui.visible = true;
            if (!m_ecsCoordinator->HasComponent<Transform>(m_optionsTitleEntity)) m_ecsCoordinator->AddComponent<Transform>(m_optionsTitleEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(m_optionsTitleEntity) = t;
            if (!m_ecsCoordinator->HasComponent<Sprite>(m_optionsTitleEntity)) m_ecsCoordinator->AddComponent<Sprite>(m_optionsTitleEntity, s); else *m_ecsCoordinator->GetComponent<Sprite>(m_optionsTitleEntity) = s;
            if (!m_ecsCoordinator->HasComponent<UIElement>(m_optionsTitleEntity)) m_ecsCoordinator->AddComponent<UIElement>(m_optionsTitleEntity, ui); else *m_ecsCoordinator->GetComponent<UIElement>(m_optionsTitleEntity) = ui;
        }

        // Difficulty row (label centered); arrows added via helper
        if (m_difficultyTextEntity == 0) m_difficultyTextEntity = m_ecsCoordinator->CreateEntity();
        if (m_difficultyValueEntity == 0) m_difficultyValueEntity = m_ecsCoordinator->CreateEntity();
        {
            // Position difficulty section below SFX slider with proper spacing
            float diffLabelY = m_screenHeight * 0.57f; // Label at 57% down (moved up a smidge)
            float diffValueY = m_screenHeight * 0.665f; // Value at 66.5% down (moved up proportionally)
            float centerX = m_optionsOverlayX + m_optionsOverlayW * 0.5f;
            // Static text "DIFFICULTY"
            Transform t(Gnosis::GNVector2(centerX, diffLabelY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            Sprite s; s.visible = true; s.layer = 4;
            UIElement ui("DIFFICULTY", "", "");
            ui.fontSize = m_isMobile ? 54.0f : 32.0f; // Increased font size
            ui.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
            ui.centerTextHorizontally = true; ui.centerTextVertically = true; ui.visible = true;
            if (!m_ecsCoordinator->HasComponent<Transform>(m_difficultyTextEntity)) m_ecsCoordinator->AddComponent<Transform>(m_difficultyTextEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(m_difficultyTextEntity) = t;
            if (!m_ecsCoordinator->HasComponent<Sprite>(m_difficultyTextEntity)) m_ecsCoordinator->AddComponent<Sprite>(m_difficultyTextEntity, s); else *m_ecsCoordinator->GetComponent<Sprite>(m_difficultyTextEntity) = s;
            if (!m_ecsCoordinator->HasComponent<UIElement>(m_difficultyTextEntity)) m_ecsCoordinator->AddComponent<UIElement>(m_difficultyTextEntity, ui); else *m_ecsCoordinator->GetComponent<UIElement>(m_difficultyTextEntity) = ui;

            // Current difficulty value centered between arrows
            Transform vt(Gnosis::GNVector2(centerX, diffValueY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            Sprite vs; vs.visible = true; vs.layer = 4;
            // Initialize with actual current difficulty instead of hardcoded placeholder
            std::string currentDifficultyName = GameCore::LevelManager::GetDifficultyName();
            UIElement vei(currentDifficultyName.c_str(), "", ""); // Use actual current difficulty
            vei.fontSize = m_isMobile ? 60.0f : 36.0f; // Increased font size
            vei.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
            vei.centerTextHorizontally = true; vei.centerTextVertically = true; vei.visible = true;
            if (!m_ecsCoordinator->HasComponent<Transform>(m_difficultyValueEntity)) m_ecsCoordinator->AddComponent<Transform>(m_difficultyValueEntity, vt); else *m_ecsCoordinator->GetComponent<Transform>(m_difficultyValueEntity) = vt;
            if (!m_ecsCoordinator->HasComponent<Sprite>(m_difficultyValueEntity)) m_ecsCoordinator->AddComponent<Sprite>(m_difficultyValueEntity, vs); else *m_ecsCoordinator->GetComponent<Sprite>(m_difficultyValueEntity) = vs;
            if (!m_ecsCoordinator->HasComponent<UIElement>(m_difficultyValueEntity)) m_ecsCoordinator->AddComponent<UIElement>(m_difficultyValueEntity, vei); else *m_ecsCoordinator->GetComponent<UIElement>(m_difficultyValueEntity) = vei;

            // Place arrows around the value
            CreateOptionsArrows(m_optionsOverlayX, m_optionsOverlayY, m_optionsOverlayW, m_optionsOverlayH, diffValueY);
        }

        // Helper to create a slider track rectangle and a label above it
        auto ensureTrackAndLabel = [&](Gnosis::Entity& trackEntity, Gnosis::Entity& labelEntity, const char* labelText, float trackY){
            // Track shape entity
            if (trackEntity == 0) trackEntity = m_ecsCoordinator->CreateEntity();
            {
                Transform t(Gnosis::GNVector2(m_optionsSliderX, trackY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                Sprite s; s.visible = false; s.layer = 3;
                UIShape shape; shape.visible = true; shape.width = m_optionsSliderW; shape.height = m_optionsSliderH;
                if (!m_ecsCoordinator->HasComponent<Transform>(trackEntity)) m_ecsCoordinator->AddComponent<Transform>(trackEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(trackEntity) = t;
                if (!m_ecsCoordinator->HasComponent<Sprite>(trackEntity)) m_ecsCoordinator->AddComponent<Sprite>(trackEntity, s); else *m_ecsCoordinator->GetComponent<Sprite>(trackEntity) = s;
                if (!m_ecsCoordinator->HasComponent<UIShape>(trackEntity)) m_ecsCoordinator->AddComponent<UIShape>(trackEntity, shape); else *m_ecsCoordinator->GetComponent<UIShape>(trackEntity) = shape;
            }
            // Label above track
            if (labelEntity == 0) labelEntity = m_ecsCoordinator->CreateEntity();
            {
                float labelY = trackY - (m_isMobile ? 61.0f : 33.0f); // Increased from 36/18 to 61/33 (+25px) to push track down more
                float labelX = m_optionsSliderX;
                Transform t(Gnosis::GNVector2(labelX, labelY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
                Sprite s; s.visible = true; s.layer = 4;
                UIElement ui(labelText, "", "");
                ui.fontSize = m_isMobile ? 54.0f : 32.0f; // Increased font size
                ui.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
                ui.centerTextHorizontally = false; ui.centerTextVertically = true; ui.visible = true;
                if (!m_ecsCoordinator->HasComponent<Transform>(labelEntity)) m_ecsCoordinator->AddComponent<Transform>(labelEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(labelEntity) = t;
                if (!m_ecsCoordinator->HasComponent<Sprite>(labelEntity)) m_ecsCoordinator->AddComponent<Sprite>(labelEntity, s); else *m_ecsCoordinator->GetComponent<Sprite>(labelEntity) = s;
                if (!m_ecsCoordinator->HasComponent<UIElement>(labelEntity)) m_ecsCoordinator->AddComponent<UIElement>(labelEntity, ui); else *m_ecsCoordinator->GetComponent<UIElement>(labelEntity) = ui;
            }
        };

        // Compute track Y positions to match UpdateOptionsKnobPositions()
        float currentY = m_screenHeight * 0.18f;
        float labelTrackGap = 15.0f * m_uiScale;
        float sectionGap = 25.0f * m_uiScale;
        float masterTrackY = currentY + labelTrackGap;
        currentY = masterTrackY + m_optionsSliderH + sectionGap;
        float musicTrackY = currentY + labelTrackGap;
        currentY = musicTrackY + m_optionsSliderH + sectionGap;
        float sfxTrackY = currentY + labelTrackGap;

        ensureTrackAndLabel(m_masterTrackEntity, m_masterLabelEntity, "MASTER", masterTrackY);
        ensureTrackAndLabel(m_musicTrackEntity,  m_musicLabelEntity,  "MUSIC",  musicTrackY);
        ensureTrackAndLabel(m_sfxTrackEntity,    m_sfxLabelEntity,    "SFX",    sfxTrackY);

        // After creating tracks/labels, ensure knob positions are consistent
        UpdateOptionsKnobPositions();
        
        // Vibration toggle row (below difficulty section)
        if (m_vibrationLabelEntity == 0) m_vibrationLabelEntity = m_ecsCoordinator->CreateEntity();
        if (m_vibrationToggleEntity == 0) m_vibrationToggleEntity = m_ecsCoordinator->CreateEntity();
        
        // Load current vibration state from game
        if (m_game) {
            m_vibrationsEnabled = m_game->GetVibrationsEnabled();
        }
        
        float vibrationLabelY = m_screenHeight * 0.745f + 15.0f;  // Adjusted to position between back button and difficulty labels (moved down 15px more)
        float centerX = m_optionsOverlayX + m_optionsOverlayW * 0.5f;
        
        // Label "VIBRATION" on left side - match X position of track labels
        {
            float labelX = m_optionsSliderX;  // Same X as track labels
            Transform t(Gnosis::GNVector2(labelX, vibrationLabelY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            Sprite s; s.visible = false; s.layer = 4;
            UIElement ui("VIBRATIONS", "", "");
            ui.fontSize = m_isMobile ? 54.0f : 32.0f;
            ui.textColor = Gnosis::GNColor(255, 255, 255, 255);
            ui.centerTextHorizontally = false;
            ui.centerTextVertically = true;
            ui.visible = true;
            if (!m_ecsCoordinator->HasComponent<Transform>(m_vibrationLabelEntity)) m_ecsCoordinator->AddComponent<Transform>(m_vibrationLabelEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(m_vibrationLabelEntity) = t;
            if (!m_ecsCoordinator->HasComponent<Sprite>(m_vibrationLabelEntity)) m_ecsCoordinator->AddComponent<Sprite>(m_vibrationLabelEntity, s); else *m_ecsCoordinator->GetComponent<Sprite>(m_vibrationLabelEntity) = s;
            if (!m_ecsCoordinator->HasComponent<UIElement>(m_vibrationLabelEntity)) m_ecsCoordinator->AddComponent<UIElement>(m_vibrationLabelEntity, ui); else *m_ecsCoordinator->GetComponent<UIElement>(m_vibrationLabelEntity) = ui;
        }
        
        // Toggle button (X sprite) on right side - configured as a proper toggle button
        {
            float toggleX = m_optionsOverlayX + m_optionsOverlayW * 0.75f;  // 75% across (further right)
            
            // Textures are actually 16x16, not 64x64
            float xBtnW = 16.0f;  // Actual texture width
            float xBtnH = 16.0f;  // Actual texture height
            float toggleScale = 7.0f;
            
            // Center toggle button vertically with the label (button height is 16 * 7 = 112px)
            float scaledButtonHeight = xBtnH * toggleScale;
            float toggleY = vibrationLabelY - (scaledButtonHeight * 0.5f);  // Center button with label
            
            Transform t(Gnosis::GNVector2(toggleX, toggleY), 0.0f, Gnosis::GNVector2(toggleScale, toggleScale));
            Sprite s;
            s.textureId = m_vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected";
            s.width = xBtnW;
            s.height = xBtnH;
            s.visible = true;
            s.layer = 4;
            
            // Configure as a proper toggle button with toggle-specific fields
            UIElement ui("", 
                m_vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected",
                m_vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected");
            ui.visible = true;
            ui.isEnabled = true;
            ui.isToggle = true;  // Mark as toggle button
            ui.toggleState = m_vibrationsEnabled;  // Set initial state
            ui.toggleOnTexture = "xbuttonselected";   // ON texture
            ui.toggleOffTexture = "xbuttonunselected"; // OFF texture
            
            if (!m_ecsCoordinator->HasComponent<Transform>(m_vibrationToggleEntity)) m_ecsCoordinator->AddComponent<Transform>(m_vibrationToggleEntity, t); else *m_ecsCoordinator->GetComponent<Transform>(m_vibrationToggleEntity) = t;
            if (!m_ecsCoordinator->HasComponent<Sprite>(m_vibrationToggleEntity)) {
                m_ecsCoordinator->AddComponent<Sprite>(m_vibrationToggleEntity, s);
            } else {
                // Update existing sprite texture to reflect current state
                auto existingSprite = m_ecsCoordinator->GetComponent<Sprite>(m_vibrationToggleEntity);
                *existingSprite = s;
            }
            if (!m_ecsCoordinator->HasComponent<UIElement>(m_vibrationToggleEntity)) {
                m_ecsCoordinator->AddComponent<UIElement>(m_vibrationToggleEntity, ui);
            } else {
                // Update existing UI element texture to reflect current state
                auto existingUI = m_ecsCoordinator->GetComponent<UIElement>(m_vibrationToggleEntity);
                *existingUI = ui;
            }
        }
        
        GN_LOG_INFO("Created vibration toggle in options menu");
    }

    void MainMenuState::ShowLevelSelect() {
        GN_LOG_INFO("📋 ShowLevelSelect() called - hiding main menu, showing level select UI");
        m_currentMode = MenuMode::LEVEL_SELECT;

        // Refresh level unlock status and high scores before showing
        RefreshLevelDisplay();

        // Hide main menu elements when OPTIONS is active too
        if (m_logoEntity != 0) {
            Sprite* logoSprite = m_ecsCoordinator->GetComponent<Sprite>(m_logoEntity);
            if (logoSprite) logoSprite->visible = false;
        }
        if (m_fButtonEntity != 0) {
            Sprite* fSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
            if (fSprite) fSprite->visible = false;
        }
        
        // Hide main menu buttons
        std::vector<Gnosis::Entity> mainButtons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_leaderboardButtonEntity};
        for (Gnosis::Entity entity : mainButtons) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (sprite) sprite->visible = false;
                if (uiElement) uiElement->visible = false;
            }
        }
        
        // Hide ad controls button and version text in level select
        if (m_adControlsButtonEntity != 0) {
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_adControlsButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsButtonEntity);
            if (sprite) sprite->visible = false;
            if (uiElement) uiElement->visible = false;
        }
        if (m_versionTextEntity != 0) {
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_versionTextEntity);
            if (uiElement) uiElement->visible = false;
        }
        
        GN_LOG_INFO("📋 ShowLevelSelect: Ad controls button and version text hidden");
        
        // Show level select elements
        UpdateLevelVisibility();
        // Initialize pan spacing based on current layout (keep pixel values per project rules)
        float screenCenterX = m_screenWidth / 2.0f;
        // Estimate spacing as 90% of screen width to give nice overlap hint
        m_levelSpacing = m_screenWidth * 0.9f;
        m_currentOffsetX = 0.0f;
        m_targetOffsetX = 0.0f;
        m_isPanning = false;
        
        if (m_backButtonEntity != 0) {
            Sprite* backSprite = m_ecsCoordinator->GetComponent<Sprite>(m_backButtonEntity);
            UIElement* backElement = m_ecsCoordinator->GetComponent<UIElement>(m_backButtonEntity);
            if (backSprite) backSprite->visible = true;
            if (backElement) backElement->visible = true;
        }
        
        if (m_levelPlayButtonEntity != 0) {
            Sprite* playSprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPlayButtonEntity);
            UIElement* playElement = m_ecsCoordinator->GetComponent<UIElement>(m_levelPlayButtonEntity);
            if (playSprite) playSprite->visible = true;
            if (playElement) playElement->visible = true;
        }
        
        // Show arrow buttons
        if (m_leftArrowButtonEntity != 0) {
            Sprite* leftSprite = m_ecsCoordinator->GetComponent<Sprite>(m_leftArrowButtonEntity);
            UIElement* leftElement = m_ecsCoordinator->GetComponent<UIElement>(m_leftArrowButtonEntity);
            if (leftSprite) leftSprite->visible = true;
            if (leftElement) leftElement->visible = true;
        }
        
        if (m_rightArrowButtonEntity != 0) {
            Sprite* rightSprite = m_ecsCoordinator->GetComponent<Sprite>(m_rightArrowButtonEntity);
            UIElement* rightElement = m_ecsCoordinator->GetComponent<UIElement>(m_rightArrowButtonEntity);
            if (rightSprite) rightSprite->visible = true;
            if (rightElement) rightElement->visible = true;
        }
    }

    void MainMenuState::HideLevelSelect() {
        GN_LOG_INFO("Hiding level select menu - returning to main menu");
        m_currentMode = MenuMode::MAIN_MENU;
        
        // Show main menu elements (this will show ad controls and version since mode is MAIN_MENU)
        SetMainMenuVisible(true);
        
        GN_LOG_INFO("📋 HideLevelSelect: SetMainMenuVisible(true) called - ad controls and version should be visible");
        if (m_logoEntity != 0) {
            Sprite* logoSprite = m_ecsCoordinator->GetComponent<Sprite>(m_logoEntity);
            if (logoSprite) logoSprite->visible = true;
        }
        if (m_fButtonEntity != 0) {
            Sprite* fSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
            if (fSprite) fSprite->visible = true;
        }
        
        // Show main menu buttons
        std::vector<Gnosis::Entity> mainButtons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_leaderboardButtonEntity};
        for (Gnosis::Entity entity : mainButtons) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (sprite) sprite->visible = true;
                if (uiElement) uiElement->visible = true;
            }
        }
        
        // Hide level select elements
        for (Gnosis::Entity entity : m_levelPaintingEntities) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) sprite->visible = false;
            }
        }
        
        for (Gnosis::Entity entity : m_levelFrameEntities) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) sprite->visible = false;
            }
        }
        
        for (Gnosis::Entity entity : m_levelTextEntities) {
            if (entity != 0) {
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) uiElement->visible = false;
            }
        }

        // Hide unlock buttons and requirement text
        for (Gnosis::Entity entity : m_unlockButtonEntities) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (sprite) sprite->visible = false;
                if (uiElement) uiElement->visible = false;
            }
        }

        for (Gnosis::Entity entity : m_requirementTextEntities) {
            if (entity != 0) {
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) uiElement->visible = false;
            }
        }

        if (m_backButtonEntity != 0) {
            Sprite* backSprite = m_ecsCoordinator->GetComponent<Sprite>(m_backButtonEntity);
            UIElement* backElement = m_ecsCoordinator->GetComponent<UIElement>(m_backButtonEntity);
            if (backSprite) backSprite->visible = false;
            if (backElement) backElement->visible = false;
        }
        
        if (m_levelPlayButtonEntity != 0) {
            Sprite* playSprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPlayButtonEntity);
            UIElement* playElement = m_ecsCoordinator->GetComponent<UIElement>(m_levelPlayButtonEntity);
            if (playSprite) playSprite->visible = false;
            if (playElement) playElement->visible = false;
        }
        
        // Hide arrow buttons
        if (m_leftArrowButtonEntity != 0) {
            Sprite* leftSprite = m_ecsCoordinator->GetComponent<Sprite>(m_leftArrowButtonEntity);
            UIElement* leftElement = m_ecsCoordinator->GetComponent<UIElement>(m_leftArrowButtonEntity);
            if (leftSprite) leftSprite->visible = false;
            if (leftElement) leftElement->visible = false;
        }
        
        if (m_rightArrowButtonEntity != 0) {
            Sprite* rightSprite = m_ecsCoordinator->GetComponent<Sprite>(m_rightArrowButtonEntity);
            UIElement* rightElement = m_ecsCoordinator->GetComponent<UIElement>(m_rightArrowButtonEntity);
            if (rightSprite) rightSprite->visible = false;
            if (rightElement) rightElement->visible = false;
        }
        
        // Hide locked indicator to prevent it showing on main menu
        if (m_lockedIndicatorEntity != 0) {
            UIElement* lockedElement = m_ecsCoordinator->GetComponent<UIElement>(m_lockedIndicatorEntity);
            if (lockedElement) lockedElement->visible = false;
        }
    }

    void MainMenuState::HandleLevelSelectInput() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        if (!m_game) {
            return;
        }
        
        const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
        
        // Handle horizontal pan using primary touch with our unified input
        bool justPressed = delegates.input.isPrimaryInputJustPressed ? delegates.input.isPrimaryInputJustPressed() : false;
        bool inputDown   = delegates.input.isPrimaryInputDown        ? delegates.input.isPrimaryInputDown()        : false;
        bool justReleased= delegates.input.isPrimaryInputJustReleased? delegates.input.isPrimaryInputJustReleased(): false;
        float touchX = 0.0f, touchY = 0.0f;
        if (delegates.input.getPrimaryInputPosition) {
            delegates.input.getPrimaryInputPosition(&touchX, &touchY);
        }

        // Pan begin: anywhere within the paintings vertical band
        float bandTop = m_screenHeight * 0.25f;
        float bandBottom = m_screenHeight * 0.75f;
        if (justPressed && touchY >= bandTop && touchY <= bandBottom) {
            m_isPanning = true;
            m_panStartX = touchX;
            m_panStartY = touchY;
            m_panStartOffsetX = m_currentOffsetX;
            m_lastPanX = touchX;
            // Immediately update positions and make neighbors visible when pan begins
            UpdateLevelPanPositions();
        }
        // Pan move
        if (m_isPanning && inputDown) {
            float delta = touchX - m_panStartX;
            m_currentOffsetX = m_panStartOffsetX + delta;
            m_lastPanX = touchX;
            // Clamp soft bounds so neighbor exists just off-screen
            float maxOffset = m_levelSpacing * (m_currentLevelIndex);
            float minOffset = -m_levelSpacing * ((int)m_levels.size() - 1 - m_currentLevelIndex);
            m_currentOffsetX = std::max(std::min(m_currentOffsetX, maxOffset + m_levelSpacing * 0.25f), minOffset - m_levelSpacing * 0.25f);
            UpdateLevelPanPositions();
        }
        // Pan end: snap to closest level index and clamp
        if (m_isPanning && justReleased) {
            // Check if this was a tap (minimal movement) on the current painting
            float totalMovement = std::abs(touchX - m_panStartX) + std::abs(touchY - m_panStartY);
            const float TAP_THRESHOLD = 10.0f; // pixels
            
            if (totalMovement < TAP_THRESHOLD) {
                // This was a tap, not a pan - check if tap is on current level painting
                if (m_currentLevelIndex < m_levelPaintingEntities.size() && m_levelPaintingEntities[m_currentLevelIndex] != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[m_currentLevelIndex]);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPaintingEntities[m_currentLevelIndex]);
                    
                    if (transform && sprite && sprite->visible) {
                        float paintingWidth = sprite->width * transform->scale.x;
                        float paintingHeight = sprite->height * transform->scale.y;
                        float paintingLeft = transform->position.x;
                        float paintingRight = transform->position.x + paintingWidth;
                        float paintingTop = transform->position.y;
                        float paintingBottom = transform->position.y + paintingHeight;
                        
                        if (touchX >= paintingLeft && touchX <= paintingRight &&
                            touchY >= paintingTop && touchY <= paintingBottom) {
                            // Tap detected on current painting - check for double-tap
                            float currentTime = m_animationTimer; // Use existing timer
                            float timeSinceLastTap = currentTime - m_lastPaintingTapTime;
                            
                            // Check if this is a valid double-tap: same painting, within time window, but not too fast
                            if (m_lastTappedPaintingIndex == m_currentLevelIndex && 
                                timeSinceLastTap >= MIN_TAP_INTERVAL && 
                                timeSinceLastTap < DOUBLE_TAP_THRESHOLD) {
                                // Double-tap detected - enter level!
                                GN_LOG_INFO("🎨 Painting double-tapped (" + std::to_string(timeSinceLastTap * 1000.0f) + "ms) - entering level " + std::to_string(m_currentLevelIndex + 1));
                                m_isPanning = false;
                                m_lastTappedPaintingIndex = -1; // Reset
                                m_lastPaintingTapTime = 0.0f;
                                OnLevelPlayButtonPressed();
                                return;
                            } else if (timeSinceLastTap < MIN_TAP_INTERVAL) {
                                // Tap too fast - likely same tap event spanning frames, ignore it
                                GN_LOG_INFO("🎨 Tap too fast (" + std::to_string(timeSinceLastTap * 1000.0f) + "ms) - ignoring to prevent false double-tap");
                                m_isPanning = false;
                                return;
                            } else {
                                // First tap or too slow - record it
                                GN_LOG_INFO("🎨 Painting tapped once (tap again within " + std::to_string(DOUBLE_TAP_THRESHOLD * 1000.0f) + "ms to enter)");
                                m_lastTappedPaintingIndex = m_currentLevelIndex;
                                m_lastPaintingTapTime = currentTime;
                                // Don't enter level on first tap - wait for second tap
                                m_isPanning = false;
                                return; // Exit early to prevent any further input processing
                            }
                        }
                    }
                }
            }
            
            m_isPanning = false;
            // Determine swipe velocity for multi-level fling
            float deltaX = touchX - m_panStartX;
            // Approx velocity proxy: delta; reduce sensitivity so flings need more distance
            float velocity = deltaX; // pixels; simple proxy
            int velocitySteps = 0;
            if (std::abs(velocity) > m_levelSpacing * 1.6f) velocitySteps = 2;
            else if (std::abs(velocity) > m_levelSpacing * 0.95f) velocitySteps = 1;

            int deltaIndex = (int)std::round(-m_currentOffsetX / std::max(m_levelSpacing, 1.0f));
            // Add velocity-driven extra steps in swipe direction
            if (velocitySteps > 0) {
                int dir = (velocity < 0) ? 1 : -1; // negative deltaX => swiping left to go to next level
                deltaIndex += dir * velocitySteps;
            }

            int targetIndex = std::max(0, std::min(m_currentLevelIndex + deltaIndex, (int)m_levels.size() - 1));
            m_pendingIndexDelta = targetIndex - m_currentLevelIndex;
            // Start snap animation from current offset toward target
            m_isSnapping = true;
            m_snapElapsed = 0.0f;
            m_snapStartOffsetX = m_currentOffsetX;
            if (m_pendingIndexDelta == 0) {
                // No movement; just recentre
                m_isSnapping = true;
                m_pendingIndexDelta = 0;
            }
        }
        
        // Handle button clicks
        bool inputPressed = delegates.input.isPrimaryInputJustPressed();
        if (inputPressed) {
            float touchX, touchY;
            if (delegates.input.getPrimaryInputPosition) {
                delegates.input.getPrimaryInputPosition(&touchX, &touchY);
                
                // Check back button
                if (m_backButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_backButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_backButtonEntity);
                    
                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnBackButtonPressed();
                            return;
                        }
                    }
                }
                
                // Check play button
                if (m_levelPlayButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPlayButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPlayButtonEntity);

                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;

                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnLevelPlayButtonPressed();
                            return;
                        }
                    }
                }

                // Check unlock buttons for locked levels
                GN_LOG_INFO("🎯 CURRENT LEVEL: m_currentLevelIndex=" + std::to_string(m_currentLevelIndex) + 
                           " (Level " + std::to_string(m_currentLevelIndex + 1) + ")");
                           
                // SIMPLIFIED: Only check unlock button for current level
                if (m_currentLevelIndex < m_unlockButtonEntities.size() && 
                    m_unlockButtonEntities[m_currentLevelIndex] != 0 && 
                    !m_levels[m_currentLevelIndex].isUnlocked) {
                    
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_unlockButtonEntities[m_currentLevelIndex]);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_unlockButtonEntities[m_currentLevelIndex]);
                    
                    if (transform && sprite && sprite->visible) {
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;

                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            // Always pass the CURRENT level number, not array index
                            int currentLevelNumber = m_levels[m_currentLevelIndex].levelNumber;
                            GN_LOG_INFO("🎯 UNLOCK CLICKED: Current level " + std::to_string(currentLevelNumber));
                            OnUnlockButtonPressed(currentLevelNumber);
                            return;
                        }
                    }
                }
                
                // Check left arrow button
                if (m_leftArrowButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_leftArrowButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_leftArrowButtonEntity);
                    
                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnLeftArrowPressed();
                            return;
                        }
                    }
                }
                
                // Check right arrow button
                if (m_rightArrowButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_rightArrowButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_rightArrowButtonEntity);
                    
                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnRightArrowPressed();
                            return;
                        }
                    }
                }
            }
        }
    }

    void MainMenuState::HandleSwipeInput() {
        if (!m_game) {
            return;
        }
        
        const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
        
        // Use the new gesture detection methods
        bool swipeLeft = delegates.input.isSwipeLeftDetected ? delegates.input.isSwipeLeftDetected() : false;
        bool swipeRight = delegates.input.isSwipeRightDetected ? delegates.input.isSwipeRightDetected() : false;
        
        // Pruned gesture debug spam
        
        if (swipeLeft && m_currentLevelIndex < m_levels.size() - 1) {
            // Swipe left - go to next level
            m_currentLevelIndex++;
            GN_LOG_INFO("Gesture detected: Swipe left - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
        } else if (swipeRight && m_currentLevelIndex > 0) {
            // Swipe right - go to previous level
            m_currentLevelIndex--;
            GN_LOG_INFO("Gesture detected: Swipe right - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
        }
        
        // Reset gesture state after processing
        if (delegates.input.resetGestureState) {
            delegates.input.resetGestureState();
        }
    }

    void MainMenuState::StartSwipe(float startX, float startY) {
        m_swipeStartX = startX;
        m_swipeStartY = startY;
        m_isSwiping = true;
        m_swipeDirection = SwipeDirection::NONE;
        GN_LOG_INFO("Swipe started at (" + std::to_string(startX) + ", " + std::to_string(startY) + ")");
    }

    void MainMenuState::UpdateSwipe(float currentX, float currentY) {
        if (!m_isSwiping) return;
        
        m_swipeEndX = currentX;
        m_swipeEndY = currentY;
        
        // Determine swipe direction
        float deltaX = currentX - m_swipeStartX;
        float deltaY = currentY - m_swipeStartY;
        
        if (std::abs(deltaX) > std::abs(deltaY) && std::abs(deltaX) > 20.0f) {
            m_swipeDirection = deltaX > 0 ? SwipeDirection::RIGHT : SwipeDirection::LEFT;
        }
    }

    void MainMenuState::EndSwipe(float endX, float endY) {
        if (!m_isSwiping) return;
        
        m_swipeEndX = endX;
        m_swipeEndY = endY;
        m_isSwiping = false;
        
        ProcessSwipe();
        GN_LOG_INFO("Swipe ended at (" + std::to_string(endX) + ", " + std::to_string(endY) + ")");
    }

    void MainMenuState::ProcessSwipe() {
        float deltaX = m_swipeEndX - m_swipeStartX;
        
        GN_LOG_INFO("Processing swipe: deltaX=" + std::to_string(deltaX) + 
                   " threshold=" + std::to_string(m_swipeThreshold) + 
                   " direction=" + std::to_string(static_cast<int>(m_swipeDirection)));
        
        if (std::abs(deltaX) > m_swipeThreshold) {
            if (m_swipeDirection == SwipeDirection::LEFT && m_currentLevelIndex < m_levels.size() - 1) {
                // Swipe left - go to next level
                m_currentLevelIndex++;
                GN_LOG_INFO("Swiped left - moved to level " + std::to_string(m_currentLevelIndex + 1));
            } else if (m_swipeDirection == SwipeDirection::RIGHT && m_currentLevelIndex > 0) {
                // Swipe right - go to previous level
                m_currentLevelIndex--;
                GN_LOG_INFO("Swiped right - moved to level " + std::to_string(m_currentLevelIndex + 1));
            }
            
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
        } else {
            GN_LOG_INFO("Swipe distance too small, ignoring");
        }
    }

    void MainMenuState::AnimateSwipe(float deltaTime) {
        if (m_swipeAnimationTimer < m_swipeAnimationDuration) {
            m_swipeAnimationTimer += deltaTime;
            float progress = m_swipeAnimationTimer / m_swipeAnimationDuration;
            
            // Smooth easing
            float easedProgress = 1.0f - (1.0f - progress) * (1.0f - progress);
            
            // Update painting positions using SAME logic as UpdateLevelVisibility for consistency
            float centerX = m_screenWidth / 2.0f;
            float centerY = m_screenHeight / 2.0f;
            
            // Pruned animation progress log
            
            for (size_t i = 0; i < m_levelPaintingEntities.size(); ++i) {
                if (m_levelPaintingEntities[i] != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                    if (transform && i < m_levels.size()) {
                        // Get painting texture size via RenderSystem
                        int pwi = 0, phi = 0;
                        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                            rs->PreloadTexture(m_levels[i].paintingTexture);
                            if (!rs->GetTextureSize(m_levels[i].paintingTexture, pwi, phi)) { pwi = 96; phi = 96; }
                        }
                        float paintingTextureWidth = static_cast<float>(pwi);
                        float paintingTextureHeight = static_cast<float>(phi);

                        // SAME dynamic scaling calculation as UpdateLevelVisibility
                        float maxWidth = m_screenWidth * 0.8f;  // 80% of screen width
                        float maxHeight = m_screenHeight * 0.4f; // 40% of screen height
                        float scaleByWidth = maxWidth / paintingTextureWidth;
                        float scaleByHeight = maxHeight / paintingTextureHeight;
                        float dynamicScale = std::min(scaleByWidth, scaleByHeight);

                        auto scaledDimensions = GetScaledDimensions(paintingTextureWidth, paintingTextureHeight, dynamicScale);
                        float paintingWidth = scaledDimensions.first;
                        float paintingHeight = scaledDimensions.second;

                        // Position current level at center; others are hidden by UpdateLevelVisibility
                        if (i == m_currentLevelIndex) {
                            float manualTopLeftX = centerX - (paintingWidth / 2.0f);
                            float manualTopLeftY = centerY - (paintingHeight / 2.0f);

                            transform->position.x = manualTopLeftX;
                            transform->position.y = manualTopLeftY;
                            transform->scale.x = dynamicScale;
                            transform->scale.y = dynamicScale;
                        }
                    }
                }

                // Keep frame aligned with painting during animation
                if (m_levelFrameEntities[i] != 0) {
                    Transform* frameTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelFrameEntities[i]);
                    if (frameTransform && m_levelPaintingEntities[i] != 0) {
                        Transform* paintingTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                        if (paintingTransform) {
                            frameTransform->position.x = paintingTransform->position.x;
                            frameTransform->position.y = paintingTransform->position.y;
                            frameTransform->scale = paintingTransform->scale;
                        }
                    }
                }

                // Keep text positioned
                if (m_levelTextEntities[i] != 0) {
                    Transform* textTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelTextEntities[i]);
                    if (textTransform && i == m_currentLevelIndex) {
                        textTransform->position.x = centerX;
                        textTransform->position.y = m_screenHeight * 0.15f; // top 15%
                    }
                }
            }
        }
    }

    void MainMenuState::UpdateLevelVisibility() {
        float centerX = m_screenWidth / 2.0f;
        float centerY = m_screenHeight / 2.0f;
        
        // Pruned verbose logs for steady-state UI updates
        
        for (size_t i = 0; i < m_levelPaintingEntities.size(); ++i) {
            // Only show the current painting - hide all others
            bool shouldBeVisible = (i == m_currentLevelIndex);
            
            // Pruned per-painting visibility spam
            
            // Update painting position, visibility and texture
            if (m_levelPaintingEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPaintingEntities[i]);
                if (transform && sprite) {
                    // Update visibility first
                    sprite->visible = shouldBeVisible;
                    
                    // Position visible painting normally; for panning, non-visible items are placed via UpdateLevelPanPositions
                    if (shouldBeVisible && i < m_levels.size()) {
                        // Get actual painting texture dimensions first via shared RenderSystem
                        int pwi = 0, phi = 0;
                        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                            rs->PreloadTexture(m_levels[i].paintingTexture);
                            if (!rs->GetTextureSize(m_levels[i].paintingTexture, pwi, phi)) { pwi = 96; phi = 96; }
                        }
                        float paintingTextureWidth = static_cast<float>(pwi);  // Correct base size
                        float paintingTextureHeight = static_cast<float>(phi); // Correct base size
                        
                        // Pruned texture dimension spam
                        
                        // Calculate scale that fits screen width with 10% padding on each side
                        float maxWidth = m_screenWidth * 0.8f; // Use 80% of screen width
                        float maxHeight = m_screenHeight * 0.4f; // Use 40% of screen height for paintings
                        
                        float scaleByWidth = maxWidth / paintingTextureWidth;
                        float scaleByHeight = maxHeight / paintingTextureHeight;
                        float dynamicScale = std::min(scaleByWidth, scaleByHeight); // Use smaller scale to fit both dimensions
                        
                        auto scaledDimensions = GetScaledDimensions(paintingTextureWidth, paintingTextureHeight, dynamicScale);
                        float paintingWidth = scaledDimensions.first;
                        float paintingHeight = scaledDimensions.second;
                        
                        // Pruned per-frame calculation logs
                        
                        // MANUAL POSITIONING - bypass CenterObjectAtPosition to test
                        // Pruned test logs
                        
                        // Calculate top-left position manually to center the painting
                        float manualTopLeftX = centerX - (paintingWidth / 2.0f);
                        float manualTopLeftY = centerY - (paintingHeight / 2.0f);
                        
                        // Pruned test logs
                        
                        // Set position directly without helper function
                        transform->position.x = manualTopLeftX;
                        transform->position.y = manualTopLeftY;
                        
                        // Also update the transform scale to use the dynamic scale
                        transform->scale.x = dynamicScale;
                        transform->scale.y = dynamicScale;
                        
                        // Pruned test logs
                        
                        // Update the texture to show the correct level painting
                        sprite->textureId = m_levels[i].paintingTexture;
                    }
                }
            }
            
            // Update frame position and visibility
            if (m_levelFrameEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelFrameEntities[i]);
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelFrameEntities[i]);
                if (transform && sprite) {
                    // Update visibility first
                    sprite->visible = shouldBeVisible && !m_levels[i].isUnlocked;
                    
                    // Only position if visible
                    if (shouldBeVisible && m_levelPaintingEntities[i] != 0) {
                        Transform* paintingTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                        if (paintingTransform) {
                            // Use EXACT same position and scale as the underlying painting
                            transform->position.x = paintingTransform->position.x;
                            transform->position.y = paintingTransform->position.y;
                            transform->scale = paintingTransform->scale;
                        }
                    }
                }
            }
            
            // Update text position and visibility
            if (m_levelTextEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelTextEntities[i]);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_levelTextEntities[i]);
                if (transform && uiElement) {
                    // Update visibility first
                    uiElement->visible = shouldBeVisible;

                    // Only position and update text for visible level
                    if (shouldBeVisible) {
                        // Center the text horizontally at the painting position
                        transform->position.x = centerX;
                        transform->position.y = m_screenHeight * 0.15f; // Keep at top 15% of screen

                        // Update the text to show the correct level name
                        if (i < m_levels.size()) {
                            uiElement->buttonText = m_levels[i].name;
                        }
                    }
                }
            }

            // Update unlock button and requirements visibility
            if (m_unlockButtonEntities[i] != 0) {
                Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_unlockButtonEntities[i]);
                UIElement* buttonElement = m_ecsCoordinator->GetComponent<UIElement>(m_unlockButtonEntities[i]);
                if (buttonSprite && buttonElement) {
                    // Show unlock button only for visible AND locked levels
                    bool showUnlockButton = shouldBeVisible && !m_levels[i].isUnlocked;
                    buttonSprite->visible = showUnlockButton;
                    buttonElement->visible = showUnlockButton;
                }
            }

            if (m_requirementTextEntities[i] != 0) {
                UIElement* reqTextElement = m_ecsCoordinator->GetComponent<UIElement>(m_requirementTextEntities[i]);
                if (reqTextElement) {
                    // Show requirements text only for visible AND locked levels
                    reqTextElement->visible = shouldBeVisible && !m_levels[i].isUnlocked;
                }
            }
        }

        // While panning, position all paintings horizontally with offset and partial visibility
        if (m_isPanning) {
            UpdateLevelPanPositions();
        }
        
        // Update locked indicator visibility and position
        if (m_lockedIndicatorEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_lockedIndicatorEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_lockedIndicatorEntity);
            if (transform && uiElement) {
                // Position above the current level's painting
                transform->position.x = centerX;
                float paintingScale = m_isMobile ? 6.0f : 2.0f;
                transform->position.y = centerY - (paintingScale * 100.0f);
                
                // Show indicator only if current level is locked
                uiElement->visible = !m_levels[m_currentLevelIndex].isUnlocked;
            }
        }
        
        // Pruned visibility summary log
    }

    void MainMenuState::CenterCurrentLevel() {
        UpdateLevelVisibility();
    }

    void MainMenuState::OnBackButtonPressed() {
        GN_LOG_INFO("Back button pressed - returning to main menu");
        m_lastMenuButtonPressTime = m_animationTimer; // Set debounce timer
        HideLevelSelect();
    }

    void MainMenuState::OnLevelPlayButtonPressed() {
        GN_LOG_INFO("Level play button pressed - starting level " + std::to_string(m_currentLevelIndex + 1));
        OnLevelSelected(m_currentLevelIndex);
    }

    void MainMenuState::OnLeftArrowPressed() {
        // Check debounce timer to prevent rapid clicking
        if (m_lastArrowPressTime < m_arrowDebounceDelay) {
            return; // Too soon since last press, ignore
        }
        
        if (m_currentLevelIndex > 0) {
            m_currentLevelIndex--;
            GN_LOG_INFO("⬅️ Left arrow pressed - moved to level " + std::to_string(m_currentLevelIndex + 1) + " (index " + std::to_string(m_currentLevelIndex) + ")");
            
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
            
            // Reset debounce timer
            m_lastArrowPressTime = 0.0f;
        }
    }

    void MainMenuState::OnRightArrowPressed() {
        // Check debounce timer to prevent rapid clicking
        if (m_lastArrowPressTime < m_arrowDebounceDelay) {
            return; // Too soon since last press, ignore
        }
        
        if (m_currentLevelIndex < m_levels.size() - 1) {
            m_currentLevelIndex++;
            GN_LOG_INFO("➡️ Right arrow pressed - moved to level " + std::to_string(m_currentLevelIndex + 1) + " (index " + std::to_string(m_currentLevelIndex) + ")");
            
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
            
            // Reset debounce timer
            m_lastArrowPressTime = 0.0f;
        }
    }

    void MainMenuState::OnLevelSelected(int levelIndex) {
        if (levelIndex >= 0 && levelIndex < m_levels.size()) {
            GN_LOG_INFO("Level selected: " + m_levels[levelIndex].name + " (Level " + std::to_string(m_levels[levelIndex].levelNumber) + ")");
            
            // Check if level is unlocked
            if (!m_levels[levelIndex].isUnlocked) {
                GN_LOG_INFO("Level is locked - cannot start");
                return;
            }
            
            // Store the selected level NUMBER (1-6) for the transition, not the index (0-5)
            m_selectedLevelIndex = m_levels[levelIndex].levelNumber;
            m_finished = true;
        }
    }

    void MainMenuState::DrawLevelSelectDebugInfo() {
        if (!m_game || !m_ecsCoordinator) {
            return;
        }
        
        const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
        if (!delegates.renderer.drawRectangle) {
            return;
        }
        
        // Draw debug info for level select elements
        GN_LOG_INFO("Level Select Debug: Current level " + std::to_string(m_currentLevelIndex + 1) + 
                   " of " + std::to_string(m_levels.size()) + 
                   ", Mode: " + (m_currentMode == MenuMode::LEVEL_SELECT ? "LEVEL_SELECT" : "MAIN_MENU"));
    }
    
    // Platform-specific layout implementation
    void MainMenuState::SetupLayout() {
        if (m_game && m_game->IsIOSPlatform()) {
            SetupIOSLayout();
        } else {
            SetupDesktopLayout();
        }
    }
    
    void MainMenuState::SetupIOSLayout() {
        GN_LOG_INFO("MainMenuState: Setting up iOS layout");
        
        // Calculate dynamic scaling for iOS based on screen size
        float scaleX = m_screenWidth / 393.0f;   // iPhone 16 logical width reference
        float scaleY = m_screenHeight / 852.0f;  // iPhone 16 logical height reference
        float dynamicScale = std::min(scaleX, scaleY);
        
        GN_LOG_INFO("MainMenuState: iOS layout - Screen: " + 
                   std::to_string((int)m_screenWidth) + "x" + 
                   std::to_string((int)m_screenHeight) + 
                   ", Scale: " + std::to_string(dynamicScale));
        
        // iOS-specific main menu layout adjustments can go here
        // e.g., adjusting button positions, logo placement, safe area handling, etc.
    }
    
    void MainMenuState::SetupDesktopLayout() {
        GN_LOG_INFO("MainMenuState: Setting up desktop layout");
        
        // Calculate dynamic scaling for desktop
        float scaleX = m_screenWidth / 800.0f;   // Desktop reference width
        float scaleY = m_screenHeight / 600.0f;  // Desktop reference height
        float dynamicScale = std::min(scaleX, scaleY);
        
        GN_LOG_INFO("MainMenuState: Desktop layout - Screen: " + 
                   std::to_string((int)m_screenWidth) + "x" + 
                   std::to_string((int)m_screenHeight) + 
                   ", Scale: " + std::to_string(dynamicScale));
        
        // Desktop-specific main menu layout adjustments can go here
    }

    // ==================== AD CONTROLS MENU ====================

    void MainMenuState::ShowAdControlsMenu() {
        GN_LOG_INFO("Showing Ad Controls menu");
        m_currentMode = MenuMode::AD_CONTROLS;
        SetMainMenuVisible(false); // Hide main menu elements including ad controls button
        CreateAdControlsLayout();
    }

    void MainMenuState::HideAdControlsMenu() {
        GN_LOG_INFO("Hiding Ad Controls menu");
        
        // Hide all ad controls menu entities
        if (m_adControlsTitleEntity != 0) {
            auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsTitleEntity);
            if (ui) ui->visible = false;
        }
        
        if (m_adControlsBackButtonEntity != 0) {
            auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsBackButtonEntity);
            if (ui) ui->visible = false;
            auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_adControlsBackButtonEntity);
            if (sprite) sprite->visible = false;
        }
        
        if (m_removeAdsLabelEntity != 0) {
            auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_removeAdsLabelEntity);
            if (ui) ui->visible = false;
        }
        
        if (m_removeAdsPriceButtonEntity != 0) {
            auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_removeAdsPriceButtonEntity);
            if (ui) ui->visible = false;
            auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_removeAdsPriceButtonEntity);
            if (sprite) sprite->visible = false;
        }
        
        // Hide overlay background
        if (m_adControlsOverlayEntity != 0) {
            auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_adControlsOverlayEntity);
            if (sprite) sprite->visible = false;
        }
    }

    void MainMenuState::CreateAdControlsLayout() {
        if (!m_ecsCoordinator) return;
        
        GN_LOG_INFO("Creating Ad Controls menu layout");
        
        // Calculate center overlay (similar to leaderboard/options)
        float overlayW = m_screenWidth * 0.8f;
        float overlayH = m_screenHeight * 0.7f;
        float overlayX = (m_screenWidth - overlayW) * 0.5f;
        float overlayY = (m_screenHeight - overlayH) * 0.5f;
        
        // Create overlay background (same as leaderboard)
        if (m_adControlsOverlayEntity == 0) {
            m_adControlsOverlayEntity = m_ecsCoordinator->CreateEntity();
        }
        
        float overlayTextureWidth = 160.0f;
        float overlayTextureHeight = 300.0f;
        float overlayScale = 7.0f;
        
        Gnosis::GNVector2 overlayPosition(
            (m_screenWidth - overlayTextureWidth * overlayScale) * 0.5f,
            (m_screenHeight - overlayTextureHeight * overlayScale) * 0.5f
        );
        
        Transform overlayTransform(overlayPosition, 0.0f, Gnosis::GNVector2(overlayScale, overlayScale));
        
        if (!m_ecsCoordinator->HasComponent<Transform>(m_adControlsOverlayEntity)) {
            m_ecsCoordinator->AddComponent<Transform>(m_adControlsOverlayEntity, overlayTransform);
        } else {
            *m_ecsCoordinator->GetComponent<Transform>(m_adControlsOverlayEntity) = overlayTransform;
        }
        
        Sprite overlaySprite("PauseMenuBackgroundMobile", (int)overlayTextureWidth, (int)overlayTextureHeight);
        overlaySprite.layer = 5;
        overlaySprite.visible = true;
        
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_adControlsOverlayEntity)) {
            m_ecsCoordinator->AddComponent<Sprite>(m_adControlsOverlayEntity, overlaySprite);
        } else {
            *m_ecsCoordinator->GetComponent<Sprite>(m_adControlsOverlayEntity) = overlaySprite;
        }
        
        // Title centered at top
        if (m_adControlsTitleEntity == 0) {
            m_adControlsTitleEntity = m_ecsCoordinator->CreateEntity();
        }
        
        float titleX = overlayX + overlayW * 0.5f;
        float titleY = overlayY + overlayH * 0.08f;
        
        Transform titleTransform(Gnosis::GNVector2(titleX, titleY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        Sprite titleSprite;
        titleSprite.visible = false;
        titleSprite.layer = 4;
        
        UIElement titleUI("AD CONTROLS", "", "");
        titleUI.fontSize = m_isMobile ? 72.0f : 42.0f;
        titleUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        titleUI.centerTextHorizontally = true;
        titleUI.centerTextVertically = true;
        titleUI.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_adControlsTitleEntity, titleTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_adControlsTitleEntity, titleSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_adControlsTitleEntity, titleUI);
        
        // "Remove Ads" label centered
        if (m_removeAdsLabelEntity == 0) {
            m_removeAdsLabelEntity = m_ecsCoordinator->CreateEntity();
        }
        
        float labelX = overlayX + overlayW * 0.5f;
        float labelY = overlayY + overlayH * 0.4f;
        
        Transform labelTransform(Gnosis::GNVector2(labelX, labelY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        Sprite labelSprite;
        labelSprite.visible = false;
        labelSprite.layer = 4;
        
        UIElement labelUI("Remove Ads", "", "");
        labelUI.fontSize = m_isMobile ? 64.0f : 38.0f;
        labelUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        labelUI.centerTextHorizontally = true;
        labelUI.centerTextVertically = true;
        labelUI.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_removeAdsLabelEntity, labelTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_removeAdsLabelEntity, labelSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_removeAdsLabelEntity, labelUI);
        
        // "$2.00" button - centered UNDER the "Remove Ads" label on FloppyButtonBlue (same size as main menu buttons)
        if (m_removeAdsPriceButtonEntity == 0) {
            m_removeAdsPriceButtonEntity = m_ecsCoordinator->CreateEntity();
        }
        
        // Use actual texture dimensions like main menu buttons
        int priceW = 0, priceH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyButtonBlue");
            if (!rs->GetTextureSize("FloppyButtonBlue", priceW, priceH)) { priceW = 90; priceH = 16; }
        }
        float priceButtonTextureWidth = static_cast<float>(priceW);
        float priceButtonTextureHeight = static_cast<float>(priceH);
        
        float priceButtonScale = 10.0f; // Match main menu button scale exactly
        
        // Calculate center position
        float priceButtonCenterX = m_screenWidth / 2.0f;
        float priceButtonCenterY = labelY + 150.0f;
        
        // Get scaled dimensions
        auto priceScaledDimensions = GetScaledDimensions(priceButtonTextureWidth, priceButtonTextureHeight, priceButtonScale);
        float priceScaledWidth = priceScaledDimensions.first;
        float priceScaledHeight = priceScaledDimensions.second;
        
        // Use CenterObjectAtPosition to get top-left coordinates (same as main menu buttons)
        auto pricePosition = CenterObjectAtPosition(priceButtonCenterX, priceButtonCenterY, priceScaledWidth, priceScaledHeight);
        float priceButtonX = pricePosition.x;
        float priceButtonY = pricePosition.y;
        
        Transform priceButtonTransform(Gnosis::GNVector2(priceButtonX, priceButtonY), 0.0f, Gnosis::GNVector2(priceButtonScale, priceButtonScale));
        Sprite priceButtonSprite("FloppyButtonBlue", priceButtonTextureWidth, priceButtonTextureHeight);
        priceButtonSprite.visible = true;
        priceButtonSprite.layer = 6;
        
        UIElement priceButtonUI("", "FloppyButtonBlue", "FloppyButtonBlue");
        priceButtonUI.buttonText = "$2.00"; // Use buttonText field like main menu buttons
        priceButtonUI.fontSize = 80.0f; // Match main menu text size exactly
        priceButtonUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        priceButtonUI.centerTextHorizontally = true;
        priceButtonUI.centerTextVertically = true;
        priceButtonUI.textOffsetY = 0.0f; // No offset for perfect centering
        priceButtonUI.visible = true;
        priceButtonUI.isEnabled = true;
        
        if (!m_ecsCoordinator->HasComponent<Transform>(m_removeAdsPriceButtonEntity)) {
            m_ecsCoordinator->AddComponent<Transform>(m_removeAdsPriceButtonEntity, priceButtonTransform);
        } else {
            *m_ecsCoordinator->GetComponent<Transform>(m_removeAdsPriceButtonEntity) = priceButtonTransform;
        }
        
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_removeAdsPriceButtonEntity)) {
            m_ecsCoordinator->AddComponent<Sprite>(m_removeAdsPriceButtonEntity, priceButtonSprite);
        } else {
            *m_ecsCoordinator->GetComponent<Sprite>(m_removeAdsPriceButtonEntity) = priceButtonSprite;
        }
        
        if (!m_ecsCoordinator->HasComponent<UIElement>(m_removeAdsPriceButtonEntity)) {
            m_ecsCoordinator->AddComponent<UIElement>(m_removeAdsPriceButtonEntity, priceButtonUI);
        } else {
            *m_ecsCoordinator->GetComponent<UIElement>(m_removeAdsPriceButtonEntity) = priceButtonUI;
        }
        
        // Back button at bottom center on FloppyButtonBlue (same size as main menu buttons)
        if (m_adControlsBackButtonEntity == 0) {
            m_adControlsBackButtonEntity = m_ecsCoordinator->CreateEntity();
        }
        
        // Use actual texture dimensions like main menu buttons
        int backW = 0, backH = 0;
        if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            rs->PreloadTexture("FloppyButtonBlue");
            if (!rs->GetTextureSize("FloppyButtonBlue", backW, backH)) { backW = 90; backH = 16; }
        }
        float backButtonTextureWidth = static_cast<float>(backW);
        float backButtonTextureHeight = static_cast<float>(backH);
        
        float backButtonScale = 10.0f; // Match main menu button scale exactly
        
        // Calculate center position
        float backButtonCenterX = m_screenWidth / 2.0f;
        float backButtonCenterY = overlayY + overlayH * 0.85f;
        
        // Get scaled dimensions
        auto backScaledDimensions = GetScaledDimensions(backButtonTextureWidth, backButtonTextureHeight, backButtonScale);
        float backScaledWidth = backScaledDimensions.first;
        float backScaledHeight = backScaledDimensions.second;
        
        // Use CenterObjectAtPosition to get top-left coordinates (same as main menu buttons)
        auto backPosition = CenterObjectAtPosition(backButtonCenterX, backButtonCenterY, backScaledWidth, backScaledHeight);
        float backButtonX = backPosition.x;
        float backButtonY = backPosition.y;
        
        Transform backButtonTransform(Gnosis::GNVector2(backButtonX, backButtonY), 0.0f, Gnosis::GNVector2(backButtonScale, backButtonScale));
        Sprite backButtonSprite("FloppyButtonBlue", backButtonTextureWidth, backButtonTextureHeight);
        backButtonSprite.visible = true;
        backButtonSprite.layer = 6;
        
        UIElement backButtonUI("", "FloppyButtonBlue", "FloppyButtonBlue");
        backButtonUI.buttonText = "BACK"; // Use buttonText field like main menu buttons
        backButtonUI.fontSize = 80.0f; // Match main menu text size exactly
        backButtonUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        backButtonUI.centerTextHorizontally = true;
        backButtonUI.centerTextVertically = true;
        backButtonUI.textOffsetY = 0.0f; // No offset for perfect centering
        backButtonUI.visible = true;
        backButtonUI.isEnabled = true;
        
        if (!m_ecsCoordinator->HasComponent<Transform>(m_adControlsBackButtonEntity)) {
            m_ecsCoordinator->AddComponent<Transform>(m_adControlsBackButtonEntity, backButtonTransform);
        } else {
            *m_ecsCoordinator->GetComponent<Transform>(m_adControlsBackButtonEntity) = backButtonTransform;
        }
        
        if (!m_ecsCoordinator->HasComponent<Sprite>(m_adControlsBackButtonEntity)) {
            m_ecsCoordinator->AddComponent<Sprite>(m_adControlsBackButtonEntity, backButtonSprite);
        } else {
            *m_ecsCoordinator->GetComponent<Sprite>(m_adControlsBackButtonEntity) = backButtonSprite;
        }
        
        if (!m_ecsCoordinator->HasComponent<UIElement>(m_adControlsBackButtonEntity)) {
            m_ecsCoordinator->AddComponent<UIElement>(m_adControlsBackButtonEntity, backButtonUI);
        } else {
            *m_ecsCoordinator->GetComponent<UIElement>(m_adControlsBackButtonEntity) = backButtonUI;
        }
        
        GN_LOG_INFO("Ad Controls menu layout created");
    }

    void MainMenuState::HandleAdControlsInput() {
        if (!m_ecsCoordinator) return;
        
        // Get touch input from platform delegates
        if (m_game) {
            const PlatformDelegates& delegates = m_game->GetPlatformDelegates();
            
            // Check for touch/click
            if (delegates.input.isTouchDown && delegates.input.isTouchDown()) {
                float touchX = 0.0f;
                float touchY = 0.0f;
                if (delegates.input.getTouchPosition) {
                    delegates.input.getTouchPosition(0, &touchX, &touchY);
                    
                    // Check back button
                    if (m_adControlsBackButtonEntity != 0) {
                        auto transform = m_ecsCoordinator->GetComponent<Transform>(m_adControlsBackButtonEntity);
                        auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_adControlsBackButtonEntity);
                        auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsBackButtonEntity);
                        
                        if (transform && sprite && ui && ui->isEnabled) {
                            float buttonW = sprite->width * transform->scale.x;
                            float buttonH = sprite->height * transform->scale.y;
                            
                            if (touchX >= transform->position.x && touchX <= transform->position.x + buttonW &&
                                touchY >= transform->position.y && touchY <= transform->position.y + buttonH) {
                                OnAdControlsBackButtonPressed();
                                return;
                            }
                        }
                    }
                    
                    // Check price button
                    if (m_removeAdsPriceButtonEntity != 0) {
                        auto transform = m_ecsCoordinator->GetComponent<Transform>(m_removeAdsPriceButtonEntity);
                        auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_removeAdsPriceButtonEntity);
                        auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_removeAdsPriceButtonEntity);
                        
                        if (transform && sprite && ui && ui->isEnabled) {
                            float buttonW = sprite->width * transform->scale.x;
                            float buttonH = sprite->height * transform->scale.y;
                            
                            if (touchX >= transform->position.x && touchX <= transform->position.x + buttonW &&
                                touchY >= transform->position.y && touchY <= transform->position.y + buttonH) {
                                OnRemoveAdsPurchasePressed();
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    void MainMenuState::OnAdControlsBackButtonPressed() {
        GN_LOG_INFO("Ad Controls Back button pressed");
        
        // Set debounce timer to prevent accidental clicks when returning to main menu
        m_lastMenuButtonPressTime = m_animationTimer;
        
        HideAdControlsMenu();
        m_currentMode = MenuMode::MAIN_MENU;
        SetMainMenuVisible(true);
    }

    void MainMenuState::OnRemoveAdsPurchasePressed() {
        GN_LOG_INFO("Remove Ads purchase button pressed - IAP not yet implemented");
        // TODO: Phase 7 - Call StoreManager to initiate purchase
        // This will be wired up when we implement StoreKit 2 integration
    }

    // ==================== VIBRATION PREFERENCE ====================

    void MainMenuState::SaveVibrationPreference(bool enabled) {
        // Saved automatically by FloppyTurdGame::SetVibrationsEnabled
        if (m_game) {
            m_game->SetVibrationsEnabled(enabled);
        }
    }

    bool MainMenuState::LoadVibrationPreference() {
        // Load from game
        if (m_game) {
            return m_game->GetVibrationsEnabled();
        }
        return true; // Default enabled
    }

} // namespace GameCore