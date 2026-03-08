# AdMob Integration Plan for FloppyTurd (SIMPLIFIED)

**Date:** November 2, 2025  
**Status:** Ready for Implementation  
**Architecture:** Aligned with existing ThreadingProxy/CommandProcessor system  
**Ad Types:** Interstitial only (no rewarded videos)

---

## Executive Summary

This plan integrates Google AdMob interstitial ads into FloppyTurd using the existing threading architecture. Ads are **preloaded and cached** to eliminate latency. All ad functionality flows through PlatformDelegates → ThreadingProxy → CommandProcessor → Swift.

---

## AdMob Account Setup ✅ COMPLETE

You have your AdMob account and Ad Unit IDs ready. During development, we'll use **test ad unit IDs** to avoid any account issues.

**Test Ad Unit ID (Interstitial):**
```
ca-app-pub-3940256099942544/4411468910
```

When ready for beta/release, you'll replace this with your real Ad Unit ID.

---

## Ad Strategy

### Frequency
- **First 3 game overs:** No ads (learning period)
- **After 3rd death:** Show ad every 5 game overs
- Configurable in code (easy to adjust based on testing)

### Preloading Strategy
- ✅ Load first ad during app initialization
- ✅ Load next ad immediately after showing one
- ✅ Always have ad ready (zero latency)
- ✅ Ad cached in memory until shown

### Performance Considerations
- Ads preload in background (non-blocking)
- Memory footprint: ~2-5 MB per cached ad
- No impact on gameplay performance
- If ad not ready, skip and load for next time

---

## Architecture Overview

### Existing System Pattern (Reference)
```
C++ Game Logic
    ↓
PlatformDelegates::GameCenterDelegate
    ↓
ThreadingProxy::enqueueGameCenterCommand()
    ↓
CommandProcessor::processCommands()
    ↓
GameCenterManager.swift (Swift)
```

### New Ad System (Same Pattern)
```
C++ Game Logic
    ↓
PlatformDelegates::AdDelegate (NEW)
    ↓
ThreadingProxy::enqueueAdCommand() (NEW)
    ↓
CommandProcessor::processCommands() (UPDATED)
    ↓
AdManager.swift (NEW)
```

---

## Implementation Steps

### Phase 1: C++ Platform Delegates (15 min)

#### File: `src/Engine/Platform/PlatformDelegates.h`

**1.1 Add AdDelegate struct** (after GameCenterDelegate, around line 758)

```cpp
// Ad delegate - iOS AdMob interstitial advertising
struct AdDelegate {
    // Initialization
    void (*initialize)();
    bool (*isInitialized)();
    
    // Interstitial ads (full-screen after game over)
    void (*preloadInterstitialAd)();      // Load ad in advance (called at startup)
    bool (*isInterstitialReady)();        // Check if ad is cached and ready
    void (*showInterstitialAd)();         // Show cached ad, preload next one
    
    // Ad removal (IAP)
    bool (*areAdsRemoved)();
    void (*setAdsRemoved)(bool removed);
    
    // Initialize to null
    AdDelegate()
        : initialize(nullptr)
        , isInitialized(nullptr)
        , preloadInterstitialAd(nullptr)
        , isInterstitialReady(nullptr)
        , showInterstitialAd(nullptr)
        , areAdsRemoved(nullptr)
        , setAdsRemoved(nullptr)
    {}
};
```

**1.2 Add AdDelegate to PlatformDelegates container** (around line 771)

```cpp
struct PlatformDelegates {
    RendererDelegate renderer;
    InputDelegate input;
    AudioDelegate audio;
    AssetDelegate asset;
    LogDelegate log;
    HapticDelegate haptic;
    SaveGameDelegate save;
    GameCenterDelegate gameCenter;
    AdDelegate ad;  // ADD THIS LINE
    
    // Platform identification
    std::string platform;
    
    PlatformDelegates() : platform("Unknown") {}
};
```

---

### Phase 2: Threading Commands (30 min)

#### File: `src/Engine/Platform/PlatformDelegates.h`

**2.1 Add AdCommand enum values to CommandType** (around line 180)

```cpp
enum class CommandType : uint32_t {
    // ... existing commands ...
    
    // Game Center commands
    CMD_GAME_CENTER_AUTHENTICATE,
    CMD_GAME_CENTER_SUBMIT_SCORE,
    CMD_GAME_CENTER_SHOW_LEADERBOARD,
    CMD_GAME_CENTER_SHOW_ALL_LEADERBOARDS,
    
    // Ad commands (NEW - Interstitial only)
    CMD_AD_INITIALIZE,
    CMD_AD_PRELOAD_INTERSTITIAL,
    CMD_AD_SHOW_INTERSTITIAL,
    CMD_AD_SET_REMOVED,
};
```

**2.2 Add AdCommand structure** (after GameCenterCommand, around line 420)

```cpp
// Ad command structure for threading (Interstitial only)
struct AdCommand {
    CommandType type;
    
    struct AdCommandData {
        bool adsRemoved = false;
        
        AdCommandData() = default;
    } data;
    
    AdCommand(CommandType t) : type(t) {}
};
```

#### File: `src/iOS/Threading/ThreadingProxy.h`

**2.3 Add ad command queue to private members** (around line 235)

```cpp
private:
    std::mutex m_queueMutex;
    std::vector<RenderCommand> m_renderCommandQueue;
    std::vector<AudioCommand> m_audioCommandQueue;
    std::vector<LogCommand> m_logCommandQueue;
    std::vector<AssetCommand> m_assetCommandQueue;
    std::vector<HapticCommand> m_hapticCommandQueue;
    std::vector<SaveCommand> m_saveCommandQueue;
    std::vector<GameCenterCommand> m_gameCenterCommandQueue;
    std::vector<AdCommand> m_adCommandQueue;  // ADD THIS LINE
```

**2.4 Add private enqueue method**

```cpp
void enqueueAdCommand(const AdCommand& command);
```

**2.5 Add public static methods** (after GameCenter methods, around line 118)

```cpp
// Ad commands (Interstitial only)
static void enqueueAdInitialize();
static void enqueueAdPreloadInterstitial();
static void enqueueAdShowInterstitial();
static void enqueueAdSetRemoved(bool removed);
```

#### File: `src/iOS/Threading/ThreadingProxy.cpp`

**2.6 Implement ThreadingProxy ad methods** (after GameCenter implementations, around line 610)

```cpp
// ========================================================================
// Ad Command Implementations (Interstitial only)
// ========================================================================

void ThreadingProxy::enqueueAdCommand(const AdCommand& command) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_adCommandQueue.push_back(command);
}

void ThreadingProxy::enqueueAdInitialize() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_INITIALIZE);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdPreloadInterstitial() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_PRELOAD_INTERSTITIAL);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdShowInterstitial() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_SHOW_INTERSTITIAL);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdSetRemoved(bool removed) {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_SET_REMOVED);
    cmd.data.adsRemoved = removed;
    s_instance->enqueueAdCommand(cmd);
}
```

**2.7 Add getter for ad commands** (after getAndClearGameCenterCommandsFromProxy, around line 735)

```cpp
// Get and clear ad commands for Swift
std::vector<AdCommand> getAndClearAdCommandsFromProxy() {
    if (!g_threadingProxy) return {};
    
    std::lock_guard<std::mutex> lock(g_threadingProxy->m_queueMutex);
    std::vector<AdCommand> commands = g_threadingProxy->m_adCommandQueue;
    g_threadingProxy->m_adCommandQueue.clear();
    return commands;
}
```

**2.8 Update clearQueue()** (around line 30)

```cpp
void ThreadingProxy::clearQueue() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_renderCommandQueue.clear();
    m_audioCommandQueue.clear();
    m_logCommandQueue.clear();
    m_assetCommandQueue.clear();
    m_hapticCommandQueue.clear();
    m_saveCommandQueue.clear();
    m_gameCenterCommandQueue.clear();
    m_adCommandQueue.clear();  // ADD THIS LINE
}
```

**2.9 Update hasCommands()** (around line 43)

```cpp
bool ThreadingProxy::hasCommands() {
    if (!s_instance) return false;
    std::lock_guard<std::mutex> lock(s_instance->m_queueMutex);
    return !s_instance->m_renderCommandQueue.empty() || 
           !s_instance->m_audioCommandQueue.empty() || 
           !s_instance->m_logCommandQueue.empty() ||
           !s_instance->m_assetCommandQueue.empty() ||
           !s_instance->m_gameCenterCommandQueue.empty() ||
           !s_instance->m_adCommandQueue.empty();  // ADD THIS LINE
}
```

---

### Phase 3: Swift AdManager (45 min)

#### Create new directory and file

**Directory:** `src/iOS/Advertising/` (create if doesn't exist)

**File:** `src/iOS/Advertising/AdManager.swift` (NEW FILE)

```swift
//
//  AdManager.swift
//  FloppyTurd
//
//  AdMob advertising manager - Interstitial ads only
//  Integrated with ThreadingProxy command system
//  Ads are preloaded and cached for zero-latency display
//

import Foundation
import GoogleMobileAds
import UIKit
import os.log

@MainActor
final class AdManager: NSObject {
    
    // MARK: - Singleton
    
    static let shared = AdManager()
    
    // MARK: - Properties
    
    private let logger = Logger(subsystem: "com.floppyturd.game", category: "AdManager")
    
    private var interstitialAd: GADInterstitialAd?
    private var isLoadingAd = false
    
    private var isInitialized = false
    private var adsRemoved = false
    
    // TEST Ad Unit ID - Use this during development
    // Replace with your real Ad Unit ID before TestFlight/release
    private let interstitialAdUnitID = "ca-app-pub-3940256099942544/4411468910" // TEST ID
    
    // PRODUCTION Ad Unit ID (commented out for now)
    // private let interstitialAdUnitID = "ca-app-pub-YOUR-REAL-ID/1234567890"
    
    // MARK: - Initialization
    
    override private init() {
        super.init()
    }
    
    func initialize() {
        guard !isInitialized else {
            logger.info("AdMob already initialized")
            return
        }
        
        logger.info("🎬 Initializing AdMob SDK...")
        
        // Configure for family-friendly content
        let requestConfiguration = GADMobileAds.sharedInstance().requestConfiguration
        requestConfiguration.tagForChildDirectedTreatment = true
        requestConfiguration.maxAdContentRating = .general
        
        // Start SDK
        GADMobileAds.sharedInstance().start { [weak self] status in
            guard let self = self else { return }
            
            self.logger.info("✅ AdMob SDK initialized successfully")
            self.isInitialized = true
            
            // Automatically preload first ad
            self.logger.info("📦 Preloading first interstitial ad...")
            self.preloadInterstitialAd()
        }
    }
    
    func getIsInitialized() -> Bool {
        return isInitialized
    }
    
    // MARK: - Interstitial Ads (Preloaded & Cached)
    
    func preloadInterstitialAd() {
        guard isInitialized else {
            logger.warning("⚠️ Cannot preload ad - AdMob not initialized")
            return
        }
        
        guard !isLoadingAd else {
            logger.info("ℹ️ Ad already loading, skipping preload request")
            return
        }
        
        // If we already have an ad cached, don't load another
        if interstitialAd != nil {
            logger.info("✅ Interstitial ad already cached and ready")
            return
        }
        
        isLoadingAd = true
        logger.info("📡 Loading interstitial ad...")
        
        let request = GADRequest()
        
        GADInterstitialAd.load(withAdUnitID: interstitialAdUnitID, request: request) { [weak self] ad, error in
            guard let self = self else { return }
            
            self.isLoadingAd = false
            
            if let error = error {
                self.logger.error("❌ Failed to load interstitial ad: \(error.localizedDescription)")
                return
            }
            
            self.interstitialAd = ad
            self.interstitialAd?.fullScreenContentDelegate = self
            self.logger.info("✅ Interstitial ad loaded and cached (ready to show)")
        }
    }
    
    func isInterstitialReady() -> Bool {
        let ready = interstitialAd != nil && !adsRemoved
        if !ready {
            logger.debug("🔍 Interstitial ready check: ad=\(self.interstitialAd != nil), removed=\(self.adsRemoved)")
        }
        return ready
    }
    
    func showInterstitialAd() {
        guard !adsRemoved else {
            logger.info("🚫 Ads removed by user, skipping interstitial")
            return
        }
        
        guard let rootViewController = UIApplication.shared.windows.first?.rootViewController else {
            logger.error("❌ No root view controller found")
            return
        }
        
        guard let interstitialAd = interstitialAd else {
            logger.warning("⚠️ Interstitial ad not ready (not cached)")
            // Preload for next time
            preloadInterstitialAd()
            return
        }
        
        logger.info("🎬 Showing interstitial ad")
        interstitialAd.present(fromRootViewController: rootViewController)
    }
    
    // MARK: - Ad Removal (IAP)
    
    func setAdsRemoved(_ removed: Bool) {
        adsRemoved = removed
        logger.info("💰 Ads removed status: \(removed)")
        
        // Clear cached ad if ads removed
        if removed {
            interstitialAd = nil
            logger.info("🗑️ Cleared cached interstitial ad")
        } else {
            // User re-enabled ads (unlikely but possible), preload one
            preloadInterstitialAd()
        }
    }
    
    func areAdsRemoved() -> Bool {
        return adsRemoved
    }
}

// MARK: - GADFullScreenContentDelegate

extension AdManager: GADFullScreenContentDelegate {
    
    func adDidRecordImpression(_ ad: GADFullScreenPresentingAd) {
        logger.info("📊 Ad impression recorded")
    }
    
    func adDidRecordClick(_ ad: GADFullScreenPresentingAd) {
        logger.info("👆 Ad clicked by user")
    }
    
    func adWillPresentFullScreenContent(_ ad: GADFullScreenPresentingAd) {
        logger.info("📺 Ad will present full screen")
    }
    
    func adDidDismissFullScreenContent(_ ad: GADFullScreenPresentingAd) {
        logger.info("✅ Ad dismissed by user")
        
        // Clear the shown ad
        interstitialAd = nil
        
        // Immediately preload next ad for zero-latency next time
        logger.info("📦 Preloading next interstitial ad...")
        preloadInterstitialAd()
    }
    
    func ad(_ ad: GADFullScreenPresentingAd, didFailToPresentFullScreenContentWithError error: Error) {
        logger.error("❌ Ad failed to present: \(error.localizedDescription)")
        
        // Clear failed ad
        interstitialAd = nil
        
        // Try to preload a new one
        preloadInterstitialAd()
    }
}
```

---

### Phase 4: Swift CommandProcessor Integration (20 min)

#### File: `src/iOS/Threading/ThreadingSystem.swift`

**4.1 Get ad commands in processCommands()** (around line 99, with other getAndClear calls)

```swift
let adCommands = GameCorePlatform.GameCore.getAndClearAdCommandsFromProxy()
```

**4.2 Process ad commands in loop** (around line 128, with other processing loops)

```swift
for adCommand in adCommands {
    executeAdCommand(adCommand)
}
```

**4.3 Add executeAdCommand method** (add after executeGameCenterCommand, around line 905)

```swift
// MARK: - Ad Command Execution

private func executeAdCommand(_ command: GameCorePlatform.GameCore.AdCommand) {
    let commandType = command.type
    
    // Process commands directly - AdManager is @MainActor
    switch commandType {
    case .CMD_AD_INITIALIZE:
        AdManager.shared.initialize()
        log("[CommandProcessor] 🎬 AdMob initialization started", level: .info)
        
    case .CMD_AD_PRELOAD_INTERSTITIAL:
        AdManager.shared.preloadInterstitialAd()
        log("[CommandProcessor] 📦 Preloading interstitial ad", level: .info)
        
    case .CMD_AD_SHOW_INTERSTITIAL:
        if AdManager.shared.isInterstitialReady() {
            AdManager.shared.showInterstitialAd()
            log("[CommandProcessor] 🎬 Showing interstitial ad", level: .info)
        } else {
            log("[CommandProcessor] ⚠️ Interstitial ad not ready, preloading for next time", level: .warning)
            AdManager.shared.preloadInterstitialAd()
        }
        
    case .CMD_AD_SET_REMOVED:
        let removed = command.data.adsRemoved
        AdManager.shared.setAdsRemoved(removed)
        log("[CommandProcessor] 💰 Ads removed status: \(removed)", level: .info)
        
    default:
        log("[CommandProcessor] ⚠️ Unknown ad command: \(commandType)", level: .warning)
    }
}
```

---

### Phase 5: C++ Game Logic Integration (30 min)

#### Create AdCounterSystem

**File:** `src/FloppyTurd/Systems/AdCounterSystem.h` (NEW FILE)

```cpp
#pragma once

namespace FloppyTurd {

/**
 * @class AdCounterSystem
 * @brief Tracks game overs and determines when to show interstitial ads
 * 
 * Strategy:
 * - No ads for first 3 game overs (learning period)
 * - Show ad every 5 game overs after that
 * - Respects "Remove Ads" IAP purchase
 */
class AdCounterSystem {
public:
    AdCounterSystem();
    ~AdCounterSystem() = default;
    
    // Game over tracking
    void OnGameOver();
    bool ShouldShowAd() const;
    void ResetCounter();
    
    // Configuration
    void SetAdFrequency(int frequency); // e.g., 5 = show ad every 5 game overs
    int GetGameOverCount() const { return m_gameOverCount; }
    int GetAdFrequency() const { return m_adFrequency; }
    
    // Ad removal (IAP)
    void SetAdsDisabled(bool disabled);
    bool AreAdsDisabled() const { return m_adsDisabled; }
    
private:
    int m_gameOverCount;
    int m_adFrequency;        // Show ad every N game overs
    int m_minGameOversBeforeAds; // Don't show ads for first N deaths (learning period)
    bool m_adsDisabled;       // User purchased "Remove Ads"
};

} // namespace FloppyTurd
```

**File:** `src/FloppyTurd/Systems/AdCounterSystem.cpp` (NEW FILE)

```cpp
#include "AdCounterSystem.h"
#include "../../Engine/Core/GNLog.h"

namespace FloppyTurd {

AdCounterSystem::AdCounterSystem()
    : m_gameOverCount(0)
    , m_adFrequency(5)           // Default: ad every 5 game overs
    , m_minGameOversBeforeAds(3) // No ads for first 3 deaths (learning period)
    , m_adsDisabled(false)
{
    GN_LOG_INFO("AdCounterSystem initialized - frequency: " + std::to_string(m_adFrequency) + 
                ", learning period: " + std::to_string(m_minGameOversBeforeAds) + " deaths");
}

void AdCounterSystem::OnGameOver() {
    if (m_adsDisabled) {
        return; // Don't count if ads are disabled
    }
    
    m_gameOverCount++;
    GN_LOG_DEBUG("Game over count: " + std::to_string(m_gameOverCount));
}

bool AdCounterSystem::ShouldShowAd() const {
    // Don't show if ads disabled
    if (m_adsDisabled) {
        GN_LOG_DEBUG("Ads disabled, not showing");
        return false;
    }
    
    // Don't show for first N game overs (learning period)
    if (m_gameOverCount < m_minGameOversBeforeAds) {
        GN_LOG_DEBUG("Learning period (death " + std::to_string(m_gameOverCount) + 
                    " of " + std::to_string(m_minGameOversBeforeAds) + "), no ad");
        return false;
    }
    
    // Show ad every N game overs
    bool shouldShow = (m_gameOverCount % m_adFrequency) == 0;
    
    if (shouldShow) {
        GN_LOG_INFO("🎬 Ad trigger: death #" + std::to_string(m_gameOverCount) + 
                   " (every " + std::to_string(m_adFrequency) + " deaths)");
    }
    
    return shouldShow;
}

void AdCounterSystem::ResetCounter() {
    m_gameOverCount = 0;
    GN_LOG_INFO("Ad counter reset");
}

void AdCounterSystem::SetAdFrequency(int frequency) {
    if (frequency < 1) frequency = 1; // Sanity check
    m_adFrequency = frequency;
    GN_LOG_INFO("Ad frequency updated: every " + std::to_string(m_adFrequency) + " deaths");
}

void AdCounterSystem::SetAdsDisabled(bool disabled) {
    m_adsDisabled = disabled;
    GN_LOG_INFO("Ads " + std::string(disabled ? "DISABLED" : "ENABLED") + " (IAP purchase)");
}

} // namespace FloppyTurd
```

#### Update FloppyTurdGame

**File:** `src/FloppyTurd/Game/FloppyTurdGame.h`

**Add includes** (at top with other includes)

```cpp
#include "../Systems/AdCounterSystem.h"
```

**Add member variable** (in private section, around line 120)

```cpp
private:
    // ... existing members ...
    std::unique_ptr<AdCounterSystem> m_adCounter;
```

**Add public method declaration** (in public section, around line 60)

```cpp
public:
    // ... existing methods ...
    
    // Ad management
    void TriggerGameOverAd();
    AdCounterSystem* GetAdCounter() { return m_adCounter.get(); }
```

**File:** `src/FloppyTurd/Game/FloppyTurdGame.cpp`

**Initialize in constructor** (around line 50)

```cpp
FloppyTurdGame::FloppyTurdGame(const PlatformDelegates& delegates)
    : m_platformDelegates(delegates)
    , m_currentState(nullptr)
    // ... other initializations ...
    , m_adCounter(std::make_unique<AdCounterSystem>())
{
    GN_LOG_INFO("FloppyTurd game engine initialized");
    
    // Initialize AdMob on startup
    if (m_platformDelegates.ad.initialize) {
        m_platformDelegates.ad.initialize();
        GN_LOG_INFO("🎬 AdMob initialization requested");
        
        // Preload first ad
        if (m_platformDelegates.ad.preloadInterstitialAd) {
            m_platformDelegates.ad.preloadInterstitialAd();
            GN_LOG_INFO("📦 Preloading first interstitial ad");
        }
    }
}
```

**Add TriggerGameOverAd implementation** (add with other methods, around line 300)

```cpp
void FloppyTurdGame::TriggerGameOverAd() {
    if (!m_adCounter) {
        GN_LOG_WARNING("AdCounter not initialized");
        return;
    }
    
    // Increment death counter
    m_adCounter->OnGameOver();
    
    // Check if we should show ad
    if (m_adCounter->ShouldShowAd()) {
        GN_LOG_INFO("🎬 Triggering interstitial ad (death #" + 
                   std::to_string(m_adCounter->GetGameOverCount()) + ")");
        
        // Check if ad is ready (preloaded and cached)
        if (m_platformDelegates.ad.isInterstitialReady && 
            m_platformDelegates.ad.isInterstitialReady()) {
            
            // Show ad (it's already cached, zero latency)
            if (m_platformDelegates.ad.showInterstitialAd) {
                m_platformDelegates.ad.showInterstitialAd();
                GN_LOG_INFO("✅ Showing cached interstitial ad");
            }
        } else {
            GN_LOG_DEBUG("⚠️ Interstitial ad not ready, will try next time");
            
            // Preload for next time
            if (m_platformDelegates.ad.preloadInterstitialAd) {
                m_platformDelegates.ad.preloadInterstitialAd();
            }
        }
    }
}
```

#### Update GameOverState

**File:** `src/FloppyTurd/States/GameOverState.cpp`

**Call ad trigger in Enter()** (around line 40, after existing game over logic)

```cpp
void GameOverState::Enter() {
    GN_LOG_INFO("Entering Game Over State");
    
    // ... existing game over logic ...
    
    // Trigger ad check (will show if counter threshold met)
    if (m_game) {
        m_game->TriggerGameOverAd();
    }
}
```

---

### Phase 6: iOS Project Configuration (20 min)

#### 6.1 Add AdMob SDK

**Option A: Swift Package Manager (Recommended)**

1. Open Xcode
2. File → Add Package Dependencies
3. Enter URL: `https://github.com/googleads/swift-package-manager-google-mobile-ads.git`
4. Select "Up to Next Major Version"
5. Click "Add Package"

**Option B: CocoaPods (if you use it)**

Add to `Podfile`:
```ruby
pod 'Google-Mobile-Ads-SDK'
```

Run:
```bash
pod install
```

#### 6.2 Update Info.plist

**File:** `FloppyTurd/Info.plist`

**Add before closing `</dict>`:**

```xml
<!-- AdMob Configuration -->
<key>GADApplicationIdentifier</key>
<string>ca-app-pub-3940256099942544~1458002511</string>

<!-- TEST App ID - Replace with your real App ID before beta/release -->
<!-- <string>ca-app-pub-YOUR-REAL-APP-ID~1234567890</string> -->

<!-- For child-directed treatment (family-friendly) -->
<key>GADIsForChildDirectedTreatment</key>
<true/>

<!-- SKAdNetwork IDs (required for iOS 14+ ad attribution) -->
<key>SKAdNetworkItems</key>
<array>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>cstr6suwn9.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>4fzdc2evr5.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>2fnua5tdw4.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>ydx93a7ass.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>v72qych5uu.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>ludvb6z3bs.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>mlmmfzh3r3.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>c6k4g5qg8m.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>wg9ef5tyq4.skadnetwork</string>
  </dict>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>hs6bdukanm.skadnetwork</string>
  </dict>
</array>
```

**Note:** When ready for beta/release, replace the GADApplicationIdentifier with your real App ID from AdMob console.

#### 6.3 Update CMakeLists.txt (if needed)

**File:** `CMakeLists.txt`

AdCounterSystem should be auto-included if you have this pattern (around line 105):

```cmake
file(GLOB_RECURSE FLOPPYTURD_CPP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems/*.cpp"  # Includes AdCounterSystem
    # ... rest of sources
)
```

If AdCounterSystem is not being compiled, add it explicitly:

```cmake
set(FLOPPYTURD_CPP_SOURCES
    # ... existing sources ...
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems/AdCounterSystem.cpp"
)
```

---

### Phase 7: Platform Delegate Setup (15 min)

#### File: `src/iOS/GameViewController.swift`

**Find setupPlatformDelegates() method** (around line 200)

**Add after GameCenter delegate setup:**

```swift
// ========================================================================
// Ad Delegate Setup (Interstitial only)
// ========================================================================

delegates.ad.initialize = {
    ThreadingProxy.enqueueAdInitialize()
}

delegates.ad.isInitialized = {
    return AdManager.shared.getIsInitialized()
}

delegates.ad.preloadInterstitialAd = {
    ThreadingProxy.enqueueAdPreloadInterstitial()
}

delegates.ad.isInterstitialReady = {
    return AdManager.shared.isInterstitialReady()
}

delegates.ad.showInterstitialAd = {
    ThreadingProxy.enqueueAdShowInterstitial()
}

delegates.ad.areAdsRemoved = {
    return AdManager.shared.areAdsRemoved()
}

delegates.ad.setAdsRemoved = { removed in
    ThreadingProxy.enqueueAdSetRemoved(removed)
}
```

---

## Testing Plan

### Phase 1: Local Development Testing (Test Ads)

**1. Build and Run**
```bash
cd /Users/aimac/Development/FloppyTurd
rm -rf build_ios
mkdir build_ios && cd build_ios
cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios-cmake-master/ios.toolchain.cmake -DPLATFORM=SIMULATOR64 ..
```

Then build in Xcode:
```
Product → Build (Cmd+B)
Product → Run (Cmd+R)
```

**2. Verify Initialization**
- Launch app
- Check Xcode console for: `"🎬 Initializing AdMob SDK..."`
- Check for: `"✅ AdMob SDK initialized successfully"`
- Check for: `"📦 Preloading first interstitial ad..."`
- Check for: `"✅ Interstitial ad loaded and cached"`

**3. Test Ad Frequency**
- Die 1st time → No ad (learning period)
- Die 2nd time → No ad (learning period)
- Die 3rd time → No ad (learning period)
- Die 4th time → No ad (counter = 4, need 5)
- Die 5th time → **AD SHOWS** ✅
- Close ad → Check for: `"📦 Preloading next interstitial ad..."`
- Die 10th time → **AD SHOWS** ✅

**4. Test Preloading**
- After showing ad, check console for immediate preload
- Next time ad is triggered, should show instantly (zero latency)

**5. Test Ad Not Ready Scenario**
- Kill app mid-session
- Restart and immediately die 5 times
- If ad not loaded yet, should skip gracefully and preload

### Phase 2: Performance Testing

**Monitor for:**
- Frame rate during gameplay (should be unchanged)
- Memory usage (expect +2-5 MB for cached ad)
- App startup time (AdMob initializes in background)
- Ad display latency (should be instant if preloaded)

**Check Xcode Memory Graph:**
- Product → Profile
- Select "Leaks" instrument
- Play game, show ads, verify no leaks

### Phase 3: Beta Testing (Real Ads)

**When ready for TestFlight:**

1. **Replace Ad Unit IDs** in `AdManager.swift`:
```swift
// Comment out test ID
// private let interstitialAdUnitID = "ca-app-pub-3940256099942544/4411468910"

// Use your real ID
private let interstitialAdUnitID = "ca-app-pub-YOUR-REAL-ID/1234567890"
```

2. **Update App ID** in `Info.plist`:
```xml
<key>GADApplicationIdentifier</key>
<string>ca-app-pub-YOUR-REAL-APP-ID~1234567890</string>
```

3. **Archive and Upload**:
- Product → Archive
- Upload to App Store Connect
- Add external testers
- They see real ads, you earn real revenue!

4. **Monitor AdMob Dashboard**:
- Go to: https://apps.admob.com/
- Check "Reports" → "Ad units"
- Monitor: Impressions, eCPM, Fill Rate, Revenue

---

## Troubleshooting

### Ad Not Showing

**Check these in order:**

1. **Is AdMob initialized?**
   - Look for: `"✅ AdMob SDK initialized successfully"`
   - If not, check Info.plist has GADApplicationIdentifier

2. **Is ad preloaded?**
   - Look for: `"✅ Interstitial ad loaded and cached"`
   - If "❌ Failed to load", check internet connection
   - Check Ad Unit ID is correct

3. **Is ad ready when triggered?**
   - Look for: `"✅ Showing cached interstitial ad"`
   - If "⚠️ not ready", ad still loading (will work next time)

4. **Did you hit the threshold?**
   - Check: `"🎬 Ad trigger: death #5"`
   - First 3 deaths = no ads (learning period)
   - Then every 5 deaths

5. **Are ads disabled?**
   - Check: Not `"🚫 Ads removed by user"`

### Build Errors

**"Cannot find GoogleMobileAds"**
- Solution: Add SDK via Swift Package Manager (see Phase 6.1)

**"Module 'GameCorePlatform' not found"**
- Solution: Clean build folder (Cmd+Shift+K), rebuild

**"AdCounterSystem.cpp not compiled"**
- Solution: Check CMakeLists.txt includes Systems directory (see Phase 6.3)

**Linker errors about AdDelegate**
- Solution: Make sure you regenerated Xcode project with cmake

### Runtime Errors

**"No root view controller found"**
- Solution: Ad showing too early, iOS not fully loaded
- Fix: Wait for app launch to complete

**"Ad failed to present: no fill"**
- With test IDs: Should never happen
- With real IDs: Low fill rate, try again later
- Solution: AdMob will skip and preload next

---

## Performance Benchmarks

### Expected Metrics

**Memory:**
- Base app: ~120 MB
- With cached ad: ~125 MB (+5 MB)
- After showing ad: ~122 MB (slight overhead remains)

**Frame Rate:**
- Should be unchanged (60 FPS target)
- Ad loading happens in background thread
- Ad showing pauses game (expected)

**Network:**
- Initial ad load: ~500 KB - 2 MB
- Subsequent ads: Similar per ad
- Cached ad: No network needed to show

**Startup Time:**
- AdMob init: ~200-500 ms (async, non-blocking)
- First ad preload: 1-3 seconds (background)
- No impact on gameplay start

---

## Revenue Estimates (Interstitial Only)

### Conservative (1000 DAU)

```
1000 users/day
× 3 sessions/user/day
× 2 deaths/session average
= 6000 deaths/day

6000 deaths ÷ 5 (ad frequency) = 1200 ad impressions/day
1200 impressions × $4 CPM = $4.80/day
× 30 days = $144/month
```

### Realistic Growth (5000 DAU)

```
5000 users/day
× 3 sessions/user/day
× 2 deaths/session
= 30,000 deaths/day

30,000 ÷ 5 = 6000 ad impressions/day
6000 impressions × $5 CPM = $30/day
× 30 days = $900/month
```

### With Optimizations

If you adjust ad frequency to every 3 deaths (test retention first!):

```
5000 DAU × 6 deaths/day ÷ 3 = 10,000 impressions/day
10,000 × $5 CPM = $50/day = $1,500/month
```

**Important:** Monitor Day 1 retention. If it drops >5%, reduce ad frequency.

---

## Configuration Tuning

### Adjust Ad Frequency

**File:** `src/FloppyTurd/Systems/AdCounterSystem.cpp`

**Change constructor** (line 10):

```cpp
AdCounterSystem::AdCounterSystem()
    : m_gameOverCount(0)
    , m_adFrequency(3)           // Change from 5 to 3 for more ads
    , m_minGameOversBeforeAds(3) // Keep learning period same
    , m_adsDisabled(false)
```

**Or set at runtime:**

```cpp
// In FloppyTurdGame constructor
m_adCounter->SetAdFrequency(3); // Show ad every 3 deaths
```

### Change Learning Period

```cpp
AdCounterSystem::AdCounterSystem()
    : m_gameOverCount(0)
    , m_adFrequency(5)
    , m_minGameOversBeforeAds(5) // Change from 3 to 5 (no ads first 5 deaths)
    , m_adsDisabled(false)
```

---

## File Checklist

### New Files to Create

- [x] `src/iOS/Advertising/AdManager.swift`
- [x] `src/FloppyTurd/Systems/AdCounterSystem.h`
- [x] `src/FloppyTurd/Systems/AdCounterSystem.cpp`

### Files to Modify

- [x] `src/Engine/Platform/PlatformDelegates.h` (AdDelegate, CommandType, AdCommand)
- [x] `src/iOS/Threading/ThreadingProxy.h` (ad queue, methods)
- [x] `src/iOS/Threading/ThreadingProxy.cpp` (ad implementations)
- [x] `src/iOS/Threading/ThreadingSystem.swift` (executeAdCommand)
- [x] `src/FloppyTurd/Game/FloppyTurdGame.h` (AdCounterSystem member, TriggerGameOverAd)
- [x] `src/FloppyTurd/Game/FloppyTurdGame.cpp` (initialize, implement TriggerGameOverAd)
- [x] `src/FloppyTurd/States/GameOverState.cpp` (call TriggerGameOverAd)
- [x] `src/iOS/GameViewController.swift` (setup ad delegates)
- [x] `FloppyTurd/Info.plist` (AdMob App ID, SKAdNetwork IDs)
- [x] `CMakeLists.txt` (verify AdCounterSystem included)

### Xcode Changes

- [x] Add GoogleMobileAds SDK (Swift Package Manager)
- [x] Add `src/iOS/Advertising/` directory to Xcode project
- [x] Verify AdManager.swift in build target

---

## Summary

**Implementation Time:** ~2.5 hours total

**What You Get:**
- ✅ Interstitial ads only (clean, simple)
- ✅ Ads preloaded and cached (zero latency)
- ✅ Non-intrusive frequency (every 5 deaths after 3 death learning period)
- ✅ Follows existing ThreadingProxy architecture
- ✅ Family-safe content filtering
- ✅ Ready for TestFlight beta testing
- ✅ Revenue potential: $144-1500/month

**Next Steps:**
1. Implement phases 1-7 (follow this doc step-by-step)
2. Test with test ad IDs
3. Verify preloading works (check console logs)
4. Test ad frequency (die 5 times, see ad)
5. Replace test IDs with your real AdMob IDs
6. Build for TestFlight
7. Monitor AdMob dashboard
8. Optimize based on retention/revenue data

Let's build this! 🚀💰