#import "GameView.h"
#import "MetalRenderer.h"
#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>

// Forward declarations
class Game;

@interface GameView () <MTKViewDelegate>
{
    MTKView *_mtkView;
    MetalRenderer *_renderer;
    Game *_game;
    BOOL _isInitialized;
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    
    // Touch state tracking
    NSMutableDictionary<NSNumber *, UITouch *> *_activeTouches;
    CGPoint _lastTouchLocation;
    BOOL _isTouching;
    UITouch *_primaryTouch;
}
@end

@implementation GameView

- (instancetype)initWithFrame:(CGRect)frame game:(Game *)game
{
    self = [super initWithFrame:frame];
    if (self) {
        _game = game;
        _isInitialized = NO;
        _activeTouches = [NSMutableDictionary dictionary];
        _isTouching = NO;
        _lastTouchLocation = CGPointZero;
        
        [self setupView];
    }
    return self;
}

- (void)setupView
{
        // Setup MTKView for Metal rendering
        _mtkView = [[MTKView alloc] initWithFrame:self.bounds];
        _mtkView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        _mtkView.delegate = self;
        _mtkView.preferredFramesPerSecond = 60;
        _mtkView.enableSetNeedsDisplay = NO;
        _device = MTLCreateSystemDefaultDevice();
        _mtkView.device = _device;
        
        if (!_device) {
            NSLog(@"Metal is not supported on this device");
            return;
        }
        
        [self addSubview:_mtkView];
        
        // Create command queue
        _commandQueue = [_device newCommandQueue];
        if (!_commandQueue) {
            NSLog(@"Failed to create Metal command queue");
            return;
        }
        NSLog(@"Metal command queue created: %@", _commandQueue);
        
        // Initialize renderer
        _renderer = new MetalRenderer();
        if (!_renderer->Initialize(_mtkView)) {
            NSLog(@"Renderer initialization failed");
            delete _renderer;
            _renderer = nullptr;
            return;
        }
        
        // Configure safe area handling
        self.autoresizesSubviews = YES;
        [self updateSafeArea];
    
    _isInitialized = YES;
    NSLog(@"GameView initialized successfully");
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    [self updateSafeArea];
}

- (void)updateSafeArea
{
    if (@available(iOS 11.0, *)) {
        UIEdgeInsets safeArea = self.safeAreaInsets;
        _mtkView.frame = CGRectMake(safeArea.left, safeArea.top,
                                   self.bounds.size.width - safeArea.left - safeArea.right,
                                   self.bounds.size.height - safeArea.top - safeArea.bottom);
    } else {
        _mtkView.frame = self.bounds;
    }
}

- (BOOL)isInitialized
{
    return _isInitialized;
}

- (void)render
{
    if (_isInitialized) {
        [_mtkView draw];
    }
}

// MARK: - Touch Handling

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];
    
    for (UITouch *touch in touches) {
        NSNumber *touchId = @((uintptr_t)touch);
        _activeTouches[touchId] = touch;
        CGPoint location = [touch locationInView:self];
        _lastTouchLocation = location;
        _isTouching = YES;
        
        // Set primary touch if this is the first touch
        if (!_primaryTouch) {
            _primaryTouch = touch;
        }
        
        // Log touch for debugging
        NSLog(@"Touch began at: %@", NSStringFromCGPoint(location));
        
        // Forward touch info to game logic (will be queried by PlatformLayer/TouchControls)
        [self updateTouchState:touch phase:UITouchPhaseBegan location:location];
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];
    
    for (UITouch *touch in touches) {
        NSNumber *touchId = @((uintptr_t)touch);
        if (_activeTouches[touchId]) {
            CGPoint location = [touch locationInView:self];
            _lastTouchLocation = location;
            
            NSLog(@"Touch moved to: %@", NSStringFromCGPoint(location));
            [self updateTouchState:touch phase:UITouchPhaseMoved location:location];
        }
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    
    for (UITouch *touch in touches) {
        NSNumber *touchId = @((uintptr_t)touch);
        if (_activeTouches[touchId]) {
            CGPoint location = [touch locationInView:self];
            NSLog(@"Touch ended at: %@", NSStringFromCGPoint(location));
            [self updateTouchState:touch phase:UITouchPhaseEnded location:location];
            [_activeTouches removeObjectForKey:touchId];
            
            // Clear primary touch if this was the primary touch
            if (_primaryTouch == touch) {
                _primaryTouch = nil;
            }
        }
    }
    
    if (_activeTouches.count == 0) {
        _isTouching = NO;
        _primaryTouch = nil;
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    
    for (UITouch *touch in touches) {
        NSNumber *touchId = @((uintptr_t)touch);
        if (_activeTouches[touchId]) {
            CGPoint location = [touch locationInView:self];
            NSLog(@"Touch cancelled at: %@", NSStringFromCGPoint(location));
            [self updateTouchState:touch phase:UITouchPhaseCancelled location:location];
            [_activeTouches removeObjectForKey:touchId];
            
            // Clear primary touch if this was the primary touch
            if (_primaryTouch == touch) {
                _primaryTouch = nil;
            }
        }
    }
    
    if (_activeTouches.count == 0) {
        _isTouching = NO;
        _primaryTouch = nil;
    }
}

- (void)updateTouchState:(UITouch *)touch phase:(UITouchPhase)phase location:(CGPoint)location
{
    // Update GlobalStateManager with touch state
    // This connects iOS touch events to the cross-platform input system
    
    // Convert CGPoint to Vector2
    Vector2 touchPos = {(float)location.x, (float)location.y};
    
    // Get GlobalStateManager instance
    auto& globalState = GlobalStateManager::GetInstance();
    
    // Update GlobalStateManager based on touch phase
    switch (phase) {
        case UITouchPhaseBegan:
            globalState.SetPrimaryInputDown(true);
            globalState.SetPrimaryInputPressed(true);
            globalState.SetPrimaryInputReleased(false);
            NSLog(@"Touch began at: %@ - Updated GlobalStateManager", NSStringFromCGPoint(location));
            break;
            
        case UITouchPhaseMoved:
            // Keep primary input down during move
            globalState.SetPrimaryInputDown(true);
            globalState.SetPrimaryInputPressed(false);
            globalState.SetPrimaryInputReleased(false);
            NSLog(@"Touch moved to: %@ - Updated GlobalStateManager", NSStringFromCGPoint(location));
            break;
            
        case UITouchPhaseEnded:
        case UITouchPhaseCancelled:
            globalState.SetPrimaryInputDown(false);
            globalState.SetPrimaryInputPressed(false);
            globalState.SetPrimaryInputReleased(true);
            NSLog(@"Touch ended at: %@ - Updated GlobalStateManager", NSStringFromCGPoint(location));
            break;
            
        default:
            break;
    }
    
    // Update touch points array
    // For now, just update with the current touch position
    // In a more sophisticated implementation, we'd track multiple touches
    std::vector<Vector2> touchPoints = {touchPos};
    globalState.SetTouchPoints(touchPoints);
}

// MARK: - Input State Queries for PlatformLayer/TouchControls

- (BOOL)isPrimaryTouchDown
{
    return _isTouching;
}

- (CGPoint)getPrimaryTouchLocation
{
    return _lastTouchLocation;
}

- (NSInteger)getActiveTouchCount
{
    return _activeTouches.count;
}

// MARK: - MTKViewDelegate Methods

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    // Update projection matrix when view size changes
    if (_renderer) {
        _renderer->SetProjectionMatrix(size.width, size.height);
    }
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    // This is called by MTKView when it's time to render
    // Delegate to MetalRenderer or trigger game render loop
    if (_isInitialized && _renderer) {
        _renderer->BeginFrame();
        // The game will call render methods here
        _renderer->EndFrame();
        _renderer->Present();
    }
}
// Implementation of missing methods from GameView.h
- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device {
    self = [super initWithFrame:frame device:device];
    if (self) {
        [self setupView];
    }
    return self;
}

- (void)renderFrame {
    // Implementation to render a frame
    [self setNeedsDisplay];
}

- (Vector2)getPrimaryTouchPosition {
    // Return the position of the primary touch
    if (_primaryTouch) {
        CGPoint location = [_primaryTouch locationInView:self];
        Vector2 result;
        result.x = (float)location.x;
        result.y = (float)location.y;
        return result;
    }
    Vector2 result = {0.0f, 0.0f};
    return result;
}

- (BOOL)isPrimaryTouchPressed {
    // Return whether the primary touch is pressed
    return _primaryTouch != nil && _primaryTouch.phase == UITouchPhaseBegan;
}

- (BOOL)isPrimaryTouchReleased {
    // Return whether the primary touch is released
    return _primaryTouch != nil && _primaryTouch.phase == UITouchPhaseEnded;
}

- (std::vector<Vector2>)getTouchPoints {
    // Return a vector of touch points
    std::vector<Vector2> points;
    for (UITouch *touch in [_activeTouches allValues]) {
        CGPoint location = [touch locationInView:self];
        Vector2 point;
        point.x = (float)location.x;
        point.y = (float)location.y;
        points.push_back(point);
    }
    return points;
}

- (MetalRenderer *)getMetalRenderer {
    // Return the MetalRenderer instance
    return _renderer;
}

- (id<MTLDevice>)getMetalDevice {
    // Return the Metal device
    return _device;
}

- (id<MTLCommandQueue>)getMetalCommandQueue {
    // Return the Metal command queue
    return _commandQueue;
}

// MARK: - Debug Overlay (to be implemented for testing)

- (void)setupDebugOverlay
{
    // Placeholder for debug overlay to visualize touch zones
    // Will be implemented after core functionality
    NSLog(@"Debug overlay setup pending implementation");
}

- (void)dealloc
{
    // Clean up Metal resources
    _device = nil;
    _commandQueue = nil;
    
    // Clean up C++ renderer
    if (_renderer) {
        _renderer->Shutdown();
        delete _renderer;
        _renderer = nullptr;
    }
    
    _mtkView = nil;
    _primaryTouch = nil;
    [_activeTouches removeAllObjects];
    _activeTouches = nil;
    
    [super dealloc];
}

@end