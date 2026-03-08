#pragma once

// Asset path constants for the enhanced category-based organization
namespace GameCore::AssetPaths {
    
    // Base asset directory
    static constexpr const char* ASSETS_ROOT = "Assets/";
    
    namespace Graphics {
        static constexpr const char* GRAPHICS_ROOT = "graphics/";
        
        namespace Characters {
            static constexpr const char* CHARACTERS_ROOT = "graphics/characters/";
            
            // Player assets
            static constexpr const char* PLAYER_TURD_BASE = "graphics/characters/player/turd/base/";
            static constexpr const char* PLAYER_TURD_PROJECTILES = "graphics/characters/player/turd/projectiles/";
            static constexpr const char* PLAYER_HATS_BASE = "graphics/characters/player/cosmetics/hats/base_hats/";
            static constexpr const char* PLAYER_HATS_BIGTURD = "graphics/characters/player/cosmetics/hats/character_combinations/bigturd/";
            static constexpr const char* PLAYER_HATS_TURDLET = "graphics/characters/player/cosmetics/hats/character_combinations/turdlet/";
            
            // Enemy assets
            static constexpr const char* ENEMIES_FLYING = "graphics/characters/enemies/flying/";
            static constexpr const char* ENEMIES_GROUND = "graphics/characters/enemies/ground/";
            static constexpr const char* ENEMIES_PROJECTILES = "graphics/characters/enemies/projectiles/";
        }
        
        namespace Environment {
            static constexpr const char* ENVIRONMENT_ROOT = "graphics/environment/";
            
            // Backgrounds
            static constexpr const char* BACKGROUNDS_LEVEL1 = "graphics/environment/backgrounds/level1/";
            static constexpr const char* BACKGROUNDS_LEVEL3 = "graphics/environment/backgrounds/level3/";
            static constexpr const char* BACKGROUNDS_SNOW = "graphics/environment/backgrounds/snow_level/";
            static constexpr const char* BACKGROUNDS_SEWER = "graphics/environment/backgrounds/sewer/";
            static constexpr const char* BACKGROUNDS_BOSS = "graphics/environment/backgrounds/boss/";
            
            // Structures
            static constexpr const char* STRUCTURES_PIPES = "graphics/environment/structures/pipes/";
            static constexpr const char* STRUCTURES_TOILETS = "graphics/environment/structures/toilets/";
            static constexpr const char* STRUCTURES_BUILDINGS = "graphics/environment/structures/buildings/";
            
            // Props
            static constexpr const char* PROPS_VEGETATION = "graphics/environment/props/vegetation/";
            static constexpr const char* PROPS_PARTICLES = "graphics/environment/props/particles/";
            
            // Objects
            static constexpr const char* OBJECTS_COINS = "graphics/environment/objects/interactive/coins/";
            static constexpr const char* OBJECTS_POWERUPS = "graphics/environment/objects/interactive/powerups/";
            static constexpr const char* OBJECTS_HAZARDS = "graphics/environment/objects/interactive/hazards/";
            static constexpr const char* OBJECTS_PAINTINGS = "graphics/environment/objects/decorative/paintings/";
            static constexpr const char* OBJECTS_CACTI = "graphics/environment/objects/decorative/cacti/";
            static constexpr const char* OBJECTS_WALLS = "graphics/environment/objects/decorative/walls/";
            static constexpr const char* OBJECTS_NPCS = "graphics/environment/objects/npcs/";
        }
        
        namespace UI {
            static constexpr const char* UI_ROOT = "graphics/ui/";
            static constexpr const char* UI_MENUS_MAIN = "graphics/ui/menus/main/";
            static constexpr const char* UI_MENUS_LEVEL_SELECT = "graphics/ui/menus/level_select/";
            static constexpr const char* UI_HUD = "graphics/ui/hud/";
            static constexpr const char* UI_ICONS = "graphics/ui/icons/";
        }
        
        namespace VFX {
            static constexpr const char* VFX_ROOT = "graphics/vfx/";
            static constexpr const char* VFX_EXPLOSIONS = "graphics/vfx/explosions/";
            static constexpr const char* VFX_PARTICLES = "graphics/vfx/particles/";
        }
    }
    
    namespace Audio {
        static constexpr const char* AUDIO_ROOT = "audio/";
        
        namespace Music {
            static constexpr const char* MUSIC_ROOT = "audio/music/";
            static constexpr const char* MUSIC_LEVELS = "audio/music/gameplay/levels/";
            static constexpr const char* MUSIC_BOSS = "audio/music/gameplay/boss/";
            static constexpr const char* MUSIC_MENUS = "audio/music/menus/";
            static constexpr const char* MUSIC_STINGERS = "audio/music/stingers/";
        }
        
        namespace SFX {
            static constexpr const char* SFX_ROOT = "audio/sfx/";
            static constexpr const char* SFX_PLAYER = "audio/sfx/gameplay/player/";
            static constexpr const char* SFX_ENEMIES = "audio/sfx/gameplay/enemies/";
            static constexpr const char* SFX_ENVIRONMENT = "audio/sfx/gameplay/environment/";
            static constexpr const char* SFX_ITEMS = "audio/sfx/gameplay/items/";
            static constexpr const char* SFX_UI = "audio/sfx/ui/";
        }
    }
    
    namespace Fonts {
        static constexpr const char* FONTS_ROOT = "fonts/";
        static constexpr const char* FONTS_BITMAP = "fonts/bitmap/";
        static constexpr const char* FONTS_TRUETYPE = "fonts/truetype/";
    }
    
    namespace Data {
        static constexpr const char* DATA_ROOT = "data/";
        static constexpr const char* DATA_LEVELS = "data/levels/";
        static constexpr const char* DATA_CONFIGS = "data/configs/";
        static constexpr const char* DATA_LOCALIZATION = "data/localization/";
    }
    
    namespace Meta {
        static constexpr const char* META_ROOT = "meta/";
        static constexpr const char* ASSET_REGISTRY = "meta/asset_registry.json";
        static constexpr const char* ASSET_DEPENDENCIES = "meta/asset_dependencies.json";
        static constexpr const char* BUILD_MANIFESTS = "meta/build_manifests/";
    }
    
    // Commonly used asset combinations
    namespace Common {
        // Player character asset sets
        struct PlayerAssetSet {
            const char* base_sprite_path;
            const char* hat_path;
            const char* projectile_path;
        };
        
        // Level asset sets
        struct LevelAssetSet {
            const char* background_path;
            const char* music_path;
            const char* objects_path;
        };
        
        // UI screen asset sets
        struct UIAssetSet {
            const char* graphics_path;
            const char* audio_path;
        };
    }
}

// Utility macros for building full asset paths
#define ASSET_PATH(category, filename) GameCore::AssetPaths::category filename
#define PLAYER_TURD_ASSET(filename) ASSET_PATH(Graphics::Characters::PLAYER_TURD_BASE, filename)
#define ENEMY_ASSET(type, filename) ASSET_PATH(Graphics::Characters::ENEMIES_##type, filename)
#define UI_ASSET(type, filename) ASSET_PATH(Graphics::UI::UI_##type, filename)
#define AUDIO_ASSET(type, filename) ASSET_PATH(Audio::type, filename)
