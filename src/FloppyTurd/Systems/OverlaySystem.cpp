#include "OverlaySystem.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include <cmath>

namespace GameCore {

    OverlaySystem::OverlaySystem(Gnosis::ECS* ecsSystem)
        : m_ecsSystem(ecsSystem)
    {
        GN_LOG_INFO("OverlaySystem initialized");
    }

    OverlaySystem::~OverlaySystem() {
        GN_LOG_INFO("OverlaySystem destroyed");
        Cleanup();
    }

    void OverlaySystem::Initialize() {
        GN_LOG_INFO("Initializing OverlaySystem...");
        LoadSnowfallTexture();
        GN_LOG_INFO("OverlaySystem initialization complete");
    }

    void OverlaySystem::Update(float deltaTime) {
        if (!m_snowfallEnabled) return;

        // Update animation frame timer
        m_snowfallFrameTimer += deltaTime;
        
        // Advance to next frame when timer exceeds frame duration
        float frameDuration = 1.0f / m_snowfallAnimFPS;
        while (m_snowfallFrameTimer >= frameDuration) {
            m_snowfallFrameTimer -= frameDuration;
            m_snowfallCurrentFrame = (m_snowfallCurrentFrame + 1) % SNOWFALL_FRAME_COUNT;
        }
    }

    void OverlaySystem::Cleanup() {
        m_snowfallEnabled = false;
        m_snowfallTextureHandle = -1;
    }

    void OverlaySystem::LoadSnowfallTexture() {
        // Texture loading is handled by SpriteSystem/RenderSystem
        // We just need to mark that we want to use the "Snowfall" texture
        // The actual handle will be resolved when needed by RenderSystem
        // For now, just set a placeholder - RenderSystem will handle the actual loading
        m_snowfallTextureHandle = 0; // Will be loaded by RenderSystem when first used
        GN_LOG_INFO("OverlaySystem: Snowfall texture will be loaded on-demand by RenderSystem");
    }

} // namespace GameCore
