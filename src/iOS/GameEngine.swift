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
import FloppyTurdEngine
import FloppyTurdGame

/// GameEngine - Swift implementation with native C++ interop
/// Direct C++ instantiation: std::make_unique<FloppyTurd::GameEngine>()
/// Manages game lifecycle and coordinates between iOS and C++ systems
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
        shutdown()
        log("GameEngine Swift bridge destroyed")
    }
    
    // MARK: - Lifecycle Management
    
    /// Initialize the game engine and all subsystems
    public func initialize() -> Bool {
        guard !isInitialized else {
            log("GameEngine already initialized", level: .warning)
            return true
        }
        
        log("Initializing GameEngine...")
        
        // Create and initialize C++ game instance
        cppGame = FloppyTurd.FloppyTurdGame()
        
        // TODO: Initialize with platform interface
        // guard cppGame?.Initialize(platform) == true else {
        //     log("Failed to initialize C++ game", level: .error)
        //     cppGame = nil
        //     return false
        // }
        
        isInitialized = true
        log("GameEngine initialized successfully")
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
        
        isInitialized = false
        log("GameEngine shutdown complete")
    }
    
    /// Start the game loop
    public func start() -> Bool {
        guard isInitialized && !isRunning else {
            log("Cannot start game - not initialized or already running", level: .warning)
            return false
        }
        
        log("Starting game loop...")
        
        guard cppGame != nil else {
            log("C++ game instance is null", level: .error)
            return false
        }

        // Start the C++ game
        cppGame?.Run()
        isRunning = true
        isPaused = false
        log("Game loop started successfully")
        return true
    }
    
    /// Stop the game loop
    public func stop() {
        guard isRunning else { return }
        
        log("Stopping game loop...")
        
        // C++ game loop will be stopped when the game is paused or shutdown
        // The Run() method handles the game loop internally
        
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
        
        cppGame?.Render()
    }
    
    // MARK: - Game Access
    
    // Note: Direct access to cppGame property is available
    // No getter function needed - use gameEngine.cppGame directly
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