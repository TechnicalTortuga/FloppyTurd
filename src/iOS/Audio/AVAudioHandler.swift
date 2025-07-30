//
//  AVAudioHandler.swift
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

import AVFoundation
import os.log

/**
 * Modern iOS Audio Manager following Apple's 2024 best practices
 * 
 * Key improvements:
 * - Single AVAudioEngine instance (Apple recommended)
 * - AVAudioPlayerNode for individual sources
 * - Async/await resource loading
 * - Proper AVAudioSession configuration
 * - Simplified, thread-safe architecture
 */
@MainActor
public class AVAudioHandler: NSObject {
    
    // MARK: - Core Audio Engine Components
    
    private let audioEngine = AVAudioEngine()
    private let musicPlayerNode = AVAudioPlayerNode()
    private let soundPlayerNode = AVAudioPlayerNode()
    private let mixerNode = AVAudioMixerNode()
    
    // MARK: - Audio Session
    
    private let audioSession = AVAudioSession.sharedInstance()
    
    // MARK: - State Management
    
    private var isInitialized = false
    private var currentMusicFile: AVAudioFile?
    private var musicVolume: Float = 0.7
    private var soundVolume: Float = 1.0
    
    // MARK: - Logging
    
    private let logger = Logger(subsystem: "com.floppyturd.game", category: "Audio")
    
    // MARK: - Initialization
    
    public override init() {
        super.init()
        logger.info("AVAudioHandler initialized with modern architecture")
    }
    
    // MARK: - Public Interface
    
    public func initialize() -> Bool {
        guard !isInitialized else { 
            logger.info("[AVAudioHandler] Already initialized")
            return true 
        }
        
        logger.info("[AVAudioHandler] Starting audio system initialization")
        
        do {
            logger.info("[AVAudioHandler] Setting up audio session...")
            try setupAudioSession()
            
            logger.info("[AVAudioHandler] Setting up audio engine...")
            setupAudioEngine()
            
            logger.info("[AVAudioHandler] Starting audio engine...")
            try audioEngine.start()
            
            isInitialized = true
            logger.info("Audio system initialized successfully")
            return true
            
        } catch {
            logger.error("Failed to initialize audio system: \(error.localizedDescription)")
            return false
        }
    }
    
    public func playMusic(_ fileName: String) {
        print("🚨 DIRECT PRINT: AVAudioHandler.playMusic() called with: \(fileName)")
        logger.info("🎵 [DEBUG] AVAudioHandler.playMusic() called with: \(fileName)")
        
        // Auto-initialize if not already done
        if !isInitialized {
            logger.info("[AVAudioHandler] Auto-initializing audio system")
            if !initialize() {
                logger.error("Failed to auto-initialize audio system")
                return
            } else {
                logger.info("[AVAudioHandler] Audio system auto-initialized successfully")
            }
        } else {
            logger.info("[AVAudioHandler] Audio system already initialized")
        }
        
        // First, check AssetManager's cache for pre-loaded audio files
        logger.info("[AVAudioHandler] Checking AssetManager cache for: \(fileName)")
        
        let extensions = ["mp3", "ogg", "wav", "m4a"]
        var audioFile: AVAudioFile?
        
        // Try to find the file in AssetManager's cache with different extensions
        for ext in extensions {
            if let cachedAudioFile = AssetManager.shared.getCachedAudio(name: fileName, extension: ext) {
                logger.info("[AVAudioHandler] Found cached audio file: \(fileName).\(ext)")
                audioFile = cachedAudioFile
                break
            }
        }
        
        // If not found in cache, try loading directly from Bundle.main (fallback)
        if audioFile == nil {
            logger.info("[AVAudioHandler] Not found in cache, loading from Bundle.main: \(fileName)")
            
            let mainBundle = Bundle.main
            var foundURL: URL?
            
            // First try with exact filename
            if let url = mainBundle.url(forResource: fileName, withExtension: nil) {
                foundURL = url
            } else {
                // Try with common audio extensions
                for ext in extensions {
                    logger.info("[AVAudioHandler] Trying extension: .\(ext)")
                    if let testURL = mainBundle.url(forResource: fileName, withExtension: ext) {
                        foundURL = testURL
                        logger.info("[AVAudioHandler] Found file with extension .\(ext): \(testURL)")
                        break
                    }
                }
            }
            
            guard let url = foundURL else {
                logger.error("Audio file not found: \(fileName)")
                return
            }
            
            // Load audio file from URL
            logger.info("[AVAudioHandler] Attempting to create AVAudioFile from: \(url)")
            do {
                audioFile = try AVAudioFile(forReading: url)
                logger.info("[AVAudioHandler] Successfully created AVAudioFile from Bundle")
            } catch {
                logger.error("Failed to create AVAudioFile from: \(url) - \(error)")
                return
            }
        }
        
        // Schedule the audio file for playback
        if let audioFile = audioFile {
            logger.info("[AVAudioHandler] Scheduling music: \(audioFile.url.lastPathComponent)")
            scheduleMusic(audioFile)
        } else {
            logger.error("No audio file available for playback: \(fileName)")
        }
    }
    
    public func playSound(_ fileName: String) {
        logger.info("🔊 [DEBUG] AVAudioHandler.playSound() called with: \(fileName)")
        
        // Auto-initialize if not already done
        if !isInitialized {
            if !initialize() {
                logger.error("Failed to auto-initialize audio system")
                return
            }
        }
        
        // First, check AssetManager's cache for pre-loaded audio files
        let extensions = ["ogg", "mp3", "wav", "m4a"]
        var audioFile: AVAudioFile?
        
        // Try to find the file in AssetManager's cache with different extensions
        for ext in extensions {
            if let cachedAudioFile = AssetManager.shared.getCachedAudio(name: fileName, extension: ext) {
                logger.info("[AVAudioHandler] Found cached sound file: \(fileName).\(ext)")
                audioFile = cachedAudioFile
                break
            }
        }
        
        // If not found in cache, try loading directly from Bundle.main (fallback)
        if audioFile == nil {
            logger.info("[AVAudioHandler] Sound not found in cache, loading from Bundle.main: \(fileName)")
            
            let mainBundle = Bundle.main
            var foundURL: URL?
            
            // First try with exact filename
            if let url = mainBundle.url(forResource: fileName, withExtension: nil) {
                foundURL = url
            } else {
                // Try with common audio extensions
                for ext in extensions {
                    if let testURL = mainBundle.url(forResource: fileName, withExtension: ext) {
                        foundURL = testURL
                        break
                    }
                }
            }
            
            guard let url = foundURL else {
                logger.error("Audio file not found: \(fileName)")
                return
            }
            
            // Load audio file from URL
            do {
                audioFile = try AVAudioFile(forReading: url)
                logger.info("[AVAudioHandler] Successfully created sound AVAudioFile from Bundle")
            } catch {
                logger.error("Failed to create AVAudioFile from: \(url) - \(error)")
                return
            }
        }
        
        // Schedule the audio file for playback
        if let audioFile = audioFile {
            scheduleSound(audioFile)
        } else {
            logger.error("No sound file available for playback: \(fileName)")
        }
    }
    
    /**
     * @brief Play a sound effect
     * @param soundName Name of the sound file to play
     * @param volume Volume level (0.0 to 1.0)
     */
    public func playSoundWithVolume(_ soundName: String, volume: Float) {
        logger.info("🔊 [DEBUG] AVAudioHandler.playSoundWithVolume() called with: \(soundName), volume: \(volume)")
        
        // Auto-initialize if not already done
        if !isInitialized {
            if !initialize() {
                logger.error("Failed to auto-initialize audio system")
                return
            }
        }
        
        // First, check AssetManager's cache for pre-loaded audio files
        let extensions = ["ogg", "mp3", "wav", "m4a"]
        var audioFile: AVAudioFile?
        
        // Try to find the file in AssetManager's cache with different extensions
        for ext in extensions {
            if let cachedAudioFile = AssetManager.shared.getCachedAudio(name: soundName, extension: ext) {
                logger.info("[AVAudioHandler] Found cached sound file: \(soundName).\(ext)")
                audioFile = cachedAudioFile
                break
            }
        }
        
        // If not found in cache, try loading directly from Bundle.main (fallback)
        if audioFile == nil {
            logger.info("[AVAudioHandler] Sound not found in cache, loading from Bundle.main: \(soundName)")
            
            let mainBundle = Bundle.main
            var foundURL: URL?
            
            // First try with exact filename
            if let url = mainBundle.url(forResource: soundName, withExtension: nil) {
                foundURL = url
            } else {
                // Try with common audio extensions
                for ext in extensions {
                    if let testURL = mainBundle.url(forResource: soundName, withExtension: ext) {
                        foundURL = testURL
                        break
                    }
                }
            }
            
            guard let url = foundURL else {
                logger.error("Audio file not found: \(soundName)")
                return
            }
            
            // Load audio file from URL
            do {
                audioFile = try AVAudioFile(forReading: url)
                logger.info("[AVAudioHandler] Successfully created sound AVAudioFile from Bundle")
            } catch {
                logger.error("Failed to create AVAudioFile from: \(url) - \(error)")
                return
            }
        }
        
        // Schedule the audio file for playback
        if let audioFile = audioFile {
            soundPlayerNode.volume = max(0.0, min(1.0, volume))
            scheduleSound(audioFile)
        } else {
            logger.error("No sound file available for playback: \(soundName)")
        }
    }
    
    public func stopMusic() {
        musicPlayerNode.stop()
        logger.info("Music stopped")
    }
    
    public func stopSound() {
        soundPlayerNode.stop()
        logger.info("All sounds stopped")
    }
    
    public func setMusicVolume(volume: Float) {
        musicVolume = max(0.0, min(1.0, volume))
        musicPlayerNode.volume = musicVolume
        logger.info("Music volume set to: \(self.musicVolume)")
    }
    
    public func setSoundVolume(volume: Float) {
        soundVolume = max(0.0, min(1.0, volume))
        soundPlayerNode.volume = soundVolume
        logger.info("Sound volume set to: \(self.soundVolume)")
    }
    
    // MARK: - Private Implementation
    
    private func setupAudioSession() throws {
        try audioSession.setCategory(
            .playback,
            mode: .gameChat,
            options: [.defaultToSpeaker, .allowBluetooth]
        )
        
        try audioSession.setPreferredSampleRate(44100.0)
        try audioSession.setPreferredIOBufferDuration(0.005) // 5ms for low latency
        try audioSession.setActive(true)
        
        logger.info("Audio session configured: rate=\(self.audioSession.sampleRate)Hz")
    }
    
    private func setupAudioEngine() {
        // Attach nodes to the engine
        audioEngine.attach(musicPlayerNode)
        audioEngine.attach(soundPlayerNode)
        audioEngine.attach(mixerNode)
        
        // Connect the audio graph
        // Music -> Mixer -> Output
        audioEngine.connect(musicPlayerNode, to: mixerNode, format: nil)
        audioEngine.connect(soundPlayerNode, to: mixerNode, format: nil)
        audioEngine.connect(mixerNode, to: audioEngine.outputNode, format: nil)
        
        // Set initial volumes
        musicPlayerNode.volume = musicVolume
        soundPlayerNode.volume = soundVolume
        
        // Prepare the engine
        audioEngine.prepare()
        
        logger.info("Audio engine configured with mixer topology")
    }
    
    private func loadAudioFile(_ fileName: String) async throws -> AVAudioFile {
        return try await withCheckedThrowingContinuation { continuation in
            // Perform file loading on a background queue
            DispatchQueue.global(qos: .userInitiated).async {
                do {
                    // Try to find the file with various extensions
                    let extensions = ["ogg", "mp3", "wav", "m4a", "aiff"]
                    var fileURL: URL?
                    
                    // First try without extension
                    if let url = Bundle.main.url(forResource: fileName, withExtension: nil) {
                        fileURL = url
                    } else {
                        // Try with each extension
                        for ext in extensions {
                            if let url = Bundle.main.url(forResource: fileName, withExtension: ext) {
                                fileURL = url
                                break
                            }
                        }
                    }
                    
                    guard let url = fileURL else {
                        throw AudioError.fileNotFound(fileName)
                    }
                    
                    let audioFile = try AVAudioFile(forReading: url)
                    continuation.resume(returning: audioFile)
                    
                } catch {
                    continuation.resume(throwing: error)
                }
            }
        }
    }
    
    private func scheduleMusic(_ audioFile: AVAudioFile) {
        logger.info("[AVAudioHandler] scheduleMusic() called")
        
        guard audioEngine.isRunning else {
            logger.error("Cannot schedule music: audio engine not running")
            return
        }
        
        logger.info("[AVAudioHandler] Audio engine is running, stopping current music")
        musicPlayerNode.stop()
        currentMusicFile = audioFile
        
        logger.info("[AVAudioHandler] Scheduling audio file: \(audioFile.url.lastPathComponent)")
        // Use synchronous scheduleFile (Apple's recommended approach for basic audio)
        musicPlayerNode.scheduleFile(audioFile, at: nil) { [weak self] in
            // Schedule looping on completion
            DispatchQueue.main.async { [weak self] in
                if let self = self, let file = self.currentMusicFile {
                    self.logger.info("[AVAudioHandler] Music finished, rescheduling for loop")
                    self.scheduleMusic(file)
                }
            }
        }
        
        // Start playback if not already playing
        if !musicPlayerNode.isPlaying {
            logger.info("[AVAudioHandler] Starting music playback")
            musicPlayerNode.play()
        } else {
            logger.info("[AVAudioHandler] Music player already playing")
        }
        
        logger.info("Music scheduled and playing: \(audioFile.url.lastPathComponent)")
    }
    
    private func scheduleSound(_ audioFile: AVAudioFile) {
        guard audioEngine.isRunning else {
            logger.error("Cannot schedule sound: audio engine not running")
            return
        }
        
        // Use synchronous scheduleFile (Apple's recommended approach for basic audio)
        soundPlayerNode.scheduleFile(audioFile, at: nil) { [weak self] in
            self?.logger.info("Sound playback completed")
        }
        
        // Start playback if not already playing
        if !soundPlayerNode.isPlaying {
            soundPlayerNode.play()
        }
        
        logger.info("Sound scheduled and playing: \(audioFile.url.lastPathComponent)")
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