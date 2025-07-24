//
//  GameEngine.swift
//  FloppyTurd
//
//  Swift implementation of game engine using Swift 5.9+ native C++ interop
//  Direct C++ instantiation: auto engine = std::make_unique<FloppyTurd::GameEngine>();
//  Created by Carl the Turdsmith on 2024
//  Copyright © 2024 FloppyTurd. All rights reserved.
//

import Foundation
import UIKit
import Metal
import QuartzCore
import FloppyTurdEngine
import FloppyTurdGame

/// GameEngine - Swift implementation with native C++ interop
/// Direct C++ instantiation: std::make_unique<FloppyTurd::GameEngine>()
/// Manages game lifecycle and coordinates between iOS and C++ systems
/// @MainActor ensures all GameEngine operations happen on the main thread
/// This is required for Swift 6 concurrency safety with UI-related operations
@MainActor
public class GameEngine: NSObject {
    
    // MARK: - Properties
    private var isInitialized: Bool = false
    private var isRunning: Bool = false
    private var isPaused: Bool = false
    
    // C++ game instance (direct C++ class instantiation)
    private var cppGame: FloppyTurd.FloppyTurdGame?
    
    // iOS subsystems
    private var metalRenderer: MetalRenderer?
    private var touchInputHandler: TouchInputHandler?
    private var audioManager: AVAudioHandler?
    
    // iOS-specific game loop using CADisplayLink
    private var displayLink: CADisplayLink?
    private var lastFrameTime: CFTimeInterval = 0
    
    // GNLog integration
    private func log(_ message: String, level: LogLevel = .info) {
        Task { @Sendable in
            switch level {
            case .trace:
                await SwiftLog.debug(message, category: "GameEngine")
            case .debug:
                await SwiftLog.debug(message, category: "GameEngine")
            case .info:
                await SwiftLog.info(message, category: "GameEngine")
            case .warning:
                await SwiftLog.warn(message, category: "GameEngine")
            case .error:
                await SwiftLog.error(message, category: "GameEngine")
            case .fatal:
                await SwiftLog.fatal(message, category: "GameEngine")
            }
        }
    }
    
    // MARK: - Initialization
    
    public override init() {
        super.init()
        log("GameEngine Swift bridge created")
    }
    
    deinit {
        // Note: Cannot call @MainActor isolated methods from deinit
        // Cleanup should be handled explicitly via shutdown() before deallocation
        // Swift will handle automatic cleanup of properties
    }
    
    // MARK: - Lifecycle Management
    
    /// Initialize the game engine and all subsystems
    public func initialize() -> Bool {
        guard !isInitialized else {
            log("GameEngine already initialized", level: .warning)
            return true
        }
        
        log("Initializing GameEngine...")
        
        // Create platform components first
        if metalRenderer == nil {
            metalRenderer = MetalRenderer()
            log("Created MetalRenderer")
        }
        
        if touchInputHandler == nil {
            touchInputHandler = TouchInputHandler()
            log("Created TouchInputHandler")
        }
        
        if audioManager == nil {
            audioManager = AVAudioHandler()
            log("Created AudioManager")
        }
        
        // Create and initialize C++ game instance
        cppGame = FloppyTurd.FloppyTurdGame()
        
        // Initialize with Swift components using the new iOS-specific method
        guard let renderer = metalRenderer,
              let inputHandler = touchInputHandler,
              let audio = audioManager else {
            log("Failed to create platform components", level: .error)
            return false
        }
        
        // Use SetSwiftComponents method for iOS
        cppGame?.SetSwiftComponents(
            Unmanaged.passUnretained(renderer).toOpaque(),
            Unmanaged.passUnretained(inputHandler).toOpaque(),
            Unmanaged.passUnretained(audio).toOpaque()
        )
        
        // Now initialize the C++ game
        let success = cppGame?.Initialize() ?? false
        
        guard success else {
            log("Failed to initialize C++ game with Swift components", level: .error)
            cppGame = nil
            return false
        }
        
        isInitialized = true
        log("GameEngine initialized successfully with Swift components")
        return true
    }
    
    /// Shutdown the game engine and cleanup resources
    public func shutdown() {
        guard isInitialized else { return }
        
        log("Shutting down GameEngine...")
        
        // Stop the game loop if running
        if isRunning {
            stop()
        }
        
        // Shutdown C++ game
        cppGame?.Shutdown()
        cppGame = nil
        
        // Cleanup iOS subsystems
        metalRenderer = nil
        touchInputHandler = nil
        audioManager = nil
        
        isInitialized = false
        log("GameEngine shutdown complete")
    }
    
    /// Start the game loop using iOS-compatible frame-based approach
    /// @MainActor ensures this runs on the main thread for Swift 6 concurrency safety
    public func start() -> Bool {
        guard isInitialized && !isRunning else {
            log("Cannot start game - not initialized or already running", level: .warning)
            return false
        }

        log("Starting iOS-compatible game loop...")
        
        guard cppGame != nil else {
            log("C++ game instance is null", level: .error)
            return false
        }

        // Start the C++ game state (but don't call the blocking Run() method)
        cppGame?.StartGame()
        
        // Set up iOS-compatible frame-based rendering using CADisplayLink
        setupDisplayLink()
        
        isRunning = true
        isPaused = false
        log("iOS-compatible game loop started successfully")
        
        return true
    }
    
    /// Stop the game loop
    public func stop() {
        guard isRunning else { return }
        
        log("Stopping game loop...")
        
        // Stop the display link
        displayLink?.invalidate()
        displayLink = nil
        
        // Stop the C++ game
        cppGame?.EndGame()
        
        isRunning = false
        isPaused = false
        log("Game loop stopped")
    }
    
    /// Pause the game
    public func pause() {
        guard isRunning && !isPaused else { return }
        
        log("Pausing game...")
        
        // Pause the display link
        displayLink?.isPaused = true
        
        cppGame?.PauseGame()
        
        isPaused = true
        log("Game paused")
    }
    
    /// Resume the game
    public func resume() {
        guard isRunning && isPaused else { return }
        
        log("Resuming game...")
        
        // Resume the display link and reset frame timing
        displayLink?.isPaused = false
        lastFrameTime = CACurrentMediaTime()
        
        cppGame?.ResumeGame()
        
        isPaused = false
        log("Game resumed")
    }
    
    // MARK: - Game State
    
    public func isGameInitialized() -> Bool {
        return isInitialized
    }
    
    public func isGameRunning() -> Bool {
        return isRunning
    }
    
    public func isGamePaused() -> Bool {
        return isPaused
    }
    
    // MARK: - Subsystem Management
    
    /// Set the Metal renderer for the game
    public func setMetalRenderer(_ renderer: MetalRenderer) {
        metalRenderer = renderer
        log("Metal renderer set", level: .debug)
        
        // TODO: Pass Metal renderer to C++ game
        // if let game = cppGame {
        //     game.SetRenderer(renderer)
        // }
    }
    
    /// Set the touch input handler for the game
    public func setTouchInputHandler(_ handler: TouchInputHandler) {
        touchInputHandler = handler
        log("Touch input handler set", level: .debug)
        
        // TODO: Pass touch input handler to C++ game
        // if let game = cppGame {
        //     game.SetInputHandler(handler)
        // }
    }
    
    // MARK: - Game Loop Integration
    
    /// Update the game (called from Metal render loop)
    public func update(deltaTime: Float) {
        guard isRunning && !isPaused else { return }
        
        cppGame?.Update(deltaTime)
    }
    
    /// Render the game (called from Metal render loop)
    public func render() {
        guard isRunning && !isPaused else { return }
        
        // For now, directly call MetalRenderer to test blue screen rendering
        // TODO: Connect MetalRenderer to C++ game properly
        if let renderer = metalRenderer {
            renderer.beginFrame()
            renderer.setClearColor(r: 0.0, g: 0.5, b: 1.0, a: 1.0) // Blue background
            renderer.clear()
            
            // Draw a simple white rectangle as a test
            renderer.drawRectangle(x: 100, y: 100, width: 200, height: 100, r: 1.0, g: 1.0, b: 1.0, a: 1.0)
            
            renderer.endFrame()
            renderer.present()
        }
        
        // Call C++ game render (currently mostly commented out)
        cppGame?.Render()
    }
    
    // MARK: - Game Access
    
    // Note: Direct access to cppGame property is available
    // No getter function needed - use gameEngine.cppGame directly
    
    // MARK: - iOS Display Link Integration
    
    /// Set up CADisplayLink for iOS-compatible frame-based rendering
    private func setupDisplayLink() {
        displayLink = CADisplayLink(target: self, selector: #selector(frameUpdate))
        displayLink?.add(to: .main, forMode: .default)
        lastFrameTime = CACurrentMediaTime()
        log("CADisplayLink setup complete for frame-based rendering")
    }
    
    /// Frame update callback called by CADisplayLink
    @objc private func frameUpdate() {
        guard isRunning && !isPaused else { 
            return 
        }
        
        let currentTime = CACurrentMediaTime()
        let deltaTime = Float(currentTime - lastFrameTime)
        lastFrameTime = currentTime
        
        // Call individual C++ methods for iOS-compatible frame-based rendering
        cppGame?.HandleInput()
        cppGame?.Update(deltaTime)
        cppGame?.Render()
    }
}

// MARK: - C++ Integration Notes

/**
 * Swift 5.9+ Native C++ Interop Integration
 * 
 * This GameEngine class now uses direct C++ class instantiation:
 * 
 * // Swift to C++ (current implementation):
 * cppGame = FloppyTurd.FloppyTurdGame()
 * cppGame?.Initialize(platform)
 * cppGame?.Run()
 * 
 * // C++ to Swift (if needed):
 * #include "GameEngine-Swift.h"
 * auto gameEngine = std::make_unique<FloppyTurd::GameEngine>();
 * 
 * The FloppyTurdGame C++ class is directly accessible from Swift
 * through the module.modulemap configuration and Swift 5.9+ interop.
 */