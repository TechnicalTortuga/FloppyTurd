#include "TransitionState.h"
#include "../../Engine/Core/GNLog.h"

namespace GameCore {

    TransitionState::TransitionState(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates, const char* targetStateName)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_finished(false)
        , m_timer(0.0f)
        , m_targetStateName(targetStateName)
    {
        GN_LOG_INFO("TransitionState created - target: " + std::string(targetStateName));
    }

    TransitionState::~TransitionState() {
        GN_LOG_INFO("TransitionState destroyed");
    }

    void TransitionState::Enter() {
        GN_LOG_INFO("Entering TransitionState - will transition to: " + std::string(m_targetStateName));
        
        m_finished = false;
        m_timer = 0.0f;

        // NOTE: Orientation should already be locked by previous state (ScreenPromptState)
        GN_LOG_INFO("TransitionState: Assuming orientation already locked to portrait");

        // Black screen is rendered automatically by Metal clear color
        GN_LOG_INFO("TransitionState: Black screen active, waiting " + std::to_string(TRANSITION_DURATION) + "s for orientation to settle");
    }

    void TransitionState::Exit() {
        GN_LOG_INFO("Exiting TransitionState");
    }

    void TransitionState::Pause() {
        // No-op
    }

    void TransitionState::Resume() {
        // No-op
    }

    void TransitionState::Update(float deltaTime) {
        m_timer += deltaTime;

        // After delay, signal that we're ready to transition
        if (m_timer >= TRANSITION_DURATION) {
            GN_LOG_INFO("TransitionState: Delay complete, ready to transition to: " + std::string(m_targetStateName));
            m_finished = true;
        }
    }

    void TransitionState::Render() {
        // Black screen - no rendering needed (Metal clear color handles it)
    }

    void TransitionState::HandleInput() {
        // No input during transition
    }

} // namespace GameCore
