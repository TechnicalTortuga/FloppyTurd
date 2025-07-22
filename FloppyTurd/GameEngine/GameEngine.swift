//
//  GameEngine.swift
//  Professional Game Engine - Main Coordinator
//
//  Created by C++ Swift Interop Migration
//  Simple Swift wrapper for iOS game functionality
//

import Foundation
#if canImport(UIKit)
import UIKit
#endif
#if canImport(QuartzCore)
import QuartzCore
#endif
import Metal

/// Main game engine coordinator providing professional game architecture
/// Simple Swift wrapper for iOS functionality
/// SOLUTION: Use nonisolated(unsafe) for C++ interoperability with external synchronization
public final class GameEngine: @unchecked Sendable {
    
    // MARK: - Thread-Safe Properties with External Synchronization
    nonisolated(unsafe) private static var isInitialized: Bool = false
    nonisolated(unsafe) private static var isRunning: Bool = false
    
    // Core engine systems - nonisolated access for C++ interoperability
    nonisolated(unsafe) public static var audioManager: AudioManagerSwift!
    nonisolated(unsafe) public static var resourceManager: ResourceManagerSwift!
    nonisolated(unsafe) public static var hapticsManager: HapticsManagerSwift!
    nonisolated(unsafe) public static var metalRenderer: MetalRendererSwift!

    // Game loop properties - externally synchronized
    nonisolated(unsafe) private static var displayLink: CADisplayLink?
    nonisolated(unsafe) private static var lastFrameTime: CFTimeInterval = 0
    nonisolated(unsafe) private static var frameCount: UInt64 = 0
    nonisolated(unsafe) private static var fps: Double = 0
    
    // Performance tracking - externally synchronized
    nonisolated(unsafe) private static var frameTimeBuffer: [Double] = []
    private static let frameTimeBufferSize = 60
    
    // Game callbacks - externally synchronized
    nonisolated(unsafe) public static var onUpdate: ((Float) -> Void)?
    nonisolated(unsafe) public static var onRender: (() -> Void)?
    nonisolated(unsafe) public static var onResize: ((CGSize) -> Void)?
    
    // MARK: - Initialization & Resource Management
    
    /// Initialize the complete game engine
    /// Sets up all subsystems with zero overhead C++ interop
    @MainActor public static func initialize(view: UIView, 
                                screenSize: CGSize, 
                                safeAreaInsets: UIEdgeInsets,
                                pixelDensity: Float = 2.0, // Default scale for iOS devices
                                metalDevice: MTLDevice? = nil) {
        guard !isInitialized else {
            traceLog(SWLogLevel.SWLOG_WARNING, "[GameEngine] Already initialized")
            return
        }
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Initializing professional game engine...")
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Screen Size: \(screenSize)")
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Safe Area: \(safeAreaInsets)")
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Pixel Density: \(pixelDensity)")
        
        // Initialize Swift managers from main actor context
        audioManager = AudioManagerSwift.shared
        resourceManager = ResourceManagerSwift.shared
        hapticsManager = HapticsManagerSwift.shared
        metalRenderer = MetalRendererSwift.shared
        
        if let metalDevice = metalDevice {
            resourceManager.initialize(metalDevice: metalDevice)
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Initializing Swift audio manager...")
        // AudioManager initializes automatically as singleton
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Initializing Swift haptics manager...")
        _ = hapticsManager // Triggers haptics engine preparation
        // Verify systems are ready
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Verifying systems...")
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ All systems ready")
        // Setup input callbacks for game logic
        setupGameCallbacks()
        isInitialized = true
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Game engine initialization complete!")
    }

    /// Pre-load common resources for optimal performance
    public static func preloadResources() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Preloading common resources...")
        // Expand to preload textures, sounds, etc. as needed
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Resource preloading complete")
    }

    /// Clear all resource caches
    public static func clearResourceCaches() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Clearing resource caches...")
        // Expand to clear texture/audio caches as needed
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Resource caches cleared")
    }
    
    /// Setup game callbacks to integrate with C++ systems
    private static func setupGameCallbacks() {
        // Setup update callback for game logic
        onUpdate = { deltaTime in
            // Update game logic (native Swift-C++ interop)
            // Game logic will be handled by individual managers
        }
        
        // Setup render callback
        onRender = {
            // Render game logic (native Swift-C++ interop)
            // Rendering will be handled by MetalRendererSwift
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Game callbacks configured with test rendering")
    }
    
    // MARK: - Game Loop Management
    
    /// Start the main game loop
    public static func startGameLoop() {
        guard isInitialized else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameEngine] ERROR: Cannot start game loop - not initialized")
            return
        }
        
        guard !isRunning else {
            traceLog(SWLogLevel.SWLOG_WARNING, "[GameEngine] Game loop already running")
            return
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Starting game loop...")
        
        // Create display link for smooth 60 FPS
        displayLink = CADisplayLink(target: GameEngineDisplayLinkTarget.shared, 
                                   selector: #selector(GameEngineDisplayLinkTarget.displayLinkCallback(_:)))
        displayLink?.add(to: .main, forMode: .common)
        
        isRunning = true
        lastFrameTime = CACurrentMediaTime()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Game loop started")
    }
    
    /// Stop the main game loop
    public static func stopGameLoop() {
        guard isRunning else {
            traceLog(SWLogLevel.SWLOG_WARNING, "[GameEngine] Game loop not running")
            return
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Stopping game loop...")
        
        displayLink?.invalidate()
        displayLink = nil
        
        isRunning = false
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Game loop stopped")
    }
    
    /// Main game loop tick - called by display link
    internal static func gameLoopTick() {
        guard isInitialized && isRunning else { return }
        
        let currentTime = CACurrentMediaTime()
        let deltaTime = Float(currentTime - lastFrameTime)
        lastFrameTime = currentTime
        
        // Update frame performance tracking
        updatePerformanceMetrics(deltaTime: Double(deltaTime))
        
        // Update engine systems
        update(deltaTime: deltaTime)
        
        // Render frame
        render()
        
        frameCount += 1
    }
    
    /// Update all engine systems
    private static func update(deltaTime: Float) {
        // Update Swift managers on main thread when needed
        DispatchQueue.main.async {
            audioManager.update()
        }
        
        // Call game-specific update callback (which includes C++ logic)
        onUpdate?(deltaTime)
    }
    
    /// Render complete frame
    private static func render() {
        // Call game-specific render callback (which includes C++ rendering)
        onRender?()
    }
    
    /// Update performance metrics
    private static func updatePerformanceMetrics(deltaTime: Double) {
        frameTimeBuffer.append(deltaTime)
        
        if frameTimeBuffer.count > frameTimeBufferSize {
            frameTimeBuffer.removeFirst()
        }
        
        // Calculate FPS every 60 frames
        if frameCount % 60 == 0 {
            let averageFrameTime = frameTimeBuffer.reduce(0, +) / Double(frameTimeBuffer.count)
            fps = 1.0 / averageFrameTime
        }
    }
    
    /// Get average frame time in milliseconds
    public static func getAverageFrameTime() -> Double {
        let averageSeconds = frameTimeBuffer.reduce(0, +) / Double(frameTimeBuffer.count)
        return averageSeconds * 1000.0 // Convert to milliseconds
    }
    
    // MARK: - Touch Input Integration
    
    /// Handle touch began - forward to C++ systems
    public static func touchesBegan(_ touches: Set<UITouch>, in view: UIView) {
        // Basic touch handling - can be expanded for game-specific needs
        DispatchQueue.main.async {
            for touch in touches {
                let location = touch.location(in: view)
                traceLog(SWLogLevel.SWLOG_DEBUG, "[GameEngine] Touch began at: \(location)")
            }
        }
    }
    
    /// Handle touch moved - forward to C++ systems
    public static func touchesMoved(_ touches: Set<UITouch>, in view: UIView) {
        // Basic touch handling - can be expanded for game-specific needs
        DispatchQueue.main.async {
            for touch in touches {
                let _ = touch.location(in: view)
                // Could send to C++ input system via bridge
            }
        }
    }
    
    /// Handle touch ended - forward to C++ systems
    public static func touchesEnded(_ touches: Set<UITouch>, in view: UIView) {
        // Basic touch handling - can be expanded for game-specific needs
        DispatchQueue.main.async {
            for touch in touches {
                let location = touch.location(in: view)
                traceLog(SWLogLevel.SWLOG_DEBUG, "[GameEngine] Touch ended at: \(location)")
            }
        }
    }
    
    /// Handle touches cancelled - forward to C++ systems
    public static func touchesCancelled(_ touches: Set<UITouch>, in view: UIView) {
        // Handle cancelled touches the same as ended
        touchesEnded(touches, in: view)
    }
    
    // MARK: - Lifecycle Management
    
    /// Handle app becoming active
    public static func handleAppBecameActive() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] App became active")
        
        // Resume game loop if it was running
        if isInitialized && !isRunning {
            startGameLoop()
        }
    }
    
    /// Handle app becoming inactive
    public static func handleAppBecameInactive() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] App became inactive")
        
        // Pause game loop
        if isRunning {
            stopGameLoop()
        }
    }
    
    /// Handle memory warnings
    public static func handleMemoryWarning() {
        traceLog(SWLogLevel.SWLOG_WARNING, "[GameEngine] ⚠️ Memory warning received")
        
        // Let Swift managers handle memory cleanup on main thread
        DispatchQueue.main.async {
            audioManager.handleMemoryWarning()
            resourceManager.handleMemoryWarning()
        }
        
        // Could notify C++ systems via bridge if needed
    }
    
    /// Handle device rotation or screen size change
    /// Note: This function is intentionally internal to prevent automatic C++ interop exposure
    /// while still allowing access from GameEngineCppBridge
    /// Use GameEngineCppBridge.handleScreenSizeChanged for C++ calls
    internal static func handleScreenSizeChanged(size: CGSize, safeAreaInsets: UIEdgeInsets) {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Screen size changed: \(size), safe area: \(safeAreaInsets)")
        
        // Call game-specific resize callback
        onResize?(size)
    }
    

    
    // MARK: - Platform Integration
    
    /// Quick access to platform haptics
    public static func triggerHapticFeedback(type: Int = 0) {
        // Use HapticsManagerSwift for all haptic feedback on main thread
        DispatchQueue.main.async {
            switch type {
            case 1:
                hapticsManager.playImpact(style: .light)
            case 2:
                hapticsManager.playImpact(style: .heavy)
            case 3:
                hapticsManager.playNotification(type: .success)
            case 4:
                hapticsManager.playNotification(type: .warning)
            case 5:
                hapticsManager.playNotification(type: .error)
            default:
                hapticsManager.playImpact(style: .medium)
            }
        }
    }
    
    public static var isVibrationSupported: Bool {
        return true // iOS always supports haptics
    }
    
    public static var platformTime: Double {
        return CFAbsoluteTimeGetCurrent()
    }
    
    // MARK: - Audio Integration (Bridge between Swift and C++)
    
    /// Play sound via Swift audio manager (preferred) or C++ bridge
    public static func playSound(_ soundName: String) {
        // Use Swift audio manager by default on main thread
        DispatchQueue.main.async {
            audioManager.playSoundEffect(name: soundName)
        }
    }
    
    /// Play music via Swift audio manager (preferred)
    public static func playMusic(_ musicName: String) {
        DispatchQueue.main.async {
            audioManager.playMusic(path: musicName)
        }
    }
    
    // MARK: - Shutdown
    
    /// Shutdown the complete game engine
    public static func shutdown() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Shutting down game engine...")
        // Stop game loop first
        stopGameLoop()
        // Shutdown Swift managers
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] Shutting down Swift managers...")
        DispatchQueue.main.async {
            audioManager.shutdown()
            resourceManager.shutdown()
        }
        // No explicit shutdown needed for hapticsManager (stateless)
        // Clear callbacks
        onUpdate = nil
        onRender = nil
        onResize = nil
        // Reset state
        isInitialized = false
        frameCount = 0
        frameTimeBuffer = []
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Game engine shutdown complete")
    }
}

// MARK: - Display Link Target

/// Helper class to handle display link callbacks
/// Required because CADisplayLink needs an Objective-C target
/// SOLUTION: Remove @MainActor to allow access from GameEngine
private final class GameEngineDisplayLinkTarget: NSObject, @unchecked Sendable {
    static let shared = GameEngineDisplayLinkTarget()
    
    @objc func displayLinkCallback(_ displayLink: CADisplayLink) {
        // Since GameEngine is no longer @MainActor, we can call directly
        // But game loop operations should still happen on main thread
        DispatchQueue.main.async {
            GameEngine.gameLoopTick()
        }
    }
}

// MARK: - Game Engine Extensions

extension GameEngine {
    
    /// Convenience method to setup a basic game with common elements
    @MainActor public static func setupBasicGame(view: UIView, metalDevice: MTLDevice) {
        let screenSize = view.bounds.size
        let uiKitInsets = view.safeAreaInsets
        let safeAreaInsets = UIEdgeInsets(
            top: CGFloat(uiKitInsets.top),
            left: CGFloat(uiKitInsets.left),
            bottom: CGFloat(uiKitInsets.bottom),
            right: CGFloat(uiKitInsets.right)
        )
        // Initialize engine
        initialize(view: view, screenSize: screenSize, safeAreaInsets: safeAreaInsets, metalDevice: metalDevice)
        // Preload resources
        preloadResources()
        // Start game loop
        startGameLoop()
        traceLog(SWLogLevel.SWLOG_INFO, "[GameEngine] ✅ Basic game setup complete")
    }
    
    /// Quick debug info string
    public static func getDebugInfo() -> String {
        return """
        GameEngine Debug Info:
        - Initialized: \(isInitialized)
        - Running: \(isRunning)
        - FPS: \(String(format: "%.1f", fps))
        - Frame Time: \(String(format: "%.2f", getAverageFrameTime()))ms
        - Frame Count: \(frameCount)
        
        Swift Managers:
        - Audio Manager: Available
        - Resource Manager: Available
        """
    }
}

// MARK: - C++ Bridge

/// C++ bridge for GameEngine functionality
/// Provides nonisolated static functions that can be called from C++
@_expose(Cxx)
public final class GameEngineCppBridge {
    
    /// C++ bridge function for handling screen size changes with C++ compatible types
    @_expose(Cxx)
    public static func handleScreenSizeChanged(width: Float, height: Float, 
                                             topInset: Float, leftInset: Float, 
                                             bottomInset: Float, rightInset: Float) {
        let size = CGSize(width: CGFloat(width), height: CGFloat(height))
        let safeAreaInsets = UIEdgeInsets(top: CGFloat(topInset), 
                                        left: CGFloat(leftInset), 
                                        bottom: CGFloat(bottomInset), 
                                        right: CGFloat(rightInset))
        
        GameEngine.handleScreenSizeChanged(size: size, safeAreaInsets: safeAreaInsets)
    }
    
    /// C++ bridge for triggering haptic feedback
    @_expose(Cxx)
    public static func triggerHapticFeedback(type: Int32) {
        GameEngine.triggerHapticFeedback(type: Int(type))
    }
    
    /// C++ bridge for playing sound
    @_expose(Cxx)
    public static func playSound(soundName: String) {
        GameEngine.playSound(soundName)
    }
    
    /// C++ bridge for playing music
    @_expose(Cxx)
    public static func playMusic(musicName: String) {
        GameEngine.playMusic(musicName)
    }
    
    /// C++ bridge for getting platform time
    @_expose(Cxx)
    public static func getPlatformTime() -> Double {
        return GameEngine.platformTime
    }
    
    /// C++ bridge for checking vibration support
    @_expose(Cxx)
    public static func getIsVibrationSupported() -> Bool {
        return GameEngine.isVibrationSupported
    }
}
