#include "ProjectileSystem.h"
#include "../Config/ProjectileSpriteConfig.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Configuration/ConfigManager.h"
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

        // Only log when we have active projectiles (avoid spam)
        if (m_playerProjectiles.activeProjectiles.size() > 0 || m_enemyProjectiles.activeProjectiles.size() > 0) {
            GN_LOG_INFO("ProjectileSystem::Update - Active player: " + std::to_string(m_playerProjectiles.activeProjectiles.size()) +
                        ", Active enemy: " + std::to_string(m_enemyProjectiles.activeProjectiles.size()));
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

            Sprite sprite("", 16.0f, 16.0f); // Empty texture - will be configured on spawn
            sprite.color = GNColor(255, 255, 255, 0); // Invisible initially
            sprite.visible = false; // Invisible until spawned
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

            Sprite sprite("", 16.0f, 16.0f); // Empty texture - will be configured on spawn
            sprite.color = GNColor(255, 255, 255, 0); // Invisible initially
            sprite.visible = false; // Invisible until spawned
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
            // Direction is already scaled to proper velocity in EnemySystem::SpawnEnemyProjectile
            // DO NOT multiply again - it's already a velocity vector, not a unit direction!
            physics->velocity = direction; // Already scaled (e.g., 400.0f)
            physics->useGravity = (projectileType == ProjectileType::SNOWBALL); // Snowballs fall
        }

        // CRITICAL: ScrollSpeed(0.0f) prevents CameraSystem from scrolling this entity
        // This allows Physics velocity to control movement instead
        ScrollSpeed scrollSpeedComponent(0.0f);
        m_ecsSystem->AddComponent(projectile, scrollSpeedComponent);
        
        GN_LOG_INFO("[PROJECTILE] Enemy projectile velocity=(" + 
                   std::to_string(physics->velocity.x) + ", " + std::to_string(physics->velocity.y) + 
                   "), ScrollSpeed=0 (physics-controlled, no camera scroll)");

        Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(projectile);
        if (projectileData) {
            projectileData->isActive = true;
            projectileData->projectileType = projectileType;
            projectileData->spawnPosition = position;
            projectileData->currentLifetime = 0.0f;
            // CRITICAL: Snowballs don't expire by time - only removed when off-screen or hitting player
            // Other projectiles can still have time limits
            projectileData->lifetime = (projectileType == ProjectileType::SNOWBALL) ? 999999.0f : 5.0f;
            projectileData->damage = damage;
            projectileData->isEnemyProjectile = true;
            projectileData->affectedByGravity = (projectileType == ProjectileType::SNOWBALL);
            
            GN_LOG_INFO("[PROJECTILE_SPAWN] Type=" + std::to_string(static_cast<int>(projectileType)) + 
                       ", lifetime=" + std::to_string(projectileData->lifetime) + "s" +
                       " (snowballs: infinite, removed only by off-screen or collision)");
            // Variable gravity based on speed tier (set by EnemySystem in velocity)
            // Will be overridden by actual tier gravity in the velocity components
            projectileData->gravity = projectileData->affectedByGravity ? 350.0f : 0.0f;
            projectileData->postApexGravity = projectileData->affectedByGravity ? 700.0f : 0.0f; // 2x heavier after apex!
            projectileData->hasPassedApex = false;
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

        // Projectile off-screen detection - generous buffers to prevent premature removal
        const float LEFT_BUFFER = 300.0f;     // Large buffer on left for snowballs going left
        const float RIGHT_BUFFER = 500.0f;    // Even larger buffer on right for ongoing flight
        const float VERTICAL_BUFFER = 300.0f; // Large vertical buffer for high arcs
        
        // Use actual screen dimensions from ConfigManager
        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float SCREEN_WIDTH = screenInfo.pixelWidth;
        const float SCREEN_HEIGHT = screenInfo.pixelHeight;

        // Check each boundary separately for detailed logging
        bool leftOff = transform->position.x < -LEFT_BUFFER;
        bool rightOff = transform->position.x > SCREEN_WIDTH + RIGHT_BUFFER;
        bool topOff = transform->position.y < -VERTICAL_BUFFER;
        bool bottomOff = transform->position.y > SCREEN_HEIGHT + VERTICAL_BUFFER;
        
        if (leftOff || rightOff || topOff || bottomOff) {
            GN_LOG_INFO("[OFF_SCREEN_CHECK] pos=(" + std::to_string(transform->position.x) + "," + 
                       std::to_string(transform->position.y) + "), screen=[" + 
                       std::to_string(-LEFT_BUFFER) + " to " + std::to_string(SCREEN_WIDTH + RIGHT_BUFFER) + ", " +
                       std::to_string(-VERTICAL_BUFFER) + " to " + std::to_string(SCREEN_HEIGHT + VERTICAL_BUFFER) + "], " +
                       "leftOff=" + std::to_string(leftOff) + 
                       ", rightOff=" + std::to_string(rightOff) + 
                       ", topOff=" + std::to_string(topOff) + 
                       ", bottomOff=" + std::to_string(bottomOff));
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

            // Update position based on velocity (for player projectiles)
            transform->position.x += physics->velocity.x * deltaTime;
            transform->position.y += physics->velocity.y * deltaTime;

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

            // Update position based on velocity
            transform->position.x += physics->velocity.x * deltaTime;
            transform->position.y += physics->velocity.y * deltaTime;

            // Apply gravity for affected projectiles (+Y = DOWN, so gravity is positive)
            if (projectileData->affectedByGravity) {
                // Check if snowball has passed apex (velocity changes from negative to positive)
                // Apex = when vertical velocity crosses zero from going up to going down
                if (!projectileData->hasPassedApex && physics->velocity.y >= 0.0f) {
                    projectileData->hasPassedApex = true;
                    GN_LOG_INFO("[SNOWBALL APEX] Entity=" + std::to_string(projectile) + 
                               " passed apex at Y=" + std::to_string(transform->position.y) +
                               ", switching to heavier gravity (" + 
                               std::to_string(projectileData->postApexGravity) + ")");
                }
                
                // Use heavier gravity after passing apex for faster fall
                float currentGravity = projectileData->hasPassedApex ? 
                                       projectileData->postApexGravity : 
                                       projectileData->gravity;
                physics->velocity.y += currentGravity * deltaTime;
            }
            
            // COMPREHENSIVE SNOWBALL TRACKING
            if (projectileData->projectileType == ProjectileType::SNOWBALL) {
                // Get screen bounds for comparison
                const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
                bool onScreen = (transform->position.x >= -300.0f && 
                                transform->position.x <= screenInfo.pixelWidth + 500.0f &&
                                transform->position.y >= -300.0f && 
                                transform->position.y <= screenInfo.pixelHeight + 300.0f);
                
                GN_LOG_INFO("[SNOWBALL " + std::to_string(projectile) + "] lifetime=" + 
                           std::to_string(projectileData->currentLifetime) + "s" +
                           ", pos=(" + std::to_string(transform->position.x) + ", " + 
                           std::to_string(transform->position.y) + ")" +
                           ", vel=(" + std::to_string(physics->velocity.x) + ", " + 
                           std::to_string(physics->velocity.y) + ")" +
                           ", onScreen=" + std::to_string(onScreen) + 
                           " [screen: 0-" + std::to_string(screenInfo.pixelWidth) + 
                           ", 0-" + std::to_string(screenInfo.pixelHeight) + "]");
            }

            // Check if expired or off-screen
            bool isExpired = (projectileData->currentLifetime >= projectileData->lifetime);
            bool isOffScreen = IsProjectileOffScreen(transform, physics);
            
            if (isExpired || isOffScreen) {
                GN_LOG_INFO("[SNOWBALL REMOVED] Entity=" + std::to_string(projectile) + 
                           ", expired=" + std::to_string(isExpired) + 
                           " (lifetime=" + std::to_string(projectileData->currentLifetime) + 
                           "/" + std::to_string(projectileData->lifetime) + ")" +
                           ", offScreen=" + std::to_string(isOffScreen) + 
                           ", pos=(" + std::to_string(transform->position.x) + "," + 
                           std::to_string(transform->position.y) + ")");
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
        // Snowballs get 9x scale (was 8x), other projectiles keep 8x
        float scale = (projectileType == ProjectileType::SNOWBALL) ? 9.0f : 8.0f;
        sprite->width = static_cast<float>(config.frameWidth) * scale;
        sprite->height = static_cast<float>(config.frameHeight) * scale;
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
        sprite->visible = true; // Make visible when spawned
        sprite->color.a = 255; // Full opacity

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
