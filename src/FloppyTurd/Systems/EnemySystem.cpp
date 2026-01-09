#include "EnemySystem.h"
#include "BossSystem.h"
#include "../Game/FloppyTurdGame.h"
#include "LevelManager.h"
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

        // Skip boss enemy (Ratking) - handled by BossSystem, not EnemySystem
        bool isRatKing = (enemy->enemyType == "Ratking" || enemy->enemyType == "RatKing");
        if (isRatKing) {
            continue;
        }

        // Process all updates for this enemy in sequence:
        // 1. Update state (hurt timer, state transitions, initialization)
        ProcessEnemyState(deltaTime, enemyEntity, enemy, sprite);
        
        // If enemy was returned to pool during state update, skip further processing
        if (!enemy->isActive) continue;
        
        // 2. Update movement (physics, bobbing, wrapping)
        ProcessEnemyMovement(deltaTime, enemy, transform, enemyEntity);
        
        // 2.5. Update snowman thrower behavior (if applicable)
        if (enemy->movementPattern == "snowman_thrower" && enemy->isThrower) {
            UpdateSnowmanThrower(deltaTime, enemyEntity, enemy, transform);
        }
        
        // 3. Update animation (state-based switching, frame advancement)
        ProcessEnemyAnimation(deltaTime, enemy, sprite, stateAnim);
        
        // 4. Check collisions (only if not in hurt state, not decorative, and not a snowman)
        bool isSnowman = (enemy->enemyType.find("SnowMan") != std::string::npos || 
                         enemy->enemyType.find("Snowman") != std::string::npos);
        if (enemy->currentState != EnemyState::Hurt && 
            enemy->currentState != EnemyState::Decorative && 
            !isSnowman) {
            // Only log if there are projectiles to check
            if (activeProjectiles.size() > 0) {
                // Debug log for RatCopters
                if (enemy->movementPattern == "flying") {
                    static int ratCollisionCheckCounter = 0;
                    if (++ratCollisionCheckCounter % 100 == 0) {
                        GN_LOG_INFO("[RATCOPTER_COLLISION_CHECK] Enemy " + std::to_string(enemyEntity) + 
                                   " checking collision, projectiles=" + std::to_string(activeProjectiles.size()) + 
                                   ", state=" + std::to_string(static_cast<int>(enemy->currentState)) + 
                                   ", hasHitbox=" + std::to_string(hitbox != nullptr));
                    }
                }
                ProcessEnemyCollision(enemyEntity, enemy, transform, hitbox, sprite, stateAnim, activeProjectiles);
            }
        }
    }
}

void EnemySystem::UpdateSnowmanThrower(float deltaTime, Entity enemy, Enemy* enemyComp, Transform* transform) {
    // Check if enemy is on screen
    bool wasOnScreen = enemyComp->isOnScreen;
    enemyComp->isOnScreen = IsEnemyOnScreen(transform);
    
    // CRITICAL: Update flip state EVERY FRAME based on position vs player
    // This ensures flip happens immediately when snowman passes player
    Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
    if (playerEntity != 0) {
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
        if (playerTransform) {
            float snowmanX = transform->position.x;
            float playerX = playerTransform->position.x;
            bool wasFlipped = enemyComp->isFacingRight;
            
            // Update flip state based on position
            if (snowmanX < playerX) {
                enemyComp->isFacingRight = true;  // Passed player, face RIGHT
            } else {
                enemyComp->isFacingRight = false; // Haven't passed yet, face LEFT
            }
            
            // Apply flip to scale if changed
            if (wasFlipped != enemyComp->isFacingRight) {
                transform->scale.x = enemyComp->isFacingRight ? -std::abs(transform->scale.x) : std::abs(transform->scale.x);
                GN_LOG_INFO("[CONTINUOUS FLIP] Snowman " + std::to_string(enemy) + 
                           " flipped from " + std::string(wasFlipped ? "RIGHT" : "LEFT") + 
                           " to " + std::string(enemyComp->isFacingRight ? "RIGHT" : "LEFT") + 
                           " at x=" + std::to_string(snowmanX) + " (player at " + std::to_string(playerX) + ")");
            }
        }
    }
    
    // Update throw cooldown timer
    if (enemyComp->throwTimer > 0.0f) {
        enemyComp->throwTimer -= deltaTime;
    }
    
    // Check if snowman should start throwing
    // Wait until snowman is FULLY on screen before first throw
    const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
    const float SPRITE_WIDTH = 384.0f; // Snowman sprite width (64px * 6 scale)
    const float FULLY_ON_SCREEN = screenInfo.pixelWidth - SPRITE_WIDTH; // Fully visible
    
    bool canThrow = (enemyComp->throwTimer <= 0.0f); // Cooldown must be finished
    bool isFullyOnScreen = (transform->position.x <= FULLY_ON_SCREEN);
    bool shouldStartThrowing = !enemyComp->isThrowing && !enemyComp->hasThrownOnScreenEntry && canThrow && isFullyOnScreen;

    if (shouldStartThrowing) {
        // CRITICAL: Set flip state BEFORE starting throw animation based on player position
        Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
        if (playerEntity != 0) {
            Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
            if (playerTransform) {
                // CRITICAL: Flip ONLY when snowman X crosses to LEFT of player X
                // Snowman scrolls from right to left, so it starts at higher X than player
                float snowmanX = transform->position.x;
                float playerX = playerTransform->position.x;
                
                // Store previous flip state to detect changes
                bool wasFlipped = enemyComp->isFacingRight;
                
                // If snowman has passed player (scrolled to the left of player)
                // Simple X comparison: snowman.x < player.x means snowman is to the LEFT
                bool hasPassedPlayer = (snowmanX < playerX);
                
                GN_LOG_INFO("[INITIAL FLIP CHECK] snowmanX=" + std::to_string(snowmanX) + 
                           ", playerX=" + std::to_string(playerX) + 
                           ", hasPassedPlayer=" + std::to_string(hasPassedPlayer) + 
                           ", wasFlipped=" + std::to_string(wasFlipped) + 
                           ", currentScale.x=" + std::to_string(transform->scale.x));
                
                if (hasPassedPlayer) {
                    // Snowman has passed player - turn around (face RIGHT)
                    enemyComp->isFacingRight = true;
                } else {
                    // Snowman hasn't reached player yet - face LEFT (default)
                    enemyComp->isFacingRight = false;
                }
                
                // Apply the flip to scale
                // Metal renderer uses abs(scale) for positioning and flips via UV
                // So NO position compensation is needed!
                float oldScaleX = transform->scale.x;
                transform->scale.x = enemyComp->isFacingRight ? -std::abs(transform->scale.x) : std::abs(transform->scale.x);
                
                if (wasFlipped != enemyComp->isFacingRight) {
                    GN_LOG_INFO("[FLIP APPLIED] Changed from " + std::string(wasFlipped ? "RIGHT" : "LEFT") + 
                               " to " + std::string(enemyComp->isFacingRight ? "RIGHT" : "LEFT") + 
                               " at x=" + std::to_string(transform->position.x) + 
                               ", scale.x changed from " + std::to_string(oldScaleX) + 
                               " to " + std::to_string(transform->scale.x));
                }
                
                GN_LOG_INFO("[SNOWMAN] Initial throw - snowmanX=" + std::to_string(snowmanX) + 
                           ", playerX=" + std::to_string(playerX) + 
                           ", isFacingRight=" + std::to_string(enemyComp->isFacingRight));
            }
        }
        
        enemyComp->isThrowing = true;
        enemyComp->currentThrowFrame = 0;
        enemyComp->throwAnimationTimer = 0.0f;
        enemyComp->hasThrownOnScreenEntry = true;
        this->ChangeEnemyState(enemyComp, EnemyState::Attacking, enemyComp->throwAnimationDuration);

        GN_LOG_INFO("[SNOWMAN] Red snowman " + std::to_string(enemy) + " started throwing - position x=" + std::to_string(transform->position.x));

        // CRITICAL: Switch to throw animation using StateAnimation
        StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(enemy);
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
        if (sa && sprite) {
            const StateAnimation::Clip* throwClip = sa->getClip("throw");
            if (throwClip) {
                // Update StateAnimation state
                sa->currentState = "throw";
                
                // CRITICAL: Update sprite to match throw animation
                sprite->textureId = throwClip->textureId;
                sprite->isAnimated = (throwClip->frameCount > 1);
                sprite->frameWidth = throwClip->frameWidth;
                sprite->frameHeight = throwClip->frameHeight;
                sprite->frameCount = throwClip->frameCount;
                sprite->frameTime = throwClip->frameTime;
                sprite->loop = false; // Don't loop throw animation!
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                sprite->playing = true;
                sprite->hasCompleted = false;
                
                // PRESERVE flip state during animation switch
                transform->scale.x = enemyComp->isFacingRight ? -std::abs(transform->scale.x) : std::abs(transform->scale.x);
                
                GN_LOG_INFO("[SNOWMAN] Switched to THROW texture: " + sprite->textureId + 
                           ", frames=" + std::to_string(sprite->frameCount) + 
                           ", loop=false, playing=true");
            } else {
                GN_LOG_ERROR("[SNOWMAN] No 'throw' clip found in StateAnimation!");
            }
        } else {
            GN_LOG_ERROR("[SNOWMAN] Missing StateAnimation or Sprite component!");
        }
    }

    // Handle player proximity for additional throws (when turning around, etc.)
    // This triggers the "turn around and throw" behavior when player passes the snowman
    bool canCheckProximity = enemyComp->isOnScreen && !enemyComp->isThrowing && enemyComp->currentState != EnemyState::Attacking;
    
    if (canCheckProximity) {
        Entity playerEntity = 0;

        if (m_levelManager) {
            playerEntity = m_levelManager->GetPlayerEntity();

            if (playerEntity != 0) {
                Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                if (playerTransform) {
                    // Use player CENTER for comparison (64x64 sprite, add 32)
                    float playerCenterX = playerTransform->position.x + 32.0f;
                    float snowmanCenterX = transform->position.x + 32.0f;
                    float horizontalOffset = snowmanCenterX - playerCenterX;

                    // Screen width is 1170px, player is typically around x=500-600px
                    // When snowman enters from right edge (x=1170+), it should start throwing immediately
                    const float rightThrowRange = 2000.0f; // Very large range - throw as soon as on screen from right
                    const float leftThrowRange = 800.0f;   // Behind player range for turn-around throw

                    // Throw in two scenarios:
                    // 1. AHEAD of player (entering from right side): horizontalOffset > 0
                    // 2. BEHIND player (leaving left side): horizontalOffset < 0 (turn around and throw once more)
                    bool inRightRange = (horizontalOffset > 0 && horizontalOffset <= rightThrowRange);
                    bool inLeftRange = (horizontalOffset < 0 && horizontalOffset >= -leftThrowRange);
                    bool cooledDown = (enemyComp->throwTimer <= 0.0f); // Must wait for cooldown
                    
                    // DEBUG: Log proximity checks
                    static int logCounter = 0;
                    if (++logCounter % 60 == 0) { // Log every 60 frames to avoid spam
                        GN_LOG_INFO("[SNOWMAN PROXIMITY] centerOffset=" + std::to_string(horizontalOffset) + 
                                   ", inRight=" + std::to_string(inRightRange) + 
                                   ", inLeft=" + std::to_string(inLeftRange) +
                                   ", cooledDown=" + std::to_string(cooledDown));
                    }
                    
                    if ((inRightRange || inLeftRange) && cooledDown) {
                        
                        // Store previous flip state to detect changes
                        bool wasFlipped = enemyComp->isFacingRight;
                        
                        // CRITICAL: Flip ONLY when snowman X crosses to LEFT of player X
                        float snowmanX = transform->position.x;
                        float playerX = playerTransform->position.x;
                        
                        if (snowmanX < playerX) {
                            // Snowman has passed player - turn around (face RIGHT)
                            enemyComp->isFacingRight = true;
                        } else {
                            // Snowman is still approaching - face LEFT (default)
                            enemyComp->isFacingRight = false;
                        }
                        
                        // Apply the flip to scale
                        // Metal renderer uses abs(scale) for positioning, so no compensation needed
                        transform->scale.x = enemyComp->isFacingRight ? -std::abs(transform->scale.x) : std::abs(transform->scale.x);
                        
                        if (wasFlipped != enemyComp->isFacingRight) {
                            GN_LOG_INFO("[PROXIMITY FLIP] Changed from " + std::string(wasFlipped ? "RIGHT" : "LEFT") + 
                                       " to " + std::string(enemyComp->isFacingRight ? "RIGHT" : "LEFT") + 
                                       " at x=" + std::to_string(transform->position.x));
                        }
                        
                        // Don't throw if snowman is about to leave screen
                        // Only throw if still well within screen bounds
                        const float MIN_SCREEN_X = 200.0f; // Don't throw if closer than 200px to left edge
                        if (transform->position.x > MIN_SCREEN_X) {
                            // Start throw animation (flip state will be preserved)
                            enemyComp->isThrowing = true;
                            enemyComp->currentThrowFrame = 0;
                            enemyComp->throwAnimationTimer = 0.0f;
                            this->ChangeEnemyState(enemyComp, EnemyState::Attacking, enemyComp->throwAnimationDuration);
                        }
                        else {
                            GN_LOG_INFO("[SNOWMAN] Skipped proximity throw - too close to left edge (x=" + 
                                       std::to_string(transform->position.x) + ")");
                        }

                        std::string direction = (horizontalOffset > 0) ? "AHEAD (entering right)" : "BEHIND (leaving left, TURNING AROUND)";
                        GN_LOG_INFO("[SNOWMAN] Additional throw triggered by player proximity - " + direction +
                                   " - Player at x=" + std::to_string(playerTransform->position.x) +
                                   ", enemy at x=" + std::to_string(transform->position.x) +
                                   ", horizontalOffset=" + std::to_string(horizontalOffset) + "px" +
                                   ", isFacingRight=" + std::to_string(enemyComp->isFacingRight));
                        
                        // CRITICAL: Switch to throw animation for proximity throw too!
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
                                sprite->loop = false;
                                sprite->currentFrame = 0;
                                sprite->currentFrameTime = 0.0f;
                                sprite->playing = true;
                                sprite->hasCompleted = false;
                                transform->scale.x = enemyComp->isFacingRight ? -std::abs(transform->scale.x) : std::abs(transform->scale.x);
                                
                                GN_LOG_INFO("[SNOWMAN PROXIMITY] Switched to THROW texture: " + sprite->textureId);
                            }
                        }
                    }
                } else {
                    GN_LOG_WARN("[SNOWMAN] Player entity " + std::to_string(playerEntity) + " has NO Transform component!");
                }
            } else {
                static bool loggedOnce = false;
                if (!loggedOnce) {
                    GN_LOG_WARN("[SNOWMAN] No player entity available from LevelManager!");
                    loggedOnce = true;
                }
            }
        } else {
            GN_LOG_ERROR("[SNOWMAN] LevelManager is NULL! Cannot check player range.");
        }
    }

    // Handle throw animation and projectile spawning
    if (enemyComp->isThrowing && enemyComp->currentState == EnemyState::Attacking) {
        enemyComp->throwAnimationTimer += deltaTime;

        // Update throw animation frame (using 0.12s per frame like old script)
        int frameIndex = static_cast<int>(enemyComp->throwAnimationTimer / 0.12f);
        enemyComp->currentThrowFrame = std::min(frameIndex, enemyComp->totalFrames - 1);
        
        // Spawn projectile on FRAME 4 (like old script line 64) when snowball leaves hand
        const int THROW_RELEASE_FRAME = 4; // Frame where snowball leaves hand
        if (enemyComp->currentThrowFrame == THROW_RELEASE_FRAME && !enemyComp->hasSpawnedProjectile) {
            GN_LOG_INFO("[SNOWMAN] Frame " + std::to_string(THROW_RELEASE_FRAME) + " (RELEASE)! Spawning projectile... timer=" + 
                       std::to_string(enemyComp->throwAnimationTimer));
            this->SpawnEnemyProjectile(enemy, enemyComp, transform);
            enemyComp->hasSpawnedProjectile = true;
            GN_LOG_INFO("[SNOWMAN] Projectile spawned at RELEASE frame, hasSpawnedProjectile=true");
        }
        
        // Complete throw animation
        if (enemyComp->throwAnimationTimer >= enemyComp->throwAnimationDuration) {
            GN_LOG_INFO("[SNOWMAN] Throw animation COMPLETE - switching to idle (timer=" + 
                       std::to_string(enemyComp->throwAnimationTimer) + " >= " + 
                       std::to_string(enemyComp->throwAnimationDuration) + ")");
            
            // CRITICAL: Set cooldown timer WHEN ANIMATION COMPLETES (tied to animation)
            enemyComp->throwTimer = enemyComp->throwCooldown; // Start cooldown (e.g., 2.0s)
            enemyComp->isThrowing = false;
            enemyComp->hasSpawnedProjectile = false;
            
            // Switch back to idle texture when throw completes (preserving flip state)
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(enemy);
            StateAnimation* sa = m_ecsSystem->GetComponent<StateAnimation>(enemy);
            if (sprite && sa) {
                const StateAnimation::Clip* idleClip = sa->getClip("idle");
                if (idleClip) {
                    // Update StateAnimation state
                    sa->currentState = "idle";
                    
                    // Update sprite to match idle animation
                    sprite->textureId = idleClip->textureId;
                    sprite->isAnimated = (idleClip->frameCount > 1);
                    sprite->frameWidth = idleClip->frameWidth;
                    sprite->frameHeight = idleClip->frameHeight;
                    sprite->frameCount = idleClip->frameCount;
                    sprite->frameTime = idleClip->frameTime;
                    sprite->loop = true; // Idle loops!
                    sprite->currentFrame = 0;
                    sprite->currentFrameTime = 0.0f;
                    sprite->playing = true;
                    sprite->hasCompleted = false;
                    
                    // PRESERVE flip state during animation switch
                    transform->scale.x = enemyComp->isFacingRight ? -std::abs(transform->scale.x) : std::abs(transform->scale.x);
                    
                    GN_LOG_INFO("[SNOWMAN] Switched to IDLE texture: " + sprite->textureId + 
                               ", cooldown=" + std::to_string(enemyComp->throwCooldown) + "s");
                }
            }
            
            this->ChangeEnemyState(enemyComp, EnemyState::Idle, 0.0f);
        }
    }
}

bool EnemySystem::IsEnemyOnScreen(const Transform* transform) {
    // On-screen check with extended bounds to align with wrap buffer
    // EXTENDED: Use 400px left buffer to match wrapping logic
    const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
    const float leftBound = -400.0f; // Extended buffer to match wrap threshold
    const float rightBound = screenInfo.pixelWidth + 200.0f; // Allow some right buffer
    
    if (!transform) return false;
    return (transform->position.x >= leftBound && transform->position.x <= rightBound);
}

void EnemySystem::ChangeEnemyState(Enemy* enemy, EnemyState newState, float duration) {
    if (!enemy) return;
    enemy->currentState = newState;
    enemy->stateTimer = 0.0f;
    enemy->stateDuration = duration;
}

void EnemySystem::SpawnEnemyProjectile(Entity enemy, const Enemy* enemyComp, const Transform* transform) {
    if (!enemyComp || !transform || !m_projectileSystem) return;
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

    // SNAPSHOT: Get player CENTER position at THIS EXACT FRAME (flash frame targeting)
    Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
    GNVector2 playerSnapshotPos(0.0f, 0.0f);
    bool hasValidPlayerPos = false;
    
    if (playerEntity != 0) {
        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
        Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(playerEntity);
        if (playerTransform && playerSprite) {
            // CRITICAL: Use CENTER of player sprite, not top-left!
            // Player sprite is 64x64, position is top-left, so add 32 to both X and Y
            const float playerHalfWidth = 32.0f;  // 64/2
            const float playerHalfHeight = 32.0f; // 64/2
            playerSnapshotPos.x = playerTransform->position.x + playerHalfWidth;
            playerSnapshotPos.y = playerTransform->position.y + playerHalfHeight;
            hasValidPlayerPos = true;
            
            GN_LOG_INFO("[PROJECTILE] Player CENTER snapshot: (" + std::to_string(playerSnapshotPos.x) + 
                       ", " + std::to_string(playerSnapshotPos.y) + ") [top-left was (" + 
                       std::to_string(playerTransform->position.x) + ", " + 
                       std::to_string(playerTransform->position.y) + ")]");
        }
    }

    // Calculate spawn position (offset from snowman, accounting for flip direction)
    GNVector2 snowmanPos = transform->position;
    
    // Spawn in FRONT of snowman based on facing direction
    // If facing LEFT (normal): spawn to LEFT (in front) → negative offset
    // If facing RIGHT (flipped): spawn to RIGHT (in front) → positive offset
    float xOffset = enemyComp->isFacingRight ? 50.0f : -50.0f;
    float yOffset = 100.0f; // Spawn below snowman (hand position, +Y = down)
    GNVector2 spawnPosition(snowmanPos.x + xOffset, snowmanPos.y + yOffset);
    
    // MULTI-FRAME ANALYSIS: Log frame-by-frame data
    static int frameCounter = 0;
    frameCounter++;
    
    GN_LOG_INFO("[FRAME " + std::to_string(frameCounter) + "] Snowman pos=(" + 
               std::to_string(snowmanPos.x) + ", " + std::to_string(snowmanPos.y) + 
               "), facing=" + std::string(enemyComp->isFacingRight ? "RIGHT" : "LEFT") + 
               ", spawn=(" + std::to_string(spawnPosition.x) + ", " + std::to_string(spawnPosition.y) + ")");

    // Calculate direction using simple normalization (like old snowman script)
    GNVector2 finalDirection(-300.0f, 0.0f); // Default: throw left horizontally
    
    if (hasValidPlayerPos) {
        // Calculate delta vector
        float dx = playerSnapshotPos.x - spawnPosition.x;
        float dy = playerSnapshotPos.y - spawnPosition.y;
        
        // NO VERTICAL REJECTION - throw at any angle!
        // User wants leading shots even when player is directly above
        {
            // BALLISTIC TRAJECTORY: Make apex reach player's Y position
            // Coordinate system: +Y = DOWN (Metal/iOS standard)
            // VARIABLE GRAVITY TIERS: Use different gravity for different speeds
            // Higher gravity = faster movement through SAME arc (no overshooting!)
            
            // Choose gravity tier randomly (3 tiers for variety)
            int speedTier = rand() % 3; // 0, 1, or 2
            float gravity;
            
            if (speedTier == 0) {
                gravity = 350.0f;  // Normal speed
            } else if (speedTier == 1) {
                gravity = 450.0f;  // Faster (28% quicker)
            } else {
                gravity = 550.0f;  // Very fast (57% quicker)
            }
            
            GN_LOG_INFO("[SNOWBALL SPEED TIER] Tier=" + std::to_string(speedTier) + 
                       ", gravity=" + std::to_string(gravity));
            
            // Vertical distance (player Y - spawn Y)
            // If player ABOVE spawn: dy is negative (player has smaller Y value)
            // If player BELOW spawn: dy is positive (player has larger Y value)
            // +Y is DOWN, so negative dy means we need to go UP
            
            // Time to reach player's Y (apex):
            // If going upward (negative dy): v_y must be negative initially
            // Physics: v_final = v_initial + g*t, at apex v_final = 0
            // So: 0 = v_y + g*t → t = -v_y / g
            // Also: dy = v_y*t + 0.5*g*t²
            
            float timeToApex;
            float v_y;
            
            if (dy < 0) {
                // Player is ABOVE spawn (smaller Y value)
                // Need to go UP (negative velocity in +Y=down system)
                // Calculate initial upward velocity to reach player Y
                // Using: v² = 2*g*distance
                v_y = -std::sqrt(2.0f * gravity * std::abs(dy));
                timeToApex = std::abs(v_y) / gravity;
            } else {
                // Player is BELOW spawn (larger Y value)
                // Just aim downward, gravity will help
                timeToApex = std::sqrt(2.0f * dy / gravity);
                v_y = 0.0f; // Start with zero, gravity will accelerate down
            }
            
            // Horizontal velocity to reach player X at the same time
            float v_x = dx / timeToApex;
            
            // NO speed multiplier - arc shape stays exactly the same
            // Faster movement is achieved by higher gravity (350 vs 200)
            // This makes the snowball move faster along the SAME trajectory
            finalDirection.x = v_x;
            finalDirection.y = v_y;
            
            GN_LOG_INFO("[SNOWBALL TRAJECTORY] gravity=" + std::to_string(gravity) + 
                       ", timeToApex=" + std::to_string(timeToApex) + "s" +
                       ", velocity=(" + std::to_string(finalDirection.x) + "," + 
                       std::to_string(finalDirection.y) + ")");
            
            // Calculate angle for logging
            float angle = std::atan2(dy, dx) * 180.0f / 3.14159f;
            
            GN_LOG_INFO("[FRAME " + std::to_string(frameCounter) + " BALLISTIC] dx=" + std::to_string(dx) + 
                       ", dy=" + std::to_string(dy) + 
                       ", timeToApex=" + std::to_string(timeToApex) + "s" +
                       ", angle=" + std::to_string(angle) + "°" +
                       ", velocity=(" + std::to_string(finalDirection.x) + ", " + 
                       std::to_string(finalDirection.y) + ")" +
                       " [APEX at player Y=" + std::to_string(playerSnapshotPos.y) + "]");
        }
    } else {
        GN_LOG_WARN("[PROJECTILE] No valid player position, using default direction");
    }

    // Spawn projectile using the ProjectileSystem
    Entity projectileEntity = m_projectileSystem->SpawnEnemyProjectile(
        spawnPosition,
        finalDirection,
        projectileType,
        enemyComp->damage
    );

    if (projectileEntity != 0) {
        GN_LOG_INFO("Enemy " + std::to_string(enemy) + " spawned projectile " + std::to_string(projectileEntity) + 
                   " at (" + std::to_string(spawnPosition.x) + ", " + std::to_string(spawnPosition.y) + ")");
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
        enemy->frameDuration = 0.12f; // 0.12 seconds per frame (match old script)
        enemy->throwCooldown = 2.0f; // 2 seconds between throws
        enemy->throwRange = 400.0f; // Start throwing when player is within 400 pixels
        enemy->currentState = EnemyState::Idle;
        enemy->isGrounded = true;
        enemy->groundOffset = 0.0f; // Snowmen sit directly on ground
        
        // Set up throw animation timing (0.72s total like old script)
        enemy->throwAnimationDuration = 0.72f; // Total throw animation time (6 frames * 0.12s)
        
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
        enemy->isGrounded = false;  // Not grounded - they fly/bob freely
        enemy->groundOffset = 0.0f;
        
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
        
    } else if (movementPattern == "flying") {
        // RatCopters - flying enemies with state machine behavior
        enemy->currentState = EnemyState::FlyIn;
        enemy->isGrounded = false; // NOT grounded - they fly freely in the air!
        enemy->groundOffset = 0.0f;
        enemy->bobbingEnabled = false; // No bobbing - sprite animation provides hover effect
        enemy->speed = 120.0f; // Base fly-in speed
        
    } else if (movementPattern == "rat_swarm") {
        // RAT SWARM: Galaga-style behavior with staggered attacks and Y-split positioning
        enemy->currentState = EnemyState::FlyIn;
        enemy->isGrounded = false;
        enemy->groundOffset = 0.0f;
        enemy->bobbingEnabled = false;
        enemy->speed = 180.0f; // Faster fly-in than before
        
        // Static counter to group rats and assign formation positions
        static int ratSwarmCounter = 0;
        int groupIndex = ratSwarmCounter % 3;
        ratSwarmCounter++;
        
        // Stagger attack delays: 0s, 0.4s, 0.8s (tighter timing)
        enemy->swarmAttackDelay = static_cast<float>(groupIndex) * 0.4f;
        enemy->formationIndex = groupIndex;
        
        // Y offset for multi-rat positioning (split vertically around player)
        // 0 = center, 1 = above, 2 = below
        // These offsets will be applied to the tracked player Y position
        switch (groupIndex) {
            case 0: enemy->formationOffset.y = 0.0f; break;      // Center
            case 1: enemy->formationOffset.y = -100.0f; break;   // Above player
            case 2: enemy->formationOffset.y = 100.0f; break;    // Below player
        }
        
        GN_LOG_INFO("[RAT_SWARM] Initialized rat " + std::to_string(groupIndex) + 
                   " with delay " + std::to_string(enemy->swarmAttackDelay) + 
                   "s, yOffset=" + std::to_string(enemy->formationOffset.y));

    } else if (movementPattern == "galaga") {
        // GALAGA: Toilet Paper specific pattern (FlyIn -> Hover -> Attack)
        enemy->currentState = EnemyState::FlyIn;
        enemy->isGrounded = false; 
        enemy->groundOffset = 0.0f;
        enemy->bobbingEnabled = true; // Enabled for hover phase
        enemy->bobAmplitude = 20.0f;  // Reduced amplitude (shorter waves)
        enemy->bobSpeed = 3.0f;       // Faster, jittery waves
        enemy->galagaHoverTimer = 2.0f; // 2 seconds hover
        enemy->speed = 250.0f;        // Fast fly-in speed
        
        // Target X: 80% of screen width (20% from right)
        const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        enemy->galagaFlyInTargetX = screenInfo.pixelWidth * 0.80f;
        enemy->galagaHoverComplete = false;
        
    } else if (movementPattern == "echelon") {
        // ECHELON: Bird formation flying
        enemy->currentState = EnemyState::Moving;
        enemy->isGrounded = false;
        enemy->bobbingEnabled = true;
        enemy->bobSpeed = 2.0f;
        enemy->bobAmplitude = 60.0f;
        enemy->speed = 150.0f;
        
        // Static counter for V-formation (groups of 5)
        static int birdFormationCounter = 0;
        int posInV = birdFormationCounter % 5; // 0=Tip, 1,2=Upper Wing, 3,4=Lower Wing
        birdFormationCounter++;
        
        // Calculate offsets for V-shape
        // 0: (0, 0)
        // 1: (40, -40)  - Upper 1
        // 2: (80, -80)  - Upper 2
        // 3: (40, 40)   - Lower 1
        // 4: (80, 80)   - Lower 2
        
        float xOff = 0.0f;
        float yOff = 0.0f;
        
        switch (posInV) {
            case 0: xOff = 0.0f; yOff = 0.0f; break;
            case 1: xOff = 60.0f; yOff = -50.0f; break;
            case 2: xOff = 120.0f; yOff = -100.0f; break;
            case 3: xOff = 60.0f; yOff = 50.0f; break;
            case 4: xOff = 120.0f; yOff = 100.0f; break;
        }
        
        enemy->formationOffset = GNVector2(xOff, yOff);
        
        // Adjust initial position based on formation
        // Note: transform isn't passed here, so we set spawnPosition logic in ProcessMovement or apply it if we could
        // Since we don't have transform here, we store offset and apply it in ProcessMovement relative to "virtual" leader position
        // OR better: LevelManager could handle this, but for now we'll just carry the offset.
        
    } else {
        // Default behavior
        enemy->currentState = EnemyState::Idle;
        enemy->isGrounded = true;
        enemy->groundOffset = 0.0f;
    }
}

void EnemySystem::GroundEnemy(Enemy* enemy, Transform* transform) {
    // CRITICAL: Never ground flying or horizontal enemies (birds, ratcopters)
    if (enemy->movementPattern == "flying" || enemy->movementPattern == "horizontal" || 
        enemy->movementPattern == "galaga" || enemy->movementPattern == "rat_swarm" ||
        enemy->movementPattern == "echelon") {
        enemy->isGrounded = false;
        return;
    }
    
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
            // Hurt animation completed - handle based on movement pattern
            if (enemy->movementPattern == "echelon") {
                // ECHELON BIRDS: Stay in active list but become invisible
                // They will be resurrected when the group wraps around
                Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(e);
                if (sprite) {
                    sprite->visible = false;
                    sprite->color.a = 0;
                    sprite->playing = false;
                }
                enemy->currentState = EnemyState::Idle; // Reset state
                enemy->health = 0; // Mark as dead
                GN_LOG_INFO("[ECHELON_DEATH] Bird " + std::to_string(e) + " died, hidden but stays in active list for resurrection on wrap");
            } else {
                // OTHER ENEMIES: Return to inactive pool for reuse
                if (m_levelManager) {
                    m_levelManager->ReturnEnemyToPool(e);
                }
                GN_LOG_INFO("Enemy " + std::to_string(e) + " hurt animation completed, returned to inactive pool");
            }
        }
    }

    // Check if state should change
    if (enemy->stateDuration > 0.0f && enemy->stateTimer >= enemy->stateDuration) {
        // Return to idle after state duration
        this->ChangeEnemyState(enemy, EnemyState::Idle);
    }
}

void EnemySystem::ProcessEnemyMovement(float deltaTime, Enemy* enemy, Transform* transform, Entity enemyEntity) {
    if (!enemy || !transform) return;
    
    // CRITICAL: Freeze all enemy movement when boss is dying (Level 6 only)
    if (m_bossSystem && m_levelManager) {
        int currentLevel = m_levelManager->GetCurrentLevelId();
        if (currentLevel == 6) {
            // Check if boss is in death state or death sequence has started
            RatKingState bossState = m_bossSystem->GetCurrentState();
            bool bossDying = (bossState == RatKingState::DEATH || m_bossSystem->IsDeathSequenceComplete());
            if (bossDying) {
                // Freeze enemy in place - no movement during boss death sequence
                return;
            }
        }
    }

    // Get screen info for device-agnostic dimensions
    const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();

    // SPECIAL CASE: RatCopter "flying" pattern with state machine (FLY_IN → HOVER → PULLBACK → BEELINE)
    if (enemy->movementPattern == "flying") {
        // CRITICAL: Skip all movement when in Hurt state - let hurt animation play
        if (enemy->currentState == EnemyState::Hurt) {
            return; // Don't process movement, let hurt animation complete
        }
        
        // State machine for RatCopter behavior
        switch (enemy->currentState) {
            case EnemyState::FlyIn: {
                // FLY_IN: Move left until 10-20% from right edge (80-90% across screen)
                // User wants pullback to START at 80-90% screen position
                float oldX = transform->position.x;
                transform->position.x -= enemy->speed * deltaTime;
                
                // TRACK PLAYER Y during fly-in for better alignment
                Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
                if (playerEntity != 0) {
                    Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                    Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(playerEntity);
                    if (playerTransform) {
                        // Get player center Y
                        float playerCenterY = playerTransform->position.y;
                        if (playerSprite) {
                            playerCenterY += (playerSprite->height * std::abs(playerTransform->scale.y)) * 0.5f;
                        }
                        
                        // Smoothly move toward player Y
                        float yDiff = playerCenterY - transform->position.y;
                        float trackSpeed = 5.0f; // Higher value for responsive tracking
                        transform->position.y += yDiff * trackSpeed * deltaTime;
                        
                        // Update baseY to match current position for hover transition
                        enemy->baseY = transform->position.y;
                        
                        // Constrain to screen bounds
                        const float topPadding = 100.0f;
                        const float bottomPadding = 100.0f;
                        transform->position.y = std::max(topPadding, std::min(screenInfo.pixelHeight - bottomPadding, transform->position.y));
                    }
                }
                
                // LOG: Track fly-in progress every 100 frames
                static int flyInLogCounter = 0;
                if (++flyInLogCounter % 100 == 0) {
                    GN_LOG_INFO("[RATCOPTER FLY_IN] Pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + 
                               "), Speed=" + std::to_string(enemy->speed) + ", Delta=" + std::to_string(deltaTime) + 
                               ", MovedBy=" + std::to_string(oldX - transform->position.x));
                }
                
                // Transition to HOVER when rat reaches 85% across screen (15% from right edge)
                // This gives time for hover animation before pullback starts
                float targetX = screenInfo.pixelWidth * 0.85f; // 15% from right edge
                if (transform->position.x <= targetX) {
                    enemy->currentState = EnemyState::Hover;
                    // Random hover duration: 0.75s to 1.0s
                    enemy->hoverTimer = 0.75f + (static_cast<float>(rand() % 26) / 100.0f);
                    GN_LOG_INFO("[RATCOPTER FLY_IN→HOVER] At " + std::to_string((transform->position.x / screenInfo.pixelWidth) * 100.0f) + 
                                "% screen, timer=" + std::to_string(enemy->hoverTimer) + "s, Y=" + std::to_string(transform->position.y));
                }
                break;
            }
            
            case EnemyState::Hover: {
                // HOVER: Stay in place with bobbing, countdown timer
                float oldY = transform->position.y;
                
                // TRACK PLAYER Y during hover - update baseY to follow player
                Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
                if (playerEntity != 0) {
                    Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                    Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(playerEntity);
                    if (playerTransform) {
                        // Get player center Y
                        float playerCenterY = playerTransform->position.y;
                        if (playerSprite) {
                            playerCenterY += (playerSprite->height * std::abs(playerTransform->scale.y)) * 0.5f;
                        }
                        
                        // Smoothly move baseY toward player Y
                        float yDiff = playerCenterY - enemy->baseY;
                        float trackSpeed = 4.0f; // Responsive tracking during hover
                        enemy->baseY += yDiff * trackSpeed * deltaTime;
                        
                        // Constrain baseY to screen bounds
                        const float topPadding = 100.0f;
                        const float bottomPadding = 100.0f;
                        enemy->baseY = std::max(topPadding, std::min(screenInfo.pixelHeight - bottomPadding, enemy->baseY));
                    }
                }
                
                // Apply bobbing for hover effect
                if (enemy->bobbingEnabled) {
                    float bobOffset = std::sin(m_time * enemy->bobSpeed + enemy->bobPhase) * enemy->bobAmplitude;
                    transform->position.y = enemy->baseY + bobOffset;
                    
                    // Constrain Y to safe bounds
                    const float topPadding = 100.0f;
                    const float bottomPadding = 100.0f;
                    const float minY = topPadding;
                    const float maxY = screenInfo.pixelHeight - bottomPadding;
                    float unconstrainedY = transform->position.y;
                    transform->position.y = std::max(minY, std::min(maxY, transform->position.y));
                    
                    // LOG: Warn if Y gets constrained (indicates baseY is wrong)
                    if (unconstrainedY != transform->position.y) {
                        GN_LOG_WARN("[RATCOPTER HOVER] Y constrained! baseY=" + std::to_string(enemy->baseY) + 
                                   ", bobOffset=" + std::to_string(bobOffset) + ", unconstrained=" + std::to_string(unconstrainedY) + 
                                   ", final=" + std::to_string(transform->position.y));
                    }
                }
                
                // Countdown hover timer
                enemy->hoverTimer -= deltaTime;
                
                // LOG: Track hover progress every 30 frames
                static int hoverLogCounter = 0;
                if (++hoverLogCounter % 30 == 0) {
                    GN_LOG_INFO("[RATCOPTER HOVER] Pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + 
                               "), BaseY=" + std::to_string(enemy->baseY) + ", Timer=" + std::to_string(enemy->hoverTimer) + 
                               "s, BobAmplitude=" + std::to_string(enemy->bobAmplitude));
                }
                
                // Transition to PULLBACK when timer expires
                if (enemy->hoverTimer <= 0.0f) {
                    enemy->currentState = EnemyState::Pullback;
                    enemy->pullbackTimer = 0.25f;
                    
                    // SNAPSHOT: Get player position for targeting (same approach as snowballs)
                    Gnosis::Entity playerEntity = m_levelManager->GetPlayerEntity();
                    Gnosis::GNVector2 playerPos(0.0f, 0.0f);
                    Gnosis::GNVector2 ratPos = transform->position;
                    bool hasValidPlayerPos = false;
                    
                    if (playerEntity != 0) {
                        Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                        if (playerTransform) {
                            // Simple position - top-left of player sprite
                            playerPos.x = playerTransform->position.x;
                            playerPos.y = playerTransform->position.y;
                            hasValidPlayerPos = true;
                        }
                    }
                    
                    if (hasValidPlayerPos) {
                        // Calculate delta vector: player MINUS rat (same as snowball targeting)
                        float dx = playerPos.x - ratPos.x;
                        float dy = playerPos.y - ratPos.y;
                        
                        // Normalize the direction
                        float length = std::sqrt(dx * dx + dy * dy);
                        if (length > 0.001f) {
                            enemy->targetDirection.x = dx / length;
                            enemy->targetDirection.y = dy / length;
                        } else {
                            enemy->targetDirection.x = -1.0f;
                            enemy->targetDirection.y = 0.0f;
                        }
                        
                        GN_LOG_INFO("[RATCOPTER TARGET] Rat(" + std::to_string(ratPos.x) + "," + std::to_string(ratPos.y) + 
                                   ") → Player(" + std::to_string(playerPos.x) + "," + std::to_string(playerPos.y) + 
                                   ") | Delta(" + std::to_string(dx) + "," + std::to_string(dy) + 
                                   ") | Dir(" + std::to_string(enemy->targetDirection.x) + "," + std::to_string(enemy->targetDirection.y) + ")");
                    } else {
                        // Fallback: aim left
                        enemy->targetDirection.x = -1.0f;
                        enemy->targetDirection.y = 0.0f;
                    }
                    
                    // Calculate pullback vector: OPPOSITE of direction to player, scaled by 20 units (like old script)
                    enemy->pullbackVector.x = -enemy->targetDirection.x * 20.0f;
                    enemy->pullbackVector.y = -enemy->targetDirection.y * 20.0f;
                    enemy->hasLockedDirection = true;
                    
                    GN_LOG_INFO("[RATCOPTER HOVER→PULLBACK] Pullback vector=(" + std::to_string(enemy->pullbackVector.x) + 
                               "," + std::to_string(enemy->pullbackVector.y) + "), Duration=0.25s");
                }
                break;
            }
            
            case EnemyState::Pullback: {
                // PULLBACK: Move in pullback direction (opposite of player) for 0.25 seconds
                // Old raylib: pos = Vector2Add(pos, Vector2Scale(pullbackVector, deltaTime * 4.0f))
                float oldX = transform->position.x;
                float oldY = transform->position.y;
                transform->position.x += enemy->pullbackVector.x * deltaTime * 4.0f;
                transform->position.y += enemy->pullbackVector.y * deltaTime * 4.0f;
                
                // LOG: Track pullback movement every 5 frames
                static int pullbackLogCounter = 0;
                if (++pullbackLogCounter % 5 == 0) {
                    GN_LOG_INFO("[RATCOPTER PULLBACK] Pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + 
                               "), MovedBy=(" + std::to_string(transform->position.x - oldX) + "," + std::to_string(transform->position.y - oldY) + 
                               "), Timer=" + std::to_string(enemy->pullbackTimer) + "s");
                }
                
                // Countdown pullback timer
                enemy->pullbackTimer -= deltaTime;
                
                // Transition to BEELINE when timer expires
                if (enemy->pullbackTimer <= 0.0f) {
                    enemy->currentState = EnemyState::Beeline;
                    // Beeline speed increased to 1000 units/sec for extremely fast, aggressive charge
                    enemy->beelineSpeed = 1000.0f;
                    
                    GN_LOG_INFO("[RATCOPTER PULLBACK→BEELINE] Speed=" + std::to_string(enemy->beelineSpeed) + 
                               ", Dir=(" + std::to_string(enemy->targetDirection.x) + "," + std::to_string(enemy->targetDirection.y) + 
                               "), StartPos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ")");
                }

                break;
            }
            
            case EnemyState::Beeline: {
                // BEELINE: Charge in the LOCKED direction (toward where player WAS)
                // Old raylib: pos = Vector2Add(pos, Vector2Scale(direction, speed * deltaTime))
                float oldX = transform->position.x;
                float oldY = transform->position.y;
                transform->position.x += enemy->targetDirection.x * enemy->beelineSpeed * deltaTime;
                transform->position.y += enemy->targetDirection.y * enemy->beelineSpeed * deltaTime;
                
                // LOG: Track beeline movement every 20 frames
                static int beelineLogCounter = 0;
                if (++beelineLogCounter % 20 == 0) {
                    GN_LOG_INFO("[RATCOPTER BEELINE] Pos=(" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + 
                               "), Speed=" + std::to_string(enemy->beelineSpeed) + ", Dir=(" + std::to_string(enemy->targetDirection.x) + 
                               "," + std::to_string(enemy->targetDirection.y) + "), MovedBy=(" + 
                               std::to_string(transform->position.x - oldX) + "," + std::to_string(transform->position.y - oldY) + ")");
                }
                
                // Check if rat has left the screen (let wrap system handle reset)
                const float margin = 100.0f;
                if (transform->position.x < -margin || 
                    transform->position.x > screenInfo.pixelWidth + margin ||
                    transform->position.y < -margin || 
                    transform->position.y > screenInfo.pixelHeight + margin) {
                    GN_LOG_INFO("[RATCOPTER BEELINE→OFFSCREEN] At (" + std::to_string(transform->position.x) + 
                                "," + std::to_string(transform->position.y) + "), will wrap");
                }
                break;
            }
            
            default:
                // Fallback to Idle/FlyIn state if in unexpected state
                GN_LOG_ERROR("[RATCOPTER ERROR] Unexpected state! Resetting to FlyIn. Pos=(" + 
                            std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ")");
                enemy->currentState = EnemyState::FlyIn;
                break;
        }
    } else if (enemy->movementPattern == "rat_swarm") {
        // RAT SWARM LOGIC (Galaga-style: FlyIn -> Hover -> Pullback -> Beeline)
        // CRITICAL: Fully decoupled from world scroll like galaga enemies
        
        const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        
        // Counter-act world scroll to stay fixed in screen space
        float worldScrollSpeed = m_levelManager->GetCurrentLevelConfig().worldSpeed;
        
        switch (enemy->currentState) {
            case EnemyState::Idle:
                // Auto-start
                enemy->currentState = EnemyState::FlyIn;
                break;
                
            case EnemyState::FlyIn: {
                // Counter world scroll to stay at fixed X position during fly-in
                transform->position.x += worldScrollSpeed * deltaTime;
                
                // TRACK PLAYER Y during fly-in (center-to-center alignment)
                Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
                if (playerEntity != 0) {
                    Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                    Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(playerEntity);
                    Sprite* enemySprite = m_ecsSystem->GetComponent<Sprite>(enemyEntity);
                    if (playerTransform) {
                        // Get player center Y
                        float playerCenterY = playerTransform->position.y;
                        if (playerSprite) {
                            playerCenterY += (playerSprite->height * std::abs(playerTransform->scale.y)) * 0.5f;
                        }
                        
                        // Get enemy center Y (using actual sprite size)
                        float enemyHalfHeight = 16.0f; // Default for 32x32 RatCopter
                        if (enemySprite) {
                            enemyHalfHeight = (enemySprite->height * std::abs(transform->scale.y)) * 0.5f;
                        }
                        float enemyCenterY = transform->position.y + enemyHalfHeight;
                        
                        // Apply formation Y offset for multi-rat positioning
                        float targetY = playerCenterY + enemy->formationOffset.y;
                        
                        // Smoothly move CENTER toward target
                        float yDiff = targetY - enemyCenterY;
                        float trackSpeed = 5.0f; // Faster than galaga for snappier rats
                        float newCenterY = enemyCenterY + yDiff * trackSpeed * deltaTime;
                        transform->position.y = newCenterY - enemyHalfHeight;
                    }
                }
                
                // Fly in toward target X (85% screen width)
                float targetX = screenInfo.pixelWidth * 0.85f;
                if (transform->position.x > targetX) {
                    float flySpeed = enemy->speed;
                    transform->position.x -= flySpeed * deltaTime;
                } else {
                    // Reached hover position
                    enemy->currentState = EnemyState::Hover;
                    enemy->baseY = transform->position.y;
                    // BASE hover time + specific swarm delay for staggered attacks
                    enemy->hoverTimer = 1.0f + enemy->swarmAttackDelay;
                    
                    GN_LOG_INFO("[RAT_SWARM] Entered HOVER. Attack in " + std::to_string(enemy->hoverTimer) + "s");
                }
                break;
            }
            
            case EnemyState::Hover: {
                // Counter world scroll to stay at fixed X
                transform->position.x += worldScrollSpeed * deltaTime;
                
                // TRACK PLAYER Y during hover (center-to-center)
                Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
                if (playerEntity != 0) {
                    Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                    Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(playerEntity);
                    Sprite* enemySprite = m_ecsSystem->GetComponent<Sprite>(enemyEntity);
                    if (playerTransform) {
                        // Get player center Y
                        float playerCenterY = playerTransform->position.y;
                        if (playerSprite) {
                            playerCenterY += (playerSprite->height * std::abs(playerTransform->scale.y)) * 0.5f;
                        }
                        
                        // Get enemy center Y
                        float enemyHalfHeight = 16.0f;
                        if (enemySprite) {
                            enemyHalfHeight = (enemySprite->height * std::abs(transform->scale.y)) * 0.5f;
                        }
                        float enemyCenterY = transform->position.y + enemyHalfHeight;
                        
                        // Apply formation Y offset
                        float targetY = playerCenterY + enemy->formationOffset.y;
                        
                        // Smoothly track target
                        float yDiff = targetY - enemyCenterY;
                        float trackSpeed = 4.0f;
                        float newCenterY = enemyCenterY + yDiff * trackSpeed * deltaTime;
                        transform->position.y = newCenterY - enemyHalfHeight;
                        enemy->baseY = transform->position.y;
                    }
                }
                
                enemy->hoverTimer -= deltaTime;
                if (enemy->hoverTimer <= 0.0f) {
                    enemy->currentState = EnemyState::Pullback;
                    enemy->pullbackTimer = 0.3f; // Short pullback
                    
                    // Target player position for beeline
                    Gnosis::Entity playerEntity = m_levelManager->GetPlayerEntity();
                    Gnosis::GNVector2 playerPos(0.0f, 0.0f);
                    bool hasPlayer = false;
                    if (playerEntity != 0) {
                        Transform* pt = m_ecsSystem->GetComponent<Transform>(playerEntity);
                        if (pt) { playerPos = pt->position; hasPlayer = true; }
                    }
                    
                    if (hasPlayer) {
                        float dx = playerPos.x - transform->position.x;
                        float dy = playerPos.y - transform->position.y;
                        float len = std::sqrt(dx*dx + dy*dy);
                        if (len > 0.001f) {
                            enemy->targetDirection.x = dx/len;
                            enemy->targetDirection.y = dy/len;
                        }
                    } else {
                        enemy->targetDirection = GNVector2(-1.0f, 0.0f);
                    }
                    
                    // Pullback vector (opposite direction)
                    enemy->pullbackVector.x = -enemy->targetDirection.x * 30.0f;
                    enemy->pullbackVector.y = -enemy->targetDirection.y * 30.0f;
                }
                break;
            }
            
            case EnemyState::Pullback: {
                transform->position.x += enemy->pullbackVector.x * deltaTime * 3.0f;
                transform->position.y += enemy->pullbackVector.y * deltaTime * 3.0f;
                enemy->pullbackTimer -= deltaTime;
                
                if (enemy->pullbackTimer <= 0.0f) {
                    enemy->currentState = EnemyState::Beeline;
                    enemy->beelineSpeed = 700.0f;
                }
                break;
            }
            
            case EnemyState::Beeline: {
                transform->position.x += enemy->targetDirection.x * enemy->beelineSpeed * deltaTime;
                transform->position.y += enemy->targetDirection.y * enemy->beelineSpeed * deltaTime;
                break;
            }
            
            default:
                break;
        }

    } else if (enemy->movementPattern == "galaga") {
        // GALAGA MOVEMENT: FlyIn -> Hover -> Circle/Wave Pattern
        // CRITICAL: This movement is FULLY DECOUPLED from world scroll in ALL states
        
        const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
        
        // Sprite offset: transform.position is TOP-LEFT, but we want to track CENTER
        const float spriteHalfSize = 32.0f;
        
        // Counter-act world scroll to stay fixed in screen space
        // CRITICAL: Use ACTUAL level speed, otherwise enemies drift or fly off if mismatch occurs!
        float worldScrollSpeed = m_levelManager->GetCurrentLevelConfig().worldSpeed;
        
        // Circle radius: 25% of screen width (50% total diameter)
        float circleRadiusX = screenInfo.pixelWidth * 0.25f;
        
        // Helpers for CENTER-based positioning
        auto getCenterX = [&]() { return transform->position.x + spriteHalfSize; };
        auto getCenterY = [&]() { return transform->position.y + spriteHalfSize; };
        auto setCenterX = [&](float cx) { transform->position.x = cx - spriteHalfSize; };
        auto setCenterY = [&](float cy) { transform->position.y = cy - spriteHalfSize; };
        
        switch (enemy->currentState) {
            case EnemyState::Idle:
                enemy->currentState = EnemyState::FlyIn;
                // Randomly choose circle (50%) or wave (50%) pattern - equal split for testing
                enemy->useCirclePattern = (rand() % 100) < 50;
                enemy->circleLoopsRemaining = 3;
                enemy->circleAngle = 0.0f;
                enemy->circleRadius = circleRadiusX;
                enemy->hasSetAnchorX = false;
                GN_LOG_INFO("[GALAGA] Spawned with pattern: " + std::string(enemy->useCirclePattern ? "CIRCLE" : "WAVE") +
                           " radius=" + std::to_string(circleRadiusX));
                break;
                
            case EnemyState::FlyIn: {
                // Counter-act world scroll DURING fly-in
                if (transform->position.x > screenInfo.pixelWidth + 100.0f) {
                   // Only log if far off screen to avoid spam, but useful to confirm spawn
                   static float lastLogTime = 0;
                   if (m_time - lastLogTime > 1.0f) {
                       GN_LOG_INFO("[GALAGA] Enemy Flying In from X=" + std::to_string(transform->position.x));
                       lastLogTime = m_time;
                   }
                }
                
                transform->position.x += worldScrollSpeed * deltaTime;
                
                // TRACK PLAYER Y during fly-in for better alignment (center to center)
                Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
                if (playerEntity != 0) {
                    Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                    Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(playerEntity);
                    Sprite* enemySprite = m_ecsSystem->GetComponent<Sprite>(enemyEntity);
                    if (playerTransform) {
                        // Get player center Y (using actual sprite size)
                        float playerCenterY = playerTransform->position.y;
                        if (playerSprite) {
                            playerCenterY += (playerSprite->height * std::abs(playerTransform->scale.y)) * 0.5f;
                        }
                        
                        // Get enemy center Y (using actual sprite size, not hardcoded 32)
                        float enemyHalfHeight = spriteHalfSize; // Default fallback
                        if (enemySprite) {
                            enemyHalfHeight = (enemySprite->height * std::abs(transform->scale.y)) * 0.5f;
                        }
                        float enemyCenterY = transform->position.y + enemyHalfHeight;
                        
                        // Smoothly move CENTER toward player CENTER
                        float yDiff = playerCenterY - enemyCenterY;
                        float trackSpeed = 1.5f; // Slow, smooth interpolation
                        float newCenterY = enemyCenterY + yDiff * trackSpeed * deltaTime;
                        transform->position.y = newCenterY - enemyHalfHeight;
                    }
                }
                
                // Target: 65% screen (requested by user)
                float targetCenterX = screenInfo.pixelWidth * 0.65f;
                float currentCenterX = getCenterX();
                float dist = currentCenterX - targetCenterX;
                
                if (dist > 5.0f) {
                    float flySpeed = enemy->speed;
                    transform->position.x -= flySpeed * deltaTime;
                } else {
                    if (!enemy->hasSetAnchorX) {
                        enemy->anchorX = getCenterX();
                        enemy->hasSetAnchorX = true;
                        enemy->baseY = getCenterY();
                    }
                    enemy->currentState = EnemyState::Hover;
                    enemy->currentState = EnemyState::Hover;
                    enemy->galagaHoverTimer = 2.0f; // 2 seconds hover (fixed)
                    GN_LOG_INFO("[GALAGA] Reached Hover at centerX=" + std::to_string(getCenterX()) + 
                               ", hovering for " + std::to_string(enemy->galagaHoverTimer) + "s");
                }
                break;
            }
            
            case EnemyState::Hover: {
                // HOVER: Track player Y position while hovering
                // Counter-act world scroll
                transform->position.x += worldScrollSpeed * deltaTime;
                
                // TRACK PLAYER Y during hover (center to center)
                Entity playerEntity = m_levelManager ? m_levelManager->GetPlayerEntity() : 0;
                if (playerEntity != 0) {
                    Transform* playerTransform = m_ecsSystem->GetComponent<Transform>(playerEntity);
                    Sprite* playerSprite = m_ecsSystem->GetComponent<Sprite>(playerEntity);
                    Sprite* enemySprite = m_ecsSystem->GetComponent<Sprite>(enemyEntity);
                    if (playerTransform) {
                        // Get player center Y (using actual sprite size)
                        float playerCenterY = playerTransform->position.y;
                        if (playerSprite) {
                            playerCenterY += (playerSprite->height * std::abs(playerTransform->scale.y)) * 0.5f;
                        }
                        
                        // Get enemy center Y (using actual sprite dimensions)
                        float enemyHalfHeight = spriteHalfSize; // Default fallback
                        if (enemySprite) {
                            enemyHalfHeight = (enemySprite->height * std::abs(transform->scale.y)) * 0.5f;
                        }
                        float enemyCenterY = transform->position.y + enemyHalfHeight;
                        
                        // Smoothly move CENTER toward player CENTER
                        float yDiff = playerCenterY - enemyCenterY;
                        float trackSpeed = 1.2f; // Slow, smooth interpolation
                        float newCenterY = enemyCenterY + yDiff * trackSpeed * deltaTime;
                        transform->position.y = newCenterY - enemyHalfHeight;
                        enemy->baseY = newCenterY; // Update baseY to match tracked center
                    }
                }
                
                enemy->galagaHoverTimer -= deltaTime;
                if (enemy->galagaHoverTimer <= 0.0f) {
                    enemy->currentState = EnemyState::Attacking;
                    // Start at angle 0 (RIGHT side of circle) for counter-clockwise motion
                    enemy->circleAngle = 0.0f;
                    enemy->circleLoopsRemaining = 3;
                    enemy->circleRadius = circleRadiusX;
                    
                    enemy->circleCenter = Gnosis::GNVector2(
                        screenInfo.pixelWidth * 0.45f,
                        screenInfo.pixelHeight * 0.4f
                    );
                    
                    // For wave: START AT PHASE 0 (Static Hover Position)
                    // Since hover is now static at baseY, and sin(0) = 0, starting at phase 0
                    // ensures a perfect match with regular sine wave.
                    if (!enemy->useCirclePattern) {
                        // Start wave time NOW. 
                        // sin((m_time - anchorX) * freq) -> sin(0) -> 0 offset from baseY
                        enemy->anchorX = m_time;
                        
                        GN_LOG_INFO("[GALAGA] Starting WAVE! Static start at Y=" + std::to_string(getCenterY()) + 
                                   " BaseY=" + std::to_string(enemy->baseY));
                    } else {
                        // For circle: unused, but set to m_time just in case
                        enemy->anchorX = m_time;
                        GN_LOG_INFO("[GALAGA] Starting CIRCLE! radius=" + std::to_string(enemy->circleRadius));
                    }
                }
                break;
            }
            
            case EnemyState::Attacking: {
                // Counter-act world scroll during attack phase
                transform->position.x += worldScrollSpeed * deltaTime;
                
                if (enemy->useCirclePattern) {
                    // CIRCLE PATTERN: COUNTER-CLOCKWISE around screen center
                    float circleSpeed = 1.2f; // radians per second (slower)
                    enemy->circleAngle -= circleSpeed * deltaTime; // NEGATIVE for counter-clockwise
                    
                    // Calculate target CENTER position on circle
                    float targetCenterX = enemy->circleCenter.x + std::cos(enemy->circleAngle) * enemy->circleRadius;
                    float targetCenterY = enemy->circleCenter.y + std::sin(enemy->circleAngle) * enemy->circleRadius;
                    
                    // Smooth lerp to target position
                    float lerpSpeed = 4.0f;
                    float currentCenterX = getCenterX();
                    float currentCenterY = getCenterY();
                    
                    float newCenterX = currentCenterX + (targetCenterX - currentCenterX) * lerpSpeed * deltaTime;
                    float newCenterY = currentCenterY + (targetCenterY - currentCenterY) * lerpSpeed * deltaTime;
                    
                    setCenterX(newCenterX);
                    setCenterY(newCenterY);
                    
                    // Check for loop completion (counter-clockwise: angle goes negative)
                    if (enemy->circleAngle <= -6.28318f) { // Full 2π rotation
                        enemy->circleAngle += 6.28318f; // Reset
                        enemy->circleLoopsRemaining--;
                        GN_LOG_INFO("[GALAGA CIRCLE] Loop completed! Remaining=" + std::to_string(enemy->circleLoopsRemaining));
                        
                        if (enemy->circleLoopsRemaining <= 0) {
                            enemy->currentState = EnemyState::Beeline;
                            enemy->targetDirection = Gnosis::GNVector2(-1.0f, 0.3f);
                            enemy->beelineSpeed = 400.0f;
                            GN_LOG_INFO("[GALAGA] Circles complete - flying out!");
                        }
                    }
                } else {
                    // WAVE PATTERN: Sinusoidal wave with shorter wavelength
                    float waveSpeed = 60.0f;         // Horizontal drift speed
                    float waveFrequency = 2.5f;      // Higher frequency = shorter wavelength (was 0.8)
                    float waveAmplitude = 300.0f;    // Larger amplitude (was 250)
                    
                    // DON'T counter-act scroll for wave - let enemy drift left
                    transform->position.x -= worldScrollSpeed * deltaTime; // Cancel the counter-act above
                    transform->position.x -= waveSpeed * deltaTime;
                    
                    // INDEPENDENT WAVE TIMING: Use per-enemy phase offset
                    float enemyWaveTime = m_time - enemy->anchorX; // Time since effective start of wave
                    float targetWaveY = enemy->baseY + std::sin(enemyWaveTime * waveFrequency) * waveAmplitude;
                    float currentCenterY = getCenterY();
                    
                    // INTERPOLATE to target Y for even smoother transition (optional, but good for safety)
                    // If the asin jump fix works perfectly, this interpolation will be minimal
                    float lerpSpeed = 5.0f;
                    float newCenterY = currentCenterY + (targetWaveY - currentCenterY) * lerpSpeed * deltaTime;
                    setCenterY(newCenterY);
                    
                    // Exit left off screen
                    if (transform->position.x < -100.0f) {
                        GN_LOG_INFO("[GALAGA WAVE] Exited left");
                    }
                }
                break;
            }
            
            case EnemyState::Beeline: {
                // Flying out after circles - no scroll compensation, just move
                transform->position.x += enemy->targetDirection.x * enemy->beelineSpeed * deltaTime;
                transform->position.y += enemy->targetDirection.y * enemy->beelineSpeed * deltaTime;
                break;
            }
            
            default:
                break;
        }

    } else if (enemy->movementPattern == "echelon") {
        // ECHELON MOVEMENT: Move left + Formation Offset + Wave
        
        // Base movement left
        transform->position.x -= enemy->speed * deltaTime;
        
        // RECALCULATE Y offset based on posInV for consistency with spawn and wrap
        float yOffset = 0.0f;
        switch (enemy->posInV) {
            case 0: yOffset = 0.0f; break;      // Tip (front center)
            case 1: yOffset = -100.0f; break;   // Upper wing 1
            case 2: yOffset = 100.0f; break;    // Lower wing 1
            case 3: yOffset = -200.0f; break;   // Upper wing 2 (trailing)
            case 4: yOffset = 200.0f; break;    // Lower wing 2 (trailing)
        }
        
        // Apply bobbing if enabled
        float bobOffset = 0.0f;
        if (enemy->bobbingEnabled && enemy->bobAmplitude > 0.0f) {
            bobOffset = std::sin(m_time * enemy->bobSpeed) * enemy->bobAmplitude;
        }
        
        // Use baseY + calculated offset + optional bobbing
        transform->position.y = enemy->baseY + yOffset + bobOffset;
        
    } else {
        // Standard horizontal movement for other enemy types
        // FIXED: Only apply speed if enemy has speed > 0 (snowmen have speed=0 and move with world scroll only)
        if (enemy->speed > 0.0f) {
            transform->position.x -= enemy->speed * deltaTime;
        }

        // Vertical movement - bobbing/sinusoidal if enabled
        if (enemy->bobbingEnabled) {
            // Calculate bobbing offset using sine wave with phase offset for variation
            // bobPhase (0-2π) prevents all enemies from bobbing in sync
            float bobOffset = std::sin(m_time * enemy->bobSpeed + enemy->bobPhase) * enemy->bobAmplitude;
            transform->position.y = enemy->baseY + bobOffset;
        }
    }

    // Screen wrapping now handled entirely by LevelManager::UpdateEnemyPooling
    // to avoid duplicate wrap logic causing state conflicts
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
    if (!enemy || !transform || !hitbox || !sprite) {
        if (enemy && enemy->movementPattern == "flying") {
            GN_LOG_WARN("[RATCOPTER_COLLISION] Skipped - missing component: enemy=" + std::to_string(enemy != nullptr) + 
                       " transform=" + std::to_string(transform != nullptr) + " hitbox=" + std::to_string(hitbox != nullptr) + 
                       " sprite=" + std::to_string(sprite != nullptr));
        }
        return;
    }
    
    // Calculate enemy center position accounting for sprite dimensions and scale</parameter>
    float enemySpriteWidth = sprite->width * std::abs(transform->scale.x);
    float enemySpriteHeight = sprite->height * std::abs(transform->scale.y);
    float enemyCenterX = transform->position.x + (enemySpriteWidth * 0.5f);
    float enemyCenterY = transform->position.y + (enemySpriteHeight * 0.5f);
    
    // Scale the enemy hitbox radius by the transform scale
    float scaledEnemyRadius = hitbox->radius * ((std::abs(transform->scale.x) + std::abs(transform->scale.y)) * 0.5f);
    
    // Log first collision check for debugging
    static bool loggedOnce = false;
    if (!loggedOnce && activeProjectiles.size() > 0) {
        GN_LOG_INFO("ProcessEnemyCollision: Enemy " + std::to_string(e) + 
                   " center=(" + std::to_string(enemyCenterX) + "," + std::to_string(enemyCenterY) + 
                   ") scaledRadius=" + std::to_string(scaledEnemyRadius) + 
                   " checking " + std::to_string(activeProjectiles.size()) + " projectiles");
        loggedOnce = true;
    }
    
    // Check collision with each projectile
    for (Entity projEntity : activeProjectiles) {
        Transform* projTransform = m_ecsSystem->GetComponent<Transform>(projEntity);
        Hitbox* projHitbox = m_ecsSystem->GetComponent<Hitbox>(projEntity);
        Projectile* proj = m_ecsSystem->GetComponent<Projectile>(projEntity);
        Sprite* projSprite = m_ecsSystem->GetComponent<Sprite>(projEntity);
        
        if (!projTransform || !projHitbox || !proj || !proj->isActive) continue;
        
        // Calculate projectile center position
        float projCenterX = projTransform->position.x;
        float projCenterY = projTransform->position.y;
        
        // If projectile has a sprite, use its dimensions for center calculation
        if (projSprite) {
            float projSpriteWidth = projSprite->width * std::abs(projTransform->scale.x);
            float projSpriteHeight = projSprite->height * std::abs(projTransform->scale.y);
            projCenterX += (projSpriteWidth * 0.5f);
            projCenterY += (projSpriteHeight * 0.5f);
        }
        
        // Scale the projectile hitbox radius
        float scaledProjRadius = projHitbox->radius * ((std::abs(projTransform->scale.x) + std::abs(projTransform->scale.y)) * 0.5f);
        
        // DEBUG: Log first projectile check
        if (!loggedOnce) {
            GN_LOG_INFO("  Checking projectile " + std::to_string(projEntity) + 
                       " center=(" + std::to_string(projCenterX) + "," + std::to_string(projCenterY) + 
                       ") scaledRadius=" + std::to_string(scaledProjRadius));
        }
        
        // Circle-circle collision using proper center points and scaled radii
        bool collision = false;
        if (hitbox->type == ColliderType::Circle && projHitbox->type == ColliderType::Circle) {
            float dx = enemyCenterX - projCenterX;
            float dy = enemyCenterY - projCenterY;
            float distance = std::sqrt(dx * dx + dy * dy);
            float combinedRadius = scaledEnemyRadius + scaledProjRadius;
            collision = (distance < combinedRadius);
            
            if (!loggedOnce) {
                GN_LOG_INFO("    dx=" + std::to_string(dx) + " dy=" + std::to_string(dy) + 
                           " distance=" + std::to_string(distance) + " combinedRadius=" + std::to_string(combinedRadius) + 
                           " collision=" + std::to_string(collision));
            }
        }
        
        if (collision) {
            GN_LOG_INFO("[COLLISION] Projectile-Enemy HIT! Projectile: " + std::to_string(projEntity) + 
                       " Enemy: " + std::to_string(e) + " Type: " + enemy->enemyType + 
                       " Pattern: " + enemy->movementPattern);
            
            // Damage the enemy
            enemy->health -= proj->damage;
            
            GN_LOG_INFO("[COLLISION] Enemy health: " + std::to_string(enemy->health + proj->damage) + 
                       " -> " + std::to_string(enemy->health) + " (damage: " + std::to_string(proj->damage) + ")");
            
            // Deactivate the projectile
            proj->isActive = false;
            
            if (enemy->health <= 0) {
                // Enemy defeated - IMMEDIATELY disable collision for non-echelon enemies
                // Echelon birds use sprite->visible for death and stay in active list for resurrection
                // FIX: Do NOT set isActive=false here for any enemy type.
                // We need isActive=true so EnemySystem::Update can process the Hurt animation
                // and call ReturnEnemyToPool when it finishes.
                // Collision is already prevented by checking currentState != Hurt in the collision loop.
                // if (enemy->movementPattern != "echelon") {
                //    enemy->isActive = false;  // Disable collision right away
                // }
                enemy->currentState = EnemyState::Hurt;
                
                // Play enemy-specific kill sound
                if (m_levelManager && GameCore::GetGame()) {
                    const PlatformDelegates& delegates = m_levelManager->GetPlatformDelegates();
                    if (delegates.audio.playSound) {
                        std::string killSound = "";
                        
                        // Determine kill sound based on enemy type (check if type contains the base name)
                        if (enemy->enemyType.find("ToiletPaper") != std::string::npos) {
                            killSound = "tpkill";
                        } else if (enemy->enemyType.find("Rat") != std::string::npos) {
                            killSound = "ratkill";
                        } else if (enemy->enemyType.find("Bird") != std::string::npos) {
                            killSound = "birdkill";
                        }
                        
                        if (!killSound.empty()) {
                            // Use PlaySFX which applies volume settings automatically
                            GameCore::GetGame()->PlaySFX(killSound);
                            GN_LOG_INFO("[COLLISION] Playing kill sound: " + killSound + " for enemy type: " + enemy->enemyType);
                        }
                    }
                }
                
                float hurtDuration = 0.6f; // Default duration
                const StateAnimation::Clip* hurtClip = nullptr;
                
                // Use already-fetched sprite and stateAnim
                if (stateAnim && sprite) {
                    // Find hurt animation clip to get accurate duration
                    hurtClip = stateAnim->getClip("hurt");
                    if (hurtClip) {
                        hurtDuration = hurtClip->frameCount * hurtClip->frameTime + 0.1f;
                         
                        GN_LOG_INFO("[COLLISION] Hurt clip found: " + hurtClip->textureId + 
                                   " frames=" + std::to_string(hurtClip->frameCount) + 
                                   " duration=" + std::to_string(hurtDuration) + "s");
                        
                        // Switch to hurt animation
                        sprite->textureId = hurtClip->textureId;
                        sprite->isAnimated = (hurtClip->frameCount > 1);
                        sprite->frameWidth = hurtClip->frameWidth;
                        sprite->frameHeight = hurtClip->frameHeight;
                        sprite->frameCount = hurtClip->frameCount;
                        sprite->frameTime = hurtClip->frameTime;
                        sprite->loop = false;
                        sprite->currentFrame = 0;
                        sprite->currentFrameTime = 0.0f;
                        sprite->playing = true;
                        sprite->hasCompleted = false;
                    }
                    
                    // All enemies enter Hurt state - they'll be returned to pool when animation completes
                    this->ChangeEnemyState(enemy, EnemyState::Hurt, hurtDuration);
                } else {
                     // No sprite/anim components - fallback
                     this->ChangeEnemyState(enemy, EnemyState::Hurt, hurtDuration);
                }
                
                enemy->hurtTimer = hurtDuration;
                
                GN_LOG_INFO("[COLLISION] Enemy " + std::to_string(e) + " DEFEATED - hurt timer=" + 
                           std::to_string(hurtDuration) + "s");
            } else {
                // Enemy took damage but not defeated - brief hurt state
                enemy->currentState = EnemyState::Hurt;
                enemy->hurtTimer = 0.3f; // Brief hurt flash
                
                // Use already-fetched sprite (no GetComponent call!)
                if (sprite) {
                    sprite->hasCompleted = false;
                }
                
                GN_LOG_INFO("[COLLISION] Enemy " + std::to_string(e) + " damaged, health remaining: " + 
                           std::to_string(enemy->health));
            }
            
            break; // Only process one collision per enemy per frame
        }
    }
}

} // namespace GameCore