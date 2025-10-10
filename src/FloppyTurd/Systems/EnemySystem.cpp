#include "EnemySystem.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include <cmath>
#include <algorithm>

// Use shorter type names
using Gnosis::ECS;
using Gnosis::EnemyType;
using GameCore::Entity;
using GameCore::GNVector2;

namespace GameCore {

EnemySystem::EnemySystem(ECS* ecsSystem, LevelManager* levelManager, ProjectileSystem* projectileSystem)
    : m_ecsSystem(ecsSystem)
    , m_levelManager(levelManager)
    , m_projectileSystem(projectileSystem)
    , m_time(0.0f) {}

void EnemySystem::Update(float deltaTime) {
    if (!m_ecsSystem || !m_levelManager || !m_projectileSystem) {
        GN_LOG_DEBUG("EnemySystem: Update skipped - missing systems");
        return;
    }
    m_time += deltaTime;

    // PHASE 2 OPTIMIZATION: Single-Pass ECS Pattern
    // Instead of iterating 5 times through enemies, iterate ONCE and process all components
    // Before: 5 loops × 10 enemies = 50 iterations per frame
    // After:  1 loop × 10 enemies = 10 iterations per frame (80% reduction!)
    const auto& activeEnemies = m_levelManager->GetActiveEnemies();
    const auto& activeProjectiles = m_projectileSystem->GetActivePlayerProjectiles();

    // Log collision opportunities
    static int frameCount = 0;
    frameCount++;
    if (activeEnemies.size() > 0 && activeProjectiles.size() > 0) {
        GN_LOG_INFO("Frame " + std::to_string(frameCount) + " COLLISION CHECK - " + std::to_string(activeEnemies.size()) + " enemies, " + 
                     std::to_string(activeProjectiles.size()) + " projectiles");
    }

    // SINGLE PASS: Process each enemy completely before moving to the next
    for (Entity enemyEntity : activeEnemies) {
        // Fetch all components ONCE per enemy (not 5 times like before)
        Transform* transform = m_ecsSystem->GetComponent<Transform>(enemyEntity);
        Enemy* enemy = m_ecsSystem->GetComponent<Enemy>(enemyEntity);
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemyEntity);
        StateAnimation* stateAnim = m_ecsSystem->GetComponent<StateAnimation>(enemyEntity);
        Hitbox* hitbox = m_ecsSystem->GetComponent<Hitbox>(enemyEntity);
        
        if (!transform || !enemy || !enemy->isActive) continue;

        // Process all updates for this enemy in sequence:
        // 1. Update state (hurt timer, state transitions, initialization)
        ProcessEnemyState(deltaTime, enemyEntity, enemy, sprite);
        
        // If enemy was returned to pool during state update, skip further processing
        if (!enemy->isActive) continue;
        
        // 2. Update movement (physics, bobbing, wrapping)
        ProcessEnemyMovement(deltaTime, enemy, transform);
        
        // 3. Update animation (state-based switching, frame advancement)
        ProcessEnemyAnimation(deltaTime, enemy, sprite, stateAnim);
        
        // 4. Check collisions (only if not in hurt state and not decorative)
        if (enemy->currentState != EnemyState::Hurt && enemy->currentState != EnemyState::Decorative) {
            // Only log if there are projectiles to check
            if (activeProjectiles.size() > 0) {
                ProcessEnemyCollision(enemyEntity, enemy, transform, hitbox, sprite, stateAnim, activeProjectiles);
            }
        }
    }
}

void EnemySystem::UpdateSnowmanThrower(float deltaTime, Entity enemy, Enemy* enemyComp, Transform* transform) {
    // Check if enemy is on screen
    bool wasOnScreen = enemyComp->isOnScreen;
    enemyComp->isOnScreen = IsEnemyOnScreen(transform);
    
    // If just came on screen, start throw timer
    if (enemyComp->isOnScreen && !wasOnScreen) {
        enemyComp->throwTimer = 0.0f;
    }
    
    // Only throw when on screen, not already throwing, and player is within range
    if (enemyComp->isOnScreen && !enemyComp->isThrowing && enemyComp->currentState != EnemyState::Attacking) {
        // Check player proximity - only throw when player is close enough
        bool playerInRange = false;
        if (m_levelManager) {
            // Get the actual player entity from LevelManager
            Entity playerEntity = m_levelManager->GetPlayerEntity();
            if (playerEntity != 0) {
                Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                if (playerTransform) {
                    float distance = std::abs(transform->position.x - playerTransform->position.x);
                    if (distance <= enemyComp->throwRange) {
                        playerInRange = true;
                        GN_LOG_DEBUG("Snowman thrower: player in range! Distance=" + std::to_string(distance) + ", throwRange=" + std::to_string(enemyComp->throwRange));
                    }
                }
            }
        }
        
        // If no player found in range, use a fallback check based on screen position
        if (!playerInRange) {
            // Assume player is around screen center (X=600) for fallback
            float distance = std::abs(transform->position.x - 600.0f);
            playerInRange = (distance <= enemyComp->throwRange);
            if (playerInRange) {
                GN_LOG_DEBUG("Snowman thrower: using fallback distance check. Distance=" + std::to_string(distance) + ", throwRange=" + std::to_string(enemyComp->throwRange));
            }
        }
        
        if (playerInRange) {
            enemyComp->throwTimer += deltaTime;
            
            // Check if it's time to throw
            if (enemyComp->throwTimer >= enemyComp->throwCooldown) {
            // Start throw animation
            enemyComp->isThrowing = true;
            enemyComp->currentThrowFrame = 0;
            enemyComp->throwAnimationTimer = 0.0f;
            ChangeEnemyState(enemyComp, EnemyState::Attacking, enemyComp->throwAnimationDuration);
            
            // Switch to throw state using StateAnimation
            StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(enemy);
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
            if (sa && sprite) {
                const StateAnimation::Clip* throwClip = sa->getClip("throw");
                if (throwClip) {
                    sa->currentState = "throw";
                    sprite->textureId = throwClip->textureId;
                    sprite->isAnimated = (throwClip->frameCount > 1);
                    sprite->frameWidth = throwClip->frameWidth;
                    sprite->frameHeight = throwClip->frameHeight;
                    sprite->frameCount = throwClip->frameCount;
                    sprite->frameTime = throwClip->frameTime;
                    sprite->loop = throwClip->loop;
                    sprite->currentFrame = 0;
                    sprite->currentFrameTime = 0.0f;
                    sprite->playing = true;
                    sprite->hasCompleted = false;
                }
            }
            
            // Reset throw timer
            enemyComp->throwTimer = 0.0f;
        }
        }
    }
    
    // Handle throw animation and projectile spawning
    if (enemyComp->isThrowing && enemyComp->currentState == EnemyState::Attacking) {
        enemyComp->throwAnimationTimer += deltaTime;
        
        // Update throw animation frame
        int frameIndex = static_cast<int>(enemyComp->throwAnimationTimer / enemyComp->frameDuration);
        enemyComp->currentThrowFrame = std::min(frameIndex, enemyComp->totalFrames - 1);
        
        // Spawn projectile at frame 3 (middle of throw animation)
        if (enemyComp->currentThrowFrame == 3 && !enemyComp->hasSpawnedProjectile) {
            SpawnEnemyProjectile(enemy, enemyComp, transform);
            enemyComp->hasSpawnedProjectile = true;
        }
        
        // Complete throw animation
        if (enemyComp->throwAnimationTimer >= enemyComp->throwAnimationDuration) {
            enemyComp->isThrowing = false;
            enemyComp->hasSpawnedProjectile = false;
            
            // Switch back to idle texture when throw completes
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
            if (sprite && sprite->textureId == "SnowManThrow") {
                sprite->textureId = "SnowManIdle";  // Revert to idle texture
            }
            
            ChangeEnemyState(enemyComp, EnemyState::Idle, 0.0f);
        }
    }
}

bool EnemySystem::IsEnemyOnScreen(const Transform* transform) {
    // Simple on-screen check - can be expanded based on camera system
    // For snow level, enemies should be considered "on screen" when they're visible
    // Use wider bounds to ensure snowmen can throw when player approaches
    return transform->position.x >= -200.0f && transform->position.x <= 1200.0f;
}

void EnemySystem::ChangeEnemyState(Enemy* enemy, EnemyState newState, float duration) {
    enemy->currentState = newState;
    enemy->stateTimer = 0.0f;
    enemy->stateDuration = duration;
}

void EnemySystem::SpawnEnemyProjectile(Entity enemy, const Enemy* enemyComp, const Transform* transform) {
    if (!m_projectileSystem) {
        GN_LOG_WARN("Cannot spawn enemy projectile - ProjectileSystem not available");
        return;
    }

    // Determine projectile type based on enemy movement pattern
    ProjectileType projectileType;
    if (enemyComp->movementPattern == "snowman_thrower") {
        projectileType = ProjectileType::SNOWBALL;
    } else if (enemyComp->movementPattern == "rat_king") {
        projectileType = ProjectileType::TOILET_PAPER;
    } else {
        // Other enemy types don't spawn projectiles
        return;
    }

    // Calculate spawn position (offset from enemy)
    GNVector2 spawnPosition = transform->position;
    spawnPosition.x -= 30.0f; // Offset from enemy
    spawnPosition.y += 20.0f; // Launch from upper body area

    // Calculate projectile direction (towards player, simplified)
    GNVector2 direction(-1.0f, -0.3f); // Left and slightly down

    // Spawn projectile using the ProjectileSystem
    Entity projectileEntity = m_projectileSystem->SpawnEnemyProjectile(
        spawnPosition,
        direction,
        projectileType,
        enemyComp->damage
    );

    if (projectileEntity != 0) {
        GN_LOG_INFO("Enemy %d spawned projectile entity: %d at position (%.1f, %.1f)",
                   enemy, projectileEntity, spawnPosition.x, spawnPosition.y);
    } else {
        GN_LOG_WARN("Failed to spawn enemy projectile - no available projectiles in pool");
    }
}

void EnemySystem::UpdateEnemyProjectiles(float deltaTime) {
    // Projectile updating is now handled by ProjectileSystem
    // This method is kept for compatibility but does nothing
}

void EnemySystem::CleanupProjectiles() {
    // Projectile cleanup is now handled by ProjectileSystem
    // This method is kept for compatibility but does nothing

    // Clear our projectile tracking list since we no longer manage projectiles directly
    m_enemyProjectiles.clear();
}

void EnemySystem::InitializeEnemyBehavior(Enemy* enemy, const std::string& movementPattern) {
    if (movementPattern == "snowman_thrower") {
        // Configure red snowman thrower
        enemy->isThrower = true;
        enemy->isAnimated = true;
        enemy->totalFrames = 6; // 6-frame throw animation
        enemy->frameDuration = 0.1f; // 0.1 seconds per frame
        enemy->throwCooldown = 2.0f; // 2 seconds between throws
        enemy->throwRange = 400.0f; // Start throwing when player is within 400 pixels
        enemy->currentState = EnemyState::Idle;
        enemy->isGrounded = true;
        enemy->groundOffset = 0.0f; // Snowmen sit directly on ground
        
        // Set up throw animation timing
        enemy->throwAnimationDuration = 0.6f; // Total throw animation time (6 frames * 0.1s)
        
    } else if (movementPattern == "decorative") {
        // Decorative enemies (like Chill, Green, Chad snowmen) - passive, no attacks
        // They move with world scroll but don't attack or bob
        enemy->currentState = EnemyState::Decorative;
        enemy->isActive = true; // KEEP THEM ACTIVE so they render!
        enemy->isGrounded = true;
        enemy->groundOffset = 0.0f;
        enemy->bobbingEnabled = false; // No vertical movement
        enemy->speed = enemy->speed; // Use config speed for horizontal scrolling (typically same as world speed)
        
    } else if (movementPattern == "horizontal") {
        // Basic horizontal moving enemies - RESPECT bobbing config from EnemyConfig
        enemy->currentState = EnemyState::Moving;
        // Don't override bobbing settings - let EnemyConfig control this
        enemy->isGrounded = true;
        enemy->groundOffset = 10.0f; // Float slightly above ground
        
    } else if (movementPattern == "vertical") {
        // Vertical moving enemies
        enemy->currentState = EnemyState::Moving;
        enemy->bobbingEnabled = true;
        enemy->bobSpeed = 1.5f;
        enemy->bobAmplitude = 50.0f;
        enemy->isGrounded = true;
        enemy->groundOffset = 20.0f; // Float above ground
        
    } else if (movementPattern == "swoop") {
        // Swooping enemies (like birds)
        enemy->currentState = EnemyState::Moving;
        enemy->bobbingEnabled = true;
        enemy->bobSpeed = 3.0f;
        enemy->bobAmplitude = 80.0f;
        enemy->isGrounded = false; // Birds can fly freely
        
    } else {
        // Default behavior
        enemy->currentState = EnemyState::Idle;
        enemy->isGrounded = true;
        enemy->groundOffset = 0.0f;
    }
}

void EnemySystem::GroundEnemy(Enemy* enemy, Transform* transform) {
    if (!enemy->isGrounded) return;
    
    // If the enemy already has a valid baseY set by LevelManager, don't override it
    // This prevents conflicts between initial positioning and grounding
    if (enemy->hasInitializedBaseY && enemy->baseY > 0.0f) {
        // Just ensure the current position matches the baseY
        GN_LOG_DEBUG("GroundEnemy: Using existing baseY=" + std::to_string(enemy->baseY) + " for enemy at x=" + std::to_string(transform->position.x));
        transform->position.y = enemy->baseY;
        return;
    }
    
    // For enemies that don't have a baseY set by LevelManager, calculate it
    // Use ConfigManager for device-agnostic screen height
    const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
    const float screenHeight = screenInfo.pixelHeight;
    
    // Get actual sprite height from the sprite component
    float enemyHeight = 64.0f; // Default fallback
    if (m_ecsSystem) {
        // Find the entity that has this transform to get its sprite
        const auto enemies = m_levelManager->GetActiveEnemies();
        for (Entity e : enemies) {
            Transform* enemyTransform = m_ecsSystem->GetComponent<Transform>(e);
            if (enemyTransform == transform) {
                Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(e);
                if (sprite) {
                    enemyHeight = sprite->height * std::abs(transform->scale.y);
                }
                break;
            }
        }
    }
    
    // Calculate ground position: place enemy so its bottom edge touches the ground level
    // For enemies spawned by LevelManager, the baseY is the actual ground level (2556)
    // We need to subtract the enemy height to position the bottom edge at ground level
    float finalY = enemy->baseY - enemyHeight - enemy->groundOffset;
    
    GN_LOG_DEBUG("GroundEnemy: Calculated finalY=" + std::to_string(finalY) + " (baseY=" + std::to_string(enemy->baseY) + " - enemyHeight=" + std::to_string(enemyHeight) + " - groundOffset=" + std::to_string(enemy->groundOffset) + ") for enemy at x=" + std::to_string(transform->position.x));
    
    // Update base Y position for bobbing calculations
    enemy->baseY = finalY;
    
    // Set current position to ground
    transform->position.y = finalY;
    
    GN_LOG_DEBUG("GroundEnemy: Set enemy position.y=" + std::to_string(transform->position.y) + " and baseY=" + std::to_string(enemy->baseY));
}

// ============================================================================
// PHASE 2: Single-Pass ECS Helper Methods
// These methods process individual enemy components instead of looping
// ============================================================================

void EnemySystem::ProcessEnemyState(float deltaTime, Entity e, Enemy* enemy, Sprite* sprite) {
    // Get transform for initialization (only fetched if needed)
    Transform* transform = nullptr;
    
    // Initialize enemy if needed
    if (!enemy->hasInitializedBaseY) {
        transform = m_ecsSystem->GetComponent<Transform>(e);
        if (!transform) return;
        
        GN_LOG_DEBUG("EnemySystem: Initializing enemy at x=" + std::to_string(transform->position.x) + 
                     " y=" + std::to_string(transform->position.y) + " with movementPattern=" + enemy->movementPattern);
        
        enemy->baseY = transform->position.y;
        enemy->spawnPosition = transform->position;
        enemy->hasInitializedBaseY = true;
        
        GN_LOG_DEBUG("EnemySystem: Using LevelManager position - baseY=" + std::to_string(enemy->baseY) + 
                     " spawnPosition.y=" + std::to_string(enemy->spawnPosition.y) + 
                     " current position.y=" + std::to_string(transform->position.y));
        
        InitializeEnemyBehavior(enemy, enemy->movementPattern);
    }

    // Update state timer
    enemy->stateTimer += deltaTime;

    // Handle hurt state timer - check if animation has actually completed
    if (enemy->currentState == EnemyState::Hurt) {
        enemy->hurtTimer -= deltaTime;
        
        bool animationCompleted = false;
        
        if (sprite) {
            // Animation is complete if hasCompleted flag is set OR timer elapsed
            animationCompleted = sprite->hasCompleted || (enemy->hurtTimer <= 0.0f);
            
            if (!animationCompleted && sprite->hasCompleted) {
                GN_LOG_INFO("Enemy " + std::to_string(e) + " hurt animation hasCompleted=true");
            }
        } else {
            // No sprite component - use timer fallback
            animationCompleted = (enemy->hurtTimer <= 0.0f);
        }
        
        if (animationCompleted) {
            // Hurt animation completed - return enemy to inactive pool for reuse
            if (m_levelManager) {
                m_levelManager->ReturnEnemyToPool(e);
            }
            GN_LOG_INFO("Enemy " + std::to_string(e) + " hurt animation completed, returned to inactive pool");
        }
    }

    // Check if state should change
    if (enemy->stateDuration > 0.0f && enemy->stateTimer >= enemy->stateDuration) {
        // Return to idle after state duration
        ChangeEnemyState(enemy, EnemyState::Idle);
    }
}

void EnemySystem::ProcessEnemyMovement(float deltaTime, Enemy* enemy, Transform* transform) {
    if (!enemy || !transform) return;

    // Get screen info for device-agnostic dimensions
    const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();

    // SPECIAL CASE: RatCopter "flying" pattern (hover + beeline)
    if (enemy->movementPattern == "flying") {
        // Hovering phase: Move left slowly while staying in vertical bounds
        if (transform->position.x > screenInfo.pixelWidth * 0.75f) {
            // Still hovering - move left slowly
            transform->position.x -= (enemy->speed * 0.4f) * deltaTime; // 40% speed for hover
            
            // Apply bobbing for hover effect
            if (enemy->bobbingEnabled) {
                float bobOffset = std::sin(m_time * enemy->bobSpeed) * enemy->bobAmplitude;
                transform->position.y = enemy->baseY + bobOffset;
                
                // Constrain Y to safe bounds (padding from top and bottom)
                const float topPadding = 100.0f;
                const float bottomPadding = 100.0f;
                const float minY = topPadding;
                const float maxY = screenInfo.pixelHeight - bottomPadding;
                transform->position.y = std::max(minY, std::min(maxY, transform->position.y));
            }
        } else {
            // Beeline phase: Reached 75% screen trigger - shoot toward player!
            // For now, just move faster and straight (player tracking can be added later)
            transform->position.x -= (enemy->speed * 1.5f) * deltaTime; // 150% speed for beeline
            // Y stays constant during beeline (locked onto target Y from hover)
        }
    } else {
        // Standard horizontal movement for other enemy types
        transform->position.x -= enemy->speed * deltaTime;

        // Vertical movement - bobbing/sinusoidal if enabled
        if (enemy->bobbingEnabled) {
            // Calculate bobbing offset using sine wave
            float bobOffset = std::sin(m_time * enemy->bobSpeed) * enemy->bobAmplitude;
            transform->position.y = enemy->baseY + bobOffset;
        }
    }

    // Screen wrapping (if enemy goes off left side, wrap to right)
    const float wrapBuffer = 200.0f;
    if (transform->position.x < -wrapBuffer) {
        // Wrap to right side
        transform->position.x = screenInfo.pixelWidth + (wrapBuffer / 2.0f);
    }
}

void EnemySystem::ProcessEnemyAnimation(float deltaTime, Enemy* enemy, Sprite* sprite, StateAnimation* stateAnim) {
    if (!enemy || !sprite) return;

    // If enemy uses StateAnimation, check for state changes
    if (stateAnim && !stateAnim->clips.empty()) {
        std::string desiredState = "idle"; // Default state
        
        // Map EnemyState to animation state string
        switch (enemy->currentState) {
            case EnemyState::Hurt:
                desiredState = "hurt";
                break;
            case EnemyState::Attacking:
                desiredState = "attack";
                break;
            case EnemyState::Idle:
            default:
                desiredState = "idle";
                break;
        }
        
        // Only update sprite properties if state changed (prevents interference with SpriteSystem)
        if (stateAnim->currentState != desiredState) {
            // Find the animation clip for the new state using getClip helper
            const StateAnimation::Clip* currentClip = stateAnim->getClip(desiredState);
            
            if (currentClip) {
                stateAnim->currentState = desiredState;
                
                GN_LOG_INFO("EnemySystem: State changed for enemy - switching from '" + 
                           sprite->textureId + "' to '" + currentClip->textureId + "'");
                
                // Update sprite to match the new animation clip
                sprite->textureId = currentClip->textureId;
                sprite->frameWidth = currentClip->frameWidth;
                sprite->frameHeight = currentClip->frameHeight;
                sprite->frameCount = currentClip->frameCount;
                sprite->frameTime = currentClip->frameTime;
                sprite->loop = currentClip->loop;
                sprite->isAnimated = (currentClip->frameCount > 1);
                sprite->playing = true;
                sprite->currentFrame = 0; // Reset to first frame on state change
                sprite->currentFrameTime = 0.0f;
            } else {
                GN_LOG_WARN("EnemySystem: No animation clip found for state '" + desiredState + "'");
            }
        }
    }
}

void EnemySystem::ProcessEnemyCollision(Entity e, Enemy* enemy, Transform* transform, Hitbox* hitbox,
                                       Sprite* sprite, StateAnimation* stateAnim,
                                       const std::vector<Gnosis::Entity>& activeProjectiles) {
    if (!enemy || !transform || !hitbox) return;
    if (enemy->currentState == EnemyState::Hurt) return; // Already hurt
    
    // DEBUG: Log first enemy's collision check details
    static bool loggedOnce = false;
    if (!loggedOnce && activeProjectiles.size() > 0) {
        GN_LOG_INFO("ProcessEnemyCollision: Enemy " + std::to_string(e) + 
                   " at (" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + 
                   ") radius=" + std::to_string(hitbox->radius) + 
                   " checking " + std::to_string(activeProjectiles.size()) + " projectiles");
        loggedOnce = true;
    }
    
    // Check collision with each projectile
    for (Entity projEntity : activeProjectiles) {
        Transform* projTransform = m_ecsSystem->GetComponent<Transform>(projEntity);
        Hitbox* projHitbox = m_ecsSystem->GetComponent<Hitbox>(projEntity);
        Projectile* proj = m_ecsSystem->GetComponent<Projectile>(projEntity);
        
        if (!projTransform || !projHitbox || !proj || !proj->isActive) continue;
        
        // DEBUG: Log first projectile check
        if (!loggedOnce) {
            GN_LOG_INFO("  Checking projectile " + std::to_string(projEntity) + 
                       " at (" + std::to_string(projTransform->position.x) + "," + std::to_string(projTransform->position.y) + 
                       ") radius=" + std::to_string(projHitbox->radius));
        }
        
        // Simple circle-circle collision for now
        bool collision = false;
        if (hitbox->type == ColliderType::Circle && projHitbox->type == ColliderType::Circle) {
            float dx = transform->position.x - projTransform->position.x;
            float dy = transform->position.y - projTransform->position.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            float combinedRadius = hitbox->radius + projHitbox->radius;
            collision = (distance < combinedRadius);
            
            if (!loggedOnce) {
                GN_LOG_INFO("    dx=" + std::to_string(dx) + " dy=" + std::to_string(dy) + 
                           " distance=" + std::to_string(distance) + " combinedRadius=" + std::to_string(combinedRadius) + 
                           " collision=" + std::to_string(collision));
            }
        }
        
        if (collision) {
            GN_LOG_INFO("Projectile-Enemy collision detected! Projectile: " + std::to_string(projEntity) + 
                       " Enemy: " + std::to_string(e));
            
            // Damage the enemy
            enemy->health -= proj->damage;
            
            // Deactivate the projectile
            proj->isActive = false;
            
            if (enemy->health <= 0) {
                // Enemy defeated - switch to hurt state, animation will play before pool return
                enemy->currentState = EnemyState::Hurt;
                
                float hurtDuration = 0.6f; // Default
                
                // Use already-fetched sprite and stateAnim (no more GetComponent calls!)
                if (stateAnim && sprite) {
                    // Find hurt animation clip to get accurate duration
                    const StateAnimation::Clip* hurtClip = stateAnim->getClip("hurt");
                    if (hurtClip) {
                        hurtDuration = hurtClip->frameCount * hurtClip->frameTime + 0.1f; // Small buffer
                    }
                    
                    // CRITICAL: Reset hasCompleted flag so animation plays from start
                    sprite->hasCompleted = false;
                }
                
                enemy->hurtTimer = hurtDuration;
                
                GN_LOG_INFO("Enemy " + std::to_string(e) + " defeated - switching to hurt animation");
            } else {
                // Enemy took damage but not defeated - brief hurt state
                enemy->currentState = EnemyState::Hurt;
                enemy->hurtTimer = 0.3f; // Brief hurt flash
                
                // Use already-fetched sprite (no GetComponent call!)
                if (sprite) {
                    sprite->hasCompleted = false;
                }
            }
            
            break; // Only process one collision per enemy per frame
        }
    }
}

} // namespace GameCore