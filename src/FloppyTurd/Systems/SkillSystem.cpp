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
        // Skills from old system - order must match the enum indices in GameComponents.h
        // SkillType enum: HalfHearts=0, ThirdHearts=1, CoinMagnet=2, HeartMagnet=3, CoinSafetyNet=4
        m_skills.emplace(SkillType::HalfHearts, SkillDefinition(
            SkillType::HalfHearts, "Half Hearts", "Show half-heart damage\nfor finer health tracking", 5, {}
        ));

        m_skills.emplace(SkillType::ThirdHearts, SkillDefinition(
            SkillType::ThirdHearts, "Third Hearts", "Show third-heart damage\nRequires: Half Hearts", 5,
            {SkillType::HalfHearts}
        ));

        m_skills.emplace(SkillType::CoinMagnet, SkillDefinition(
            SkillType::CoinMagnet, "Coin Magnet", "Automatically attract\nnearby coins to player", 5, {}
        ));

        m_skills.emplace(SkillType::HeartMagnet, SkillDefinition(
            SkillType::HeartMagnet, "Heart Magnet", "Automatically attract\nnearby hearts to player", 5, {}
        ));

        m_skills.emplace(SkillType::CoinSafetyNet, SkillDefinition(
            SkillType::CoinSafetyNet, "Coin Safety Net", "Sacrifice all coins\nto prevent death\n(once per level)", 5, {}
        ));
    }

    bool SkillSystem::UnlockSkill(SkillType skill, int& playerCoins)
    {
        if (!CanUnlockSkill(skill, playerCoins)) {
            return false;
        }

        auto it = m_skills.find(skill);
        if (it == m_skills.end()) {
            return false;
        }

        // Mark skill as unlocked (coin deduction handled by PauseSystem)
        it->second.isUnlocked = true;

        // Auto-activate passive skills
        if (skill == SkillType::HalfHearts || skill == SkillType::ThirdHearts ||
            skill == SkillType::CoinMagnet || skill == SkillType::HeartMagnet) {
            it->second.isActive = true;
        }

        // Trigger haptic feedback for skill unlock
        if (m_platformDelegates) {
            if (m_platformDelegates->haptic.triggerPattern) {
                m_platformDelegates->haptic.triggerPattern("level_unlock");
            }
        }

        SaveSkillProgress();
        return true;
    }

    bool SkillSystem::IsSkillUnlocked(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() && it->second.isUnlocked;
    }

    bool SkillSystem::IsSkillActive(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() && it->second.isActive;
    }

    bool SkillSystem::CanUnlockSkill(SkillType skill, int playerCoins) const
    {
        auto it = m_skills.find(skill);
        if (it == m_skills.end()) {
            return false;
        }

        if (it->second.isUnlocked) {
            return false; // Already unlocked
        }

        if (playerCoins < it->second.coinCost) {
            return false; // Not enough coins
        }

        return HasPrerequisites(skill);
    }

    int SkillSystem::GetSkillCost(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.coinCost : 0;
    }

    const SkillDefinition* SkillSystem::GetSkillDefinition(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? &it->second : nullptr;
    }

    void SkillSystem::ActivateSkill(SkillType skill)
    {
        auto it = m_skills.find(skill);
        if (it != m_skills.end() && it->second.isUnlocked) {
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

        // Update magnet effects via PickupSystem
        if (m_pickupSystem) {
            bool coinMagnetActive = IsSkillActive(SkillType::CoinMagnet);
            bool heartMagnetActive = IsSkillActive(SkillType::HeartMagnet);

            m_pickupSystem->SetCoinMagnetEnabled(coinMagnetActive);
            m_pickupSystem->SetHeartMagnetEnabled(heartMagnetActive);

            GN_LOG_DEBUG("SkillSystem: Updated magnet effects - coinMagnet=" + std::to_string(coinMagnetActive) +
                         " heartMagnet=" + std::to_string(heartMagnetActive));
        }
    }

    void SkillSystem::ApplyHeartModeUpgrade(SkillType skill, Gnosis::Entity playerEntity)
    {
        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return;
        }

        auto* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(playerEntity);
        if (!playerComp) {
            return;
        }

        HeartMode newMode = HeartMode::WHOLE;

        if (skill == SkillType::HalfHearts) {
            newMode = HeartMode::HALVES;
        } else if (skill == SkillType::ThirdHearts) {
            newMode = HeartMode::THIRDS;
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
        GN_LOG_INFO("Player ghost slices before safety net: " + std::to_string(playerComp->ghostSlices));

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
        m_coinSafetyNetUsedThisLevel = false;
        GN_LOG_INFO("Coin safety net reset for new level");
    }

    void SkillSystem::ApplyPassiveSkillEffects(Gnosis::Entity playerEntity)
    {
        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return;
        }

        // Apply heart mode upgrades
        if (IsSkillActive(SkillType::ThirdHearts)) {
            ApplyHeartModeUpgrade(SkillType::ThirdHearts, playerEntity);
        } else if (IsSkillActive(SkillType::HalfHearts)) {
            ApplyHeartModeUpgrade(SkillType::HalfHearts, playerEntity);
        }
    }

    bool SkillSystem::HasPrerequisites(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        if (it == m_skills.end()) {
            return false;
        }

        for (SkillType prereq : it->second.prerequisites) {
            if (!IsSkillUnlocked(prereq)) {
                return false;
            }
        }

        return true;
    }

    std::vector<SkillType> SkillSystem::GetAvailableSkills() const
    {
        std::vector<SkillType> available;
        for (const auto& pair : m_skills) {
            if (!pair.second.isUnlocked) {
                available.push_back(pair.first);
            }
        }
        return available;
    }

    std::string SkillSystem::GetSkillDisplayName(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.name : "Unknown Skill";
    }

    std::string SkillSystem::GetSkillDescription(SkillType skill) const
    {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.description : "Unknown skill description";
    }

    void SkillSystem::SaveSkillProgress()
    {
        GN_LOG_INFO("SkillSystem: Saving skill progress...");
        
        // Use the game's save system to persist skill unlock status
        auto* game = GameCore::GetGame();
        if (!game) {
            GN_LOG_WARN("SkillSystem: Cannot save - no game instance available");
            return;
        }
        
        // Map SkillType enum to array index and save unlock status
        // Order: HalfHearts=0, ThirdHearts=1, CoinMagnet=2, HeartMagnet=3, CoinSafetyNet=4
        if (IsSkillUnlocked(SkillType::HalfHearts)) {
            game->UnlockSkill(static_cast<int>(SkillType::HalfHearts));
        }
        if (IsSkillUnlocked(SkillType::ThirdHearts)) {
            game->UnlockSkill(static_cast<int>(SkillType::ThirdHearts));
        }
        if (IsSkillUnlocked(SkillType::CoinMagnet)) {
            game->UnlockSkill(static_cast<int>(SkillType::CoinMagnet));
        }
        if (IsSkillUnlocked(SkillType::HeartMagnet)) {
            game->UnlockSkill(static_cast<int>(SkillType::HeartMagnet));
        }
        if (IsSkillUnlocked(SkillType::CoinSafetyNet)) {
            game->UnlockSkill(static_cast<int>(SkillType::CoinSafetyNet));
        }
        
        GN_LOG_INFO("SkillSystem: Skill progress saved successfully");
    }

    void SkillSystem::LoadSkillProgress()
    {
        GN_LOG_INFO("SkillSystem: Loading skill progress...");
        
        // Load skill unlock status from the game's save system
        auto* game = GameCore::GetGame();
        if (!game) {
            GN_LOG_WARN("SkillSystem: Cannot load - no game instance available, using defaults");
            return;
        }
        
        // Load unlock status for each skill and update our internal state
        // Order: HalfHearts=0, ThirdHearts=1, CoinMagnet=2, HeartMagnet=3, CoinSafetyNet=4
        if (game->IsSkillUnlocked(static_cast<int>(SkillType::HalfHearts))) {
            auto it = m_skills.find(SkillType::HalfHearts);
            if (it != m_skills.end()) {
                it->second.isUnlocked = true;
                it->second.isActive = true; // Auto-activate passive skills
                GN_LOG_INFO("SkillSystem: Loaded HalfHearts as unlocked");
            }
        }
        
        if (game->IsSkillUnlocked(static_cast<int>(SkillType::ThirdHearts))) {
            auto it = m_skills.find(SkillType::ThirdHearts);
            if (it != m_skills.end()) {
                it->second.isUnlocked = true;
                it->second.isActive = true; // Auto-activate passive skills
                GN_LOG_INFO("SkillSystem: Loaded ThirdHearts as unlocked");
            }
        }
        
        if (game->IsSkillUnlocked(static_cast<int>(SkillType::CoinMagnet))) {
            auto it = m_skills.find(SkillType::CoinMagnet);
            if (it != m_skills.end()) {
                it->second.isUnlocked = true;
                it->second.isActive = true; // Auto-activate passive skills
                GN_LOG_INFO("SkillSystem: Loaded CoinMagnet as unlocked");
            }
        }
        
        if (game->IsSkillUnlocked(static_cast<int>(SkillType::HeartMagnet))) {
            auto it = m_skills.find(SkillType::HeartMagnet);
            if (it != m_skills.end()) {
                it->second.isUnlocked = true;
                it->second.isActive = true; // Auto-activate passive skills
                GN_LOG_INFO("SkillSystem: Loaded HeartMagnet as unlocked");
            }
        }
        
        if (game->IsSkillUnlocked(static_cast<int>(SkillType::CoinSafetyNet))) {
            auto it = m_skills.find(SkillType::CoinSafetyNet);
            if (it != m_skills.end()) {
                it->second.isUnlocked = true;
                it->second.isActive = true; // Auto-activate passive skills
                GN_LOG_INFO("SkillSystem: Loaded CoinSafetyNet as unlocked");
            }
        }
        
        GN_LOG_INFO("SkillSystem: Skill progress loaded successfully");
    }

} // namespace GameCore
