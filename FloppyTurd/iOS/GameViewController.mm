#import "GameViewController.h"
#import <Metal/Metal.h>
#import "RaylibCompat.h"
#import "Game.h"
#import "PlatformLayerDelegate.h"

// Include our C++ game headers
extern "C" int game_main(int argc, char *argv[]);

@interface GameViewController () {
    MTKView *_metalView;
    dispatch_queue_t _gameQueue;
    BOOL _gameInitialized;
    CADisplayLink *_displayLink;
    CFTimeInterval _previousTime;
}
@end

@implementation GameViewController

- (void)loadView {
    // Create MTKView
    _metalView = [[MTKView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    _metalView.device = MTLCreateSystemDefaultDevice();
    _metalView.delegate = self;
    _metalView.preferredFramesPerSecond = 60;
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    // Disable MTKView's automatic drawing - we'll control it via CADisplayLink
    _metalView.paused = YES;
    _metalView.enableSetNeedsDisplay = YES; // Enable setNeedsDisplay so our calls work
    
    self.view = _metalView;
}

- (void)viewDidLoad {
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] GameViewController viewDidLoad STARTING");
    NSLog(@"[INIT] ========================================");
    [super viewDidLoad];
    
    // Setup touch handling
    _metalView.multipleTouchEnabled = YES;
    NSLog(@"[INIT] Set up touch handling");
    
    // Create game queue
    _gameQueue = dispatch_queue_create("com.floppyturd.gamequeue", DISPATCH_QUEUE_SERIAL);
    NSLog(@"[INIT] Created game queue: %p", _gameQueue);
    
    // Initialize game on background queue
    NSLog(@"[INIT] Dispatching game initialization to background queue");
    dispatch_async(_gameQueue, ^{
        NSLog(@"[INIT] ========================================");
        NSLog(@"[INIT] BACKGROUND QUEUE: Starting game initialization");
        NSLog(@"[INIT] ========================================");
        
        NSLog(@"[INIT] About to call game_main()");
        NSLog(@"[INIT] game_main function pointer: %p", (void*)game_main);
        // Call game_main to create the game instance
        int result = game_main(0, nullptr);
        NSLog(@"[INIT] game_main() returned: %d", result);
        
        // Force flush the logs to make sure we see them
        fflush(stdout);
        fflush(stderr);
        
        // Get the game instance and initialize it
        Game* game = GetGameInstance();
        NSLog(@"[INIT] GetGameInstance() returned: %p", game);
        
        if (game) {
            NSLog(@"[INIT] game->IsInitialized() = %d", game->IsInitialized());
            
            dispatch_async(dispatch_get_main_queue(), ^{
                NSLog(@"[INIT] ========================================");
                NSLog(@"[INIT] MAIN QUEUE: Setting up rendering pipeline");
                NSLog(@"[INIT] ========================================");
                
                NSLog(@"[INIT] Initializing PlatformLayer with MTKView: %p", _metalView);
                // Initialize the PlatformLayer with the MTKView to set up the delegate
                PlatformLayer& platformLayer = PlatformLayer::GetInstance();
                platformLayer.Initialize((__bridge void*)_metalView);
                NSLog(@"[INIT] PlatformLayer initialized");
                NSLog(@"[INIT] PlatformLayer delegate: %p", platformLayer.GetDelegate());
                
                NSLog(@"[INIT] Setting up display link on main queue");
                // Set up display link on main thread after initialization
                [self setupDisplayLink];
                _gameInitialized = YES;
                
                // Force a small delay to ensure initialization is complete
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    NSLog(@"[INIT] ========================================");
                    NSLog(@"[INIT] DELAYED CHECK: game->IsInitialized() = %d", game->IsInitialized());
                    NSLog(@"[INIT] ========================================");
                });
                NSLog(@"[INIT] ========================================");
                NSLog(@"[INIT] Game initialization COMPLETE");
                NSLog(@"[INIT] _gameInitialized = YES");
                NSLog(@"[INIT] ========================================");
            });
        } else {
            NSLog(@"[ERROR] ========================================");
            NSLog(@"[ERROR] FAILED to get game instance!");
            NSLog(@"[ERROR] game_main() returned: %d", result);
            NSLog(@"[ERROR] GetGameInstance() returned: %p", game);
            NSLog(@"[ERROR] ========================================");
        }
    });
    
    NSLog(@"[INIT] GameViewController viewDidLoad completed (async init started)");
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
    NSLog(@"[DEBUG] gameLoopTick called");
    if (!_gameInitialized) {
        NSLog(@"[DEBUG] Game not initialized, skipping gameLoopTick");
        return;
    }
    
    Game* game = GetGameInstance();
    if (!game) {
        NSLog(@"[DEBUG] Game instance is null, skipping gameLoopTick");
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
    
    // Handle safe area for notched devices
    if (@available(iOS 11.0, *)) {
        UIEdgeInsets safeArea = self.view.safeAreaInsets;
        UpdateSafeAreaInsets(safeArea.top, safeArea.right, safeArea.bottom, safeArea.left);
    }
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle view size changes
    SetWindowSize(size.width, size.height);
}

- (void)drawInMTKView:(MTKView *)view {
    NSLog(@"[DEBUG] drawInMTKView called");
    if (!_gameInitialized) {
        NSLog(@"[DEBUG] Game not initialized, skipping draw");
        return;
    }
    
    Game* game = GetGameInstance();
    if (!game) {
        NSLog(@"[DEBUG] Game instance is null, skipping draw");
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
    // Convert UITouch to game input format
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
    if (_displayLink) {
        [_displayLink invalidate];
        _displayLink = nil;
    }
    
    // Shutdown the game
    Game* game = GetGameInstance();
    if (game) {
        game->Shutdown();
        delete game;
        SetGameInstance(nullptr);
    }
    
    // ARC handles deallocation automatically
}

#pragma mark - Device Orientation

- (BOOL)shouldAutorotate {
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    // Support both landscape orientations and portrait
    return UIInterfaceOrientationMaskAll;
}

@end 