#import "GameView.h"
#import "MetalRenderer.h"
#import "Game.h"
#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>

@interface GameView ()
{
    MetalRenderer *_renderer;
    BOOL _isInitialized;
    id<MTLCommandQueue> _commandQueue;
    
    // Touch state tracking
    NSMutableDictionary<NSNumber *, UITouch *> *_activeTouches;
    CGPoint _lastTouchLocation;
    BOOL _isTouching;
    UITouch *_primaryTouch;
}


@end



@implementation GameView

- (id<MTLDevice>)getMetalDevice
{
    return self.device;
}

- (id<MTLCommandQueue>)getMetalCommandQueue
{
    return _commandQueue;
}

// Override device property to trigger Metal setup when device is assigned
- (void)setDevice:(id<MTLDevice>)device {
    [super setDevice:device];
    TraceLog(LOG_INFO, "[GameView] Device set to: %p", device);
    
    if (device && !_isInitialized) {
        TraceLog(LOG_INFO, "[GameView] Device assigned, calling setupView");
        [self setupView];
    }
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        _isInitialized = NO;
        _activeTouches = [NSMutableDictionary dictionary];
        _isTouching = NO;
        _lastTouchLocation = CGPointZero;
        // Note: setupView will be called when device is set
        TraceLog(LOG_INFO, "[GameView] initWithFrame: basic initialization complete, device will be set later");
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device
{
    self = [super initWithFrame:frame device:device];
    if (self) {
        _isInitialized = NO;
        _activeTouches = [NSMutableDictionary dictionary];
        _isTouching = NO;
        _lastTouchLocation = CGPointZero;
        [self setupView];
    }
    return self;
}

- (void)dealloc
{
    TraceLog(LOG_INFO, "[GameView] Deallocating...");
    
    // Unregister from notifications
    [self unregisterFromAppStateNotifications];
    
    // Pause rendering and release resources
    self.paused = YES;
    
    // Clean up renderer
    if (_renderer) {
        _renderer->Shutdown();
        delete _renderer;
        _renderer = nullptr;
        g_metalRenderer = nullptr;
    }
    
    // Clean up touch tracking
    [_activeTouches removeAllObjects];
    _activeTouches = nil;
    _primaryTouch = nil;
    
    TraceLog(LOG_INFO, "[GameView] Deallocated successfully");
}

- (void)setupView
{
    TraceLog(LOG_INFO, "[GameView] setupView START - Configuring MTKView");
    
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    TraceLog(LOG_INFO, "[GameView] Set autoresizingMask");
    
    self.autoresizesSubviews = YES;
    TraceLog(LOG_INFO, "[GameView] Set autoresizesSubviews");
    
    self.delegate = self;
    TraceLog(LOG_INFO, "[GameView] Set delegate");
    
    // self.device is set by the MTKView's initializer or GameViewController
    TraceLog(LOG_INFO, "[GameView] Device property should be set externally");
    
    self.framebufferOnly = NO;
    TraceLog(LOG_INFO, "[GameView] Set framebufferOnly");
    
    self.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    TraceLog(LOG_INFO, "[GameView] Set depthStencilPixelFormat");
    
    self.sampleCount = 4;
    TraceLog(LOG_INFO, "[GameView] Set sampleCount");
    
    self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    TraceLog(LOG_INFO, "[GameView] Set clearColor");
    
    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    TraceLog(LOG_INFO, "[GameView] Set colorPixelFormat");
    
    self.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    TraceLog(LOG_INFO, "[GameView] Set depthStencilPixelFormat");
    
    self.preferredFramesPerSecond = 60;
    TraceLog(LOG_INFO, "[GameView] Set preferredFramesPerSecond");
    
    self.autoResizeDrawable = YES;
    TraceLog(LOG_INFO, "[GameView] Set autoResizeDrawable");
    
    self.enableSetNeedsDisplay = NO;
    TraceLog(LOG_INFO, "[GameView] Set enableSetNeedsDisplay");
    
    self.paused = NO;
    TraceLog(LOG_INFO, "[GameView] Set paused");
    
    [self setupMetalResources];
    TraceLog(LOG_INFO, "[GameView] Called setupMetalResources");

    TraceLog(LOG_INFO, "[GameView] setupView COMPLETE - Metal resources initialized: %@", _isInitialized ? @"YES" : @"NO");
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    // Safe area is handled automatically by the MTKView
    
    // Initialize Metal resources if not already done
    if (!_isInitialized) {
        TraceLog(LOG_INFO, "[GameView] layoutSubviews: Attempting Metal setup");
        [self setupMetalResources];
    }
}

- (void)setupMetalResources
{
    @autoreleasepool {  // Add autorelease pool for resource management
        TraceLog(LOG_INFO, "[GameView] setupMetalResources START...");
        TraceLog(LOG_INFO, "[GameView] setupMetalResources: After first log");
        if (_isInitialized) {
            TraceLog(LOG_INFO, "[GameView] setupMetalResources: _isInitialized is %@", _isInitialized ? @"YES" : @"NO");
            return;
        }
        @try {
            if (!self.device) {
                TraceLog(LOG_ERROR, "[GameView] No Metal device provided");
                return;
            }
            TraceLog(LOG_INFO, "[GameView] Using self.device: %p", self.device);
             
             _commandQueue = [self.device newCommandQueueWithMaxCommandBufferCount:2];
            if (!_commandQueue) {
                TraceLog(LOG_ERROR, "[GameView] Failed to create command queue");
                return;
            }
            TraceLog(LOG_INFO, "[GameView] Created command queue: %p", _commandQueue);
            
            _renderer = new MetalRenderer();
            if (!_renderer) {
                TraceLog(LOG_ERROR, "[GameView] Failed to create MetalRenderer");
                return;
            }
            TraceLog(LOG_INFO, "[GameView] Created MetalRenderer at address: %p", (void*)_renderer);
            
            if (!_renderer->Initialize(self)) {
                TraceLog(LOG_ERROR, "[GameView] Failed to initialize MetalRenderer");
                delete _renderer;
                _renderer = nullptr;
                return;
            }
            
            // Set global renderer reference for IOSTraits to use
            g_metalRenderer = _renderer;
            TraceLog(LOG_INFO, "[GameView] Set global g_metalRenderer pointer: %p", (void*)g_metalRenderer);
            
            _isInitialized = YES;
            TraceLog(LOG_INFO, "[GameView] Metal resources initialized successfully");
        } @catch (NSException *exception) {
            TraceLog(LOG_ERROR, "Exception in setupMetalResources: %@", exception);
        }
    }
    TraceLog(LOG_INFO, "[GameView] setupMetalResources COMPLETE");
}

- (void)registerForAppStateNotifications
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    
    // App lifecycle notifications
    [center addObserver:self
              selector:@selector(applicationWillResignActive:)
                  name:UIApplicationWillResignActiveNotification
                object:nil];
                
    [center addObserver:self
              selector:@selector(applicationDidBecomeActive:)
                  name:UIApplicationDidBecomeActiveNotification
                object:nil];
                
    [center addObserver:self
              selector:@selector(applicationDidEnterBackground:)
                  name:UIApplicationDidEnterBackgroundNotification
                object:nil];
                
    [center addObserver:self
              selector:@selector(applicationWillEnterForeground:)
                  name:UIApplicationWillEnterForegroundNotification
                object:nil];
    
    // Memory warning notification
    [center addObserver:self
              selector:@selector(didReceiveMemoryWarning)
                  name:UIApplicationDidReceiveMemoryWarningNotification
                object:nil];
    
    TraceLog(LOG_INFO, "[GameView] Registered for app state notifications");
}

- (void)unregisterFromAppStateNotifications
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
    TraceLog(LOG_INFO, "[GameView] Application will resign active");
    
    // Pause the view's drawing loop
    self.paused = YES;
    
    // Notify the renderer to pause rendering
    if (_renderer) {
        _renderer->PauseRendering();
        TraceLog(LOG_INFO, "[GameView] Renderer paused");
    }
    
    // Flush any pending GPU work
    if (_commandQueue) {
        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
    TraceLog(LOG_INFO, "[GameView] Application did become active");
    
    // Resume the view's drawing loop
    self.paused = NO;
    
    // Notify the renderer to resume rendering
    if (_renderer) {
        _renderer->ResumeRendering();
        TraceLog(LOG_INFO, "[GameView] Renderer resumed");
    }
    
    // Force a redraw
    [self setNeedsDisplay];
}

- (void)applicationDidEnterBackground:(NSNotification *)notification
{
    TraceLog(LOG_INFO, "[GameView] Application did enter background");
    
    // Pause the view's drawing loop
    self.paused = YES;
    
    // Notify the renderer to release resources
    if (_renderer) {
        _renderer->PauseRendering();
        TraceLog(LOG_INFO, "[GameView] Renderer paused for background");
    }
    
    // Flush any pending GPU work
    if (_commandQueue) {
        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }
}

- (void)applicationWillEnterForeground:(NSNotification *)notification
{
    TraceLog(LOG_INFO, "[GameView] Application will enter foreground");
    
    // Ensure Metal resources are recreated if needed
    if (_renderer) {
        _renderer->ResumeRendering();
        TraceLog(LOG_INFO, "[GameView] Renderer resumed from background");
    }
    
    // Resume the view's drawing loop
    self.paused = NO;
    
    // Force a redraw
    [self setNeedsDisplay];
}





- (BOOL)isInitialized
{
    return _isInitialized;
}

- (void)render
{
    if (_isInitialized) {
        [self draw];
    }
}

- (void)renderFrame {
    // Skip if not ready to render
    if (!_isInitialized || !_renderer) {
        TraceLog(LOG_WARNING, "[GameView] renderFrame: Not initialized or no renderer");
        return;
    }
    
    // Get the game instance
    Game* game = GetGameInstance();
    if (!game) {
        TraceLog(LOG_WARNING, "[GameView] renderFrame: No game instance");
        return;
    }
    
    @try {
        // Call the game's render frame method
        game->RenderFrame();
        
        // Force a redraw if we're not already drawing
        if (self.paused) {
            [self draw];
        }
    } @catch (NSException *exception) {
        TraceLog(LOG_ERROR, "[GameView] Exception in renderFrame: %@", exception);
    }
}

- (void)forceRender {
    // Force an immediate render by calling drawInMTKView directly
    Game* game = GetGameInstance();
    if (_isInitialized && _renderer && game) {
        TraceLog(LOG_INFO, "[GameView] forceRender: Forcing immediate render");
        [self drawInMTKView:self];
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
        TraceLog(LOG_INFO, "[GameView] Touch began at: (%.1f, %.1f)", location.x, location.y);
        
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
            
            TraceLog(LOG_INFO, "[GameView] Touch moved to: (%.1f, %.1f)", location.x, location.y);
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
            TraceLog(LOG_INFO, "[GameView] Touch ended at: (%.1f, %.1f)", location.x, location.y);
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
            TraceLog(LOG_INFO, "[GameView] Touch cancelled at: (%.1f, %.1f)", location.x, location.y);
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
            TraceLog(LOG_INFO, "[GameView] Touch began at: (%.1f, %.1f) - Updated GlobalStateManager", location.x, location.y);
            break;
            
        case UITouchPhaseMoved:
            // Keep primary input down during move
            globalState.SetPrimaryInputDown(true);
            globalState.SetPrimaryInputPressed(false);
            globalState.SetPrimaryInputReleased(false);
            TraceLog(LOG_INFO, "[GameView] Touch moved to: (%.1f, %.1f) - Updated GlobalStateManager", location.x, location.y);
            break;
            
        case UITouchPhaseEnded:
        case UITouchPhaseCancelled:
            globalState.SetPrimaryInputDown(false);
            globalState.SetPrimaryInputPressed(false);
            globalState.SetPrimaryInputReleased(true);
            TraceLog(LOG_INFO, "[GameView] Touch ended at: (%.1f, %.1f) - Updated GlobalStateManager", location.x, location.y);
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

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
    TraceLog(LOG_INFO, "[GameView] Drawable size will change to: %.0fx%.0f", size.width, size.height);
    
    // Update the renderer with the new size
    if (_renderer) {
        // Flush any pending work before resizing
        _renderer->FlushBatch();
        
        // Notify the renderer about the size change
        _renderer->SetViewportSize(size.width, size.height);
        
        // Recreate resources if needed
        _renderer->RecreateResources();
        
        TraceLog(LOG_INFO, "[GameView] Updated renderer for new drawable size");
    }
    
    // Force a redraw
    [self setNeedsDisplay];
}

- (void)drawInMTKView:(MTKView *)view
{
    @autoreleasepool {  // Add autorelease pool as per best practices
        // This is called by MTKView when it's time to render
        // Delegate to MetalRenderer or trigger game render loop
        Game* game = GetGameInstance();
        if (_isInitialized && _renderer && game) {
            TraceLog(LOG_INFO, "[GameView] drawInMTKView: Starting frame render");
            
            _renderer->BeginFrame();
            
            // Call the game's render frame method - this is the missing piece!
            game->RenderFrame();
            
            _renderer->EndFrame();
            _renderer->Present();
            
            TraceLog(LOG_INFO, "[GameView] drawInMTKView: Frame render completed");
        } else {
            TraceLog(LOG_WARNING, "[GameView] drawInMTKView: Not ready to render - initialized=%d, renderer=%p, game=%p", 
                     _isInitialized, _renderer, game);
        }
    }
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



// MARK: - Debug Overlay (to be implemented for testing)

- (void)setupDebugOverlay
{
    // Placeholder for debug overlay to visualize touch zones
    // Will be implemented after core functionality
    TraceLog(LOG_INFO, "[GameView] Debug overlay setup pending implementation");
}



@end