//
//  RenderSystem.cpp
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

#include "RenderSystem.h"
#include "../Core/ECS.h"
#include "../Platform/PlatformInterfaces.h"
#include "../../FloppyTurd/Components/GameComponents.h"
#include <algorithm>
#include <iostream>

namespace Gnosis {

    RenderSystem::RenderSystem(std::shared_ptr<IRenderer> renderer, ECS* ecsSystem)
        : m_renderer(renderer)
        , m_ecs(ecsSystem)
        , m_activeCameraEntity(INVALID_ENTITY)
        , m_clearColor(64, 128, 255, 255) // Nice sky blue
        , m_viewport(0, 0, 800, 600)
        , m_debugMode(false)
        , m_stats{} {
        
        // Reserve space for render queue to avoid frequent allocations
        m_renderQueue.reserve(1000);
    }

    void RenderSystem::OnInitialize() {
        if (!m_renderer) {
            std::cerr << "RenderSystem: No renderer provided!" << std::endl;
            return;
        }
        
        // RenderSystem now works directly with ECS - no component signature needed
        // We'll query entities with Transform and Sprite components directly from ECS
        
        std::cout << "RenderSystem initialized" << std::endl;
    }

    void RenderSystem::Update(float deltaTime) {
        // Update render statistics
        m_stats.frameTime = deltaTime;
        
        // Clear previous frame stats
        m_stats.spritesRendered = 0;
        m_stats.drawCalls = 0;
        m_stats.texturesUsed = 0;
    }

    void RenderSystem::Render() {
        if (!m_renderer) {
            return;
        }
        
        // Begin frame
        m_renderer->BeginFrame();
        
        // TODO: Set viewport when IRenderer interface supports it
        // m_renderer->SetViewport(
        //     static_cast<int>(m_viewport.x), 
        //     static_cast<int>(m_viewport.y),
        //     static_cast<int>(m_viewport.width), 
        //     static_cast<int>(m_viewport.height)
        // );
        
        // Clear screen
        m_renderer->Clear(m_clearColor);
        
        // Collect all renderable sprites
        CollectRenderableSprites();
        
        // Sort sprites by depth for proper layering
        SortSpritesByDepth();
        
        // Render all sprites
        RenderSprites();
        
        // Draw debug information if enabled
        if (m_debugMode) {
            DrawDebugInfo();
        }
        
        // End frame and present
        m_renderer->EndFrame();
        m_renderer->Present();
    }

    void RenderSystem::OnShutdown() {
        m_renderQueue.clear();
        m_activeCameraEntity = INVALID_ENTITY;
        std::cout << "RenderSystem shutdown" << std::endl;
    }

    void RenderSystem::SetActiveCamera(Entity cameraEntity) {
        m_activeCameraEntity = cameraEntity;
        std::cout << "RenderSystem: Active camera set to entity " << cameraEntity << std::endl;
    }

    void RenderSystem::SetViewport(int x, int y, int width, int height) {
        m_viewport = GNRectangle(static_cast<float>(x), static_cast<float>(y), 
                                static_cast<float>(width), static_cast<float>(height));
        
        // TODO: Set viewport when IRenderer interface supports it
        // if (m_renderer) {
        //     m_renderer->SetViewport(x, y, width, height);
        // }
    }

    void RenderSystem::ResetStats() {
        m_stats = RenderStats();
    }

    void RenderSystem::CollectRenderableSprites() {
        m_renderQueue.clear();
        
        if (!m_ecs) {
            return;
        }
        
        // Get all entities that have both Transform and Sprite components
        auto entities = m_ecs->GetEntitiesWithComponents<FloppyTurd::Transform, FloppyTurd::Sprite>();
        
        for (Entity entity : entities) {
            // Get transform and sprite components
            FloppyTurd::Transform* transform = m_ecs->GetComponent<FloppyTurd::Transform>(entity);
            FloppyTurd::Sprite* sprite = m_ecs->GetComponent<FloppyTurd::Sprite>(entity);
            
            if (!transform || !sprite) {
                continue; // Safety check
            }
            
            // Skip if sprite is not visible
            if (!sprite->visible) {
                continue;
            }
            
            // Check if sprite is within camera bounds (basic frustum culling)
            if (!IsSpriteVisible(*transform, *sprite)) {
                continue;
            }
            
            // Add to render queue
            SpriteRenderData renderData;
            renderData.entity = entity;
            renderData.transform = transform;
            renderData.sprite = sprite;
            renderData.depth = CalculateSpriteDepth(*transform, *sprite);
            
            m_renderQueue.push_back(renderData);
        }
    }

    void RenderSystem::SortSpritesByDepth() {
        // Sort by depth (lower values render first, higher values render on top)
        std::sort(m_renderQueue.begin(), m_renderQueue.end(),
            [](const SpriteRenderData& a, const SpriteRenderData& b) {
                return a.depth < b.depth;
            });
    }

    void RenderSystem::RenderSprites() {
        std::string currentTexture = "";
        
        for (const auto& spriteData : m_renderQueue) {
            RenderSprite(spriteData);
            m_stats.spritesRendered++;
            
            // Track texture changes for batching statistics
            if (spriteData.sprite->textureId != currentTexture) {
                currentTexture = spriteData.sprite->textureId;
                m_stats.drawCalls++;
            }
        }
    }

    void RenderSystem::RenderSprite(const SpriteRenderData& spriteData) {
        const auto& transform = *spriteData.transform;
        const auto& sprite = *spriteData.sprite;
        
        // Calculate world position (apply camera transform if we have an active camera)
        GNVector2 screenPos = WorldToScreen(transform.position);
        
        // Create destination rectangle
        GNRectangle destRect(
            screenPos.x - (sprite.width * transform.scale.x * 0.5f),
            screenPos.y - (sprite.height * transform.scale.y * 0.5f),
            sprite.width * transform.scale.x,
            sprite.height * transform.scale.y
        );
        
        // For now, draw a colored rectangle as placeholder
        // TODO: Load and use actual textures when texture system is implemented
        if (sprite.textureId.empty()) {
            // Draw colored rectangle
            m_renderer->DrawRectangle(destRect, sprite.color);
        } else {
            // TODO: Draw textured sprite when texture loading is implemented
            // For now, draw a colored rectangle with different color to indicate it should be textured
            GNColor textureColor(sprite.color.r / 2, sprite.color.g / 2, sprite.color.b / 2, sprite.color.a);
            m_renderer->DrawRectangle(destRect, textureColor);
        }
    }

    GNVector2 RenderSystem::WorldToScreen(const GNVector2& worldPos) const {
        // For now, simple 1:1 mapping
        // TODO: Apply camera transformation when camera system is implemented
        return worldPos;
    }

    GNVector2 RenderSystem::ScreenToWorld(const GNVector2& screenPos) const {
        // For now, simple 1:1 mapping
        // TODO: Apply inverse camera transformation when camera system is implemented
        return screenPos;
    }

    float RenderSystem::CalculateSpriteDepth(const FloppyTurd::Transform& transform, const FloppyTurd::Sprite& sprite) const {
        // Calculate depth based on layer and Y position
        // Higher layers render on top, and within the same layer, higher Y values render on top
        float layerDepth = static_cast<float>(sprite.layer) * 1000.0f;
        float positionDepth = transform.position.y * 0.01f; // Small contribution from Y position
        
        return layerDepth + positionDepth;
    }

    bool RenderSystem::IsSpriteVisible(const FloppyTurd::Transform& transform, const FloppyTurd::Sprite& sprite) const {
        // Simple bounds check against viewport
        // TODO: Implement proper frustum culling with camera
        
        GNRectangle spriteBounds(
            transform.position.x - (sprite.width * transform.scale.x * 0.5f),
            transform.position.y - (sprite.height * transform.scale.y * 0.5f),
            sprite.width * transform.scale.x,
            sprite.height * transform.scale.y
        );
        
        return m_viewport.Intersects(spriteBounds);
    }

    void RenderSystem::DrawDebugInfo() {
        if (!m_renderer) {
            return;
        }
        
        DrawDebugGrid();
        DrawDebugCameraBounds();
        DrawDebugSpriteOutlines();
    }

    void RenderSystem::DrawDebugGrid() {
        // Draw a simple grid for debugging
        const float gridSize = 50.0f;
        const GNColor gridColor(100, 100, 100, 128);
        
        // Vertical lines
        for (float x = 0; x < m_viewport.width; x += gridSize) {
            GNVector2 start(x, 0);
            GNVector2 end(x, m_viewport.height);
            m_renderer->DrawLine(start, end, gridColor, 1.0f);
        }
        
        // Horizontal lines
        for (float y = 0; y < m_viewport.height; y += gridSize) {
            GNVector2 start(0, y);
            GNVector2 end(m_viewport.width, y);
            m_renderer->DrawLine(start, end, gridColor, 1.0f);
        }
    }

    void RenderSystem::DrawDebugCameraBounds() {
        // Draw camera bounds if we have an active camera
        if (HasActiveCamera()) {
            const GNColor cameraColor(255, 255, 0, 128); // Yellow
            m_renderer->DrawRectangleOutline(m_viewport, cameraColor, 2.0f);
        }
    }

    void RenderSystem::DrawDebugSpriteOutlines() {
        // Draw outlines around all rendered sprites
        const GNColor outlineColor(255, 0, 255, 255); // Magenta
        
        for (const auto& spriteData : m_renderQueue) {
            const auto& transform = *spriteData.transform;
            const auto& sprite = *spriteData.sprite;
            
            GNVector2 screenPos = WorldToScreen(transform.position);
            GNRectangle bounds(
                screenPos.x - (sprite.width * transform.scale.x * 0.5f),
                screenPos.y - (sprite.height * transform.scale.y * 0.5f),
                sprite.width * transform.scale.x,
                sprite.height * transform.scale.y
            );
            
            m_renderer->DrawRectangleOutline(bounds, outlineColor, 1.0f);
        }
    }

} // namespace Gnosis