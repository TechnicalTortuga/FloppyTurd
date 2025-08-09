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
#include "../Systems/UISystem.h"
#include "../Config/LevelConfig.h"
#include <memory>
#include <vector>
#include <map>

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
        void HandleInput() override;

        // State queries
        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Gameplay"; }

        // Game-specific methods
        void SetLevel(int levelId);
        void RestartLevel();
        void GameOver();
        void LevelComplete();
        
        // Score and progression
        int GetCurrentScore() const { return m_currentScore; }
        int GetCurrentCoins() const { return m_currentCoins; }
        int GetCurrentLives() const { return m_currentLives; }
        
        // Pause menu
        void TogglePause();
        bool IsPaused() const { return m_isPaused; }
        
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
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::unique_ptr<LevelManager> m_levelManager;
        std::unique_ptr<UISystem> m_uiSystem;

        // Level configuration
        int m_currentLevelId;
        LevelConfig m_currentLevelConfig;

        // Game entities
        Gnosis::Entity m_playerEntity;
        Gnosis::Entity m_cameraEntity;
        std::vector<Gnosis::Entity> m_backgroundEntities;
        std::vector<Gnosis::Entity> m_obstacles;
        std::vector<Gnosis::Entity> m_pickups;
        std::vector<Gnosis::Entity> m_projectiles;
        std::vector<Gnosis::Entity> m_enemies;

        // Game state
        int m_currentScore;
        int m_currentCoins;
        int m_currentLives;
        float m_gameTime;
        float m_difficultyTimer;
        float m_difficultyLevel;
        
        // Player state
        bool m_playerAlive;
        float m_invulnerabilityTimer;
        int m_pipesCleared;                     // Number of pipes passed through
        // Hurt state is now managed by PlayerControllerSystem
        
        // Game flow
        bool m_finished;
        bool m_isPaused;
        bool m_levelCompleted;
        bool m_gameOver;
        
        // UI elements
        Gnosis::Entity m_scoreTextEntity;
        Gnosis::Entity m_livesTextEntity;
        Gnosis::Entity m_coinsTextEntity;
        Gnosis::Entity m_pipeCounterEntity;     // Pipe counter display under notch
        Gnosis::Entity m_pauseMenuEntity;
        Gnosis::Entity m_tempMenuButtonEntity;  // Temporary button to return to main menu

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
        void CreateUI();
        void DestroyUI();
        void UpdateGameLogic(float deltaTime);
        void UpdateSpawning(float deltaTime);
        void UpdateDifficulty(float deltaTime);
        void HandleGameEvents();
        void SpawnObstacle();
        void SpawnPickup();
        void SpawnEnemy();
        void CleanupOffscreenEntities();
        void CheckLevelCompletion();
        void SaveGameProgress();
        
        // Collision and pipe tracking
        void CheckToiletCollisions();
        void UpdatePipeCounterUI();
        void OnPipeCleared();
        
        // Debug rendering
        void DrawDebugRectangles();
        
        // Audio methods
        void StartLevelMusic();
        void StopLevelMusic();
        
        // Event handlers
        void OnPlayerJump();
        void OnPlayerShoot();
        void OnPlayerHurt(int damage);
        void OnPlayerDeath();
        void OnCoinCollected(int value);
        void OnPickupCollected();
        void OnObstacleHit();
        void OnEnemyDefeated();
        
        // Menu navigation (for pause menu integration later)
        void ReturnToMainMenu();  // Function to connect to pause menu later
        void CheckMenuButtonClick(float touchX, float touchY);
    };

} // namespace GameCore

#endif // FLOPPY_TURD_GAMEPLAY_STATE_H 