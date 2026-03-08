#ifndef FLOPPY_TURD_LOADING_STATE_H
#define FLOPPY_TURD_LOADING_STATE_H

#include "GameState.h"
#include <string>

namespace GameCore {

    /**
     * @brief Loading State - shows rotating poop hat icon while loading
     * 
     * Displays the base poop hat rotating around the center of the screen
     * until loading is complete, then transitions to main menu
     */
    class LoadingState : public GameState {
    public:
        LoadingState(Gnosis::ECS* ecsCoordinator);
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
        
        // Get loading progress (0.0 to 1.0)
        float GetLoadingProgress() const;
        
        // Set the level being loaded (used to apply level-specific optimizations)
        void SetLoadingLevel(const std::string& levelName);
        
        // Check if we're loading a specific level
        bool IsLoadingLevel(const std::string& levelName) const;

    private:
        Gnosis::ECS* m_ecsCoordinator;  // Reference to shared ECS coordinator
        bool m_finished;
        bool m_assetsLoaded;
        float m_loadingTimer;
        float m_rotationAngle;  // Current rotation angle in degrees
        void* m_poopHatEntity;  // Pointer to the poop hat entity
        std::string m_currentLoadingLevel;  // Name of the level being loaded
        
        static const float MIN_LOADING_DURATION;  // Minimum time to show loading screen
        static const float MAX_LOADING_DURATION;  // Maximum time before forcing transition
        static const float ROTATION_SPEED;   // degrees per second
        static const float ORBIT_RADIUS;     // pixels from center
        
        // Helper functions
        void CreateLoadingEntities();
        void DestroyLoadingEntities();
        void UpdatePoopHatPosition();
        void PreloadAssets();
        
        // Helper to check if we should reduce logging (for problematic levels like desert)
        bool ShouldReduceLogging() const;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_LOADING_STATE_H