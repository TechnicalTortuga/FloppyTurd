#pragma once

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include "../Config/LevelConfig.h"
#include <string>
#include <vector>

namespace GameCore {

    /**
     * HeartSystem - Manages player heart UI display and heart-related logic
     * 
     * Features:
     * - Renders hearts in vertical column matching coin bag X position
     * - Supports whole hearts, halves, and thirds display modes
     * - Handles difficulty-based initial heart counts
     * - Manages heart slice calculations and display states
     */
    class HeartSystem {
    private:
        Gnosis::ECS* m_ecsCoordinator;
        PlatformDelegates m_delegates;
        
        // Heart texture names for different states
        std::vector<std::string> m_heartTextureNames;
        
        // Screen info for responsive positioning
        ScreenInfo m_screenInfo;
        bool m_screenInfoValid;
        
        // Pre-allocated heart entities (max 9 hearts possible in a level)
        static constexpr int MAX_HEARTS = 9;
        Gnosis::Entity m_heartEntities[MAX_HEARTS];
        int m_allocatedHearts;      // Number of hearts currently allocated
        
    public:
        HeartSystem(Gnosis::ECS* ecsCoordinator, const PlatformDelegates& delegates);
        
        /**
         * Update heart system (called every frame)
         */
        void Update(float deltaTime);
        

        
        /**
         * Initialize player hearts based on difficulty
         */
        void InitializePlayerHearts(Gnosis::Entity playerEntity, GameCore::Difficulty difficulty);
        
        /**
         * Set heart mode (whole, halves, thirds)
         */
        void SetHeartMode(Gnosis::Entity playerEntity, HeartMode mode);
        
        /**
         * Add heart slices (from pickups)
         */
        void AddHeartSlices(Gnosis::Entity playerEntity, int slices);
        
        /**
         * Remove heart slices (from damage)
         */
        void RemoveHeartSlices(Gnosis::Entity playerEntity, int slices);
        
        /**
         * Get current heart slice count
         */
        int GetCurrentSlices(Gnosis::Entity playerEntity) const;
        
        /**
         * Get maximum heart slice count  
         */
        int GetMaxSlices(Gnosis::Entity playerEntity) const;
        
        /**
         * Check if player is dead (no slices left)
         */
        bool IsPlayerDead(Gnosis::Entity playerEntity) const;
        
        /**
         * Create heart UI entity
         */
        Gnosis::Entity CreateHeartUI(float x, float y);
        
        /**
         * Update heart UI positioning (call when screen size changes)
         */
        void UpdateHeartUIPositioning(Gnosis::Entity heartUIEntity, float coinBagX, float menuButtonY);
        
        /**
         * Update heart displays when player health changes
         */
        void UpdateHeartDisplay(Gnosis::Entity playerEntity);
        
        /**
         * Clean up all heart UI entities
         */
        void DestroyAllHeartUI();
        
        /**
         * Update heart count and display based on new difficulty
         */
        void UpdateHeartCountForDifficulty(Gnosis::Entity playerEntity, GameCore::Difficulty difficulty);
        
        /**
         * Update visibility of hearts based on current player state
         */
        void UpdateHeartVisibility(Gnosis::Entity playerEntity);
        
        /**
         * Hide all heart entities (for pause menu)
         */
        void HideAllHearts();
        
        /**
         * Show only the current active heart entities (for resume from pause)
         */
        void ShowAllHearts();
        
    private:
        /**
         * Calculate heart container count for difficulty
         */
        int GetHeartsForDifficulty(GameCore::Difficulty difficulty) const;
        
        /**
         * Get texture name for heart state
         */
        std::string GetHeartTexture(HeartMode mode, int heartIndex, int totalHearts, 
                                   int currentSlices, int maxSlices) const;
        
        /**
         * Pre-allocate all heart entities at specified position (creates MAX_HEARTS entities)
         */
        void CreateAllHeartEntities(float x, float y);
        
        /**
         * Update screen information
         */
        void UpdateScreenInfo();
    };

} // namespace GameCore
