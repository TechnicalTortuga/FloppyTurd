#ifndef FLOPPY_TURD_SPRITE_SYSTEM_H
#define FLOPPY_TURD_SPRITE_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <vector>
#include <unordered_map>

namespace GameCore {

    /**
     * SpriteSystem - Updates sprite animations only.
     * 
     * Responsibilities:
     * - Animation frame updates based on deltaTime
     * - Frame looping and animation state management
     * 
     * Rendering and texture management are handled exclusively by RenderSystem.
     */
    class SpriteSystem {
    public:
        explicit SpriteSystem(Gnosis::ECS* ecsCoordinator, const PlatformDelegates& delegates);
        ~SpriteSystem() = default;

        // Update and render all sprites
        void Update(float deltaTime);
        void Render();
        
        // Animation control
        void PlayAnimation(Gnosis::Entity entity);
        void PauseAnimation(Gnosis::Entity entity);
        void StopAnimation(Gnosis::Entity entity);
        void SetAnimationFrame(Gnosis::Entity entity, int frame);

    private:
        Gnosis::ECS* m_ecsCoordinator;
        const PlatformDelegates& m_delegates;
        
        Gnosis::GNRectangle CalculateSourceRect(const Sprite& sprite) const;
        bool IsEntityVisible(Gnosis::Entity entity) const;
        
        // Helper methods
        void UpdateSpriteAnimation(Sprite* sprite, float deltaTime);
        void RenderSprite(Gnosis::Entity entity, const Transform& transform, const Sprite& sprite);
    };



} // namespace GameCore

#endif // FLOPPY_TURD_SPRITE_SYSTEM_H
