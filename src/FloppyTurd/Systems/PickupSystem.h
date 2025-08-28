#ifndef FLOPPY_TURD_PICKUP_SYSTEM_H
#define FLOPPY_TURD_PICKUP_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Config/LevelConfig.h"
#include "../Components/GameComponents.h"
#include "LevelManager.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace GameCore {

    /**
     * PickupSystem - Centralized management for coin/heart pickups
     * - Spawns pickups for obstacle groups per level ratios
     * - Handles player collision with pickups (single-trigger per pickup)
     * - Deactivates/hides collected pickups and removes from active processing
     * - Reactivates/repositions pickups on obstacle group wrap
     */
    class PickupSystem {
    public:
        // Callback function type for coin collection
        using CoinCollectedCallback = std::function<void(int)>;
        
        PickupSystem(Gnosis::ECS* ecsSystem,
                     LevelManager* levelManager,
                     GameCore::PlatformDelegates* platformDelegates,
                     const LevelConfig* levelConfig)
            : m_ecsSystem(ecsSystem)
            , m_levelManager(levelManager)
            , m_platformDelegates(platformDelegates)
            , m_levelConfig(levelConfig)
            , m_playerEntity(0) {}

        void SetPlayerEntity(Gnosis::Entity player) { m_playerEntity = player; }
        void SetLevelConfig(const LevelConfig* cfg) { m_levelConfig = cfg; }
        void SetCoinCollectedCallback(CoinCollectedCallback callback) { m_coinCollectedCallback = callback; }

        // Main per-frame update: spawns for new groups, handles collisions, wraps
        void Update(float deltaTime);

        // Tear down any remaining pickup entities (on level exit)
        void ClearAll();

    private:
        // Dependencies
        Gnosis::ECS* m_ecsSystem;
        LevelManager* m_levelManager;
        GameCore::PlatformDelegates* m_platformDelegates;
        const LevelConfig* m_levelConfig;
        Gnosis::Entity m_playerEntity;
        CoinCollectedCallback m_coinCollectedCallback;

        // Active pickup tracking for O(1) removal
        std::vector<Gnosis::Entity> m_activePickups;
        std::unordered_map<Gnosis::Entity, size_t> m_pickupIndex;
        std::unordered_map<int, std::vector<Gnosis::Entity>> m_groupCoins;  // groupId -> pickups

        // Per-frame gate to avoid double-processing same pickup
        std::unordered_set<Gnosis::Entity> m_collectedThisFrame;

        // Internal helpers
        void handlePickupCollisions();
        void spawnCoinsForGroup(int groupId);
        void repositionCoinsForGroup(int groupId);
        void removeGroupIfMissing(const std::unordered_set<int>& currentGroups);

        // Utility
        inline bool isCoinType(const std::string& type) const {
            return type == "GoldCoin" || type == "BlueCoin" || type == "RedCoin";
        }
    };

} // namespace GameCore

#endif // FLOPPY_TURD_PICKUP_SYSTEM_H


