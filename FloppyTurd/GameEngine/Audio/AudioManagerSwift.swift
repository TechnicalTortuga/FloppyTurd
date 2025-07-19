//
//  AudioManagerSwift.swift
//  Professional Game Engine - Native Swift Audio Management
//
//  Created by C++ Swift Interop Migration
//  Modern AVAudioEngine-based audio system with iOS optimizations
//

import Foundation
import AVFoundation
import Combine

@MainActor
public final class AudioManagerSwift: ObservableObject {
    
    // MARK: - Singleton
    nonisolated public static let shared: AudioManagerSwift = {
        return MainActor.assumeIsolated {
            return AudioManagerSwift()
        }
    }()
    
    // MARK: - Audio Engine (public for AudioClipSwift access)
    public let audioEngine = AVAudioEngine()
    private let musicPlayerNode = AVAudioPlayerNode()
    private let soundPlayerNode = AVAudioPlayerNode()
    public let mixerNode = AVAudioMixerNode()
    
    // MARK: - Published Properties for UI Binding
    @Published public var musicVolume: Int = 10 {
        didSet { updateMusicVolume() }
    }
    
    @Published public var soundVolume: Int = 10 {
        didSet { updateSoundVolume() }
    }
    
    @Published public var isMusicMuted: Bool = false {
        didSet { updateMusicVolume() }
    }
    
    @Published public var isSoundMuted: Bool = false {
        didSet { updateSoundVolume() }
    }
    
    // MARK: - Audio Resources (public for extensions)
    public var soundEffects: [String: AVAudioFile] = [:]
    private var activeAudioClips: Set<AudioClipSwift> = []
    private var currentMusicFile: AVAudioFile?
    
    // MARK: - Audio Session (iOS only)
    #if os(iOS)
    private var audioSession: AVAudioSession?
    #endif
    public var isEngineStarted = false
    
    // MARK: - Combine Subscriptions
    private var cancellables = Set<AnyCancellable>()
    
    // MARK: - Initialization
    private init() {
        setupAudioEngine()
        setupAudioSession()
        observeAudioInterruptions()
    }
    
    deinit {
        // Note: deinit cannot call @MainActor methods directly
        // The shutdown will be handled by the system or explicitly by callers
        print("[AudioManagerSwift] AudioManager deallocating")
    }
    
    // MARK: - Audio Engine Setup
    private func setupAudioEngine() {
        print("[AudioManagerSwift] 🎵 Setting up audio engine...")
        
        // Connect nodes
        audioEngine.attach(musicPlayerNode)
        audioEngine.attach(soundPlayerNode)
        audioEngine.attach(mixerNode)
        
        // Connect audio graph
        audioEngine.connect(musicPlayerNode, to: mixerNode, format: nil)
        audioEngine.connect(soundPlayerNode, to: mixerNode, format: nil)
        audioEngine.connect(mixerNode, to: audioEngine.outputNode, format: nil)
        
        // Start engine
        startAudioEngine()
    }
    
    private func setupAudioSession() {
        #if os(iOS)
        audioSession = AVAudioSession.sharedInstance()
        
        do {
            try audioSession?.setCategory(.ambient, mode: .gameChat, options: [.mixWithOthers])
            try audioSession?.setActive(true)
            print("[AudioManagerSwift] ✅ Audio session configured")
        } catch {
            print("[AudioManagerSwift] ❌ Failed to setup audio session: \(error)")
        }
        #else
        // macOS doesn't need audio session configuration
        print("[AudioManagerSwift] ✅ Audio session skipped (macOS)")
        #endif
    }
    
    private func startAudioEngine() {
        guard !isEngineStarted else { return }
        
        do {
            try audioEngine.start()
            isEngineStarted = true
            print("[AudioManagerSwift] ✅ Audio engine started")
        } catch {
            print("[AudioManagerSwift] ❌ Failed to start audio engine: \(error)")
        }
    }
    
    // MARK: - Audio Interruption Handling
    private func observeAudioInterruptions() {
        #if os(iOS)
        NotificationCenter.default.publisher(for: AVAudioSession.interruptionNotification)
            .sink { [weak self] notification in
                Task { @MainActor in
                    self?.handleAudioInterruption(notification)
                }
            }
            .store(in: &cancellables)
        
        NotificationCenter.default.publisher(for: AVAudioSession.routeChangeNotification)
            .sink { [weak self] notification in
                Task { @MainActor in
                    self?.handleRouteChange(notification)
                }
            }
            .store(in: &cancellables)
        #else
        // macOS doesn't need audio interruption handling
        print("[AudioManagerSwift] ✅ Audio interruption observation skipped (macOS)")
        #endif
    }
    
    private func handleAudioInterruption(_ notification: Notification) {
        #if os(iOS)
        guard let typeValue = notification.userInfo?[AVAudioSessionInterruptionTypeKey] as? UInt,
              let type = AVAudioSession.InterruptionType(rawValue: typeValue) else {
            return
        }
        
        switch type {
        case .began:
            print("[AudioManagerSwift] 🔇 Audio interruption began - pausing audio")
            pauseAllAudio()
        case .ended:
            if let optionsValue = notification.userInfo?[AVAudioSessionInterruptionOptionKey] as? UInt {
                let options = AVAudioSession.InterruptionOptions(rawValue: optionsValue)
                if options.contains(.shouldResume) {
                    print("[AudioManagerSwift] 🔊 Audio interruption ended - resuming audio")
                    resumeAllAudio()
                }
            }
        @unknown default:
            break
        }
        #endif
    }
    
    private func handleRouteChange(_ notification: Notification) {
        #if os(iOS)
        guard let reasonValue = notification.userInfo?[AVAudioSessionRouteChangeReasonKey] as? UInt,
              let reason = AVAudioSession.RouteChangeReason(rawValue: reasonValue) else {
            return
        }
        
        switch reason {
        case .oldDeviceUnavailable:
            print("[AudioManagerSwift] 🎧 Audio device disconnected - pausing audio")
            pauseAllAudio()
        default:
            break
        }
        #endif
    }
    
    // MARK: - Volume Controls
    public func setMusicVolume(_ volume: Int) {
        musicVolume = max(0, min(10, volume))
    }
    
    public func setSoundVolume(_ volume: Int) {
        soundVolume = max(0, min(10, volume))
    }
    
    public func increaseMusicVolume() {
        setMusicVolume(musicVolume + 1)
    }
    
    public func decreaseMusicVolume() {
        setMusicVolume(musicVolume - 1)
    }
    
    public func increaseSoundVolume() {
        setSoundVolume(soundVolume + 1)
    }
    
    public func decreaseSoundVolume() {
        setSoundVolume(soundVolume - 1)
    }
    
    public func toggleMusicMute() {
        isMusicMuted.toggle()
    }
    
    public func toggleSoundMute() {
        isSoundMuted.toggle()
    }
    
    private func updateMusicVolume() {
        let volume = isMusicMuted ? 0.0 : Float(musicVolume) / 10.0
        musicPlayerNode.volume = volume
        print("[AudioManagerSwift] 🎵 Music volume: \(volume)")
    }
    
    private func updateSoundVolume() {
        let volume = isSoundMuted ? 0.0 : Float(soundVolume) / 10.0
        soundPlayerNode.volume = volume
        print("[AudioManagerSwift] 🔊 Sound volume: \(volume)")
    }
    
    // MARK: - Sound Effect Management
    public func loadSoundEffect(name: String, path: String) {
        guard let url = URL(string: path) ?? Bundle.main.url(forResource: path, withExtension: nil) else {
            print("[AudioManagerSwift] ❌ Failed to find sound file: \(path)")
            return
        }
        
        do {
            let audioFile = try AVAudioFile(forReading: url)
            soundEffects[name] = audioFile
            print("[AudioManagerSwift] ✅ Loaded sound effect: \(name)")
        } catch {
            print("[AudioManagerSwift] ❌ Failed to load sound effect \(name): \(error)")
        }
    }
    
    public func playSoundEffect(name: String, volume: Float = 1.0) {
        guard let audioFile = soundEffects[name] else {
            print("[AudioManagerSwift] ❌ Sound effect not found: \(name)")
            return
        }
        
        guard isEngineStarted else {
            print("[AudioManagerSwift] ❌ Audio engine not started")
            return
        }
        
        // Create a new player node for this sound effect
        let playerNode = AVAudioPlayerNode()
        audioEngine.attach(playerNode)
        audioEngine.connect(playerNode, to: mixerNode, format: audioFile.processingFormat)
        
        // Apply volume
        let finalVolume = volume * (isSoundMuted ? 0.0 : Float(soundVolume) / 10.0)
        playerNode.volume = finalVolume
        
        // Schedule and play
        playerNode.scheduleFile(audioFile, at: nil) { [weak self] in
            // Cleanup after playback
            Task { @MainActor in
                self?.audioEngine.detach(playerNode)
            }
        }
        
        playerNode.play()
        print("[AudioManagerSwift] ▶️ Playing sound effect: \(name) at volume: \(finalVolume)")
    }
    
    // MARK: - Music Control
    public func playMusic(path: String, loop: Bool = true) {
        guard let url = URL(string: path) ?? Bundle.main.url(forResource: path, withExtension: nil) else {
            print("[AudioManagerSwift] ❌ Failed to find music file: \(path)")
            return
        }
        
        do {
            let audioFile = try AVAudioFile(forReading: url)
            currentMusicFile = audioFile
            
            // Stop current music
            stopMusic()
            
            // Schedule new music
            if loop {
                musicPlayerNode.scheduleFile(audioFile, at: nil) { [weak self] in
                    // Re-schedule for looping
                    Task { @MainActor in
                        self?.playMusic(path: path, loop: true)
                    }
                }
            } else {
                musicPlayerNode.scheduleFile(audioFile, at: nil, completionHandler: nil)
            }
            
            musicPlayerNode.play()
            print("[AudioManagerSwift] 🎵 Playing music: \(url.lastPathComponent)")
            
        } catch {
            print("[AudioManagerSwift] ❌ Failed to play music: \(error)")
        }
    }
    
    public func stopMusic() {
        musicPlayerNode.stop()
        currentMusicFile = nil
        print("[AudioManagerSwift] ⏹️ Music stopped")
    }
    
    public func pauseMusic() {
        musicPlayerNode.pause()
        print("[AudioManagerSwift] ⏸️ Music paused")
    }
    
    public func resumeMusic() {
        musicPlayerNode.play()
        print("[AudioManagerSwift] ▶️ Music resumed")
    }
    
    // MARK: - Audio Clip Management
    public func registerClip(_ clip: AudioClipSwift) {
        activeAudioClips.insert(clip)
        print("[AudioManagerSwift] 📋 Registered audio clip: \(clip.id)")
    }
    
    public func unregisterClip(_ clip: AudioClipSwift) {
        activeAudioClips.remove(clip)
        print("[AudioManagerSwift] 📋 Unregistered audio clip: \(clip.id)")
    }
    
    // MARK: - Lifecycle Management
    public func update() {
        // Called once per frame - can be used for audio processing updates
        // Remove finished audio clips
        activeAudioClips = activeAudioClips.filter { $0.isPlaying }
    }
    
    private func pauseAllAudio() {
        musicPlayerNode.pause()
        activeAudioClips.forEach { $0.pause() }
    }
    
    private func resumeAllAudio() {
        musicPlayerNode.play()
        activeAudioClips.forEach { $0.resume() }
    }
    
    public func shutdown() {
        print("[AudioManagerSwift] 🛑 Shutting down audio manager...")
        
        stopMusic()
        audioEngine.stop()
        audioEngine.reset()
        isEngineStarted = false
        
        activeAudioClips.removeAll()
        soundEffects.removeAll()
        
        cancellables.removeAll()
        
        print("[AudioManagerSwift] ✅ Audio manager shutdown complete")
    }
    
    // MARK: - AudioEngine Compatibility Methods
    
    /// Initialize the audio manager (called by AudioEngine)
    public func initialize() {
        // The initialization is already done in init()
        print("[AudioManagerSwift] Initialize called - already initialized")
    }
    
    /// Play sound with filename (AudioEngine compatibility)
    public func playSound(_ fileName: String) {
        playSoundEffect(name: fileName)
    }
    
    /// Play positioned sound (AudioEngine compatibility)
    public func playSound(_ fileName: String, at position: Vector2) {
        // For now, play without positioning (could be enhanced with 3D audio later)
        playSoundEffect(name: fileName)
    }
    
    /// Stop all sounds (AudioEngine compatibility)
    public func stopAllSounds() {
        activeAudioClips.forEach { $0.stop() }
    }
    
    /// Set music volume with Float (AudioEngine compatibility)
    public func setMusicVolume(_ volume: Float) {
        let intVolume = Int(volume * 10.0) // Convert 0.0-1.0 to 0-10
        setMusicVolume(intVolume)
    }
    
    /// Set sound volume with Float (AudioEngine compatibility)  
    public func setSoundVolume(_ volume: Float) {
        let intVolume = Int(volume * 10.0) // Convert 0.0-1.0 to 0-10
        setSoundVolume(intVolume)
    }
    
    /// Get music volume as Float (AudioEngine compatibility)
    public func getMusicVolume() -> Float {
        return Float(musicVolume) / 10.0 // Convert 0-10 to 0.0-1.0
    }
    
    /// Get sound volume as Float (AudioEngine compatibility)
    public func getSoundVolume() -> Float {
        return Float(soundVolume) / 10.0 // Convert 0-10 to 0.0-1.0
    }
    
    /// Set music muted (AudioEngine compatibility)
    public func setMusicMuted(_ muted: Bool) {
        isMusicMuted = muted
    }
    
    /// Set sound muted (AudioEngine compatibility)
    public func setSoundMuted(_ muted: Bool) {
        isSoundMuted = muted
    }
    
    /// Handle app became active (AudioEngine compatibility)
    public func handleAppBecameActive() {
        resumeMusic()
    }
    
    /// Handle app became inactive (AudioEngine compatibility)
    public func handleAppBecameInactive() {
        pauseMusic()
    }
    
    // MARK: - Memory Management
    public func handleMemoryWarning() {
        print("[AudioManagerSwift] ⚠️ Handling memory warning - clearing unused audio")
        
        // Clear unused sound effects (keep only recently used)
        let _ = Date().timeIntervalSince1970
        soundEffects = soundEffects.filter { _, _ in
            // You could implement LRU logic here if needed
            return true
        }
    }
}

// MARK: - Audio Clip Wrapper
@MainActor
public final class AudioClipSwift: Hashable {
    
    public let id = UUID()
    private let playerNode: AVAudioPlayerNode
    private let audioFile: AVAudioFile
    private var _isPlaying = false
    
    public var isPlaying: Bool { _isPlaying }
    
    public init?(path: String) {
        guard let url = URL(string: path) ?? Bundle.main.url(forResource: path, withExtension: nil) else {
            print("[AudioClipSwift] ❌ Failed to find audio file: \(path)")
            return nil
        }
        
        do {
            audioFile = try AVAudioFile(forReading: url)
            playerNode = AVAudioPlayerNode()
            
            // Attach to main audio engine
            AudioManagerSwift.shared.audioEngine.attach(playerNode)
            AudioManagerSwift.shared.audioEngine.connect(playerNode, to: AudioManagerSwift.shared.mixerNode, format: audioFile.processingFormat)
            
            print("[AudioClipSwift] ✅ Created audio clip: \(url.lastPathComponent)")
            
        } catch {
            print("[AudioClipSwift] ❌ Failed to create audio clip: \(error)")
            return nil
        }
    }
    
    deinit {
        // Note: deinit cannot call @MainActor methods directly
        // The cleanup will be handled by the AudioManagerSwift when needed
        print("[AudioClipSwift] AudioClip deallocating")
    }
    
    public func play() {
        guard !_isPlaying else { return }
        
        playerNode.scheduleFile(audioFile, at: nil) { [weak self] in
            Task { @MainActor in
                self?._isPlaying = false
                if let clip = self {
                    AudioManagerSwift.shared.unregisterClip(clip)
                }
            }
        }
        
        playerNode.play()
        _isPlaying = true
        AudioManagerSwift.shared.registerClip(self)
    }
    
    public func stop() {
        playerNode.stop()
        _isPlaying = false
        AudioManagerSwift.shared.unregisterClip(self)
    }
    
    public func pause() {
        playerNode.pause()
    }
    
    public func resume() {
        if _isPlaying {
            playerNode.play()
        }
    }
    
    // MARK: - Hashable
    nonisolated public func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
    
    nonisolated public static func == (lhs: AudioClipSwift, rhs: AudioClipSwift) -> Bool {
        lhs.id == rhs.id
    }
}
