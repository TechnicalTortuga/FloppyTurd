//
//  AVAudioHandler.swift
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

import AVFoundation
import UIKit

/// Modern iOS Audio Manager following Swift 6 concurrency best practices
///
/// Key design decisions:
/// - AVPlayer for music playback (stable for streaming/looping)
/// - AVAudioEngine for sound effects (low latency)
/// - MainActor isolation for UI thread safety
/// - Proper notification handling with nonisolated methods
/// - Comprehensive error handling and recovery
@MainActor
public final class AVAudioHandler: NSObject {

    // MARK: - Core Audio Components

    private let audioEngine = AVAudioEngine()
    private let soundPlayerNode = AVAudioPlayerNode()  // legacy single node
    private let mixerNode = AVAudioMixerNode()
    private let sfxPoolSize: Int = 8
    private var sfxNodes: [AVAudioPlayerNode] = []
    private var sfxRoundRobinIndex: Int = 0

    // MARK: - Audio Session

    private let audioSession = AVAudioSession.sharedInstance()

    // MARK: - State Management

    private var isInitialized = false
    private var isEngineRunning = false
    private var musicVolume: Float = 0.7
    public private(set) var soundVolume: Float = 1.0

    // MARK: - Music Player

    private var musicPlayer: AVPlayer?
    private var musicEndObserver: NSObjectProtocol?
    private var isMusicLooping = true

    // MARK: - Audio Session Notifications

    private var routeChangeObserver: NSObjectProtocol?
    private var interruptionObserver: NSObjectProtocol?
    private var mediaResetObserver: NSObjectProtocol?

    // MARK: - Sound Cache

    private var soundCache: [String: AVAudioFile] = [:]

    // MARK: - Logging

    private func log(_ message: String, level: LogLevel = .info) {
        switch level {
        case .trace, .debug:
            SwiftLog.debug(message, category: "AVAudioHandler")
        case .info:
            SwiftLog.info(message, category: "AVAudioHandler")
        case .warning:
            SwiftLog.warn(message, category: "AVAudioHandler")
        case .error:
            SwiftLog.error(message, category: "AVAudioHandler")
        case .fatal:
            SwiftLog.fatal(message, category: "AVAudioHandler")
        }
    }

    // MARK: - Initialization

    public override init() {
        super.init()
        // Notification setup will be called after initialization
    }

    // MARK: - Public Interface

    public func initialize() -> Bool {
        guard !isInitialized else {
            log("[AVAudioHandler] Already initialized")
            return true
        }

        log("[AVAudioHandler] Starting audio system initialization")

        do {
            log("[AVAudioHandler] Setting up audio session...")
            try setupAudioSession()

            log("[AVAudioHandler] Setting up audio engine...")
            setupAudioEngine()

            log("[AVAudioHandler] Starting audio engine...")
            try startEngine()

            // Setup notifications after successful initialization
            setupNotificationObservers()

            isInitialized = true
            log("Audio system initialized successfully")
            return true

        } catch {
            log("Failed to initialize audio system: \(error.localizedDescription)", level: .error)
            return false
        }
    }

    public func playMusic(_ fileName: String) {
        log("🎵 [AVAudioHandler] playMusic() called with: \(fileName)")

        // Auto-initialize if not already done
        if !isInitialized {
            log("[AVAudioHandler] Auto-initializing audio system")
            if !initialize() {
                log("Failed to auto-initialize audio system")
                return
            }
        }

        // Stop any existing music
        stopMusic()

        // Find the audio file
        guard let url = findAudioFile(named: fileName) else {
            log("Music file not found: \(fileName)")
            return
        }
        // Ready to play

        // Create player item and player
        let playerItem = AVPlayerItem(url: url)
        musicPlayer = AVPlayer(playerItem: playerItem)

        // Set volume
        musicPlayer?.volume = musicVolume

        // Play the music
        musicPlayer?.play()

        setupMusicLoopObserverIfNeeded()
        log("Music started: \(fileName)")
    }

    public func playSound(_ fileName: String) {
        log("🔊 [AVAudioHandler] playSound() called with: \(fileName)")

        // Auto-initialize if not already done
        if !isInitialized {
            log("[AVAudioHandler] Audio system not initialized, attempting auto-initialization")
            if !initialize() {
                log("Failed to auto-initialize audio system")
                return
            }
        }

        // Ensure engine is running
        guard ensureEngineRunning() else {
            log("Cannot play sound: audio engine not running")
            return
        }

        // Load or get cached audio file
        guard let audioFile = loadSoundFile(fileName) else {
            log("Failed to load sound file: \(fileName)")
            return
        }

        // Schedule and play the sound
        scheduleSound(audioFile)
    }

    public func stopMusic() {
        musicPlayer?.pause()
        musicPlayer = nil
        if let observer = musicEndObserver {
            NotificationCenter.default.removeObserver(observer)
            musicEndObserver = nil
        }
        log("Music stopped")
    }

    public func isMusicPlaying() -> Bool {
        guard let player = musicPlayer else { return false }
        return player.rate > 0 && player.error == nil
    }

    public func stopSound(_ soundName: String? = nil) {
        if let soundName = soundName {
            // Stop specific sound by name
            log("Stopping specific sound: \(soundName)")
            // For now, stop all sounds since we need to implement per-sound tracking
            // TODO: Implement per-sound tracking to stop only specific sounds
            stopAllSounds()
        } else {
            // Stop all sounds (legacy behavior)
            log("Stopping all sounds")
            stopAllSounds()
        }
    }

    private func stopAllSounds() {
        soundPlayerNode.stop()
        for node in sfxNodes {
            node.stop()
        }
        log("All sounds stopped")
    }

    public func setMusicVolume(volume: Float) {
        musicVolume = max(0.0, min(1.0, volume))
        musicPlayer?.volume = musicVolume
        log("Music volume set to: \(self.musicVolume)")
    }

    public func setSoundVolume(volume: Float) {
        soundVolume = max(0.0, min(1.0, volume))
        soundPlayerNode.volume = soundVolume
        for node in sfxNodes { node.volume = soundVolume }
        log("Sound volume set to: \(self.soundVolume)")
    }

    public func pause() {
        musicPlayer?.pause()
        if audioEngine.isRunning {
            audioEngine.pause()
        }
        log("Audio paused")
    }

    public func resume() {
        musicPlayer?.play()
        if !audioEngine.isRunning {
            do {
                try startEngine()
            } catch {
                log("Failed to resume audio engine: \(error)", level: .error)
            }
        }
        log("Audio resumed")
    }

    // MARK: - Private Setup Methods

    private func setupAudioSession() throws {
        // Configure for game audio
        try audioSession.setCategory(
            .playback,
            mode: .default,
            options: [.mixWithOthers])

        // Low latency for sound effects
        try audioSession.setPreferredIOBufferDuration(0.005)  // 5ms
        try audioSession.setActive(true)

        log("Audio session configured: rate=\(self.audioSession.sampleRate)Hz")
    }

    private func setupAudioEngine() {
        // Attach nodes to the engine
        audioEngine.attach(mixerNode)
        // Build SFX pool and connect to mixer
        sfxNodes = (0..<sfxPoolSize).map { _ in AVAudioPlayerNode() }
        for node in sfxNodes {
            audioEngine.attach(node)
            audioEngine.connect(node, to: mixerNode, format: nil)
            node.volume = soundVolume
        }
        // Keep legacy node connected (not used for playback anymore)
        audioEngine.attach(soundPlayerNode)
        audioEngine.connect(soundPlayerNode, to: mixerNode, format: nil)
        // Connect mixer to main mixer
        audioEngine.connect(mixerNode, to: audioEngine.mainMixerNode, format: nil)

        // Set initial volumes
        mixerNode.outputVolume = 1.0

        // Prepare the engine
        audioEngine.prepare()

        log("Audio engine configured (player → mixer → mainMixer)")
    }

    private func startEngine() throws {
        guard !audioEngine.isRunning else {
            log("Audio engine already running")
            return
        }

        try audioEngine.start()
        isEngineRunning = true
        log("Audio engine started successfully")
    }

    private func stopEngine() {
        guard audioEngine.isRunning else { return }

        soundPlayerNode.stop()
        audioEngine.stop()
        isEngineRunning = false
        log("Audio engine stopped")
    }

    private func ensureEngineRunning() -> Bool {
        if !audioEngine.isRunning {
            do {
                try startEngine()
                return true
            } catch {
                log("Failed to start audio engine: \(error)", level: .error)
                return false
            }
        }
        return true
    }

    // MARK: - File Loading

    private func findAudioFile(named fileName: String) -> URL? {
        let extensions = ["mp3", "m4a", "wav", "aac", "ogg"]

        // First try with the filename as-is in the bundle
        if let url = Bundle.main.url(forResource: fileName, withExtension: nil) {
            return url
        }

        // Try with common extensions in the bundle
        for ext in extensions {
            if let url = Bundle.main.url(forResource: fileName, withExtension: ext) {
                return url
            }
        }

        // Try NSDataAsset for Asset Catalog resources
        if let dataAsset = NSDataAsset(name: fileName) {
            // Write the data to a temporary file
            let tempURL = FileManager.default.temporaryDirectory
                .appendingPathComponent(fileName)
                .appendingPathExtension("mp3")

            do {
                try dataAsset.data.write(to: tempURL)
                return tempURL
            } catch {
                log("Failed to write asset data to temp file: \(error)", level: .error)
            }
        }

        // Also try with extensions in Asset Catalog
        let fileNameWithoutExt = (fileName as NSString).deletingPathExtension
        for ext in extensions {
            if let dataAsset = NSDataAsset(name: "\(fileNameWithoutExt)") {
                let tempURL = FileManager.default.temporaryDirectory
                    .appendingPathComponent(fileNameWithoutExt)
                    .appendingPathExtension(ext)

                do {
                    try dataAsset.data.write(to: tempURL)
                    return tempURL
                } catch {
                    log("Failed to write asset data to temp file: \(error)", level: .error)
                }
            }
        }

        return nil
    }

    private func loadSoundFile(_ fileName: String) -> AVAudioFile? {
        // Check cache first
        if let cachedFile = soundCache[fileName] {
            log("[AVAudioHandler] Using cached sound for: \(fileName)")
            return cachedFile
        }

        // Find file URL
        guard let url = findAudioFile(named: fileName) else {
            log("Sound file not found: \(fileName)")
            return nil
        }

        // Load audio file
        do {
            let audioFile = try AVAudioFile(forReading: url)
            soundCache[fileName] = audioFile
            log("[AVAudioHandler] Successfully loaded sound file: \(fileName)")
            return audioFile
        } catch {
            log("Failed to create AVAudioFile from: \(url) - \(error)", level: .error)
            return nil
        }
    }

    // MARK: - Sound Playback

    private func scheduleSound(_ audioFile: AVAudioFile) {
        log("[AVAudioHandler] Scheduling sound: \(audioFile.url.lastPathComponent)")
        // Choose an idle node or reuse round-robin
        var chosen: AVAudioPlayerNode?
        if let idle = sfxNodes.first(where: { !$0.isPlaying }) {
            chosen = idle
        } else if !sfxNodes.isEmpty {
            let idx = sfxRoundRobinIndex % sfxNodes.count
            sfxRoundRobinIndex = (sfxRoundRobinIndex + 1) % sfxNodes.count
            chosen = sfxNodes[idx]
            chosen?.stop()
        }
        guard let node = chosen else { return }
        node.volume = soundVolume
        node.scheduleFile(audioFile, at: nil, completionHandler: nil)
        if !node.isPlaying { node.play() }
        log("[AVAudioHandler] Sound playing on pool node: \(audioFile.url.lastPathComponent)")
    }

    // MARK: - Music Looping

    private func setupMusicLoopObserverIfNeeded() {
        // Remove any existing observer
        if let observer = musicEndObserver {
            NotificationCenter.default.removeObserver(observer)
            musicEndObserver = nil
        }

        guard isMusicLooping, let item = musicPlayer?.currentItem else { return }

        // Create observer for when music ends
        musicEndObserver = NotificationCenter.default.addObserver(
            forName: .AVPlayerItemDidPlayToEndTime,
            object: item,
            queue: .main
        ) { [weak self] _ in
            guard let self = self else { return }
            Task { @MainActor in
                self.handleMusicEnd()
            }
        }
    }

    private func handleMusicEnd() {
        log("Music ended - looping")
        musicPlayer?.seek(to: .zero)
        musicPlayer?.play()
    }

    // MARK: - Notification Setup

    private func setupNotificationObservers() {
        // Route change notifications
        routeChangeObserver = NotificationCenter.default.addObserver(
            forName: AVAudioSession.routeChangeNotification,
            object: nil,
            queue: nil
        ) { [weak self] notification in
            self?.handleRouteChangeNotification(notification)
        }

        // Interruption notifications
        interruptionObserver = NotificationCenter.default.addObserver(
            forName: AVAudioSession.interruptionNotification,
            object: nil,
            queue: nil
        ) { [weak self] notification in
            self?.handleInterruptionNotification(notification)
        }

        // Media services reset
        mediaResetObserver = NotificationCenter.default.addObserver(
            forName: AVAudioSession.mediaServicesWereResetNotification,
            object: nil,
            queue: nil
        ) { [weak self] _ in
            self?.handleMediaServicesResetNotification()
        }
    }

    private func cleanupNotificationObservers() {
        if let observer = routeChangeObserver {
            NotificationCenter.default.removeObserver(observer)
            routeChangeObserver = nil
        }
        if let observer = interruptionObserver {
            NotificationCenter.default.removeObserver(observer)
            interruptionObserver = nil
        }
        if let observer = mediaResetObserver {
            NotificationCenter.default.removeObserver(observer)
            mediaResetObserver = nil
        }
    }
}

// MARK: - Notification Handlers (nonisolated)

extension AVAudioHandler {

    // These methods are nonisolated to be safely called from notification callbacks

    nonisolated private func handleRouteChangeNotification(_ notification: Notification) {
        Task { @MainActor in
            self.handleRouteChange()
        }
    }

    nonisolated private func handleInterruptionNotification(_ notification: Notification) {
        Task { @MainActor in
            self.handleInterruption()
        }
    }

    nonisolated private func handleMediaServicesResetNotification() {
        Task { @MainActor in
            self.handleMediaServicesReset()
        }
    }

    // MARK: - MainActor handlers

    private func handleRouteChange() {
        log("Audio route changed")

        // Restart engine if needed after route change
        if isInitialized && !audioEngine.isRunning {
            _ = ensureEngineRunning()
        }
    }

    private func handleInterruption() {
        log("Audio interruption occurred")

        // Simple recovery - ensure engine is running
        if isInitialized && !audioEngine.isRunning {
            _ = ensureEngineRunning()
        }
    }

    private func handleMediaServicesReset() {
        log("Media services were reset - reinitializing")

        // Full reset required
        stopEngine()
        isInitialized = false
        isEngineRunning = false
        soundCache.removeAll()

        // Cleanup observers before reinit
        cleanupNotificationObservers()

        // Re-initialize
        _ = initialize()
    }
}

// MARK: - Error Types

public enum AudioError: LocalizedError {
    case fileNotFound(String)
    case invalidFormat(String)
    case engineNotRunning

    public var errorDescription: String? {
        switch self {
        case .fileNotFound(let fileName):
            return "Audio file not found: \(fileName)"
        case .invalidFormat(let fileName):
            return "Invalid audio format: \(fileName)"
        case .engineNotRunning:
            return "Audio engine is not running"
        }
    }
}
