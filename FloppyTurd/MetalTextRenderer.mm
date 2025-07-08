#import "MetalTextRenderer.h"
#import "PlatformLayer.h"
#import "MetalRenderer.h"
#import "MetalTexture.h"
#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>
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
#endif
}

MetalTextRenderer::~MetalTextRenderer() {
    Shutdown();
}
bool MetalTextRenderer::Initialize(id<MTLDevice> device) {
    m_device = device;
    
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
#endif
    
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        
        // Check if this is an asset catalog path
        if ([path hasPrefix:@"asset://"]) {
            std::string cppPath = [path UTF8String];
            ResourcePathParts parts = ResourceManager::ParseResourcePath(cppPath);
            NSString* assetName = [NSString stringWithUTF8String:parts.baseName.c_str()];
            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Loading from asset catalog: %s", [assetName UTF8String]);
            
            // For asset catalog fonts, we need to load them differently
            // Try to load as a system font first (many fonts are available as system fonts)
            font.ctFont = CTFontCreateWithName((__bridge CFStringRef)assetName, fontSize, nullptr);
            
            // Check if CTFont was created and has glyphs
            bool fontValid = font.ctFont && CTFontGetGlyphCount((CTFontRef)font.ctFont) > 0;
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
                
                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying font name variation: %s", [variation1 UTF8String]);
                font.ctFont = CTFontCreateWithName((__bridge CFStringRef)variation1, fontSize, nullptr);
                if (font.ctFont) {
                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font with variation: %s", [variation1 UTF8String]);
                } else {
                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying font name variation: %s", [variation2 UTF8String]);
                    font.ctFont = CTFontCreateWithName((__bridge CFStringRef)variation2, fontSize, nullptr);
                    if (font.ctFont) {
                        TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font with variation: %s", [variation2 UTF8String]);
                    } else {
                        TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying font name variation: %s", [variation3 UTF8String]);
                        font.ctFont = CTFontCreateWithName((__bridge CFStringRef)variation3, fontSize, nullptr);
                        if (font.ctFont) {
                            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font with variation: %s", [variation3 UTF8String]);
                        } else {
                            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying font name variation: %s", [variation4 UTF8String]);
                            font.ctFont = CTFontCreateWithName((__bridge CFStringRef)variation4, fontSize, nullptr);
                            if (font.ctFont) {
                                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font with variation: %s", [variation4 UTF8String]);
                            } else {
                                TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Trying font name variation: %s", [variation5 UTF8String]);
                                font.ctFont = CTFontCreateWithName((__bridge CFStringRef)variation5, fontSize, nullptr);
                                if (font.ctFont) {
                                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font with variation: %s", [variation5 UTF8String]);
                                }
                            }
                        }
                    }
                }
                // If not available as system font, try to load from bundle
                NSString* bundlePath = [[NSBundle mainBundle] pathForResource:assetName ofType:@"ttf"];
                if (!bundlePath) {
                    bundlePath = [[NSBundle mainBundle] pathForResource:assetName ofType:@"otf"];
                }
                
                if (bundlePath) {
                    TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Found bundle path for font: %s", [bundlePath UTF8String]);
                    NSURL* fontURL = [NSURL fileURLWithPath:bundlePath];
                    CGDataProviderRef dataProvider = CGDataProviderCreateWithURL((__bridge CFURLRef)fontURL);
                    
                    if (dataProvider) {
                        CGFontRef cgFont = CGFontCreateWithDataProvider(dataProvider);
                        if (cgFont) {
                            font.ctFont = CTFontCreateWithGraphicsFont(cgFont, fontSize, nullptr, nullptr);
                            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully created CTFont from CGFont for asset: %s", [assetName UTF8String]);
                            CGFontRelease(cgFont);
                        } else {
                            TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to create CGFont from data provider for asset: %s", [assetName UTF8String]);
                        }
                        CGDataProviderRelease(dataProvider);
                    } else {
                        TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: Failed to create data provider for asset: %s", [assetName UTF8String]);
                    }
                } else {
                    TraceLog(LOG_ERROR, "[METAL ERROR] LoadFont: No bundle path found for asset: %s", [assetName UTF8String]);
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
    if (!font.ctFont || !m_device) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Invalid font or device");
        return false;
    }
    
    TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Generating atlas for font with %d glyphs", (int)CTFontGetGlyphCount((CTFontRef)font.ctFont));
    
    // Define the character set to include in the atlas (ASCII printable characters)
    const int startChar = 32;  // Space
    const int endChar = 126;   // Tilde
    const int charCount = endChar - startChar + 1;
    
    // Calculate atlas size (power of 2 for better GPU performance)
    const int glyphSize = font.baseSize + 4; // Add padding
    const int atlasSize = 512; // Start with 512x512, can be increased if needed
    const int glyphsPerRow = atlasSize / glyphSize;
    const int glyphsPerCol = atlasSize / glyphSize;
    
    if (charCount > glyphsPerRow * glyphsPerCol) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Too many characters for atlas size");
        return false;
    }
    
    // Create bitmap context for the atlas
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    uint8_t* pixelData = (uint8_t*)calloc(atlasSize * atlasSize * 4, 1); // Zeroed for transparency
    CGContextRef context = CGBitmapContextCreate(pixelData, atlasSize, atlasSize, 8, atlasSize * 4, colorSpace, kCGImageAlphaPremultipliedLast);
    
    if (!context) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Failed to create CGContext");
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        return false;
    }
    
    // Clear context to transparent
    CGContextClearRect(context, CGRectMake(0, 0, atlasSize, atlasSize));
    
    // Flip coordinate system for Core Text
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    CGContextTranslateCTM(context, 0, atlasSize);
    CGContextScaleCTM(context, 1.0, -1.0);
    
    // Set text color to white
    CGFloat components[] = {1.0f, 1.0f, 1.0f, 1.0f};
    CGColorRef textColor = CGColorCreate(colorSpace, components);
    
    // Allocate glyph data arrays
    font.glyphCount = charCount;
    font.glyphs = (void*)malloc(charCount * sizeof(int) * 4); // Simple array of ints for glyph data
    font.recs = (Rectangle*)malloc(charCount * sizeof(Rectangle));
    
    if (!font.glyphs || !font.recs) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Failed to allocate glyph arrays");
        CGContextRelease(context);
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        CGColorRelease(textColor);
        return false;
    }
    
    // Render each character to the atlas
    for (int i = 0; i < charCount; i++) {
        int charCode = startChar + i;
        int row = i / glyphsPerRow;
        int col = i % glyphsPerRow;
        
        int x = col * glyphSize + 2; // Add 2px padding
        int y = row * glyphSize + 2;
        
        // Convert character to string
        char charStr[2] = {(char)charCode, 0};
        NSString* string = [NSString stringWithUTF8String:charStr];
        
        // Create attributed string
        NSDictionary* attributes = @{
            (NSString*)kCTFontAttributeName: (__bridge id)font.ctFont,
            (NSString*)kCTForegroundColorAttributeName: (__bridge id)textColor
        };
        NSAttributedString* attributedString = [[NSAttributedString alloc] initWithString:string attributes:attributes];
        
        // Create line and draw
        CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attributedString);
        CGContextSetTextPosition(context, x, y + font.baseSize); // Adjust Y position for baseline
        CTLineDraw(line, context);
        
        // Get glyph metrics
        CFArrayRef glyphRuns = CTLineGetGlyphRuns(line);
        if (CFArrayGetCount(glyphRuns) > 0) {
            CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(glyphRuns, 0);
            CFIndex glyphCount = CTRunGetGlyphCount(run);
            
            if (glyphCount > 0) {
                CGGlyph glyph;
                CTRunGetGlyphs(run, CFRangeMake(0, 1), &glyph);
                
                // Get glyph bounds
                CGRect bounds;
                CTFontGetBoundingRectsForGlyphs((CTFontRef)font.ctFont, kCTFontHorizontalOrientation, &glyph, &bounds, 1);
                
                // Store glyph info in simple int array
                int* glyphData = (int*)font.glyphs;
                glyphData[i * 4 + 0] = charCode; // value
                glyphData[i * 4 + 1] = 0;        // offsetX
                glyphData[i * 4 + 2] = 0;        // offsetY
                glyphData[i * 4 + 3] = (int)CTFontGetAdvancesForGlyphs((CTFontRef)font.ctFont, kCTFontHorizontalOrientation, &glyph, nullptr, 1); // advanceX
                
                // Store texture coordinates
                font.recs[i] = {
                    (float)x / atlasSize,
                    (float)y / atlasSize,
                    (float)glyphSize / atlasSize,
                    (float)glyphSize / atlasSize
                };
            }
        }
        
        CFRelease(line);
    }
    
    // Create Metal texture from the atlas
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                           width:atlasSize
                                                                                          height:atlasSize
                                                                                       mipmapped:NO];
    id<MTLTexture> metalTexture = [m_device newTextureWithDescriptor:textureDesc];
    
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Failed to create Metal texture");
        CGContextRelease(context);
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        CGColorRelease(textColor);
        return false;
    }
    
    // Upload pixel data to texture
    [metalTexture replaceRegion:MTLRegionMake2D(0, 0, atlasSize, atlasSize)
                   mipmapLevel:0
                     withBytes:pixelData
                   bytesPerRow:atlasSize * 4];
    
    // Set font texture properties
    font.texture.id = (unsigned int)(uintptr_t)metalTexture; // Use pointer as ID
    font.texture.width = atlasSize;
    font.texture.height = atlasSize;
    font.texture.mipmaps = 1;
    font.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    font.texture.texture = (__bridge void*)metalTexture;
    
    TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Successfully generated atlas %dx%d with %d glyphs", atlasSize, atlasSize, charCount);
    
    // Cleanup
    CGContextRelease(context);
    free(pixelData);
    CGColorSpaceRelease(colorSpace);
    CGColorRelease(textColor);
    
    return true;
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
        id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)font.texture.texture;
        // Metal textures are automatically managed by ARC
    }
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

id<MTLTexture> MetalTextRenderer::CreateTextureFromCGImage(CGImageRef image) {
    if (!image || !m_device) return nullptr;
    
    return MetalTexture::CreateFromCGImage(image, m_device);
}

void MetalTextRenderer::ClearTextCache() {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        TraceLog(LOG_INFO, "[CLEANUP] Clearing text texture cache (%lu textures)", m_textTextureCache.size());
        m_textTextureCache.clear(); // ARC will release the Metal textures
    }

#endif // defined(__APPLE__) && TARGET_OS_IOS 