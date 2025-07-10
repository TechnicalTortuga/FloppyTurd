#import "GameView.h"
#import "MetalRenderer.h"
#import "UIManager.h"
#import "RaylibCompat.h"

@implementation GameView {
    MetalRenderer* _metalRenderer;
    BOOL _isInitialized;
    BOOL _touchActive;
    CGPoint _currentTouchPosition;
    CGPoint _touchStartPosition;
    BOOL _touchPressed;
    BOOL _touchReleased;
    CFTimeInterval _touchStartTime;
    CFTimeInterval _lastFrameTime;
    
    // Multi-touch support
    NSMutableArray<UITouch *> *_activeTouches;
    NSMutableDictionary *_touchStartPositions;
    NSMutableDictionary *_touchStartTimes;
    float _initialPinchDistance;
    float _currentPinchDistance;
}

- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device game:(Game*)game {
    self = [super initWithFrame:frame device:device];
    if (self) {
        TraceLog(LOG_INFO, "[INIT] ========================================");
        TraceLog(LOG_INFO, "[INIT] GameView initWithFrame STARTING");
        TraceLog(LOG_INFO, "[INIT] ========================================");
        TraceLog(LOG_INFO, "[INIT] Frame: %@", NSStringFromCGRect(frame));
        TraceLog(LOG_INFO, "[INIT] Device: %p", device);
        TraceLog(LOG_INFO, "[INIT] Game: %p", game);
        
        // Configure MTKView properties
        self.delegate = self;
        self.enableSetNeedsDisplay = YES;
        self.multipleTouchEnabled = YES;
        self.userInteractionEnabled = YES;
        
        // Debug: Test touch configuration
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] GameView touch configuration:");
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] - multipleTouchEnabled: %@", self.multipleTouchEnabled ? @"YES" : @"NO");
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] - userInteractionEnabled: %@", self.userInteractionEnabled ? @"YES" : @"NO");
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] - delegate: %p", self.delegate);
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] - frame: %@", NSStringFromCGRect(self.frame));
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] - bounds: %@", NSStringFromCGRect(self.bounds));
        self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
        self.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0);
        self.preferredFramesPerSecond = 60;
        
        TraceLog(LOG_INFO, "[INIT] MTKView properties configured");
        
        // Store game reference
        _game = game;
        
        // Initialize MetalRenderer
        TraceLog(LOG_INFO, "[INIT] Creating MetalRenderer");
        _metalRenderer = new MetalRenderer();
        if (!_metalRenderer->Initialize(self)) {
            TraceLog(LOG_ERROR, "[ERROR] Failed to initialize MetalRenderer");
            return nil;
        }
        TraceLog(LOG_INFO, "[INIT] MetalRenderer initialized successfully: %p", _metalRenderer);
        
        // Initialize TouchControls with actual screen dimensions (pixels, not points)
        UIScreen* screen = [UIScreen mainScreen];
        CGRect nativeBounds = screen.nativeBounds;
        _touchControls = new TouchControls();
        _touchControls->Initialize(nativeBounds.size.width, nativeBounds.size.height);
        TraceLog(LOG_INFO, "[INIT] TouchControls initialized with screen pixel dimensions: %.0fx%.0f", nativeBounds.size.width, nativeBounds.size.height);
        
        // Initialize multi-touch tracking
        _activeTouches = [[NSMutableArray alloc] init];
        _touchStartPositions = [[NSMutableDictionary alloc] init];
        _touchStartTimes = [[NSMutableDictionary alloc] init];
        _initialPinchDistance = 0.0f;
        _currentPinchDistance = 0.0f;
        
        // Initialize UIManager with current screen metrics
        // Reuse existing screen and nativeBounds variables
        CGFloat nativeScale = screen.nativeScale;
        UIEdgeInsets insets = self.safeAreaInsets;
        CGRect safeAreaPoints = UIEdgeInsetsInsetRect(frame, insets);
        CGRect safeAreaPixels = CGRectMake(safeAreaPoints.origin.x * nativeScale,
                                           safeAreaPoints.origin.y * nativeScale,
                                           safeAreaPoints.size.width * nativeScale,
                                           safeAreaPoints.size.height * nativeScale);
        
        UIManager& uiManager = UIManager::GetInstance();
        uiManager.Initialize((float)frame.size.width, (float)frame.size.height,
                            (float)nativeBounds.size.width, (float)nativeBounds.size.height,
                            { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height },
                            { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height });
        
        TraceLog(LOG_INFO, "[INIT] UIManager initialized with safe area");
        
        _isInitialized = YES;
        TraceLog(LOG_INFO, "[INIT] ========================================");
        TraceLog(LOG_INFO, "[INIT] GameView initialization COMPLETED");
        TraceLog(LOG_INFO, "[INIT] ========================================");
    }
    return self;
}

#pragma mark - Touch Handling

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] touchesBegan called with %lu touches", (unsigned long)touches.count);
    for (UITouch *touch in touches) {
        CGPoint location = [touch locationInView:self];
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] touchesBegan: touch at (%.1f, %.1f)", location.x, location.y);
        
        // Add touch to active touches array
        if (![_activeTouches containsObject:touch]) {
            [_activeTouches addObject:touch];
            NSNumber *touchKey = @((NSUInteger)touch);
            [_touchStartPositions setObject:[NSValue valueWithCGPoint:location] forKey:touchKey];
            [_touchStartTimes setObject:@(CACurrentMediaTime()) forKey:touchKey];
        }
        
        // Update primary touch state for polling (use first touch)
        if (_activeTouches.count == 1) {
            _touchActive = YES;
            _currentTouchPosition = location;
            _touchStartPosition = location;
            _touchPressed = YES;
            _touchReleased = NO;
            _touchStartTime = CACurrentMediaTime();
        }
        
        // Update pinch distance for multi-touch
        [self updatePinchDistance];
        
        // Log view bounds for coordinate context
        CGRect bounds = self.bounds;
        TraceLog(LOG_INFO, "[TOUCH] View bounds: (%.1f, %.1f, %.1f, %.1f)", 
                 bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
        
        // SINGLE SOURCE OF TRUTH: Direct call to TouchControls static method
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] Calling TouchControls::SetTouchState(true, %.1f, %.1f)", location.x, location.y);
        TouchControls::SetTouchState(true, location.x, location.y);
        TraceLog(LOG_INFO, "[TOUCH] TouchControls::SetTouchState called with (%.1f, %.1f, true)", location.x, location.y);
        
        // IMMEDIATE PROCESSING: Update the static buffer right away so the processed state is available
        TouchControls::UpdateStatic();
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] Immediate UpdateStatic() called after SetTouchState");
        
        // MULTI-TOUCH SUPPORT: Update multi-touch buffer for gesture recognition
        int touchIndex = (int)[_activeTouches indexOfObject:touch];
        if (touchIndex < 3) { // Support up to 3 touches
            TouchControls::SetMultiTouchState(touchIndex, true, location.x, location.y);
            TraceLog(LOG_INFO, "[TOUCH] Multi-touch: touch %d began at (%.1f, %.1f)", touchIndex, location.x, location.y);
        }
    }
    [self setNeedsDisplay];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    TraceLog(LOG_INFO, "[TOUCH] touchesMoved called with %lu touches", (unsigned long)touches.count);
    
    for (UITouch *touch in touches) {
        CGPoint location = [touch locationInView:self];
        TraceLog(LOG_INFO, "[TOUCH] touchesMoved: touch at (%.1f, %.1f)", location.x, location.y);
        
        // Update primary touch state for polling (use first touch)
        if (_activeTouches.count > 0 && [_activeTouches indexOfObject:touch] == 0) {
            _currentTouchPosition = location;
        }
        
        // Update pinch distance for multi-touch
        [self updatePinchDistance];
        
        // SINGLE SOURCE OF TRUTH: Direct call to TouchControls static method
        TouchControls::SetTouchState(true, location.x, location.y);
        TraceLog(LOG_INFO, "[TOUCH] TouchControls::SetTouchState called with (%.1f, %.1f, true)", location.x, location.y);
        
        // IMMEDIATE PROCESSING: Update the static buffer right away so the processed state is available
        TouchControls::UpdateStatic();
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] Immediate UpdateStatic() called after SetTouchState (moved)");
        
        // MULTI-TOUCH SUPPORT: Update multi-touch buffer for gesture recognition
        int touchIndex = (int)[_activeTouches indexOfObject:touch];
        if (touchIndex < 3) { // Support up to 3 touches
            TouchControls::SetMultiTouchState(touchIndex, true, location.x, location.y);
            TraceLog(LOG_INFO, "[TOUCH] Multi-touch: touch %d moved to (%.1f, %.1f)", touchIndex, location.x, location.y);
        }
    }
    [self setNeedsDisplay];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] touchesEnded called with %lu touches", (unsigned long)touches.count);
    for (UITouch *touch in touches) {
        CGPoint location = [touch locationInView:self];
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] touchesEnded: touch at (%.1f, %.1f)", location.x, location.y);
        
        // MULTI-TOUCH SUPPORT: Update multi-touch buffer before removing touch
        int touchIndex = (int)[_activeTouches indexOfObject:touch];
        if (touchIndex < 3) { // Support up to 3 touches
            TouchControls::SetMultiTouchState(touchIndex, false, location.x, location.y);
            TraceLog(LOG_INFO, "[TOUCH] Multi-touch: touch %d ended at (%.1f, %.1f)", touchIndex, location.x, location.y);
        }
        
        // Remove touch from active touches array
        NSNumber *touchKey = @((NSUInteger)touch);
        [_activeTouches removeObject:touch];
        [_touchStartPositions removeObjectForKey:touchKey];
        [_touchStartTimes removeObjectForKey:touchKey];
        
        // Update primary touch state for polling (if this was the first touch)
        if (_activeTouches.count == 0) {
            _touchActive = NO;
            _touchPressed = NO;
            _touchReleased = YES;
        }
        
        // Update pinch distance for multi-touch
        [self updatePinchDistance];
        
        // SINGLE SOURCE OF TRUTH: Direct call to TouchControls static method
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] Calling TouchControls::SetTouchState(false, %.1f, %.1f)", location.x, location.y);
        TouchControls::SetTouchState(false, location.x, location.y);
        TraceLog(LOG_INFO, "[TOUCH] TouchControls::SetTouchState called with (%.1f, %.1f, false)", location.x, location.y);
        
        // IMMEDIATE PROCESSING: Update the static buffer right away so the processed state is available
        TouchControls::UpdateStatic();
        TraceLog(LOG_INFO, "[TOUCH_DEBUG] Immediate UpdateStatic() called after SetTouchState (ended)");
    }
    [self setNeedsDisplay];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    TraceLog(LOG_INFO, "[TOUCH] touchesCancelled called with %lu touches", (unsigned long)touches.count);
    
    for (UITouch *touch in touches) {
        // MULTI-TOUCH SUPPORT: Update multi-touch buffer before removing touch
        int touchIndex = (int)[_activeTouches indexOfObject:touch];
        if (touchIndex < 3) { // Support up to 3 touches
            TouchControls::SetMultiTouchState(touchIndex, false, 0.0f, 0.0f);
            TraceLog(LOG_INFO, "[TOUCH] Multi-touch: touch %d cancelled", touchIndex);
        }
        
        // Remove touch from active touches array
        NSNumber *touchKey = @((NSUInteger)touch);
        [_activeTouches removeObject:touch];
        [_touchStartPositions removeObjectForKey:touchKey];
        [_touchStartTimes removeObjectForKey:touchKey];
    }
    
    // Reset all touch states
    _touchActive = NO;
    _touchPressed = NO;
    _touchReleased = NO;
    _currentPinchDistance = 0.0f;
    _initialPinchDistance = 0.0f;
    
    // SINGLE SOURCE OF TRUTH: Cancel all touches - set to false with zero coordinates
    TouchControls::SetTouchState(false, 0.0f, 0.0f);
    TraceLog(LOG_INFO, "[TOUCH] TouchControls::SetTouchState called with (0.0, 0.0, false) - cancelled");
    
    // IMMEDIATE PROCESSING: Update the static buffer right away so the processed state is available
    TouchControls::UpdateStatic();
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] Immediate UpdateStatic() called after SetTouchState (cancelled)");
    
    [self setNeedsDisplay];
}

// Implementation of the missing processTouchAtPoint method
- (void)processTouchAtPoint:(CGPoint)point isDown:(BOOL)isDown {
    TraceLog(LOG_INFO, "[TOUCH] processTouchAtPoint called: (%.1f, %.1f), isDown: %s", 
             point.x, point.y, isDown ? "YES" : "NO");
    
    // SINGLE SOURCE OF TRUTH: Direct call to TouchControls static method
    TouchControls::SetTouchState(isDown, point.x, point.y);
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    TraceLog(LOG_INFO, "[RENDER] Drawable size changed to: %@", NSStringFromCGSize(size));
    
    // Update TouchControls with new dimensions
    if (_touchControls) {
        _touchControls->Initialize(size.width, size.height);
        TraceLog(LOG_INFO, "[RENDER] TouchControls updated with new dimensions: %.0fx%.0f", size.width, size.height);
    }
    
    // Update UIManager with new screen metrics
    UIScreen* screen = [UIScreen mainScreen];
    CGFloat nativeScale = screen.nativeScale;
    CGRect bounds = self.bounds;
    UIEdgeInsets insets = self.safeAreaInsets;
    CGRect safeAreaPoints = UIEdgeInsetsInsetRect(bounds, insets);
    CGRect safeAreaPixels = CGRectMake(safeAreaPoints.origin.x * nativeScale,
                                       safeAreaPoints.origin.y * nativeScale,
                                       safeAreaPoints.size.width * nativeScale,
                                       safeAreaPoints.size.height * nativeScale);
    
    UIManager& uiManager = UIManager::GetInstance();
    uiManager.Initialize((float)bounds.size.width, (float)bounds.size.height,
                        (float)(size.width * nativeScale), (float)(size.height * nativeScale),
                        { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height },
                        { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height });
    
    TraceLog(LOG_INFO, "[RENDER] UIManager updated with new dimensions");
}

- (void)drawInMTKView:(MTKView *)view {
    static int frameCount = 0;
    frameCount++;
    
    if (frameCount == 1 || frameCount % 60 == 0) {
        TraceLog(LOG_INFO, "[RENDER] drawInMTKView called (frame: %d)", frameCount);
    }
    
    if (!_isInitialized || !_metalRenderer) {
        if (frameCount == 1) {
            TraceLog(LOG_ERROR, "[ERROR] drawInMTKView: GameView not properly initialized");
        }
        return;
    }
    
    // Begin Metal frame
    _metalRenderer->BeginFrame();
    _metalRenderer->Clear({25, 25, 25, 255}); // Dark gray background
    
    // Render game if available and initialized
    if (_game && _game->IsInitialized()) {
        if (frameCount == 1 || frameCount % 120 == 0) {
            TraceLog(LOG_INFO, "[RENDER] Rendering game frame (frame: %d)", frameCount);
        }
        
        // Render the game frame
        _game->RenderFrame();
        
        // Draw touch controls overlay (always active for touch input)
        if (_touchControls && _touchControls->IsEnabled()) {
            _touchControls->Draw(0.3f); // Draw with 30% alpha for subtle overlay
        }
    } else {
        if (frameCount == 1 || frameCount % 120 == 0) {
            TraceLog(LOG_WARNING, "[WARN] drawInMTKView: Game not available or not initialized");
        }
    }
    
    // End Metal frame
    _metalRenderer->EndFrame();
    _metalRenderer->Present();
    
    if (frameCount == 1 || frameCount % 60 == 0) {
        TraceLog(LOG_INFO, "[RENDER] drawInMTKView completed (frame: %d)", frameCount);
    }
}

#pragma mark - MetalRenderer Access

- (void*)getMetalRenderer {
    return _metalRenderer;
}

#pragma mark - Touch State Polling

- (BOOL)isTouchActive {
    return _touchActive;
}

- (CGPoint)getCurrentTouchPosition {
    return _currentTouchPosition;
}

- (CGPoint)getTouchStartPosition {
    return _touchStartPosition;
}

- (BOOL)isTouchPressed {
    return _touchPressed;
}

- (BOOL)isTouchReleased {
    return _touchReleased;
}

- (float)getTouchDuration {
    if (!_touchActive) return 0.0f;
    return (float)(CACurrentMediaTime() - _touchStartTime);
}

- (float)getTouchDistance {
    if (!_touchActive) return 0.0f;
    float dx = _currentTouchPosition.x - _touchStartPosition.x;
    float dy = _currentTouchPosition.y - _touchStartPosition.y;
    return sqrtf(dx * dx + dy * dy);
}

- (void)resetFrameTouchStates {
    // Reset frame-specific states that should only last one frame
    _touchPressed = NO;
    _touchReleased = NO;
}

#pragma mark - Multi-touch Support

- (int)getTouchCount {
    return (int)_activeTouches.count;
}

- (CGPoint)getTouchPositionAtIndex:(int)index {
    if (index >= 0 && index < _activeTouches.count) {
        UITouch *touch = _activeTouches[index];
        return [touch locationInView:self];
    }
    return CGPointMake(0, 0);
}

- (BOOL)isTouchActiveAtIndex:(int)index {
    return (index >= 0 && index < _activeTouches.count);
}

- (float)getPinchDistance {
    return _currentPinchDistance;
}

- (float)getPinchScale {
    if (_initialPinchDistance > 0.0f) {
        return _currentPinchDistance / _initialPinchDistance;
    }
    return 1.0f;
}

- (void)updatePinchDistance {
    if (_activeTouches.count >= 2) {
        CGPoint pos1 = [self getTouchPositionAtIndex:0];
        CGPoint pos2 = [self getTouchPositionAtIndex:1];
        
        float dx = pos2.x - pos1.x;
        float dy = pos2.y - pos1.y;
        _currentPinchDistance = sqrtf(dx * dx + dy * dy);
        
        // Set initial distance on first multi-touch
        if (_initialPinchDistance == 0.0f) {
            _initialPinchDistance = _currentPinchDistance;
        }
    } else {
        _currentPinchDistance = 0.0f;
        _initialPinchDistance = 0.0f;
    }
}

- (void)getTouchData:(int*)touchCount 
         positions:(CGPoint*)positions 
         active:(BOOL*)active 
         pinchDistance:(float*)pinchDistance 
         pinchScale:(float*)pinchScale {
    
    if (touchCount) *touchCount = (int)_activeTouches.count;
    if (pinchDistance) *pinchDistance = _currentPinchDistance;
    if (pinchScale) *pinchScale = [self getPinchScale];
    
    // Fill positions and active arrays if provided
    if (positions && active) {
        int maxTouches = MIN((int)_activeTouches.count, 10); // Limit to 10 touches max
        for (int i = 0; i < maxTouches; i++) {
            positions[i] = [self getTouchPositionAtIndex:i];
            active[i] = [self isTouchActiveAtIndex:i];
        }
    }
}

#pragma mark - TouchControls Polling

- (int)getTouchControlsTouchCount {
    if (!_touchControls) return 0;
    return _touchControls->GetTouchCount();
}

- (CGPoint)getTouchControlsTouchPositionAtIndex:(int)index {
    if (!_touchControls) return CGPointMake(0, 0);
    Vector2 pos = _touchControls->GetTouchPosition(index);
    return CGPointMake(pos.x, pos.y);
}

- (BOOL)isTouchControlsTouchActiveAtIndex:(int)index {
    if (!_touchControls) return NO;
    return _touchControls->IsTouchActive(index);
}

- (float)getTouchControlsPinchDistance {
    if (!_touchControls) return 0.0f;
    return _touchControls->GetPinchDistance();
}

- (float)getTouchControlsPinchScale {
    if (!_touchControls) return 1.0f;
    return _touchControls->GetPinchScale();
}

#pragma mark - Safe Area Handling

- (void)updateSafeAreaWithInsets:(UIEdgeInsets)insets {
    UIScreen* screen = [UIScreen mainScreen];
    CGFloat nativeScale = screen.nativeScale;
    CGRect bounds = self.bounds;
    CGRect safeAreaPoints = UIEdgeInsetsInsetRect(bounds, insets);
    CGRect safeAreaPixels = CGRectMake(safeAreaPoints.origin.x * nativeScale,
                                       safeAreaPoints.origin.y * nativeScale,
                                       safeAreaPoints.size.width * nativeScale,
                                       safeAreaPoints.size.height * nativeScale);
    
    UIManager& uiManager = UIManager::GetInstance();
    uiManager.Initialize((float)bounds.size.width, (float)bounds.size.height,
                        (float)(bounds.size.width * nativeScale), (float)(bounds.size.height * nativeScale),
                        { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height },
                        { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height });
    
    TraceLog(LOG_INFO, "[SAFE_AREA] Updated safe area with insets: %@", NSStringFromUIEdgeInsets(insets));
}

#pragma mark - Cleanup

- (void)dealloc {
    TraceLog(LOG_INFO, "[CLEANUP] GameView dealloc");
    
    if (_metalRenderer) {
        _metalRenderer->Shutdown();
        delete _metalRenderer;
        _metalRenderer = nullptr;
    }
    
    if (_touchControls) {
        delete _touchControls;
        _touchControls = nullptr;
    }
}

@end 