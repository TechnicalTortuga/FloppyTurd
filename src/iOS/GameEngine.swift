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

import GameCoreEngine
import GameCorePlatform
import GameCoreGame

// import FloppyTurdGame (C++ class is available via module.modulemap and C++ interop)

/// GameEngine - Swift implementation with native C++ interop
/// Direct C++ instantiation: std::make_unique<FloppyTurd::GameEngine>()
/// Manages game lifecycle and coordinates between iOS and C++ systems
/// @MainActor ensures all GameEngine operations happen on the main thread
/// This is required for Swift 6 concurrency safety with UI-related operations
@MainActor
public class GameEngine: NSObject {
    
    // MARK: - Properties
    
    private var isInitialized = false
    private var isRunning = false
    private var isPaused = false
    private var lastFrameTime: CFTimeInterval = 0
    
    // Platform-specific managers
    private var metalRenderer: MetalRenderer?
    private var touchInputHandler: TouchInputHandler?
    private var inputHandler: TouchInputHandler?
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
        
        // Create platform components first
        if metalRenderer == nil {
            metalRenderer = MetalRenderer()
            log("Created MetalRenderer")
        }
        
        if touchInputHandler == nil {
            touchInputHandler = TouchInputHandler()
            log("Created TouchInputHandler")
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
        
        // Create Swift objects
        log("Creating Swift component objects...")
        let inputHandler = TouchInputHandler()
        
        // Store references
        self.inputHandler = inputHandler
        // audioManager already set - don't overwrite it
        
        log("Swift components created - ready to initialize C++ game")
        
        // Now initialize the C++ game
        let success = cppGame?.Initialize() ?? false
        
        log("C++ game Initialize() returned: \(success)")
        
        guard success else {
            log("Failed to initialize C++ game with Swift components", level: .error)
            cppGame = nil
            return false
        }
        
        log("C++ game initialized successfully - starting game and showing main menu")
        
        // Show main menu and start background music
        cppGame?.ShowMainMenu()
        log("ShowMainMenu() called")
        
        cppGame?.StartGame()
        log("StartGame() called")
        
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
        log("Game started - now driven by MTKView draw loop")
        
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
    
    // MARK: - Game Loop Integration
    
    /// Update the game (called from Metal render loop)
    public func update(deltaTime: Float) {
        guard isRunning && !isPaused else { return }
        
        cppGame?.Update(deltaTime)
    }
    
    /// Render the game (called from Metal render loop)
    public func render() {
        guard isRunning && !isPaused else { return }
        
        // Process any queued C++ render commands synchronously
        commandProcessor?.processCommands()
        
        // For now, directly call MetalRenderer to test blue screen rendering
        // TODO: Connect MetalRenderer to C++ game properly
        if let renderer = metalRenderer {
            renderer.beginFrame()
            renderer.setClearColor(r: 0.0, g: 0.5, b: 1.0, a: 1.0) // Blue background
            renderer.clear()
            
            // PRIMITIVE SHAPES TEST: Show off our new rendering capabilities
            
            // 1. Large background rectangle (semi-transparent blue)
            renderer.drawRectangle(x: 50, y: 50, width: 1079, height: 2456, r: 0.0, g: 0.3, b: 0.8, a: 0.3)
            
            // 2. Solid rectangle (red)
            renderer.drawRectangle(x: 100, y: 100, width: 300, height: 200, r: 1.0, g: 0.0, b: 0.0, a: 1.0)
            
            // 3. Circle (green)
            renderer.drawCircle(x: 600, y: 200, radius: 100, r: 0.0, g: 1.0, b: 0.0, a: 1.0)
            
            // 4. Triangle (blue)
            renderer.drawTriangle(x1: 200, y1: 400, x2: 350, y2: 400, x3: 275, y3: 300, r: 0.0, g: 0.0, b: 1.0, a: 1.0)
            
            // 5. Line (white)
            renderer.drawLine(x1: 100, y1: 600, x2: 700, y2: 650, width: 8, r: 1.0, g: 1.0, b: 1.0, a: 1.0)
            
            // 6. Rounded rectangle (yellow) - perfect for buttons!
            renderer.drawRoundedRectangle(x: 400, y: 800, width: 400, height: 100, cornerRadius: 25, r: 1.0, g: 1.0, b: 0.0, a: 1.0)
            
            // 7. Small rounded rectangle (purple) - another button
            renderer.drawRoundedRectangle(x: 200, y: 1000, width: 200, height: 80, cornerRadius: 40, r: 0.8, g: 0.0, b: 0.8, a: 1.0)
            
            // 8. SDF TEXT RENDERING TEST
            // Initialize font system (using placeholder for now)
            if renderer.loadFont(fontName: "Whacky_Joe", fontSize: 32) {
                // Test text rendering with much larger sizes for better diagnosis
                renderer.drawText("FLOPPY TURD!", x: 100, y: 500, fontSize: 120, r: 1.0, g: 1.0, b: 1.0, a: 1.0)
                renderer.drawText("High Quality SDF Text", x: 100, y: 700, fontSize: 80, r: 1.0, g: 0.8, b: 0.0, a: 1.0)
                renderer.drawText("Ready for UI buttons!", x: 100, y: 900, fontSize: 60, r: 0.0, g: 1.0, b: 0.8, a: 1.0)
            } else {
                // Fallback - draw text placeholders using rectangles
                renderer.drawRectangle(x: 100, y: 1200, width: 400, height: 40, r: 0.5, g: 0.5, b: 0.5, a: 0.8) // Text placeholder
                renderer.drawRectangle(x: 100, y: 1280, width: 350, height: 30, r: 0.4, g: 0.4, b: 0.4, a: 0.8) // Text placeholder
                renderer.drawRectangle(x: 100, y: 1340, width: 300, height: 25, r: 0.3, g: 0.3, b: 0.3, a: 0.8) // Text placeholder
            }
            
            renderer.endFrame()
            renderer.present()
        }
        
        // Call C++ game render (currently mostly commented out)
        cppGame?.Render()
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