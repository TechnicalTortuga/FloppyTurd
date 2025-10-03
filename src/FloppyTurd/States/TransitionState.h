#pragma once

#include "GameState.h"
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"

namespace GameCore {

    // Simple black screen transition state for orientation changes
    // Used when transitioning from landscape to portrait (boss -> main menu)
    class TransitionState : public GameState {
    public:
        TransitionState(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, const char* targetStateName);
        ~TransitionState();

        // State interface
        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;
        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;
        const char* GetStateName() const override { return "Transition"; }

        bool IsFinished() const { return m_finished; }
        const char* GetTargetStateName() const { return m_targetStateName; }

    private:
        Gnosis::ECS* m_ecsSystem;
        GameCore::PlatformDelegates* m_platformDelegates;
        bool m_finished;
        float m_timer;
        const char* m_targetStateName;  // Where to transition to after delay
        
        static constexpr float TRANSITION_DURATION = 0.15f; // 150ms delay
    };

} // namespace GameCore
