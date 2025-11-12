#ifndef FLOPPY_TURD_MAIN_MENU_STATE_H
#define FLOPPY_TURD_MAIN_MENU_STATE_H

#include "GameState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
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
        MainMenuState(Gnosis::ECS* ecsCoordinator, PlatformDelegates* platformDelegates);
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
        
        // Public menu mode enum for external access
        enum class MenuMode {
            MAIN_MENU,
            LEVEL_SELECT,
            OPTIONS,
            AD_CONTROLS
        };
        
        // Level selection
        int GetSelectedLevelIndex() const { return m_selectedLevelIndex; }
        bool GetEnteredViaQuickplay() const { return m_enteredViaQuickplay; }
        void RefreshLevelDisplay();
        void UpdateUnlockButtonVisibility(size_t levelIndex, bool isUnlocked);
        void OnUnlockButtonPressed(int levelNumber);
        
        // Menu mode control for returning from gameplay
        void SetStartingMenuMode(MenuMode mode) { m_currentMode = mode; }
        void SetReturnToLevel(int levelNumber) { 
            m_returnToLevelNumber = levelNumber; 
            m_shouldReturnToSpecificLevel = true;
        }
        void SetEnteredViaQuickplay(bool quickplay) { m_enteredViaQuickplay = quickplay; }
        
        // Leaderboard transition
        bool IsTransitioningToLeaderboard() const { return m_transitioningToLeaderboard; }

    private:
        enum class MenuOption {
            PLAYING = 0,
            OPTIONS = 1,
            QUICK_PLAY = 2,
            LEADERBOARD = 3,
            HOW_TO = 4,
            COUNT = 5
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
        PlatformDelegates* m_platformDelegates;  // For system creation
        GameCore::RenderSystem* m_renderSystem;  // Cached render system reference
        // Cache game instance to avoid repeated extern lookups
        FloppyTurdGame* m_game = nullptr;
        bool m_finished;
        int m_selectedOption;
        float m_animationTimer;
        bool m_isMobile;
        bool m_uiInitialized = false;
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
        Gnosis::Entity m_leaderboardButtonEntity;
        Gnosis::Entity m_howToButtonEntity;
        
        // Main menu UI - ad controls and version
        Gnosis::Entity m_adControlsButtonEntity;
        Gnosis::Entity m_versionTextEntity;
        
        // Level select entities
        std::vector<LevelInfo> m_levels;
        int m_currentLevelIndex;
        int m_selectedLevelIndex; // Level selected for gameplay
        int m_returnToLevelNumber = -1; // Level to return to after gameplay
        bool m_shouldReturnToSpecificLevel = false; // Whether to return to a specific level
        bool m_enteredViaQuickplay = false; // Whether entered via Quickplay button
        bool m_transitioningToLeaderboard = false; // Whether transitioning to leaderboard state
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
        Gnosis::Entity m_optionsOverlayEntity = 0;
        Gnosis::Entity m_masterTrackEntity = 0;
        Gnosis::Entity m_musicTrackEntity = 0;
        Gnosis::Entity m_sfxTrackEntity = 0;
        // UI shape entities for slider tracks
        Gnosis::Entity m_masterTrackShape = 0;
        Gnosis::Entity m_musicTrackShape = 0;
        Gnosis::Entity m_sfxTrackShape = 0;
        // Slider text labels on their own row
        Gnosis::Entity m_masterLabelEntity = 0;
        Gnosis::Entity m_musicLabelEntity = 0;
        Gnosis::Entity m_sfxLabelEntity = 0;
        Gnosis::Entity m_optionsTitleEntity = 0;
        Gnosis::Entity m_difficultyTextEntity = 0;
        Gnosis::Entity m_difficultyValueEntity = 0; // centered value between arrows
        
        // Vibration toggle entities
        Gnosis::Entity m_vibrationLabelEntity = 0;
        Gnosis::Entity m_vibrationToggleEntity = 0;
        bool m_vibrationsEnabled = true; // Default ON
        bool m_draggingMaster = false;
        bool m_draggingMusic = false;
        bool m_draggingSFX = false;
        // Single-knob drag state tracking
        int m_activeDragKnob = -1;           // -1=none, 0=master, 1=music, 2=sfx
        float m_dragStartX = 0.0f;           // Touch X when drag started
        float m_dragKnobStartX = 0.0f;       // Knob X position when drag started
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
        std::vector<Gnosis::Entity> m_unlockButtonEntities;
        std::vector<Gnosis::Entity> m_requirementTextEntities;
        Gnosis::Entity m_lockedIndicatorEntity; // Single [Locked!] indicator that moves around
        Gnosis::Entity m_levelPlayButtonEntity;
        
        // Ad Controls menu entities
        Gnosis::Entity m_adControlsOverlayEntity = 0;
        Gnosis::Entity m_adControlsTitleEntity = 0;
        Gnosis::Entity m_adControlsBackButtonEntity = 0;
        Gnosis::Entity m_removeAdsLabelEntity = 0;
        Gnosis::Entity m_removeAdsPriceButtonEntity = 0;
        
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
        // Standard UI font size to match main menu buttons (except the big OPTIONS title)
        // Base button font size (will be scaled by m_uiScale where rendered)
        float m_buttonFontSize = 20.0f;
        // Main menu button scale for sprites (text size is controlled separately)
        float m_menuButtonScale = 10.0f;
        // Global UI text size across menus (mobile target now 80)
        float m_globalUIFontSize = 80.0f;
        // Horizontal pan state for level select
        bool m_isPanning = false;
        float m_panStartX = 0.0f;
        float m_panStartY = 0.0f;
        float m_panStartOffsetX = 0.0f;
        float m_lastPanX = 0.0f;
        float m_levelSpacing = 0.0f;
        bool m_isSnapping = false;
        
        // Double-tap detection for painting clicks
        float m_lastPaintingTapTime = 0.0f;
        int m_lastTappedPaintingIndex = -1;
        static constexpr float DOUBLE_TAP_THRESHOLD = 0.5f; // 500ms between taps (max)
        static constexpr float MIN_TAP_INTERVAL = 0.05f; // 50ms minimum between taps (prevents single tap counting as double)
        float m_snapElapsed = 0.0f;
        float m_snapDuration = 0.18f;
        float m_snapStartOffsetX = 0.0f;
        int m_pendingIndexDelta = 0;
        
        // Button debouncing for level select arrows
        float m_lastArrowPressTime;
        float m_arrowDebounceDelay;

        // Button debouncing for unlock buttons
        float m_lastUnlockPressTime;
        float m_unlockDebounceDelay;
        
        // Input debounce when entering from gameplay to prevent accidental clicks
        float m_inputDebounceTimer;
        static constexpr float INPUT_DEBOUNCE_DURATION = 0.3f; // 300ms debounce
        
        // Fart button debounce to prevent rapid-fire farts
        float m_fartButtonDebounceTimer;
        static constexpr float FART_BUTTON_DEBOUNCE = 0.5f; // 500ms between farts
        
        // Menu button debounce to prevent clicking into another menu when switching pages
        float m_lastMenuButtonPressTime;
        static constexpr float MENU_BUTTON_DEBOUNCE = 0.3f; // 300ms between menu transitions
        
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
        void OnLeaderboardButtonPressed();
        void OnHowToButtonPressed();
        void OnAdControlsButtonPressed();
        
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
        
        // Ad Controls menu functions
        void ShowAdControlsMenu();
        void HideAdControlsMenu();
        void CreateAdControlsLayout();
        void HandleAdControlsInput();
        void OnAdControlsBackButtonPressed();
        void OnRemoveAdsPurchasePressed();
        
        // Vibration preference helper functions
        void SaveVibrationPreference(bool enabled);
        bool LoadVibrationPreference();
        void StartSwipe(float startX, float startY);
        void UpdateSwipe(float currentX, float currentY);
        void EndSwipe(float endX, float endY);
        void ProcessSwipe();
        void AnimateSwipe(float deltaTime);
        void UpdateLevelVisibility();
        void CenterCurrentLevel();
        void UpdateLevelPanPositions();
        void UpdatePanSnapAnimation(float deltaTime);
        
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

        // Orientation change handling
        void CheckForOrientationChange();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_MAIN_MENU_STATE_H
