#ifndef FLOPPY_TURD_CREDITS_STATE_H
#define FLOPPY_TURD_CREDITS_STATE_H

#include "GameState.h"
#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <vector>
#include <string>

namespace GameCore {

    /**
     * @brief Credits State - displays scrolling credits with bouncing turd and pipes
     * 
     * Features:
     * - Landscape mode only
     * - Scrolling credit text (horizontal)
     * - Bouncing turd sprite (no collision)
     * - Scrolling toilet pair pipes
     * - Background music (EndTheme.mp3)
     * - Skip button in bottom right corner
     * - Fades in from white (from boss death sequence)
     * - Returns to main menu via ScreenPromptState when complete
     */
    class CreditsState : public GameState {
    public:
        CreditsState(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates);
        ~CreditsState() override;

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
        const char* GetStateName() const override { return "Credits"; }

        // Check if credits should be skipped
        bool ShouldSkip() const { return m_shouldSkip; }

    private:
        // Core systems
        Gnosis::ECS* m_ecsSystem;
        PlatformDelegates* m_platformDelegates;

        // State flags
        bool m_finished;
        bool m_shouldSkip;
        bool m_musicStarted;

        // Timing
        float m_elapsedTime;
        float m_totalMusicDuration;
        float m_turdBounceTimer;
        float m_textScrollOffset;
        float m_whiteFadeAlpha;  // Fade in from white at start

        // Scroll settings
        static constexpr float SCROLL_SPEED = 40.0f;  // pixels per second
        static constexpr float TURD_BOUNCE_SPEED = 2.0f;  // radians per second
        static constexpr float TURD_BOUNCE_AMPLITUDE = 10.0f;  // pixels
        static constexpr float PIPE_SPEED = 80.0f;  // pixels per second
        static constexpr float PIPE_SPACING = 120.0f;  // pixels between pipes
        static constexpr float FADE_IN_DURATION = 1.0f;  // seconds to fade in from white
        static constexpr float TEXT_BASE_Y = 90.0f;  // Base Y position for credit text

        // Screen dimensions (cached for landscape)
        float m_screenWidth;
        float m_screenHeight;

        // Entities
        Gnosis::Entity m_backgroundEntity;
        Gnosis::Entity m_turdEntity;
        Gnosis::Entity m_skipButtonEntity;
        Gnosis::Entity m_whiteFadeEntity;  // White overlay that fades out at start
        std::vector<Gnosis::Entity> m_pipeEntities;
        std::vector<Gnosis::Entity> m_creditTextEntities;

        // Credit entry structure
        struct CreditEntry {
            std::string title;
            std::string name;
            float yOffset;  // Random vertical offset for visual variety
        };
        std::vector<CreditEntry> m_creditEntries;

        // Entity creation
        void CreateEntities();
        void CreateBackground();
        void CreateTurd();
        void CreatePipes();
        void CreateCreditText();
        void CreateSkipButton();
        void CreateWhiteFadeOverlay();

        // Entity destruction
        void DestroyEntities();

        // Update helpers
        void UpdateTurdBounce(float deltaTime);
        void UpdatePipes(float deltaTime);
        void UpdateCreditScroll(float deltaTime);
        void UpdateWhiteFade(float deltaTime);
        void CheckMusicCompletion();

        // Input helpers
        bool IsSkipButtonPressed(float touchX, float touchY);

        // Audio
        void StartCreditsMusic();
        void StopCreditsMusic();
        float GetMusicDuration();

        // Screen info helpers
        void CacheScreenDimensions();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_CREDITS_STATE_H