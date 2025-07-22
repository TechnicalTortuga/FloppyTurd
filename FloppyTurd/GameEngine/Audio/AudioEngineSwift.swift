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
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioEngine] Resource not found: \(relativePath)")
            return relativePath
        }
        return bundlePath
    }
    
    // MARK: - Initialization
    
    /// Initialize the Swift audio engine
    public func initialize() {
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Initializing Swift audio system")
        
        guard !isInitialized else {
            traceLog(SWLogLevel.SWLOG_WARNING, "[AudioEngine] Already initialized")
            return
        }
        
        // Initialize the AudioManagerSwift instance
        audioManager.initialize()
        
        isInitialized = true
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] ✅ Swift audio engine initialization complete")
    }
    
    // MARK: - Music Functions
    
    /// Play background music
    public func playMusic(_ fileName: String) {
        guard isInitialized else { 
            traceLog(SWLogLevel.SWLOG_WARNING, "[AudioEngine] Not initialized")
            return 
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Playing music: \(fileName)")
        audioManager.playMusic(path: fileName)
    }
    
    /// Stop background music
    public func stopMusic() {
        guard isInitialized else { return }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Stopping music")
        audioManager.stopMusic()
    }
    
    /// Pause background music
    public func pauseMusic() {
        guard isInitialized else { return }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Pausing music")
        audioManager.pauseMusic()
    }
    
    /// Resume background music
    public func resumeMusic() {
        guard isInitialized else { return }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Resuming music")
        audioManager.resumeMusic()
    }
    
    // MARK: - Sound Effects
    
    /// Play sound effect
    public func playSound(_ fileName: String) {
        guard isInitialized else {
            traceLog(SWLogLevel.SWLOG_WARNING, "[AudioEngine] Not initialized")
            return
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Playing sound: \(fileName)")
        audioManager.playSound(fileName)
    }
    
    /// Play positioned sound effect
    public func playSound(_ fileName: String, at position: Vector2) {
        guard isInitialized else { return }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Playing positioned sound: \(fileName) at (\(position.x), \(position.y))")
        audioManager.playSound(fileName, at: position)
    }
    
    /// Stop all sound effects
    public func stopAllSounds() {
        guard isInitialized else { return }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Stopping all sounds")
        audioManager.stopAllSounds()
    }
    
    // MARK: - Volume Control
    
    /// Set music volume
    public func setMusicVolume(_ volume: Float) {
        guard isInitialized else { return }
        
        let clampedVolume = max(0.0, min(1.0, volume))
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Setting music volume: \(clampedVolume)")
        audioManager.setMusicVolume(clampedVolume)
    }
    
    /// Set sound effects volume
    public func setSFXVolume(_ volume: Float) {
        guard isInitialized else { return }
        
        let clampedVolume = max(0.0, min(1.0, volume))
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Setting SFX volume: \(clampedVolume)")
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
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Setting music muted: \(muted)")
        audioManager.setMusicMuted(muted)
    }
    
    /// Mute/unmute sound effects
    public func setSFXMuted(_ muted: Bool) {
        guard isInitialized else { return }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Setting SFX muted: \(muted)")
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
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] App became active - restoring audio state")
        audioManager.handleAppBecameActive()
    }
    
    /// Handle app becoming inactive - pause audio
    public func handleAppBecameInactive() {
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] App became inactive - pausing audio")
        audioManager.handleAppBecameInactive()
    }
    
    /// Shutdown the audio engine
    public func shutdown() {
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Shutting down audio engine")
        
        guard isInitialized else { return }
        
        // Stop all audio
        stopMusic()
        stopAllSounds()
        
        // Shutdown the audio manager
        audioManager.shutdown()
        
        isInitialized = false
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] ✅ Audio engine shutdown complete")
    }
}

// MARK: - C++ Interop Bridge
/// Bridge class for C++ interoperability with AudioEngine
@_expose(Cxx)
public class AudioEngineCppBridge {
    
    /// Load sound - equivalent to LoadSound() (C++ Interop)
    @_expose(Cxx) nonisolated public static func loadSound(_ filename: String) -> Int32 {
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Loading sound: \(filename)")
        return 1 // Return dummy sound ID for now
    }
    
    /// Unload sound - equivalent to UnloadSound() (C++ Interop)
    @_expose(Cxx) nonisolated public static func unloadSound(_ soundId: Int32) {
        print("[AudioEngine] Unloading sound ID: \(soundId)")
    }
    
    /// Play sound - equivalent to PlaySound() (C++ Interop)
    @_expose(Cxx) nonisolated public static func playSound(_ soundId: Int32) {
        Task { @MainActor in
            AudioEngine.shared.playSound("sound_\(soundId)")
        }
    }
    
    /// Stop sound - equivalent to StopSound() (C++ Interop)
    @_expose(Cxx) nonisolated public static func stopSound(_ soundId: Int32) {
        print("[AudioEngine] Stopping sound ID: \(soundId)")
    }
    
    /// Pause sound - equivalent to PauseSound() (C++ Interop)
    @_expose(Cxx) nonisolated public static func pauseSound(_ soundId: Int32) {
        print("[AudioEngine] Pausing sound ID: \(soundId)")
    }
    
    /// Resume sound - equivalent to ResumeSound() (C++ Interop)
    @_expose(Cxx) nonisolated public static func resumeSound(_ soundId: Int32) {
        print("[AudioEngine] Resuming sound ID: \(soundId)")
    }
    
    /// Load music stream - equivalent to LoadMusicStream() (C++ Interop)
    @_expose(Cxx) nonisolated public static func loadMusicStream(_ filename: String) -> Int32 {
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioEngine] Loading music stream: \(filename)")
        return 1 // Return dummy music ID for now
    }
    
    /// Unload music stream - equivalent to UnloadMusicStream() (C++ Interop)
    @_expose(Cxx) nonisolated public static func unloadMusicStream(_ musicId: Int32) {
        print("[AudioEngine] Unloading music stream ID: \(musicId)")
    }
    
    /// Play music stream - equivalent to PlayMusicStream() (C++ Interop)
    @_expose(Cxx) nonisolated public static func playMusicStream(_ musicId: Int32) {
        Task { @MainActor in
            AudioEngine.shared.playMusic("music_\(musicId)")
        }
    }
    
    /// Stop music stream - equivalent to StopMusicStream() (C++ Interop)
    @_expose(Cxx) nonisolated public static func stopMusicStream(_ musicId: Int32) {
        Task { @MainActor in
            AudioEngine.shared.stopMusic()
        }
    }
    
    /// Update music stream - equivalent to UpdateMusicStream() (C++ Interop)
    @_expose(Cxx) nonisolated public static func updateMusicStream(_ musicId: Int32) {
        // No-op for Swift implementation
    }
    
    /// Set sound volume - equivalent to SetSoundVolume() (C++ Interop)
    @_expose(Cxx) nonisolated public static func setSoundVolume(_ soundId: Int32, _ volume: Float) {
        Task { @MainActor in
            AudioEngine.shared.setSFXVolume(volume)
        }
    }
    
    /// Set music volume - equivalent to SetMusicVolume() (C++ Interop)
    @_expose(Cxx) nonisolated public static func setMusicVolume(_ musicId: Int32, _ volume: Float) {
        Task { @MainActor in
            AudioEngine.shared.setMusicVolume(volume)
        }
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
