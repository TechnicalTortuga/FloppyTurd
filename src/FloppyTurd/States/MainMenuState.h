#ifndef FLOPPY_TURD_MAIN_MENU_STATE_H
#define FLOPPY_TURD_MAIN_MENU_STATE_H

#include "GameState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"

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
        
        // Level selection
        int GetSelectedLevelIndex() const { return m_selectedLevelIndex; }

    private:
        enum class MenuOption {
            PLAYING = 0,
            OPTIONS = 1,
            QUICK_PLAY = 2,
            QUIT = 3,
            COUNT = 4
        };

        enum class MenuMode {
            MAIN_MENU,
            LEVEL_SELECT
        };

        struct LevelInfo {
            std::string name;
            std::string paintingTexture;
            std::string lockedTexture;
            bool isUnlocked;
            int levelNumber;
        };

        enum class SwipeDirection {
            NONE,
            LEFT,
            RIGHT
        };

        Gnosis::ECS* m_ecsCoordinator;  // Reference to shared ECS coordinator
        bool m_finished;
        int m_selectedOption;
        float m_animationTimer;
        bool m_isMobile;
        MenuMode m_currentMode;
        
        // Screen dimensions for consistent layout calculations
        float m_screenWidth;
        float m_screenHeight;
        
        // UI Entity storage
        Gnosis::Entity m_backgroundEntity;
        Gnosis::Entity m_logoEntity;
        Gnosis::Entity m_fButtonEntity;
        
        // Menu button entities (now with integrated text)
        Gnosis::Entity m_playButtonEntity;
        Gnosis::Entity m_optionsButtonEntity;
        Gnosis::Entity m_quickPlayButtonEntity;
        Gnosis::Entity m_quitButtonEntity;
        
        // Level select entities
        std::vector<LevelInfo> m_levels;
        int m_currentLevelIndex;
        int m_selectedLevelIndex; // Level selected for gameplay
        Gnosis::Entity m_backButtonEntity;
        Gnosis::Entity m_leftArrowButtonEntity;
        Gnosis::Entity m_rightArrowButtonEntity;
        std::vector<Gnosis::Entity> m_levelPaintingEntities;
        std::vector<Gnosis::Entity> m_levelFrameEntities;
        std::vector<Gnosis::Entity> m_levelTextEntities;
        Gnosis::Entity m_lockedIndicatorEntity; // Single [Locked!] indicator that moves around
        Gnosis::Entity m_levelPlayButtonEntity;
        
        // Swipe mechanics for level select
        float m_swipeStartX;
        float m_swipeStartY;
        float m_swipeEndX;
        float m_swipeEndY;
        bool m_isSwiping;
        float m_swipeThreshold;
        float m_swipeAnimationTimer;
        float m_swipeAnimationDuration;
        SwipeDirection m_swipeDirection;
        float m_targetOffsetX;
        float m_currentOffsetX;
        
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
        // UI element creation
        void CreateUIElements();
        
        // Update functions
        void UpdateMenuSelection();
        void UpdateMenuAnimations(float deltaTime);
        
        // Input handling
        void OnMenuOptionSelected(MenuOption option);
        void OnFButtonPressed();
        void HandleMainMenuInput(const PlatformDelegates& delegates);
        
        // Button click handlers
        void OnPlayButtonPressed();
        void OnOptionsButtonPressed();
        void OnQuickPlayButtonPressed();
        void OnQuitButtonPressed();
        
        // Input checking
        void CheckMenuButtonClicks(float touchX, float touchY);
        
        // Button state management
        void ResetAllButtonStates();
        void UpdateButtonSprite(Gnosis::Entity entity, const GameCore::UIElement& uiElement);
        
        // Level select functions
        void InitializeLevels();
        void CreateLevelSelectLayout();
        void CreateLevelPaintings();
        void CreateBackButton();
        void CreateLevelPlayButton();
        void CreateLockedIndicator();
        void CreateArrowButtons();
        void ShowLevelSelect();
        void HideLevelSelect();
        void HandleLevelSelectInput();
        void HandleSwipeInput();
        void OnBackButtonPressed();
        void OnLevelPlayButtonPressed();
        void OnLeftArrowPressed();
        void OnRightArrowPressed();
        void OnLevelSelected(int levelIndex);
        void StartSwipe(float startX, float startY);
        void UpdateSwipe(float currentX, float currentY);
        void EndSwipe(float endX, float endY);
        void ProcessSwipe();
        void AnimateSwipe(float deltaTime);
        void UpdateLevelVisibility();
        void CenterCurrentLevel();
        
        // Debug functions
        void DrawButtonDebugRectangles();
        void DrawLevelSelectDebugInfo();
        
        // Utility functions
        const char* GetMenuOptionText(int optionIndex) const;
        int GetMenuOptionCount() const;
        bool IsMobilePlatform() const;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_MAIN_MENU_STATE_H
