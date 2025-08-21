#include "HeartSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>
#include <random>

namespace GameCore {

    HeartSystem::HeartSystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates)
    : m_ecsCoordinator(ecsCoordinator)
    , m_delegates(delegates)
    , m_screenInfoValid(false)
    , m_allocatedHearts(0)
{
    // Initialize heart entities array
    for (int i = 0; i < MAX_HEARTS; ++i) {
        m_heartEntities[i] = Gnosis::INVALID_ENTITY;
    }
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("HeartSystem: ECS coordinator is null");
        }
        
        // Initialize heart texture names
        m_heartTextureNames = {
            "TurdHeartSmall",                    // Full heart
            "TurdHeartHollow",                   // Empty heart
            "TurdHeart1Half",                    // Half heart (1/2)
            "TurdHeart0HalfHollow1Half",         // Empty half + 1 half
            "TurdHeart1Third",                   // One third
            "TurdHeart2Thirds",                  // Two thirds  
            "TurdHeart1ThirdHollow",             // Empty + 1 third
            "TurdHeart2ThirdsHollow2Thirds",     // Empty + 2 thirds
            "TurdHeart0ThirdHollow1Third",       // Empty + 1 third visible
            "TurdHeart1ThirdsHollow1Thirds"      // 1 third + 1 third hollow
        };
        
        UpdateScreenInfo();
        GN_LOG_INFO("HeartSystem: Initialized with heart texture support");
    }

    void HeartSystem::Update(float deltaTime) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // Update screen info if needed
        UpdateScreenInfo();
        
        // Update heart visibility based on current player state
        auto playerEntities = m_ecsCoordinator->GetEntitiesWithComponents<PlayerComponent>();
        if (!playerEntities.empty()) {
            Gnosis::Entity playerEntity = playerEntities[0]; // Assume first player
            UpdateHeartVisibility(playerEntity);
        }
    }



    void HeartSystem::InitializePlayerHearts(Gnosis::Entity playerEntity, GameCore::Difficulty difficulty) {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        if (!player) {
            GN_LOG_ERROR("HeartSystem: Player component not found");
            return;
        }
        
        player->difficulty = difficulty;
        player->hearts = GetHeartsForDifficulty(difficulty);
        player->heartMode = HeartMode::WHOLE;
        player->liveSlices = player->hearts * static_cast<int>(player->heartMode);
        player->maxHealth = player->liveSlices;
        player->health = player->liveSlices;
        player->ghostSlices = 0;
        player->hollowTurds = false;
        
        GN_LOG_INFO("HeartSystem: Initialized player with " + std::to_string(player->hearts) + 
                   " hearts (" + std::to_string(player->liveSlices) + " slices) for difficulty " + 
                   std::to_string(static_cast<int>(difficulty)));
    }

    void HeartSystem::SetHeartMode(Gnosis::Entity playerEntity, HeartMode mode) {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        if (!player) {
            GN_LOG_ERROR("HeartSystem: Player component not found");
            return;
        }
        
        // Calculate current health percentage to preserve when changing modes
        float healthPercentage = static_cast<float>(player->liveSlices) / 
                                static_cast<float>(player->hearts * static_cast<int>(player->heartMode));
        
        player->heartMode = mode;
        int newMaxSlices = player->hearts * static_cast<int>(mode);
        player->liveSlices = std::max(1, static_cast<int>(healthPercentage * newMaxSlices));
        player->maxHealth = newMaxSlices;
        player->health = player->liveSlices;
        
        GN_LOG_INFO("HeartSystem: Set heart mode to " + std::to_string(static_cast<int>(mode)) + 
                   " (" + std::to_string(player->liveSlices) + "/" + std::to_string(newMaxSlices) + " slices)");
    }

    void HeartSystem::AddHeartSlices(Gnosis::Entity playerEntity, int slices) {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        if (!player) {
            GN_LOG_ERROR("HeartSystem: Player component not found");
            return;
        }
        
        int maxSlices = player->hearts * static_cast<int>(player->heartMode);
        player->liveSlices = std::min(player->liveSlices + slices, maxSlices);
        player->health = player->liveSlices;
        
        GN_LOG_INFO("HeartSystem: Added " + std::to_string(slices) + " heart slices. " +
                   "Now: " + std::to_string(player->liveSlices) + "/" + std::to_string(maxSlices));
    }

    void HeartSystem::RemoveHeartSlices(Gnosis::Entity playerEntity, int slices) {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        if (!player) {
            GN_LOG_ERROR("HeartSystem: Player component not found");
            return;
        }
        
        // Use ghost slices first if hollow turds is enabled
        if (player->hollowTurds && player->ghostSlices > 0) {
            int ghostSlicesToUse = std::min(slices, player->ghostSlices);
            player->ghostSlices -= ghostSlicesToUse;
            slices -= ghostSlicesToUse;
            
            GN_LOG_INFO("HeartSystem: Used " + std::to_string(ghostSlicesToUse) + " ghost slices. " +
                       "Ghost slices remaining: " + std::to_string(player->ghostSlices));
        }
        
        // Apply remaining damage to live slices
        if (slices > 0) {
            player->liveSlices = std::max(0, player->liveSlices - slices);
            player->health = player->liveSlices;
            
            GN_LOG_INFO("HeartSystem: Removed " + std::to_string(slices) + " live heart slices. " +
                       "Now: " + std::to_string(player->liveSlices));
        }
    }

    int HeartSystem::GetCurrentSlices(Gnosis::Entity playerEntity) const {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        return player ? player->liveSlices : 0;
    }

    int HeartSystem::GetMaxSlices(Gnosis::Entity playerEntity) const {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        return player ? (player->hearts * static_cast<int>(player->heartMode)) : 0;
    }

    bool HeartSystem::IsPlayerDead(Gnosis::Entity playerEntity) const {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        return player ? (player->liveSlices <= 0) : true;
    }

    Gnosis::Entity HeartSystem::CreateHeartUI(float x, float y) {
        GN_LOG_INFO("HeartSystem::CreateHeartUI: Pre-allocating " + std::to_string(MAX_HEARTS) + " hearts at (" + 
                    std::to_string(x) + ", " + std::to_string(y) + ")");

        // Pre-allocate all heart entities but start them as hidden
        CreateAllHeartEntities(x, y);

        // Create a container entity to track the heart UI system
        Gnosis::Entity containerEntity = m_ecsCoordinator->CreateEntity();
        if (containerEntity == Gnosis::INVALID_ENTITY) {
            GN_LOG_ERROR("HeartSystem::CreateHeartUI: Failed to create container entity");
            return Gnosis::INVALID_ENTITY;
        }

        // Add HeartUI component to container for positioning data
        HeartUI heartUIData;
        heartUIData.x = x;
        heartUIData.y = y;
        heartUIData.visible = true;
        m_ecsCoordinator->AddComponent(containerEntity, heartUIData);

        GN_LOG_INFO("HeartSystem::CreateHeartUI: Created container entity " + std::to_string(containerEntity) + 
                    " with " + std::to_string(m_allocatedHearts) + " pre-allocated hearts");
        return containerEntity;
    }

    void HeartSystem::UpdateHeartUIPositioning(Gnosis::Entity heartUIEntity, float coinBagX, float menuButtonY) {
        HeartUI* heartUI = m_ecsCoordinator->GetComponent<HeartUI>(heartUIEntity);
        if (!heartUI) {
            GN_LOG_ERROR("HeartSystem: HeartUI component not found");
            return;
        }
        
        // Position hearts at same X as coin bag, starting from menu button Y level
        heartUI->x = coinBagX;
        heartUI->y = menuButtonY;
        
        GN_LOG_INFO("HeartSystem: Updated heart UI position to (" + std::to_string(coinBagX) + 
                   ", " + std::to_string(menuButtonY) + ")");
    }

    void HeartSystem::UpdateHeartDisplay(Gnosis::Entity playerEntity) {
        // Use the new UpdateHeartVisibility method
        UpdateHeartVisibility(playerEntity);
    }

    int HeartSystem::GetHeartsForDifficulty(GameCore::Difficulty difficulty) const {
        switch (difficulty) {
            case GameCore::Difficulty::Runny:   return 3;  // Easy - 3 hearts
            case GameCore::Difficulty::Regular: return 2;  // Normal - 2 hearts
            case GameCore::Difficulty::Rough:   return 1;  // Hard - 1 heart
            default:                            return 2;  // Default to normal
        }
    }

    std::string HeartSystem::GetHeartTexture(HeartMode mode, int heartIndex, int totalHearts,
                                           int currentSlices, int maxSlices) const {
        // Calculate slices per heart
        int slicesPerHeart = static_cast<int>(mode);
        
        // Calculate which slice range this heart represents
        int heartStartSlice = heartIndex * slicesPerHeart;
        int heartEndSlice = heartStartSlice + slicesPerHeart;
        
        // Determine how many slices are filled in this heart
        int filledSlices = 0;
        if (currentSlices > heartStartSlice) {
            filledSlices = std::min(slicesPerHeart, currentSlices - heartStartSlice);
        }
        
        // Return appropriate texture based on mode and filled slices
        switch (mode) {
            case HeartMode::WHOLE:
                return (filledSlices > 0) ? "TurdHeartSmall" : "TurdHeartHollow";
                
            case HeartMode::HALVES:
                if (filledSlices == 2) return "TurdHeartSmall";          // Full heart
                if (filledSlices == 1) return "TurdHeart1Half";          // Half heart
                return "TurdHeartHollow";                                // Empty heart
                
            case HeartMode::THIRDS:
                if (filledSlices == 3) return "TurdHeartSmall";          // Full heart
                if (filledSlices == 2) return "TurdHeart2Thirds";        // Two thirds
                if (filledSlices == 1) return "TurdHeart1Third";         // One third
                return "TurdHeartHollow";                                // Empty heart
                
            default:
                return "TurdHeartSmall";  // Fallback
        }
    }



    void HeartSystem::UpdateScreenInfo() {
        if (m_delegates.renderer.getScreenInfo) {
            m_delegates.renderer.getScreenInfo(&m_screenInfo);
            m_screenInfoValid = true;
        } else {
            m_screenInfoValid = false;
        }
    }

    void HeartSystem::CreateAllHeartEntities(float x, float y) {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("HeartSystem::CreateAllHeartEntities: ECS coordinator is null");
            return;
        }

        // Calculate heart positioning
        const float heartScale = 8.0f; // Scale to match other UI elements
        const float heartTextureHeight = 24.0f; // TurdHeartSmall is 24x24 pixels
        const float scaledHeartHeight = heartTextureHeight * heartScale;
        const float pixelGap = 4.0f * heartScale; // 4px gap scaled to match hearts
        const float heartSpacing = scaledHeartHeight + pixelGap;

        // Pre-allocate all MAX_HEARTS entities
        for (int i = 0; i < MAX_HEARTS; ++i) {
            Gnosis::Entity heartEntity = m_ecsCoordinator->CreateEntity();
            if (heartEntity == Gnosis::INVALID_ENTITY) {
                GN_LOG_ERROR("HeartSystem::CreateAllHeartEntities: Failed to create heart entity " + std::to_string(i));
                continue;
            }

            // Position each heart vertically
            float heartY = y + (i * heartSpacing);

            // Add Transform component
            Transform heartTransform(Gnosis::GNVector2(x, heartY), 0.0f, Gnosis::GNVector2(heartScale, heartScale));
            m_ecsCoordinator->AddComponent<Transform>(heartEntity, heartTransform);

            // Add UIElement component (start with empty heart and hidden)
            UIElement heartUI;
            heartUI.normalTextureId = "TurdHeartHollow"; // Start with empty hearts
            heartUI.visible = false; // Start hidden - will be shown based on player health
            heartUI.isEnabled = true;
            heartUI.textLayer = 10; // High UI layer for visibility
            m_ecsCoordinator->AddComponent<UIElement>(heartEntity, heartUI);

            // Store in our pre-allocated array
            m_heartEntities[i] = heartEntity;
            m_allocatedHearts++;

            GN_LOG_INFO("HeartSystem::CreateAllHeartEntities: Created heart entity " + std::to_string(heartEntity) + 
                       " at index " + std::to_string(i) + " at (" + std::to_string(x) + ", " + std::to_string(heartY) + ")");
        }

        GN_LOG_INFO("HeartSystem::CreateAllHeartEntities: Pre-allocated " + std::to_string(m_allocatedHearts) + " heart entities");
    }

    void HeartSystem::UpdateHeartVisibility(Gnosis::Entity playerEntity) {
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        if (!player) {
            GN_LOG_ERROR("HeartSystem::UpdateHeartVisibility: Player component not found");
            return;
        }

        // Hide all hearts if player is dead (no live slices)
        if (player->liveSlices <= 0) {
            for (int i = 0; i < MAX_HEARTS; ++i) {
                if (m_heartEntities[i] == Gnosis::INVALID_ENTITY) {
                    continue;
                }

                UIElement* heartUI = m_ecsCoordinator->GetComponent<UIElement>(m_heartEntities[i]);
                if (heartUI) {
                    heartUI->visible = false;
                }
            }
            GN_LOG_INFO("HeartSystem::UpdateHeartVisibility: Player is dead - hiding all hearts");
            return;
        }

        // Show hearts based on player's current heart count
        for (int i = 0; i < MAX_HEARTS; ++i) {
            if (m_heartEntities[i] == Gnosis::INVALID_ENTITY) {
                continue;
            }

            UIElement* heartUI = m_ecsCoordinator->GetComponent<UIElement>(m_heartEntities[i]);
            if (!heartUI) {
                continue;
            }

            // Show hearts up to player's current heart count
            if (i < player->hearts) {
                heartUI->visible = true;

                // Determine texture based on current health
                std::string textureName = GetHeartTexture(player->heartMode, i, 
                                                        player->hearts, player->liveSlices, 
                                                        player->hearts * static_cast<int>(player->heartMode));
                heartUI->normalTextureId = textureName;
            } else {
                // Hide hearts beyond player's current count
                heartUI->visible = false;
            }
        }

        GN_LOG_INFO("HeartSystem::UpdateHeartVisibility: Updated visibility for " + std::to_string(player->hearts) + 
                   " hearts with " + std::to_string(player->liveSlices) + " live slices");
    }

    void HeartSystem::DestroyAllHeartUI() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        GN_LOG_INFO("HeartSystem::DestroyAllHeartUI: Destroying " + std::to_string(m_allocatedHearts) + " heart entities");
        
        // Destroy all allocated heart entities
        for (int i = 0; i < m_allocatedHearts; ++i) {
            if (m_heartEntities[i] != Gnosis::INVALID_ENTITY) {
                m_ecsCoordinator->DestroyEntity(m_heartEntities[i]);
                m_heartEntities[i] = Gnosis::INVALID_ENTITY;
                GN_LOG_INFO("HeartSystem: Destroyed heart entity at index " + std::to_string(i));
            }
        }
        
        m_allocatedHearts = 0;
        GN_LOG_INFO("HeartSystem: All heart UI entities destroyed and cleaned up");
    }

    void HeartSystem::UpdateHeartCountForDifficulty(Gnosis::Entity playerEntity, GameCore::Difficulty difficulty) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        PlayerComponent* player = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
        if (!player) {
            GN_LOG_ERROR("HeartSystem: Player component not found for heart count update");
            return;
        }
        
        // Calculate new heart count based on difficulty
        int newHeartCount = GetHeartsForDifficulty(difficulty);
        
        // If heart count is the same, no need to update
        if (player->hearts == newHeartCount) {
            return;
        }
        
        GN_LOG_INFO("HeartSystem: Updating heart count from " + std::to_string(player->hearts) + 
                   " to " + std::to_string(newHeartCount) + " for difficulty change");
        
        // Update player heart count and related stats
        player->hearts = newHeartCount;
        player->maxHealth = newHeartCount;
        player->health = newHeartCount; // Reset to full health
        player->liveSlices = newHeartCount * static_cast<int>(player->heartMode);
        player->difficulty = difficulty;
        
        // Update heart visibility without dynamic allocation
        UpdateHeartVisibility(playerEntity);
        
        GN_LOG_INFO("HeartSystem: Successfully updated hearts for difficulty change without reallocation");
    }



} // namespace GameCore
