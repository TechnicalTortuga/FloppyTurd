#ifndef METAL_TEXTURE_CACHE_H
#define METAL_TEXTURE_CACHE_H

#include <string>
#include <unordered_map>
#include "RaylibCompat.h"

// Forward declaration for Metal texture
#ifdef __APPLE__
#ifdef __OBJC__
@class MTLTexture;
#endif
#endif

// This class is designed to safely cache textures while maintaining
// a clean separation between C++ and Objective-C/Metal code
class MetalTextureCache {
public:
    static MetalTextureCache& GetInstance();
    
    // Deleted copy constructor and assignment operator for singleton
    MetalTextureCache(const MetalTextureCache&) = delete;
    MetalTextureCache& operator=(const MetalTextureCache&) = delete;
    
    // Interface for loading and caching textures
    Texture2D GetOrLoadTexture(const std::string& fileName);
    Texture2D LoadTextureFromData(void* data, int width, int height, int format);
    void UnloadTexture(unsigned int textureId);
    void UnloadTexture(void* texturePtr);
    void UnloadAllTextures();
    
    // Mipmap generation
    void GenerateMipmapsForTexture(Texture2D texture);
    
    // Lifecycle management
    void Initialize(void* metalDevice);
    void Shutdown();

private:
    // Private constructor for singleton
    MetalTextureCache();
    ~MetalTextureCache();
    
    // Cache storage
    std::unordered_map<std::string, Texture2D> m_textureCache;
    std::unordered_map<void*, unsigned int> m_textureRefCounts;
    
    // Metal device pointer (void* to avoid including Objective-C headers)
    void* m_metalDevice;
    
    // Next texture ID
    unsigned int m_nextTextureId;
    
    // Helper methods
    Texture2D CreateFallbackTexture();
    unsigned int GenerateTextureId();
};

#endif // METAL_TEXTURE_CACHE_H
