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
        , m_backgroundEntity(0)
        , m_logoEntity(0)
        , m_fButtonEntity(0)
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
            if (delegates.audio.playMusic) {
                delegates.audio.playMusic("FloppyTurdMenu", 0.7f, -1); // -1 = infinite loop
                GN_LOG_INFO("Started main menu music: FloppyTurdMenu.mp3");
            }
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
        
        // Handle touch/click input for F button
        if (delegates.input.isPrimaryInputJustPressed && delegates.input.isPrimaryInputJustPressed()) {
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
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnFButtonPressed();
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
        
        // TODO: Create desktop UI elements:
        // 1. Background (MainMenu.png)
        // 2. Floppy Turd Logo at top center (FloppyLogo.png)
        // 3. Interactive F button in logo (F.png) - plays fart sound when clicked
        // 4. Vertical menu buttons in center:
        //    - Playing
        //    - Options  
        //    - Quick Play
        //    - Quit
        // 5. Menu selection highlight/cursor (keyboard/gamepad navigation)
        
        // Asset paths for desktop layout:
        // Background: AssetPaths::Graphics::UI::UI_MENUS_MAIN + "MainMenu.png"
        // Logo: AssetPaths::Graphics::UI::UI_MENUS_MAIN + "FloppyLogo.png"
        // F Button: AssetPaths::Graphics::UI::UI_ICONS + "F.png"
        
        GN_LOG_INFO("Desktop layout: Logo at top, vertical menu buttons, keyboard navigation");
    }

    void MainMenuState::CreateMobileLayout() {
        GN_LOG_INFO("Creating mobile main menu layout");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMobileLayout");
            return;
        }
        
        // Get actual screen dimensions from platform (use realistic mobile portrait dimensions)
        float screenWidth = 1179.0f;
        float screenHeight = 2556.0f;
        float centerX = screenWidth / 2.0f;
        
        // 1. Create Background Entity (MainMenuMobile.png) - FULL SCREEN SCALING
        m_backgroundEntity = m_ecsCoordinator->CreateEntity();
        Transform bgTransform(Gnosis::GNVector2(centerX, screenHeight / 2.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        // Background should fill entire screen - use viewport dimensions as sprite size
        Sprite bgSprite("MainMenuMobile", screenWidth, screenHeight);
        bgSprite.layer = 0; // Background layer
        bgSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_backgroundEntity, bgTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        GN_LOG_INFO("Created full-screen background entity: MainMenuMobile.png (%fx%f)", screenWidth, screenHeight);
        
        // 2. Create Logo Entity (FloppyLogo.png) - MUCH LARGER SCALING (~3x)
        m_logoEntity = m_ecsCoordinator->CreateEntity();
        float logoY = screenHeight * 0.25f; // Top quarter
        Transform logoTransform(Gnosis::GNVector2(centerX, logoY), 0.0f, Gnosis::GNVector2(3.5f, 3.5f)); // 3.5x scale for mobile
        Sprite logoSprite("FloppyLogo", 400.0f, 200.0f); // Base size, will be scaled by transform
        logoSprite.layer = 1; // Logo layer
        logoSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_logoEntity, logoSprite);
        GN_LOG_INFO("Created scaled logo entity: FloppyLogo.png (3.5x scale)");
        
        // 3. Create Interactive F Button Entity (F.png) - MUCH LARGER SCALING (~3x)
        m_fButtonEntity = m_ecsCoordinator->CreateEntity();
        // Position F button relative to scaled logo - need to adjust for larger logo
        float fButtonX = centerX + 380.0f; // Adjusted for 3.5x scaled logo width
        float fButtonY = logoY + 35.0f; // Slightly below logo center, adjusted for scale
        Transform fButtonTransform(Gnosis::GNVector2(fButtonX, fButtonY), 0.0f, Gnosis::GNVector2(4.5f, 4.5f)); // Even larger for touch
        Sprite fButtonSprite("F", 80.0f, 80.0f); // Base size, will be scaled by transform
        fButtonSprite.layer = 2; // F button layer
        fButtonSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_fButtonEntity, fButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_fButtonEntity, fButtonSprite);
        GN_LOG_INFO("Created large F button entity: F.png (4.5x scale)");
        
        GN_LOG_INFO("Mobile layout created: Background, Logo, and F Button entities");
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
                float screenHeight = 2556.0f;
                float originalLogoY = screenHeight * 0.25f;
                logoTransform->position.y = originalLogoY + logoFloat;
            }
        }
        
        // F Button gentle pulsing animation
        if (m_fButtonEntity != 0) {
            Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
            if (fButtonTransform) {
                // Create a subtle pulsing scale effect - based on new 4.5x base scale
                float pulseScale = 4.5f + sin(m_animationTimer * 2.5f) * 0.3f; // 4.2f to 4.8f scale range
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
        
        // Play random fart sound (fart1.ogg through fart11.ogg)
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
                GN_LOG_INFO("Playing fart sound: %s.ogg", fartSoundName.c_str());
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

} // namespace GameCore
