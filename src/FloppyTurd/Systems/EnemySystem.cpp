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
            // Ground the enemy first
            GroundEnemy(enemy, transform);
            
            enemy->baseY = transform->position.y;
            enemy->spawnPosition = transform->position;
            enemy->hasInitializedBaseY = true;
            
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
        }
        
        // For grounded enemies, ensure they stay at the correct ground level
        // but don't override bobbing movement
        if (enemy->isGrounded && !enemy->bobbingEnabled) {
            GroundEnemy(enemy, transform);
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
    
    // Only throw when on screen and not already throwing
    if (enemyComp->isOnScreen && !enemyComp->isThrowing && enemyComp->currentState != EnemyState::Attacking) {
        enemyComp->throwTimer += deltaTime;
        
        // Check if it's time to throw
        if (enemyComp->throwTimer >= enemyComp->throwCooldown) {
            // Start throw animation
            enemyComp->isThrowing = true;
            enemyComp->currentThrowFrame = 0;
            enemyComp->throwAnimationTimer = 0.0f;
            ChangeEnemyState(enemyComp, EnemyState::Attacking, enemyComp->throwAnimationDuration);
            
            // Reset throw timer
            enemyComp->throwTimer = 0.0f;
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
        if (transform->position.x < -100.0f || transform->position.x > 900.0f || 
            transform->position.y < -100.0f || transform->position.y > 600.0f) {
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
        // Basic horizontal moving enemies
        enemy->currentState = EnemyState::Moving;
        enemy->bobbingEnabled = true;
        enemy->bobSpeed = 2.0f;
        enemy->bobAmplitude = 30.0f;
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
    
    // For side-scrolling games, enemies should be grounded at the bottom of the screen
    // Use the actual iPhone 16 screen height minus the sprite height
    const float screenHeight = 2556.0f; // iPhone 16 portrait screen height
    
    // Get the sprite component to calculate actual height with scale
    // For snowmen, use 64x64 sprite dimensions, but consider scale
    float enemyHeight = 64.0f * std::abs(transform->scale.y); // Snowman height with scale
    
    float groundY = screenHeight - enemyHeight;
    
    // Update base Y position for bobbing calculations
    enemy->baseY = groundY;
    
    // Set current position to ground
    transform->position.y = groundY;
}

} // namespace GameCore


