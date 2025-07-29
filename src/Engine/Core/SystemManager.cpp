#include "SystemManager.h"
#include "ECS.h"
#include "Component.h"
#include "../../FloppyTurd/Systems/SpriteSystem.h"
#include "GNLog.h"

namespace Gnosis {

    // Define the static member variable for Component type IDs
    size_t Component::nextTypeId = 0;

    SystemManager::SystemManager(ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_delegates(delegates)
        , m_initialized(false)
    {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("SystemManager: ECS coordinator is null");
        }
    }

    SystemManager::~SystemManager() {
        Shutdown();
    }

    void SystemManager::Initialize() {
        if (m_initialized) {
            GN_LOG_WARN("SystemManager: Already initialized");
            return;
        }

        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("SystemManager: Cannot initialize without ECS coordinator");
            return;
        }

        GN_LOG_INFO("SystemManager: Initializing systems...");
        
        InitializeSystems();
        
        m_initialized = true;
        GN_LOG_INFO("SystemManager: Systems initialized successfully");
    }

    void SystemManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        GN_LOG_INFO("SystemManager: Shutting down systems...");
        
        ShutdownSystems();
        
        m_initialized = false;
        GN_LOG_INFO("SystemManager: Systems shut down successfully");
    }

    void SystemManager::Update(float deltaTime) {
        if (!m_initialized) {
            return;
        }

        UpdateSystems(deltaTime);
    }
    
    void SystemManager::Render() {
        if (!m_initialized) {
            return;
        }

        // Render all systems
        if (m_spriteSystem) {
            m_spriteSystem->Render();
        }
        
        // Future system rendering:
        // if (m_particleSystem) {
        //     m_particleSystem->Render();
        // }
    }

    GameCore::SpriteSystem* SystemManager::GetSpriteSystem() const {
        return m_spriteSystem.get();
    }

    void SystemManager::InitializeSystems() {
        // Initialize systems in dependency order
        
        // Rendering systems (SpriteSystem handles sprite animation and rendering)
        m_spriteSystem = std::make_unique<GameCore::SpriteSystem>(m_ecsCoordinator, m_delegates);
        GN_LOG_INFO("SystemManager: SpriteSystem initialized with integrated rendering");
        
        // Future systems would be initialized here:
        // m_physicsSystem = std::make_unique<GameCore::PhysicsSystem>(m_ecsCoordinator);
        // m_collisionSystem = std::make_unique<GameCore::CollisionSystem>(m_ecsCoordinator);
        // m_audioSystem = std::make_unique<GameCore::AudioSystem>(m_ecsCoordinator);
    }

    void SystemManager::ShutdownSystems() {
        // Shutdown systems in reverse order
        
        m_spriteSystem.reset();
        GN_LOG_INFO("SystemManager: SpriteSystem shut down");
        
        // Future systems would be shut down here in reverse order
    }

    void SystemManager::UpdateSystems(float deltaTime) {
        // Update systems in the correct order:
        // 1. Input systems (future)
        // 2. Logic systems (future - AI, gameplay)
        // 3. Physics systems (future)
        // 4. Rendering systems (sprite animation, etc.)
        
        if (m_spriteSystem) {
            m_spriteSystem->Update(deltaTime);
        }
        
        // Future system updates:
        // if (m_physicsSystem) {
        //     m_physicsSystem->Update(deltaTime);
        // }
        // if (m_collisionSystem) {
        //     m_collisionSystem->Update(deltaTime);
        // }
    }

} // namespace Gnosis
