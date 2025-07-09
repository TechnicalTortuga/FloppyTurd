#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS

#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Metal/Metal.h>
#include "RaylibCompat.h"
#include "FontCache.h"
#include <vector>
#include <string>

// Font atlas generation types
enum class AtlasType {
    BITMAP,     // Traditional RGBA bitmap
    SDF,        // Single-channel signed distance field
    MSDF        // Multi-channel signed distance field
};

// iOS-only pixel format enum (do not use raylib pixel formats)
enum IOSPixelFormat {
    IOS_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 = 0, // 4 channels (bitmap)
    IOS_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1 // 1 channel (SDF)
};

// Enhanced font atlas structure
struct FontAtlas {
    AtlasType type;
    int width, height;
    int glyphCount;
    float distanceRange;      // For SDF: distance field range in pixels
    float glyphScale;         // Font size in pixels per em
    
    // Glyph data
    std::vector<Rectangle> glyphRects;    // UV coordinates in atlas
    std::vector<Vector2> glyphOffsets;    // Glyph positioning offsets
    std::vector<float> glyphAdvances;     // Horizontal advance for each glyph
    
    // Texture data
    std::vector<uint8_t> textureData;     // Raw texture data
    int textureChannels;      // 1 for SDF, 4 for bitmap
    
    FontAtlas() : type(AtlasType::SDF), width(0), height(0), glyphCount(0), 
                  distanceRange(2.0f), glyphScale(16.0f), textureChannels(1) {}
};

// Font atlas generation configuration
struct AtlasConfig {
    AtlasType type = AtlasType::SDF;
    int atlasSize = 512;
    float fontSize = 16.0f;
    float distanceRange = 2.0f;
    int glyphPadding = 2;
    bool generateMipmaps = false;
    
    AtlasConfig() = default;
    AtlasConfig(AtlasType t, int size, float fontSz, float distRange) 
        : type(t), atlasSize(size), fontSize(fontSz), distanceRange(distRange) {}
};

// Main font atlas generator class
class FontAtlasGenerator {
public:
    FontAtlasGenerator();
    ~FontAtlasGenerator();
    
    // Initialize with Metal device
    bool Initialize(id<MTLDevice> device);
    
    // Generate font atlas
    bool GenerateAtlas(Font& font, const AtlasConfig& config = AtlasConfig());
    
    // Check cache and generate atlas if needed
    bool GenerateAtlasWithCache(Font& font, const AtlasConfig& config = AtlasConfig());
    
    // Generate SDF from bitmap
    bool GenerateSDF(const uint8_t* sourceBitmap, int width, int height, 
                     uint8_t* sdfBitmap, float distanceRange);
    
    // Create Metal texture from atlas
    id<MTLTexture> CreateMetalTexture(const FontAtlas& atlas);
    
    // Create Metal texture from raw data
    id<MTLTexture> CreateMetalTextureFromData(const std::vector<uint8_t>& data, int width, int height, int channels);
    
    // Validation and error checking
    bool ValidateFont(const Font& font);
    bool ValidateAtlas(const FontAtlas& atlas);
    
    // Utility functions
    void LogAtlasInfo(const FontAtlas& atlas);
    void SaveAtlasToPNG(const FontAtlas& atlas, const char* filename);
    
private:
    id<MTLDevice> m_device;
    
    // Core generation methods
    bool GenerateBitmapAtlas(Font& font, FontAtlas& atlas, const AtlasConfig& config);
    bool GenerateSDFAtlas(Font& font, FontAtlas& atlas, const AtlasConfig& config);
    
    // CGContext creation helpers
    CGContextRef CreateBitmapContext(int width, int height, bool isSDF);
    CGContextRef CreateSDFContext(int width, int height);
    CGContextRef CreateRGBAContext(int width, int height);
    
    // Glyph processing
    bool RasterizeGlyphs(Font& font, CGContextRef context, FontAtlas& atlas, 
                        const AtlasConfig& config);
    bool ProcessGlyphMetrics(Font& font, FontAtlas& atlas, const AtlasConfig& config);
    
    // SDF processing
    float CalculateSDFDistance(const uint8_t* source, int width, int height, 
                              int x, int y, int searchRadius);
    void NormalizeSDF(uint8_t* sdfData, int width, int height, float distanceRange);
    
    // Error handling
    void LogError(const char* message, ...);
    void LogInfo(const char* message, ...);
    void LogDebug(const char* message, ...);
};

#endif // defined(__APPLE__) && TARGET_OS_IOS 