#include "MainMenuState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include <iostream>
#include <random>
#include <cmath>

namespace GameCore {

    MainMenuState::MainMenuState(Gnosis::ECS* ecsCoordinator)
        : m_ecsCoordinator(ecsCoordinator)
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
        , m_quitButtonEntity(0)
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
        , m_fontLoaded(false)
        , m_assetsLoaded(false) {
        
        // Detect if we're on a mobile platform
        m_isMobile = IsMobilePlatform();
        
        // Initialize level data
        InitializeLevels();
    }

    MainMenuState::~MainMenuState() {
        // UI cleanup will be handled by platform-specific rendering system
    }

    void MainMenuState::Enter() {
        GN_LOG_INFO("Entering Main Menu State");
        m_finished = false;
        m_selectedOption = 0;
        m_animationTimer = 0.0f;
        m_assetsLoaded = false;
        
        // Start playing main menu music using delegate system
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
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
        }
        
        // Load the Whacky Joe font for text rendering
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
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
        
        // Create the appropriate layout based on platform
        if (m_isMobile) {
            CreateMobileLayout();
        } else {
            CreateDesktopLayout();
        }
        
        // Create UI elements (buttons with integrated text)
        CreateUIElements();
        
        // Create level select layout (hidden initially)
        CreateLevelSelectLayout();
        
        m_assetsLoaded = true;
        GN_LOG_INFO("Main Menu State fully initialized");
    }

    void MainMenuState::Exit() {
        GN_LOG_INFO("Exiting Main Menu State");
        
        // Stop menu music using delegate system
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.audio.stopMusic) {
                delegates.audio.stopMusic();
                GN_LOG_INFO("Stopped main menu music");
            }
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
            if (m_quitButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_quitButtonEntity);
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
            
            if (m_lockedIndicatorEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_lockedIndicatorEntity);
            }
            
            GN_LOG_INFO("Cleaned up main menu and level select UI entities");
        }
    }

    void MainMenuState::Pause() {
        // Main menu can be paused if needed
        GN_LOG_INFO("Main Menu State paused");
    }

    void MainMenuState::Resume() {
        // Resume main menu
        GN_LOG_INFO("Main Menu State resumed");
    }

    void MainMenuState::Update(float deltaTime) {
        m_animationTimer += deltaTime;
        
        // Update menu animations (logo bobbing, button highlights, etc.)
        UpdateMenuAnimations(deltaTime);
        
        // Handle menu selection changes
        UpdateMenuSelection();
        
        // Update swipe animation if in level select mode
        if (m_currentMode == MenuMode::LEVEL_SELECT) {
            AnimateSwipe(deltaTime);
        }
        
        // Update ECS systems
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Update(deltaTime);
        }
    }

    void MainMenuState::Render() {
        // Render through ECS system (sprites, buttons, text, etc.)
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Render();
        }
        
        // Debug: Draw button bounds rectangles
        extern FloppyTurdGame* g_Game;
        if (g_Game && g_Game->GetPlatformDelegates().renderer.drawRectangle) {
            // Draw debug rectangles for each button
            DrawButtonDebugRectangles();
            
            // Draw level select debug info
            if (m_currentMode == MenuMode::LEVEL_SELECT) {
                DrawLevelSelectDebugInfo();
            }
        }
    }

    void MainMenuState::HandleInput() {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }
        
        // Get input from platform delegates
        extern FloppyTurdGame* g_Game;
        if (!g_Game) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        
        // Debug: Check if input delegates are properly set
        if (!delegates.input.isPrimaryInputJustPressed) {
            GN_LOG_ERROR("MainMenuState: isPrimaryInputJustPressed delegate is NULL!");
            return;
        }
        
        // Handle input based on current mode
        if (m_currentMode == MenuMode::MAIN_MENU) {
            HandleMainMenuInput(delegates);
        } else if (m_currentMode == MenuMode::LEVEL_SELECT) {
            HandleLevelSelectInput();
        }
    }
    
    void MainMenuState::HandleMainMenuInput(const PlatformDelegates& delegates) {
        // Handle touch/click input for F button
        bool inputPressed = delegates.input.isPrimaryInputJustPressed();
        if (inputPressed) {
            GN_LOG_INFO("🎮 MainMenuState: Input detected! Checking F button bounds...");
        }
        
        if (inputPressed) {
            float touchX, touchY;
            if (delegates.input.getPrimaryInputPosition) {
                delegates.input.getPrimaryInputPosition(&touchX, &touchY);
                
                // Check if touch/click is within F button bounds
                if (m_fButtonEntity != 0) {
                    Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
                    Sprite* fButtonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
                    
                    if (fButtonTransform && fButtonSprite) {
                        // Calculate F button bounds (centered)
                        float buttonWidth = fButtonSprite->width * fButtonTransform->scale.x;
                        float buttonHeight = fButtonSprite->height * fButtonTransform->scale.y;
                        float buttonLeft = fButtonTransform->position.x - (buttonWidth / 2.0f);
                        float buttonRight = fButtonTransform->position.x + (buttonWidth / 2.0f);
                        float buttonTop = fButtonTransform->position.y - (buttonHeight / 2.0f);
                        float buttonBottom = fButtonTransform->position.y + (buttonHeight / 2.0f);
                        
                        // Check if touch is within bounds
                        GN_LOG_INFO("🎯 MainMenuState: Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                                   "), F button bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                                   " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom));
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            GN_LOG_INFO("🎉 MainMenuState: F BUTTON HIT! Playing fart sound...");
                            OnFButtonPressed();
                        } else {
                            GN_LOG_INFO("❌ MainMenuState: Touch missed F button");
                            
                            // Check menu buttons
                            CheckMenuButtonClicks(touchX, touchY);
                        }
                    }
                }
            }
        }
        
        // TODO: Handle menu navigation (up/down arrows) for desktop
        // TODO: Handle selection (enter/space) for menu options
    }

    void MainMenuState::CreateDesktopLayout() {
        GN_LOG_INFO("Creating desktop main menu layout");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateDesktopLayout");
            return;
        }
        
        // Get actual screen dimensions from platform delegates
        float screenWidth = 800.0f;  // Default fallback
        float screenHeight = 600.0f; // Default fallback
        
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.renderer.getScreenSize) {
                delegates.renderer.getScreenSize(&screenWidth, &screenHeight);
                GN_LOG_INFO("Desktop screen dimensions: %fx%f", screenWidth, screenHeight);
            }
        }
        
        // Store screen dimensions for consistent use across the class
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        
        float centerX = screenWidth / 2.0f;
        float centerY = screenHeight / 2.0f;
        
        // 1. Create Background Entity (MainMenu.png) - FULL SCREEN SCALING
        m_backgroundEntity = m_ecsCoordinator->CreateEntity();
        Transform bgTransform(Gnosis::GNVector2(centerX, centerY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        
        // Background texture dimensions (from file: 320x180)
        float textureWidth = 320.0f;
        float textureHeight = 180.0f;
        
        // Calculate scale to fill screen
        float scaleX = screenWidth / textureWidth;
        float scaleY = screenHeight / textureHeight;
        
        // Create sprite with actual texture dimensions, scale will be applied by transform
        Sprite bgSprite("MainMenu", textureWidth, textureHeight);
        bgSprite.layer = 0; // Background layer
        bgSprite.visible = true;
        
        // Apply the calculated scale to the transform
        bgTransform.scale.x = scaleX;
        bgTransform.scale.y = scaleY;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backgroundEntity, bgTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        GN_LOG_INFO("Created full-screen background entity: MainMenu.png (texture: %fx%f, scale: %fx%f, screen: %fx%f)", 
                   textureWidth, textureHeight, scaleX, scaleY, screenWidth, screenHeight);
        
        // 2. Create Logo Entity (FloppyLogo.png) - Desktop scaling
        m_logoEntity = m_ecsCoordinator->CreateEntity();
        float logoY = screenHeight * 0.25f; // Top quarter
        Transform logoTransform(Gnosis::GNVector2(centerX, logoY), 0.0f, Gnosis::GNVector2(2.0f, 2.0f)); // 2x scale for desktop
        Sprite logoSprite("FloppyLogo", 400.0f, 200.0f); // Base size, will be scaled by transform
        logoSprite.layer = 1; // Logo layer
        logoSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_logoEntity, logoSprite);
        GN_LOG_INFO("Created scaled logo entity: FloppyLogo.png (2x scale)");
        
        // 3. Create Interactive F Button Entity (F.png) - Desktop scaling
        m_fButtonEntity = m_ecsCoordinator->CreateEntity();
        // Position F button relative to scaled logo
        float fButtonX = centerX + 200.0f; // Adjusted for 2x scaled logo width
        float fButtonY = logoY + 20.0f; // Slightly below logo center, adjusted for scale
        Transform fButtonTransform(Gnosis::GNVector2(fButtonX, fButtonY), 0.0f, Gnosis::GNVector2(2.5f, 2.5f)); // Larger for desktop
        Sprite fButtonSprite("F", 80.0f, 80.0f); // Base size, will be scaled by transform
        fButtonSprite.layer = 2; // F button layer
        fButtonSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_fButtonEntity, fButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_fButtonEntity, fButtonSprite);
        GN_LOG_INFO("Created F button entity: F.png (2.5x scale)");
        
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
        
        // Get actual screen dimensions from platform delegates
        float screenWidth = 1179.0f;  // Default fallback for mobile
        float screenHeight = 2556.0f; // Default fallback for mobile
        
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.renderer.getScreenSize) {
                delegates.renderer.getScreenSize(&screenWidth, &screenHeight);
                GN_LOG_INFO("Mobile screen dimensions: %fx%f", screenWidth, screenHeight);
            }
        }
        
        // Store screen dimensions for consistent use across the class
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        
        float centerX = screenWidth / 2.0f;
        
        // 1. Create Background Entity (MainMenuMobile.png) - FULL SCREEN SCALING
        m_backgroundEntity = m_ecsCoordinator->CreateEntity();
        Transform bgTransform(Gnosis::GNVector2(centerX, screenHeight / 2.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        
        // Background texture dimensions (from file: 393x852)
        float textureWidth = 393.0f;
        float textureHeight = 852.0f;
        
        // Calculate scale to fill screen
        float scaleX = screenWidth / textureWidth;
        float scaleY = screenHeight / textureHeight;
        
        // Create sprite with actual texture dimensions, scale will be applied by transform
        Sprite bgSprite("MainMenuMobile", textureWidth, textureHeight);
        bgSprite.layer = 0; // Background layer
        bgSprite.visible = true;
        
        // Apply the calculated scale to the transform
        bgTransform.scale.x = scaleX;
        bgTransform.scale.y = scaleY;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backgroundEntity, bgTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        GN_LOG_INFO("Created full-screen background entity: MainMenuMobile.png (texture: %fx%f, scale: %fx%f, screen: %fx%f)", 
                   textureWidth, textureHeight, scaleX, scaleY, screenWidth, screenHeight);
        
        // 2. Create Logo Entity (FloppyLogo.png) - EVEN BIGGER SCALING (~8x)
        m_logoEntity = m_ecsCoordinator->CreateEntity();
        float logoY = screenHeight * 0.20f; // Shifted more upward (was 0.25f)
        float logoX = centerX - 80.0f; // Shifted more to the left
        Transform logoTransform(Gnosis::GNVector2(logoX, logoY), 0.0f, Gnosis::GNVector2(8.0f, 8.0f)); // 8x scale for mobile
        Sprite logoSprite("FloppyLogo", 400.0f, 200.0f); // Base size, will be scaled by transform
        logoSprite.layer = 1; // Logo layer
        logoSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_logoEntity, logoSprite);
        GN_LOG_INFO("Created scaled logo entity: FloppyLogo.png (8x scale, shifted up and left)");
        
        // 3. Create F Button Entity (F.png) - SAME SCALE AS LOGO, POSITIONED TO FILL GAP
        m_fButtonEntity = m_ecsCoordinator->CreateEntity();
        float fButtonX = logoX - 270.0f; // FINAL FINE-TUNE - before the "L" in "Floppy"
        float fButtonY = logoY + 20.0f; // Slightly down (was -40.0f)
        Transform fButtonTransform(Gnosis::GNVector2(fButtonX, fButtonY), 0.0f, Gnosis::GNVector2(8.0f, 8.0f)); // 8x scale to match logo
        Sprite fButtonSprite("F", 50.0f, 50.0f); // Base size, will be scaled by transform
        fButtonSprite.layer = 2; // F button layer (above logo)
        fButtonSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_fButtonEntity, fButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_fButtonEntity, fButtonSprite);
        GN_LOG_INFO("Created F button entity: F.png (8x scale, positioned to fill logo gap - more right and down)");
        
        // Create menu buttons for mobile
        CreateMobileMenuButtons();
        
        GN_LOG_INFO("Mobile layout created: Background, Logo, F Button, and Menu Button entities");
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
                
                // Update logo Y position (preserve original Y + float offset)
                float originalLogoY = m_screenHeight * 0.25f;
                logoTransform->position.y = originalLogoY + logoFloat;
            }
        }
        
        // F Button gentle pulsing animation
        if (m_fButtonEntity != 0) {
            Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
            if (fButtonTransform) {
                // Create a subtle pulsing scale effect - adjust base scale based on platform
                float baseScale = m_isMobile ? 8.0f : 2.5f;
                float pulseScale = baseScale + sin(m_animationTimer * 2.5f) * 0.3f;
                fButtonTransform->scale.x = pulseScale;
                fButtonTransform->scale.y = pulseScale;
            }
        }
    }

    void MainMenuState::OnMenuOptionSelected(MenuOption option) {
        GN_LOG_INFO("Menu option selected: %d", static_cast<int>(option));
        
        switch (option) {
            case MenuOption::PLAYING:
                GN_LOG_INFO("Starting game...");
                // TODO: Transition to game state
                m_finished = true;
                break;
                
            case MenuOption::OPTIONS:
                GN_LOG_INFO("Opening options menu...");
                // TODO: Transition to options state
                break;
                
            case MenuOption::QUICK_PLAY:
                GN_LOG_INFO("Starting quick play...");
                // TODO: Transition to quick play state
                m_finished = true;
                break;
                
            case MenuOption::QUIT:
                GN_LOG_INFO("Quitting game...");
                // TODO: Quit application
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
        
        // Use delegate system to play sound
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.audio.playSound) {
                delegates.audio.playSound(fartSoundName.c_str(), 0.8f); // 80% volume
                GN_LOG_INFO("Playing fart sound: %s.mp3", fartSoundName.c_str());
            }
        }
        
        // Add visual feedback - make F button briefly larger
        if (m_fButtonEntity != 0 && m_ecsCoordinator) {
            Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
            if (fButtonTransform) {
                // Temporarily scale up the F button for feedback - based on new 4.5x scale
                fButtonTransform->scale.x = 5.2f; // Bigger feedback for larger base scale
                fButtonTransform->scale.y = 5.2f;
                // Note: This will be smoothed back by the pulsing animation
            }
        }
    }

    const char* MainMenuState::GetMenuOptionText(int optionIndex) const {
        switch (static_cast<MenuOption>(optionIndex)) {
            case MenuOption::PLAYING:    return "Playing";
            case MenuOption::OPTIONS:    return "Options";
            case MenuOption::QUICK_PLAY: return "Quick Play";
            case MenuOption::QUIT:       return "Quit";
            default:                     return "Unknown";
        }
    }

    int MainMenuState::GetMenuOptionCount() const {
        return static_cast<int>(MenuOption::COUNT);
    }

    bool MainMenuState::IsMobilePlatform() const {
        // TODO: Implement platform detection
        // This should check the actual platform we're running on
        // For now, return false (desktop) as default
        
        #ifdef __APPLE__
            #include "TargetConditionals.h"
            #if TARGET_OS_IPHONE
                return true;  // iOS
            #else
                return false; // macOS
            #endif
        #elif defined(__ANDROID__)
            return true;  // Android
        #else
            return false; // Desktop (Windows, Linux, etc.)
        #endif
    }

    void MainMenuState::CreateMenuButtons() {
        GN_LOG_INFO("Creating desktop menu buttons");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMenuButtons");
            return;
        }
        
        float centerX = m_screenWidth / 2.0f;
        float buttonY = m_screenHeight * 0.55f; // Position buttons higher up
        float buttonSpacing = 150.0f; // Much more spacing between buttons
        float buttonScale = 12.0f; // Even bigger buttons!
        
        // Create Play Button
        m_playButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform playTransform(Gnosis::GNVector2(centerX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite playSprite("FloppyButtonBlue", 200.0f, 60.0f);
        playSprite.layer = 2; // Button layer (lower than text)
        playSprite.visible = true;
        UIElement playButton("PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = 48.0f; // Increased for better readability
        playButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Play button with text: '%s' (length: %zu)", playButton.buttonText.c_str(), playButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_playButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_playButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_playButtonEntity, playButton);
        
        // Create Options Button
        m_optionsButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform optionsTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite optionsSprite("FloppyButtonBlue", 200.0f, 60.0f);
        optionsSprite.layer = 2; // Button layer (lower than text)
        optionsSprite.visible = true;
        UIElement optionsButton("OPTIONS", "FloppyButtonBlue", "FloppyButtonBlueHover");
        optionsButton.fontSize = 48.0f; // Increased for better readability
        optionsButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Options button with text: '%s' (length: %zu)", optionsButton.buttonText.c_str(), optionsButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_optionsButtonEntity, optionsTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_optionsButtonEntity, optionsSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_optionsButtonEntity, optionsButton);
        
        // Create Quick Play Button
        m_quickPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quickPlayTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 2), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quickPlaySprite("FloppyButtonBlue", 200.0f, 60.0f);
        quickPlaySprite.layer = 2; // Button layer (lower than text)
        quickPlaySprite.visible = true;
        UIElement quickPlayButton("QUICK PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quickPlayButton.fontSize = 42.0f; // Increased, but smaller for longer text
        quickPlayButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Quick Play button with text: '%s' (length: %zu)", quickPlayButton.buttonText.c_str(), quickPlayButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quickPlayButtonEntity, quickPlayTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quickPlayButtonEntity, quickPlaySprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_quickPlayButtonEntity, quickPlayButton);
        
        // Create Quit Button
        m_quitButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quitTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 3), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quitSprite("FloppyButtonBlue", 200.0f, 60.0f);
        quitSprite.layer = 2; // Button layer (lower than text)
        quitSprite.visible = true;
        UIElement quitButton("QUIT", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quitButton.fontSize = 48.0f; // Increased for better readability
        quitButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Quit button with text: '%s' (length: %zu)", quitButton.buttonText.c_str(), quitButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quitButtonEntity, quitSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_quitButtonEntity, quitButton);
        
        GN_LOG_INFO("Created desktop menu buttons: Play, Options, Quick Play, Quit");
    }

    void MainMenuState::CreateMobileMenuButtons() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMobileMenuButtons");
            return;
        }
        
        float centerX = m_screenWidth / 2.0f;
        float buttonY = m_screenHeight * 0.45f; // Position buttons higher up to avoid bottom overlap
        float buttonSpacing = 300.0f; // Increased spacing to eliminate Y-bound overlap
        float buttonScale = 12.0f; // Back to original scale
        
        GN_LOG_INFO("Creating mobile menu buttons - Screen height: " + std::to_string(m_screenHeight) + 
                   ", buttonY: " + std::to_string(buttonY) + 
                   ", buttonSpacing: " + std::to_string(buttonSpacing));
        
        // Create Play Button
        m_playButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform playTransform(Gnosis::GNVector2(centerX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        // Use actual scaled button dimensions: 64x16 texture * 12 scale = 768x192
        Sprite playSprite("FloppyButtonBlue", 64.0f * buttonScale, 16.0f * buttonScale);
        playSprite.layer = 2; // Button layer (lower than text)
        playSprite.visible = true;
        UIElement playButton("PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = 96.0f; // Scale up to match button size
        playButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created mobile Play button with text: '%s' (length: %zu)", playButton.buttonText.c_str(), playButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_playButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_playButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_playButtonEntity, playButton);
        GN_LOG_INFO("PLAY button Y position: " + std::to_string(buttonY));
        
        // Create Options Button
        m_optionsButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform optionsTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        // Use actual scaled button dimensions: 64x16 texture * 12 scale = 768x192
        Sprite optionsSprite("FloppyButtonBlue", 64.0f * buttonScale, 16.0f * buttonScale);
        optionsSprite.layer = 2; // Button layer (lower than text)
        optionsSprite.visible = true;
        UIElement optionsButton("OPTIONS", "FloppyButtonBlue", "FloppyButtonBlueHover");
        optionsButton.fontSize = 96.0f;
        optionsButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created mobile Options button with text: '%s' (length: %zu)", optionsButton.buttonText.c_str(), optionsButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_optionsButtonEntity, optionsTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_optionsButtonEntity, optionsSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_optionsButtonEntity, optionsButton);
        GN_LOG_INFO("OPTIONS button Y position: " + std::to_string(buttonY + buttonSpacing));
        
        // Create Quick Play Button
        m_quickPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quickPlayTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 2), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        // Use actual scaled button dimensions: 64x16 texture * 12 scale = 768x192
        Sprite quickPlaySprite("FloppyButtonBlue", 64.0f * buttonScale, 16.0f * buttonScale);
        quickPlaySprite.layer = 2; // Button layer (lower than text)
        quickPlaySprite.visible = true;
        UIElement quickPlayButton("QUICK PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quickPlayButton.fontSize = 84.0f; // Slightly smaller for longer text
        quickPlayButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created mobile Quick Play button with text: '%s' (length: %zu)", quickPlayButton.buttonText.c_str(), quickPlayButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quickPlayButtonEntity, quickPlayTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quickPlayButtonEntity, quickPlaySprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_quickPlayButtonEntity, quickPlayButton);
        GN_LOG_INFO("QUICK PLAY button Y position: " + std::to_string(buttonY + buttonSpacing * 2));
        
        // Create Quit Button
        m_quitButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quitTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 3), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        // Use actual scaled button dimensions: 64x16 texture * 12 scale = 768x192
        Sprite quitSprite("FloppyButtonBlue", 64.0f * buttonScale, 16.0f * buttonScale);
        quitSprite.layer = 2; // Button layer (lower than text)
        quitSprite.visible = true;
        UIElement quitButton("QUIT", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quitButton.fontSize = 96.0f;
        quitButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created mobile Quit button with text: '%s' (length: %zu)", quitButton.buttonText.c_str(), quitButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quitButtonEntity, quitSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_quitButtonEntity, quitButton);
        GN_LOG_INFO("QUIT button Y position: " + std::to_string(buttonY + buttonSpacing * 3));
        
        GN_LOG_INFO("Created mobile menu buttons: Play, Options, Quick Play, Quit");
    }

    void MainMenuState::DrawButtonDebugRectangles() {
        extern FloppyTurdGame* g_Game;
        if (!g_Game || !m_ecsCoordinator) {
            GN_LOG_INFO("DrawButtonDebugRectangles: No game or ECS coordinator");
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        if (!delegates.renderer.drawRectangle) {
            GN_LOG_INFO("DrawButtonDebugRectangles: No drawRectangle delegate");
            return;
        }
        
        GN_LOG_INFO("DrawButtonDebugRectangles: Drawing debug rectangles...");
        
        // Draw debug rectangles for each button
        std::vector<Gnosis::Entity> buttons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
        
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
        ShowLevelSelect();
    }

    void MainMenuState::OnOptionsButtonPressed() {
        GN_LOG_INFO("Options button pressed - transitioning to options menu");
        OnMenuOptionSelected(MenuOption::OPTIONS);
    }

    void MainMenuState::OnQuickPlayButtonPressed() {
        GN_LOG_INFO("Quick Play button pressed - starting level 1");
        OnMenuOptionSelected(MenuOption::QUICK_PLAY);
    }

    void MainMenuState::OnQuitButtonPressed() {
        GN_LOG_INFO("Quit button pressed - exiting game");
        OnMenuOptionSelected(MenuOption::QUIT);
    }

    void MainMenuState::CheckMenuButtonClicks(float touchX, float touchY) {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }
        
        // Reset all button states first
        ResetAllButtonStates();
        
        // Check Play Button
        if (m_playButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_playButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_playButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_playButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Use actual button texture dimensions (64x16) instead of sprite dimensions (200x60)
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 PLAY Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: PLAY BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_playButtonEntity, *uiElement);
                    OnPlayButtonPressed();
                    return;
                }
            }
        }
        
        // Check Options Button
        if (m_optionsButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_optionsButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_optionsButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_optionsButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Use actual button texture dimensions (64x16) instead of sprite dimensions (200x60)
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 OPTIONS Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: OPTIONS BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_optionsButtonEntity, *uiElement);
                    OnOptionsButtonPressed();
                    return;
                }
            }
        }
        
        // Check Quick Play Button
        if (m_quickPlayButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_quickPlayButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_quickPlayButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_quickPlayButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Use actual button texture dimensions (64x16) instead of sprite dimensions (200x60)
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 QUICK PLAY Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: QUICK PLAY BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_quickPlayButtonEntity, *uiElement);
                    OnQuickPlayButtonPressed();
                    return;
                }
            }
        }
        
        // Check Quit Button
        if (m_quitButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_quitButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_quitButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_quitButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Use actual button texture dimensions (64x16) instead of sprite dimensions (200x60)
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 QUIT Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: QUIT BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_quitButtonEntity, *uiElement);
                    OnQuitButtonPressed();
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
        std::vector<Gnosis::Entity> buttonEntities = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
        
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
        m_levels.clear();
        
        // Add all levels with their painting textures
        m_levels.push_back({"A Flop in the Park", "ParkLevelPainting", "LockedPainting", true, 1});      // Park is unlocked
        m_levels.push_back({"Home Sweet Home", "SewerLevelPainting", "LockedPainting", false, 2});       // Sewer is locked
        m_levels.push_back({"The Good, The Bad,\nand the Stinky", "DesertLevelPainting", "LockedPainting", false, 3}); // Desert is locked
        m_levels.push_back({"Polar Pandemonium", "SnowLevelPainting", "LockedPainting", false, 4});      // Snow is locked
        m_levels.push_back({"Dung in the Dungeon", "CastleLevelPainting", "LockedPainting", false, 5});  // Castle is locked
        m_levels.push_back({"Curtains for Crap", "RatKingPainting", "LockedPainting", false, 6});       // Boss is locked
        
        m_currentLevelIndex = 0;
        GN_LOG_INFO("Initialized " + std::to_string(m_levels.size()) + " levels");
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
        HideLevelSelect();
        
        GN_LOG_INFO("Level select layout created");
    }

    void MainMenuState::CreateArrowButtons() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // Create left arrow button
        m_leftArrowButtonEntity = m_ecsCoordinator->CreateEntity();
        float leftButtonX = m_screenWidth * 0.1f;  // 10% from left edge
        float buttonY = m_screenHeight / 2.0f;     // Center vertically
        float buttonScale = m_isMobile ? 4.0f : 2.0f;
        
        Transform leftTransform(Gnosis::GNVector2(leftButtonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite leftSprite("LeftArrow", 100.0f, 100.0f);
        leftSprite.layer = 5; // Top layer
        leftSprite.visible = false;
                    UIElement leftButton("", "LeftArrow", "LeftArrowHover"); // Remove text, keep arrow sprite
        leftButton.fontSize = m_isMobile ? 96.0f : 48.0f;
        leftButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        leftButton.visible = false;
        m_ecsCoordinator->AddComponent<Transform>(m_leftArrowButtonEntity, leftTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_leftArrowButtonEntity, leftSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_leftArrowButtonEntity, leftButton);
        
        // Create right arrow button
        m_rightArrowButtonEntity = m_ecsCoordinator->CreateEntity();
        float rightButtonX = m_screenWidth * 0.9f;  // 90% from left edge (10% from right)
        
        Transform rightTransform(Gnosis::GNVector2(rightButtonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite rightSprite("RightArrow", 100.0f, 100.0f);
        rightSprite.layer = 5; // Top layer
        rightSprite.visible = false;
                    UIElement rightButton("", "RightArrow", "RightArrowHover"); // Remove text, keep arrow sprite
        rightButton.fontSize = m_isMobile ? 96.0f : 48.0f;
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
        indicatorElement.fontSize = m_isMobile ? 72.0f : 36.0f; // Big red text
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
        
        float centerX = m_screenWidth / 2.0f;
        float centerY = m_screenHeight / 2.0f;
        float paintingSpacing = m_screenWidth * 1.2f; // Spacing between paintings
        float paintingScale = m_isMobile ? 6.0f : 2.0f;
        
        m_levelPaintingEntities.clear();
        m_levelFrameEntities.clear();
        m_levelTextEntities.clear();
        
        for (size_t i = 0; i < m_levels.size(); ++i) {
            // Create painting entity
            Gnosis::Entity paintingEntity = m_ecsCoordinator->CreateEntity();
            float xPos = centerX + (i - m_currentLevelIndex) * paintingSpacing;
            Transform paintingTransform(Gnosis::GNVector2(xPos, centerY), 0.0f, Gnosis::GNVector2(paintingScale, paintingScale));
            Sprite paintingSprite(m_levels[i].paintingTexture, 200.0f, 150.0f);
            paintingSprite.layer = 3; // Above background, below UI
            paintingSprite.visible = false; // Initially hidden
            m_ecsCoordinator->AddComponent<Transform>(paintingEntity, paintingTransform);
            m_ecsCoordinator->AddComponent<Sprite>(paintingEntity, paintingSprite);
            m_levelPaintingEntities.push_back(paintingEntity);
            
            GN_LOG_INFO("🎨 Created painting for " + m_levels[i].name + " with texture: " + m_levels[i].paintingTexture + 
                       " at position (" + std::to_string(xPos) + ", " + std::to_string(centerY) + ")");
            GN_LOG_INFO("🎨 Painting entity ID: " + std::to_string(paintingEntity) + ", Sprite textureId: " + paintingSprite.textureId);
            
            // Create frame entity (locked/unlocked indicator)
            Gnosis::Entity frameEntity = m_ecsCoordinator->CreateEntity();
            Transform frameTransform(Gnosis::GNVector2(xPos, centerY), 0.0f, Gnosis::GNVector2(paintingScale * 1.1f, paintingScale * 1.1f));
            std::string frameTexture = m_levels[i].isUnlocked ? "" : "LockedPainting"; // Use LockedPainting for locked levels, empty for unlocked
            Sprite frameSprite(frameTexture, 220.0f, 170.0f);
            frameSprite.layer = 4; // Above paintings
            frameSprite.visible = false;
            m_ecsCoordinator->AddComponent<Transform>(frameEntity, frameTransform);
            m_ecsCoordinator->AddComponent<Sprite>(frameEntity, frameSprite);
            m_levelFrameEntities.push_back(frameEntity);
            
            // Create level name text entity
            Gnosis::Entity textEntity = m_ecsCoordinator->CreateEntity();
            Transform textTransform(Gnosis::GNVector2(xPos, m_screenHeight * 0.15f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f)); // Position at top 15% of screen
            UIElement textElement(m_levels[i].name, "", "");
            textElement.fontSize = m_isMobile ? 64.0f : 24.0f; // Even larger font for mobile readability
            textElement.textColor = Gnosis::GNColor(255, 255, 255, 255);
            textElement.visible = false;
            m_ecsCoordinator->AddComponent<Transform>(textEntity, textTransform);
            m_ecsCoordinator->AddComponent<UIElement>(textEntity, textElement);
            m_levelTextEntities.push_back(textEntity);
        }
        
        GN_LOG_INFO("Created " + std::to_string(m_levels.size()) + " level paintings");
    }

    void MainMenuState::CreateBackButton() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        m_backButtonEntity = m_ecsCoordinator->CreateEntity();
        float buttonX = m_screenWidth / 2.0f;  // Center horizontally
        float buttonY = m_screenHeight * 0.9f;  // 90% from top (near bottom)
        float buttonScale = m_isMobile ? 8.0f : 3.0f;
        
        Transform backTransform(Gnosis::GNVector2(buttonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite backSprite("FloppyButtonBlue", 200.0f, 60.0f);
        backSprite.layer = 5; // Top layer
        backSprite.visible = false;
        UIElement backButton("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
        backButton.fontSize = m_isMobile ? 64.0f : 32.0f;
        backButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        backButton.visible = false;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backButtonEntity, backTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backButtonEntity, backSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_backButtonEntity, backButton);
        
        GN_LOG_INFO("Created back button");
    }

    void MainMenuState::CreateLevelPlayButton() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        m_levelPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        float buttonX = m_screenWidth / 2.0f;
        float buttonY = m_screenHeight * 0.8f;
        float buttonScale = m_isMobile ? 10.0f : 4.0f;
        
        Transform playTransform(Gnosis::GNVector2(buttonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite playSprite("FloppyButtonBlue", 200.0f, 60.0f);
        playSprite.layer = 5; // Top layer
        playSprite.visible = false;
        UIElement playButton("PLAY LEVEL", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = m_isMobile ? 72.0f : 36.0f;
        playButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        playButton.visible = false;
        
        m_ecsCoordinator->AddComponent<Transform>(m_levelPlayButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_levelPlayButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_levelPlayButtonEntity, playButton);
        
        GN_LOG_INFO("Created level play button");
    }

    void MainMenuState::ShowLevelSelect() {
        GN_LOG_INFO("Showing level select menu");
        m_currentMode = MenuMode::LEVEL_SELECT;
        
        // Hide main menu elements
        if (m_logoEntity != 0) {
            Sprite* logoSprite = m_ecsCoordinator->GetComponent<Sprite>(m_logoEntity);
            if (logoSprite) logoSprite->visible = false;
        }
        if (m_fButtonEntity != 0) {
            Sprite* fSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
            if (fSprite) fSprite->visible = false;
        }
        
        // Hide main menu buttons
        std::vector<Gnosis::Entity> mainButtons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
        for (Gnosis::Entity entity : mainButtons) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (sprite) sprite->visible = false;
                if (uiElement) uiElement->visible = false;
            }
        }
        
        // Show level select elements
        UpdateLevelVisibility();
        
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
        GN_LOG_INFO("Hiding level select menu");
        m_currentMode = MenuMode::MAIN_MENU;
        
        // Show main menu elements
        if (m_logoEntity != 0) {
            Sprite* logoSprite = m_ecsCoordinator->GetComponent<Sprite>(m_logoEntity);
            if (logoSprite) logoSprite->visible = true;
        }
        if (m_fButtonEntity != 0) {
            Sprite* fSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
            if (fSprite) fSprite->visible = true;
        }
        
        // Show main menu buttons
        std::vector<Gnosis::Entity> mainButtons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
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
    }

    void MainMenuState::HandleLevelSelectInput() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        extern FloppyTurdGame* g_Game;
        if (!g_Game) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        
        // Handle swipe input
        HandleSwipeInput();
        
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
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                        float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                        float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                        float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                        
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
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                        float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                        float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                        float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnLevelPlayButtonPressed();
                            return;
                        }
                    }
                }
                
                // Check left arrow button
                if (m_leftArrowButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_leftArrowButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_leftArrowButtonEntity);
                    
                    if (transform && sprite) {
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                        float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                        float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                        float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                        
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
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                        float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                        float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                        float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                        
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
        extern FloppyTurdGame* g_Game;
        if (!g_Game) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        
        // Use the new gesture detection methods
        bool swipeLeft = delegates.input.isSwipeLeftDetected ? delegates.input.isSwipeLeftDetected() : false;
        bool swipeRight = delegates.input.isSwipeRightDetected ? delegates.input.isSwipeRightDetected() : false;
        
        GN_LOG_DEBUG("Gesture Input: Left=" + std::to_string(swipeLeft) + 
                    " Right=" + std::to_string(swipeRight));
        
        if (swipeLeft && m_currentLevelIndex < m_levels.size() - 1) {
            // Swipe left - go to next level
            m_currentLevelIndex++;
            GN_LOG_INFO("Gesture detected: Swipe left - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Start animation with lurch feel
            m_swipeAnimationTimer = 0.0f;
            CenterCurrentLevel();
        } else if (swipeRight && m_currentLevelIndex > 0) {
            // Swipe right - go to previous level
            m_currentLevelIndex--;
            GN_LOG_INFO("Gesture detected: Swipe right - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Start animation with lurch feel
            m_swipeAnimationTimer = 0.0f;
            CenterCurrentLevel();
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
            
            // Start animation to center the new level
            m_swipeAnimationTimer = 0.0f;
            CenterCurrentLevel();
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
            
            // Update painting positions
            float centerX = m_screenWidth / 2.0f;
            float paintingSpacing = m_screenWidth * 1.2f;
            
            for (size_t i = 0; i < m_levelPaintingEntities.size(); ++i) {
                if (m_levelPaintingEntities[i] != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                    if (transform) {
                        float targetX = centerX + (i - m_currentLevelIndex) * paintingSpacing;
                        transform->position.x = targetX;
                    }
                }
                
                if (m_levelFrameEntities[i] != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelFrameEntities[i]);
                    if (transform) {
                        float targetX = centerX + (i - m_currentLevelIndex) * paintingSpacing;
                        transform->position.x = targetX;
                    }
                }
                
                if (m_levelTextEntities[i] != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelTextEntities[i]);
                    if (transform) {
                        float targetX = centerX + (i - m_currentLevelIndex) * paintingSpacing;
                        transform->position.x = targetX;
                    }
                }
            }
        }
    }

    void MainMenuState::UpdateLevelVisibility() {
        float centerX = m_screenWidth / 2.0f;
        float centerY = m_screenHeight / 2.0f;
        float paintingSpacing = m_screenWidth * 0.8f; // Reduced from 1.2f to 0.8f for closer spacing
        float visibilityThreshold = paintingSpacing * 1.5f;
        
        GN_LOG_DEBUG("🎨 Screen: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) + 
                    ", Spacing: " + std::to_string(paintingSpacing) + 
                    ", Current Level: " + std::to_string(m_currentLevelIndex));
        
        for (size_t i = 0; i < m_levelPaintingEntities.size(); ++i) {
            // Calculate the target position for this painting
            float targetX = centerX + (i - m_currentLevelIndex) * paintingSpacing;
            float distanceFromCenter = std::abs(static_cast<float>(i - m_currentLevelIndex)) * paintingSpacing;
            bool shouldBeVisible = distanceFromCenter <= visibilityThreshold;
            
            // Update painting position, visibility and texture
            if (m_levelPaintingEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPaintingEntities[i]);
                if (transform && sprite) {
                    // Update position to center the current level
                    transform->position.x = targetX;
                    transform->position.y = centerY;
                    
                    // Update visibility
                    sprite->visible = shouldBeVisible;
                    
                    // Update the texture to show the correct level painting
                    if (i < m_levels.size()) {
                        std::string oldTexture = sprite->textureId;
                        sprite->textureId = m_levels[i].paintingTexture;
                        GN_LOG_INFO("🎨 Updated painting " + std::to_string(i) + " from '" + oldTexture + "' to texture: '" + m_levels[i].paintingTexture + "' at position (" + std::to_string(targetX) + ", " + std::to_string(centerY) + ")");
                    }
                }
            }
            
            // Update frame position and visibility
            if (m_levelFrameEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelFrameEntities[i]);
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelFrameEntities[i]);
                if (transform && sprite) {
                    // Update position to match the painting
                    transform->position.x = targetX;
                    transform->position.y = centerY;
                    
                    // Update visibility - only show frame if level is locked
                    sprite->visible = shouldBeVisible && !m_levels[i].isUnlocked;
                }
            }
            
            // Update text position and visibility
            if (m_levelTextEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelTextEntities[i]);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_levelTextEntities[i]);
                if (transform && uiElement) {
                    // Update position to stay at top of screen
                    transform->position.x = targetX;
                    transform->position.y = m_screenHeight * 0.15f; // Keep at top 15% of screen
                    
                    // Update visibility
                    uiElement->visible = shouldBeVisible;
                    
                    // Update the text to show the correct level name
                    if (i < m_levels.size()) {
                        uiElement->buttonText = m_levels[i].name;
                        GN_LOG_DEBUG("Updated text " + std::to_string(i) + " to: " + m_levels[i].name);
                    }
                }
            }
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
                
                GN_LOG_DEBUG("🔒 Locked indicator visibility: " + std::string(uiElement->visible ? "SHOWN" : "HIDDEN") + 
                            " for level: " + m_levels[m_currentLevelIndex].name);
            }
        }
        
        GN_LOG_INFO("Updated level visibility for current level: " + std::to_string(m_currentLevelIndex + 1));
    }

    void MainMenuState::CenterCurrentLevel() {
        UpdateLevelVisibility();
    }

    void MainMenuState::OnBackButtonPressed() {
        GN_LOG_INFO("Back button pressed - returning to main menu");
        HideLevelSelect();
    }

    void MainMenuState::OnLevelPlayButtonPressed() {
        GN_LOG_INFO("Level play button pressed - starting level " + std::to_string(m_currentLevelIndex + 1));
        OnLevelSelected(m_currentLevelIndex);
    }

    void MainMenuState::OnLeftArrowPressed() {
        if (m_currentLevelIndex > 0) {
            m_currentLevelIndex--;
            GN_LOG_INFO("Left arrow pressed - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Start animation with lurch feel
            m_swipeAnimationTimer = 0.0f;
            CenterCurrentLevel();
        }
    }

    void MainMenuState::OnRightArrowPressed() {
        if (m_currentLevelIndex < m_levels.size() - 1) {
            m_currentLevelIndex++;
            GN_LOG_INFO("Right arrow pressed - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Start animation with lurch feel
            m_swipeAnimationTimer = 0.0f;
            CenterCurrentLevel();
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
            
            // Store the selected level index for the transition
            m_selectedLevelIndex = levelIndex;
            m_finished = true;
        }
    }

    void MainMenuState::DrawLevelSelectDebugInfo() {
        extern FloppyTurdGame* g_Game;
        if (!g_Game || !m_ecsCoordinator) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        if (!delegates.renderer.drawRectangle) {
            return;
        }
        
        // Draw debug info for level select elements
        GN_LOG_INFO("Level Select Debug: Current level " + std::to_string(m_currentLevelIndex + 1) + 
                   " of " + std::to_string(m_levels.size()) + 
                   ", Mode: " + (m_currentMode == MenuMode::LEVEL_SELECT ? "LEVEL_SELECT" : "MAIN_MENU"));
    }

} // namespace GameCore
