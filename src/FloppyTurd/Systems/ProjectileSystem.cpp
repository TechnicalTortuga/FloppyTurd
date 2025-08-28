#include "ProjectileSystem.h"
#include "../Config/ProjectileSpriteConfig.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    ProjectileSystem::ProjectileSystem(Gnosis::ECS* ecsSystem)
        : m_ecsSystem(ecsSystem)
    {
        GN_LOG_INFO("ProjectileSystem initialized");
    }

    ProjectileSystem::~ProjectileSystem() {
        GN_LOG_INFO("ProjectileSystem destroyed");
        Cleanup();
    }

    void ProjectileSystem::Initialize() {
        GN_LOG_INFO("Initializing ProjectileSystem...");
        InitializePools();
        GN_LOG_INFO("ProjectileSystem initialization complete");
    }

    void ProjectileSystem::Update(float deltaTime) {
        if (!m_ecsSystem) {
            return;
        }

        UpdateActiveProjectiles(deltaTime);
    }

    void ProjectileSystem::Cleanup() {
        GN_LOG_INFO("Cleaning up ProjectileSystem...");

        // Destroy all projectiles in pools
        for (auto projectile : m_playerProjectiles.allProjectiles) {
            if (projectile != 0) {
                m_ecsSystem->DestroyEntity(projectile);
            }
        }

        for (auto projectile : m_enemyProjectiles.allProjectiles) {
            if (projectile != 0) {
                m_ecsSystem->DestroyEntity(projectile);
            }
        }

        // Clear pool vectors
        m_playerProjectiles.Clear();
        m_enemyProjectiles.Clear();

        GN_LOG_INFO("ProjectileSystem cleanup complete");
    }

    void ProjectileSystem::ResetForNewGame() {
        GN_LOG_INFO("ProjectileSystem reset for new game");

        // Return all active projectiles to pool (don't destroy entities)
        for (auto projectile : m_playerProjectiles.activeProjectiles) {
            ReturnProjectileToPool(projectile);
        }
        m_playerProjectiles.activeProjectiles.clear();

        for (auto projectile : m_enemyProjectiles.activeProjectiles) {
            ReturnProjectileToPool(projectile);
        }
        m_enemyProjectiles.activeProjectiles.clear();

        GN_LOG_INFO("ProjectileSystem reset complete");
    }

    void ProjectileSystem::InitializePools() {
        GN_LOG_INFO("Initializing projectile pools...");

        // Create player projectile pool (8 projectiles)
        for (int i = 0; i < MAX_PLAYER_PROJECTILES; ++i) {
            Entity projectile = m_ecsSystem->CreateEntity();

            // Initialize with all required components but mark inactive
            Transform transform(Gnosis::GNVector2(-1000.0f, -1000.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(projectile, transform);

            Physics physics;
            physics.velocity = Gnosis::GNVector2(0.0f, 0.0f);
            physics.useGravity = false;
            m_ecsSystem->AddComponent<Physics>(projectile, physics);

            Sprite sprite("projectile", 16.0f, 16.0f);
            sprite.color = GNColor(255, 255, 255, 0); // Invisible initially
            m_ecsSystem->AddComponent<Sprite>(projectile, sprite);

            Hitbox hitbox;
            hitbox.type = ColliderType::Circle;
            hitbox.radius = 8.0f;
            hitbox.tag = "projectile";
            m_ecsSystem->AddComponent<Hitbox>(projectile, hitbox);

            Projectile projectileData;
            projectileData.isActive = false;
            projectileData.projectileType = Gnosis::ProjectileType::POOP_BALL;
            m_ecsSystem->AddComponent<Projectile>(projectile, projectileData);

            m_playerProjectiles.allProjectiles.push_back(projectile);
            m_playerProjectiles.inactiveProjectiles.push_back(projectile);

            GN_LOG_INFO("Created player projectile entity: %d", projectile);
        }

        // Create enemy projectile pool (16 projectiles)
        for (int i = 0; i < MAX_ENEMY_PROJECTILES; ++i) {
            Entity projectile = m_ecsSystem->CreateEntity();

            // Initialize with all required components but mark inactive
            Transform transform(Gnosis::GNVector2(-1000.0f, -1000.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(projectile, transform);

            Physics physics;
            physics.velocity = Gnosis::GNVector2(0.0f, 0.0f);
            physics.useGravity = false;
            m_ecsSystem->AddComponent<Physics>(projectile, physics);

            Sprite sprite("projectile", 16.0f, 16.0f);
            sprite.color = GNColor(255, 255, 255, 0); // Invisible initially
            m_ecsSystem->AddComponent<Sprite>(projectile, sprite);

            Hitbox hitbox;
            hitbox.type = ColliderType::Circle;
            hitbox.radius = 8.0f;
            hitbox.tag = "enemy_projectile";
            m_ecsSystem->AddComponent<Hitbox>(projectile, hitbox);

            Projectile projectileData;
            projectileData.isActive = false;
            projectileData.isEnemyProjectile = true;
            projectileData.projectileType = ProjectileType::SNOWBALL;
            m_ecsSystem->AddComponent<Projectile>(projectile, projectileData);

            m_enemyProjectiles.allProjectiles.push_back(projectile);
            m_enemyProjectiles.inactiveProjectiles.push_back(projectile);

            GN_LOG_INFO("Created enemy projectile entity: %d", projectile);
        }

        GN_LOG_INFO("Projectile pools initialized - Player: %d, Enemy: %d",
                   MAX_PLAYER_PROJECTILES, MAX_ENEMY_PROJECTILES);
    }

    Entity ProjectileSystem::SpawnPlayerProjectile(const GNVector2& position,
                                                const GNVector2& direction,
                                                ProjectileType projectileType) {
        GN_LOG_INFO("Attempting to spawn player projectile at (" + std::to_string(position.x) + "," + std::to_string(position.y) + ")");

        Entity projectile = GetInactiveProjectile(true);
        if (projectile == 0) {
            GN_LOG_WARN("No inactive player projectiles available");
            return 0;
        }

        // Configure projectile
        Transform* transform = m_ecsSystem->GetComponent<Transform>(projectile);
        if (transform) {
            transform->position = position;
        }

        Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
        if (physics) {
            physics->velocity = direction * 1500.0f; // Much faster to ensure right movement
            physics->useGravity = false;
        }

        // Add ScrollSpeed component to make projectile move right relative to world
        // World scrolls left at ~200.0f, we want projectile to move right at 1500.0f relative to world
        // Formula: ScrollSpeed = -(worldSpeed + desiredRelativeSpeed)
        // So: ScrollSpeed = -(200.0f + 1500.0f) = -1700.0f
        ScrollSpeed scrollSpeedComponent(-1700.0f); // Move right relative to world
        m_ecsSystem->AddComponent(projectile, scrollSpeedComponent);

        Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(projectile);
        if (projectileData) {
            projectileData->isActive = true;
            projectileData->projectileType = projectileType;
            projectileData->spawnPosition = position;
            projectileData->currentLifetime = 0.0f;
            projectileData->lifetime = 3.0f;
            projectileData->damage = 1;
            projectileData->isEnemyProjectile = false;
        }

        // Debug: Log final projectile setup
        GN_LOG_INFO("Projectile created: id=" + std::to_string(projectile) +
                   ", pos=(" + std::to_string(position.x) + "," + std::to_string(position.y) + ")" +
                   ", vel=(" + std::to_string(physics->velocity.x) + "," + std::to_string(physics->velocity.y) + ")" +
                   ", scrollSpeed=-1700.0f");

        // Configure sprite
        ConfigureProjectileSprite(projectile, projectileType);

        // Move from inactive to active pool
        m_playerProjectiles.inactiveProjectiles.erase(
            std::remove(m_playerProjectiles.inactiveProjectiles.begin(),
                       m_playerProjectiles.inactiveProjectiles.end(), projectile),
            m_playerProjectiles.inactiveProjectiles.end()
        );
        m_playerProjectiles.activeProjectiles.push_back(projectile);

        GN_LOG_INFO("Spawned player projectile: %d at position (%.1f, %.1f)",
                   projectile, position.x, position.y);

        return projectile;
    }

    Entity ProjectileSystem::SpawnEnemyProjectile(const GNVector2& position,
                                               const GNVector2& direction,
                                               ProjectileType projectileType,
                                               int damage) {
        Entity projectile = GetInactiveProjectile(false);
        if (projectile == 0) {
            GN_LOG_WARN("No inactive enemy projectiles available");
            return 0;
        }

        // Configure projectile
        Transform* transform = m_ecsSystem->GetComponent<Transform>(projectile);
        if (transform) {
            transform->position = position;
        }

        Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
        if (physics) {
            physics->velocity = direction * 200.0f; // Enemy projectiles slightly slower
            physics->useGravity = (projectileType == ProjectileType::SNOWBALL); // Snowballs fall
        }

        Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(projectile);
        if (projectileData) {
            projectileData->isActive = true;
            projectileData->projectileType = projectileType;
            projectileData->spawnPosition = position;
            projectileData->currentLifetime = 0.0f;
            projectileData->lifetime = 5.0f; // Enemy projectiles live longer
            projectileData->damage = damage;
            projectileData->isEnemyProjectile = true;
            projectileData->affectedByGravity = (projectileType == ProjectileType::SNOWBALL);
            projectileData->gravity = projectileData->affectedByGravity ? 200.0f : 0.0f;
        }

        // Configure sprite
        ConfigureProjectileSprite(projectile, projectileType);

        // Move from inactive to active pool
        m_enemyProjectiles.inactiveProjectiles.erase(
            std::remove(m_enemyProjectiles.inactiveProjectiles.begin(),
                       m_enemyProjectiles.inactiveProjectiles.end(), projectile),
            m_enemyProjectiles.inactiveProjectiles.end()
        );
        m_enemyProjectiles.activeProjectiles.push_back(projectile);

        GN_LOG_INFO("Spawned enemy projectile: %d at position (%.1f, %.1f)",
                   projectile, position.x, position.y);

        return projectile;
    }

    Entity ProjectileSystem::GetInactiveProjectile(bool isPlayerProjectile) {
        auto& pool = isPlayerProjectile ? m_playerProjectiles : m_enemyProjectiles;

        if (pool.inactiveProjectiles.empty()) {
            return 0; // No inactive projectiles available
        }

        Gnosis::Entity projectile = pool.inactiveProjectiles.back();
        pool.inactiveProjectiles.pop_back();
        return projectile;
    }

    void ProjectileSystem::ReturnProjectileToPool(Entity projectile) {
        if (!m_ecsSystem) return;

        Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(projectile);
        if (!projectileData) return;

        // Mark as inactive
        projectileData->isActive = false;

        // Move to off-screen position
        Transform* transform = m_ecsSystem->GetComponent<Transform>(projectile);
        if (transform) {
            transform->position = Gnosis::GNVector2(-1000.0f, -1000.0f);
        }

        // Make invisible
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(projectile);
        if (sprite) {
            sprite->color = GNColor(255, 255, 255, 255); // Keep visible for debugging
        }

        // Move from active to inactive pool
        bool isPlayerProjectile = !projectileData->isEnemyProjectile;
        auto& pool = isPlayerProjectile ? m_playerProjectiles : m_enemyProjectiles;

        // Note: We don't erase from activeProjectiles here because the calling function
        // is already handling the iterator-based removal to avoid invalidation
        pool.inactiveProjectiles.push_back(projectile);

        GN_LOG_INFO("Returned projectile " + std::to_string(projectile) + " to " +
                   (isPlayerProjectile ? "player" : "enemy") + " pool");
    }

    bool ProjectileSystem::IsProjectileOffScreen(const Transform* transform, const Physics* physics) {
        if (!transform) return false;

        // Simple off-screen detection - can be enhanced with camera bounds
        const float OFFSCREEN_BUFFER = 200.0f; // Larger buffer for projectiles
        const float SCREEN_WIDTH = 1170.0f;    // iPhone 16 width approximation
        const float SCREEN_HEIGHT = 2532.0f;   // iPhone 16 height approximation

        // Special handling for projectiles - allow them to go further right before cleanup
        if (transform->position.x < -OFFSCREEN_BUFFER ||
            transform->position.y < -OFFSCREEN_BUFFER ||
            transform->position.y > SCREEN_HEIGHT + OFFSCREEN_BUFFER) {
            return true;
        }

        // Only remove projectiles that have gone too far right (way off screen)
        if (transform->position.x > SCREEN_WIDTH + OFFSCREEN_BUFFER * 3) {
            return true;
        }

        return false;
    }

    void ProjectileSystem::UpdateActiveProjectiles(float deltaTime) {
        // Update player projectiles
        for (auto it = m_playerProjectiles.activeProjectiles.begin();
             it != m_playerProjectiles.activeProjectiles.end(); ) {
            Gnosis::Entity projectile = *it;

            Transform* transform = m_ecsSystem->GetComponent<Transform>(projectile);
            Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
            Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(projectile);

            if (!transform || !physics || !projectileData) {
                ++it;
                continue;
            }

            // Update lifetime
            projectileData->currentLifetime += deltaTime;

            // Debug: Log projectile position occasionally (every 10 frames to avoid spam)
            static int frameCounter = 0;
            frameCounter++;
            if (frameCounter % 10 == 0 && physics && transform) {
                GN_LOG_INFO("Projectile update: id=" + std::to_string(projectile) +
                           ", pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ")" +
                           ", lifetime=" + std::to_string(projectileData->currentLifetime) + "/" + std::to_string(projectileData->lifetime));
            }

            // Check if expired or off-screen
            bool isExpired = projectileData->currentLifetime >= projectileData->lifetime;
            bool isOffScreen = IsProjectileOffScreen(transform, physics);
            if (isExpired || isOffScreen) {
                GN_LOG_INFO("Removing projectile: expired=" + std::to_string(isExpired) + ", offscreen=" + std::to_string(isOffScreen) +
                           ", pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ")");
                ReturnProjectileToPool(projectile);
                it = m_playerProjectiles.activeProjectiles.erase(it);
            } else {
                ++it;
            }
        }

        // Update enemy projectiles
        for (auto it = m_enemyProjectiles.activeProjectiles.begin();
             it != m_enemyProjectiles.activeProjectiles.end(); ) {
            Gnosis::Entity projectile = *it;

            Transform* transform = m_ecsSystem->GetComponent<Transform>(projectile);
            Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
            Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(projectile);

            if (!transform || !physics || !projectileData) {
                ++it;
                continue;
            }

            // Update lifetime
            projectileData->currentLifetime += deltaTime;

            // Apply gravity for affected projectiles
            if (projectileData->affectedByGravity) {
                physics->velocity.y += projectileData->gravity * deltaTime;
            }

            // Check if expired or off-screen
            if (projectileData->currentLifetime >= projectileData->lifetime ||
                IsProjectileOffScreen(transform, physics)) {
                ReturnProjectileToPool(projectile);
                it = m_enemyProjectiles.activeProjectiles.erase(it);
            } else {
                ++it;
            }
        }
    }



    void ProjectileSystem::ConfigureProjectileSprite(Entity projectile, ProjectileType projectileType) {
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(projectile);
        if (!sprite) return;

        auto& configSystem = ProjectileSpriteConfig::GetInstance();
        const auto& config = configSystem.GetSpriteSpec(projectileType);

        // Configure sprite properties
        sprite->textureId = config.assetName;
        sprite->width = static_cast<float>(config.frameWidth) * 8.0f;  // 8x scale
        sprite->height = static_cast<float>(config.frameHeight) * 8.0f; // 8x scale
        sprite->isAnimated = (config.frameCount > 1);
        sprite->frameWidth = config.frameWidth;
        sprite->frameHeight = config.frameHeight;
        sprite->frameCount = config.frameCount;
        sprite->currentFrame = 0;
        sprite->frameTime = config.animationSpeed;
        sprite->playing = true;
        sprite->loop = true;
        sprite->color = config.color;
        sprite->layer = config.layer;

        GN_LOG_INFO("Configured sprite for projectile type " + std::to_string(static_cast<int>(projectileType)) +
                   ": " + config.assetName + " (" + std::to_string(config.frameWidth) + "x" +
                   std::to_string(config.frameHeight) + ", " + std::to_string(config.frameCount) + " frames)");
    }



    int ProjectileSystem::GetActivePlayerProjectileCount() const {
        return static_cast<int>(m_playerProjectiles.activeProjectiles.size());
    }

    int ProjectileSystem::GetActiveEnemyProjectileCount() const {
        return static_cast<int>(m_enemyProjectiles.activeProjectiles.size());
    }

    int ProjectileSystem::GetTotalPoolSize() const {
        return MAX_PLAYER_PROJECTILES + MAX_ENEMY_PROJECTILES;
    }

} // namespace GameCore
