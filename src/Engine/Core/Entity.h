#pragma once

#include "GnosisTypes.h"

namespace Gnosis {

    /**
     * Entity represents a unique identifier for game objects in our ECS system.
     * Entities are just IDs - they have no data or behavior themselves.
     * All functionality comes from Components attached to entities.
     */
    class EntityManager {
    private:
        Entity nextEntityId;
        std::vector<bool> entityExists;
        std::vector<Entity> freeEntities;
        
    public:
        EntityManager() : nextEntityId(1) {
            // Reserve space for reasonable number of entities
            entityExists.reserve(10000);
            freeEntities.reserve(1000);
        }
        
        /**
         * Creates a new entity and returns its ID
         */
        Entity CreateEntity() {
            Entity entityId;
            
            if (!freeEntities.empty()) {
                // Reuse a previously destroyed entity ID
                entityId = freeEntities.back();
                freeEntities.pop_back();
                entityExists[entityId] = true;
            } else {
                // Create a new entity ID
                entityId = nextEntityId++;
                
                // Expand the exists array if needed
                if (entityId >= entityExists.size()) {
                    entityExists.resize(entityId + 1, false);
                }
                entityExists[entityId] = true;
            }
            
            return entityId;
        }
        
        /**
         * Destroys an entity and marks its ID for reuse
         */
        void DestroyEntity(Entity entity) {
            if (entity == INVALID_ENTITY || entity >= entityExists.size()) {
                return;
            }
            
            if (entityExists[entity]) {
                entityExists[entity] = false;
                freeEntities.push_back(entity);
            }
        }
        
        /**
         * Checks if an entity exists and is valid
         */
        bool IsEntityValid(Entity entity) const {
            if (entity == INVALID_ENTITY || entity >= entityExists.size()) {
                return false;
            }
            return entityExists[entity];
        }
        
        /**
         * Gets the total number of active entities
         */
        size_t GetActiveEntityCount() const {
            size_t count = 0;
            for (bool exists : entityExists) {
                if (exists) count++;
            }
            return count;
        }
        
        /**
         * Gets all active entities
         */
        std::vector<Entity> GetAllActiveEntities() const {
            std::vector<Entity> activeEntities;
            activeEntities.reserve(GetActiveEntityCount());
            
            for (Entity i = 1; i < entityExists.size(); ++i) {
                if (entityExists[i]) {
                    activeEntities.push_back(i);
                }
            }
            
            return activeEntities;
        }
        
        /**
         * Clears all entities (useful for level transitions)
         */
        void Clear() {
            entityExists.clear();
            freeEntities.clear();
            nextEntityId = 1;
        }
    };
    
} // namespace Gnosis