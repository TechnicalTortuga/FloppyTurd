#pragma once

#include "GnosisTypes.h"
#include "Entity.h"
#include "Component.h"
#include "../Events/EventManager.h"
#include "../Platform/PlatformDelegates.h"
#include "SystemManager.h"
#include "../../iOS/Threading/ThreadingProxy.h"
#include <memory>
#include <sstream> // Added for debug logging

namespace Gnosis {

    /**
     * Main ECS Coordinator that manages all ECS subsystems.
     * This is the primary interface for interacting with the ECS architecture.
     * Now directly manages systems without the old System base class architecture.
     */
    class ECS {
    private:
        std::unique_ptr<EntityManager> entityManager;
        std::unique_ptr<ComponentManager> componentManager;
        std::unique_ptr<EventManager> eventManager;
        std::unique_ptr<SystemManager> systemManager;

        bool initialized;

        // Screen dimensions (set by RenderSystem, accessible by all systems)
        float screenWidth;
        float screenHeight;
        float logicalWidth;
        float logicalHeight;
        
    public:
        ECS() : initialized(false), screenWidth(0), screenHeight(0), logicalWidth(0), logicalHeight(0) {}
        
        ~ECS() {
            Shutdown();
        }
        
        /**
         * Initialize the ECS system
         */
        void Initialize(const GameCore::PlatformDelegates& delegates) {
            if (initialized) {
                return;
            }
            
            // Create all managers
            entityManager = std::unique_ptr<EntityManager>(new EntityManager());
            componentManager = std::unique_ptr<ComponentManager>(new ComponentManager());
            eventManager = std::unique_ptr<EventManager>(new EventManager());
            
            // Create system manager with platform delegates
            systemManager = std::unique_ptr<SystemManager>(new SystemManager(this, delegates));
            systemManager->Initialize();
            
            initialized = true;
        }
        
        /**
         * Shutdown the ECS system
         */
        void Shutdown() {
            if (!initialized) {
                return;
            }
            
            eventManager->Clear();
            
            // Clean up system manager first
            if (systemManager) {
                systemManager->Shutdown();
                systemManager.reset();
            }
            
            eventManager.reset();
            componentManager.reset();
            entityManager.reset();
            
            initialized = false;
        }
        
        /**
         * Update the ECS system (called every frame)
         * Systems should call this and then handle their own updates
         */
        void Update(float deltaTime) {
            if (!initialized) {
                return;
            }
            
            // Process events first
            eventManager->ProcessEvents();
            
            // Update all systems via system manager
            if (systemManager) {
                systemManager->Update(deltaTime);
            }
        }
        
        /**
         * Render the ECS system (called every frame after update)
         * Systems handle their own rendering now
         */
        void Render() {
            if (!initialized) {
                return;
            }
            
            // Render all systems via system manager
            if (systemManager) {
                systemManager->Render();
            }
        }
        
        // Entity Management
        
        /**
         * Create a new entity
         */
        Entity CreateEntity() {
            if (!initialized) {
                return INVALID_ENTITY;
            }
            return entityManager->CreateEntity();
        }
        
        /**
         * Destroy an entity and all its components
         */
        void DestroyEntity(Entity entity) {
            if (!initialized) {
                return;
            }
            
            // Remove all components
            componentManager->EntityDestroyed(entity);
            
            // Destroy the entity
            entityManager->DestroyEntity(entity);
        }
        
        /**
         * Check if an entity is valid
         */
        bool IsEntityValid(Entity entity) const {
            if (!initialized) {
                return false;
            }
            return entityManager->IsEntityValid(entity);
        }
        
        // Component Management
        
        /**
         * Add a component to an entity
         */
        template<typename T>
        void AddComponent(Entity entity, const T& component) {
            if (!initialized) {
                return;
            }
            
            componentManager->AddComponent<T>(entity, component);
        }
        
        /**
         * Remove a component from an entity
         */
        template<typename T>
        void RemoveComponent(Entity entity) {
            if (!initialized) {
                return;
            }
            
            componentManager->RemoveComponent<T>(entity);
        }
        
        /**
         * Get a component from an entity
         */
        template<typename T>
        T* GetComponent(Entity entity) {
            if (!initialized) {
                return nullptr;
            }
            if (!componentManager->HasComponent<T>(entity)) {
                return nullptr;
            }
            return &componentManager->GetComponent<T>(entity);
        }
        
        /**
         * Check if an entity has a component
         */
        template<typename T>
        bool HasComponent(Entity entity) const {
            if (!initialized) {
                return false;
            }
            return componentManager->HasComponent<T>(entity);
        }
        
        /**
         * Get the component signature for an entity
         */
        ComponentSignature GetEntitySignature(Entity entity) const {
            if (!initialized) {
                return ComponentSignature();
            }
            return componentManager->GetEntitySignature(entity);
        }
        
        // Event Management
        
        /**
         * Subscribe to an event type
         */
        void SubscribeToEvent(EventType eventType, EventHandler handler) {
            if (!initialized) {
                return;
            }
            eventManager->Subscribe(eventType, handler);
        }
        
        /**
         * Dispatch an event immediately
         */
        void DispatchEvent(const Event& event) {
            if (!initialized) {
                return;
            }
            eventManager->DispatchEvent(event);
        }
        
        /**
         * Queue an event to be processed later
         */
        void QueueEvent(std::unique_ptr<Event> event) {
            if (!initialized) {
                return;
            }
            eventManager->QueueEvent(std::move(event));
        }
        
        /**
         * Get the event manager for direct access
         */
        EventManager* GetEventManager() {
            return eventManager.get();
        }
        
        /**
         * Get the system manager for direct access
         */
        SystemManager* GetSystemManager() {
            return systemManager.get();
        }
        
        // Utility Methods
        
        /**
         * Get the number of active entities
         */
        size_t GetEntityCount() const {
            if (!initialized) {
                return 0;
            }
            return entityManager->GetActiveEntityCount();
        }
        
        /**
         * Check if the ECS is initialized
         */
        bool IsInitialized() const {
            return initialized;
        }
        
        /**
         * Get all entities that have the specified components
         * This is used by systems that don't inherit from the old System base class
         */
        template<typename... ComponentTypes>
        std::vector<Entity> GetEntitiesWithComponents() {
            std::vector<Entity> result;
            
            if (!initialized) {
                return result;
            }
            
            // Create component signature for the required components
            ComponentSignature requiredSignature;
            int dummy[] = { (requiredSignature.set(Component::GetComponentTypeId<ComponentTypes>()), 0)... };
            (void)dummy; // Suppress unused variable warning
            
            // Get all entities and check their signatures
            auto allEntities = entityManager->GetAllActiveEntities();
            
            for (Entity entity : allEntities) {
                ComponentSignature entitySignature = componentManager->GetEntitySignature(entity);
                
                // Check if entity has all required components
                if ((entitySignature & requiredSignature) == requiredSignature) {
                    result.push_back(entity);
                }
            }
            
            return result;
        }

        // Screen Dimension Management (accessible by all systems)

        /**
         * Set screen dimensions (called by RenderSystem)
         */
        void SetScreenDimensions(float width, float height, float logicalW, float logicalH) {
            screenWidth = width;
            screenHeight = height;
            logicalWidth = logicalW;
            logicalHeight = logicalH;
        }

        /**
         * Get screen width in pixels
         */
        float GetScreenWidth() const { return screenWidth; }

        /**
         * Get screen height in pixels
         */
        float GetScreenHeight() const { return screenHeight; }

        /**
         * Get logical screen width
         */
        float GetLogicalWidth() const { return logicalWidth; }

        /**
         * Get logical screen height
         */
        float GetLogicalHeight() const { return logicalHeight; }

    };
    
} // namespace Gnosis