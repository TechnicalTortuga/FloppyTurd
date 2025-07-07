#import "PlatformLayerDelegate.h"
#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>
#import "MetalRenderer.h"
#import "Game.h"
#import "RaylibCompat.h"

// Helper to convert Color to unsigned int (RGBA)
static inline unsigned int ColorToUInt(Color c) {
    return ((unsigned int)c.r << 24) | ((unsigned int)c.g << 16) | ((unsigned int)c.b << 8) | ((unsigned int)c.a);
}

@implementation PlatformLayerDelegate {
    MTKView* _view;
    id<MTLDevice> _device;
    MetalRenderer* _metalRenderer;
    BOOL _isInitialized;
}

- (instancetype)initWithView:(MTKView*)view {
    NSLog(@"[DEBUG] PlatformLayerDelegate initWithView called");
    self = [super init];
    if (self) {
        _view = view;
        _device = view.device;
        NSLog(@"[DEBUG] Metal device: %@", _device ? @"available" : @"nil");
        
        if (!_device) {
            NSLog(@"[ERROR] Metal is not supported on this device");
            return nil;
        }
        
        // Initialize our optimized MetalRenderer
        _metalRenderer = new MetalRenderer();
        if (!_metalRenderer->Initialize(view)) {
            NSLog(@"[ERROR] Failed to initialize MetalRenderer");
            return nil;
        }
        
        NSLog(@"[DEBUG] Setting up MTKView properties");
        _view.device = _device;
        // Don't set self as delegate - GameViewController will be the delegate
        _view.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0);
        _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        
        NSLog(@"[DEBUG] MetalRenderer setup complete");
        _isInitialized = YES;
        NSLog(@"[DEBUG] PlatformLayerDelegate initialization complete");
    }
    return self;
}

- (void)setupMetalPipeline {
    NSLog(@"[DEBUG] setupMetalPipeline called");
    if (!_device) {
        NSLog(@"[ERROR] setupMetalPipeline: No Metal device available");
        return;
    }

    // MetalRenderer handles all pipeline setup internally
    if (_metalRenderer) {
        NSLog(@"[DEBUG] MetalRenderer pipeline already initialized");
    } else {
        NSLog(@"[ERROR] setupMetalPipeline: MetalRenderer not initialized");
    }
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    NSLog(@"[DEBUG] Drawable size changed to: %@", NSStringFromCGSize(size));
}

- (void)drawInMTKView:(MTKView*)view {
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {
        NSLog(@"[RENDER] drawInMTKView called (frame: %d)", frameCount);
    }
    
    if (!_isInitialized) {
        if (frameCount == 1) {
            NSLog(@"[ERROR] drawInMTKView: PlatformLayerDelegate not initialized");
        }
        return;
    }
    
    if (!_metalRenderer) {
        if (frameCount == 1) {
            NSLog(@"[ERROR] drawInMTKView: MetalRenderer not available");
        }
        return;
    }
    
    // Get the game instance
    Game* game = GetGameInstance();
    NSLog(@"[ACCESS] GetGameInstance() called from drawInMTKView, returning: %p", game);
    if (!game) {
        if (frameCount == 1 || frameCount % 120 == 0) {
            NSLog(@"[ERROR] drawInMTKView: Game instance is null");
        }
    } else if (!game->IsInitialized()) {
        if (frameCount == 1 || frameCount % 120 == 0) {
            NSLog(@"[WARN] drawInMTKView: Game instance exists but not initialized");
        }
    } else {
        if (frameCount == 1 || frameCount % 120 == 0) {
            NSLog(@"[RENDER] Rendering game frame (frame: %d)", frameCount);
        }
        // Render the game frame to generate draw commands
        game->RenderFrame();
    }
    
    // Use our optimized MetalRenderer for rendering
    _metalRenderer->BeginFrame();
    _metalRenderer->Clear({25, 25, 25, 255}); // Dark gray background
    
    // Process any pending draw commands (if needed)
    // The MetalRenderer handles all the optimized rendering internally
    
    _metalRenderer->EndFrame();
    _metalRenderer->Present();
    
    if (frameCount == 1 || frameCount % 120 == 0) {
        NSLog(@"[RENDER] drawInMTKView completed (frame: %d)", frameCount);
    }
}

// MetalRenderer handles all the drawing internally, so we don't need these methods anymore

- (void)dealloc {
    if (_metalRenderer) {
        _metalRenderer->Shutdown();
        delete _metalRenderer;
        _metalRenderer = nullptr;
    }
}

#pragma mark - Public Methods

- (void)drawRectangleWithPosX:(int)posX posY:(int)posY width:(int)width height:(int)height color:(unsigned int)color {
    if (![NSThread isMainThread]) {
        NSLog(@"[ERROR] drawRectangleWithPosX called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self drawRectangleWithPosX:posX posY:posY width:width height:height color:color];
        });
        return;
    }
    
    if (!_metalRenderer) {
        NSLog(@"[ERROR] drawRectangleWithPosX: MetalRenderer not available");
        return;
    }
    
    NSLog(@"[DEBUG] drawRectangleWithPosX called: posX=%d, posY=%d, width=%d, height=%d, color=0x%08X", posX, posY, width, height, color);
    
    // Convert color from RGBA to Color struct
    Color raylibColor = {
        (unsigned char)((color >> 24) & 0xFF),
        (unsigned char)((color >> 16) & 0xFF),
        (unsigned char)((color >> 8) & 0xFF),
        (unsigned char)(color & 0xFF)
    };
    
    // Use our optimized MetalRenderer
    _metalRenderer->DrawRectangle(posX, posY, width, height, raylibColor);
}

- (void)drawText:(const char*)text x:(float)x y:(float)y fontSize:(float)fontSize color:(unsigned int)color font:(void*)font {
    // Ensure we're on the main thread for Metal operations
    if (![NSThread isMainThread]) {
        NSLog(@"[ERROR] drawText called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self drawText:text x:x y:y fontSize:fontSize color:color font:font];
        });
        return;
    }
    NSLog(@"[DEBUG] drawText called: text=%s, x=%f, y=%f, fontSize=%f", text, x, y, fontSize);
    // Convert unsigned int to Color struct
    Color raylibColor = {
        (unsigned char)((color >> 24) & 0xFF),
        (unsigned char)((color >> 16) & 0xFF),
        (unsigned char)((color >> 8) & 0xFF),
        (unsigned char)(color & 0xFF)
    };
    NSDictionary* command = @{
        @"type": @"text",
        @"text": [NSString stringWithUTF8String:text],
        @"x": @(x),
        @"y": @(y),
        @"fontSize": @(fontSize),
        @"color": @(color),
        @"font": @((uintptr_t)font)
    };
    [_drawCommands addObject:command];
}

- (void)drawTexture:(void*)texture x:(float)x y:(float)y width:(float)width height:(float)height tint:(unsigned int)tint {
    // Ensure we're on the main thread for Metal operations
    if (![NSThread isMainThread]) {
        NSLog(@"[ERROR] drawTexture called on non-main thread! Current thread: %@", [NSThread currentThread]);
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
    if (!_metalRenderer) {
        NSLog(@"[ERROR] drawTexture: MetalRenderer not available");
        return;
    }
    
    NSLog(@"[DEBUG] drawTexture called: texture=%p, x=%f, y=%f, width=%f, height=%f", texture, x, y, width, height);
    
    // Convert void* texture to MTLTexture
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture;
    if (!metalTexture) {
        NSLog(@"[ERROR] drawTexture: Invalid texture pointer");
        return;
    }
    
    // Create source and destination rectangles
    Rectangle source = {0, 0, (float)metalTexture.width, (float)metalTexture.height};
    Rectangle dest = {x, y, width, height};
    
    // Use our optimized MetalRenderer
    _metalRenderer->DrawTexture(metalTexture, source, dest, raylibColor);
}

- (void*)loadTextureFromImage:(void*)imageData width:(int)width height:(int)height format:(int)format {
    // Ensure we're on the main thread for Metal operations
    if (![NSThread isMainThread]) {
        NSLog(@"[ERROR] loadTextureFromImage called on non-main thread! Current thread: %@", [NSThread currentThread]);
        __block void* result = nullptr;
        dispatch_sync(dispatch_get_main_queue(), ^{
            result = [self loadTextureFromImage:imageData width:width height:height format:format];
        });
        return result;
    }
    
    NSLog(@"[DEBUG] loadTextureFromImage called: imageData=%p, width=%d, height=%d, format=%d", imageData, width, height, format);
    
    if (!imageData || width <= 0 || height <= 0) {
        NSLog(@"[ERROR] loadTextureFromImage: Invalid parameters");
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
        NSLog(@"[ERROR] loadTextureFromImage: Failed to create Metal texture");
        return nullptr;
    }
    
    NSLog(@"[DEBUG] loadTextureFromImage: Created texture: %p (retain count: %lu)", texture, (unsigned long)CFGetRetainCount((__bridge CFTypeRef)texture));
    
    // Upload image data
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    return (__bridge_retained void*)texture;
}

- (void)processDrawCommands:(MTKView*)view {
    // MetalRenderer handles all command processing internally
    // This method is kept for compatibility but delegates to MetalRenderer
    if (_metalRenderer) {
        NSLog(@"[DEBUG] processDrawCommands: Delegating to MetalRenderer");
        // MetalRenderer handles its own command processing during BeginFrame/EndFrame
    } else {
        NSLog(@"[ERROR] processDrawCommands: MetalRenderer not available");
    }
}

@end
