#pragma once
#include "RaylibCompat.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <future>
#include "PlatformLayer.h"

// Resource quality levels for different device tiers
enum class ResourceQuality {
    LOW,    // Mobile low-end devices
    MEDIUM, // Mobile mid-range devices  
    HIGH,   // Desktop and high-end mobile
    AUTO    // Automatically detect based on platform
};

// Resource types for better organization and caching
enum class ResourceType {
    TEXTURE,
    SOUND,
    MUSIC,
    FONT
};

// Resource loading modes
enum class LoadingMode {
    SYNC,           // Load immediately (blocking)
    ASYNC,          // Load in background
    STREAM,         // Stream for large files (music)
    LAZY            // Load when first requested
};

// Individual resource metadata
struct ResourceInfo {
    std::string id;              // Unique identifier (e.g., "player_idle")
    std::string relativePath;    // Path relative to resources folder
    ResourceType type;
    LoadingMode loadingMode;
    ResourceQuality minQuality;  // Minimum quality level to load this resource
    bool isLoaded = false;
    bool isLoading = false;
    std::string qualityVariant;  // Which quality version is loaded (for fallbacks)
};

// Cached resource container
template<typename T>
struct CachedResource {
    T resource;
    bool isValid = false;
    std::string path;           // Full resolved path
    size_t memoryUsage = 0;     // Estimated memory usage in bytes
    float lastAccessed = 0.0f;  // Time since last access (for LRU cache)
};

class ResourceManager {
public:
    static ResourceManager& GetInstance() {
        static ResourceManager instance;
        return instance;
    }

    // Initialize the resource manager
    void Initialize(ResourceQuality quality = ResourceQuality::AUTO);
    void Shutdown();

    // Resource loading methods
    Texture2D GetTexture(const std::string& id);
    Sound GetSound(const std::string& id);
    Music GetMusic(const std::string& id);
    Font GetFont(const std::string& id);

    // Async loading for mobile optimization
    std::future<bool> LoadTextureAsync(const std::string& id);
    std::future<bool> LoadSoundAsync(const std::string& id);
    std::future<bool> LoadMusicAsync(const std::string& id);

    // Resource management
    void PreloadLevel(int levelIndex);           // Preload all resources for a level
    void UnloadLevel(int levelIndex);            // Unload level-specific resources
    void ClearCache();                           // Clear all cached resources
    void TrimCache(size_t maxMemoryMB = 50);     // LRU cache trimming for mobile

    // Quality and performance
    void SetResourceQuality(ResourceQuality quality);
    ResourceQuality GetResourceQuality() const { return currentQuality; }
    size_t GetMemoryUsage() const;              // Total memory used by cached resources
    
    // Platform-specific optimizations
    void EnableTextureCompression(bool enable);
    void SetMaxTextureSize(int maxSize);        // Limit texture size for mobile devices
    void EnableResourceStreaming(bool enable);   // For large music files

    // Utility methods
    bool IsResourceLoaded(const std::string& id) const;
    void ReloadResource(const std::string& id);  // Force reload (useful for hot-reloading)
    std::vector<std::string> GetLoadedResources() const;
    const std::unordered_map<std::string, ResourceInfo>& GetResourceRegistry() const { return resourceRegistry; }

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // Internal loading methods
    bool LoadTextureInternal(const std::string& id);
    bool LoadSoundInternal(const std::string& id);
    bool LoadMusicInternal(const std::string& id);
    bool LoadFontInternal(const std::string& id);

    // Path resolution
    std::string ResolvePath(const std::string& id, ResourceType type);
    std::string GetQualityVariant(const std::string& path, ResourceQuality quality);
    
    // Cache management
    void UpdateAccessTime(const std::string& id);
    void EvictLRUResource();
    
    // Resource registration (called during initialization)
    void RegisterAllResources();
    void RegisterResource(const std::string& id, const std::string& relativePath, 
                         ResourceType type, LoadingMode mode = LoadingMode::LAZY,
                         ResourceQuality minQuality = ResourceQuality::LOW);

    // Member variables
    ResourceQuality currentQuality = ResourceQuality::HIGH;
    bool compressionEnabled = false;
    bool streamingEnabled = true;
    int maxTextureSize = 2048;
    size_t maxCacheMemoryMB = 100;
    
    // Resource registry and cache
    std::unordered_map<std::string, ResourceInfo> resourceRegistry;
    std::unordered_map<std::string, CachedResource<Texture2D>> textureCache;
    std::unordered_map<std::string, CachedResource<Sound>> soundCache;
    std::unordered_map<std::string, CachedResource<Music>> musicCache;
    std::unordered_map<std::string, CachedResource<Font>> fontCache;
    
    // Platform integration
    PlatformLayer* platform = nullptr;
    
    // Performance tracking
    size_t totalMemoryUsage = 0;
    float frameTime = 0.0f;
}; 