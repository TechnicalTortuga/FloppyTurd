#import "MetalTextRenderer.h"
#import "PlatformLayer.h"
#import "MetalRenderer.h"
#import "MetalTexture.h"
#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>
#import <MobileCoreServices/MobileCoreServices.h>
#include <unordered_map>
#include <string>
#include <mutex>
#include "ResourceManager.h"

#if defined(__APPLE__) && TARGET_OS_IOS

// Global text renderer instance
MetalTextRenderer* g_textRenderer = nullptr;

MetalTextRenderer::MetalTextRenderer() 
    : m_device(nullptr)
{
    // Initialize default font with proper Texture2D structure
    m_defaultFont = {}; // Zero-initialize all fields
    m_defaultFont.font = nullptr;
    m_defaultFont.baseSize = 16;
    m_defaultFont.glyphCount = 0;
    m_defaultFont.glyphPadding = 16;
    m_defaultFont.texture = {0, 0, 0, 1, 0, nullptr}; // id, width, height, mipmaps, format, texture
    m_defaultFont.recs = nullptr;
    m_defaultFont.glyphs = nullptr;
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    m_defaultFont.fontData = nullptr;
    m_defaultFont.ctFont = nullptr;
    m_defaultFont.size = 16;
    m_defaultFont.name = nullptr;
#endif
}

MetalTextRenderer::~MetalTextRenderer() {
    Shutdown();
}
bool MetalTextRenderer::Initialize(id<MTLDevice> device) {
    m_device = device;
    
    // DEBUG: Log all available font family and font names using TraceLog
    NSArray *familyNames = [UIFont familyNames];
    for (NSString *family in familyNames) {
        TraceLog(LOG_INFO, "[FONT DEBUG] Family: %s", [family UTF8String]);
        NSArray *fontNames = [UIFont fontNamesForFamilyName:family];
        for (NSString *name in fontNames) {
            TraceLog(LOG_INFO, "[FONT DEBUG]   Font: %s", [name UTF8String]);
        }
    }
    
    // DEBUG: Also log all available CTFont families (but not individual font names to avoid API issues)
    CFArrayRef ctFamilyNames = CTFontManagerCopyAvailableFontFamilyNames();
    if (ctFamilyNames) {
        CFIndex familyCount = CFArrayGetCount(ctFamilyNames);
        TraceLog(LOG_INFO, "[FONT DEBUG] CTFontManager found %ld font families", (long)familyCount);
        
        for (CFIndex i = 0; i < familyCount; i++) {
            CFStringRef familyName = (CFStringRef)CFArrayGetValueAtIndex(ctFamilyNames, i);
            if (familyName) {
                const char* familyStr = CFStringGetCStringPtr(familyName, kCFStringEncodingUTF8);
                if (familyStr) {
                    TraceLog(LOG_INFO, "[FONT DEBUG] CTFamily: %s", familyStr);
                }
            }
        }
        CFRelease(ctFamilyNames);
    }
    
    // Initialize atlas generator
    if (!m_atlasGenerator.Initialize(device)) {
        TraceLog(LOG_ERROR, "[METAL ERROR] MetalTextRenderer::Initialize: Failed to initialize atlas generator");
        return false;
    }
    
    // Create default font
    m_defaultFont = LoadSystemFont("Helvetica", 16);
    
    return true;
}

void MetalTextRenderer::Shutdown() {
    if (m_defaultFont.ctFont) {
        CFRelease(m_defaultFont.ctFont);
        m_defaultFont.ctFont = nullptr;
    }
        
        // Clear text texture cache
        ClearTextCache();
    
    m_device = nullptr;
        TraceLog(LOG_INFO, "[SHUTDOWN] MetalTextRenderer shutdown complete");
}

Font MetalTextRenderer::LoadFont(const char* fileName, int fontSize) {
        TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: fileName='%s', fontSize=%d", fileName, fontSize);
        
    // Initialize font with proper Texture2D structure
    Font font = {}; // Zero-initialize all fields
    font.font = nullptr;
    font.baseSize = fontSize;
    font.glyphCount = 0;
    font.glyphPadding = fontSize;
    font.texture = {0, 0, 0, 1, 0, nullptr}; // id, width, height, mipmaps, format, texture
    font.recs = nullptr;
    font.glyphs = nullptr;
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    font.fontData = nullptr;
    font.ctFont = nullptr;
    font.size = fontSize;
    font.name = fileName; // Store the font name for debugging
#endif
    
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        
        // Check if this is an asset catalog path
        if ([path hasPrefix:@"asset://"]) {
            std::string cppPath = [path UTF8String];
            ResourcePathParts parts = ResourceManager::ParseResourcePath(cppPath);
            NSString* assetName = [NSString stringWithUTF8String:parts.baseName.c_str()];
            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Loading from asset catalog: %s", [assetName UTF8String]);
            
            // For asset catalog fonts, check if this is a TTF/OTF file that should be loaded from bundle
            // Check both the asset name and the original fileName for TTF/OTF extensions
            NSString* fileExtension = [assetName pathExtension];
            NSString* originalFileName = [NSString stringWithUTF8String:fileName];
            NSString* originalExtension = [originalFileName pathExtension];
            
            bool isTTF = ([fileExtension isEqualToString:@"ttf"] || [fileExtension isEqualToString:@"otf"] ||
                         [originalExtension isEqualToString:@"ttf"] || [originalExtension isEqualToString:@"otf"]);
            
            if (isTTF) {
                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Detected TTF/OTF file, loading from bundle: %s", [assetName UTF8String]);
                
                // Load directly from bundle for TTF/OTF files
                NSString* bundlePath = [[NSBundle mainBundle] pathForResource:assetName ofType:nil];
                if (!bundlePath) {
                    // Try without extension
                    NSString* baseName = [assetName stringByDeletingPathExtension];
                    bundlePath = [[NSBundle mainBundle] pathForResource:baseName ofType:fileExtension];
                }
                if (!bundlePath) {
                    // Try with .ttf extension explicitly
                    NSString* baseName = [assetName stringByDeletingPathExtension];
                    bundlePath = [[NSBundle mainBundle] pathForResource:baseName ofType:@"ttf"];
                }
                if (!bundlePath) {
                    // Try the original fileName (which should have the full name with extension)
                    NSString* originalFileName = [NSString stringWithUTF8String:fileName];
                    NSString* originalBaseName = [originalFileName stringByDeletingPathExtension];
                    bundlePath = [[NSBundle mainBundle] pathForResource:originalBaseName ofType:@"ttf"];
                }
                
                if (bundlePath) {
                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Found bundle path for TTF: %s", [bundlePath UTF8String]);
                    NSURL* fontURL = [NSURL fileURLWithPath:bundlePath];

                    // Register the font with CoreText if not already registered
                    CFErrorRef error = NULL;
                    if (!CTFontManagerRegisterFontsForURL((__bridge CFURLRef)fontURL, kCTFontManagerScopeProcess, &error)) {
                        if (error) {
                            CFStringRef errorDesc = CFErrorCopyDescription(error);
                            TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to register font at %s: %s", [bundlePath UTF8String], CFStringGetCStringPtr(errorDesc, kCFStringEncodingUTF8));
                            CFRelease(errorDesc);
                            CFRelease(error);
                        } else {
                            TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to register font at %s (unknown error)", [bundlePath UTF8String]);
                        }
                    } else {
                        TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully registered font at %s", [bundlePath UTF8String]);
                    }

                    CGDataProviderRef dataProvider = CGDataProviderCreateWithURL((__bridge CFURLRef)fontURL);
                    if (dataProvider) {
                        CGFontRef cgFont = CGFontCreateWithDataProvider(dataProvider);
                        if (cgFont) {
                            font.ctFont = CTFontCreateWithGraphicsFont(cgFont, fontSize, nullptr, nullptr);
                            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully created CTFont from CGFont for TTF: %s", [assetName UTF8String]);
                            
                            // Verify the font is correct
                            CFStringRef psName = CTFontCopyPostScriptName((CTFontRef)font.ctFont);
                            if (psName) {
                                char psNameBuf[128];
                                CFStringGetCString(psName, psNameBuf, sizeof(psNameBuf), kCFStringEncodingUTF8);
                                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: TTF CTFont PostScript name: %s", psNameBuf);
                                CFRelease(psName);
                            }
                            
                            CGFontRelease(cgFont);
                        } else {
                            TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to create CGFont from data provider for TTF: %s", [assetName UTF8String]);
                        }
                        CGDataProviderRelease(dataProvider);
                    } else {
                        TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to create data provider for TTF: %s", [assetName UTF8String]);
                    }
                } else {
                    TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: No bundle path found for TTF: %s", [assetName UTF8String]);
                }
            } else {
                // For non-TTF fonts, try to load as a system font first
                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying to load as system font: %s", [assetName UTF8String]);
                font.ctFont = CTFontCreateWithName((__bridge CFStringRef)assetName, fontSize, nullptr);
                
                // Check if CTFont was created and has glyphs, AND verify it's the correct font
                bool fontValid = false;
                if (font.ctFont && CTFontGetGlyphCount((CTFontRef)font.ctFont) > 0) {
                    // Verify we got the correct font by checking PostScript name
                    CFStringRef psName = CTFontCopyPostScriptName((CTFontRef)font.ctFont);
                    if (psName) {
                        char psNameBuf[128];
                        CFStringGetCString(psName, psNameBuf, sizeof(psNameBuf), kCFStringEncodingUTF8);
                        TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: CTFont PostScript name: %s", psNameBuf);
                        
                        // Check if the PostScript name matches our expected font name (case-insensitive)
                        NSString* expectedName = [assetName stringByReplacingOccurrencesOfString:@"_" withString:@""];
                        NSString* actualName = [NSString stringWithUTF8String:psNameBuf];
                        fontValid = [actualName caseInsensitiveCompare:expectedName] == NSOrderedSame;
                        
                        if (!fontValid) {
                            TraceLog(LOG_WARNING, "[METAL WARNING] LoadFont: Font name mismatch - expected '%s', got '%s'", [expectedName UTF8String], psNameBuf);
                        }
                        CFRelease(psName);
                    }
                }
                
                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Initial font check - ctFont=%p, glyphCount=%d, fontValid=%s", 
                         font.ctFont, font.ctFont ? (int)CTFontGetGlyphCount((CTFontRef)font.ctFont) : 0, fontValid ? "true" : "false");
                
                // If the initial font loading failed or has 0 glyphs, try font name variations
                if (!fontValid) {
                    TraceLog(LOG_WARNING, "[METAL WARNING] LoadFont: Failed to load asset catalog font as system font: %s", [assetName UTF8String]);
                    
                    // Try common variations of the font name
                    NSString* variation1 = [assetName stringByReplacingOccurrencesOfString:@"_" withString:@" "];
                    NSString* variation2 = [assetName stringByReplacingOccurrencesOfString:@"_" withString:@""];
                    NSString* variation3 = [assetName stringByReplacingOccurrencesOfString:@" " withString:@""];
                    NSString* variation4 = [NSString stringWithFormat:@"%@-Regular", assetName];
                    NSString* variation5 = [NSString stringWithFormat:@"%@-Regular", [assetName stringByReplacingOccurrencesOfString:@"_" withString:@" "]];
                    
                    NSArray* variations = @[variation1, variation2, variation3, variation4, variation5];
                    for (NSString* variation in variations) {
                        TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying font name variation: %s", [variation UTF8String]);
                        font.ctFont = CTFontCreateWithName((__bridge CFStringRef)variation, fontSize, nullptr);
                        
                        if (font.ctFont && CTFontGetGlyphCount((CTFontRef)font.ctFont) > 0) {
                            // Verify we got the correct font
                            CFStringRef psName = CTFontCopyPostScriptName((CTFontRef)font.ctFont);
                            if (psName) {
                                char psNameBuf[128];
                                CFStringGetCString(psName, psNameBuf, sizeof(psNameBuf), kCFStringEncodingUTF8);
                                NSString* actualName = [NSString stringWithUTF8String:psNameBuf];
                                NSString* expectedName = [assetName stringByReplacingOccurrencesOfString:@"_" withString:@""];
                                
                                if ([actualName caseInsensitiveCompare:expectedName] == NSOrderedSame) {
                                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font with variation: %s", [variation UTF8String]);
                                    fontValid = true;
                                    CFRelease(psName);
                                    break;
                                } else {
                                    TraceLog(LOG_WARNING, "[METAL WARNING] LoadFont: Font name mismatch for variation '%s' - expected '%s', got '%s'", 
                                            [variation UTF8String], [expectedName UTF8String], psNameBuf);
                                    CFRelease(font.ctFont);
                                    font.ctFont = nullptr;
                                }
                                CFRelease(psName);
                            }
                        }
                    }
                }
            }
        } else {
            // Try to load from bundle (legacy path)
            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying to load from bundle: %s", [path UTF8String]);
            NSString* bundlePath = [[NSBundle mainBundle] pathForResource:path ofType:nil];
            if (!bundlePath) {
                bundlePath = path;
                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Using direct path: %s", [bundlePath UTF8String]);
            } else {
                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Found bundle path: %s", [bundlePath UTF8String]);
            }
            
            NSURL* fontURL = [NSURL fileURLWithPath:bundlePath];
            CGDataProviderRef dataProvider = CGDataProviderCreateWithURL((__bridge CFURLRef)fontURL);
            
            if (dataProvider) {
                CGFontRef cgFont = CGFontCreateWithDataProvider(dataProvider);
                if (cgFont) {
                    font.ctFont = CTFontCreateWithGraphicsFont(cgFont, fontSize, nullptr, nullptr);
                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully created CTFont from CGFont");
                    CGFontRelease(cgFont);
                } else {
                    TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to create CGFont from data provider");
                }
                CGDataProviderRelease(dataProvider);
            } else {
                TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to create data provider from URL: %s", [fontURL.path UTF8String]);
            }
        }
        
        if (!font.ctFont) {
            TraceLog(LOG_WARNING, "[METAL WARNING] Failed to load font: %s, falling back to system font", [path UTF8String]);
            return LoadSystemFont("Helvetica", fontSize);
        } else {
            // Generate font atlas for better performance
            if (GenerateFontAtlas(font)) {
                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font: %s with generated atlas", [path UTF8String]);
            } else {
                TraceLog(LOG_WARNING, "[METAL WARNING] LoadFont: Failed to generate atlas for font: %s, using fallback", [path UTF8String]);
                return LoadSystemFont("Helvetica", fontSize);
            }
        }
    }
    
    return font;
}

Font MetalTextRenderer::LoadSystemFont(const char* fontName, int fontSize) {
        TraceLog(LOG_INFO, "[METAL DEBUG] LoadSystemFont: fontName='%s', fontSize=%d", fontName, fontSize);
        
    // Initialize font with proper Texture2D structure
    Font font = {}; // Zero-initialize all fields
    font.font = nullptr;
    font.baseSize = fontSize;
    font.glyphCount = 0;
    font.glyphPadding = fontSize;
    font.texture = {0, 0, 0, 1, 0, nullptr}; // id, width, height, mipmaps, format, texture
    font.recs = nullptr;
    font.glyphs = nullptr;
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    font.fontData = nullptr;
    font.ctFont = nullptr;
    font.size = fontSize;
    font.name = fontName; // Store the font name for debugging
#endif
    
    @autoreleasepool {
        NSString* name = [NSString stringWithUTF8String:fontName];
        UIFont* uiFont = [UIFont fontWithName:name size:fontSize];
        
        if (!uiFont) {
                TraceLog(LOG_WARNING, "[METAL WARNING] LoadSystemFont: Failed to load UIFont for '%s', falling back to system font", fontName);
            uiFont = [UIFont systemFontOfSize:fontSize];
        }
        
        font.ctFont = CTFontCreateWithName((__bridge CFStringRef)uiFont.fontName, fontSize, nullptr);
            
            if (!font.ctFont) {
                TraceLog(LOG_ERROR, "[METAL ERROR] LoadSystemFont: Failed to create CTFont for font: %s, falling back to Helvetica", fontName);
                return LoadSystemFont("Helvetica", fontSize);
            } else {
                // Generate font atlas for better performance
                if (GenerateFontAtlas(font)) {
                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadSystemFont: Successfully created CTFont for font: %s with generated atlas", fontName);
                } else {
                    TraceLog(LOG_WARNING, "[METAL WARNING] LoadSystemFont: Failed to generate atlas for font: %s, using fallback", fontName);
                    return LoadSystemFont("Helvetica", fontSize);
                }
            }
    }
    
    return font;
}

bool MetalTextRenderer::GenerateFontAtlas(Font& font) {
    TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Using new FontAtlasGenerator");
    
    // Dynamically calculate atlas size
    int glyphCount = 95; // Default to 95 printable ASCII glyphs; adjust if needed
    if (font.glyphCount > 0) glyphCount = font.glyphCount;
    int glyphSize = (int)(font.baseSize * 2.0f) + 4 * 2; // font size * 2 + reduced padding
    int glyphsPerRow = (int)ceil(sqrt((float)glyphCount));
    int minAtlasSize = glyphsPerRow * glyphSize * 2; // double the minimum atlas size for more space
    // Round up to next power of two
    int atlasSize = 256;
    while (atlasSize < minAtlasSize) atlasSize *= 2;
    if (atlasSize > 4096) atlasSize = 4096; // Clamp to 4096 max
    TraceLog(LOG_INFO, "[METAL DEBUG] Dynamic atlas size: %d (glyphSize=%d, glyphs=%d, glyphPadding=4)", atlasSize, glyphSize, glyphCount);
    // Configure atlas generation
    AtlasConfig config;
    config.type = AtlasType::SDF;  // Use SDF for better quality
    config.glyphPadding = 4; // Reduced padding for tighter spacing
    config.distanceRange = 4.0f;
    config.fontSize = font.baseSize * 2.0f;
    config.atlasSize = atlasSize;
    
    // Generate atlas using the new generator
    bool success = m_atlasGenerator.GenerateAtlas(font, config);
    
    if (success) {
        TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Successfully generated SDF atlas");
        // Debug: Save Whacky Joe SDF atlas as PNG if this is the Whacky Joe font
        if (font.name && strstr(font.name, "whacky_joe") != nullptr) {
            TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Saving SDF atlas for Whacky Joe font");
            // Note: We need to save the atlas from FontAtlasGenerator, not from here
            // The actual PNG saving will be done in FontAtlasGenerator::GenerateAtlas
        }
    } else {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Failed to generate atlas with new generator");
        
        // Fallback to bitmap mode if SDF fails
        TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Trying bitmap fallback");
        config.type = AtlasType::BITMAP;
        success = m_atlasGenerator.GenerateAtlas(font, config);
        
        if (success) {
            TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Successfully generated bitmap atlas");
        } else {
            TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Both SDF and bitmap generation failed");
        }
    }
    
    return success;
}

void MetalTextRenderer::UnloadFont(Font font) {
    if (font.ctFont) {
        CFRelease(font.ctFont);
    }
    if (font.glyphs) {
        free(font.glyphs);
    }
    if (font.recs) {
        free(font.recs);
    }
    if (font.texture.texture) {
        // Release the retained Metal texture
        CFBridgingRelease(font.texture.texture);
    }
}

void MetalTextRenderer::SaveAtlasToPNG(const Font& font, const char* fileName) {
    if (!font.texture.texture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] SaveAtlasToPNG: No texture to save");
        return;
    }

    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)font.texture.texture;
    int width = font.texture.width;
    int height = font.texture.height;

    uint8_t* pixelData = (uint8_t*)malloc(width * height * 4);
    [metalTexture getBytes:pixelData bytesPerRow:width * 4 fromRegion:MTLRegionMake2D(0, 0, width, height) mipmapLevel:0];

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(pixelData, width, height, 8, width * 4, colorSpace, kCGImageAlphaPremultipliedLast);
    CGImageRef cgImage = CGBitmapContextCreateImage(context);

    NSString* path = [NSString stringWithUTF8String:fileName];
    NSURL* url = [NSURL fileURLWithPath:path];
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, kUTTypePNG, 1, nullptr);
    if (destination) {
        CGImageDestinationAddImage(destination, cgImage, nullptr);
        CGImageDestinationFinalize(destination);
        CFRelease(destination);
        TraceLog(LOG_INFO, "[METAL DEBUG] SaveAtlasToPNG: Saved atlas to %s", fileName);
    } else {
        TraceLog(LOG_ERROR, "[METAL ERROR] SaveAtlasToPNG: Failed to create image destination for %s", fileName);
    }

    CGImageRelease(cgImage);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    free(pixelData);
}

Font MetalTextRenderer::GetDefaultFont() {
    return m_defaultFont;
}

Vector2 MetalTextRenderer::MeasureText(const char* text, int fontSize) {
    if (!text) return {0, 0};
    
    Font tempFont = LoadSystemFont("Helvetica", fontSize);
    Vector2 size = MeasureTextEx(tempFont, text, fontSize, 0);
    UnloadFont(tempFont);
    
    return size;
}

Vector2 MetalTextRenderer::MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
    if (!text || !font.ctFont) return {0, 0};
    
    CGSize size = GetTextSize(text, (CTFontRef)font.ctFont, spacing);
    float scale = fontSize / font.size;
    
    return {static_cast<float>(size.width * scale), static_cast<float>(size.height * scale)};
}

CTFontRef MetalTextRenderer::CreateCTFont(const char* fontName, float fontSize) {
    @autoreleasepool {
        NSString* name = [NSString stringWithUTF8String:fontName];
        return CTFontCreateWithName((__bridge CFStringRef)name, fontSize, nullptr);
    }
}

CGSize MetalTextRenderer::GetTextSize(const char* text, CTFontRef font, float spacing) {
    if (!text || !font) return CGSizeZero;
    
    @autoreleasepool {
        NSString* string = [NSString stringWithUTF8String:text];
        
        // Create attributed string
        NSDictionary* attributes = @{
            (NSString*)kCTFontAttributeName: (__bridge id)font
        };
        
        NSAttributedString* attributedString = [[NSAttributedString alloc] initWithString:string attributes:attributes];
        
        // Create frame setter
        CTFramesetterRef frameSetter = CTFramesetterCreateWithAttributedString((__bridge CFAttributedStringRef)attributedString);
        
        // Get size
        CGSize size = CTFramesetterSuggestFrameSizeWithConstraints(frameSetter, CFRangeMake(0, 0), nullptr, CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX), nullptr);
        
        CFRelease(frameSetter);
        
        // Add spacing
        if (spacing > 0 && string.length > 1) {
            size.width += (string.length - 1) * spacing;
        }
        
        return size;
    }
}

id<MTLTexture> MetalTextRenderer::RenderTextToTexture(const char* text, Font font, float fontSize, Color color) {
        char key[256];
        snprintf(key, sizeof(key), "%s_%d_%d_%d_%d_%d", text, (int)fontSize, color.r, color.g, color.b, color.a);
        std::string cacheKey = key;
        
        // Thread-safe cache lookup
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            auto it = m_textTextureCache.find(cacheKey);
            if (it != m_textTextureCache.end()) {
                TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: Cache hit for '%s'", key);
                return it->second;
            }
        }
        
        TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: Cache miss for '%s'", key);
        TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: text='%s', font.ctFont=%p, fontSize=%.2f, color=(%d,%d,%d,%d)", text, font.ctFont, fontSize, color.r, color.g, color.b, color.a);
        
    if (!text || !font.ctFont || !m_device) {
            TraceLog(LOG_ERROR, "[METAL ERROR] RenderTextToTexture: Invalid input (text or font.ctFont or m_device is null)\ntext='%s', font.ctFont=%p, m_device=%p", text, font.ctFont, m_device);
        return nullptr;
    }
        
        // Clamp font size to minimum for readability
        float clampedFontSize = fmaxf(fontSize, 16.0f);
        if (clampedFontSize != fontSize) {
            TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: Clamped fontSize from %.1f to %.1f", fontSize, clampedFontSize);
            fontSize = clampedFontSize;
        }
    
    @autoreleasepool {
        NSString* string = [NSString stringWithUTF8String:text];
        
        // Scale font if needed
        CTFontRef scaledFont = (CTFontRef)font.ctFont;
        if (fontSize != font.size) {
            scaledFont = CTFontCreateCopyWithAttributes((CTFontRef)font.ctFont, fontSize, nullptr, nullptr);
                TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: Created scaled font from %d to %.1f", font.size, fontSize);
        }
        
        // Get text size
        CGSize textSize = GetTextSize(text, scaledFont, 0);
            TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: Calculated text size: %.1fx%.1f", textSize.width, textSize.height);
            
        if (textSize.width <= 0 || textSize.height <= 0) {
                TraceLog(LOG_WARNING, "[METAL WARNING] RenderTextToTexture: Calculated text size is zero or negative for text: '%s'. Returning nil.", text);
            if (scaledFont != font.ctFont) CFRelease(scaledFont);
            return nullptr;
        }
        
        // Create bitmap context
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        size_t width = (size_t)ceil(textSize.width);
        size_t height = (size_t)ceil(textSize.height);
        
            uint8_t* pixelData = (uint8_t*)calloc(width * height * 4, 1); // Zeroed for transparency
        CGContextRef context = CGBitmapContextCreate(pixelData, width, height, 8, width * 4, colorSpace, kCGImageAlphaPremultipliedLast);
            TraceLog(LOG_INFO, "[METAL DEBUG] CGContext created: %p, size=%zux%zu", context, width, height);
        
        if (!context) {
                TraceLog(LOG_ERROR, "[METAL ERROR] RenderTextToTexture: Failed to create CGContext");
            free(pixelData);
            CGColorSpaceRelease(colorSpace);
            if (scaledFont != font.ctFont) CFRelease(scaledFont);
            return nullptr;
        }
            
            // Clear context to transparent (already zeroed, but ensure)
            CGContextClearRect(context, CGRectMake(0, 0, width, height));
            
            // Flip coordinate system for Core Text (only once)
            CGContextSetTextMatrix(context, CGAffineTransformIdentity);
            CGContextTranslateCTM(context, 0, height);
            CGContextScaleCTM(context, 1.0, -1.0);
            TraceLog(LOG_INFO, "[METAL DEBUG] CGContext transform: flipped Y, height=%zu", height);
        
        // Set text color
        CGFloat components[] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        CGColorRef textColor = CGColorCreate(colorSpace, components);
        
        // Create attributed string
        NSDictionary* attributes = @{
            (NSString*)kCTFontAttributeName: (__bridge id)scaledFont,
            (NSString*)kCTForegroundColorAttributeName: (__bridge id)textColor
        };
        NSAttributedString* attributedString = [[NSAttributedString alloc] initWithString:string attributes:attributes];
        
        // Create line and draw
        CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attributedString);
        
            // Get baseline offset
            CGFloat ascent = 0, descent = 0, leading = 0;
            double lineWidth = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
            TraceLog(LOG_INFO, "[METAL DEBUG] Text baseline: ascent=%.2f, descent=%.2f, leading=%.2f", ascent, descent, leading);
        
            // Draw text at (0, ascent) to align baseline
            CGContextSetTextPosition(context, 0, ascent);
        CTLineDraw(line, context);
        
        // Create texture from bitmap
        id<MTLTexture> texture = CreateTextureFromCGImage(CGBitmapContextCreateImage(context));
        
        // Cleanup
        CFRelease(line);
        CGColorRelease(textColor);
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        free(pixelData);
        
        if (scaledFont != font.ctFont) CFRelease(scaledFont);
        
        if (texture) {
                // Thread-safe cache insertion
                {
                    std::lock_guard<std::mutex> lock(m_cacheMutex);
                    m_textTextureCache[cacheKey] = texture;
                }
                TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: Cached Metal texture %p for key '%s'", texture, key);
        } else {
                TraceLog(LOG_ERROR, "[METAL ERROR] RenderTextToTexture: Failed to create Metal texture for text '%s' from CGBitmapContext", text);
        }
        
        return texture;
    }
}

id<MTLTexture> MetalTextRenderer::RenderTextToTexture(const char* text, int fontSize, Color color) {
    return RenderTextToTexture(text, m_defaultFont, fontSize, color);
}

id<MTLTexture> MetalTextRenderer::RenderTextToTexture(const char* text, int fontSize, Color color, Font* font) {
    if (!font) {
        TraceLog(LOG_WARNING, "[METAL WARNING] RenderTextToTexture: Font pointer is null, using default font");
        return RenderTextToTexture(text, fontSize, color);
    }
    
    TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture with font pointer: text='%s', fontSize=%d, font.ctFont=%p, font.glyphCount=%d", 
             text, fontSize, font->ctFont, font->glyphCount);
    
    return RenderTextToTexture(text, *font, fontSize, color);
}

id<MTLTexture> MetalTextRenderer::CreateTextureFromCGImage(CGImageRef image) {
    if (!image || !m_device) return nullptr;
    
    return MetalTexture::CreateFromCGImage(image, m_device);
}

void MetalTextRenderer::ClearTextCache() {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        TraceLog(LOG_INFO, "[CLEANUP] Clearing text texture cache (%lu textures)", m_textTextureCache.size());
        m_textTextureCache.clear(); // ARC will release the Metal textures
    }

// C function bridge for loading system fonts from C++ code
extern "C" Font LoadSystemFontForUI(const char* fontName, float fontSize) {
    // Get the global text renderer instance
    if (g_textRenderer) {
        return g_textRenderer->LoadSystemFont(fontName, fontSize);
    } else {
        // Fallback to default font if text renderer is not available
        Font defaultFont = { 0 };
        defaultFont.baseSize = fontSize;
        return defaultFont;
    }
}

#endif // defined(__APPLE__) && TARGET_OS_IOS 