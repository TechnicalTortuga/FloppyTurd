#include "BossSystem.h"
#include "ProjectileSystem.h"
#include "../Config/EnemyConfigs.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Platform/HapticHelpers.h"
#include <cmath>
#include <algorithm>

namespace GameCore {

BossSystem::BossSystem(Gnosis::ECS* ecsSystem, LevelManager* levelManager, ProjectileSystem* projectileSystem, ExplosionSystem* explosionSystem, PlatformDelegates* platformDelegates)
    : m_ecsSystem(ecsSystem)
    , m_levelManager(levelManager)
    , m_projectileSystem(projectileSystem)
    , m_explosionSystem(explosionSystem)
    , m_platformDelegates(platformDelegates)
{
    GN_LOG_INFO("BossSystem created");
}

BossSystem::~BossSystem() {
    // Clean up arm entities
    if (backArmEntity != 0 && m_ecsSystem) {
        m_ecsSystem->DestroyEntity(backArmEntity);
        backArmEntity = 0;
    }

    if (frontArmEntity != 0 && m_ecsSystem) {
        m_ecsSystem->DestroyEntity(frontArmEntity);
        frontArmEntity = 0;
    }

    // Clean up lock-on dot entities
    DestroyLockOnDotEntities();

    UnloadSprites();
    GN_LOG_INFO("BossSystem destroyed");
}

void BossSystem::InitializeForLevel() {
    GN_LOG_INFO("BossSystem: InitializeForLevel called");

    if (!m_ecsSystem || !m_levelManager) {
        GN_LOG_ERROR("BossSystem: Missing ECS system or Level manager!");
        return;
    }

    // Find the Rat King entity in active enemies
    const auto& activeEnemies = m_levelManager->GetActiveEnemies();
    GN_LOG_INFO("BossSystem: Looking for Rat King in " + std::to_string(activeEnemies.size()) + " active enemies");

    for (Entity e : activeEnemies) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(e);
        Enemy* enemy = m_ecsSystem->GetComponent<Enemy>(e);

        GN_LOG_DEBUG("BossSystem: Checking enemy entity " + std::to_string(e) +
                    " - transform: " + std::to_string(transform != nullptr) +
                    ", enemy: " + std::to_string(enemy != nullptr));

        if (enemy) {
            GN_LOG_DEBUG("BossSystem: Enemy type: '" + enemy->enemyType + "'");
        }

        if (transform && enemy && (enemy->enemyType == "Ratking" || enemy->enemyType == "RatKing")) {
            bossEntity = e;
            // Store the current position from LevelManager, but don't override it later
            position.x = transform->position.x;
            position.y = transform->position.y;
            isActive = true;

            GN_LOG_INFO("BossSystem: Found Rat King entity " + std::to_string(bossEntity) +
                       " at initial position (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");

            // Set screen-aware walk boundaries
            // User Requirements:
            // - Left edge of Rat King can't go less than 50% of screen
            // - Right edge of Rat King (position + width) shouldn't exceed screen edge
            
            // Boundary calculations for top-left positioned sprite
            // Sprite is 128x128 pixels scaled 8x = 1024x1024 pixels rendered
            float ratKingSpriteWidth = 128.0f * scale; // 1024px rendered width
            float ratKingSpriteHeight = 128.0f * scale; // 1024px rendered height

            // Left boundary: left edge at 50% screen
            walkRangeMin = screenWidth * 0.5f; // Left edge of sprite at 50% screen
            
            // Right boundary: right edge at screen edge
            // position.x + width = screenWidth
            // position.x = screenWidth - width
            walkRangeMax = screenWidth - ratKingSpriteWidth; // Right edge touches screen edge

            // Don't override LevelManager's spawn position during initialization
            // Only apply boundary clamping during movement in HandleWalking
            GN_LOG_INFO("BossSystem: Walk boundaries set - Min: " + std::to_string(walkRangeMin) +
                       ", Max: " + std::to_string(walkRangeMax) +
                       ", SpriteWidth: " + std::to_string(ratKingSpriteWidth) +
                       ", Keeping LevelManager position: " + std::to_string(position.x));



            // Create additional sprite entities for multi-sprite animation
            CreateBodyPartEntities();

            // Load sprites for all body parts
            LoadSprites();

            // Set initial state
            ChangeState(RatKingState::IDLE);
            return;
        }
    }

    GN_LOG_WARN("BossSystem: No Rat King entity found in active enemies");
}

void BossSystem::Update(float deltaTime) {
    if (!isActive) {
        GN_LOG_DEBUG("BossSystem: Update skipped - boss is not active (dead or removed)");
        return;
    }
    
    if (bossEntity == 0) {
        GN_LOG_DEBUG("BossSystem: Update skipped - bossEntity is 0");
        return;
    }

    // Check if boss entity still has required components
    if (!m_ecsSystem->GetComponent<Transform>(bossEntity) || !m_ecsSystem->GetComponent<Sprite>(bossEntity)) {
        GN_LOG_ERROR("BossSystem: Boss entity " + std::to_string(bossEntity) + " missing required components!");
        isActive = false;
        return;
    }

    GN_LOG_DEBUG("BossSystem: Update starting - state=" + std::to_string(static_cast<int>(currentState)) +
                ", position=(" + std::to_string(position.x) + "," + std::to_string(position.y) + ")");

    // Update current state
    switch (currentState) {
        case RatKingState::IDLE: HandleIdle(deltaTime); break;
        case RatKingState::WALKING_LEFT:
        case RatKingState::WALKING_RIGHT: HandleWalking(deltaTime); break;
        case RatKingState::AIMING: HandleAiming(deltaTime); break;
        case RatKingState::THROWING: HandleThrowing(deltaTime); break;
        case RatKingState::HURT: HandleHurt(deltaTime); break;
        case RatKingState::DEATH: HandleDeath(deltaTime); break;
    }

    // Update arm rotations every frame during aiming state for smooth tracking
    if (currentState == RatKingState::AIMING) {
        UpdateArmRotations();
    }

    // Update sprites
    UpdateSprites(deltaTime);

    // Update hurt buffer
    if (currentState == RatKingState::HURT && hurtBuffer > 0) {
        --hurtBuffer;
    }

    // Update aiming lock-on indicator
    if (currentState == RatKingState::AIMING) {
        UpdateLockOnIndicator();
    }

    GN_LOG_DEBUG("BossSystem: Update completed - health=" + std::to_string(health) + "/" + std::to_string(maxHealth));
}

void BossSystem::UpdateScreenDimensions(float width, float height) {
    screenWidth = width;
    screenHeight = height;
    GN_LOG_INFO("BossSystem: Updated screen dimensions - Width: " + std::to_string(screenWidth) + ", Height: " + std::to_string(screenHeight));
}

void BossSystem::HandleDamage(int damage) {
    if (!isActive) {
        GN_LOG_DEBUG("Rat King damage blocked - boss not active");
        return;
    }

    GN_LOG_INFO("🛡️ BOSS DAMAGE: Current position BEFORE damage (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ") health=" + std::to_string(health));

    // Check if boss is currently in hurt state with active invincibility buffer
    if (currentState == RatKingState::HURT && hurtBuffer > 0) {
        GN_LOG_DEBUG("Rat King damage blocked by invincibility buffer (hurtBuffer=" + std::to_string(hurtBuffer) + ")");
        return;
    }

    // Check if boss is already dead
    if (currentState == RatKingState::DEATH) {
        GN_LOG_DEBUG("Rat King damage blocked - already dead");
        return;
    }

    health -= damage;
    if (health < 0) health = 0;

    GN_LOG_INFO("Rat King took " + std::to_string(damage) + " damage, health now: " + std::to_string(health) + "/" + std::to_string(maxHealth));

    if (health <= 0) {
        GN_LOG_INFO("Rat King health depleted - transitioning to DEATH state");
        ChangeState(RatKingState::DEATH);
    } else {
        // Trigger haptic feedback for boss taking damage
        if (m_platformDelegates) {
            HapticHelpers::TriggerBossDamage(*m_platformDelegates);
        }
        
        ChangeState(RatKingState::HURT);
        hurtBuffer = 24; // 24 frames of invincibility (matches old system)
        hurtFlashTimer = 0.0f; // Start hurt flashing
    }

    // Check for minion spawning (matches old system: 30 HP intervals)
    while (health <= nextMinionHealthThreshold && nextMinionHealthThreshold > 0) {
        if (health >= 50) {  // Above 25% health
            SpawnMinionWave(4);
        } else {
            SpawnMinionWave(6);
        }
        nextMinionHealthThreshold -= 30;  // Next threshold (30 HP intervals)
    }

    // Check for dynamic music changes
    float healthPercent = static_cast<float>(health) / maxHealth;
    if (healthPercent <= 0.40f && !hasTriggeredLowHealthMusic) {
        hasTriggeredLowHealthMusic = true;
        ChangeMusic("BossThemeLowHealth.mp3");
        GN_LOG_INFO("Rat King switching to low health music");
    }
}

void BossSystem::HandleIdle(float deltaTime) {
    idleTimer += deltaTime;
    if (idleTimer > 0.5f) {  // Temporarily reduced from 2.0f to 0.5f for faster testing
        idleTimer = 0.0f;
        // Choose walking destination or aiming
        if (rand() % 3 != 0) {
            // 2/3 chance to aim (temporarily increased for testing)
            ChangeState(RatKingState::AIMING);
        } else {
            // 2/3 chance to walk to a specific destination
            // Choose either left boundary or right boundary as destination
            float currentX = position.x;
            float distanceToLeft = abs(currentX - walkRangeMin);
            float distanceToRight = abs(currentX - walkRangeMax);

            if (distanceToLeft < distanceToRight) {
                // Closer to left, so walk to right
                m_walkDestination = walkRangeMax;
                ChangeState(RatKingState::WALKING_RIGHT);
            } else {
                // Closer to right or equidistant, walk to left
                m_walkDestination = walkRangeMin;
                ChangeState(RatKingState::WALKING_LEFT);
            }
        }
    }
}

void BossSystem::HandleWalking(float deltaTime) {
    // Move towards the destination
    float direction = (m_walkDestination > position.x) ? 1.0f : -1.0f;

    // Update position towards destination
    position.x += direction * walkSpeed * deltaTime;

    // Check if we've reached the destination (within tolerance)
    float distanceToDestination = abs(position.x - m_walkDestination);
    if (distanceToDestination < 5.0f) { // Within 5 pixels of destination
        position.x = m_walkDestination; // Snap to exact position
        walkTimer = 0.0f;
        ChangeState(RatKingState::IDLE);
        GN_LOG_DEBUG("BossSystem: Reached walk destination at " + std::to_string(m_walkDestination));
    }

    // Safety check - if we somehow go beyond boundaries, clamp them
    if (position.x < walkRangeMin) {
        position.x = walkRangeMin;
        walkTimer = 0.0f;
        ChangeState(RatKingState::IDLE);
    } else if (position.x > walkRangeMax) {
        position.x = walkRangeMax;
        walkTimer = 0.0f;
        ChangeState(RatKingState::IDLE);
    }

    // Update walk timer as fallback (in case destination is never reached due to floating point issues)
    walkTimer += deltaTime;
    if (walkTimer > 5.0f) { // Increased timeout for purposeful walking
        walkTimer = 0.0f;
        ChangeState(RatKingState::IDLE);
        GN_LOG_DEBUG("BossSystem: Walk timeout reached, returning to idle");
    }

    // Update entity position - only update X for walking movement, preserve Y from LevelManager
    if (bossEntity != 0 && m_ecsSystem) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(bossEntity);
        if (transform) {
            transform->position.x = position.x;
            // Keep the Y position that was set by LevelManager, don't override it
            position.y = transform->position.y; // Sync stored Y with current transform Y
        }
    }
}

void BossSystem::HandleAiming(float deltaTime) {
    // Scale aiming duration based on health (like old system)
    float healthPercent = static_cast<float>(health) / maxHealth;
    if (healthPercent <= 0.40f) {
        aimingData.aimDuration = 0.8f;  // Faster at low health (was 0.5f)
    } else if (healthPercent <= 0.5f) {
        aimingData.aimDuration = 1.5f;  // Medium speed (was 1.0f)
    } else {
        aimingData.aimDuration = 2.0f;  // Normal aiming time (was 1.2f)
    }

    aimingData.aimTimer += deltaTime;

    GN_LOG_DEBUG("BossSystem: HandleAiming - timer=" + std::to_string(aimingData.aimTimer) +
                 "/" + std::to_string(aimingData.aimDuration) +
                 ", deltaTime=" + std::to_string(deltaTime));

    // Log boss position
    GN_LOG_INFO("🎯 BOSS POSITION: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ") scale=" + std::to_string(scale));

    // Update shoulder position for arm rotation
    aimingData.shoulderPivot = GetShoulderPosition();

    // Smooth angle interpolation towards player
    GNVector2 shoulder = GetShoulderPosition();
    GN_LOG_INFO("🎯 SHOULDER POSITION: (" + std::to_string(shoulder.x) + ", " + std::to_string(shoulder.y) + ")");
    Gnosis::GNVector2 toPlayer = Gnosis::Vector2Subtract(aimingData.playerPosition, shoulder);
    float targetAngle = atan2(toPlayer.y, toPlayer.x) * Gnosis::RAD2DEG;
    
    // Normalize to 0-360 range BEFORE clamping (atan2 can return negative angles)
    if (targetAngle < 0.0f) targetAngle += 360.0f;
    
    targetAngle = Gnosis::Clamp(targetAngle, 120.0f, 240.0f); // Limit aiming range

    aimingData.currentArmAngle = Gnosis::SmoothAngleLerp(aimingData.currentArmAngle, targetAngle, deltaTime * 4.0f);

    GN_LOG_DEBUG("BossSystem: Aiming - playerPos=(" + std::to_string(aimingData.playerPosition.x) + "," + std::to_string(aimingData.playerPosition.y) +
                 "), shoulder=(" + std::to_string(shoulder.x) + "," + std::to_string(shoulder.y) +
                 "), toPlayer=(" + std::to_string(toPlayer.x) + "," + std::to_string(toPlayer.y) +
                 "), targetAngle=" + std::to_string(targetAngle) + "°, currentAngle=" + std::to_string(aimingData.currentArmAngle) + "°");

    // Update lock-on indicator dots for visual feedback
    UpdateLockOnIndicator();

    // Check for lock-on completion
    if (aimingData.aimTimer >= aimingData.aimDuration) {
        GN_LOG_INFO("BossSystem: Aiming complete - transitioning to THROWING, final angle=" + std::to_string(aimingData.currentArmAngle));
        aimingData.hasLockedOn = true;
        aimingData.lockOnAngle = aimingData.currentArmAngle;
        ChangeState(RatKingState::THROWING);
    }
}

void BossSystem::HandleThrowing(float deltaTime) {
    // Animation updates are handled by SpriteSystem
    // We just need to check for projectile spawning and animation completion

    // Spawn projectile on frame 5 of the 7-frame animation (0-based, so frame 6)
    if (bossEntity != 0 && !hasFiredProjectile) {
        Sprite* torsoSprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
        if (torsoSprite && torsoSprite->currentFrame >= 5) {  // Frame 6 (0-based)
            hasFiredProjectile = true;
            SpawnProjectile();
        }
    }

    // Check if animation completed (frame 6 of 7-frame animation)
    if (bossEntity != 0) {
        Sprite* torsoSprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
        if (torsoSprite && torsoSprite->currentFrame >= 6) {  // Animation completed (frame 7, 0-based)
            hasFiredProjectile = false;
            ChangeState(RatKingState::IDLE);
        }
    }
}

void BossSystem::HandleHurt(float deltaTime) {
    GN_LOG_DEBUG("🛡️ HandleHurt: position=(" + std::to_string(position.x) + ", " + std::to_string(position.y) + ") hurtTimer=" + std::to_string(hurtTimer));
    
    hurtTimer += deltaTime;
    if (hurtTimer > 0.9f) {  // 6 frames at 0.15f per frame
        hurtTimer = 0.0f;
        GN_LOG_INFO("🛡️ HandleHurt: Hurt animation complete, returning to IDLE. Position=(" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");
        ChangeState(RatKingState::IDLE);
    }
}

void BossSystem::HandleDeath(float deltaTime) {
    GN_LOG_DEBUG("BossSystem: HandleDeath - deltaTime=" + std::to_string(deltaTime));
    
    // Start death sequence on first frame
    if (!m_deathSequenceStarted) {
        StartDeathSequence();
    }
    
    // Update death sequence
    UpdateDeathSequence(deltaTime);
}

void BossSystem::StartDeathSequence() {
    GN_LOG_INFO("🔥 Starting Rat King death sequence!");
    
    m_deathSequenceStarted = true;
    m_deathSequenceTimer = 0.0f;
    m_explosionIndex = 0;
    m_hasPlayedScreech = false;
    m_hasPlayedBossKill = false;
    
    // Trigger dramatic boss death haptic pattern (~2 second sequence)
    if (m_platformDelegates) {
        HapticHelpers::TriggerBossDeath(*m_platformDelegates);
        GN_LOG_INFO("🎮 Triggered boss death haptic pattern");
    }
    
    // Play BossKill sound immediately at reduced volume so screech can be heard
    if (m_platformDelegates && m_platformDelegates->audio.playSound) {
        m_platformDelegates->audio.playSound("BossKill.mp3", 0.6f);  // Reduced from 1.0 to 0.6
        m_hasPlayedBossKill = true;
        GN_LOG_INFO("🎵 Playing BossKill sound at volume 0.6");
    }
}

void BossSystem::UpdateDeathSequence(float deltaTime) {
    m_deathSequenceTimer += deltaTime;
    
    // Debug: Log timer every frame during death sequence
    GN_LOG_DEBUG("UpdateDeathSequence: timer=" + std::to_string(m_deathSequenceTimer) + 
                 ", hasPlayedScreech=" + std::to_string(m_hasPlayedScreech));
    
    // Play screech halfway through BossKill sound (around 0.5-1.0 seconds)
    if (!m_hasPlayedScreech && m_deathSequenceTimer >= 0.5f) {
        GN_LOG_INFO("🔊 Screech condition met! Timer: " + std::to_string(m_deathSequenceTimer));
        if (m_platformDelegates && m_platformDelegates->audio.playSound) {
            // Play screech at BOOSTED VOLUME to be heard over BossKill
            m_platformDelegates->audio.playSound("RatKingScreech.mp3", 1.2f);
            m_hasPlayedScreech = true;
            GN_LOG_INFO("🔊 PLAYING RatKingScreech.mp3 at BOOSTED volume 1.2 (to be heard over BossKill)");
        } else {
            GN_LOG_ERROR("❌ Cannot play screech - platformDelegates or playSound is null!");
        }
    }
    
    // Spawn explosions at timed intervals (slow, dramatic)
    // Explosion 1 at 0.3s
    if (m_explosionIndex == 0 && m_deathSequenceTimer >= 0.3f) {
        SpawnExplosionAtRandomPosition();
        m_explosionIndex++;
        
        // Trigger haptic for explosion
        if (m_platformDelegates) {
            HapticHelpers::TriggerHeavyCollision(*m_platformDelegates);
        }
    }
    // Explosion 2 at 0.7s
    else if (m_explosionIndex == 1 && m_deathSequenceTimer >= 0.7f) {
        SpawnExplosionAtRandomPosition();
        m_explosionIndex++;
        
        // Trigger haptic for explosion
        if (m_platformDelegates) {
            HapticHelpers::TriggerHeavyCollision(*m_platformDelegates);
        }
    }
    // Explosion 3 at 1.1s (big one)
    else if (m_explosionIndex == 2 && m_deathSequenceTimer >= 1.1f) {
        if (m_explosionSystem) {
            GNVector2 bossCenter = {position.x + (64.0f * scale), position.y + (64.0f * scale)};
            float randX = (rand() % 128) - 64.0f;
            float randY = (rand() % 128) - 64.0f;
            GNVector2 explosionPos = {bossCenter.x + randX, bossCenter.y + randY};
            m_explosionSystem->SpawnBigExplosion(explosionPos, scale, 0.5f); // Slow animation
            GN_LOG_INFO("Spawned BIG explosion at (" + std::to_string(explosionPos.x) + ", " + std::to_string(explosionPos.y) + ")");
        }
        m_explosionIndex++;
        
        // Trigger stronger haptic for BIG explosion
        if (m_platformDelegates && m_platformDelegates->haptic.triggerImpact) {
            m_platformDelegates->haptic.triggerImpact(HapticStyle::HEAVY, 1.0f);
        }
    }
    // Explosion 4 at 1.5s
    else if (m_explosionIndex == 3 && m_deathSequenceTimer >= 1.5f) {
        SpawnExplosionAtRandomPosition();
        m_explosionIndex++;
        
        // Trigger haptic for explosion
        if (m_platformDelegates) {
            HapticHelpers::TriggerHeavyCollision(*m_platformDelegates);
        }
    }
    
    // Start fade to white at 2.0 seconds
    if (m_deathSequenceTimer >= 2.0f) {
        float fadeTime = m_deathSequenceTimer - 2.0f;
        m_whiteFadeAlpha = Gnosis::Clamp(fadeTime / 2.0f, 0.0f, 1.0f); // Fade over 2 seconds (slower)
        
        // Mark sequence as complete when fade finishes
        if (m_whiteFadeAlpha >= 1.0f && !m_deathSequenceComplete) {
            m_deathSequenceComplete = true;
            isActive = false;
            GN_LOG_INFO("💀 Rat King death sequence complete - ready to return to main menu");
        }
    }
}

void BossSystem::SpawnExplosionAtRandomPosition() {
    if (!m_explosionSystem) return;
    
    // Calculate boss center (128x128 sprite scaled by 8 = 1024x1024)
    GNVector2 bossCenter = {position.x + (64.0f * scale), position.y + (64.0f * scale)};
    
    // Random position spanning the ENTIRE boss sprite area (1024x1024 pixels)
    // Offset range: -512 to +512 pixels from center (covers full 1024px sprite)
    float randX = (rand() % 1024) - 512.0f;
    float randY = (rand() % 1024) - 512.0f;
    GNVector2 explosionPos = {bossCenter.x + randX, bossCenter.y + randY};
    
    // Spawn small explosion with slow animation
    m_explosionSystem->SpawnSmallExplosion(explosionPos, scale, 0.5f);
    GN_LOG_INFO("Spawned small explosion at (" + std::to_string(explosionPos.x) + ", " + std::to_string(explosionPos.y) + ")");
}

void BossSystem::ChangeState(RatKingState newState) {
    GN_LOG_INFO("Rat King changing state from " + std::to_string(static_cast<int>(currentState)) +
               " to " + std::to_string(static_cast<int>(newState)));

    currentState = newState;
    hasFiredProjectile = false;

    switch (newState) {
        case RatKingState::IDLE:
            GN_LOG_DEBUG("BossSystem: Setting IDLE state - hiding arms");
            SetCurrentSprite(sprites.idleSprite);
            // Hide arm sprites during idle
            SetArmSpriteVisibility(false, false);
            // Hide lock-on dots
            for (Entity dotEntity : aimingData.dotEntities) {
                DebugDraw* debugDraw = m_ecsSystem->GetComponent<DebugDraw>(dotEntity);
                if (debugDraw) debugDraw->showCollider = false;
            }
            idleTimer = 0.0f;
            break;

        case RatKingState::WALKING_LEFT:
        case RatKingState::WALKING_RIGHT:
            GN_LOG_DEBUG("BossSystem: Setting WALKING state - hiding arms");
            SetCurrentSprite(sprites.walkSprite);
            // Hide arm sprites during walking
            SetArmSpriteVisibility(false, false);
            // Hide lock-on dots
            for (Entity dotEntity : aimingData.dotEntities) {
                DebugDraw* debugDraw = m_ecsSystem->GetComponent<DebugDraw>(dotEntity);
                if (debugDraw) debugDraw->showCollider = false;
            }
            walkTimer = 0.0f;
            break;

        case RatKingState::AIMING:
            GN_LOG_DEBUG("BossSystem: Setting AIMING state - showing arms");
            // Use torso for aiming
            SetCurrentSprite(sprites.torsoSprite);
            // Show arm sprites for aiming
            SetArmSpriteVisibility(true, true);
            aimingData.aimTimer = 0.0f;
            aimingData.hasLockedOn = false;
            aimingData.currentArmAngle = 180.0f;  // Start facing down

            // Ensure lock-on dots are created and ready
            if (aimingData.dotEntities.empty()) {
                GN_LOG_INFO("BossSystem: Creating lock-on dots on AIMING state entry");
                CreateLockOnDotEntities();
            } else {
                GN_LOG_INFO("BossSystem: Lock-on dots already exist (" + std::to_string(aimingData.dotEntities.size()) + " dots)");
            }

            // Debug: Check arm sprite visibility after setting
            if (backArmEntity != 0) {
                Sprite* backSprite = m_ecsSystem->GetComponent<Sprite>(backArmEntity);
                if (backSprite) {
                    GN_LOG_DEBUG("BossSystem: AIMING - Back arm sprite visible=" + std::to_string(backSprite->visible) +
                                ", textureId=" + backSprite->textureId +
                                ", currentFrame=" + std::to_string(backSprite->currentFrame));
                }
            }
            if (frontArmEntity != 0) {
                Sprite* frontSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
                if (frontSprite) {
                    GN_LOG_DEBUG("BossSystem: AIMING - Front arm sprite visible=" + std::to_string(frontSprite->visible) +
                                ", textureId=" + frontSprite->textureId +
                                ", currentFrame=" + std::to_string(frontSprite->currentFrame));
                }
            }
            break;

        case RatKingState::THROWING:
            GN_LOG_DEBUG("BossSystem: Setting THROWING state - showing arms");
            // Use torso for throwing
            SetCurrentSprite(sprites.torsoSprite);
            // Show arm sprites for throwing
            SetArmSpriteVisibility(true, true);
            // Hide lock-on dots during throwing
            for (Entity dotEntity : aimingData.dotEntities) {
                DebugDraw* debugDraw = m_ecsSystem->GetComponent<DebugDraw>(dotEntity);
                if (debugDraw) debugDraw->showCollider = false;
            }
            hasFiredProjectile = false;  // Reset projectile flag

            // Ensure torso sprite is playing for throwing animation
            if (bossEntity != 0) {
                Sprite* torsoSprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
                if (torsoSprite) {
                    torsoSprite->playing = true;
                    torsoSprite->currentFrame = 0;  // Start from beginning
                }
            }

            // Debug: Check arm sprite visibility after setting
            if (backArmEntity != 0) {
                Sprite* backSprite = m_ecsSystem->GetComponent<Sprite>(backArmEntity);
                if (backSprite) {
                    GN_LOG_DEBUG("BossSystem: THROWING - Back arm sprite visible=" + std::to_string(backSprite->visible) +
                                ", textureId=" + backSprite->textureId +
                                ", currentFrame=" + std::to_string(backSprite->currentFrame));
                }
            }
            if (frontArmEntity != 0) {
                Sprite* frontSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
                if (frontSprite) {
                    GN_LOG_DEBUG("BossSystem: THROWING - Front arm sprite visible=" + std::to_string(frontSprite->visible) +
                                ", textureId=" + frontSprite->textureId +
                                ", currentFrame=" + std::to_string(frontSprite->currentFrame));
                }
            }
            break;

        case RatKingState::HURT:
            GN_LOG_INFO("🛡️ BossSystem: Setting HURT state - position=(" + std::to_string(position.x) + ", " + std::to_string(position.y) + ") hiding arms");
            SetCurrentSprite(sprites.hurtSprite);
            sprites.hurtSprite->isAnimated = true;
            sprites.hurtSprite->currentFrame = 0;
            sprites.hurtSprite->currentFrameTime = 0.0f;
            sprites.hurtSprite->frameTime = 0.15f; // Adjust to match 6 frames in 0.9s
            // Hide arm sprites during hurt
            SetArmSpriteVisibility(false, false);
            // Hide lock-on dots immediately when hurt (cancel aiming)
            for (Entity dotEntity : aimingData.dotEntities) {
                DebugDraw* debugDraw = m_ecsSystem->GetComponent<DebugDraw>(dotEntity);
                if (debugDraw) debugDraw->showCollider = false;
            }
            // Reset aiming state
            aimingData.hasLockedOn = false;
            aimingData.aimTimer = 0.0f;
            GN_LOG_INFO("🛡️ BossSystem: HURT state setup complete - position=(" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");
            hasFlashedHurt = false;
            hurtTimer = 0.0f;
            break;

        case RatKingState::DEATH:
            GN_LOG_INFO("BossSystem: Setting DEATH state - hiding arms, starting death animation");
            SetCurrentSprite(sprites.deathSprite);
            // Hide arm sprites during death
            SetArmSpriteVisibility(false, false);
            // Hide lock-on dots
            for (Entity dotEntity : aimingData.dotEntities) {
                DebugDraw* debugDraw = m_ecsSystem->GetComponent<DebugDraw>(dotEntity);
                if (debugDraw) debugDraw->showCollider = false;
            }
            deathTimer = 0.0f;
            
            // Ensure death sprite is playing
            if (bossEntity != 0) {
                Sprite* deathSprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
                if (deathSprite) {
                    deathSprite->playing = true;
                    deathSprite->currentFrame = 0;
                    GN_LOG_INFO("BossSystem: Death animation started - frameCount=" + std::to_string(deathSprite->frameCount));
                }
            }
            break;
    }

    // Reset animation for new sprite
    if (bossEntity != 0 && m_ecsSystem) {
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
        if (sprite) {
            sprite->Reset();
            GN_LOG_DEBUG("BossSystem: Reset sprite animation for new state");
        } else {
            GN_LOG_ERROR("BossSystem: Cannot reset sprite - boss entity missing Sprite component!");
        }
    } else {
        GN_LOG_ERROR("BossSystem: Cannot reset sprite - bossEntity=" + std::to_string(bossEntity) + ", m_ecsSystem=" + std::to_string(m_ecsSystem != nullptr));
    }
}

void BossSystem::SetPlayerPosition(GNVector2 playerPos) {
    aimingData.playerPosition = playerPos;
}

void BossSystem::Reset() {
    GN_LOG_INFO("BossSystem: Resetting boss to initial state");
    
    // Reset health
    health = maxHealth;
    
    // Reset state
    ChangeState(RatKingState::IDLE);
    
    // Reset timers
    idleTimer = 0.0f;
    walkTimer = 0.0f;
    hurtTimer = 0.0f;
    deathTimer = 0.0f;
    
    // Reset animation state
    hasFiredProjectile = false;
    hurtBuffer = 0;
    hasFlashedHurt = false;
    hurtFlashTimer = 0.0f;
    
    // Reset minion spawning threshold
    nextMinionHealthThreshold = 185;
    hasTriggeredLowHealthMusic = false;
    
    // Reset aiming data
    aimingData.aimTimer = 0.0f;
    aimingData.hasLockedOn = false;
    aimingData.lockOnAngle = 0.0f;
    aimingData.currentArmAngle = 180.0f;
    aimingData.lockOnDots.clear();
    
    // Reset death sequence state
    m_deathSequenceStarted = false;
    m_deathSequenceComplete = false;
    m_deathSequenceTimer = 0.0f;
    m_whiteFadeAlpha = 0.0f;
    m_hasPlayedScreech = false;
    m_hasPlayedBossKill = false;
    m_explosionIndex = 0;
    
    // Destroy and recreate lock-on dot entities to ensure clean state
    DestroyLockOnDotEntities();
    
    GN_LOG_INFO("BossSystem: Reset complete - health=" + std::to_string(health) + "/" + std::to_string(maxHealth));
}

void BossSystem::SpawnProjectile() {
    if (!m_projectileSystem) return;
    
    // Don't spawn projectiles if boss is dead or inactive
    if (!isActive || currentState == RatKingState::DEATH) {
        GN_LOG_WARN("BossSystem: SpawnProjectile blocked - boss is dead or inactive");
        return;
    }

    GNVector2 shoulder = GetShoulderPosition();
    
    // Calculate launch position (hand location, lowered by 16*scale) - MUST MATCH lock-on dots!
    Gnosis::GNVector2 handLoc = { shoulder.x - (40.0f * scale), shoulder.y + (16.0f * scale) };
    
    // FIXED: Use the SAME target adjustment as lock-on dots for accurate aiming
    // Offset target 8px*scale higher for better visual accuracy (matches UpdateLockOnIndicator line 759)
    Gnosis::GNVector2 adjustedTarget = {aimingData.playerPosition.x, aimingData.playerPosition.y - (8.0f * scale)};
    Gnosis::GNVector2 toPlayer = Gnosis::Vector2Subtract(adjustedTarget, handLoc);
    float length = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
    GNVector2 direction = {toPlayer.x / length, toPlayer.y / length};
    
    // Log the angle for debugging (convert direction back to angle)
    float angleRad = atan2(direction.y, direction.x);

    // Spawn position: hand location offset 20 pixels in throw direction (same as lock-on dots)
    Gnosis::GNVector2 directionOffset = Gnosis::Vector2Scale(direction, 20.0f * scale);
    Gnosis::GNVector2 spawnPos = Gnosis::Vector2Add(handLoc, directionOffset);

    // Spawn toilet paper projectile with proper speed
    Entity projectile = m_projectileSystem->SpawnEnemyProjectile(
        GNVector2(spawnPos.x, spawnPos.y),
        GNVector2(direction.x, direction.y),
        ProjectileType::TOILET_PAPER,
        1  // damage
    );

    if (projectile != 0) {
        // FIXED: Increased projectile speed from 500.0f to 1000.0f for faster, more accurate projectiles
        Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
        if (physics) {
            physics->velocity = direction * 1000.0f; // Doubled speed for faster, more accurate aiming
        }

        GN_LOG_INFO("Rat King spawned toilet paper projectile at angle: " +
                   std::to_string(aimingData.currentArmAngle) + " degrees, angleRad: " + std::to_string(angleRad) +
                   ", direction: (" + std::to_string(direction.x) + ", " + std::to_string(direction.y) +
                   "), shoulder: (" + std::to_string(shoulder.x) + ", " + std::to_string(shoulder.y) +
                   "), handLoc: (" + std::to_string(handLoc.x) + ", " + std::to_string(handLoc.y) +
                   "), adjustedTarget: (" + std::to_string(adjustedTarget.x) + ", " + std::to_string(adjustedTarget.y) +
                   "), spawn: (" + std::to_string(spawnPos.x) + ", " + std::to_string(spawnPos.y) +
                   "), velocity: (" + std::to_string(direction.x * 1000.0f) + ", " + std::to_string(direction.y * 1000.0f) + ")");
    }

    // Dual projectile at low health (below 20%)
    if (health <= 4) {
        float offsetRad = Gnosis::DEG2RAD * (rand() % 21 - 10); // Random value between -10 and 10 degrees
        float dualAngleRad = angleRad + offsetRad;
        GNVector2 dualDirection = {cosf(dualAngleRad), sinf(dualAngleRad)};

        // Normalize dual direction
        length = sqrtf(dualDirection.x * dualDirection.x + dualDirection.y * dualDirection.y);
        if (length > 0.0f) {
            dualDirection.x /= length;
            dualDirection.y /= length;
        }

        Gnosis::GNVector2 dualHandLoc = { shoulder.x - (40.0f * scale), shoulder.y + (16.0f * scale) };
        Gnosis::GNVector2 dualDirectionScaled = Gnosis::Vector2Scale(dualDirection, 20.0f * scale);
        Gnosis::GNVector2 dualSpawnPos = Gnosis::Vector2Add(dualHandLoc, dualDirectionScaled);

        Entity dualProjectile = m_projectileSystem->SpawnEnemyProjectile(
            GNVector2(dualSpawnPos.x, dualSpawnPos.y),
            GNVector2(dualDirection.x, dualDirection.y),
            ProjectileType::TOILET_PAPER,
            1  // damage
        );

        if (dualProjectile != 0) {
            // Set dual projectile speed (also increased to 1000.0f)
            Physics* dualPhysics = m_ecsSystem->GetComponent<Physics>(dualProjectile);
            if (dualPhysics) {
                dualPhysics->velocity = dualDirection * 1000.0f;
            }

            GN_LOG_INFO("Rat King spawned dual toilet paper projectile at angle: " +
                       std::to_string((angleRad + offsetRad) * Gnosis::RAD2DEG) + " degrees");
        }
    }
}

void BossSystem::SpawnMinionWave(int count) {
    if (!m_levelManager) return;

    GN_LOG_INFO("Rat King spawning " + std::to_string(count) + " RatCopter minions");

    // Get RatCopter config from registry
    EnemyConfigRegistry::Initialize();
    const EnemyConfig& ratCopterConfig = EnemyConfigRegistry::GetConfig("RatCopterIdle");

    if (ratCopterConfig.textureId.empty()) {
        GN_LOG_ERROR("BossSystem: Could not find RatCopter config");
        return;
    }

    // Spawn minions with varied Y positions across the playable area
    // 20% from top to 80% from bottom for best results
    float spawnX = position.x + 250.0f; // Spawn off to the right of boss
    float minY = screenHeight * 0.20f;  // 20% from top
    float maxY = screenHeight * 0.80f; // 80% from top (20% from bottom)

    for (int i = 0; i < count; ++i) {
        // Add random variance to X position
        float randomXOffset = (rand() % 100 - 50) * scale; // ±50 scaled pixels
        float x = spawnX + randomXOffset + (i * 32.0f * scale); // Stagger horizontally
        
        // Random Y position with good variance (20% from top to 80% from bottom)
        float randomYFactor = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float y = minY + (randomYFactor * (maxY - minY));

        // Spawn the minion
        Gnosis::Entity minion = m_levelManager->SpawnEnemy(ratCopterConfig, x, y);
        if (minion != 0) {
            // Mark as boss minion to prevent castle-level wrapping behavior
            Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(minion);
            if (enemyComp) {
                enemyComp->isBossMinion = true;
            }
            
            GN_LOG_INFO("Rat King spawned RatCopter minion " + std::to_string(i + 1) +
                       " at position (" + std::to_string(x) + ", " + std::to_string(y) + ") as boss minion");
        }
    }
}

void BossSystem::UpdateLockOnIndicator() {
    if (!m_ecsSystem) return;
    
    // Ensure dot entities exist
    if (aimingData.dotEntities.empty()) {
        GN_LOG_INFO("BossSystem: Creating lock-on dot entities for first time");
        CreateLockOnDotEntities();
    }
    
    aimingData.lockOnDots.clear();

    int dots = 20;  // Reduced for better visibility
    // Increase spacing so dots reach from launch to player
    float spacing = 32.0f * (screenWidth / 1179.0f); // Larger spacing (32px) so dots reach player
    float progress = Gnosis::Clamp(aimingData.aimTimer / aimingData.aimDuration, 0.0f, 1.0f);

    GNVector2 shoulder = GetShoulderPosition();

    GN_LOG_DEBUG("BossSystem: UpdateLockOnIndicator - progress=" + std::to_string(progress) + 
                 ", aimTimer=" + std::to_string(aimingData.aimTimer) +
                 ", aimDuration=" + std::to_string(aimingData.aimDuration) +
                 ", shoulder=(" + std::to_string(shoulder.x) + "," + std::to_string(shoulder.y) + ")");

    // Oscillation effect before lock-on (creates an arcing sweep until locked)
    float oscillationAmount = 0.0f;
    if (!aimingData.hasLockedOn && progress < 1.0f) {
        // Oscillate with a sine wave that dampens as we approach lock-on
        float oscillationSpeed = 8.0f;  // Speed of oscillation
        float oscillationRange = 15.0f * (1.0f - progress);  // Dampens as we get closer to lock
        oscillationAmount = sinf(aimingData.aimTimer * oscillationSpeed) * oscillationRange;
    }

    // Calculate launch position (hand location, lowered by 16*scale)
    GNVector2 handLoc = { shoulder.x - (40.0f * scale), shoulder.y + (16.0f * scale) };
    
    // Always aim directly at player CENTER (not just X position)
    // The playerPosition is already the center from SetPlayerPosition
    Gnosis::GNVector2 toPlayer = Gnosis::Vector2Subtract(aimingData.playerPosition, handLoc);
    float distanceToPlayer = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
    GNVector2 direction = {toPlayer.x / distanceToPlayer, toPlayer.y / distanceToPlayer};
    
    // Flash effect: in the last 20% of aiming duration, flash red and white
    bool isFlashing = (progress >= 0.80f);
    bool showWhite = false;
    if (isFlashing) {
        // Flash at 10Hz (10 times per second)
        float flashTimer = aimingData.aimTimer * 10.0f;
        showWhite = (static_cast<int>(flashTimer) % 2 == 0);
    }
    
    for (int i = 0; i < dots && i < (int)aimingData.dotEntities.size(); ++i) {
        float fill = (float)i / (float)dots;
        
        // Place dots along the line from launch position to player CENTER
        float distance = i * spacing;
        Gnosis::GNVector2 dotPos = Gnosis::Vector2Add(handLoc, Gnosis::Vector2Scale(direction, distance));

        // Store dot data
        LockOnDot dot;
        dot.position = dotPos;
        dot.progress = fill;
        aimingData.lockOnDots.push_back(dot);
        
        // Update the dot entity's transform and visibility
        Entity dotEntity = aimingData.dotEntities[i];
        Transform* dotTransform = m_ecsSystem->GetComponent<Transform>(dotEntity);
        DebugDraw* debugDraw = m_ecsSystem->GetComponent<DebugDraw>(dotEntity);
        
        if (dotTransform && debugDraw) {
            // Show dot if within progress range
            if (fill <= progress && aimingData.aimTimer > 0.1f) { // Small delay for fade-in
                dotTransform->position = dotPos;
                dotTransform->scale = {scale, scale}; // Scale dot with boss scale
                debugDraw->showCollider = true;
                
                // Color: flash red/white in final 20% of aiming, otherwise yellow->red gradient
                if (isFlashing) {
                    if (showWhite) {
                        debugDraw->colliderColor.r = static_cast<uint8_t>(255);
                        debugDraw->colliderColor.g = static_cast<uint8_t>(255);
                        debugDraw->colliderColor.b = static_cast<uint8_t>(255);
                    } else {
                        debugDraw->colliderColor.r = static_cast<uint8_t>(255);
                        debugDraw->colliderColor.g = static_cast<uint8_t>(0);
                        debugDraw->colliderColor.b = static_cast<uint8_t>(0);
                    }
                } else {
                    // Color interpolation from yellow to red based on progress
                    float colorProgress = fill;
                    debugDraw->colliderColor.r = static_cast<uint8_t>(255);  // Always full red
                    debugDraw->colliderColor.g = static_cast<uint8_t>(255 * (1.0f - colorProgress));  // Yellow -> Red
                    debugDraw->colliderColor.b = static_cast<uint8_t>(0);
                }
                debugDraw->colliderColor.a = static_cast<uint8_t>(255);
                
                if (i < 3) {  // Log first 3 dots for debugging
                    GN_LOG_DEBUG("BossSystem: Dot " + std::to_string(i) + " visible at (" + 
                                std::to_string(dotPos.x) + "," + std::to_string(dotPos.y) + 
                                "), color=(" + std::to_string((int)debugDraw->colliderColor.r) + "," + 
                                std::to_string((int)debugDraw->colliderColor.g) + "," + 
                                std::to_string((int)debugDraw->colliderColor.b) + ")");
                }
            } else {
                debugDraw->showCollider = false;
            }
        }
    }
    
    // Hide any remaining dots beyond the progress
    for (size_t i = aimingData.lockOnDots.size(); i < aimingData.dotEntities.size(); ++i) {
        DebugDraw* debugDraw = m_ecsSystem->GetComponent<DebugDraw>(aimingData.dotEntities[i]);
        if (debugDraw) {
            debugDraw->showCollider = false;
        }
    }
}

void BossSystem::CreateLockOnDotEntities() {
    if (!m_ecsSystem) return;
    
    // Destroy any existing dots first
    DestroyLockOnDotEntities();
    
    // Create 20 dot entities (reduced for better visibility)
    int dotCount = 20;
    aimingData.dotEntities.reserve(dotCount);
    
    for (int i = 0; i < dotCount; ++i) {
        Entity dotEntity = m_ecsSystem->CreateEntity();
        
        // Add transform (position will be updated each frame)
        Transform dotTransform;
        dotTransform.position = {0, 0};
        dotTransform.scale = {1.0f, 1.0f};
        m_ecsSystem->AddComponent<Transform>(dotEntity, dotTransform);
        
        // Add small sprite for world-space rendering
        Sprite dotSprite("", 4.0f * scale, 4.0f * scale); // 4px sprite scaled up
        dotSprite.visible = false;
        dotSprite.layer = 10; // World space layer
        dotSprite.color = Gnosis::GNColor(255, 255, 0, 255);
        m_ecsSystem->AddComponent<Sprite>(dotEntity, dotSprite);
        
        // Add DebugDraw for filled circle visualization in world space
        DebugDraw debugDraw;
        debugDraw.showBounds = false;
        debugDraw.showCollider = true;
        debugDraw.colliderColor = Gnosis::GNColor(255, 255, 0, 255);
        debugDraw.alpha = 1.0f;
        debugDraw.debugLayer = 10;
        m_ecsSystem->AddComponent<DebugDraw>(dotEntity, debugDraw);
        
        // Add circular hitbox for rendering the dot
        Hitbox dotHitbox;
        dotHitbox.type = ColliderType::Circle;
        dotHitbox.radius = 2.0f; // 2px radius, will be scaled by transform
        dotHitbox.offsetX = 0.0f;
        dotHitbox.offsetY = 0.0f;
        m_ecsSystem->AddComponent<Hitbox>(dotEntity, dotHitbox);
        
        aimingData.dotEntities.push_back(dotEntity);
    }
    
    GN_LOG_INFO("BossSystem: Created " + std::to_string(dotCount) + " lock-on dot entities");
}

void BossSystem::DestroyLockOnDotEntities() {
    if (!m_ecsSystem) return;
    
    for (Entity dotEntity : aimingData.dotEntities) {
        if (dotEntity != 0) {
            m_ecsSystem->DestroyEntity(dotEntity);
        }
    }
    
    aimingData.dotEntities.clear();
    GN_LOG_DEBUG("BossSystem: Destroyed all lock-on dot entities");
}

void BossSystem::ChangeMusic(const std::string& musicFile) {
    if (!m_platformDelegates || !m_platformDelegates->audio.playMusic) return;

    GN_LOG_INFO("BossSystem: Changing music to " + musicFile);
    float musicVolume = 0.7f;  // 70% volume for boss music
    m_platformDelegates->audio.playMusic(musicFile.c_str(), musicVolume, -1);
}

Gnosis::GNVector2 BossSystem::GetShoulderPosition() const {
    // Shoulder is at a specific pixel offset from position (matching old system)
    // Old system used: { position.x + 60, position.y + 38 } for 128px sprite at scale 1.0
    // With scale 8.0, we need to scale these offsets: 60 * 8 = 480, 38 * 8 = 304
    return {
        position.x + (60.0f * scale),
        position.y + (38.0f * scale)
    };
}

void BossSystem::CreateBodyPartEntities() {
    if (!m_ecsSystem || bossEntity == 0) return;

    // Get the main boss entity's transform for positioning
    Transform* bossTransform = m_ecsSystem->GetComponent<Transform>(bossEntity);
    if (!bossTransform) return;

    // Create back arm entity
    // Position sprite CENTER at shoulder point (like spike ball at base center)
    // Old Raylib: dest = {shoulder.x, shoulder.y, 128*scale, 128*scale}, origin = {64, 64}
    // This means dest position WAS the center position, not top-left!
    backArmEntity = m_ecsSystem->CreateEntity();
    GNVector2 shoulder = GetShoulderPosition();
    Transform backArmTransform(
        Gnosis::GNVector2(shoulder.x + (16.0f * scale), shoulder.y + (32.0f * scale)),  // Position at arm joint (16px right, 32px down from shoulder)
        0.0f,
        Gnosis::GNVector2(scale, scale)
    );
    m_ecsSystem->AddComponent<Transform>(backArmEntity, backArmTransform);
    // Create ANIMATED sprite (7 frames, 128x128 each) - will use animated pivot rendering
    Sprite backArmSprite("RatkingAimBackArmOnly.png", 896.0f, 128.0f, 128, 128, 7, 0.16f);
    backArmSprite.layer = 7; // Behind torso (player is layer 6)
    backArmSprite.visible = false;
    backArmSprite.isAnimated = true;  // Animated sprite with pivot rotation!
    backArmSprite.loop = false;       // Don't loop the throwing animation
    backArmSprite.playing = false;    // Start paused
    m_ecsSystem->AddComponent<Sprite>(backArmEntity, backArmSprite);

    // Add PivotRotationRenderer with manual control enabled
    // Pivot at sprite center (0,0) since we positioned sprite center at shoulder
    // Old Raylib used origin={64,64} with dest at shoulder, meaning pivot at center
    PivotRotationRenderer backArmPivot(true, 0.0f, 0.0f, 0.0f, true); // manual=true, pivot at center (shoulder)
    m_ecsSystem->AddComponent<PivotRotationRenderer>(backArmEntity, backArmPivot);

    // Verify component was added
    if (m_ecsSystem->HasComponent<PivotRotationRenderer>(backArmEntity)) {
        GN_LOG_INFO("BossSystem: ✅ Back arm PivotRotationRenderer component added successfully");
    } else {
        GN_LOG_ERROR("BossSystem: ❌ Failed to add PivotRotationRenderer to back arm!");
    }

    GN_LOG_INFO("BossSystem: Created back arm entity " + std::to_string(backArmEntity));

    // Create front arm entity
    // Position sprite CENTER at shoulder point (like spike ball at base center)
    // Old Raylib: dest = {shoulder.x, shoulder.y, 128*scale, 128*scale}, origin = {64, 64}
    // This means dest position WAS the center position, not top-left!
    frontArmEntity = m_ecsSystem->CreateEntity();
    Transform frontArmTransform(
        Gnosis::GNVector2(shoulder.x + (16.0f * scale), shoulder.y + (32.0f * scale)),  // Position at arm joint (16px right, 32px down from shoulder)
        0.0f,
        Gnosis::GNVector2(scale, scale)
    );
    m_ecsSystem->AddComponent<Transform>(frontArmEntity, frontArmTransform);
    // Create ANIMATED sprite (7 frames, 128x128 each) - will use animated pivot rendering
    Sprite frontArmSprite("RatkingAimTossArmOnly.png", 896.0f, 128.0f, 128, 128, 7, 0.16f);
    frontArmSprite.layer = 9; // In front of torso (layer 8)
    frontArmSprite.visible = false;
    frontArmSprite.isAnimated = true;  // Animated sprite with pivot rotation!
    frontArmSprite.loop = false;       // Don't loop the throwing animation
    frontArmSprite.playing = false;    // Start paused
    m_ecsSystem->AddComponent<Sprite>(frontArmEntity, frontArmSprite);

    // Add PivotRotationRenderer with manual control enabled
    // Pivot at sprite center (0,0) since we positioned sprite center at shoulder
    // Old Raylib used origin={64,64} with dest at shoulder, meaning pivot at center
    PivotRotationRenderer frontArmPivot(true, 0.0f, 0.0f, 0.0f, true); // manual=true, pivot at center (shoulder)
    m_ecsSystem->AddComponent<PivotRotationRenderer>(frontArmEntity, frontArmPivot);

    // Verify component was added
    if (m_ecsSystem->HasComponent<PivotRotationRenderer>(frontArmEntity)) {
        GN_LOG_INFO("BossSystem: ✅ Front arm PivotRotationRenderer component added successfully");
    } else {
        GN_LOG_ERROR("BossSystem: ❌ Failed to add PivotRotationRenderer to front arm!");
    }

    GN_LOG_INFO("BossSystem: Created front arm entity " + std::to_string(frontArmEntity));

    GN_LOG_INFO("BossSystem: All body part entities created");
}

void BossSystem::LoadSprites() {
    GN_LOG_INFO("BossSystem: Loading Rat King sprites with multi-entity approach");

    // Load sprites for main body (torso)
    sprites.idleSprite = new Sprite("Ratking", 128.0f, 128.0f);
    sprites.idleSprite->isAnimated = false;
    sprites.idleSprite->visible = true;
    sprites.idleSprite->layer = 8;

    sprites.walkSprite = new Sprite("RatkingWalk", 128.0f, 128.0f);
    sprites.walkSprite->isAnimated = true;
    sprites.walkSprite->frameWidth = 128;
    sprites.walkSprite->frameHeight = 128;
    sprites.walkSprite->frameCount = 8;
    sprites.walkSprite->frameTime = 0.12f;
    sprites.walkSprite->loop = true;
    sprites.walkSprite->visible = false;
    sprites.walkSprite->layer = 8;

    sprites.torsoSprite = new Sprite("RatkingAimTorsoOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
    sprites.torsoSprite->loop = false;
    sprites.torsoSprite->visible = false;
    sprites.torsoSprite->layer = 8;

    // Load sprites for arms (these will be used by the arm entities)
    sprites.backArmSprite = new Sprite("RatkingAimBackArmOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
    sprites.backArmSprite->loop = false;
    sprites.backArmSprite->visible = false;
    sprites.backArmSprite->layer = 7; // Behind torso (layer 8)

    sprites.frontArmSprite = new Sprite("RatkingAimTossArmOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
    sprites.frontArmSprite->loop = false;
    sprites.frontArmSprite->visible = false;
    sprites.frontArmSprite->layer = 9; // In front of torso (layer 8)

    // Load hurt and death sprites
    sprites.hurtSprite = new Sprite("RatkingHurt", 128.0f, 128.0f, 128, 128, 6, 0.15f);
    sprites.hurtSprite->loop = false;
    sprites.hurtSprite->visible = false;
    sprites.hurtSprite->layer = 8;

    sprites.deathSprite = new Sprite("RatkingDeath", 128.0f, 128.0f, 128, 128, 6, 0.20f);
    sprites.deathSprite->loop = false;
    sprites.deathSprite->visible = false;
    sprites.deathSprite->layer = 8;

    GN_LOG_INFO("BossSystem: All Rat King sprites loaded for multi-entity system");
}

void BossSystem::UnloadSprites() {
    // Clean up sprites
    delete sprites.idleSprite;
    delete sprites.walkSprite;
    delete sprites.torsoSprite;
    delete sprites.backArmSprite;
    delete sprites.frontArmSprite;
    delete sprites.hurtSprite;
    delete sprites.deathSprite;

    sprites = RatKingSprites{}; // Reset to null
    GN_LOG_INFO("BossSystem: Sprites unloaded");
}

void BossSystem::SetCurrentSprite(Sprite* sprite) {
    if (!sprite) {
        GN_LOG_ERROR("BossSystem: SetCurrentSprite called with null sprite!");
        return;
    }

    if (bossEntity == 0) {
        GN_LOG_ERROR("BossSystem: SetCurrentSprite called with bossEntity=0!");
        return;
    }

    if (!m_ecsSystem) {
        GN_LOG_ERROR("BossSystem: SetCurrentSprite called with null m_ecsSystem!");
        return;
    }

    GN_LOG_DEBUG("BossSystem: SetCurrentSprite called - sprite texture: " +
                (sprite->textureId.empty() ? "EMPTY" : sprite->textureId) +
                ", bossEntity=" + std::to_string(bossEntity));

    Sprite* entitySprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
    if (entitySprite) {
        GN_LOG_DEBUG("BossSystem: Found existing sprite component, copying properties");

        // Copy sprite properties
        *entitySprite = *sprite;
        entitySprite->Reset();

        // Ensure sprite is visible and positioned correctly
        entitySprite->visible = true;
        GN_LOG_DEBUG("BossSystem: Set sprite visible=true, textureId=" + entitySprite->textureId);

        // Update transform position - only update stored position, don't override LevelManager's positioning
        Transform* entityTransform = m_ecsSystem->GetComponent<Transform>(bossEntity);
        if (entityTransform) {
            // Sync the stored position with the current transform position (set by LevelManager)
            position.x = entityTransform->position.x;
            position.y = entityTransform->position.y;
            GN_LOG_DEBUG("BossSystem: Synced stored position with transform position (" +
                        std::to_string(position.x) + "," + std::to_string(position.y) + ")");
        } else {
            GN_LOG_ERROR("BossSystem: Boss entity missing Transform component during SetCurrentSprite!");
        }
    } else {
        GN_LOG_ERROR("BossSystem: Boss entity " + std::to_string(bossEntity) + " missing Sprite component!");
    }
}

void BossSystem::UpdateArmRotations() {
    if (currentState != RatKingState::AIMING) return;

    GNVector2 shoulder = GetShoulderPosition();
    
    // Log arm positions
    if (backArmEntity != 0) {
        Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
        if (backArmTransform) {
            GN_LOG_INFO("🦾 BACK ARM TRANSFORM: pos=(" + std::to_string(backArmTransform->position.x) + ", " + 
                       std::to_string(backArmTransform->position.y) + ") rotation=" + 
                       std::to_string(backArmTransform->rotation) + "° scale=(" + 
                       std::to_string(backArmTransform->scale.x) + ", " + std::to_string(backArmTransform->scale.y) + ")");
        }
    }
    
    if (frontArmEntity != 0) {
        Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
        if (frontArmTransform) {
            GN_LOG_INFO("🦾 FRONT ARM TRANSFORM: pos=(" + std::to_string(frontArmTransform->position.x) + ", " + 
                       std::to_string(frontArmTransform->position.y) + ") rotation=" + 
                       std::to_string(frontArmTransform->rotation) + "° scale=(" + 
                       std::to_string(frontArmTransform->scale.x) + ", " + std::to_string(frontArmTransform->scale.y) + ")");
        }
    }

    // Calculate angle to player
    Gnosis::GNVector2 toPlayer = Gnosis::Vector2Subtract(aimingData.playerPosition, shoulder);
    float targetAngle = atan2(toPlayer.y, toPlayer.x) * Gnosis::RAD2DEG;
    
    // Calculate progress through aiming duration
    float progress = Gnosis::Clamp(aimingData.aimTimer / aimingData.aimDuration, 0.0f, 1.0f);
    
    // Oscillation effect BEFORE lock-on (arms swing ±90° from horizontal baseline)
    // Old Raylib: arms oscillate, then lock onto player
    // Baseline is 180° (pointing left), oscillate ±90° (90° = up, 270° = down)
    float displayAngle = targetAngle;
    if (!aimingData.hasLockedOn && progress < 1.0f) {
        // Wide oscillation that dampens as we approach lock-on
        float oscillationSpeed = 3.0f;  // Oscillation speed
        float oscillationRange = 90.0f * (1.0f - progress);  // Start at ±90°, dampen to 0
        float oscillationAmount = sinf(aimingData.aimTimer * oscillationSpeed) * oscillationRange;
        displayAngle = targetAngle + oscillationAmount;
    }

    // Smooth angle interpolation
    float oldAngle = aimingData.currentArmAngle;
    aimingData.currentArmAngle = Gnosis::SmoothAngleLerp(aimingData.currentArmAngle, displayAngle, 0.016f * 8.0f);
    
    // Apply -180° visual offset to match old Raylib rendering (line 85: visualOffset = -180.0f)
    float visualRotation = aimingData.currentArmAngle - 180.0f;

    GN_LOG_DEBUG("BossSystem: UpdateArmRotations - playerPos=(" + std::to_string(aimingData.playerPosition.x) + "," + std::to_string(aimingData.playerPosition.x) +
                 "), shoulder=(" + std::to_string(shoulder.x) + "," + std::to_string(shoulder.y) +
                 "), targetAngle=" + std::to_string(targetAngle) +
                 "), currentAngle=" + std::to_string(aimingData.currentArmAngle) +
                 "), oldAngle=" + std::to_string(oldAngle));

    // Apply rotation AND position to back arm (position must update as boss moves!)
    if (backArmEntity != 0) {
        Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
        PivotRotationRenderer* backPivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(backArmEntity);
        if (backArmTransform && backPivotRenderer) {
            // Update position to track shoulder as boss moves
            backArmTransform->position.x = shoulder.x - (64.0f * scale);
            backArmTransform->position.y = shoulder.y - (64.0f * scale);
            
            // Set the rotation directly on the transform using visual rotation with -180° offset
            float oldRotation = backArmTransform->rotation;
            backArmTransform->rotation = visualRotation;
            // Manual control is enabled, so we don't need to set rotationSpeed
            // The rotation comes directly from Transform.rotation
            GN_LOG_DEBUG("BossSystem: Set back arm rotation from " + std::to_string(oldRotation) + "° to " + std::to_string(aimingData.currentArmAngle) + "°");
            GN_LOG_DEBUG("BossSystem: Back arm transform position: (" + std::to_string(backArmTransform->position.x) + ", " + std::to_string(backArmTransform->position.y) + ")");
        } else {
            GN_LOG_ERROR("BossSystem: Back arm transform or pivot renderer component missing!");
            if (!backArmTransform) GN_LOG_ERROR("BossSystem: Back arm Transform component is null!");
            if (!backPivotRenderer) GN_LOG_ERROR("BossSystem: Back arm PivotRotationRenderer component is null!");
        }
    } else {
        GN_LOG_ERROR("BossSystem: Back arm entity is 0!");
    }

    // Apply rotation AND position to front arm (position must update as boss moves!)
    if (frontArmEntity != 0) {
        Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
        PivotRotationRenderer* frontPivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(frontArmEntity);
        if (frontArmTransform && frontPivotRenderer) {
            // Update position to track shoulder as boss moves
            frontArmTransform->position.x = shoulder.x - (64.0f * scale);
            frontArmTransform->position.y = shoulder.y - (64.0f * scale);
            
            // Set the rotation directly on the transform using visual rotation with -180° offset
            float oldRotation = frontArmTransform->rotation;
            frontArmTransform->rotation = visualRotation;
            // Manual control is enabled, so we don't need to set rotationSpeed
            // The rotation comes directly from Transform.rotation
            GN_LOG_DEBUG("BossSystem: Set front arm rotation from " + std::to_string(oldRotation) + "° to " + std::to_string(aimingData.currentArmAngle) + "°");
            GN_LOG_DEBUG("BossSystem: Front arm transform position: (" + std::to_string(frontArmTransform->position.x) + ", " + std::to_string(frontArmTransform->position.y) + ")");
        } else {
            GN_LOG_ERROR("BossSystem: Front arm transform or pivot renderer component missing!");
            if (!frontArmTransform) GN_LOG_ERROR("BossSystem: Front arm Transform component is null!");
            if (!frontPivotRenderer) GN_LOG_ERROR("BossSystem: Front arm PivotRotationRenderer component is null!");
        }
    } else {
        GN_LOG_ERROR("BossSystem: Front arm entity is 0!");
    }
}

void BossSystem::SetArmSpriteVisibility(bool showBackArm, bool showFrontArm) {
    if (!m_ecsSystem) {
        GN_LOG_ERROR("BossSystem: SetArmSpriteVisibility called with null m_ecsSystem!");
        return;
    }

    GN_LOG_DEBUG("BossSystem: SetArmSpriteVisibility - backArm=" + std::to_string(showBackArm) +
                ", frontArm=" + std::to_string(showFrontArm) +
                ", backArmEntity=" + std::to_string(backArmEntity) +
                ", frontArmEntity=" + std::to_string(frontArmEntity) +
                ", sprites.backArmSprite=" + std::to_string(sprites.backArmSprite != nullptr) +
                ", sprites.frontArmSprite=" + std::to_string(sprites.frontArmSprite != nullptr));

    // Set back arm visibility
    if (backArmEntity != 0) {
        Sprite* backArmSprite = m_ecsSystem->GetComponent<Sprite>(backArmEntity);
        Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
        if (backArmSprite && backArmTransform) {
            GN_LOG_DEBUG("BossSystem: Setting back arm visibility to " + std::to_string(showBackArm));
            if (showBackArm && sprites.backArmSprite) {
                // Save the layer before copying
                int savedLayer = backArmSprite->layer;
                *backArmSprite = *sprites.backArmSprite;
                backArmSprite->Reset();
                // Restore layer and ensure visibility is set correctly after copying template properties
                backArmSprite->layer = savedLayer;
                backArmSprite->visible = true;
                GN_LOG_DEBUG("BossSystem: Copied back arm sprite properties - textureId=" + backArmSprite->textureId +
                            ", visible=" + std::to_string(backArmSprite->visible) +
                            ", isAnimated=" + std::to_string(backArmSprite->isAnimated));
                
                // CRITICAL: Update arm position to current shoulder position with joint offset
                GNVector2 shoulder = GetShoulderPosition();
                backArmTransform->position = Gnosis::GNVector2(
                    shoulder.x + (16.0f * scale),
                    shoulder.y + (32.0f * scale)
                );
                GN_LOG_INFO("BossSystem: Updated back arm position to shoulder (" + 
                           std::to_string(shoulder.x) + ", " + std::to_string(shoulder.y) + ")");
            } else {
                backArmSprite->visible = false;
            }
            GN_LOG_DEBUG("BossSystem: Set back arm visible=" + std::to_string(showBackArm) +
                        ", final textureId=" + backArmSprite->textureId);
        } else {
            GN_LOG_ERROR("BossSystem: Back arm entity " + std::to_string(backArmEntity) + " missing Sprite/Transform component!");
        }
    } else {
        GN_LOG_DEBUG("BossSystem: No back arm entity to update");
    }

    // Set front arm visibility
    if (frontArmEntity != 0) {
        Sprite* frontArmSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
        Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
        if (frontArmSprite && frontArmTransform) {
            GN_LOG_DEBUG("BossSystem: Setting front arm visibility to " + std::to_string(showFrontArm));
            if (showFrontArm && sprites.frontArmSprite) {
                // Save the layer before copying
                int savedLayer = frontArmSprite->layer;
                *frontArmSprite = *sprites.frontArmSprite;
                frontArmSprite->Reset();
                // Restore layer and ensure visibility is set correctly after copying template properties
                frontArmSprite->layer = savedLayer;
                frontArmSprite->visible = true;
                GN_LOG_DEBUG("BossSystem: Copied front arm sprite properties - textureId=" + frontArmSprite->textureId +
                            ", visible=" + std::to_string(frontArmSprite->visible) +
                            ", isAnimated=" + std::to_string(frontArmSprite->isAnimated));
                
                // CRITICAL: Update arm position to current shoulder position with joint offset
                GNVector2 shoulder = GetShoulderPosition();
                frontArmTransform->position = Gnosis::GNVector2(
                    shoulder.x + (16.0f * scale),
                    shoulder.y + (32.0f * scale)
                );
                GN_LOG_INFO("BossSystem: Updated front arm position to shoulder (" + 
                           std::to_string(shoulder.x) + ", " + std::to_string(shoulder.y) + ")");
            } else {
                frontArmSprite->visible = false;
            }
            GN_LOG_DEBUG("BossSystem: Set front arm visible=" + std::to_string(showFrontArm) +
                        ", final textureId=" + frontArmSprite->textureId);
        } else {
            GN_LOG_ERROR("BossSystem: Front arm entity " + std::to_string(frontArmEntity) + " missing Sprite/Transform component!");
        }
    } else {
        GN_LOG_DEBUG("BossSystem: No front arm entity to update");
    }
}

void BossSystem::UpdateSprites(float deltaTime) {
    if (!m_ecsSystem) {
        GN_LOG_ERROR("BossSystem: UpdateSprites failed - m_ecsSystem is null");
        return;
    }

    GN_LOG_DEBUG("BossSystem: UpdateSprites starting - deltaTime=" + std::to_string(deltaTime));

    // Update hurt flash timer
    if (currentState == RatKingState::HURT) {
        hurtFlashTimer += deltaTime;
    }

    // Update main boss sprite position and hurt flashing
    if (bossEntity != 0) {
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
        if (sprite) {
            GN_LOG_DEBUG("BossSystem: Updating boss sprite - entity=" + std::to_string(bossEntity) +
                        ", visible=" + std::to_string(sprite->visible) +
                        ", currentFrame=" + std::to_string(sprite->currentFrame) +
                        ", textureId=" + sprite->textureId +
                        ", isAnimated=" + std::to_string(sprite->isAnimated) +
                        ", playing=" + std::to_string(sprite->playing) +
                        ", state=" + std::to_string(static_cast<int>(currentState)));

            // Update transform position
            Transform* bossTransform = m_ecsSystem->GetComponent<Transform>(bossEntity);
            if (bossTransform) {
                bossTransform->position.x = position.x;
                bossTransform->position.y = position.y;
                GN_LOG_DEBUG("BossSystem: Updated boss transform position to (" +
                           std::to_string(position.x) + "," + std::to_string(position.y) + ")");
            } else {
                GN_LOG_ERROR("BossSystem: Boss entity " + std::to_string(bossEntity) + " missing Transform component!");
            }

            // Handle hurt flashing effect
            if (IsHurtFlashing()) {
                // Flash white during hurt animation
                float flashIntensity = sinf(hurtFlashTimer * 20.0f) * 0.5f + 0.5f;
                sprite->color.r = 255;
                sprite->color.g = static_cast<unsigned char>(255 * (1.0f - flashIntensity * 0.3f));
                sprite->color.b = static_cast<unsigned char>(255 * (1.0f - flashIntensity * 0.3f));
                GN_LOG_DEBUG("BossSystem: Applied hurt flashing effect");
            } else {
                // Reset to normal color
                sprite->color.r = 255;
                sprite->color.g = 255;
                sprite->color.b = 255;
            }

            // Ensure sprite remains visible
            if (!sprite->visible) {
                sprite->visible = true;
                GN_LOG_WARN("BossSystem: Boss sprite was invisible, forcing visible!");
            }

            // Ensure sprite has proper rendering properties
            // NOTE: DO NOT override layer here - it's set correctly in SetCurrentSprite
            if (!sprite->playing && sprite->isAnimated) {
                sprite->playing = true;
                GN_LOG_WARN("BossSystem: Boss sprite animation was stopped, restarting!");
            }
        } else {
            GN_LOG_ERROR("BossSystem: Boss entity " + std::to_string(bossEntity) + " missing Sprite component!");
        }
    } else {
        GN_LOG_ERROR("BossSystem: bossEntity is 0!");
    }

    // Update arm sprite positions and hurt flashing
    switch (currentState) {
        case RatKingState::AIMING:
            // During AIMING: Freeze all sprites on frame 0 and handle rotation
            if (bossEntity != 0) {
                Sprite* torsoSprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
                if (torsoSprite) {
                    torsoSprite->currentFrame = 0;  // Freeze on first frame
                    torsoSprite->playing = false;    // Stop automatic animation
                }
            }

            // Update positions for arm sprites and freeze them
            if (backArmEntity != 0) {
                Sprite* backArmSprite = m_ecsSystem->GetComponent<Sprite>(backArmEntity);
                Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
                if (backArmSprite && backArmTransform) {
                    backArmSprite->currentFrame = 0;  // Freeze on first frame
                    backArmSprite->playing = false;    // Stop automatic animation
                    // Position at shoulder with arm joint offset (16px right, 32px down in sprite space)
                    GNVector2 shoulder = GetShoulderPosition();
                    backArmTransform->position.x = shoulder.x + (16.0f * scale);
                    backArmTransform->position.y = shoulder.y + (32.0f * scale);

                    // Apply hurt flashing to arm sprites too
                    if (IsHurtFlashing()) {
                        backArmSprite->color.r = 255;
                        backArmSprite->color.g = 200;
                        backArmSprite->color.b = 200;
                    } else {
                        backArmSprite->color.r = 255;
                        backArmSprite->color.g = 255;
                        backArmSprite->color.b = 255;
                    }
                }
            }

            if (frontArmEntity != 0) {
                Sprite* frontArmSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
                Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
                if (frontArmSprite && frontArmTransform) {
                    frontArmSprite->currentFrame = 0;  // Freeze on first frame
                    frontArmSprite->playing = false;    // Stop automatic animation
                    // Position at shoulder with arm joint offset (16px right, 32px down in sprite space)
                    GNVector2 shoulder = GetShoulderPosition();
                    frontArmTransform->position.x = shoulder.x + (16.0f * scale);
                    frontArmTransform->position.y = shoulder.y + (32.0f * scale);

                    // Apply hurt flashing to arm sprites too
                    if (IsHurtFlashing()) {
                        frontArmSprite->color.r = 255;
                        frontArmSprite->color.g = 200;
                        frontArmSprite->color.b = 200;
                    } else {
                        frontArmSprite->color.r = 255;
                        frontArmSprite->color.g = 255;
                        frontArmSprite->color.b = 255;
                    }
                }
            }

            // Arm rotations are now updated every frame in the main Update() method
            break;

        case RatKingState::THROWING:
            // During THROWING: Let animations play normally
            // Update positions for arm sprites
            if (backArmEntity != 0) {
                Sprite* backArmSprite = m_ecsSystem->GetComponent<Sprite>(backArmEntity);
                Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
                if (backArmSprite && backArmTransform) {
                    // Start animation if not already playing
                    if (!backArmSprite->playing) {
                        backArmSprite->playing = true;
                        backArmSprite->currentFrame = 0;  // Reset to start
                    }
                    // Position at shoulder with arm joint offset (16px right, 32px down in sprite space)
                    GNVector2 shoulder = GetShoulderPosition();
                    backArmTransform->position.x = shoulder.x + (16.0f * scale);
                    backArmTransform->position.y = shoulder.y + (32.0f * scale);

                    // Apply hurt flashing to arm sprites too
                    if (IsHurtFlashing()) {
                        backArmSprite->color.r = 255;
                        backArmSprite->color.g = 200;
                        backArmSprite->color.b = 200;
                    } else {
                        backArmSprite->color.r = 255;
                        backArmSprite->color.g = 255;
                        backArmSprite->color.b = 255;
                    }
                }
            }

            if (frontArmEntity != 0) {
                Sprite* frontArmSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
                Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
                if (frontArmSprite && frontArmTransform) {
                    // Start animation if not already playing
                    if (!frontArmSprite->playing) {
                        frontArmSprite->playing = true;
                        frontArmSprite->currentFrame = 0;  // Reset to start
                    }
                    // Position at shoulder with arm joint offset (16px right, 32px down in sprite space)
                    GNVector2 shoulder = GetShoulderPosition();
                    frontArmTransform->position.x = shoulder.x + (16.0f * scale);
                    frontArmTransform->position.y = shoulder.y + (32.0f * scale);

                    // Apply hurt flashing to arm sprites too
                    if (IsHurtFlashing()) {
                        frontArmSprite->color.r = 255;
                        frontArmSprite->color.g = 200;
                        frontArmSprite->color.b = 200;
                    } else {
                        frontArmSprite->color.r = 255;
                        frontArmSprite->color.g = 255;
                        frontArmSprite->color.b = 255;
                    }
                }
            }

            // Sync arm frames with torso if in throwing state
            if (bossEntity != 0 && backArmEntity != 0 && frontArmEntity != 0) {
                Sprite* torsoSprite = m_ecsSystem->GetComponent<Sprite>(bossEntity);
                Sprite* backArmSprite = m_ecsSystem->GetComponent<Sprite>(backArmEntity);
                Sprite* frontArmSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);

                if (torsoSprite && backArmSprite && frontArmSprite) {
                    int torsoFrame = torsoSprite->currentFrame;
                    backArmSprite->currentFrame = torsoFrame;
                    frontArmSprite->currentFrame = torsoFrame;
                }
            }
            break;

        default:
            // For other states, just update positions of arm sprites (if visible)
            if (backArmEntity != 0) {
                Sprite* backArmSprite = m_ecsSystem->GetComponent<Sprite>(backArmEntity);
                Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
                if (backArmSprite && backArmTransform) {
                    GNVector2 shoulder = GetShoulderPosition();
                    backArmTransform->position.x = shoulder.x + (16.0f * scale);
                    backArmTransform->position.y = shoulder.y + (32.0f * scale);
                    // Reset color
                    backArmSprite->color.r = 255;
                    backArmSprite->color.g = 255;
                    backArmSprite->color.b = 255;
                }
            }

            if (frontArmEntity != 0) {
                Sprite* frontArmSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
                Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
                if (frontArmSprite && frontArmTransform) {
                    GNVector2 shoulder = GetShoulderPosition();
                    frontArmTransform->position.x = shoulder.x + (16.0f * scale);
                    frontArmTransform->position.y = shoulder.y + (32.0f * scale);
                    // Reset color
                    frontArmSprite->color.r = 255;
                    frontArmSprite->color.g = 255;
                    frontArmSprite->color.b = 255;
                }
            }
            break;
    }
}

} // namespace GameCore
