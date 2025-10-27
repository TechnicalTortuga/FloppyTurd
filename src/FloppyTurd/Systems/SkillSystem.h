#ifndef FLOPPY_TURD_SKILL_SYSTEM_H
#define FLOPPY_TURD_SKILL_SYSTEM_H

#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <vector>
#include <string>
#include <map>

namespace GameCore {

    // Forward declaration for PickupSystem
    class PickupSystem;

    /**
     * SkillDefinition - Defines a skill's properties
     */
    struct SkillDefinition {
        SkillType type;
        std::string name;
        std::string description;
        int coinCost;
        std::vector<SkillType> prerequisites; // Skills required before unlocking this one
        bool isUnlocked;
        bool isActive; // Whether the skill effect is currently active

        SkillDefinition(SkillType t, const std::string& n, const std::string& desc, int cost,
                       const std::vector<SkillType>& prereqs = {})
            : type(t), name(n), description(desc), coinCost(cost),
              prerequisites(prereqs), isUnlocked(false), isActive(false) {}
    };

    /**
     * SkillSystem - Manages skill unlocking, activation, and effects
     * Integrates with PlayerControllerSystem and PickupSystem to apply skill effects
     */
    class SkillSystem {
    public:
        SkillSystem(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates = nullptr);
        ~SkillSystem();

        // Integration with other systems
        void SetPickupSystem(PickupSystem* pickupSystem) { m_pickupSystem = pickupSystem; }
        void SetPlatformDelegates(PlatformDelegates* platformDelegates) { m_platformDelegates = platformDelegates; }

        // Skill management
        void InitializeSkills();
        bool UnlockSkill(SkillType skill, int& playerCoins);
        bool IsSkillUnlocked(SkillType skill) const;
        bool IsSkillActive(SkillType skill) const;
        bool CanUnlockSkill(SkillType skill, int playerCoins) const;
        int GetSkillCost(SkillType skill) const;
        const SkillDefinition* GetSkillDefinition(SkillType skill) const;

        // Skill effects
        void ActivateSkill(SkillType skill);
        void DeactivateSkill(SkillType skill);
        void UpdateSkillEffects(float deltaTime, Gnosis::Entity playerEntity);

        // Heart system integration
        void ApplyHeartModeUpgrade(SkillType skill, Gnosis::Entity playerEntity);
        void SetHeartMode(Gnosis::Entity playerEntity, HeartMode mode);



        // Safety net
        bool TryActivateCoinSafetyNet(Gnosis::Entity playerEntity);

        // Reset
        void ResetForNewLevel();

        // Save/Load
        void SaveSkillProgress();
        void LoadSkillProgress();

        // UI helpers
        std::vector<SkillType> GetAvailableSkills() const;
        std::string GetSkillDisplayName(SkillType skill) const;
        std::string GetSkillDescription(SkillType skill) const;

    private:
        Gnosis::ECS* m_ecsSystem;
        PickupSystem* m_pickupSystem;
        PlatformDelegates* m_platformDelegates;

        // Skill definitions and state
        std::map<SkillType, SkillDefinition> m_skills;

        // Safety net state
        bool m_coinSafetyNetUsedThisLevel;

        // Helper methods
        void InitializeSkillDefinitions();
        bool HasPrerequisites(SkillType skill) const;
        void ApplyPassiveSkillEffects(Gnosis::Entity playerEntity);
    };

} // namespace GameCore

#endif // FLOPPY_TURD_SKILL_SYSTEM_H
