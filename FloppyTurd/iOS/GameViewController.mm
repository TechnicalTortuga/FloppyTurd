#import "GameViewController.h"
#import <Metal/Metal.h>
#import "MetalRaylibCompat.h"

// Include our C++ game headers
extern "C" int game_main(int argc, char *argv[]);

@interface GameViewController () {
    MTKView *_metalView;
    dispatch_queue_t _gameQueue;
    BOOL _gameInitialized;
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
    
    self.view = _metalView;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Setup touch handling
    _metalView.multipleTouchEnabled = YES;
    
    // Create game queue
    _gameQueue = dispatch_queue_create("com.floppyturd.gamequeue", DISPATCH_QUEUE_SERIAL);
    
    // Initialize game on background queue
    dispatch_async(_gameQueue, ^{
        // Set up any iOS-specific paths or resources here
        
        // Call the C++ game main
        game_main(0, nullptr);
        
        _gameInitialized = YES;
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
    // This is called at the preferred FPS rate
    // The actual Metal rendering will be handled by MetalRenderer
    // triggered from the game loop
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

#pragma mark - Device Orientation

- (BOOL)shouldAutorotate {
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    // Support both landscape orientations and portrait
    return UIInterfaceOrientationMaskAll;
}

@end 