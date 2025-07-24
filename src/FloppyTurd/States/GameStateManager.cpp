#include "GameState.h"
#include "../../Engine/Core/GNLog.h"

namespace FloppyTurd {

    GameStateManager::GameStateManager() 
        : m_shouldPop(false), m_shouldClear(false) {
        GN_LOG_INFO("GameStateManager created");
    }

    GameStateManager::~GameStateManager() noexcept {
        ClearStates();
        GN_LOG_INFO("GameStateManager destroyed");
    }

    void GameStateManager::PushState(std::unique_ptr<GameState> state) {
        if (state) {
            m_pendingStates.push_back(std::move(state));
            GN_LOG_INFO("State queued for push");
        }
    }

    void GameStateManager::PopState() {
        m_shouldPop = true;
        GN_LOG_INFO("State queued for pop");
    }

    void GameStateManager::ChangeState(std::unique_ptr<GameState> state) {
        m_shouldClear = true;
        if (state) {
            m_pendingStates.push_back(std::move(state));
        }
        GN_LOG_INFO("State change queued");
    }

    void GameStateManager::ClearStates() {
        m_stateStack.clear();
        m_pendingStates.clear();
        m_shouldPop = false;
        m_shouldClear = false;
        GN_LOG_INFO("All states cleared");
    }

    void GameStateManager::Update(float deltaTime) {
        ProcessPendingChanges();
        
        if (!m_stateStack.empty()) {
            m_stateStack.back()->Update(deltaTime);
        }
    }

    void GameStateManager::Render() {
        if (!m_stateStack.empty()) {
            m_stateStack.back()->Render();
        }
    }

    void GameStateManager::HandleInput() {
        if (!m_stateStack.empty()) {
            m_stateStack.back()->HandleInput();
        }
    }

    bool GameStateManager::IsEmpty() const {
        return m_stateStack.empty();
    }

    GameState* GameStateManager::GetCurrentState() const {
        if (!m_stateStack.empty()) {
            return m_stateStack.back().get();
        }
        return nullptr;
    }

    size_t GameStateManager::GetStateCount() const {
        return m_stateStack.size();
    }

    void GameStateManager::ProcessPendingChanges() {
        // Handle clear request
        if (m_shouldClear) {
            m_stateStack.clear();
            m_shouldClear = false;
        }
        
        // Handle pop request
        if (m_shouldPop && !m_stateStack.empty()) {
            m_stateStack.pop_back();
            m_shouldPop = false;
        }
        
        // Handle pending state pushes
        for (auto& state : m_pendingStates) {
            if (state) {
                m_stateStack.push_back(std::move(state));
            }
        }
        m_pendingStates.clear();
    }

}