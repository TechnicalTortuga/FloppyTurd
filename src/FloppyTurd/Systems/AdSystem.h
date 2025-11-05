#pragma once

#include "../../Engine/Platform/PlatformDelegates.h"

namespace FloppyTurd {

/**
 * @class AdSystem
 * @brief Manages interstitial ads - tracks player deaths and triggers ads at appropriate intervals
 * 
 * This system implements a learning period (first N deaths = no ads) followed by
 * periodic ad displays (every M deaths). Ads are preloaded for zero-latency presentation.
 * 
 * Configuration:
 * - Learning period: First 3 deaths show no ads (player learns the game)
 * - Ad frequency: Show ad every 5 deaths after learning period
 * 
 * Flow:
 * 1. Game init: Preload first ad
 * 2. Player dies: Increment counter, check if should show ad
 * 3. Show ad if counter >= threshold and past learning period
 * 4. Reset counter after showing ad
 */
class AdSystem {
public:
    AdSystem();
    ~AdSystem();
    
    /**
     * Initialize the ad system
     * @param delegates Platform delegates for ad operations
     */
    void Initialize(GameCore::PlatformDelegates* delegates);
    
    /**
     * Call this when the player dies/game over occurs
     * Increments death counter and triggers ad if threshold is met
     */
    void OnPlayerDeath();
    
    /**
     * Reset the death counter (e.g., after showing an ad)
     */
    void ResetCounter();
    
    /**
     * Get the current death count since last ad
     */
    int GetDeathCount() const { return m_deathCountSinceLastAd; }
    
    /**
     * Get the total death count (lifetime)
     */
    int GetTotalDeathCount() const { return m_totalDeathCount; }
    
    /**
     * Check if we're still in the learning period (no ads shown yet)
     */
    bool IsInLearningPeriod() const { return m_totalDeathCount < m_learningPeriodDeaths; }
    
    /**
     * Set the learning period threshold (default: 3 deaths)
     */
    void SetLearningPeriod(int deaths) { m_learningPeriodDeaths = deaths; }
    
    /**
     * Set the ad frequency (default: every 5 deaths)
     */
    void SetAdFrequency(int deaths) { m_adFrequencyDeaths = deaths; }
    
    /**
     * Manually trigger ad preloading (called automatically after showing ad)
     */
    void PreloadNextAd();
    
    /**
     * Check if an ad is ready to be shown
     */
    bool IsAdReady() const;
    
private:
    GameCore::PlatformDelegates* m_platformDelegates;
    
    // Death counters
    int m_deathCountSinceLastAd;    // Deaths since last ad shown
    int m_totalDeathCount;           // Total deaths in current session
    
    // Configuration
    int m_learningPeriodDeaths;      // Number of deaths before first ad (default: 3)
    int m_adFrequencyDeaths;         // Show ad every N deaths (default: 5)
    
    // Ad state
    bool m_isInitialized;
    bool m_adPreloaded;              // Whether an ad has been preloaded
    
    /**
     * Show an interstitial ad
     */
    void ShowAd();
    
    /**
     * Check if we should show an ad based on current counters
     */
    bool ShouldShowAd() const;
};

} // namespace FloppyTurd