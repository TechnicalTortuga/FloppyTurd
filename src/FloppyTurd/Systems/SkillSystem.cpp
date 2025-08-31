#include "SkillSystem.h"
#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include <algorithm>
#include <cmath>

namespace GameCore {

    SkillSystem::SkillSystem(Gnosis::ECS* ecsSystem)
        : m_ecsSystem(ecsSystem)
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
        // Skills from old system
        m_skills.emplace(SkillType::HalfHearts, SkillDefinition(
            SkillType::HalfHearts, "Half Hearts", "Show half-heart damage\nfor finer health tracking", 300, {}
        ));

        m_skills.emplace(SkillType::ThirdHearts, SkillDefinition(
            SkillType::ThirdHearts, "Third Hearts", "Show third-heart damage\nRequires: Half Hearts", 500,
            {SkillType::HalfHearts}
        ));

        m_skills.emplace(SkillType::CoinMagnet, SkillDefinition(
            SkillType::CoinMagnet, "Coin Magnet", "Automatically attract\nnearby coins to player", 400, {}
        ));

        m_skills.emplace(SkillType::HeartMagnet, SkillDefinition(
            SkillType::HeartMagnet, "Heart Magnet", "Automatically attract\nnearby hearts to player", 450, {}
        ));

        m_skills.emplace(SkillType::CoinSafetyNet, SkillDefinition(
            SkillType::CoinSafetyNet, "Coin Safety Net", "Sacrifice all coins\nto prevent death\n(once per level)", 600, {}
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

        // Deduct coins
        playerCoins -= it->second.coinCost;
        it->second.isUnlocked = true;

        // Auto-activate passive skills
        if (skill == SkillType::HalfHearts || skill == SkillType::ThirdHearts ||
            skill == SkillType::CoinMagnet || skill == SkillType::HeartMagnet) {
            it->second.isActive = true;
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

        // Update magnet effects
        if (IsSkillActive(SkillType::CoinMagnet)) {
            UpdateCoinMagnet(deltaTime, playerEntity);
        }

        if (IsSkillActive(SkillType::HeartMagnet)) {
            UpdateHeartMagnet(deltaTime, playerEntity);
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

    void SkillSystem::UpdateCoinMagnet(float deltaTime, Gnosis::Entity playerEntity)
    {
        UpdateMagnetEffect(deltaTime, playerEntity, "BlueCoin", COIN_MAGNET_RANGE);
    }

    void SkillSystem::UpdateHeartMagnet(float deltaTime, Gnosis::Entity playerEntity)
    {
        UpdateMagnetEffect(deltaTime, playerEntity, "PoopHeart", HEART_MAGNET_RANGE);
    }

    void SkillSystem::UpdateMagnetEffect(float deltaTime, Gnosis::Entity playerEntity,
                                       const std::string& pickupType, float range)
    {
        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return;
        }

        auto* playerTransform = m_ecsSystem->GetComponent<Gnosis::Transform>(playerEntity);
        if (!playerTransform) {
            return;
        }

        // TODO: Implement magnet effect using proper ECS query methods
        // The current ECS system doesn't support template-based component queries
        // This would need to be implemented when the ECS system provides proper query methods

        // For now, we'll skip the magnet implementation to avoid compilation errors
        // The magnet skills will be functional once the ECS query system is available
    }

    bool SkillSystem::TryActivateCoinSafetyNet(Gnosis::Entity playerEntity)
    {
        if (!IsSkillActive(SkillType::CoinSafetyNet) || m_coinSafetyNetUsedThisLevel) {
            return false;
        }

        if (!m_ecsSystem->IsEntityValid(playerEntity)) {
            return false;
        }

        auto* playerComp = m_ecsSystem->GetComponent<PlayerComponent>(playerEntity);
        if (!playerComp) {
            return false;
        }

        // Check if player has any coins to sacrifice
        if (playerComp->sessionCoins <= 0) {
            return false;
        }

        // Sacrifice all session coins and restore 1 heart slice
        playerComp->sessionCoins = 0;
        playerComp->liveSlices = std::min(playerComp->liveSlices + 1,
                                         playerComp->hearts * static_cast<int>(playerComp->heartMode));

        m_coinSafetyNetUsedThisLevel = true;
        return true;
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
        // TODO: Implement save functionality using the game's save system
        // This would typically save to a file or player preferences
    }

    void SkillSystem::LoadSkillProgress()
    {
        // TODO: Implement load functionality
        // For now, skills start locked
    }

} // namespace GameCore
