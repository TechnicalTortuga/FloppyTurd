#pragma once

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include <vector>
#include <unordered_set>

namespace GameCore {

    // Forward declarations
    class SkillSystem;
    class LevelManager;

    /**
     * ProjectileSystem - Manages all projectile behavior, pooling, and lifecycle
     *
     * Responsibilities:
     * - Object pooling for player (8) and enemy (16) projectiles
     * - Projectile spawning, updating, and cleanup
     * - Off-screen detection and pool recycling
     * - Sprite animation management
     * - Collision detection coordination
     * - Homing projectile behavior (via SkillSystem)
     * - Projectile splitting (via SkillSystem)
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

        // System dependencies
        void SetSkillSystem(SkillSystem* skillSystem) { m_skillSystem = skillSystem; }
        void SetLevelManager(LevelManager* levelManager) { m_levelManager = levelManager; }

        // Pool management
        void InitializePools();

        // Projectile spawning (now handles splitting internally based on SkillSystem)
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
        SkillSystem* m_skillSystem = nullptr;
        LevelManager* m_levelManager = nullptr;

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

        // Pool constants - increased for split projectiles
        static constexpr int MAX_PLAYER_PROJECTILES = 24;  // 8 * 3 for max split
        static constexpr int MAX_ENEMY_PROJECTILES = 16;

        // Private methods
        Entity GetInactiveProjectile(bool isPlayerProjectile);
        void ReturnProjectileToPool(Entity projectile);
        bool IsProjectileOffScreen(const Transform* transform, const Physics* physics);
        void ConfigureProjectileSprite(Entity projectile, ProjectileType projectileType);
        void UpdateActiveProjectiles(float deltaTime);

        // Homing and split support
        Entity FindNearestOnScreenEnemy(const GNVector2& projectilePos, const GNVector2& projectileDir,
                                        float maxAngleDegrees, const std::unordered_set<Entity>& excludeTargets);
        void ApplyHomingBehavior(Entity projectile, float deltaTime);
        void SpawnSplitProjectiles(const GNVector2& position, const GNVector2& direction, 
                                   int splitCount, ProjectileType projectileType);
        bool IsEntityOnScreen(Entity entity) const;
    };

} // namespace GameCore

