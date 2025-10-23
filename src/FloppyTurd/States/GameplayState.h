#ifndef FLOPPY_TURD_GAMEPLAY_STATE_H
#define FLOPPY_TURD_GAMEPLAY_STATE_H

#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "GameState.h"
#include "../../Engine/Utility/FrameProfiler.h"
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
#include "../Systems/PauseSystem.h"
#include "../Systems/OverlaySystem.h"
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
        GameplayState(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates, int levelId = 0);
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
        void HandlePauseMenuInput();
        void HandleGameplayInput();
        void HandleInput() override;

        // Profiling controls
        void SetProfilingEnabled(bool enabled);
        bool IsProfilingEnabled() const { return m_profilingEnabled; }

        // State queries
        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Gameplay"; }

        // Game-specific methods
        void SetLevel(int levelId);
        void RestartLevel();
        void GameOver();
        void LevelComplete();
        
        // Level info
        int GetCurrentLevelId() const { return m_currentLevelId; }
        
        // Pickup and coin coordination handled by PickupSystem
        
        // Score and progression
        int GetCurrentScore() const { return m_currentScore; }
        int GetCurrentLives() const { return m_currentLives; }
        
        // Pause menu
        void TogglePause();
        bool IsPaused() const { return m_currentSubState == GameplaySubState::Paused; }
        void ReturnToMainMenu();
        
        // Platform-specific layout functions
        void SetupLayout();
        void SetupIOSLayout();
        void SetupDesktopLayout();

        // Orientation-specific UI management
        bool IsLandscapeMode() const;
        void UpdateUILayoutForOrientation();
        void RepositionSettingsButtonForPauseMenu(bool isPauseMenuActive);
        void RepositionUIElementsLandscape();
        void RepositionUIElementsPortrait();
        void RescaleBackgroundsForOrientation(bool isLandscape, float screenWidth, float screenHeight);
        void RegisterScreenInfoCallback();

        // Debug visualization
        void CreateDebugButtonRectangle();
        void UpdateDebugButtonRectangle(float left, float top, float right, float bottom);
        void CreateDebugShootingZoneRectangle();
        void UpdateDebugShootingZoneRectangle(float left, float top, float right, float bottom);

    private:
        // Core systems
        Gnosis::ECS* m_ecsSystem;
        PlatformDelegates* m_platformDelegates;

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

        // Pause system (handles all pause menu functionality)
        std::unique_ptr<PauseSystem> m_pauseSystem;
        
        // Overlay system (snowfall and other visual effects between world and UI)
        std::unique_ptr<OverlaySystem> m_overlaySystem;

        // Cached screen dimensions (eliminate 40+ repeated GetScreenInfo() calls)
        float m_cachedScreenWidth;
        float m_cachedScreenHeight;

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
        float m_pipeIncrementCooldown;          // Cooldown timer to prevent multiple increments from stacked pipes
        // Hurt state is now managed by PlayerControllerSystem
        
        // Game flow
        bool m_finished;
        bool m_levelCompleted;
        
        // Gameplay sub-states
        enum class GameplaySubState {
            Playing,    // Normal gameplay
            Paused,     // Pause menu active
            GameOver    // Game over sequence active
        };
        GameplaySubState m_currentSubState;
        float m_gameOverTimer;          // Timer for game over sequence timing
        float m_morteFloatOffset;       // Floating animation for morte sprite

        // Profiling
        Gnosis::FrameProfiler m_frameProfiler;
        bool m_profilingEnabled = false;

        // UI elements
        Gnosis::Entity m_scoreTextEntity;
        Gnosis::Entity m_livesTextEntity;
        Gnosis::Entity m_coinsTextEntity;       // Coin counter number (UI)
        Gnosis::Entity m_coinBagEntity;         // Coin bag icon (32x32)
        Gnosis::Entity m_shootingZoneEntity;    // Shooting zone visual indicator
        
        // Initial positions for coin bag and text (for reset consistency)
        float m_coinBagInitialX = 0.0f;
        float m_coinBagInitialY = 0.0f;
        float m_coinsTextInitialX = 0.0f;
        float m_coinsTextInitialY = 0.0f;
        Gnosis::Entity m_heartUIEntity;         // Heart UI display entity
        Gnosis::Entity m_pipeCounterEntity;     // Pipe counter UI element
        
        // Pause menu system
        Gnosis::Entity m_settingsButtonEntity;  // Settings button (replaces [MENU] button)

        // Debug visualization entities
        Gnosis::Entity m_debugButtonRect;       // Red rectangle showing button collision bounds
        Gnosis::Entity m_debugShootingZoneRect; // Blue rectangle showing shooting zone collision bounds
        // Settings button debouncing
        float m_lastSettingsButtonPressTime;
        float m_settingsButtonDebounceDelay;

        // Coin management helpers
        int GetCurrentPlayerCoins() const;
        void DeductPlayerCoins(int amount);

    public:
        // System access for PauseSystem
        SkillSystem* GetSkillSystem() { return m_skillSystem.get(); }
        HatsSystem* GetHatsSystem() { return m_hatsSystem.get(); }
        Gnosis::Entity GetPlayerEntity() const { return m_playerEntity; }

        float m_uiScale = 1.0f;  // UI scaling factor for consistent sizing

        // Debug hitbox visualization entities
        std::vector<Gnosis::Entity> m_debugHitboxEntities;
        
        // Game over UI elements
        Gnosis::Entity m_gameOverBackgroundEntity;   // Light from heaven background
        Gnosis::Entity m_morteEntity;                 // FloppyTurdMorte floating sprite
        Gnosis::Entity m_gameOverScoreEntity;        // Score display background
        Gnosis::Entity m_pipesLabelEntity;           // "Pipes: X" text label
        Gnosis::Entity m_coinsLabelEntity;           // "Coins: X" text label
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

        // Orientation-specific UI positioning constants (Portrait mode - current implementation)
        static constexpr float PORTRAIT_SETTINGS_X = 0.85f;    // 85% from left
        static constexpr float PORTRAIT_SETTINGS_Y = 0.05f;    // 5% from top
        static constexpr float PORTRAIT_COINBAG_X = 0.01f;     // 1% from left
        static constexpr float PORTRAIT_COINBAG_Y = 0.87f;     // 87% from top (bottom left corner)
        static constexpr float PORTRAIT_PIPE_Y = 0.10f;        // 10% from top
        static constexpr float PORTRAIT_PLAYER_START_X = 400.0f;
        static constexpr float PORTRAIT_PLAYER_START_Y = 639.0f;
        static constexpr float BOSS_PLAYER_X_PERCENT = 0.15f;  // 15% from left for boss level to avoid UI overlap

    // Orientation-specific UI positioning constants (Landscape mode - new)
    static constexpr float LANDSCAPE_SETTINGS_X = 0.95f;   // 95% from left (further right in landscape)
    static constexpr float LANDSCAPE_SETTINGS_Y = 0.08f;   // Slightly lower (8% from top)
        static constexpr float LANDSCAPE_COINBAG_X = 0.01f;    // 1% from left (even further left to avoid player overlap)
        static constexpr float LANDSCAPE_COINBAG_Y = 0.75f;    // 75% from top (raised 5% more)
        static constexpr float LANDSCAPE_PIPE_Y = 0.12f;       // Slightly lower (12% from top)
        static constexpr float LANDSCAPE_PLAYER_OFFSET_X = 320.0f; // Moved further right (~150px)

        // Private methods
        void InitializeSystems();
        void CreateGameEntities();
        void CreateBackgroundLayers();
        void DestroyGameEntities();

        // Screen dimension caching (eliminates 40+ repeated GetScreenInfo() calls)
        void CacheScreenDimensions();

        // Coordinate conversion helpers (eliminates duplicate normalization code)
        void NormalizeCoordinates(float pixelX, float pixelY, float& outNormalizedX, float& outNormalizedY);
        void DenormalizeCoordinates(float normalizedX, float normalizedY, float& outPixelX, float& outPixelY);
        Gnosis::GNVector2 CenterObjectAtPosition(float centerX, float centerY, float width, float height);

        void ResetPlayerEntity();
        void CreateUI();
        void DestroyUI();
        void UpdateGameLogic(float deltaTime);
        void UpdateObjectPools(float deltaTime);
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
        void ShowPauseMenu();
        void HidePauseMenu();
        void CreateSettingsButton();
        
        // Stats management
        void IncrementDeathCounter();         // Increment death counter when player dies
        bool CheckSettingsButtonClick(float touchX, float touchY);
        

        // Input helpers
        bool IsTapInSettingsButtonArea(float touchX, float touchY);
        bool IsTapOutsideMenuArea(float touchX, float touchY);
    };

} // namespace GameCore

#endif // FLOPPY_TURD_GAMEPLAY_STATE_H 