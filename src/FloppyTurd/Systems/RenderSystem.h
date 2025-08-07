#ifndef FLOPPY_TURD_RENDER_SYSTEM_H
#define FLOPPY_TURD_RENDER_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include <vector>
#include <map>

namespace GameCore {

    /**
     * @brief Unified Render System
     * 
     * Handles all rendering through the camera system with proper layering:
     * - Background parallax layers (layers 0-2)
     * - Game objects (layer 3)
     * - Player (layer 4)
     * - Effects and UI (layers 5+)
     * 
     * Enhanced with dynamic screen information for responsive rendering.
     */
    class RenderSystem {
    public:
        RenderSystem(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates& platformDelegates);
        ~RenderSystem();

        // Main render method
        void Render();

        // Camera management
        void SetActiveCamera(Gnosis::Entity cameraEntity);
        Gnosis::Entity GetActiveCamera() const { return m_activeCamera; }

        // Layer management
        void SetRenderLayers(bool enabled) { m_useRenderLayers = enabled; }
        bool GetRenderLayers() const { return m_useRenderLayers; }

        // Screen space rendering (for UI)
        void RenderScreenSpace();
        
        // Dynamic screen information
        const ScreenInfo& GetScreenInfo() const { return m_screenInfo; }
        void UpdateScreenInfo(); // Call when screen changes (rotation, etc.)
        float GetDynamicScale() const;
        float GetUIScale() const;
        
        // Platform-specific layout setup
        void SetupLayout();  // Calls appropriate platform layout function

    private:
        Gnosis::ECS* m_ecsSystem;
        GameCore::PlatformDelegates& m_platformDelegates;
        
        Gnosis::Entity m_activeCamera;
        bool m_useRenderLayers;
        
        // Dynamic screen information
        ScreenInfo m_screenInfo;
        bool m_screenInfoValid;
        
        // Render data structures
        struct RenderItem {
            Gnosis::Entity entity;
            Transform* transform;
            Sprite* sprite;
            int layer;
            float depth;
        };
        
        std::vector<RenderItem> m_renderQueue;
        
        // Helper methods
        void CollectRenderItems();
        void SortRenderQueue();
        void RenderWorldSpace();
        void RenderSingleItem(const RenderItem& item);
        
        // Dynamic layout helpers
        void SetupIOSLayout();
        void SetupDesktopLayout();
        void CalculateDynamicScaling();
        
        // World-to-screen transformation
        Gnosis::GNVector2 WorldToScreen(const Gnosis::GNVector2& worldPos);
        float GetCameraScale() const;
        Gnosis::GNVector2 GetCameraPosition() const;
        
        // Constants
        static constexpr int MAX_RENDER_LAYERS = 10;
        static constexpr int BACKGROUND_LAYER_START = 0;
        static constexpr int BACKGROUND_LAYER_END = 2;
        static constexpr int GAME_OBJECT_LAYER = 3;
        static constexpr int PLAYER_LAYER = 4;
        static constexpr int EFFECT_LAYER_START = 5;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_RENDER_SYSTEM_H
