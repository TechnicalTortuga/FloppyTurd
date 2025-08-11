#ifndef FLOPPY_TURD_MAIN_MENU_STATE_H
#define FLOPPY_TURD_MAIN_MENU_STATE_H

#include "GameState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include "../Systems/SpriteSystem.h"
#include "../Systems/RenderSystem.h"
#include <memory>

namespace GameCore {

    /**
     * @brief Main Menu State - displays the game's main menu with logo and options
     * 
     * Features the Floppy Turd logo with interactive F button, and menu options
     * for Playing, Options, Quick Play, and Quit. Supports both desktop and mobile layouts.
     */
    class MainMenuState : public GameState {
    public:
        MainMenuState(Gnosis::ECS* ecsCoordinator, GameCore::PlatformDelegates* platformDelegates);
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
            LEVEL_SELECT,
            OPTIONS
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
        GameCore::PlatformDelegates* m_platformDelegates;  // For system creation
        std::unique_ptr<SpriteSystem> m_spriteSystem;  // For texture dimension queries (rendering disabled)
        std::unique_ptr<RenderSystem> m_renderSystem;  // For screen info access
        bool m_finished;
        int m_selectedOption;
        float m_animationTimer;
        bool m_isMobile;
        MenuMode m_currentMode;
        
        // Screen dimensions for consistent layout calculations
        float m_screenWidth;
        float m_screenHeight;
        // Global UI scale (mobile around 8x)
        float m_uiScale = 1.0f;
        
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
        // Options menu specific UI
        Gnosis::Entity m_optionsLeftArrowEntity = 0;
        Gnosis::Entity m_optionsRightArrowEntity = 0;
        Gnosis::Entity m_masterKnobEntity = 0;
        Gnosis::Entity m_musicKnobEntity = 0;
        Gnosis::Entity m_sfxKnobEntity = 0;
        Gnosis::Entity m_optionsBackButtonEntity = 0;
        Gnosis::Entity m_masterTrackEntity = 0;
        Gnosis::Entity m_musicTrackEntity = 0;
        Gnosis::Entity m_sfxTrackEntity = 0;
        Gnosis::Entity m_optionsTitleEntity = 0;
        Gnosis::Entity m_difficultyTextEntity = 0;
        bool m_draggingMaster = false;
        bool m_draggingMusic = false;
        bool m_draggingSFX = false;
        // Cached slider layout
        float m_optionsSliderX = 0.0f;
        float m_optionsSliderY = 0.0f;
        float m_optionsSliderW = 0.0f;
        float m_optionsSliderH = 18.0f;
        float m_optionsSliderSpacing = 70.0f;
        // Cached options overlay rect
        float m_optionsOverlayX = 0.0f;
        float m_optionsOverlayY = 0.0f;
        float m_optionsOverlayW = 0.0f;
        float m_optionsOverlayH = 0.0f;
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
        
        // Button debouncing for level select arrows
        float m_lastArrowPressTime;
        float m_arrowDebounceDelay;
        
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
        void HandleMainMenuInput();
        void HandleOptionsInput();
        
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
        // Options UI helpers
        void CreateOptionsArrows(float overlayX, float overlayY, float overlayW, float overlayH, float diffY);
        void DestroyOptionsUI();
        void CreateOptionsKnobs();
        void UpdateOptionsKnobPositions();
        void SetMainMenuVisible(bool visible);
        void SetOptionsVisible(bool visible);
        void CreateOptionsTracksAndLabels();
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
        void RenderOptionsMenu();
        
        // Utility functions
        const char* GetMenuOptionText(int optionIndex) const;
        int GetMenuOptionCount() const;
        bool IsMobilePlatform() const;
        
        // Platform-specific layout functions
        void SetupLayout();
        void SetupIOSLayout();
        void SetupDesktopLayout();

        // Options menu helpers
        void ShowOptionsMenu();
        void HideOptionsMenu();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_MAIN_MENU_STATE_H
