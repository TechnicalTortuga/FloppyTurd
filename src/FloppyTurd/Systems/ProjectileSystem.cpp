#include "ProjectileSystem.h"
#include "SkillSystem.h"
#include "LevelManager.h"
#include "../Config/ProjectileSpriteConfig.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include <algorithm>
#include <cmath>

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
            hitbox.radius = 28.0f;  // Substantially larger hitbox (was 18.0f)
            hitbox.offsetX = 16.0f;  // Center on 32x32 sprite
            hitbox.offsetY = 16.0f;  // Center on 32x32 sprite
            hitbox.tag = "projectile";
            m_ecsSystem->AddComponent<Hitbox>(projectile, hitbox);

            // DebugDraw DISABLED per user request
            // DebugDraw debugDraw;
            // debugDraw.debugLayer = 100;
            // debugDraw.showBounds = false;
            // debugDraw.showCollider = true;
            // debugDraw.colliderColor = {255, 255, 0, 255}; // Yellow for player projectiles
            // debugDraw.alpha = 0.5f;
            // m_ecsSystem->AddComponent<DebugDraw>(projectile, debugDraw);

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
            hitbox.radius = 14.0f;  // 14 * scale from center for better hit detection
            hitbox.offsetX = 16.0f;  // Center on 32x32 sprite (will be reconfigured based on actual sprite)
            hitbox.offsetY = 16.0f;  // Center on 32x32 sprite
            hitbox.tag = "enemy_projectile";
            m_ecsSystem->AddComponent<Hitbox>(projectile, hitbox);

            // DebugDraw DISABLED per user request
            // DebugDraw debugDrawEnemy;
            // debugDrawEnemy.debugLayer = 100;
            // debugDrawEnemy.showBounds = false;
            // debugDrawEnemy.showCollider = true;
            // debugDrawEnemy.colliderColor = {255, 165, 0, 255}; // Orange for enemy projectiles
            // debugDrawEnemy.alpha = 0.5f;
            // m_ecsSystem->AddComponent<DebugDraw>(projectile, debugDrawEnemy);

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
            transform->scale = Gnosis::GNVector2(1.0f, 1.0f); // Keep sprite at normal size
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

            // Set up homing if skill is active
            if (m_skillSystem && m_skillSystem->HasHomingProjectiles()) {
                projectileData->isHoming = true;
                projectileData->homingStrength = m_skillSystem->GetHomingStrength();
                projectileData->hasAcquiredTarget = false;
                projectileData->targetEnemy = 0;
                
                // Find nearest on-screen enemy and ALWAYS aim toward it (clamped to angle cone)
                float maxAngleDegrees = m_skillSystem->GetHomingAngleCone(); // ±20° rank 1, ±40° rank 2
                Entity nearestEnemy = 0;
                float nearestDistance = std::numeric_limits<float>::max();
                
                if (m_levelManager) {
                    const auto& activeEnemies = m_levelManager->GetActiveEnemies();
                    for (Entity enemy : activeEnemies) {
                        if (!IsEntityOnScreen(enemy)) continue;
                        
                        Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
                        if (!enemyComp || enemyComp->health <= 0) continue;
                        
                        Transform* enemyTransform = m_ecsSystem->GetComponent<Transform>(enemy);
                        Sprite* enemySprite = m_ecsSystem->GetComponent<Sprite>(enemy);
                        if (!enemyTransform) continue;
                        
                        // Calculate enemy CENTER (not top-left corner)
                        float enemyCenterX = enemyTransform->position.x;
                        float enemyCenterY = enemyTransform->position.y;
                        if (enemySprite) {
                            enemyCenterX += (enemySprite->width * std::abs(enemyTransform->scale.x)) * 0.5f;
                            enemyCenterY += (enemySprite->height * std::abs(enemyTransform->scale.y)) * 0.5f;
                        }
                        
                        float dx = enemyCenterX - position.x;
                        float dy = enemyCenterY - position.y;
                        float distance = std::sqrt(dx * dx + dy * dy);
                        
                        // Only target enemies AHEAD of player (positive X direction)
                        if (dx > 0 && distance < nearestDistance && distance > 1.0f) {
                            nearestDistance = distance;
                            nearestEnemy = enemy;
                        }
                    }
                }
                
                // ALWAYS shoot toward enemy at clamped angle if ANY enemy is on screen
                if (nearestEnemy != 0 && physics) {
                    projectileData->targetEnemy = nearestEnemy;
                    projectileData->hasAcquiredTarget = true;
                    
                    Transform* targetTransform = m_ecsSystem->GetComponent<Transform>(nearestEnemy);
                    Sprite* targetSprite = m_ecsSystem->GetComponent<Sprite>(nearestEnemy);
                    if (targetTransform) {
                        // Calculate enemy CENTER
                        float targetCenterX = targetTransform->position.x;
                        float targetCenterY = targetTransform->position.y;
                        if (targetSprite) {
                            targetCenterX += (targetSprite->width * std::abs(targetTransform->scale.x)) * 0.5f;
                            targetCenterY += (targetSprite->height * std::abs(targetTransform->scale.y)) * 0.5f;
                        }
                        
                        float dx = targetCenterX - position.x;
                        float dy = targetCenterY - position.y;
                        
                        // Calculate angle to enemy center in degrees
                        float angleToEnemyRad = std::atan2(dy, dx);
                        float angleToEnemyDeg = angleToEnemyRad * 180.0f / 3.14159f;
                        
                        // CLAMP angle to ±maxAngleDegrees (±20° rank 1, ±40° rank 2)
                        // This ensures we ALWAYS shoot toward the enemy, clamped to the cone
                        float clampedAngleDeg = std::max(-maxAngleDegrees, std::min(maxAngleDegrees, angleToEnemyDeg));
                        float clampedAngleRad = clampedAngleDeg * 3.14159f / 180.0f;
                        
                        // Calculate direction from clamped angle
                        float shootDirX = std::cos(clampedAngleRad);
                        float shootDirY = std::sin(clampedAngleRad);
                        
                        // Apply velocity at clamped angle
                        physics->velocity.x = shootDirX * 1500.0f;
                        physics->velocity.y = shootDirY * 1500.0f;
                        
                        GN_LOG_INFO("[HOMING] Target found! Enemy center at (" + std::to_string(targetCenterX) + 
                                   "," + std::to_string(targetCenterY) + "). Angle: " + std::to_string(angleToEnemyDeg) + 
                                   "° -> clamped to: " + std::to_string(clampedAngleDeg) + "° (max: ±" + std::to_string(maxAngleDegrees) + "°)");
                    }
                } else {
                    GN_LOG_INFO("[HOMING] No valid target on screen - shooting straight right");
                    // Keep default straight-right velocity
                    physics->velocity.x = 1500.0f;
                    physics->velocity.y = 0.0f;
                }
            } else {
                projectileData->isHoming = false;
                projectileData->homingStrength = 0.0f;
                projectileData->targetEnemy = 0;
                projectileData->hasAcquiredTarget = false;
            }
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

        // Spawn split projectiles if skill is active
        if (m_skillSystem) {
            int splitCount = m_skillSystem->GetProjectileSplitCount();
            if (splitCount > 1) {
                SpawnSplitProjectiles(position, direction, splitCount, projectileType);
            }
        }

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
            // CRITICAL: Snowballs and toilet paper don't expire by time - only removed when off-screen or hitting player
            // Other projectiles can still have time limits
            projectileData->lifetime = (projectileType == ProjectileType::SNOWBALL || projectileType == ProjectileType::TOILET_PAPER) ? 999999.0f : 5.0f;
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

            // Apply homing behavior before position update
            if (projectileData->isHoming) {
                ApplyHomingBehavior(projectile, deltaTime);
            }

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

            // Check if expired, off-screen, or deactivated by collision
            bool isExpired = projectileData->currentLifetime >= projectileData->lifetime;
            bool isOffScreen = IsProjectileOffScreen(transform, physics);
            bool isDeactivated = !projectileData->isActive;
            if (isExpired || isOffScreen || isDeactivated) {
                GN_LOG_INFO("Removing projectile: expired=" + std::to_string(isExpired) + ", offscreen=" + std::to_string(isOffScreen) + ", deactivated=" + std::to_string(isDeactivated) +
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
            
            // NOTE: Collision detection with player is handled in GameplayState
            // where all other collision checks happen
            
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

        // Configure hitbox per projectile type with proper offset based on sprite size
        Hitbox* hitbox = m_ecsSystem->GetComponent<Hitbox>(projectile);
        if (hitbox) {
            hitbox->type = ColliderType::Circle;
            // CRITICAL: Offset MUST be 0 since sprite is PRE-SCALED
            // Collision math adds: position + spriteHalf + (offset * scale)
            // Since sprite is already scaled to final size, offset would double-count
            // Player hitbox also uses offset=0 for the same reason
            hitbox->offsetX = 0.0f;
            hitbox->offsetY = 0.0f;
            
            // Store UNSCALED radius - the actual radius in the base sprite asset
            // TP sprite: 32x32px with ~10px radius circle
            // Snowball sprite: 32x32px with ~4px radius circle (small for precision)
            // Poop sprite: 16x16px with ~7px radius circle
            // Collision math will apply Transform.scale to get effective radius
            switch (projectileType) {
                case ProjectileType::TOILET_PAPER:
                    hitbox->radius = 10.0f;
                    break;
                case ProjectileType::SNOWBALL:
                    hitbox->radius = 4.0f;
                    break;
                case ProjectileType::POOP_BALL:
                case ProjectileType::LARGE_POOP_BALL:
                default:
                    hitbox->radius = 32.0f; // Direct effective radius (Transform.scale = 1.0)
                    break;
            }
            
            GN_LOG_INFO("✅ HITBOX CONFIGURED for projectile type " + std::to_string(static_cast<int>(projectileType)) +
                       ": radius=" + std::to_string(hitbox->radius) + "px (PRE-SCALED to match rendered size)" +
                       ", offset=(" + std::to_string(hitbox->offsetX) + "," + std::to_string(hitbox->offsetY) + ") [MUST BE 0]" +
                       ", spriteSize=" + std::to_string(sprite->width) + "x" + std::to_string(sprite->height) +
                       ", frameSize=" + std::to_string(config.frameWidth) + "x" + std::to_string(config.frameHeight) +
                       ", Transform.scale=1.0 (fixed)");
        }

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

    // ========== HOMING AND SPLIT SUPPORT ==========

    bool ProjectileSystem::IsEntityOnScreen(Entity entity) const {
        if (!m_ecsSystem || entity == 0) return false;

        Transform* transform = m_ecsSystem->GetComponent<Transform>(entity);
        if (!transform) return false;

        const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        const float BUFFER = 50.0f; // Small buffer for just off-screen

        return transform->position.x >= -BUFFER &&
               transform->position.x <= screenInfo.pixelWidth + BUFFER &&
               transform->position.y >= -BUFFER &&
               transform->position.y <= screenInfo.pixelHeight + BUFFER;
    }

    Entity ProjectileSystem::FindNearestOnScreenEnemy(const GNVector2& projectilePos, const GNVector2& projectileDir,
                                                       float maxAngleDegrees, const std::unordered_set<Entity>& excludeTargets) {
        if (!m_levelManager || !m_ecsSystem) return 0;

        Entity nearestEnemy = 0;
        float nearestDistance = std::numeric_limits<float>::max();

        // Get all active enemies from LevelManager
        const auto& activeEnemies = m_levelManager->GetActiveEnemies();
        
        // Convert max angle to radians for comparison
        const float maxAngleRad = maxAngleDegrees * 3.14159f / 180.0f;
        const float cosMaxAngle = std::cos(maxAngleRad);

        for (Entity enemy : activeEnemies) {
            // Skip excluded targets (already targeted by another split projectile)
            if (excludeTargets.count(enemy) > 0) continue;

            // Skip dead/inactive enemies
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
            if (!enemyComp || enemyComp->health <= 0) continue;

            // Must be on-screen
            if (!IsEntityOnScreen(enemy)) continue;

            Transform* enemyTransform = m_ecsSystem->GetComponent<Transform>(enemy);
            if (!enemyTransform) continue;

            // Calculate vector to enemy
            float dx = enemyTransform->position.x - projectilePos.x;
            float dy = enemyTransform->position.y - projectilePos.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < 1.0f) continue; // Too close, skip
            
            // Normalize direction to enemy
            float enemyDirX = dx / distance;
            float enemyDirY = dy / distance;
            
            // Check if enemy is within the angle cone
            // Dot product of normalized vectors gives cos(angle between them)
            float dotProduct = projectileDir.x * enemyDirX + projectileDir.y * enemyDirY;
            
            // If angle is within cone (dot product >= cos(maxAngle))
            if (dotProduct >= cosMaxAngle) {
                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    nearestEnemy = enemy;
                }
            }
        }

        return nearestEnemy;
    }

    void ProjectileSystem::ApplyHomingBehavior(Entity projectile, float deltaTime) {
        if (!m_ecsSystem || !m_skillSystem) return;

        Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(projectile);
        if (!projectileData || !projectileData->isHoming || !projectileData->isActive) return;

        Transform* transform = m_ecsSystem->GetComponent<Transform>(projectile);
        Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
        if (!transform || !physics) return;

        // If no target acquired yet, projectile keeps flying straight
        if (!projectileData->hasAcquiredTarget || projectileData->targetEnemy == 0) {
            return; // No target, just continue straight
        }

        // Check if target is still valid and on-screen
        Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(projectileData->targetEnemy);
        if (!enemyComp || enemyComp->health <= 0 || !IsEntityOnScreen(projectileData->targetEnemy)) {
            // Target lost - TRY TO FIND A NEW TARGET
            Entity newTarget = 0;
            float nearestDistance = std::numeric_limits<float>::max();
            
            if (m_levelManager) {
                const auto& activeEnemies = m_levelManager->GetActiveEnemies();
                for (Entity enemy : activeEnemies) {
                    if (!IsEntityOnScreen(enemy)) continue;
                    if (enemy == projectileData->targetEnemy) continue; // Skip old target
                    
                    Enemy* newEnemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
                    if (!newEnemyComp || newEnemyComp->health <= 0) continue;
                    
                    Transform* enemyTransform = m_ecsSystem->GetComponent<Transform>(enemy);
                    if (!enemyTransform) continue;
                    
                    // Only target enemies ahead (positive X direction)
                    float dx = enemyTransform->position.x - transform->position.x;
                    float dy = enemyTransform->position.y - transform->position.y;
                    if (dx <= 0) continue;
                    
                    float distance = std::sqrt(dx * dx + dy * dy);
                    if (distance < nearestDistance) {
                        nearestDistance = distance;
                        newTarget = enemy;
                    }
                }
            }
            
            if (newTarget != 0) {
                projectileData->targetEnemy = newTarget;
                GN_LOG_INFO("[HOMING] Retargeted to new enemy " + std::to_string(newTarget));
            } else {
                projectileData->targetEnemy = 0;
                return; // No targets, continue straight
            }
        }

        Transform* targetTransform = m_ecsSystem->GetComponent<Transform>(projectileData->targetEnemy);
        Sprite* targetSprite = m_ecsSystem->GetComponent<Sprite>(projectileData->targetEnemy);
        if (!targetTransform) return;

        // Calculate enemy center (account for sprite size)
        float targetCenterX = targetTransform->position.x;
        float targetCenterY = targetTransform->position.y;
        if (targetSprite) {
            targetCenterX += (targetSprite->width * std::abs(targetTransform->scale.x)) * 0.5f;
            targetCenterY += (targetSprite->height * std::abs(targetTransform->scale.y)) * 0.5f;
        }

        // Calculate distance to target center
        float dx = targetCenterX - transform->position.x;
        float dy = targetCenterY - transform->position.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Only steer if not very close
        if (distance > 20.0f) {
            // DISTANCE-BASED HOMING: closer = stronger attraction (like PickupSystem magnets)
            // Range: 1x at max range, up to 3x when very close
            const float maxHomingRange = 800.0f;
            float distanceRatio = std::min(distance / maxHomingRange, 1.0f); // 0 = close, 1 = far
            float strengthMultiplier = 1.0f + (1.0f - distanceRatio) * 2.0f; // 1x to 3x
            float homingStrength = projectileData->homingStrength * strengthMultiplier;

            // Calculate steering - use both X and Y for proper homing
            // Normalize direction to target
            float dirX = dx / distance;
            float dirY = dy / distance;
            
            // Calculate current angle and target angle
            float currentAngle = std::atan2(physics->velocity.y, physics->velocity.x);
            float targetAngle = std::atan2(dy, dx);
            
            // Calculate angle difference
            float angleDiff = targetAngle - currentAngle;
            
            // Normalize angle difference to [-PI, PI]
            while (angleDiff > 3.14159f) angleDiff -= 2.0f * 3.14159f;
            while (angleDiff < -3.14159f) angleDiff += 2.0f * 3.14159f;
            
            // Apply steering (rotate toward target)
            float maxSteerRate = homingStrength * 0.1f * deltaTime; // Radians per second
            float steerAmount = std::max(-maxSteerRate, std::min(maxSteerRate, angleDiff));
            float newAngle = currentAngle + steerAmount;
            
            // Maintain constant speed of 1500
            const float projectileSpeed = 1500.0f;
            physics->velocity.x = std::cos(newAngle) * projectileSpeed;
            physics->velocity.y = std::sin(newAngle) * projectileSpeed;
        }
    }

    void ProjectileSystem::SpawnSplitProjectiles(const GNVector2& position, const GNVector2& direction,
                                                  int splitCount, ProjectileType projectileType) {
        if (splitCount <= 1) return; // No split needed

        // Collect targets for different projectiles (so they don't all target the same enemy)
        std::unordered_set<Entity> usedTargets;

        // Define parallel offsets (perpendicular to direction)
        // For 2 projectiles: offset by ±30 pixels
        // For 3 projectiles: 0, ±60 pixels
        const float PARALLEL_SPACING = 120.0f; // Doubled from 60.0f for better separation

        for (int i = 1; i < splitCount; ++i) {
            // Calculate perpendicular offset
            float perpX = -direction.y; // Rotate 90 degrees
            float perpY = direction.x;

            float offsetMultiplier;
            if (splitCount == 2) {
                offsetMultiplier = (i == 1) ? 1.0f : -1.0f; // +30, already spawned center
            } else { // splitCount == 3
                offsetMultiplier = (i == 1) ? 1.0f : -1.0f; // ±60
            }

            GNVector2 offsetPos = position;
            offsetPos.x += perpX * PARALLEL_SPACING * offsetMultiplier;
            offsetPos.y += perpY * PARALLEL_SPACING * offsetMultiplier;

            // Spawn the split projectile
            Entity splitProjectile = GetInactiveProjectile(true);
            if (splitProjectile == 0) {
                GN_LOG_WARN("No inactive projectiles for split");
                continue;
            }

            // Configure transform
            Transform* transform = m_ecsSystem->GetComponent<Transform>(splitProjectile);
            if (transform) {
                transform->position = offsetPos;
                transform->scale = Gnosis::GNVector2(1.0f, 1.0f);
            }

            // Configure physics
            Physics* physics = m_ecsSystem->GetComponent<Physics>(splitProjectile);
            if (physics) {
                physics->velocity = direction * 1500.0f;
                physics->useGravity = false;
            }

            // Add scroll speed
            ScrollSpeed scrollSpeedComponent(-1700.0f);
            m_ecsSystem->AddComponent(splitProjectile, scrollSpeedComponent);

            // Configure projectile data
            Projectile* projectileData = m_ecsSystem->GetComponent<Projectile>(splitProjectile);
            if (projectileData) {
                projectileData->isActive = true;
                projectileData->projectileType = projectileType;
                projectileData->spawnPosition = offsetPos;
                projectileData->currentLifetime = 0.0f;
                projectileData->lifetime = 3.0f;
                projectileData->damage = 1;
                projectileData->isEnemyProjectile = false;

                // Set up homing if skill is active
                if (m_skillSystem && m_skillSystem->HasHomingProjectiles()) {
                    projectileData->isHoming = true;
                    projectileData->homingStrength = m_skillSystem->GetHomingStrength();
                    projectileData->hasAcquiredTarget = false;

                    // Try to find a target different from others
                    float angleCone = m_skillSystem->GetHomingAngleCone();
                    Entity target = FindNearestOnScreenEnemy(offsetPos, direction, angleCone, usedTargets);
                    projectileData->targetEnemy = target;
                    if (target != 0) {
                        usedTargets.insert(target);
                        projectileData->hasAcquiredTarget = true;
                    }
                } else {
                    projectileData->isHoming = false;
                    projectileData->homingStrength = 0.0f;
                    projectileData->targetEnemy = 0;
                    projectileData->hasAcquiredTarget = false;
                }
            }

            // Configure sprite
            ConfigureProjectileSprite(splitProjectile, projectileType);

            // Move to active pool
            m_playerProjectiles.inactiveProjectiles.erase(
                std::remove(m_playerProjectiles.inactiveProjectiles.begin(),
                           m_playerProjectiles.inactiveProjectiles.end(), splitProjectile),
                m_playerProjectiles.inactiveProjectiles.end()
            );
            m_playerProjectiles.activeProjectiles.push_back(splitProjectile);

            GN_LOG_INFO("[SPLIT] Spawned split projectile " + std::to_string(i) + "/" + std::to_string(splitCount - 1) +
                       " at (" + std::to_string(offsetPos.x) + ", " + std::to_string(offsetPos.y) + ")");
        }
    }

} // namespace GameCore

