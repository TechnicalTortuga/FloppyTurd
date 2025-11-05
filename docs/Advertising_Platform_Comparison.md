# iOS Mobile Game Advertising Platform Comparison

**Date:** November 2, 2025  
**Game:** FloppyTurd (Casual/Arcade iOS Game)  
**Target Audience:** Family-friendly, kids & adults  
**Monetization Strategy:** Non-intrusive ads + "Remove Ads" IAP

---

## Executive Summary

This document compares 4 major iOS advertising platforms suitable for casual mobile games. All platforms support iOS native apps (non-Unity) and offer child-safe content filtering.

**Quick Recommendation:** Start with **Google AdMob** for ease of implementation and strong content controls, with optional mediation to Unity Ads or AppLovin later for revenue optimization.

---

## Platform Comparison Matrix

| Feature | Google AdMob | Unity Ads | AppLovin MAX | ironSource |
|---------|--------------|-----------|--------------|------------|
| **iOS Native Support** | ✅ Yes (Swift) | ✅ Yes (non-Unity) | ✅ Yes | ✅ Yes |
| **SDK Size** | ~5-8 MB | ~3-5 MB | ~4-6 MB | ~5-7 MB |
| **Ease of Integration** | ⭐⭐⭐⭐⭐ Excellent | ⭐⭐⭐⭐ Good | ⭐⭐⭐⭐ Good | ⭐⭐⭐ Moderate |
| **Child Safety Controls** | ✅ Excellent | ✅ Good | ✅ Good | ✅ Good |
| **Non-Personalized Ads** | ✅ Yes (GDPR mode) | ✅ Yes | ✅ Yes | ✅ Yes |
| **Content Filtering** | ⭐⭐⭐⭐⭐ Best | ⭐⭐⭐⭐ Good | ⭐⭐⭐⭐ Good | ⭐⭐⭐ Moderate |
| **eCPM (Est.)** | $2-8 | $3-10 | $4-12 | $3-9 |
| **Fill Rate** | 95-98% | 90-95% | 92-97% | 88-94% |
| **Payment Threshold** | $100 | $100 | $50 | $50 |
| **Mediation Support** | ✅ AdMob Mediation | ✅ Unity LevelPlay | ✅ MAX (Built-in) | ✅ ironSource Mediation |
| **Documentation Quality** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Developer Experience** | Beginner-friendly | Moderate | Moderate | Advanced |

---

## 1. Google AdMob

### Overview
Industry-leading mobile ad platform from Google. Best for indie developers and first-time ad integration.

### ✅ Pros
- **Easiest integration** - Best documentation and Swift examples
- **Excellent content controls** - Family-safe content categories
- **High fill rates** (95-98%) - Rarely shows blank ad slots
- **COPPA compliance** - Built-in tools for child-directed apps
- **Reliable payments** - Google's payment infrastructure
- **Test ads built-in** - Easy development/testing
- **AdMob mediation** - Can add other networks later
- **No personalization option** - Perfect for privacy-focused apps

### ⚠️ Cons
- Slightly lower eCPM than competitors (but higher fill rate compensates)
- $100 payment threshold (vs $50 for others)
- Requires Google AdMob account setup

### Content Control
- **Family-safe categories available** ✅
- **Sensitive content blocking** ✅
- **Ad review center** - Preview ads before they show ✅
- **Block specific advertisers** ✅
- **Age-appropriate content filtering** ✅

### Technical Details
```swift
// Integration via Swift Package Manager or CocoaPods
pod 'Google-Mobile-Ads-SDK'

// Simple initialization
import GoogleMobileAds
GADMobileAds.sharedInstance().start()
```

### Ad Formats for FloppyTurd
- **Interstitial** - Full-screen after game over (recommended)
- **Rewarded Video** - Optional for bonus coins/continue
- **Banner** - Main menu only (least recommended)

### Estimated Revenue
- Casual arcade game: $2-6 eCPM
- With good content and retention: $4-8 eCPM
- 1000 DAU → ~$80-240/month (at 3 ads per session)

### Links
- Documentation: https://developers.google.com/admob/ios/quick-start
- Content Policies: https://support.google.com/admob/answer/9999955
- Family Policies: https://support.google.com/families/answer/7103338

---

## 2. Unity Ads (LevelPlay)

### Overview
Strong competitor from Unity, excellent for games. Good even without Unity engine.

### ✅ Pros
- **Higher eCPM** - Generally beats AdMob by 20-40%
- **Game-focused demand** - Advertisers targeting gamers
- **LevelPlay mediation** - Built-in mediation platform
- **Good documentation** for iOS native
- **Lower payment threshold** ($100 but more frequent)
- **Rewarded video expertise** - Best format for games

### ⚠️ Cons
- Slightly more complex integration than AdMob
- Not as strong content filtering as AdMob
- Requires Unity Developer account
- Less documentation for non-Unity iOS apps

### Content Control
- **Family-safe content** ✅
- **COPPA compliance tools** ✅
- **Less granular than AdMob** ⚠️
- **Requires manual review** ⚠️

### Technical Details
```swift
// CocoaPods integration
pod 'UnityAds'

// Basic setup
import UnityAds
UnityAds.initialize("YOUR_GAME_ID", testMode: true)
```

### Best Use Case
- **Secondary network** via mediation
- **Rewarded video ads** (they excel at this)
- Games with already good retention

### Estimated Revenue
- Casual arcade: $3-10 eCPM
- Higher for rewarded video
- 1000 DAU → ~$120-300/month

### Links
- iOS SDK: https://docs.unity.com/ads/ImplementingNonUnityProjects.html
- Mediation: https://docs.unity.com/mediation/

---

## 3. AppLovin MAX

### Overview
High-performance ad platform with advanced mediation. Best for developers focused on revenue optimization.

### ✅ Pros
- **Highest eCPM potential** - Premium demand
- **MAX mediation built-in** - Access multiple networks
- **Real-time bidding** - Maximizes ad value
- **Advanced analytics** - Deep revenue insights
- **A/B testing tools** - Optimize ad frequency
- **$50 payment threshold** - Lower barrier

### ⚠️ Cons
- More complex setup than AdMob
- Requires more optimization work
- Steeper learning curve
- Some advertisers show aggressive creative

### Content Control
- **Family content filtering** ✅
- **Sensitive content blocking** ✅
- **Less transparent than AdMob** ⚠️
- **Requires careful configuration** ⚠️

### Technical Details
```swift
// CocoaPods
pod 'AppLovinSDK'

// Initialize
import AppLovinSDK
ALSdk.shared()!.initializeSdk()
```

### Best Use Case
- **After establishing base revenue** with AdMob
- **Advanced developers** comfortable with optimization
- Apps with strong user base (5000+ DAU)

### Estimated Revenue
- Casual arcade: $4-12 eCPM (with optimization)
- Requires active management
- 1000 DAU → ~$150-360/month (with optimization)

### Links
- iOS Integration: https://dash.applovin.com/documentation/mediation/ios/getting-started/integration
- MAX Mediation: https://dash.applovin.com/documentation/mediation/ios

---

## 4. ironSource (Unity Company)

### Overview
Enterprise-level ad platform, recently acquired by Unity. Advanced features but complex.

### ✅ Pros
- **Strong gaming focus** - Deep game advertiser network
- **Comprehensive mediation** - Connect to multiple networks
- **Advanced segmentation** - Target specific user groups
- **A/B testing suite** - Built-in experimentation
- **Good for hyper-casual** - Proven with arcade games

### ⚠️ Cons
- **Complex integration** - Steeper learning curve
- **Requires more code** - More setup than competitors
- **Less indie-friendly** - Geared toward studios
- **Content control** - Requires manual configuration

### Content Control
- **Family-safe options** ✅
- **COPPA support** ✅
- **Less intuitive filtering** ⚠️
- **Manual blocklist management** ⚠️

### Technical Details
```swift
// CocoaPods
pod 'IronSourceSDK'

// Complex initialization
import IronSource
IronSource.setUserId("USER_ID")
IronSource.initWithAppKey("APP_KEY")
```

### Best Use Case
- **Established games** with proven metrics
- **Hyper-casual arcade games** (perfect fit)
- After validating with simpler platform

### Estimated Revenue
- Hyper-casual: $3-9 eCPM
- Requires optimization
- 1000 DAU → ~$120-270/month

### Links
- iOS SDK: https://developers.is.com/ironsource-mobile/ios/ios-sdk/
- Mediation: https://developers.is.com/ironsource-mobile/ios/mediation-networks-ios/

---

## Content Safety & Family-Friendly Comparison

### Sexual/Adult Content Protection
| Platform | Rating |
|----------|--------|
| AdMob | ⭐⭐⭐⭐⭐ Best - Strict policies, easy blocking |
| Unity Ads | ⭐⭐⭐⭐ Good - Family categories available |
| AppLovin | ⭐⭐⭐⭐ Good - Requires configuration |
| ironSource | ⭐⭐⭐ Moderate - Manual management needed |

### Non-Personalized Ads (Privacy-First)
| Platform | Support |
|----------|---------|
| AdMob | ✅ Full support (best GDPR compliance) |
| Unity Ads | ✅ Yes (contextual ads) |
| AppLovin | ✅ Yes (with configuration) |
| ironSource | ✅ Yes (requires setup) |

### COPPA & Kids App Compliance
All platforms support COPPA compliance, but **AdMob** has the most intuitive tools and clearest documentation.

---

## Recommended Integration Strategy

### Phase 1: MVP (Week 1)
1. **Start with Google AdMob**
   - Easiest to implement
   - Best content safety controls
   - Family-friendly by default
   - Excellent documentation
   
2. **Implement:**
   - Interstitial ads (after 5 game overs)
   - Rewarded video (optional bonus coins)
   - Test with AdMob test IDs
   
3. **Configure:**
   - Enable family-safe content only
   - Disable personalized ads
   - Block sensitive categories
   - Set COPPA mode if targeting kids

### Phase 2: Optimization (Post-Beta)
1. **Add AdMob Mediation**
   - Connect Unity Ads as secondary
   - A/B test ad frequency (3 vs 5 game overs)
   - Monitor revenue and retention
   
2. **Analyze:**
   - Which network has better eCPM
   - User retention impact
   - Fill rates per network

### Phase 3: Advanced (Post-Launch)
1. **Consider AppLovin MAX** if:
   - You have 5000+ DAU
   - Revenue is primary goal
   - Have time for optimization
   
2. **Implement:**
   - MAX mediation with multiple networks
   - Advanced A/B testing
   - Segmented ad strategies

---

## Technical Implementation Plan

### Architecture: AdManager Substate Pattern

```
GameplayState
    ├─ Playing (normal gameplay)
    ├─ Paused
    ├─ GameOver
    │   └─ AdManager Substate (if counter threshold met)
    └─ Resume/Restart
```

### Implementation Structure

```
src/
├─ FloppyTurd/
│   ├─ States/
│   │   ├─ GameplayState.cpp
│   │   └─ AdManagerState.cpp (NEW)
│   └─ Systems/
│       └─ AdCounterSystem.cpp (NEW)
└─ iOS/
    ├─ Advertising/ (NEW)
    │   ├─ AdManager.swift
    │   ├─ AdDelegate.swift
    │   └─ AdConfiguration.swift
    └─ Platform/
        └─ PlatformDelegates.h (UPDATE)
```

### Key Components

1. **C++ Side: AdCounterSystem**
   - Track game over count
   - Determine when to show ad
   - Trigger ad state transition
   - Handle "Remove Ads" purchase state

2. **C++ Side: AdManagerState**
   - Lightweight state that shows ad
   - Waits for ad completion/dismissal
   - Returns to previous state
   - No game logic, just ad coordination

3. **Swift Side: AdManager**
   - Initialize ad SDK
   - Load ads in advance (preload)
   - Show interstitial when requested
   - Handle rewarded video callbacks
   - Check "Remove Ads" IAP status

4. **Platform Bridge**
   - C++ function: `showInterstitialAd()`
   - C++ function: `showRewardedAd(rewardCallback)`
   - Swift callbacks for completion
   - Pass back to C++ game state

---

## Code Integration Examples

### 1. Info.plist Configuration (AdMob)

```xml
<!-- Add to Info.plist -->
<key>GADApplicationIdentifier</key>
<string>ca-app-pub-XXXXXXXXXXXXXXXX~YYYYYYYYYY</string>

<!-- For child-directed treatment -->
<key>GADIsForChildDirectedTreatment</key>
<true/>

<!-- SKAdNetwork IDs (required for iOS 14+) -->
<key>SKAdNetworkItems</key>
<array>
  <dict>
    <key>SKAdNetworkIdentifier</key>
    <string>cstr6suwn9.skadnetwork</string>
  </dict>
  <!-- ... more network IDs ... -->
</array>
```

### 2. Swift AdManager Implementation

```swift
// FloppyTurd/src/iOS/Advertising/AdManager.swift

import GoogleMobileAds
import os.log

@MainActor
final class AdManager: NSObject {
    static let shared = AdManager()
    
    private let logger = Logger(subsystem: "com.floppyturd.game", category: "AdManager")
    private var interstitialAd: GADInterstitialAd?
    private var rewardedAd: GADRewardedAd?
    
    private var isInitialized = false
    private var adsRemoved = false
    
    // Ad Unit IDs (use test IDs during development)
    private let interstitialAdUnitID = "ca-app-pub-3940256099942544/4411468910" // TEST ID
    private let rewardedAdUnitID = "ca-app-pub-3940256099942544/1712485313" // TEST ID
    
    override private init() {
        super.init()
    }
    
    // Initialize AdMob SDK
    func initialize() {
        guard !isInitialized else { return }
        
        logger.info("Initializing AdMob SDK...")
        
        // Configure for family-friendly content
        let requestConfiguration = GADMobileAds.sharedInstance().requestConfiguration
        requestConfiguration.tagForChildDirectedTreatment = true
        requestConfiguration.maxAdContentRating = .general
        
        // Start SDK
        GADMobileAds.sharedInstance().start { [weak self] status in
            self?.logger.info("AdMob SDK initialized")
            self?.isInitialized = true
            self?.preloadAds()
        }
    }
    
    // Preload ads in background
    func preloadAds() {
        loadInterstitialAd()
        loadRewardedAd()
    }
    
    // Load interstitial ad
    private func loadInterstitialAd() {
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
    
    // Load rewarded ad
    private func loadRewardedAd() {
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
    
    // Show interstitial ad (called from C++)
    func showInterstitialAd(completion: @escaping (Bool) -> Void) {
        guard !adsRemoved else {
            logger.info("Ads removed by user, skipping ad")
            completion(false)
            return
        }
        
        guard let rootViewController = UIApplication.shared.windows.first?.rootViewController else {
            logger.error("No root view controller found")
            completion(false)
            return
        }
        
        guard let interstitialAd = interstitialAd else {
            logger.warning("Interstitial ad not ready, loading new one")
            loadInterstitialAd()
            completion(false)
            return
        }
        
        logger.info("Showing interstitial ad")
        interstitialAd.present(fromRootViewController: rootViewController)
        
        // Store completion handler
        self.adCompletionHandler = completion
    }
    
    // Show rewarded ad
    func showRewardedAd(completion: @escaping (Bool, Int) -> Void) {
        guard let rootViewController = UIApplication.shared.windows.first?.rootViewController else {
            completion(false, 0)
            return
        }
        
        guard let rewardedAd = rewardedAd else {
            logger.warning("Rewarded ad not ready")
            loadRewardedAd()
            completion(false, 0)
            return
        }
        
        rewardedAd.present(fromRootViewController: rootViewController) { [weak self] in
            let reward = rewardedAd.adReward
            self?.logger.info("User earned reward: \(reward.amount) \(reward.type)")
            completion(true, reward.amount.intValue)
            self?.loadRewardedAd() // Preload next
        }
    }
    
    // Set ads removed status (after IAP purchase)
    func setAdsRemoved(_ removed: Bool) {
        adsRemoved = removed
        logger.info("Ads removed status: \(removed)")
    }
    
    private var adCompletionHandler: ((Bool) -> Void)?
}

// MARK: - GADFullScreenContentDelegate
extension AdManager: GADFullScreenContentDelegate {
    func adDidPresentFullScreenContent(_ ad: GADFullScreenPresentingAd) {
        logger.info("Ad presented")
    }
    
    func adDidDismissFullScreenContent(_ ad: GADFullScreenPresentingAd) {
        logger.info("Ad dismissed")
        adCompletionHandler?(true)
        adCompletionHandler = nil
        
        // Preload next ad
        if ad is GADInterstitialAd {
            loadInterstitialAd()
        }
    }
    
    func ad(_ ad: GADFullScreenPresentingAd, didFailToPresentFullScreenContentWithError error: Error) {
        logger.error("Ad failed to present: \(error.localizedDescription)")
        adCompletionHandler?(false)
        adCompletionHandler = nil
    }
}
```

### 3. C++ Bridge Functions

```cpp
// FloppyTurd/src/Engine/Platform/PlatformDelegates.h

struct AdDelegate {
    // Show interstitial ad (after game over)
    void (*showInterstitialAd)(void (*completion)(bool success));
    
    // Show rewarded video (for bonus coins)
    void (*showRewardedAd)(void (*completion)(bool success, int rewardAmount));
    
    // Check if ads are removed (IAP purchased)
    bool (*areAdsRemoved)();
    
    // Set ads removed status
    void (*setAdsRemoved)(bool removed);
};
```

### 4. C++ AdCounterSystem

```cpp
// FloppyTurd/src/FloppyTurd/Systems/AdCounterSystem.h

class AdCounterSystem {
public:
    AdCounterSystem();
    
    void IncrementGameOver();
    bool ShouldShowAd() const;
    void Reset();
    
    void SetAdFrequency(int frequency); // 3, 5, etc.
    int GetGameOverCount() const { return m_gameOverCount; }
    
private:
    int m_gameOverCount = 0;
    int m_adFrequency = 5; // Show ad every 5 game overs
    bool m_adsDisabled = false;
};
```

---

## "Remove Ads" IAP Integration

### StoreKit 2 Implementation

```swift
// FloppyTurd/src/iOS/InAppPurchase/RemoveAdsProduct.swift

import StoreKit

@MainActor
final class RemoveAdsManager: ObservableObject {
    static let shared = RemoveAdsManager()
    
    private let productID = "com.floppyturd.game.removeads"
    @Published private(set) var isPurchased = false
    
    func checkPurchaseStatus() async {
        // Check if already purchased
        if let result = await Transaction.currentEntitlement(for: productID) {
            isPurchased = true
            AdManager.shared.setAdsRemoved(true)
        }
    }
    
    func purchase() async throws {
        guard let product = try await Product.products(for: [productID]).first else {
            throw PurchaseError.productNotFound
        }
        
        let result = try await product.purchase()
        
        switch result {
        case .success(let verification):
            let transaction = try checkVerified(verification)
            await transaction.finish()
            
            isPurchased = true
            AdManager.shared.setAdsRemoved(true)
            
        case .userCancelled, .pending:
            break
            
        @unknown default:
            break
        }
    }
}
```

### UI Button in Main Menu

```swift
// Add to MainMenuState UI
Button("Remove Ads - $2.99") {
    Task {
        try? await RemoveAdsManager.shared.purchase()
    }
}
.disabled(RemoveAdsManager.shared.isPurchased)
```

---

## Ad Frequency Strategy

### Recommendation: Start Conservative

```
First 3 game overs: No ads (learning period)
Game overs 4-8: Ad every 5 deaths
Game overs 9+: Ad every 3 deaths

OR

Always: Ad every 5 deaths (safest)
```

### A/B Testing Plan

| Group A | Group B | Metric |
|---------|---------|--------|
| Ad every 3 deaths | Ad every 5 deaths | Retention Day 1 |
| Ad every 3 deaths | Ad every 5 deaths | Retention Day 7 |
| Ad every 3 deaths | Ad every 5 deaths | ARPDAU |
| Ad every 3 deaths | Ad every 5 deaths | IAP conversion |

### Monitoring
- If Day 1 retention drops > 5%, reduce frequency
- If ARPDAU is strong but retention weak, offer "Remove Ads" prominently
- Sweet spot is usually 4-5 game overs for casual arcade games

---

## Privacy & Compliance Checklist

### Required Implementations

- [ ] Add ATT (App Tracking Transparency) prompt
  - Show on first launch
  - Explain why (better ads, support development)
  - Respect "Do Not Track" choice

- [ ] Privacy Policy
  - Mention ad providers (AdMob, etc.)
  - Explain data collection
  - Link in App Store listing
  - Link in app settings

- [ ] COPPA Compliance (if targeting kids)
  - Enable child-directed treatment
  - Disable personalized ads
  - Use family-safe content only

- [ ] GDPR Compliance
  - Use Google's UMP SDK (User Messaging Platform)
  - Show consent dialog in EU
  - Allow users to change consent

### App Store Privacy Declarations

```
Data Used to Track You:
- [x] Identifiers (for advertising)

Data Linked to You:
- [x] Usage Data (ad interactions)

Data Not Linked to You:
- [x] Crash Data
- [x] Performance Data
```

---

## Final Recommendation for FloppyTurd

### Start With: Google AdMob

**Reasoning:**
1. ✅ **Family-friendly focus** - Best content controls
2. ✅ **Easy integration** - Best for indie developers
3. ✅ **Non-personalized ads** - Privacy-first approach
4. ✅ **Reliable revenue** - Good for casual arcade games
5. ✅ **Great documentation** - Swift examples and support

### Implementation Timeline

**Week 1:** AdMob Integration
- Setup AdMob account
- Integrate SDK via Swift Package Manager
- Implement AdManager.swift
- Add C++ bridge functions
- Test with test ad units

**Week 2:** Game Integration
- Create AdCounterSystem
- Add ad trigger logic (every 5 game overs)
- Implement "Remove Ads" IAP
- Add ATT prompt
- Test with real ad units

**Week 3:** Polish & Testing
- A/B test ad frequency
- Monitor retention metrics
- Optimize ad placement timing
- Test edge cases (no internet, ad fails, etc.)

### Revenue Projection

**Conservative Estimate (1000 DAU):**
- 1000 users × 3 sessions/day = 3000 sessions
- 3000 sessions × 0.6 ads/session = 1800 ad impressions/day
- 1800 impressions × $4 CPM = $7.20/day
- **$216/month** base revenue

**With Growth (5000 DAU):**
- **$1,080/month** ad revenue
- Plus "Remove Ads" IAP purchases
- Total potential: **$1,200-1,500/month**

This is realistic for a well-designed casual arcade game with good retention.

---

## Next Steps

1. **Create AdMob account** at https://apps.admob.com/
2. **Register FloppyTurd app** in AdMob console
3. **Get Ad Unit IDs** for interstitial and rewarded ads
4. **Set up test devices** for development
5. **Integrate SDK** using Swift Package Manager
6. **Implement AdManager.swift** (code above)
7. **Test with test ads** before going live

Once ads are working and revenue is flowing, consider adding Unity Ads via AdMob Mediation for 20-30% revenue boost.

---

**Remember:** Always prioritize user experience over revenue. A fun game that respects players will earn more long-term than an ad-heavy game that drives users away.

Good luck with FloppyTurd! 💩🚀