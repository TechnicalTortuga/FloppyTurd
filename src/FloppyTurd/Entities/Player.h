#ifndef FLOPPY_TURD_PLAYER_H
#define FLOPPY_TURD_PLAYER_H

#include "../../Engine/Core/Entity.h"
#include "../../Engine/Core/GnosisTypes.h"
#include "../../Engine/Core/ECS.h"
#include "../Components/GameComponents.h"
#include <vector>
#include <map>

namespace FloppyTurd {

    /**
     * @brief Player entity for Floppy Turd game
     * 
     * This class represents the player character (the turd) in Floppy Turd.
     * It handles player-specific logic like jumping, shooting, and collision responses.
     */
    class Player {
    public:
        Player();
        ~Player();

        // Entity management
        void Initialize(Gnosis::ECS* ecsSystem);
        void Shutdown();
        
        // Player actions
        void Jump(float force = 300.0f);
        void Shoot();
        void TakeDamage(int damage = 1);
        void Heal(int amount = 1);
        void CollectCoin(int value = 1);
        
        // Player state
        bool IsAlive() const { return m_health > 0; }
        bool IsInvulnerable() const { return m_invulnerabilityTimer > 0.0f; }
        int GetHealth() const { return m_health; }
        int GetMaxHealth() const { return m_maxHealth; }
        int GetScore() const { return m_score; }
        int GetCoins() const { return m_coins; }
        
        // Equipment and skills
        void EquipHat(FloppyTurd::HatType hat);
        void UnequipHat();
        FloppyTurd::HatType GetEquippedHat() const { return m_equippedHat; }
        
        void ActivateSkill(FloppyTurd::SkillType skill);
        bool IsSkillActive(FloppyTurd::SkillType skill) const;
        float GetSkillCooldown(FloppyTurd::SkillType skill) const;
        
        // Position and movement
        Gnosis::GNVector2 GetPosition() const;
        void SetPosition(const Gnosis::GNVector2& position);
        Gnosis::GNVector2 GetVelocity() const;
        void SetVelocity(const Gnosis::GNVector2& velocity);
        
        // Update
        void Update(float deltaTime);
        
        // Entity access
        Gnosis::Entity GetEntity() const { return m_entity; }
        
    private:
        // Core entity
        Gnosis::Entity m_entity;
        Gnosis::ECS* m_ecsSystem;
        
        // Player stats
        int m_health;
        int m_maxHealth;
        int m_score;
        int m_coins;
        
        // State timers
        float m_invulnerabilityTimer;
        float m_shootCooldown;
        
        // Equipment
        FloppyTurd::HatType m_equippedHat;
        
        // Active skills and their timers
        struct SkillState {
            bool active;
            float duration;
            float cooldown;
        };
        std::map<FloppyTurd::SkillType, SkillState> m_skills;
        
        // Player configuration
        static const float JUMP_FORCE;
        static const float GRAVITY;
        static const float MAX_FALL_SPEED;
        static const float INVULNERABILITY_DURATION;
        static const float SHOOT_COOLDOWN_DURATION;
        
        // Helper methods
        void UpdateTimers(float deltaTime);
        void UpdateSkills(float deltaTime);
        void ApplyGravity(float deltaTime);
        void ClampVelocity();
        
        // Skill implementations
        void ActivateDoubleJump();
        void ActivateShield();
        void ActivateSpeedBoost();
        void ActivateRapidFire();
        
        void DeactivateSkill(FloppyTurd::SkillType skill);
    };
    
    // Player constants
    const float Player::JUMP_FORCE = 300.0f;
    const float Player::GRAVITY = 980.0f;
    const float Player::MAX_FALL_SPEED = 500.0f;
    const float Player::INVULNERABILITY_DURATION = 2.0f;
    const float Player::SHOOT_COOLDOWN_DURATION = 0.3f;

} // namespace FloppyTurd

#endif // FLOPPY_TURD_PLAYER_H