#ifndef FLOPPY_TURD_GAMEPLAY_STATE_H
#define FLOPPY_TURD_GAMEPLAY_STATE_H

#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "GameState.h"
#include "../Entities/Player.h"
#include "../Systems/SpriteSystem.h"
#include "../Systems/PlayerControllerSystem.h"
#include "../Systems/CameraSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/LevelManager.h"
#include "../Systems/PickupSystem.h"
#include "../Systems/EnemySystem.h"
#include "../Systems/UISystem.h"
#include "../Systems/HeartSystem.h"
#include "../Systems/ProjectileSystem.h"
#include "../Systems/HatsSystem.h"
#include "../Systems/SkillSystem.h"
#include "../Systems/BossSystem.h"
#include "../Systems/BossHealthBar.h"
#include "../Config/LevelConfig.h"
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

namespace GameCore {

    // System pointers (will be implemented in Phase 2)
    // For now, using raw pointers since systems don't exist yet

    /**
     * @brief Gameplay State - Main game loop for FloppyTurd
     * 
     * This state manages the core gameplay loop including:
     * - Player movement and controls
     * - Obstacle spawning and collision detection
     * - Scoring and progression
     * - Audio and visual feedback
     * - Pause/resume functionality
     */
    class GameplayState : public GameState {
    public:
        GameplayState(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, int levelId = 0);
        ~GameplayState() override;

        // State lifecycle
        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        // State updates
        void Update(float deltaTime) override;
        void Render() override;
        void HandleSettingsButtonInput();
    void HandleInput() override;

        // State queries
        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Gameplay"; }

        // Game-specific methods
        void SetLevel(int levelId);
        void RestartLevel();
        void GameOver();
        void LevelComplete();
        
        // Pickup and coin coordination handled by PickupSystem
        
        // Score and progression
        int GetCurrentScore() const { return m_currentScore; }
        int GetCurrentLives() const { return m_currentLives; }
        
        // Pause menu
        void TogglePause();
        bool IsPaused() const { return m_currentSubState == GameplaySubState::Paused; }
        
        // Platform-specific layout functions
        void SetupLayout();
        void SetupIOSLayout();
        void SetupDesktopLayout();

    private:
        // Core systems
        Gnosis::ECS* m_ecsSystem;
        GameCore::PlatformDelegates* m_platformDelegates;
        
        // Game systems
        std::unique_ptr<SpriteSystem> m_spriteSystem;
        std::unique_ptr<PlayerControllerSystem> m_playerControllerSystem;
        std::unique_ptr<CameraSystem> m_cameraSystem;
        RenderSystem* m_renderSystem;  // 🎯 NEW: Borrowed from SystemManager, not owned
        std::unique_ptr<LevelManager> m_levelManager;
        std::unique_ptr<UISystem> m_uiSystem;
        std::unique_ptr<PickupSystem> m_pickupSystem;
        std::unique_ptr<EnemySystem> m_enemySystem;
        std::unique_ptr<HeartSystem> m_heartSystem;
        std::unique_ptr<ProjectileSystem> m_projectileSystem;
        std::unique_ptr<HatsSystem> m_hatsSystem;
        std::unique_ptr<SkillSystem> m_skillSystem;

        // Boss systems (level 6 only)
        std::unique_ptr<BossSystem> m_bossSystem;
        std::unique_ptr<BossHealthBar> m_bossHealthBar;

        // Level configuration
        int m_currentLevelId;
        LevelConfig m_currentLevelConfig;

        // Game entities
        Gnosis::Entity m_playerEntity;
        Gnosis::Entity m_cameraEntity;
        std::vector<Gnosis::Entity> m_backgroundEntities;
        std::vector<Gnosis::Entity> m_obstacles;
        // Moved to PickupSystem: m_pickups, m_pickupIndex
        // Projectiles now managed by ProjectileSystem: m_projectiles
        std::vector<Gnosis::Entity> m_enemies;
        
        // Moved to PickupSystem: m_groupCoins

        // Game state
        int m_currentScore;
        int m_currentLives;
        float m_gameTime;
        float m_difficultyTimer;
        float m_difficultyLevel;
        
        // Player state
        bool m_playerAlive;
        float m_invulnerabilityTimer;
        int m_pipesCleared;                     // Number of pipes passed through
        int m_sessionCoinsCollected;            // Coins collected in current session
        // Hurt state is now managed by PlayerControllerSystem
        
        // Game flow
        bool m_finished;
        bool m_levelCompleted;
        
        // Gameplay sub-states
        enum class GameplaySubState {
            Playing,    // Normal gameplay
            Paused,     // Game is paused
            GameOver    // Game over sequence active
        };
        GameplaySubState m_currentSubState;
        float m_gameOverTimer;          // Timer for game over sequence timing
        float m_morteFloatOffset;       // Floating animation for morte sprite
        
        // UI elements
        Gnosis::Entity m_scoreTextEntity;
        Gnosis::Entity m_livesTextEntity;
        Gnosis::Entity m_coinsTextEntity;       // Coin counter number (UI)
        Gnosis::Entity m_coinBagEntity;         // Coin bag icon (32x32)
        Gnosis::Entity m_pipeCounterEntity;     // Pipe counter display under notch
        Gnosis::Entity m_pauseMenuEntity;
        Gnosis::Entity m_tempMenuButtonEntity;  // Temporary button to return to main menu
        Gnosis::Entity m_heartUIEntity;         // Heart UI display entity
        
        // Pause menu system
        Gnosis::Entity m_settingsButtonEntity;  // Settings button (replaces [MENU] button)
        Gnosis::Entity m_pauseMenuBackgroundEntity; // Pause menu background overlay
        Gnosis::Entity m_pauseMenuRibbonEntity;     // Ribbon containing tab buttons
        bool m_hatsGridCreated;                       // Prevent duplicate hats grid creation
        std::vector<Gnosis::Entity> m_ribbonButtons; // SKILLS, HATS, STATS, SYSTEM buttons
        Gnosis::Entity m_pauseMenuContentEntity;     // Content area for current tab
        int m_currentPauseTab;                       // Current active tab (0=SKILLS, 1=HATS, 2=STATS, 3=SYSTEM)
        // Settings button debouncing
        float m_lastSettingsButtonPressTime;
        float m_settingsButtonDebounceDelay;

        // Action button debouncing
        float m_lastActionButtonPressTime;
        float m_actionButtonDebounceDelay;

        // Skills tab button debouncing
        float m_lastSkillButtonPressTime;
        float m_skillButtonDebounceDelay;

        // Pause menu creation state
        bool m_pauseMenuCreated;

        // Pause menu tab content management
        void ShowPauseMenuTab(int tabIndex);
        void DestroyPauseMenuTabContent();
        void CreateSystemTabContent();
        void CreateSkillsTabContent();
        void CreateHatsTabContent();
        void CreateStatsTabContent();

        // Hats tab interaction
        void HandleHatsTabClick(float touchX, float touchY);
        void HandleHatsButtonClicks(float touchX, float touchY, float centerX, float buttonY);
        void HandleHatPurchase();
        void HandleHatEquip();
        void ScheduleDelayedSound(float delaySeconds);

        // Coin management helpers
        int GetCurrentPlayerCoins() const;
        void DeductPlayerCoins(int amount);

        // Pause menu tab content entities
        Gnosis::Entity m_systemTabEntity;       // System tab content (audio + main menu)
        Gnosis::Entity m_mainMenuButtonEntity;  // Main menu button in system tab

        Gnosis::Entity m_placeholderLabelEntity; // Placeholder label for non-system tabs
        
        // Individual tab content entities
        Gnosis::Entity m_skillsContentEntity;   // Skills tab content
        Gnosis::Entity m_skillsBackgroundEntity; // Skills tab black background
        Gnosis::Entity m_skillsTitleEntity;       // Skills tab title text
        Gnosis::Entity m_hatsTitleEntity;         // Hats tab title text
        Gnosis::Entity m_statsTitleEntity;        // Stats tab title text
        Gnosis::Entity m_systemTitleEntity;       // System tab title text
        Gnosis::Entity m_skillsNameEntity;       // Skills tab name text
        Gnosis::Entity m_skillsDescriptionEntity; // Skills tab description text
        Gnosis::Entity m_skillsCostEntity;        // Skills tab cost text
        Gnosis::Entity m_skillsUnlockButtonEntity; // Skills tab unlock button
        Gnosis::Entity m_skillsLeftArrowEntity;   // Skills tab left arrow
        Gnosis::Entity m_skillsRightArrowEntity;  // Skills tab right arrow

        // Skill menu state
        int m_currentSkillIndex;                   // Current skill being displayed (0-4)
        std::vector<GameCore::SkillType> m_availableSkills; // List of available skills

        // Skill menu functions
        void UpdateSkillDisplay();
        void HandleSkillLeftArrow();
        void HandleSkillRightArrow();
        void HandleSkillUnlock();
        void HandleSkillsTabClick(float touchX, float touchY);
        Gnosis::Entity m_hatsContentEntity;     // Hats tab content
        Gnosis::Entity m_hatsBackgroundEntity;   // Hats tab black background
        Gnosis::Entity m_statsContentEntity;    // Stats tab content
        Gnosis::Entity m_statsBackgroundEntity; // Stats tab black background rectangle
        
        // Individual stats display entities
        Gnosis::Entity m_totalPipesTextEntity;     // Total pipes cleared across all games
        Gnosis::Entity m_totalFlopsTextEntity;     // Total deaths/flops
        Gnosis::Entity m_totalCoinsTextEntity;     // Total coins collected
        Gnosis::Entity m_sessionCoinsTextEntity;   // Session coins collected

        Gnosis::Entity m_enemiesKilledTextEntity;  // Total enemies killed
        Gnosis::Entity m_currentSessionTextEntity; // Current session pipes

        // Level high score display entities
        std::vector<Gnosis::Entity> m_levelHighScoreEntities;

        // Audio slider UI entities (matching MainMenuState style)
        Gnosis::Entity m_masterKnobEntity = 0;
        Gnosis::Entity m_masterTrackEntity = 0;
        Gnosis::Entity m_masterLabelEntity = 0;
        Gnosis::Entity m_musicKnobEntity = 0;
        Gnosis::Entity m_sfxKnobEntity = 0;
        Gnosis::Entity m_musicTrackEntity = 0;
        Gnosis::Entity m_sfxTrackEntity = 0;
        Gnosis::Entity m_musicLabelEntity = 0;
        Gnosis::Entity m_sfxLabelEntity = 0;

        // Slider drag state
        bool m_draggingMaster = false;
        bool m_draggingMusic = false;
        bool m_draggingSFX = false;
        int m_activeDragKnob = -1; // -1=none, 0=master, 1=music, 2=sfx
        float m_dragStartX = 0.0f;
        float m_dragKnobStartX = 0.0f;

        // Cached slider layout
        float m_sliderX = 0.0f;
        float m_sliderY = 0.0f;
        float m_sliderW = 0.0f;
        float m_sliderH = 18.0f;
        float m_sliderSpacing = 70.0f;
        float m_uiScale = 1.0f;  // UI scaling factor for consistent sizing

        // Current slider values (0.0-1.0)
        float m_masterSliderValue = 1.0f;
        float m_musicSliderValue = 1.0f;
        float m_sfxSliderValue = 1.0f;

        // Delayed sound system for ooo sounds
        float m_delayedSoundTime = 0.0f;
        std::string m_delayedSoundName;

        // Debug hitbox visualization entities
        std::vector<Gnosis::Entity> m_debugHitboxEntities;
        
        // Game over UI elements
        Gnosis::Entity m_gameOverBackgroundEntity;   // Light from heaven background
        Gnosis::Entity m_morteEntity;                 // FloppyTurdMorte floating sprite
        Gnosis::Entity m_gameOverScoreEntity;        // Score display
        Gnosis::Entity m_deathMessageEntity;         // Funny death message text
        Gnosis::Entity m_tryAgainButtonEntity;       // Try again button
        Gnosis::Entity m_quitButtonEntity;           // Quit to main menu button

        // Tracks whether we've already repositioned UI based on real pixel dimensions
        bool m_uiPositionsSynced = false;

        // Spawn timers
        float m_obstacleSpawnTimer;
        float m_pickupSpawnTimer;
        float m_enemySpawnTimer;
        
        // Input delay timer to prevent auto-shooting when entering level
        float m_inputDelayTimer;
        static constexpr float INPUT_DELAY_TIME = 0.5f; // 0.5 seconds delay
        
        // Configuration
        static constexpr float OBSTACLE_SPAWN_INTERVAL = 2.0f;
        static constexpr float PICKUP_SPAWN_INTERVAL = 5.0f;
        static constexpr float ENEMY_SPAWN_INTERVAL = 3.0f;
        static constexpr float DIFFICULTY_INCREASE_INTERVAL = 30.0f;
        static constexpr float MAX_DIFFICULTY_LEVEL = 10.0f;
        static constexpr int STARTING_LIVES = 3;

        // Private methods
        void InitializeSystems();
        void CreateGameEntities();
        void CreateBackgroundLayers();
        void DestroyGameEntities();

        // Pause menu tab switching (call when ribbon button pressed)
        void OnPauseMenuTabSelected(int tabIndex);
    void ResetPlayerEntity();
        void CreateUI();
        void DestroyUI();
        void UpdateGameLogic(float deltaTime);
        void UpdateSpawning(float deltaTime);
        void UpdateDifficulty(float deltaTime);
        void HandleGameEvents();
        // REMOVED: Legacy spawn methods - replaced with LevelManager pooling and GameplayState coordination
        void CleanupOffscreenEntities();
        void CheckLevelCompletion();
        void SaveGameProgress();
        
        // Collision and pipe tracking
        void CheckToiletCollisions();
        void UpdatePipeCounterUI();
        void UpdateCoinCounterUI();
        void OnPipeCleared();
        
        // Debug rendering
        void DrawDebugRectangles();
        
        // Audio methods
        void StartLevelMusic();
        void StopLevelMusic();
        
        // Event handlers
        void OnPlayerJump();
        void OnPlayerShoot();
        
        // Sub-state management methods
        void UpdateSubState(float deltaTime);
        void TriggerGameOver();
        void TriggerPause();
        void TriggerResume();
        
        // Game over methods
        void CreateGameOverUI();
        void DestroyGameOverUI();
        void HideRegularUI();
        void ShowRegularUI();
        bool HasPlayerFallenOffScreen();
        void UpdateMorteFloating(float deltaTime);
        void HandleGameOverInput();
        void TryAgain();
        void QuitToMainMenu();
        std::string GetRandomDeathMessage();
        void OnPlayerHurt(int damage);
        void OnPlayerDeath();
        void OnCoinCollected(int value);
        void OnHeartCollected(int healAmount);
        void OnPickupCollected();
        void OnObstacleHit();
        void OnEnemyDefeated();
        
        // Pause menu methods
        void CreatePauseMenu();
        void DestroyPauseMenu();
        void ShowPauseMenu();
        void HidePauseMenu();
        void CreateSettingsButton();
        void CreatePauseMenuBackground();
        void CreatePauseMenuRibbon();
        void CreatePauseMenuContent();
        void CreateRibbonButtons();
        void CreateSystemTab();
        void CreateSkillsTab();
        void CreateHatsTab();
        void CreateStatsTab();
        void CreateAudioSliders();
        void CreateDebugHitboxRectangles();
        void UpdateDebugHitboxPositions();
        void ShowDebugHitboxes(bool show);
        void SwitchPauseTab(int tabIndex);
        void ShowTabContent(int tabIndex);
        void ShowCurrentTabContent();
        void ShowSystemTab();
        void ShowSkillsTab();
        void ShowHatsTab();
        void ShowStatsTab();
        void HideAllTabContent();
        
        // Stats management
        void UpdateGameStatsFromSession();    // Update game stats with current session data
        void RefreshStatsDisplay();           // Refresh the stats text entities with current values
        void IncrementDeathCounter();         // Increment death counter when player dies
        void CheckSettingsButtonClick(float touchX, float touchY);
        void HandlePauseMenuInput(float touchX, float touchY);
        bool HandlePauseMenuRibbonClick(float touchX, float touchY);
        void HandlePauseMenuContentClick(float touchX, float touchY);
        void HandleSystemTabClick(float touchX, float touchY);
        void HandleKnobDrag(float touchX, float touchY);
        bool IsTapOutsideMenuArea(float touchX, float touchY);
        bool IsTapInSettingsButtonArea(float touchX, float touchY);
        
        // Menu navigation (for pause menu integration later)
        void ReturnToMainMenu();  // Function to connect to pause menu later
        void CheckMenuButtonClick(float touchX, float touchY);
    };

} // namespace GameCore

#endif // FLOPPY_TURD_GAMEPLAY_STATE_H 