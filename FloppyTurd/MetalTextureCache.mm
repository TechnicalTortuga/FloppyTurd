#include "MetalTextureCache.h"
#include <string>
#include "ResourceManager.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import "PlatformAPI.h"
#import "MetalRenderer.h"

// Singleton instance
MetalTextureCache& MetalTextureCache::GetInstance() {
    static MetalTextureCache instance;
    return instance;
}

MetalTextureCache::MetalTextureCache() 
    : m_metalDevice(nullptr)
    , m_commandQueue(nullptr)
    , m_nextTextureId(1)  // Start from 1, 0 is reserved for invalid
{
}

MetalTextureCache::~MetalTextureCache() {
    Shutdown();
}

void MetalTextureCache::Initialize(void* metalDevice) {
    m_metalDevice = metalDevice;
    
    // Create a shared command queue for texture operations
    id<MTLDevice> device = (__bridge id<MTLDevice>)metalDevice;
    if (device) {
        m_commandQueue = (__bridge_retained void*)[device newCommandQueue];
        TraceLog(LOG_INFO, "[INIT] MetalTextureCache created shared command queue: %p", m_commandQueue);
    }
    
    m_textureCache.clear();
    m_textureRefCounts.clear();
    TraceLog(LOG_INFO, "[INIT] MetalTextureCache initialized with device: %p", m_metalDevice);
}

void MetalTextureCache::Shutdown() {
    TraceLog(LOG_INFO, "[SHUTDOWN] Starting MetalTextureCache shutdown");
    UnloadAllTextures();
    
    // Release command queue
    if (m_commandQueue) {
        TraceLog(LOG_INFO, "[SHUTDOWN] Releasing shared command queue: %p", m_commandQueue);
        CFRelease(m_commandQueue);
        m_commandQueue = nullptr;
    }
    
    m_metalDevice = nullptr;
    TraceLog(LOG_INFO, "[SHUTDOWN] MetalTextureCache shutdown complete");
}

Texture2D MetalTextureCache::GetOrLoadTexture(const char* fileName) {
    // DETAILED CORRUPTION DEBUGGING - Check parameter at entry
    TraceLog(LOG_INFO, "[MetalTextureCache] GetOrLoadTexture ENTRY - Raw parameter check");
    
    // Validate input parameter
    if (!fileName) {
        TraceLog(LOG_ERROR, "[ERROR] GetOrLoadTexture called with NULL fileName");
        return CreateFallbackTexture();
    }
    
    // Log detailed info about the received parameter
    TraceLog(LOG_INFO, "[MetalTextureCache] fileName pointer: %p", fileName);
    TraceLog(LOG_INFO, "[MetalTextureCache] strlen(fileName): %zu", strlen(fileName));
    TraceLog(LOG_INFO, "[MetalTextureCache] First char: '%c' (0x%02X)", fileName[0], (unsigned char)fileName[0]);
    TraceLog(LOG_INFO, "[MetalTextureCache] Raw fileName: %s", fileName);
    
    // Immediately convert to NSString to ensure proper memory management in Objective-C++ context
    NSString* nsFileName = [NSString stringWithUTF8String:fileName];
    if (!nsFileName) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to convert fileName to NSString: %s", fileName);
        return CreateFallbackTexture();
    }
    
    // Log the NSString conversion result
    TraceLog(LOG_INFO, "[MetalTextureCache] NSString conversion result: %s", [nsFileName UTF8String]);
    
    // Create std::string for cache lookup (this ensures consistent key format)
    std::string fileNameKey(fileName);
    TraceLog(LOG_INFO, "[MetalTextureCache] std::string conversion result: %s", fileNameKey.c_str());
    
    // Use NSString's UTF8String for all logging to ensure consistent memory
    TraceLog(LOG_INFO, "[MetalTextureCache] GetOrLoadTexture() STARTED with fileName: %s", [nsFileName UTF8String]);
    
    // Check if the texture is already cached
    TraceLog(LOG_INFO, "[MetalTextureCache] Checking texture cache for: %s", [nsFileName UTF8String]);
    auto it = m_textureCache.find(fileNameKey);
    if (it != m_textureCache.end()) {
        TraceLog(LOG_INFO, "[MetalTextureCache] Found cached texture for: %s", [nsFileName UTF8String]);
        // Increment reference count
        m_textureRefCounts[it->second.texture]++;
        TraceLog(LOG_INFO, "[INFO] Using cached texture for %s (refCount: %u)", 
              [nsFileName UTF8String], m_textureRefCounts[it->second.texture]);
        return it->second;
    }
    
    TraceLog(LOG_INFO, "[MetalTextureCache] Texture not in cache, proceeding to load: %s", [nsFileName UTF8String]);
    
    // Texture not in cache, load it
    int width = 0;
    int height = 0;
    Texture2D texture = {0};
    id<MTLDevice> device = (__bridge id<MTLDevice>)m_metalDevice;
    
    if (!device) {
        TraceLog(LOG_ERROR, "[ERROR] Metal device is null in MetalTextureCache");
        return CreateFallbackTexture();
    }
    
    UIImage* uiImage = nil;
    
    if (fileNameKey.substr(0, 8) == "asset://") {
        // Handle asset:// URLs by extracting the resource name
        ResourcePathParts parts = ResourceManager::ParseResourcePath(fileNameKey);
        NSString* name = [NSString stringWithUTF8String:parts.baseName.c_str()];
        uiImage = [UIImage imageNamed:name];
        TraceLog(LOG_INFO, "[TEXTURE] Loading from asset catalog: %s -> %s", fileNameKey.c_str(), parts.baseName.c_str());
    } else {
        // Handle regular file paths - these come from GetResourcePath which already did the lookup
        // Extract the filename and try asset catalog first
        NSString* lastPathComponent = [nsFileName lastPathComponent];
        NSString* baseName = [lastPathComponent stringByDeletingPathExtension];
        
        TraceLog(LOG_INFO, "[TEXTURE] Parsing path: '%s'", [nsFileName UTF8String]);
        TraceLog(LOG_INFO, "[TEXTURE] -> lastPathComponent: '%s'", [lastPathComponent UTF8String]);
        TraceLog(LOG_INFO, "[TEXTURE] -> baseName: '%s'", [baseName UTF8String]);
        
        // First try: Asset catalog lookup using just the base name
        uiImage = [UIImage imageNamed:baseName];
        if (uiImage) {
            TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from asset catalog using base name: %s -> %s", [nsFileName UTF8String], [baseName UTF8String]);
        } else {
            TraceLog(LOG_INFO, "[TEXTURE] ✗ Asset catalog lookup failed for base name: %s", [baseName UTF8String]);
            
            // Second try: Asset catalog with full relative path (without extension)
            NSString* pathWithoutExt = [nsFileName stringByDeletingPathExtension];
            uiImage = [UIImage imageNamed:pathWithoutExt];
            if (uiImage) {
                TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from asset catalog using path without extension: %s", [pathWithoutExt UTF8String]);
            } else {
                TraceLog(LOG_INFO, "[TEXTURE] ✗ Asset catalog lookup failed for path without extension: %s", [pathWithoutExt UTF8String]);
                
                // Third try: Asset catalog with full relative path
                uiImage = [UIImage imageNamed:nsFileName];
                if (uiImage) {
                    TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from asset catalog using full path: %s", [nsFileName UTF8String]);
                } else {
                    TraceLog(LOG_INFO, "[TEXTURE] ✗ Asset catalog lookup failed for full path: %s", [nsFileName UTF8String]);
                    
                    // Fourth try: Direct file path (bundle resources)
                    uiImage = [UIImage imageWithContentsOfFile:nsFileName];
                    if (uiImage) {
                        TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from file path: %s", [nsFileName UTF8String]);
                    } else {
                        TraceLog(LOG_ERROR, "[ERROR] ✗ Failed to load texture from any source: %s", [nsFileName UTF8String]);
                        return CreateFallbackTexture();
                    }
                }
            }
        }
    }
    
    if (!uiImage) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to load UIImage for: %s", [nsFileName UTF8String]);
        return CreateFallbackTexture();
    }
    
    CGImageRef cgImage = uiImage.CGImage;
    if (!cgImage) {
        TraceLog(LOG_ERROR, "[ERROR] CGImage is null for: %s", [nsFileName UTF8String]);
        return CreateFallbackTexture();
    }
    
        width = (int)CGImageGetWidth(cgImage);
        height = (int)CGImageGetHeight(cgImage);
        
        TraceLog(LOG_INFO, "[TEXTURE] Loading texture: %s, dimensions: %dx%d", [nsFileName UTF8String], width, height);
        
        if (width <= 0 || height <= 0) {
            TraceLog(LOG_ERROR, "[ERROR] Invalid dimensions for: %s (w=%d, h=%d)", [nsFileName UTF8String], width, height);
            return CreateFallbackTexture();
        }
        
        // Create Metal texture with proper usage flags
        MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = width;
        textureDescriptor.height = height;
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        textureDescriptor.storageMode = MTLStorageModeShared;
        
        // Support mipmap generation
        textureDescriptor.mipmapLevelCount = 1 + floor(log2(fmax(width, height)));
        
        id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
        if (!metalTexture) {
            TraceLog(LOG_ERROR, "[ERROR] Failed to create Metal texture for: %s", [nsFileName UTF8String]);
            return CreateFallbackTexture();
        }
        
        // Load image data into texture
        MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(nil, width, height, 8, 4 * width, 
                                                    colorSpace, kCGImageAlphaPremultipliedLast);
        
        if (!context) {
            TraceLog(LOG_ERROR, "[ERROR] Failed to create bitmap context for: %s", [nsFileName UTF8String]);
            CGColorSpaceRelease(colorSpace);
            return CreateFallbackTexture();
        }
        
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
        void* imageData = CGBitmapContextGetData(context);
        
        if (!imageData) {
            TraceLog(LOG_ERROR, "[ERROR] Failed to get image data for: %s", [nsFileName UTF8String]);
            CGContextRelease(context);
            CGColorSpaceRelease(colorSpace);
            return CreateFallbackTexture();
        }
        
        [metalTexture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
        
        // Generate mipmaps if needed
        if (textureDescriptor.mipmapLevelCount > 1) {
            id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)m_commandQueue;
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
        
        texture.id = GenerateTextureId();
        texture.texture = (__bridge_retained void*)metalTexture; // Retain the Metal texture
        texture.width = width;
        texture.height = height;
        texture.mipmaps = textureDescriptor.mipmapLevelCount;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    
    // Cache the texture
    m_textureCache[fileNameKey] = texture;
    m_textureRefCounts[texture.texture] = 1; // Initial reference count
    
            TraceLog(LOG_INFO, "[INFO] Loaded and cached new texture for %s (id=%u)", 
          [nsFileName UTF8String], texture.id);
          
    return texture;
}

// ABI-safe NSString version that bypasses const char* corruption issues
Texture2D MetalTextureCache::GetOrLoadTexture(NSString* fileName) {
    TraceLog(LOG_INFO, "[MetalTextureCache] GetOrLoadTexture(NSString*) ENTRY - ABI-safe version");
    
    if (!fileName) {
        TraceLog(LOG_ERROR, "[ERROR] GetOrLoadTexture called with NULL NSString fileName");
        return CreateFallbackTexture();
    }
    
    TraceLog(LOG_INFO, "[MetalTextureCache] NSString parameter received: %s", [fileName UTF8String]);
    
    // Create std::string for cache lookup (this ensures consistent key format)
    std::string fileNameKey([fileName UTF8String]);
    TraceLog(LOG_INFO, "[MetalTextureCache] std::string conversion result: %s", fileNameKey.c_str());
    
    // Use NSString's UTF8String for all logging to ensure consistent memory
    TraceLog(LOG_INFO, "[MetalTextureCache] GetOrLoadTexture() STARTED with fileName: %s", [fileName UTF8String]);
    
    // Check if the texture is already cached
    TraceLog(LOG_INFO, "[MetalTextureCache] Checking texture cache for: %s", [fileName UTF8String]);
    auto it = m_textureCache.find(fileNameKey);
    if (it != m_textureCache.end()) {
        TraceLog(LOG_INFO, "[MetalTextureCache] Found cached texture for: %s", [fileName UTF8String]);
        // Increment reference count
        m_textureRefCounts[it->second.texture]++;
        TraceLog(LOG_INFO, "[INFO] Using cached texture for %s (refCount: %u)", 
              [fileName UTF8String], m_textureRefCounts[it->second.texture]);
        return it->second;
    }
    
    TraceLog(LOG_INFO, "[MetalTextureCache] Texture not in cache, proceeding to load: %s", [fileName UTF8String]);
    
    // Texture not in cache, load it
    int width = 0;
    int height = 0;
    Texture2D texture = {0};
    id<MTLDevice> device = (__bridge id<MTLDevice>)m_metalDevice;
    
    if (!device) {
        TraceLog(LOG_ERROR, "[ERROR] Metal device is null in MetalTextureCache");
        return CreateFallbackTexture();
    }
    
    UIImage* uiImage = nil;
    
    if (fileNameKey.substr(0, 8) == "asset://") {
        // Handle asset:// URLs by extracting the resource name
        ResourcePathParts parts = ResourceManager::ParseResourcePath(fileNameKey);
        NSString* name = [NSString stringWithUTF8String:parts.baseName.c_str()];
        uiImage = [UIImage imageNamed:name];
        TraceLog(LOG_INFO, "[TEXTURE] Loading from asset catalog: %s -> %s", fileNameKey.c_str(), parts.baseName.c_str());
    } else {
        // Handle regular file paths - try asset catalog first with multiple strategies
        NSString* lastPathComponent = [fileName lastPathComponent];
        NSString* baseName = [lastPathComponent stringByDeletingPathExtension];
        
        TraceLog(LOG_INFO, "[TEXTURE] Parsing path: '%s'", [fileName UTF8String]);
        TraceLog(LOG_INFO, "[TEXTURE] -> lastPathComponent: '%s'", [lastPathComponent UTF8String]);
        TraceLog(LOG_INFO, "[TEXTURE] -> baseName: '%s'", [baseName UTF8String]);
        
        // First try: Asset catalog lookup using just the base name
        uiImage = [UIImage imageNamed:baseName];
        if (uiImage) {
            TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from asset catalog using base name: %s -> %s", [fileName UTF8String], [baseName UTF8String]);
        } else {
            TraceLog(LOG_INFO, "[TEXTURE] ✗ Asset catalog lookup failed for base name: %s", [baseName UTF8String]);
            
            // Second try: Asset catalog with full relative path (without extension)
            NSString* pathWithoutExt = [fileName stringByDeletingPathExtension];
            uiImage = [UIImage imageNamed:pathWithoutExt];
            if (uiImage) {
                TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from asset catalog using path without extension: %s", [pathWithoutExt UTF8String]);
            } else {
                TraceLog(LOG_INFO, "[TEXTURE] ✗ Asset catalog lookup failed for path without extension: %s", [pathWithoutExt UTF8String]);
                
                // Third try: Asset catalog with full relative path
                uiImage = [UIImage imageNamed:fileName];
                if (uiImage) {
                    TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from asset catalog using full path: %s", [fileName UTF8String]);
                } else {
                    TraceLog(LOG_INFO, "[TEXTURE] ✗ Asset catalog lookup failed for full path: %s", [fileName UTF8String]);
                    
                    // Fourth try: Direct file path (bundle resources)
                    uiImage = [UIImage imageWithContentsOfFile:fileName];
                    if (uiImage) {
                        TraceLog(LOG_INFO, "[TEXTURE] ✓ Loaded from file path: %s", [fileName UTF8String]);
                    } else {
                        TraceLog(LOG_ERROR, "[ERROR] ✗ Failed to load texture from any source: %s", [fileName UTF8String]);
                        return CreateFallbackTexture();
                    }
                }
            }
        }
    }
    
    if (!uiImage) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to load UIImage for: %s", [fileName UTF8String]);
        return CreateFallbackTexture();
    }
    
    CGImageRef cgImage = uiImage.CGImage;
    if (!cgImage) {
        TraceLog(LOG_ERROR, "[ERROR] CGImage is null for: %s", [fileName UTF8String]);
        return CreateFallbackTexture();
    }
    
    width = (int)CGImageGetWidth(cgImage);
    height = (int)CGImageGetHeight(cgImage);
    
    TraceLog(LOG_INFO, "[TEXTURE] Loading texture: %s, dimensions: %dx%d", [fileName UTF8String], width, height);
    
    if (width <= 0 || height <= 0) {
        TraceLog(LOG_ERROR, "[ERROR] Invalid dimensions for: %s (w=%d, h=%d)", [fileName UTF8String], width, height);
        return CreateFallbackTexture();
    }
    
    // Create Metal texture with proper usage flags
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    textureDescriptor.storageMode = MTLStorageModeShared;
    
    // Support mipmap generation
    textureDescriptor.mipmapLevelCount = 1 + floor(log2(fmax(width, height)));
    
    id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to create Metal texture for: %s", [fileName UTF8String]);
        return CreateFallbackTexture();
    }
    
    // Load image data into texture
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(nil, width, height, 8, 4 * width, 
                                                colorSpace, kCGImageAlphaPremultipliedLast);
    
    if (!context) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to create bitmap context for: %s", [fileName UTF8String]);
        CGColorSpaceRelease(colorSpace);
        return CreateFallbackTexture();
    }
    
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
    void* imageData = CGBitmapContextGetData(context);
    
    if (!imageData) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to get image data for: %s", [fileName UTF8String]);
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        return CreateFallbackTexture();
    }
    
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    // Generate mipmaps if needed
    if (textureDescriptor.mipmapLevelCount > 1) {
        id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)m_commandQueue;
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
    
    // Create Texture2D structure
    texture.id = GenerateTextureId();
    texture.width = width;
    texture.height = height;
    texture.mipmaps = textureDescriptor.mipmapLevelCount;
    texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    texture.texture = (__bridge_retained void*)metalTexture;
    
    // Store in cache with reference counting
    m_textureCache[fileNameKey] = texture;
    m_textureRefCounts[texture.texture] = 1;
    
    TraceLog(LOG_INFO, "[INFO] Loaded and cached new texture for %s (id=%u)", [fileName UTF8String], texture.id);
    
    return texture;
}

Texture2D MetalTextureCache::LoadTextureFromData(void* data, int width, int height, int format) {
    if (!data || width <= 0 || height <= 0) {
        TraceLog(LOG_ERROR, "[ERROR] Invalid texture data: %p, w=%d, h=%d", data, width, height);
        return CreateFallbackTexture();
    }
    
    id<MTLDevice> device = (__bridge id<MTLDevice>)m_metalDevice;
    if (!device) {
        TraceLog(LOG_ERROR, "[ERROR] Metal device is null in LoadTextureFromData");
        return CreateFallbackTexture();
    }
    
    // Create Metal texture descriptor
    MTLTextureDescriptor* textureDesc = [[MTLTextureDescriptor alloc] init];
    textureDesc.pixelFormat = MTLPixelFormatRGBA8Unorm; // Assuming RGBA format
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.usage = MTLTextureUsageShaderRead;
    textureDesc.storageMode = MTLStorageModeShared;
    
    // Support mipmap generation
    textureDesc.mipmapLevelCount = 1 + floor(log2(fmax(width, height)));
    
    id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDesc];
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to create Metal texture in LoadTextureFromData");
        return CreateFallbackTexture();
    }
    
    // Copy data to texture
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:4 * width];
    
    // Generate mipmaps if needed
    if (textureDesc.mipmapLevelCount > 1) {
        id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)m_commandQueue;
        if (commandQueue) {
        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
        [blitEncoder generateMipmapsForTexture:metalTexture];
        [blitEncoder endEncoding];
        [commandBuffer commit];
        }
    }
    
    // Create Texture2D structure
    Texture2D texture;
    texture.id = GenerateTextureId();
    texture.texture = (__bridge_retained void*)metalTexture; // Retain the Metal texture
    texture.width = width;
    texture.height = height;
    texture.mipmaps = textureDesc.mipmapLevelCount;
    texture.format = format;
    
    // Track reference count
    m_textureRefCounts[texture.texture] = 1;
    
            TraceLog(LOG_INFO, "[INFO] Created texture from data: %p (id=%u, w=%d, h=%d)", 
          texture.texture, texture.id, width, height);
          
    return texture;
}

void MetalTextureCache::UnloadTexture(unsigned int textureId) {
    for (auto it = m_textureCache.begin(); it != m_textureCache.end(); ++it) {
        if (it->second.id == textureId) {
            UnloadTexture(it->second.texture);
            return;
        }
    }
}

void MetalTextureCache::UnloadTexture(void* texturePtr) {
    if (!texturePtr) {
        TraceLog(LOG_WARNING, "[WARNING] Attempting to unload null texture pointer");
        return;
    }
    
    // Decrement reference count
    auto it = m_textureRefCounts.find(texturePtr);
    if (it == m_textureRefCounts.end()) {
        TraceLog(LOG_WARNING, "[WARNING] Attempting to unload texture not managed by cache: %p", texturePtr);
        // Release it anyway to prevent leaks
        CFRelease(texturePtr);
        return;
    }
    
    it->second--;
            TraceLog(LOG_INFO, "[TEXTURE] Decremented texture refcount: %p (new count: %u)", texturePtr, it->second);
    
    if (it->second <= 0) {
        // No more references, release the Metal texture
        TraceLog(LOG_INFO, "[TEXTURE] Releasing texture: %p", texturePtr);
        CFRelease(texturePtr); // Release our bridge_retained reference
        
        // Remove from reference count map
        m_textureRefCounts.erase(it);
        
        // Remove from cache
        for (auto cacheIt = m_textureCache.begin(); cacheIt != m_textureCache.end(); /* no increment */) {
            if (cacheIt->second.texture == texturePtr) {
                TraceLog(LOG_INFO, "[TEXTURE] Removing texture from cache: %s", cacheIt->first.c_str());
                cacheIt = m_textureCache.erase(cacheIt);
            } else {
                ++cacheIt;
            }
        }
    }
}

void MetalTextureCache::UnloadAllTextures() {
    TraceLog(LOG_INFO, "[SHUTDOWN] Unloading all textures (%lu textures in cache)", m_textureCache.size());
    
    // Release all Metal textures
    for (auto& it : m_textureRefCounts) {
        if (it.first) {
            TraceLog(LOG_INFO, "[SHUTDOWN] Releasing texture: %p (refCount was: %u)", it.first, it.second);
            CFRelease(it.first);
        } else {
            TraceLog(LOG_WARNING, "[WARNING] Skipping release of null texture pointer (refCount was: %u)", it.second);
        }
    }
    
    // Clear collections
    m_textureCache.clear();
    m_textureRefCounts.clear();
    TraceLog(LOG_INFO, "[SHUTDOWN] All textures released and collections cleared");
}

void MetalTextureCache::GenerateMipmapsForTexture(Texture2D texture) {
    if (!texture.texture) return;
    
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
    id<MTLDevice> device = (__bridge id<MTLDevice>)m_metalDevice;
    
    if (!device) return;
    
    id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)m_commandQueue;
    if (commandQueue) {
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
    [blitEncoder generateMipmapsForTexture:metalTexture];
    [blitEncoder endEncoding];
    [commandBuffer commit];
    
    // Update mipmap count in texture structure
    texture.mipmaps = metalTexture.mipmapLevelCount;
    }
}

unsigned int MetalTextureCache::GenerateTextureId() {
    return m_nextTextureId++;
}

Texture2D MetalTextureCache::CreateFallbackTexture() {
    TraceLog(LOG_INFO, "[INFO] Creating fallback texture");
    
    // Create a small 2x2 magenta/black checkered texture as fallback
    const int fallbackSize = 2;
    const uint32_t fallbackData[] = {
        0xFFFF00FF, 0xFF000000,  // Magenta, Black
        0xFF000000, 0xFFFF00FF   // Black, Magenta
    };
    
    id<MTLDevice> device = (__bridge id<MTLDevice>)m_metalDevice;
    if (!device) {
        TraceLog(LOG_ERROR, "[ERROR] No Metal device available for fallback texture");
        Texture2D emptyTexture = {0};
        return emptyTexture;
    }
    
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = fallbackSize;
    textureDescriptor.height = fallbackSize;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[ERROR] Failed to create fallback Metal texture");
        Texture2D emptyTexture = {0};
        return emptyTexture;
    }
    
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)fallbackSize, (NSUInteger)fallbackSize, 1}};
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:fallbackData bytesPerRow:4 * fallbackSize];
    
    Texture2D fallbackTexture;
    fallbackTexture.id = GenerateTextureId();
    fallbackTexture.texture = (__bridge_retained void*)metalTexture;
    fallbackTexture.width = fallbackSize;
    fallbackTexture.height = fallbackSize;
    fallbackTexture.mipmaps = 1;
    fallbackTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    
    // Track reference count
    m_textureRefCounts[fallbackTexture.texture] = 1;
    
    return fallbackTexture;
}

#endif // defined(__APPLE__) && TARGET_OS_IOS
