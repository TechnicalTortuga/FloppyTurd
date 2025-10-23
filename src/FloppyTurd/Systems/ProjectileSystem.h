#pragma once

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include <vector>

namespace GameCore {

    /**
     * ProjectileSystem - Manages all projectile behavior, pooling, and lifecycle
     *
     * Responsibilities:
     * - Object pooling for player (8) and enemy (16) projectiles
     * - Projectile spawning, updating, and cleanup
     * - Off-screen detection and pool recycling
     * - Sprite animation management
     * - Collision detection coordination
     */
    class ProjectileSystem {
    public:
        ProjectileSystem(Gnosis::ECS* ecsSystem);
        ~ProjectileSystem();

        // System lifecycle
        void Initialize();
        void Update(float deltaTime);
        void Cleanup();
        void ResetForNewGame();

        // Pool management
        void InitializePools();

        // Projectile spawning
        Entity SpawnPlayerProjectile(const GNVector2& position,
                                    const GNVector2& direction,
                                    ProjectileType projectileType = ProjectileType::POOP_BALL);

        Entity SpawnEnemyProjectile(const GNVector2& position,
                                   const GNVector2& direction,
                                   ProjectileType projectileType,
                                   int damage = 1);

        // Pool status
        int GetActivePlayerProjectileCount() const;
        int GetActiveEnemyProjectileCount() const;
        int GetTotalPoolSize() const;

        // Collision detection access
        const std::vector<Entity>& GetActivePlayerProjectiles() const { return m_playerProjectiles.activeProjectiles; }
        const std::vector<Entity>& GetActiveEnemyProjectiles() const { return m_enemyProjectiles.activeProjectiles; }

    private:
        Gnosis::ECS* m_ecsSystem;

        // Projectile pools
        struct ProjectilePool {
            std::vector<Entity> activeProjectiles;
            std::vector<Entity> inactiveProjectiles;
            std::vector<Entity> allProjectiles; // Pre-allocated pool

            void Clear() {
                activeProjectiles.clear();
                inactiveProjectiles.clear();
                allProjectiles.clear();
            }
        };

        ProjectilePool m_playerProjectiles;
        ProjectilePool m_enemyProjectiles;

        // Pool constants
        static constexpr int MAX_PLAYER_PROJECTILES = 8;
        static constexpr int MAX_ENEMY_PROJECTILES = 16;

        // Private methods
        Entity GetInactiveProjectile(bool isPlayerProjectile);
        void ReturnProjectileToPool(Entity projectile);
        bool IsProjectileOffScreen(const Transform* transform, const Physics* physics);


        void ConfigureProjectileSprite(Entity projectile, ProjectileType projectileType);
        void UpdateActiveProjectiles(float deltaTime);


    };

} // namespace GameCore
