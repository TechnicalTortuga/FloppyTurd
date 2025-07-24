#ifndef FLOPPY_TURD_LOADING_STATE_H
#define FLOPPY_TURD_LOADING_STATE_H

#include "GameState.h"

namespace FloppyTurd {

    /**
     * @brief Loading State - shows blue screen with "Loading" text
     * 
     * Simple initial state to verify our rendering pipeline works
     */
    class LoadingState : public GameState {
    public:
        LoadingState(Gnosis::ECS* ecsSystem);
        ~LoadingState() override = default;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "Loading"; }

    private:
        Gnosis::ECS* m_ecsSystem;
        bool m_finished;
        float m_loadingTimer;
        float m_textBlinkTimer;
        bool m_showText;
        
        static const float LOADING_DURATION;
        static const float TEXT_BLINK_INTERVAL;
    };

} // namespace FloppyTurd

#endif // FLOPPY_TURD_LOADING_STATE_H