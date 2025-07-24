#ifndef FLOPPY_TURD_GAME_STATE_H
#define FLOPPY_TURD_GAME_STATE_H

#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Core/ECS.h"
#include <memory>

namespace FloppyTurd {

    /**
     * @brief Base class for all game states in Floppy Turd
     * 
     * This abstract class defines the interface for game states like
     * MainMenu, Playing, Paused, GameOver, etc.
     */
    class GameState {
    public:
        GameState() = default;
        virtual ~GameState() = default;

        // State lifecycle
        virtual void Enter() = 0;
        virtual void Exit() = 0;
        virtual void Pause() = 0;
        virtual void Resume() = 0;

        // State updates
        virtual void Update(float deltaTime) = 0;
        virtual void Render() = 0;
        virtual void HandleInput() = 0;

        // State queries
        virtual bool IsFinished() const = 0;
        virtual const char* GetStateName() const = 0;
    };

    /**
     * @brief Game State Manager
     * 
     * Manages the stack of game states and handles transitions between them.
     */
    class GameStateManager {
    public:
        GameStateManager();
        ~GameStateManager() noexcept;

        // State management
        void PushState(std::unique_ptr<GameState> state);
        void PopState();
        void ChangeState(std::unique_ptr<GameState> state);
        void ClearStates();

        // Updates
        void Update(float deltaTime);
        void Render();
        void HandleInput();

        // Queries
        bool IsEmpty() const;
        GameState* GetCurrentState() const;
        size_t GetStateCount() const;

    private:
        std::vector<std::unique_ptr<GameState> > m_stateStack;
        std::vector<std::unique_ptr<GameState> > m_pendingStates;
        bool m_shouldPop;
        bool m_shouldClear;

        void ProcessPendingChanges();
    };

    /**
     * @brief Main Menu State
     */
    class MainMenuState : public GameState {
    public:
        MainMenuState(Gnosis::ECS* ecsSystem);
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
        Gnosis::ECS* m_ecsSystem;
        bool m_finished;
        int m_selectedOption;
        float m_animationTimer;

        void CreateMenuEntities();
        void DestroyMenuEntities();
        void UpdateMenuSelection();
    };

    /**
     * @brief Playing State - main gameplay
     */
    class PlayingState : public GameState {
    public:
        PlayingState(Gnosis::ECS* ecsSystem);
        ~PlayingState() override;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Playing"; }

        // Game-specific methods
        void SpawnObstacle();
        void SpawnPowerUp();
        void CheckCollisions();
        void UpdateScore(int points);
        void GameOver();

    private:
        Gnosis::ECS* m_ecsSystem;
        bool m_finished;
        bool m_paused;
        
        // Game entities
        Gnosis::Entity m_playerEntity;
        std::vector<Gnosis::Entity> m_obstacles;
        std::vector<Gnosis::Entity> m_powerUps;
        std::vector<Gnosis::Entity> m_projectiles;
        
        // Game state
        int m_score;
        int m_lives;
        float m_gameSpeed;
        float m_obstacleSpawnTimer;
        float m_powerUpSpawnTimer;
        
        // Spawn timers
        static const float OBSTACLE_SPAWN_INTERVAL;
        static const float POWERUP_SPAWN_INTERVAL;
        static const float GAME_SPEED_INCREASE;
        
        void CreateGameEntities();
        void DestroyGameEntities();
        void UpdateSpawning(float deltaTime);
        void UpdateGameSpeed(float deltaTime);
        void CleanupDestroyedEntities();
    };

    /**
     * @brief Paused State
     */
    class PausedState : public GameState {
    public:
        PausedState(Gnosis::ECS* ecsSystem);
        ~PausedState() override;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Paused"; }

    private:
        Gnosis::ECS* m_ecsSystem;
        bool m_finished;
        int m_selectedOption;
        
        void CreatePauseUI();
        void DestroyPauseUI();
    };

    /**
     * @brief Game Over State
     */
    class GameOverState : public GameState {
    public:
        GameOverState(Gnosis::ECS* ecsSystem, int finalScore, int coinsEarned);
        ~GameOverState() override;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "GameOver"; }

    private:
        Gnosis::ECS* m_ecsSystem;
        bool m_finished;
        int m_finalScore;
        int m_coinsEarned;
        int m_selectedOption;
        float m_animationTimer;
        bool m_newHighScore;
        
        void CreateGameOverUI();
        void DestroyGameOverUI();
        void CheckHighScore();
        void SaveGameData();
    };

    /**
     * @brief Shop State
     */
    class ShopState : public GameState {
    public:
        ShopState(Gnosis::ECS* ecsSystem);
        ~ShopState() override;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Shop"; }

    private:
        Gnosis::ECS* m_ecsSystem;
        bool m_finished;
        int m_selectedCategory;
        int m_selectedItem;
        int m_playerCoins;
        
        void CreateShopUI();
        void DestroyShopUI();
        void UpdateShopDisplay();
        void PurchaseItem();
        void LoadPlayerData();
        void SavePlayerData();
    };

} // namespace FloppyTurd

#endif // FLOPPY_TURD_GAME_STATE_H