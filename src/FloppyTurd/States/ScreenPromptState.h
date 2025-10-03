#ifndef FLOPPY_TURD_SCREEN_PROMPT_STATE_H
#define FLOPPY_TURD_SCREEN_PROMPT_STATE_H

#include "GameState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GnosisTypes.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include "../../Engine/Core/GNLog.h"
#include <string>

namespace GameCore {

    /**
     * @brief Screen Rotation Prompt State
     *
     * Shows a black screen with white text asking the player to rotate to the specified orientation
     * (landscape for entering boss level, portrait for exiting boss level)
     */
    class ScreenPromptState : public GameState {
    public:
        ScreenPromptState(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, bool waitForLandscape = true);
        ~ScreenPromptState() override;

        void Enter() override;
        void Exit() override;
        void Pause() override;
        void Resume() override;

        void Update(float deltaTime) override;
        void Render() override;
        void HandleInput() override;

        bool IsFinished() const override { return m_finished; }
        const char* GetStateName() const override { return "ScreenPrompt"; }

    private:
        Gnosis::ECS* m_ecsSystem;
        GameCore::PlatformDelegates* m_platformDelegates;
        FloppyTurdGame* m_game;
        bool m_finished;
        float m_displayTime;
        float m_fadeTimer;
        bool m_fadingOut;
        Gnosis::Entity m_backgroundEntity;
        Gnosis::Entity m_textEntity;
        Gnosis::Entity m_arrowsEntity;
        bool m_uiInitialized;
        float m_landscapeDetectedTime;
        bool m_hasSeenPortrait;
        bool m_waitForLandscape;  // true = wait for landscape (entering boss), false = wait for portrait (exiting boss)

        // Screen info tracking for orientation changes
        bool m_currentOrientation;
        float m_currentScreenWidth;
        float m_currentScreenHeight;

        void CreatePromptUI();
        void DestroyPromptUI();
        void CheckOrientation();
        void ForceOrientationCheck();
        void RepositionUIElements(const ScreenInfo& screenInfo);
    };

} // namespace GameCore

#endif // FLOPPY_TURD_SCREEN_PROMPT_STATE_H
