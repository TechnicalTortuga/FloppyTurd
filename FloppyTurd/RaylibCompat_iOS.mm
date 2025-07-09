#include "RaylibCompat.h"
#ifdef PLATFORM_IOS
#include <Metal/Metal.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include "MetalTextureCache.h"
#include "PlatformLayer.h"
#include <CoreGraphics/CoreGraphics.h>
#include "Game.h"
#import <AVFoundation/AVFoundation.h>
#include "GameLog.h"
#include "MetalTextRenderer.h"
#include "ResourceManager.h"
#include "LogManager.h"
#include <string>
#import "AudioStateManager.h"
#include "MetalRenderer.h"
#import "PlatformLayerDelegate.h"

// Global game instance
Game* g_gameInstance = nullptr;

// Game instance accessor function
Game* GetGameInstance()
{
    NSLog(@"[ACCESS] GetGameInstance() called from thread: %@, returning: %p", [NSThread currentThread], g_gameInstance);
    return g_gameInstance;
}

void SetGameInstance(Game* instance)
{
    NSLog(@"[ACCESS] SetGameInstance() called from thread: %@, with instance: %p", [NSThread currentThread], instance);
    g_gameInstance = instance;
}

// Forward declarations for iOS-specific functions
extern "C" {
void DrawTexturePro_iOS(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
void DrawTexture_iOS(Texture2D texture, int posX, int posY, Color tint);
void DrawTextureV_iOS(Texture2D texture, Vector2 position, Color tint);
void DrawTextureRec_iOS(Texture2D texture, Rectangle source, Rectangle dest, Color tint);
void UnloadTexture_iOS(Texture2D texture);
Texture2D CreateFallbackTexture(const char* fileName);
}

// Helper function declaration

// Error logging for Metal operations
static void LogMetalError(NSError *error, NSString *operation) {
    if (error) {
        NSLog(@"[ERROR] Metal operation failed: %@ - Error: %@", operation, error.localizedDescription);
    }
}

// iOS-specific implementations using Metal and Objective-C++

extern "C" {

Texture2D LoadTexture_iOS(const char *fileName)
{
    NSLog(@"[EXTRA LOG] LoadTexture_iOS: fileName=%s", fileName);
    
    // Initialize texture cache if needed
    static bool cacheInitialized = false;
    if (!cacheInitialized) {
        // Get Metal device through PlatformLayer
        void* metalDevice = PlatformLayer::GetInstance().GetMetalDevice();
        if (metalDevice) {
            MetalTextureCache::GetInstance().Initialize(metalDevice);
            cacheInitialized = true;
        } else {
            NSLog(@"[ERROR] LoadTexture_iOS: Failed to get Metal device for initializing texture cache");
        }
    }
    
    // Use the texture cache to load or retrieve the texture
    if (cacheInitialized) {
        std::string filePath(fileName);
        Texture2D texture = MetalTextureCache::GetInstance().GetOrLoadTexture(filePath);
        
        if (texture.texture) {
            NSLog(@"[EXTRA LOG] LoadTexture_iOS: SUCCESS texture=%p, w=%d, h=%d, file=%s", 
                  texture.texture, texture.width, texture.height, fileName);
            return texture;
        }
    }
    
    // Fallback to original implementation if cache fails or isn't initialized
    NSLog(@"[WARNING] LoadTexture_iOS: Cache miss or not initialized, falling back to direct loading: %s", fileName);
    
    Texture2D texture = { 0 };
    int width, height;
    
    std::string filePath(fileName);
    
    // Handle asset catalog resources
    if (filePath.substr(0, 8) == "asset://") {
        ResourcePathParts parts = ResourceManager::ParseResourcePath(filePath);
        NSString* name = [NSString stringWithUTF8String:parts.baseName.c_str()];
        NSLog(@"[DEBUG] LoadTexture_iOS: Loading asset catalog texture: %@", name);
        UIImage* uiImage = [UIImage imageNamed:name];
        
        if (!uiImage) {
            NSLog(@"[ERROR] LoadTexture_iOS: Failed to load asset catalog texture: %s", fileName);
            TraceLog(LOG_ERROR, "Failed to load asset catalog texture: %s", fileName);
            
            // Create a fallback texture instead of returning empty
            NSLog(@"[DEBUG] LoadTexture_iOS: Creating fallback texture for: %s", fileName);
            return CreateFallbackTexture(fileName);
        }
        
        NSLog(@"[DEBUG] LoadTexture_iOS: Successfully loaded UIImage for %@", name);
        
        CGImageRef cgImage = uiImage.CGImage;
        if (!cgImage) {
            NSLog(@"[ERROR] LoadTexture_iOS: CGImage is null for: %s", fileName);
            return CreateFallbackTexture(fileName);
        }
        
        width = (int)CGImageGetWidth(cgImage);
        height = (int)CGImageGetHeight(cgImage);
        
        if (width <= 0 || height <= 0) {
            NSLog(@"[ERROR] LoadTexture_iOS: Invalid dimensions for: %s (w=%d, h=%d)", fileName, width, height);
            return CreateFallbackTexture(fileName);
        }
        
        // Get Metal device through PlatformLayer
        id<MTLDevice> device = (__bridge id<MTLDevice>)PlatformLayer::GetInstance().GetMetalDevice();
        if (!device) {
            NSLog(@"[ERROR] LoadTexture_iOS: Failed to get Metal device for: %s", fileName);
            TraceLog(LOG_ERROR, "Failed to get Metal device for asset catalog texture");
            return CreateFallbackTexture(fileName);
        }

        // Create Metal texture with proper usage flags and mipmap support
        MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = width;
        textureDescriptor.height = height;
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        textureDescriptor.storageMode = MTLStorageModeShared;
        textureDescriptor.mipmapLevelCount = 1 + floor(log2(fmax(width, height)));
        
        NSError *error = nil;
        id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
        if (!metalTexture) {
            NSLog(@"Failed to create Metal texture: %@", error);
            // ARC will handle descriptor cleanup
            return CreateFallbackTexture(fileName);
        }
        
        NSLog(@"[DEBUG] LoadTexture_iOS: Created Metal texture: %p (retain count: %lu)", (__bridge void*)metalTexture, (unsigned long)CFGetRetainCount((__bridge CFTypeRef)metalTexture));
        
        // Load image data into texture
        MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(nil, width, height, 8, 4 * width, colorSpace, kCGImageAlphaPremultipliedLast);
        
        if (!context) {
            NSLog(@"[ERROR] LoadTexture_iOS: Failed to create bitmap context for: %s", fileName);
            CGColorSpaceRelease(colorSpace);
            return CreateFallbackTexture(fileName);
        }
        
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
        void* imageData = CGBitmapContextGetData(context);
        
        if (!imageData) {
            NSLog(@"[ERROR] LoadTexture_iOS: Failed to get image data for: %s", fileName);
            CGContextRelease(context);
            CGColorSpaceRelease(colorSpace);
            return CreateFallbackTexture(fileName);
        }
        
        [metalTexture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
        
        // Generate mipmaps if needed
        if (textureDescriptor.mipmapLevelCount > 1) {
            // Use the shared command queue from PlatformLayer
            id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)PlatformLayer::GetInstance().GetMetalCommandQueue();
            if (commandQueue) {
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
            [blitEncoder generateMipmapsForTexture:metalTexture];
            [blitEncoder endEncoding];
            [commandBuffer commit];
            }
        }
        
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        texture.id = 0; // Will be assigned by texture cache
        texture.texture = (__bridge_retained void*)metalTexture;
        texture.width = width;
        texture.height = height;
        texture.mipmaps = textureDescriptor.mipmapLevelCount;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        
        // Add to texture cache for future use
        if (cacheInitialized) {
            // Update texture cache with this new texture for future reuse
            // First release our reference since cache will retain it
            void* tempPtr = texture.texture;
            texture.texture = nullptr;
            CFRelease(tempPtr);
            
            // Use LoadTextureFromData to properly register in cache
            return MetalTextureCache::GetInstance().LoadTextureFromData(
                imageData, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        }
    } else {
        // Handle regular file paths
        void* texturePtr = PlatformLayer::GetInstance().LoadTexture(fileName, &width, &height);
        if (texturePtr) {
            texture.id = 0; // Not used in Metal
            texture.texture = texturePtr;
            texture.width = width;
            texture.height = height;
            texture.mipmaps = 1;
            texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        } else {
            NSLog(@"[ERROR] LoadTexture_iOS: Failed to load regular file texture: %s", fileName);
            return CreateFallbackTexture(fileName);
        }
    }
    
    if (texture.texture == NULL) {
        NSLog(@"[EXTRA LOG] LoadTexture_iOS: FAILED to load texture for %s", fileName);
        return CreateFallbackTexture(fileName);
    } else {
        NSLog(@"[EXTRA LOG] LoadTexture_iOS: SUCCESS texture=%p, w=%d, h=%d, file=%s", texture.texture, texture.width, texture.height, fileName);
    }
    return texture;
}

Texture2D LoadTextureFromImage_iOS(Image image)
{
    if (!image.data) {
        TraceLog(LOG_ERROR, "Cannot load texture from null image");
        return Texture2D{0, 0, 0, 0, 0};
    }
    
    // Check if texture cache is initialized
    static bool cacheInitialized = false;
    static bool checkedCache = false;
    
    if (!checkedCache) {
        // Get Metal device through PlatformLayer
        void* metalDevice = PlatformLayer::GetInstance().GetMetalDevice();
        if (metalDevice) {
            // Initialize cache if needed
            MetalTextureCache::GetInstance().Initialize(metalDevice);
            cacheInitialized = true;
        }
        checkedCache = true;
    }
    
    // Use texture cache if available
    if (cacheInitialized) {
        Texture2D texture = MetalTextureCache::GetInstance().LoadTextureFromData(
            image.data, image.width, image.height, image.format);
            
        if (texture.texture) {
            NSLog(@"[INFO] Texture created from image using cache: %p (w=%d, h=%d)", 
                  texture.texture, texture.width, texture.height);
            return texture;
        }
    }
    
    // Fallback to original implementation
    NSLog(@"[WARNING] Falling back to original implementation for LoadTextureFromImage_iOS");
    
    // Use PlatformLayer delegate for texture creation instead of MetalRenderer
    PlatformLayer& platform = PlatformLayer::GetInstance();
    // Pass the image data pointer directly - image.data is already a void*
    void* texture = platform.LoadTextureFromImage(image.data, image.width, image.height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    
    if (!texture) {
        TraceLog(LOG_ERROR, "Failed to create texture from image");
        return Texture2D{0, 0, 0, 0, 0};
    }
    
    Texture2D texture2D;
    texture2D.id = 0; // Not used in Metal
    texture2D.texture = texture;
    texture2D.width = image.width;
    texture2D.height = image.height;
    texture2D.mipmaps = 1;
    texture2D.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return texture2D;
}

Image LoadImage_iOS(const char* fileName)
{
    std::string path = fileName;
    
    // Handle asset catalog resources
    if (path.substr(0, 8) == "asset://") {
        ResourcePathParts parts = ResourceManager::ParseResourcePath(path);
        NSString* name = [NSString stringWithUTF8String:parts.baseName.c_str()];
        UIImage* uiImage = [UIImage imageNamed:name];
        
        if (!uiImage) {
            TraceLog(LOG_ERROR, "Failed to load asset catalog image: %s", path.c_str());
            return Image{nullptr, 0, 0, 0, 0};
        }
        
        // Convert UIImage to CGImage to get pixel data
        CGImageRef cgImage = uiImage.CGImage;
        size_t width = CGImageGetWidth(cgImage);
        size_t height = CGImageGetHeight(cgImage);
        
        // Create a context to draw the image into for raw pixel data
        void* data = malloc(width * height * 4);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(data, width, height, 8, width * 4, colorSpace, kCGImageAlphaPremultipliedLast);
        CGColorSpaceRelease(colorSpace);
        
        // Draw image into context
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
        CGContextRelease(context);
        
        // Return raylib-compatible Image structure
        Image img;
        img.data = data;
        img.width = (int)width;
        img.height = (int)height;
        img.mipmaps = 1;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        return img;
    }
    
    // Handle regular file paths
    NSString* pathStr = [NSString stringWithUTF8String:path.c_str()];
    UIImage* uiImage = [UIImage imageNamed:pathStr];
    if (!uiImage) {
        // Fallback to file system path if not found in bundle
        uiImage = [UIImage imageWithContentsOfFile:pathStr];
    }
    
    if (!uiImage) {
        TraceLog(LOG_ERROR, "Failed to load image from path: %s", path.c_str());
        return Image{nullptr, 0, 0, 0, 0};
    }
    
    // Convert UIImage to CGImage to get pixel data
    CGImageRef cgImage = uiImage.CGImage;
    size_t width = CGImageGetWidth(cgImage);
    size_t height = CGImageGetHeight(cgImage);
    
    // Create a context to draw the image into for raw pixel data
    void* data = malloc(width * height * 4);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(data, width, height, 8, width * 4, colorSpace, kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(colorSpace);
    
    // Draw image into context
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
    CGContextRelease(context);
    
    // Return raylib-compatible Image structure
    Image img;
    img.data = data;
    img.width = (int)width;
    img.height = (int)height;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return img;
}

void UnloadImage_iOS(Image image)
{
    if (image.data) {
        free(image.data);
    }
}

Image GenImageColor_iOS(int width, int height, Color color)
{
    void* data = malloc(width * height * 4);
    unsigned char* pixels = (unsigned char*)data;
    unsigned char r = color.r;
    unsigned char g = color.g;
    unsigned char b = color.b;
    unsigned char a = color.a;
    
    for (int i = 0; i < width * height; i++) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = a;
    }
    
    Image img;
    img.data = data;
    img.width = width;
    img.height = height;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return img;
}

void ImageResize_iOS(Image* image, int newWidth, int newHeight)
{
    if (!image || !image->data) return;
    
    // Create a new buffer for resized image
    void* newData = malloc(newWidth * newHeight * 4);
    unsigned char* src = (unsigned char*)image->data;
    unsigned char* dst = (unsigned char*)newData;
    
    // Simple nearest-neighbor resize
    float xRatio = (float)image->width / newWidth;
    float yRatio = (float)image->height / newHeight;
    
    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            int srcX = (int)(x * xRatio);
            int srcY = (int)(y * yRatio);
            int srcIndex = (srcY * image->width + srcX) * 4;
            int dstIndex = (y * newWidth + x) * 4;
            
            dst[dstIndex + 0] = src[srcIndex + 0];
            dst[dstIndex + 1] = src[srcIndex + 1];
            dst[dstIndex + 2] = src[srcIndex + 2];
            dst[dstIndex + 3] = src[srcIndex + 3];
        }
    }
    
    // Free old data and update image
    free(image->data);
    image->data = newData;
    image->width = newWidth;
    image->height = newHeight;
}

void ImageDraw_iOS(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint)
{
    if (!dst || !dst->data || !src.data) return;
    
    unsigned char* dstPixels = (unsigned char*)dst->data;
    unsigned char* srcPixels = (unsigned char*)src.data;
    
    // Clamp source rectangle to src image bounds
    int srcX = (int)std::max(0.0f, srcRec.x);
    int srcY = (int)std::max(0.0f, srcRec.y);
    int srcW = (int)std::min((float)src.width - srcX, srcRec.width);
    int srcH = (int)std::min((float)src.height - srcY, srcRec.height);
    
    // Clamp destination rectangle to dst image bounds
    int dstX = (int)std::max(0.0f, dstRec.x);
    int dstY = (int)std::max(0.0f, dstRec.y);
    int dstW = (int)std::min((float)dst->width - dstX, dstRec.width);
    int dstH = (int)std::min((float)dst->height - dstY, dstRec.height);
    
    // Only draw the overlapping area
    int drawW = std::min(srcW, dstW);
    int drawH = std::min(srcH, dstH);
    
    float tintR = tint.r / 255.0f;
    float tintG = tint.g / 255.0f;
    float tintB = tint.b / 255.0f;
    float tintA = tint.a / 255.0f;
    
    for (int y = 0; y < drawH; y++) {
        for (int x = 0; x < drawW; x++) {
            int srcIdx = ((srcY + y) * src.width + (srcX + x)) * 4;
            int dstIdx = ((dstY + y) * dst->width + (dstX + x)) * 4;
            
            float srcA = srcPixels[srcIdx + 3] / 255.0f * tintA;
            float invA = 1.0f - srcA;
            
            dstPixels[dstIdx + 0] = (unsigned char)((srcPixels[srcIdx + 0] * tintR * srcA + dstPixels[dstIdx + 0] * invA));
            dstPixels[dstIdx + 1] = (unsigned char)((srcPixels[srcIdx + 1] * tintG * srcA + dstPixels[dstIdx + 1] * invA));
            dstPixels[dstIdx + 2] = (unsigned char)((srcPixels[srcIdx + 2] * tintB * srcA + dstPixels[dstIdx + 2] * invA));
            dstPixels[dstIdx + 3] = (unsigned char)(std::min(255.0f, (srcPixels[srcIdx + 3] * tintA + dstPixels[dstIdx + 3] * invA * (dstPixels[dstIdx + 3] / 255.0f))) * 255.0f);
        }
    }
}

void BeginDrawing_iOS(void* renderTexture) {
    PlatformLayer::GetInstance().BeginDrawing(renderTexture);
}

void EndDrawing_iOS(void* renderTexture) {
    PlatformLayer::GetInstance().EndDrawing(renderTexture);
}

void DrawRectangle_iOS(int posX, int posY, int width, int height, unsigned int color) {
    PlatformLayer::GetInstance().DrawRectangle(posX, posY, width, height, color);
}

void DrawText(const char *text, int posX, int posY, int fontSize, Color color) {
    TraceLog(LOG_INFO, "[RaylibCompat_iOS] DrawText called: text='%s', pos=(%d, %d), fontSize=%d, color=(%d,%d,%d,%d)", text, posX, posY, fontSize, color.r, color.g, color.b, color.a);
    PlatformLayer::GetInstance().DrawText(text, (float)posX, (float)posY, (float)fontSize, color, nullptr);
}

void DrawTexture_iOS(Texture2D texture, int posX, int posY, Color tint)
{
    if (texture.texture == NULL) {
        NSLog(@"[EXTRA LOG] DrawTexture_iOS: SKIP invalid texture at posX=%d posY=%d", posX, posY);
        return;
    }
    NSLog(@"[EXTRA LOG] DrawTexture_iOS: texture=%p, posX=%d, posY=%d, w=%d, h=%d", texture.texture, posX, posY, texture.width, texture.height);
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    Rectangle dest = { (float)posX, (float)posY, (float)texture.width, (float)texture.height };
    DrawTexturePro_iOS(texture, source, dest, { 0.0f, 0.0f }, 0.0f, tint);
}

void DrawTextureV_iOS(Texture2D texture, Vector2 position, Color tint)
{
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    Rectangle dest = { position.x, position.y, (float)texture.width, (float)texture.height };
    DrawTexturePro_iOS(texture, source, dest, { 0.0f, 0.0f }, 0.0f, tint);
}

void DrawTextureEx_iOS(Texture2D texture, Vector2 position, float rotation, float scale, Color tint)
{
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    Rectangle dest = { position.x, position.y, (float)texture.width * scale, (float)texture.height * scale };
    DrawTexturePro_iOS(texture, source, dest, { 0.0f, 0.0f }, rotation, tint);
}

void DrawTextureRec_iOS(Texture2D texture, Rectangle source, Rectangle dest, Color tint)
{
    DrawTexturePro_iOS(texture, source, dest, { 0.0f, 0.0f }, 0.0f, tint);
}

void DrawTexturePro_iOS(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint)
{
    if (texture.texture == NULL) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: SKIP invalid texture");
        return;
    }

    // Add type checking to prevent crashes
    id<NSObject> textureObj = (__bridge id<NSObject>)texture.texture;
    // Check if it's a Metal texture (including simulator textures)
    if (![textureObj isKindOfClass:NSClassFromString(@"MTLTexture")] && 
        ![textureObj isKindOfClass:NSClassFromString(@"MTLSimTexture")]) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Texture pointer is not a Metal texture! Type: %@, pointer: %p", 
                 NSStringFromClass([textureObj class]), texture.texture);
        return;
    }

    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Invalid texture pointer (NULL)");
        return;
    }

    if (metalTexture.width == 0 || metalTexture.height == 0) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Texture has invalid dimensions (w=%lu, h=%lu)",
                 (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
        return;
    }

    if (source.width <= 0 || source.height <= 0 || dest.width <= 0 || dest.height <= 0) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Invalid rectangles: src=(%.3f,%.3f,%.3f,%.3f), dest=(%.1f,%.1f,%.1f,%.1f)",
                 source.x, source.y, source.width, source.height, dest.x, dest.y, dest.width, dest.height);
        return;
    }

    // Additional safety check for source rectangle bounds
    if (source.x < 0 || source.y < 0 || 
        source.x + source.width > metalTexture.width || 
        source.y + source.height > metalTexture.height) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Source rectangle out of bounds: src=(%.3f,%.3f,%.3f,%.3f), texture=(%lu,%lu)",
                 source.x, source.y, source.width, source.height, 
                 (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
        return;
    }

    TraceLog(LOG_INFO, "[RaylibCompat_iOS] DrawTexturePro_iOS: Drawing texture=%p (w=%lu, h=%lu), src=(%.3f,%.3f,%.3f,%.3f), dest=(%.1f,%.1f,%.1f,%.1f), origin=(%.1f,%.1f), rotation=%.1f, tint=(%d,%d,%d,%d)",
             texture.texture, (unsigned long)metalTexture.width, (unsigned long)metalTexture.height,
             source.x, source.y, source.width, source.height,
             dest.x, dest.y, dest.width, dest.height,
             origin.x, origin.y, rotation, tint.r, tint.g, tint.b, tint.a);

    // Add try-catch to catch any Metal-related crashes
    @try {
        // Get the MetalRenderer from the PlatformLayerDelegate
        PlatformLayer& platform = PlatformLayer::GetInstance();
        void* delegatePtr = platform.GetDelegate();
        MetalRenderer* metalRenderer = nullptr;
        
        if (delegatePtr) {
            // Cast to PlatformLayerDelegate and get the MetalRenderer
            PlatformLayerDelegate* delegate = (__bridge PlatformLayerDelegate*)delegatePtr;
            if ([delegate respondsToSelector:@selector(getMetalRenderer)]) {
                metalRenderer = (MetalRenderer*)[delegate getMetalRenderer];
            }
        }
        
        if (metalRenderer) {
            // Use the new DrawTexture overload that accepts texture format
            metalRenderer->DrawTexture(metalTexture, source, dest, tint, RenderLayer::UI, texture.format);
        } else {
            // Fallback to PlatformLayer if MetalRenderer not available
            platform.DrawTexture((__bridge void*)metalTexture, dest.x, dest.y, dest.width, dest.height, tint);
        }
    } @catch (NSException *exception) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Exception in DrawTexture: %s", [exception.reason UTF8String]);
    } @catch (...) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Unknown exception in DrawTexture");
    }
}

void UnloadTexture_iOS(Texture2D texture)
{
    if (texture.texture) {
        PlatformLayer::GetInstance().UnloadTexture(texture.texture);
    }
}

// Helper function to create fallback textures
extern "C" Texture2D CreateFallbackTexture(const char* fileName)
{
    NSLog(@"[DEBUG] CreateFallbackTexture: Creating fallback for: %s", fileName);
    
    // Create a 2x2 magenta texture as fallback
    const int fallbackSize = 2;
    const int fallbackData[] = {
        static_cast<int>(0xFFFF00FF), static_cast<int>(0xFFFF00FF),  // Magenta pixels
        static_cast<int>(0xFFFF00FF), static_cast<int>(0xFFFF00FF)
    };
    
    id<MTLDevice> device = (__bridge id<MTLDevice>)PlatformLayer::GetInstance().GetMetalDevice();
    if (!device) {
        NSLog(@"[ERROR] CreateFallbackTexture: No Metal device available");
        return { 0, 0, 0, 0, 0 };
    }
    
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = fallbackSize;
    textureDescriptor.height = fallbackSize;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    NSError *error = nil;
    id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
    if (!metalTexture) {
        NSLog(@"Failed to create Metal texture: %@", error);
        // ARC will handle descriptor cleanup
        return { 0, 0, 0, 0, 0 };
    }
    
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)fallbackSize, (NSUInteger)fallbackSize, 1}};
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:fallbackData bytesPerRow:4 * fallbackSize];
    
    Texture2D fallbackTexture;
    fallbackTexture.id = 0; // Not used in Metal
    fallbackTexture.texture = (__bridge_retained void*)metalTexture;
    fallbackTexture.width = fallbackSize;
    fallbackTexture.height = fallbackSize;
    fallbackTexture.mipmaps = 1;
    fallbackTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    
    NSLog(@"[DEBUG] CreateFallbackTexture: Fallback texture created successfully (id=%p)", fallbackTexture.id);
    return fallbackTexture;
}

// iOS-specific render texture cleanup
extern "C" void UnloadRenderTexture_iOS(RenderTexture2D target)
{
    // Clean up the Metal render texture
    if (target.texture.texture != nullptr) {
        // Release the Metal texture
        id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)target.texture.texture;
        if (metalTexture) {
            // The texture will be automatically released when the bridge is released
            CFBridgingRelease(target.texture.texture);
            NSLog(@"[DEBUG] UnloadRenderTexture_iOS: Released Metal render texture: %p", (__bridge void*)metalTexture);
        }
    }
}

// iOS game initialization function
extern "C" int game_main(int argc, char *argv[])
{
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] game_main() STARTING (iOS version)");
    NSLog(@"[INIT] argc=%d", argc);
    NSLog(@"[INIT] ========================================");
    
    // Suppress unused parameter warnings
    (void)argc;
    (void)argv;
    
    // Setup working directory first
    // SetupWorkingDirectory(); // iOS handles working directory differently
    NSLog(@"[INIT] Skipping working directory setup for iOS");
    
    // Seed the random number generator
    srand(static_cast<unsigned int>(time(NULL)));
    NSLog(@"[INIT] Seeded random number generator");

    // PlatformLayer is already initialized by GameViewController
    NSLog(@"[INIT] PlatformLayer already initialized by GameViewController");

    // On iOS, we only create the game instance
    // The actual game loop is driven by the iOS display system
    NSLog(@"[INIT] About to create Game instance and set it.");
    
    Game* gameInstance = nullptr;
    try {
        NSLog(@"[INIT] Creating Game instance...");
        gameInstance = new Game();
        NSLog(@"[INIT] Game instance created successfully: %p", gameInstance);
    } catch (const std::exception& e) {
        NSLog(@"[ERROR] Exception creating Game instance: %s", e.what());
        return -1;
    } catch (...) {
        NSLog(@"[ERROR] Unknown exception creating Game instance");
        return -1;
    }
    
    if (!gameInstance) {
        NSLog(@"[ERROR] Game instance creation returned nullptr");
        return -1;
    }
    
    SetGameInstance(gameInstance);
    NSLog(@"[INIT] Game instance created and set: %p", g_gameInstance);
    
    // Initialize the game instance
    NSLog(@"[INIT] Initializing Game instance - STARTING");
    NSLog(@"[INIT] Calling Game::Initialize() on instance: %p", g_gameInstance);
    bool initResult = g_gameInstance->Initialize();
    NSLog(@"[INIT] Game::Initialize() COMPLETED with result: %s", initResult ? "true" : "false");
    if (!initResult) {
        NSLog(@"[ERROR] Failed to initialize Game instance! Keeping instance alive for debugging.");
        return -1;
    }
    
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] game_main() COMPLETED SUCCESSFULLY");
    NSLog(@"[INIT] ========================================");
    return 0; // iOS will keep the app running via UIApplicationMain
}

// Function to get the main screen's bounds in a cross-platform way if needed
// For now, this is handled in GameViewController
/*
extern "C" CGRect GetScreenBounds() {
    return [[UIScreen mainScreen] bounds];
}
*/

} // extern "C"

// struct Sound {
//     void* player; // Actually an AVAudioPlayer*
//     int length;
// };

// struct Music {
//     void* player; // Actually an AVAudioPlayer*
//     int length;
// };

void InitAudioDevice() {
    GameLog::Log("[AUDIO] InitAudioDevice (iOS/AVFoundation)");
    NSError* error = nil;
    
    // Use AVAudioSessionCategoryPlayback for better music support
    // This allows music to continue playing when the app is in the background
    BOOL success = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback 
                                                          error:&error];
    if (!success) {
        GameLog::Log("[AUDIO] ERROR: Failed to set audio session category: %s", 
                     error.localizedDescription.UTF8String);
    }
    
    // Activate the audio session
    success = [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if (!success) {
        GameLog::Log("[AUDIO] ERROR: Failed to activate audio session: %s", 
                     error.localizedDescription.UTF8String);
    } else {
        GameLog::Log("[AUDIO] Audio session activated successfully");
    }
}

void CloseAudioDevice() {
    GameLog::Log("[AUDIO] CloseAudioDevice (iOS/AVFoundation)");
}

Sound LoadSound(const char* fileName) {
    Sound s = {0};
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        NSURL* url = [NSURL fileURLWithPath:path];
        NSError* error = nil;
        AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
        if (player && !error) {
            [player prepareToPlay];
            s.player = (__bridge_retained void*)player;
            s.length = (int)(player.duration * 1000);
            GameLog::Log("[AUDIO] Loaded sound: %s (duration: %d ms)", fileName, s.length);
        } else {
            GameLog::Log("[AUDIO] ERROR: Failed to load sound: %s (%s)", fileName, error.localizedDescription.UTF8String);
        }
    }
    return s;
}

void UnloadSound(Sound sound) {
    if (sound.player) {
        AVAudioPlayer* player = (__bridge_transfer AVAudioPlayer*)sound.player;
        [player stop];
        GameLog::Log("[AUDIO] Unloaded sound");
    }
}

void PlaySound(Sound sound) {
    if (sound.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
        [player play];
        GameLog::Log("[AUDIO] PlaySound");
    }
}

void SetSoundVolume(Sound sound, float volume) {
    if (sound.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
        player.volume = volume;
        GameLog::Log("[AUDIO] SetSoundVolume: %f", volume);
    }
}

Music LoadMusic(const char* fileName) {
    Music m = {0};
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        GameLog::Log("[AUDIO] Attempting to load music from path: %s", fileName);
        
        NSURL* url = nil;
        
        // Handle asset catalog resources
        if (path.length >= 8 && [[path substringToIndex:8] isEqualToString:@"asset://"]) {
            std::string cppPath = [path UTF8String];
            ResourcePathParts parts = ResourceManager::ParseResourcePath(cppPath);
            NSString* assetName = [NSString stringWithUTF8String:parts.baseName.c_str()];
            GameLog::Log("[AUDIO] Loading asset catalog music: %s", [assetName UTF8String]);
            NSDataAsset* dataAsset = [[NSDataAsset alloc] initWithName:assetName];
            if (dataAsset && dataAsset.data) {
                // Create a temporary file with the asset data
                NSString* tempDir = NSTemporaryDirectory();
                NSString* tempFileName = [NSString stringWithFormat:@"%@_%@.mp3", assetName, [[NSUUID UUID] UUIDString]];
                NSString* tempFilePath = [tempDir stringByAppendingPathComponent:tempFileName];
                
                if ([dataAsset.data writeToFile:tempFilePath atomically:YES]) {
                    url = [NSURL fileURLWithPath:tempFilePath];
                    GameLog::Log("[AUDIO] Successfully created temp file from asset catalog: %@", tempFilePath);
                } else {
                    GameLog::Log("[AUDIO] ERROR: Failed to write asset data to temp file");
                }
            } else {
                GameLog::Log("[AUDIO] ERROR: Failed to load asset catalog data for: %@", assetName);
                
                // Debug: List available data assets
                GameLog::Log("[AUDIO] DEBUG: Cannot list all data assets - method not available");
                // Note: NSDataAsset doesn't have an allDataAssets method
                // We can only access assets by name using initWithName:
            }
        } else {
            // Handle regular file paths
            url = [NSURL fileURLWithPath:path];
        }
        
        if (!url) {
            GameLog::Log("[AUDIO] ERROR: Failed to create URL from path: %s", fileName);
            return m;
        }
        
        GameLog::Log("[AUDIO] Created URL: %@ -- %@", url, url.absoluteString);
        
        NSError* error = nil;
        AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
        if (player && !error) {
            [player prepareToPlay];
            m.player = (__bridge_retained void*)player;
            m.length = (int)(player.duration * 1000);
            GameLog::Log("[AUDIO] Successfully loaded music: %s (duration: %d ms)", fileName, m.length);
        } else {
            GameLog::Log("[AUDIO] ERROR: Failed to load music: %s", fileName);
            if (error) {
                GameLog::Log("[AUDIO] Error details: %s", error.localizedDescription.UTF8String);
            }
        }
    }
    return m;
}

Music LoadMusicStream(const char* fileName) {
    // For iOS, LoadMusicStream is the same as LoadMusic since we use AVAudioPlayer
    // which handles streaming internally
    GameLog::Log("[AUDIO] LoadMusicStream called, delegating to LoadMusic: %s", fileName);
    return LoadMusic(fileName);
}

void UnloadMusic(Music music) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge_transfer AVAudioPlayer*)music.player;
        [player stop];
        GameLog::Log("[AUDIO] Unloaded music");
    }
}

void UnloadMusicStream(Music music) {
    // Alias for UnloadMusic to maintain raylib compatibility
    UnloadMusic(music);
}

void PlayMusic(Music music) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        
        // Ensure audio session is active before trying to play
        NSError* error = nil;
        BOOL sessionActive = [[AVAudioSession sharedInstance] setActive:YES error:&error];
        if (!sessionActive) {
            GameLog::Log("[AUDIO] WARNING: Failed to activate audio session: %s", 
                         error.localizedDescription.UTF8String);
        }
        
        BOOL success = [player play];
        if (success) {
            GameLog::Log("[AUDIO] PlayMusic: Started playing successfully");
        } else {
            GameLog::Log("[AUDIO] PlayMusic: Failed to start playing");
        }
    } else {
        GameLog::Log("[AUDIO] PlayMusic: No valid player to play");
    }
}

void PlayMusicLoop(Music music) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        player.numberOfLoops = -1; // -1 means infinite loop
        [player play];
        GameLog::Log("[AUDIO] PlayMusicLoop (infinite)");
    }
}

void PauseMusic(Music music) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        [player pause];
        GameLog::Log("[AUDIO] PauseMusic");
    }
}

void ResumeMusic(Music music) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        
        // Ensure audio session is active before trying to play
        NSError* error = nil;
        BOOL sessionActive = [[AVAudioSession sharedInstance] setActive:YES error:&error];
        if (!sessionActive) {
            GameLog::Log("[AUDIO] WARNING: Failed to activate audio session: %s", 
                         error.localizedDescription.UTF8String);
        }
        
        // Try to play the music
        BOOL success = [player play];
        if (success) {
            GameLog::Log("[AUDIO] ResumeMusic: Successfully resumed");
        } else {
            GameLog::Log("[AUDIO] ResumeMusic: Failed to resume, player may need to be restarted");
        }
    } else {
        GameLog::Log("[AUDIO] ResumeMusic: No valid player to resume");
    }
}

void SetMusicVolume(Music music, float volume) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        player.volume = volume;
        GameLog::Log("[AUDIO] SetMusicVolume: %f", volume);
    }
}

void StopMusic(Music music) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        [player stop];
        GameLog::Log("[AUDIO] StopMusic");
    }
}

bool IsMusicPlaying(Music music) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        return player.isPlaying;
    }
    return false;
}

void SetLooping(Music music, bool looping) {
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        player.numberOfLoops = looping ? -1 : 0; // -1 = infinite loop, 0 = play once
        GameLog::Log("[AUDIO] SetLooping: %s", looping ? "true" : "false");
    }
}

// Music stream functions (aliases for compatibility)
void PlayMusicStream(Music music) {
    PlayMusic(music);
}

void StopMusicStream(Music music) {
    StopMusic(music);
}

void UpdateMusicStream(Music music) {
    // No update needed for iOS AVAudioPlayer
    // This function exists for raylib compatibility
}

bool IsMusicStreamPlaying(Music music) {
    return IsMusicPlaying(music);
}

bool IsMouseButtonReleased(int button)
{
    // On iOS, treat touch release as left mouse button release
    if (button == 0) {
        return PlatformLayer::GetInstance().IsPrimaryInputReleased();
    }
    return false;
}

// ========== ESSENTIAL MISSING RAYLIB FUNCTIONS ==========
// Only add functions that are not already defined

extern "C" {

void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint)
{
    // Convert Vector2 position to Rectangle dest using source dimensions
    Rectangle dest = { position.x, position.y, source.width, source.height };
    DrawTexturePro_iOS(texture, source, dest, { 0.0f, 0.0f }, 0.0f, tint);
}

// Essential drawing functions that are missing
void ClearBackground(Color color) {
    // Simple background clear - just draw a full-screen rectangle
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), color);
}

// Text drawing wrappers
void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    TraceLog(LOG_INFO, "[RaylibCompat_iOS] DrawTextEx: text='%s', pos=(%.1f, %.1f), fontSize=%.1f, spacing=%.1f, color=(%d,%d,%d,%d)", 
             text, position.x, position.y, fontSize, spacing, tint.r, tint.g, tint.b, tint.a);

    if (font.texture.id == 0 || !font.recs || !font.glyphs || !font.texture.texture) {
        TraceLog(LOG_WARNING, "[RaylibCompat_iOS] DrawTextEx: Invalid font, falling back to DrawText");
        DrawText(text, (int)position.x, (int)position.y, (int)fontSize, tint);
        return;
    }

    float scale = fontSize / (float)font.baseSize;
    float x = position.x;
    float y = position.y;
    const int startChar = 32;
    const int endChar = 126;
    int* glyphData = (int*)font.glyphs;

    for (const char* p = text; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < startChar || c > endChar) {
            x += fontSize * 0.5f;
            continue;
        }
        int index = c - startChar;
        if (index < 0 || index >= font.glyphCount) {
            x += fontSize * 0.5f;
            continue;
        }

        Rectangle src = {
            font.recs[index].x * font.texture.width,
            font.recs[index].y * font.texture.height,
            font.recs[index].width * font.texture.width,
            font.recs[index].height * font.texture.height
        };
        Rectangle dst = {
            x,
            y + (glyphData[index * 4 + 1] * scale),
            src.width * scale,
            src.height * scale
        };

        if (src.width <= 0 || src.height <= 0) {
            TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTextEx: Invalid UVs for char '%c'", c);
            x += fontSize * 0.5f;
            continue;
        }

        // Use SDF pipeline for grayscale textures
        if (font.texture.format == IOS_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE) {
            // Ensure MetalRenderer uses SDF shader (handled in MetalRenderer::DrawTexture)
            DrawTexturePro_iOS(font.texture, src, dst, {0, 0}, 0.0f, tint);
        } else {
            DrawTexturePro_iOS(font.texture, src, dst, {0, 0}, 0.0f, tint);
        }

        x += glyphData[index * 4 + 3] * scale + spacing;
    }
}

// Texture drawing wrappers
void DrawTexture(Texture2D texture, int posX, int posY, Color tint) {
    DrawTexture_iOS(texture, posX, posY, tint);
}

void DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
    DrawTextureV_iOS(texture, position, tint);
}

void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
    DrawTextureEx_iOS(texture, position, rotation, scale, tint);
}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    DrawTexturePro_iOS(texture, source, dest, origin, rotation, tint);
}

// Rectangle drawing wrappers
void DrawRectangle(int posX, int posY, int width, int height, Color color) {
    DrawRectangle_iOS(posX, posY, width, height, (color.r << 24) | (color.g << 16) | (color.b << 8) | color.a);
}

void DrawRectangleRec(Rectangle rec, Color color) {
    DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, (int)rec.height, color);
}

void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    // For now, use regular rectangle drawing
    DrawRectangleRec(rec, color);
}

// Line and circle drawing (simple implementations)
void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) {
    // Simple line drawing - not implemented for iOS yet
}

void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
    PlatformLayer::GetInstance().DrawLineEx(startPos.x, startPos.y, endPos.x, endPos.y, thick, color);
}

void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, Color color) {
    // Use default line thickness of 1.0f
    DrawRectangleRoundedLinesEx(rec, roundness, segments, 1.0f, color);
}

void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    PlatformLayer::GetInstance().DrawRectangleRoundedLines(rec.x, rec.y, rec.width, rec.height, roundness, segments, lineThick, color);
}

void DrawCircleV(Vector2 center, float radius, Color color) {
    // Simple circle drawing - not implemented for iOS yet
}

// Essential Vector2 math functions
Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    return { v1.x + v2.x, v1.y + v2.y };
}

Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
    return { v1.x - v2.x, v1.y - v2.y };
}

Vector2 Vector2Scale(Vector2 v, float scale) {
    return { v.x * scale, v.y * scale };
}

float Vector2Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float Vector2Distance(Vector2 v1, Vector2 v2) {
    return Vector2Length(Vector2Subtract(v2, v1));
}

Vector2 Vector2Normalize(Vector2 v) {
    float len = Vector2Length(v);
    if (len < 0.0001f) return { 0.0f, 0.0f };
    float inv = 1.0f / len;
    return { v.x * inv, v.y * inv };
}

// Essential color functions
Color ColorLerp(Color a, Color b, float t) {
    Color result;
    result.r = (unsigned char)(a.r + (b.r - a.r) * t);
    result.g = (unsigned char)(a.g + (b.g - a.g) * t);
    result.b = (unsigned char)(a.b + (b.b - a.b) * t);
    result.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return result;
}

Color ColorAlpha(Color color, float alpha) {
    color.a = (unsigned char)(alpha * 255.0f);
    return color;
}

Color Fade(Color color, float alpha) {
    return ColorAlpha(color, alpha);
}

// Essential input functions (only add if not already defined)
bool IsMouseButtonDown(int button) {
    if (button >= 0 && button < 3) {
        return PlatformLayer::GetInstance().IsPrimaryInputDown();
    }
    return false;
}

bool IsKeyPressed(int key) {
    // iOS doesn't have keyboard input in the same way
    return false;
}

bool IsKeyDown(int key) {
    // iOS doesn't have keyboard input in the same way
    return false;
}

Vector2 GetMouseDelta(void) {
    return { 0, 0 }; // Not implemented for iOS
}

// Essential screen functions
int GetScreenWidth(void) {
    return PlatformLayer::GetInstance().GetScreenWidth();
}

int GetScreenHeight(void) {
    return PlatformLayer::GetInstance().GetScreenHeight();
}

// Essential window functions
bool IsWindowFullscreen(void) {
    return false; // iOS is always fullscreen
}

void ToggleFullscreen(void) {
    // Not applicable on iOS
}

void SetWindowPosition(int x, int y) {
    // Not applicable on iOS
}

void SetWindowSize(int width, int height) {
    // Not applicable on iOS
}

// Essential monitor functions
int GetCurrentMonitor(void) {
    return 0; // iOS has only one screen
}

int GetMonitorWidth(int monitor) {
    return GetScreenWidth();
}

int GetMonitorHeight(int monitor) {
    return GetScreenHeight();
}

Vector2 GetMonitorPosition(int monitor) {
    return { 0, 0 };
}

// Essential time functions
double GetTime(void) {
    // Simple time implementation
    static double startTime = 0.0;
    if (startTime == 0.0) {
        startTime = [[NSDate date] timeIntervalSince1970];
    }
    return [[NSDate date] timeIntervalSince1970] - startTime;
}

float GetFrameTime(void) {
    // Simple frame time implementation
    static double lastTime = 0.0;
    double currentTime = GetTime();
    float frameTime = (float)(currentTime - lastTime);
    lastTime = currentTime;
    return frameTime > 0.0f ? frameTime : 0.016f; // Default to 60 FPS
}

// Essential font functions
Font GetFontDefault(void) {
    NSLog(@"[LOG] GetFontDefault called");
    // Try to load Whacky Joe font first, fall back to system font if it fails
    static Font defaultFont = {0};
    static bool initialized = false;
    static bool whackyJoeAttempted = false; // Guard against infinite recursion
    
    if (!initialized) {
        // Try to load Whacky Joe font through ResourceManager (only once)
        if (!whackyJoeAttempted) {
            whackyJoeAttempted = true;
            Font whackyJoeFont = ResourceManager::GetInstance().GetFont("whacky_joe_font");
            if (whackyJoeFont.baseSize > 0 && 
#if defined(__APPLE__) && TARGET_OS_IPHONE
                whackyJoeFont.ctFont != nullptr
#else
                whackyJoeFont.glyphCount > 0 && whackyJoeFont.texture.texture != nullptr
#endif
            ) {
                defaultFont = whackyJoeFont;
                NSLog(@"[DEBUG] GetFontDefault: Using Whacky Joe font with ctFont: %p", defaultFont.ctFont);
                initialized = true;
                return defaultFont;
            } else {
                NSLog(@"[DEBUG] GetFontDefault: Whacky Joe font failed to load, falling back to system font");
            }
        }
        
        // Fall back to system font
        if (g_textRenderer) {
            Font fallbackFont = g_textRenderer->LoadSystemFont("Chalkduster", 16);
            if (!fallbackFont.ctFont) {
                fallbackFont = g_textRenderer->LoadSystemFont("Chalkduster-Regular", 16);
                if (fallbackFont.ctFont) {
                    TraceLog(LOG_INFO, "[GetFontDefault] Loaded Chalkduster-Regular");
                }
            } else {
                TraceLog(LOG_INFO, "[GetFontDefault] Loaded Chalkduster");
            }
            defaultFont = fallbackFont;
        } else {
            NSLog(@"[ERROR] GetFontDefault: g_textRenderer not available");
            // Fallback to empty font structure
            defaultFont = { nullptr, 16, 0, 0, {0, 0, 0, 1, 0, nullptr}, nullptr, nullptr };
#if defined(__APPLE__) && TARGET_OS_IPHONE
            defaultFont.fontData = nullptr;
            defaultFont.ctFont = nullptr;
            defaultFont.size = 16;
#endif
        }
        initialized = true;
    }
    
    return defaultFont;
}

Font LoadFont(const char* fileName) {
    NSLog(@"[LOG] LoadFont called with fileName: %s", fileName);
    // Try to load the font file using MetalTextRenderer
    if (g_textRenderer) {
        // Extract font size from the font name or use default
        int fontSize = 16; // Default size
        
        // Try to load as TTF/OTF first
        Font font = g_textRenderer->LoadFont(fileName, fontSize);
        if (font.ctFont != nullptr) {
            NSLog(@"[DEBUG] LoadFont: Successfully loaded font: %s", fileName);
            
            // TODO: Re-enable PNG export once we fix the crash
            // std::string debugPath = "/tmp/font_atlas_" + std::string(fileName) + ".png";
            // g_textRenderer->SaveAtlasToPNG(font, debugPath.c_str());
            // NSLog(@"[DEBUG] LoadFont: Saved atlas for font '%s' to %s", fileName, [NSString stringWithUTF8String:debugPath.c_str()]);
            
            return font;
        }
        
        // If that fails, try to load as system font
        NSString* fontName = [NSString stringWithUTF8String:fileName];
        NSString* baseName = [fontName stringByDeletingPathExtension];
        NSString* extension = [fontName pathExtension];
        
        if ([extension isEqualToString:@"ttf"] || [extension isEqualToString:@"otf"]) {
            // For TTF/OTF files, try to load from bundle
            NSString* bundlePath = [[NSBundle mainBundle] pathForResource:baseName ofType:extension];
            if (bundlePath) {
                font = g_textRenderer->LoadFont([bundlePath UTF8String], fontSize);
                if (font.ctFont != nullptr) {
                    NSLog(@"[DEBUG] LoadFont: Successfully loaded TTF/OTF from bundle: %@", bundlePath);
                    return font;
                }
            }
        }
        
        NSLog(@"[WARNING] LoadFont: Failed to load font: %s, falling back to default", fileName);
    }
    
    // Fall back to default font
    return GetFontDefault();
}

void UnloadFont(Font font) {
    // Not implemented for iOS
}

// Essential text measurement
int MeasureText(const char* text, int fontSize) {
    // Use the more accurate MeasureTextEx for better results
    Vector2 size = MeasureTextEx(GetFontDefault(), text, (float)fontSize, 1.0f);
    return (int)size.x;
}

Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
    if (font.glyphCount == 0) {
        // Fallback for system fonts or invalid fonts
        return (Vector2){ (float)strlen(text) * (fontSize / 2.0f), (float)fontSize };
    }

    float textWidth = 0.0f;
    float scale = fontSize / (float)font.baseSize;

    const int startChar = 32;
    const int endChar = 126;
    int* glyphData = (int*)font.glyphs;

    for (const char* p = text; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < startChar || c > endChar) {
            // For unknown characters, you might want a default advance
            textWidth += (font.recs[0].width * scale); 
            continue;
        }
        int index = c - startChar;
        if (index < 0 || index >= font.glyphCount) {
            textWidth += (font.recs[0].width * scale);
            continue;
        }

        // Add the advance width of the character
        textWidth += glyphData[index * 4 + 3] * scale;
    }

    // Add inter-character spacing
    textWidth += (strlen(text) - 1) * spacing;

    return (Vector2){ textWidth, (float)fontSize };
}

// Essential texture settings
void SetTextureFilter(Texture2D texture, int filter) {
    // Not implemented for iOS
}

void SetTextureWrap(Texture2D texture, int wrap) {
    // Not implemented for iOS
}

// Essential image functions
Image GenImageColor(int width, int height, Color color) {
    // Create a simple colored image
    Image image = { 0 };
    image.width = width;
    image.height = height;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    image.mipmaps = 1;
    
    // Allocate memory for the image data
    int dataSize = width * height * 4; // 4 bytes per pixel (RGBA)
    image.data = malloc(dataSize);
    
    if (image.data) {
        unsigned char* pixels = (unsigned char*)image.data;
        for (int i = 0; i < width * height; i++) {
            pixels[i * 4 + 0] = color.r;
            pixels[i * 4 + 1] = color.g;
            pixels[i * 4 + 2] = color.b;
            pixels[i * 4 + 3] = color.a;
        }
    }
    
    return image;
}

Texture2D LoadTextureFromImage(Image image) {
    // Convert image to texture
    id<MTLDevice> device = (__bridge id<MTLDevice>)PlatformLayer::GetInstance().GetMetalDevice();
    if (!device || !image.data) {
        return { 0, 0, 0, 0, 0 };
    }
    
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = image.width;
    textureDescriptor.height = image.height;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    NSError *error = nil;
    id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
    if (!metalTexture) {
        return { 0, 0, 0, 0, 0 };
    }
    
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)image.width, (NSUInteger)image.height, 1}};
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:image.data bytesPerRow:4 * image.width];
    
    Texture2D texture;
    texture.id = 0;
    texture.texture = (__bridge_retained void*)metalTexture;
    texture.width = image.width;
    texture.height = image.height;
    texture.mipmaps = 1;
    texture.format = image.format;
    
    return texture;
}

Image LoadImageFromTexture(Texture2D texture) {
    // Not implemented for iOS
    Image image = { 0 };
    return image;
}

void UnloadImage(Image image) {
    if (image.data) {
        free(image.data);
    }
}

void ImageResize(Image* image, int newWidth, int newHeight) {
    // Not implemented for iOS
}

// Essential render texture functions
RenderTexture2D LoadRenderTexture(int width, int height) {
    RenderTexture2D renderTexture = { 0 };
    void* metalRenderTexture = PlatformLayer::GetInstance().LoadRenderTexture(width, height);
    if (metalRenderTexture) {
        renderTexture.id = 0; // Not used for Metal
        // Initialize the texture field directly without overwriting
        renderTexture.texture.id = 0; // Not used for Metal
        renderTexture.texture.texture = metalRenderTexture;
        renderTexture.texture.width = width;
        renderTexture.texture.height = height;
        renderTexture.texture.mipmaps = 1;
        renderTexture.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        // Initialize depth texture (not used for Metal)
        renderTexture.depth.id = 0;
        renderTexture.depth.texture = nullptr;
        renderTexture.depth.width = 0;
        renderTexture.depth.height = 0;
        renderTexture.depth.mipmaps = 1;
        renderTexture.depth.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        NSLog(@"[DEBUG] LoadRenderTexture: Successfully created render texture with Metal texture: %p", metalRenderTexture);
    } else {
        NSLog(@"[ERROR] LoadRenderTexture: Failed to create Metal render texture!");
    }
    return renderTexture;
}

// Essential music functions
float GetMusicTimeLength_iOS(Music music) {
    // Get the actual music duration using AVFoundation
    if (music.player) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
        if (player) {
            return (float)player.duration;
        }
    }
    // Fallback for credits - return 123 seconds (2:03)
    return 123.0f;
}

// Essential utility functions
const char* TextFormat(const char* text, ...) {
    // Not implemented for iOS - return a simple copy
    char* result = (char*)malloc(strlen(text) + 1);
    if (result) {
        strcpy(result, text);
    }
    return result;
}

void TraceLog(int logLevel, const char* text, ...) {
    va_list args;
    va_start(args, text);
    NSString* format = [NSString stringWithUTF8String:text];
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    NSLog(@"[TRACE] %@", message);
    
    // Use LogManager for file logging
    std::string cppMessage = [message UTF8String];
    LogManager::GetInstance().Log(cppMessage, "TRACE");
    
    va_end(args);
}

// Essential app lifecycle
void SetConfigFlags(unsigned int flags) {
    // Not implemented for iOS
}

void SetExitKey(int key) {
    // Not implemented for iOS
}

// Essential collision detection functions
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) {
    // Check if circle overlaps with rectangle
    // Find the closest point to the circle within the rectangle
    float closestX = (center.x < rec.x) ? rec.x : (center.x > rec.x + rec.width) ? rec.x + rec.width : center.x;
    float closestY = (center.y < rec.y) ? rec.y : (center.y > rec.y + rec.height) ? rec.y + rec.height : center.y;
    
    // Calculate distance between circle center and closest point
    float distanceX = center.x - closestX;
    float distanceY = center.y - closestY;
    float distanceSquared = distanceX * distanceX + distanceY * distanceY;
    
    // If distance is less than radius, collision occurred
    return distanceSquared <= (radius * radius);
}

bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    // Check if point is inside rectangle
    return (point.x >= rec.x && point.x <= rec.x + rec.width &&
            point.y >= rec.y && point.y <= rec.y + rec.height);
}

bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    // Check if two rectangles overlap
    return !(rec1.x + rec1.width < rec2.x || 
             rec2.x + rec2.width < rec1.x || 
             rec1.y + rec1.height < rec2.y || 
             rec2.y + rec2.height < rec1.y);
}

// Essential drawing function
void BeginDrawing(void) {
    BeginDrawing_iOS(nullptr);
}

extern "C" int GetCurrentFPS() {
    return PlatformLayer::GetInstance().GetLastFPS();
}

extern "C" float GetCurrentFrameTime() {
    return PlatformLayer::GetInstance().GetLastFrameTime();
}

} // extern "C"

// C++ function for music duration (outside extern C block)
float GetMusicDuration(Music music) {
    return GetMusicTimeLength_iOS(music);
}

// iOS-specific input handling functions
void UpdateTouchState(int touchId, float x, float y, bool pressed) {
    // Forward touch state to PlatformLayer
    PlatformLayer::GetInstance().UpdateTouchState();
}

void ClearAllTouchStates(void) {
    // Simple implementation - just call UpdateTouchState to refresh
    PlatformLayer::GetInstance().UpdateTouchState();
}

void UpdateSafeAreaInsets(float top, float right, float bottom, float left) {
    // Simple implementation - safe area insets are handled by UIKit
    // This function is called but we don't need to do anything special
    (void)top; (void)right; (void)bottom; (void)left; // Suppress unused parameter warnings
}

// Window management functions
void CloseWindow(void) {
    // Not applicable on iOS - window is managed by UIKit
}

void EndDrawing(void) {
    // Not applicable on iOS - drawing is managed by Metal
}

Vector2 GetMousePosition(void) {
    // Return touch position converted to mouse position (use index 0 for primary touch)
    return PlatformLayer::GetInstance().GetTouchPosition(0);
}

// Texture functions
Texture2D LoadTexture(const char* fileName) {
    return LoadTexture_iOS(fileName);
}

void UnloadTexture(Texture2D texture) {
    UnloadTexture_iOS(texture);
}

void UnloadRenderTexture(RenderTexture2D target) {
    UnloadRenderTexture_iOS(target);
}

// ========== UTILITY FUNCTIONS ==========

extern "C" float Clamp(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

extern "C" int GetRandomValue(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    
    return min + (arc4random_uniform(max - min + 1));
}

// ========== APP LIFECYCLE ==========

extern "C" void OnAppPause() {
    // iOS-specific pause handling if needed
    // For now, just a stub implementation
}

extern "C" void OnAppResume() {
    // iOS-specific resume handling if needed
    // For now, just a stub implementation
}

#endif // PLATFORM_IOS
