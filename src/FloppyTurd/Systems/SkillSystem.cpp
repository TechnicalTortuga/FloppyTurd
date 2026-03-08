#include "SkillSystem.h"
#include "PickupSystem.h"
#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Platform/HapticHelpers.h"
#include "../Game/FloppyTurdGame.h"
#include <algorithm>
#include <cmath>

namespace GameCore {

    SkillSystem::SkillSystem(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates)
        : m_ecsSystem(ecsSystem)
        , m_pickupSystem(nullptr)
        , m_platformDelegates(platformDelegates)
        , m_coinSafetyNetUsedThisLevel(false)
    {
        InitializeSkillDefinitions();
        LoadSkillProgress();
    }

    SkillSystem::~SkillSystem()
    {
        SaveSkillProgress();
    }

    void SkillSystem::InitializeSkills()
    {
        // Skills are already initialized in constructor via InitializeSkillDefinitions
        // This method is for external initialization if needed
    }

    void SkillSystem::InitializeSkillDefinitions()
    {
        // Ranked skills with costs (doubled from original except split)
        // Format: SkillDefinition(type, name, description, maxRank, {rank1Cost, rank2Cost, ...})
        
        // Health Upgrade: Rank I = half hearts (200), Rank II = third hearts (500)
        m_skills.emplace(SkillType::HealthUpgrade, SkillDefinition(
            SkillType::HealthUpgrade, "Health Upgrade", 
            "Upgrade heart precision\nRank I: Half hearts\nRank II: Third hearts",
            2, {200, 500}
        ));

        // Coin Magnet: Rank I = current range (100), Rank II = 2x range (300)
        m_skills.emplace(SkillType::CoinMagnet, SkillDefinition(
            SkillType::CoinMagnet, "Coin Magnet", 
            "Attract nearby coins\nRank II: Double range",
            2, {100, 300}
        ));

        // Heart Magnet: Rank I = current range (150), Rank II = 2x range (400)
        m_skills.emplace(SkillType::HeartMagnet, SkillDefinition(
            SkillType::HeartMagnet, "Heart Magnet", 
            "Attract nearby hearts\nRank II: Double range",
            2, {150, 400}
        ));

        // Homing Projectiles: Rank I = weak homing (300), Rank II = strong homing (700)
        m_skills.emplace(SkillType::HomingProjectiles, SkillDefinition(
            SkillType::HomingProjectiles, "Homing Shots", 
            "Projectiles seek enemies\nRank II: Stronger homing",
            2, {300, 700}
        ));

        // Split Projectiles: Rank I = 2 shots (200), Rank II = 3 shots (400)
        m_skills.emplace(SkillType::SplitProjectiles, SkillDefinition(
            SkillType::SplitProjectiles, "Split Shot", 
            "Fire multiple projectiles\nRank I: 2 shots\nRank II: 3 shots",
            2, {200, 400}
        ));

        // Non-ranked skill: Coin Safety Net (single unlock at 600)
        m_skills.emplace(SkillType::CoinSafetyNet, SkillDefinition(
            SkillType::CoinSafetyNet, "Coin Safety Net", 
            "Sacrifice all coins to\nprevent death (1x/level)",
            1, {600}
        ));
    }

    bool SkillSystem::UpgradeSkill(SkillType skill, int& playerCoins)
    {
        if (!CanUpgradeSkill(skill, playerCoins)) {
            return false;
        }

        auto it = m_skills.find(skill);
        if (it == m_skills.end()) {
            return false;
        }

        int cost = it->second.GetNextRankCost();
        if (cost <= 0) {
            return false; // Already at max rank
        }

        // Upgrade rank (coin deduction handled by caller/PauseSystem)
        it->second.currentRank++;
        it->second.isActive = true; // Auto-activate on upgrade

        GN_LOG_INFO("SkillSystem: Upgraded " + it->second.name + " to Rank " + 
                    it->second.GetRankDisplay());

        // Trigger haptic feedback for skill unlock
        if (m_platformDelegates && m_platformDelegates->haptic.triggerPattern) {
            m_platformDelegates->haptic.triggerPattern("level_unlock");
        }

        SaveSkillProgress();
        return true;
    }

    bool SkillSystem::IsSkillUnlocked(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() && it->second.IsUnlocked();
    }

    int SkillSystem::GetSkillRank(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.currentRank : 0;
    }

    int SkillSystem::GetSkillMaxRank(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.maxRank : 0;
    }

    bool SkillSystem::CanUpgradeSkill(SkillType skill, int playerCoins) const
    {
        auto it = m_skills.find(skill);
        if (it == m_skills.end()) {
            return false;
        }

        int cost = it->second.GetNextRankCost();
        if (cost <= 0) {
            return false; // Already at max rank
        }

        return playerCoins >= cost;
    }

    int SkillSystem::GetNextUpgradeCost(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.GetNextRankCost() : 0;
    }

    const SkillDefinition* SkillSystem::GetSkillDefinition(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? &it->second : nullptr;
    }

    SkillDefinition* SkillSystem::GetSkillDefinitionMutable(SkillType skill)
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? &it->second : nullptr;
    }

    bool SkillSystem::IsSkillActive(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() && it->second.isActive;
    }

    void SkillSystem::ActivateSkill(SkillType skill)
    {
        auto it = m_skills.find(skill);
        if (it != m_skills.end() && it->second.IsUnlocked()) {
            it->second.isActive = true;
        }
    }

    void SkillSystem::DeactivateSkill(SkillType skill)
    {
        auto it = m_skills.find(skill);
        if (it != m_skills.end()) {
            it->second.isActive = false;
        }
    }

    void SkillSystem::UpdateSkillEffects(float deltaTime, Gnosis::Entity playerEntity)
    {
        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return;
        }

        // Apply passive skill effects
        ApplyPassiveSkillEffects(playerEntity);

        // Update magnet effects via PickupSystem (with multipliers based on rank)
        if (m_pickupSystem) {
            bool coinMagnetActive = IsSkillActive(SkillType::CoinMagnet);
            bool heartMagnetActive = IsSkillActive(SkillType::HeartMagnet);

            m_pickupSystem->SetCoinMagnetEnabled(coinMagnetActive);
            m_pickupSystem->SetHeartMagnetEnabled(heartMagnetActive);
            
            // Set magnet strength multipliers based on rank
            m_pickupSystem->SetCoinMagnetStrength(GetCoinMagnetMultiplier());
            m_pickupSystem->SetHeartMagnetStrength(GetHeartMagnetMultiplier());
        }
    }

    void SkillSystem::ApplyHeartModeUpgrade(Gnosis::Entity playerEntity)
    {
        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return;
        }

        auto* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(playerEntity);
        if (!playerComp) {
            return;
        }

        // Determine heart mode based on HealthUpgrade rank
        HeartMode newMode = HeartMode::WHOLE;
        int healthRank = GetSkillRank(SkillType::HealthUpgrade);
        
        if (healthRank >= 2) {
            newMode = HeartMode::THIRDS;
        } else if (healthRank == 1) {
            newMode = HeartMode::HALVES;
        }

        SetHeartMode(playerEntity, newMode);
    }

    void SkillSystem::SetHeartMode(Gnosis::Entity playerEntity, HeartMode mode)
    {
        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return;
        }

        auto* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(playerEntity);
        if (!playerComp) {
            return;
        }

        if (mode == playerComp->heartMode) {
            return;
        }

        // Convert current health to new mode
        int totalSlices = playerComp->liveSlices + playerComp->ghostSlices;
        int unscaledHearts = totalSlices / static_cast<int>(playerComp->heartMode);

        playerComp->heartMode = mode;
        int newTotalSlices = unscaledHearts * static_cast<int>(mode);

        // Ensure we don't exceed maximum hearts
        int maxSlices = playerComp->hearts * static_cast<int>(mode);
        playerComp->liveSlices = std::min(newTotalSlices, maxSlices);
        playerComp->ghostSlices = 0;
    }

    // Projectile skill queries
    bool SkillSystem::HasHomingProjectiles() const
    {
        return GetSkillRank(SkillType::HomingProjectiles) > 0;
    }

    float SkillSystem::GetHomingStrength() const
    {
        int rank = GetSkillRank(SkillType::HomingProjectiles);
        if (rank <= 0) return 0.0f;
        if (rank == 1) return 6.0f;  // Medium base strength for distance scaling
        return 10.0f;                 // Strong base strength for Rank II (rank 2+)
    }

    float SkillSystem::GetHomingAngleCone() const
    {
        int rank = GetSkillRank(SkillType::HomingProjectiles);
        if (rank <= 0) return 0.0f;
        if (rank == 1) return 20.0f;  // ±20 degrees (40 degree total arc)
        return 40.0f;                  // ±40 degrees (80 degree total arc) for Rank II
    }

    int SkillSystem::GetProjectileSplitCount() const
    {
        int rank = GetSkillRank(SkillType::SplitProjectiles);
        if (rank <= 0) return 1;      // No split, single projectile
        if (rank == 1) return 2;      // Split into 2
        return 3;                      // Split into 3 (rank 2+)
    }

    // Magnet skill queries
    float SkillSystem::GetCoinMagnetMultiplier() const
    {
        int rank = GetSkillRank(SkillType::CoinMagnet);
        if (rank <= 1) return 1.0f;   // Rank 0 or 1 = normal
        return 2.0f;                   // Rank 2+ = double range
    }

    float SkillSystem::GetHeartMagnetMultiplier() const
    {
        int rank = GetSkillRank(SkillType::HeartMagnet);
        if (rank <= 1) return 1.0f;   // Rank 0 or 1 = normal
        return 2.0f;                   // Rank 2+ = double range
    }

    bool SkillSystem::TryActivateCoinSafetyNet(Gnosis::Entity playerEntity)
    {
        GN_LOG_INFO("=== COIN SAFETY NET CHECK ===");
        GN_LOG_INFO("Skill active: " + std::to_string(IsSkillActive(SkillType::CoinSafetyNet)));
        GN_LOG_INFO("Already used this level: " + std::to_string(m_coinSafetyNetUsedThisLevel));

        if (!IsSkillActive(SkillType::CoinSafetyNet) || m_coinSafetyNetUsedThisLevel) {
            GN_LOG_INFO("Coin safety net cannot activate - not active or already used");
            return false;
        }

        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            GN_LOG_INFO("Coin safety net cannot activate - invalid player entity");
            return false;
        }

        auto* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(playerEntity);
        if (!playerComp) {
            GN_LOG_INFO("Coin safety net cannot activate - no player component");
            return false;
        }

        GN_LOG_INFO("Player coins before safety net: " + std::to_string(playerComp->sessionCoins));
        GN_LOG_INFO("Player live slices before safety net: " + std::to_string(playerComp->liveSlices));

        // Check if player has any coins to sacrifice
        if (playerComp->sessionCoins <= 0) {
            GN_LOG_INFO("Coin safety net cannot activate - no coins to sacrifice");
            return false;
        }

        GN_LOG_INFO("=== ACTIVATING COIN SAFETY NET ===");
        // Sacrifice all session coins and restore 1 heart slice
        int coinsBefore = playerComp->sessionCoins;
        playerComp->sessionCoins = 0;
        int liveSlicesBefore = playerComp->liveSlices;
        playerComp->liveSlices = std::min(playerComp->liveSlices + 1,
                                         playerComp->hearts * static_cast<int>(playerComp->heartMode));

        GN_LOG_INFO("Coins spent: " + std::to_string(coinsBefore) + " -> 0");
        GN_LOG_INFO("Live slices restored: " + std::to_string(liveSlicesBefore) + " -> " + std::to_string(playerComp->liveSlices));

        m_coinSafetyNetUsedThisLevel = true;
        GN_LOG_INFO("Coin safety net activated successfully!");
        return true;
    }

    void SkillSystem::ResetForNewLevel()
    {
        // Reset the coin safety net flag so it can be used again
        bool wasUsed = m_coinSafetyNetUsedThisLevel;
        m_coinSafetyNetUsedThisLevel = false;
        GN_LOG_INFO("🔄 Coin safety net reset for new level (was used: " + std::to_string(wasUsed) + ", now available: " + std::to_string(!m_coinSafetyNetUsedThisLevel) + ")");
    }

    void SkillSystem::ApplyPassiveSkillEffects(Gnosis::Entity playerEntity)
    {
        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return;
        }

        // Apply heart mode upgrades based on HealthUpgrade rank
        ApplyHeartModeUpgrade(playerEntity);
    }

    std::vector<SkillType> SkillSystem::GetAvailableSkills() const
    {
        std::vector<SkillType> available;
        for (const auto& pair : m_skills) {
            // Include skills that can be upgraded (not at max rank)
            if (pair.second.currentRank < pair.second.maxRank) {
                available.push_back(pair.first);
            }
        }
        return available;
    }

    std::string SkillSystem::GetSkillDisplayName(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        if (it == m_skills.end()) return "Unknown Skill";
        
        std::string name = it->second.name;
        if (it->second.currentRank > 0 && it->second.maxRank > 1) {
            name += " " + it->second.GetRankDisplay();
        }
        return name;
    }

    std::string SkillSystem::GetSkillDescription(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.description : "Unknown skill description";
    }

    void SkillSystem::SaveSkillProgress()
    {
        GN_LOG_INFO("SkillSystem: Saving skill progress (ranked system)...");
        
        auto* game = GameCore::GetGame();
        if (!game) {
            GN_LOG_WARN("SkillSystem: Cannot save - no game instance available");
            return;
        }
        
        // Save skill ranks (use skill index * 10 + rank as a compact representation)
        // This allows storing rank 0-9 for each skill slot
        for (const auto& pair : m_skills) {
            int skillIndex = static_cast<int>(pair.first);
            int rank = pair.second.currentRank;
            
            // Use the existing skill unlock system with encoded rank
            // Rank is stored as: slot = skillIndex, value = rank
            game->SetSkillRank(skillIndex, rank);
        }
        
        GN_LOG_INFO("SkillSystem: Skill progress saved (ranked system)");
    }

    void SkillSystem::LoadSkillProgress()
    {
        GN_LOG_INFO("SkillSystem: Loading skill progress (ranked system)...");
        
        auto* game = GameCore::GetGame();
        if (!game) {
            GN_LOG_WARN("SkillSystem: Cannot load - no game instance available, using defaults");
            return;
        }
        
        // Load skill ranks
        for (auto& pair : m_skills) {
            int skillIndex = static_cast<int>(pair.first);
            int rank = game->GetSkillRank(skillIndex);
            
            pair.second.currentRank = rank;
            pair.second.isActive = (rank > 0); // Auto-activate if unlocked
            
            if (rank > 0) {
                GN_LOG_INFO("SkillSystem: Loaded " + pair.second.name + " at Rank " + pair.second.GetRankDisplay());
            }
        }
        
        GN_LOG_INFO("SkillSystem: Skill progress loaded (ranked system)");
    }

} // namespace GameCore
