//
//  GameViewSwift.swift
//  Professional Game Engine - Native Swift Game View
//
//  Created by C++ Swift Interop Migration
//  Direct Swift replacement for GameView.mm with modern architecture
//

#if canImport(UIKit)
import UIKit
#endif
import Metal
import MetalKit
import simd

// MARK: - Type Aliases
// Removed internal typealias Color; now use C++ types directly via GameEngineCpp interop
typealias GameState = GameViewSwift.GameState

/// Professional Swift game view with native Metal integration
/// Direct replacement for GameView.mm with zero Objective-C++ dependencies
@MainActor
public class GameViewSwift: MTKView {
    
    // MARK: - Core Engine Components
    private var gameEngine: GameEngine?
    private var metalRenderer: MetalRendererSwift?
    private var textRenderer: MetalTextRendererSwift?
    private var isInitialized: Bool = false
    
    // MARK: - Touch Handling
    private var activeTouches: [UITouch: Int] = [:]
    private var touchIDCounter: Int = 0
    private var lastTouchLocation: CGPoint = .zero
    private var isTouching: Bool = false
    private var primaryTouch: UITouch?
    
    // MARK: - Performance Monitoring
    private var frameStartTime: CFTimeInterval = 0
    private var lastFrameTime: CFTimeInterval = 0
    private var frameCount: Int = 0
    private var fpsCounter: Float = 0.0
    
    // MARK: - Game State
    private var gameState: GameState = .initializing
    private var gamePaused: Bool = false
    private var backgroundTime: CFTimeInterval = 0
    
    public enum GameState {
        case initializing
        case running
        case paused
        case stopped
        case error
    }
    
    // MARK: - Initialization
    
    /// Initialize with frame and Metal device
    public init(frame: CGRect, metalDevice: MTLDevice) {
        super.init(frame: frame, device: metalDevice)
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Initializing with frame: \(frame)")
        setupView()
    }
    
    required init(coder: NSCoder) {
        super.init(coder: coder)
        setupView()
    }
    
    deinit {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Deallocating game view")
        Task { [weak self] in
            guard let self = self else { return }
            await self.shutdown()
        }
    }
    
    /// Setup the view and initialize all game systems
    private func setupView() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Setting up view...")
        
        guard let metalDevice = device else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewSwift] ERROR: No Metal device available")
            gameState = .error
            return
        }
        
        // Configure MTKView properties
        colorPixelFormat = .bgra8Unorm
        depthStencilPixelFormat = .depth32Float
        clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0)
        
        // Enable multi-touch
        isMultipleTouchEnabled = true
        isUserInteractionEnabled = true
        
        // Set up the delegate and display link
        delegate = self
        enableSetNeedsDisplay = false
        gamePaused = false
        
        // Initialize core engine systems
        if !initializeEngineComponents(device: metalDevice) {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewSwift] ERROR: Failed to initialize engine components")
            gameState = .error
            return
        }
        
        // Setup complete
        isInitialized = true
        gameState = .running
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] ✅ View setup complete, starting game loop")
    }
    
    /// Initialize all engine components
    private func initializeEngineComponents(device: MTLDevice) -> Bool {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Initializing engine components...")
        
        // Create command queue
        guard let commandQueue = device.makeCommandQueue() else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewSwift] ERROR: Failed to create command queue")
            return false
        }
        _ = commandQueue
        
        // Initialize Metal renderer
        metalRenderer = MetalRendererSwift()
        guard let renderer = metalRenderer,
              renderer.initialize(view: self) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewSwift] ERROR: Failed to initialize Metal renderer")
            return false
        }
        
        // Initialize text renderer
        textRenderer = MetalTextRendererSwift()
        guard let textRenderer = textRenderer,
              textRenderer.initialize(device: device, view: self) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewSwift] ERROR: Failed to initialize text renderer")
            return false
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] ✅ C++ game engine initialized successfully")
        
        // Initialize main game engine via static methods
        GameEngine.initialize(view: self, 
                            screenSize: bounds.size, 
                            safeAreaInsets: safeAreaInsets,
                            metalDevice: device)
        
        // Create a simple game engine wrapper for instance access
        gameEngine = GameEngine()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] ✅ All engine components initialized")
        return true
    }
    
    // MARK: - Game Loop Integration
    
    /// Start the game loop
    public func startGameLoop() {
        guard isInitialized else {
            traceLog(SWLogLevel.SWLOG_WARNING, "[GameViewSwift] WARNING: Cannot start game loop - not initialized")
            return
        }
        
        gamePaused = false
        gameState = .running
        frameStartTime = CACurrentMediaTime()
        
        // Start the global game engine loop
        GameEngine.startGameLoop()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] ✅ Game loop started")
    }
    
    /// Pause the game loop
    public func pauseGameLoop() {
        gamePaused = true
        gameState = .paused
        backgroundTime = CACurrentMediaTime()
        
        // Pause Metal rendering
        self.gamePaused = true
        
        // Pause the global game engine loop
        GameEngine.stopGameLoop()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Game loop paused")
    }
    
    /// Resume the game loop
    public func resumeGameLoop() {
        guard gameState == .paused else { return }
        
        // Calculate time spent in background
        let pauseDuration = CACurrentMediaTime() - backgroundTime
        frameStartTime += pauseDuration
        
        gamePaused = false
        gameState = .running
        self.gamePaused = false
        
        // Resume the global game engine loop
        GameEngine.startGameLoop()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Game loop resumed after \(pauseDuration)s")
    }
    
    /// Stop the game loop
    public func stopGameLoop() {
        gamePaused = true
        gameState = .stopped
        self.gamePaused = true
        
        // Stop the global game engine loop
        GameEngine.stopGameLoop()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Game loop stopped")
    }
    
    // MARK: - Touch Handling (Modern Swift approach)
    
    public override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let touchID = assignTouchID(for: touch)
            let location = touch.location(in: self)
            
            // Convert to game coordinates
            let gameLocation = convertToGameCoordinates(point: location)
            
            // Track primary touch
            if primaryTouch == nil {
                primaryTouch = touch
                isTouching = true
                lastTouchLocation = location
            }
            
            // Send to game engine
            GameEngine.touchesBegan(Set([touch]), in: self)
            
            traceLog(SWLogLevel.SWLOG_DEBUG, "[GameViewSwift] Touch began: ID=\(touchID), location=\(gameLocation)")
        }
    }
    
    public override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            guard let touchID = activeTouches[touch] else { continue }
            
            let location = touch.location(in: self)
            let gameLocation = convertToGameCoordinates(point: location)
            
            // Update primary touch
            if touch == primaryTouch {
                lastTouchLocation = location
            }
            
            // Send to game engine
            GameEngine.touchesMoved(Set([touch]), in: self)
        }
    }
    
    public override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            guard let touchID = activeTouches[touch] else { continue }
            
            let location = touch.location(in: self)
            let gameLocation = convertToGameCoordinates(point: location)
            
            // Handle primary touch end
            if touch == primaryTouch {
                primaryTouch = nil
                isTouching = false
            }
            
            // Send to game engine
            GameEngine.touchesEnded(Set([touch]), in: self)
            
            // Remove from tracking
            activeTouches.removeValue(forKey: touch)
            
            traceLog(SWLogLevel.SWLOG_DEBUG, "[GameViewSwift] Touch ended: ID=\(touchID), location=\(gameLocation)")
        }
    }
    
    public override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        // Handle cancelled touches the same as ended
        touchesEnded(touches, with: event)
    }
    
    // MARK: - Touch Utilities
    
    private func assignTouchID(for touch: UITouch) -> Int {
        if let existingID = activeTouches[touch] {
            return existingID
        }
        
        let newID = touchIDCounter
        touchIDCounter += 1
        activeTouches[touch] = newID
        return newID
    }
    
    private func convertToGameCoordinates(point: CGPoint) -> CGPoint {
        // Convert from view coordinates to game world coordinates
        // This depends on your coordinate system setup
        let bounds = self.bounds
        
        // For now, simple conversion (you can enhance this based on your coordinate system)
        return CGPoint(
            x: point.x,
            y: bounds.height - point.y  // Flip Y coordinate if needed
        )
    }
    
    // MARK: - Lifecycle Management
    
    /// Handle app entering background
    public func applicationDidEnterBackground() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Application entering background")
        pauseGameLoop()
        
        // Notify global game engine
        GameEngine.handleAppBecameInactive()
    }
    
    /// Handle app entering foreground
    public func applicationWillEnterForeground() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Application entering foreground")
        resumeGameLoop()
        
        // Notify global game engine
        GameEngine.handleAppBecameActive()
    }
    
    /// Shutdown all systems
    @MainActor
    private func shutdown() async {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Shutting down game systems...")
        
        stopGameLoop()
        
        // Shutdown global game engine
        GameEngine.shutdown()
        
        // Shutdown local components
        textRenderer?.shutdown()
        metalRenderer?.shutdown()
        
        // Clear references
        gameEngine = nil
        textRenderer = nil
        metalRenderer = nil
        
        // Clear touch tracking
        activeTouches.removeAll()
        primaryTouch = nil
        
        gameState = .stopped
        isInitialized = false
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] ✅ Shutdown complete")
    }
    
    // MARK: - Public API
    
    /// Get current game state
    public var currentGameState: GameState {
        return gameState
    }
    
    /// Get current FPS
    public var currentFPS: Float {
        return fpsCounter
    }
    
    /// Check if game is running
    public var isGameRunning: Bool {
        return gameState == .running && !gamePaused
    }
    
    /// Get the Metal device
    public var metalDevice: MTLDevice? {
        return device
    }
    
    /// Access to game engine (for external control)
    public var engine: GameEngine? {
        return gameEngine
    }
}

// MARK: - MTKViewDelegate Implementation

extension GameViewSwift: MTKViewDelegate {
    
    public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewSwift] Drawable size changed: \(size)")
        
        // Update renderer viewport
        metalRenderer?.updateViewport(size: size)
        
        // Notify global game engine of size change via C++ bridge
        GameEngineCppBridge.handleScreenSizeChanged(
            width: Float(size.width),
            height: Float(size.height),
            topInset: Float(safeAreaInsets.top),
            leftInset: Float(safeAreaInsets.left),
            bottomInset: Float(safeAreaInsets.bottom),
            rightInset: Float(safeAreaInsets.right)
        )
    }
    
    public func draw(in view: MTKView) {
        // Performance monitoring
        let currentTime = CACurrentMediaTime()
        let deltaTime = Float(currentTime - lastFrameTime)
        lastFrameTime = currentTime
        
        // Update FPS counter
        frameCount += 1
        if frameCount % 60 == 0 {
            let totalTime = currentTime - frameStartTime
            fpsCounter = Float(frameCount) / Float(totalTime)
        }
        
        // Skip frame if paused or not initialized
        guard isGameRunning && isInitialized else { return }
        
        // Begin frame
        metalRenderer?.beginDrawing()
        
        // Clear background
        metalRenderer?.clearBackground(RaylibColor.black)
        
        // The actual game update and render is handled by GameEngine's display link
        // This MTK draw method just ensures Metal rendering stays in sync
        
        // Flush text rendering
        if let commandEncoder = metalRenderer?.getCurrentEncoder() {
            textRenderer?.flushTextBatch(encoder: commandEncoder)
        }
        
        // End frame
        metalRenderer?.endDrawing()
    }
}
