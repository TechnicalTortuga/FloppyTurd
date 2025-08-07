#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <chrono>
#include <fstream>
#include "../Core/GNLog.h"

#ifdef PLATFORM_IOS
#include "../Platform/PlatformDelegates.h"
#endif

namespace GameCore {

    /**
     * Threading-safe C++ Asset Management System
     * 
     * Integrates with delegate/proxy/threading system for concurrency-safe loading
     * - Uses platform delegates for async asset loading
     * - C++ owns the asset lifecycle
     * - Platform-specific implementations via delegates
     * - Thread-safe operations through command queue
     */

    enum class AssetType {
        Texture,
        Audio,
        Font,
        Shader,
        Data
    };

    struct AssetLoadRequest {
        std::string name;
        AssetType type;
        std::string extension;
        std::function<void(bool success, const std::string& error)> callback;
    };

    struct AssetInfo {
        std::string name;
        AssetType type;
        std::string path;
        bool loaded;
        bool loading;
    };

    // Asset Manager - C++-centric with platform delegates
    class AssetManager {
    public:
        static AssetManager& getInstance();
        
        // Core asset management (thread-safe via delegates)
        void loadTexture(const std::string& name, const std::string& extension = "png", 
                        std::function<void(bool success, const std::string& error)> callback = nullptr);
        
        void loadAudio(const std::string& name, const std::string& extension = "mp3",
                      std::function<void(bool success, const std::string& error)> callback = nullptr);
        
        void loadFont(const std::string& name, int size = 16, const std::string& extension = "ttf",
                     std::function<void(bool success, const std::string& error)> callback = nullptr);
        
        void loadShader(const std::string& vertexPath, const std::string& fragmentPath,
                       std::function<void(bool success, const std::string& error)> callback = nullptr);
        
        void loadData(const std::string& name, const std::string& extension = "json",
                     std::function<void(bool success, const std::string& error)> callback = nullptr);
        
        // Asset state queries (thread-safe)
        bool isAssetLoaded(const std::string& name, AssetType type) const;
        bool isAssetLoading(const std::string& name, AssetType type) const;
        std::string getAssetPath(const std::string& name, AssetType type) const;
        
        // Dynamic texture metadata (NEW)
        bool getTextureMetadata(const std::string& textureId, TextureMetadata* metadata) const;
        bool getTextureDimensions(const std::string& textureId, int* width, int* height) const;
        void cacheTextureMetadata(const std::string& textureId, const TextureMetadata& metadata);
        
        // Raw byte access (thread-safe)
        std::vector<uint8_t>* getAssetData(const std::string& name, AssetType type, int size = 0);
        const std::vector<uint8_t>* getAssetData(const std::string& name, AssetType type, int size = 0) const;
        void setAssetData(const std::string& name, AssetType type, const std::vector<uint8_t>& data, int size = 0);
        
        // Asset management
        void unloadAsset(const std::string& name, AssetType type);
        void unloadAllAssets();
        
        // Preloading with threading
        void preloadEssentialAssets(std::function<void()> onComplete = nullptr);
        
        // Platform integration
        void initialize();
        void initialize(const PlatformDelegates& delegates); // NEW: Initialize with delegates
        void shutdown();
        
        // Memory management
        size_t getMemoryUsage() const;
        void clearCache();
        
    private:
        AssetManager();
        ~AssetManager() = default;
        
        std::string getAssetFullPath(const std::string& name, AssetType type, const std::string& extension);
        std::string generateAssetKey(const std::string& name, AssetType type, int size = 0);
        void enqueueAssetLoad(const AssetLoadRequest& request);
        void cacheAssetData(const std::string& key, const std::vector<uint8_t>& data);
        std::vector<uint8_t> getCachedAssetData(const std::string& key);
        
        // Real asset file loading
        std::vector<uint8_t> readFileBytes(const std::string& filePath);
        void loadAssetFromDisk(const AssetLoadRequest& request);
        
        // Production-ready asset data structures
        struct AssetData {
            std::vector<uint8_t> bytes;
            size_t size = 0;
            std::string path;
            std::string extension;
            AssetType type;
            std::chrono::steady_clock::time_point lastAccess;
            int referenceCount = 0;
        };
        
        // Thread-safe asset management
        std::unordered_map<std::string, AssetInfo> m_assetRegistry;
        std::unordered_map<std::string, AssetData> m_assetDataCache;
        std::unordered_map<std::string, std::vector<std::function<void(bool, const std::string&)>>> m_loadingCallbacks;
        
        // Dynamic texture metadata cache (NEW)
        std::unordered_map<std::string, TextureMetadata> m_textureMetadataCache;
        
        // Platform delegates reference
        const PlatformDelegates* m_platformDelegates;
        
        std::mutex m_cacheMutex;
        std::mutex m_registryMutex;
        std::mutex m_callbackMutex;
        mutable std::mutex m_metadataMutex;
        
        bool m_initialized = false;
    };

    // Asset loading delegates (integrated with existing system)
    namespace AssetDelegates {
        
        // Called from C++ via platform delegates
        void loadTexture(const std::string& name, const std::string& extension, 
                        std::function<void(bool success, const std::string& error)> callback);
        
        void loadAudio(const std::string& name, const std::string& extension,
                      std::function<void(bool success, const std::string& error)> callback);
        
        void loadFont(const std::string& name, int size, const std::string& extension,
                     std::function<void(bool success, const std::string& error)> callback);
        
        void loadData(const std::string& name, const std::string& extension,
                     std::function<void(bool success, const std::string& error)> callback);
        
        std::string getAssetPath(const std::string& relativePath, AssetType type);
        bool fileExists(const std::string& relativePath, AssetType type);
    }

} // namespace GameCore
