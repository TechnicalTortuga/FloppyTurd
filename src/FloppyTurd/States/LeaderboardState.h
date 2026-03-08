#ifndef FLOPPY_TURD_LEADERBOARD_STATE_H
#define FLOPPY_TURD_LEADERBOARD_STATE_H

#include "GameState.h"
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../iOS/Threading/ThreadingProxy.h"
#include "../Game/FloppyTurdGame.h"
#include <vector>
#include <string>

namespace GameCore {

    /**
     * @brief Leaderboard State - displays game statistics and leaderboards
     * 
     * Features:
     * - Multiple leaderboard pages (levels 1-5 pipes, level 6 boss time, total stats)
     * - Left/Right arrows to navigate between pages
     * - Uses pause menu background mobile overlay on main menu background
     * - Back button to return to main menu
     * - Game Center integration for online leaderboards
     * 
     * Leaderboard Pages:
     * 1. Level 1 - Park (High Score: Pipes)
     * 2. Level 2 - Gold Flush (High Score: Pipes)
     * 3. Level 3 - Ice Throne (High Score: Pipes)
     * 4. Level 4 - Sewer (High Score: Pipes)
     * 5. Level 5 - Sky Castle (High Score: Pipes)
     * 6. Level 6 - Rat King Boss (Best Time)
     * 7. Total Enemies Defeated
     * 8. Total Coins Collected
     * 9. Total Pipes Cleared
     */
    class LeaderboardState : public GameState {
    public:
        LeaderboardState(ECS* ecsSystem, PlatformDelegates* platformDelegates);
        ~LeaderboardState() override;

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
        const char* GetStateName() const override { return "Leaderboard"; }

    private:
        // Static instance for callbacks (only one leaderboard state active at a time)
        static LeaderboardState* s_instance;

        // Leaderboard page types
        enum class LeaderboardPage {
            LEVEL_2_SEWER = 0,          // "Home Sweet Home"
            LEVEL_3_DESERT = 1,         // "The Good, The Bad, and the Stinky"
            LEVEL_4_SNOW = 2,           // "Polar Pandemonium"
            LEVEL_5_CASTLE = 3,         // "Dung in the Dungeon"
            LEVEL_6_BOSS = 4,           // "Curtains for Crap"
            TOTAL_ENEMIES = 5,
            TOTAL_COINS = 6,
            TOTAL_PIPES = 7,
            LEVEL_1_PARK = 8,           // "A Flop in the Park" (Legacy - moved to end)
            COUNT = 9
        };

        // Core systems
        ECS* m_ecsSystem;
        PlatformDelegates* m_platformDelegates;
        FloppyTurdGame* m_game;

        // State flags
        bool m_finished;
        bool m_initialized;

        // Current page
        LeaderboardPage m_currentPage;

        // Screen dimensions
        float m_screenWidth;
        float m_screenHeight;
        float m_uiScale;

        // Overlay position tracking (for positioning UI within the centered overlay)
        float m_overlayX;
        float m_overlayY;
        float m_overlayWidth;
        float m_overlayHeight;

        // UI Entities
        Entity m_backgroundEntity;
        Entity m_overlayBackgroundEntity;
        Entity m_titleEntity;
        Entity m_backButtonEntity;
        Entity m_leftArrowEntity;
        Entity m_rightArrowEntity;
        
        // Content entities for current page
        Entity m_pageTitleEntity;
        Entity m_localScoreEntity;
        std::vector<Entity> m_contentEntities;

        // Input debouncing
        float m_lastArrowPressTime;
        float m_lastButtonPressTime;
        static constexpr float BUTTON_DEBOUNCE_DELAY = 0.2f;

        // UI creation
        void CreateUI();
        void CreateBackground();
        void CreateNavigationButtons();
        void CreatePageContent();
        void DestroyPageContent();
        void UpdatePageContent();

        // Navigation
        void OnLeftArrowPressed();
        void OnRightArrowPressed();
        void OnBackButtonPressed();

        // Page info helpers
        std::string GetPageTitle(LeaderboardPage page) const;
        std::string GetLocalScoreText(LeaderboardPage page) const;
        std::string GetLeaderboardID(LeaderboardPage page) const;
        
        // Game Center leaderboard data fetching
        void LoadLeaderboardData();
        void UpdateLeaderboardUI(const LeaderboardEntry* entries, int count);
        void UpdateLocalPlayerUI(int rank, int64_t score);
        
        // Static callback functions for Game Center (C-compatible)
        static void OnLeaderboardEntriesLoaded(const LeaderboardEntry* entries, int count, bool success);
        static void OnLocalPlayerEntryLoaded(int rank, int64_t score, bool success);
        
        // Utility
        std::string FormatTime(float seconds) const;
        std::string FormatScore(int64_t score) const;
        bool IsMobilePlatform() const;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_LEADERBOARD_STATE_H