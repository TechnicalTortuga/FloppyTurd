//
//  GameViewController.swift
//  FloppyTurd
//
//  Main game view controller for iOS using Swift 5.9+ native C++ interop
//  Direct C++ instantiation: auto controller = std::make_unique<FloppyTurd::GameViewController>();
//  Handles Metal rendering, touch input, and game lifecycle
//

import UIKit
import Metal
import MetalKit

/// Main game view controller for iOS with native C++ interop
/// Direct C++ instantiation: std::make_unique<FloppyTurd::GameViewController>()
/// Manages the Metal view, game engine lifecycle, and user input
@MainActor
public class GameViewController: UIViewController {
    
    // MARK: - Properties
    
    private var metalView: MTKView!
    private var metalRenderer: MetalRenderer!
    private var touchInputHandler: TouchInputHandler!
    private var gameEngine: GameEngine!
    
    // MARK: - Logging Helper - Direct Swift/C++ interop
    private func log(_ message: String, level: LogLevel = .info) {
        Task {
            switch level {
            case .trace:
                await SwiftLog.debug(message, category: "GameViewController")
            case .debug:
                await SwiftLog.debug(message, category: "GameViewController")
            case .info:
                await SwiftLog.info(message, category: "GameViewController")
            case .warning:
                await SwiftLog.warn(message, category: "GameViewController")
            case .error:
                await SwiftLog.error(message, category: "GameViewController")
            case .fatal:
                await SwiftLog.fatal(message, category: "GameViewController")
            }
        }
    }
    
    private func shutdownGame() async {
        await gameEngine.shutdown()
        await log("Game shutdown complete")
    }
    
    // Game state
    private var isGameInitialized = false
    private var isPaused = false
    
    // MARK: - Lifecycle
    
    override public func viewDidLoad() {
        super.viewDidLoad()
        log("GameViewController loading...")
        
        setupMetalView()
        setupGameEngine()
        setupTouchInput()
        setupNotifications()
        
        log("GameViewController loaded successfully")
    }
    
    override public func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        log("GameViewController will appear")
        
        if isGameInitialized {
            if isPaused {
                resumeGame()
            } else if !gameEngine.isGameRunning() {
                // Start the game loop for the first time
                log("Starting game loop...")
                if gameEngine.start() {
                    log("Game loop started successfully")
                } else {
                    log("Failed to start game loop", level: .error)
                }
            }
        }
    }
    
    override public func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        log("GameViewController did disappear")
        
        shutdownGameSync()
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self)
        Task.detached { [weak self] in
            await self?.shutdownGame()
        }
    }
    
    // MARK: - Setup Methods
    
    private func setupMetalView() {
        log("Setting up Metal view...")
        
        guard let device = MTLCreateSystemDefaultDevice() else {
            log("Failed to create Metal device", level: .error)
            fatalError("Metal is not supported on this device")
        }
        
        metalView = MTKView(frame: view.bounds, device: device)
        metalView.delegate = self
        metalView.preferredFramesPerSecond = 60
        metalView.colorPixelFormat = .bgra8Unorm_srgb
        metalView.depthStencilPixelFormat = .depth32Float
        metalView.sampleCount = 1
        metalView.clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
        
        view.addSubview(metalView)
        metalView.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            metalView.topAnchor.constraint(equalTo: view.topAnchor),
            metalView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            metalView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            metalView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
        
        log("Metal view setup complete")
    }
    
    private func setupGameEngine() {
        log("Setting up game engine...")
        
        log("Initializing C++ game on main thread...")
        
        // Initialize the C++ game engine through Swift interop
        gameEngine = GameEngine()
        
        // Initialize Metal renderer
        metalRenderer = MetalRenderer()
        
        // Connect renderer to game engine
        gameEngine.setMetalRenderer(metalRenderer)
        
        // Initialize the game
        if gameEngine.initialize() {
            isGameInitialized = true
            log("Game engine initialized successfully on main thread")
        } else {
            log("Failed to initialize game engine", level: .error)
            fatalError("Game engine initialization failed")
        }
    }
    
    private func setupTouchInput() {
        log("Setting up touch input...")
        
        touchInputHandler = TouchInputHandler()
        touchInputHandler.delegate = self
        
        // Connect touch input handler to game engine
        gameEngine.setTouchInputHandler(touchInputHandler)
        
        // Add gesture recognizers
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleTap(_:)))
        metalView.addGestureRecognizer(tapGesture)
        
        let panGesture = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
        metalView.addGestureRecognizer(panGesture)
        
        log("Touch input setup complete")
    }
    
    private func setupNotifications() {
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(gameWillPause),
            name: .gameWillPause,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(gameDidResume),
            name: .gameDidResume,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(gameShouldSave),
            name: .gameShouldSave,
            object: nil
        )
    }
    
    // MARK: - Game Control
    
    private func pauseGame() {
        guard isGameInitialized && !isPaused else { return }
        
        log("Pausing game")
        gameEngine.pause()
        metalView.isPaused = true
        isPaused = true
    }
    
    private func resumeGame() {
        guard isGameInitialized && isPaused else { return }
        
        log("Resuming game")
        gameEngine.resume()
        metalView.isPaused = false
        isPaused = false
    }
    
    private func shutdownGameSync() {
        guard isGameInitialized else { return }
        
        log("Shutting down game")
        gameEngine.shutdown()
        isGameInitialized = false
    }
    
    // MARK: - Touch Handling
    
    @objc private func handleTap(_ gesture: UITapGestureRecognizer) {
        let location = gesture.location(in: metalView)
        let _ = CGPoint(
            x: location.x / metalView.bounds.width,
            y: location.y / metalView.bounds.height
        )
        touchInputHandler.handleTap(gesture)
    }
    
    @objc private func handlePan(_ gesture: UIPanGestureRecognizer) {
        // Handle pan gestures through touch events
        // This will be processed by the touch input handler's gesture recognizers
    }
    
    // MARK: - Notification Handlers
    
    @objc private func gameWillPause() {
        pauseGame()
    }
    
    @objc private func gameDidResume() {
        resumeGame()
    }
    
    @objc private func gameShouldSave() {
        guard isGameInitialized else { return }
        // TODO: Implement saveGameState method in GameEngine
        // gameEngine.saveGameState()
    }
}

// MARK: - MTKViewDelegate

extension GameViewController: MTKViewDelegate {
    
    public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        log("Metal view size changed to \(size)")
        metalRenderer?.updateViewportSize(size)
    }
    
    public func draw(in view: MTKView) {
        guard isGameInitialized && !isPaused else { return }
        
        // Calculate delta time (simplified for now)
        let deltaTime: Float = 1.0 / 60.0 // 60 FPS target
        
        // Update game logic
        gameEngine.update(deltaTime: deltaTime)
        
        // Render frame
        gameEngine.render()
    }
}

// MARK: - TouchInputHandlerDelegate

extension GameViewController: TouchInputHandlerDelegate {
    
    public func touchInputHandler(_ handler: TouchInputHandler, didReceiveInput input: Any) {
        // Forward touch input to game engine on main actor
        // Since GameViewController is @MainActor, this method runs on main thread
        // TODO: Define proper TouchInput type and implement handleTouchInput in GameEngine
        // gameEngine.handleTouchInput(input)
    }
}

// MARK: - C++ Integration Notes

/**
 * Swift 5.9+ Native C++ Interop Integration
 * 
 * With Swift 5.9+, C++ can directly instantiate this Swift class without C-style bridging:
 * 
 * // C++ Example:
 * #include "GameEngine-Swift.h"
 * 
 * // Direct instantiation
 * auto viewController = std::make_unique<FloppyTurd::GameViewController>();
 * 
 * // Integration with iOS app lifecycle
 * viewController->viewDidLoad();
 * viewController->viewWillAppear(true);
 * 
 * // Access Metal rendering and touch input
 * auto metalRenderer = viewController->getMetalRenderer();
 * auto touchHandler = viewController->getTouchInputHandler();
 * 
 * The Swift class automatically provides C++ compatible methods
 * through Swift's native C++ interoperability features.
 */