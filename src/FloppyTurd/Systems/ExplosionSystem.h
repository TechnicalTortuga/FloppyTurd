#pragma once

#include "../Components/GameComponents.h"
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Core/GNLog.h"
#include <vector>

namespace GameCore {

/**
 * @brief Explosion System for boss death sequences
 * 
 * Manages animated explosion effects that play during dramatic boss deaths.
 * Supports both small (7-frame) and big (8-frame) explosion animations.
 */
class ExplosionSystem {
public:
    ExplosionSystem(Gnosis::ECS* ecsSystem);
    ~ExplosionSystem();

    /**
     * @brief Update all active explosions
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);

    /**
     * @brief Spawn a small explosion (7 frames, 32x32)
     * @param position World position to spawn explosion
     * @param scale Scale factor (default 8.0f for boss explosions)
     * @param animationSpeed Animation speed multiplier (1.0 = normal, 0.5 = slow-mo)
     * @return Entity ID of spawned explosion
     */
    Entity SpawnSmallExplosion(const Gnosis::GNVector2& position, float scale = 8.0f, float animationSpeed = 1.0f);

    /**
     * @brief Spawn a big explosion (8 frames, 32x32)
     * @param position World position to spawn explosion
     * @param scale Scale factor (default 8.0f for boss explosions)
     * @param animationSpeed Animation speed multiplier (1.0 = normal, 0.5 = slow-mo)
     * @return Entity ID of spawned explosion
     */
    Entity SpawnBigExplosion(const Gnosis::GNVector2& position, float scale = 8.0f, float animationSpeed = 1.0f);

    /**
     * @brief Check if any explosions are still playing
     * @return true if at least one explosion is active
     */
    bool HasActiveExplosions() const;

    /**
     * @brief Clear all active explosions
     */
    void ClearAll();

    /**
     * @brief Get count of active explosions
     */
    int GetActiveCount() const { return static_cast<int>(m_activeExplosions.size()); }

private:
    Gnosis::ECS* m_ecsSystem;
    std::vector<Entity> m_activeExplosions;

    /**
     * @brief Create an explosion entity with the specified sprite
     * @param textureId Texture asset name ("blast_small" or "blast_big")
     * @param frameCount Number of animation frames
     * @param position World position
     * @param scale Scale factor
     * @param animationSpeed Animation speed multiplier
     */
    Entity CreateExplosion(const std::string& textureId, int frameCount, 
                          const Gnosis::GNVector2& position, float scale, float animationSpeed);
};

} // namespace GameCore