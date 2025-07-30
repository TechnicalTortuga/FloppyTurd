#pragma once

#include "GnosisTypes.h"
#include <typeinfo>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <stdexcept>
#include <cstddef>
#include "GNLog.h"

namespace Gnosis {

    /**
     * Base class for all components in our ECS system.
     * Components are pure data containers with no behavior.
     */
    class Component {
    public:
        virtual ~Component() = default;
        
        // Each component type gets a unique ID for fast lookups
        template<typename T>
        static size_t GetComponentTypeId() {
            static size_t typeId = nextTypeId++;
            return typeId;
        }
        
    private:
        static size_t nextTypeId;
    };
    
    // Maximum number of component types we support
    const size_t MAX_COMPONENTS = 64;
    
    // Component signature for fast entity queries
    typedef std::bitset<64> ComponentSignature;
    
    /**
     * Manages all components for all entities.
     * Uses archetype-based storage for cache-friendly iteration.
     */
    class ComponentManager {
    private:
        // Base class for component arrays
        class IComponentArray {
        public:
            virtual ~IComponentArray() = default;
            virtual void EntityDestroyed(Entity entity) = 0;
            virtual std::size_t Size() const = 0;
        };
        
        // Templated component array for specific component types
        template<typename T>
        class ComponentArray : public IComponentArray {
        private:
            // Dense array of components
            std::vector<T> components;
            // Maps entity ID to index in components array
            std::unordered_map<Entity, size_t> entityToIndex;
            // Maps index back to entity ID
            std::unordered_map<std::size_t, Entity> indexToEntity;
            std::size_t size;
            
        public:
            ComponentArray() : size(0) {
                components.reserve(1000); // Reserve space for performance
            }
            
            void AddComponent(Entity entity, T component) {
                if (entityToIndex.find(entity) != entityToIndex.end()) {
                    // Entity already has this component, update it
                    size_t index = entityToIndex[entity];
                    components[index] = component;
                    return;
                }
                
                // Add new component
                std::size_t newIndex = size;
                entityToIndex[entity] = newIndex;
                indexToEntity[newIndex] = entity;
                
                if (newIndex >= components.size()) {
                    components.resize(newIndex + 1);
                }
                components[newIndex] = component;
                ++size;
            }
            
            void RemoveComponent(Entity entity) {
                if (entityToIndex.find(entity) == entityToIndex.end()) {
                    return; // Entity doesn't have this component
                }
                
                // Get the index of the component to remove
                std::size_t indexToRemove = entityToIndex[entity];
                std::size_t lastIndex = size - 1;
                
                // Move the last component to fill the gap
                if (indexToRemove != lastIndex) {
                    components[indexToRemove] = components[lastIndex];
                    
                    // Update mappings for the moved component
                    Entity lastEntity = indexToEntity[lastIndex];
                    entityToIndex[lastEntity] = indexToRemove;
                    indexToEntity[indexToRemove] = lastEntity;
                }
                
                // Clean up mappings
                entityToIndex.erase(entity);
                indexToEntity.erase(lastIndex);
                --size;
            }
            
            T& GetComponent(Entity entity) {
                auto it = entityToIndex.find(entity);
                if (it == entityToIndex.end()) {
                    throw std::runtime_error("Entity does not have this component");
                }
                return components[it->second];
            }
            
            const T& GetComponent(Entity entity) const {
                auto it = entityToIndex.find(entity);
                if (it == entityToIndex.end()) {
                    throw std::runtime_error("Entity does not have this component");
                }
                return components[it->second];
            }
            
            bool HasComponent(Entity entity) const {
                return entityToIndex.find(entity) != entityToIndex.end();
            }
            
            void EntityDestroyed(Entity entity) override {
                if (HasComponent(entity)) {
                    RemoveComponent(entity);
                }
            }
            
            std::size_t Size() const override {
                return size;
            }
            
            // Get all components for iteration
            const std::vector<T>& GetComponents() const {
                return components;
            }
            
            // Get all entities that have this component
            std::vector<Entity> GetEntities() const {
                std::vector<Entity> entities;
                entities.reserve(size);
                for (const auto& pair : entityToIndex) {
                    entities.push_back(pair.first);
                }
                return entities;
            }
        };
        
        // Component arrays for each component type
        std::unordered_map<std::size_t, std::unique_ptr<IComponentArray> > componentArrays;
        
        // Entity signatures (which components each entity has)
        std::unordered_map<Entity, ComponentSignature> entitySignatures;
        
        template<typename T>
        ComponentArray<T>* GetComponentArray() {
            std::size_t typeId = Component::GetComponentTypeId<T>();
            
            auto it = componentArrays.find(typeId);
            if (it == componentArrays.end()) {
                // Create new component array for this type
                ComponentArray<T>* arrayPtr = new ComponentArray<T>();
                componentArrays[typeId] = std::unique_ptr<IComponentArray>(arrayPtr);
                return arrayPtr;
            }
            
            return static_cast<ComponentArray<T>*>(it->second.get());
        }
        
        template<typename T>
        const ComponentArray<T>* GetComponentArray() const {
            std::size_t typeId = Component::GetComponentTypeId<T>();
            
            auto it = componentArrays.find(typeId);
            if (it == componentArrays.end()) {
                return nullptr;
            }
            
            return static_cast<const ComponentArray<T>*>(it->second.get());
        }
        
    public:
        /**
         * Adds a component to an entity
         */
        template<typename T>
        void AddComponent(Entity entity, T component) {
            GN_LOG_INFO(std::string("[ComponentManager] Adding component ") + typeid(T).name() + " to entity " + std::to_string(entity));
            GetComponentArray<T>()->AddComponent(entity, component);
            
            // Update entity signature
            std::size_t typeId = Component::GetComponentTypeId<T>();
            entitySignatures[entity].set(typeId);
            GN_LOG_INFO(std::string("[ComponentManager] Added component ") + typeid(T).name() + " to entity " + std::to_string(entity));
        }
        
        /**
         * Removes a component from an entity
         */
        template<typename T>
        void RemoveComponent(Entity entity) {
            GetComponentArray<T>()->RemoveComponent(entity);
            
            // Update entity signature
            std::size_t typeId = Component::GetComponentTypeId<T>();
            entitySignatures[entity].reset(typeId);
        }
        
        /**
         * Gets a component from an entity
         */
        template<typename T>
        T& GetComponent(Entity entity) {
            return GetComponentArray<T>()->GetComponent(entity);
        }
        
        template<typename T>
        const T& GetComponent(Entity entity) const {
            const ComponentArray<T>* array = GetComponentArray<T>();
            if (!array) {
                throw std::runtime_error("Component type not registered");
            }
            return array->GetComponent(entity);
        }
        
        /**
         * Checks if an entity has a specific component
         */
        template<typename T>
        bool HasComponent(Entity entity) const {
            const ComponentArray<T>* array = GetComponentArray<T>();
            return array ? array->HasComponent(entity) : false;
        }
        
        /**
         * Gets the signature (component mask) for an entity
         */
        ComponentSignature GetEntitySignature(Entity entity) const {
            auto it = entitySignatures.find(entity);
            if (it != entitySignatures.end()) {
                return it->second;
            }
            return ComponentSignature(); // Empty signature
        }
        
        /**
         * Queries for entities that have all specified component types
         */
        template<typename... ComponentTypes>
        std::vector<Entity> Query() const {
            ComponentSignature requiredSignature;
            
            // Set bits for all required component types
            int dummy[] = { (requiredSignature.set(Component::GetComponentTypeId<ComponentTypes>()), 0)... };
            (void)dummy; // Suppress unused variable warning
            
            std::vector<Entity> matchingEntities;
            
            for (const auto& pair : entitySignatures) {
                Entity entity = pair.first;
                const ComponentSignature& entitySig = pair.second;
                
                // Check if entity has all required components
                if ((entitySig & requiredSignature) == requiredSignature) {
                    matchingEntities.push_back(entity);
                }
            }
            
            return matchingEntities;
        }
        
        /**
         * Called when an entity is destroyed to clean up its components
         */
        void EntityDestroyed(Entity entity) {
            // Remove from all component arrays
            for (auto& pair : componentArrays) {
                pair.second->EntityDestroyed(entity);
            }
            
            // Remove signature
            entitySignatures.erase(entity);
        }
        
        /**
         * Gets all entities that have a specific component type
         */
        template<typename T>
        std::vector<Entity> GetEntitiesWithComponent() const {
            const ComponentArray<T>* array = GetComponentArray<T>();
            return array ? array->GetEntities() : std::vector<Entity>();
        }
        
        /**
         * Clears all components (useful for level transitions)
         */
        void Clear() {
            componentArrays.clear();
            entitySignatures.clear();
        }
    };
    
} // namespace Gnosis