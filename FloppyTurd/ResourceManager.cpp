#include "ResourceManager.h"
#include <algorithm>
#include <filesystem>
#include <thread>

void ResourceManager::Initialize(ResourceQuality quality) {
    platform = &PlatformLayer::GetInstance();
    
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

    RegisterAllResources();
    TraceLog(LOG_INFO, "ResourceManager initialized with quality: %d", (int)currentQuality);
}

void ResourceManager::Shutdown() {
    ClearCache();
    resourceRegistry.clear();
    TraceLog(LOG_INFO, "ResourceManager shutdown complete");
}

Texture2D ResourceManager::GetTexture(const std::string& id) {
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
    static Texture2D placeholder = { 0 };
    if (placeholder.id == 0) {
        // Create a 2x2 magenta placeholder texture
        Image img = GenImageColor(2, 2, MAGENTA);
        placeholder = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    return placeholder;
}

Sound ResourceManager::GetSound(const std::string& id) {
    auto it = soundCache.find(id);
    if (it != soundCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }

    if (LoadSoundInternal(id)) {
        UpdateAccessTime(id);
        return soundCache[id].resource;
    }

    TraceLog(LOG_WARNING, "Failed to load sound: %s", id.c_str());
    return { 0 };
}

Music ResourceManager::GetMusic(const std::string& id) {
    auto it = musicCache.find(id);
    if (it != musicCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }

    if (LoadMusicInternal(id)) {
        UpdateAccessTime(id);
        return musicCache[id].resource;
    }

    TraceLog(LOG_WARNING, "Failed to load music: %s", id.c_str());
    return { 0 };
}

Font ResourceManager::GetFont(const std::string& id) {
    auto it = fontCache.find(id);
    if (it != fontCache.end() && it->second.isValid) {
        UpdateAccessTime(id);
        return it->second.resource;
    }

    if (LoadFontInternal(id)) {
        UpdateAccessTime(id);
        return fontCache[id].resource;
    }

    TraceLog(LOG_WARNING, "Failed to load font: %s", id.c_str());
    return GetFontDefault();
}

bool ResourceManager::LoadTextureInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::TEXTURE) {
        return false;
    }

    const auto& info = regIt->second;
    if (info.minQuality > currentQuality) {
        return false; // Skip loading if quality too low
    }

    std::string fullPath = ResolvePath(id, ResourceType::TEXTURE);
    if (fullPath.empty()) {
        return false;
    }

    Texture2D texture = LoadTexture(fullPath.c_str());
    if (texture.id == 0) {
        return false;
    }

    // Apply quality restrictions
    if (texture.width > maxTextureSize || texture.height > maxTextureSize) {
        // Scale down large textures on mobile
        Image img = LoadImageFromTexture(texture);
        UnloadTexture(texture);
        
        float scale = (float)maxTextureSize / std::max(img.width, img.height);
        ImageResize(&img, (int)(img.width * scale), (int)(img.height * scale));
        
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // Estimate memory usage (rough calculation)
    size_t memUsage = texture.width * texture.height * 4; // Assuming RGBA
    
    // Check if we need to trim cache
    if (totalMemoryUsage + memUsage > maxCacheMemoryMB * 1024 * 1024) {
        TrimCache(maxCacheMemoryMB);
    }

    // Cache the texture
    CachedResource<Texture2D> cached;
    cached.resource = texture;
    cached.isValid = true;
    cached.path = fullPath;
    cached.memoryUsage = memUsage;
    cached.lastAccessed = GetTime();

    textureCache[id] = cached;
    totalMemoryUsage += memUsage;

    TraceLog(LOG_INFO, "Loaded texture: %s (%dx%d, %.1fKB)", 
             id.c_str(), texture.width, texture.height, memUsage / 1024.0f);
    return true;
}

bool ResourceManager::LoadSoundInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::SOUND) {
        return false;
    }

    std::string fullPath = ResolvePath(id, ResourceType::SOUND);
    if (fullPath.empty()) {
        return false;
    }

    Sound sound = LoadSound(fullPath.c_str());
    if (sound.stream.buffer == nullptr) {
        return false;
    }

    CachedResource<Sound> cached;
    cached.resource = sound;
    cached.isValid = true;
    cached.path = fullPath;
    cached.memoryUsage = 44100 * 2 * 2; // Rough estimate for stereo 16-bit at 44.1kHz
    cached.lastAccessed = GetTime();

    soundCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    return true;
}

bool ResourceManager::LoadMusicInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::MUSIC) {
        return false;
    }

    std::string fullPath = ResolvePath(id, ResourceType::MUSIC);
    if (fullPath.empty()) {
        return false;
    }

    Music music = LoadMusicStream(fullPath.c_str());
    if (music.stream.buffer == nullptr) {
        return false;
    }

    CachedResource<Music> cached;
    cached.resource = music;
    cached.isValid = true;
    cached.path = fullPath;
    cached.memoryUsage = streamingEnabled ? 1024 : 1024 * 1024; // Much smaller if streaming
    cached.lastAccessed = GetTime();

    musicCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    return true;
}

bool ResourceManager::LoadFontInternal(const std::string& id) {
    auto regIt = resourceRegistry.find(id);
    if (regIt == resourceRegistry.end() || regIt->second.type != ResourceType::FONT) {
        return false;
    }

    std::string fullPath = ResolvePath(id, ResourceType::FONT);
    if (fullPath.empty()) {
        return false;
    }

    Font font = LoadFont(fullPath.c_str());
    if (font.texture.id == 0) {
        return false;
    }

    CachedResource<Font> cached;
    cached.resource = font;
    cached.isValid = true;
    cached.path = fullPath;
    cached.memoryUsage = font.texture.width * font.texture.height * 4;
    cached.lastAccessed = GetTime();

    fontCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    return true;
}

std::string ResourceManager::ResolvePath(const std::string& id, ResourceType type) {
    auto it = resourceRegistry.find(id);
    if (it == resourceRegistry.end()) {
        return "";
    }

    std::string basePath = platform->GetResourcePath(it->second.relativePath);
    
    // Try quality variants for textures
    if (type == ResourceType::TEXTURE && currentQuality != ResourceQuality::HIGH) {
        std::string qualityPath = GetQualityVariant(basePath, currentQuality);
        if (!qualityPath.empty() && std::filesystem::exists(qualityPath)) {
            return qualityPath;
        }
    }

    return basePath;
}

std::string ResourceManager::GetQualityVariant(const std::string& path, ResourceQuality quality) {
    std::filesystem::path p(path);
    std::string stem = p.stem().string();
    std::string extension = p.extension().string();
    std::string directory = p.parent_path().string();

    std::string suffix;
    switch (quality) {
        case ResourceQuality::LOW:    suffix = "_low"; break;
        case ResourceQuality::MEDIUM: suffix = "_med"; break;
        default: return path; // Use original for HIGH quality
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
        return;
    }
    
    auto musIt = musicCache.find(id);
    if (musIt != musicCache.end()) {
        musIt->second.lastAccessed = currentTime;
        return;
    }
    
    auto fontIt = fontCache.find(id);
    if (fontIt != fontCache.end()) {
        fontIt->second.lastAccessed = currentTime;
    }
}

void ResourceManager::TrimCache(size_t maxMemoryMB) {
    size_t targetMemory = maxMemoryMB * 1024 * 1024;
    
    while (totalMemoryUsage > targetMemory && !textureCache.empty()) {
        std::string lruId;
        float oldestTime = GetTime();
        
        for (const auto& [id, cached] : textureCache) {
            if (cached.lastAccessed < oldestTime) {
                oldestTime = cached.lastAccessed;
                lruId = id;
            }
        }
        
        if (!lruId.empty()) {
            auto it = textureCache.find(lruId);
            if (it != textureCache.end()) {
                UnloadTexture(it->second.resource);
                totalMemoryUsage -= it->second.memoryUsage;
                textureCache.erase(it);
                TraceLog(LOG_INFO, "Evicted texture from cache: %s", lruId.c_str());
            }
        } else {
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
    }
    
    for (auto& [id, cached] : fontCache) {
        if (cached.isValid) {
            UnloadFont(cached.resource);
        }
    }

    textureCache.clear();
    soundCache.clear();
    musicCache.clear();
    fontCache.clear();
    totalMemoryUsage = 0;

    TraceLog(LOG_INFO, "ResourceManager cache cleared");
}

void ResourceManager::RegisterResource(const std::string& id, const std::string& relativePath, 
                                     ResourceType type, LoadingMode mode, ResourceQuality minQuality) {
    ResourceInfo info;
    info.id = id;
    info.relativePath = relativePath;
    info.type = type;
    info.loadingMode = mode;
    info.minQuality = minQuality;
    
    resourceRegistry[id] = info;
}

void ResourceManager::RegisterAllResources() {
    // UI and Core Resources (always load these first)
    RegisterResource("scoreboard", "ui/Score.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turd_heart", "ui/TurdHeartSmall.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("coin_bag", "ui/CoinBag.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("pause_menu_bg", "ui/PauseMenuBackground.png", ResourceType::TEXTURE);
    
    // CRITICAL MISSING UI ELEMENTS - Adding these to prevent crashes
    RegisterResource("arrow_left", "ui/LeftArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_right", "ui/RightArrow.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_left_hover", "ui/LeftArrowHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("arrow_right_hover", "ui/RightArrowHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("blue_button", "ui/BlueButton.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("blue_button_hover", "ui/BlueButtonHover.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    
    // CRITICAL MISSING SOUNDS - Adding these to prevent crashes
    RegisterResource("got_score", "ui/bubble.mp3", ResourceType::SOUND, LoadingMode::SYNC);
    
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
    
    // Button textures
    RegisterResource("blue_button", "ui/FloppyButtonBlue.png", ResourceType::TEXTURE);
    RegisterResource("blue_button_hover", "ui/FloppyButtonBlueHover.png", ResourceType::TEXTURE);
    RegisterResource("arrow_left", "ui/LeftArrow.png", ResourceType::TEXTURE);
    RegisterResource("arrow_left_hover", "ui/LeftArrowHover.png", ResourceType::TEXTURE);
    RegisterResource("arrow_right", "ui/RightArrow.png", ResourceType::TEXTURE);
    RegisterResource("arrow_right_hover", "ui/RightArrowHover.png", ResourceType::TEXTURE);
    RegisterResource("arrow_up", "ui/UpArrow.png", ResourceType::TEXTURE);
    RegisterResource("arrow_down", "ui/DownArrow.png", ResourceType::TEXTURE);
    
    // Upgrade Menu UI
    RegisterResource("turd_point_menu", "ui/TurdPointMenu.png", ResourceType::TEXTURE);
    RegisterResource("turd_point_menu_border", "ui/SkillMenuBorder.png", ResourceType::TEXTURE);
    RegisterResource("turd_point_info", "ui/SkillPointInfoBackground.png", ResourceType::TEXTURE);
    RegisterResource("tp_menu_button_locked", "ui/TurdPointButton.png", ResourceType::TEXTURE);
    RegisterResource("tp_menu_button_available", "ui/TurdPointButtonAvailable.png", ResourceType::TEXTURE);
    RegisterResource("tp_menu_button_focused", "ui/TurdPointButtonFocused.png", ResourceType::TEXTURE);
    RegisterResource("tp_menu_button_selected", "ui/TurdPointButtonClaimed.png", ResourceType::TEXTURE);
    
    // Options Menu UI
    RegisterResource("mute_button", "ui/mutebutton.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_hover", "ui/mutebuttonhover.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_clicked", "ui/mutebuttonclicked.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_locked", "ui/mutebuttonlocked.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_locked_hover", "ui/mutebuttonlockedhover.png", ResourceType::TEXTURE);
    RegisterResource("mute_button_locked_clicked", "ui/mutebuttonlockedclicked.png", ResourceType::TEXTURE);
    RegisterResource("minus_button", "ui/minusbutton.png", ResourceType::TEXTURE);
    RegisterResource("minus_button_hover", "ui/minusbuttonhover.png", ResourceType::TEXTURE);
    RegisterResource("minus_button_clicked", "ui/minusbuttonclicked.png", ResourceType::TEXTURE);
    RegisterResource("plus_button", "ui/plusbutton.png", ResourceType::TEXTURE);
    RegisterResource("plus_button_hover", "ui/plusbuttonhover.png", ResourceType::TEXTURE);
    RegisterResource("plus_button_clicked", "ui/plusbuttonclicked.png", ResourceType::TEXTURE);
    RegisterResource("volume_meter_full", "ui/volumemeterfull.png", ResourceType::TEXTURE);
    RegisterResource("volume_meter_empty", "ui/volumemeterempty.png", ResourceType::TEXTURE);
    
    // Main Menu Buttons
    RegisterResource("play_button", "mainmenu/PlayButton.png", ResourceType::TEXTURE);
    RegisterResource("play_button_focused", "mainmenu/PlayButtonFocused.png", ResourceType::TEXTURE);
    RegisterResource("play_button_selected", "mainmenu/PlayButtonSelect.png", ResourceType::TEXTURE);
    RegisterResource("quickplay_button", "mainmenu/QuickPlayButton.png", ResourceType::TEXTURE);
    RegisterResource("quickplay_button_focused", "mainmenu/QuickPlayButtonFocused.png", ResourceType::TEXTURE);
    RegisterResource("quickplay_button_selected", "mainmenu/QuickPlayButtonSelect.png", ResourceType::TEXTURE);
    RegisterResource("options_button", "mainmenu/OptionButton.png", ResourceType::TEXTURE);
    RegisterResource("options_button_focused", "mainmenu/OptionButtonFocused.png", ResourceType::TEXTURE);
    RegisterResource("options_button_selected", "mainmenu/OptionButtonSelect.png", ResourceType::TEXTURE);
    
    // Sounds (essential)
    RegisterResource("click", "sounds/confirm.ogg", ResourceType::SOUND);
    RegisterResource("hurt", "sounds/hurt.mp3", ResourceType::SOUND);
    RegisterResource("got_coin", "sounds/pickup.ogg", ResourceType::SOUND);
    RegisterResource("got_health", "sounds/SmallHealthPickup.wav", ResourceType::SOUND);
    RegisterResource("got_health_big", "sounds/BigHealthPickup.wav", ResourceType::SOUND);
    
    // Enemy kill sounds
    RegisterResource("ratcopter_kill", "sounds/ratkill.ogg", ResourceType::SOUND);
    RegisterResource("toilet_paper_kill", "sounds/tpkill.ogg", ResourceType::SOUND);
    RegisterResource("ratking_kill", "sounds/RatKingKill.ogg", ResourceType::SOUND);
    
    // Boss sounds
    RegisterResource("boss_beat", "music/BossBeatv2.mp3", ResourceType::SOUND);
    RegisterResource("boss_kill", "music/BossKill.ogg", ResourceType::SOUND);
    
    // Fart sounds (critical for game)
    RegisterResource("fart1", "sounds/fart1.ogg", ResourceType::SOUND);
    RegisterResource("fart2", "sounds/fart2.ogg", ResourceType::SOUND);
    RegisterResource("fart3", "sounds/fart3.ogg", ResourceType::SOUND);
    RegisterResource("fart4", "sounds/fart4.ogg", ResourceType::SOUND);
    RegisterResource("fart5", "sounds/fart5.ogg", ResourceType::SOUND);
    RegisterResource("fart6", "sounds/fart6.ogg", ResourceType::SOUND);
    RegisterResource("fart7", "sounds/fart7.ogg", ResourceType::SOUND);
    RegisterResource("fart8", "sounds/fart8.ogg", ResourceType::SOUND);
    RegisterResource("fart9", "sounds/fart9.ogg", ResourceType::SOUND);
    RegisterResource("fart10", "sounds/fart10.ogg", ResourceType::SOUND);
    RegisterResource("fart11", "sounds/fart11.ogg", ResourceType::SOUND);
    
    // Music - CRITICAL for Playing class
    RegisterResource("game_over_music", "music/GameOver.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    
    // Player sprites - CRITICAL for gameplay (synchronous loading)
    RegisterResource("turdlet_idle", "turd/TurdletIdle.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turdlet_jump", "turd/TurdletJump.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turdlet_shoot", "turd/TurdletShoot.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("turdlet_hurt", "turd/TurdletHurt.png", ResourceType::TEXTURE, LoadingMode::SYNC);
    RegisterResource("big_turd_idle", "turd/BigTurdIdle.png", ResourceType::TEXTURE);
    RegisterResource("big_turd_jump", "turd/BigTurdJump.png", ResourceType::TEXTURE);
    RegisterResource("big_turd_shoot", "turd/BigTurdShoot.png", ResourceType::TEXTURE);
    RegisterResource("big_turd_hurt", "turd/BigTurdHurt.png", ResourceType::TEXTURE);
    RegisterResource("teenage_turd_idle", "turd/TeenageTurdIdle.png", ResourceType::TEXTURE);
    RegisterResource("teenage_turd_jump", "turd/TeenageTurdJump.png", ResourceType::TEXTURE);
    RegisterResource("teenage_turd_shoot", "turd/TeenageTurdShoot.png", ResourceType::TEXTURE);
    RegisterResource("teenage_turd_hurt", "turd/TeenageTurdHurt.png", ResourceType::TEXTURE);
    
    // Projectile sprites  
    RegisterResource("poop_small", "turd/Floppy Poop.png", ResourceType::TEXTURE);
    RegisterResource("poop_mid", "turd/Floppy Poop Mid.png", ResourceType::TEXTURE);
    RegisterResource("poop_large", "turd/Floppy Poop Large.png", ResourceType::TEXTURE);
    
    // Note: Hat registrations moved down to be with hat sprites to avoid duplicates
    
    // Main Menu
    RegisterResource("main_menu_bg", "mainmenu/MainMenu.png", ResourceType::TEXTURE);
    RegisterResource("floppy_logo", "mainmenu/FloppyLogo.png", ResourceType::TEXTURE);
    RegisterResource("fin_logo", "mainmenu/F.png", ResourceType::TEXTURE);
    RegisterResource("main_menu_music", "mainmenu/FloppyTurdMenu.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("main_menu_music_alt", "mainmenu/FloppyTurdMenu Fart Variant.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    
    // Level paintings
    RegisterResource("empty_painting", "mainmenu/EmptyPainting.png", ResourceType::TEXTURE);
    RegisterResource("locked_painting", "mainmenu/LockedPainting.png", ResourceType::TEXTURE);
    RegisterResource("park_painting", "mainmenu/ParkLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("sewer_painting", "mainmenu/SewerLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("desert_painting", "mainmenu/DesertLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("snow_painting", "mainmenu/SnowLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("castle_painting", "mainmenu/CastleLevelPainting.png", ResourceType::TEXTURE);
    RegisterResource("ratking_painting", "mainmenu/RatKingPainting.png", ResourceType::TEXTURE);

    // Level 1 (Park) - High priority
    RegisterResource("park_back", "environment/Level1BackLayerBackground.png", ResourceType::TEXTURE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("park_mid", "environment/Level1MidLayerBackground.png", ResourceType::TEXTURE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("park_front", "environment/Level1FrontLayerBackground.png", ResourceType::TEXTURE, LoadingMode::LAZY, ResourceQuality::MEDIUM);
    RegisterResource("park_clouds", "environment/level1Clouds.png", ResourceType::TEXTURE);
    RegisterResource("top_toilet", "environment/TopToilet.png", ResourceType::TEXTURE);
    RegisterResource("bottom_toilet", "environment/BottomToilet.png", ResourceType::TEXTURE);
    RegisterResource("level1_music", "music/Level1.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level1_fast", "music/Level1Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level1_slow", "music/Level1Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Level 2 (Sewer)
    RegisterResource("sewer_wall_a", "environment/sewerwidevarA.png", ResourceType::TEXTURE);
    RegisterResource("sewer_wall_b", "environment/sewerwidevarB.png", ResourceType::TEXTURE);
    RegisterResource("sewer_wall_c", "environment/sewerwidevarC.png", ResourceType::TEXTURE);
    RegisterResource("sewer_wall_d", "environment/sewerwidevarD.png", ResourceType::TEXTURE);
    RegisterResource("bottom_pipe_blue", "environment/BottomPipeWideBlue.png", ResourceType::TEXTURE);
    RegisterResource("bottom_pipe_orange", "environment/BottomPipeWide.png", ResourceType::TEXTURE);
    RegisterResource("top_pipe_blue", "environment/TopPipeWideBlue.png", ResourceType::TEXTURE);
    RegisterResource("top_pipe_orange", "environment/TopPipeWide.png", ResourceType::TEXTURE);
    RegisterResource("level2_music", "music/Level2.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level2_fast", "music/Level2Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level2_slow", "music/Level2Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Level 3 (Desert)  
    RegisterResource("desert_back", "environment/Level3BackLayerBackground.png", ResourceType::TEXTURE);
    RegisterResource("desert_mid", "environment/Level3MidLayerBackground.png", ResourceType::TEXTURE);
    RegisterResource("desert_front", "environment/Level3FrontLayerBackground.png", ResourceType::TEXTURE);
    RegisterResource("desert_cacti_layer", "environment/Cacti.png", ResourceType::TEXTURE);
    RegisterResource("dancing_cacti_small", "environment/dancingcactismall.png", ResourceType::TEXTURE);
    RegisterResource("dancing_cacti", "environment/dancingcacti.png", ResourceType::TEXTURE);
    RegisterResource("dancing_cacti_cowboy", "environment/dancingcacticowboy.png", ResourceType::TEXTURE);
    RegisterResource("bird_idle", "enemies/BirdIdle.png", ResourceType::TEXTURE);
    RegisterResource("bird_hurt", "enemies/BirdHurt.png", ResourceType::TEXTURE);
    RegisterResource("level3_music", "music/Level3.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level3_fast", "music/Level3Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level3_slow", "music/Level3Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Level 4 (Snow)
    RegisterResource("snow_background", "environment/SnowLevelBackground.png", ResourceType::TEXTURE);
    RegisterResource("snow_mountains", "environment/SnowLevelMountains.png", ResourceType::TEXTURE);
    RegisterResource("snow_back_trees", "environment/SnowLevelBackTrees.png", ResourceType::TEXTURE);
    RegisterResource("snow_tundra", "environment/SnowLevelTundra.png", ResourceType::TEXTURE);
    RegisterResource("snow_front_trees", "environment/SnowLevelFrontTrees.png", ResourceType::TEXTURE);
    RegisterResource("snowfall", "environment/snow_tile.png", ResourceType::TEXTURE);
    RegisterResource("snowman_idle", "enemies/SnowManIdle.png", ResourceType::TEXTURE);
    RegisterResource("level4_music", "music/SnowLevel.mp3", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level4_fast", "music/Level4Fast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level4_slow", "music/Level4Slow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level4_snow_fast", "music/Level4SnowFast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("level4_snow_slow", "music/Level4SnowSlow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Level 5 (Castle)
    RegisterResource("castle_wall", "environment/castlelevelbackgroundwall.png", ResourceType::TEXTURE);
    RegisterResource("castle_bars", "environment/castlelevelfloorceiling.png", ResourceType::TEXTURE);
    RegisterResource("curtains", "environment/curtains.png", ResourceType::TEXTURE);
    RegisterResource("torch_pillar", "environment/TorchPillar.png", ResourceType::TEXTURE);
    RegisterResource("chandelier", "environment/castlelevelchandelier.png", ResourceType::TEXTURE);
    RegisterResource("floor_torch", "environment/castlelevelfloortorch.png", ResourceType::TEXTURE);
    RegisterResource("top_toilet_gold", "environment/TopToiletGold.png", ResourceType::TEXTURE);
    RegisterResource("bottom_toilet_gold", "environment/BottomToiletGold.png", ResourceType::TEXTURE);
    RegisterResource("level5_music", "music/Level5.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Boss Level (Level 6)
    RegisterResource("boss_background", "environment/ratkingbackground.png", ResourceType::TEXTURE);
    RegisterResource("boss_dark_clouds", "environment/darkclouds.png", ResourceType::TEXTURE);
    RegisterResource("boss_floor", "environment/BossFloor.png", ResourceType::TEXTURE);
    RegisterResource("boss_walls", "environment/BossWalls.png", ResourceType::TEXTURE);
    RegisterResource("boss_curtains", "environment/screenCurtains.png", ResourceType::TEXTURE);
    RegisterResource("boss_pillar", "environment/bosspillar.png", ResourceType::TEXTURE);
    RegisterResource("ratking_idle", "enemies/Ratking.png", ResourceType::TEXTURE);
    RegisterResource("ratking_walk", "enemies/RatkingWalk.png", ResourceType::TEXTURE);
    RegisterResource("ratking_hurt", "enemies/RatkingHurt.png", ResourceType::TEXTURE);
    RegisterResource("ratking_death", "enemies/RatkingDeath.png", ResourceType::TEXTURE);
    RegisterResource("ratking_aim_torso", "enemies/RatkingAimTorsoOnly.png", ResourceType::TEXTURE);
    RegisterResource("ratking_aim_front_arm", "enemies/RatkingAimTossArmOnly.png", ResourceType::TEXTURE);
    RegisterResource("ratking_aim_back_arm", "enemies/RatkingAimBackArmOnly.png", ResourceType::TEXTURE);
    RegisterResource("boss_music", "music/BossThemeFast.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("boss_music_slow", "music/BossThemeSlow.ogg", ResourceType::MUSIC, LoadingMode::STREAM);
    RegisterResource("boss_low_health", "music/BossThemeLowHealth.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Missing Level Assets
    RegisterResource("top_toilet_snow", "environment/TopToiletSnow.png", ResourceType::TEXTURE);
    RegisterResource("bottom_toilet_snow", "environment/BottomToiletSnow.png", ResourceType::TEXTURE);
    RegisterResource("outhouse_solo", "environment/Outhouse.png", ResourceType::TEXTURE);
    RegisterResource("outhouse_toilet", "environment/OuthouseToilet.png", ResourceType::TEXTURE);
    RegisterResource("brick_wall", "objects/BrickWall.png", ResourceType::TEXTURE);
    RegisterResource("spike_ball", "objects/SpikeBall.png", ResourceType::TEXTURE);
    RegisterResource("spike_ball_base", "objects/SpikeBallBase.png", ResourceType::TEXTURE);

    // Snowman Enemy Variants
    RegisterResource("snowman_red", "enemies/SnowManIdle.png", ResourceType::TEXTURE);
    RegisterResource("snowman_red_throw", "enemies/SnowManThrow.png", ResourceType::TEXTURE);
    RegisterResource("snowman_blue", "enemies/SnowManChill.png", ResourceType::TEXTURE);
    RegisterResource("snowman_green", "enemies/SnowManGreen.png", ResourceType::TEXTURE);
    RegisterResource("snowman_chad", "enemies/SnowManChad.png", ResourceType::TEXTURE);
    RegisterResource("snowball", "enemies/Snowball.png", ResourceType::TEXTURE);

    // VFX textures - CRITICAL for game effects
    RegisterResource("blast_small", "vfx/blast_small.png", ResourceType::TEXTURE);
    RegisterResource("blast_big", "vfx/blast_big.png", ResourceType::TEXTURE);

    // Pickups - CRITICAL for gameplay
    RegisterResource("gold_coin", "objects/GoldCoin.png", ResourceType::TEXTURE);
    RegisterResource("blue_coin", "objects/BlueCoin.png", ResourceType::TEXTURE);
    RegisterResource("red_coin", "objects/RedCoin.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart", "objects/PooHeart.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart_big", "objects/PooHeartBig.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart_invisible", "objects/PooHeartRainbowBeam.png", ResourceType::TEXTURE);

    // Enemies - Important for gameplay
    RegisterResource("ratcopter_idle", "enemies/RatCopterIdle.png", ResourceType::TEXTURE);
    RegisterResource("ratcopter_hurt", "enemies/RatCopterHurt.png", ResourceType::TEXTURE);
    RegisterResource("toilet_paper_idle", "enemies/ToiletPaperFlap.png", ResourceType::TEXTURE);
    RegisterResource("toilet_paper_hurt", "enemies/ToiletPaperHit.png", ResourceType::TEXTURE);
    RegisterResource("toilet_paper_projectile", "enemies/ToiletPaperProjectile.png", ResourceType::TEXTURE);

    // Cactus variants - CRITICAL for desert level
    RegisterResource("cacti_a", "objects/CactiA.png", ResourceType::TEXTURE);
    RegisterResource("cacti_b", "objects/CactiB.png", ResourceType::TEXTURE);
    RegisterResource("cacti_c", "objects/CactiC.png", ResourceType::TEXTURE);
    RegisterResource("cacti_d", "objects/CactiD.png", ResourceType::TEXTURE);
    RegisterResource("cacti_e", "objects/CactiE.png", ResourceType::TEXTURE);
    RegisterResource("cacti_bush", "objects/CactiBush.png", ResourceType::TEXTURE);

    // Castle paintings - for decorations
    RegisterResource("painting_a", "objects/CabinPainting.png", ResourceType::TEXTURE);
    RegisterResource("painting_b", "objects/RabbitKnightPainting.png", ResourceType::TEXTURE);
    RegisterResource("painting_c", "objects/RatBeachPainting.png", ResourceType::TEXTURE);
    RegisterResource("painting_d", "objects/RiverWalkPainting.png", ResourceType::TEXTURE);

    // NPCs
    RegisterResource("janitor", "objects/Janitor.png", ResourceType::TEXTURE);
    RegisterResource("janitor_sweep", "objects/JanitorSweep.png", ResourceType::TEXTURE);
    RegisterResource("janitor_surprise", "objects/JanitorSurprise.png", ResourceType::TEXTURE);

    // Boss bar elements
    RegisterResource("boss_bar_frame", "ui/BossBarFrame.png", ResourceType::TEXTURE);
    RegisterResource("boss_bar_health", "ui/BossBarHealth.png", ResourceType::TEXTURE);
    RegisterResource("boss_bar_hurt", "ui/BossBarHurt.png", ResourceType::TEXTURE);

    // Additional UI elements
    RegisterResource("achievement_locked", "ui/achievementicon.png", ResourceType::TEXTURE);
    RegisterResource("achievement_unlocked", "ui/achievementiconunlocked.png", ResourceType::TEXTURE);

    // Credits
    RegisterResource("credits_background", "ui/FloppyTurdCreditsBackground.png", ResourceType::TEXTURE);
    RegisterResource("credits_music", "music/EndTheme.ogg", ResourceType::MUSIC, LoadingMode::STREAM);

    // Fonts
    RegisterResource("whacky_joe_font", "fonts/Whacky_Joe.fnt", ResourceType::FONT);

    // Pickup items and objects - CRITICAL for level gameplay
    RegisterResource("gold_coin", "objects/GoldCoin.png", ResourceType::TEXTURE);
    RegisterResource("blue_coin", "objects/BlueCoin.png", ResourceType::TEXTURE);
    RegisterResource("red_coin", "objects/RedCoin.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart", "objects/PooHeart.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart_big", "objects/PooHeartBig.png", ResourceType::TEXTURE);
    RegisterResource("poo_heart_invisible", "objects/PooHeartRainbowBeam.png", ResourceType::TEXTURE);
    
    // CRITICAL MISSING DESERT LEVEL RESOURCES
    RegisterResource("outhouse_solo", "environment/Outhouse.png", ResourceType::TEXTURE);
    RegisterResource("brick_wall", "objects/BrickWall.png", ResourceType::TEXTURE);
    
    // Janitor sprites (for SewerLevel)
    RegisterResource("janitor_idle", "objects/Janitor.png", ResourceType::TEXTURE);
    RegisterResource("janitor_sweep", "objects/JanitorSweep.png", ResourceType::TEXTURE);
    RegisterResource("janitor_surprise", "objects/JanitorSurprise.png", ResourceType::TEXTURE);

    // Hat icons (for selection UI)
    RegisterResource("cowboy_hat", "hats/cowboyhat.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat", "hats/flowerhat.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat", "hats/dooraghat.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat", "hats/ballcap.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat", "hats/PinwheelHat.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat", "hats/strawhat.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat", "hats/SamuraiHelmet.png", ResourceType::TEXTURE);
    RegisterResource("top_hat", "hats/tophat.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat", "hats/ushanka.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat", "hats/Beret.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat", "hats/Crown.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat", "hats/poophat.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat", "hats/RamsesHat.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat", "hats/SpartanHelmet.png", ResourceType::TEXTURE);
    RegisterResource("shell_hat", "hats/shellhat.png", ResourceType::TEXTURE);
    
    // Hat sprites - CRITICAL MISSING RESOURCES! These are what Hat constructor needs
    // Cowboy Hat sprites
    RegisterResource("cowboy_hat_turdlet_jump", "hats/cowboyhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("cowboy_hat_turdlet_shoot", "hats/cowboyhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("cowboy_hat_big_jump", "hats/cowboyhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("cowboy_hat_big_shoot", "hats/cowboyhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Flower Hat sprites
    RegisterResource("flower_hat_turdlet_jump", "hats/flowerhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat_turdlet_shoot", "hats/flowerhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat_big_jump", "hats/flowerhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("flower_hat_big_shoot", "hats/flowerhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Doorag Hat sprites  
    RegisterResource("doorag_hat_turdlet_jump", "hats/dooragturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat_turdlet_shoot", "hats/dooragturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat_big_jump", "hats/dooragbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("doorag_hat_big_shoot", "hats/dooragbigturdshoot.png", ResourceType::TEXTURE);
    
    // Ball Cap sprites
    RegisterResource("ballcap_hat_turdlet_jump", "hats/ballcapturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat_turdlet_shoot", "hats/ballcapturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat_big_jump", "hats/ballcapbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ballcap_hat_big_shoot", "hats/ballcapbigturdshoot.png", ResourceType::TEXTURE);
    
    // Pinwheel Hat sprites
    RegisterResource("pinwheel_hat_turdlet_jump", "hats/pinwheelturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat_turdlet_shoot", "hats/pinwheelturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat_big_jump", "hats/pinwheelbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("pinwheel_hat_big_shoot", "hats/pinwheelbigturdshoot.png", ResourceType::TEXTURE);
    
    // Straw Hat sprites
    RegisterResource("straw_hat_turdlet_jump", "hats/strawhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat_turdlet_shoot", "hats/strawhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat_big_jump", "hats/strawhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("straw_hat_big_shoot", "hats/strawhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Samurai Hat sprites
    RegisterResource("samurai_hat_turdlet_jump", "hats/samuraiturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat_turdlet_shoot", "hats/samuraiturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat_big_jump", "hats/samuraibigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("samurai_hat_big_shoot", "hats/samuraibigturdshoot.png", ResourceType::TEXTURE);
    
    // Top Hat sprites
    RegisterResource("top_hat_turdlet_jump", "hats/tophatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("top_hat_turdlet_shoot", "hats/tophatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("top_hat_big_jump", "hats/tophatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("top_hat_big_shoot", "hats/tophatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Ushanka sprites
    RegisterResource("ushanka_hat_turdlet_jump", "hats/ushankaturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat_turdlet_shoot", "hats/ushankaturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat_big_jump", "hats/ushankabigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ushanka_hat_big_shoot", "hats/ushankabigturdshoot.png", ResourceType::TEXTURE);
    
    // Beret sprites
    RegisterResource("beret_hat_turdlet_jump", "hats/berethatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat_turdlet_shoot", "hats/berethatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat_big_jump", "hats/berethatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("beret_hat_big_shoot", "hats/berethatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Crown sprites
    RegisterResource("crown_hat_turdlet_jump", "hats/crownhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat_turdlet_shoot", "hats/crownhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat_big_jump", "hats/crownhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("crown_hat_big_shoot", "hats/crownhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Poop Hat sprites
    RegisterResource("poop_hat_turdlet_jump", "hats/poophatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat_turdlet_shoot", "hats/poophatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat_big_jump", "hats/poophatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("poop_hat_big_shoot", "hats/poophatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Ramses Hat sprites
    RegisterResource("ramses_hat_turdlet_jump", "hats/ramsesturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat_turdlet_shoot", "hats/ramsesturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat_big_jump", "hats/ramsesbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("ramses_hat_big_shoot", "hats/ramsesbigturdshoot.png", ResourceType::TEXTURE);
    
    // Spartan Hat sprites
    RegisterResource("spartan_hat_turdlet_jump", "hats/spartanhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat_turdlet_shoot", "hats/spartanhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat_big_jump", "hats/spartanhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("spartan_hat_big_shoot", "hats/spartanhatbigturdshoot.png", ResourceType::TEXTURE);
    
    // Shell Hat sprites
    RegisterResource("shell_hat_turdlet_jump", "hats/shellhatturdletjump.png", ResourceType::TEXTURE);
    RegisterResource("shell_hat_turdlet_shoot", "hats/shellhatturdletshoot.png", ResourceType::TEXTURE);
    RegisterResource("shell_hat_big_jump", "hats/shellhatbigturdjump.png", ResourceType::TEXTURE);
    RegisterResource("shell_hat_big_shoot", "hats/shellhatbigturdshoot.png", ResourceType::TEXTURE);

    TraceLog(LOG_INFO, "Registered %d resources", (int)resourceRegistry.size());
}

size_t ResourceManager::GetMemoryUsage() const {
    return totalMemoryUsage;
}

bool ResourceManager::IsResourceLoaded(const std::string& id) const {
    return textureCache.find(id) != textureCache.end() ||
           soundCache.find(id) != soundCache.end() ||
           musicCache.find(id) != musicCache.end() ||
           fontCache.find(id) != fontCache.end();
}

void ResourceManager::SetResourceQuality(ResourceQuality quality) {
    if (quality != currentQuality) {
        currentQuality = quality;
        ClearCache(); // Force reload with new quality
        TraceLog(LOG_INFO, "Resource quality changed to: %d", (int)quality);
    }
} 