#ifndef FLOPPY_TURD_MAIN_MENU_STATE_H
#define FLOPPY_TURD_MAIN_MENU_STATE_H

#include "GameState.h"
#include "../../Engine/Platform/PlatformDelegates.h"

namespace GameCore {

    /**
     * @brief Main Menu State - displays the game's main menu with logo and options
     * 
     * Features the Floppy Turd logo with interactive F button, and menu options
     * for Playing, Options, Quick Play, and Quit. Supports both desktop and mobile layouts.
     */
    class MainMenuState : public GameState {
    public:
        MainMenuState(Gnosis::ECS* ecsCoordinator);
        ~MainMenuState() override;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "MainMenu"; }

    private:
        enum class MenuOption {
            PLAYING = 0,
            OPTIONS = 1,
            QUICK_PLAY = 2,
            QUIT = 3,
            COUNT = 4
        };

        Gnosis::ECS* m_ecsCoordinator;  // Reference to shared ECS coordinator
        bool m_finished;
        int m_selectedOption;
        float m_animationTimer;
        bool m_isMobile;
        
        // Screen dimensions for consistent layout calculations
        float m_screenWidth;
        float m_screenHeight;
        
        // UI Entity storage
        Gnosis::Entity m_backgroundEntity;
        Gnosis::Entity m_logoEntity;
        Gnosis::Entity m_fButtonEntity;
        
        // Menu button entities
        Gnosis::Entity m_playButtonEntity;
        Gnosis::Entity m_optionsButtonEntity;
        Gnosis::Entity m_quickPlayButtonEntity;
        Gnosis::Entity m_quitButtonEntity;
        
        // Font loading state
        bool m_fontLoaded;
        
        // Asset loading state
        bool m_assetsLoaded;

        // Layout functions - separate for desktop and mobile
        void CreateDesktopLayout();
        void CreateMobileLayout();
        
        // Button creation functions
        void CreateMenuButtons();
        void CreateMobileMenuButtons();
        
        // Rendering functions
        void RenderButtonText();
        void RenderButtonTextForEntity(Gnosis::Entity entity, const PlatformDelegates& delegates);
        
        // Update functions
        void UpdateMenuSelection();
        void UpdateMenuAnimations(float deltaTime);
        
        // Input handling
        void OnMenuOptionSelected(MenuOption option);
        void OnFButtonPressed();
        
        // Button click handlers
        void OnPlayButtonPressed();
        void OnOptionsButtonPressed();
        void OnQuickPlayButtonPressed();
        void OnQuitButtonPressed();
        
        // Input checking
        void CheckMenuButtonClicks(float touchX, float touchY);
        
        // Utility functions
        const char* GetMenuOptionText(int optionIndex) const;
        int GetMenuOptionCount() const;
        bool IsMobilePlatform() const;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_MAIN_MENU_STATE_H
