//
//  HapticManager.swift
//  FloppyTurd
//
//  iOS Haptic Feedback System - Thread-safe singleton manager for haptic feedback
//

import CoreHaptics
import UIKit

/// Log levels for haptic manager
enum HapticLogLevel: String {
    case trace = "TRACE"
    case debug = "DEBUG"
    case info = "INFO"
    case warn = "WARN"
    case error = "ERROR"
}

/// Haptic Manager - Centralized haptic feedback system
/// Manages UIFeedbackGenerator (iOS 10+) and Core Haptics (iOS 13+)
/// Thread-safe singleton for game-wide haptic feedback
@MainActor
class HapticManager {

    // MARK: - Singleton
    static let shared = HapticManager()

    // MARK: - UIFeedbackGenerator Storage
    private var impactGenerators:
        [UIImpactFeedbackGenerator.FeedbackStyle: UIImpactFeedbackGenerator] = [:]
    private var selectionGenerator: UISelectionFeedbackGenerator?
    private var notificationGenerator: UINotificationFeedbackGenerator?

    // MARK: - Core Haptics
    private var hapticEngine: CHHapticEngine?
    private var patternPlayers: [String: CHHapticPatternPlayer] = [:]

    // MARK: - State
    private var isEnabled: Bool = true
    private var isSupported: Bool = false

    // MARK: - Initialization
    private init() {
        log("Initializing HapticManager", level: .info)
        isSupported = checkDeviceSupport()

        if isSupported {
            setupGenerators()
            setupCoreHaptics()
        } else {
            log("Haptic feedback not supported on this device", level: .warn)
        }
    }

    // MARK: - Device Support
    private func checkDeviceSupport() -> Bool {
        // Check if device supports haptics
        let supportsHaptics = CHHapticEngine.capabilitiesForHardware().supportsHaptics
        log("Device haptic support: \(supportsHaptics)", level: .info)
        return supportsHaptics
    }

    // MARK: - Setup
    private func setupGenerators() {
        log("Setting up UIFeedbackGenerators", level: .debug)

        // Create impact generators for all styles
        impactGenerators[.light] = UIImpactFeedbackGenerator(style: .light)
        impactGenerators[.medium] = UIImpactFeedbackGenerator(style: .medium)
        impactGenerators[.heavy] = UIImpactFeedbackGenerator(style: .heavy)

        if #available(iOS 13.0, *) {
            impactGenerators[.rigid] = UIImpactFeedbackGenerator(style: .rigid)
            impactGenerators[.soft] = UIImpactFeedbackGenerator(style: .soft)
        }

        // Create selection and notification generators
        selectionGenerator = UISelectionFeedbackGenerator()
        notificationGenerator = UINotificationFeedbackGenerator()

        // Prepare all generators
        impactGenerators.values.forEach { $0.prepare() }
        selectionGenerator?.prepare()
        notificationGenerator?.prepare()

        log("UIFeedbackGenerators setup complete", level: .debug)
    }

    private func setupCoreHaptics() {
        guard #available(iOS 13.0, *) else {
            log("Core Haptics requires iOS 13.0+", level: .warn)
            return
        }

        do {
            hapticEngine = try CHHapticEngine()

            // Setup reset handler
            hapticEngine?.resetHandler = { [weak self] in
                self?.log("Haptic engine reset", level: .warn)
                do {
                    try self?.hapticEngine?.start()
                } catch {
                    self?.log(
                        "Failed to restart haptic engine: \(error.localizedDescription)",
                        level: .error)
                }
            }

            // Setup stopped handler
            hapticEngine?.stoppedHandler = { [weak self] reason in
                self?.log("Haptic engine stopped: \(reason.rawValue)", level: .warn)
            }

            // Start the engine
            try hapticEngine?.start()
            log("Core Haptics engine started", level: .info)

            // Preload common patterns
            preloadCommonPatterns()

        } catch {
            log("Failed to setup Core Haptics: \(error.localizedDescription)", level: .error)
            hapticEngine = nil
        }
    }

    // MARK: - Impact Feedback
    func triggerImpact(style: UIImpactFeedbackGenerator.FeedbackStyle, intensity: CGFloat = 1.0) {
        guard isEnabled && isSupported else { return }

        DispatchQueue.main.async { [weak self] in
            if let generator = self?.impactGenerators[style] {
                generator.impactOccurred(intensity: intensity)
                // Re-prepare for next use
                generator.prepare()
            }
        }
    }

    // MARK: - Selection Feedback
    func triggerSelection() {
        guard isEnabled && isSupported else { return }

        DispatchQueue.main.async { [weak self] in
            self?.selectionGenerator?.selectionChanged()
            self?.selectionGenerator?.prepare()
        }
    }

    // MARK: - Notification Feedback
    func triggerNotification(type: UINotificationFeedbackGenerator.FeedbackType) {
        guard isEnabled && isSupported else { return }

        DispatchQueue.main.async { [weak self] in
            self?.notificationGenerator?.notificationOccurred(type)
            self?.notificationGenerator?.prepare()
        }
    }

    // MARK: - Preparation
    func prepare(style: UIImpactFeedbackGenerator.FeedbackStyle) {
        guard isEnabled && isSupported else { return }

        DispatchQueue.main.async { [weak self] in
            self?.impactGenerators[style]?.prepare()
        }
    }

    func prepareSelection() {
        guard isEnabled && isSupported else { return }

        DispatchQueue.main.async { [weak self] in
            self?.selectionGenerator?.prepare()
        }
    }

    func prepareNotification() {
        guard isEnabled && isSupported else { return }

        DispatchQueue.main.async { [weak self] in
            self?.notificationGenerator?.prepare()
        }
    }

    // MARK: - Pattern Playback
    func triggerPattern(name: String) {
        guard isEnabled && isSupported else { return }
        guard #available(iOS 13.0, *) else { return }

        DispatchQueue.main.async { [weak self] in
            guard let self = self else { return }

            if let player = self.patternPlayers[name] {
                do {
                    try player.start(atTime: CHHapticTimeImmediate)
                    self.log("Triggered haptic pattern: \(name)", level: .debug)
                } catch {
                    self.log(
                        "Failed to play pattern '\(name)': \(error.localizedDescription)",
                        level: .error)
                }
            } else {
                self.log("Pattern '\(name)' not found", level: .warn)
            }
        }
    }

    // MARK: - Pattern Loading
    private func preloadCommonPatterns() {
        guard #available(iOS 13.0, *) else { return }

        do {
            try preloadBossDeathPattern()
            try preloadLevelUnlockPattern()
            try preloadHatUnlockPattern()
            log("Common haptic patterns preloaded", level: .info)
        } catch {
            log("Failed to preload patterns: \(error.localizedDescription)", level: .error)
        }
    }

    @available(iOS 13.0, *)
    private func preloadBossDeathPattern() throws {
        // Boss Death Pattern: Dramatic escalating haptic sequence
        // Phase 1: Warning tremors (0-0.5s)
        let warningEvents: [CHHapticEvent] = [
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.3),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.3),
                ], relativeTime: 0.0),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.4),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.4),
                ], relativeTime: 0.15),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.5),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5),
                ], relativeTime: 0.35),
        ]

        // Phase 2: Massive impact (0.5s)
        let impactEvent = CHHapticEvent(
            eventType: .hapticTransient,
            parameters: [
                CHHapticEventParameter(parameterID: .hapticIntensity, value: 1.0),
                CHHapticEventParameter(parameterID: .hapticSharpness, value: 1.0),
            ], relativeTime: 0.5)

        // Phase 3: Rumble aftermath (0.55-1.2s)
        let rumbleEvent = CHHapticEvent(
            eventType: .hapticContinuous,
            parameters: [
                CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.8),
                CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5),
            ], relativeTime: 0.55, duration: 0.65)

        // Phase 4: Decay pulses (1.2-2.0s)
        let decayEvents: [CHHapticEvent] = [
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.4),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.3),
                ], relativeTime: 1.2),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.25),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.2),
                ], relativeTime: 1.5),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.15),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.1),
                ], relativeTime: 1.8),
        ]

        var allEvents = warningEvents
        allEvents.append(impactEvent)
        allEvents.append(rumbleEvent)
        allEvents.append(contentsOf: decayEvents)

        let pattern = try CHHapticPattern(events: allEvents, parameters: [])
        let player = try hapticEngine?.makePlayer(with: pattern)
        patternPlayers["boss_death"] = player

        log("Boss death pattern loaded", level: .debug)
    }

    @available(iOS 13.0, *)
    private func preloadLevelUnlockPattern() throws {
        // Level Unlock: Celebratory ascending pattern
        let events: [CHHapticEvent] = [
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.5),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5),
                ], relativeTime: 0.0),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.7),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.7),
                ], relativeTime: 0.1),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.9),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.9),
                ], relativeTime: 0.2),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 1.0),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 1.0),
                ], relativeTime: 0.3),
        ]

        let pattern = try CHHapticPattern(events: events, parameters: [])
        let player = try hapticEngine?.makePlayer(with: pattern)
        patternPlayers["level_unlock"] = player

        log("Level unlock pattern loaded", level: .debug)
    }

    @available(iOS 13.0, *)
    private func preloadHatUnlockPattern() throws {
        // Hat Unlock: Playful double-tap pattern
        let events: [CHHapticEvent] = [
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.7),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.8),
                ], relativeTime: 0.0),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.7),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.8),
                ], relativeTime: 0.1),
            CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.9),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 1.0),
                ], relativeTime: 0.3),
        ]

        let pattern = try CHHapticPattern(events: events, parameters: [])
        let player = try hapticEngine?.makePlayer(with: pattern)
        patternPlayers["hat_unlock"] = player

        log("Hat unlock pattern loaded", level: .debug)
    }

    // MARK: - Enable/Disable
    func setEnabled(_ enabled: Bool) {
        isEnabled = enabled
        log("Haptic feedback \(enabled ? "enabled" : "disabled")", level: .info)

        if !enabled {
            // Stop any playing patterns
            if #available(iOS 13.0, *) {
                patternPlayers.values.forEach { player in
                    do {
                        try player.stop(atTime: CHHapticTimeImmediate)
                    } catch {
                        log("Failed to stop pattern: \(error.localizedDescription)", level: .error)
                    }
                }
            }
        }
    }

    func getEnabled() -> Bool {
        return isEnabled
    }

    func getSupported() -> Bool {
        return isSupported
    }

    // MARK: - Logging
    private func log(_ message: String, level: HapticLogLevel) {
        #if DEBUG
            let timestamp = Date()
            let formatter = DateFormatter()
            formatter.dateFormat = "HH:mm:ss.SSS"
            let timeString = formatter.string(from: timestamp)

            let logMessage = "[\(timeString)] [HapticManager] [\(level.rawValue)] \(message)"
            print(logMessage)
        #endif
    }

    // MARK: - Cleanup
    // Note: deinit removed because @MainActor isolated classes cannot safely
    // access their properties from deinit. Cleanup will happen automatically
    // when the singleton is deallocated (which is effectively never for a singleton).
    // If explicit cleanup is needed, add a shutdown() method and call it manually.
}

// MARK: - Convenience Methods
extension HapticManager {

    /// Trigger light impact (UI interactions, light taps)
    func light() {
        triggerImpact(style: .light)
    }

    /// Trigger medium impact (collisions, moderate events)
    func medium() {
        triggerImpact(style: .medium)
    }

    /// Trigger heavy impact (boss attacks, major events)
    func heavy() {
        triggerImpact(style: .heavy)
    }

    /// Trigger rigid impact (sharp, precise feedback)
    @available(iOS 13.0, *)
    func rigid() {
        triggerImpact(style: .rigid)
    }

    /// Trigger soft impact (gentle, cushioned feedback)
    @available(iOS 13.0, *)
    func soft() {
        triggerImpact(style: .soft)
    }

    /// Trigger selection feedback (UI navigation)
    func selection() {
        triggerSelection()
    }

    /// Trigger success notification
    func success() {
        triggerNotification(type: .success)
    }

    /// Trigger warning notification
    func warning() {
        triggerNotification(type: .warning)
    }

    /// Trigger error notification
    func error() {
        triggerNotification(type: .error)
    }
}
