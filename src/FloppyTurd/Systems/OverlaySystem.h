#pragma once

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <vector>
#include <string>

namespace GameCore {

    /**
     * OverlaySystem - Manages visual overlays that render between game world and UI
     * 
     * Features:
     * - Tiled texture rendering (repeats across screen)
     * - Wrapping/scrolling overlays
     * - Level-specific effects (snowfall, rain, etc.)
     */
    class OverlaySystem {
    public:
        OverlaySystem(Gnosis::ECS* ecsSystem);
        ~OverlaySystem();

        void SetPlatformDelegates(const PlatformDelegates& delegates) { m_platformDelegates = delegates; }
        
        void Initialize();
        void Update(float deltaTime);
        void Cleanup();

        // Snowfall overlay control
        void EnableSnowfall(bool enabled) { m_snowfallEnabled = enabled; }
        void SetSnowfallAnimSpeed(float fps) { m_snowfallAnimFPS = fps; }
        
        // State getters for renderer
        bool IsSnowfallEnabled() const { return m_snowfallEnabled; }
        std::string GetSnowfallTextureName() const { return "Snowfall"; } // Texture name for RenderSystem to load
        int GetSnowfallCurrentFrame() const { return m_snowfallCurrentFrame; }
        float GetSnowfallScale() const { return SNOWFALL_SCALE; }
        int GetSnowfallFrameSize() const { return SNOWFALL_FRAME_SIZE; }

    private:
        Gnosis::ECS* m_ecsSystem;
        PlatformDelegates m_platformDelegates;

        // Snowfall overlay state
        bool m_snowfallEnabled = false;
        float m_snowfallAnimFPS = 12.0f;          // Animation speed (frames per second)
        float m_snowfallFrameTimer = 0.0f;        // Time accumulator for frame changes
        int m_snowfallCurrentFrame = 0;           // Current frame index (0-15)
        int m_snowfallTextureHandle = -1;
        
        // Snowfall rendering params
        static constexpr float SNOWFALL_SCALE = 8.0f;  // Scale multiplier (8x)
        static constexpr int SNOWFALL_FRAME_SIZE = 32;  // Each frame is 32x32
        static constexpr int SNOWFALL_FRAME_COUNT = 16; // 16 frames in sprite sheet
        
        void LoadSnowfallTexture();
    };

} // namespace GameCore
