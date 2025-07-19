#include "ResourceManager.h"
#include "PlatformAPI.h"
#include "GameLog.h"
#include <algorithm>

// Simple, clean ResourceManager implementation that delegates to platform-specific implementations
// No platform conditionals - everything goes through PlatformAPI

void ResourceManager::Initialize(ResourceQuality quality) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    try {
        // Auto-detect quality based on platform if requested
        if (quality == ResourceQuality::AUTO) {
            if (PlatformAPI::GetInstance().PreferLowPowerMode()) {
                currentQuality = ResourceQuality::LOW;
                maxCacheMemoryMB = 25;  // Limit cache on low-power devices
            } else {
                currentQuality = ResourceQuality::HIGH;
                maxCacheMemoryMB = 100;
            }
        } else {
            currentQuality = quality;
        }

        // Platform-specific optimizations
        if (PlatformAPI::GetInstance().PreferLowPowerMode()) {
            maxTextureSize = PlatformAPI::GetInstance().GetRecommendedTextureSize();
        }

        // Register all resources
        RegisterAllResources();
        
        GameLog::Log("ResourceManager initialized with quality: %d", (int)currentQuality);
        
    } catch (const std::exception& e) {
        GameLog::Log("Exception during ResourceManager initialization: %s", e.what());
        throw;
    } catch (...) {
        GameLog::Log("Unknown exception during ResourceManager initialization");
        throw;
    }
}

void ResourceManager::Shutdown() {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    ClearCache();
    GameLog::Log("ResourceManager shutdown complete");
}

Texture2D ResourceManager::GetTexture(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    try {
        // Check cache first
        auto it = textureCache.find(id);
        if (it != textureCache.end() && it->second.isValid) {
            UpdateAccessTime(id);
            return it->second.resource;
        }
        
        // Load if not in cache
        if (LoadTextureInternal(id)) {
            UpdateAccessTime(id);
            return textureCache[id].resource;
        }
        
        GameLog::Log("Failed to load texture: %s", id.c_str());
        
        // Return placeholder texture
        static Texture2D placeholder = { 0 };
        if (placeholder.id == 0) {
            try {
                // Create simple placeholder texture using PlatformAPI
                placeholder.id = 1; // Simple placeholder
                placeholder.width = 2;
                placeholder.height = 2;
                placeholder.mipmaps = 1;
                placeholder.format = 7; // UNCOMPRESSED_R8G8B8A8
            } catch (const std::exception& e) {
                GameLog::Log("Exception creating placeholder texture: %s", e.what());
                placeholder = { 0 };
            } catch (...) {
                GameLog::Log("Unknown exception creating placeholder texture");
                placeholder = { 0 };
            }
        }
        
        return placeholder;
        
    } catch (const std::exception& e) {
        GameLog::Log("Exception in GetTexture for %s: %s", id.c_str(), e.what());
        return { 0 };
    } catch (...) {
        GameLog::Log("Unknown exception in GetTexture for %s", id.c_str());
        return { 0 };
    }
}

Sound ResourceManager::GetSound(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    // Check cache first
    auto it = soundCache.find(id);
    if (it != soundCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }
    
    // Load if not in cache
    if (LoadSoundInternal(id)) {
        UpdateAccessTime(id);
        return soundCache[id].resource;
    }
    
    GameLog::Log("Failed to load sound: %s", id.c_str());
    return { 0 };
}

Music ResourceManager::GetMusic(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    // Check cache first
    auto it = musicCache.find(id);
    if (it != musicCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }
    
    // Load if not in cache
    if (LoadMusicInternal(id)) {
        UpdateAccessTime(id);
        return musicCache[id].resource;
    }
    
    GameLog::Log("Failed to load music: %s", id.c_str());
    return { 0 };
}

Font ResourceManager::GetFont(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    // Check cache first
    auto it = fontCache.find(id);
    if (it != fontCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }
    
    // Load if not in cache
    if (LoadFontInternal(id)) {
        UpdateAccessTime(id);
        return fontCache[id].resource;
    }
    
    GameLog::Log("Failed to load font: %s", id.c_str());
    
    // Return default font as fallback
    return PlatformAPI::GetInstance().GetFontDefault();
}

std::string ResourceManager::ResolvePath(const std::string& id, ResourceType type) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end()) {
        GameLog::Log("Resource %s not found in registry", id.c_str());
        return id;
    }
    
    const auto& info = regIt->second;
    std::string basePath = PlatformAPI::GetInstance().GetResourcePath(info.relativePath.c_str());
    
    // Get quality variant if needed
    std::string fullPath = GetQualityVariant(basePath, currentQuality);
    
    GameLog::Log("[ResourceManager] Resolved path for %s: %s", id.c_str(), fullPath.c_str());
    return fullPath;
}

bool ResourceManager::LoadTextureInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::TEXTURE) {
        return false;
    }
    
    const auto& info = regIt->second;
    std::string fullPath = ResolvePath(id, ResourceType::TEXTURE);
    
    try {
        // Load texture using PlatformAPI
        Texture2D texture = PlatformAPI::GetInstance().LoadTexture(fullPath.c_str());
        
        if (texture.id > 0) {
            // Scale texture if needed for mobile optimization
            if (maxTextureSize > 0 && (texture.width > maxTextureSize || texture.height > maxTextureSize)) {
                // For now, just log - texture scaling can be implemented later if needed
                GameLog::Log("Texture %s exceeds max size (%dx%d > %d)", id.c_str(), texture.width, texture.height, maxTextureSize);
            }
            
            // Cache the texture
            textureCache[id] = { texture, true, std::chrono::steady_clock::now() };
            
            // Track memory usage (rough estimate)
            size_t textureMemory = texture.width * texture.height * 4; // Assume RGBA
            totalMemoryUsage += textureMemory;
            
            GameLog::Log("Loaded texture: %s (%dx%d, %.1fKB)", id.c_str(), texture.width, texture.height, textureMemory / 1024.0f);
            return true;
        }
        
    } catch (const std::exception& e) {
        GameLog::Log("Exception in LoadTextureInternal for %s: %s", id.c_str(), e.what());
        return false;
    } catch (...) {
        GameLog::Log("Unknown exception in LoadTextureInternal for %s", id.c_str());
        return false;
    }
    
    return false;
}

bool ResourceManager::LoadSoundInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::SOUND) {
        return false;
    }
    
    std::string fullPath = ResolvePath(id, ResourceType::SOUND);
    
    try {
        Sound sound = PlatformAPI::GetInstance().LoadSound(fullPath.c_str());
        
        if (sound.id > 0) {
            soundCache[id] = { sound, true, std::chrono::steady_clock::now() };
            GameLog::Log("Loaded sound: %s", id.c_str());
            return true;
        }
        
    } catch (const std::exception& e) {
        GameLog::Log("Exception loading sound %s: %s", id.c_str(), e.what());
    }
    
    GameLog::Log("Failed to load sound: %s", fullPath.c_str());
    return false;
}

bool ResourceManager::LoadMusicInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::MUSIC) {
        return false;
    }
    
    std::string fullPath = ResolvePath(id, ResourceType::MUSIC);
    
    try {
        Music music = PlatformAPI::GetInstance().LoadMusic(fullPath.c_str());
        
        if (music.id > 0) {
            musicCache[id] = { music, true, std::chrono::steady_clock::now() };
            GameLog::Log("Loaded music: %s", id.c_str());
            return true;
        }
        
    } catch (const std::exception& e) {
        GameLog::Log("Exception loading music %s: %s", id.c_str(), e.what());
    }
    
    GameLog::Log("Failed to load music: %s", fullPath.c_str());
    return false;
}

bool ResourceManager::LoadFontInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::FONT) {
        return false;
    }
    
    std::string fullPath = ResolvePath(id, ResourceType::FONT);
    
    try {
        Font font = PlatformAPI::GetInstance().LoadFont(fullPath.c_str());
        
        if (font.id > 0) {
            fontCache[id] = { font, true, std::chrono::steady_clock::now() };
            GameLog::Log("Loaded font: %s", id.c_str());
            return true;
        }
        
    } catch (const std::exception& e) {
        GameLog::Log("Exception loading font %s: %s", id.c_str(), e.what());
    }
    
    GameLog::Log("Failed to load font: %s", fullPath.c_str());
    return false;
}

void ResourceManager::ClearCache() {
    // Unload all cached resources using PlatformAPI
    for (auto& pair : textureCache) {
        if (pair.second.isValid) {
            PlatformAPI::GetInstance().UnloadTexture(pair.second.resource);
        }
    }
    
    for (auto& pair : soundCache) {
        if (pair.second.isValid) {
            PlatformAPI::GetInstance().UnloadSound(pair.second.resource);
        }
    }
    
    for (auto& pair : musicCache) {
        if (pair.second.isValid) {
            PlatformAPI::GetInstance().UnloadMusic(pair.second.resource);
        }
    }
    
    for (auto& pair : fontCache) {
        if (pair.second.isValid) {
            PlatformAPI::GetInstance().UnloadFont(pair.second.resource);
        }
    }
    
    // Clear all caches
    textureCache.clear();
    soundCache.clear();
    musicCache.clear();
    fontCache.clear();
    
    totalMemoryUsage = 0;
    
    GameLog::Log("Resource cache cleared");
}

void ResourceManager::UpdateAccessTime(const std::string& id) {
    auto now = std::chrono::steady_clock::now();
    
    auto texIt = textureCache.find(id);
    if (texIt != textureCache.end()) {
        texIt->second.lastAccessed = now;
        return;
    }
    
    auto soundIt = soundCache.find(id);
    if (soundIt != soundCache.end()) {
        soundIt->second.lastAccessed = now;
        return;
    }
    
    auto musicIt = musicCache.find(id);
    if (musicIt != musicCache.end()) {
        musicIt->second.lastAccessed = now;
        return;
    }
    
    auto fontIt = fontCache.find(id);
    if (fontIt != fontCache.end()) {
        fontIt->second.lastAccessed = now;
        return;
    }
}

std::string ResourceManager::GetQualityVariant(const std::string& path, ResourceQuality quality) {
    // Simple implementation - can be enhanced later
    return path;
}

void ResourceManager::RegisterAllResources() {
    // Register common game resources
    // This would typically be loaded from a configuration file
    
    // Example texture registrations
    RegisterResource("player_idle", "sprites/player_idle.png", ResourceType::TEXTURE);
    RegisterResource("player_walk", "sprites/player_walk.png", ResourceType::TEXTURE);
    RegisterResource("background", "sprites/background.png", ResourceType::TEXTURE);
    
    // Example sound registrations
    RegisterResource("jump_sound", "audio/jump.wav", ResourceType::SOUND);
    RegisterResource("collect_sound", "audio/collect.wav", ResourceType::SOUND);
    
    // Example music registrations
    RegisterResource("main_theme", "audio/main_theme.ogg", ResourceType::MUSIC);
    
    // Example font registrations
    RegisterResource("ui_font", "fonts/ui_font.ttf", ResourceType::FONT);
    
    GameLog::Log("Registered %d resources", (int)resourceRegistry.size());
}

void ResourceManager::RegisterResource(const std::string& id, const std::string& relativePath, 
                                     ResourceType type, LoadingMode mode, ResourceQuality minQuality) {
    ResourceInfo info;
    info.id = id;
    info.relativePath = relativePath;
    info.type = type;
    info.loadingMode = mode;
    info.minQuality = minQuality;
    info.isLoaded = false;
    info.isLoading = false;
    
    resourceRegistry[id] = info;
}

// Additional utility methods
size_t ResourceManager::GetMemoryUsage() const {
    std::lock_guard<std::mutex> lock(resourceMutex);
    return totalMemoryUsage;
}

bool ResourceManager::IsResourceLoaded(const std::string& id) const {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    auto texIt = textureCache.find(id);
    if (texIt != textureCache.end()) return texIt->second.isValid;
    
    auto soundIt = soundCache.find(id);
    if (soundIt != soundCache.end()) return soundIt->second.isValid;
    
    auto musicIt = musicCache.find(id);
    if (musicIt != musicCache.end()) return musicIt->second.isValid;
    
    auto fontIt = fontCache.find(id);
    if (fontIt != fontCache.end()) return fontIt->second.isValid;
    
    return false;
}

void ResourceManager::SetResourceQuality(ResourceQuality quality) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    if (currentQuality != quality) {
        currentQuality = quality;
        // Could trigger resource reloading here if needed
        GameLog::Log("Resource quality changed to: %d", (int)quality);
    }
}
