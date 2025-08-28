#pragma once

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include <unordered_map>

// Use shorter type names from GameComponents.h
using GameCore::ProjectileType;
using GameCore::GNVector2;
using GameCore::GNColor;

namespace GameCore {

    /**
     * ProjectileSpriteConfig - Centralized configuration for projectile sprites and animations
     *
     * Provides sprite specifications for different projectile types including:
     * - Asset names and paths
     * - Frame dimensions and counts
     * - Animation timing
     * - Visual properties
     */
    class ProjectileSpriteConfig {
    public:
        struct SpriteSpec {
            std::string assetName;
            int frameWidth;
            int frameHeight;
            int frameCount;
            float animationSpeed;
            GNColor color;
            int layer;

            SpriteSpec()
                : frameWidth(16)
                , frameHeight(16)
                , frameCount(1)
                , animationSpeed(0.1f)
                , color(GNColor(255, 255, 255, 255))
                , layer(5)
            {}

            SpriteSpec(const std::string& name, int w, int h, int count, float speed,
                      const GNColor& c = GNColor(255, 255, 255, 255), int lyr = 5)
                : assetName(name)
                , frameWidth(w)
                , frameHeight(h)
                , frameCount(count)
                , animationSpeed(speed)
                , color(c)
                , layer(lyr)
            {}
        };

        static ProjectileSpriteConfig& GetInstance();

        const SpriteSpec& GetSpriteSpec(ProjectileType type) const;
        void Initialize();

    private:
        ProjectileSpriteConfig();
        ~ProjectileSpriteConfig() = default;
        ProjectileSpriteConfig(const ProjectileSpriteConfig&) = delete;
        ProjectileSpriteConfig& operator=(const ProjectileSpriteConfig&) = delete;

        std::unordered_map<ProjectileType, SpriteSpec> m_spriteSpecs;

        void LoadDefaultConfigurations();
    };

} // namespace GameCore
