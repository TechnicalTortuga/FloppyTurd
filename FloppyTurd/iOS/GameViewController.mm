#import "GameViewController.h"
#import <Metal/Metal.h>
#import "RaylibCompat.h"
#import "Game.h"
#import "GameView.h"
#import "MetalRenderer.h"
#import "RaylibCompat_iOS.h"
#import "UIManager.h"
#import "PlatformLayer.h"

// Game state
extern "C++" {
    Game* GetGameInstance();
}

// Include our C++ game headers
extern "C" int game_main(int argc, char *argv[]);

@interface GameViewController () {
    GameView *_gameView;
    dispatch_queue_t _gameQueue;
    BOOL _gameInitialized;
    CADisplayLink *_displayLink;
    CFTimeInterval _previousTime;
    id<MTLDevice> _device;
}

@end

@implementation GameViewController

- (void)loadView {
    // Create and configure the GameView (custom MTKView subclass)
    _gameView = [[GameView alloc] initWithFrame:[[UIScreen mainScreen] bounds] 
                                         device:MTLCreateSystemDefaultDevice() 
                                           game:nullptr]; // Will be set after game initialization
    
    self.view = _gameView;
}

- (void)viewDidLoad {
    TraceLog(LOG_INFO, "[INIT] ========================================");
    TraceLog(LOG_INFO, "[INIT] GameViewController viewDidLoad STARTING on thread: %@", [NSThread currentThread]);
    
    [super viewDidLoad];
    
    TraceLog(LOG_INFO, "[ACCESS] Step 1: Checking initial game instance before any initialization on thread: %@", [NSThread currentThread]);
    Game* initialGame = GetGameInstance();
    TraceLog(LOG_INFO, "[ACCESS] Step 1: GetGameInstance() called before initialization, returning: %p on thread: %@", initialGame, [NSThread currentThread]);
    
    // Initialize Metal
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        [self showErrorAlert:@"This device does not support Metal"];
        return;
    }
    
    // GameView is already configured in loadView
    // The GameView handles all MTKView configuration and touch handling automatically
    
    TraceLog(LOG_INFO, "[INIT] GameView configuration completed");
    TraceLog(LOG_INFO, "[INIT] GameView frame: %@", NSStringFromCGRect(_gameView.frame));
    TraceLog(LOG_INFO, "[INIT] GameView bounds: %@", NSStringFromCGRect(_gameView.bounds));
    
    // Create game queue for background operations
    _gameQueue = dispatch_queue_create("com.floppyturd.game", DISPATCH_QUEUE_SERIAL);
    
    // CRITICAL: Initialize PlatformLayer BEFORE calling game_main
    TraceLog(LOG_INFO, "[INIT] Initializing PlatformLayer with GameView: %p", _gameView);
    PlatformLayer& platformLayer = PlatformLayer::GetInstance();
    
    // Get the GameView's MetalRenderer to share with PlatformLayer
    void* gameViewMetalRenderer = [_gameView getMetalRenderer];
    TraceLog(LOG_INFO, "[INIT] GameView MetalRenderer: %p", gameViewMetalRenderer);
    
    platformLayer.Initialize((__bridge void*)_gameView, (__bridge void*)self, gameViewMetalRenderer);
    TraceLog(LOG_INFO, "[INIT] PlatformLayer initialized successfully");
    
    // --- Metal/iOS Native Pixel Initialization ---
    UIScreen* screen = [UIScreen mainScreen];
    CGRect nativeBounds = screen.nativeBounds; // in pixels
    CGFloat nativeScale = screen.nativeScale;
    CGRect bounds = self.view.bounds; // in points
    UIEdgeInsets insets = self.view.safeAreaInsets;
    CGRect safeAreaPoints = UIEdgeInsetsInsetRect(bounds, insets); // in points
    CGRect safeAreaPixels = CGRectMake(safeAreaPoints.origin.x * nativeScale,
                                       safeAreaPoints.origin.y * nativeScale,
                                       safeAreaPoints.size.width * nativeScale,
                                       safeAreaPoints.size.height * nativeScale);
    // Set Metal layer drawable size to native pixel size
    if ([self.view isKindOfClass:[MTKView class]]) {
        MTKView* mtkView = (MTKView*)self.view;
        mtkView.contentScaleFactor = nativeScale;
        mtkView.drawableSize = CGSizeMake(nativeBounds.size.width, nativeBounds.size.height);
    }
    // Initialize UIManager with both point and pixel safe area
    UIManager& uiManager = UIManager::GetInstance();
    uiManager.Initialize((float)bounds.size.width, (float)bounds.size.height, // points
                        (float)nativeBounds.size.width, (float)nativeBounds.size.height, // pixels
                        { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height }, // safe area in points
                        { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height }); // safe area in pixels
    TraceLog(LOG_INFO, "[INIT] UIManager initialized: points=%.0fx%.0f, pixels=%.0fx%.0f, safeAreaPoints=(%.0f,%.0f,%.0f,%.0f), safeAreaPixels=(%.0f,%.0f,%.0f,%.0f)",
        bounds.size.width, bounds.size.height, nativeBounds.size.width, nativeBounds.size.height,
        safeAreaPoints.origin.x, safeAreaPoints.origin.y, safeAreaPoints.size.width, safeAreaPoints.size.height,
        safeAreaPixels.origin.x, safeAreaPixels.origin.y, safeAreaPixels.size.width, safeAreaPixels.size.height);
    
    // Now initialize the game directly on the main thread.
    // This is CRITICAL to prevent race conditions where the game loop
    // starts before the game instance is created.
    TraceLog(LOG_INFO, "[INIT] Step 2: Starting synchronous game initialization on main thread: %@", [NSThread currentThread]);
    int result = game_main(0, nullptr);
    
    if (result == 0) {
        TraceLog(LOG_INFO, "[INIT] Step 3: Game initialization successful, checking game instance on thread: %@", [NSThread currentThread]);
        Game* postInitGame = GetGameInstance();
        TraceLog(LOG_INFO, "[ACCESS] Step 3: GetGameInstance() called after initialization, returning: %p on thread: %@", postInitGame, [NSThread currentThread]);
        
        // Set the game instance in GameView for rendering
        _gameView.game = postInitGame;
        TraceLog(LOG_INFO, "[INIT] Game instance set in GameView: %p", postInitGame);
        
        _gameInitialized = YES;
        [self startGameLoop];
    } else {
        TraceLog(LOG_ERROR, "[ERROR] Step 3: Game initialization failed with code: %d on thread: %@", result, [NSThread currentThread]);
        Game* failedInitGame = GetGameInstance();
        TraceLog(LOG_INFO, "[ACCESS] Step 3: GetGameInstance() called after failed initialization, returning: %p on thread: %@", failedInitGame, [NSThread currentThread]);
        [self showErrorAlert:[NSString stringWithFormat:@"Game initialization failed. Error code: %d", result]];
    }
    
    TraceLog(LOG_INFO, "[INIT] GameViewController viewDidLoad COMPLETED");
    TraceLog(LOG_INFO, "[INIT] ========================================");
}

- (void)setupDisplayLink {
    TraceLog(LOG_INFO, "[DEBUG] setupDisplayLink called");
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(gameLoopTick:)];
    _displayLink.preferredFramesPerSecond = 60;
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    _previousTime = CACurrentMediaTime();
    TraceLog(LOG_INFO, "[DEBUG] setupDisplayLink completed, displayLink=%p", _displayLink);
}

- (void)gameLoopTick:(CADisplayLink *)sender {
    TraceLog(LOG_INFO, "[DEBUG] gameLoopTick called on thread: %@", [NSThread currentThread]);
    if (!_gameInitialized) {
        TraceLog(LOG_INFO, "[DEBUG] Game not initialized, skipping gameLoopTick on thread: %@", [NSThread currentThread]);
        return;
    }
    
    Game* game = GetGameInstance();
    TraceLog(LOG_INFO, "[ACCESS] GetGameInstance() called from gameLoopTick, returning: %p on thread: %@", game, [NSThread currentThread]);
    if (!game) {
        TraceLog(LOG_INFO, "[DEBUG] Game instance is null, skipping gameLoopTick on thread: %@", [NSThread currentThread]);
        return;
    }
    
    // Calculate delta time
    CFTimeInterval currentTime = CACurrentMediaTime();
    float deltaTime = (float)(currentTime - _previousTime);
    _previousTime = currentTime;
    
    // Clamp delta time to prevent large jumps
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    
    TraceLog(LOG_INFO, "[DEBUG] gameLoopTick: deltaTime=%f, calling game->HandleInputFrame + UpdateFrame", deltaTime);
    
    // Run game update on the game queue
    dispatch_async(_gameQueue, ^{
        // Phase 1: Handle input (latch all input before game logic)
        game->HandleInputFrame();
        
        // Phase 2: Update game logic
        game->UpdateFrame(deltaTime);
        
        // Phase 3: Trigger rendering on main thread
        dispatch_async(dispatch_get_main_queue(), ^{
            TraceLog(LOG_INFO, "[DEBUG] gameLoopTick: Triggering setNeedsDisplay");
            [_gameView setNeedsDisplay];
        });
    });
}

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    TraceLog(LOG_INFO, "[LAYOUT] View bounds: %@", NSStringFromCGRect(self.view.bounds));
    
    // Update GameView frame to match the view's bounds
    if (_gameView && !CGRectEqualToRect(_gameView.frame, self.view.bounds)) {
        _gameView.frame = self.view.bounds;
        TraceLog(LOG_INFO, "[LAYOUT] GameView frame updated to: %@", NSStringFromCGRect(_gameView.frame));
    }
    
    // Handle rotation if needed
    UIInterfaceOrientation newOrientation = self.view.window.windowScene.interfaceOrientation;
    TraceLog(LOG_INFO, "[ROTATION] Current orientation: %ld", (long)newOrientation);
    
    // Notify the game about orientation change
    if (_gameView) {
        // Trigger a redraw with the new orientation
        [_gameView setNeedsDisplay];
    }
}

#pragma mark - Touch Handling

// Touch handling is now managed by GameView
// The GameView processes touch events and forwards them to TouchControls

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    // Touch state clearing is handled by GameView
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    
    // Pause the display link when view is not visible
    if (_displayLink) {
        [_displayLink setPaused:YES];
    }
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    // Resume the display link when view becomes visible
    if (_displayLink) {
        [_displayLink setPaused:NO];
        _previousTime = CACurrentMediaTime(); // Reset time to avoid large delta
    }
}

- (void)dealloc {
    TraceLog(LOG_INFO, "[CLEANUP] GameViewController dealloc");
    
    // Invalidate display link
    if (_displayLink) {
        [_displayLink invalidate];
        _displayLink = nil;
    }
    
    // Shutdown the game
    Game* game = GetGameInstance();
    TraceLog(LOG_INFO, "[ACCESS] GetGameInstance() called from dealloc, returning: %p", game);
    if (game) {
        TraceLog(LOG_INFO, "[CLEANUP] Shutting down game instance");
        game->Shutdown();
        delete game;
        TraceLog(LOG_INFO, "[ACCESS] SetGameInstance(nullptr) called from dealloc");
        SetGameInstance(nullptr);
        Game* afterNullGame = GetGameInstance();
        TraceLog(LOG_INFO, "[ACCESS] GetGameInstance() called after SetGameInstance(nullptr), returning: %p", afterNullGame);
    }
    
    // Clear the GameView delegate
    if (_gameView) {
        _gameView.delegate = nil;
    }
    
    // PlatformLayer is a singleton that manages its own lifetime
    TraceLog(LOG_INFO, "[CLEANUP] PlatformLayer cleanup handled by singleton");
    
    // ARC will handle the rest of the Objective-C objects
}

#pragma mark - UI Helpers

- (void)showLoadingIndicator:(BOOL)show {
    static UIActivityIndicatorView *spinner = nil;
    if (show) {
        if (!spinner) {
            spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
            spinner.center = self.view.center;
            spinner.color = [UIColor whiteColor];
            spinner.hidesWhenStopped = YES;
        }
        [self.view addSubview:spinner];
        [spinner startAnimating];
    } else {
        if (spinner) {
            [spinner stopAnimating];
            [spinner removeFromSuperview];
        }
    }
}

#pragma mark - Alert Handling

- (void)startGameLoop {
    TraceLog(LOG_INFO, "[INIT] Starting game loop with CADisplayLink");
    
    // Create display link for 60 FPS
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(gameLoopTick:)];
    _displayLink.preferredFramesPerSecond = 60;
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    
    _previousTime = CACurrentMediaTime();
    TraceLog(LOG_INFO, "[INIT] Game loop started successfully");
}

- (void)showErrorAlert:(NSString *)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Initialization Failed"
                                                                       message:message
                                                                preferredStyle:UIAlertControllerStyleAlert];
        
        UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK"
                                                           style:UIAlertActionStyleDefault
                                                         handler:^(UIAlertAction * action) {
            // Exit the app or try to restart
            exit(1);
        }];
        
        [alert addAction:okAction];
        [self presentViewController:alert animated:YES completion:nil];
    });
}

- (void)showErrorAlert:(NSString*)title withMessage:(NSString*)message {
    // Ensure UI updates happen on the main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *alert = [UIAlertController 
            alertControllerWithTitle:title 
            message:message 
            preferredStyle:UIAlertControllerStyleAlert];
            
        UIAlertAction *okAction = [UIAlertAction 
            actionWithTitle:@"OK" 
            style:UIAlertActionStyleDefault 
            handler:nil];
        
        [alert addAction:okAction];
        [self presentViewController:alert animated:YES completion:nil];
    });
}

#pragma mark - Device Orientation

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    
    // Update orientation on transition
    [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        // Get current screen scale and bounds
        UIScreen* screen = [UIScreen mainScreen];
        CGFloat nativeScale = screen.nativeScale;
        CGRect nativeBounds = screen.nativeBounds;
        
        // Calculate pixel dimensions
        CGSize sizePixels = CGSizeMake(size.width * nativeScale, size.height * nativeScale);
        
        // Get safe area in points and convert to pixels
        UIEdgeInsets insets = self.view.safeAreaInsets;
        CGRect safeAreaPoints = UIEdgeInsetsInsetRect(CGRectMake(0, 0, size.width, size.height), insets);
        CGRect safeAreaPixels = CGRectMake(safeAreaPoints.origin.x * nativeScale,
                                           safeAreaPoints.origin.y * nativeScale,
                                           safeAreaPoints.size.width * nativeScale,
                                           safeAreaPoints.size.height * nativeScale);
        
        // Update UIManager with new screen size
        UIManager& uiManager = UIManager::GetInstance();
        uiManager.Initialize((float)size.width, (float)size.height, // points
                            (float)sizePixels.width, (float)sizePixels.height, // pixels
                            { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height }, // safe area in points
                            { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height }); // safe area in pixels
        
        TraceLog(LOG_INFO, "[SYSTEM] Orientation changed, updated UIManager: points=%.0fx%.0f, pixels=%.0fx%.0f", 
              size.width, size.height, sizePixels.width, sizePixels.height);
        
    } completion:nil];
}

- (BOOL)shouldAutorotate {
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

@end
