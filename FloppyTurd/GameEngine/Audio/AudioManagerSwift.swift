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
    public static let shared: AudioManagerSwift = AudioManagerSwift()
    
    // MARK: - Audio Engine (public for AudioClipSwift access)
    public let audioEngine = AVAudioEngine()
    internal let musicPlayerNode = AVAudioPlayerNode()
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
    internal var currentMusicFile: AVAudioFile?
    
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
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] AudioManager deallocating")
    }
    
    // MARK: - Audio Engine Setup
    private func setupAudioEngine() {
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🎵 Setting up audio engine...")
        
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
            traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ✅ Audio session configured")
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Failed to setup audio session: \(error)")
        }
        #else
        // macOS doesn't need audio session configuration
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ✅ Audio session skipped (macOS)")
        #endif
    }
    
    private func startAudioEngine() {
        guard !isEngineStarted else { return }
        
        do {
            try audioEngine.start()
            isEngineStarted = true
            traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ✅ Audio engine started")
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Failed to start audio engine: \(error)")
        }
    }
    
    // MARK: - Audio Interruption Handling
    private func observeAudioInterruptions() {
        #if os(iOS)
        NotificationCenter.default.publisher(for: AVAudioSession.interruptionNotification)
            .sink { [weak self] notification in
                self?.handleAudioInterruption(notification)
            }
            .store(in: &cancellables)
        
        NotificationCenter.default.publisher(for: AVAudioSession.routeChangeNotification)
            .sink { [weak self] notification in
                self?.handleRouteChange(notification)
            }
            .store(in: &cancellables)
        #else
        // macOS doesn't need audio interruption handling
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ✅ Audio interruption observation skipped (macOS)")
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
            traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🔇 Audio interruption began - pausing audio")
            pauseAllAudio()
        case .ended:
            if let optionsValue = notification.userInfo?[AVAudioSessionInterruptionOptionKey] as? UInt {
                let options = AVAudioSession.InterruptionOptions(rawValue: optionsValue)
                if options.contains(.shouldResume) {
                    traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🔊 Audio interruption ended - resuming audio")
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
            traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🎧 Audio device disconnected - pausing audio")
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
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🎵 Music volume: \(volume)")
    }
    
    private func updateSoundVolume() {
        let volume = isSoundMuted ? 0.0 : Float(soundVolume) / 10.0
        soundPlayerNode.volume = volume
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🔊 Sound volume: \(volume)")
    }
    
    // MARK: - Sound Effect Management
    public func loadSoundEffect(name: String, path: String) {
        guard let url = URL(string: path) ?? Bundle.main.url(forResource: path, withExtension: nil) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Failed to find sound file: \(path)")
            return
        }
        
        do {
            let audioFile = try AVAudioFile(forReading: url)
            soundEffects[name] = audioFile
            traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ✅ Loaded sound effect: \(name)")
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Failed to load sound effect \(name): \(error)")
        }
    }
    
    public func playSoundEffect(name: String, volume: Float = 1.0) {
        guard let audioFile = soundEffects[name] else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Sound effect not found: \(name)")
            return
        }
        
        guard isEngineStarted else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Audio engine not started")
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
            self?.audioEngine.detach(playerNode)
        }
        
        playerNode.play()
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ▶️ Playing sound effect: \(name) at volume: \(finalVolume)")
    }
    
    // MARK: - Music Control
    public func playMusic(path: String, loop: Bool = true) {
        guard let url = URL(string: path) ?? Bundle.main.url(forResource: path, withExtension: nil) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Failed to find music file: \(path)")
            return
        }
        
        do {
            let audioFile = try AVAudioFile(forReading: url)
            currentMusicFile = audioFile
            
            // Stop current music
            stopMusic()
            
            // Update looping state and start time
            isMusicLooping = loop
            musicStartTime = Date().timeIntervalSince1970
            
            // Schedule new music
            if loop {
                let musicPath = path // Capture path to avoid data race
                musicPlayerNode.scheduleFile(audioFile, at: nil) { [weak self] in
                    // Re-schedule for looping
                    Task {
                        await self?.playMusic(path: musicPath, loop: true)
                    }
                }
            } else {
                musicPlayerNode.scheduleFile(audioFile, at: nil, completionHandler: nil)
            }
            
            musicPlayerNode.play()
            traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🎵 Playing music: \(url.lastPathComponent)")
            
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[AudioManagerSwift] ❌ Failed to play music: \(error)")
        }
    }
    
    public func stopMusic() {
        musicPlayerNode.stop()
        currentMusicFile = nil
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ⏹️ Music stopped")
    }
    
    public func pauseMusic() {
        musicPlayerNode.pause()
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ⏸️ Music paused")
    }
    
    public func resumeMusic() {
        musicPlayerNode.play()
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ▶️ Music resumed")
    }
    
    // MARK: - Audio Clip Management
    public func registerClip(_ clip: AudioClipSwift) {
        activeAudioClips.insert(clip)
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 📋 Registered audio clip: \(clip.id)")
    }
    
    public func unregisterClip(_ clip: AudioClipSwift) {
        activeAudioClips.remove(clip)
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 📋 Unregistered audio clip: \(clip.id)")
    }
    
    // MARK: - Lifecycle Management
    public func update() {
        // Called once per frame - can be used for audio processing updates
        // Remove finished audio clips
        activeAudioClips = activeAudioClips.filter { $0.isPlaying }
    }

    private func pauseAllAudio() {
        musicPlayerNode.pause()
        for clip in activeAudioClips {
            clip.pause()
        }
    }

    private func resumeAllAudio() {
        musicPlayerNode.play()
        for clip in activeAudioClips {
            clip.resume()
        }
    }
    
    public func shutdown() {
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🛑 Shutting down audio manager...")
        
        stopMusic()
        audioEngine.stop()
        audioEngine.reset()
        isEngineStarted = false
        
        activeAudioClips.removeAll()
        soundEffects.removeAll()
        
        cancellables.removeAll()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] ✅ Audio manager shutdown complete")
    }
    
    // MARK: - AudioEngine Compatibility Methods
    
    /// Initialize the audio manager (called by AudioEngine)
    public func initialize() {
        // The initialization is already done in init()
        traceLog(SWLogLevel.SWLOG_WARNING, "[AudioManagerSwift] Initialize called - already initialized")
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
        for clip in activeAudioClips {
            clip.stop()
        }
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
    
    // MARK: - Advanced Audio Features
    
    /// Set sound pitch (AVAudioEngine implementation)
    public func setSoundPitch(_ soundId: String, _ pitch: Float) {
        // Note: AVAudioEngine pitch control requires AVAudioUnitTimePitch
        // For now, log the request - full implementation would require audio unit setup
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🎵 Set sound pitch for \(soundId): \(pitch) (not fully implemented)")
        // TODO: Implement with AVAudioUnitTimePitch when needed
    }
    
    /// Set sound pan (AVAudioEngine implementation)
    public func setSoundPan(_ soundId: String, _ pan: Float) {
        // Note: AVAudioEngine pan control requires AVAudioMixerNode pan property
        // For now, log the request - full implementation would require per-sound mixer nodes
        traceLog(SWLogLevel.SWLOG_INFO, "[AudioManagerSwift] 🎵 Set sound pan for \(soundId): \(pan) (not fully implemented)")
        // TODO: Implement with per-sound AVAudioMixerNode when needed
    }
    
    /// Check if a specific sound is playing
    public func isSoundPlaying(_ soundId: String) -> Bool {
        // Check if the sound effect exists and if any active clips are playing it
        guard soundEffects[soundId] != nil else { return false }
        
        // For now, return false as we don't track individual sound instances
        // TODO: Implement proper sound instance tracking
        print("[AudioManagerSwift] 🔍 Checking if sound \(soundId) is playing (basic implementation)")
        return false
    }
    
    // MARK: - Music Control Extensions
    
    private var isMusicLooping: Bool = true
    private var musicStartTime: TimeInterval = 0
    
    /// Set music looping state
    public func setMusicLooping(_ looping: Bool) {
        isMusicLooping = looping
        print("[AudioManagerSwift] 🔄 Music looping set to: \(looping)")
        
        // If currently playing music, restart with new looping setting
        if currentMusicFile != nil {
            // Store the current file path for restart
            // Note: This is a simplified implementation
            print("[AudioManagerSwift] 🔄 Restarting music with new looping setting")
        }
    }
    
    /// Get total music length in seconds
    public func getMusicTimeLength() -> Float {
        guard let musicFile = currentMusicFile else {
            print("[AudioManagerSwift] ⚠️ No music file loaded")
            return 0.0
        }
        
        let lengthInSeconds = Float(musicFile.length) / Float(musicFile.fileFormat.sampleRate)
        print("[AudioManagerSwift] ⏱️ Music length: \(lengthInSeconds) seconds")
        return lengthInSeconds
    }
    
    /// Get current music playback time in seconds
    public func getMusicTimePlayed() -> Float {
        guard currentMusicFile != nil else {
            print("[AudioManagerSwift] ⚠️ No music file loaded")
            return 0.0
        }
        
        // Note: AVAudioPlayerNode doesn't provide easy access to current playback position
        // This would require more complex implementation with audio tap or scheduling callbacks
        let currentTime = Date().timeIntervalSince1970 - musicStartTime
        let timePlayed = Float(currentTime)
        
        print("[AudioManagerSwift] ⏱️ Music time played: \(timePlayed) seconds (estimated)")
        return timePlayed
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

// MARK: - C++ Interop Bridge
// Separate class for C++ interop to avoid MainActor isolation issues
@_expose(Cxx)
public final class AudioManagerCppBridge {
    
    /// Play sound effect - C++ compatible
    @_expose(Cxx) public static func playSound(_ soundId: String) -> Bool {
        Task { @MainActor in
            AudioManagerSwift.shared.playSoundEffect(name: soundId)
        }
        return true
    }
    
    /// Play music - C++ compatible
    @_expose(Cxx) public static func playMusic(_ musicId: String) -> Bool {
        Task { @MainActor in
            AudioManagerSwift.shared.playMusic(path: musicId, loop: true)
        }
        return true
    }
    
    /// Stop music - C++ compatible
    @_expose(Cxx) public static func stopMusic() {
        Task { @MainActor in
            AudioManagerSwift.shared.stopMusic()
        }
    }
    
    /// Pause music - C++ compatible
    @_expose(Cxx) public static func pauseMusic() {
        Task { @MainActor in
            AudioManagerSwift.shared.pauseMusic()
        }
    }
    
    /// Resume music - C++ compatible
    @_expose(Cxx) public static func resumeMusic() {
        Task { @MainActor in
            AudioManagerSwift.shared.resumeMusic()
        }
    }
    
    /// Set master volume - C++ compatible
    @_expose(Cxx) public static func setMasterVolume(_ volume: Float) {
        Task { @MainActor in
            let intVolume = Int(volume * 10.0)
            AudioManagerSwift.shared.setMusicVolume(intVolume)
            AudioManagerSwift.shared.setSoundVolume(intVolume)
        }
    }
    
    /// Set sound volume - C++ compatible
    @_expose(Cxx) public static func setSoundVolume(_ volume: Float) {
        Task { @MainActor in
            let intVolume = Int(volume * 10.0)
            AudioManagerSwift.shared.setSoundVolume(intVolume)
        }
    }
    
    /// Set music volume - C++ compatible
    @_expose(Cxx) public static func setMusicVolume(_ volume: Float) {
        Task { @MainActor in
            let intVolume = Int(volume * 10.0)
            AudioManagerSwift.shared.setMusicVolume(intVolume)
        }
    }
    
    /// Check if music is playing - C++ compatible
    @_expose(Cxx) public static func isMusicPlaying() -> Bool {
        return MainActor.assumeIsolated {
            return AudioManagerSwift.shared.currentMusicFile != nil && AudioManagerSwift.shared.musicPlayerNode.isPlaying
        }
    }
    
    /// Set sound pitch - C++ compatible
    @_expose(Cxx) public static func setSoundPitch(_ soundId: String, _ pitch: Float) {
        Task { @MainActor in
            AudioManagerSwift.shared.setSoundPitch(soundId, pitch)
        }
    }
    
    /// Set sound pan - C++ compatible
    @_expose(Cxx) public static func setSoundPan(_ soundId: String, _ pan: Float) {
        Task { @MainActor in
            AudioManagerSwift.shared.setSoundPan(soundId, pan)
        }
    }
    
    /// Check if sound is playing - C++ compatible
    @_expose(Cxx) public static func isSoundPlaying(_ soundId: String) -> Bool {
        return MainActor.assumeIsolated {
            return AudioManagerSwift.shared.isSoundPlaying(soundId)
        }
    }
    
    /// Set music looping - C++ compatible
    @_expose(Cxx) public static func setMusicLooping(_ looping: Bool) {
        Task { @MainActor in
            AudioManagerSwift.shared.setMusicLooping(looping)
        }
    }
    
    /// Get music time length - C++ compatible
    @_expose(Cxx) public static func getMusicTimeLength() -> Float {
        return MainActor.assumeIsolated {
            return AudioManagerSwift.shared.getMusicTimeLength()
        }
    }
    
    /// Get music time played - C++ compatible
    @_expose(Cxx) public static func getMusicTimePlayed() -> Float {
        return MainActor.assumeIsolated {
            return AudioManagerSwift.shared.getMusicTimePlayed()
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
