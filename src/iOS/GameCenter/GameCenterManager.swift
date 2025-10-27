//
//  GameCenterManager.swift
//  FloppyTurd
//
//  Created by Carl the Code-Conjuring Turdsmith
//  Game Center integration for leaderboards and achievements
//

import Foundation
import GameKit

/// @brief Singleton manager for Game Center authentication and leaderboard submission
/// This class handles all Game Center operations using native Swift/GameKit APIs
@MainActor
class GameCenterManager: NSObject {

    // MARK: - Singleton

    static let shared = GameCenterManager()

    // MARK: - Properties

    private(set) var isAuthenticated: Bool = false
    private(set) var localPlayer: GKLocalPlayer?

    /// View controller for presenting Game Center UI
    weak var viewController: UIViewController?

    // MARK: - Initialization

    private override init() {
        super.init()
        localPlayer = GKLocalPlayer.local
    }

    // MARK: - Authentication

    /// Authenticate the local player with Game Center
    /// - Parameter completion: Callback with authentication result
    func authenticate(completion: @escaping (Bool, Error?) -> Void) {
        guard let localPlayer = localPlayer else {
            print("⚠️ [GameCenter] Local player not available")
            completion(
                false,
                NSError(
                    domain: "GameCenter", code: -1,
                    userInfo: [NSLocalizedDescriptionKey: "Local player not available"]))
            return
        }

        print("🎮 [GameCenter] Starting authentication...")

        localPlayer.authenticateHandler = { [weak self] viewController, error in
            guard let self = self else { return }

            if let error = error {
                print("❌ [GameCenter] Authentication error: \(error.localizedDescription)")
                self.isAuthenticated = false
                completion(false, error)
                return
            }

            if let viewController = viewController {
                // Present authentication view controller if needed
                print("📱 [GameCenter] Presenting authentication view controller")
                self.viewController?.present(viewController, animated: true)
                return
            }

            if localPlayer.isAuthenticated {
                print("✅ [GameCenter] Authentication successful")
                print("👤 [GameCenter] Player: \(localPlayer.displayName)")
                print("🆔 [GameCenter] Player ID: \(localPlayer.gamePlayerID)")
                self.isAuthenticated = true
                completion(true, nil)
            } else {
                print("⚠️ [GameCenter] Authentication failed - player not authenticated")
                self.isAuthenticated = false
                completion(
                    false,
                    NSError(
                        domain: "GameCenter", code: -2,
                        userInfo: [NSLocalizedDescriptionKey: "Player not authenticated"]))
            }
        }
    }

    // MARK: - Leaderboard Submission

    /// Submit a score to a specific leaderboard
    /// - Parameters:
    ///   - score: The score value to submit
    ///   - leaderboardID: The App Store Connect leaderboard identifier
    ///   - completion: Callback with submission result
    func submitScore(
        _ score: Int64, leaderboardID: String, completion: @escaping (Bool, Error?) -> Void
    ) {
        guard isAuthenticated else {
            print("⚠️ [GameCenter] Cannot submit score - not authenticated")
            completion(
                false,
                NSError(
                    domain: "GameCenter", code: -3,
                    userInfo: [NSLocalizedDescriptionKey: "Not authenticated"]))
            return
        }

        print("📊 [GameCenter] Submitting score: \(score) to leaderboard: \(leaderboardID)")

        Task {
            do {
                // iOS 14+ async API
                try await GKLeaderboard.submitScore(
                    Int(score),
                    context: 0,
                    player: localPlayer!,
                    leaderboardIDs: [leaderboardID]
                )
                print("✅ [GameCenter] Score submitted successfully")
                completion(true, nil)
            } catch {
                print("❌ [GameCenter] Failed to submit score: \(error.localizedDescription)")
                completion(false, error)
            }
        }
    }

    // MARK: - Leaderboard Display

    /// Show a specific leaderboard UI
    /// - Parameter leaderboardID: The App Store Connect leaderboard identifier
    func showLeaderboard(_ leaderboardID: String) {
        guard isAuthenticated else {
            print("⚠️ [GameCenter] Cannot show leaderboard - not authenticated")
            return
        }

        guard let viewController = viewController else {
            print("⚠️ [GameCenter] Cannot show leaderboard - no view controller set")
            return
        }

        print("📱 [GameCenter] Showing leaderboard: \(leaderboardID)")

        let gameCenterVC = GKGameCenterViewController(
            leaderboardID: leaderboardID, playerScope: .global, timeScope: .allTime)
        gameCenterVC.gameCenterDelegate = self

        viewController.present(gameCenterVC, animated: true)
    }

    /// Show all leaderboards UI
    func showAllLeaderboards() {
        guard isAuthenticated else {
            print("⚠️ [GameCenter] Cannot show leaderboards - not authenticated")
            return
        }

        guard let viewController = viewController else {
            print("⚠️ [GameCenter] Cannot show leaderboards - no view controller set")
            return
        }

        print("📱 [GameCenter] Showing all leaderboards")

        let gameCenterVC = GKGameCenterViewController(state: .leaderboards)
        gameCenterVC.gameCenterDelegate = self

        viewController.present(gameCenterVC, animated: true)
    }

    // MARK: - Player Info

    /// Get the authenticated player's display name
    /// - Returns: Player display name or nil if not authenticated
    func getPlayerName() -> String? {
        guard isAuthenticated, let localPlayer = localPlayer else {
            return nil
        }
        return localPlayer.displayName
    }

    /// Get the authenticated player's ID
    /// - Returns: Player ID or nil if not authenticated
    func getPlayerID() -> String? {
        guard isAuthenticated, let localPlayer = localPlayer else {
            return nil
        }
        return localPlayer.gamePlayerID
    }

    // MARK: - View Controller Management

    /// Set the view controller for presenting Game Center UI
    /// - Parameter vc: The view controller to use for presentation
    func setViewController(_ vc: UIViewController) {
        self.viewController = vc
        print("📱 [GameCenter] View controller set")
    }
}

// MARK: - GKGameCenterControllerDelegate

extension GameCenterManager: GKGameCenterControllerDelegate {
    nonisolated func gameCenterViewControllerDidFinish(
        _ gameCenterViewController: GKGameCenterViewController
    ) {
        print("📱 [GameCenter] Dismissing Game Center UI")
        Task { @MainActor in
            gameCenterViewController.dismiss(animated: true)
        }
    }
}
