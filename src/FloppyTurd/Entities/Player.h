#ifndef FLOPPY_TURD_PLAYER_H
#define FLOPPY_TURD_PLAYER_H

// Use forward declarations and proper includes to avoid circular dependencies
#include "../../Engine/Core/GnosisTypes.h"

#include "../Components/GameComponents.h"
#include <vector>
#include <map>

namespace GameCore {

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
        // REMOVED: CollectCoin - coin accounting centralized in GameplayState
        
        // Player state
        bool IsAlive() const { return m_health > 0; }
        bool IsInvulnerable() const { return m_invulnerabilityTimer > 0.0f; }
        int GetMaxHealth() const { return m_maxHealth; }
        int GetScore() const { return m_score; }
        int GetCoins() const { return m_coins; }
        
        // Equipment and skills
        void EquipHat(HatType hat);
        void UnequipHat();
        HatType GetEquippedHat() const { return m_equippedHat; }

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
        
        float m_invulnerabilityTimer;
        float m_shootCooldown;
        
        // Equipment
        HatType m_equippedHat;

        // Player configuration
        static const float JUMP_FORCE;
        static const float GRAVITY;
        static const float MAX_FALL_SPEED;
        static const float INVULNERABILITY_DURATION;
        static const float SHOOT_COOLDOWN_DURATION;
        
        // Helper methods
        void UpdateTimers(float deltaTime);
        void ApplyGravity(float deltaTime);
        void ClampVelocity();
    };
    
} // namespace GameCore

#endif // FLOPPY_TURD_PLAYER_H