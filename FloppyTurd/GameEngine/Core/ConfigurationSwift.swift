//
//  ConfigurationSwift.swift
//  FloppyTurd
//
//  Created by Carl the Code-Conjuring Turdsmith
//  Configuration management for C++ interop
//

import Foundation
import UIKit

// MARK: - Configuration Flags (matching Raylib's ConfigFlags)
public struct ConfigFlags: OptionSet, Sendable {
    public let rawValue: UInt32
    
    public init(rawValue: UInt32) {
        self.rawValue = rawValue
    }
    
    // Raylib-compatible flags
    public static let vsyncHint           = ConfigFlags(rawValue: 1 << 6)   // FLAG_VSYNC_HINT
    public static let fullscreenMode      = ConfigFlags(rawValue: 1 << 1)   // FLAG_FULLSCREEN_MODE
    public static let windowResizable     = ConfigFlags(rawValue: 1 << 2)   // FLAG_WINDOW_RESIZABLE
    public static let windowUndecorated   = ConfigFlags(rawValue: 1 << 3)   // FLAG_WINDOW_UNDECORATED
    public static let windowHidden        = ConfigFlags(rawValue: 1 << 7)   // FLAG_WINDOW_HIDDEN
    public static let windowMinimized     = ConfigFlags(rawValue: 1 << 9)   // FLAG_WINDOW_MINIMIZED
    public static let windowMaximized     = ConfigFlags(rawValue: 1 << 10)  // FLAG_WINDOW_MAXIMIZED
    public static let windowUnfocused     = ConfigFlags(rawValue: 1 << 11)  // FLAG_WINDOW_UNFOCUSED
    public static let windowTopmost       = ConfigFlags(rawValue: 1 << 12)  // FLAG_WINDOW_TOPMOST
    public static let windowAlwaysRun     = ConfigFlags(rawValue: 1 << 8)   // FLAG_WINDOW_ALWAYS_RUN
    public static let windowTransparent   = ConfigFlags(rawValue: 1 << 4)   // FLAG_WINDOW_TRANSPARENT
    public static let windowHighdpi       = ConfigFlags(rawValue: 1 << 13)  // FLAG_WINDOW_HIGHDPI
    public static let windowMousePassthrough = ConfigFlags(rawValue: 1 << 14) // FLAG_WINDOW_MOUSE_PASSTHROUGH
    public static let msaa4xHint          = ConfigFlags(rawValue: 1 << 5)   // FLAG_MSAA_4X_HINT
    public static let interlacedHint      = ConfigFlags(rawValue: 1 << 16)  // FLAG_INTERLACED_HINT
}

// MARK: - Configuration Management for C++ Interop
@_expose(Cxx)
public class ConfigurationSwift {
    
    nonisolated(unsafe) private static var currentFlags: ConfigFlags = []
    nonisolated(unsafe) private static var isInitialized = false
    
    // MARK: - Configuration Management
    @_expose(Cxx)
    nonisolated public static func setConfigFlags(_ flags: UInt32) {
        currentFlags = ConfigFlags(rawValue: flags)
        applyConfigFlags()
    }
    
    @_expose(Cxx)
    nonisolated public static func getConfigFlags() -> UInt32 {
        return currentFlags.rawValue
    }
    
    @_expose(Cxx)
    nonisolated public static func addConfigFlag(_ flag: UInt32) {
        currentFlags.insert(ConfigFlags(rawValue: flag))
        applyConfigFlags()
    }
    
    @_expose(Cxx)
    nonisolated public static func removeConfigFlag(_ flag: UInt32) {
        currentFlags.remove(ConfigFlags(rawValue: flag))
        applyConfigFlags()
    }
    
    @_expose(Cxx)
    nonisolated public static func hasConfigFlag(_ flag: UInt32) -> Bool {
        return currentFlags.contains(ConfigFlags(rawValue: flag))
    }
    
    // MARK: - iOS-Specific Configuration
    @_expose(Cxx)
    nonisolated public static func setPreferredFramesPerSecond(_ fps: Int32) {
        DispatchQueue.main.async {
            if let displayLink = getCurrentDisplayLink() {
                if #available(iOS 10.0, *) {
                    displayLink.preferredFramesPerSecond = Int(fps)
                } else {
                    // Fallback for older iOS versions
                    displayLink.frameInterval = max(1, 60 / Int(fps))
                }
            }
        }
    }
    
    @_expose(Cxx)
    nonisolated public static func setIdleTimerDisabled(_ disabled: Bool) {
        DispatchQueue.main.async {
            UIApplication.shared.isIdleTimerDisabled = disabled
        }
    }
    
    @_expose(Cxx)
    nonisolated public static func isIdleTimerDisabled() -> Bool {
        return MainActor.assumeIsolated {
            UIApplication.shared.isIdleTimerDisabled
        }
    }
    
    @_expose(Cxx)
    nonisolated public static func setStatusBarHidden(_ hidden: Bool) {
        DispatchQueue.main.async {
            if #available(iOS 13.0, *) {
                // iOS 13+ uses scene-based status bar management
                // This would need to be handled in the scene delegate
                traceLog(SWLogLevel.SWLOG_WARNING, "Status bar management on iOS 13+ requires scene delegate implementation")
            } else {
                UIApplication.shared.isStatusBarHidden = hidden
            }
        }
    }
    
    @_expose(Cxx)
    nonisolated public static func isStatusBarHidden() -> Bool {
        if #available(iOS 13.0, *) {
            // For iOS 13+, we'd need to check the scene's status bar
            return false // Default assumption
        } else {
            return MainActor.assumeIsolated {
                UIApplication.shared.isStatusBarHidden
            }
        }
    }
    
    // MARK: - Performance Configuration
    @_expose(Cxx)
    nonisolated public static func setLowPowerModeEnabled(_ enabled: Bool) {
        // iOS doesn't allow apps to directly control low power mode
        // This is a no-op, but we log the intent
        traceLog(SWLogLevel.SWLOG_INFO, "Low power mode control requested (enabled: \(enabled)) - iOS manages this automatically")
    }
    
    @_expose(Cxx)
    nonisolated public static func isLowPowerModeEnabled() -> Bool {
        return ProcessInfo.processInfo.isLowPowerModeEnabled
    }
    
    // MARK: - Private Helper Functions
    nonisolated private static func applyConfigFlags() {
        // Apply VSync hint
        if currentFlags.contains(.vsyncHint) {
            setPreferredFramesPerSecond(60) // Standard 60fps with VSync
        }
        
        // Apply idle timer configuration
        if currentFlags.contains(.windowAlwaysRun) {
            setIdleTimerDisabled(true)
        }
        
        // Apply status bar configuration
        if currentFlags.contains(.fullscreenMode) {
            setStatusBarHidden(true)
        }
        
        // Log applied configurations
        traceLog(SWLogLevel.SWLOG_INFO, "Applied config flags: 0x\(String(currentFlags.rawValue, radix: 16))")
    }
    
    nonisolated private static func getCurrentDisplayLink() -> CADisplayLink? {
        // This would need to be provided by the main game view
        // For now, return nil as we don't have direct access
        return nil
    }
    
    // MARK: - Initialization
    @_expose(Cxx)
    nonisolated public static func initialize() {
        guard !isInitialized else { return }
        
        // Set default configuration for iOS
        currentFlags = [.windowAlwaysRun, .vsyncHint]
        applyConfigFlags()
        
        isInitialized = true
        traceLog(SWLogLevel.SWLOG_INFO, "ConfigurationSwift initialized with default iOS settings")
    }
    
    @_expose(Cxx)
    nonisolated public static func shutdown() {
        currentFlags = []
        isInitialized = false
        traceLog(SWLogLevel.SWLOG_INFO, "ConfigurationSwift shutdown")
    }
}

// MARK: - Configuration Presets
extension ConfigurationSwift {
    
    /// Apply optimal settings for iOS gaming
    @_expose(Cxx)
    nonisolated public static func applyiOSGameSettings() {
        let gameFlags: ConfigFlags = [.windowAlwaysRun, .vsyncHint, .fullscreenMode]
        setConfigFlags(gameFlags.rawValue)
        traceLog(SWLogLevel.SWLOG_INFO, "Applied iOS game configuration preset")
    }
    
    /// Apply settings for development/debugging
    @_expose(Cxx)
    nonisolated public static func applyDevelopmentSettings() {
        let devFlags: ConfigFlags = [.windowAlwaysRun, .vsyncHint]
        setConfigFlags(devFlags.rawValue)
        traceLog(SWLogLevel.SWLOG_INFO, "Applied development configuration preset")
    }
}