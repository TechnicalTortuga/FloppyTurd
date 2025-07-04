#include "RaylibCompat.h"
#ifdef PLATFORM_IOS
#include <Metal/Metal.h>
#include <Foundation/Foundation.h>
#include "PlatformLayer.h"

// iOS-specific implementations using Metal and Objective-C++
void* LoadTexture_iOS(const char* fileName, int* width, int* height) {
    return PlatformLayer::GetInstance().LoadTexture(fileName, width, height);
}

void UnloadTexture_iOS(void* texture) {
    PlatformLayer::GetInstance().UnloadTexture(texture);
}

Texture2D LoadTexture_iOS(const char *fileName)
{
    Texture2D texture = { 0 };
    int width, height;
    void* texturePtr = PlatformLayer::GetInstance().LoadTexture(fileName, &width, &height);
    if (texturePtr) {
        texture.id = (unsigned int)(uintptr_t)texturePtr;
        texture.width = width;
        texture.height = height;
        texture.mipmaps = 1;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    }
    return texture;
}

void UnloadTexture_iOS(Texture2D texture)
{
    if (texture.id) {
        PlatformLayer::GetInstance().UnloadTexture((void*)(uintptr_t)texture.id);
    }
}

Texture2D LoadTextureFromImage_iOS(Image image)
{
    if (!image.data) {
        TraceLog(LOG_ERROR, "Cannot load texture from null image");
        return Texture2D{0, 0, 0, 0, 0};
    }
    
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:image.width height:image.height mipmapped:NO];
    id<MTLTexture> metalTexture = [PlatformLayer::GetInstance().GetDelegate().device newTextureWithDescriptor:textureDesc];
    
    MTLRegion region = MTLRegionMake2D(0, 0, image.width, image.height);
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:image.data bytesPerRow:image.width * 4];
    
    Texture2D texture;
    texture.id = (unsigned int)(uintptr_t)(__bridge void*)metalTexture;
    texture.width = image.width;
    texture.height = image.height;
    texture.mipmaps = 1;
    texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return texture;
}

Image LoadImage_iOS(const char* fileName)
{
    std::string path = fileName;
    UIImage* uiImage = [UIImage imageNamed:[NSString stringWithUTF8String:path.c_str()]];
    if (!uiImage) {
        // Fallback to file system path if not found in bundle
        uiImage = [UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
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
    // Normalize source rectangle if flipped
    Rectangle normalizedSource = source;
    if (source.width < 0) {
        normalizedSource.x += source.width;
        normalizedSource.width = -source.width;
    }
    if (source.height < 0) {
        normalizedSource.y += source.height;
        normalizedSource.height = -source.height;
    }

    // Calculate vertices with rotation and origin
    float width = dest.width;
    float height = dest.height;
    float x = dest.x;
    float y = dest.y;
    float ox = origin.x;
    float oy = origin.y;

    // Apply rotation (CPU side for simplicity, could be moved to shader for performance)
    float cosRot = cosf(rotation * DEG2RAD);
    float sinRot = sinf(rotation * DEG2RAD);

    // Define vertex positions relative to origin, rotated, then offset by position
    struct Vertex {
        float x, y;
        float u, v;
        float r, g, b, a;
    };

    float left = -ox;
    float right = width - ox;
    float top = -oy;
    float bottom = height - oy;

    Vertex vertices[4] = {
        // Top-left
        {
            x + (left * cosRot - top * sinRot),
            y + (left * sinRot + top * cosRot),
            source.x / texture.width, 
            source.y / texture.height,
            tint.r / 255.f, tint.g / 255.f, tint.b / 255.f, tint.a / 255.f
        },
        // Top-right
        {
            x + (right * cosRot - top * sinRot),
            y + (right * sinRot + top * cosRot),
            (source.x + source.width) / texture.width, 
            source.y / texture.height,
            tint.r / 255.f, tint.g / 255.f, tint.b / 255.f, tint.a / 255.f
        },
        // Bottom-right
        {
            x + (right * cosRot - bottom * sinRot),
            y + (right * sinRot + bottom * cosRot),
            (source.x + source.width) / texture.width, 
            (source.y + source.height) / texture.height,
            tint.r / 255.f, tint.g / 255.f, tint.b / 255.f, tint.a / 255.f
        },
        // Bottom-left
        {
            x + (left * cosRot - bottom * sinRot),
            y + (left * sinRot + bottom * cosRot),
            source.x / texture.width, 
            (source.y + source.height) / texture.height,
            tint.r / 255.f, tint.g / 255.f, tint.b / 255.f, tint.a / 255.f
        }
    };

    // Enqueue draw command via PlatformLayer
    PlatformLayer::GetInstance().EnqueueDrawCommand(vertices, (void*)(uintptr_t)texture.id, 4);
}

// Add other iOS-specific raylib compatibility functions here as needed
#endif
