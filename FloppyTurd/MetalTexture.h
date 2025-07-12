#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS

#import <Metal/Metal.h>
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>
#include "PlatformAPI.h"

// C++ namespace for Metal texture operations
namespace MetalTexture {

// Load texture from file path
id<MTLTexture> LoadFromFile(const char* filePath, id<MTLDevice> device);

// Load texture from UIImage
id<MTLTexture> LoadFromUIImage(UIImage* image, id<MTLDevice> device);

// Create texture from CGImage
id<MTLTexture> CreateFromCGImage(CGImageRef cgImage, id<MTLDevice> device);

// Create solid color texture
id<MTLTexture> CreateSolidColor(Color color, int width, int height, id<MTLDevice> device);

// Utility functions
CGSize GetTextureSize(id<MTLTexture> texture);
MTLPixelFormat GetTextureFormat(id<MTLTexture> texture);
bool IsValidTexture(id<MTLTexture> texture);

} // namespace MetalTexture

#endif // defined(__APPLE__) && TARGET_OS_IOS 