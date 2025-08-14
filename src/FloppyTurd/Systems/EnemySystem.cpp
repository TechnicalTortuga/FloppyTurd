#include "EnemySystem.h"
#include <cmath>

namespace GameCore {

EnemySystem::EnemySystem(Gnosis::ECS* ecsSystem, LevelManager* levelManager)
    : m_ecsSystem(ecsSystem)
    , m_levelManager(levelManager)
    , m_time(0.0f) {}

void EnemySystem::Update(float deltaTime) {
    if (!m_ecsSystem || !m_levelManager) return;
    m_time += deltaTime;

    const auto enemies = m_levelManager->GetActiveEnemies();
    for (Gnosis::Entity e : enemies) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(e);
        Enemy* enemy = m_ecsSystem->GetComponent<Enemy>(e);
        if (!transform || !enemy || !enemy->isActive) continue;

        if (!enemy->hasInitializedBaseY) {
            enemy->baseY = transform->position.y;
            enemy->hasInitializedBaseY = true;
        }

        if (enemy->bobbingEnabled) {
            float y = enemy->baseY + std::sin(enemy->bobPhase + m_time * enemy->bobSpeed) * enemy->bobAmplitude;
            transform->position.y = y;
        }
    }
}

} // namespace GameCore


