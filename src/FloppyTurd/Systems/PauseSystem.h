#pragma once

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include "../Systems/SkillSystem.h"
#include "../Systems/HatsSystem.h"
#include "../Input/InputManager.h"
#include <vector>
#include <string>

namespace GameCore {

    // Forward declaration to avoid circular dependency
    class GameplayState;

    /**
     * Pause menu tab enumeration
     */
    enum class PauseMenuTab {
        SKILLS = 0,
        HATS = 1,
        STATS = 2,
        SYSTEM = 3
    };

    /**
     * PauseSystem - Manages the entire pause menu system
     * Handles creation, display, input processing, and cleanup of pause menu
     */
    class PauseSystem {
    public:
        PauseSystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates, GameplayState* gameplayState);
        ~PauseSystem();

        // Core lifecycle
        void Initialize();
        void Update(float deltaTime);
        void HandleInput(float touchX, float touchY, TouchState touchState = TouchState::PRESSED);
        void Cleanup();

        // Menu state management
        void Show();
        void Hide();
        bool IsVisible() const { return m_isVisible; }

        // Tab management
        void SwitchToTab(PauseMenuTab tab);
        PauseMenuTab GetCurrentTab() const { return m_currentTab; }

        // Screen dimension updates (called when screen size changes)
        void UpdateScreenDimensions(float width, float height);

        // Orientation helpers
        bool IsLandscapeMode() const;

        // Data update methods (called by GameplayState)
        void UpdateSkillData(int currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills,
                               SkillSystem* skillSystem, int /*unused*/, Entity playerEntity);
        void UpdateStatsData(int sessionPipes, int sessionCoins, int totalCoins, int grossTotalCoins, int totalFlops,
                           int enemiesKilled, int totalPipes);


        // Dragging state methods (called by GameplayState)
        bool IsDragging() const { return m_draggingMaster || m_draggingMusic || m_draggingSFX; }
        void StopDragging();

        // Public interface methods (called by GameplayState)
        void ShowPauseMenu();
        void HidePauseMenu();
        void HandlePauseMenuInput(float touchX, float touchY);
        void HandleKnobDrag(float touchX, float touchY, TouchState touchState);
        void StopKnobDrag();
        bool IsKnobDragging() const;
        void HandlePauseMenuContentClick(float touchX, float touchY);

    private:
        // Helper function for consistent button collision detection
        bool IsTouchInButtonBounds(float touchX, float touchY, Entity buttonEntity, bool isCentered = false);

        // Core system dependencies
        Gnosis::ECS* m_ecsCoordinator;
        const GameCore::PlatformDelegates& m_platformDelegates;
        GameplayState* m_gameplayState;

        // Helper methods

        // Menu state
        bool m_isVisible;
        bool m_isCreated;
        PauseMenuTab m_currentTab;

        // Screen dimensions (cached from GameplayState)
        float m_screenWidth;
        float m_screenHeight;

        // Menu entities
        Entity m_pauseMenuEntity;
        Entity m_pauseMenuBackgroundEntity;
        Entity m_pauseMenuRibbonEntity;
        std::vector<Gnosis::Entity> m_ribbonButtons;
        Entity m_pauseMenuContentEntity;

        // Tab content entities
        Entity m_systemTitleEntity;
        Entity m_skillsTitleEntity;
        Entity m_hatsTitleEntity;
        Entity m_statsTitleEntity;
        Entity m_skillsContentEntity;
        Entity m_hatsContentEntity;

        // System tab entities
        Entity m_mainMenuButtonEntity;
        Entity m_masterKnobEntity;
        Entity m_masterTrackEntity;
        Entity m_masterLabelEntity;
        Entity m_musicKnobEntity;
        Entity m_sfxKnobEntity;
        Entity m_musicTrackEntity;
        Entity m_sfxTrackEntity;
        Entity m_musicLabelEntity;
        Entity m_sfxLabelEntity;

        // Skills tab entities
        Entity m_skillsBackgroundEntity;
        Entity m_skillsNameEntity;
        Entity m_skillsDescriptionEntity;
        Entity m_skillsCostEntity;
        Entity m_skillsUnlockButtonEntity;
        Entity m_skillsLeftArrowEntity;
        Entity m_skillsRightArrowEntity;

        // Hats tab entities
        Entity m_hatsBackgroundEntity;
        Entity m_hatsActionButtonEntity;
        Entity m_hatsCostDisplayEntity;
        std::vector<Entity> m_hatFrameEntities;
        std::vector<Entity> m_hatIconEntities;
        std::vector<Entity> m_lockedFrameEntities;

        // Stats tab entities
        Entity m_statsBackgroundEntity;
        Entity m_currentSessionTextEntity;
        Entity m_sessionCoinsTextEntity;
        Entity m_totalCoinsTextEntity;
        Entity m_totalFlopsTextEntity;
        Entity m_grossTotalCoinsTextEntity;
        Entity m_enemiesKilledTextEntity;
        Entity m_totalPipesTextEntity;

        // Audio slider state
        bool m_draggingMaster;
        bool m_draggingMusic;
        bool m_draggingSFX;
        int m_activeDragKnob;
        float m_dragStartX;
        float m_dragKnobStartX;
        float m_sliderX;
        float m_sliderY;
        float m_sliderW;
        float m_sliderH;
        float m_sliderSpacing;
        Entity m_draggedKnobEntity;
        float m_dragOffsetX;

        // Button debouncing state
        float m_lastSkillButtonPressTime;
        float m_skillButtonDebounceDelay;
        float m_lastActionButtonPressTime;
        float m_actionButtonDebounceDelay;
        float m_masterSliderValue;
        float m_musicSliderValue;
        float m_sfxSliderValue;


        // Debouncing for pause menu interactions
        float m_lastRibbonButtonPressTime;
        float m_ribbonButtonDebounceDelay;
        float m_lastSettingsButtonPressTime;
        float m_settingsButtonDebounceDelay;

        // Skills menu state
        int m_currentSkillIndex;
        std::vector<GameCore::SkillType> m_availableSkills;
        SkillSystem* m_skillSystem;
        HatsSystem* m_hatsSystem;
        int m_playerCoins;
        Entity m_playerEntity;

        // Stats data
        int m_sessionPipes;
        int m_sessionCoins;
        int m_totalCoins;
        int m_grossTotalCoins;
        int m_totalFlops;
        int m_enemiesKilled;
        int m_totalPipes;

        // Initialization methods
        void CreatePauseMenu();
        void CreatePauseMenuBackground();
        void CreatePauseMenuRibbon();
        void CreateRibbonButtons();
        void CreatePauseMenuContent();

        // Tab creation methods
        void CreateSystemTab();
        void CreateSkillsTab();
        void CreateHatsTab();
        void CreateHatsGridUI(float centerX, float centerY, float gridWidth, float gridHeight);
        void CreateHatActionButton();
        void CreateHatCostDisplay();
        void UpdateHatFrameVisuals();
        void UpdateHatDisplay();
        void SyncPlayerCoins();
        void CreateStatsTab();
        void CreateAudioSliders();

        // Tab management methods
        void ShowTabContent(PauseMenuTab tab);
        void HideAllTabContent();
        void ShowSystemTab();
        void ShowSkillsTab();
        void ShowHatsTab();
        void ShowStatsTab();

        // Input handling methods
        bool HandlePauseMenuRibbonClick(float touchX, float touchY);
        void HandleSystemTabClick(float touchX, float touchY);
        void HandleSkillsTabClick(float touchX, float touchY);
        void HandleHatsTabClick(float touchX, float touchY);
        void HandleStatsTabClick(float touchX, float touchY);

        // Skills management methods
        void UpdateSkillDisplay(int currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills,
                               SkillSystem* skillSystem, int playerCoins, Gnosis::Entity playerEntity);
        void UpdateSkillDisplayFromCurrentState();
        void HandleSkillLeftArrow(int& currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills);
        void HandleSkillRightArrow(int& currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills);
        void HandleSkillUnlock(int currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills,
                              SkillSystem* skillSystem, int& playerCoins, Gnosis::Entity playerEntity);

        // Helper methods
        void UpdateGameStatsFromSession(int sessionPipes, int sessionCoins, int totalCoins, int grossTotalCoins, int totalFlops,
                                       int enemiesKilled, int totalPipes);
        void RefreshStatsDisplay(int sessionPipes, int sessionCoins, int totalCoins, int grossTotalCoins, int totalFlops,
                           int enemiesKilled, int totalPipes);
        void IncrementDeathCounter();
        bool IsTapOutsideMenuArea(float touchX, float touchY) const;
        bool IsTapInSettingsButtonArea(float touchX, float touchY) const;

        // Audio slider constants
        static constexpr float SLIDER_X = 150.0f;
        static constexpr float SLIDER_Y = 200.0f;
        static constexpr float SLIDER_W = 200.0f;
        static constexpr float SLIDER_H = 18.0f;
        static constexpr float SLIDER_SPACING = 70.0f;
        static constexpr float UI_SCALE = 1.0f;
    };

} // namespace GameCore
