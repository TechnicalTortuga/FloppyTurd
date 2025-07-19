//
//  AudioEngineSwift.swift
//  Professional Game Engine - Swift Audio Layer
//
//  Created by C++ Swift Interop Migration  
//  Native Swift audio system with AVAudioEngine integration
//

import Foundation
import AVFoundation

/// Professional Swift audio engine using native AVAudioEngine
/// Replaces legacy C++ audio system with modern Swift implementation
@MainActor
public class AudioEngine {
    
    // MARK: - Properties
    private let audioManager = AudioManagerSwift.shared
    private var isInitialized = false
    
    // MARK: - Singleton
    public static let shared = AudioEngine()
    
    private init() {}
    
    // MARK: - Helper Methods
    
    /// Helper to resolve resource paths
    private static func resolveResourcePath(_ relativePath: String) -> String {
        // Use Bundle resource resolution for Swift architecture
        guard let bundlePath = Bundle.main.path(forResource: relativePath, ofType: nil) else {
            print("[AudioEngine] Resource not found: \(relativePath)")
            return relativePath
        }
        return bundlePath
    }
    
    // MARK: - Initialization
    
    /// Initialize the Swift audio engine
    public func initialize() {
        print("[AudioEngine] Initializing Swift audio system")
        
        guard !isInitialized else {
            print("[AudioEngine] Already initialized")
            return
        }
        
        // Initialize the AudioManagerSwift instance
        audioManager.initialize()
        
        isInitialized = true
        print("[AudioEngine] ✅ Swift audio engine initialization complete")
    }
    
    // MARK: - Music Functions
    
    /// Play background music
    public func playMusic(_ fileName: String) {
        guard isInitialized else { 
            print("[AudioEngine] Not initialized")
            return 
        }
        
        print("[AudioEngine] Playing music: \(fileName)")
        audioManager.playMusic(path: fileName)
    }
    
    /// Stop background music
    public func stopMusic() {
        guard isInitialized else { return }
        
        print("[AudioEngine] Stopping music")
        audioManager.stopMusic()
    }
    
    /// Pause background music
    public func pauseMusic() {
        guard isInitialized else { return }
        
        print("[AudioEngine] Pausing music")
        audioManager.pauseMusic()
    }
    
    /// Resume background music
    public func resumeMusic() {
        guard isInitialized else { return }
        
        print("[AudioEngine] Resuming music")
        audioManager.resumeMusic()
    }
    
    // MARK: - Sound Effects
    
    /// Play sound effect
    public func playSound(_ fileName: String) {
        guard isInitialized else {
            print("[AudioEngine] Not initialized")
            return
        }
        
        print("[AudioEngine] Playing sound: \(fileName)")
        audioManager.playSound(fileName)
    }
    
    /// Play positioned sound effect
    public func playSound(_ fileName: String, at position: Vector2) {
        guard isInitialized else { return }
        
        print("[AudioEngine] Playing positioned sound: \(fileName) at (\(position.x), \(position.y))")
        audioManager.playSound(fileName, at: position)
    }
    
    /// Stop all sound effects
    public func stopAllSounds() {
        guard isInitialized else { return }
        
        print("[AudioEngine] Stopping all sounds")
        audioManager.stopAllSounds()
    }
    
    // MARK: - Volume Control
    
    /// Set music volume
    public func setMusicVolume(_ volume: Float) {
        guard isInitialized else { return }
        
        let clampedVolume = max(0.0, min(1.0, volume))
        print("[AudioEngine] Setting music volume: \(clampedVolume)")
        audioManager.setMusicVolume(clampedVolume)
    }
    
    /// Set sound effects volume
    public func setSFXVolume(_ volume: Float) {
        guard isInitialized else { return }
        
        let clampedVolume = max(0.0, min(1.0, volume))
        print("[AudioEngine] Setting SFX volume: \(clampedVolume)")
        audioManager.setSoundVolume(clampedVolume)
    }
    
    /// Get current music volume
    public func getMusicVolume() -> Float {
        return audioManager.getMusicVolume()
    }
    
    /// Get current SFX volume
    public func getSFXVolume() -> Float {
        return audioManager.getSoundVolume()
    }
    
    // MARK: - Muting Controls
    
    /// Mute/unmute music
    public func setMusicMuted(_ muted: Bool) {
        guard isInitialized else { return }
        
        print("[AudioEngine] Setting music muted: \(muted)")
        audioManager.setMusicMuted(muted)
    }
    
    /// Mute/unmute sound effects
    public func setSFXMuted(_ muted: Bool) {
        guard isInitialized else { return }
        
        print("[AudioEngine] Setting SFX muted: \(muted)")
        audioManager.setSoundMuted(muted)
    }
    
    /// Check if music is muted
    public func getMusicMuted() -> Bool {
        return audioManager.isMusicMuted
    }
    
    /// Check if SFX is muted
    public func getSFXMuted() -> Bool {
        return audioManager.isSoundMuted
    }
    
    // MARK: - Audio State Management
    
    /// Handle app becoming active - restore audio state
    public func handleAppBecameActive() {
        print("[AudioEngine] App became active - restoring audio state")
        audioManager.handleAppBecameActive()
    }
    
    /// Handle app becoming inactive - pause audio
    public func handleAppBecameInactive() {
        print("[AudioEngine] App became inactive - pausing audio")
        audioManager.handleAppBecameInactive()
    }
    
    /// Shutdown the audio engine
    public func shutdown() {
        print("[AudioEngine] Shutting down audio engine")
        
        guard isInitialized else { return }
        
        // Stop all audio
        stopMusic()
        stopAllSounds()
        
        // Shutdown the audio manager
        audioManager.shutdown()
        
        isInitialized = false
        print("[AudioEngine] ✅ Audio engine shutdown complete")
    }
}

// MARK: - Audio Engine Extensions

extension AudioEngine {
    
    /// Convenience method for playing game-specific audio with automatic path resolution
    public func playGameSound(_ soundName: String, volume: Float = 1.0) {
        let resourcePath = Self.resolveResourcePath("audio/\(soundName)")
        if !resourcePath.isEmpty {
            playSound(resourcePath)
        } else {
            // Fallback to direct name
            playSound(soundName)
        }
    }
    
    /// Convenience method for playing game music with automatic path resolution
    public func playGameMusic(_ musicName: String, volume: Float = 1.0) {
        let resourcePath = Self.resolveResourcePath("audio/\(musicName)")
        if !resourcePath.isEmpty {
            playMusic(resourcePath)
        } else {
            // Fallback to direct name
            playMusic(musicName)
        }
    }
}
