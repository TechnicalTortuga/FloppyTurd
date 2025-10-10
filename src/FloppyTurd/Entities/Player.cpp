#include "Player.h"
#include "../Components/GameComponents.h"

namespace GameCore {

    const float Player::JUMP_FORCE = 300.0f;
    const float Player::GRAVITY = 980.0f;
    const float Player::MAX_FALL_SPEED = 500.0f;
    const float Player::INVULNERABILITY_DURATION = 2.0f;
    const float Player::SHOOT_COOLDOWN_DURATION = 0.3f;

    Player::Player() 
        : m_entity(Gnosis::INVALID_ENTITY)
        , m_ecsSystem(nullptr)
        , m_health(3)
        , m_maxHealth(3)
        , m_score(0)
        , m_coins(0)
        , m_invulnerabilityTimer(0.0f)
        , m_shootCooldown(0.0f)
        , m_equippedHat(HatType::None)
    {}

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
        hitbox.radius = 14.0f; // Reduced by 2px from legacy radius of 16.0f
        hitbox.isTrigger = false;
        m_ecsSystem->AddComponent<FloppyTurd::Hitbox>(m_entity, hitbox);
        
        // Add DebugDraw component to show player hitbox (disabled for now)
        // FloppyTurd::DebugDraw debugDraw;
        // debugDraw.showBounds = false;
        // debugDraw.showCollider = true;
        // debugDraw.colliderColor = Gnosis::GNColor(0, 0, 255, 128); // Blue for player
        // debugDraw.alpha = 0.8f;
        // debugDraw.debugLayer = 20;
        // m_ecsSystem->AddComponent<FloppyTurd::DebugDraw>(m_entity, debugDraw);
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

    void Player::EquipHat(HatType hat) {
        m_equippedHat = hat;
        // TODO: Apply hat effects
    }

    void Player::UnequipHat() {
        m_equippedHat = HatType::None;
        // TODO: Remove hat effects
    }

    Gnosis::GNVector2 Player::GetPosition() const {
{{ ... }}
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

} // namespace GameCore