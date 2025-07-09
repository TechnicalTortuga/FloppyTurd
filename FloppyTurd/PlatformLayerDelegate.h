#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@class GameViewController;

@interface PlatformLayerDelegate : NSObject <MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) MTKView* view;
@property (nonatomic, assign) BOOL isInitialized;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLLibrary> library;
@property (nonatomic, strong) NSMutableArray* drawCommands;

- (instancetype)initWithView:(MTKView*)view gameViewController:(GameViewController*)gameViewController;
- (void)drawRectangleWithPosX:(int)posX posY:(int)posY width:(int)width height:(int)height color:(unsigned int)color;
- (void)drawLineEx:(float)x1 y1:(float)y1 x2:(float)x2 y2:(float)y2 thickness:(float)thickness color:(unsigned int)color;
- (void)drawRectangleRoundedLines:(float)x y:(float)y width:(float)width height:(float)height roundness:(float)roundness segments:(int)segments lineThick:(float)lineThick color:(unsigned int)color;
- (void)drawText:(const char*)text x:(float)x y:(float)y fontSize:(float)fontSize color:(unsigned int)color font:(void*)font;
- (void)drawTexture:(void*)texture x:(float)x y:(float)y width:(float)width height:(float)height tint:(unsigned int)tint;
- (void*)loadTextureFromImage:(void*)imageData width:(int)width height:(int)height format:(int)format;
- (void)processDrawCommands:(MTKView*)view;
- (float)getLastFrameTime;
- (int)getLastFPS;
- (id<MTLCommandQueue>)getMetalCommandQueue;
- (void*)getMetalRenderer; // Get the MetalRenderer instance
// Add methods for rendering, texture management, etc., as needed
@end 