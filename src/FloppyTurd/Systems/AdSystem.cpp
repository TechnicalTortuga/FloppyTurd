#include "AdSystem.h"
#include "../../Engine/Core/GNLog.h"

namespace FloppyTurd {

AdSystem::AdSystem()
    : m_platformDelegates(nullptr)
    , m_deathCountSinceLastAd(0)
    , m_totalDeathCount(0)
    , m_learningPeriodDeaths(3)      // First 3 deaths = learning period, no ads
    , m_adFrequencyDeaths(5)          // Show ad every 5 deaths after learning period
    , m_isInitialized(false)
    , m_adPreloaded(false)
{
    GN_LOG_INFO("AdSystem: Initialized with learning period: %d deaths, ad frequency: %d deaths",
                m_learningPeriodDeaths, m_adFrequencyDeaths);
}

AdSystem::~AdSystem() {
    GN_LOG_INFO("AdSystem: Shutting down - Total deaths: %d", m_totalDeathCount);
}

void AdSystem::Initialize(GameCore::PlatformDelegates* delegates) {
    if (!delegates) {
        GN_LOG_ERROR("AdSystem: Cannot initialize with null delegates");
        return;
    }
    
    m_platformDelegates = delegates;
    m_isInitialized = true;
    
    GN_LOG_INFO("AdSystem: Initialized successfully");
    
    // Preload the first ad immediately for instant availability
    PreloadNextAd();
}

void AdSystem::OnPlayerDeath() {
    if (!m_isInitialized) {
        GN_LOG_WARN("AdSystem: OnPlayerDeath called but system not initialized");
        return;
    }
    
    // Increment counters
    m_deathCountSinceLastAd++;
    m_totalDeathCount++;
    
    GN_LOG_INFO("AdSystem: Player died - Death count since last ad: %d, Total: %d",
                m_deathCountSinceLastAd, m_totalDeathCount);
    
    // Check if we're still in learning period
    if (IsInLearningPeriod()) {
        GN_LOG_INFO("AdSystem: Still in learning period (%d/%d deaths) - no ad shown",
                    m_totalDeathCount, m_learningPeriodDeaths);
        return;
    }
    
    // Check if we should show an ad
    if (ShouldShowAd()) {
        GN_LOG_INFO("AdSystem: Death threshold reached (%d/%d) - showing ad",
                    m_deathCountSinceLastAd, m_adFrequencyDeaths);
        ShowAd();
        ResetCounter();
    } else {
        GN_LOG_INFO("AdSystem: Ad threshold not yet reached (%d/%d deaths)",
                    m_deathCountSinceLastAd, m_adFrequencyDeaths);
    }
}

void AdSystem::ResetCounter() {
    m_deathCountSinceLastAd = 0;
    GN_LOG_INFO("AdSystem: Death counter reset");
}

void AdSystem::PreloadNextAd() {
    GN_LOG_INFO("AdSystem::PreloadNextAd() called");
    GN_LOG_INFO("   m_isInitialized: %s", m_isInitialized ? "true" : "false");
    GN_LOG_INFO("   m_platformDelegates: %s", m_platformDelegates ? "valid" : "null");
    
    if (!m_isInitialized || !m_platformDelegates) {
        GN_LOG_WARN("AdSystem: Cannot preload ad - system not initialized");
        return;
    }
    
    if (!m_platformDelegates->ad.preloadAd) {
        GN_LOG_WARN("AdSystem: Ad preload delegate not set");
        return;
    }
    
    GN_LOG_INFO("AdSystem: Calling preloadAd delegate...");
    m_platformDelegates->ad.preloadAd();
    m_adPreloaded = true;
    GN_LOG_INFO("AdSystem: Preload delegate called successfully");
}

bool AdSystem::IsAdReady() const {
    if (!m_isInitialized || !m_platformDelegates) {
        return false;
    }
    
    if (!m_platformDelegates->ad.isAdReady) {
        return false;
    }
    
    return m_platformDelegates->ad.isAdReady();
}

void AdSystem::ShowAd() {
    GN_LOG_INFO("AdSystem::ShowAd() called");
    GN_LOG_INFO("   m_isInitialized: %s", m_isInitialized ? "true" : "false");
    GN_LOG_INFO("   m_platformDelegates: %s", m_platformDelegates ? "valid" : "null");
    
    if (!m_isInitialized || !m_platformDelegates) {
        GN_LOG_WARN("AdSystem: Cannot show ad - system not initialized");
        return;
    }
    
    if (!m_platformDelegates->ad.showAd) {
        GN_LOG_WARN("AdSystem: Ad show delegate not set");
        return;
    }
    
    // Check if ad is ready before showing
    bool adReady = IsAdReady();
    GN_LOG_INFO("   IsAdReady: %s", adReady ? "true" : "false");
    
    if (!adReady) {
        GN_LOG_WARN("AdSystem: Ad not ready to show - preloading for next time");
        PreloadNextAd();
        return;
    }
    
    GN_LOG_INFO("AdSystem: Calling showAd delegate...");
    m_platformDelegates->ad.showAd();
    
    // The ad will auto-preload the next one after dismissal (handled by AdManager.swift)
    // But we mark it as not preloaded here for tracking
    m_adPreloaded = false;
    GN_LOG_INFO("AdSystem: Show ad delegate called successfully");
}

bool AdSystem::ShouldShowAd() const {
    // Don't show during learning period
    if (IsInLearningPeriod()) {
        return false;
    }
    
    // Show if we've reached the frequency threshold
    return m_deathCountSinceLastAd >= m_adFrequencyDeaths;
}

void AdSystem::LoadCountersFromGameStats(int deathsSinceLastAd, int totalDeaths) {
    m_deathCountSinceLastAd = deathsSinceLastAd;
    m_totalDeathCount = totalDeaths;
    GN_LOG_INFO("AdSystem: Loaded counters from save - Deaths since last ad: %d, Total: %d",
                m_deathCountSinceLastAd, m_totalDeathCount);
}

void AdSystem::GetCountersForSave(int& outDeathsSinceLastAd, int& outTotalDeaths) const {
    outDeathsSinceLastAd = m_deathCountSinceLastAd;
    outTotalDeaths = m_totalDeathCount;
}

} // namespace FloppyTurd