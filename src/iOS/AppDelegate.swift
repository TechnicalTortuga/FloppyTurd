//
//  AppDelegate.swift
//  FloppyTurd
//
//  iOS Application Delegate using Swift 5.9+ native C++ interop
//  Entry point for FloppyTurd iOS app with direct C++ integration
//  Handles app lifecycle events and crash reporting setup
//
//  With Swift 5.9+, this class can be directly accessed from C++ if needed:
//  auto appDelegate = std::make_unique<FloppyTurd::AppDelegate>();
//

import UIKit
import os.log

@main
class AppDelegate: UIResponder, UIApplicationDelegate {

    // MARK: - Properties

    var window: UIWindow?
    private let logger = Logger(subsystem: "com.floppyturd.ios", category: "AppDelegate")

    // MARK: - Application Lifecycle

    @MainActor
    func application(
        _ application: UIApplication,
        didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
    ) -> Bool {
        logger.info("FloppyTurd app launching...")

        // Setup crash reporting (nonisolated)
        Task.detached { [weak self] in
            self?.setupCrashReporting()
        }

        // Create main window
        window = UIWindow(frame: UIScreen.main.bounds)

        // Create and set root view controller
        let gameViewController = GameViewController()

        window?.rootViewController = gameViewController
        window?.makeKeyAndVisible()

        // CRITICAL: Force portrait orientation AFTER window is visible and viewDidLoad has completed
        // This ensures metalRenderer is initialized before attempting orientation lock
        logger.info("Window made visible, now locking to portrait orientation")
        gameViewController.lockToPortrait()
        logger.info("Locked to portrait orientation after window setup")

        // Enable device orientation monitoring
        UIDevice.current.beginGeneratingDeviceOrientationNotifications()
        logger.info("Device orientation monitoring enabled")

        logger.info("FloppyTurd app launched successfully")
        return true
    }

    @MainActor
    func applicationWillResignActive(_ application: UIApplication) {
        logger.info("App will resign active - pausing game")
        // Pause the game when app loses focus
        NotificationCenter.default.post(name: .gameWillPause, object: nil)
    }

    @MainActor
    func applicationDidEnterBackground(_ application: UIApplication) {
        logger.info("App entered background - saving state")
        // Save game state when entering background
        NotificationCenter.default.post(name: .gameShouldSave, object: nil)
    }

    @MainActor
    func applicationWillEnterForeground(_ application: UIApplication) {
        logger.info("App will enter foreground - preparing to resume")
        // Prepare to resume game
        NotificationCenter.default.post(name: .gameWillResume, object: nil)
    }

    @MainActor
    func applicationDidBecomeActive(_ application: UIApplication) {
        logger.info("App became active - resuming game")
        // Resume the game when app becomes active
        NotificationCenter.default.post(name: .gameDidResume, object: nil)
    }

    @MainActor
    func applicationWillTerminate(_ application: UIApplication) {
        logger.info("App will terminate - final save")
        // Final save before termination
        NotificationCenter.default.post(name: .gameShouldSave, object: nil)
    }

    // MARK: - Orientation Support

    @MainActor
    func application(
        _ application: UIApplication, supportedInterfaceOrientationsFor window: UIWindow?
    ) -> UIInterfaceOrientationMask {
        // Delegate to the game view controller for orientation decisions
        if let gameViewController = window?.rootViewController as? GameViewController {
            let supportedOrientations = gameViewController.supportedInterfaceOrientations
            logger.debug(
                "AppDelegate: Returning supported orientations: \(supportedOrientations.rawValue)")
            return supportedOrientations
        }

        // Fallback to all orientations if we can't find the game view controller
        logger.debug("AppDelegate: Using fallback orientation support (all orientations)")
        return .all
    }

    // MARK: - Private Methods

    nonisolated
        private func setupCrashReporting()
    {
        // TODO: Implement crash reporting setup
        // This could integrate with services like Crashlytics, Sentry, etc.
        print("[AppDelegate] Crash reporting setup - TODO: Implement")
    }
}

// MARK: - Notification Names

extension Notification.Name {
    static let gameWillPause = Notification.Name("gameWillPause")
    static let gameDidResume = Notification.Name("gameDidResume")
    static let gameWillResume = Notification.Name("gameWillResume")
    static let gameShouldSave = Notification.Name("gameShouldSave")
}
