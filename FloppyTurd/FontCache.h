#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "RaylibCompat.h"  // For Rectangle and Vector2 types

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

// Font cache entry structure
struct FontCacheEntry {
    std::string fontName;
    int fontSize;
    int atlasSize;
    std::string atlasType; // "SDF" or "BITMAP"
    std::string cacheKey;
    std::string atlasPath;
    std::string metadataPath;
    time_t lastModified;
    bool isValid;
    
    FontCacheEntry() : fontSize(0), atlasSize(0), lastModified(0), isValid(false) {}
};

// Font cache manager
class FontCache {
public:
    static FontCache& GetInstance();
    
    // Initialize cache directory
    bool Initialize();
    
    // Generate cache key for font
    std::string GenerateCacheKey(const std::string& fontName, int fontSize, int atlasSize, const std::string& atlasType);
    
    // Check if cached atlas exists and is valid
    bool HasValidCache(const std::string& fontName, int fontSize, int atlasSize, const std::string& atlasType);
    
    // Get cached atlas path
    std::string GetCachedAtlasPath(const std::string& fontName, int fontSize, int atlasSize, const std::string& atlasType);
    
    // Save atlas to cache
    bool SaveAtlasToCache(const std::string& fontName, int fontSize, int atlasSize, 
                         const std::string& atlasType, const std::vector<uint8_t>& atlasData,
                         const std::vector<Rectangle>& glyphRects,
                         const std::vector<Vector2>& glyphOffsets,
                         const std::vector<float>& glyphAdvances);
    
    // Load atlas from cache
    bool LoadAtlasFromCache(const std::string& fontName, int fontSize, int atlasSize,
                           const std::string& atlasType, std::vector<uint8_t>& atlasData,
                           std::vector<Rectangle>& glyphRects,
                           std::vector<Vector2>& glyphOffsets,
                           std::vector<float>& glyphAdvances);
    
    // Clear all cached atlases
    void ClearCache();
    
    // Get cache statistics
    size_t GetCacheSize() const;
    size_t GetCacheEntryCount() const;
    
private:
    FontCache() = default;
    ~FontCache() = default;
    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;
    
    std::string m_cacheDirectory;
    std::unordered_map<std::string, FontCacheEntry> m_cacheEntries;
    mutable std::mutex m_cacheMutex;
    
    // Helper methods
    bool CreateCacheDirectory();
    std::string GetCacheDirectory();
    bool SaveMetadata(const FontCacheEntry& entry);
    bool LoadMetadata(const std::string& cacheKey, FontCacheEntry& entry);
    bool IsFileValid(const std::string& filePath, time_t expectedTime);
    std::string GetFontFileModificationTime(const std::string& fontName);
}; 