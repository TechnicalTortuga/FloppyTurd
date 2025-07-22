//
//  GameViewControllerSwift.swift
//  Professional Game Engine - Native Swift View Controller
//
//  Created by C++ Swift Interop Migration
//  Direct Swift replacement for GameViewController.mm with modern UI patterns
//

#if canImport(UIKit)
import UIKit
#endif
import Metal
import MetalKit

// MARK: - Direct C++ Interop (replacing old bridge functions)
// Note: These functions are now accessed directly through GameEngineCpp module

/// Professional Swift game view controller with native lifecycle management
/// Direct replacement for GameViewController.mm with modern Swift patterns
@MainActor
public class GameViewControllerSwift: UIViewController {
    
    // MARK: - Properties
    private var gameView: GameViewSwift?
    private var metalDevice: MTLDevice?
    private var loadingViewController: LoadingViewController?
    
    // MARK: - Loading State
    private var isGameLoaded: Bool = false
    private var initializationError: Error?
    
    // MARK: - Lifecycle
    
    public    override func viewDidLoad() {
        super.viewDidLoad()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] 🚀 Starting game view controller...")
        
        // Continue with normal initialization
        setupUI()
        loadGameAsync()
    }
    // MARK: - Async Game Loading Stub
    private func loadGameAsync() {
        // For compatibility with legacy code, just call initializeGameAsync
        initializeGameAsync()
    }
    
    public override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] viewWillAppear")
        
        // Hide status bar for immersive gaming
        setupImmersiveMode()
        
        // Start game if not already started
        if isGameLoaded {
            gameView?.resumeGameLoop()
        }
    }
    
    public override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] viewDidDisappear")
        
        // Pause game when view disappears
        gameView?.pauseGameLoop()
    }
    
    public override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        // Update game view frame to match controller
        gameView?.frame = view.bounds
    }
    
    deinit {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Deallocating view controller")
        Task { [weak self] in
            guard let self = self else { return }
            await self.cleanupResources()
        }
    }
    
    // MARK: - Setup
    
    private func setupUI() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Setting up view controller...")
        
        // Show loading screen immediately
        showLoadingScreen()
        
        // Initialize Metal device
        guard let device = MTLCreateSystemDefaultDevice() else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewControllerSwift] ERROR: Metal is not supported on this device")
            showError(message: "Metal graphics are not supported on this device. The game requires Metal to run.")
            return
        }
        
        metalDevice = device
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] ✅ Metal device created: \(device.name)")
        
        // Initialize game asynchronously to avoid blocking UI
        initializeGameAsync()
    }
    
    private func setupImmersiveMode() {
        // Enable immersive gaming mode
        setNeedsStatusBarAppearanceUpdate()
        
        // Disable idle timer during gameplay
        UIApplication.shared.isIdleTimerDisabled = true
        
        // Set preferred frame rate (if available)
        if #available(iOS 10.3, *), let gameView = self.gameView {
            gameView.preferredFramesPerSecond = 60
        }
    }
    
    // MARK: - Game Initialization
    
    private func initializeGameAsync() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Starting async game initialization...")
        
        // Perform game initialization on a background queue
        Task.detached { [weak self] in
            guard let self = self else { return }
            do {
                // Simulate any heavy initialization work here
                // (loading assets, initializing systems, etc.)
                try await self.performGameInitialization()
                await MainActor.run { self.onGameInitializationComplete() }
            } catch {
                traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewControllerSwift] ERROR: Game initialization failed: \(error)")
                await MainActor.run {
                    self.initializationError = error
                    self.onGameInitializationFailed(error: error)
                }
            }
        }
    }
    
    @MainActor
    private func performGameInitialization() async throws {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Performing game initialization...")
        
        // Add any heavy initialization work here
        // For now, we'll just simulate some work
        try await Task.sleep(nanoseconds: 1_000_000_000) // Simulate loading time
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] ✅ Game initialization complete")
    }
    
    private func onGameInitializationComplete() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Game initialization completed successfully")
        
        // Step 1: Create game view first (this sets up Metal)
        createGameView()
        
        // Step 2: The Swift architecture doesn't need SetGlobalGameView
        // Swift GameEngine handles this internally via GameViewSwift
        if let gv = self.gameView {
            traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] GameView ready: \(gv)")
        }
        
        // Step 3: Initialize UIManager with screen dimensions
        initializeUIManager()
        
        // Step 4: Initialize Swift GameEngine (replaces C++ game_main)
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Initializing Swift GameEngine...")
        guard let gameView = self.gameView else {
            showError(message: "GameView not created")
            return
        }
        
        // The GameEngine initialization is handled by GameViewSwift internally
        // Just start the game loop
        gameView.startGameLoop()
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] ✅ Swift GameEngine started successfully")
        
        // Step 5: Hide loading screen and start the game
        hideLoadingScreen()
        startGame()
    }
    
    private func onGameInitializationFailed(error: Error) {
        traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewControllerSwift] Game initialization failed: \(error)")
        
        hideLoadingScreen()
        showError(message: "Failed to initialize the game: \(error.localizedDescription)")
    }
    
    // MARK: - UIManager Setup
    
    private func initializeUIManager() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Initializing UIManager with screen dimensions...")
        
        let bounds = view.bounds
        let nativeBounds = UIScreen.main.nativeBounds
        let nativeScale = UIScreen.main.nativeScale
        let insets = view.safeAreaInsets
        
        let sizePoints = bounds.size
        let sizePixels = CGSize(width: nativeBounds.size.width, height: nativeBounds.size.height)
        
        let safeAreaPoints = CGRect(
            x: 0 + insets.left,
            y: 0 + insets.top,
            width: sizePoints.width - insets.left - insets.right,
            height: sizePoints.height - insets.top - insets.bottom
        )
        let safeAreaPixels = CGRect(
            x: safeAreaPoints.origin.x * nativeScale,
            y: safeAreaPoints.origin.y * nativeScale,
            width: safeAreaPoints.size.width * nativeScale,
            height: safeAreaPoints.size.height * nativeScale
        )
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Screen dimensions: points=\(sizePoints.width)x\(sizePoints.height), pixels=\(sizePixels.width)x\(sizePixels.height)")
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Safe area: points=\(safeAreaPoints), pixels=\(safeAreaPixels)")
        
        // Initialize UIManager using Swift architecture
        let uiManager = UIManagerSwift.shared
        
        // Create Rectangle structures for safe areas (C++ types from GameEngineCpp)
        let safeAreaPointsRect = Rectangle(
            x: Float(safeAreaPoints.origin.x),
            y: Float(safeAreaPoints.origin.y),
            width: Float(safeAreaPoints.size.width),
            height: Float(safeAreaPoints.size.height)
        )
        
        let safeAreaPixelsRect = Rectangle(
            x: Float(safeAreaPixels.origin.x),
            y: Float(safeAreaPixels.origin.y),
            width: Float(safeAreaPixels.size.width),
            height: Float(safeAreaPixels.size.height)
        )
        
        // Update screen info in Swift UIManager (automatically handles initialization)
        uiManager.updateScreenInfo()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] ✅ UIManagerSwift updated successfully")
    }
    
    // MARK: - Game View Management
    
    private func createGameView() {
        guard let device = metalDevice else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewControllerSwift] ERROR: No Metal device available")
            return
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Creating game view...")
        
        // Create the game view
        gameView = GameViewSwift(frame: view.bounds, metalDevice: device)
        
        guard let gameView = gameView else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewControllerSwift] ERROR: Failed to create game view")
            showError(message: "Failed to create the game view")
            return
        }
        
        // Add to view hierarchy
        view.addSubview(gameView)
        gameView.translatesAutoresizingMaskIntoConstraints = false
        
        // Setup constraints
        NSLayoutConstraint.activate([
            gameView.topAnchor.constraint(equalTo: view.topAnchor),
            gameView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            gameView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            gameView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] ✅ Game view created and added to hierarchy")
    }
    
    private func startGame() {
        guard let gameView = gameView else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewControllerSwift] ERROR: No game view to start")
            return
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Starting game...")
        
        gameView.startGameLoop()
        isGameLoaded = true
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] ✅ Game started successfully")
    }
    
    // MARK: - Loading Screen Management
    
    private func showLoadingScreen() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Showing loading screen...")
        
        loadingViewController = LoadingViewController()
        
        guard let loadingVC = loadingViewController else { return }
        
        addChild(loadingVC)
        view.addSubview(loadingVC.view)
        loadingVC.view.frame = view.bounds
        loadingVC.view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        loadingVC.didMove(toParent: self)
        
        loadingVC.showLoadingMessage("Initializing FloppyTurd...")
    }
    
    private func hideLoadingScreen() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Hiding loading screen...")
        
        guard let loadingVC = loadingViewController else { return }
        
        UIView.animate(withDuration: 0.3, animations: {
            loadingVC.view.alpha = 0.0
        }) { _ in
            loadingVC.willMove(toParent: nil)
            loadingVC.view.removeFromSuperview()
            loadingVC.removeFromParent()
            self.loadingViewController = nil
        }
    }
    
    // MARK: - Error Handling
    
    private func showError(message: String) {
        traceLog(SWLogLevel.SWLOG_ERROR, "[GameViewControllerSwift] Showing error: \(message)")
        
        let alert = UIAlertController(
            title: "FloppyTurd Error",
            message: message,
            preferredStyle: .alert
        )
        
        alert.addAction(UIAlertAction(title: "Retry", style: .default) { _ in
            self.retryInitialization()
        })
        
        alert.addAction(UIAlertAction(title: "Exit", style: .destructive) { _ in
            self.exitApplication()
        })
        
        present(alert, animated: true)
    }
    
    private func retryInitialization() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Retrying initialization...")
        Task {
            await cleanupResources()
            setupUI()
        }
    }
    
    private func exitApplication() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Exiting application...")
        Task {
            await cleanupResources()
            exit(0)
        }
    }
    
    // MARK: - App Lifecycle Integration
    
    @objc private func applicationDidEnterBackground() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Application entering background")
        gameView?.applicationDidEnterBackground()
    }
    
    @objc private func applicationWillEnterForeground() {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Application entering foreground")
        gameView?.applicationWillEnterForeground()
    }
    
    public override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        // Register for app lifecycle notifications
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(applicationDidEnterBackground),
            name: UIApplication.didEnterBackgroundNotification,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(applicationWillEnterForeground),
            name: UIApplication.willEnterForegroundNotification,
            object: nil
        )
    }
    
    // MARK: - Status Bar
    
    public override var prefersStatusBarHidden: Bool {
        return true // Hide status bar for immersive gaming
    }
    
    public override var preferredStatusBarUpdateAnimation: UIStatusBarAnimation {
        return .fade
    }
    
    // MARK: - Orientation
    
    public override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        // Support landscape orientations for gaming
        return [.landscapeLeft, .landscapeRight]
    }
    
    public override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
        return .landscapeRight
    }
    
    public override var shouldAutorotate: Bool {
        return true
    }
    
    // MARK: - Memory Management
    
    public override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        
        traceLog(SWLogLevel.SWLOG_WARNING, "[GameViewControllerSwift] Received memory warning")
        
        // Let the game engine handle memory cleanup
        GameEngine.handleMemoryWarning()
    }
    
    @MainActor
    private func cleanupResources() async {
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] Cleaning up resources...")
        
        // Remove notifications
        NotificationCenter.default.removeObserver(self)
        
        // Re-enable idle timer
        UIApplication.shared.isIdleTimerDisabled = false
        
        // Clean up game view
        gameView?.stopGameLoop()
        gameView?.removeFromSuperview()
        gameView = nil
        
        // Clean up loading screen
        loadingViewController?.willMove(toParent: nil)
        loadingViewController?.view.removeFromSuperview()
        loadingViewController?.removeFromParent()
        loadingViewController = nil
        
        // Reset state
        isGameLoaded = false
        initializationError = nil
        
        traceLog(SWLogLevel.SWLOG_INFO, "[GameViewControllerSwift] ✅ Resource cleanup complete")
    }
    
    // MARK: - Public API
    
    /// Get the current game view
    public var currentGameView: GameViewSwift? {
        return gameView
    }
    
    /// Check if the game is currently loaded and running
    public var isGameActive: Bool {
        return isGameLoaded && gameView?.isGameRunning == true
    }
    
    /// Get current FPS (for debugging)
    public var currentFPS: Float {
        return gameView?.currentFPS ?? 0.0
    }
}

// MARK: - Loading View Controller

/// Simple loading view controller for game initialization
private class LoadingViewController: UIViewController {
    
    private var activityIndicator: UIActivityIndicatorView!
    private var messageLabel: UILabel!
    private var progressView: UIProgressView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupLoadingUI()
    }
    
    private func setupLoadingUI() {
        view.backgroundColor = UIColor.black
        
        // Activity indicator
        if #available(iOS 13.0, *) {
            activityIndicator = UIActivityIndicatorView(style: .large)
        } else {
            activityIndicator = UIActivityIndicatorView(style: .whiteLarge)
        }
        
        activityIndicator.color = .white
        activityIndicator.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(activityIndicator)
        
        // Message label
        messageLabel = UILabel()
        messageLabel.text = "Loading..."
        messageLabel.textColor = .white
        messageLabel.textAlignment = .center
        messageLabel.font = UIFont.systemFont(ofSize: 18, weight: .medium)
        messageLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(messageLabel)
        
        // Progress view
        progressView = UIProgressView(progressViewStyle: .default)
        progressView.trackTintColor = UIColor.white.withAlphaComponent(0.3)
        progressView.progressTintColor = .white
        progressView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(progressView)
        
        // Layout constraints
        NSLayoutConstraint.activate([
            activityIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            activityIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: -40),
            
            messageLabel.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            messageLabel.topAnchor.constraint(equalTo: activityIndicator.bottomAnchor, constant: 20),
            messageLabel.leadingAnchor.constraint(greaterThanOrEqualTo: view.leadingAnchor, constant: 20),
            messageLabel.trailingAnchor.constraint(lessThanOrEqualTo: view.trailingAnchor, constant: -20),
            
            progressView.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            progressView.topAnchor.constraint(equalTo: messageLabel.bottomAnchor, constant: 20),
            progressView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 40),
            progressView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -40)
        ])
        
        // Start animating
        activityIndicator.startAnimating()
    }
    
    func showLoadingMessage(_ message: String) {
        messageLabel.text = message
    }
    
    func updateProgress(_ progress: Float) {
        progressView.setProgress(progress, animated: true)
    }
}
