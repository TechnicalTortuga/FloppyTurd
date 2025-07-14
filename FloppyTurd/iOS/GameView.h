#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#import <MetalKit/MetalKit.h>
#import "PlatformAPI.h"

// Forward declarations
class MetalRenderer;

@interface GameView : MTKView <MTKViewDelegate>

// Initialization
- (instancetype)initWithFrame:(CGRect)frame;
- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device;

// Rendering
- (void)renderFrame; // Called by Game::RenderFrame

// Input querying (for Game/TouchControls)
- (Vector2)getPrimaryTouchPosition;
- (BOOL)isPrimaryTouchDown;
- (BOOL)isPrimaryTouchPressed;
- (BOOL)isPrimaryTouchReleased;
- (std::vector<Vector2>)getTouchPoints;

// Internal MetalRenderer access (if needed)
- (MetalRenderer *)getMetalRenderer;

// Metal device and command queue access
- (id<MTLDevice>)getMetalDevice;
- (id<MTLCommandQueue>)getMetalCommandQueue;

@end

#endif // GAME_VIEW_H