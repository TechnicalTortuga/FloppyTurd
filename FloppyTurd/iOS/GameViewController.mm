#import "GameViewController.h"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import "Game.h"
#import "GameView.h"
#import "MetalRenderer.h"
#import "UIManager.h"
#import "PlatformAPI.h"

// Game state
extern "C++" {
    extern "C" Game* GetGameInstance();
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
    // Create and configure the GameView
    _gameView = [[GameView alloc] initWithFrame:[[UIScreen mainScreen] bounds] game:GetGameInstance()];
    self.view = _gameView;
    
    // Set the global GameView pointer for RaylibCompat_iOS performance optimization
    SetGlobalGameView(_gameView);
    NSLog(@"[INIT] SetGlobalGameView called with GameView: %p", _gameView);
}

- (void)viewDidLoad {
    NSLog(@"[TEST_NSLOG] ========================================");
    NSLog(@"[TEST_NSLOG] This is a test NSLog call from GameViewController.mm");
    NSLog(@"[TEST_NSLOG] ========================================");
    
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] GameViewController viewDidLoad STARTING on thread: %@", [NSThread currentThread]);
    
    [super viewDidLoad];
    
    NSLog(@"[ACCESS] Step 1: Checking initial game instance before any initialization on thread: %@", [NSThread currentThread]);
    Game* initialGame = GetGameInstance();
    NSLog(@"[ACCESS] Step 1: GetGameInstance() called before initialization, returning: %p on thread: %@", initialGame, [NSThread currentThread]);
    
    // Initialize Metal
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        [self showErrorAlert:@"This device does not support Metal"];
        return;
    }
    
    // GameView handles its own configuration and touch handling
    NSLog(@"[INIT] GameView initialized and handling touch input and rendering");
    
    // Create game queue for background operations
    _gameQueue = dispatch_queue_create("com.floppyturd.game", DISPATCH_QUEUE_SERIAL);
    
    // CRITICAL: Initialize PlatformAPI BEFORE calling game_main
    NSLog(@"[INIT] PlatformAPI is already initialized");
    
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
    // GameView handles drawable size internally
    // Initialize UIManager with both point and pixel safe area
    UIManager& uiManager = UIManager::GetInstance();
    uiManager.Initialize((float)bounds.size.width, (float)bounds.size.height, // points
                        (float)nativeBounds.size.width, (float)nativeBounds.size.height, // pixels
                        { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height }, // safe area in points
                        { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height }); // safe area in pixels
    NSLog(@"[INIT] UIManager initialized: points=%.0fx%.0f, pixels=%.0fx%.0f, safeAreaPoints=(%.0f,%.0f,%.0f,%.0f), safeAreaPixels=(%.0f,%.0f,%.0f,%.0f)",
        bounds.size.width, bounds.size.height, nativeBounds.size.width, nativeBounds.size.height,
        safeAreaPoints.origin.x, safeAreaPoints.origin.y, safeAreaPoints.size.width, safeAreaPoints.size.height,
        safeAreaPixels.origin.x, safeAreaPixels.origin.y, safeAreaPixels.size.width, safeAreaPixels.size.height);
    
    // Now initialize the game directly on the main thread.
    // This is CRITICAL to prevent race conditions where the game loop
    // starts before the game instance is created.
    NSLog(@"[INIT] Step 2: Starting synchronous game initialization on main thread: %@", [NSThread currentThread]);
    int result = game_main(0, nullptr);
    
    if (result == 0) {
        NSLog(@"[INIT] Step 3: Game initialization successful, checking game instance on thread: %@", [NSThread currentThread]);
        Game* postInitGame = GetGameInstance();
        NSLog(@"[ACCESS] Step 3: GetGameInstance() called after initialization, returning: %p on thread: %@", postInitGame, [NSThread currentThread]);
        _gameInitialized = YES;
        [self startGameLoop];
    } else {
        NSLog(@"[ERROR] Step 3: Game initialization failed with code: %d on thread: %@", result, [NSThread currentThread]);
        Game* failedInitGame = GetGameInstance();
        NSLog(@"[ACCESS] Step 3: GetGameInstance() called after failed initialization, returning: %p on thread: %@", failedInitGame, [NSThread currentThread]);
        [self showErrorAlert:[NSString stringWithFormat:@"Game initialization failed. Error code: %d", result]];
    }
    
    NSLog(@"[INIT] GameViewController viewDidLoad COMPLETED");
    NSLog(@"[INIT] ========================================");
}

- (void)setupDisplayLink {
    NSLog(@"[DEBUG] setupDisplayLink called");
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(gameLoopTick:)];
    _displayLink.preferredFramesPerSecond = 60;
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    _previousTime = CACurrentMediaTime();
    NSLog(@"[DEBUG] setupDisplayLink completed, displayLink=%p", _displayLink);
}

- (void)gameLoopTick:(CADisplayLink *)sender {
    NSLog(@"[DEBUG] gameLoopTick called on thread: %@", [NSThread currentThread]);
    if (!_gameInitialized) {
        NSLog(@"[DEBUG] Game not initialized, skipping gameLoopTick on thread: %@", [NSThread currentThread]);
        return;
    }
    
    Game* game = GetGameInstance();
    NSLog(@"[ACCESS] GetGameInstance() called from gameLoopTick, returning: %p on thread: %@", game, [NSThread currentThread]);
    if (!game) {
        NSLog(@"[DEBUG] Game instance is null, skipping gameLoopTick on thread: %@", [NSThread currentThread]);
        return;
    }
    
    // Calculate delta time
    CFTimeInterval currentTime = CACurrentMediaTime();
    float deltaTime = (float)(currentTime - _previousTime);
    _previousTime = currentTime;
    
    // Clamp delta time to prevent large jumps
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    
    NSLog(@"[DEBUG] gameLoopTick: deltaTime=%f, calling game->UpdateFrame", deltaTime);
    
    // Run game update on the game queue
    dispatch_async(_gameQueue, ^{
        // Update game logic
        game->UpdateFrame(deltaTime);
        
        // Trigger rendering on main thread via GameView
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"[DEBUG] gameLoopTick: Triggering render on GameView");
            [_gameView render];
        });
    });
}

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    NSLog(@"[LAYOUT] View bounds: %@", NSStringFromCGRect(self.view.bounds));
    
    // GameView handles its own layout updates
    if (_gameView && !CGRectEqualToRect(_gameView.frame, self.view.bounds)) {
        _gameView.frame = self.view.bounds;
        NSLog(@"[LAYOUT] GameView frame updated to: %@", NSStringFromCGRect(_gameView.frame));
    }
    
    // Handle rotation if needed
    UIInterfaceOrientation newOrientation = self.view.window.windowScene.interfaceOrientation;
    NSLog(@"[ROTATION] Current orientation: %ld", (long)newOrientation);
    
    // Notify the game about orientation change
    if (_gameView) {
        // Trigger a redraw with the new orientation
        [_gameView render];
    }
}

#pragma mark - Touch Handling

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    // GameView handles touch events internally
    [super touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    // GameView handles touch events internally
    [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    // GameView handles touch events internally
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    // GameView handles touch events internally
    [super touchesCancelled:touches withEvent:event];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    // GameView handles touch state clearing internally
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
    NSLog(@"[CLEANUP] GameViewController dealloc");
    
    // Invalidate display link
    if (_displayLink) {
        [_displayLink invalidate];
        _displayLink = nil;
    }
    
    // Shutdown the game
    Game* game = GetGameInstance();
    NSLog(@"[ACCESS] GetGameInstance() called from dealloc, returning: %p", game);
    if (game) {
        NSLog(@"[CLEANUP] Shutting down game instance");
        game->Shutdown();
        delete game;
        NSLog(@"[ACCESS] SetGameInstance(nullptr) called from dealloc");
        SetGameInstance(nullptr);
        Game* afterNullGame = GetGameInstance();
        NSLog(@"[ACCESS] GetGameInstance() called after SetGameInstance(nullptr), returning: %p", afterNullGame);
    }
    
    // GameView handles its own cleanup
    
    // PlatformAPI is a singleton that manages its own lifetime
    NSLog(@"[CLEANUP] PlatformAPI cleanup handled by singleton");
    
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
    NSLog(@"[INIT] Starting game loop with CADisplayLink");
    
    // Create display link for 60 FPS
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(gameLoopTick:)];
    _displayLink.preferredFramesPerSecond = 60;
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    
    _previousTime = CACurrentMediaTime();
    NSLog(@"[INIT] Game loop started successfully");
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
        
        NSLog(@"[SYSTEM] Orientation changed, updated UIManager: points=%.0fx%.0f, pixels=%.0fx%.0f", 
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
