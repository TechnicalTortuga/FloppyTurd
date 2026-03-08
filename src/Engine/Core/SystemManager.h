#ifndef GNOSIS_SYSTEM_MANAGER_H
#define GNOSIS_SYSTEM_MANAGER_H

#include <memory>
#include <vector>
#include "../Platform/PlatformDelegates.h"

// Forward declarations
namespace Gnosis {
    class ECS;
}

namespace GameCore {
    class SpriteSystem;
    class UISystem;
    class RenderSystem;
}

namespace Gnosis {
    using GameCore::PlatformDelegates;

    /**
     * SystemManager - Manages all ECS systems
     * 
     * This class handles:
     * - System initialization and cleanup
     * - System update order and orchestration
     * - System access and management
     * 
     * Systems are updated in a specific order to ensure proper dependencies:
     * 1. Input systems
     * 2. Logic systems (AI, gameplay)
     * 3. Physics systems
     * 4. Rendering systems (sprite animation, etc.)
     */
    class SystemManager {
    public:
        explicit SystemManager(ECS* ecsCoordinator, const PlatformDelegates& delegates);
        ~SystemManager();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update(float deltaTime);
        void Render();

        // System access
        GameCore::SpriteSystem* GetSpriteSystem() const;
        GameCore::UISystem* GetUISystem() const;
        GameCore::RenderSystem* GetRenderSystem() const;

        // System management
        bool IsInitialized() const { return m_initialized; }

    private:
        ECS* m_ecsCoordinator;
        const PlatformDelegates& m_delegates;
        bool m_initialized;

        // Systems (in update order)
        std::unique_ptr<GameCore::SpriteSystem> m_spriteSystem;
        std::unique_ptr<GameCore::UISystem> m_uiSystem;
        std::unique_ptr<GameCore::RenderSystem> m_renderSystem;
        
        // Future systems:
        // std::unique_ptr<GameCore::PhysicsSystem> m_physicsSystem;
        // std::unique_ptr<GameCore::CollisionSystem> m_collisionSystem;
        // std::unique_ptr<GameCore::AudioSystem> m_audioSystem;

        // Helper methods
        void InitializeSystems();
        void ShutdownSystems();
        void UpdateSystems(float deltaTime);
    };

} // namespace Gnosis

#endif // GNOSIS_SYSTEM_MANAGER_H
