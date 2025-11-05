# AdMob Integration Plan for FloppyTurd

**Date:** November 2, 2025  
**Status:** Ready for Implementation  
**Architecture:** Aligned with existing ThreadingProxy/CommandProcessor system

---

## Executive Summary

This plan integrates Google AdMob into FloppyTurd using the existing threading architecture. No separate delegates - all ad functionality flows through PlatformDelegates → ThreadingProxy → CommandProcessor → Swift, matching the established patterns for GameCenter, Audio, Haptics, etc.

---

## Pre-Implementation Questions Answered

### Q: Does the app need to be deployed first?
**A: No.** You can set up AdMob and test with test ad units before any deployment.

**Steps:**
1. Create AdMob account (tonight)
2. Register app in AdMob console (even if not published yet)
3. Get Ad Unit IDs (test IDs work immediately)
4. Develop with test ads (no App Store required)
5. Switch to real Ad Unit IDs before beta/release

### Q: Is there beta testing middle ground?
**A: Yes - TestFlight.** This is exactly what you want.

**Beta Testing Flow:**
```
Development → TestFlight Beta → App Store Release
    ↓              ↓                    ↓
Test Ads    Real Ads (Beta)      Real Ads (Public)
AdMob ✓     AdMob ✓              AdMob ✓
GameCenter ✓ GameCenter ✓         GameCenter ✓
```

**TestFlight Details:**
- Up to 10,000 beta testers
- Real AdMob ads work (not test ads)
- Real GameCenter integration works
- Full analytics and crash reporting
- No App Store approval needed for beta builds
- Can iterate quickly (new builds in minutes)
- Apple automatically handles distribution

**TestFlight Setup:**
1. Archive app in Xcode (Product → Archive)
2. Upload to App Store Connect
3. Add external testers (email addresses)
4. Testers install via TestFlight app
5. They see real ads and real GameCenter
6. You get feedback and metrics

### Q: What's the full deployment process?
**A: Four stages:**

#### Stage 1: Local Development (Now)
- Use AdMob test ad unit IDs
- Test with iOS Simulator
- Test on device with Xcode
- No AdMob account needed yet

#### Stage 2: AdMob Setup (Tonight)
- Create AdMob account: https://apps.admob.com/
- Register app: "FloppyTurd" (iOS)
- Create ad units:
  - Interstitial Ad Unit → Get ID
  - Rewarded Video Ad Unit → Get ID
- Configure family-safe content settings
- Enable COPPA if targeting kids under 13

#### Stage 3: TestFlight Beta (This Week)
- Replace test IDs with real Ad Unit IDs
- Archive and upload to App Store Connect
- Add beta testers
- They see real ads and earn you real revenue
- Monitor AdMob dashboard for impressions/revenue
- Test GameCenter leaderboards with real users

#### Stage 4: App Store Release (Next Week)
- Submit for App Review
- Wait 24-48 hours for approval
- Publish to App Store
- Start earning from public users

---

## Architecture Overview

### Existing System (Reference)
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

## Implementation Plan

### Phase 1: C++ Platform Delegates (30 min)

#### 1.1 Add AdDelegate to PlatformDelegates.h

**File:** `src/Engine/Platform/PlatformDelegates.h`

**Location:** After `GameCenterDelegate` struct (around line 758)

```cpp
// Ad delegate - iOS AdMob advertising integration
struct AdDelegate {
    // Initialization
    void (*initialize)();
    bool (*isInitialized)();
    
    // Interstitial ads (full-screen after game over)
    void (*loadInterstitialAd)();
    bool (*isInterstitialReady)();
    void (*showInterstitialAd)(void (*completion)(bool success));
    
    // Rewarded video ads (optional bonus)
    void (*loadRewardedAd)();
    bool (*isRewardedReady)();
    void (*showRewardedAd)(void (*completion)(bool success, int rewardAmount));
    
    // Ad removal (IAP)
    bool (*areAdsRemoved)();
    void (*setAdsRemoved)(bool removed);
    
    // Initialize to null
    AdDelegate()
        : initialize(nullptr)
        , isInitialized(nullptr)
        , loadInterstitialAd(nullptr)
        , isInterstitialReady(nullptr)
        , showInterstitialAd(nullptr)
        , loadRewardedAd(nullptr)
        , isRewardedReady(nullptr)
        , showRewardedAd(nullptr)
        , areAdsRemoved(nullptr)
        , setAdsRemoved(nullptr)
    {}
};
```

#### 1.2 Add AdDelegate to PlatformDelegates Container

**File:** `src/Engine/Platform/PlatformDelegates.h`

**Update PlatformDelegates struct** (around line 759):

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
    
    // ... rest of struct
};
```

---

### Phase 2: Threading Commands (45 min)

#### 2.1 Add AdCommand Enum Values

**File:** `src/Engine/Platform/PlatformDelegates.h`

**Add to CommandType enum** (around line 180):

```cpp
enum class CommandType : uint32_t {
    // ... existing commands ...
    
    // Game Center commands
    CMD_GAME_CENTER_AUTHENTICATE,
    CMD_GAME_CENTER_SUBMIT_SCORE,
    CMD_GAME_CENTER_SHOW_LEADERBOARD,
    CMD_GAME_CENTER_SHOW_ALL_LEADERBOARDS,
    
    // Ad commands (NEW)
    CMD_AD_INITIALIZE,
    CMD_AD_LOAD_INTERSTITIAL,
    CMD_AD_SHOW_INTERSTITIAL,
    CMD_AD_LOAD_REWARDED,
    CMD_AD_SHOW_REWARDED,
    CMD_AD_SET_REMOVED,
};
```

#### 2.2 Add AdCommand Structure

**File:** `src/Engine/Platform/PlatformDelegates.h`

**Add after GameCenterCommand struct** (around line 415):

```cpp
// Ad command structure for threading
struct AdCommand {
    CommandType type;
    
    struct AdCommandData {
        bool adsRemoved = false;
        
        AdCommandData() = default;
    } data;
    
    AdCommand(CommandType t) : type(t) {}
};
```

#### 2.3 Add Queue to ThreadingProxy

**File:** `src/iOS/Threading/ThreadingProxy.h`

**Add to private member variables** (around line 230):

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

**Add enqueue function to private methods**:

```cpp
void enqueueAdCommand(const AdCommand& command);
```

#### 2.4 Add Public Static Methods to ThreadingProxy

**File:** `src/iOS/Threading/ThreadingProxy.h`

**Add after GameCenter methods** (around line 115):

```cpp
// Ad commands
static void enqueueAdInitialize();
static void enqueueAdLoadInterstitial();
static void enqueueAdShowInterstitial();
static void enqueueAdLoadRewarded();
static void enqueueAdShowRewarded();
static void enqueueAdSetRemoved(bool removed);
```

#### 2.5 Implement ThreadingProxy Ad Methods

**File:** `src/iOS/Threading/ThreadingProxy.cpp`

**Add after GameCenter implementations** (around line 610):

```cpp
// Ad command implementations

void ThreadingProxy::enqueueAdCommand(const AdCommand& command) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_adCommandQueue.push_back(command);
}

void ThreadingProxy::enqueueAdInitialize() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_INITIALIZE);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdLoadInterstitial() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_LOAD_INTERSTITIAL);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdShowInterstitial() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_SHOW_INTERSTITIAL);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdLoadRewarded() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_LOAD_REWARDED);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdShowRewarded() {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_SHOW_REWARDED);
    s_instance->enqueueAdCommand(cmd);
}

void ThreadingProxy::enqueueAdSetRemoved(bool removed) {
    if (!s_instance) return;
    AdCommand cmd(CommandType::CMD_AD_SET_REMOVED);
    cmd.data.adsRemoved = removed;
    s_instance->enqueueAdCommand(cmd);
}
```

#### 2.6 Add Getter for Ad Commands

**File:** `src/iOS/Threading/ThreadingProxy.cpp`

**Add after getAndClearGameCenterCommandsFromProxy()** (around line 730):

```cpp
std::vector<AdCommand> getAndClearAdCommandsFromProxy() {
    if (!g_threadingProxy) return {};
    
    std::lock_guard<std::mutex> lock(g_threadingProxy->m_queueMutex);
    std::vector<AdCommand> commands = g_threadingProxy->m_adCommandQueue;
    g_threadingProxy->m_adCommandQueue.clear();
    return commands;
}
```

#### 2.7 Update clearQueue and hasCommands

**File:** `src/iOS/Threading/ThreadingProxy.cpp`

**Update clearQueue()** (around line 30):

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

**Update hasCommands()** (around line 38):

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

### Phase 3: Swift AdManager (60 min)

#### 3.1 Create AdManager.swift

**File:** `src/iOS/Advertising/AdManager.swift` (NEW FILE)

```swift
//
//  AdManager.swift
//  FloppyTurd
//
//  AdMob advertising manager using Google Mobile Ads SDK
//  Integrated with ThreadingProxy command system
//

import Foundation
import GoogleMobileAds
import os.log

@MainActor
final class AdManager: NSObject {
    
    // MARK: - Singleton
    
    static let shared = AdManager()
    
    // MARK: - Properties
    
    private let logger = Logger(subsystem: "com.floppyturd.game", category: "AdManager")
    
    private var interstitialAd: GADInterstitialAd?
    private var rewardedAd: GADRewardedAd?
    
    private var isInitialized = false
    private var adsRemoved = false
    
    // Ad Unit IDs - USE TEST IDs FOR DEVELOPMENT
    // Replace with real IDs from AdMob console before beta/release
    private let interstitialAdUnitID = "ca-app-pub-3940256099942544/4411468910" // TEST ID
    private let rewardedAdUnitID = "ca-app-pub-3940256099942544/1712485313"     // TEST ID
    
    // Completion handlers for C++ callbacks
    private var interstitialCompletion: ((Bool) -> Void)?
    private var rewardedCompletion: ((Bool, Int) -> Void)?
    
    // MARK: - Initialization
    
    override private init() {
        super.init()
    }
    
    func initialize() {
        guard !isInitialized else {
            logger.info("AdMob already initialized")
            return
        }
        
        logger.info("Initializing AdMob SDK...")
        
        // Configure for family-friendly content
        let requestConfiguration = GADMobileAds.sharedInstance().requestConfiguration
        requestConfiguration.tagForChildDirectedTreatment = true
        requestConfiguration.maxAdContentRating = .general
        
        // Start SDK
        GADMobileAds.sharedInstance().start { [weak self] status in
            self?.logger.info("AdMob SDK initialized successfully")
            self?.isInitialized = true
            
            // Preload ads
            self?.loadInterstitialAd()
            self?.loadRewardedAd()
        }
    }
    
    func getIsInitialized() -> Bool {
        return isInitialized
    }
    
    // MARK: - Interstitial Ads
    
    func loadInterstitialAd() {
        guard isInitialized else {
            logger.warning("Cannot load interstitial - AdMob not initialized")
            return
        }
        
        let request = GADRequest()
        
        GADInterstitialAd.load(withAdUnitID: interstitialAdUnitID, request: request) { [weak self] ad, error in
            if let error = error {
                self?.logger.error("Failed to load interstitial ad: \(error.localizedDescription)")
                return
            }
            
            self?.interstitialAd = ad
            self?.interstitialAd?.fullScreenContentDelegate = self
            self?.logger.info("Interstitial ad loaded successfully")
        }
    }
    
    func isInterstitialReady() -> Bool {
        return interstitialAd != nil
    }
    
    func showInterstitialAd(completion: @escaping (Bool) -> Void) {
        guard !adsRemoved else {
            logger.info("Ads removed by user, skipping interstitial")
            completion(false)
            return
        }
        
        guard let rootViewController = UIApplication.shared.windows.first?.rootViewController else {
            logger.error("No root view controller found")
            completion(false)
            return
        }
        
        guard let interstitialAd = interstitialAd else {
            logger.warning("Interstitial ad not ready")
            loadInterstitialAd() // Load for next time
            completion(false)
            return
        }
        
        logger.info("Showing interstitial ad")
        self.interstitialCompletion = completion
        interstitialAd.present(fromRootViewController: rootViewController)
    }
    
    // MARK: - Rewarded Ads
    
    func loadRewardedAd() {
        guard isInitialized else {
            logger.warning("Cannot load rewarded ad - AdMob not initialized")
            return
        }
        
        let request = GADRequest()
        
        GADRewardedAd.load(withAdUnitID: rewardedAdUnitID, request: request) { [weak self] ad, error in
            if let error = error {
                self?.logger.error("Failed to load rewarded ad: \(error.localizedDescription)")
                return
            }
            
            self?.rewardedAd = ad
            self?.rewardedAd?.fullScreenContentDelegate = self
            self?.logger.info("Rewarded ad loaded successfully")
        }
    }
    
    func isRewardedReady() -> Bool {
        return rewardedAd != nil
    }
    
    func showRewardedAd(completion: @escaping (Bool, Int) -> Void) {
        guard let rootViewController = UIApplication.shared.windows.first?.rootViewController else {
            logger.error("No root view controller found")
            completion(false, 0)
            return
        }
        
        guard let rewardedAd = rewardedAd else {
            logger.warning("Rewarded ad not ready")
            loadRewardedAd() // Load for next time
            completion(false, 0)
            return
        }
        
        logger.info("Showing rewarded ad")
        self.rewardedCompletion = completion
        
        rewardedAd.present(fromRootViewController: rootViewController) { [weak self] in
            let reward = rewardedAd.adReward
            let rewardAmount = reward.amount.intValue
            self?.logger.info("User earned reward: \(rewardAmount) \(reward.type)")
            
            // Call completion with reward
            self?.rewardedCompletion?(true, rewardAmount)
            self?.rewardedCompletion = nil
            
            // Preload next ad
            self?.loadRewardedAd()
        }
    }
    
    // MARK: - Ad Removal (IAP)
    
    func setAdsRemoved(_ removed: Bool) {
        adsRemoved = removed
        logger.info("Ads removed status: \(removed)")
        
        // Clear loaded ads if ads removed
        if removed {
            interstitialAd = nil
            rewardedAd = nil
        }
    }
    
    func areAdsRemoved() -> Bool {
        return adsRemoved
    }
}

// MARK: - GADFullScreenContentDelegate

extension AdManager: GADFullScreenContentDelegate {
    
    func adDidPresentFullScreenContent(_ ad: GADFullScreenPresentingAd) {
        logger.info("Ad presented full screen")
    }
    
    func adDidDismissFullScreenContent(_ ad: GADFullScreenPresentingAd) {
        logger.info("Ad dismissed")
        
        // Handle interstitial dismissal
        if ad is GADInterstitialAd {
            interstitialCompletion?(true)
            interstitialCompletion = nil
            
            // Preload next interstitial
            loadInterstitialAd()
        }
        
        // Rewarded ad dismissal handled in present() callback
    }
    
    func ad(_ ad: GADFullScreenPresentingAd, didFailToPresentFullScreenContentWithError error: Error) {
        logger.error("Ad failed to present: \(error.localizedDescription)")
        
        // Handle failure
        if ad is GADInterstitialAd {
            interstitialCompletion?(false)
            interstitialCompletion = nil
        } else if ad is GADRewardedAd {
            rewardedCompletion?(false, 0)
            rewardedCompletion = nil
        }
    }
}
```

---

### Phase 4: Swift CommandProcessor Integration (30 min)

#### 4.1 Update ThreadingSystem.swift

**File:** `src/iOS/Threading/ThreadingSystem.swift`

**Add to processCommands()** (around line 99):

```swift
let adCommands = GameCorePlatform.GameCore.getAndClearAdCommandsFromProxy()
```

**Add to processing loop** (around line 128):

```swift
for adCommand in adCommands {
    executeAdCommand(adCommand)
}
```

**Add executeAdCommand method** (add after executeGameCenterCommand, around line 900):

```swift
private func executeAdCommand(_ command: GameCorePlatform.GameCore.AdCommand) {
    let commandType = command.type
    
    // Process commands directly - AdManager is @MainActor
    switch commandType {
    case .CMD_AD_INITIALIZE:
        AdManager.shared.initialize()
        log("[CommandProcessor] AdMob initialized", level: .info)
        
    case .CMD_AD_LOAD_INTERSTITIAL:
        AdManager.shared.loadInterstitialAd()
        log("[CommandProcessor] Loading interstitial ad", level: .info)
        
    case .CMD_AD_SHOW_INTERSTITIAL:
        AdManager.shared.showInterstitialAd { [weak self] success in
            if success {
                self?.log("[CommandProcessor] Interstitial ad shown successfully", level: .info)
            } else {
                self?.log("[CommandProcessor] Interstitial ad failed to show", level: .warning)
            }
        }
        
    case .CMD_AD_LOAD_REWARDED:
        AdManager.shared.loadRewardedAd()
        log("[CommandProcessor] Loading rewarded ad", level: .info)
        
    case .CMD_AD_SHOW_REWARDED:
        AdManager.shared.showRewardedAd { [weak self] success, rewardAmount in
            if success {
                self?.log("[CommandProcessor] Rewarded ad shown, reward: \(rewardAmount)", level: .info)
                // TODO: Add callback to C++ to grant reward
            } else {
                self?.log("[CommandProcessor] Rewarded ad failed to show", level: .warning)
            }
        }
        
    case .CMD_AD_SET_REMOVED:
        let removed = command.data.adsRemoved
        AdManager.shared.setAdsRemoved(removed)
        log("[CommandProcessor] Ads removed status: \(removed)", level: .info)
        
    default:
        log("[CommandProcessor] Unknown ad command: \(commandType)", level: .warning)
    }
}
```

---

### Phase 5: C++ Game Logic Integration (45 min)

#### 5.1 Create AdCounterSystem

**File:** `src/FloppyTurd/Systems/AdCounterSystem.h` (NEW FILE)

```cpp
#pragma once

namespace FloppyTurd {

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
    , m_minGameOversBeforeAds(3) // No ads for first 3 deaths
    , m_adsDisabled(false)
{
    GN_LOG_INFO("AdCounterSystem initialized - frequency: " + std::to_string(m_adFrequency));
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
        return false;
    }
    
    // Don't show for first N game overs (learning period)
    if (m_gameOverCount < m_minGameOversBeforeAds) {
        return false;
    }
    
    // Show ad every N game overs
    return (m_gameOverCount % m_adFrequency) == 0;
}

void AdCounterSystem::ResetCounter() {
    m_gameOverCount = 0;
    GN_LOG_DEBUG("Ad counter reset");
}

void AdCounterSystem::SetAdFrequency(int frequency) {
    m_adFrequency = frequency;
    GN_LOG_INFO("Ad frequency updated: " + std::to_string(m_adFrequency));
}

void AdCounterSystem::SetAdsDisabled(bool disabled) {
    m_adsDisabled = disabled;
    GN_LOG_INFO("Ads disabled: " + std::string(disabled ? "YES" : "NO"));
}

} // namespace FloppyTurd
```

#### 5.2 Update FloppyTurdGame

**File:** `src/FloppyTurd/Game/FloppyTurdGame.h`

**Add member variable** (around line 100):

```cpp
private:
    // ... existing members ...
    std::unique_ptr<AdCounterSystem> m_adCounter;
```

**File:** `src/FloppyTurd/Game/FloppyTurdGame.cpp`

**Initialize in constructor** (around line 50):

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
        GN_LOG_INFO("AdMob initialization requested");
    }
}
```

**Add helper method**:

```cpp
void FloppyTurdGame::TriggerGameOverAd() {
    if (!m_adCounter) return;
    
    m_adCounter->OnGameOver();
    
    if (m_adCounter->ShouldShowAd()) {
        GN_LOG_INFO("Triggering interstitial ad after game over");
        
        // Check if ad is ready
        if (m_platformDelegates.ad.isInterstitialReady && 
            m_platformDelegates.ad.isInterstitialReady()) {
            
            // Show ad
            if (m_platformDelegates.ad.showInterstitialAd) {
                m_platformDelegates.ad.showInterstitialAd([](bool success) {
                    if (success) {
                        GN_LOG_INFO("Interstitial ad shown successfully");
                    } else {
                        GN_LOG_WARNING("Interstitial ad failed to show");
                    }
                });
            }
        } else {
            GN_LOG_DEBUG("Interstitial ad not ready, loading for next time");
            if (m_platformDelegates.ad.loadInterstitialAd) {
                m_platformDelegates.ad.loadInterstitialAd();
            }
        }
    }
}
```

#### 5.3 Update GameOverState

**File:** `src/FloppyTurd/States/GameOverState.cpp`

**Call ad trigger in Enter()** (around line 40):

```cpp
void GameOverState::Enter() {
    GN_LOG_INFO("Entering Game Over State");
    
    // ... existing game over logic ...
    
    // Trigger ad check
    if (m_game) {
        m_game->TriggerGameOverAd();
    }
}
```

---

### Phase 6: iOS Project Configuration (30 min)

#### 6.1 Add AdMob SDK via Swift Package Manager

**In Xcode:**
1. File → Add Package Dependencies
2. Enter URL: `https://github.com/googleads/swift-package-manager-google-mobile-ads.git`
3. Select "Up to Next Major Version" (latest)
4. Click "Add Package"

**Alternative - CocoaPods (if you use it):**

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

**Add AdMob App ID** (you'll get this from AdMob console tonight):

```xml
<key>GADApplicationIdentifier</key>
<string>ca-app-pub-XXXXXXXXXXXXXXXX~YYYYYYYYYY</string>

<!-- For child-directed treatment (if targeting kids) -->
<key>GADIsForChildDirectedTreatment</key>
<true/>

<!-- SKAdNetwork IDs (required for iOS 14+ attribution) -->
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
  <!-- Add more from: https://developers.google.com/admob/ios/quick-start -->
</array>
```

#### 6.3 Update CMakeLists.txt

**File:** `CMakeLists.txt`

**Add AdCounterSystem to build** (around line 105):

```cmake
file(GLOB_RECURSE FLOPPYTURD_CPP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems/*.cpp"  # This includes AdCounterSystem
    # ... rest of sources
)
```

---

### Phase 7: Platform Delegate Setup (15 min)

#### 7.1 Initialize Ad Delegates in iOS

**File:** `src/iOS/GameViewController.swift`

**Update setupPlatformDelegates()** (add after GameCenter setup):

```swift
// Ad delegate setup
delegates.ad.initialize = {
    ThreadingProxy.enqueueAdInitialize()
}

delegates.ad.isInitialized = {
    return AdManager.shared.getIsInitialized()
}

delegates.ad.loadInterstitialAd = {
    ThreadingProxy.enqueueAdLoadInterstitial()
}

delegates.ad.isInterstitialReady = {
    return AdManager.shared.isInterstitialReady()
}

delegates.ad.showInterstitialAd = { completion in
    ThreadingProxy.enqueueAdShowInterstitial()
    // Note: completion is stored in AdManager
}

delegates.ad.loadRewardedAd = {
    ThreadingProxy.enqueueAdLoadRewarded()
}

delegates.ad.isRewardedReady = {
    return AdManager.shared.isRewardedReady()
}

delegates.ad.showRewardedAd = { completion in
    ThreadingProxy.enqueueAdShowRewarded()
    // Note: completion is stored in AdManager
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

### Development Testing (Test Ads)

1. **Build and Run**
   - Launch app in simulator/device
   - Play game and die 5 times
   - Verify test ad appears

2. **Test Ad Loading**
   - Check Xcode console for "Interstitial ad loaded"
   - Verify no errors in logs

3. **Test Ad Dismissal**
   - Close ad
   - Verify game continues normally
   - Verify next ad loads

4. **Test Ad Frequency**
   - Die 3 times → no ad (learning period)
   - Die 2 more times (5 total) → ad shows
   - Die 5 more times (10 total) → ad shows again

5. **Test Rewarded Ads** (later)
   - Add UI button to trigger rewarded ad
   - Verify reward is granted
   - Verify ad loads again

### Beta Testing (Real Ads)

1. **Setup AdMob**
   - Create account
   - Register app
   - Get real Ad Unit IDs
   - Replace test IDs in `AdManager.swift`

2. **TestFlight Build**
   - Archive app
   - Upload to App Store Connect
   - Add external testers
   - Wait for review (usually 24 hours)

3. **Beta Tester Experience**
   - Testers install via TestFlight
   - They see real ads
   - You earn real revenue (in testing mode)
   - Monitor AdMob dashboard

4. **Metrics to Monitor**
   - Impression rate (ads shown per session)
   - Fill rate (% of ad requests filled)
   - eCPM (revenue per 1000 impressions)
   - Retention (Day 1, Day 7)

---

## AdMob Account Setup (Tonight)

### Step-by-Step

1. **Go to:** https://apps.admob.com/

2. **Sign In** with Google account

3. **Add Your App**
   - Click "Apps" → "Add App"
   - Platform: iOS
   - App name: FloppyTurd
   - Is it published? NO (or YES if already live)

4. **Create Ad Units**
   
   **Interstitial Ad Unit:**
   - Click "Ad units" → "Add ad unit"
   - Format: Interstitial
   - Ad unit name: "FloppyTurd Interstitial"
   - Copy the Ad Unit ID (ca-app-pub-XXXX/YYYY)
   
   **Rewarded Video Ad Unit:**
   - Click "Ad units" → "Add ad unit"
   - Format: Rewarded
   - Ad unit name: "FloppyTurd Rewarded"
   - Copy the Ad Unit ID (ca-app-pub-XXXX/ZZZZ)

5. **Configure Settings**
   - Go to "Blocking controls"
   - Select "General categories"
   - Block: Dating, Gambling, Mature content, Sexual content
   - Enable "Family self-certified ads inventory"

6. **Copy App ID**
   - Go to "App settings"
   - Copy "App ID" (ca-app-pub-XXXX~YYYY)
   - This goes in Info.plist

7. **Update Code**
   - Replace test IDs in `AdManager.swift`:
     ```swift
     private let interstitialAdUnitID = "ca-app-pub-YOUR-ID/1234567890"
     private let rewardedAdUnitID = "ca-app-pub-YOUR-ID/0987654321"
     ```
   - Add App ID to Info.plist:
     ```xml
     <key>GADApplicationIdentifier</key>
     <string>ca-app-pub-YOUR-APP-ID~1234567890</string>
     ```

---

## Revenue Estimates

### Conservative (1000 DAU)

```
1000 users/day
× 3 sessions/user
× 0.4 ads/session (not every death shows ad)
= 1200 impressions/day

1200 impressions × $4 CPM = $4.80/day
× 30 days = $144/month
```

### Realistic Growth (5000 DAU)

```
5000 users/day
× 3 sessions/user
× 0.4 ads/session
= 6000 impressions/day

6000 impressions × $5 CPM = $30/day
× 30 days = $900/month
```

### With "Remove Ads" IAP

```
5000 DAU × 2% conversion × $2.99 = $299/month additional
Total: $900 + $299 = $1,199/month
```

---

## Troubleshooting

### Ad Not Showing

**Check:**
1. AdMob initialized? Check console logs
2. Using correct Ad Unit ID?
3. Internet connection?
4. Ad loaded before showing? Check `isInterstitialReady()`
5. Test ads working? Try test ID first

### Build Errors

**Common Issues:**
1. Missing GoogleMobileAds import → Check Swift Package Manager
2. Module not found → Clean build folder (Cmd+Shift+K)
3. Linker errors → Check CMakeLists includes AdCounterSystem

### AdMob Account Issues

**Solutions:**
1. Account suspended? Check email from Google
2. Ad serving limited? Complete account verification
3. Payment threshold? Need $100 to receive payment

---

## Next Steps After Implementation

1. **Week 1:** Local development with test ads
2. **Week 1:** Get AdMob account approved (instant)
3. **Week 2:** TestFlight beta with real ads
4. **Week 2:** Monitor metrics, adjust frequency
5. **Week 3:** App Store submission
6. **Week 4:** Launch and scale!

---

## File Checklist

### New Files to Create
- [ ] `src/iOS/Advertising/AdManager.swift`
- [ ] `src/FloppyTurd/Systems/AdCounterSystem.h`
- [ ] `src/FloppyTurd/Systems/AdCounterSystem.cpp`

### Files to Modify
- [ ] `src/Engine/Platform/PlatformDelegates.h` (AdDelegate, CommandType, AdCommand)
- [ ] `src/iOS/Threading/ThreadingProxy.h` (ad methods)
- [ ] `src/iOS/Threading/ThreadingProxy.cpp` (ad implementations)
- [ ] `src/iOS/Threading/ThreadingSystem.swift` (executeAdCommand)
- [ ] `src/FloppyTurd/Game/FloppyTurdGame.h` (add AdCounterSystem member)
- [ ] `src/FloppyTurd/Game/FloppyTurdGame.cpp` (initialize, TriggerGameOverAd)
- [ ] `src/FloppyTurd/States/GameOverState.cpp` (call TriggerGameOverAd)
- [ ] `src/iOS/GameViewController.swift` (setup ad delegates)
- [ ] `FloppyTurd/Info.plist` (AdMob App ID, SKAdNetwork)
- [ ] `CMakeLists.txt` (if not auto-including AdCounterSystem)

---

## Summary

This plan integrates AdMob using your existing architecture patterns:
- ✅ No separate delegates - uses PlatformDelegates
- ✅ Threading via ThreadingProxy command queue
- ✅ Swift execution via CommandProcessor
- ✅ Matches GameCenter pattern exactly
- ✅ Non-intrusive ad frequency (every 5 game overs)
- ✅ Learning period (first 3 deaths no ads)
- ✅ Ready for TestFlight beta testing
- ✅ Revenue projections: $200-1200/month realistic

**Time Estimate:** 3-4 hours total implementation
**Revenue Potential:** $200-1500/month (with good retention)

Good luck tonight setting up your AdMob account! 🚀💰