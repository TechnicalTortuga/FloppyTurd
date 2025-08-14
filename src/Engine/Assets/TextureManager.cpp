#include "TextureManager.h"
#include <stdexcept>

namespace GameCore {

    TextureManager& TextureManager::Instance() {
        static TextureManager instance;
        return instance;
    }

    void TextureManager::Initialize(const PlatformDelegates& delegates) {
        m_delegates = delegates;
        m_isInitialized = true;
        
        GN_LOG_INFO("TextureManager initialized");
        
        // Preload essential textures if delegate is available
        if (m_delegates.asset.preloadEssentialAssets) {
            m_delegates.asset.preloadEssentialAssets();
        }
    }

    TextureMetadata TextureManager::GetTextureMetadata(const std::string& textureId) {
        if (!m_isInitialized) {
            GN_LOG_ERROR("TextureManager not initialized! Call Initialize() first.");
            throw std::runtime_error("TextureManager not initialized");
        }
        
        // Check cache first
        auto it = m_metadataCache.find(textureId);
        if (it != m_metadataCache.end()) {
            return it->second;
        }
        
        // Try to load metadata
        TextureMetadata metadata;
        if (FetchMetadataFromPlatform(textureId, metadata)) {
            CacheTextureMetadata(textureId, metadata);
            return metadata;
        }
        
        // Texture not found - throw error with meaningful message
        std::string errorMsg = "Texture '" + textureId + "' not found or metadata unavailable. "
                              "Ensure the texture exists in the asset catalog and try again.";
        GN_LOG_ERROR(errorMsg);
        throw std::runtime_error(errorMsg);
    }

    bool TextureManager::LoadTextureMetadata(const std::string& textureId) {
        try {
            GetTextureMetadata(textureId);
            return true;
        } catch (const std::exception& e) {
            GN_LOG_WARN("Failed to load texture metadata for '" + textureId + "': " + e.what());
            return false;
        }
    }

    void TextureManager::CacheTextureMetadata(const std::string& textureId, const TextureMetadata& metadata) {
        m_metadataCache[textureId] = metadata;
        LogTextureInfo(textureId, metadata);
        
        // Also cache via platform delegate if available
        if (m_delegates.asset.cacheTextureMetadata) {
            m_delegates.asset.cacheTextureMetadata(textureId.c_str(), &metadata);
        }
    }

    int TextureManager::GetTextureWidth(const std::string& textureId) {
        return GetTextureMetadata(textureId).width;
    }

    int TextureManager::GetTextureHeight(const std::string& textureId) {
        return GetTextureMetadata(textureId).height;
    }

    std::pair<int, int> TextureManager::GetTextureDimensions(const std::string& textureId) {
        const auto& metadata = GetTextureMetadata(textureId);
        return std::make_pair(metadata.width, metadata.height);
    }

    bool TextureManager::LoadLevelTextures(const std::vector<std::string>& textureIds) {
        bool allSucceeded = true;
        int successCount = 0;
        
        for (const std::string& textureId : textureIds) {
            if (LoadTextureMetadata(textureId)) {
                successCount++;
            } else {
                allSucceeded = false;
            }
        }
        
        GN_LOG_INFO("Loaded " + std::to_string(successCount) + "/" + 
                   std::to_string(textureIds.size()) + " level textures");
        
        return allSucceeded;
    }

    void TextureManager::PreloadEssentialTextures() {
        // Define essential textures that should always be available
        std::vector<std::string> essentialTextures = {
            // Player textures
            "TurdletIdle", "TurdletJump", "TurdletFall",
            
            // UI textures
            "ButtonPlay", "ButtonExit", "ButtonBack",
            "LeftArrow", "RightArrow", "F",
            
            // Common pickup textures
            "BlueCoin", "GoldCoin", "RedCoin", "PooHeart", "PooHeartBig", "PooHeartRainbow",
            
            // Common enemy textures
            "BirdIdle", "ToiletPaperFlap",
            
            // Main menu backgrounds
            "MainMenu", "MainMenuMobile"
        };
        
        LoadLevelTextures(essentialTextures);
    }

    void TextureManager::ClearCache() {
        m_metadataCache.clear();
        GN_LOG_INFO("TextureManager cache cleared");
    }

    bool TextureManager::IsTextureCached(const std::string& textureId) const {
        return m_metadataCache.find(textureId) != m_metadataCache.end();
    }

    size_t TextureManager::GetCacheSize() const {
        return m_metadataCache.size();
    }

    bool TextureManager::ValidateTextureExists(const std::string& textureId) const {
        if (!m_isInitialized) {
            return false;
        }
        
        // Check cache first
        if (IsTextureCached(textureId)) {
            return true;
        }
        
        // Try platform delegate if available
        if (m_delegates.asset.getTextureMetadata) {
            TextureMetadata metadata;
            return m_delegates.asset.getTextureMetadata(textureId.c_str(), &metadata);
        }
        
        return false;
    }

    void TextureManager::DumpCacheInfo() const {
        GN_LOG_INFO("=== TextureManager Cache Info ===");
        GN_LOG_INFO("Cache size: " + std::to_string(m_metadataCache.size()) + " textures");
        
        for (const auto& pair : m_metadataCache) {
            const std::string& textureId = pair.first;
            const TextureMetadata& metadata = pair.second;
            
            GN_LOG_INFO("  " + textureId + ": " + 
                       std::to_string(metadata.width) + "x" + std::to_string(metadata.height) + 
                       " (" + metadata.format + ", " + std::to_string(metadata.channels) + " channels)");
        }
        GN_LOG_INFO("=== End Cache Info ===");
    }

    bool TextureManager::FetchMetadataFromPlatform(const std::string& textureId, TextureMetadata& metadata) {
        // Try renderer delegate first
        if (m_delegates.renderer.getTextureMetadata) {
            if (m_delegates.renderer.getTextureMetadata(textureId.c_str(), &metadata)) {
                metadata.assetPath = textureId;
                metadata.isLoaded = true;
                return true;
            }
        }
        
        // Try asset delegate
        if (m_delegates.asset.getTextureMetadata) {
            if (m_delegates.asset.getTextureMetadata(textureId.c_str(), &metadata)) {
                metadata.assetPath = textureId;
                metadata.isLoaded = true;
                return true;
            }
        }
        
        return false;
    }

    void TextureManager::LogTextureInfo(const std::string& textureId, const TextureMetadata& metadata) const {
        GN_LOG_DEBUG("Cached texture metadata: " + textureId + " -> " + 
                    std::to_string(metadata.width) + "x" + std::to_string(metadata.height) + 
                    " (" + metadata.format + ", " + std::to_string(metadata.dataSize) + " bytes)");
    }

} // namespace GameCore
