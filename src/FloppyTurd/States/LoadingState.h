#ifndef FLOPPY_TURD_LOADING_STATE_H
#define FLOPPY_TURD_LOADING_STATE_H

#include "GameState.h"

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

    private:
        Gnosis::ECS* m_ecsCoordinator;  // Reference to shared ECS coordinator
        bool m_finished;
        float m_loadingTimer;
        float m_rotationAngle;  // Current rotation angle in degrees
        void* m_poopHatEntity;  // Pointer to the poop hat entity (TODO: proper type when ECS is integrated)
        
        static const float LOADING_DURATION;
        static const float ROTATION_SPEED;   // degrees per second
        static const float ORBIT_RADIUS;     // pixels from center
        
        // Helper functions
        void CreateLoadingEntities();
        void DestroyLoadingEntities();
        void UpdatePoopHatPosition();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_LOADING_STATE_H