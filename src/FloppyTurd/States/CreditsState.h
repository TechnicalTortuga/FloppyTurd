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
        float m_whiteFadeAlpha;  // Fade in from white at start, fade out to white at end
        bool m_fadingOut;  // Track if we're fading out to white before transition

        // Scroll settings
        static constexpr float SCROLL_SPEED = 129.9375f;  // pixels per second (~5% faster than previous effective rate)
        static constexpr float TURD_BOUNCE_SPEED = 2.0f;  // radians per second
        static constexpr float TURD_BOUNCE_AMPLITUDE = 10.0f;  // pixels
        static constexpr float PIPE_SPEED = 180.0f;  // pixels per second (increased significantly for faster pipes)
        static constexpr float PIPE_SPACING = 120.0f;  // pixels between pipes
        static constexpr float TOILET_GAP = 400.0f;    // vertical gap between top/bottom toilets (reduced from 800)
        static constexpr float LABEL_GAP_FACTOR = 0.5f; // reduce horizontal gap between labels by half

        // Timing
        static constexpr float FADE_IN_DURATION = 1.0f;   // seconds to fade in from white at start
        static constexpr float FADE_OUT_DURATION = 2.0f;  // seconds to fade out to white before transition (doubled)
        static constexpr float CREDITS_DURATION = 121.0f; // 2 minutes 1 second (song duration + buffer)

        // Final label behavior: center tolerances and hold duration before fade
        static constexpr float FINAL_CENTER_TOLERANCE = 16.0f;   // px from exact horizontal screen center to trigger final hold
        static constexpr float FINAL_CENTER_Y_TOLERANCE = 16.0f; // px from exact vertical screen center (for optional vertical centering)
        static constexpr float FINAL_HOLD_DURATION = 5.0f;       // seconds to hold final label before fade to white
        static constexpr bool  FINAL_CENTER_VERTICALLY = true;   // also vertically center final label on hold

        // Exit behavior and diagnostics
        static constexpr bool ENFORCE_FINAL_SEQUENCE_EXIT_ONLY = true; // only exit via final centered label + hold
        static constexpr bool ENABLE_CREDITS_VERBOSE_LOG = true;       // stronger logging for credits flow

        // Labels
        static constexpr const char* SPECIAL_THANKS_TITLE = "Special Thanks"; // ensure presence/title consistency

        static constexpr float TEXT_BASE_Y = 90.0f;  // Base Y position for credit text

        // Screen dimensions (cached for landscape)
        float m_screenWidth;
        float m_screenHeight;

        // Entities
        Gnosis::Entity m_backgroundEntity;
        std::vector<Gnosis::Entity> m_backgroundEntities;  // Optional multi-background support for seamless width
        Gnosis::Entity m_turdEntity;
        Gnosis::Entity m_skipButtonEntity;
        Gnosis::Entity m_whiteFadeEntity;  // White overlay that fades out at start
        std::vector<Gnosis::Entity> m_pipeEntities;
        std::vector<Gnosis::Entity> m_creditTextEntities;

        // Final label / sequence state
        bool m_finalHoldActive;
        float m_finalHoldTimer;
        Gnosis::Entity m_finalTitleEntity;
        Gnosis::Entity m_finalNameEntity;

        // Exit control: when true, only the final sequence may finish the credits
        bool m_enforceFinalExitOnly;
        
        // Diagnostics: track which entry is the final one for logging
        size_t m_finalEntryIndex;

        // Credit entry structure
        struct CreditEntry {
            std::string title;
            std::string name;
            float yOffset;  // Random vertical offset for visual variety
            Gnosis::Entity titleEntity;  // Entity for title text (0 if no title)
            Gnosis::Entity nameEntity;   // Entity for name text (0 if no name)
            float titleWidth;  // Computed width of title text in pixels
            float nameWidth;   // Computed width of name text in pixels
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
        // Final label helpers
        void BeginFinalSequence();                               // Starts final 5s hold and triggers fade-out
        bool IsFinalLabelCentered(float tolerancePx) const;      // Returns true when final label is centered on screen

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