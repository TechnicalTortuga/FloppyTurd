//
//  AdManager.swift
//  FloppyTurd
//
//  Created by Carl the Code-Conjuring Turdsmith
//  AdMob interstitial advertising integration with preloading/caching
//

import Foundation
import GameCorePlatform
import GoogleMobileAds
import UIKit

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
    private var interstitialAd: InterstitialAd?

    /// Whether an ad is currently being loaded
    private var isLoading: Bool = false

    /// View controller for presenting ads and controlling pause/resume
    weak var viewController: GameViewController?

    // MARK: - Ad Unit IDs

    /// Test ad unit ID for development (shows test ads)
    /// Replace with your real ad unit ID before releasing to TestFlight/App Store
    private let testAdUnitID = "ca-app-pub-3940256099942544/4411468910"  // Google's test interstitial ID

    /// Real ad unit ID (set this when you have your real AdMob ad unit)
    /// For now, we use the test ID
    private var adUnitID: String {
        #if DEBUG
            return testAdUnitID
        #else
            // TODO: Replace with your real ad unit ID from AdMob console
            // Example: "ca-app-pub-XXXXXXXXXXXXXXXX/YYYYYYYYYY"
            return testAdUnitID  // Using test ID for now
        #endif
    }

    // MARK: - Initialization

    private override init() {
        super.init()
        SwiftLog.info(
            "AdManager initialized - ready to serve interstitial ads", category: "AdManager")
    }

    // MARK: - Ad Lifecycle

    /// Preload an interstitial ad (call this after game init and after each ad shown)
    func preloadAd() {
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

        SwiftLog.info("Preloading interstitial ad...", category: "AdManager")
        isLoading = true
        GameCore.setAdReadyState(false)

        let request = Request()

        // Load interstitial ad asynchronously
        Task {
            do {
                let ad = try await InterstitialAd.load(with: adUnitID, request: request)

                await MainActor.run {
                    self.isLoading = false
                    self.interstitialAd = ad
                    self.interstitialAd?.fullScreenContentDelegate = self
                    SwiftLog.info("Interstitial ad preloaded successfully", category: "AdManager")
                    GameCore.setAdReadyState(true)
                }
            } catch {
                await MainActor.run {
                    self.isLoading = false
                    self.interstitialAd = nil
                    SwiftLog.error(
                        "Failed to load interstitial ad: \(error.localizedDescription)",
                        category: "AdManager"
                    )
                    GameCore.setAdReadyState(false)
                }
            }
        }
    }

    /// Show the preloaded interstitial ad
    func showAd() {
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

    /// Initialize the Google Mobile Ads SDK
    /// Call this from AppDelegate or GameViewController on app launch
    static func initializeSDK() {
        SwiftLog.info("Initializing Google Mobile Ads SDK...", category: "AdManager")

        MobileAds.shared.start { status in
            SwiftLog.info("Google Mobile Ads SDK initialized", category: "AdManager")
            SwiftLog.info("Adapter statuses:", category: "AdManager")
            for adapter in status.adapterStatusesByClassName.values {
                SwiftLog.debug("  - \(adapter.description)", category: "AdManager")
            }
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
