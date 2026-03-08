//
//  AdManager.swift
//  PooperTrooper
//
//  Created by Carl the Code-Conjuring Turdsmith
//  AdMob interstitial advertising integration with preloading/caching
//  Background loading via Swift Actor for improved performance
//

import Foundation
import GameCorePlatform
import UIKit

#if !targetEnvironment(simulator)
import GoogleMobileAds
#endif

// MARK: - Ad Loading Error Types

/// Custom error types for ad loading operations
enum AdLoadingError: Error {
    case alreadyLoading
    case loadFailed(Error)
    case noAdAvailable
    case sdkNotInitialized
}

// =============================================================================
// SIMULATOR STUBS - Ads disabled on simulator to avoid XCFramework linking issues
// =============================================================================
#if targetEnvironment(simulator)

/// Stub AdManager for simulator builds (no actual ad loading)
@MainActor
class AdManager: NSObject {
    static let shared = AdManager()
    private(set) var adsEnabled: Bool = true
    weak var viewController: GameViewController?
    
    private override init() {
        super.init()
        SwiftLog.info("AdManager: SIMULATOR BUILD - ads disabled", category: "AdManager")
    }
    
    func preloadAd() {
        SwiftLog.info("AdManager: SIMULATOR - preloadAd() stub called", category: "AdManager")
        GameCore.setAdReadyState(false)
    }
    
    func showAd() {
        SwiftLog.info("AdManager: SIMULATOR - showAd() stub called", category: "AdManager")
    }
    
    func isAdReady() -> Bool {
        return false
    }
    
    func setAdsEnabled(_ enabled: Bool) {
        adsEnabled = enabled
        GameCore.setAdReadyState(false)
    }
    
    static func initializeSDK() {
        SwiftLog.info("AdManager: SIMULATOR - SDK initialization stub called", category: "AdManager")
    }
}

#else
// =============================================================================
// DEVICE IMPLEMENTATION - Full AdMob integration
// =============================================================================

// MARK: - Background Ad Loading Actor

/// Background actor for offloading ad loading from main thread
/// Communicates with main-thread AdManager via actor isolation
actor AdLoadingActor {
    // MARK: - Private State (actor-isolated, thread-safe by design)
    
    /// Cached ad ready for presentation
    private var cachedAd: InterstitialAd?
    
    /// Flag to prevent concurrent load operations
    private var isCurrentlyLoading: Bool = false
    
    /// AdMob ad unit ID
    private let adUnitID: String
    
    /// SDK initialization state
    private var isSDKInitialized: Bool = false
    
    // MARK: - Initialization
    
    init(adUnitID: String) {
        self.adUnitID = adUnitID
        SwiftLog.info("AdLoadingActor initialized with ad unit: \(adUnitID)")
    }
    
    // MARK: - SDK Initialization (runs on background thread)
    
    func initializeSDK() async {
        guard !isSDKInitialized else {
            SwiftLog.debug("SDK already initialized, skipping")
            return
        }
        
        let startTime = CFAbsoluteTimeGetCurrent()
        SwiftLog.info("⏱️ [PROFILE] AdLoadingActor: SDK initialization START (background thread)")
        
        // Initialize AdMob SDK on background thread
        await withCheckedContinuation { (continuation: CheckedContinuation<Void, Never>) in
            MobileAds.shared.start { status in
                let duration = (CFAbsoluteTimeGetCurrent() - startTime) * 1000.0
                SwiftLog.info("⏱️ [PROFILE] AdLoadingActor: SDK initialization COMPLETE - \(String(format: "%.2f", duration))ms")
                SwiftLog.info("AdMob SDK initialized on background thread - status: \(status.adapterStatusesByClassName)")
                continuation.resume()
            }
        }
        
        isSDKInitialized = true
    }
    
    // MARK: - Ad Loading (runs on background thread)
    
    /// Preload an interstitial ad on background thread
    /// - Returns: Loaded ad ready for presentation
    /// - Throws: AdLoadingError if load fails
    nonisolated func preloadAd() async throws -> InterstitialAd {
        // Load ad on background thread (actor's executor)
        // This is where the magic happens - network I/O runs off main thread
        let startTime = CFAbsoluteTimeGetCurrent()
        SwiftLog.info("⏱️ [PROFILE] AdLoadingActor: Ad preload START (background thread)")
        
        do {
            // Create ad request
            let request = Request()
            
            // Load ad (this happens on background)
            let ad = try await InterstitialAd.load(with: adUnitID, request: request)
            
            let duration = (CFAbsoluteTimeGetCurrent() - startTime) * 1000.0
            SwiftLog.info("⏱️ [PROFILE] AdLoadingActor: Ad preload COMPLETE - \(String(format: "%.2f", duration))ms")
            SwiftLog.info("✅ Ad loaded successfully on background thread")
            
            return ad
            
        } catch {
            let duration = (CFAbsoluteTimeGetCurrent() - startTime) * 1000.0
            SwiftLog.error("⏱️ [PROFILE] AdLoadingActor: Ad preload FAILED after \(String(format: "%.2f", duration))ms - \(error)")
            throw AdLoadingError.loadFailed(error)
        }
    }
    
    // MARK: - State Queries (thread-safe via actor isolation)
    
    /// Check if an ad is currently loaded and cached
    func hasLoadedAd() -> Bool {
        return cachedAd != nil
    }
    
    /// Retrieve and clear cached ad (one-time use)
    func retrieveAd() -> InterstitialAd? {
        let ad = cachedAd
        if ad != nil {
            SwiftLog.info("AdLoadingActor: Retrieved cached ad, clearing cache")
            cachedAd = nil
        }
        return ad
    }
    
    /// Clear cached ad without retrieving
    func clearCache() {
        if cachedAd != nil {
            SwiftLog.info("AdLoadingActor: Clearing cached ad")
            cachedAd = nil
        }
        isCurrentlyLoading = false
    }
}

// MARK: - Main Thread Ad Manager

/// @brief Singleton manager for AdMob interstitial ads with preloading and caching
/// This class handles all AdMob operations using the Google Mobile Ads SDK
/// Follows the same pattern as GameCenterManager for consistency
@MainActor
class AdManager: NSObject {

    // MARK: - Singleton

    static let shared = AdManager()

    // MARK: - Properties

    /// Whether ads are enabled (can be disabled via IAP "Remove Ads")
    private(set) var adsEnabled: Bool = true

    /// The currently loaded interstitial ad (cached and ready to show)
    /// This is the main-thread presentation copy
    private var interstitialAd: InterstitialAd?

    /// Whether an ad is currently being loaded
    private var isLoading: Bool = false

    /// View controller for presenting ads and controlling pause/resume
    weak var viewController: GameViewController?
    
    /// Background loading actor for off-main-thread ad loading
    private let loadingActor: AdLoadingActor

    // MARK: - Ad Unit IDs

    /// Test ad unit ID for development (shows test ads)
    private let testAdUnitID = "ca-app-pub-3940256099942544/4411468910"  // Google's test interstitial ID

    /// Production ad unit ID from AdMob
    private let productionAdUnitID = "ca-app-pub-2487109358798103/2326483309"  // Real Floppy Turd interstitial

    /// Real ad unit ID (uses test in debug, production in release)
    private var adUnitID: String {
        #if DEBUG
            return testAdUnitID
        #else
            return productionAdUnitID
        #endif
    }

    // MARK: - Initialization

    private override init() {
        // Initialize the background loading actor with ad unit ID
        #if DEBUG
        self.loadingActor = AdLoadingActor(adUnitID: testAdUnitID)
        #else
        self.loadingActor = AdLoadingActor(adUnitID: productionAdUnitID)
        #endif
        
        super.init()
        SwiftLog.info(
            "AdManager initialized with background loading actor - ready to serve interstitial ads", category: "AdManager")
    }

    // MARK: - Ad Lifecycle

    /// Preload an interstitial ad (call this after game init and after each ad shown)
    /// Now uses background thread via AdLoadingActor for improved performance
    func preloadAd() {
        SwiftLog.info("[AdManager] preloadAd() called", category: "AdManager")
        SwiftLog.info("   adsEnabled: \(adsEnabled)", category: "AdManager")
        SwiftLog.info("   isLoading: \(isLoading)", category: "AdManager")
        SwiftLog.info("   interstitialAd cached: \(interstitialAd != nil)", category: "AdManager")
        
        // Don't load if ads are disabled
        guard adsEnabled else {
            SwiftLog.warn("Ads are disabled - skipping preload", category: "AdManager")
            GameCore.setAdReadyState(false)
            return
        }

        // Don't load if already loading
        guard !isLoading else {
            SwiftLog.warn(
                "Ad is already loading - skipping duplicate preload", category: "AdManager")
            return
        }

        // Don't load if we already have a cached ad
        guard interstitialAd == nil else {
            SwiftLog.info("Ad already cached and ready - skipping preload", category: "AdManager")
            GameCore.setAdReadyState(true)
            return
        }

        // 🔍 PERFORMANCE PROFILING: Start timing
        let startTime = CFAbsoluteTimeGetCurrent()
        SwiftLog.info("⏱️ [PROFILE] AdManager: Preload START (main thread)", category: "AdManager")
        
        isLoading = true
        GameCore.setAdReadyState(false)

        // 🚀 NEW: Use Task.detached for background loading (off main thread)
        Task.detached(priority: .userInitiated) { [weak self] in
            guard let self = self else { return }
            
            let backgroundStartTime = CFAbsoluteTimeGetCurrent()
            SwiftLog.info("⏱️ [PROFILE] AdManager: Background task started", category: "AdManager")
            
            do {
                // Load ad on background thread via actor
                let ad = try await self.loadingActor.preloadAd()
                
                let backgroundDuration = (CFAbsoluteTimeGetCurrent() - backgroundStartTime) * 1000.0
                SwiftLog.info("⏱️ [PROFILE] AdManager: Background loading completed in \(String(format: "%.2f", backgroundDuration))ms", category: "AdManager")
                
                // Transfer to main thread for presentation setup (UIKit requirement)
                await MainActor.run { [weak self] in
                    guard let self = self else { return }
                    
                    let totalDuration = (CFAbsoluteTimeGetCurrent() - startTime) * 1000.0
                    
                    self.isLoading = false
                    self.interstitialAd = ad
                    self.interstitialAd?.fullScreenContentDelegate = self
                    
                    SwiftLog.info("✅ Ad loaded on background thread, transferred to main for presentation", category: "AdManager")
                    SwiftLog.info("⏱️ [PROFILE] AdManager: TOTAL time (including main thread transfer): \(String(format: "%.2f", totalDuration))ms", category: "AdManager")
                    SwiftLog.info("📊 [PERFORMANCE] Main thread freed during ad load - background handling saved ~\(String(format: "%.0f", backgroundDuration))ms", category: "AdManager")
                    
                    GameCore.setAdReadyState(true)
                }
            } catch {
                let errorDuration = (CFAbsoluteTimeGetCurrent() - startTime) * 1000.0
                
                await MainActor.run { [weak self] in
                    guard let self = self else { return }
                    
                    self.isLoading = false
                    self.interstitialAd = nil
                    
                    SwiftLog.error(
                        "Failed to load ad on background thread: \(error)",
                        category: "AdManager"
                    )
                    SwiftLog.error("⏱️ [PROFILE] AdManager: Ad load FAILED after \(String(format: "%.2f", errorDuration))ms", category: "AdManager")
                    
                    GameCore.setAdReadyState(false)
                }
            }
        }
    }

    /// Show the preloaded interstitial ad
    func showAd() {
        SwiftLog.info("[AdManager] showAd() called", category: "AdManager")
        SwiftLog.info("   adsEnabled: \(adsEnabled)", category: "AdManager")
        SwiftLog.info("   interstitialAd cached: \(interstitialAd != nil)", category: "AdManager")
        SwiftLog.info("   viewController set: \(viewController != nil)", category: "AdManager")
        
        // Don't show if ads are disabled
        guard adsEnabled else {
            SwiftLog.warn("Ads are disabled - skipping show", category: "AdManager")
            return
        }

        // Check if we have a cached ad ready
        guard let interstitialAd = interstitialAd else {
            SwiftLog.warn("No ad loaded - cannot show. Preloading now...", category: "AdManager")
            preloadAd()  // Preload for next time
            return
        }

        // Check if we have a view controller to present from
        guard let viewController = viewController else {
            SwiftLog.error("No view controller set - cannot present ad", category: "AdManager")
            return
        }

        SwiftLog.info("Showing interstitial ad...", category: "AdManager")

        // Present the ad
        interstitialAd.present(from: viewController)

        // Clear the cached ad (it will be reloaded after dismissal)
        self.interstitialAd = nil
        GameCore.setAdReadyState(false)
    }

    /// Check if an ad is ready to be shown
    func isAdReady() -> Bool {
        let ready = adsEnabled && interstitialAd != nil
        SwiftLog.info(
            "Ad ready check: \(ready) (enabled: \(adsEnabled), cached: \(interstitialAd != nil))",
            category: "AdManager"
        )
        return ready
    }

    /// Enable or disable ads (e.g., after IAP "Remove Ads" purchase)
    func setAdsEnabled(_ enabled: Bool) {
        SwiftLog.info("Setting ads enabled: \(enabled)", category: "AdManager")
        adsEnabled = enabled

        // If ads are being disabled, clear any cached ad
        if !enabled {
            interstitialAd = nil
            isLoading = false
            GameCore.setAdReadyState(false)
        } else {
            // Update C++ with current state
            GameCore.setAdReadyState(interstitialAd != nil)
        }
    }

    // MARK: - SDK Initialization

    /// Initialize the Google Mobile Ads SDK on background thread
    /// Call this from AppDelegate or GameViewController on app launch
    static func initializeSDK() {
        let startTime = CFAbsoluteTimeGetCurrent()
        SwiftLog.info("⏱️ [PROFILE] AdManager: SDK initialization START (main thread dispatch)", category: "AdManager")
        
        // Configure COPPA compliance for family-friendly content (9+ rating)
        let requestConfiguration = MobileAds.shared.requestConfiguration
        requestConfiguration.maxAdContentRating = .general  // Family-friendly ads only
        // Note: Uncomment the line below if specifically targeting children under 13
        // requestConfiguration.tag(forChildDirectedTreatment: true)
        
        SwiftLog.info("✅ AdMob configured for family-friendly content (General rating)", category: "AdManager")
        
        // 🚀 NEW: Use Task.detached to initialize SDK on background thread
        Task.detached(priority: .userInitiated) {
            let backgroundStartTime = CFAbsoluteTimeGetCurrent()
            SwiftLog.info("⏱️ [PROFILE] AdManager: SDK initialization moved to background thread", category: "AdManager")
            
            // Initialize SDK on background via the shared actor
            await AdManager.shared.loadingActor.initializeSDK()
            
            let duration = (CFAbsoluteTimeGetCurrent() - backgroundStartTime) * 1000.0
            let totalDuration = (CFAbsoluteTimeGetCurrent() - startTime) * 1000.0
            
            SwiftLog.info("✅ Google Mobile Ads SDK initialized on background thread", category: "AdManager")
            SwiftLog.info("⏱️ [PROFILE] AdManager: SDK init background time: \(String(format: "%.2f", duration))ms", category: "AdManager")
            SwiftLog.info("⏱️ [PROFILE] AdManager: SDK init TOTAL time: \(String(format: "%.2f", totalDuration))ms", category: "AdManager")
            SwiftLog.info("📊 [PERFORMANCE] Main thread freed during SDK init - background handling saved ~\(String(format: "%.0f", duration))ms", category: "AdManager")
        }
    }
}

// MARK: - FullScreenContentDelegate

extension AdManager: FullScreenContentDelegate {

    /// Tells the delegate that an impression has been recorded for the ad.
    func adDidRecordImpression(_ ad: FullScreenPresentingAd) {
        SwiftLog.info("Ad recorded impression", category: "AdManager")
    }

    /// Tells the delegate that a click has been recorded for the ad.
    func adDidRecordClick(_ ad: FullScreenPresentingAd) {
        SwiftLog.info("Ad recorded click", category: "AdManager")
    }

    /// Tells the delegate that the ad failed to present full screen content.
    func ad(
        _ ad: FullScreenPresentingAd, didFailToPresentFullScreenContentWithError error: Error
    ) {
        SwiftLog.error("Ad failed to present: \(error.localizedDescription)", category: "AdManager")

        // Clear the failed ad and preload a new one
        interstitialAd = nil
        GameCore.setAdReadyState(false)
        preloadAd()
    }

    /// Tells the delegate that the ad will present full screen content.
    func adWillPresentFullScreenContent(_ ad: FullScreenPresentingAd) {
        SwiftLog.info("Ad will present full screen content", category: "AdManager")

        // Pause the game while the ad is showing via GameViewController
        if let vc = viewController {
            vc.pauseGameForAd()
            SwiftLog.info(
                "Game paused for ad presentation via GameViewController", category: "AdManager")
        }
    }

    /// Tells the delegate that the ad will dismiss full screen content.
    func adWillDismissFullScreenContent(_ ad: FullScreenPresentingAd) {
        SwiftLog.info("Ad will dismiss full screen content", category: "AdManager")
    }

    /// Tells the delegate that the ad dismissed full screen content.
    func adDidDismissFullScreenContent(_ ad: FullScreenPresentingAd) {
        SwiftLog.info("Ad dismissed - user closed the ad", category: "AdManager")

        // Clear the shown ad
        interstitialAd = nil
        GameCore.setAdReadyState(false)

        // Resume the game after the ad is dismissed via GameViewController
        if let vc = viewController {
            vc.resumeGameFromAd()
            SwiftLog.info(
                "Game resumed after ad dismissal via GameViewController", category: "AdManager")
        }

        // Preload the next ad immediately for zero-latency next time
        SwiftLog.info("Preloading next ad...", category: "AdManager")
        preloadAd()
    }
}

#endif // !targetEnvironment(simulator)
