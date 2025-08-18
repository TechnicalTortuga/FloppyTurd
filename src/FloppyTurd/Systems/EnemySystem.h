#ifndef FLOPPY_TURD_ENEMY_SYSTEM_H
#define FLOPPY_TURD_ENEMY_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"
#include <vector>

namespace GameCore {

    class EnemySystem {
    public:
        EnemySystem(Gnosis::ECS* ecsSystem, LevelManager* levelManager);
        void Update(float deltaTime);

    private:
        // Core system references
        Gnosis::ECS* m_ecsSystem;
        LevelManager* m_levelManager;
        float m_time;
        
        // Enemy behavior methods
        void UpdateEnemyStates(float deltaTime);
        void UpdateEnemyMovement(float deltaTime);
        void UpdateEnemyAnimations(float deltaTime);
        void UpdateSnowmanThrower(float deltaTime, Gnosis::Entity enemy, Enemy* enemyComp, Transform* transform);
        
        // Helper methods
        bool IsEnemyOnScreen(const Transform* transform);
        void ChangeEnemyState(Enemy* enemy, EnemyState newState, float duration = 0.0f);
        void SpawnEnemyProjectile(Gnosis::Entity enemy, const Enemy* enemyComp, const Transform* transform);
        void InitializeEnemyBehavior(Enemy* enemy, const std::string& movementPattern);
        void GroundEnemy(Enemy* enemy, Transform* transform);
        
        // Projectile management
        std::vector<Gnosis::Entity> m_enemyProjectiles;
        void UpdateEnemyProjectiles(float deltaTime);
        void CleanupProjectiles();
    };

} // namespace GameCore

#endif // FLOPPY_TURD_ENEMY_SYSTEM_H


