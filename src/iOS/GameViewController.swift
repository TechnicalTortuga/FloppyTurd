//
//  GameViewController.swift
//  FloppyTurd
//
//  Main game view controller for iOS using Swift 5.9+ native C++ interop
//  Direct C++ instantiation: auto controller = std::make_unique<FloppyTurd::GameViewController>();
//  Handles Metal rendering, touch input, and game lifecycle
//

import Metal
import MetalKit
import UIKit

/// Main game view controller for iOS with native C++ interop
/// Direct C++ instantiation: std::make_unique<FloppyTurd::GameViewController>()
/// Manages the Metal view, game engine lifecycle, and user input
@MainActor
public class GameViewController: UIViewController {

    // MARK: - Properties

    private var metalView: MTKView!
    private var metalRenderer: MetalRenderer!
    private var touchInputHandler: TouchInputHandler?
    public var gameEngine: GameEngine!

    // Orientation control
    private var orientationLocked = false
    private var lockedOrientation: UIInterfaceOrientationMask = .all

    // MARK: - Logging Helper - Direct Swift/C++ interop
    private func log(_ message: String, level: LogLevel = .info) {
        switch level {
        case .trace:
            SwiftLog.debug(message, category: "GameViewController")
        case .debug:
            SwiftLog.debug(message, category: "GameViewController")
        case .info:
            SwiftLog.info(message, category: "GameViewController")
        case .warning:
            SwiftLog.warn(message, category: "GameViewController")
        case .error:
            SwiftLog.error(message, category: "GameViewController")
        case .fatal:
            SwiftLog.fatal(message, category: "GameViewController")
        }
    }

    private func shutdownGame() {
        gameEngine.shutdown()
        log("Game shutdown complete")
    }

    // Game state
    private var isGameInitialized = false
    private var isPaused = false

    // MARK: - Lifecycle

    override public func viewDidLoad() {
        super.viewDidLoad()
        log("GameViewController loading...")

        log("About to call setupMetalView()...")
        setupMetalView()
        log("setupMetalView() completed")

        log("About to call setupGameEngine()...")
        setupGameEngine()
        log("setupGameEngine() completed successfully")

        log("About to call setupTouchInput()...")
        setupTouchInput()
        log("setupTouchInput() completed")

        log("About to call setupNotifications()...")
        setupNotifications()
        log("setupNotifications() completed")

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
            metalView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
        ])

        log("Metal view setup complete")
    }

    private func setupGameEngine() {
        log("Setting up game engine...")

        log("Initializing C++ game on main thread...")

        // Initialize the C++ game engine through Swift interop
        log("About to create GameEngine() object...")
        gameEngine = GameEngine()
        log("GameEngine() object created successfully")

        // Initialize Metal renderer
        log("About to create MetalRenderer...")
        metalRenderer = MetalRenderer()
        log("MetalRenderer created successfully")

        // Connect Metal view to renderer - THIS WAS MISSING!
        log("About to connect MTKView to MetalRenderer...")
        metalRenderer.setMetalView(metalView)
        log("Connected MTKView to MetalRenderer")

        // Connect renderer to game engine
        log("About to connect renderer to game engine...")
        gameEngine.setMetalRenderer(metalRenderer)
        log("Connected renderer to game engine")

        // Connect game view controller to game engine for orientation commands
        log("About to connect game view controller to game engine...")
        gameEngine.setGameViewController(self)
        log("Connected game view controller to game engine")

        // Initialize the game
        log("About to initialize game engine...")
        if gameEngine.initialize() {
            isGameInitialized = true
            log("Game engine initialized successfully on main thread")
        } else {
            log("Failed to initialize game engine")
            fatalError("Game engine initialization failed")
        }
        log("setupGameEngine completed successfully")
    }

    private func setupTouchInput() {
        log("Setting up touch input...")

        // Use GameEngine's TouchInputHandler (GameEngine creates it internally and sets itself as delegate)
        // GameEngine.initialize() must be called before this to ensure TouchInputHandler exists
        touchInputHandler = gameEngine.getTouchInputHandler()
        if let handler = touchInputHandler {
            // Ensure the handler is initialized with the MTKView so it knows the correct view size
            // and installs its own gesture recognizers for continuous move tracking
            let _ = handler.initialize(with: metalView)
            log(
                "TouchInputHandler.initialize(with:) called to set view size and gestures",
                level: .debug)
        } else {
            log("TouchInputHandler is nil in setupTouchInput()", level: .warning)
        }

        // Add only a tap recognizer here; TouchInputHandler installs its own pan for smooth tracking
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleTap(_:)))
        tapGesture.cancelsTouchesInView = false
        metalView.addGestureRecognizer(tapGesture)

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
        log(
            "🎯 GameViewController: Tap detected at (\(location.x), \(location.y)) - forwarding to TouchInputHandler",
            level: .debug)
        touchInputHandler?.handleTap(gesture)
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
        log("Metal view size changed to \(size) - POTENTIAL ORIENTATION CHANGE", level: .info)
        metalRenderer?.updateViewportSize(width: Float(size.width), height: Float(size.height))

        // CRITICAL FIX: Update ConfigManager with fresh screen info when orientation changes
        // This ensures ScreenPromptState gets the updated screen information immediately
        if let renderer = metalRenderer {
            let screenInfo = renderer.getScreenInfo()
            log(
                "Orientation change detected: \(screenInfo.pixelWidth)x\(screenInfo.pixelHeight), portrait: \(screenInfo.isPortrait)",
                level: .info)

            // Update ConfigManager through the C++ game engine
            gameEngine?.cppGame?.UpdateScreenInfo(screenInfo)
            log(
                "ConfigManager updated via C++ game engine - ScreenPromptState should reposition UI now",
                level: .info)
        }

        log("Viewport size updated", level: .debug)
    }

    // MARK: - Orientation Control

    /// Lock orientation to prevent rotation changes
    /// - Parameter orientation: The orientation to lock to (nil means current orientation)
    public func lockOrientation(to orientation: UIInterfaceOrientationMask? = nil) {
        orientationLocked = true
        if let orientation = orientation {
            lockedOrientation = orientation
        } else {
            // Lock to current orientation
            lockedOrientation = supportedInterfaceOrientations
        }
        log("Orientation locked to: \(lockedOrientation)", level: .info)
    }

    /// Unlock orientation to allow rotation changes
    public func unlockOrientation() {
        orientationLocked = false
        lockedOrientation = .all
        log("Orientation unlocked", level: .info)

        // Notify system of orientation support changes
        setNeedsUpdateOfSupportedInterfaceOrientations()
        navigationController?.setNeedsUpdateOfSupportedInterfaceOrientations()
    }

    /// Lock orientation to portrait only
    public func lockToPortrait() {
        orientationLocked = true
        lockedOrientation = [.portrait, .portraitUpsideDown]
        log("Orientation locked to portrait only", level: .info)

        // Modern iOS orientation handling with proper API calls
        setNeedsUpdateOfSupportedInterfaceOrientations()
        navigationController?.setNeedsUpdateOfSupportedInterfaceOrientations()

        // Force orientation change if needed
        if let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene {
            if windowScene.interfaceOrientation.isLandscape {
                // Request portrait orientation with proper error handling
                Task { @MainActor in
                    if let windowScene = UIApplication.shared.connectedScenes.first
                        as? UIWindowScene
                    {
                        windowScene.requestGeometryUpdate(.iOS(interfaceOrientations: .portrait)) {
                            error in
                            self.log(
                                "Orientation lock to portrait failed: \(error.localizedDescription)",
                                level: .error)
                        }
                        self.log(
                            "Successfully requested orientation lock to portrait", level: .info)
                    }
                }
            }
        }
    }

    /// Lock orientation to landscape only
    public func lockToLandscape() {
        orientationLocked = true
        lockedOrientation = [.landscapeLeft, .landscapeRight]
        log("Orientation locked to landscape only", level: .info)

        // Modern iOS orientation handling with proper API calls
        setNeedsUpdateOfSupportedInterfaceOrientations()
        navigationController?.setNeedsUpdateOfSupportedInterfaceOrientations()

        // Force orientation change if needed
        if let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene {
            if windowScene.interfaceOrientation.isPortrait {
                // Request landscape orientation with proper error handling
                Task { @MainActor in
                    if let windowScene = UIApplication.shared.connectedScenes.first
                        as? UIWindowScene
                    {
                        windowScene.requestGeometryUpdate(
                            .iOS(interfaceOrientations: .landscapeLeft)
                        ) { error in
                            self.log(
                                "Orientation lock to landscape failed: \(error.localizedDescription)",
                                level: .error)
                        }
                        self.log(
                            "Successfully requested orientation lock to landscape", level: .info)
                    }
                }
            }
        }
    }

    override public var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        if orientationLocked {
            return lockedOrientation
        }
        return .all  // Allow all orientations by default
    }

    override public var shouldAutorotate: Bool {
        return !orientationLocked
    }

    // MARK: - Orientation Handling

    override public func viewWillTransition(
        to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator
    ) {
        super.viewWillTransition(to: size, with: coordinator)

        log("View will transition to size: \(size)")

        // Update screen info when view transitions (orientation change)
        coordinator.animate(alongsideTransition: { _ in
            // This is called during the animation
        }) { _ in
            // This is called after the transition completes
            DispatchQueue.main.async {
                // Force MetalRenderer to update screen info after orientation change
                if let renderer = self.metalRenderer {
                    let screenInfo = renderer.getScreenInfo()
                    self.log(
                        "Orientation changed - New screen info: \(screenInfo.pixelWidth)x\(screenInfo.pixelHeight), portrait: \(screenInfo.isPortrait)",
                        level: .info)

                    // ConfigManager is automatically updated when MetalRenderer updates its screen info
                }
                self.log("View transition completed", level: .debug)
            }
        }
    }

    public func draw(in view: MTKView) {
        guard isGameInitialized && !isPaused else { return }

        // DEBUG: Log that draw is being called
        log("MTKView draw() called - rendering frame", level: .debug)

        // Calculate delta time using GameEngine's MTKView-driven timing
        let deltaTime = gameEngine.updateDeltaTime()

        // Update game logic
        gameEngine.update(deltaTime: deltaTime)

        // Render frame
        gameEngine.render()

        // DEBUG: Log that render completed
        log("MTKView draw() completed", level: .debug)
    }

    // MARK: - Touch Event Forwarding

    public override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesBegan(touches, with: event)
        if let view = self.view {
            touchInputHandler?.touchesBegan(touches, with: event, in: view)
        }
    }

    public override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesMoved(touches, with: event)
        if let view = self.view {
            touchInputHandler?.touchesMoved(touches, with: event, in: view)
        }
    }

    public override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesEnded(touches, with: event)
        if let view = self.view {
            touchInputHandler?.touchesEnded(touches, with: event, in: view)
        }
    }

    public override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesCancelled(touches, with: event)
        if let view = self.view {
            touchInputHandler?.touchesCancelled(touches, with: event, in: view)
        }
    }
}

// MARK: - C++ Integration Notes

/**
 * Swift 5.9+ Native C++ Interop Integration
 *
 * With Swift 5.9+, C++ can directly instantiate this Swift class without C-style bridging:
 *
 * // C++ Example:
 * #include "FloppyTurd-Swift.h"
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
