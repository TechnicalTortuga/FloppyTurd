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
        std::string assetName = filePath.substr(8); // Remove "asset://" prefix
        NSString* name = [NSString stringWithUTF8String:assetName.c_str()];
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
            id<MTLCommandQueue> commandQueue = [device newCommandQueue];
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
            [blitEncoder generateMipmapsForTexture:metalTexture];
            [blitEncoder endEncoding];
            [commandBuffer commit];
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
        std::string assetName = path.substr(8); // Remove "asset://" prefix
        NSString* name = [NSString stringWithUTF8String:assetName.c_str()];
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

void DrawText_iOS(const char* text, int posX, int posY, int fontSize, unsigned int color) {
    Color raylibColor;
    raylibColor.r = (color >> 24) & 0xFF;
    raylibColor.g = (color >> 16) & 0xFF;
    raylibColor.b = (color >> 8) & 0xFF;
    raylibColor.a = color & 0xFF;
    PlatformLayer::GetInstance().DrawText(text, (float)posX, (float)posY, (float)fontSize, raylibColor, nullptr);
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
    // Validate texture
    if (texture.texture == NULL) {
        NSLog(@"[EXTRA LOG] DrawTexturePro_iOS: SKIP invalid texture");
        return;
    }
    
    // Validate texture pointer
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
    if (!metalTexture) {
        NSLog(@"[ERROR] DrawTexturePro_iOS: Invalid texture (NULL) pointer for texture=%p", texture.texture);
        return;
    }
    
    // Check if texture is still valid
    if (metalTexture.width == 0 || metalTexture.height == 0) {
        NSLog(@"[ERROR] DrawTexturePro_iOS: Texture has invalid dimensions (w=%lu, h=%lu)", (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
        return;
    }
    
    NSLog(@"[DEBUG] DrawTexturePro_iOS: Drawing texture=%p (ptr=%p, w=%lu, h=%lu)", texture.texture, (__bridge void*)metalTexture, (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
    
    // Call the platform layer to draw the texture
    PlatformLayer::GetInstance().DrawTexture((__bridge void*)metalTexture, dest.x, dest.y, dest.width, dest.height, tint);
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

#endif // PLATFORM_IOS

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
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:nil];
    [[AVAudioSession sharedInstance] setActive:YES error:nil];
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
        NSURL* url = [NSURL fileURLWithPath:path];
        NSError* error = nil;
        AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
        if (player && !error) {
            [player prepareToPlay];
            m.player = (__bridge_retained void*)player;
            m.length = (int)(player.duration * 1000);
            GameLog::Log("[AUDIO] Loaded music: %s (duration: %d ms)", fileName, m.length);
        } else {
            GameLog::Log("[AUDIO] ERROR: Failed to load music: %s (%s)", fileName, error.localizedDescription.UTF8String);
        }
    }
    return m;
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
        [player play];
        GameLog::Log("[AUDIO] PlayMusic");
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
        [player play];
        GameLog::Log("[AUDIO] ResumeMusic");
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
