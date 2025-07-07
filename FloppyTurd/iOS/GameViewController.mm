#import "GameViewController.h"
#import <Metal/Metal.h>
#import "RaylibCompat.h"
#import "Game.h"
#import "PlatformLayerDelegate.h"
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

@interface GameViewController () <MTKViewDelegate> {
    MTKView *_metalView;
    dispatch_queue_t _gameQueue;
    BOOL _gameInitialized;
    CADisplayLink *_displayLink;
    CFTimeInterval _previousTime;
    id<MTLDevice> _device;
}

@end

@implementation GameViewController

- (void)loadView {
    // Create and configure the MTKView
    _metalView = [[MTKView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    _metalView.device = MTLCreateSystemDefaultDevice();
    _metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    // Disable MTKView's automatic drawing - we'll control it via CADisplayLink
    _metalView.paused = YES;
    _metalView.enableSetNeedsDisplay = YES; // Enable setNeedsDisplay so our calls work
    
    self.view = _metalView;
}

- (void)viewDidLoad {
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
    
    // Configure MTKView
    _metalView.device = _device;
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _metalView.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0);
    
    // Disable MTKView's automatic drawing - we'll control it via CADisplayLink
    _metalView.paused = YES;
    _metalView.enableSetNeedsDisplay = YES;
    
    // Setup touch handling
    _metalView.multipleTouchEnabled = YES;
    NSLog(@"[INIT] Set up touch handling");
    
    // Create game queue for background operations
    _gameQueue = dispatch_queue_create("com.floppyturd.game", DISPATCH_QUEUE_SERIAL);
    
    // CRITICAL: Initialize PlatformLayer BEFORE calling game_main
    NSLog(@"[INIT] Initializing PlatformLayer with MTKView: %p", _metalView);
    PlatformLayer& platformLayer = PlatformLayer::GetInstance();
    platformLayer.Initialize((__bridge void*)_metalView);
    NSLog(@"[INIT] PlatformLayer initialized successfully");
    
    // Initialize UIManager with safe area
    CGRect safeAreaRect = self.view.safeAreaLayoutGuide.layoutFrame;
    Rectangle safeArea = {
        static_cast<float>(safeAreaRect.origin.x),
        static_cast<float>(safeAreaRect.origin.y),
        static_cast<float>(safeAreaRect.size.width),
        static_cast<float>(safeAreaRect.size.height)
    };
    
    UIManager& uiManager = UIManager::GetInstance();
    uiManager.Initialize(
        static_cast<float>(safeAreaRect.size.width),
        static_cast<float>(safeAreaRect.size.height),
        safeArea
    );
    NSLog(@"[INIT] UIManager initialized with safe area: %.0fx%.0f", safeArea.width, safeArea.height);
    
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
        
        // Trigger rendering on main thread
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"[DEBUG] gameLoopTick: Triggering setNeedsDisplay");
            [_metalView setNeedsDisplay];
        });
    });
}

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    NSLog(@"[LAYOUT] View bounds: %@", NSStringFromCGRect(self.view.bounds));
    
    // Update MTKView frame to match the view's bounds
    if (_metalView && !CGRectEqualToRect(_metalView.frame, self.view.bounds)) {
        _metalView.frame = self.view.bounds;
        NSLog(@"[LAYOUT] MTKView frame updated to: %@", NSStringFromCGRect(_metalView.frame));
    }
    
    // Handle rotation if needed
    UIInterfaceOrientation newOrientation = self.view.window.windowScene.interfaceOrientation;
    NSLog(@"[ROTATION] Current orientation: %ld", (long)newOrientation);
    
    // Notify the game about orientation change
    if (_metalView) {
        // Trigger a redraw with the new orientation
        [_metalView setNeedsDisplay];
    }
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle view size changes
    SetWindowSize(size.width, size.height);
}

- (void)drawInMTKView:(MTKView *)view {
    // This is called at the preferred FPS rate
    // The actual Metal rendering will be handled by MetalRenderer
    // triggered from the game loop
    NSLog(@"[DEBUG] drawInMTKView called on thread: %@", [NSThread currentThread]);
    if (!_gameInitialized) {
        NSLog(@"[DEBUG] Game not initialized, skipping draw on thread: %@", [NSThread currentThread]);
        return;
    }
    
    Game* game = GetGameInstance();
    NSLog(@"[ACCESS] GetGameInstance() called from drawInMTKView, returning: %p on thread: %@", game, [NSThread currentThread]);
    if (!game) {
        NSLog(@"[DEBUG] Game instance is null, skipping draw on thread: %@", [NSThread currentThread]);
        return;
    }
    
    // Get the platform layer delegate to handle rendering
    PlatformLayer& platformLayer = PlatformLayer::GetInstance();
    if (platformLayer.GetDelegate()) {
        PlatformLayerDelegate* delegate = (__bridge PlatformLayerDelegate*)platformLayer.GetDelegate();
        // Let the delegate handle the rendering with MetalRenderer
        [delegate drawInMTKView:view];
    } else {
        NSLog(@"[ERROR] drawInMTKView: No platform layer delegate available");
    }
}

#pragma mark - Touch Handling

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        CGPoint location = [touch locationInView:_metalView];
        UpdateTouchState((int)touch.hash, location.x, location.y, true);
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        CGPoint location = [touch locationInView:_metalView];
        UpdateTouchState((int)touch.hash, location.x, location.y, true);
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        CGPoint location = [touch locationInView:_metalView];
        UpdateTouchState((int)touch.hash, location.x, location.y, false);
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        UpdateTouchState((int)touch.hash, 0, 0, false);
    }
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    ClearAllTouchStates(); // Clear touches when view is no longer active
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
    
    // Clear the MTKView delegate
    if (_metalView) {
        _metalView.delegate = nil;
    }
    
    // PlatformLayer is a singleton that manages its own lifetime
    NSLog(@"[CLEANUP] PlatformLayer cleanup handled by singleton");
    
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
        // Update UIManager with new screen size
        CGRect safeArea = self.view.safeAreaLayoutGuide.layoutFrame;
        Rectangle safeAreaRect = {
            (float)safeArea.origin.x, (float)safeArea.origin.y,
            (float)safeArea.size.width, (float)safeArea.size.height
        };
        
        UIManager& uiManager = UIManager::GetInstance();
        uiManager.Initialize(size.width, size.height, safeAreaRect);
        
        NSLog(@"[SYSTEM] Orientation changed, updated UIManager.");
        
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
