#ifndef FLOPPY_TURD_ENEMY_SYSTEM_H
#define FLOPPY_TURD_ENEMY_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "LevelManager.h"
#include "ProjectileSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <vector>

// Use shorter type names
using Gnosis::ECS;
using Gnosis::EnemyType;
using GameCore::Entity;

namespace GameCore {

    class EnemySystem {
    public:
        EnemySystem(ECS* ecsSystem, LevelManager* levelManager, ProjectileSystem* projectileSystem);
        void Update(float deltaTime);

    private:
        // Core system references
        ECS* m_ecsSystem;
        LevelManager* m_levelManager;
        ProjectileSystem* m_projectileSystem;
        float m_time;
        
        // Enemy behavior methods
        void UpdateEnemyStates(float deltaTime);
        void UpdateEnemyMovement(float deltaTime);
        void UpdateEnemyAnimations(float deltaTime);
        void UpdateSnowmanThrower(float deltaTime, Entity enemy, Enemy* enemyComp, Transform* transform);

        // Helper methods
        bool IsEnemyOnScreen(const Transform* transform);
        void ChangeEnemyState(Enemy* enemy, EnemyState newState, float duration = 0.0f);
        void SpawnEnemyProjectile(Entity enemy, const Enemy* enemyComp, const Transform* transform);
        void InitializeEnemyBehavior(Enemy* enemy, const std::string& movementPattern);
        void GroundEnemy(Enemy* enemy, Transform* transform);
        
        // Projectile management
        std::vector<Entity> m_enemyProjectiles;
        void UpdateEnemyProjectiles(float deltaTime);
        void CleanupProjectiles();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_ENEMY_SYSTEM_H


