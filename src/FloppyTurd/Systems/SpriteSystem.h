#ifndef FLOPPY_TURD_SPRITE_SYSTEM_H
#define FLOPPY_TURD_SPRITE_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <vector>
#include <unordered_map>

namespace GameCore {

    /**
     * SpriteSystem - Updates sprite animations and renders sprites to platform
     * 
     * This system handles:
     * - Animation frame updates based on deltaTime
     * - Frame looping and animation state management
     * - Direct rendering to platform renderers (Metal/Raylib)
     * - Texture loading and management
     * 
     * All-in-one sprite system for simplicity and performance.
     */
    class SpriteSystem {
    public:
        explicit SpriteSystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates);
        ~SpriteSystem() = default;

        // Update and render all sprites
        void Update(float deltaTime);
        void Render();
        
        // Animation control
        void PlayAnimation(Gnosis::Entity entity);
        void PauseAnimation(Gnosis::Entity entity);
        void StopAnimation(Gnosis::Entity entity);
        void SetAnimationFrame(Gnosis::Entity entity, int frame);
        
        // Texture management
        void LoadTexture(const std::string& textureId, const std::string& filePath);
        void UnloadTexture(const std::string& textureId);
        void SetTextureBasePath(const std::string& basePath);

    private:
        Gnosis::ECS* m_ecsCoordinator;
        const GameCore::PlatformDelegates& m_delegates;
        std::string m_textureBasePath;
        
        // Texture cache (textureId -> platform texture handle)
        std::unordered_map<std::string, uint32_t> m_textureCache;
        
        // Pending texture loads (textureId -> pending status)
        std::unordered_map<std::string, bool> m_pendingTextures;
        
        // Scoped struct for texture loading context (replaces function-local struct)
        struct TextureLoadContext {
            std::string textureId;
            std::string path;
            SpriteSystem* system;
            Gnosis::Entity entity;
            
            TextureLoadContext(const std::string& id, const std::string& p, SpriteSystem* s, Gnosis::Entity e)
                : textureId(id), path(p), system(s), entity(e) {}
        };
        
        uint32_t GetOrLoadTexture(const std::string& textureId, Gnosis::Entity entity = Gnosis::Entity());
        static void HandleTextureLoaded(GameCore::TextureData* textureData, const char* error, void* userData);
        std::string GetFullTexturePath(const std::string& textureId) const;
        Gnosis::GNRectangle CalculateSourceRect(const Sprite& sprite) const;
        bool IsEntityVisible(Gnosis::Entity entity) const;
        
        // Helper methods
        void UpdateSpriteAnimation(Sprite* sprite, float deltaTime);
        void RenderSprite(Gnosis::Entity entity, const Transform& transform, const Sprite& sprite);
    };



} // namespace GameCore

#endif // FLOPPY_TURD_SPRITE_SYSTEM_H
