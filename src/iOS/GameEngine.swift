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
import GameCoreEngine
import GameCoreGame
import GameCorePlatform
import Metal
import QuartzCore
import UIKit

// import FloppyTurdGame (C++ class is available via module.modulemap and C++ interop)

/// GameEngine - Swift implementation with native C++ interop
/// Direct C++ instantiation: std::make_unique<FloppyTurd::GameEngine>()
/// Manages game lifecycle and coordinates between iOS and C++ systems
/// @MainActor ensures all GameEngine operations happen on the main thread
/// This is required for Swift 6 concurrency safety with UI-related operations
@MainActor
public class GameEngine: NSObject, TouchInputDelegate {

    // MARK: - Properties

    private var isInitialized = false
    private var isRunning = false
    private var isPaused = false
    private var lastFrameTime: CFTimeInterval = 0

    // Platform-specific managers
    private var metalRenderer: MetalRenderer?
    private var touchInputHandler: TouchInputHandler?
    private var audioManager: AVAudioHandler?
    private var commandProcessor: CommandProcessor?

    // C++ Game Engine instance
    private var cppGame: GameCoreGame.GameCore.FloppyTurdGame?

    // GNLog integration - Direct SwiftLog (like ThreadingSystem)
    private func log(_ message: String, level: LogLevel = .info) {
        switch level {
        case .trace:
            SwiftLog.debug(message, category: "GameEngine")
        case .debug:
            SwiftLog.debug(message, category: "GameEngine")
        case .info:
            SwiftLog.info(message, category: "GameEngine")
        case .warning:
            SwiftLog.warn(message, category: "GameEngine")
        case .error:
            SwiftLog.error(message, category: "GameEngine")
        case .fatal:
            SwiftLog.fatal(message, category: "GameEngine")
        }
    }

    // MARK: - Initialization

    public override init() {
        // Use direct SwiftLog (like ThreadingSystem) to avoid Task/await delays
        SwiftLog.info("🚨 GameEngine.init() STARTED", category: "GameEngine")

        super.init()

        // Create command processor (this initializes the ThreadingProxy)
        commandProcessor = CommandProcessor()

        SwiftLog.info("🚨 GameEngine.super.init() COMPLETED", category: "GameEngine")

        SwiftLog.info("🚨 GameEngine Swift bridge created successfully", category: "GameEngine")
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

        // Set up iOS logging system first
        log("Setting up iOS logging system...")
        setupIOSLogging()
        log("iOS logging system configured")

        // Create platform components first
        if metalRenderer == nil {
            metalRenderer = MetalRenderer()
            log("Created MetalRenderer")
        }

        if touchInputHandler == nil {
            touchInputHandler = TouchInputHandler()
            touchInputHandler?.delegate = self
            log("Created TouchInputHandler with GameEngine as delegate")
        }

        // Create audio manager
        log("Creating AVAudioHandler...")
        audioManager = AVAudioHandler()
        log("AVAudioHandler created successfully - instance: \(ObjectIdentifier(audioManager!))")

        // Set up the command processor with Swift components
        log("Setting up CommandProcessor with Swift components...")
        commandProcessor?.setMetalRenderer(metalRenderer!)
        commandProcessor?.setAudioManager(audioManager!)
        log("CommandProcessor configured with Swift components")

        // Ensure ThreadingProxy is properly initialized before C++ game creation
        log("Ensuring ThreadingProxy is initialized...")
        GameCorePlatform.GameCore.initializeThreadingSystem()
        log("ThreadingProxy initialization confirmed")

        // Create C++ Game Engine - This returns the main game instance
        log("Creating C++ FloppyTurdGame object...")
        cppGame = GameCoreGame.GameCore.FloppyTurdGame()

        guard cppGame != nil else {
            log("Failed to create C++ FloppyTurdGame object", level: .error)
            return false
        }

        log("C++ FloppyTurdGame object created successfully")

        log("Swift components already created - ready to initialize C++ game")

        // Set up screen info before initializing C++ game
        setupScreenInfo()

        // Now initialize the C++ game
        let success = cppGame?.Initialize() ?? false

        log("C++ game Initialize() returned: \(success)")

        guard success else {
            log("Failed to initialize C++ game with Swift components", level: .error)
            cppGame = nil
            return false
        }

        log("C++ game initialized successfully - LoadingState should be active")

        // Start the game (this sets m_running = true)
        cppGame?.StartGame()
        log("StartGame() called - game is now running")

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
        commandProcessor = nil

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

        // Initialize frame timing for MTKView-driven rendering
        lastFrameTime = CACurrentMediaTime()

        isRunning = true
        isPaused = false
        log("Game started - now driven by MTKView draw loop with TouchInputHandler delegate system")

        return true
    }

    /// Stop the game loop
    public func stop() {
        guard isRunning else { return }

        log("Stopping game loop...")

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

        cppGame?.PauseGame()

        isPaused = true
        log("Game paused")
    }

    /// Resume the game
    public func resume() {
        guard isRunning && isPaused else { return }

        log("Resuming game...")

        // Reset frame timing for MTKView-driven rendering
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

        // Update the CommandProcessor with the new renderer
        commandProcessor?.setMetalRenderer(renderer)

        log("Metal renderer set", level: .debug)

        // TODO: Pass Metal renderer to C++ game
        // if let game = cppGame {
        //     game.SetRenderer(renderer)
        // }
    }

    /// Set the audio manager for the game
    public func setAudioManager(_ manager: AVAudioHandler) {
        audioManager = manager

        // Update the CommandProcessor with the new audio manager
        commandProcessor?.setAudioManager(manager)

        log("Audio manager set", level: .debug)
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

    /// Get the touch input handler for the game
    public func getTouchInputHandler() -> TouchInputHandler? {
        return touchInputHandler
    }

    /// Handle touch input and forward to C++ game state manager
    public func handleTouchInput(_ input: Any) {
        guard isRunning && !isPaused else { return }
        guard cppGame != nil else {
            log("Cannot handle touch input - C++ game not initialized", level: .warning)
            return
        }

        // Forward touch input to C++ game state manager
        cppGame?.HandleInput()
        log("Touch input forwarded to C++ state manager", level: .debug)
    }

    /// Handle touch press events and forward to C++ game state manager
    /// Update gesture state and forward to C++ game state manager
    public func updateGestureState(
        swipeLeft: Bool, swipeRight: Bool, swipeUp: Bool, swipeDown: Bool
    ) {
        guard isRunning && !isPaused else { return }
        guard cppGame != nil else {
            log("Cannot update gesture state - C++ game not initialized", level: .warning)
            return
        }

        log(
            "🎯 Gesture state: Left=\(swipeLeft), Right=\(swipeRight), Up=\(swipeUp), Down=\(swipeDown)",
            level: .debug)

        // Update gesture state in ThreadingProxy for C++ side to access
        GameCore.updateGestureState(swipeLeft, swipeRight, swipeUp, swipeDown)

        // Forward gesture input to C++ game state manager
        cppGame?.HandleInput()
        log("Gesture state forwarded to C++ state manager", level: .debug)
    }

    // MARK: - Game Loop Integration

    /// Update the game (called from Metal render loop)
    public func update(deltaTime: Float) {
        guard isRunning && !isPaused else { return }

        // Update TouchInputHandler (handles frame-based input processing internally)
        touchInputHandler?.update()

        cppGame?.Update(deltaTime)

        // Reset input frame state after C++ game has processed input
        GameCore.resetInputFrameState()
    }

    /// Render the game (called from Metal render loop)
    public func render() {
        guard isRunning && !isPaused else { return }

        log("GameEngine.render() called", level: .debug)

        // Call C++ game's Render() function to queue render commands
        log("About to call cppGame?.Render()", level: .debug)
        cppGame?.Render()
        log("Finished calling cppGame?.Render()", level: .debug)

        // Process any queued C++ render commands synchronously
        // This includes beginFrame, clearScreen, draw calls, endFrame, and present
        commandProcessor?.processCommands()

        log("GameEngine.render() completed", level: .debug)
    }

    // MARK: - Game Access

    // Note: Direct access to cppGame property is available
    // No getter function needed - use gameEngine.cppGame directly

    // MARK: - MTKView Integration

    /// Update delta time for MTKView-driven rendering
    public func updateDeltaTime() -> Float {
        let currentTime = CACurrentMediaTime()
        let deltaTime = Float(currentTime - lastFrameTime)
        lastFrameTime = currentTime
        return deltaTime
    }

    // MARK: - iOS Logging Setup

    /// Set up iOS logging system with GNLog integration
    private func setupIOSLogging() {
        // Create iOS log handler
        let iosLogHandler = iOSLogHandler()

        // Initialize the iOS log handler
        let success = iosLogHandler.initialize()
        if success {
            log("iOS log handler initialized successfully")
        } else {
            log("Failed to initialize iOS log handler", level: .error)
        }

        // Add the iOS log handler to the GNLog system
        // This should be done through the C++ interop system
        // The iOS log handler should be automatically integrated when the C++ game is initialized
        log(
            "iOS logging system setup completed - will be integrated with GNLog when C++ game initializes"
        )
    }

    // MARK: - TouchInputDelegate Implementation

    /// Handle touch press events from TouchInputHandler
    public func onTouchPress(normalizedPosition: CGPoint, viewSize: CGSize) {
        guard isRunning && !isPaused else { return }
        guard cppGame != nil else {
            log("Cannot handle touch press - C++ game not initialized", level: .warning)
            return
        }

        // Map normalized coordinates to actual device pixel dimensions for 1:1 hit-testing
        let pixelBounds = UIScreen.main.nativeBounds
        let gameX = Float(normalizedPosition.x) * Float(pixelBounds.width)
        let gameY = Float(normalizedPosition.y) * Float(pixelBounds.height)

        log(
            "🎯 Touch PRESS: normalized (\(normalizedPosition.x), \(normalizedPosition.y)) -> game coords (\(gameX), \(gameY))",
            level: .debug)

        // Update touch state in ThreadingProxy for C++ side to access
        GameCore.updateTouchState(gameX, gameY, true, true, false)  // PRESS EVENT

        // Forward input event to C++ game state manager
        cppGame?.HandleInput()
    }

    /// Handle touch move events from TouchInputHandler
    public func onTouchMove(normalizedPosition: CGPoint, viewSize: CGSize) {
        guard isRunning && !isPaused else { return }
        guard cppGame != nil else {
            log("Cannot handle touch move - C++ game not initialized", level: .warning)
            return
        }
        let pixelBounds = UIScreen.main.nativeBounds
        let gameX = Float(normalizedPosition.x) * Float(pixelBounds.width)
        let gameY = Float(normalizedPosition.y) * Float(pixelBounds.height)
        // Update touch state as down without press/release flags
        GameCore.updateTouchState(gameX, gameY, true, false, false)
        cppGame?.HandleInput()
    }

    /// Handle touch release events from TouchInputHandler
    public func onTouchRelease(normalizedPosition: CGPoint, viewSize: CGSize) {
        guard isRunning && !isPaused else { return }
        guard cppGame != nil else {
            log("Cannot handle touch release - C++ game not initialized", level: .warning)
            return
        }

        // Map normalized coordinates to actual device pixel dimensions for 1:1 hit-testing
        let pixelBounds = UIScreen.main.nativeBounds
        let gameX = Float(normalizedPosition.x) * Float(pixelBounds.width)
        let gameY = Float(normalizedPosition.y) * Float(pixelBounds.height)

        log(
            "🎯 Touch RELEASE: normalized (\(normalizedPosition.x), \(normalizedPosition.y)) -> game coords (\(gameX), \(gameY))",
            level: .debug)

        // Update touch state in ThreadingProxy for C++ side to access
        GameCore.updateTouchState(gameX, gameY, false, false, true)  // RELEASE EVENT

        // Forward input event to C++ game state manager
        cppGame?.HandleInput()
    }

    // MARK: - Screen Info Management

    private func setupScreenInfo() {
        guard let metalRenderer = metalRenderer else {
            log("Cannot setup screen info - MetalRenderer not available", level: .error)
            return
        }

        log("Setting up screen info from MetalRenderer...")

        // Get screen info from MetalRenderer
        let screenInfo = metalRenderer.getScreenInfo()

        // Push screen info to C++ side
        GameCorePlatform.GameCore.setScreenInfoDirect(screenInfo)

        log(
            "Screen info pushed to C++ successfully - "
                + "\(Int(screenInfo.pixelWidth))x\(Int(screenInfo.pixelHeight)) pixels, "
                + "\(screenInfo.logicalWidth)x\(screenInfo.logicalHeight) logical")
    }

    public func updateScreenInfo() {
        // Call this when orientation changes or screen properties change
        setupScreenInfo()
    }
}

// MARK: - C++ Integration Notes

/**
 * Swift 5.9+ Native C++ Interop Integration
 *
 * This GameEngine class now uses direct C++ class instantiation:
 *
 * // Swift to C++ (current implementation):
 * cppGame = GameCoreGame.GameCore.FloppyTurdGame()
 * cppGame?.Initialize(platform)
 * cppGame?.Run()
 *
 * // C++ to Swift (if needed):
 * #include "FloppyTurd-Swift.h"
 * auto gameEngine = std::make_unique<FloppyTurd::GameEngine>();
 *
 * The FloppyTurdGame C++ class is directly accessible from Swift
 * through the GameCoreGame module and Swift 5.9+ interop.
 */
