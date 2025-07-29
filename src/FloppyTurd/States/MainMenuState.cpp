#include "MainMenuState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include <iostream>

namespace GameCore {

    MainMenuState::MainMenuState(Gnosis::ECS* ecsCoordinator)
        : m_ecsCoordinator(ecsCoordinator)
        , m_finished(false)
        , m_selectedOption(0)
        , m_animationTimer(0.0f)
        , m_isMobile(false) {
        
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
        
        // TODO: Initialize platform-specific UI rendering for menu
        
        // Start playing main menu music
        // TODO: Use AssetManager to play menu music
        GN_LOG_INFO("Starting main menu music");
    }

    void MainMenuState::Exit() {
        GN_LOG_INFO("Exiting Main Menu State");
        
        // Stop menu music
        // TODO: Stop menu music through AssetManager
        // TODO: Cleanup platform-specific UI rendering
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
        // TODO: Get input from platform delegates
        // Handle menu navigation (up/down arrows)
        // Handle selection (enter/space)
        // Handle F button press for fart sound
        
        // For now, just log input handling
        // GN_LOG_DEBUG("Handling main menu input");
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
        
        // TODO: Create mobile UI elements:
        // 1. Background (MainMenuMobile.png) - optimized for mobile aspect ratio
        // 2. Floppy Turd Logo at top (FloppyLogo.png) - scaled for mobile
        // 3. Interactive F button in logo (F.png) - larger touch target
        // 4. Touch-friendly menu buttons with larger spacing:
        //    - Playing
        //    - Options  
        //    - Quick Play
        //    - Quit
        // 5. No cursor - direct touch interaction
        
        // Asset paths for mobile layout:
        // Background: AssetPaths::Graphics::UI::UI_MENUS_MAIN + "MainMenuMobile.png"
        // Logo: AssetPaths::Graphics::UI::UI_MENUS_MAIN + "FloppyLogo.png"
        // F Button: AssetPaths::Graphics::UI::UI_ICONS + "F.png"
        
        GN_LOG_INFO("Mobile layout: Touch-optimized buttons, larger spacing, no cursor");
    }

    void MainMenuState::UpdateMenuSelection() {
        // TODO: Handle menu selection logic
        // - Update selected option based on input
        // - Update visual highlights
        // - Handle selection confirmation
        
        // Placeholder for menu selection updates
    }

    void MainMenuState::UpdateMenuAnimations(float deltaTime) {
        // TODO: Update menu animations
        // - Logo bobbing/floating animation
        // - Button hover effects
        // - Background animations
        // - Particle effects
        
        // Simple animation timer for now
        float logoFloat = sin(m_animationTimer * 2.0f) * 5.0f; // 5 pixel float
        
        // This will be used to update entity positions when ECS is integrated
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
        GN_LOG_INFO("F button pressed - playing fart sound!");
        
        // TODO: Play fart sound through AssetManager
        // AssetManager::getInstance().playSound(AssetPaths::Audio::SFX::SFX_PLAYER + "fart1.ogg");
        
        // For now, just log it
        GN_LOG_INFO("*FART SOUND*");
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
