#import "MetalTexture.h"
#import "MetalRenderer.h"
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>

#if defined(__APPLE__) && TARGET_OS_IOS

@interface MetalTextureLoader : NSObject
+ (id<MTLTexture>)loadTextureFromFile:(NSString*)filePath device:(id<MTLDevice>)device;
+ (id<MTLTexture>)loadTextureFromUIImage:(UIImage*)image device:(id<MTLDevice>)device;
+ (id<MTLTexture>)createTextureFromCGImage:(CGImageRef)cgImage device:(id<MTLDevice>)device;
+ (id<MTLTexture>)createSolidColorTexture:(Color)color size:(CGSize)size device:(id<MTLDevice>)device;
@end

@implementation MetalTextureLoader

+ (id<MTLTexture>)loadTextureFromFile:(NSString*)filePath device:(id<MTLDevice>)device {
    @autoreleasepool {
        // Extract the filename without path and extension for asset catalog lookup
        NSString* fileName = [filePath lastPathComponent];
        NSString* assetName = [fileName stringByDeletingPathExtension];
        
        // First try to load from asset catalog using the asset name
        UIImage* image = [UIImage imageNamed:assetName];
        if (image) {
            TraceLog(LOG_INFO, "[TEXTURE] Loaded texture from asset catalog: %s", [assetName UTF8String]);
            return [self loadTextureFromUIImage:image device:device];
        }
        
        // Try with the full path in asset catalog
        image = [UIImage imageNamed:filePath];
        if (image) {
            TraceLog(LOG_INFO, "[TEXTURE] Loaded texture from asset catalog with path: %s", [filePath UTF8String]);
            return [self loadTextureFromUIImage:image device:device];
        }
        
        // Fallback: try to load from bundle path
        NSString* bundlePath = [[NSBundle mainBundle] pathForResource:filePath ofType:nil];
        if (bundlePath) {
            image = [UIImage imageWithContentsOfFile:bundlePath];
            if (image) {
                TraceLog(LOG_INFO, "[TEXTURE] Loaded texture from bundle path: %s", [bundlePath UTF8String]);
                return [self loadTextureFromUIImage:image device:device];
            }
        }
        
        // Last resort: try the original file path
        image = [UIImage imageWithContentsOfFile:filePath];
        if (image) {
            TraceLog(LOG_INFO, "[TEXTURE] Loaded texture from file path: %s", [filePath UTF8String]);
            return [self loadTextureFromUIImage:image device:device];
        }
        
        TraceLog(LOG_ERROR, "[TEXTURE ERROR] Failed to load texture: %s (tried asset: %s, path: %s)", [filePath UTF8String], [assetName UTF8String], [bundlePath UTF8String]);
        return nil;
    }
}

+ (id<MTLTexture>)loadTextureFromUIImage:(UIImage*)image device:(id<MTLDevice>)device {
    if (!image || !device) {
        return nil;
    }
    
    CGImageRef cgImage = image.CGImage;
    if (!cgImage) {
        return nil;
    }
    
    return [self createTextureFromCGImage:cgImage device:device];
}

+ (id<MTLTexture>)createTextureFromCGImage:(CGImageRef)cgImage device:(id<MTLDevice>)device {
    if (!cgImage || !device) {
        return nil;
    }
    
    @autoreleasepool {
        size_t width = CGImageGetWidth(cgImage);
        size_t height = CGImageGetHeight(cgImage);
        
        TraceLog(LOG_INFO, "[TEXTURE] Loading texture: %zux%zu", width, height);
        
        // Create texture descriptor
        MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = width;
        textureDescriptor.height = height;
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        
        // Create texture
        id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
        if (!texture) {
            TraceLog(LOG_ERROR, "[ERROR] Failed to create Metal texture");
            return nil;
        }
        
        // Create bitmap context to extract pixel data
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        uint8_t* rawData = (uint8_t*)malloc(width * height * 4);
        
        CGContextRef context = CGBitmapContextCreate(rawData, width, height, 8, width * 4,
                                                   colorSpace, kCGImageAlphaPremultipliedLast);
        
        CGColorSpaceRelease(colorSpace);
        
        if (!context) {
            free(rawData);
            TraceLog(LOG_ERROR, "Failed to create bitmap context");
            return nil;
        }
        
        // Draw image into context
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
        CGContextRelease(context);
        
        // Upload pixel data to texture
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [texture replaceRegion:region mipmapLevel:0 withBytes:rawData bytesPerRow:width * 4];
        
        free(rawData);
        
        return texture;
    }
}

+ (id<MTLTexture>)createSolidColorTexture:(Color)color size:(CGSize)size device:(id<MTLDevice>)device {
    if (!device || size.width <= 0 || size.height <= 0) {
        return nil;
    }
    
    @autoreleasepool {
        NSUInteger width = (NSUInteger)size.width;
        NSUInteger height = (NSUInteger)size.height;
        
        // Create texture descriptor
        MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = width;
        textureDescriptor.height = height;
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        
        // Create texture
        id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
        if (!texture) {
            return nil;
        }
        
        // Create solid color data
        size_t dataSize = width * height * 4;
        uint8_t* pixelData = (uint8_t*)malloc(dataSize);
        
        for (NSUInteger i = 0; i < width * height; i++) {
            pixelData[i * 4 + 0] = color.r;     // Red
            pixelData[i * 4 + 1] = color.g;     // Green
            pixelData[i * 4 + 2] = color.b;     // Blue
            pixelData[i * 4 + 3] = color.a;     // Alpha
        }
        
        // Upload pixel data to texture
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [texture replaceRegion:region mipmapLevel:0 withBytes:pixelData bytesPerRow:width * 4];
        
        free(pixelData);
        
        return texture;
    }
}

@end

// C++ wrapper functions
namespace MetalTexture {

id<MTLTexture> LoadFromFile(const char* filePath, id<MTLDevice> device) {
    if (!filePath || !device) {
        return nil;
    }
    
    NSString* path = [NSString stringWithUTF8String:filePath];
    return [MetalTextureLoader loadTextureFromFile:path device:device];
}

id<MTLTexture> LoadFromUIImage(UIImage* image, id<MTLDevice> device) {
    return [MetalTextureLoader loadTextureFromUIImage:image device:device];
}

id<MTLTexture> CreateFromCGImage(CGImageRef cgImage, id<MTLDevice> device) {
    return [MetalTextureLoader createTextureFromCGImage:cgImage device:device];
}

id<MTLTexture> CreateSolidColor(Color color, int width, int height, id<MTLDevice> device) {
    return [MetalTextureLoader createSolidColorTexture:color 
                                                  size:CGSizeMake(width, height) 
                                                device:device];
}

CGSize GetTextureSize(id<MTLTexture> texture) {
    if (!texture) {
        return CGSizeZero;
    }
    
    return CGSizeMake(texture.width, texture.height);
}

MTLPixelFormat GetTextureFormat(id<MTLTexture> texture) {
    if (!texture) {
        return MTLPixelFormatInvalid;
    }
    
    return texture.pixelFormat;
}

bool IsValidTexture(id<MTLTexture> texture) {
    return texture != nil && texture.width > 0 && texture.height > 0;
}

} // namespace MetalTexture

#endif // defined(__APPLE__) && TARGET_OS_IOS 