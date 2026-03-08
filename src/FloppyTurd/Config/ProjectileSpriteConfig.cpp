#include "ProjectileSpriteConfig.h"
#include "../../Engine/Core/GNLog.h"

namespace GameCore {

    ProjectileSpriteConfig& ProjectileSpriteConfig::GetInstance() {
        static ProjectileSpriteConfig instance;
        return instance;
    }

    ProjectileSpriteConfig::ProjectileSpriteConfig() {
        Initialize();
    }

    void ProjectileSpriteConfig::Initialize() {
        GN_LOG_INFO("Initializing projectile sprite configurations...");
        LoadDefaultConfigurations();
        GN_LOG_INFO("Projectile sprite configurations loaded");
    }

    const ProjectileSpriteConfig::SpriteSpec& ProjectileSpriteConfig::GetSpriteSpec(ProjectileType type) const {
        GN_LOG_INFO("Looking up sprite spec for projectile type: " + std::to_string(static_cast<int>(type)));
        auto it = m_spriteSpecs.find(type);
        if (it != m_spriteSpecs.end()) {
            GN_LOG_INFO("Found sprite spec for type " + std::to_string(static_cast<int>(type)) + ": " + it->second.assetName);
            return it->second;
        }

        // Return default spec if type not found
        static SpriteSpec defaultSpec("Floppy Poop.png", 16, 16, 5, 0.1f, GNColor(255, 255, 255, 255));
        GN_LOG_ERROR("Unknown projectile type " + std::to_string(static_cast<int>(type)) + " requested, returning default spec with texture: " + defaultSpec.assetName);
        return defaultSpec;
    }

    void ProjectileSpriteConfig::LoadDefaultConfigurations() {
        // Player projectile configurations - using POOP_BALL for player projectiles
        // Default to regular Floppy Poop.png (Turdlet form)
        m_spriteSpecs[ProjectileType::POOP_BALL] = SpriteSpec(
            "Floppy Poop.png",    // assetName (Turdlet form)
            16,                   // frameWidth
            16,                   // frameHeight
            5,                    // frameCount (5 frames as specified)
            0.1f,                 // animationSpeed (10 FPS)
            GNColor(255, 255, 255, 255), // color (white)
            5                     // layer (above background, below UI)
        );

        // Large poop ball for big turd form
        m_spriteSpecs[ProjectileType::LARGE_POOP_BALL] = SpriteSpec(
            "Floppy Poop Large.png", // assetName (Big turd form)
            16,                   // frameWidth (16x16 per frame)
            16,                   // frameHeight
            5,                    // frameCount (5 frames as specified)
            0.15f,                // animationSpeed (slightly slower)
            GNColor(255, 255, 255, 255), // color (white)
            5                     // layer (above background, below UI)
        );

        // Enemy projectile configurations
        m_spriteSpecs[ProjectileType::SNOWBALL] = SpriteSpec(
            "Snowball",           // Use existing snowball texture
            16,                   // 16x16 per frame as specified
            16,
            4,                    // 4-frame snowball animation
            0.1f,
            GNColor(255, 255, 255, 255),
            5
        );

        m_spriteSpecs[ProjectileType::TOILET_PAPER] = SpriteSpec(
            "toiletpaperprojectile.png",
            32,                   // 32x32 per frame as specified
            32,
            4,                    // 4-frame toilet paper animation
            0.08f,                // Fast animation for spinning effect
            GNColor(255, 255, 255, 255),
            5
        );

        GN_LOG_INFO("Loaded sprite configurations for " + std::to_string(m_spriteSpecs.size()) + " projectile types");
    }

} // namespace GameCore
