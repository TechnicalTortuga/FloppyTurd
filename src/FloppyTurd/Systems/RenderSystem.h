#ifndef FLOPPY_TURD_RENDER_SYSTEM_H
#define FLOPPY_TURD_RENDER_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include <vector>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <utility>

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
        RenderSystem(Gnosis::ECS* ecsSystem, const GameCore::PlatformDelegates& platformDelegates);
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
        
        // Texture management (1:1 with SpriteSystem)
        void SetTextureBasePath(const std::string& basePath);
        // Unified texture preload/query APIs
        bool PreloadTexture(const std::string& textureId);
        void PreloadTextures(const std::vector<std::string>& textureIds);
        bool IsTextureLoaded(const std::string& textureId) const;
        uint32_t GetTextureHandle(const std::string& textureId) const;
        bool GetTextureSize(const std::string& textureId, int& outWidth, int& outHeight) const;
        
        // Platform-specific layout setup
        void SetupLayout();  // Calls appropriate platform layout function

    private:
        Gnosis::ECS* m_ecsSystem;
        const GameCore::PlatformDelegates& m_platformDelegates;
        
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
            Text* text;
            UIShape* shape;
            int layer;
            float depth;
            
            // Debug rendering fields
            bool isDebugBounds = false;
            bool isDebugCollider = false;
            Gnosis::GNColor debugColor = {255, 255, 255, 255};
            float debugAlpha = 0.3f;
            float debugWidth = 0.0f;
            float debugHeight = 0.0f;
            float debugOffsetX = 0.0f;
            float debugOffsetY = 0.0f;
            // Circle support
            bool debugIsCircle = false;
            float debugRadius = 0.0f;
            // Absolute positioning (ignore sprite center offset)
            bool debugAbsolutePos = false;
        };
        
        std::vector<RenderItem> m_renderQueue;
        // Async texture loading state (mirrors SpriteSystem minimal behavior)
        std::unordered_set<std::string> m_pendingTextures;
        std::unordered_map<std::string, uint32_t> m_textureCache;
        std::unordered_map<std::string, std::pair<int, int>> m_textureDimensions; // width,height
        std::string m_textureBasePath; // Base path for texture loading (mirrors SpriteSystem)
        
        // Helper methods
        void CollectRenderItems();
        void SortRenderQueue();
        void RenderWorldSpace();
        void RenderSingleItem(const RenderItem& item);

        // Texture load helpers
        // 1:1 with SpriteSystem flow
        uint32_t GetOrLoadTexture(const std::string& textureId, Gnosis::Entity entity);
        std::string GetFullTexturePath(const std::string& textureId) const;
        static void HandleTextureLoaded(GameCore::TextureData* textureData, const char* error, void* userData);
        void EnsureTextureReady(const std::string& textureId); // retained but not used for gating
        struct TextureLoadContext {
            std::string textureId;
            RenderSystem* system;
            Gnosis::Entity entity;
            TextureLoadContext(const std::string& id, RenderSystem* sys, Gnosis::Entity e)
                : textureId(id), system(sys), entity(e) {}
            // Overload preserved for legacy call sites (unused by new path)
            TextureLoadContext(const std::string& id, RenderSystem* sys)
                : textureId(id), system(sys), entity() {}
        };
        
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
