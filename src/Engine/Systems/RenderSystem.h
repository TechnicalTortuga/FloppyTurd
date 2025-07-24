//
//  RenderSystem.h
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

#pragma once

#include "../Core/GnosisTypes.h"
#include "../Platform/PlatformInterfaces.h"
#include "../../FloppyTurd/Components/GameComponents.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace Gnosis {

    // Forward declarations
    class ECS;

    /**
     * @class RenderSystem
     * @brief Core rendering system that manages sprite rendering and camera transforms
     * 
     * The RenderSystem is responsible for:
     * - Managing the active camera
     * - Sorting sprites by layer for proper depth ordering
     * - Transforming world coordinates to screen coordinates
     * - Batching draw calls for performance
     * - Interfacing with platform-specific renderers (Raylib, Metal)
     * 
     * Note: This system works directly with ECS and does not inherit from the old System base class
     */
    class RenderSystem {
    public:
        RenderSystem(std::shared_ptr<IRenderer> renderer, ECS* ecsSystem);
        virtual ~RenderSystem() = default;

        // System interface
        void OnInitialize();
        void Update(float deltaTime);
        void Render();
        void OnShutdown();

        // Camera management
        void SetActiveCamera(Entity cameraEntity);
        Entity GetActiveCamera() const { return m_activeCameraEntity; }
        bool HasActiveCamera() const { return m_activeCameraEntity != INVALID_ENTITY; }

        // Rendering configuration
        void SetClearColor(const GNColor& color) { m_clearColor = color; }
        const GNColor& GetClearColor() const { return m_clearColor; }
        
        void SetViewport(int x, int y, int width, int height);
        GNRectangle GetViewport() const { return m_viewport; }

        // Debug rendering
        void SetDebugMode(bool enabled) { m_debugMode = enabled; }
        bool IsDebugMode() const { return m_debugMode; }
        void DrawDebugInfo();

        // Statistics
        struct RenderStats {
            uint32_t spritesRendered = 0;
            uint32_t drawCalls = 0;
            uint32_t texturesUsed = 0;
            float frameTime = 0.0f;
        };
        const RenderStats& GetStats() const { return m_stats; }
        void ResetStats();

        // System interface
        std::string GetName() const { return "RenderSystem"; }

    private:
        // Core rendering data
        std::shared_ptr<IRenderer> m_renderer;
        ECS* m_ecs;
        Entity m_activeCameraEntity;
        GNColor m_clearColor;
        GNRectangle m_viewport;
        bool m_debugMode;
        RenderStats m_stats;

        // Sprite batching for performance
        struct SpriteRenderData {
            Entity entity;
            FloppyTurd::Transform* transform;
            FloppyTurd::Sprite* sprite;
            float depth; // Calculated from layer and position
        };
        std::vector<SpriteRenderData> m_renderQueue;

        // Helper methods
        void CollectRenderableSprites();
        void SortSpritesByDepth();
        void RenderSprites();
        void RenderSprite(const SpriteRenderData& spriteData);
        
        // TODO: Add GNMatrix4 to GnosisTypes.h later
        // GNMatrix4 GetCameraMatrix() const;
        GNVector2 WorldToScreen(const GNVector2& worldPos) const;
        GNVector2 ScreenToWorld(const GNVector2& screenPos) const;
        
        float CalculateSpriteDepth(const FloppyTurd::Transform& transform, const FloppyTurd::Sprite& sprite) const;
        bool IsSpriteVisible(const FloppyTurd::Transform& transform, const FloppyTurd::Sprite& sprite) const;
        
        // Debug helpers
        void DrawDebugGrid();
        void DrawDebugCameraBounds();
        void DrawDebugSpriteOutlines();
    };

} // namespace Gnosis