
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
            print("[HapticsManagerSwift] Failed to start haptic engine: \(error)")
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
}
