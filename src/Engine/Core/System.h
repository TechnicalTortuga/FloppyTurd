#pragma once

#include "GnosisTypes.h"
#include "Component.h"
#include <vector>
#include <memory>
#include <string>

namespace Gnosis {

    // Forward declarations
    class EntityManager;
    class ComponentManager;
    class EventManager;
    
    /**
     * Base class for all systems in our ECS architecture.
     * Systems contain the logic and operate on entities with specific components.
     */
    class System {
    protected:
        // System has access to core ECS managers
        EntityManager* entityManager;
        ComponentManager* componentManager;
        EventManager* eventManager;
        
        // Entities that this system is interested in
        std::vector<Entity> entities;
        
        // Component signature that entities must have to be processed by this system
        ComponentSignature signature;
        
    public:
        System() : entityManager(nullptr), componentManager(nullptr), eventManager(nullptr) {}
        virtual ~System() = default;
        
        /**
         * Initialize the system with ECS managers
         */
        void Initialize(EntityManager* entityMgr, ComponentManager* componentMgr, EventManager* eventMgr) {
            entityManager = entityMgr;
            componentManager = componentMgr;
            eventManager = eventMgr;
            OnInitialize();
        }
        
        /**
         * Update the system (called every frame)
         */
        virtual void Update(float deltaTime) = 0;
        
        /**
         * Render the system (called every frame after update)
         */
        virtual void Render() {}
        
        /**
         * Called when the system is first initialized
         */
        virtual void OnInitialize() {}
        
        /**
         * Called when the system is being shut down
         */
        virtual void OnShutdown() {}
        
        /**
         * Set the component signature for this system
         */
        void SetSignature(const ComponentSignature& sig) {
            signature = sig;
        }
        
        /**
         * Get the component signature for this system
         */
        const ComponentSignature& GetSignature() const {
            return signature;
        }
        
        /**
         * Called when an entity's signature changes
         */
        void EntitySignatureChanged(Entity entity, const ComponentSignature& entitySignature) {
            if ((entitySignature & signature) == signature) {
                // Entity matches our signature, add it
                AddEntity(entity);
            } else {
                // Entity no longer matches, remove it
                RemoveEntity(entity);
            }
        }
        
        /**
         * Called when an entity is destroyed
         */
        void EntityDestroyed(Entity entity) {
            RemoveEntity(entity);
        }
        
        /**
         * Get all entities this system is processing
         */
        const std::vector<Entity>& GetEntities() const {
            return entities;
        }
        
        /**
         * Get the system name (for debugging)
         */
        virtual std::string GetName() const = 0;
        
    protected:
        /**
         * Add an entity to this system
         */
        void AddEntity(Entity entity) {
            // Check if entity is already in the list
            for (Entity e : entities) {
                if (e == entity) {
                    return; // Already exists
                }
            }
            entities.push_back(entity);
        }
        
        /**
         * Remove an entity from this system
         */
        void RemoveEntity(Entity entity) {
            for (auto it = entities.begin(); it != entities.end(); ++it) {
                if (*it == entity) {
                    entities.erase(it);
                    break;
                }
            }
        }
    };
    
    /**
     * Manages all systems in the game.
     * Handles system registration, updates, and cleanup.
     */
    class SystemManager {
    private:
        std::vector<std::unique_ptr<System> > systems;
        EntityManager* entityManager;
        ComponentManager* componentManager;
        EventManager* eventManager;
        
    public:
        SystemManager() : entityManager(nullptr), componentManager(nullptr), eventManager(nullptr) {}
        
        ~SystemManager() {
            Shutdown();
        }
        
        /**
         * Initialize the system manager with ECS managers
         */
        void Initialize(EntityManager* entityMgr, ComponentManager* componentMgr, EventManager* eventMgr) {
            entityManager = entityMgr;
            componentManager = componentMgr;
            eventManager = eventMgr;
        }
        
        /**
         * Register a new system
         */
        template<typename T>
        T* RegisterSystem() {
            T* system = new T();
            systems.push_back(std::unique_ptr<System>(system));
            
            // Initialize the system if managers are available
            if (entityManager && componentManager && eventManager) {
                system->Initialize(entityManager, componentManager, eventManager);
            }
            
            return system;
        }
        
        /**
         * Set the component signature for a system type
         */
        template<typename T>
        void SetSystemSignature(const ComponentSignature& signature) {
            for (auto& system : systems) {
                T* typedSystem = dynamic_cast<T*>(system.get());
                if (typedSystem) {
                    typedSystem->SetSignature(signature);
                    break;
                }
            }
        }
        
        /**
         * Get a system by type
         */
        template<typename T>
        T* GetSystem() {
            for (auto& system : systems) {
                T* typedSystem = dynamic_cast<T*>(system.get());
                if (typedSystem) {
                    return typedSystem;
                }
            }
            return nullptr;
        }
        
        /**
         * Update all systems
         */
        void Update(float deltaTime) {
            for (auto& system : systems) {
                system->Update(deltaTime);
            }
        }
        
        /**
         * Render all systems - DISABLED: Use unified SystemManager::Render() instead
         */
        void Render() {
            // DISABLED: Generic system render loop disabled to prevent duplicate rendering
            // All rendering now goes through SystemManager::Render() -> RenderSystem::Render()
            // This prevents multiple ECS coordinators from calling individual system render methods
        }
        
        /**
         * Called when an entity's signature changes
         */
        void EntitySignatureChanged(Entity entity, const ComponentSignature& signature) {
            for (auto& system : systems) {
                system->EntitySignatureChanged(entity, signature);
            }
        }
        
        /**
         * Called when an entity is destroyed
         */
        void EntityDestroyed(Entity entity) {
            for (auto& system : systems) {
                system->EntityDestroyed(entity);
            }
        }
        
        /**
         * Shutdown all systems
         */
        void Shutdown() {
            for (auto& system : systems) {
                system->OnShutdown();
            }
            systems.clear();
        }
        
        /**
         * Get the number of registered systems
         */
        size_t GetSystemCount() const {
            return systems.size();
        }
    };
    
} // namespace Gnosis