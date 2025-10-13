#ifndef FLOPPY_TURD_RENDER_SYSTEM_H
#define FLOPPY_TURD_RENDER_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Utility/FrameProfiler.h"
#include "../Components/GameComponents.h"
#include <vector>
#include <map>
#include <set>
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
        RenderSystem(Gnosis::ECS* ecsSystem, const PlatformDelegates& platformDelegates);
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

        // Level information
        void SetCurrentLevelId(int levelId) { m_currentLevelId = levelId; }
        int GetCurrentLevelId() const { return m_currentLevelId; }

        // Render cache invalidation
        void MarkRenderCacheDirty();
        void SetRenderDebugLogging(bool enabled) { m_renderDebugLogging = enabled; }
        bool IsRenderDebugLoggingEnabled() const { return m_renderDebugLogging; }
        
        // FPS Counter
        void ToggleFPS() { m_showFPS = !m_showFPS; }
        bool IsFPSShown() const { return m_showFPS; }
        float GetCurrentFPS() const { return m_currentFPS; }
        
        // Profiling controls
        void SetProfilingEnabled(bool enabled);
        bool IsProfilingEnabled() const { return m_profilingEnabled; }

        // Direct screen dimension accessors (most commonly used)
        float GetScreenWidth() const { return m_screenInfo.pixelWidth; }
        float GetScreenHeight() const { return m_screenInfo.pixelHeight; }
        float GetLogicalWidth() const { return m_screenInfo.logicalWidth; }
        float GetLogicalHeight() const { return m_screenInfo.logicalHeight; }
        
        // Texture management (1:1 with SpriteSystem)
        void SetTextureBasePath(const std::string& basePath);
        // Unified texture preload/query APIs
        bool PreloadTexture(const std::string& textureId);
        void PreloadTextures(const std::vector<std::string>& textureIds);
        bool IsTextureLoaded(const std::string& textureId) const;
        uint32_t GetTextureHandle(const std::string& textureId) const;
        bool GetTextureSize(const std::string& textureId, int& outWidth, int& outHeight) const;

        // 🎯 NEW: Synchronous texture metadata cache API
        bool GetCachedTextureInfo(const std::string& textureId, int& width, int& height);
        void UpdateCacheFromAsyncResult(const std::string& textureId, uint32_t handle, int width, int height);
        
         // Platform-specific layout setup
        void SetupLayout();  // Calls appropriate platform layout function

    private:
        Gnosis::ECS* m_ecsSystem;
        const PlatformDelegates& m_platformDelegates;
        
        Gnosis::Entity m_activeCamera;
        bool m_useRenderLayers;
        
        // Dynamic screen information
        ScreenInfo m_screenInfo;
        bool m_screenInfoValid;
        int m_currentLevelId;
        
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
        std::set<int> m_currentFrameLayers; // Reusable layer set for batching (avoids per-frame allocation)
        
        // Reusable batching containers (cleared/reused each frame to avoid allocation)
        std::map<int, std::unordered_map<uint32_t, std::vector<const RenderItem*>>> m_layeredBatches;
        std::map<int, std::vector<const RenderItem*>> m_layeredNonBatchable;
        
        // Async texture loading state (mirrors SpriteSystem minimal behavior)
        std::unordered_set<std::string> m_pendingTextures;
        std::unordered_map<std::string, uint32_t> m_textureCache;
        std::unordered_map<std::string, std::pair<int, int>> m_textureDimensions; // width,height
        std::string m_textureBasePath; // Base path for texture loading (mirrors SpriteSystem)

        // 🎯 NEW: Synchronous texture metadata cache
        struct CachedTextureInfo {
            uint32_t handle;
            int width;
            int height;
            bool isLoaded;
            std::string assetPath;
        };
        std::unordered_map<std::string, CachedTextureInfo> m_textureMetadataCache;
        
        // FPS tracking
        bool m_showFPS = true;  // Show by default for debugging
        float m_currentFPS = 60.0f;
        float m_frameTimeAccum = 0.0f;
        int m_frameCount = 0;
        
        // Profiling
        Gnosis::FrameProfiler m_frameProfiler;
        bool m_profilingEnabled = false;
        
        // Helper methods
        void CollectRenderItems();
        void SortRenderQueue();
        void RenderWorldSpace();
        void RenderSingleItem(const RenderItem& item);
        void RebuildRenderCaches();
        
        // Batch rendering helper
        void RenderSpriteBatch(uint32_t textureHandle, const std::vector<const RenderItem*>& items);

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

        // Cached entity sets (rebuilt only when dirty)
        std::vector<Gnosis::Entity> m_cachedSpriteEntities;
        std::vector<Gnosis::Entity> m_cachedTextEntities;
        std::vector<Gnosis::Entity> m_cachedUIEntities;
        std::vector<Gnosis::Entity> m_cachedDebugEntities;
        std::vector<Gnosis::Entity> m_cachedShapeEntities;

        bool m_renderCacheDirty = true;
        size_t m_cachedEntityCount = 0;
        bool m_renderDebugLogging = false;

        // Component version tracking for automatic cache invalidation
        size_t m_cachedTransformVersion = 0;
        size_t m_cachedSpriteVersion = 0;
        size_t m_cachedTextVersion = 0;
        size_t m_cachedUIElementVersion = 0;
        size_t m_cachedDebugDrawVersion = 0;
        size_t m_cachedUIShapeVersion = 0;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_RENDER_SYSTEM_H
