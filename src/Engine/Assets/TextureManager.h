#pragma once

#include "../Platform/PlatformDelegates.h"
#include "../Core/GNLog.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace GameCore {

    /**
     * @class TextureManager
     * @brief Dynamic texture metadata management system
     * 
     * Eliminates all hardcoded texture dimensions by providing dynamic texture
     * metadata loading and caching. No more fallback values or hardcoded sizes!
     */
    class TextureManager {
    public:
        static TextureManager& Instance();
        
        // Initialization
        void Initialize(const PlatformDelegates& delegates);
        
        // Dynamic texture metadata - NO FALLBACKS, MUST SUCCEED OR FAIL CLEANLY
        TextureMetadata GetTextureMetadata(const std::string& textureId);
        bool LoadTextureMetadata(const std::string& textureId);
        void CacheTextureMetadata(const std::string& textureId, const TextureMetadata& metadata);
        
        // Convenience methods that throw meaningful errors if texture not found
        int GetTextureWidth(const std::string& textureId);
        int GetTextureHeight(const std::string& textureId);
        std::pair<int, int> GetTextureDimensions(const std::string& textureId);
        
        // Batch loading for level initialization
        bool LoadLevelTextures(const std::vector<std::string>& textureIds);
        void PreloadEssentialTextures();
        
        // Cache management
        void ClearCache();
        bool IsTextureCached(const std::string& textureId) const;
        size_t GetCacheSize() const;
        
        // Validation and debugging
        bool ValidateTextureExists(const std::string& textureId) const;
        void DumpCacheInfo() const;
        
    private:
        TextureManager() = default;
        ~TextureManager() = default;
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;
        
        // Core data
        PlatformDelegates m_delegates;
        std::unordered_map<std::string, TextureMetadata> m_metadataCache;
        bool m_isInitialized = false;
        
        // Helper methods
        bool FetchMetadataFromPlatform(const std::string& textureId, TextureMetadata& metadata);
        void LogTextureInfo(const std::string& textureId, const TextureMetadata& metadata) const;
    };

    // Global convenience functions for easy access
    inline int GetTextureWidth(const std::string& textureId) {
        return TextureManager::Instance().GetTextureWidth(textureId);
    }
    
    inline int GetTextureHeight(const std::string& textureId) {
        return TextureManager::Instance().GetTextureHeight(textureId);
    }
    
    inline std::pair<int, int> GetTextureDimensions(const std::string& textureId) {
        return TextureManager::Instance().GetTextureDimensions(textureId);
    }

} // namespace GameCore
