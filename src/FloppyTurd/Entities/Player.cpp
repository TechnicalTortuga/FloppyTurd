#include "Player.h"
#include "../Components/GameComponents.h"

namespace GameCore {

    Player::Player() 
        : m_entity(Gnosis::INVALID_ENTITY)
        , m_ecsSystem(nullptr)
        , m_health(3)
        , m_maxHealth(3)
        , m_score(0)
        , m_coins(0)
        , m_invulnerabilityTimer(0.0f)
        , m_shootCooldown(0.0f)
        , m_equippedHat(GameCore::HatType::None)
    {
        // Initialize skill states
        m_skills[GameCore::SkillType::DoubleJump] = {false, 0.0f, 5.0f};
        m_skills[GameCore::SkillType::Shield] = {false, 0.0f, 10.0f};
        m_skills[GameCore::SkillType::SpeedBoost] = {false, 0.0f, 8.0f};
        m_skills[GameCore::SkillType::RapidFire] = {false, 0.0f, 12.0f};
    }

    Player::~Player() {
        Shutdown();
    }

    void Player::Initialize(Gnosis::ECS* ecsSystem) {
        m_ecsSystem = ecsSystem;
        
        if (!m_ecsSystem) {
            return;
        }
        
        // Create the player entity
        m_entity = m_ecsSystem->CreateEntity();
        
        // Add Transform component
        GameCore::Transform transform;
        transform.position = Gnosis::GNVector2(100.0f, 300.0f);
        transform.scale = Gnosis::GNVector2(1.0f, 1.0f);
        transform.rotation = 0.0f;
        m_ecsSystem->AddComponent<FloppyTurd::Transform>(m_entity, transform);
        
        // Add Physics component
        FloppyTurd::Physics physics;
        physics.velocity = Gnosis::GNVector2(0.0f, 0.0f);
        physics.acceleration = Gnosis::GNVector2(0.0f, GRAVITY);
        physics.mass = 1.0f;
        physics.drag = 0.98f;
        m_ecsSystem->AddComponent<FloppyTurd::Physics>(m_entity, physics);
        
        // Add Sprite component
        FloppyTurd::Sprite sprite;
        sprite.textureId = "player_turd";
        sprite.width = 32.0f;
        sprite.height = 32.0f;
        sprite.color = Gnosis::GNColor(255, 255, 255, 255);
        m_ecsSystem->AddComponent<FloppyTurd::Sprite>(m_entity, sprite);
        
        // Add Hitbox component (circle)
        FloppyTurd::Hitbox hitbox;
        hitbox.type = FloppyTurd::ColliderType::Circle;
        hitbox.radius = 16.0f; // Legacy radius pre-scaling; adjust if needed
        hitbox.isTrigger = false;
        m_ecsSystem->AddComponent<FloppyTurd::Hitbox>(m_entity, hitbox);
    }

    void Player::Shutdown() {
        if (m_ecsSystem && m_entity != Gnosis::INVALID_ENTITY) {
            m_ecsSystem->DestroyEntity(m_entity);
            m_entity = Gnosis::INVALID_ENTITY;
        }
        m_ecsSystem = nullptr;
    }

    void Player::Jump(float force) {
        if (!m_ecsSystem || m_entity == Gnosis::INVALID_ENTITY) {
            return;
        }
        
        auto* physics = m_ecsSystem->GetComponent<FloppyTurd::Physics>(m_entity);
        if (physics) {
            physics->velocity.y = -force; // Negative Y is up
        }
    }

    void Player::Shoot() {
        if (m_shootCooldown > 0.0f) {
            return;
        }
        
        // TODO: Create projectile entity
        // For now, just reset cooldown
        m_shootCooldown = SHOOT_COOLDOWN_DURATION;
    }

    void Player::TakeDamage(int damage) {
        if (IsInvulnerable()) {
            return;
        }
        
        m_health -= damage;
        if (m_health < 0) {
            m_health = 0;
        }
        
        // Start invulnerability period
        m_invulnerabilityTimer = INVULNERABILITY_DURATION;
    }

    void Player::Heal(int amount) {
        m_health += amount;
        if (m_health > m_maxHealth) {
            m_health = m_maxHealth;
        }
    }

    // REMOVED: CollectCoin - coin accounting centralized in GameplayState

    void Player::EquipHat(FloppyTurd::HatType hat) {
        m_equippedHat = hat;
        // TODO: Apply hat effects
    }

    void Player::UnequipHat() {
        m_equippedHat = FloppyTurd::HatType::None;
        // TODO: Remove hat effects
    }

    void Player::ActivateSkill(FloppyTurd::SkillType skill) {
        auto it = m_skills.find(skill);
        if (it == m_skills.end() || it->second.cooldown > 0.0f) {
            return;
        }
        
        switch (skill) {
            case FloppyTurd::SkillType::DoubleJump:
                ActivateDoubleJump();
                break;
            case FloppyTurd::SkillType::Shield:
                ActivateShield();
                break;
            case FloppyTurd::SkillType::SpeedBoost:
                ActivateSpeedBoost();
                break;
            case FloppyTurd::SkillType::RapidFire:
                ActivateRapidFire();
                break;
        }
    }

    bool Player::IsSkillActive(FloppyTurd::SkillType skill) const {
        auto it = m_skills.find(skill);
        return it != m_skills.end() && it->second.active;
    }

    float Player::GetSkillCooldown(FloppyTurd::SkillType skill) const {
        auto it = m_skills.find(skill);
        return it != m_skills.end() ? it->second.cooldown : 0.0f;
    }

    Gnosis::GNVector2 Player::GetPosition() const {
        if (!m_ecsSystem || m_entity == Gnosis::INVALID_ENTITY) {
            return {0.0f, 0.0f};
        }
        
        auto* transform = m_ecsSystem->GetComponent<FloppyTurd::Transform>(m_entity);
        return transform ? transform->position : Gnosis::GNVector2{0.0f, 0.0f};
    }

    void Player::SetPosition(const Gnosis::GNVector2& position) {
        if (!m_ecsSystem || m_entity == Gnosis::INVALID_ENTITY) {
            return;
        }
        
        auto* transform = m_ecsSystem->GetComponent<FloppyTurd::Transform>(m_entity);
        if (transform) {
            transform->position = position;
        }
    }

    Gnosis::GNVector2 Player::GetVelocity() const {
        if (!m_ecsSystem || m_entity == Gnosis::INVALID_ENTITY) {
            return {0.0f, 0.0f};
        }
        
        auto* physics = m_ecsSystem->GetComponent<FloppyTurd::Physics>(m_entity);
        return physics ? physics->velocity : Gnosis::GNVector2{0.0f, 0.0f};
    }

    void Player::SetVelocity(const Gnosis::GNVector2& velocity) {
        if (!m_ecsSystem || m_entity == Gnosis::INVALID_ENTITY) {
            return;
        }
        
        auto* physics = m_ecsSystem->GetComponent<FloppyTurd::Physics>(m_entity);
        if (physics) {
            physics->velocity = velocity;
        }
    }

    void Player::Update(float deltaTime) {
        UpdateTimers(deltaTime);
        UpdateSkills(deltaTime);
        ApplyGravity(deltaTime);
        ClampVelocity();
    }

    void Player::UpdateTimers(float deltaTime) {
        if (m_invulnerabilityTimer > 0.0f) {
            m_invulnerabilityTimer -= deltaTime;
        }
        
        if (m_shootCooldown > 0.0f) {
            m_shootCooldown -= deltaTime;
        }
    }

    void Player::UpdateSkills(float deltaTime) {
        for (auto& [skillType, skillState] : m_skills) {
            if (skillState.active) {
                skillState.duration -= deltaTime;
                if (skillState.duration <= 0.0f) {
                    DeactivateSkill(skillType);
                }
            }
            
            if (skillState.cooldown > 0.0f) {
                skillState.cooldown -= deltaTime;
            }
        }
    }

    void Player::ApplyGravity(float deltaTime) {
        if (!m_ecsSystem || m_entity == Gnosis::INVALID_ENTITY) {
            return;
        }
        
        auto* physics = m_ecsSystem->GetComponent<FloppyTurd::Physics>(m_entity);
        if (physics) {
            physics->velocity.y += GRAVITY * deltaTime;
        }
    }

    void Player::ClampVelocity() {
        if (!m_ecsSystem || m_entity == Gnosis::INVALID_ENTITY) {
            return;
        }
        
        auto* physics = m_ecsSystem->GetComponent<FloppyTurd::Physics>(m_entity);
        if (physics) {
            if (physics->velocity.y > MAX_FALL_SPEED) {
                physics->velocity.y = MAX_FALL_SPEED;
            }
        }
    }

    void Player::ActivateDoubleJump() {
        auto& skill = m_skills[FloppyTurd::SkillType::DoubleJump];
        skill.active = true;
        skill.duration = 5.0f;
        skill.cooldown = 5.0f;
    }

    void Player::ActivateShield() {
        auto& skill = m_skills[FloppyTurd::SkillType::Shield];
        skill.active = true;
        skill.duration = 3.0f;
        skill.cooldown = 10.0f;
        
        // Make player invulnerable
        m_invulnerabilityTimer = skill.duration;
    }

    void Player::ActivateSpeedBoost() {
        auto& skill = m_skills[FloppyTurd::SkillType::SpeedBoost];
        skill.active = true;
        skill.duration = 4.0f;
        skill.cooldown = 8.0f;
        
        // TODO: Increase movement speed
    }

    void Player::ActivateRapidFire() {
        auto& skill = m_skills[FloppyTurd::SkillType::RapidFire];
        skill.active = true;
        skill.duration = 6.0f;
        skill.cooldown = 12.0f;
        
        // TODO: Reduce shoot cooldown
    }

    void Player::DeactivateSkill(FloppyTurd::SkillType skill) {
        auto it = m_skills.find(skill);
        if (it != m_skills.end()) {
            it->second.active = false;
            it->second.duration = 0.0f;
        }
        
        // TODO: Remove skill effects
    }

} // namespace GameCore