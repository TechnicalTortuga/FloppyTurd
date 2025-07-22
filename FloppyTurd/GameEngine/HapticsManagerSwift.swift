
import Foundation
import CoreHaptics
import UIKit

@MainActor
public final class HapticsManagerSwift {
    nonisolated public static let shared: HapticsManagerSwift = {
        return MainActor.assumeIsolated {
            return HapticsManagerSwift()
        }
    }()
    private var engine: CHHapticEngine?

    private init() {
        prepareEngine()
    }

    private func prepareEngine() {
        guard CHHapticEngine.capabilitiesForHardware().supportsHaptics else { return }
        do {
            engine = try CHHapticEngine()
            try engine?.start()
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[HapticsManagerSwift] ❌ Haptic engine error: \(error)")
        }
    }

    public func playImpact(style: UIImpactFeedbackGenerator.FeedbackStyle = .medium) {
        let generator = UIImpactFeedbackGenerator(style: style)
        generator.prepare()
        generator.impactOccurred()
    }

    public func playNotification(type: UINotificationFeedbackGenerator.FeedbackType = .success) {
        let generator = UINotificationFeedbackGenerator()
        generator.prepare()
        generator.notificationOccurred(type)
    }

    public func playSelection() {
        let generator = UISelectionFeedbackGenerator()
        generator.prepare()
        generator.selectionChanged()
    }
    
    // MARK: - C++ Interop Functions
    
    /// Trigger haptic feedback with type - C++ compatible
    nonisolated public static func triggerHapticFeedback(_ type: Int32) {
        Task { @MainActor in
            switch type {
            case 0: // Light impact
                HapticsManagerSwift.shared.playImpact(style: .light)
            case 1: // Medium impact
                HapticsManagerSwift.shared.playImpact(style: .medium)
            case 2: // Heavy impact
                HapticsManagerSwift.shared.playImpact(style: .heavy)
            case 3: // Success notification
                HapticsManagerSwift.shared.playNotification(type: .success)
            case 4: // Warning notification
                HapticsManagerSwift.shared.playNotification(type: .warning)
            case 5: // Error notification
                HapticsManagerSwift.shared.playNotification(type: .error)
            case 6: // Selection
                HapticsManagerSwift.shared.playSelection()
            default:
                HapticsManagerSwift.shared.playImpact(style: .medium)
            }
        }
    }
    
    /// Vibrate device for specified milliseconds - C++ compatible
    nonisolated public static func vibrate(_ milliseconds: Int32) {
        Task { @MainActor in
            // For iOS, we use haptic feedback instead of traditional vibration
            // Convert milliseconds to appropriate haptic feedback
            if milliseconds < 100 {
                HapticsManagerSwift.shared.playImpact(style: .light)
            } else if milliseconds < 300 {
                HapticsManagerSwift.shared.playImpact(style: .medium)
            } else {
                HapticsManagerSwift.shared.playImpact(style: .heavy)
            }
        }
    }
    
    /// Play impact feedback - C++ compatible
    nonisolated public static func playImpactFeedback(_ intensity: Float) {
        Task { @MainActor in
            let style: UIImpactFeedbackGenerator.FeedbackStyle
            if intensity < 0.3 {
                style = .light
            } else if intensity < 0.7 {
                style = .medium
            } else {
                style = .heavy
            }
            HapticsManagerSwift.shared.playImpact(style: style)
        }
    }
}
