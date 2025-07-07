#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS

// IMPORTANT: This header contains Objective-C++ code and should ONLY be included in .mm files.
// Do NOT include this in any header files that might be included by .cpp files to prevent build errors.
#import <Metal/Metal.h>
#import <CoreText/CoreText.h>
#import <UIKit/UIKit.h>
#include "RaylibCompat.h"
#include <unordered_map>
#include <string>
#include <mutex>

// Text rendering utility for Metal
class MetalTextRenderer {
public:
    MetalTextRenderer();
    ~MetalTextRenderer();
    
    bool Initialize(id<MTLDevice> device);
    void Shutdown();
    
    // Font management
    Font LoadFont(const char* fileName, int fontSize);
    Font LoadSystemFont(const char* fontName, int fontSize);
    void UnloadFont(Font font);
    Font GetDefaultFont();
    
    // Text measurement
    Vector2 MeasureText(const char* text, int fontSize);
    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing);
    
    // Text rendering to texture (for use with Metal renderer)
    id<MTLTexture> RenderTextToTexture(const char* text, Font font, float fontSize, Color color);
    id<MTLTexture> RenderTextToTexture(const char* text, int fontSize, Color color);
    
    // Cache management
    void ClearTextCache();
    
private:
    id<MTLDevice> m_device;
    Font m_defaultFont;
    
    // Text texture cache (moved from static to instance variable)
    std::unordered_map<std::string, id<MTLTexture>> m_textTextureCache;
    std::mutex m_cacheMutex;
    
    // Helper methods
    CTFontRef CreateCTFont(const char* fontName, float fontSize);
    CGSize GetTextSize(const char* text, CTFontRef font, float spacing);
    id<MTLTexture> CreateTextureFromCGImage(CGImageRef image);
};

// Global text renderer instance
extern MetalTextRenderer* g_textRenderer;

#endif // defined(__APPLE__) && TARGET_OS_IOS 