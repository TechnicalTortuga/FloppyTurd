#include "EnemySystem.h"
#include <cmath>
#include <algorithm>

namespace GameCore {

EnemySystem::EnemySystem(Gnosis::ECS* ecsSystem, LevelManager* levelManager)
    : m_ecsSystem(ecsSystem)
    , m_levelManager(levelManager)
    , m_time(0.0f) {}

void EnemySystem::Update(float deltaTime) {
    if (!m_ecsSystem || !m_levelManager) return;
    m_time += deltaTime;

    // Update all enemy behaviors
    UpdateEnemyStates(deltaTime);
    UpdateEnemyMovement(deltaTime);
    UpdateEnemyAnimations(deltaTime);
    
    // Update enemy projectiles
    UpdateEnemyProjectiles(deltaTime);
    
    // Clean up expired projectiles
    CleanupProjectiles();
}

void EnemySystem::UpdateEnemyStates(float deltaTime) {
    const auto enemies = m_levelManager->GetActiveEnemies();
    for (Gnosis::Entity e : enemies) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(e);
        Enemy* enemy = m_ecsSystem->GetComponent<Enemy>(e);
        if (!transform || !enemy || !enemy->isActive) continue;

        // Initialize enemy if needed
        if (!enemy->hasInitializedBaseY) {
            GN_LOG_DEBUG("EnemySystem: Initializing enemy at x=" + std::to_string(transform->position.x) + " y=" + std::to_string(transform->position.y) + " with movementPattern=" + enemy->movementPattern);
            
            // For enemies spawned by LevelManager, use the position that was already set
            // Don't call GroundEnemy during initialization - respect LevelManager's positioning
            enemy->baseY = transform->position.y;
            enemy->spawnPosition = transform->position;
            enemy->hasInitializedBaseY = true;
            
            GN_LOG_DEBUG("EnemySystem: Using LevelManager position - baseY=" + std::to_string(enemy->baseY) + " spawnPosition.y=" + std::to_string(enemy->spawnPosition.y) + " current position.y=" + std::to_string(transform->position.y));
            
            // Initialize enemy behavior based on movement pattern
            InitializeEnemyBehavior(enemy, enemy->movementPattern);
        }

        // Update state timer
        enemy->stateTimer += deltaTime;
        
        // Check if state should change
        if (enemy->stateDuration > 0.0f && enemy->stateTimer >= enemy->stateDuration) {
            // Return to idle after state duration
            ChangeEnemyState(enemy, EnemyState::Idle);
        }

        // Handle specific enemy types
        if (enemy->movementPattern == "snowman_thrower") {
            UpdateSnowmanThrower(deltaTime, e, enemy, transform);
        }
        
        // Handle decorative enemies (no behavior)
        if (enemy->movementPattern == "decorative") {
            enemy->currentState = EnemyState::Decorative;
        }
    }
}

void EnemySystem::UpdateEnemyMovement(float deltaTime) {
    const auto enemies = m_levelManager->GetActiveEnemies();
    for (Gnosis::Entity e : enemies) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(e);
        Enemy* enemy = m_ecsSystem->GetComponent<Enemy>(e);
        if (!transform || !enemy || !enemy->isActive) continue;

        // Skip decorative enemies
        if (enemy->currentState == EnemyState::Decorative) continue;

        // Handle bobbing movement while maintaining grounding
        if (enemy->bobbingEnabled) {
            float bobOffset = std::sin(enemy->bobPhase + m_time * enemy->bobSpeed) * enemy->bobAmplitude;
            transform->position.y = enemy->baseY + bobOffset;
            GN_LOG_DEBUG("EnemySystem: Applied bobbing - baseY=" + std::to_string(enemy->baseY) + " bobOffset=" + std::to_string(bobOffset) + " final y=" + std::to_string(transform->position.y));
        } else {
            // Log if position has changed from baseY (shouldn't happen for static enemies)
            if (std::abs(transform->position.y - enemy->baseY) > 0.1f) {
                GN_LOG_DEBUG("EnemySystem: Position mismatch detected - baseY=" + std::to_string(enemy->baseY) + " current y=" + std::to_string(transform->position.y) + " at x=" + std::to_string(transform->position.x));
            }
        }
        // Handle screen wrapping for moving enemies
        if (enemy->movementPattern == "horizontal" || enemy->movementPattern == "snowman_thrower") {
            // Check if enemy is off-screen to the left
            if (transform->position.x < -100.0f) {
                // Wrap to the right side of the screen
                transform->position.x = 1279.0f + 100.0f; // iPhone 16 width + buffer
                GN_LOG_DEBUG("EnemySystem: Wrapped enemy to right side at x=" + std::to_string(transform->position.x));
            }
            // Check if enemy is off-screen to the right
            else if (transform->position.x > 1379.0f) {
                // Wrap to the left side of the screen
                transform->position.x = -100.0f;
                GN_LOG_DEBUG("EnemySystem: Wrapped enemy to left side at x=" + std::to_string(transform->position.x));
            }
        }
    }
}

void EnemySystem::UpdateEnemyAnimations(float deltaTime) {
    const auto enemies = m_levelManager->GetActiveEnemies();
    for (Gnosis::Entity e : enemies) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(e);
        Enemy* enemy = m_ecsSystem->GetComponent<Enemy>(e);
        if (!transform || !enemy || !enemy->isActive) continue;

        // Update animation timer
        if (enemy->isAnimated) {
            enemy->animationTimer += deltaTime;
            
            // Update frame
            if (enemy->animationTimer >= enemy->frameDuration) {
                enemy->currentFrame = (enemy->currentFrame + 1) % enemy->totalFrames;
                enemy->animationTimer = 0.0f;
            }
        }

        // Handle throw animation for snowman thrower
        if (enemy->isThrowing && enemy->isThrower) {
            enemy->throwAnimationTimer += deltaTime;
            
            // Update throw frame (6 frames total)
            if (enemy->throwAnimationTimer >= enemy->throwAnimationDuration / 6.0f) {
                enemy->currentThrowFrame = (enemy->currentThrowFrame + 1) % 6;
                enemy->throwAnimationTimer = 0.0f;
                
                // If throw animation is complete, return to idle
                if (enemy->currentThrowFrame == 0) {
                    enemy->isThrowing = false;
                    ChangeEnemyState(enemy, EnemyState::Idle);
                }
            }
        }
    }
}

void EnemySystem::UpdateSnowmanThrower(float deltaTime, Gnosis::Entity enemy, Enemy* enemyComp, Transform* transform) {
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
            Gnosis::Entity playerEntity = m_levelManager->GetPlayerEntity();
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

void EnemySystem::SpawnEnemyProjectile(Gnosis::Entity enemy, const Enemy* enemyComp, const Transform* transform) {
    // Create snowball projectile
    Gnosis::Entity projectile = m_ecsSystem->CreateEntity();
    
    // Add projectile component
    Projectile projComp;
    projComp.damage = enemyComp->damage;
    projComp.speed = 200.0f; // Snowball speed
    projComp.lifetime = 5.0f;
    projComp.isEnemyProjectile = true;
    projComp.affectedByGravity = true;
    projComp.gravity = 200.0f; // Snowball falls with gravity
    
    // Calculate direction towards player (simplified - can be enhanced)
    projComp.direction = Gnosis::GNVector2(-1.0f, -0.3f); // Left and slightly down
    
    m_ecsSystem->AddComponent<Projectile>(projectile, projComp);
    
    // Add transform component
    Transform projTransform;
    projTransform.position = transform->position;
    projTransform.position.x -= 30.0f; // Offset from enemy
    projTransform.position.y += 20.0f; // Launch from upper body area
    
    m_ecsSystem->AddComponent<Transform>(projectile, projTransform);
    
    // Add sprite component for snowball with proper animation
    Sprite projSprite;
    projSprite.textureId = "Snowball"; // Use existing snowball texture
    projSprite.width = 32.0f;
    projSprite.height = 32.0f;
    projSprite.layer = 5; // Above background, below UI
    projSprite.isAnimated = true;
    projSprite.frameWidth = 32;  // 32x32 frame size
    projSprite.frameHeight = 32;
    projSprite.frameCount = 4; // 4-frame snowball animation
    projSprite.currentFrame = 0;
    projSprite.frameTime = 0.1f; // 0.1 seconds per frame
    projSprite.playing = true;
    projSprite.loop = true;
    
    m_ecsSystem->AddComponent<Sprite>(projectile, projSprite);
    
    // Add physics component for movement
    Physics projPhysics;
    projPhysics.velocity = projComp.direction * projComp.speed;
    projPhysics.useGravity = projComp.affectedByGravity;
    
    m_ecsSystem->AddComponent<Physics>(projectile, projPhysics);
    
    // Store projectile reference
    m_enemyProjectiles.push_back(projectile);
}

void EnemySystem::UpdateEnemyProjectiles(float deltaTime) {
    for (auto it = m_enemyProjectiles.begin(); it != m_enemyProjectiles.end(); ++it) {
        Gnosis::Entity projectile = *it;
        
        Transform* transform = m_ecsSystem->GetComponent<Transform>(projectile);
        Projectile* projComp = m_ecsSystem->GetComponent<Projectile>(projectile);
        Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
        
        if (!transform || !projComp || !physics) continue;
        
        // Update lifetime
        projComp->currentLifetime += deltaTime;
        if (projComp->currentLifetime >= projComp->lifetime) {
            // Mark for cleanup
            m_ecsSystem->DestroyEntity(projectile);
            continue;
        }
        
        // Apply gravity if enabled
        if (projComp->affectedByGravity) {
            physics->velocity.y += projComp->gravity * deltaTime;
        }
        
        // Update position
        transform->position += physics->velocity * deltaTime;
        
        // Check if projectile is off screen
        // Use proper screen bounds for iPhone 16 (1179x2556)
        if (transform->position.x < -100.0f || transform->position.x > 1279.0f || 
            transform->position.y < -100.0f || transform->position.y > 2656.0f) {
            // Mark for cleanup
            m_ecsSystem->DestroyEntity(projectile);
            continue;
        }
    }
}

void EnemySystem::CleanupProjectiles() {
    // Remove destroyed projectiles from our list
    m_enemyProjectiles.erase(
        std::remove_if(m_enemyProjectiles.begin(), m_enemyProjectiles.end(),
            [this](Gnosis::Entity entity) {
                return !m_ecsSystem->IsEntityValid(entity);
            }),
        m_enemyProjectiles.end()
    );
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
        // Decorative enemies (like other snowmen) - no behavior
        enemy->currentState = EnemyState::Decorative;
        enemy->isActive = false; // Don't process them in update loops
        enemy->isGrounded = true;
        enemy->groundOffset = 0.0f;
        
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
    // But be careful not to double-subtract the sprite height!
    const float screenHeight = 2556.0f;
    
    // Get actual sprite height from the sprite component
    float enemyHeight = 64.0f; // Default fallback
    if (m_ecsSystem) {
        // Find the entity that has this transform to get its sprite
        const auto enemies = m_levelManager->GetActiveEnemies();
        for (Gnosis::Entity e : enemies) {
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

} // namespace GameCore


