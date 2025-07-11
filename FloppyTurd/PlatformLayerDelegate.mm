#import "PlatformLayerDelegate.h"
#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>
#import "MetalRenderer.h"
#import "Game.h"
#import "RaylibCompat.h"
#import "GameViewController.h"

// ColorToUInt is now defined in RaylibCompat.h

@implementation PlatformLayerDelegate {
    MTKView* _view;
    id<MTLDevice> _device;
    BOOL _isInitialized;
    GameViewController* _gameViewController;
}

- (instancetype)initWithView:(MTKView*)view gameViewController:(GameViewController*)gameViewController {
    self = [super init];
    if (self) {
        TraceLog(LOG_INFO, "[INIT] ========================================");
        TraceLog(LOG_INFO, "[INIT] PlatformLayerDelegate initWithView STARTING");
        TraceLog(LOG_INFO, "[INIT] ========================================");
        TraceLog(LOG_INFO, "[INIT] MTKView: %p", view);
        TraceLog(LOG_INFO, "[INIT] MTKView frame: %@", NSStringFromCGRect(view.frame));
        TraceLog(LOG_INFO, "[INIT] MTKView bounds: %@", NSStringFromCGRect(view.bounds));
        TraceLog(LOG_INFO, "[INIT] GameViewController: %p", gameViewController);
        
        _view = view;
        _device = view.device;
        _gameViewController = gameViewController;
        TraceLog(LOG_INFO, "[INIT] Metal device: %p", _device);
        
        if (!_device) {
            TraceLog(LOG_ERROR, "[ERROR] Metal is not supported on this device");
            return nil;
        }
        
        
        TraceLog(LOG_INFO, "[INIT] Setting up MTKView properties");
        view.enableSetNeedsDisplay = YES;
        view.preferredFramesPerSecond = 60;
        view.multipleTouchEnabled = YES;
        view.userInteractionEnabled = YES;
        TraceLog(LOG_INFO, "[INIT] MTKView delegate set to self: %p", view.delegate);
        TraceLog(LOG_INFO, "[INIT] MTKView enableSetNeedsDisplay: %s", view.enableSetNeedsDisplay ? "YES" : "NO");
        TraceLog(LOG_INFO, "[INIT] MTKView preferredFramesPerSecond: %ld", (long)view.preferredFramesPerSecond);
        TraceLog(LOG_INFO, "[INIT] MTKView multipleTouchEnabled: %s", view.multipleTouchEnabled ? "YES" : "NO");
        TraceLog(LOG_INFO, "[INIT] MTKView userInteractionEnabled: %s", view.userInteractionEnabled ? "YES" : "NO");
        
        _isInitialized = YES;
        TraceLog(LOG_INFO, "[INIT] ========================================");
        TraceLog(LOG_INFO, "[INIT] PlatformLayerDelegate initialization COMPLETED");
        TraceLog(LOG_INFO, "[INIT] ========================================");
    }
    return self;
}

- (void)setupMetalPipeline {
    TraceLog(LOG_INFO, "[DEBUG] setupMetalPipeline called");
    if (!_device) {
        TraceLog(LOG_ERROR, "[ERROR] setupMetalPipeline: No Metal device available");
        return;
    }
}

#pragma mark - MTKViewDelegate

// MTKViewDelegate methods removed as GameView handles rendering responsibilities

// MetalRenderer handles all the drawing internally, so we don't need these methods anymore

- (void)dealloc {
}

#pragma mark - Public Methods

- (void)drawRectangleWithPosX:(int)posX posY:(int)posY width:(int)width height:(int)height color:(unsigned int)color {
    TraceLog(LOG_INFO, "[DEBUG] drawRectangleWithPosX ENTRY: posX=%d, posY=%d, width=%d, height=%d, color=0x%08X", posX, posY, width, height, color);
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] drawRectangleWithPosX called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self drawRectangleWithPosX:posX posY:posY width:width height:height color:color];
        });
        return;
    }
    TraceLog(LOG_INFO, "[DEBUG] drawRectangleWithPosX: Functionality moved to GameView");
}

- (void)drawLineEx:(float)x1 y1:(float)y1 x2:(float)x2 y2:(float)y2 thickness:(float)thickness color:(unsigned int)color {
    TraceLog(LOG_INFO, "[DEBUG] drawLineEx ENTRY: (%.1f,%.1f) to (%.1f,%.1f), thickness=%.1f, color=0x%08X", x1, y1, x2, y2, thickness, color);
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] drawLineEx called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self drawLineEx:x1 y1:y1 x2:x2 y2:y2 thickness:thickness color:color];
        });
        return;
    }
    TraceLog(LOG_INFO, "[DEBUG] drawLineEx: Functionality moved to GameView");
}

- (void)drawRectangleRoundedLines:(float)x y:(float)y width:(float)width height:(float)height roundness:(float)roundness segments:(int)segments lineThick:(float)lineThick color:(unsigned int)color {
    TraceLog(LOG_INFO, "[DEBUG] drawRectangleRoundedLines ENTRY: rect=(%.1f,%.1f,%.1f,%.1f), roundness=%.1f, lineThick=%.1f, color=0x%08X", x, y, width, height, roundness, lineThick, color);
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] drawRectangleRoundedLines called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self drawRectangleRoundedLines:x y:y width:width height:height roundness:roundness segments:segments lineThick:lineThick color:color];
        });
        return;
    }
    TraceLog(LOG_INFO, "[DEBUG] drawRectangleRoundedLines: Functionality moved to GameView");
}

- (void)drawText:(const char*)text x:(float)x y:(float)y fontSize:(float)fontSize color:(unsigned int)color font:(void*)font {
    // TraceLog(LOG_INFO, "[DEBUG] drawText ENTRY: text=%s, x=%f, y=%f, fontSize=%f, color=0x%08X, font=%p", text, x, y, fontSize, color, font);
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] drawText called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self drawText:text x:x y:y fontSize:fontSize color:color font:font];
        });
        return;
    }
    Color raylibColor = {
        (unsigned char)((color >> 24) & 0xFF),
        (unsigned char)((color >> 16) & 0xFF),
        (unsigned char)((color >> 8) & 0xFF),
        (unsigned char)(color & 0xFF)
    };
    // TraceLog(LOG_INFO, "[DEBUG] drawText calling MetalRenderer: text=%s, x=%f, y=%f, fontSize=%f, color=(%d,%d,%d,%d), font=%p", text, x, y, fontSize, raylibColor.r, raylibColor.g, raylibColor.b, raylibColor.a, font);
    TraceLog(LOG_INFO, "[DEBUG] drawText: Functionality moved to GameView");
}

- (void)drawTexture:(void*)texture x:(float)x y:(float)y width:(float)width height:(float)height tint:(unsigned int)tint {
    // Ensure we're on the main thread for Metal operations
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] drawTexture called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self drawTexture:texture x:x y:y width:width height:height tint:tint];
        });
        return;
    }
    // Convert unsigned int to Color struct
    Color raylibColor = {
        (unsigned char)((tint >> 24) & 0xFF),
        (unsigned char)((tint >> 16) & 0xFF),
        (unsigned char)((tint >> 8) & 0xFF),
        (unsigned char)(tint & 0xFF)
    };
    TraceLog(LOG_INFO, "[DEBUG] drawTexture: Functionality moved to GameView");
}

- (void*)loadTextureFromImage:(void*)imageData width:(int)width height:(int)height format:(int)format {
    // Ensure we're on the main thread for Metal operations
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] loadTextureFromImage called on non-main thread! Current thread: %@", [NSThread currentThread]);
        __block void* result = nullptr;
        dispatch_sync(dispatch_get_main_queue(), ^{
            result = [self loadTextureFromImage:imageData width:width height:height format:format];
        });
        return result;
    }
    
    TraceLog(LOG_INFO, "[DEBUG] loadTextureFromImage called: imageData=%p, width=%d, height=%d, format=%d", imageData, width, height, format);
    
    if (!imageData || width <= 0 || height <= 0) {
        TraceLog(LOG_ERROR, "[ERROR] loadTextureFromImage: Invalid parameters");
        return nullptr;
    }
    
    // Create texture descriptor
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    // Create texture
    id<MTLTexture> texture = [_device newTextureWithDescriptor:textureDescriptor];
    if (!texture) {
        TraceLog(LOG_ERROR, "[ERROR] loadTextureFromImage: Failed to create Metal texture");
        return nullptr;
    }
    
    TraceLog(LOG_INFO, "[DEBUG] loadTextureFromImage: Created texture: %p (retain count: %lu)", texture, (unsigned long)CFGetRetainCount((__bridge CFTypeRef)texture));
    
    // Upload image data
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    return (__bridge_retained void*)texture;
}

- (void)processDrawCommands:(MTKView*)view {
    // MetalRenderer handles all command processing internally
    // This method is kept for compatibility but delegates to MetalRenderer
    if (g_metalRenderer) {
        TraceLog(LOG_INFO, "[DEBUG] processDrawCommands: Delegating to MetalRenderer");
        // MetalRenderer handles its own command processing during BeginFrame/EndFrame
    } else {
        TraceLog(LOG_ERROR, "[ERROR] processDrawCommands: MetalRenderer not available");
    }
}

- (float)getLastFrameTime {
    TraceLog(LOG_INFO, "[DEBUG] getLastFrameTime: Using global MetalRenderer");
    if (g_metalRenderer) {
        return g_metalRenderer->GetDebugStats().frameTime;
    }
    return 0.0f;
}

- (int)getLastFPS {
    TraceLog(LOG_INFO, "[DEBUG] getLastFPS: Using global MetalRenderer");
    if (g_metalRenderer) {
        float frameTime = g_metalRenderer->GetDebugStats().frameTime;
        if (frameTime > 0.0f) {
            return (int)(1.0f / frameTime + 0.5f);
        }
    }
    return 0;
}

- (id<MTLCommandQueue>)getMetalCommandQueue {
    return _commandQueue;
}

- (void*)getMetalRenderer {
    TraceLog(LOG_INFO, "[DEBUG] getMetalRenderer: Returning global MetalRenderer");
    return g_metalRenderer;
}

#pragma mark - Touch Event Forwarding

// Touch event forwarding removed as GameView handles touch events directly

@end
