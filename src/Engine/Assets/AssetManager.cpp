#include "AssetManager.h"
#include "../Platform/PlatformDelegates.h"
#include "../Threading/ThreadingProxy.h"
#include <mutex>

namespace GameCore {

    // Static instance
    AssetManager& AssetManager::getInstance() {
        static AssetManager instance;
        return instance;
    }

    AssetManager::AssetManager() {
        // Constructor - initialization happens in initialize()
    }

    void AssetManager::initialize() {
        if (m_initialized) return;
        
        GN_LOG_INFO("Initializing AssetManager with threading support and caching");
        m_initialized = true;
        
        // Clear all caches
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            m_assetRegistry.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_assetDataCache.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_loadingCallbacks.clear();
        }
    }

    void AssetManager::shutdown() {
        if (!m_initialized) return;
        
        GN_LOG_INFO("Shutting down AssetManager - clearing caches");
        unloadAllAssets();
        
        // Clear all caches
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            m_assetRegistry.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_assetDataCache.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_loadingCallbacks.clear();
        }
        
        m_initialized = false;
    }

    std::string AssetManager::getAssetFullPath(const std::string& name, AssetType type, const std::string& extension) {
        std::string basePath;
        
        switch (type) {
            case AssetType::Texture:
                basePath = "textures/";
                break;
            case AssetType::Audio:
                basePath = "audio/";
                break;
            case AssetType::Font:
                basePath = "fonts/";
                break;
            case AssetType::Shader:
                basePath = "shaders/";
                break;
            case AssetType::Data:
                basePath = "data/";
                break;
        }
        
        return basePath + name + "." + extension;
    }

    std::string AssetManager::generateAssetKey(const std::string& name, AssetType type, int size) {
        std::string key = std::to_string(static_cast<int>(type)) + "_" + name;
        if (type == AssetType::Font && size > 0) {
            key += "_" + std::to_string(size);
        }
        return key;
    }

    void AssetManager::cacheAssetData(const std::string& key, const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        AssetData assetData;
        assetData.bytes = data;
        assetData.size = data.size();
        assetData.lastAccess = std::chrono::steady_clock::now();
        assetData.referenceCount = 1;
        m_assetDataCache[key] = std::move(assetData);
        GN_LOG_INFO("Cached asset data for key: " + key + " (" + std::to_string(data.size()) + " bytes)");
    }

    std::vector<uint8_t> AssetManager::getCachedAssetData(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_assetDataCache.find(key);
        if (it != m_assetDataCache.end()) {
            it->second.lastAccess = std::chrono::steady_clock::now();
            it->second.referenceCount++;
            GN_LOG_INFO("Retrieved cached asset data for key: " + key + " (" + std::to_string(it->second.size) + " bytes)");
            return it->second.bytes;
        }
        return {};
    }

    std::vector<uint8_t> AssetManager::getAssetData(const std::string& name, AssetType type, int size) {
        if (!m_initialized) return {};
        
        std::string assetKey = generateAssetKey(name, type, size);
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        
        auto it = m_assetDataCache.find(assetKey);
        if (it != m_assetDataCache.end()) {
            it->second.lastAccess = std::chrono::steady_clock::now();
            it->second.referenceCount++;
            return it->second.bytes;
        }
        
        GN_LOG_WARN("Asset data not found in cache: " + assetKey);
        return {};
    }

    std::vector<uint8_t>* AssetManager::getAssetData(const std::string& name, AssetType type, int size) {
        if (!m_initialized) return nullptr;
        
        std::string assetKey = generateAssetKey(name, type, size);
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        
        auto it = m_assetDataCache.find(assetKey);
        if (it != m_assetDataCache.end()) {
            it->second.lastAccess = std::chrono::steady_clock::now();
            it->second.referenceCount++;
            return &it->second.bytes;
        }
        
        return nullptr;
    }
    
    const std::vector<uint8_t>* AssetManager::getAssetData(const std::string& name, AssetType type, int size) const {
        if (!m_initialized) return nullptr;
        
        std::string assetKey = generateAssetKey(name, type, size);
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        
        auto it = m_assetDataCache.find(assetKey);
        if (it != m_assetDataCache.end()) {
            return &it->second.bytes;
        }
        
        return nullptr;
    }
    
    void AssetManager::setAssetData(const std::string& name, AssetType type, const std::vector<uint8_t>& data, int size) {
        if (!m_initialized) return;
        
        std::string assetKey = generateAssetKey(name, type, size);
        cacheAssetData(assetKey, data);
        
        // Update registry
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            AssetInfo info;
            info.name = name;
            info.type = type;
            info.path = getAssetFullPath(name, type, "");
            info.loaded = true;
            info.loading = false;
            m_assetRegistry[assetKey] = info;
        }
        
        GN_LOG_INFO("Set asset data for: " + name + " (" + std::to_string(data.size()) + " bytes)");
    }

    bool AssetManager::releaseAssetData(const std::string& name, AssetType type, int size) {
        if (!m_initialized) return false;
        
        std::string assetKey = generateAssetKey(name, type, size);
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        
        auto it = m_assetDataCache.find(assetKey);
        if (it != m_assetDataCache.end()) {
            it->second.referenceCount--;
            if (it->second.referenceCount <= 0) {
                m_assetDataCache.erase(it);
                GN_LOG_INFO("Released and removed asset: " + assetKey);
                return true;
            }
            GN_LOG_INFO("Released asset reference: " + assetKey + " (refs: " + std::to_string(it->second.referenceCount) + ")");
        }
        return false;
    }

    void AssetManager::loadTexture(const std::string& name, const std::string& extension, 
                                   std::function<void(bool success, const std::string& error)> callback) {
        if (!m_initialized) {
            if (callback) callback(false, "AssetManager not initialized");
            return;
        }
        
        std::string fullPath = getAssetFullPath(name, AssetType::Texture, extension);
        std::string assetKey = generateAssetKey(name, AssetType::Texture);
        
        // Check cache first
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_assetRegistry.find(assetKey);
            if (it != m_assetRegistry.end()) {
                const auto& info = it->second;
                if (info.loaded) {
                    // Check if we have cached data
                    auto cachedData = getCachedAssetData(assetKey);
                    if (!cachedData.empty()) {
                        if (callback) callback(true, "");
                        return;
                    }
                }
                if (info.loading) {
                    // Queue callback for when loading completes
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_loadingCallbacks[assetKey].push_back(callback);
                    return;
                }
            }
        }
        
        // Register as loading
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            AssetInfo info;
            info.name = name;
            info.type = AssetType::Texture;
            info.loaded = false;
            info.loading = true;
            m_assetRegistry[assetKey] = info;
        }
        
        // Enqueue load request
        AssetLoadRequest request;
        request.type = AssetType::Texture;
        request.name = name;
        request.path = fullPath;
        request.key = assetKey;
        request.callback = [this, assetKey, callback](bool success, const std::string& error, const std::vector<uint8_t>& data) {
            handleAssetLoaded(assetKey, success, error, data);
            if (callback) callback(success, error);
        };
        
        enqueueAssetLoad(request);
    }

    void AssetManager::loadAudio(const std::string& name, const std::string& extension,
                                std::function<void(bool success, const std::string& error)> callback) {
        if (!m_initialized) {
            if (callback) callback(false, "AssetManager not initialized");
            return;
        }
        
        std::string fullPath = getAssetFullPath(name, AssetType::Audio, extension);
        std::string assetKey = generateAssetKey(name, AssetType::Audio);
        
        // Check cache first
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_assetRegistry.find(assetKey);
            if (it != m_assetRegistry.end()) {
                const auto& info = it->second;
                if (info.loaded) {
                    // Check if we have cached data
                    auto cachedData = getCachedAssetData(assetKey);
                    if (!cachedData.empty()) {
                        if (callback) callback(true, "");
                        return;
                    }
                }
                if (info.loading) {
                    // Queue callback for when loading completes
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_loadingCallbacks[assetKey].push_back(callback);
                    return;
                }
            }
        }
        
        // Register as loading
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            AssetInfo info;
            info.name = name;
            info.type = AssetType::Audio;
            info.loaded = false;
            info.loading = true;
            m_assetRegistry[assetKey] = info;
        }
        
        // Enqueue load request
        AssetLoadRequest request;
        request.type = AssetType::Audio;
        request.name = name;
        request.path = fullPath;
        request.key = assetKey;
        request.callback = [this, assetKey, callback](bool success, const std::string& error, const std::vector<uint8_t>& data) {
            handleAssetLoaded(assetKey, success, error, data);
            if (callback) callback(success, error);
        };
        
        enqueueAssetLoad(request);
    }

    void AssetManager::loadFont(const std::string& name, int size, const std::string& extension,
                               std::function<void(bool success, const std::string& error)> callback) {
        if (!m_initialized) {
            if (callback) callback(false, "AssetManager not initialized");
            return;
        }
        
        std::string fullPath = getAssetFullPath(name, AssetType::Font, extension);
        std::string assetKey = generateAssetKey(name, AssetType::Font, size);
        
        // Check cache first
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_assetRegistry.find(assetKey);
            if (it != m_assetRegistry.end()) {
                const auto& info = it->second;
                if (info.loaded) {
                    // Check if we have cached data
                    auto cachedData = getCachedAssetData(assetKey);
                    if (!cachedData.empty()) {
                        if (callback) callback(true, "");
                        return;
                    }
                }
                if (info.loading) {
                    // Queue callback for when loading completes
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_loadingCallbacks[assetKey].push_back(callback);
                    return;
                }
            }
        }
        
        // Register as loading
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            AssetInfo info;
            info.name = name;
            info.type = AssetType::Font;
            info.loaded = false;
            info.loading = true;
            m_assetRegistry[assetKey] = info;
        }
        
        // Enqueue load request
        AssetLoadRequest request;
        request.type = AssetType::Font;
        request.name = name;
        request.path = fullPath;
        request.key = assetKey;
        request.callback = [this, assetKey, callback](bool success, const std::string& error, const std::vector<uint8_t>& data) {
            handleAssetLoaded(assetKey, success, error, data);
            if (callback) callback(success, error);
        };
        
        enqueueAssetLoad(request);
    }

    void AssetManager::loadData(const std::string& name, const std::string& extension,
                               std::function<void(bool success, const std::string& error)> callback) {
        if (!m_initialized) {
            if (callback) callback(false, "AssetManager not initialized");
            return;
        }
        
        std::string fullPath = getAssetFullPath(name, AssetType::Data, extension);
        std::string assetKey = generateAssetKey(name, AssetType::Data);
        
        // Check cache first
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_assetRegistry.find(assetKey);
            if (it != m_assetRegistry.end()) {
                const auto& info = it->second;
                if (info.loaded) {
                    // Check if we have cached data
                    auto cachedData = getCachedAssetData(assetKey);
                    if (!cachedData.empty()) {
                        if (callback) callback(true, "");
                        return;
                    }
                }
                if (info.loading) {
                    // Queue callback for when loading completes
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_loadingCallbacks[assetKey].push_back(callback);
                    return;
                }
            }
        }
        
        // Register as loading
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            AssetInfo info;
            info.name = name;
            info.type = AssetType::Data;
            info.loaded = false;
            info.loading = true;
            m_assetRegistry[assetKey] = info;
        }
        
        // Enqueue load request
        AssetLoadRequest request;
        request.type = AssetType::Data;
        request.name = name;
        request.path = fullPath;
        request.key = assetKey;
        request.callback = [this, assetKey, callback](bool success, const std::string& error, const std::vector<uint8_t>& data) {
            handleAssetLoaded(assetKey, success, error, data);
            if (callback) callback(success, error);
        };
        
        enqueueAssetLoad(request);
    }

    void AssetManager::handleAssetLoaded(const std::string& assetKey, bool success, const std::string& error, const std::vector<uint8_t>& data) {
        if (success && !data.empty()) {
            // Cache the asset data with full metadata
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            AssetData assetData;
            assetData.bytes = data;
            assetData.size = data.size();
            assetData.lastAccess = std::chrono::steady_clock::now();
            assetData.referenceCount = 1;
            m_assetDataCache[assetKey] = std::move(assetData);
            
            GN_LOG_INFO("Cached asset: " + assetKey + " (" + std::to_string(data.size()) + " bytes)");
        } else if (!success) {
            GN_LOG_ERROR("Failed to load asset: " + assetKey + " - " + error);
        }
        
        // Update registry
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_assetRegistry.find(assetKey);
            if (it != m_assetRegistry.end()) {
                it->second.loaded = success;
                it->second.loading = false;
                if (!success) {
                    it->second.error = error;
                }
            }
        }
        
        // Execute queued callbacks
        std::vector<std::function<void(bool, const std::string&)>> callbacks;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            auto it = m_loadingCallbacks.find(assetKey);
            if (it != m_loadingCallbacks.end()) {
                callbacks = std::move(it->second);
                m_loadingCallbacks.erase(it);
            }
        }
        
        for (const auto& cb : callbacks) {
            if (cb) cb(success, error);
        }
    }

    void AssetManager::unloadAsset(const std::string& name, AssetType type) {
        if (!m_initialized) return;
        
        std::string assetKey = generateAssetKey(name, type);
        
        {
            std::lock_guard<std::mutex> lock1(m_registryMutex);
            std::lock_guard<std::mutex> lock2(m_cacheMutex);
            
            m_assetRegistry.erase(assetKey);
            m_assetDataCache.erase(assetKey);
        }
        
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_loadingCallbacks.erase(assetKey);
        }
        
        GN_LOG_INFO("Unloaded asset: " + name);
    }

    void AssetManager::unloadAllAssets() {
        if (!m_initialized) return;
        
        {
            std::lock_guard<std::mutex> lock1(m_registryMutex);
            std::lock_guard<std::mutex> lock2(m_cacheMutex);
            std::lock_guard<std::mutex> lock3(m_callbackMutex);
            
            m_assetRegistry.clear();
            m_assetDataCache.clear();
            m_loadingCallbacks.clear();
        }
        
        GN_LOG_INFO("Unloaded all assets and cleared caches");
    }

    bool AssetManager::isAssetLoaded(const std::string& name, AssetType type) const {
        if (!m_initialized) return false;
        
        std::string assetKey = generateAssetKey(name, type);
        std::lock_guard<std::mutex> lock(m_registryMutex);
        auto it = m_assetRegistry.find(assetKey);
        return it != m_assetRegistry.end() && it->second.loaded;
    }

    void AssetManager::preloadAssets(const std::vector<AssetLoadRequest>& assets,
                                   std::function<void(bool success, const std::string& error)> callback) {
        if (!m_initialized) {
            if (callback) callback(false, "AssetManager not initialized");
            return;
        }
        
        GN_LOG_INFO("Preloading " + std::to_string(assets.size()) + " assets");
        
        size_t* remaining = new size_t(assets.size());
        bool* anyFailed = new bool(false);
        
        for (const auto& asset : assets) {
            auto assetCallback = [remaining, anyFailed, callback](bool success, const std::string& error) {
                if (!success) {
                    *anyFailed = true;
                }
                
                (*remaining)--;
                if (*remaining == 0) {
                    if (callback) callback(!*anyFailed, *anyFailed ? "Some assets failed to load" : "");
                    delete remaining;
                    delete anyFailed;
                }
            };
            
            switch (asset.type) {
                case AssetType::Texture:
                    loadTexture(asset.name, asset.extension, assetCallback);
                    break;
                case AssetType::Audio:
                    loadAudio(asset.name, asset.extension, assetCallback);
                    break;
                case AssetType::Font:
                    loadFont(asset.name, asset.size, asset.extension, assetCallback);
                    break;
                case AssetType::Data:
                    loadData(asset.name, asset.extension, assetCallback);
                    break;
                default:
                    break;
            }
        }
    }

    size_t AssetManager::getMemoryUsage() const {
        size_t total = 0;
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& data : m_assetDataCache) {
            total += data.second.size();
        }
        return total;
    }

    void AssetManager::clearCache() {
        if (!m_initialized) return;
        
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_assetDataCache.clear();
        GN_LOG_INFO("Cleared asset data cache");
    }

    void AssetManager::enqueueAssetLoad(const AssetLoadRequest& request) {
        if (!g_platformDelegates) {
            if (request.callback) {
                request.callback(false, "Platform delegates not initialized", {});
            }
            return;
        }
        
        // Real asset file loading with actual byte data
        switch (request.type) {
            case AssetType::Texture:
                if (!g_platformDelegates->asset.loadTexture) {
                    loadAssetFromDisk(request);
                    return;
                }
                g_platformDelegates->asset.loadTexture(request.path.c_str(), 
                    [request](bool success, const char* error) {
                        std::vector<uint8_t> textureData;
                        if (success) {
                            textureData = readFileBytes(request.path);
                        }
                        if (request.callback) {
                            request.callback(success, error ? error : "", textureData);
                        }
                    });
                break;
                
            case AssetType::Audio:
                if (!g_platformDelegates->asset.loadAudio) {
                    loadAssetFromDisk(request);
                    return;
                }
                g_platformDelegates->asset.loadAudio(request.path.c_str(), 
                    [request](bool success, const char* error) {
                        std::vector<uint8_t> audioData;
                        if (success) {
                            audioData = readFileBytes(request.path);
                        }
                        if (request.callback) {
                            request.callback(success, error ? error : "", audioData);
                        }
                    });
                break;
                
            case AssetType::Font:
                if (!g_platformDelegates->asset.loadFont) {
                    loadAssetFromDisk(request);
                    return;
                }
                g_platformDelegates->asset.loadFont(request.path.c_str(), request.size,
                    [request](bool success, const char* error) {
                        std::vector<uint8_t> fontData;
                        if (success) {
                            fontData = readFileBytes(request.path);
                        }
                        if (request.callback) {
                            request.callback(success, error ? error : "", fontData);
                        }
                    });
                break;
                
            case AssetType::Shader:
                if (!g_platformDelegates->asset.loadShader) {
                    loadAssetFromDisk(request);
                    return;
                }
                g_platformDelegates->asset.loadShader(request.path.c_str(), "",
                    [request](bool success, const char* error) {
                        std::vector<uint8_t> shaderData;
                        if (success) {
                            shaderData = readFileBytes(request.path);
                        }
                        if (request.callback) {
                            request.callback(success, error ? error : "", shaderData);
                        }
                    });
                break;
                
            case AssetType::Data:
                if (!g_platformDelegates->asset.loadData) {
                    loadAssetFromDisk(request);
                    return;
                }
                g_platformDelegates->asset.loadData(request.path.c_str(), 
                    [request](bool success, const char* error) {
                        std::vector<uint8_t> data;
                        if (success) {
                            data = readFileBytes(request.path);
                        }
                        if (request.callback) {
                            request.callback(success, error ? error : "", data);
                        }
                    });
                break;
        }
    }

    std::vector<uint8_t> AssetManager::readFileBytes(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            GN_LOG_ERROR("Failed to open file: " + filePath + " - Check if file exists and permissions are correct");
            throw std::runtime_error("Failed to open file: " + filePath);
        }
        
        std::streamsize size = file.tellg();
        if (size < 0) {
            GN_LOG_ERROR("Failed to get file size for: " + filePath);
            throw std::runtime_error("Failed to get file size: " + filePath);
        }
        
        file.seekg(0, std::ios::beg);
        
        if (size == 0) {
            GN_LOG_WARNING("File is empty: " + filePath);
            return {};
        }
        
        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            GN_LOG_ERROR("Failed to read file data: " + filePath + " - File may be corrupted or access denied");
            throw std::runtime_error("Failed to read file data: " + filePath);
        }
        
        GN_LOG_INFO("Successfully read " + std::to_string(size) + " bytes from: " + filePath);
        return buffer;
    }

    void AssetManager::loadAssetFromDisk(const AssetLoadRequest& request) {
        try {
            std::vector<uint8_t> data = readFileBytes(request.path);
            bool success = !data.empty();
            std::string error = success ? "" : "Failed to load file: " + request.path;
            
            // Cache the asset data if successful
            if (success) {
                std::string assetKey = generateAssetKey(request.name, request.type);
                cacheAssetData(assetKey, data);
            }
            
            if (request.callback) {
                request.callback(success, error);
            }
        } catch (const std::exception& e) {
            GN_LOG_ERROR("Exception in loadAssetFromDisk: " + std::string(e.what()));
            if (request.callback) {
                request.callback(false, e.what());
            }
        }
    }

    bool AssetManager::isAssetLoaded(const std::string& name, AssetType type) const {
        if (!m_initialized) return false;
        
        std::string assetKey = generateAssetKey(name, type);
        std::lock_guard<std::mutex> lock(m_registryMutex);
        
        auto it = m_assetRegistry.find(assetKey);
        return it != m_assetRegistry.end() && it->second.loaded;
    }

    bool AssetManager::isAssetLoading(const std::string& name, AssetType type) const {
        if (!m_initialized) return false;
        
        std::string assetKey = generateAssetKey(name, type);
        std::lock_guard<std::mutex> lock(m_registryMutex);
        
        auto it = m_assetRegistry.find(assetKey);
        return it != m_assetRegistry.end() && it->second.loading;
    }

    std::string AssetManager::getAssetPath(const std::string& name, AssetType type) const {
        if (!m_initialized) return "";
        
        std::string assetKey = generateAssetKey(name, type);
        std::lock_guard<std::mutex> lock(m_registryMutex);
        
        auto it = m_assetRegistry.find(assetKey);
        return it != m_assetRegistry.end() ? it->second.path : "";
    }

    void AssetManager::unloadAsset(const std::string& name, AssetType type) {
        std::string assetKey;
        switch (type) {
            case AssetType::Texture: assetKey = "texture_" + name; break;
            case AssetType::Audio: assetKey = "audio_" + name; break;
            case AssetType::Font: assetKey = "font_" + name + "_16"; break;
            case AssetType::Shader: assetKey = "shader_" + name; break;
            case AssetType::Data: assetKey = "data_" + name; break;
        }
        
        m_assetRegistry.erase(assetKey);
        m_assetData.erase(assetKey);
        GN_LOG_INFO("Unloaded asset: " + name);
    }

    void AssetManager::unloadAllAssets() {
        m_assetRegistry.clear();
        m_assetData.clear();
        GN_LOG_INFO("Unloaded all assets");
    }

    void AssetManager::preloadEssentialAssets(std::function<void()> onComplete) {
        if (!m_initialized) {
            if (onComplete) onComplete();
            return;
        }
        
        GN_LOG_INFO("Preloading essential assets...");
        
        // Essential assets for Floppy Turd
        std::vector<std::pair<std::string, AssetType>> essentialAssets = {
            {"FloppyTurdMenu", AssetType::Audio},
            {"player", AssetType::Texture},
            {"background", AssetType::Texture},
            {"pipe", AssetType::Texture},
            {"font", AssetType::Font}
        };
        
        int totalAssets = essentialAssets.size();
        int loadedAssets = 0;
        
        for (const auto& [name, type] : essentialAssets) {
            switch (type) {
                case AssetType::Texture:
                    loadTexture(name, "png", [&loadedAssets, totalAssets, onComplete](bool success, const std::string& error) {
                        loadedAssets++;
                        if (loadedAssets == totalAssets && onComplete) {
                            onComplete();
                        }
                    });
                    break;
                case AssetType::Audio:
                    loadAudio(name, "mp3", [&loadedAssets, totalAssets, onComplete](bool success, const std::string& error) {
                        loadedAssets++;
                        if (loadedAssets == totalAssets && onComplete) {
                            onComplete();
                        }
                    });
                    break;
                case AssetType::Font:
                    loadFont(name, 16, "ttf", [&loadedAssets, totalAssets, onComplete](bool success, const std::string& error) {
                        loadedAssets++;
                        if (loadedAssets == totalAssets && onComplete) {
                            onComplete();
                        }
                    });
                    break;
                default:
                    loadedAssets++;
                    break;
            }
        }
    }

    size_t AssetManager::getMemoryUsage() const {
        size_t totalSize = 0;
        for (const auto& [key, data] : m_assetData) {
            totalSize += data.size();
        }
        return totalSize;
    }

    void AssetManager::clearCache() {
        unloadAllAssets();
    }

    // Asset Delegates Implementation
    namespace AssetDelegates {

        void loadTexture(const std::string& name, const std::string& extension, 
                        std::function<void(bool success, const std::string& error)> callback) {
            std::string fullPath = AssetManager::getInstance().getAssetPath(name, AssetType::Texture);
            
            // Use platform delegates for actual loading
            if (PlatformDelegates::loadTexture) {
                PlatformDelegates::loadTexture(fullPath.c_str(), [callback](bool success, const char* error) {
                    if (callback) {
                        callback(success, error ? std::string(error) : "");
                    }
                });
            } else {
                GN_LOG_ERROR("Platform texture loading not available");
                if (callback) callback(false, "Platform texture loading not available");
            }
        }

        void loadAudio(const std::string& name, const std::string& extension,
                      std::function<void(bool success, const std::string& error)> callback) {
            std::string fullPath = AssetManager::getInstance().getAssetPath(name, AssetType::Audio);
            
            if (PlatformDelegates::loadAudio) {
                PlatformDelegates::loadAudio(fullPath.c_str(), [callback](bool success, const char* error) {
                    if (callback) {
                        callback(success, error ? std::string(error) : "");
                    }
                });
            } else {
                GN_LOG_ERROR("Platform audio loading not available");
                if (callback) callback(false, "Platform audio loading not available");
            }
        }

        void loadFont(const std::string& name, int size, const std::string& extension,
                     std::function<void(bool success, const std::string& error)> callback) {
            std::string fullPath = AssetManager::getInstance().getAssetPath(name, AssetType::Font);
            
            if (PlatformDelegates::loadFont) {
                PlatformDelegates::loadFont(fullPath.c_str(), size, [callback](bool success, const char* error) {
                    if (callback) {
                        callback(success, error ? std::string(error) : "");
                    }
                });
            } else {
                GN_LOG_ERROR("Platform font loading not available");
                if (callback) callback(false, "Platform font loading not available");
            }
        }

        void loadData(const std::string& name, const std::string& extension,
                     std::function<void(bool success, const std::string& error)> callback) {
            std::string fullPath = AssetManager::getInstance().getAssetPath(name, AssetType::Data);
            
            if (PlatformDelegates::loadData) {
                PlatformDelegates::loadData(fullPath.c_str(), [callback](bool success, const char* error) {
                    if (callback) {
                        callback(success, error ? std::string(error) : "");
                    }
                });
            } else {
                GN_LOG_ERROR("Platform data loading not available");
                if (callback) callback(false, "Platform data loading not available");
            }
        }

        std::string getAssetPath(const std::string& relativePath, AssetType type) {
            if (PlatformDelegates::getAssetPath) {
                return PlatformDelegates::getAssetPath(relativePath.c_str());
            }
            return relativePath;
        }

        bool fileExists(const std::string& relativePath, AssetType type) {
            if (PlatformDelegates::fileExists) {
                return PlatformDelegates::fileExists(relativePath.c_str());
            }
            return false;
        }
    }

} // namespace GameCore
