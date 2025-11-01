#include "PickupSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>
#include <set>

namespace GameCore {

    void PickupSystem::Update(float deltaTime) {
        if (!m_ecsSystem || !m_levelManager || !m_levelConfig || m_playerEntity == 0) {
            return;
        }

        // GN_LOG_DEBUG("PickupSystem::Update begin - active=" + std::to_string(m_activePickups.size()));

        // 1) Handle collisions (deactivate and remove from active list immediately)
        handlePickupCollisions();

        // Heart bobbing (coins do not bob)
        for (Gnosis::Entity e : m_activePickups) {
            Pickup* p = m_ecsSystem->GetComponent<Pickup>(e);
            Transform* t = m_ecsSystem->GetComponent<Transform>(e);
            if (!p || !t || !p->isActive) continue;
            if (!IsPickupCoinType(p->type) && p->bobbingAmplitude > 0.0f && p->bobbingSpeed > 0.0f) {
                // Establish base Y once per activation
                if (p->bobbingTimer == 0.0f) {
                    p->bobbingBaseY = t->position.y;
                }
                p->bobbingTimer += deltaTime;
                float offset = std::sin(p->bobbingTimer * 2.0f * 3.14159265f * p->bobbingSpeed) * p->bobbingAmplitude;
                t->position.y = p->bobbingBaseY + offset;
            }
        }

        // 2) Apply magnet effects if enabled
        if (m_coinMagnetEnabled || m_heartMagnetEnabled) {
            applyMagnetEffects(deltaTime);
        }

        // 3) Discover groups currently active and spawn coins for new ones
        // Use a helper to compute the active group set so we can recompute it after wraps
        auto recomputeActiveGroups = [&]() -> std::unordered_set<int> {
            std::unordered_set<int> groups;
            const auto& activeObstaclesLocal = m_levelManager->GetActiveObstacles();
            for (Gnosis::Entity obstacle : activeObstaclesLocal) {
                Group* group = m_ecsSystem->GetComponent<Group>(obstacle);
                if (group) {
                    groups.insert(group->id);
                }
            }
            return groups;
        };

        // 3) REMOVED: Legacy coin spawning loop - all spawning now handled by LevelManager orchestrator
        // LevelManager calls SpawnCoinsForGroup() during SpawnGroup() initialization
        // Coin repositioning happens automatically when groups wrap via UpdateGroupMemberPositions()
        
        // Compute current active groups for cleanup
        std::unordered_set<int> currentGroups = recomputeActiveGroups();

        // 4) Remove coin groups that no longer exist
        removeGroupIfMissing(currentGroups);

        // GN_LOG_DEBUG("PickupSystem::Update end - active=" + std::to_string(m_activePickups.size()));
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

            // Performance: Commented out expensive debug logging
            // GN_LOG_DEBUG(std::string("PickupSystem: collided id=") + std::to_string(e) +
            //              " type=" + p->pickupType +
            //              " active=true visible=" + (s->visible ? "true" : "false"));

            // Award effects
            if (IsPickupCoinType(p->type)) {
                // Performance: Commented out expensive debug logging
                // std::string idxStr = "n/a";
                // auto idxIt = m_pickupIndex.find(e);
                // if (idxIt != m_pickupIndex.end()) idxStr = std::to_string(idxIt->second);
                // GN_LOG_DEBUG(std::string("PickupSystem: willPlaySound id=") + std::to_string(e) +
                //              " type=" + p->pickupType +
                //              " idx=" + idxStr +
                //              " active=" + (p->isActive ? "true" : "false") +
                //              " visible=" + (s->visible ? "true" : "false"));

                // Call callback function instead of directly modifying sessionCoins
                int coinValue = (p->type == PickupType::GoldCoin ? 1 : p->value);
                if (m_coinCollectedCallback) {
                    m_coinCollectedCallback(coinValue);
                } else {
                    // Fallback: direct modification if callback not set
                    playerComp->sessionCoins += coinValue;
                }
                
                if (m_platformDelegates && m_platformDelegates->audio.playSound) {
                    m_platformDelegates->audio.playSound("pickup.mp3", 0.6f);
                }
            } else {
                // Performance: Commented out expensive debug logging
                // std::string idxStr = "n/a";
                // auto idxIt = m_pickupIndex.find(e);
                // if (idxIt != m_pickupIndex.end()) idxStr = std::to_string(idxIt->second);
                // GN_LOG_DEBUG(std::string("PickupSystem: willPlaySound id=") + std::to_string(e) +
                //              " type=" + p->pickupType +
                //              " idx=" + idxStr +
                //              " active=" + (p->isActive ? "true" : "false") +
                //              " visible=" + (s->visible ? "true" : "false"));

                // Play appropriate heart sound based on type
                std::string soundFile = "SmallHealthPickup.wav"; // Default to small
                int healAmount = 1; // Default heal amount

                if (p->type == PickupType::HeartBig) {
                    soundFile = "BigHealthPickup.wav";
                    healAmount = 3; // Big hearts heal 3 slices like in old system
                } else if (p->type == PickupType::Heart) {
                    soundFile = "SmallHealthPickup.wav";
                    healAmount = 1; // Small hearts heal 1 slice
                }

                if (m_platformDelegates && m_platformDelegates->audio.playSound) {
                    m_platformDelegates->audio.playSound(soundFile.c_str(), 0.8f);
                }

                // Call heart collection callback to actually heal the player
                if (m_heartCollectedCallback) {
                    m_heartCollectedCallback(healAmount);
                } else {
                    GN_LOG_INFO("PickupSystem: Heart collected but no callback set - healAmount=" + std::to_string(healAmount));
                }
            }

            // Deactivate and hide immediately
            p->isActive = false;
            s->visible = false;

            // Performance: Commented out expensive debug logging
            // GN_LOG_DEBUG(std::string("PickupSystem: deactivated id=") + std::to_string(e) +
            //              " active=" + (p->isActive ? "true" : "false") +
            //              " visible=" + (s->visible ? "true" : "false"));

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
            // GN_LOG_DEBUG(std::string("PickupSystem: removed from active list id=") + std::to_string(e));

            // Note: do not increment i; a new element may have been swapped into index i
        }
    }

    // NEW ORCHESTRATOR PATTERN: Public method that returns created entities
    std::vector<Gnosis::Entity> PickupSystem::SpawnCoinsForGroup(int groupId, GroupPattern pattern, float gapWidth) {
        if (!m_ecsSystem || !m_levelManager || !m_levelConfig) return {};
        if (!m_levelManager->GetObstacleSystem()->IsGroupReadyForCoins(groupId)) return {};

        auto positions = m_levelManager->GetObstacleSystem()->CalculateCoinPositionsForGroup(groupId, pattern, gapWidth);

        std::vector<Gnosis::Entity> coins;

        auto choosePickupType = [this]() -> PickupType {
            if (!m_levelConfig || m_levelConfig->pickupRatios.empty()) {
                return PickupType::GoldCoin;
            }
            // NOTE: Config still uses strings, need to convert
            float total = 0.0f;
            for (const auto& r : m_levelConfig->pickupRatios) total += (r.weight > 0.0f ? r.weight : 0.0f);
            if (total <= 0.0f) return PickupType::GoldCoin;
            float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float target = roll * total;
            float accum = 0.0f;
            for (const auto& r : m_levelConfig->pickupRatios) {
                float w = (r.weight > 0.0f ? r.weight : 0.0f);
                accum += w;
                if (target <= accum) {
                    // Convert string to enum
                    if (r.pickupType == "GoldCoin") return PickupType::GoldCoin;
                    if (r.pickupType == "BlueCoin") return PickupType::BlueCoin;
                    if (r.pickupType == "RedCoin") return PickupType::RedCoin;
                    if (r.pickupType == "Heart" || r.pickupType == "PooHeart") return PickupType::Heart;
                    if (r.pickupType == "HeartBig" || r.pickupType == "PooHeartBig") return PickupType::HeartBig;
                    return PickupType::GoldCoin;
                }
            }
            return PickupType::GoldCoin;
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

            const PickupType type = choosePickupType();

            Sprite sprite;
            if (IsPickupCoinType(type)) {
                sprite = Sprite(GetTextureForPickup(type), 16.0f, 16.0f, 16, 16, 10, 0.1f);
                sprite.isAnimated = true;
                sprite.playing = true;
                sprite.loop = true;
            } else {
                // Hearts are 32x32 static sprites
                sprite = Sprite(GetTextureForPickup(type), 32.0f, 32.0f);
                sprite.isAnimated = false;
                sprite.playing = false;
                sprite.loop = false;
            }
            sprite.color = Gnosis::GNColor(255, 255, 255, 255);
            sprite.visible = true;
            sprite.layer = 5; // Layer 5 = PickupsAndEffects (above decorations)
            m_ecsSystem->AddComponent<Sprite>(e, sprite);

            Pickup pickup(type, 0);  // Use enum constructor
            if (type == PickupType::GoldCoin) pickup.value = 1;
            else if (type == PickupType::BlueCoin) pickup.value = 2;
            else if (type == PickupType::RedCoin) pickup.value = 5;
            else pickup.value = 1;
            pickup.isActive = true;
            // Hearts bob slightly; coins rely on spin animation only
            if (!IsPickupCoinType(type)) {
                pickup.bobbingSpeed = 1.5f;
                pickup.bobbingAmplitude = 6.0f;
            }
            m_ecsSystem->AddComponent<Pickup>(e, pickup);

            ScrollSpeed scroll(m_levelConfig->worldSpeed);
            m_ecsSystem->AddComponent<ScrollSpeed>(e, scroll);

            Hitbox hb;
            hb.type = ColliderType::Rectangle;
            // Tighter hitboxes: coins 16x16, hearts 20x20
            hb.width = IsPickupCoinType(type) ? 16.0f : 20.0f;
            hb.height = IsPickupCoinType(type) ? 16.0f : 20.0f;
            hb.offsetX = 0.0f;
            hb.offsetY = 0.0f;
            m_ecsSystem->AddComponent<Hitbox>(e, hb);

            // Type-specific visual centering
            if (!IsPickupCoinType(type)) {
                Transform* tt = m_ecsSystem->GetComponent<Transform>(e);
                if (tt) {
                    float extraHalf = (32.0f - 16.0f) * tt->scale.x * 0.5f;
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
        return coins;  // NEW: Return entities for manifest tracking
    }

    // LEGACY VERSION DELETED - Use public SpawnCoinsForGroup() with enum-based types

    // LEGACY VERSION DELETED - Coin repositioning now handled by LevelManager::UpdateGroupMemberPositions()

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

void PickupSystem::applyMagnetEffects(float deltaTime) {
    if (!m_ecsSystem || m_playerEntity == 0) {
        return;
    }

    auto* playerTransform = m_ecsSystem->GetComponent<Gnosis::Transform>(m_playerEntity);
    auto* playerHitbox = m_ecsSystem->GetComponent<Hitbox>(m_playerEntity);
    auto* playerSprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
    if (!playerTransform || !playerHitbox || !playerSprite) {
        return;
    }

    // Calculate player's actual center position (same as collision detection)
    float pHalfW = playerSprite->width * playerTransform->scale.x * 0.5f;
    float pHalfH = playerSprite->height * playerTransform->scale.y * 0.5f;
    float playerCenterX = playerTransform->position.x + pHalfW + (playerHitbox->offsetX * playerTransform->scale.x);
    float playerCenterY = playerTransform->position.y + pHalfH + (playerHitbox->offsetY * playerTransform->scale.y);

    // Performance: Commented out per-frame debug logging
    // GN_LOG_DEBUG("PickupSystem::applyMagnetEffects - coinMagnet=" + std::to_string(m_coinMagnetEnabled) +
    //              " heartMagnet=" + std::to_string(m_heartMagnetEnabled));

    for (Gnosis::Entity e : m_activePickups) {
        auto* pickupComp = m_ecsSystem->GetComponent<Pickup>(e);
        auto* transform = m_ecsSystem->GetComponent<Gnosis::Transform>(e);

        if (!pickupComp || !transform || !pickupComp->isActive) {
            continue;
        }

        bool isCoin = IsPickupCoinType(pickupComp->type);
        bool isHeart = IsPickupHeartType(pickupComp->type);

        // Check if magnet is enabled for this pickup type
        bool magnetEnabled = (isCoin && m_coinMagnetEnabled) || (isHeart && m_heartMagnetEnabled);
        if (!magnetEnabled) {
            continue;
        }

        // Calculate distance to player's center position
        float dx = playerCenterX - transform->position.x;
        float dy = playerCenterY - transform->position.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Check if within magnet range
        float range = isCoin ? COIN_MAGNET_RANGE : HEART_MAGNET_RANGE;
        if (distance <= range && distance > 1.0f) { // Very small minimum distance
            // Calculate direction from pickup to player (normalized)
            float dirX = dx / distance;
            float dirY = dy / distance;

            // Calculate pull strength - stronger when closer, but more consistent
            float distanceRatio = distance / range; // 0 = very close, 1 = at max range
            float strengthMultiplier = 1.0f + (1.0f - distanceRatio) * 2.0f; // 1x to 3x multiplier

            // Apply consistent magnet movement
            float magnetStrength = MAGNET_SPEED * deltaTime * strengthMultiplier;

            // Move pickup toward player
            transform->position.x += dirX * magnetStrength;
            transform->position.y += dirY * magnetStrength;

            // Debug logging (only for gold coins to avoid spam)
            if (isCoin && pickupComp->type == PickupType::GoldCoin) {
                // Performance: Commented out expensive per-pickup debug logging
                // GN_LOG_DEBUG("Coin Magnet: GoldCoin distance=" + std::to_string(distance) +
                //              " multiplier=" + std::to_string(strengthMultiplier) +
                //              " move=(" + std::to_string(dirX * magnetStrength) + ", " +
                //              std::to_string(dirY * magnetStrength) + ")");
            }
        }
    }
}

} // namespace GameCore


