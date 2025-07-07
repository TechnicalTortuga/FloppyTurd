#import "MetalTextRenderer.h"
#import "MetalTexture.h"
#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>

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
    
    m_device = nullptr;
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
            NSString* assetName = [path substringFromIndex:7]; // Remove "asset://" prefix
            NSLog(@"[DEBUG] LoadFont: Loading from asset catalog: %@", assetName);
            
            // For asset catalog fonts, we need to load them differently
            // Try to load as a system font first (many fonts are available as system fonts)
            font.ctFont = CTFontCreateWithName((__bridge CFStringRef)assetName, fontSize, nullptr);
            
            if (!font.ctFont) {
                // If not available as system font, try to load from bundle
                NSString* bundlePath = [[NSBundle mainBundle] pathForResource:assetName ofType:@"ttf"];
                if (!bundlePath) {
                    bundlePath = [[NSBundle mainBundle] pathForResource:assetName ofType:@"otf"];
                }
                
                if (bundlePath) {
                    NSURL* fontURL = [NSURL fileURLWithPath:bundlePath];
                    CGDataProviderRef dataProvider = CGDataProviderCreateWithURL((__bridge CFURLRef)fontURL);
                    
                    if (dataProvider) {
                        CGFontRef cgFont = CGFontCreateWithDataProvider(dataProvider);
                        if (cgFont) {
                            font.ctFont = CTFontCreateWithGraphicsFont(cgFont, fontSize, nullptr, nullptr);
                            CGFontRelease(cgFont);
                        }
                        CGDataProviderRelease(dataProvider);
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
            TraceLog(LOG_WARNING, "Failed to load font: %s, falling back to system font", [path UTF8String]);
            return LoadSystemFont("Helvetica", fontSize);
        } else {
            TraceLog(LOG_INFO, "[METAL DEBUG] LoadFont: Successfully loaded font: %s", [path UTF8String]);
        }
    }
    
    return font;
}

Font MetalTextRenderer::LoadSystemFont(const char* fontName, int fontSize) {
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
            uiFont = [UIFont systemFontOfSize:fontSize];
        }
        
        font.ctFont = CTFontCreateWithName((__bridge CFStringRef)uiFont.fontName, fontSize, nullptr);
    }
    
    return font;
}

void MetalTextRenderer::UnloadFont(Font font) {
    if (font.ctFont) {
        CFRelease(font.ctFont);
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
    TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: text='%s', font.ctFont=%p, fontSize=%.2f, color=(%d,%d,%d,%d)", text, font.ctFont, fontSize, color.r, color.g, color.b, color.a);
    if (!text || !font.ctFont || !m_device) {
        TraceLog(LOG_ERROR, "[METAL ERROR] RenderTextToTexture: Invalid input (text or font.ctFont or m_device is null)\ntext='%s', font.ctFont=%p, m_device=%p", text, font.ctFont, m_device);
        return nullptr;
    }
    
    @autoreleasepool {
        NSString* string = [NSString stringWithUTF8String:text];
        
        // Scale font if needed
        CTFontRef scaledFont = (CTFontRef)font.ctFont;
        if (fontSize != font.size) {
            scaledFont = CTFontCreateCopyWithAttributes((CTFontRef)font.ctFont, fontSize, nullptr, nullptr);
        }
        
        // Get text size
        CGSize textSize = GetTextSize(text, scaledFont, 0);
        if (textSize.width <= 0 || textSize.height <= 0) {
            TraceLog(LOG_WARNING, "[METAL WARNING] RenderTextToTexture: Calculated text size is zero or negative for text: '%s'. Returning nil.", text);
            if (scaledFont != font.ctFont) CFRelease(scaledFont);
            return nullptr;
        }
        
        // Create bitmap context
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        size_t width = (size_t)ceil(textSize.width);
        size_t height = (size_t)ceil(textSize.height);
        
        uint8_t* pixelData = (uint8_t*)calloc(width * height * 4, 1);
        CGContextRef context = CGBitmapContextCreate(pixelData, width, height, 8, width * 4, colorSpace, kCGImageAlphaPremultipliedLast);
        
        if (!context) {
            free(pixelData);
            CGColorSpaceRelease(colorSpace);
            if (scaledFont != font.ctFont) CFRelease(scaledFont);
            return nullptr;
        }
        
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
        
        // Flip coordinate system for Core Text
        CGContextSetTextMatrix(context, CGAffineTransformIdentity);
        CGContextTranslateCTM(context, 0, height);
        CGContextScaleCTM(context, 1.0, -1.0);
        
        // Draw text
        CGContextSetTextPosition(context, 0, 0);
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
            TraceLog(LOG_INFO, "[METAL DEBUG] RenderTextToTexture: Created Metal texture %p for text '%s'", texture, text);
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

#endif // defined(__APPLE__) && TARGET_OS_IOS 