#include "PickupSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>
#include <set>

namespace GameCore {

    void PickupSystem::Update(float deltaTime) {
        if (!m_ecsSystem || !m_levelManager || !m_levelConfig || m_playerEntity == 0) {
            return;
        }

        GN_LOG_DEBUG("PickupSystem::Update begin - active=" + std::to_string(m_activePickups.size()));

        // 1) Handle collisions (deactivate and remove from active list immediately)
        handlePickupCollisions();

        // Heart bobbing (coins do not bob)
        for (Gnosis::Entity e : m_activePickups) {
            Pickup* p = m_ecsSystem->GetComponent<Pickup>(e);
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            if (!p || !t || !p->isActive) continue;
            if (!isCoinType(p->pickupType) && p->bobbingAmplitude > 0.0f && p->bobbingSpeed > 0.0f) {
                // Establish base Y once per activation
                if (p->bobbingTimer == 0.0f) {
                    p->bobbingBaseY = t->position.y;
                }
                p->bobbingTimer += deltaTime;
                float offset = std::sin(p->bobbingTimer * 2.0f * 3.14159265f * p->bobbingSpeed) * p->bobbingAmplitude;
                t->position.y = p->bobbingBaseY + offset;
            }
        }

        // 2) Discover groups currently active and spawn coins for new ones
        const auto& activeObstacles = m_levelManager->GetActiveObstacles();
        std::unordered_set<int> currentGroups;
        for (Gnosis::Entity obstacle : activeObstacles) {
            Group* group = m_ecsSystem->GetComponent<Group>(obstacle);
            if (group) {
                currentGroups.insert(group->id);
            }
        }

        for (int groupId : currentGroups) {
            if (m_groupCoins.find(groupId) == m_groupCoins.end()) {
                if (m_levelManager->GetObstacleSystem()->IsGroupReadyForCoins(groupId)) {
                    spawnCoinsForGroup(groupId);
                }
            }
        }

        // 3) Consume wrap events and reposition/reactivate coins
        for (int wrapped : m_levelManager->ConsumeWrappedGroups()) {
            repositionCoinsForGroup(wrapped);
        }

        // 4) Remove coin groups that no longer exist
        removeGroupIfMissing(currentGroups);

        GN_LOG_DEBUG("PickupSystem::Update end - active=" + std::to_string(m_activePickups.size()));
    }

    void PickupSystem::ClearAll() {
        if (!m_ecsSystem) return;
        for (Gnosis::Entity e : m_activePickups) {
            if (e != 0) m_ecsSystem->DestroyEntity(e);
        }
        m_activePickups.clear();
        m_pickupIndex.clear();
        m_groupCoins.clear();
        m_collectedThisFrame.clear();
    }

    void PickupSystem::handlePickupCollisions() {
        if (!m_ecsSystem || m_activePickups.empty()) return;

        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        Hitbox* playerHitbox = m_ecsSystem->GetComponent<Hitbox>(m_playerEntity);
        Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        PlayerComponent* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
        if (!playerTransform || !playerHitbox || !playerSprite || !playerComp) return;

        float pHalfW = playerSprite->width * playerTransform->scale.x * 0.5f;
        float pHalfH = playerSprite->height * playerTransform->scale.y * 0.5f;
        float pCenterX = playerTransform->position.x + pHalfW + (playerHitbox->offsetX * playerTransform->scale.x);
        float pCenterY = playerTransform->position.y + pHalfH + (playerHitbox->offsetY * playerTransform->scale.y);
        float pRadius = playerHitbox->radius * ((playerTransform->scale.x + playerTransform->scale.y) * 0.5f);

        m_collectedThisFrame.clear();

        // Index-based iteration to allow safe swap-pop removal
        size_t i = 0;
        while (i < m_activePickups.size()) {
            Gnosis::Entity e = m_activePickups[i];
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
            Pickup* p = m_ecsSystem->GetComponent<Pickup>(e);
            Hitbox* hb = m_ecsSystem->GetComponent<Hitbox>(e);
            if (!t || !s || !p || !hb || !p->isActive) {
                i++;
                continue;
            }

            float scaledW = hb->width * t->scale.x;
            float scaledH = hb->height * t->scale.y;
            float centerX = t->position.x + (scaledW * 0.5f);
            float centerY = t->position.y + (scaledH * 0.5f);
            float left = centerX - (scaledW * 0.5f);
            float top = centerY - (scaledH * 0.5f);

            float closestX = std::max(left, std::min(pCenterX, left + scaledW));
            float closestY = std::max(top, std::min(pCenterY, top + scaledH));
            float dx = pCenterX - closestX;
            float dy = pCenterY - closestY;
            bool collided = (dx * dx + dy * dy) <= (pRadius * pRadius);

            if (!collided || m_collectedThisFrame.find(e) != m_collectedThisFrame.end()) {
                i++;
                continue;
            }
            m_collectedThisFrame.insert(e);

            GN_LOG_DEBUG(std::string("PickupSystem: collided id=") + std::to_string(e) +
                         " type=" + p->pickupType +
                         " active=true visible=" + (s->visible ? "true" : "false"));

            // Award effects
            if (isCoinType(p->pickupType)) {
                // Log RIGHT before playing sound to track which entity is retriggering
                std::string idxStr = "n/a";
                auto idxIt = m_pickupIndex.find(e);
                if (idxIt != m_pickupIndex.end()) idxStr = std::to_string(idxIt->second);
                GN_LOG_DEBUG(std::string("PickupSystem: willPlaySound id=") + std::to_string(e) +
                             " type=" + p->pickupType +
                             " idx=" + idxStr +
                             " active=" + (p->isActive ? "true" : "false") +
                             " visible=" + (s->visible ? "true" : "false"));

                playerComp->sessionCoins += (p->pickupType == "GoldCoin" ? 1 : p->value);
                if (m_platformDelegates && m_platformDelegates->audio.playSound) {
                    m_platformDelegates->audio.playSound("pickup.mp3", 0.6f);
                }
            } else {
                std::string idxStr = "n/a";
                auto idxIt = m_pickupIndex.find(e);
                if (idxIt != m_pickupIndex.end()) idxStr = std::to_string(idxIt->second);
                GN_LOG_DEBUG(std::string("PickupSystem: willPlaySound id=") + std::to_string(e) +
                             " type=" + p->pickupType +
                             " idx=" + idxStr +
                             " active=" + (p->isActive ? "true" : "false") +
                             " visible=" + (s->visible ? "true" : "false"));

                if (m_platformDelegates && m_platformDelegates->audio.playSound) {
                    m_platformDelegates->audio.playSound("pickup.mp3", 0.7f);
                }
            }

            // Deactivate and hide immediately
            p->isActive = false;
            s->visible = false;

            GN_LOG_DEBUG(std::string("PickupSystem: deactivated id=") + std::to_string(e) +
                         " active=" + (p->isActive ? "true" : "false") +
                         " visible=" + (s->visible ? "true" : "false"));

            // Remove from active list via swap/pop at the CURRENT iteration index i
            // This guarantees we remove the exact pickup we just collided with
            size_t last = m_activePickups.size() - 1;
            if (i != last) {
                Gnosis::Entity moved = m_activePickups[last];
                m_activePickups[i] = moved;
                m_pickupIndex[moved] = i;
            }
            m_activePickups.pop_back();
            m_pickupIndex.erase(e);
            GN_LOG_DEBUG(std::string("PickupSystem: removed from active list id=") + std::to_string(e));

            // Note: do not increment i; a new element may have been swapped into index i
        }
    }

    void PickupSystem::spawnCoinsForGroup(int groupId) {
        if (!m_ecsSystem || !m_levelManager || !m_levelConfig) return;
        if (!m_levelManager->GetObstacleSystem()->IsGroupReadyForCoins(groupId)) return;

        auto pattern = m_levelManager->GetObstacleSystem()->DetectGroupPattern(groupId);
        auto positions = m_levelManager->GetObstacleSystem()->CalculateCoinPositionsForGroup(groupId, pattern);

        std::vector<Gnosis::Entity> coins;

        auto choosePickupType = [this]() -> std::string {
            if (!m_levelConfig || m_levelConfig->pickupRatios.empty()) {
                return std::string("GoldCoin");
            }
            float total = 0.0f;
            for (const auto& r : m_levelConfig->pickupRatios) total += (r.weight > 0.0f ? r.weight : 0.0f);
            if (total <= 0.0f) return std::string("GoldCoin");
            float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float target = roll * total;
            float accum = 0.0f;
            for (const auto& r : m_levelConfig->pickupRatios) {
                float w = (r.weight > 0.0f ? r.weight : 0.0f);
                accum += w;
                if (target <= accum) return r.pickupType;
            }
            return m_levelConfig->pickupRatios.back().pickupType;
        };

        for (const auto& pos : positions) {
            Gnosis::Entity e = m_ecsSystem->CreateEntity();
            if (e == 0) continue;

            // Default cell is designed around 16x16@scale; type-specific overrides will adjust
            const float cellBase = 16.0f;
            const float scale = 8.0f;
            const float halfCell = cellBase * scale * 0.5f; // 64px
            Transform tr(Gnosis::GNVector2(pos.x - halfCell, pos.y - halfCell), 0.0f, Gnosis::GNVector2(scale, scale));
            m_ecsSystem->AddComponent<Transform>(e, tr);

            const std::string type = choosePickupType();

            Sprite sprite;
            if (isCoinType(type)) {
                sprite = Sprite(type, 16.0f, 16.0f, 16, 16, 10, 0.1f);
                sprite.isAnimated = true;
                sprite.playing = true;
                sprite.loop = true;
            } else {
                // Hearts are 32x32 static sprites
                sprite = Sprite(type, 32.0f, 32.0f);
                sprite.isAnimated = false;
                sprite.playing = false;
                sprite.loop = false;
            }
            sprite.color = Gnosis::GNColor(255, 255, 255, 255);
            sprite.visible = true;
            sprite.layer = 3;
            m_ecsSystem->AddComponent<Sprite>(e, sprite);

            Pickup pickup;
            pickup.pickupType = type;
            if (type == "GoldCoin") pickup.value = 1; else if (type == "BlueCoin") pickup.value = 2; else if (type == "RedCoin") pickup.value = 5; else pickup.value = 1;
            pickup.isActive = true;
            // Hearts bob slightly; coins rely on spin animation only
            if (!isCoinType(type)) {
                pickup.bobbingSpeed = 1.5f;
                pickup.bobbingAmplitude = 6.0f;
            } else {
                pickup.bobbingSpeed = 0.0f;
                pickup.bobbingAmplitude = 0.0f;
            }
            m_ecsSystem->AddComponent<Pickup>(e, pickup);

            ScrollSpeed scroll(m_levelConfig->worldSpeed);
            m_ecsSystem->AddComponent<ScrollSpeed>(e, scroll);

            Hitbox hb;
            hb.type = ColliderType::Rectangle;
            hb.width = isCoinType(type) ? 16.0f : 32.0f;
            hb.height = isCoinType(type) ? 16.0f : 32.0f;
            hb.offsetX = 0.0f;
            hb.offsetY = 0.0f;
            m_ecsSystem->AddComponent<Hitbox>(e, hb);

            // Type-specific visual centering within the shared cell (128x128 at scale)
            // Coins (16x16) already centered by cell origin above; hearts (32x32) need -16 X and -16 Y
            if (!isCoinType(type)) {
                Transform* tt = m_ecsSystem->GetComponent<Transform>(e);
                if (tt) {
                    // Heart is 32x32 vs coin cell 16x16. At scale, extra half-extent = (32-16)*scale/2
                    float extraHalf = (32.0f - 16.0f) * tt->scale.x * 0.5f; // typically 64px
                    tt->position.x -= extraHalf;
                    tt->position.y -= extraHalf;
                }
            }

            // Track active
            m_pickupIndex[e] = m_activePickups.size();
            m_activePickups.push_back(e);
            coins.push_back(e);
        }

        m_groupCoins[groupId] = coins;
    }

    void PickupSystem::repositionCoinsForGroup(int groupId) {
        auto it = m_groupCoins.find(groupId);
        if (!m_ecsSystem || !m_levelManager || it == m_groupCoins.end() || it->second.empty()) return;

        auto pattern = m_levelManager->GetObstacleSystem()->DetectGroupPattern(groupId);
        auto newPositions = m_levelManager->GetObstacleSystem()->CalculateCoinPositionsForGroup(groupId, pattern);

        auto& coins = it->second;
        for (size_t i = 0; i < coins.size() && i < newPositions.size(); ++i) {
            Gnosis::Entity e = coins[i];
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            Sprite* s = m_ecsSystem->GetComponent<Sprite>(e);
            Pickup* p = m_ecsSystem->GetComponent<Pickup>(e);
            if (!t) continue;

            const float cellBase = 16.0f;
            const float halfCell = cellBase * t->scale.x * 0.5f; // 64px at scale 8
            const auto& np = newPositions[i];
            t->position.x = np.x - halfCell;
            t->position.y = np.y - halfCell;

            // Re-apply heart centering shift relative to cell origin
            if (p && !isCoinType(p->pickupType)) {
                float extraHalf = (32.0f - 16.0f) * t->scale.x * 0.5f;
                t->position.x -= extraHalf;
                t->position.y -= extraHalf;
            }

            if (s) s->visible = true;
            if (p) {
                bool wasInactive = !p->isActive;
                p->isActive = true;
                p->bobbingTimer = 0.0f;
                if (wasInactive) {
                    m_pickupIndex[e] = m_activePickups.size();
                    m_activePickups.push_back(e);
                }
                GN_LOG_DEBUG(std::string("PickupSystem: reactivated id=") + std::to_string(e) +
                             " group=" + std::to_string(groupId) +
                             " posX=" + std::to_string(t->position.x));
            }
        }
    }

    void PickupSystem::removeGroupIfMissing(const std::unordered_set<int>& currentGroups) {
        if (!m_ecsSystem) return;
        auto it = m_groupCoins.begin();
        while (it != m_groupCoins.end()) {
            int groupId = it->first;
            if (currentGroups.find(groupId) == currentGroups.end()) {
                for (Gnosis::Entity e : it->second) {
                    auto found = m_pickupIndex.find(e);
                    if (found != m_pickupIndex.end()) {
                        size_t idx = found->second;
                        size_t last = m_activePickups.size() - 1;
                        if (idx != last) {
                            Gnosis::Entity moved = m_activePickups[last];
                            m_activePickups[idx] = moved;
                            m_pickupIndex[moved] = idx;
                        }
                        m_activePickups.pop_back();
                        m_pickupIndex.erase(found);
                    }
                    m_ecsSystem->DestroyEntity(e);
                }
                it = m_groupCoins.erase(it);
            } else {
                ++it;
            }
        }
    }

} // namespace GameCore


