#ifndef FLOPPY_TURD_ENEMY_SYSTEM_H
#define FLOPPY_TURD_ENEMY_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "LevelManager.h"
#include "../../Engine/Core/GNLog.h"

namespace GameCore {

    class EnemySystem {
    public:
        EnemySystem(Gnosis::ECS* ecsSystem, LevelManager* levelManager);
        void Update(float deltaTime);

    private:
        Gnosis::ECS* m_ecsSystem;
        LevelManager* m_levelManager;
        float m_time;
    };

} // namespace GameCore

#endif // FLOPPY_TURD_ENEMY_SYSTEM_H


