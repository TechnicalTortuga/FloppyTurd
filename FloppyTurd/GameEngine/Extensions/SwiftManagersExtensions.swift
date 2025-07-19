//
//  SwiftManagersExtensions.swift
//  Professional Game Engine - Swift Managers Convenience Extensions
//
//  Created by C++ Swift Interop Migration
//  Convenience methods for common Swift manager operations
//

import Foundation
#if canImport(UIKit)
import UIKit
#endif
import Metal

// MARK: - AudioManagerSwift Extensions

public extension AudioManagerSwift {
    
    /// Quick method to play a sound effect by filename
    func playSound(_ filename: String, volume: Float = 1.0) {
        // Auto-register and play if not already loaded
        if soundEffects[filename] == nil {
            loadSoundEffect(name: filename, path: filename)
        }
        playSoundEffect(name: filename, volume: volume)
    }
    
    /// Play background music with automatic looping
    func playBackgroundMusic(_ filename: String) {
        playMusic(path: filename, loop: true)
    }
    
    /// Fade music volume over time
    func fadeMusicVolume(to targetVolume: Int, duration: TimeInterval) {
        let startVolume = musicVolume
        let volumeDifference = targetVolume - startVolume
        let steps = 20
        let stepDuration = duration / Double(steps)

        class StepCounter: @unchecked Sendable { var value = 0 }
        let currentStep = StepCounter()

        Timer.scheduledTimer(withTimeInterval: stepDuration, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }
            currentStep.value += 1

            if currentStep.value >= steps {
                timer.invalidate()
                Task { @MainActor in
                    self.setMusicVolume(targetVolume)
                }
            } else {
                let progress = Double(currentStep.value) / Double(steps)
                let newVolume = startVolume + Int(Double(volumeDifference) * progress)
                Task { @MainActor in
                    self.setMusicVolume(newVolume)
                }
            }
        }
    }
    
    /// Get volume as normalized float (0.0 to 1.0)
    var normalizedMusicVolume: Float {
        return Float(musicVolume) / 10.0
    }
    
    var normalizedSoundVolume: Float {
        return Float(soundVolume) / 10.0
    }
    
    /// Audio settings for saving/loading
    var audioSettings: [String: Any] {
        return [
            "musicVolume": musicVolume,
            "soundVolume": soundVolume,
            "isMusicMuted": isMusicMuted,
            "isSoundMuted": isSoundMuted
        ]
    }
    
    /// Restore audio settings from dictionary
    func restoreSettings(_ settings: [String: Any]) {
        if let musicVol = settings["musicVolume"] as? Int {
            setMusicVolume(musicVol)
        }
        if let soundVol = settings["soundVolume"] as? Int {
            setSoundVolume(soundVol)
        }
        if let musicMuted = settings["isMusicMuted"] as? Bool {
            isMusicMuted = musicMuted
        }
        if let soundMuted = settings["isSoundMuted"] as? Bool {
            isSoundMuted = soundMuted
        }
    }
}

// MARK: - ResourceManagerSwift Extensions

public extension ResourceManagerSwift {
    
    /// Quick texture loading with automatic fallback
    func loadTexture(_ filename: String, device: MTLDevice) -> MTLTexture? {
        // Try exact filename first
        if let texture = getTexture(id: filename) {
            return texture
        }
        
        // Auto-register with common paths
        let baseFilename = URL(fileURLWithPath: filename).deletingPathExtension().lastPathComponent
        registerResource(id: baseFilename, relativePath: filename, type: .texture)
        
        return getTexture(id: baseFilename)
    }
    
    /// Load font with automatic system font fallback
    func loadFont(_ fontName: String, size: CGFloat) -> UIFont {
        if let font = getFont(id: fontName, size: size) {
            return font
        }
        
        // Try registering common font paths
        registerResource(id: fontName, relativePath: "fonts/\(fontName).ttf", type: .font)
        registerResource(id: fontName, relativePath: "fonts/\(fontName).otf", type: .font)
        
        return getFont(id: fontName, size: size) ?? UIFont.systemFont(ofSize: size)
    }
    
    /// Load data with automatic error handling
    func loadData(_ filename: String) -> Data? {
        if let data = getSoundData(id: filename) {
            return data
        }
        
        // Try to register and load
        registerResource(id: filename, relativePath: filename, type: .data)
        return getSoundData(id: filename)
    }
    
    /// Preload common game resources
    func preloadCommonResources(device: MTLDevice) {
        print("[ResourceManagerSwift] 📦 Preloading common game resources...")
        
        // Common UI textures
        let commonTextures = [
            "button_normal", "button_pressed", "button_disabled",
            "background", "logo", "icon_sound", "icon_music",
            "progress_bar", "slider_thumb", "checkbox_on", "checkbox_off"
        ]
        
        for textureId in commonTextures {
            _ = getTexture(id: textureId)
        }
        
        // Common fonts
        let commonFonts = [
            ("ui_font", 16.0),
            ("title_font", 24.0),
            ("small_font", 12.0)
        ]
        
        for (fontId, size) in commonFonts {
            _ = getFont(id: fontId, size: size)
        }
        
        // Common sounds
        let commonSounds = [
            "click", "hover", "success", "error", "notification"
        ]
        
        for soundId in commonSounds {
            _ = getSoundData(id: soundId)
        }
        
        print("[ResourceManagerSwift] ✅ Common resources preloaded")
    }
    
    /// Get memory usage in MB
    var memoryUsageMB: Double {
        return Double(totalMemoryUsage) / (1024.0 * 1024.0)
    }
    
    /// Resource loading statistics
    var loadingStats: [String: Int] {
        return [
            "totalResources": resourceRegistry.count,
            "loadedResources": getLoadedResources().count,
            "memoryUsageMB": Int(memoryUsageMB)
        ]
    }
}

// MARK: - Combined Swift Managers Helper

/// Helper class for coordinated manager operations
@MainActor
public class SwiftManagersCoordinator {
    
    public static let shared = SwiftManagersCoordinator()
    
    private let audioManager = AudioManagerSwift.shared
    private let resourceManager = ResourceManagerSwift.shared
    
    private init() {}
    
    /// Initialize both managers with coordinated setup
    public func initializeAll(metalDevice: MTLDevice, quality: ResourceQuality = .auto) -> Bool {
        print("[SwiftManagersCoordinator] 🚀 Initializing all Swift managers...")
        
        // Initialize resource manager first (audio might need resources)
        resourceManager.initialize(quality: quality, metalDevice: metalDevice)
        
        // AudioManager initializes automatically
        
        // Preload common resources
        resourceManager.preloadCommonResources(device: metalDevice)
        
        // Load common audio settings
        loadAudioSettings()
        
        let success = resourceManager.isInitialized
        print("[SwiftManagersCoordinator] \(success ? "✅" : "❌") Managers initialization: \(success ? "success" : "failed")")
        
        return success
    }
    
    /// Shutdown both managers
    public func shutdownAll() {
        print("[SwiftManagersCoordinator] 🛑 Shutting down all Swift managers...")
        
        // Save audio settings before shutdown
        saveAudioSettings()
        
        audioManager.shutdown()
        resourceManager.shutdown()
        
        print("[SwiftManagersCoordinator] ✅ All managers shutdown complete")
    }
    
    /// Handle memory warnings for both managers
    public func handleMemoryWarning() {
        print("[SwiftManagersCoordinator] ⚠️ Handling memory warning for all managers...")
        
        audioManager.handleMemoryWarning()
        resourceManager.handleMemoryWarning()
    }
    
    /// Save audio settings to UserDefaults
    private func saveAudioSettings() {
        let settings = audioManager.audioSettings
        UserDefaults.standard.set(settings, forKey: "FloppyTurd.AudioSettings")
        print("[SwiftManagersCoordinator] 💾 Audio settings saved")
    }
    
    /// Load audio settings from UserDefaults
    private func loadAudioSettings() {
        if let settings = UserDefaults.standard.object(forKey: "FloppyTurd.AudioSettings") as? [String: Any] {
            audioManager.restoreSettings(settings)
            print("[SwiftManagersCoordinator] 📱 Audio settings loaded")
        }
    }
    
    /// Get combined status for debugging
    public func getDebugStatus() -> String {
        let audioStats = audioManager.audioSettings
        let resourceStats = resourceManager.loadingStats
        
        return """
        Swift Managers Status:
        
        Audio Manager:
        - Music Volume: \(audioStats["musicVolume"] ?? 0)/10
        - Sound Volume: \(audioStats["soundVolume"] ?? 0)/10
        - Music Muted: \(audioStats["isMusicMuted"] ?? false)
        - Sound Muted: \(audioStats["isSoundMuted"] ?? false)
        
        Resource Manager:
        - Initialized: \(resourceManager.isInitialized)
        - Quality: \(resourceManager.currentQuality)
        - Total Resources: \(resourceStats["totalResources"] ?? 0)
        - Loaded Resources: \(resourceStats["loadedResources"] ?? 0)
        - Memory Usage: \(resourceStats["memoryUsageMB"] ?? 0) MB
        """
    }
}

// MARK: - Error Handling Extensions

public extension AudioManagerSwift {
    
    /// Play sound with error handling
    func playSoundSafely(_ name: String, volume: Float = 1.0) {
        playSoundEffect(name: name, volume: volume)
    }
}

public extension ResourceManagerSwift {
    
    /// Load texture with error handling and logging
    func loadTextureSafely(id: String, device: MTLDevice) -> MTLTexture? {
        guard let texture = getTexture(id: id) else {
            print("[ResourceManagerSwift] ⚠️ Failed to load texture: \(id)")
            return nil
        }
        
        print("[ResourceManagerSwift] ✅ Loaded texture: \(id) (\(texture.width)x\(texture.height))")
        return texture
    }
}

// MARK: - Performance Monitoring

@MainActor
public class SwiftManagersPerformanceMonitor {
    
    public static let shared = SwiftManagersPerformanceMonitor()
    
    private var loadTimes: [String: TimeInterval] = [:]
    private var loadCounts: [String: Int] = [:]
    
    private init() {}
    
    /// Track resource loading time
    public func trackLoadTime(resourceId: String, startTime: CFAbsoluteTime) {
        let endTime = CFAbsoluteTimeGetCurrent()
        let loadTime = endTime - startTime
        
        loadTimes[resourceId] = loadTime
        loadCounts[resourceId] = (loadCounts[resourceId] ?? 0) + 1
        
        if loadTime > 0.1 { // Log slow loads
            print("[PerformanceMonitor] ⏱️ Slow load: \(resourceId) took \(String(format: "%.3f", loadTime))s")
        }
    }
    
    /// Get performance report
    public func getPerformanceReport() -> String {
        let totalLoads = loadCounts.values.reduce(0, +)
        let averageLoadTime = loadTimes.values.reduce(0, +) / max(1, Double(loadTimes.count))
        
        return """
        Swift Managers Performance:
        - Total Loads: \(totalLoads)
        - Average Load Time: \(String(format: "%.3f", averageLoadTime))s
        - Tracked Resources: \(loadTimes.count)
        """
    }
}
