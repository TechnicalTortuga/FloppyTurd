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
        
        // Single-pass ECS pattern: process individual enemy components
        void ProcessEnemyState(float deltaTime, Entity e, Enemy* enemy, Sprite* sprite);
        void ProcessEnemyMovement(float deltaTime, Enemy* enemy, Transform* transform, Gnosis::Entity enemyEntity);
        void ProcessEnemyAnimation(float deltaTime, Enemy* enemy, Sprite* sprite, StateAnimation* stateAnim);
        void ProcessEnemyCollision(Entity e, Enemy* enemy, Transform* transform, Hitbox* hitbox, Sprite* sprite, StateAnimation* stateAnim, const std::vector<Gnosis::Entity>& activeProjectiles);
        
        // Special enemy behaviors
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


