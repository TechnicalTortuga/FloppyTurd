#pragma once
#import <MetalKit/MetalKit.h>
#include "Game.h"
#include "TouchControls.h"

@interface GameView : MTKView <MTKViewDelegate>

@property (nonatomic, assign) Game* game;
@property (nonatomic, assign) TouchControls* touchControls;

- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device game:(Game*)game;

// Touch handling methods
- (void)processTouchAtPoint:(CGPoint)point isDown:(BOOL)isDown;

// Safe area handling
- (void)updateSafeAreaWithInsets:(UIEdgeInsets)insets;

// MetalRenderer access
- (void*)getMetalRenderer;

// Touch state polling for game loop
- (BOOL)isTouchActive;
- (CGPoint)getCurrentTouchPosition;
- (CGPoint)getTouchStartPosition;
- (BOOL)isTouchPressed;
- (BOOL)isTouchReleased;
- (float)getTouchDuration;
- (float)getTouchDistance;

// Reset frame-specific touch states (called from game loop)
- (void)resetFrameTouchStates;

// Multi-touch support
- (int)getTouchCount;
- (CGPoint)getTouchPositionAtIndex:(int)index;
- (BOOL)isTouchActiveAtIndex:(int)index;
- (float)getPinchDistance;
- (float)getPinchScale;

// Get all touch data for game loop processing
- (void)getTouchData:(int*)touchCount 
         positions:(CGPoint*)positions 
         active:(BOOL*)active 
         pinchDistance:(float*)pinchDistance 
         pinchScale:(float*)pinchScale;

// Forward multi-touch data to TouchControls
- (void)updateTouchControlsWithMultiTouch;

// Poll multi-touch data from TouchControls for game loop
- (int)getTouchControlsTouchCount;
- (CGPoint)getTouchControlsTouchPositionAtIndex:(int)index;
- (BOOL)isTouchControlsTouchActiveAtIndex:(int)index;
- (float)getTouchControlsPinchDistance;
- (float)getTouchControlsPinchScale;

@end 