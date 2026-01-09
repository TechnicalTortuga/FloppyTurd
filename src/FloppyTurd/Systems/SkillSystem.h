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
     * SkillDefinition - Defines a skill's properties with rank support
     */
    struct SkillDefinition {
        SkillType type;
        std::string name;
        std::string description;
        int maxRank;                        // Maximum rank for this skill (1 = non-ranked, 2+ = ranked)
        std::vector<int> rankCosts;         // Cost for each rank (index 0 = rank 1 cost)
        int currentRank;                    // Current rank (0 = not unlocked)
        bool isActive;                      // Whether the skill effect is currently active

        // Constructor for ranked skills
        SkillDefinition(SkillType t, const std::string& n, const std::string& desc, 
                       int maxR, const std::vector<int>& costs)
            : type(t), name(n), description(desc), maxRank(maxR),
              rankCosts(costs), currentRank(0), isActive(false) {}
        
        // Convenience: check if skill is unlocked (any rank > 0)
        bool IsUnlocked() const { return currentRank > 0; }
        
        // Get cost for next rank upgrade (0 if max rank reached)
        int GetNextRankCost() const {
            if (currentRank >= maxRank) return 0;
            return rankCosts[currentRank]; // currentRank is 0-indexed for next rank
        }
        
        // Get display string for current rank (e.g., "I", "II", "III")
        std::string GetRankDisplay() const {
            static const char* numerals[] = {"", "I", "II", "III"};
            if (currentRank <= 0 || currentRank > 3) return "";
            return numerals[currentRank];
        }
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

        // Skill management (rank-aware)
        void InitializeSkills();
        bool UpgradeSkill(SkillType skill, int& playerCoins);  // Upgrades to next rank
        bool IsSkillUnlocked(SkillType skill) const;           // Returns true if rank > 0
        int GetSkillRank(SkillType skill) const;               // Returns current rank (0 = not unlocked)
        int GetSkillMaxRank(SkillType skill) const;            // Returns max rank for skill
        bool CanUpgradeSkill(SkillType skill, int playerCoins) const;
        int GetNextUpgradeCost(SkillType skill) const;
        const SkillDefinition* GetSkillDefinition(SkillType skill) const;
        SkillDefinition* GetSkillDefinitionMutable(SkillType skill);

        // Skill effects
        bool IsSkillActive(SkillType skill) const;
        void ActivateSkill(SkillType skill);
        void DeactivateSkill(SkillType skill);
        void UpdateSkillEffects(float deltaTime, Gnosis::Entity playerEntity);

        // Heart system integration
        void ApplyHeartModeUpgrade(Gnosis::Entity playerEntity);  // Based on HealthUpgrade rank
        void SetHeartMode(Gnosis::Entity playerEntity, HeartMode mode);

        // Projectile skill queries
        bool HasHomingProjectiles() const;
        float GetHomingStrength() const;    // Based on HomingProjectiles rank
        float GetHomingAngleCone() const;   // Targeting cone: ±10° (Rank I) or ±20° (Rank II)
        int GetProjectileSplitCount() const; // Based on SplitProjectiles rank (0, 2, or 3)

        // Magnet skill queries
        float GetCoinMagnetMultiplier() const;  // 1.0, 1.0, or 2.0 based on rank
        float GetHeartMagnetMultiplier() const; // 1.0, 1.0, or 2.0 based on rank

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
        void ApplyPassiveSkillEffects(Gnosis::Entity playerEntity);
    };

} // namespace GameCore

#endif // FLOPPY_TURD_SKILL_SYSTEM_H
