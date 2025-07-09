#include "FontCache.h"
#include "RaylibCompat.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <sys/stat.h>
#include <dirent.h>

#if defined(__APPLE__) && TARGET_OS_IOS
#import <Foundation/Foundation.h>
#endif

FontCache& FontCache::GetInstance() {
    static FontCache instance;
    return instance;
}

bool FontCache::Initialize() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    m_cacheDirectory = GetCacheDirectory();
    if (m_cacheDirectory.empty()) {
        return false;
    }
    
    return CreateCacheDirectory();
}

std::string FontCache::GetCacheDirectory() {
#if defined(__APPLE__) && TARGET_OS_IOS
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString* cacheDir = [paths firstObject];
    NSString* fontCacheDir = [cacheDir stringByAppendingPathComponent:@"FontCache"];
    return [fontCacheDir UTF8String];
#else
    return "/tmp/FontCache";
#endif
}

bool FontCache::CreateCacheDirectory() {
    if (m_cacheDirectory.empty()) {
        return false;
    }
    
    // Create directory if it doesn't exist
    struct stat st;
    if (stat(m_cacheDirectory.c_str(), &st) != 0) {
#if defined(__APPLE__) && TARGET_OS_IOS
        NSFileManager* fileManager = [NSFileManager defaultManager];
        NSString* cachePath = [NSString stringWithUTF8String:m_cacheDirectory.c_str()];
        NSError* error = nil;
        BOOL success = [fileManager createDirectoryAtPath:cachePath
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:&error];
        if (!success) {
            NSLog(@"[FONT CACHE] Failed to create cache directory: %@", error);
            return false;
        }
#else
        if (mkdir(m_cacheDirectory.c_str(), 0755) != 0) {
            return false;
        }
#endif
    }
    
    return true;
}

std::string FontCache::GenerateCacheKey(const std::string& fontName, int fontSize, int atlasSize, const std::string& atlasType) {
    std::ostringstream oss;
    oss << fontName << "_" << fontSize << "_" << atlasSize << "_" << atlasType;
    return oss.str();
}

bool FontCache::HasValidCache(const std::string& fontName, int fontSize, int atlasSize, const std::string& atlasType) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    std::string cacheKey = GenerateCacheKey(fontName, fontSize, atlasSize, atlasType);
    
    // Check if we have this entry in memory
    auto it = m_cacheEntries.find(cacheKey);
    if (it != m_cacheEntries.end()) {
        return it->second.isValid;
    }
    
    // Try to load metadata from disk
    FontCacheEntry entry;
    if (!LoadMetadata(cacheKey, entry)) {
        return false;
    }
    
    // Validate the entry
    if (entry.fontName != fontName || entry.fontSize != fontSize || 
        entry.atlasSize != atlasSize || entry.atlasType != atlasType) {
        return false;
    }
    
    // Check if files exist and are valid
    if (!IsFileValid(entry.atlasPath, entry.lastModified)) {
        return false;
    }
    
    // Store in memory cache
    m_cacheEntries[cacheKey] = entry;
    return true;
}

std::string FontCache::GetCachedAtlasPath(const std::string& fontName, int fontSize, int atlasSize, const std::string& atlasType) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    std::string cacheKey = GenerateCacheKey(fontName, fontSize, atlasSize, atlasType);
    auto it = m_cacheEntries.find(cacheKey);
    
    if (it != m_cacheEntries.end()) {
        return it->second.atlasPath;
    }
    
    // Try to load from disk
    FontCacheEntry entry;
    if (LoadMetadata(cacheKey, entry)) {
        m_cacheEntries[cacheKey] = entry;
        return entry.atlasPath;
    }
    
    return "";
}

bool FontCache::SaveAtlasToCache(const std::string& fontName, int fontSize, int atlasSize, 
                                const std::string& atlasType, const std::vector<uint8_t>& atlasData,
                                const std::vector<Rectangle>& glyphRects,
                                const std::vector<Vector2>& glyphOffsets,
                                const std::vector<float>& glyphAdvances) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    std::string cacheKey = GenerateCacheKey(fontName, fontSize, atlasSize, atlasType);
    
    // Create cache entry
    FontCacheEntry entry;
    entry.fontName = fontName;
    entry.fontSize = fontSize;
    entry.atlasSize = atlasSize;
    entry.atlasType = atlasType;
    entry.cacheKey = cacheKey;
    entry.atlasPath = m_cacheDirectory + "/" + cacheKey + ".atlas";
    entry.metadataPath = m_cacheDirectory + "/" + cacheKey + ".meta";
    entry.lastModified = time(nullptr);
    entry.isValid = true;
    
    // Save atlas data
    std::ofstream atlasFile(entry.atlasPath, std::ios::binary);
    if (!atlasFile.is_open()) {
        return false;
    }
    
    // Write atlas data
    atlasFile.write(reinterpret_cast<const char*>(atlasData.data()), atlasData.size());
    atlasFile.close();
    
    // Save metadata
    if (!SaveMetadata(entry)) {
        // Clean up atlas file if metadata save failed
        unlink(entry.atlasPath.c_str());
        return false;
    }
    
    // Store in memory cache
    m_cacheEntries[cacheKey] = entry;
    
    return true;
}

bool FontCache::LoadAtlasFromCache(const std::string& fontName, int fontSize, int atlasSize,
                                  const std::string& atlasType, std::vector<uint8_t>& atlasData,
                                  std::vector<Rectangle>& glyphRects,
                                  std::vector<Vector2>& glyphOffsets,
                                  std::vector<float>& glyphAdvances) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    std::string cacheKey = GenerateCacheKey(fontName, fontSize, atlasSize, atlasType);
    std::string atlasPath = GetCachedAtlasPath(fontName, fontSize, atlasSize, atlasType);
    
    if (atlasPath.empty()) {
        return false;
    }
    
    // Load atlas data
    std::ifstream atlasFile(atlasPath, std::ios::binary);
    if (!atlasFile.is_open()) {
        return false;
    }
    
    // Get file size
    atlasFile.seekg(0, std::ios::end);
    size_t fileSize = atlasFile.tellg();
    atlasFile.seekg(0, std::ios::beg);
    
    // Read atlas data
    atlasData.resize(fileSize);
    atlasFile.read(reinterpret_cast<char*>(atlasData.data()), fileSize);
    atlasFile.close();
    
    // Load metadata for glyph information
    FontCacheEntry entry;
    if (LoadMetadata(cacheKey, entry)) {
        // For now, we'll reconstruct glyph data from the atlas
        // In a more complete implementation, we'd store this separately
        int glyphCount = 95; // Default ASCII range
        glyphRects.resize(glyphCount);
        glyphOffsets.resize(glyphCount);
        glyphAdvances.resize(glyphCount);
        
        // Calculate glyph layout (this is a simplified version)
        int glyphSize = fontSize + 4; // Approximate
        int glyphsPerRow = atlasSize / glyphSize;
        
        for (int i = 0; i < glyphCount; i++) {
            int row = i / glyphsPerRow;
            int col = i % glyphsPerRow;
            
            glyphRects[i] = {
                (float)(col * glyphSize) / (float)atlasSize,
                (float)(row * glyphSize) / (float)atlasSize,
                (float)glyphSize / (float)atlasSize,
                (float)glyphSize / (float)atlasSize
            };
            
            glyphOffsets[i] = {0, 0};
            glyphAdvances[i] = (float)glyphSize;
        }
    }
    
    return true;
}

bool FontCache::SaveMetadata(const FontCacheEntry& entry) {
    std::ofstream metaFile(entry.metadataPath);
    if (!metaFile.is_open()) {
        return false;
    }
    
    metaFile << "fontName=" << entry.fontName << std::endl;
    metaFile << "fontSize=" << entry.fontSize << std::endl;
    metaFile << "atlasSize=" << entry.atlasSize << std::endl;
    metaFile << "atlasType=" << entry.atlasType << std::endl;
    metaFile << "cacheKey=" << entry.cacheKey << std::endl;
    metaFile << "atlasPath=" << entry.atlasPath << std::endl;
    metaFile << "lastModified=" << entry.lastModified << std::endl;
    metaFile << "isValid=" << (entry.isValid ? "1" : "0") << std::endl;
    
    metaFile.close();
    return true;
}

bool FontCache::LoadMetadata(const std::string& cacheKey, FontCacheEntry& entry) {
    std::string metadataPath = m_cacheDirectory + "/" + cacheKey + ".meta";
    std::ifstream metaFile(metadataPath);
    if (!metaFile.is_open()) {
        return false;
    }
    
    entry.cacheKey = cacheKey;
    entry.metadataPath = metadataPath;
    
    std::string line;
    while (std::getline(metaFile, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "fontName") entry.fontName = value;
            else if (key == "fontSize") entry.fontSize = std::stoi(value);
            else if (key == "atlasSize") entry.atlasSize = std::stoi(value);
            else if (key == "atlasType") entry.atlasType = value;
            else if (key == "atlasPath") entry.atlasPath = value;
            else if (key == "lastModified") entry.lastModified = std::stol(value);
            else if (key == "isValid") entry.isValid = (value == "1");
        }
    }
    
    metaFile.close();
    return true;
}

bool FontCache::IsFileValid(const std::string& filePath, time_t expectedTime) {
    struct stat st;
    if (stat(filePath.c_str(), &st) != 0) {
        return false;
    }
    
    // Check if file exists and is readable
    if (!S_ISREG(st.st_mode)) {
        return false;
    }
    
    // For now, just check if file exists and has reasonable size
    // In a more complete implementation, we'd check modification times
    return st.st_size > 0;
}

void FontCache::ClearCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Clear memory cache
    m_cacheEntries.clear();
    
    // Clear disk cache
    if (m_cacheDirectory.empty()) {
        return;
    }
    
    DIR* dir = opendir(m_cacheDirectory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string filePath = m_cacheDirectory + "/" + entry->d_name;
                unlink(filePath.c_str());
            }
        }
        closedir(dir);
    }
}

size_t FontCache::GetCacheSize() const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    size_t totalSize = 0;
    for (const auto& pair : m_cacheEntries) {
        struct stat st;
        if (stat(pair.second.atlasPath.c_str(), &st) == 0) {
            totalSize += st.st_size;
        }
    }
    
    return totalSize;
}

size_t FontCache::GetCacheEntryCount() const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_cacheEntries.size();
} 