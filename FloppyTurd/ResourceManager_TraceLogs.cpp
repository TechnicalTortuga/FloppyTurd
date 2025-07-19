#include "ResourceManager.h"
#include "PlatformTypes.h"  // For LOG_INFO, LOG_ERROR, etc.
#include <iostream>
#include <filesystem>
#include <algorithm>

void ResourceManager::Initialize(ResourceQuality quality) {
    TraceLog(LOG_INFO, "Initializing ResourceManager with quality level: %d", static_cast<int>(quality));
    
    if (quality == ResourceQuality::AUTO) {
        // Auto-detect quality based on platform
        if (PlatformAPI::GetInstance().PreferLowPowerMode()) {
            currentQuality = ResourceQuality::LOW;
        } else {
            currentQuality = ResourceQuality::HIGH;
        }
    } else {
        currentQuality = quality;
    }
    
    RegisterAllResources();
    TraceLog(LOG_INFO, "ResourceManager initialized successfully");
}

void ResourceManager::Shutdown() {
    TraceLog(LOG_INFO, "Shutting down ResourceManager");
    ClearCache();
}

// Texture loading
Texture2D ResourceManager::GetTexture(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    std::unordered_map<std::string, CachedResource<Texture2D> >::iterator it = textureCache.find(id);
    if (it != textureCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }
    
    if (LoadTextureInternal(id)) {
        return textureCache[id].resource;
    }
    
    TraceLog(LOG_ERROR, "Failed to load texture: %s", id.c_str());
    Texture2D empty = {0};
    return empty;
}

Sound ResourceManager::GetSound(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    std::unordered_map<std::string, CachedResource<Sound> >::iterator it = soundCache.find(id);
    if (it != soundCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }
    
    if (LoadSoundInternal(id)) {
        return soundCache[id].resource;
    }
    
    TraceLog(LOG_ERROR, "Failed to load sound: %s", id.c_str());
    Sound empty = {0};
    return empty;
}

Music ResourceManager::GetMusic(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    std::unordered_map<std::string, CachedResource<Music> >::iterator it = musicCache.find(id);
    if (it != musicCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }
    
    if (LoadMusicInternal(id)) {
        return musicCache[id].resource;
    }
    
    TraceLog(LOG_ERROR, "Failed to load music: %s", id.c_str());
    Music empty = {0};
    return empty;
}

Font ResourceManager::GetFont(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    std::unordered_map<std::string, CachedResource<Font> >::iterator it = fontCache.find(id);
    if (it != fontCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }
    
    if (LoadFontInternal(id)) {
        return fontCache[id].resource;
    }
    
    TraceLog(LOG_ERROR, "Failed to load font: %s", id.c_str());
    Font empty = {0};
    return empty;
}

// Async loading methods
std::future<bool> ResourceManager::LoadTextureAsync(const std::string& id) {
    return std::async(std::launch::async, [this, id]() {
        std::lock_guard<std::mutex> lock(resourceMutex);
        return LoadTextureInternal(id);
    });
}

std::future<bool> ResourceManager::LoadSoundAsync(const std::string& id) {
    return std::async(std::launch::async, [this, id]() {
        std::lock_guard<std::mutex> lock(resourceMutex);
        return LoadSoundInternal(id);
    });
}

std::future<bool> ResourceManager::LoadMusicAsync(const std::string& id) {
    return std::async(std::launch::async, [this, id]() {
        std::lock_guard<std::mutex> lock(resourceMutex);
        return LoadMusicInternal(id);
    });
}

// Internal loading implementations using PlatformAPI delegation
bool ResourceManager::LoadTextureInternal(const std::string& id) {
    std::string path = ResolvePath(id, ResourceType::TEXTURE);
    if (path.empty()) {
        TraceLog(LOG_ERROR, "Could not resolve path for texture: %s", id.c_str());
        return false;
    }
    
    try {
        Texture2D texture = PlatformAPI::GetInstance().LoadTexture(path);
        if (texture.id != 0) {
            CachedResource<Texture2D> cached;
            cached.resource = texture;
            cached.isValid = true;
            cached.path = path;
            cached.memoryUsage = texture.width * texture.height * 4; // Estimate RGBA bytes
            cached.lastAccessed = 0.0f;
            textureCache[id] = cached;
            totalMemoryUsage += cached.memoryUsage;
            return true;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Exception loading texture %s: %s", id.c_str(), e.what());
    }
    
    return false;
}

bool ResourceManager::LoadSoundInternal(const std::string& id) {
    std::string path = ResolvePath(id, ResourceType::SOUND);
    if (path.empty()) {
        TraceLog(LOG_ERROR, "Could not resolve path for sound: %s", id.c_str());
        return false;
    }
    
    try {
        Sound sound = PlatformAPI::GetInstance().LoadSound(path);
        if (sound.stream.buffer != nullptr) {
            CachedResource<Sound> cached;
            cached.resource = sound;
            cached.isValid = true;
            cached.path = path;
            cached.memoryUsage = sound.frameCount * sizeof(short) * 2; // Estimate stereo 16-bit
            cached.lastAccessed = 0.0f;
            soundCache[id] = cached;
            totalMemoryUsage += cached.memoryUsage;
            return true;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Exception loading sound %s: %s", id.c_str(), e.what());
    }
    
    return false;
}

bool ResourceManager::LoadMusicInternal(const std::string& id) {
    std::string path = ResolvePath(id, ResourceType::MUSIC);
    if (path.empty()) {
        TraceLog(LOG_ERROR, "Could not resolve path for music: %s", id.c_str());
        return false;
    }
    
    try {
        Music music = PlatformAPI::GetInstance().LoadMusic(path);
        if (music.stream.buffer != nullptr) {
            CachedResource<Music> cached;
            cached.resource = music;
            cached.isValid = true;
            cached.path = path;
            cached.memoryUsage = 1024 * 1024; // Estimate 1MB for music (streaming)
            cached.lastAccessed = 0.0f;
            musicCache[id] = cached;
            totalMemoryUsage += cached.memoryUsage;
            return true;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Exception loading music %s: %s", id.c_str(), e.what());
    }
    
    return false;
}

bool ResourceManager::LoadFontInternal(const std::string& id) {
    std::string path = ResolvePath(id, ResourceType::FONT);
    if (path.empty()) {
        TraceLog(LOG_ERROR, "Could not resolve path for font: %s", id.c_str());
        return false;
    }
    
    try {
        Font font = PlatformAPI::GetInstance().LoadFont(path, 32); // Default size
        if (font.texture.id != 0) {
            CachedResource<Font> cached;
            cached.resource = font;
            cached.isValid = true;
            cached.path = path;
            cached.memoryUsage = font.texture.width * font.texture.height * 4;
            cached.lastAccessed = 0.0f;
            fontCache[id] = cached;
            totalMemoryUsage += cached.memoryUsage;
            return true;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Exception loading font %s: %s", id.c_str(), e.what());
    }
    
    return false;
}

// Path resolution and utility methods
std::string ResourceManager::ResolvePath(const std::string& id, ResourceType type) {
    std::unordered_map<std::string, ResourceInfo>::iterator regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end()) {
        TraceLog(LOG_WARNING, "Resource not registered: %s", id.c_str());
        return "";
    }
    
    const ResourceInfo& info = regIt->second;
    std::string basePath = PlatformAPI::GetInstance().GetResourcePath();
    std::string fullPath = basePath + "/" + info.relativePath;
    return GetQualityVariant(fullPath, currentQuality);
}

std::string ResourceManager::GetQualityVariant(const std::string& path, ResourceQuality quality) {
    if (quality == ResourceQuality::AUTO) {
        quality = currentQuality;
    }
    
    std::filesystem::path p(path);
    std::string basename = p.stem().string();
    std::string extension = p.extension().string();
    std::string directory = p.parent_path().string();
    
    // Try quality-specific variants first
    std::vector<std::string> qualityPrefixes;
    switch (quality) {
        case ResourceQuality::HIGH:
            qualityPrefixes.push_back("_hd");
            qualityPrefixes.push_back("_high");
            qualityPrefixes.push_back("");
            break;
        case ResourceQuality::MEDIUM:
            qualityPrefixes.push_back("_md");
            qualityPrefixes.push_back("_medium");
            qualityPrefixes.push_back("");
            break;
        case ResourceQuality::LOW:
            qualityPrefixes.push_back("_low");
            qualityPrefixes.push_back("_ld");
            qualityPrefixes.push_back("");
            break;
        default:
            qualityPrefixes.push_back("");
            break;
    }
    
    for (size_t i = 0; i < qualityPrefixes.size(); ++i) {
        const std::string& prefix = qualityPrefixes[i];
        std::string candidatePath = directory + "/" + basename + prefix + extension;
        
        if (PlatformAPI::GetInstance().FileExists(candidatePath)) {
            return candidatePath;
        }
    }
    
    return path; // Fallback to original path
}

// Resource management methods
void ResourceManager::PreloadLevel(int levelIndex) {
    TraceLog(LOG_INFO, "Preloading resources for level: %d", levelIndex);
    // Implementation would depend on level organization
}

void ResourceManager::UnloadLevel(int levelIndex) {
    TraceLog(LOG_INFO, "Unloading resources for level: %d", levelIndex);
    // Implementation would depend on level organization
}

void ResourceManager::ClearCache() {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    // Unload all textures using PlatformAPI
    for (std::unordered_map<std::string, CachedResource<Texture2D> >::iterator it = textureCache.begin(); it != textureCache.end(); ++it) {
        if (it->second.isValid) {
            PlatformAPI::GetInstance().UnloadTexture(it->second.resource);
        }
    }
    textureCache.clear();
    
    // Unload all sounds using PlatformAPI
    for (std::unordered_map<std::string, CachedResource<Sound> >::iterator it = soundCache.begin(); it != soundCache.end(); ++it) {
        if (it->second.isValid) {
            PlatformAPI::GetInstance().UnloadSound(it->second.resource);
        }
    }
    soundCache.clear();
    
    // Unload all music using PlatformAPI
    for (std::unordered_map<std::string, CachedResource<Music> >::iterator it = musicCache.begin(); it != musicCache.end(); ++it) {
        if (it->second.isValid) {
            PlatformAPI::GetInstance().UnloadMusic(it->second.resource);
        }
    }
    musicCache.clear();
    
    // Unload all fonts using PlatformAPI
    for (std::unordered_map<std::string, CachedResource<Font> >::iterator it = fontCache.begin(); it != fontCache.end(); ++it) {
        if (it->second.isValid) {
            PlatformAPI::GetInstance().UnloadFont(it->second.resource);
        }
    }
    fontCache.clear();
    
    totalMemoryUsage = 0;
}

void ResourceManager::ClearFontCache() {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    for (std::unordered_map<std::string, CachedResource<Font> >::iterator it = fontCache.begin(); it != fontCache.end(); ++it) {
        if (it->second.isValid) {
            totalMemoryUsage -= it->second.memoryUsage;
            PlatformAPI::GetInstance().UnloadFont(it->second.resource);
        }
    }
    fontCache.clear();
}

void ResourceManager::TrimCache(size_t maxMemoryMB) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    size_t maxBytes = maxMemoryMB * 1024 * 1024;
    while (totalMemoryUsage > maxBytes && !textureCache.empty()) {
        EvictLRUResource();
    }
}

void ResourceManager::SetResourceQuality(ResourceQuality quality) {
    if (quality != currentQuality) {
        TraceLog(LOG_INFO, "Changing resource quality from %d to %d", static_cast<int>(currentQuality), static_cast<int>(quality));
        currentQuality = quality;
        ClearCache(); // Force reload with new quality
    }
}

size_t ResourceManager::GetMemoryUsage() const {
    return totalMemoryUsage;
}

void ResourceManager::EnableTextureCompression(bool enable) {
    compressionEnabled = enable;
    TraceLog(LOG_INFO, "Texture compression %s", enable ? "enabled" : "disabled");
}

void ResourceManager::SetMaxTextureSize(int maxSize) {
    maxTextureSize = maxSize;
    TraceLog(LOG_INFO, "Max texture size set to: %d", maxSize);
}

void ResourceManager::EnableResourceStreaming(bool enable) {
    streamingEnabled = enable;
    TraceLog(LOG_INFO, "Resource streaming %s", enable ? "enabled" : "disabled");
}

// Utility methods
bool ResourceManager::IsResourceLoaded(const std::string& id) const {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    return (textureCache.find(id) != textureCache.end() && textureCache.find(id)->second.isValid) ||
           (soundCache.find(id) != soundCache.end() && soundCache.find(id)->second.isValid) ||
           (musicCache.find(id) != musicCache.end() && musicCache.find(id)->second.isValid) ||
           (fontCache.find(id) != fontCache.end() && fontCache.find(id)->second.isValid);
}

void ResourceManager::ReloadResource(const std::string& id) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    // Determine resource type and reload
    std::unordered_map<std::string, ResourceInfo>::iterator regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end()) {
        TraceLog(LOG_WARNING, "Cannot reload unregistered resource: %s", id.c_str());
        return;
    }
    
    switch (regIt->second.type) {
        case ResourceType::TEXTURE:
            textureCache.erase(id);
            LoadTextureInternal(id);
            break;
        case ResourceType::SOUND:
            soundCache.erase(id);
            LoadSoundInternal(id);
            break;
        case ResourceType::MUSIC:
            musicCache.erase(id);
            LoadMusicInternal(id);
            break;
        case ResourceType::FONT:
            fontCache.erase(id);
            LoadFontInternal(id);
            break;
    }
}

std::vector<std::string> ResourceManager::GetLoadedResources() const {
    std::lock_guard<std::mutex> lock(resourceMutex);
    
    std::vector<std::string> loaded;
    for (std::unordered_map<std::string, CachedResource<Texture2D> >::const_iterator it = textureCache.begin(); it != textureCache.end(); ++it) {
        if (it->second.isValid) loaded.push_back(it->first);
    }
    for (std::unordered_map<std::string, CachedResource<Sound> >::const_iterator it = soundCache.begin(); it != soundCache.end(); ++it) {
        if (it->second.isValid) loaded.push_back(it->first);
    }
    for (std::unordered_map<std::string, CachedResource<Music> >::const_iterator it = musicCache.begin(); it != musicCache.end(); ++it) {
        if (it->second.isValid) loaded.push_back(it->first);
    }
    for (std::unordered_map<std::string, CachedResource<Font> >::const_iterator it = fontCache.begin(); it != fontCache.end(); ++it) {
        if (it->second.isValid) loaded.push_back(it->first);
    }
    return loaded;
}

std::string ResourceManager::GetResourcePathForType(const std::string& id, ResourceType type) {
    return ResolvePath(id, type);
}

int ResourceManager::GetRegisteredResourceCount() const {
    return static_cast<int>(resourceRegistry.size());
}

void ResourceManager::LogRegisteredResources(int maxCount) const {
    TraceLog(LOG_INFO, "Registered resources (%d of %d):", (maxCount < static_cast<int>(resourceRegistry.size())) ? maxCount : static_cast<int>(resourceRegistry.size()), static_cast<int>(resourceRegistry.size()));
    
    int count = 0;
    for (std::unordered_map<std::string, ResourceInfo>::const_iterator it = resourceRegistry.begin(); it != resourceRegistry.end() && count < maxCount; ++it, ++count) {
        TraceLog(LOG_INFO, "  %s -> %s", it->first.c_str(), it->second.relativePath.c_str());
    }
}

std::string ResourceManager::GetResourcePath(const std::string& id) const {
    std::unordered_map<std::string, ResourceInfo>::const_iterator regIt = resourceRegistry.find(id);
    if (regIt != resourceRegistry.end()) {
        return regIt->second.relativePath;
    }
    return "";
}

ResourcePathParts ResourceManager::ParseResourcePath(const std::string& path) {
    std::filesystem::path p(path);
    ResourcePathParts parts;
    parts.directory = p.parent_path().string();
    parts.baseName = p.stem().string();
    parts.extension = p.extension().string();
    return parts;
}

// Private helper methods
void ResourceManager::UpdateAccessTime(const std::string& id) {
    // Update access time for LRU cache (simplified)
    frameTime += 1.0f; // Increment frame counter
}

void ResourceManager::EvictLRUResource() {
    // Find and evict least recently used resource
    // Simplified implementation - would need proper LRU tracking
    if (!textureCache.empty()) {
        std::unordered_map<std::string, CachedResource<Texture2D> >::iterator it = textureCache.begin();
        totalMemoryUsage -= it->second.memoryUsage;
        if (it->second.isValid) {
            PlatformAPI::GetInstance().UnloadTexture(it->second.resource);
        }
        textureCache.erase(it);
    }
}

void ResourceManager::RegisterAllResources() {
    TraceLog(LOG_INFO, "Registering all resources...");
    
    // UI and Core Resources (always load these first)
    RegisterResource("arrow_left", "ui/LeftArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_right", "ui/RightArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_left_hover", "ui/LeftArrowHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_right_hover", "ui/RightArrowHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_up", "ui/UpArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_down", "ui/DownArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("blue_button", "ui/BlueButton.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("blue_button_hover", "ui/BlueButtonHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("scoreboard", "ui/Score.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turd_heart", "ui/TurdHeartSmall.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("coin_bag", "ui/CoinBag.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("pause_menu_bg", "ui/PauseMenuBackground.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    
    // Heart variants - CRITICAL for health system
    RegisterResource("turd_heart_small", "ui/TurdHeartSmall.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_0_half", "ui/TurdHeart0Half.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_0_half_hollow", "ui/TurdHeart0HalfHollow.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_0_third_hollow", "ui/TurdHeart0ThirdHollow.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_1_half", "ui/TurdHeart1Half.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_1_third", "ui/TurdHeart1Third.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_2_thirds", "ui/TurdHeart2Thirds.png", ResourceType::TEXTURE);
    
    // Game Over UI - CRITICAL for Playing class
    RegisterResource("game_over_bg", "ui/GameOverBackground.png", ResourceType::TEXTURE);
    RegisterResource("try_again_bg", "ui/TryAgainBackground.png", ResourceType::TEXTURE);
    RegisterResource("dead_floppy", "ui/FloppyTurdMorte.png", ResourceType::TEXTURE);
    RegisterResource("game_over_score", "ui/GameOverScore.png", ResourceType::TEXTURE);
    
    // Hat Frame UI - CRITICAL for Playing class hat menu
    RegisterResource("hat_frame_normal", "ui/HatFrame.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_hover", "ui/HatFrameHover.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_selected", "ui/HatFrameSelected.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_locked", "ui/HatFrameLocked.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_denied", "ui/HatFrameDenied.png", ResourceType::TEXTURE);
    
    // Continue with remaining resources registration...
    // Player sprites - CRITICAL for gameplay (synchronous loading)
    RegisterResource("turdlet_idle", "turd/TurdletIdle.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turdlet_jump", "turd/TurdletJump.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turdlet_shoot", "turd/TurdletShoot.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turdlet_hurt", "turd/TurdletHurt.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    
    // Sounds (essential)
    RegisterResource("click", "sounds/confirm.ogg", ResourceType::SOUND);
    RegisterResource("hurt", "sounds/hurt.mp3", ResourceType::SOUND);
    RegisterResource("got_coin", "sounds/pickup.ogg", ResourceType::SOUND);
    RegisterResource("got_health", "sounds/SmallHealthPickup.wav", ResourceType::SOUND);
    RegisterResource("got_health_big", "sounds/BigHealthPickup.wav", ResourceType::SOUND);
    RegisterResource("got_score", "ui/bubble.mp3", ResourceType::SOUND, LoadingMode::SYNC);
    
    // Fonts
    RegisterResource("whacky_joe_font", "fonts/Whacky_Joe.ttf", ResourceType::FONT);
    
    TraceLog(LOG_INFO, "Resource registration completed. Total resources: %d", static_cast<int>(resourceRegistry.size()));
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
