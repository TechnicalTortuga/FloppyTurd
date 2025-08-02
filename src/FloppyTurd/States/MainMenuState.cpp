#include "MainMenuState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include <iostream>
#include <random>

namespace GameCore {

    MainMenuState::MainMenuState(Gnosis::ECS* ecsCoordinator)
        : m_ecsCoordinator(ecsCoordinator)
        , m_finished(false)
        , m_selectedOption(0)
        , m_animationTimer(0.0f)
        , m_isMobile(false)
        , m_screenWidth(800.0f)
        , m_screenHeight(600.0f)
        , m_backgroundEntity(0)
        , m_logoEntity(0)
        , m_fButtonEntity(0)
        , m_playButtonEntity(0)
        , m_optionsButtonEntity(0)
        , m_quickPlayButtonEntity(0)
        , m_quitButtonEntity(0)
        , m_fontLoaded(false)
        , m_assetsLoaded(false) {
        
        // Detect if we're on a mobile platform
        m_isMobile = IsMobilePlatform();
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
            GN_LOG_INFO("Cleaned up main menu UI entities");
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
        
        // Update ECS systems
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Update(deltaTime);
        }
    }

    void MainMenuState::Render() {
        // Render through ECS system
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Render();
        }
        
        // Render button text using platform delegates
        RenderButtonText();
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
                        GN_LOG_INFO("🎯 MainMenuState: Touch at (%f, %f), F button bounds: L=%f R=%f T=%f B=%f", 
                                   touchX, touchY, buttonLeft, buttonRight, buttonTop, buttonBottom);
                        
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
        playSprite.layer = 3; // Button layer
        playSprite.visible = true;
        Button playButton("PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = 32.0f;
        GN_LOG_INFO("Created Play button with text: '%s' (length: %zu)", playButton.buttonText.c_str(), playButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_playButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_playButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<Button>(m_playButtonEntity, playButton);
        
        // Create Options Button
        m_optionsButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform optionsTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite optionsSprite("FloppyButtonBlue", 200.0f, 60.0f);
        optionsSprite.layer = 3;
        optionsSprite.visible = true;
        Button optionsButton("OPTIONS", "FloppyButtonBlue", "FloppyButtonBlueHover");
        optionsButton.fontSize = 32.0f;
        GN_LOG_INFO("Created Options button with text: '%s' (length: %zu)", optionsButton.buttonText.c_str(), optionsButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_optionsButtonEntity, optionsTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_optionsButtonEntity, optionsSprite);
        m_ecsCoordinator->AddComponent<Button>(m_optionsButtonEntity, optionsButton);
        
        // Create Quick Play Button
        m_quickPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quickPlayTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 2), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quickPlaySprite("FloppyButtonBlue", 200.0f, 60.0f);
        quickPlaySprite.layer = 3;
        quickPlaySprite.visible = true;
        Button quickPlayButton("QUICK PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quickPlayButton.fontSize = 28.0f; // Slightly smaller for longer text
        GN_LOG_INFO("Created Quick Play button with text: '%s' (length: %zu)", quickPlayButton.buttonText.c_str(), quickPlayButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quickPlayButtonEntity, quickPlayTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quickPlayButtonEntity, quickPlaySprite);
        m_ecsCoordinator->AddComponent<Button>(m_quickPlayButtonEntity, quickPlayButton);
        
        // Create Quit Button
        m_quitButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quitTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 3), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quitSprite("FloppyButtonBlue", 200.0f, 60.0f);
        quitSprite.layer = 3;
        quitSprite.visible = true;
        Button quitButton("QUIT", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quitButton.fontSize = 32.0f;
        GN_LOG_INFO("Created Quit button with text: '%s' (length: %zu)", quitButton.buttonText.c_str(), quitButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quitButtonEntity, quitSprite);
        m_ecsCoordinator->AddComponent<Button>(m_quitButtonEntity, quitButton);
        
        GN_LOG_INFO("Created desktop menu buttons: Play, Options, Quick Play, Quit");
    }

    void MainMenuState::CreateMobileMenuButtons() {
        GN_LOG_INFO("Creating mobile menu buttons");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMobileMenuButtons");
            return;
        }
        
        float centerX = m_screenWidth / 2.0f;
        float buttonY = m_screenHeight * 0.60f; // Position buttons higher up
        float buttonSpacing = 180.0f; // Much more spacing for mobile
        float buttonScale = 12.0f; // Even bigger buttons for mobile!
        
        // Create Play Button
        m_playButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform playTransform(Gnosis::GNVector2(centerX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite playSprite("FloppyButtonBlue", 200.0f, 60.0f);
        playSprite.layer = 3; // Button layer
        playSprite.visible = true;
        Button playButton("PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = 48.0f; // Larger font for mobile
        GN_LOG_INFO("Created mobile Play button with text: '%s' (length: %zu)", playButton.buttonText.c_str(), playButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_playButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_playButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<Button>(m_playButtonEntity, playButton);
        
        // Create Options Button
        m_optionsButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform optionsTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite optionsSprite("FloppyButtonBlue", 200.0f, 60.0f);
        optionsSprite.layer = 3;
        optionsSprite.visible = true;
        Button optionsButton("OPTIONS", "FloppyButtonBlue", "FloppyButtonBlueHover");
        optionsButton.fontSize = 48.0f;
        GN_LOG_INFO("Created mobile Options button with text: '%s' (length: %zu)", optionsButton.buttonText.c_str(), optionsButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_optionsButtonEntity, optionsTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_optionsButtonEntity, optionsSprite);
        m_ecsCoordinator->AddComponent<Button>(m_optionsButtonEntity, optionsButton);
        
        // Create Quick Play Button
        m_quickPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quickPlayTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 2), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quickPlaySprite("FloppyButtonBlue", 200.0f, 60.0f);
        quickPlaySprite.layer = 3;
        quickPlaySprite.visible = true;
        Button quickPlayButton("QUICK PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quickPlayButton.fontSize = 42.0f; // Slightly smaller for longer text
        GN_LOG_INFO("Created mobile Quick Play button with text: '%s' (length: %zu)", quickPlayButton.buttonText.c_str(), quickPlayButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quickPlayButtonEntity, quickPlayTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quickPlayButtonEntity, quickPlaySprite);
        m_ecsCoordinator->AddComponent<Button>(m_quickPlayButtonEntity, quickPlayButton);
        
        // Create Quit Button
        m_quitButtonEntity = m_ecsCoordinator->CreateEntity();
        Transform quitTransform(Gnosis::GNVector2(centerX, buttonY + buttonSpacing * 3), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quitSprite("FloppyButtonBlue", 200.0f, 60.0f);
        quitSprite.layer = 3;
        quitSprite.visible = true;
        Button quitButton("QUIT", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quitButton.fontSize = 48.0f;
        GN_LOG_INFO("Created mobile Quit button with text: '%s' (length: %zu)", quitButton.buttonText.c_str(), quitButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quitButtonEntity, quitSprite);
        m_ecsCoordinator->AddComponent<Button>(m_quitButtonEntity, quitButton);
        
        GN_LOG_INFO("Created mobile menu buttons: Play, Options, Quick Play, Quit");
    }

    void MainMenuState::OnPlayButtonPressed() {
        GN_LOG_INFO("Play button pressed - transitioning to level select");
        OnMenuOptionSelected(MenuOption::PLAYING);
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
        
        // Check Play Button
        if (m_playButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_playButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_playButtonEntity);
            
            if (transform && sprite) {
                float buttonWidth = sprite->width * transform->scale.x;
                float buttonHeight = sprite->height * transform->scale.y;
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: PLAY BUTTON HIT!");
                    OnPlayButtonPressed();
                    return;
                }
            }
        }
        
        // Check Options Button
        if (m_optionsButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_optionsButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_optionsButtonEntity);
            
            if (transform && sprite) {
                float buttonWidth = sprite->width * transform->scale.x;
                float buttonHeight = sprite->height * transform->scale.y;
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: OPTIONS BUTTON HIT!");
                    OnOptionsButtonPressed();
                    return;
                }
            }
        }
        
        // Check Quick Play Button
        if (m_quickPlayButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_quickPlayButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_quickPlayButtonEntity);
            
            if (transform && sprite) {
                float buttonWidth = sprite->width * transform->scale.x;
                float buttonHeight = sprite->height * transform->scale.y;
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: QUICK PLAY BUTTON HIT!");
                    OnQuickPlayButtonPressed();
                    return;
                }
            }
        }
        
        // Check Quit Button
        if (m_quitButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_quitButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_quitButtonEntity);
            
            if (transform && sprite) {
                float buttonWidth = sprite->width * transform->scale.x;
                float buttonHeight = sprite->height * transform->scale.y;
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonRight = transform->position.x + (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                float buttonBottom = transform->position.y + (buttonHeight / 2.0f);
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: QUIT BUTTON HIT!");
                    OnQuitButtonPressed();
                    return;
                }
            }
        }
        
        GN_LOG_INFO("❌ MainMenuState: Touch missed all buttons");
    }

    void MainMenuState::RenderButtonText() {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            GN_LOG_INFO("❌ RenderButtonText: ECS coordinator or assets not ready");
            return;
        }
        
        if (!m_fontLoaded) {
            GN_LOG_INFO("❌ RenderButtonText: Font not loaded yet");
            return;
        }
        
        // Get platform delegates for text rendering
        extern FloppyTurdGame* g_Game;
        if (!g_Game) {
            GN_LOG_INFO("❌ RenderButtonText: Game instance not available");
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        if (!delegates.renderer.drawText) {
            GN_LOG_INFO("❌ RenderButtonText: drawText delegate not available");
            return;
        }
        
        GN_LOG_INFO("🎨 RenderButtonText: Rendering text for all buttons");
        
        // Render text for each button
        RenderButtonTextForEntity(m_playButtonEntity, delegates);
        RenderButtonTextForEntity(m_optionsButtonEntity, delegates);
        RenderButtonTextForEntity(m_quickPlayButtonEntity, delegates);
        RenderButtonTextForEntity(m_quitButtonEntity, delegates);
        
        GN_LOG_INFO("🎨 RenderButtonText: Finished text rendering");
    }
    
    void MainMenuState::RenderButtonTextForEntity(Gnosis::Entity entity, const PlatformDelegates& delegates) {
        if (entity == 0) {
            GN_LOG_INFO("❌ RenderButtonTextForEntity: Entity is null");
            return;
        }
        
        Transform* transform = m_ecsCoordinator->GetComponent<Transform>(entity);
        Button* button = m_ecsCoordinator->GetComponent<Button>(entity);
        
        if (!transform || !button) {
            GN_LOG_INFO("❌ RenderButtonTextForEntity: Transform or Button component missing for entity %d", entity);
            return;
        }
        
        // Calculate text position (center of button)
        float textX = transform->position.x;
        float textY = transform->position.y;
        
        // Choose text color based on button state
        float r, g, b, a;
        if (button->isHovered) {
            r = button->textHoverColor.r / 255.0f;
            g = button->textHoverColor.g / 255.0f;
            b = button->textHoverColor.b / 255.0f;
            a = button->textHoverColor.a / 255.0f;
        } else {
            r = button->textColor.r / 255.0f;
            g = button->textColor.g / 255.0f;
            b = button->textColor.b / 255.0f;
            a = button->textColor.a / 255.0f;
        }
        
        // Draw the text using platform delegates
        GN_LOG_INFO("🎨 Rendering button text: '%s' at (%.1f, %.1f) with size %.1f, color (%.2f, %.2f, %.2f, %.2f)", 
                   button->buttonText.c_str(), textX, textY, button->fontSize, r, g, b, a);
        delegates.renderer.drawText(button->buttonText, textX, textY, button->fontSize, r, g, b, a);
    }

} // namespace GameCore
