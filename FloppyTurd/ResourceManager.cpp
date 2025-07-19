#include "ResourceManager.h"
#include <algorithm>
#include <filesystem>
#include <thread>
#include <iostream>
#include "GameLog.h"

void ResourceManager::Initialize(ResourceQuality quality) {
    GameLog::Log("ResourceManager::Initialize() STARTING");
    
    try {
        // Use PlatformAPI instead of PlatformLayer
        platform = &CurrentPlatformAPI::GetInstance();
        GameLog::Log("Got PlatformAPI instance: %p", platform);
        
        // Auto-detect quality based on platform if requested
        if (quality == ResourceQuality::AUTO) {
            if (platform->PreferLowPowerMode()) {
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
        if (platform->PreferLowPowerMode()) {
            maxTextureSize = platform->GetRecommendedTextureSize();
            compressionEnabled = true;
            streamingEnabled = true;
        }

        GameLog::Log("About to call RegisterAllResources()");
        RegisterAllResources();
        GameLog::Log("RegisterAllResources() completed successfully");
        
        TraceLog(LOG_INFO, "ResourceManager initialized with quality: %d", (int)currentQuality);
        GameLog::Log("ResourceManager::Initialize() COMPLETED SUCCESSFULLY");
        
    } catch (const std::exception& e) {
        GameLog::Log("Exception in ResourceManager::Initialize(): %s", e.what());
        throw; // Re-throw to be caught by Game::Initialize()
    } catch (...) {
        GameLog::Log("Unknown exception in ResourceManager::Initialize()");
        throw;
    }
}

void ResourceManager::Shutdown() {
    ClearCache();
    resourceRegistry.clear();
    TraceLog(LOG_INFO, "ResourceManager shutdown complete");
}

Texture2D ResourceManager::GetTexture(const std::string& id) {
    try {
        auto it = textureCache.find(id);
        if (it != textureCache.end() && it->second.isValid) {
            UpdateAccessTime(id);
            return it->second.resource;
        }

        // Load the texture if not cached
        if (LoadTextureInternal(id)) {
            UpdateAccessTime(id);
            return textureCache[id].resource;
        }

        // Return a placeholder texture on failure
        TraceLog(LOG_WARNING, "Failed to load texture: %s", id.c_str());
        
        // Create a placeholder texture
        static Texture2D placeholder = { 0 };
        try {
#if defined(__APPLE__) && TARGET_OS_IPHONE
            if (placeholder.texture == nullptr) {
#else
            if (placeholder.id == 0) {
#endif
                // Create a 2x2 magenta placeholder texture
                Image img = GenImageColor(2, 2, MAGENTA);
                placeholder = LoadTextureFromImage(img);
                UnloadImage(img);
            }
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Exception creating placeholder texture: %s", e.what());
            // Return empty texture if placeholder creation fails
            placeholder = { 0 };
        } catch (...) {
            TraceLog(LOG_ERROR, "Unknown exception creating placeholder texture");
            // Return empty texture if placeholder creation fails
            placeholder = { 0 };
        }
        
        return placeholder;
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Exception in GetTexture for %s: %s", id.c_str(), e.what());
        // Return empty texture on exception
        return { 0 };
    } catch (...) {
        TraceLog(LOG_ERROR, "Unknown exception in GetTexture for %s", id.c_str());
        // Return empty texture on exception
        return { 0 };
    }
}

Sound ResourceManager::GetSound(const std::string& id) {f (LoadSoundInternal(id)) {
    auto it = soundCache.find(id);        UpdateAccessTime(id);
    if (it != soundCache.end() && it->second.isValid) {source;
        UpdateAccessTime(id);
        return it->second.resource;
    }raceLog(LOG_WARNING, "Failed to load sound: %s", id.c_str());
    return { 0 };
    if (LoadSoundInternal(id)) {
        UpdateAccessTime(id);
        return soundCache[id].resource;usic ResourceManager::GetMusic(const std::string& id) {
    }    auto it = musicCache.find(id);

    TraceLog(LOG_WARNING, "Failed to load sound: %s", id.c_str());
    return { 0 };
}

Music ResourceManager::GetMusic(const std::string& id) {f (LoadMusicInternal(id)) {
    auto it = musicCache.find(id);        UpdateAccessTime(id);
    if (it != musicCache.end() && it->second.isValid) {source;
        UpdateAccessTime(id);
        return it->second.resource;
    }raceLog(LOG_WARNING, "Failed to load music: %s", id.c_str());
    return { 0 };
    if (LoadMusicInternal(id)) {
        UpdateAccessTime(id);
        return musicCache[id].resource;ont ResourceManager::GetFont(const std::string& id) {
    }    auto it = fontCache.find(id);

    TraceLog(LOG_WARNING, "Failed to load music: %s", id.c_str());
    return { 0 };
}

Font ResourceManager::GetFont(const std::string& id) {f (LoadFontInternal(id)) {
    auto it = fontCache.find(id);        UpdateAccessTime(id);
    if (it != fontCache.end() && it->second.isValid) {source;
        UpdateAccessTime(id);
        return it->second.resource;
    }raceLog(LOG_WARNING, "Failed to load font: %s", id.c_str());
    
    if (LoadFontInternal(id)) {t if we're not already trying to load whacky_joe_font
        UpdateAccessTime(id);if (id != "whacky_joe_font") {
        return fontCache[id].resource;
    }
llback font structure to prevent infinite recursion
    TraceLog(LOG_WARNING, "Failed to load font: %s", id.c_str()); fallbackFont = {0};
    
    // Guard against infinite recursion - only call GetFontDefault if we're not already trying to load whacky_joe_font= 0;
    if (id != "whacky_joe_font") {_IPHONE
    return GetFontDefault(); = nullptr;
    } else {
        // Return a basic fallback font structure to prevent infinite recursion
        Font fallbackFont = {0};f
        fallbackFont.baseSize = 16;
        fallbackFont.glyphCount = 0;
#if defined(__APPLE__) && TARGET_OS_IPHONE
        fallbackFont.texture.texture = nullptr;
#elseool ResourceManager::LoadTextureInternal(const std::string& id) {
        fallbackFont.texture.id = 0;    try {
#endif
        return fallbackFont;f (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::TEXTURE) {
    }
}

bool ResourceManager::LoadTextureInternal(const std::string& id) {onst auto& info = regIt->second;
    try {        if (info.minQuality > currentQuality) {
        auto regIt = resourceRegistry.find(id); if quality too low
        if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::TEXTURE) {
            return false;
        }td::string fullPath = ResolvePath(id, ResourceType::TEXTURE);
        TraceLog(LOG_INFO, "[ResourceManager] Resolved path for %s: %s", id.c_str(), fullPath.c_str());
        const auto& info = regIt->second;
        if (info.minQuality > currentQuality) {G, "[ResourceManager] Empty path resolved for texture ID: %s", id.c_str());
            return false; // Skip loading if quality too low
        }

        std::string fullPath = ResolvePath(id, ResourceType::TEXTURE);l LoadTexture with path: %s", fullPath.c_str());
        if (fullPath.empty()) {llPath.c_str());
            return false;E
        }   if (texture.texture == nullptr) {

        Texture2D texture = LoadTexture(fullPath.c_str());  if (texture.id == 0) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
        if (texture.texture == nullptr) {   return false;
#else        }
        if (texture.id == 0) {
#endif
            return false;ure.height > maxTextureSize) {
        }ale down large textures on mobile

        // Apply quality restrictionsomTexture(texture);
        if (texture.width > maxTextureSize || texture.height > maxTextureSize) {UnloadTexture(texture);
            // Scale down large textures on mobile
            try {
                Image img = LoadImageFromTexture(texture);ImageResize(&img, (int)(img.width * scale), (int)(img.height * scale));
                UnloadTexture(texture);
                tureFromImage(img);
                float scale = (float)maxTextureSize / std::max(img.width, img.height);
                ImageResize(&img, (int)(img.width * scale), (int)(img.height * scale));
                ERROR, "Exception scaling texture %s: %s", id.c_str(), e.what());
                texture = LoadTextureFromImage(img);e;
                UnloadImage(img);
            } catch (const std::exception& e) {ERROR, "Unknown exception scaling texture %s", id.c_str());
                TraceLog(LOG_ERROR, "Exception scaling texture %s: %s", id.c_str(), e.what());   return false;
                return false;   }
            } catch (...) {        }
                TraceLog(LOG_ERROR, "Unknown exception scaling texture %s", id.c_str());
                return false;
            }size_t memUsage = texture.width * texture.height * 4; // Assuming RGBA
        }

        // Estimate memory usage (rough calculation)> maxCacheMemoryMB * 1024 * 1024) {
        size_t memUsage = texture.width * texture.height * 4; // Assuming RGBA   TrimCache(maxCacheMemoryMB);
                }
        // Check if we need to trim cache
        if (totalMemoryUsage + memUsage > maxCacheMemoryMB * 1024 * 1024) {
            TrimCache(maxCacheMemoryMB);cached;
        }ure;

        // Cache the texture
        CachedResource<Texture2D> cached;
        cached.resource = texture;        cached.lastAccessed = GetTime();
        cached.isValid = true;
        cached.path = fullPath;
        cached.memoryUsage = memUsage;        totalMemoryUsage += memUsage;
        cached.lastAccessed = GetTime();

        textureCache[id] = cached;c_str(), texture.width, texture.height, memUsage / 1024.0f);
        totalMemoryUsage += memUsage;

        TraceLog(LOG_INFO, "Loaded texture: %s (%dx%d, %.1fKB)", ERROR, "Exception in LoadTextureInternal for %s: %s", id.c_str(), e.what());
                 id.c_str(), texture.width, texture.height, memUsage / 1024.0f);e;
        return true;
    } catch (const std::exception& e) {ERROR, "Unknown exception in LoadTextureInternal for %s", id.c_str());
        TraceLog(LOG_ERROR, "Exception in LoadTextureInternal for %s: %s", id.c_str(), e.what());   return false;
        return false;   }
    } catch (...) {}
        TraceLog(LOG_ERROR, "Unknown exception in LoadTextureInternal for %s", id.c_str());
        return false;st std::string& id) {
    }
}urceRegistry.end() || regIt->second.type != ResourceType::SOUND) {
   return false;
bool ResourceManager::LoadSoundInternal(const std::string& id) {    }
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::SOUND) {ResolvePath(id, ResourceType::SOUND);
        return false;y()) {
    }   return false;
    }
    std::string fullPath = ResolvePath(id, ResourceType::SOUND);
    if (fullPath.empty()) {());
        return false;ll be set by platform implementation
    }efined(__APPLE__) && TARGET_OS_IPHONE
{
    Sound sound = LoadSound(fullPath.c_str());
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (sound.player == nullptr) {
#else   TraceLog(LOG_ERROR, "Failed to load sound: %s", fullPath.c_str());
    if (sound.chunk == nullptr) {        return false;
#endif
        TraceLog(LOG_ERROR, "Failed to load sound: %s", fullPath.c_str());
        return false;cached;
    };

    CachedResource<Sound> cached;
    cached.resource = sound;    cached.memoryUsage = 44100 * 2 * 2; // Rough estimate for stereo 16-bit at 44.1kHz
    cached.isValid = true;tTime();
    cached.path = fullPath;
    cached.memoryUsage = 44100 * 2 * 2; // Rough estimate for stereo 16-bit at 44.1kHzd] = cached;
    cached.lastAccessed = GetTime();   totalMemoryUsage += cached.memoryUsage;
    return true;
    soundCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    return true;
}urceRegistry.find(id);
f (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::MUSIC) {
bool ResourceManager::LoadMusicInternal(const std::string& id) {        return false;
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::MUSIC) {
        return false;ath = ResolvePath(id, ResourceType::MUSIC);
    }f (fullPath.empty()) {
        return false;
    std::string fullPath = ResolvePath(id, ResourceType::MUSIC);
    if (fullPath.empty()) {
        return false;th.c_str());
    }usic.length = 0; // Length will be set by platform implementation
OS_IPHONE
    Music music = LoadMusicStream(fullPath.c_str()); (music.player == nullptr) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (music.player == nullptr) {= nullptr) {
#elsef
    if (music.music == nullptr) {        TraceLog(LOG_ERROR, "Failed to load music: %s", fullPath.c_str());
#endif
        TraceLog(LOG_ERROR, "Failed to load music: %s", fullPath.c_str());
        return false;
    }ached;

    CachedResource<Music> cached;
    cached.resource = music;    cached.path = fullPath;
    cached.isValid = true;eamingEnabled ? 1024 : 1024 * 1024; // Much smaller if streaming
    cached.path = fullPath;
    cached.memoryUsage = streamingEnabled ? 1024 : 1024 * 1024; // Much smaller if streaming
    cached.lastAccessed = GetTime();   musicCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    musicCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    return true;
}:LoadFontInternal(const std::string& id) {
uto regIt = resourceRegistry.find(id);
bool ResourceManager::LoadFontInternal(const std::string& id) {    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::FONT) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::FONT) {
        return false;
    }td::string fullPath = ResolvePath(id, ResourceType::FONT);
    if (fullPath.empty()) {
    std::string fullPath = ResolvePath(id, ResourceType::FONT);
    if (fullPath.empty()) {
        return false;
    }ont font = LoadFont(fullPath.c_str());
T_OS_IPHONE
    Font font = LoadFont(fullPath.c_str()); (font.ctFont == nullptr) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (font.ctFont == nullptr) {f (font.texture.id == 0) {
#else#endif
    if (font.texture.id == 0) {        return false;
#endif    }
        return false;
    }

ched;

    CachedResource<Font> cached;
    cached.resource = font;    cached.path = fullPath;
    cached.isValid = true;nt.texture.width * font.texture.height * 4;
    cached.path = fullPath;
    cached.memoryUsage = font.texture.width * font.texture.height * 4;
    cached.lastAccessed = GetTime();   fontCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    fontCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    return true;
}(const std::string& id, ResourceType type) {
;
std::string ResourceManager::ResolvePath(const std::string& id, ResourceType type) {
    TraceLog(LOG_INFO, "[DEBUG] ResolvePath called with id=%s, type=%d", id.c_str(), (int)type);
    
    auto it = resourceRegistry.find(id);
    if (it == resourceRegistry.end()) {
        TraceLog(LOG_ERROR, "[DEBUG] ResolvePath: Resource not found in registry: %s", id.c_str());std::string basePath = GetResourcePath(relativePath.c_str());
        return "";
    }if (type == ResourceType::TEXTURE && currentQuality != ResourceQuality::HIGH) {

    TraceLog(LOG_INFO, "[DEBUG] ResolvePath: Found resource with relativePath=%s", it->second.relativePath.c_str());    if (!qualityPath.empty() && std::filesystem::exists(qualityPath)) {
    
    std::string basePath = platform->GetResourcePath(it->second.relativePath);
    
    TraceLog(LOG_INFO, "[DEBUG] ResolvePath: GetResourcePath returned: %s", basePath.c_str());
    
    // Try quality variants for textures
    if (type == ResourceType::TEXTURE && currentQuality != ResourceQuality::HIGH) {
        std::string qualityPath = GetQualityVariant(basePath, currentQuality);string ResourceManager::GetQualityVariant(const std::string& path, ResourceQuality quality) {
        if (!qualityPath.empty() && std::filesystem::exists(qualityPath)) {    std::filesystem::path p(path);
            TraceLog(LOG_INFO, "[DEBUG] ResolvePath: Using quality variant: %s", qualityPath.c_str());
            return qualityPath;nsion = p.extension().string();
        }   std::string directory = p.parent_path().string();
    }

    TraceLog(LOG_INFO, "[DEBUG] ResolvePath: Final resolved path: %s", basePath.c_str());
    return basePath;fix = "_low"; break;
}d"; break;
H quality
std::string ResourceManager::GetQualityVariant(const std::string& path, ResourceQuality quality) {    }
    std::filesystem::path p(path);
    std::string stem = p.stem().string(); "/" + stem + suffix + extension;
    std::string extension = p.extension().string();
    std::string directory = p.parent_path().string();
{
    std::string suffix;loat currentTime = GetTime();
    switch (quality) {    
        case ResourceQuality::LOW:    suffix = "_low"; break;
        case ResourceQuality::MEDIUM: suffix = "_med"; break;   if (texIt != textureCache.end()) {
        default: return path; // Use original for HIGH quality        texIt->second.lastAccessed = currentTime;
    }

    return directory + "/" + stem + suffix + extension;
}

void ResourceManager::UpdateAccessTime(const std::string& id) {
    float currentTime = GetTime();
    
    auto texIt = textureCache.find(id);
    if (texIt != textureCache.end()) {
        texIt->second.lastAccessed = currentTime;
        return;
    }
    
    auto sndIt = soundCache.find(id);
    if (sndIt != soundCache.end()) {
        sndIt->second.lastAccessed = currentTime;
        return;;
    }
    
    auto musIt = musicCache.find(id);
    if (musIt != musicCache.end()) {t maxMemoryMB) {
        musIt->second.lastAccessed = currentTime;B * 1024 * 1024;
        return;
    }hile (totalMemoryUsage > targetMemory && !textureCache.empty()) {
           std::string lruId;
    auto fontIt = fontCache.find(id);        float oldestTime = GetTime();
    if (fontIt != fontCache.end()) {
        fontIt->second.lastAccessed = currentTime;) {
    }        if (cached.lastAccessed < oldestTime) {
}
;
void ResourceManager::TrimCache(size_t maxMemoryMB) {
    size_t targetMemory = maxMemoryMB * 1024 * 1024;}
    
    while (totalMemoryUsage > targetMemory && !textureCache.empty()) {
        std::string lruId;
        float oldestTime = GetTime();reCache.end()) {
           UnloadTexture(it->second.resource);
        for (const auto& [id, cached] : textureCache) {       totalMemoryUsage -= it->second.memoryUsage;
            if (cached.lastAccessed < oldestTime) {        textureCache.erase(it);
                oldestTime = cached.lastAccessed;INFO, "Evicted texture from cache: %s", lruId.c_str());
                lruId = id;
            }
        }
        
        if (!lruId.empty()) {
            auto it = textureCache.find(lruId);
            if (it != textureCache.end()) {
                UnloadTexture(it->second.resource);ager::ClearCache() {
                totalMemoryUsage -= it->second.memoryUsage;
                textureCache.erase(it);auto& [id, cached] : textureCache) {
                TraceLog(LOG_INFO, "Evicted texture from cache: %s", lruId.c_str());   if (cached.isValid) {
            }           UnloadTexture(cached.resource);
        } else {        }
            break; // No more resources to evict
        }
    }
}

void ResourceManager::ClearCache() {
    // Unload all cached resources
    for (auto& [id, cached] : textureCache) {
        if (cached.isValid) {
            UnloadTexture(cached.resource);
        }
    }
    
    for (auto& [id, cached] : soundCache) {
        if (cached.isValid) {
            UnloadSound(cached.resource);
        }
    }
    
    for (auto& [id, cached] : musicCache) {
        if (cached.isValid) {
            UnloadMusicStream(cached.resource);
        }
    }ache.clear();
    otalMemoryUsage = 0;
    for (auto& [id, cached] : fontCache) {
        if (cached.isValid) {esourceManager cache cleared");
            UnloadFont(cached.resource);
        }
    }ClearFontCache() {
ached resources
    textureCache.clear();    for (auto& [id, cached] : fontCache) {
    soundCache.clear();
    musicCache.clear();           UnloadFont(cached.resource);
    fontCache.clear();        }
    totalMemoryUsage = 0;

    TraceLog(LOG_INFO, "ResourceManager cache cleared");
}

void ResourceManager::ClearFontCache() {MemoryUsage = 0;
    // Unload only font cached resourcesor (const auto& [id, cached] : textureCache) {
    for (auto& [id, cached] : fontCache) {        totalMemoryUsage += cached.memoryUsage;
        if (cached.isValid) {
            UnloadFont(cached.resource);for (const auto& [id, cached] : soundCache) {
        }moryUsage;
    }

    fontCache.clear();
    
    // Recalculate total memory usage
    totalMemoryUsage = 0;che cleared");
    for (const auto& [id, cached] : textureCache) {
        totalMemoryUsage += cached.memoryUsage;
    }d::string& id, const std::string& relativePath, 
    for (const auto& [id, cached] : soundCache) {                                ResourceType type, LoadingMode mode, ResourceQuality minQuality) {
        totalMemoryUsage += cached.memoryUsage;    ResourceInfo info;
    }
    for (const auto& [id, cached] : musicCache) {   info.relativePath = relativePath;
        totalMemoryUsage += cached.memoryUsage;    info.type = type;
    }

    TraceLog(LOG_INFO, "ResourceManager font cache cleared");
}try[id] = info;

void ResourceManager::RegisterResource(const std::string& id, const std::string& relativePath, 
                                     ResourceType type, LoadingMode mode, ResourceQuality minQuality) {erAllResources() {
    ResourceInfo info;tarting RegisterAllResources - registering hundreds of resources...");
    info.id = id;
    info.relativePath = relativePath;ways load these first)
    info.type = type;   TraceLog(LOG_INFO, "[DEBUG] Registering UI and Core Resources...");
    info.loadingMode = mode;    RegisterResource("scoreboard", "ui/Score.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    info.minQuality = minQuality;rtSmall.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    ourceType::TEXTURE, LoadingMode::SYNC);
    resourceRegistry[id] = info;
}

void ResourceManager::RegisterAllResources() {SYNC);
    // UI and Core Resources (always load these first)RegisterResource("arrow_right", "ui/RightArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("scoreboard", "ui/Score.png", ResourceType::TEXTURE, LoadingMode::SYNC);ourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turd_heart", "ui/TurdHeartSmall.png", ResourceType::TEXTURE, LoadingMode::SYNC);gMode::SYNC);
    RegisterResource("coin_bag", "ui/CoinBag.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("pause_menu_bg", "ui/PauseMenuBackground.png", ResourceType::TEXTURE););
    
    // CRITICAL MISSING UI ELEMENTS - Adding these to prevent crashes
    RegisterResource("arrow_left", "ui/LeftArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_right", "ui/RightArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_left_hover", "ui/LeftArrowHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_right_hover", "ui/RightArrowHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("blue_button", "ui/BlueButton.png", ResourceType::TEXTURE, LoadingMode::SYNC);RegisterResource("turd_heart_0_half", "ui/TurdHeart0Half.png", ResourceType::TEXTURE);
    RegisterResource("blue_button_hover", "ui/BlueButtonHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);ui/TurdHeart0HalfHollow.png", ResourceType::TEXTURE);
    eType::TEXTURE);
    // CRITICAL MISSING SOUNDS - Adding these to prevent crashes
    RegisterResource("got_score", "ui/bubble.mp3", ResourceType::SOUND, LoadingMode::SYNC);
    
    // Heart variants - CRITICAL for health system
    RegisterResource("turd_heart_small", "ui/TurdHeartSmall.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_0_half", "ui/TurdHeart0Half.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_0_half_hollow", "ui/TurdHeart0HalfHollow.png", ResourceType::TEXTURE);RegisterResource("try_again_bg", "ui/TryAgainBackground.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_0_third_hollow", "ui/TurdHeart0ThirdHollow.png", ResourceType::TEXTURE);rdMorte.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_1_half", "ui/TurdHeart1Half.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_1_third", "ui/TurdHeart1Third.png", ResourceType::TEXTURE);
    RegisterResource("turd_heart_2_thirds", "ui/TurdHeart2Thirds.png", ResourceType::TEXTURE);
    
    // Game Over UI - CRITICAL for Playing classRegisterResource("hat_frame_hover", "ui/HatFrameHover.png", ResourceType::TEXTURE);
    RegisterResource("game_over_bg", "ui/GameOverBackground.png", ResourceType::TEXTURE);lected.png", ResourceType::TEXTURE);
    RegisterResource("try_again_bg", "ui/TryAgainBackground.png", ResourceType::TEXTURE);TURE);
    RegisterResource("dead_floppy", "ui/FloppyTurdMorte.png", ResourceType::TEXTURE););
    RegisterResource("game_over_score", "ui/GameOverScore.png", ResourceType::TEXTURE);
    
    // Hat Frame UI - CRITICAL for Playing class hat menu
    RegisterResource("hat_frame_normal", "ui/HatFrame.png", ResourceType::TEXTURE);RegisterResource("blue_button_hover", "ui/FloppyButtonBlueHover.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_hover", "ui/HatFrameHover.png", ResourceType::TEXTURE);arrow_up", "ui/UpArrow.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_selected", "ui/HatFrameSelected.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_locked", "ui/HatFrameLocked.png", ResourceType::TEXTURE);
    RegisterResource("hat_frame_denied", "ui/HatFrameDenied.png", ResourceType::TEXTURE);
    
    // Button textureseType::TEXTURE);
    RegisterResource("blue_button", "ui/FloppyButtonBlue.png", ResourceType::TEXTURE);XTURE);
    RegisterResource("blue_button_hover", "ui/FloppyButtonBlueHover.png", ResourceType::TEXTURE);sourceType::TEXTURE);
    RegisterResource("arrow_left", "ui/LeftArrow.png", ResourceType::TEXTURE);png", ResourceType::TEXTURE);
    RegisterResource("arrow_left_hover", "ui/LeftArrowHover.png", ResourceType::TEXTURE);RegisterResource("tp_menu_button_focused", "ui/TurdPointButtonFocused.png", ResourceType::TEXTURE);
    RegisterResource("arrow_right", "ui/RightArrow.png", ResourceType::TEXTURE);tp_menu_button_selected", "ui/TurdPointButtonClaimed.png", ResourceType::TEXTURE);
    RegisterResource("arrow_right_hover", "ui/RightArrowHover.png", ResourceType::TEXTURE);
    RegisterResource("arrow_up", "ui/UpArrow.png", ResourceType::TEXTURE);
    RegisterResource("arrow_down", "ui/DownArrow.png", ResourceType::TEXTURE);
    
    // Upgrade Menu UI
    RegisterResource("turd_point_menu", "ui/TurdPointMenu.png", ResourceType::TEXTURE);
    RegisterResource("turd_point_menu_border", "ui/SkillMenuBorder.png", ResourceType::TEXTURE);
    RegisterResource("turd_point_info", "ui/SkillPointInfoBackground.png", ResourceType::TEXTURE);RegisterResource("mute_button_locked_clicked", "ui/mutebuttonlockedclicked.png", ResourceType::TEXTURE);
    RegisterResource("tp_menu_button_locked", "ui/TurdPointButton.png", ResourceType::TEXTURE);minus_button", "ui/minusbutton.png", ResourceType::TEXTURE);
    RegisterResource("tp_menu_button_available", "ui/TurdPointButtonAvailable.png", ResourceType::TEXTURE);pe::TEXTURE);
    RegisterResource("tp_menu_button_focused", "ui/TurdPointButtonFocused.png", ResourceType::TEXTURE);TURE);
    RegisterResource("tp_menu_button_selected", "ui/TurdPointButtonClaimed.png", ResourceType::TEXTURE);
    
    // Options Menu UI
    RegisterResource("mute_button", "ui/mutebutton.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_hover", "ui/mutebuttonhover.png", ResourceType::TEXTURE);URE);
    RegisterResource("mute_button_clicked", "ui/mutebuttonclicked.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_locked", "ui/mutebuttonlocked.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_locked_hover", "ui/mutebuttonlockedhover.png", ResourceType::TEXTURE);TURE);
    RegisterResource("mute_button_locked_clicked", "ui/mutebuttonlockedclicked.png", ResourceType::TEXTURE);:TEXTURE);
    RegisterResource("minus_button", "ui/minusbutton.png", ResourceType::TEXTURE);TURE);
    RegisterResource("minus_button_hover", "ui/minusbuttonhover.png", ResourceType::TEXTURE);::TEXTURE);
    RegisterResource("minus_button_clicked", "ui/minusbuttonclicked.png", ResourceType::TEXTURE); ResourceType::TEXTURE);
    RegisterResource("plus_button", "ui/plusbutton.png", ResourceType::TEXTURE);RegisterResource("quickplay_button_selected", "mainmenu/QuickPlayButtonSelect.png", ResourceType::TEXTURE);
    RegisterResource("plus_button_hover", "ui/plusbuttonhover.png", ResourceType::TEXTURE);tions_button", "mainmenu/OptionButton.png", ResourceType::TEXTURE);
    RegisterResource("plus_button_clicked", "ui/plusbuttonclicked.png", ResourceType::TEXTURE);ourceType::TEXTURE);
    RegisterResource("volume_full", "ui/volumemeterfull.png", ResourceType::TEXTURE);URE);
    RegisterResource("volume_empty", "ui/volumemeterempty.png", ResourceType::TEXTURE);
    
    // Main Menu Buttons
    RegisterResource("play_button", "mainmenu/PlayButton.png", ResourceType::TEXTURE);
    RegisterResource("play_button_focused", "mainmenu/PlayButtonFocused.png", ResourceType::TEXTURE);
    RegisterResource("play_button_selected", "mainmenu/PlayButtonSelect.png", ResourceType::TEXTURE);
    RegisterResource("quickplay_button", "mainmenu/QuickPlayButton.png", ResourceType::TEXTURE);
    RegisterResource("quickplay_button_focused", "mainmenu/QuickPlayButtonFocused.png", ResourceType::TEXTURE);RegisterResource("got_health_big", "sounds/BigHealthPickup.wav", ResourceType::SOUND);
    RegisterResource("quickplay_button_selected", "mainmenu/QuickPlayButtonSelect.png", ResourceType::TEXTURE);
    RegisterResource("options_button", "mainmenu/OptionButton.png", ResourceType::TEXTURE);
    RegisterResource("options_button_focused", "mainmenu/OptionButtonFocused.png", ResourceType::TEXTURE);Type::SOUND);
    RegisterResource("options_button_selected", "mainmenu/OptionButtonSelect.png", ResourceType::TEXTURE);::SOUND);
    
    // Sounds (essential)
    RegisterResource("click", "sounds/confirm.ogg", ResourceType::SOUND);// Boss sounds
    RegisterResource("hurt", "sounds/hurt.mp3", ResourceType::SOUND);ss_beat", "music/BossBeatv2.mp3", ResourceType::SOUND);
    RegisterResource("got_coin", "sounds/pickup.ogg", ResourceType::SOUND);
    RegisterResource("got_health", "sounds/SmallHealthPickup.wav", ResourceType::SOUND);
    RegisterResource("got_health_big", "sounds/BigHealthPickup.wav", ResourceType::SOUND);
    RegisterResource("fart1", "sounds/fart1.ogg", ResourceType::SOUND);
    // Enemy kill soundsce("fart2", "sounds/fart2.ogg", ResourceType::SOUND);
    RegisterResource("ratcopter_kill", "sounds/ratkill.ogg", ResourceType::SOUND);
    RegisterResource("toilet_paper_kill", "sounds/tpkill.ogg", ResourceType::SOUND);
    RegisterResource("ratking_kill", "sounds/RatKingKill.ogg", ResourceType::SOUND);RegisterResource("fart5", "sounds/fart5.ogg", ResourceType::SOUND);
    fart6.ogg", ResourceType::SOUND);
    // Boss sounds
    RegisterResource("boss_beat", "music/BossBeatv2.mp3", ResourceType::SOUND);
    RegisterResource("boss_kill", "music/BossKill.ogg", ResourceType::SOUND);
    );
    // Fart sounds (critical for game));
    RegisterResource("fart1", "sounds/fart1.ogg", ResourceType::SOUND);
    RegisterResource("fart2", "sounds/fart2.ogg", ResourceType::SOUND);
    RegisterResource("fart3", "sounds/fart3.ogg", ResourceType::SOUND);ype::MUSIC, LoadingMode::STREAM);
    RegisterResource("fart4", "sounds/fart4.ogg", ResourceType::SOUND);
    RegisterResource("fart5", "sounds/fart5.ogg", ResourceType::SOUND);
    RegisterResource("fart6", "sounds/fart6.ogg", ResourceType::SOUND);
    RegisterResource("fart7", "sounds/fart7.ogg", ResourceType::SOUND);RegisterResource("turdlet_idle", "turd/TurdletIdle.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("fart8", "sounds/fart8.ogg", ResourceType::SOUND);d/TurdletJump.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("fart9", "sounds/fart9.ogg", ResourceType::SOUND);;
    RegisterResource("fart10", "sounds/fart10.ogg", ResourceType::SOUND);RegisterResource("turdlet_hurt", "turd/TurdletHurt.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("fart11", "sounds/fart11.ogg", ResourceType::SOUND);rceType::TEXTURE);
    
    // Music - CRITICAL for Playing class
    RegisterResource("game_over_music", "music/GameOver.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    
    // Player sprites - CRITICAL for gameplay (synchronous loading)EXTURE);
    RegisterResource("turdlet_idle", "turd/TurdletIdle.png", ResourceType::TEXTURE, LoadingMode::SYNC);:TEXTURE);
    RegisterResource("turdlet_jump", "turd/TurdletJump.png", ResourceType::TEXTURE, LoadingMode::SYNC);TURE);
    RegisterResource("turdlet_shoot", "turd/TurdletShoot.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turdlet_hurt", "turd/TurdletHurt.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("big_turd_idle", "turd/BigTurdIdle.png", ResourceType::TEXTURE);
    RegisterResource("big_turd_jump", "turd/BigTurdJump.png", ResourceType::TEXTURE);
    RegisterResource("big_turd_shoot", "turd/BigTurdShoot.png", ResourceType::TEXTURE);
    RegisterResource("big_turd_hurt", "turd/BigTurdHurt.png", ResourceType::TEXTURE);
    RegisterResource("teenage_turd_idle", "turd/TeenageTurdIdle.png", ResourceType::TEXTURE);ons moved down to be with hat sprites to avoid duplicates
    RegisterResource("teenage_turd_jump", "turd/TeenageTurdJump.png", ResourceType::TEXTURE);
    RegisterResource("teenage_turd_shoot", "turd/TeenageTurdShoot.png", ResourceType::TEXTURE);
    RegisterResource("teenage_turd_hurt", "turd/TeenageTurdHurt.png", ResourceType::TEXTURE);
    RegisterResource("main_menu_bg_mobile", "mainmenu/MainMenuMobile.png", ResourceType::TEXTURE);
    // Projectile sprites  );
    RegisterResource("poop_small", "turd/Floppy Poop.png", ResourceType::TEXTURE);RegisterResource("fin_logo", "mainmenu/F.png", ResourceType::TEXTURE);
    RegisterResource("poop_mid", "turd/Floppy Poop Mid.png", ResourceType::TEXTURE);urce("main_menu_music", "mainmenu/FloppyTurdMenu.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("poop_large", "turd/Floppy Poop Large.png", ResourceType::TEXTURE);", ResourceType::MUSIC, LoadingMode::STREAM);
    
    // Note: Hat registrations moved down to be with hat sprites to avoid duplicates
    rceType::TEXTURE);
    // Main Menu
    RegisterResource("main_menu_bg", "mainmenu/MainMenu.png", ResourceType::TEXTURE);
    RegisterResource("main_menu_bg_mobile", "mainmenu/MainMenuMobile.png", ResourceType::TEXTURE);RegisterResource("sewer_painting", "mainmenu/SewerLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("floppy_logo", "mainmenu/FloppyLogo.png", ResourceType::TEXTURE);desert_painting", "mainmenu/DesertLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("fin_logo", "mainmenu/F.png", ResourceType::TEXTURE);E);
    RegisterResource("main_menu_music", "mainmenu/FloppyTurdMenu.mp3", ResourceType::MUSIC, LoadingMode::STREAM);URE);
    RegisterResource("main_menu_music_alt", "mainmenu/FloppyTurdMenu Fart Variant.mp3", ResourceType::MUSIC, LoadingMode::STREAM);;
    
    // Level paintings
    RegisterResource("empty_painting", "mainmenu/EmptyPainting.png", ResourceType::TEXTURE);
    RegisterResource("locked_painting", "mainmenu/LockedPainting.png", ResourceType::TEXTURE);E, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("park_painting", "mainmenu/ParkLevelPainting.png", ResourceType::TEXTURE);RE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("sewer_painting", "mainmenu/SewerLevelPainting.png", ResourceType::TEXTURE);    RegisterResource("park_front", "environment/Level1FrontLayerBackground.png", ResourceType::TEXTURE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("desert_painting", "mainmenu/DesertLevelPainting.png", ResourceType::TEXTURE);environment/level1Clouds.png", ResourceType::TEXTURE);
    RegisterResource("snow_painting", "mainmenu/SnowLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("castle_painting", "mainmenu/CastleLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("ratking_painting", "mainmenu/RatKingPainting.png", ResourceType::TEXTURE);
e::STREAM);
    // Level 1 (Park) - High prioritygMode::STREAM);
    RegisterResource("park_back", "environment/Level1BackLayerBackground.png", ResourceType::TEXTURE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("park_mid", "environment/Level1MidLayerBackground.png", ResourceType::TEXTURE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("park_front", "environment/Level1FrontLayerBackground.png", ResourceType::TEXTURE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("park_clouds", "environment/level1Clouds.png", ResourceType::TEXTURE);
    RegisterResource("top_toilet", "environment/TopToilet.png", ResourceType::TEXTURE);    RegisterResource("sewer_wall_c", "environment/sewerwidevarC.png", ResourceType::TEXTURE);
    RegisterResource("bottom_toilet", "environment/BottomToilet.png", ResourceType::TEXTURE);sewer_wall_d", "environment/sewerwidevarD.png", ResourceType::TEXTURE);
    RegisterResource("level1_music", "music/Level1.mp3", ResourceType::MUSIC, LoadingMode::STREAM);TEXTURE);
    RegisterResource("level1_fast", "music/Level1Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);XTURE);
    RegisterResource("level1_slow", "music/Level1Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);E);
;
    // Level 2 (Sewer)
    RegisterResource("sewer_wall_a", "environment/sewerwidevarA.png", ResourceType::TEXTURE););
    RegisterResource("sewer_wall_b", "environment/sewerwidevarB.png", ResourceType::TEXTURE);REAM);
    RegisterResource("sewer_wall_c", "environment/sewerwidevarC.png", ResourceType::TEXTURE);
    RegisterResource("sewer_wall_d", "environment/sewerwidevarD.png", ResourceType::TEXTURE);
    RegisterResource("bottom_pipe_blue", "environment/BottomPipeWideBlue.png", ResourceType::TEXTURE););
    RegisterResource("bottom_pipe_orange", "environment/BottomPipeWide.png", ResourceType::TEXTURE);
    RegisterResource("top_pipe_blue", "environment/TopPipeWideBlue.png", ResourceType::TEXTURE);    RegisterResource("desert_front", "environment/Level3FrontLayerBackground.png", ResourceType::TEXTURE);
    RegisterResource("top_pipe_orange", "environment/TopPipeWide.png", ResourceType::TEXTURE);ert_cacti_layer", "environment/Cacti.png", ResourceType::TEXTURE);
    RegisterResource("level2_music", "music/Level2.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level2_fast", "music/Level2Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level2_slow", "music/Level2Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Level 3 (Desert)  
    RegisterResource("desert_back", "environment/Level3BackLayerBackground.png", ResourceType::TEXTURE);REAM);
    RegisterResource("desert_mid", "environment/Level3MidLayerBackground.png", ResourceType::TEXTURE);
    RegisterResource("desert_front", "environment/Level3FrontLayerBackground.png", ResourceType::TEXTURE);LoadingMode::STREAM);
    RegisterResource("desert_cacti_layer", "environment/Cacti.png", ResourceType::TEXTURE);
    RegisterResource("dancing_cacti_small", "environment/dancingcactismall.png", ResourceType::TEXTURE);
    RegisterResource("dancing_cacti", "environment/dancingcacti.png", ResourceType::TEXTURE);
    RegisterResource("dancing_cacti_cowboy", "environment/dancingcacticowboy.png", ResourceType::TEXTURE);
    RegisterResource("bird_idle", "enemies/BirdIdle.png", ResourceType::TEXTURE);    RegisterResource("snow_back_trees", "environment/SnowLevelBackTrees.png", ResourceType::TEXTURE);
    RegisterResource("bird_hurt", "enemies/BirdHurt.png", ResourceType::TEXTURE);"snow_tundra", "environment/SnowLevelTundra.png", ResourceType::TEXTURE);
    RegisterResource("level3_music", "music/Level3.mp3", ResourceType::MUSIC, LoadingMode::STREAM);;
    RegisterResource("level3_fast", "music/Level3Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level3_slow", "music/Level3Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
STREAM);
    // Level 4 (Snow)
    RegisterResource("snow_background", "environment/SnowLevelBackground.png", ResourceType::TEXTURE);ingMode::STREAM);
    RegisterResource("snow_mountains", "environment/SnowLevelMountains.png", ResourceType::TEXTURE);C, LoadingMode::STREAM);
    RegisterResource("snow_back_trees", "environment/SnowLevelBackTrees.png", ResourceType::TEXTURE);:STREAM);
    RegisterResource("snow_tundra", "environment/SnowLevelTundra.png", ResourceType::TEXTURE);
    RegisterResource("snow_front_trees", "environment/SnowLevelFrontTrees.png", ResourceType::TEXTURE);
    RegisterResource("snowfall", "environment/snow_tile.png", ResourceType::TEXTURE);
    RegisterResource("snowman_idle", "enemies/SnowManIdle.png", ResourceType::TEXTURE);
    RegisterResource("level4_music", "music/SnowLevel.mp3", ResourceType::MUSIC, LoadingMode::STREAM);    RegisterResource("curtains", "environment/curtains.png", ResourceType::TEXTURE);
    RegisterResource("level4_fast", "music/Level4Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);orch_pillar", "environment/TorchPillar.png", ResourceType::TEXTURE);
    RegisterResource("level4_slow", "music/Level4Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level4_snow_fast", "music/Level4SnowFast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level4_snow_slow", "music/Level4SnowSlow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);e::TEXTURE);
::TEXTURE);
    // Level 5 (Castle)
    RegisterResource("castle_wall", "environment/castlelevelbackgroundwall.png", ResourceType::TEXTURE);
    RegisterResource("castle_bars", "environment/castlelevelfloorceiling.png", ResourceType::TEXTURE);
    RegisterResource("curtains", "environment/curtains.png", ResourceType::TEXTURE);
    RegisterResource("torch_pillar", "environment/TorchPillar.png", ResourceType::TEXTURE);
    RegisterResource("chandelier", "environment/castlelevelchandelier.png", ResourceType::TEXTURE);    RegisterResource("boss_floor", "environment/BossFloor.png", ResourceType::TEXTURE);
    RegisterResource("floor_torch", "environment/castlelevelfloortorch.png", ResourceType::TEXTURE);walls", "environment/BossWalls.png", ResourceType::TEXTURE);
    RegisterResource("top_toilet_gold", "environment/TopToiletGold.png", ResourceType::TEXTURE);
    RegisterResource("bottom_toilet_gold", "environment/BottomToiletGold.png", ResourceType::TEXTURE);
    RegisterResource("level5_music", "music/Level5.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Boss Level (Level 6)
    RegisterResource("boss_background", "environment/ratkingbackground.png", ResourceType::TEXTURE);
    RegisterResource("boss_dark_clouds", "environment/darkclouds.png", ResourceType::TEXTURE);ceType::TEXTURE);
    RegisterResource("boss_floor", "environment/BossFloor.png", ResourceType::TEXTURE);urceType::TEXTURE);
    RegisterResource("boss_walls", "environment/BossWalls.png", ResourceType::TEXTURE);rceType::TEXTURE);
    RegisterResource("boss_curtains", "environment/screenCurtains.png", ResourceType::TEXTURE);gMode::STREAM);
    RegisterResource("boss_pillar", "environment/bosspillar.png", ResourceType::TEXTURE);:STREAM);
    RegisterResource("ratking_idle", "enemies/Ratking.png", ResourceType::TEXTURE);STREAM);
    RegisterResource("ratking_walk", "enemies/RatkingWalk.png", ResourceType::TEXTURE);
    RegisterResource("ratking_hurt", "enemies/RatkingHurt.png", ResourceType::TEXTURE);
    RegisterResource("ratking_death", "enemies/RatkingDeath.png", ResourceType::TEXTURE);
    RegisterResource("ratking_aim_torso", "enemies/RatkingAimTorsoOnly.png", ResourceType::TEXTURE);
    RegisterResource("ratking_aim_front_arm", "enemies/RatkingAimTossArmOnly.png", ResourceType::TEXTURE);    RegisterResource("outhouse_solo", "environment/Outhouse.png", ResourceType::TEXTURE);
    RegisterResource("ratking_aim_back_arm", "enemies/RatkingAimBackArmOnly.png", ResourceType::TEXTURE);use_toilet", "environment/OuthouseToilet.png", ResourceType::TEXTURE);
    RegisterResource("boss_music", "music/BossThemeFast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("boss_music_slow", "music/BossThemeSlow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("boss_low_health", "music/BossThemeLowHealth.ogg", ResourceType::MUSIC, LoadingMode::STREAM);E);

    // Missing Level Assets
    RegisterResource("top_toilet_snow", "environment/TopToiletSnow.png", ResourceType::TEXTURE);E);
    RegisterResource("bottom_toilet_snow", "environment/BottomToiletSnow.png", ResourceType::TEXTURE);;
    RegisterResource("outhouse_solo", "environment/Outhouse.png", ResourceType::TEXTURE);    RegisterResource("snowman_blue", "enemies/SnowManChill.png", ResourceType::TEXTURE);
    RegisterResource("outhouse_toilet", "environment/OuthouseToilet.png", ResourceType::TEXTURE);_green", "enemies/SnowManGreen.png", ResourceType::TEXTURE);
    RegisterResource("brick_wall", "objects/BrickWall.png", ResourceType::TEXTURE);;
    RegisterResource("spike_ball", "objects/SpikeBall.png", ResourceType::TEXTURE);
    RegisterResource("spike_ball_base", "objects/SpikeBallBase.png", ResourceType::TEXTURE);

    // Snowman Enemy Variants
    RegisterResource("snowman_red", "enemies/SnowManIdle.png", ResourceType::TEXTURE););
    RegisterResource("snowman_red_throw", "enemies/SnowManThrow.png", ResourceType::TEXTURE);    RegisterResource("blast_big", "vfx/blast_big.png", ResourceType::TEXTURE);
    RegisterResource("snowman_blue", "enemies/SnowManChill.png", ResourceType::TEXTURE);
    RegisterResource("snowman_green", "enemies/SnowManGreen.png", ResourceType::TEXTURE);
    RegisterResource("snowman_chad", "enemies/SnowManChad.png", ResourceType::TEXTURE);E);
    RegisterResource("snowball", "enemies/Snowball.png", ResourceType::TEXTURE);    RegisterResource("blue_coin", "objects/BlueCoin.png", ResourceType::TEXTURE);
cts/RedCoin.png", ResourceType::TEXTURE);
    // VFX textures - CRITICAL for game effects
    RegisterResource("blast_small", "vfx/blast_small.png", ResourceType::TEXTURE);XTURE);
    RegisterResource("blast_big", "vfx/blast_big.png", ResourceType::TEXTURE);ResourceType::TEXTURE);

    // Pickups - CRITICAL for gameplay
    RegisterResource("gold_coin", "objects/GoldCoin.png", ResourceType::TEXTURE);
    RegisterResource("blue_coin", "objects/BlueCoin.png", ResourceType::TEXTURE);    RegisterResource("ratcopter_hurt", "enemies/RatCopterHurt.png", ResourceType::TEXTURE);
    RegisterResource("red_coin", "objects/RedCoin.png", ResourceType::TEXTURE);", "enemies/ToiletPaperFlap.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart", "objects/PooHeart.png", ResourceType::TEXTURE);RE);
    RegisterResource("poo_heart_big", "objects/PooHeartBig.png", ResourceType::TEXTURE);ceType::TEXTURE);
    RegisterResource("poo_heart_invisible", "objects/PooHeartRainbowBeam.png", ResourceType::TEXTURE);

    // Enemies - Important for gameplay
    RegisterResource("ratcopter_idle", "enemies/RatCopterIdle.png", ResourceType::TEXTURE);    RegisterResource("cacti_b", "objects/CactiB.png", ResourceType::TEXTURE);
    RegisterResource("ratcopter_hurt", "enemies/RatCopterHurt.png", ResourceType::TEXTURE);g", ResourceType::TEXTURE);
    RegisterResource("toilet_paper_idle", "enemies/ToiletPaperFlap.png", ResourceType::TEXTURE);
    RegisterResource("toilet_paper_hurt", "enemies/ToiletPaperHit.png", ResourceType::TEXTURE);
    RegisterResource("toilet_paper_projectile", "enemies/ToiletPaperProjectile.png", ResourceType::TEXTURE);TURE);

    // Cactus variants - CRITICAL for desert level
    RegisterResource("cacti_a", "objects/CactiA.png", ResourceType::TEXTURE);RE);
    RegisterResource("cacti_b", "objects/CactiB.png", ResourceType::TEXTURE);    RegisterResource("painting_b", "objects/RabbitKnightPainting.png", ResourceType::TEXTURE);
    RegisterResource("cacti_c", "objects/CactiC.png", ResourceType::TEXTURE);ts/RatBeachPainting.png", ResourceType::TEXTURE);
    RegisterResource("cacti_d", "objects/CactiD.png", ResourceType::TEXTURE);RE);
    RegisterResource("cacti_e", "objects/CactiE.png", ResourceType::TEXTURE);
    RegisterResource("cacti_bush", "objects/CactiBush.png", ResourceType::TEXTURE);

    // Castle paintings - for decorations    RegisterResource("janitor_sweep", "objects/JanitorSweep.png", ResourceType::TEXTURE);
    RegisterResource("painting_a", "objects/CabinPainting.png", ResourceType::TEXTURE);rResource("janitor_surprise", "objects/JanitorSurprise.png", ResourceType::TEXTURE);
    RegisterResource("painting_b", "objects/RabbitKnightPainting.png", ResourceType::TEXTURE);
    RegisterResource("painting_c", "objects/RatBeachPainting.png", ResourceType::TEXTURE);
    RegisterResource("painting_d", "objects/RiverWalkPainting.png", ResourceType::TEXTURE);
    RegisterResource("boss_bar_health", "ui/BossBarHealth.png", ResourceType::TEXTURE);
    // NPCsss_bar_hurt", "ui/BossBarHurt.png", ResourceType::TEXTURE);
    RegisterResource("janitor", "objects/Janitor.png", ResourceType::TEXTURE);
    RegisterResource("janitor_sweep", "objects/JanitorSweep.png", ResourceType::TEXTURE);
    RegisterResource("janitor_surprise", "objects/JanitorSurprise.png", ResourceType::TEXTURE);TEXTURE);
    RegisterResource("achievement_unlocked", "ui/achievementiconunlocked.png", ResourceType::TEXTURE);
    // Boss bar elements
    RegisterResource("boss_bar_frame", "ui/BossBarFrame.png", ResourceType::TEXTURE);
    RegisterResource("boss_bar_health", "ui/BossBarHealth.png", ResourceType::TEXTURE););
    RegisterResource("boss_bar_hurt", "ui/BossBarHurt.png", ResourceType::TEXTURE);    RegisterResource("credits_music", "music/EndTheme.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Additional UI elements
    RegisterResource("achievement_locked", "ui/achievementicon.png", ResourceType::TEXTURE);
    RegisterResource("achievement_unlocked", "ui/achievementiconunlocked.png", ResourceType::TEXTURE);    // Use TTF version for iOS/Metal
Resource("whacky_joe_font", "fonts/Whacky_Joe.ttf", ResourceType::FONT);
    // Credits
    RegisterResource("credits_background", "ui/FloppyTurdCreditsBackground.png", ResourceType::TEXTURE);aylib
    RegisterResource("credits_music", "music/EndTheme.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
f
    // Fonts
#if defined(__APPLE__) && TARGET_OS_IPHONE
    // Use TTF version for iOS/MetalgisterResource("gold_coin", "objects/GoldCoin.png", ResourceType::TEXTURE);
    RegisterResource("whacky_joe_font", "fonts/Whacky_Joe.ttf", ResourceType::FONT);    RegisterResource("blue_coin", "objects/BlueCoin.png", ResourceType::TEXTURE);
#elserceType::TEXTURE);
    // Use FNT version for desktop/raylib
    RegisterResource("whacky_joe_font", "fonts/Whacky_Joe.fnt", ResourceType::FONT);XTURE);
#endifResourceType::TEXTURE);

    // Pickup items and objects - CRITICAL for level gameplay
    RegisterResource("gold_coin", "objects/GoldCoin.png", ResourceType::TEXTURE);
    RegisterResource("blue_coin", "objects/BlueCoin.png", ResourceType::TEXTURE);RegisterResource("brick_wall", "objects/BrickWall.png", ResourceType::TEXTURE);
    RegisterResource("red_coin", "objects/RedCoin.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart", "objects/PooHeart.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart_big", "objects/PooHeartBig.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart_invisible", "objects/PooHeartRainbowBeam.png", ResourceType::TEXTURE);RegisterResource("janitor_sweep", "objects/JanitorSweep.png", ResourceType::TEXTURE);
    , "objects/JanitorSurprise.png", ResourceType::TEXTURE);
    // CRITICAL MISSING DESERT LEVEL RESOURCES
    RegisterResource("outhouse_solo", "environment/Outhouse.png", ResourceType::TEXTURE);
    RegisterResource("brick_wall", "objects/BrickWall.png", ResourceType::TEXTURE);
        RegisterResource("flower_hat", "hats/flowerhat.png", ResourceType::TEXTURE);
    // Janitor sprites (for SewerLevel)"hats/dooraghat.png", ResourceType::TEXTURE);
    RegisterResource("janitor_idle", "objects/Janitor.png", ResourceType::TEXTURE);
    RegisterResource("janitor_sweep", "objects/JanitorSweep.png", ResourceType::TEXTURE);RE);
    RegisterResource("janitor_surprise", "objects/JanitorSurprise.png", ResourceType::TEXTURE);
TURE);
    // Hat icons (for selection UI)
    RegisterResource("cowboy_hat", "hats/cowboyhat.png", ResourceType::TEXTURE);;
    RegisterResource("flower_hat", "hats/flowerhat.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat", "hats/dooraghat.png", ResourceType::TEXTURE);;
    RegisterResource("ballcap_hat", "hats/ballcap.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat", "hats/PinwheelHat.png", ResourceType::TEXTURE);URE);
    RegisterResource("straw_hat", "hats/strawhat.png", ResourceType::TEXTURE);:TEXTURE);
    RegisterResource("samurai_hat", "hats/SamuraiHelmet.png", ResourceType::TEXTURE););
    RegisterResource("top_hat", "hats/tophat.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat", "hats/ushanka.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat", "hats/Beret.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat", "hats/Crown.png", ResourceType::TEXTURE);RegisterResource("cowboy_hat_turdlet_jump", "hats/cowboyhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat", "hats/poophat.png", ResourceType::TEXTURE);sourceType::TEXTURE);
    RegisterResource("ramses_hat", "hats/RamsesHat.png", ResourceType::TEXTURE);boy_hat_big_jump", "hats/cowboyhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat", "hats/SpartanHelmet.png", ResourceType::TEXTURE);
    RegisterResource("shell_hat", "hats/shellhat.png", ResourceType::TEXTURE);
    
    // Hat sprites - CRITICAL MISSING RESOURCES! These are what Hat constructor needs);
    // Cowboy Hat spritesRegisterResource("flower_hat_turdlet_shoot", "hats/flowerhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("cowboy_hat_turdlet_jump", "hats/cowboyhatturdletjump.png", ResourceType::TEXTURE);wer_hat_big_jump", "hats/flowerhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("cowboy_hat_turdlet_shoot", "hats/cowboyhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("cowboy_hat_big_jump", "hats/cowboyhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("cowboy_hat_big_shoot", "hats/cowboyhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Flower Hat spritesRegisterResource("doorag_hat_turdlet_shoot", "hats/dooragturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat_turdlet_jump", "hats/flowerhatturdletjump.png", ResourceType::TEXTURE);g_hat_big_jump", "hats/dooragbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat_turdlet_shoot", "hats/flowerhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat_big_jump", "hats/flowerhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat_big_shoot", "hats/flowerhatbigturdshoot.png", ResourceType::TEXTURE);
    RE);
    // Doorag Hat sprites  RegisterResource("ballcap_hat_turdlet_shoot", "hats/ballcapturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat_turdlet_jump", "hats/dooragturdletjump.png", ResourceType::TEXTURE);allcap_hat_big_jump", "hats/ballcapbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat_turdlet_shoot", "hats/dooragturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat_big_jump", "hats/dooragbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat_big_shoot", "hats/dooragbigturdshoot.png", ResourceType::TEXTURE);
    RE);
    // Ball Cap spritesRegisterResource("pinwheel_hat_turdlet_shoot", "hats/pinwheelturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat_turdlet_jump", "hats/ballcapturdletjump.png", ResourceType::TEXTURE);eel_hat_big_jump", "hats/pinwheelbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat_turdlet_shoot", "hats/ballcapturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat_big_jump", "hats/ballcapbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat_big_shoot", "hats/ballcapbigturdshoot.png", ResourceType::TEXTURE);
    
    // Pinwheel Hat spritesRegisterResource("straw_hat_turdlet_shoot", "hats/strawhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat_turdlet_jump", "hats/pinwheelturdletjump.png", ResourceType::TEXTURE);raw_hat_big_jump", "hats/strawhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat_turdlet_shoot", "hats/pinwheelturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat_big_jump", "hats/pinwheelbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat_big_shoot", "hats/pinwheelbigturdshoot.png", ResourceType::TEXTURE);
    E);
    // Straw Hat spritesRegisterResource("samurai_hat_turdlet_shoot", "hats/samuraiturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat_turdlet_jump", "hats/strawhatturdletjump.png", ResourceType::TEXTURE);rai_hat_big_jump", "hats/samuraibigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat_turdlet_shoot", "hats/strawhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat_big_jump", "hats/strawhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat_big_shoot", "hats/strawhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Samurai Hat spritesRegisterResource("top_hat_turdlet_shoot", "hats/tophatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat_turdlet_jump", "hats/samuraiturdletjump.png", ResourceType::TEXTURE);top_hat_big_jump", "hats/tophatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat_turdlet_shoot", "hats/samuraiturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat_big_jump", "hats/samuraibigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat_big_shoot", "hats/samuraibigturdshoot.png", ResourceType::TEXTURE);
    XTURE);
    // Top Hat spritesRegisterResource("ushanka_hat_turdlet_shoot", "hats/ushankaturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("top_hat_turdlet_jump", "hats/tophatturdletjump.png", ResourceType::TEXTURE);ushanka_hat_big_jump", "hats/ushankabigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("top_hat_turdlet_shoot", "hats/tophatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("top_hat_big_jump", "hats/tophatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("top_hat_big_shoot", "hats/tophatbigturdshoot.png", ResourceType::TEXTURE);
    ;
    // Ushanka spritesRegisterResource("beret_hat_turdlet_shoot", "hats/berethatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat_turdlet_jump", "hats/ushankaturdletjump.png", ResourceType::TEXTURE);("beret_hat_big_jump", "hats/berethatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat_turdlet_shoot", "hats/ushankaturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat_big_jump", "hats/ushankabigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat_big_shoot", "hats/ushankabigturdshoot.png", ResourceType::TEXTURE);
    );
    // Beret spritesRegisterResource("crown_hat_turdlet_shoot", "hats/crownhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat_turdlet_jump", "hats/berethatturdletjump.png", ResourceType::TEXTURE);("crown_hat_big_jump", "hats/crownhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat_turdlet_shoot", "hats/berethatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat_big_jump", "hats/berethatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat_big_shoot", "hats/berethatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Crown spritesRegisterResource("poop_hat_turdlet_shoot", "hats/poophatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat_turdlet_jump", "hats/crownhatturdletjump.png", ResourceType::TEXTURE);oop_hat_big_jump", "hats/poophatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat_turdlet_shoot", "hats/crownhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat_big_jump", "hats/crownhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat_big_shoot", "hats/crownhatbigturdshoot.png", ResourceType::TEXTURE);
    E);
    // Poop Hat spritesRegisterResource("ramses_hat_turdlet_shoot", "hats/ramsesturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat_turdlet_jump", "hats/poophatturdletjump.png", ResourceType::TEXTURE);ses_hat_big_jump", "hats/ramsesbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat_turdlet_shoot", "hats/poophatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat_big_jump", "hats/poophatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat_big_shoot", "hats/poophatbigturdshoot.png", ResourceType::TEXTURE);
    XTURE);
    // Ramses Hat spritesRegisterResource("spartan_hat_turdlet_shoot", "hats/spartanhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat_turdlet_jump", "hats/ramsesturdletjump.png", ResourceType::TEXTURE);tan_hat_big_jump", "hats/spartanhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat_turdlet_shoot", "hats/ramsesturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat_big_jump", "hats/ramsesbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat_big_shoot", "hats/ramsesbigturdshoot.png", ResourceType::TEXTURE);
    
    // Spartan Hat spritesRegisterResource("shell_hat_turdlet_shoot", "hats/shellhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat_turdlet_jump", "hats/spartanhatturdletjump.png", ResourceType::TEXTURE);ell_hat_big_jump", "hats/shellhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat_turdlet_shoot", "hats/spartanhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat_big_jump", "hats/spartanhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat_big_shoot", "hats/spartanhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Shell Hat sprites
    RegisterResource("shell_hat_turdlet_jump", "hats/shellhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("shell_hat_turdlet_shoot", "hats/shellhatturdletshoot.png", ResourceType::TEXTURE);   return totalMemoryUsage;
    RegisterResource("shell_hat_big_jump", "hats/shellhatbigturdjump.png", ResourceType::TEXTURE);}
    RegisterResource("shell_hat_big_shoot", "hats/shellhatbigturdshoot.png", ResourceType::TEXTURE);
urceLoaded(const std::string& id) const {
    TraceLog(LOG_INFO, "Registered %d resources", (int)resourceRegistry.size());   return textureCache.find(id) != textureCache.end() ||
}           soundCache.find(id) != soundCache.end() ||

size_t ResourceManager::GetMemoryUsage() const {
    return totalMemoryUsage;
}
Quality quality) {
bool ResourceManager::IsResourceLoaded(const std::string& id) const {   if (quality != currentQuality) {
    return textureCache.find(id) != textureCache.end() ||        currentQuality = quality;
           soundCache.find(id) != soundCache.end() ||
           musicCache.find(id) != musicCache.end() || quality changed to: %d", (int)quality);
           fontCache.find(id) != fontCache.end();
}

void ResourceManager::SetResourceQuality(ResourceQuality quality) {w public API to get resolved resource path
    if (quality != currentQuality) {td::string ResourceManager::GetResourcePathForType(const std::string& id, ResourceType type) {
        currentQuality = quality;    return ResolvePath(id, type);
        ClearCache(); // Force reload with new quality
        TraceLog(LOG_INFO, "Resource quality changed to: %d", (int)quality);
    }::ParseResourcePath(const std::string& path) {
}   ResourcePathParts parts;
    std::string working = path;
// New public API to get resolved resource path
std::string ResourceManager::GetResourcePath(const std::string& id, ResourceType type) {://", 0) == 0) {
    return ResolvePath(id, type);r(8);
}

ResourcePathParts ResourceManager::ParseResourcePath(const std::string& path) {last_of("/");
    ResourcePathParts parts;f (lastSlash != std::string::npos) {
    std::string working = path;bstr(0, lastSlash);
    // Remove asset:// prefix if present + 1);
    if (working.rfind("asset://", 0) == 0) {
        working = working.substr(8);
    }
    // Find last slash for directory
    size_t lastSlash = working.find_last_of("/");nsion
    if (lastSlash != std::string::npos) {me.find_last_of('.');
        parts.directory = working.substr(0, lastSlash);f (lastDot != std::string::npos) {
        parts.baseName = working.substr(lastSlash + 1);seName.substr(lastDot + 1);
    } else {Dot);
        parts.directory = "";
        parts.baseName = working;
    }
    // Find last dot for extensionarts;
    size_t lastDot = parts.baseName.find_last_of('.');
    if (lastDot != std::string::npos) {
        parts.extension = parts.baseName.substr(lastDot + 1);nostics methods
        parts.baseName = parts.baseName.substr(0, lastDot);t ResourceManager::GetRegisteredResourceCount() const {





}    return parts;    }        parts.extension = "";    } else {    return static_cast<int>(resourceRegistry.size());
}

void ResourceManager::LogRegisteredResources(int maxCount) const {
    if (maxCount <= 0 || maxCount > 100) {
        maxCount = 10;
    }
    
    TraceLog(LOG_INFO, "[DEBUG] Listing first %d registered resources (total: %d):", 
             maxCount, (int)resourceRegistry.size());
    
    int count = 0;
    for (const auto& pair : resourceRegistry) {
        if (count >= maxCount) break;
        
        const auto& info = pair.second;
        TraceLog(LOG_INFO, "[DEBUG] %d. %s -> %s (type: %d)", 
                count + 1, pair.first.c_str(), info.relativePath.c_str(), (int)info.type);
        count++;
    }
    
    if (resourceRegistry.size() > static_cast<size_t>(maxCount)) {
        TraceLog(LOG_INFO, "[DEBUG] ... and %d more resources", 
                (int)resourceRegistry.size() - maxCount);
    }
}

std::string ResourceManager::GetResourcePath(const std::string& id) const {
    auto it = resourceRegistry.find(id);
    if (it == resourceRegistry.end()) {
        TraceLog(LOG_WARNING, "[DEBUG] GetResourcePath: Resource '%s' not found in registry", id.c_str());
        return "";
    }
    
    const auto& info = it->second;
    std::string basePath = ::GetResourcePath(info.relativePath.c_str());
    return basePath;
}