#include "LoadingState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include <iostream>

namespace FloppyTurd {

    // Static constants
    const float LoadingState::LOADING_DURATION = 3.0f;  // 3 seconds
    const float LoadingState::TEXT_BLINK_INTERVAL = 0.5f; // Blink every 0.5 seconds

    LoadingState::LoadingState(Gnosis::ECS* ecsSystem)
        : m_ecsSystem(ecsSystem)
        , m_finished(false)
        , m_loadingTimer(0.0f)
        , m_textBlinkTimer(0.0f)
        , m_showText(true) {
    }

    void LoadingState::Enter() {
        GN_LOG_INFO("Entering Loading State");
        m_finished = false;
        m_loadingTimer = 0.0f;
        m_textBlinkTimer = 0.0f;
        m_showText = true;
    }

    void LoadingState::Exit() {
        GN_LOG_INFO("Exiting Loading State");
    }

    void LoadingState::Pause() {
        // Loading state doesn't need to handle pause
    }

    void LoadingState::Resume() {
        // Loading state doesn't need to handle resume
    }

    void LoadingState::Update(float deltaTime) {
        // Update loading timer
        m_loadingTimer += deltaTime;
        
        // Update text blink timer
        m_textBlinkTimer += deltaTime;
        if (m_textBlinkTimer >= TEXT_BLINK_INTERVAL) {
            m_showText = !m_showText;
            m_textBlinkTimer = 0.0f;
        }
        
        // Check if loading is complete
        if (m_loadingTimer >= LOADING_DURATION) {
            m_finished = true;
        }
    }

    void LoadingState::Render() {
        // This will be handled by the RenderSystem
        // For now, we'll just ensure the ECS renders our entities
        if (m_ecsSystem) {
            m_ecsSystem->Render();
        }
    }

    void LoadingState::HandleInput() {
        // Loading state doesn't handle input
        // Could add skip functionality later
    }

} // namespace FloppyTurd