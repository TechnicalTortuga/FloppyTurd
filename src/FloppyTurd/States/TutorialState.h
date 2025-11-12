#ifndef FLOPPY_TURD_TUTORIAL_STATE_H
#define FLOPPY_TURD_TUTORIAL_STATE_H

#include "GameState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GnosisTypes.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include "../Systems/SpriteSystem.h"
#include "../Systems/PlayerControllerSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/ProjectileSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <string>
#include <vector>
#include <memory>

namespace GameCore {

    /**
     * @brief Tutorial State - Interactive how-to guide
     *
     * Refactored to use GameplayState patterns:
     * - Uses SpriteSystem, PlayerControllerSystem, RenderSystem
     * - Park Level background (static, no scrolling)
     * - Player constrained to bottom 50% of screen
     * - UI matches GameplayState layout exactly
     * - Simple pause menu (no PauseSystem dependency)
     * - Functional shooting with coin counter
     */
    class TutorialState : public GameState {
    public:
        TutorialState(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates);
        ~TutorialState() override;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Tutorial"; }

    private:
        // Core systems
        Gnosis::ECS* m_ecsSystem;
        PlatformDelegates* m_platformDelegates;
        FloppyTurdGame* m_game;
        
        // Game systems (shared from SystemManager - NOT owned!)
        SpriteSystem* m_spriteSystem;
        PlayerControllerSystem* m_playerControllerSystem;
        RenderSystem* m_renderSystem;
        ProjectileSystem* m_projectileSystem;
        
        // Cached screen dimensions
        float m_cachedScreenWidth;
        float m_cachedScreenHeight;
        
        // Game entities
        Gnosis::Entity m_playerEntity;
        std::vector<Gnosis::Entity> m_backgroundEntities;
        
        // UI entities
        Gnosis::Entity m_pipeCounterEntity;
        Gnosis::Entity m_coinBagEntity;
        Gnosis::Entity m_coinsTextEntity;
        Gnosis::Entity m_shootingZoneEntity;
        Gnosis::Entity m_settingsButtonEntity;
        std::vector<Gnosis::Entity> m_instructionTextEntities;
        
        // Simple pause menu entities (NO PauseSystem)
        Gnosis::Entity m_pauseBackgroundEntity;
        Gnosis::Entity m_pausedTextEntity;
        Gnosis::Entity m_returnToMenuButtonEntity;
        
        // Input state
        bool m_touchPressed;
        bool m_touchReleased;
        Gnosis::GNVector2 m_touchPosition;
        
        // State flags
        bool m_initialized;
        bool m_isPaused;
        bool m_finished;
        int m_tutorialCoins;  // Functional coin counter (starts at 999)
        
        // Jump mechanics (variable jump like GameplayState)
        bool m_jumpButtonHeld;
        uint64_t m_jumpStartTime;
        float m_jumpCooldown;
        
        // System initialization
        void InitializeSystems();
        
        // Entity creation
        void CreateBackground();
        void CreatePlayer();
        void CreateUI();
        void CreateInstructionText();
        void CreateSettingsButton();
        void CreatePauseMenu();
        
        // Pause menu control
        void ShowPauseMenu();
        void HidePauseMenu();
        void TogglePause();
        
        // Screen dimensions
        void CacheScreenDimensions();
        
        // Player constraint
        void ConstrainPlayer();
        
        // Cleanup
        void DestroyUI();
        void DestroyEntities();
        
        // Input handling
        void UpdateInput();
        
        // Input helpers
        bool CheckSettingsButtonClick(float x, float y);
        bool CheckReturnToMenuButtonClick(float x, float y);
        bool CheckShootingZoneClick(float x, float y);
        
        // Gameplay actions
        void HandleJump();
        void HandleJumpRelease();
        void HandleShooting();
        void UpdatePhysics(float deltaTime);
        void UpdateJumpMechanics(float deltaTime);
        void UpdatePlayerAnimation();  // Check animation completion
        
        // UI updates
        void UpdateCoinCounterUI();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_TUTORIAL_STATE_H
