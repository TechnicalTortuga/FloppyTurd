#include "BossSystem.h"
#include "ProjectileSystem.h"
#include "../Config/EnemyConfigs.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <cmath>
#include <algorithm>

namespace GameCore {

BossSystem::BossSystem(Gnosis::ECS* ecsSystem, LevelManager* levelManager, ProjectileSystem* projectileSystem, PlatformDelegates* platformDelegates)
    : m_ecsSystem(ecsSystem)
    , m_levelManager(levelManager)
    , m_projectileSystem(projectileSystem)
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
    if (!isActive || bossEntity == 0) {
        GN_LOG_DEBUG("BossSystem: Update skipped - isActive=" + std::to_string(isActive) + ", bossEntity=" + std::to_string(bossEntity));
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
    if (!isActive) return;

    health -= damage;
    if (health < 0) health = 0;

    GN_LOG_INFO("Rat King took " + std::to_string(damage) + " damage, health now: " + std::to_string(health));

    if (health <= 0) {
        ChangeState(RatKingState::DEATH);
    } else {
        ChangeState(RatKingState::HURT);
        hurtBuffer = 10; // Prevent multiple hits
        hurtFlashTimer = 0.0f; // Start hurt flashing
    }

    // Check for minion spawning
    while (health <= nextMinionHealthThreshold && nextMinionHealthThreshold > 0) {
        if (health >= 10) {  // Above 50% health
            SpawnMinionWave(4);
        } else {
            SpawnMinionWave(6);
        }
        nextMinionHealthThreshold -= 4;  // Next threshold (20% intervals)
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
        aimingData.aimDuration = 0.5f;  // Faster at low health
    } else if (healthPercent <= 0.5f) {
        aimingData.aimDuration = 1.0f;
    } else {
        aimingData.aimDuration = 1.2f;  // Normal aiming time
    }

    aimingData.aimTimer += deltaTime;

    GN_LOG_DEBUG("BossSystem: HandleAiming - timer=" + std::to_string(aimingData.aimTimer) +
                 "/" + std::to_string(aimingData.aimDuration) +
                 ", deltaTime=" + std::to_string(deltaTime));

    // Update shoulder position for arm rotation
    aimingData.shoulderPivot = GetShoulderPosition();

    // Smooth angle interpolation towards player
    Gnosis::GNVector2 toPlayer = Gnosis::Vector2Subtract(aimingData.playerPosition, aimingData.shoulderPivot);
    float targetAngle = atan2(toPlayer.y, toPlayer.x) * Gnosis::RAD2DEG;
    targetAngle = Gnosis::Clamp(targetAngle, 120.0f, 240.0f); // Limit aiming range

    aimingData.currentArmAngle = Gnosis::SmoothAngleLerp(aimingData.currentArmAngle, targetAngle, deltaTime * 4.0f);

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
    hurtTimer += deltaTime;
    if (hurtTimer > 0.9f) {  // 6 frames at 0.15f per frame
        hurtTimer = 0.0f;
        ChangeState(RatKingState::IDLE);
    }
}

void BossSystem::HandleDeath(float deltaTime) {
    deathTimer += deltaTime;
    if (deathTimer > 0.9f) {  // 6 frames at 0.15f per frame
        deathTimer = 0.0f;
        isActive = false;
        // TODO: Trigger level completion
    }
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
            idleTimer = 0.0f;
            break;

        case RatKingState::WALKING_LEFT:
        case RatKingState::WALKING_RIGHT:
            GN_LOG_DEBUG("BossSystem: Setting WALKING state - hiding arms");
            SetCurrentSprite(sprites.walkSprite);
            // Hide arm sprites during walking
            SetArmSpriteVisibility(false, false);
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
            GN_LOG_DEBUG("BossSystem: Setting HURT state - hiding arms");
            SetCurrentSprite(sprites.hurtSprite);
            // Hide arm sprites during hurt
            SetArmSpriteVisibility(false, false);
            hasFlashedHurt = false;
            hurtTimer = 0.0f;
            break;

        case RatKingState::DEATH:
            GN_LOG_DEBUG("BossSystem: Setting DEATH state - hiding arms");
            SetCurrentSprite(sprites.deathSprite);
            // Hide arm sprites during death
            SetArmSpriteVisibility(false, false);
            deathTimer = 0.0f;
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

void BossSystem::SpawnProjectile() {
    if (!m_projectileSystem) return;

    GNVector2 shoulder = GetShoulderPosition();
    // Use the current arm angle instead of the lock-on angle to ensure projectiles aim at current player position
    float angleRad = aimingData.currentArmAngle * Gnosis::DEG2RAD; // Convert to radians for cos/sin
    GNVector2 direction = {cosf(angleRad), sinf(angleRad)};

    // Normalize the direction vector
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (length > 0.0f) {
        direction.x /= length;
        direction.y /= length;
    }

    // Spawn from shoulder with smaller offset (like old system)
    Gnosis::GNVector2 spawnOffset = {-40.0f, 0.0f}; // Offset to the left from shoulder
    Gnosis::GNVector2 spawnPos = Gnosis::Vector2Add(shoulder, spawnOffset);

    // Spawn toilet paper projectile with proper speed
    Entity projectile = m_projectileSystem->SpawnEnemyProjectile(
        GNVector2(spawnPos.x, spawnPos.y),
        GNVector2(direction.x, direction.y),
        ProjectileType::TOILET_PAPER,
        1  // damage
    );

    if (projectile != 0) {
        // Set projectile speed by modifying its physics component
        Physics* physics = m_ecsSystem->GetComponent<Physics>(projectile);
        if (physics) {
            physics->velocity = direction * 300.0f; // Faster speed for better gameplay
        }

        GN_LOG_INFO("Rat King spawned toilet paper projectile at angle: " +
                   std::to_string(aimingData.currentArmAngle) + " degrees, velocity: (" +
                   std::to_string(direction.x * 300.0f) + ", " + std::to_string(direction.y * 300.0f) + ")");
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

        Gnosis::GNVector2 dualSpawnPos = Gnosis::Vector2Add(shoulder, spawnOffset);

        Entity dualProjectile = m_projectileSystem->SpawnEnemyProjectile(
            GNVector2(dualSpawnPos.x, dualSpawnPos.y),
            GNVector2(dualDirection.x, dualDirection.y),
            ProjectileType::TOILET_PAPER,
            1  // damage
        );

        if (dualProjectile != 0) {
            // Set dual projectile speed
            Physics* dualPhysics = m_ecsSystem->GetComponent<Physics>(dualProjectile);
            if (dualPhysics) {
                dualPhysics->velocity = dualDirection * 300.0f;
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

    // Spawn minions around the boss position
    float spawnRadius = 150.0f;  // Distance from boss
    float angleStep = 2.0f * Gnosis::PI / count;  // Evenly distribute around boss

    for (int i = 0; i < count; ++i) {
        float angle = i * angleStep;
        float x = position.x + cosf(angle) * spawnRadius;
        float y = position.y + sinf(angle) * spawnRadius;

        // Spawn the minion
        Gnosis::Entity minion = m_levelManager->SpawnEnemy(ratCopterConfig, x, y);
        if (minion != 0) {
            GN_LOG_INFO("Rat King spawned RatCopter minion " + std::to_string(i + 1) +
                       " at position (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        }
    }
}

void BossSystem::UpdateLockOnIndicator() {
    // Lock-on indicator will be handled by RenderSystem through UI shapes
    // Store the current aiming data for the RenderSystem to use
    aimingData.lockOnDots.clear();

    int dots = 40;
    float spacing = 8.0f;
    float progress = Gnosis::Clamp(aimingData.aimTimer / aimingData.aimDuration, 0.0f, 1.0f);

    GNVector2 shoulder = GetShoulderPosition();

    for (int i = 0; i < dots; ++i) {
        float fill = (float)i / (float)dots;
        if (fill > progress) break;

        // Calculate dot position along the aiming line
        float distance = 50.0f + i * spacing;  // Start 50px from shoulder
        GNVector2 direction = {cosf(aimingData.currentArmAngle), sinf(aimingData.currentArmAngle)};
        Gnosis::GNVector2 dotPos = Gnosis::Vector2Add(shoulder, Gnosis::Vector2Scale(direction, distance));

        // Store dot data for RenderSystem to use
        LockOnDot dot;
        dot.position = dotPos;
        dot.progress = fill;
        aimingData.lockOnDots.push_back(dot);
    }
}

void BossSystem::ChangeMusic(const std::string& musicFile) {
    if (!m_platformDelegates || !m_platformDelegates->audio.playMusic) return;

    GN_LOG_INFO("BossSystem: Changing music to " + musicFile);
    float musicVolume = 0.7f;  // 70% volume for boss music
    m_platformDelegates->audio.playMusic(musicFile.c_str(), musicVolume, -1);
}

Gnosis::GNVector2 BossSystem::GetShoulderPosition() const {
    // Shoulder is at center of Rat King sprite
    return {
        position.x + (128.0f * scale / 2.0f),
        position.y + (128.0f * scale / 2.0f)
    };
}

void BossSystem::CreateBodyPartEntities() {
    if (!m_ecsSystem || bossEntity == 0) return;

    // Get the main boss entity's transform for positioning
    Transform* bossTransform = m_ecsSystem->GetComponent<Transform>(bossEntity);
    if (!bossTransform) return;

    // Create back arm entity
    backArmEntity = m_ecsSystem->CreateEntity();
    Transform backArmTransform = *bossTransform; // Same position as boss
    m_ecsSystem->AddComponent<Transform>(backArmEntity, backArmTransform);
    Sprite backArmSprite("RatkingAimBackArmOnly", 128.0f, 128.0f);
    backArmSprite.layer = 4; // Behind torso
    backArmSprite.visible = false;
    m_ecsSystem->AddComponent<Sprite>(backArmEntity, backArmSprite);

    // Add PivotRotationRenderer with manual control enabled
    // This allows us to set Transform.rotation directly without automatic rotation
    PivotRotationRenderer backArmPivot(true, 0.0f, 0.0f, 0.0f, true); // manual=true
    m_ecsSystem->AddComponent<PivotRotationRenderer>(backArmEntity, backArmPivot);

    // Verify component was added
    if (m_ecsSystem->HasComponent<PivotRotationRenderer>(backArmEntity)) {
        GN_LOG_INFO("BossSystem: ✅ Back arm PivotRotationRenderer component added successfully");
    } else {
        GN_LOG_ERROR("BossSystem: ❌ Failed to add PivotRotationRenderer to back arm!");
    }

    GN_LOG_INFO("BossSystem: Created back arm entity " + std::to_string(backArmEntity));

    // Create front arm entity
    frontArmEntity = m_ecsSystem->CreateEntity();
    Transform frontArmTransform = *bossTransform; // Same position as boss
    m_ecsSystem->AddComponent<Transform>(frontArmEntity, frontArmTransform);
    Sprite frontArmSprite("RatkingAimTossArmOnly", 128.0f, 128.0f);
    frontArmSprite.layer = 6; // In front of torso
    frontArmSprite.visible = false;
    m_ecsSystem->AddComponent<Sprite>(frontArmEntity, frontArmSprite);

    // Add PivotRotationRenderer with manual control enabled
    // This allows us to set Transform.rotation directly without automatic rotation
    PivotRotationRenderer frontArmPivot(true, 0.0f, 0.0f, 0.0f, true); // manual=true
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
    sprites.idleSprite->layer = 5;

    sprites.walkSprite = new Sprite("RatkingWalk", 128.0f, 128.0f);
    sprites.walkSprite->isAnimated = true;
    sprites.walkSprite->frameWidth = 128;
    sprites.walkSprite->frameHeight = 128;
    sprites.walkSprite->frameCount = 8;
    sprites.walkSprite->frameTime = 0.12f;
    sprites.walkSprite->loop = true;
    sprites.walkSprite->visible = false;
    sprites.walkSprite->layer = 5;

    sprites.torsoSprite = new Sprite("RatkingAimTorsoOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
    sprites.torsoSprite->loop = false;
    sprites.torsoSprite->visible = false;
    sprites.torsoSprite->layer = 5;

    // Load sprites for arms (these will be used by the arm entities)
    sprites.backArmSprite = new Sprite("RatkingAimBackArmOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
    sprites.backArmSprite->loop = false;
    sprites.backArmSprite->visible = false;
    sprites.backArmSprite->layer = 4;

    sprites.frontArmSprite = new Sprite("RatkingAimTossArmOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
    sprites.frontArmSprite->loop = false;
    sprites.frontArmSprite->visible = false;
    sprites.frontArmSprite->layer = 6;

    // Load hurt and death sprites
    sprites.hurtSprite = new Sprite("RatkingHurt", 128.0f, 128.0f, 128, 128, 6, 0.15f);
    sprites.hurtSprite->loop = false;
    sprites.hurtSprite->visible = false;
    sprites.hurtSprite->layer = 5;

    sprites.deathSprite = new Sprite("RatkingDeath", 128.0f, 128.0f, 128, 128, 6, 0.20f);
    sprites.deathSprite->loop = false;
    sprites.deathSprite->visible = false;
    sprites.deathSprite->layer = 5;

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

    // Calculate angle to player (same as in HandleAiming)
    Gnosis::GNVector2 toPlayer = Gnosis::Vector2Subtract(aimingData.playerPosition, shoulder);
    float targetAngle = atan2(toPlayer.y, toPlayer.x) * Gnosis::RAD2DEG;
    targetAngle = Gnosis::Clamp(targetAngle, 120.0f, 240.0f); // Limit aiming range

    // Smooth angle interpolation (same as in HandleAiming)
    float oldAngle = aimingData.currentArmAngle;
    aimingData.currentArmAngle = Gnosis::SmoothAngleLerp(aimingData.currentArmAngle, targetAngle, 0.016f * 4.0f);

    GN_LOG_DEBUG("BossSystem: UpdateArmRotations - playerPos=(" + std::to_string(aimingData.playerPosition.x) + "," + std::to_string(aimingData.playerPosition.y) +
                 "), shoulder=(" + std::to_string(shoulder.x) + "," + std::to_string(shoulder.y) +
                 "), targetAngle=" + std::to_string(targetAngle) +
                 "), currentAngle=" + std::to_string(aimingData.currentArmAngle) +
                 "), oldAngle=" + std::to_string(oldAngle));

    // Apply rotation to back arm using PivotRotationRenderer (same as spike balls)
    if (backArmEntity != 0) {
        Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
        PivotRotationRenderer* backPivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(backArmEntity);
        if (backArmTransform && backPivotRenderer) {
            // Set the rotation directly on the transform for immediate aiming (like spike balls do)
            float oldRotation = backArmTransform->rotation;
            backArmTransform->rotation = aimingData.currentArmAngle;
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

    // Apply rotation to front arm using PivotRotationRenderer (same as spike balls)
    if (frontArmEntity != 0) {
        Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
        PivotRotationRenderer* frontPivotRenderer = m_ecsSystem->GetComponent<PivotRotationRenderer>(frontArmEntity);
        if (frontArmTransform && frontPivotRenderer) {
            // Set the rotation directly on the transform for immediate aiming (like spike balls do)
            float oldRotation = frontArmTransform->rotation;
            frontArmTransform->rotation = aimingData.currentArmAngle;
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
        if (backArmSprite) {
            GN_LOG_DEBUG("BossSystem: Setting back arm visibility to " + std::to_string(showBackArm));
            if (showBackArm && sprites.backArmSprite) {
                *backArmSprite = *sprites.backArmSprite;
                backArmSprite->Reset();
                // Ensure visibility is set correctly after copying template properties
                backArmSprite->visible = true;
                GN_LOG_DEBUG("BossSystem: Copied back arm sprite properties - textureId=" + backArmSprite->textureId +
                            ", visible=" + std::to_string(backArmSprite->visible) +
                            ", isAnimated=" + std::to_string(backArmSprite->isAnimated));
            } else {
                backArmSprite->visible = false;
            }
            GN_LOG_DEBUG("BossSystem: Set back arm visible=" + std::to_string(showBackArm) +
                        ", final textureId=" + backArmSprite->textureId);

            // Update back arm transform position
            Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
            if (backArmTransform) {
                backArmTransform->position.x = position.x;
                backArmTransform->position.y = position.y;
                GN_LOG_DEBUG("BossSystem: Updated back arm position");
            } else {
                GN_LOG_ERROR("BossSystem: Back arm entity missing Transform component!");
            }
        } else {
            GN_LOG_ERROR("BossSystem: Back arm entity " + std::to_string(backArmEntity) + " missing Sprite component!");
        }
    } else {
        GN_LOG_DEBUG("BossSystem: No back arm entity to update");
    }

    // Set front arm visibility
    if (frontArmEntity != 0) {
        Sprite* frontArmSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
        if (frontArmSprite) {
            GN_LOG_DEBUG("BossSystem: Setting front arm visibility to " + std::to_string(showFrontArm));
            if (showFrontArm && sprites.frontArmSprite) {
                *frontArmSprite = *sprites.frontArmSprite;
                frontArmSprite->Reset();
                // Ensure visibility is set correctly after copying template properties
                frontArmSprite->visible = true;
                GN_LOG_DEBUG("BossSystem: Copied front arm sprite properties - textureId=" + frontArmSprite->textureId +
                            ", visible=" + std::to_string(frontArmSprite->visible) +
                            ", isAnimated=" + std::to_string(frontArmSprite->isAnimated));
            } else {
                frontArmSprite->visible = false;
            }
            GN_LOG_DEBUG("BossSystem: Set front arm visible=" + std::to_string(showFrontArm) +
                        ", final textureId=" + frontArmSprite->textureId);

            // Update front arm transform position
            Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
            if (frontArmTransform) {
                frontArmTransform->position.x = position.x;
                frontArmTransform->position.y = position.y;
                GN_LOG_DEBUG("BossSystem: Updated front arm position");
            } else {
                GN_LOG_ERROR("BossSystem: Front arm entity missing Transform component!");
            }
        } else {
            GN_LOG_ERROR("BossSystem: Front arm entity " + std::to_string(frontArmEntity) + " missing Sprite component!");
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
            sprite->layer = 3; // Foreground layer (higher than background layer 2)
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
                    backArmTransform->position.x = position.x;
                    backArmTransform->position.y = position.y;

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
                    frontArmTransform->position.x = position.x;
                    frontArmTransform->position.y = position.y;

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
                    backArmTransform->position.x = position.x;
                    backArmTransform->position.y = position.y;

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
                    frontArmTransform->position.x = position.x;
                    frontArmTransform->position.y = position.y;

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
                if (backArmSprite) {
                    // Update back arm transform position
                    Transform* backArmTransform = m_ecsSystem->GetComponent<Transform>(backArmEntity);
                    if (backArmTransform) {
                        backArmTransform->position.x = position.x;
                        backArmTransform->position.y = position.y;
                    }
                    // Reset color
                    backArmSprite->color.r = 255;
                    backArmSprite->color.g = 255;
                    backArmSprite->color.b = 255;
                }
            }

            if (frontArmEntity != 0) {
                Sprite* frontArmSprite = m_ecsSystem->GetComponent<Sprite>(frontArmEntity);
                if (frontArmSprite) {
                    // Update front arm transform position
                    Transform* frontArmTransform = m_ecsSystem->GetComponent<Transform>(frontArmEntity);
                    if (frontArmTransform) {
                        frontArmTransform->position.x = position.x;
                        frontArmTransform->position.y = position.y;
                    }
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
