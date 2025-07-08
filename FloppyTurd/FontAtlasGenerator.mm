#import "FontAtlasGenerator.h"
#import "PlatformLayer.h"
#include <cmath>
#include <cstdarg>
#include <vector>
#include <ImageIO/ImageIO.h>
#include <MobileCoreServices/MobileCoreServices.h>

// Helper to save grayscale bitmap as PNG
static void SaveGrayscaleBitmapToPNG(const uint8_t* data, int width, int height, const char* path) {
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    CGContextRef context = CGBitmapContextCreate((void*)data, width, height, 8, width, colorSpace, kCGImageAlphaNone);
    CGImageRef image = CGBitmapContextCreateImage(context);
    NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, kUTTypePNG, 1, NULL);
    CGImageDestinationAddImage(dest, image, NULL);
    CGImageDestinationFinalize(dest);
    CFRelease(dest);
    CGImageRelease(image);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
}

#if defined(__APPLE__) && TARGET_OS_IOS

FontAtlasGenerator::FontAtlasGenerator() : m_device(nullptr) {
}

FontAtlasGenerator::~FontAtlasGenerator() {
    m_device = nullptr;
}

bool FontAtlasGenerator::Initialize(id<MTLDevice> device) {
    if (!device) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] Initialize: Invalid Metal device");
        return false;
    }
    
    m_device = device;
    TraceLog(LOG_INFO, "[FONT ATLAS] FontAtlasGenerator initialized successfully");
    return true;
}

bool FontAtlasGenerator::GenerateAtlas(Font& font, const AtlasConfig& config) {
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Starting atlas generation");
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: font.ctFont=%p, config.type=%d, config.atlasSize=%d", 
             font.ctFont, (int)config.type, config.atlasSize);
    
    if (!font.ctFont) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateAtlas: No CTFont available");
        return false;
    }
    
    if (!m_device) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateAtlas: No Metal device");
        return false;
    }
    
    // Create atlas structure
    FontAtlas atlas;
    atlas.type = config.type;
    atlas.width = config.atlasSize;
    atlas.height = config.atlasSize;
    atlas.distanceRange = config.distanceRange;
    atlas.glyphScale = config.fontSize;
    atlas.textureChannels = (config.type == AtlasType::BITMAP) ? 4 : 1;
    
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Created atlas structure: %dx%d, channels=%d", 
             atlas.width, atlas.height, atlas.textureChannels);
    
    // Generate atlas based on type
    bool success = false;
    if (config.type == AtlasType::SDF) {
        success = GenerateSDFAtlas(font, atlas, config);
    } else {
        success = GenerateBitmapAtlas(font, atlas, config);
    }
    
    if (!success) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateAtlas: Failed to generate atlas");
        return false;
    }
    
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Generated %d glyphs", atlas.glyphCount);
    
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Generated texture data, size=%zu bytes", 
             atlas.textureData.size());
    
    // Debug: Save Whacky Joe SDF atlas as PNG if this is the Whacky Joe font
    if (font.name && strstr(font.name, "Whacky_Joe") != nullptr) {
        TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Saving SDF atlas for Whacky Joe font to /tmp/whacky_joe_sdf_atlas.png");
        SaveAtlasToPNG(atlas, "/tmp/whacky_joe_sdf_atlas.png");
    }
    
    // Validate atlas
    if (!ValidateAtlas(atlas)) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateAtlas: Atlas validation failed");
        return false;
    }
    
    // Create Metal texture
    id<MTLTexture> metalTexture = CreateMetalTexture(atlas);
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateAtlas: Failed to create Metal texture");
        return false;
    }
    
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Created Metal texture: %p, size=%lux%lu", 
             metalTexture, (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
    
    // Update font structure with atlas data
    font.texture.id = (unsigned int)(uintptr_t)metalTexture;
    font.texture.width = atlas.width;
    font.texture.height = atlas.height;
    font.texture.mipmaps = 1;
    // For SDF, use grayscale pixel format (note: coloring is done in the shader, not the atlas)
    font.texture.format = (atlas.type == AtlasType::BITMAP) ? 
                         IOS_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 : 
                         IOS_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE; // SDF: grayscale only
    font.texture.texture = (__bridge_retained void*)metalTexture;
    
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Updated font texture: id=%u, size=%dx%d, format=%d", 
             font.texture.id, font.texture.width, font.texture.height, font.texture.format);
    
    // Copy glyph data to font structure
    font.glyphCount = atlas.glyphCount;
    if (atlas.glyphCount > 0) {
        // Allocate memory for glyph data
        font.recs = (Rectangle*)malloc(atlas.glyphCount * sizeof(Rectangle));
        font.glyphs = (int*)malloc(atlas.glyphCount * 4 * sizeof(int)); // 4 ints per glyph
        
        if (font.recs && font.glyphs) {
            // Copy glyph rectangles (UV coordinates)
            for (int i = 0; i < atlas.glyphCount; i++) {
                font.recs[i] = atlas.glyphRects[i];
            }
            
            // Copy glyph data (offset, advance, etc.)
            for (int i = 0; i < atlas.glyphCount; i++) {
                int* glyphData = (int*)font.glyphs + (i * 4);
                glyphData[0] = (int)atlas.glyphOffsets[i].x;  // offset x
                glyphData[1] = (int)atlas.glyphOffsets[i].y;  // offset y
                glyphData[2] = 0;  // unused
                glyphData[3] = (int)atlas.glyphAdvances[i];   // advance
            }
            
            TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Copied glyph data for %d glyphs", atlas.glyphCount);
        } else {
            TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateAtlas: Failed to allocate glyph data memory");
            if (font.recs) free(font.recs);
            if (font.glyphs) free(font.glyphs);
            font.recs = nullptr;
            font.glyphs = nullptr;
            return false;
        }
    }
    
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateAtlas: Atlas generation completed successfully");
    return true;
}

bool FontAtlasGenerator::GenerateSDFAtlas(Font& font, FontAtlas& atlas, const AtlasConfig& config) {
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateSDFAtlas: Generating SDF atlas");
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Config: fontSize=%.1f, atlasSize=%d, distanceRange=%.1f", 
             config.fontSize, config.atlasSize, config.distanceRange);
    
    // Create high-resolution bitmap context (4x for better SDF quality)
    int highResSize = config.atlasSize * 4;
    CGContextRef highResContext = CreateSDFContext(highResSize, highResSize);
    if (!highResContext) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateSDFAtlas: Failed to create high-res context");
        return false;
    }
    
    // Rasterize glyphs to high-resolution bitmap
    if (!RasterizeGlyphs(font, highResContext, atlas, config)) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateSDFAtlas: Failed to rasterize glyphs");
        CGContextRelease(highResContext);
        return false;
    }
    
    // Save high-res bitmap for debugging
    uint8_t* highResData = (uint8_t*)CGBitmapContextGetData(highResContext);
    if (font.name && (strstr(font.name, "Helvetica") != nullptr || strstr(font.name, "Chalkduster") != nullptr)) {
        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Saving high-res bitmap for %s", font.name);
        SaveGrayscaleBitmapToPNG(highResData, highResSize, highResSize, "/tmp/helvetica_bitmap.png");
    }
    
    // Generate SDF from high-resolution bitmap
    atlas.textureData.resize(config.atlasSize * config.atlasSize);
    if (!GenerateSDF(highResData, highResSize, highResSize, atlas.textureData.data(), config.distanceRange)) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateSDFAtlas: Failed to generate SDF");
        CGContextRelease(highResContext);
        return false;
    }
    
    // Normalize SDF data to ensure full range
    NormalizeSDF(atlas.textureData.data(), config.atlasSize, config.atlasSize, config.distanceRange);
    
    // Log SDF generation statistics
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] SDF generation: highResSize=%d, finalSize=%d, distanceRange=%.1f", 
             highResSize, config.atlasSize, config.distanceRange);
    
    // Check SDF data range
    uint8_t minVal = 255, maxVal = 0;
    for (int i = 0; i < config.atlasSize * config.atlasSize; i++) {
        minVal = std::min(minVal, atlas.textureData[i]);
        maxVal = std::max(maxVal, atlas.textureData[i]);
    }
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] SDF data range: min=%d, max=%d (should be 0-255)", minVal, maxVal);
    
    // Save SDF atlas for debugging
    if (font.name && (strstr(font.name, "Helvetica") != nullptr || strstr(font.name, "Chalkduster") != nullptr)) {
        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Saving SDF atlas for %s", font.name);
        SaveAtlasToPNG(atlas, "/tmp/helvetica_sdf_atlas.png");
    }
    
    // Process glyph metrics
    if (!ProcessGlyphMetrics(font, atlas, config)) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] GenerateSDFAtlas: Failed to process glyph metrics");
        CGContextRelease(highResContext);
        return false;
    }
    
    CGContextRelease(highResContext);
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateSDFAtlas: SDF generation completed");
    
    // Log SDF value range
    float minSDF = 1.0f, maxSDF = 0.0f;
    for (size_t i = 0; i < atlas.textureData.size(); ++i) {
        float v = atlas.textureData[i] / 255.0f;
        if (v < minSDF) minSDF = v;
        if (v > maxSDF) maxSDF = v;
    }
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] SDF buffer min=%.3f, max=%.3f", minSDF, maxSDF);
    
    return true;
}

bool FontAtlasGenerator::GenerateBitmapAtlas(Font& font, FontAtlas& atlas, const AtlasConfig& config) {
    TraceLog(LOG_INFO, "FontAtlasGenerator::GenerateBitmapAtlas: Generating bitmap atlas");
    
    // Create RGBA context for bitmap generation
    CGContextRef context = CreateRGBAContext(config.atlasSize, config.atlasSize);
    if (!context) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::GenerateBitmapAtlas: Failed to create context");
        return false;
    }
    
    // Rasterize glyphs
    if (!RasterizeGlyphs(font, context, atlas, config)) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::GenerateBitmapAtlas: Failed to rasterize glyphs");
        CGContextRelease(context);
        return false;
    }
    
    // Get bitmap data
    uint8_t* bitmapData = (uint8_t*)CGBitmapContextGetData(context);
    if (!bitmapData) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::GenerateBitmapAtlas: Failed to get bitmap data");
        CGContextRelease(context);
        return false;
    }
    
    // Copy bitmap data to atlas
    size_t dataSize = config.atlasSize * config.atlasSize * 4;
    atlas.textureData.resize(dataSize);
    memcpy(atlas.textureData.data(), bitmapData, dataSize);
    
    // Process glyph metrics
    if (!ProcessGlyphMetrics(font, atlas, config)) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::GenerateBitmapAtlas: Failed to process glyph metrics");
        CGContextRelease(context);
        return false;
    }
    
    CGContextRelease(context);
    TraceLog(LOG_INFO, "FontAtlasGenerator::GenerateBitmapAtlas: Bitmap generation completed");
    return true;
}

CGContextRef FontAtlasGenerator::CreateSDFContext(int width, int height) {
    TraceLog(LOG_INFO, "[FONT ATLAS] CreateSDFContext: Creating %dx%d grayscale context", width, height);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    if (!colorSpace) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] CreateSDFContext: Failed to create color space");
        return nullptr;
    }
    
    uint8_t* pixelData = (uint8_t*)calloc(width * height, 1);
    if (!pixelData) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] CreateSDFContext: Failed to allocate pixel data");
        CGColorSpaceRelease(colorSpace);
        return nullptr;
    }
    
    CGContextRef context = CGBitmapContextCreate(pixelData, width, height, 8, width, 
                                                colorSpace, kCGImageAlphaNone);
    if (!context) {
        TraceLog(LOG_ERROR, "[FONT ATLAS] CreateSDFContext: Failed to create bitmap context");
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        return nullptr;
    }
    
    // Set black background and white text for SDF
    CGContextSetGrayFillColor(context, 0.0f, 1.0f);
    CGContextFillRect(context, CGRectMake(0, 0, width, height));
    CGContextSetGrayFillColor(context, 1.0f, 1.0f);
    
    // Flip Y-axis to match Metal texture coordinates (top-left origin)
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    CGContextTranslateCTM(context, 0, height);
    CGContextScaleCTM(context, 1.0, -1.0);
    
    CGColorSpaceRelease(colorSpace);
    TraceLog(LOG_INFO, "[FONT ATLAS] CreateSDFContext: Context created successfully");
    return context;
}

CGContextRef FontAtlasGenerator::CreateRGBAContext(int width, int height) {
    TraceLog(LOG_INFO, "FontAtlasGenerator::CreateRGBAContext: Creating %dx%d RGBA context", width, height);
    
    // Create RGB color space
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    if (!colorSpace) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::CreateRGBAContext: Failed to create color space");
        return nullptr;
    }
    
    // Allocate four-channel pixel data
    uint8_t* pixelData = (uint8_t*)calloc(width * height * 4, 1);
    if (!pixelData) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::CreateRGBAContext: Failed to allocate pixel data");
        CGColorSpaceRelease(colorSpace);
        return nullptr;
    }
    
    // Create bitmap context
    CGContextRef context = CGBitmapContextCreate(pixelData, width, height, 8, width * 4, 
                                                colorSpace, kCGImageAlphaPremultipliedLast);
    if (!context) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::CreateRGBAContext: Failed to create bitmap context");
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        return nullptr;
    }
    
    // Set up context for Core Text (flip Y-axis)
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    CGContextTranslateCTM(context, 0, height);
    CGContextScaleCTM(context, 1.0, -1.0);
    
    // Set transparent background
    CGContextClearRect(context, CGRectMake(0, 0, width, height));
    
    CGColorSpaceRelease(colorSpace);
    TraceLog(LOG_INFO, "FontAtlasGenerator::CreateRGBAContext: Context created successfully");
    return context;
}

bool FontAtlasGenerator::RasterizeGlyphs(Font& font, CGContextRef context, FontAtlas& atlas, 
                                        const AtlasConfig& config) {
    TraceLog(LOG_INFO, "FontAtlasGenerator::RasterizeGlyphs: Rasterizing %d glyphs", 
            (int)CTFontGetGlyphCount((CTFontRef)font.ctFont));
    
    const int startChar = 32;
    const int endChar = 126;
    const int charCount = endChar - startChar + 1;
    
    // Calculate glyph layout
    int baseGlyphSize = (int)config.fontSize + config.glyphPadding * 2;
    float scaleFactor = (atlas.type == AtlasType::SDF) ? 4.0f : 1.0f;
    int glyphSize = (int)(baseGlyphSize * scaleFactor);
    int contextSize = (atlas.type == AtlasType::SDF) ? config.atlasSize * 4 : config.atlasSize;
    int glyphsPerRow = contextSize / glyphSize;
    int glyphsPerCol = contextSize / glyphSize;
    
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph layout: size=%d, perRow=%d, perCol=%d, total=%d, contextSize=%d", 
             glyphSize, glyphsPerRow, glyphsPerCol, glyphsPerRow * glyphsPerCol, contextSize);
    
    if (charCount > glyphsPerRow * glyphsPerCol) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::RasterizeGlyphs: Too many characters (%d) for context size %dx%d", 
                charCount, contextSize, contextSize);
        return false;
    }
    
    // Set text color (white for SDF, black for bitmap)
    CGFloat textColorComponents[4];
    if (atlas.type == AtlasType::SDF) {
        textColorComponents[0] = 1.0f; // White for SDF
        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] SDF Text Color: WHITE (1.0, 1.0, 1.0, 1.0)");
    } else {
        textColorComponents[0] = 0.0f; // Black for bitmap
        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Bitmap Text Color: BLACK (0.0, 0.0, 0.0, 1.0)");
    }
    textColorComponents[1] = textColorComponents[0];
    textColorComponents[2] = textColorComponents[0];
    textColorComponents[3] = 1.0f;
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGColorRef textColor = CGColorCreate(colorSpace, textColorComponents);
    CGColorSpaceRelease(colorSpace);
    
    // Log context information
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Context info: size=%dx%d, data=%p, bytesPerRow=%zu", 
             contextSize, contextSize, CGBitmapContextGetData(context), CGBitmapContextGetBytesPerRow(context));
    
    // Verify background color was set correctly
    uint8_t* pixelData = (uint8_t*)CGBitmapContextGetData(context);
    if (pixelData) {
        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Background check: corner pixels (0,0)=%d, (0,%d)=%d, (%d,0)=%d, (%d,%d)=%d", 
                 pixelData[0], contextSize-1, pixelData[(contextSize-1) * contextSize], 
                 contextSize-1, pixelData[contextSize-1], 
                 contextSize-1, contextSize-1, pixelData[(contextSize-1) * contextSize + (contextSize-1)]);
    }
    
    // Initialize glyph arrays
    atlas.glyphCount = charCount;
    atlas.glyphRects.resize(charCount);
    atlas.glyphOffsets.resize(charCount);
    atlas.glyphAdvances.resize(charCount);
    
    int nonzeroPixelCount = 0;
    
    for (int i = 0; i < charCount; i++) {
        int charCode = startChar + i;
        int row = i / glyphsPerRow;
        int col = i % glyphsPerRow;
        int x = col * glyphSize + (int)(config.glyphPadding * scaleFactor);
        int y = row * glyphSize + (int)(config.glyphPadding * scaleFactor);
        // Ensure glyph is within context bounds
        if (x < 0 || x + glyphSize > contextSize || y < 0 || y + glyphSize > contextSize) {
            TraceLog(LOG_WARNING, "[FONT ATLAS] RasterizeGlyphs: Glyph %d (char '%c') out of bounds at (%d,%d)", 
                     i, charCode, x, y);
            continue;
        }
        
        // Create single-character string
        char charStr[2] = {(char)charCode, 0};
        NSString* string = [NSString stringWithUTF8String:charStr];
        
        // Create scaled font for SDF
        CTFontRef scaledFont = (CTFontRef)font.ctFont;
        CGFloat actualFontSize = config.fontSize;
        if (scaleFactor != 1.0f) {
            actualFontSize = config.fontSize * scaleFactor;
            scaledFont = CTFontCreateCopyWithAttributes((CTFontRef)font.ctFont, 
                                                       actualFontSize, 
                                                       nullptr, nullptr);
            TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d: Scaled font size from %.1f to %.1f (scale=%.1f)", 
                     i, config.fontSize, actualFontSize, scaleFactor);
        } else {
            TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d: Using original font size %.1f (no scaling)", 
                     i, actualFontSize);
        }
        
        // Create attributed string
        NSDictionary* attributes = @{
            (NSString*)kCTFontAttributeName: (__bridge id)scaledFont,
            (NSString*)kCTForegroundColorAttributeName: (__bridge id)textColor
        };
        NSAttributedString* attributedString = [[NSAttributedString alloc] initWithString:string attributes:attributes];
        
        // Create text line and draw
        CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attributedString);
        if (line) {
            // Get baseline metrics
            CGFloat ascent = 0, descent = 0, leading = 0;
            double lineWidth = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
            
            // Draw text at baseline position (y + ascent) in flipped context
            CGFloat drawX = x;
            CGFloat drawY = y + ascent;

            // Apply a local vertical flip for each character to render it upright.
            CGContextSaveGState(context);
            CGContextTranslateCTM(context, drawX, drawY);
            CGContextScaleCTM(context, 1.0, -1.0);
            CGContextSetTextPosition(context, 0, 0); // Draw at the new, transformed origin
            CTLineDraw(line, context);
            CGContextRestoreGState(context);
            
            TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d (char '%c'): Drawing at (%.1f, %.1f), metrics: ascent=%.1f, descent=%.1f, leading=%.1f, lineWidth=%.1f", 
                     i, charCode, drawX, drawY, ascent, descent, leading, lineWidth);
            
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
                    CTFontGetBoundingRectsForGlyphs(scaledFont, kCTFontHorizontalOrientation, &glyph, &bounds, 1);
                    
                    // Get glyph advance
                    CGFloat advance = CTFontGetAdvancesForGlyphs(scaledFont, kCTFontHorizontalOrientation, &glyph, nullptr, 1);
                    
                    // Store glyph data (normalize UVs for final atlas size)
                    float uvScale = (atlas.type == AtlasType::SDF) ? (float)contextSize / config.atlasSize : 1.0f;
                    float uvX = (float)x / contextSize;
                    float uvY = (float)y / contextSize;
                    atlas.glyphRects[i] = {
                        uvX,
                        uvY,
                        (float)glyphSize / contextSize,
                        (float)glyphSize / contextSize
                    };
                    atlas.glyphOffsets[i] = {(float)bounds.origin.x / scaleFactor, (float)bounds.origin.y / scaleFactor};
                    atlas.glyphAdvances[i] = (float)advance / scaleFactor;
                    
                    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d (char '%c'): Raw bounds=(%.1f,%.1f,%.1f,%.1f), scaled bounds=(%.1f,%.1f,%.1f,%.1f)", 
                             i, charCode, 
                             bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height,
                             bounds.origin.x / scaleFactor, bounds.origin.y / scaleFactor, 
                             bounds.size.width / scaleFactor, bounds.size.height / scaleFactor);
                    
                    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d (char '%c'): Raw advance=%.1f, scaled advance=%.1f, uvScale=%.3f", 
                             i, charCode, advance, advance / scaleFactor, uvScale);
                    
                    // Count nonzero pixels for validation
                    if (atlas.type == AtlasType::SDF) {
                        uint8_t* pixelData = (uint8_t*)CGBitmapContextGetData(context);
                        int glyphPixelCount = 0;
                        int totalPixelsChecked = 0;
                        
                        for (int gy = y; gy < y + glyphSize && gy < contextSize; gy++) {
                            for (int gx = x; gx < x + glyphSize && gx < contextSize; gx++) {
                                int index = gy * contextSize + gx;
                                totalPixelsChecked++;
                                if (pixelData[index] > 128) {
                                    glyphPixelCount++;
                                    nonzeroPixelCount++;
                                }
                            }
                        }
                        
                        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d (char '%c'): Pixel count: %d/%d pixels > 128 (%.1f%%), total checked: %d", 
                                 i, charCode, glyphPixelCount, glyphSize * glyphSize, 
                                 (float)glyphPixelCount / (glyphSize * glyphSize) * 100.0f, totalPixelsChecked);
                        
                        if (glyphPixelCount > 0) {
                            TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d (char '%c'): Sample pixels at (%d,%d)=%d, (%d,%d)=%d, (%d,%d)=%d", 
                                     i, charCode, 
                                     x, y, pixelData[y * contextSize + x],
                                     x + glyphSize/2, y + glyphSize/2, pixelData[(y + glyphSize/2) * contextSize + (x + glyphSize/2)],
                                     x + glyphSize-1, y + glyphSize-1, pixelData[(y + glyphSize-1) * contextSize + (x + glyphSize-1)]);
                        }
                    }
                    
                    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Glyph %d (char '%c'): pos=(%d,%d), size=%d, bounds=(%.1f,%.1f,%.1f,%.1f), advance=%.1f, uv=(%.3f,%.3f,%.3f,%.3f)", 
                             i, charCode, x, y, glyphSize, 
                             bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height, 
                             advance, 
                             atlas.glyphRects[i].x, atlas.glyphRects[i].y, atlas.glyphRects[i].width, atlas.glyphRects[i].height);
                    
                    // Verify UV coordinates are within valid range
                    if (atlas.glyphRects[i].x < 0.0f || atlas.glyphRects[i].x > 1.0f ||
                        atlas.glyphRects[i].y < 0.0f || atlas.glyphRects[i].y > 1.0f ||
                        atlas.glyphRects[i].width < 0.0f || atlas.glyphRects[i].width > 1.0f ||
                        atlas.glyphRects[i].height < 0.0f || atlas.glyphRects[i].height > 1.0f) {
                        TraceLog(LOG_WARNING, "[FONT ATLAS DEBUG] Glyph %d (char '%c'): INVALID UV coordinates detected!", 
                                 i, charCode);
                    }
                }
            }
            CFRelease(line);
            if (scaledFont != font.ctFont) {
                CFRelease(scaledFont);
            }
        }
    }
    
    CGColorRelease(textColor);
    
    TraceLog(LOG_INFO, "FontAtlasGenerator::RasterizeGlyphs: Rasterized %d glyphs, nonzero pixels: %d", 
             charCount, nonzeroPixelCount);
    
    // Log final atlas statistics
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Final atlas stats: glyphCount=%d, contextSize=%d, glyphSize=%d", 
             charCount, contextSize, glyphSize);
    
    // Log a few sample glyph UVs for verification
    for (int i = 0; i < std::min(5, charCount); i++) {
        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] Sample glyph %d UV: (%.3f, %.3f, %.3f, %.3f)", 
                 i, atlas.glyphRects[i].x, atlas.glyphRects[i].y, 
                 atlas.glyphRects[i].width, atlas.glyphRects[i].height);
    }
    
    if (nonzeroPixelCount == 0) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::RasterizeGlyphs: No glyphs were rasterized!");
        return false;
    }
    
    return true;
}

bool FontAtlasGenerator::GenerateSDF(const uint8_t* sourceBitmap, int width, int height, 
                                    uint8_t* sdfBitmap, float distanceRange) {
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateSDF: Generating SDF from %dx%d bitmap", width, height);
    
    // Use a larger search radius to ensure we can find edges
    int searchRadius = (int)ceil(distanceRange * 4.0f); // Increase search radius
    int outputWidth = width / 4;  // Downsample to final atlas size
    int outputHeight = height / 4;
    
    float minDistance = 1000.0f, maxDistance = -1000.0f;
    int edgePixelCount = 0;
    
    for (int y = 0; y < outputHeight; y++) {
        for (int x = 0; x < outputWidth; x++) {
            // Sample from high-resolution bitmap
            int srcX = x * 4;
            int srcY = y * 4;
            
            // Calculate SDF distance
            float distance = CalculateSDFDistance(sourceBitmap, width, height, srcX, srcY, searchRadius);
            
            // Track min/max for debugging
            if (distance < minDistance) minDistance = distance;
            if (distance > maxDistance) maxDistance = distance;
            if (abs(distance) < distanceRange) edgePixelCount++; // Count edge pixels
            
            // Normalize distance to 0-255 range
            // SDF distances range from -distanceRange to +distanceRange
            // Map to [0, 1] range: (distance + distanceRange) / (2 * distanceRange)
            float normalizedDistance = (distance + distanceRange) / (2.0f * distanceRange);
            normalizedDistance = fmax(0.0f, fmin(1.0f, normalizedDistance));
            
            // Store in output bitmap
            int outputIndex = y * outputWidth + x;
            sdfBitmap[outputIndex] = (uint8_t)(normalizedDistance * 255.0f);
        }
    }
    
    TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] GenerateSDF: Distance range: %.3f to %.3f, edge pixels: %d", 
             minDistance, maxDistance, edgePixelCount);
    if (edgePixelCount < outputWidth * outputHeight * 0.01f) {
        TraceLog(LOG_WARNING, "[FONT ATLAS] GenerateSDF: Low edge pixel count, possible glyph rendering issue");
    }
    
    TraceLog(LOG_INFO, "[FONT ATLAS] GenerateSDF: SDF generation completed");
    return true;
}

float FontAtlasGenerator::CalculateSDFDistance(const uint8_t* source, int width, int height, 
                                              int x, int y, int searchRadius) {
    float minDistance = (float)searchRadius;
    bool isInside = source[y * width + x] > 128;
    bool foundEdge = false;
    
    for (int dy = -searchRadius; dy <= searchRadius; dy++) {
        for (int dx = -searchRadius; dx <= searchRadius; dx++) {
            int sx = x + dx, sy = y + dy;
            if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                bool sampleInside = source[sy * width + sx] > 128;
                if (sampleInside != isInside) {
                    float distance = sqrt((float)(dx*dx + dy*dy));
                    if (distance < minDistance) {
                        minDistance = distance;
                        foundEdge = true;
                    }
                }
            }
        }
    }
    
    return foundEdge ? (isInside ? minDistance : -minDistance) : (isInside ? searchRadius : -searchRadius);
}

void FontAtlasGenerator::NormalizeSDF(uint8_t* sdfData, int width, int height, float distanceRange) {
    TraceLog(LOG_INFO, "[FONT ATLAS] NormalizeSDF: Normalizing SDF data");
    
    float minVal = 1.0f, maxVal = 0.0f;
    for (int i = 0; i < width * height; i++) {
        float v = sdfData[i] / 255.0f;
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
    }
    
    if (maxVal - minVal > 0.001f) {
        float scale = 1.0f / (maxVal - minVal);
        for (int i = 0; i < width * height; i++) {
            float v = sdfData[i] / 255.0f;
            v = (v - minVal) * scale; // Normalize to [0, 1]
            sdfData[i] = (uint8_t)(v * 255.0f);
        }
        TraceLog(LOG_INFO, "[FONT ATLAS DEBUG] NormalizeSDF: Normalized range from [%.3f, %.3f] to [0, 1]", minVal, maxVal);
    } else {
        TraceLog(LOG_WARNING, "[FONT ATLAS] NormalizeSDF: Minimal range [%.3f, %.3f], skipping normalization", minVal, maxVal);
    }
}

bool FontAtlasGenerator::ProcessGlyphMetrics(Font& font, FontAtlas& atlas, const AtlasConfig& config) {
    TraceLog(LOG_INFO, "FontAtlasGenerator::ProcessGlyphMetrics: Processing metrics for %d glyphs", atlas.glyphCount);
    
    // Validate glyph count
    if (atlas.glyphCount <= 0) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ProcessGlyphMetrics: Invalid glyph count");
        return false;
    }
    
    // Validate arrays
    if (atlas.glyphRects.size() != atlas.glyphCount ||
        atlas.glyphOffsets.size() != atlas.glyphCount ||
        atlas.glyphAdvances.size() != atlas.glyphCount) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ProcessGlyphMetrics: Array size mismatch");
        return false;
    }
    
    TraceLog(LOG_INFO, "FontAtlasGenerator::ProcessGlyphMetrics: Metrics processing completed");
    return true;
}

id<MTLTexture> FontAtlasGenerator::CreateMetalTexture(const FontAtlas& atlas) {
    TraceLog(LOG_INFO, "FontAtlasGenerator::CreateMetalTexture: Creating Metal texture %dx%d", atlas.width, atlas.height);
    
    if (!m_device) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::CreateMetalTexture: No Metal device");
        return nullptr;
    }
    
    if (atlas.textureData.empty()) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::CreateMetalTexture: No texture data");
        return nullptr;
    }
    
    // Create texture descriptor
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
                                        (atlas.type == AtlasType::BITMAP) ? MTLPixelFormatRGBA8Unorm : MTLPixelFormatR8Unorm
                                        width:atlas.width
                                        height:atlas.height
                                        mipmapped:NO];
    
    // Use shared storage mode for CPU access
    textureDesc.storageMode = MTLStorageModeShared;
    
    // Create texture
    id<MTLTexture> metalTexture = [m_device newTextureWithDescriptor:textureDesc];
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::CreateMetalTexture: Failed to create Metal texture");
        return nullptr;
    }
    
    // Upload texture data
    [metalTexture replaceRegion:MTLRegionMake2D(0, 0, atlas.width, atlas.height)
                   mipmapLevel:0
                     withBytes:atlas.textureData.data()
                   bytesPerRow:atlas.width * atlas.textureChannels];
    
    TraceLog(LOG_INFO, "FontAtlasGenerator::CreateMetalTexture: Metal texture created successfully");
    return metalTexture;
}

bool FontAtlasGenerator::ValidateFont(const Font& font) {
    if (!font.ctFont) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ValidateFont: Invalid CTFont");
        return false;
    }
    
    CFIndex glyphCount = CTFontGetGlyphCount((CTFontRef)font.ctFont);
    if (glyphCount <= 0) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ValidateFont: Font has no glyphs");
        return false;
    }
    
    TraceLog(LOG_INFO, "FontAtlasGenerator::ValidateFont: Font validated, glyph count: %ld", glyphCount);
    return true;
}

bool FontAtlasGenerator::ValidateAtlas(const FontAtlas& atlas) {
    if (atlas.width <= 0 || atlas.height <= 0) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ValidateAtlas: Invalid dimensions");
        return false;
    }
    
    if (atlas.glyphCount <= 0) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ValidateAtlas: No glyphs in atlas");
        return false;
    }
    
    if (atlas.textureData.empty()) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ValidateAtlas: No texture data");
        return false;
    }
    
    size_t expectedSize = atlas.width * atlas.height * atlas.textureChannels;
    if (atlas.textureData.size() != expectedSize) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::ValidateAtlas: Texture data size mismatch");
        return false;
    }
    
    TraceLog(LOG_INFO, "FontAtlasGenerator::ValidateAtlas: Atlas validated successfully");
    return true;
}

void FontAtlasGenerator::LogAtlasInfo(const FontAtlas& atlas) {
    TraceLog(LOG_INFO, "FontAtlasGenerator::LogAtlasInfo: Atlas Info:");
    TraceLog(LOG_INFO, "  Type: %s", (atlas.type == AtlasType::SDF) ? "SDF" : "Bitmap");
    TraceLog(LOG_INFO, "  Dimensions: %dx%d", atlas.width, atlas.height);
    TraceLog(LOG_INFO, "  Glyph Count: %d", atlas.glyphCount);
    TraceLog(LOG_INFO, "  Distance Range: %.2f", atlas.distanceRange);
    TraceLog(LOG_INFO, "  Glyph Scale: %.2f", atlas.glyphScale);
    TraceLog(LOG_INFO, "  Texture Channels: %d", atlas.textureChannels);
    TraceLog(LOG_INFO, "  Texture Data Size: %zu bytes", atlas.textureData.size());
}

void FontAtlasGenerator::SaveAtlasToPNG(const FontAtlas& atlas, const char* filename) {
    TraceLog(LOG_INFO, "FontAtlasGenerator::SaveAtlasToPNG: Saving atlas to %s", filename);
    if (atlas.textureData.empty()) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::SaveAtlasToPNG: No texture data to save");
        return;
    }
    // Save the atlas as-is, no Y-flip
    CGColorSpaceRef colorSpace = (atlas.type == AtlasType::BITMAP) ? 
                                 CGColorSpaceCreateDeviceRGB() : 
                                 CGColorSpaceCreateDeviceGray();
    CGContextRef context = CGBitmapContextCreate((void*)atlas.textureData.data(), 
                                                atlas.width, atlas.height, 8,
                                                atlas.width * atlas.textureChannels,
                                                colorSpace, 
                                                (atlas.type == AtlasType::BITMAP) ? 
                                                kCGImageAlphaPremultipliedLast : 
                                                kCGImageAlphaNone);
    if (!context) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::SaveAtlasToPNG: Failed to create context");
        CGColorSpaceRelease(colorSpace);
        return;
    }
    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    if (!cgImage) {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::SaveAtlasToPNG: Failed to create CGImage");
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        return;
    }
    // Save to PNG
    NSString* path = [NSString stringWithUTF8String:filename];
    NSURL* url = [NSURL fileURLWithPath:path];
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, 
                                                                        kUTTypePNG, 1, nullptr);
    if (destination) {
        CGImageDestinationAddImage(destination, cgImage, nullptr);
        CGImageDestinationFinalize(destination);
        CFRelease(destination);
        TraceLog(LOG_INFO, "FontAtlasGenerator::SaveAtlasToPNG: Successfully saved atlas");
    } else {
        TraceLog(LOG_ERROR, "FontAtlasGenerator::SaveAtlasToPNG: Failed to create image destination");
    }
    CGImageRelease(cgImage);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
}

void FontAtlasGenerator::LogError(const char* message, ...) {
    va_list args;
    va_start(args, message);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);
    TraceLog(LOG_ERROR, "[FontAtlasGenerator] %s", buffer);
}

void FontAtlasGenerator::LogInfo(const char* message, ...) {
    va_list args;
    va_start(args, message);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);
    TraceLog(LOG_INFO, "[FontAtlasGenerator] %s", buffer);
}

void FontAtlasGenerator::LogDebug(const char* message, ...) {
    va_list args;
    va_start(args, message);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);
    TraceLog(LOG_DEBUG, "[FontAtlasGenerator] %s", buffer);
}

#endif // defined(__APPLE__) && TARGET_OS_IOS 