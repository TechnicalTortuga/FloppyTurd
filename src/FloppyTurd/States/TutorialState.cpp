#include "TutorialState.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Core/GNLog.h"
#include "../Input/InputManager.h"
#include "../../Engine/Utility/Utils.h"
#include <cmath>

namespace GameCore {

    TutorialState::TutorialState(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_game(nullptr)
        , m_spriteSystem(nullptr)
        , m_playerControllerSystem(nullptr)
        , m_renderSystem(nullptr)
        , m_projectileSystem(nullptr)
        , m_cachedScreenWidth(0.0f)
        , m_cachedScreenHeight(0.0f)
        , m_playerEntity(0)
        , m_pipeCounterEntity(0)
        , m_coinBagEntity(0)
        , m_coinsTextEntity(0)
        , m_shootingZoneEntity(0)
        , m_settingsButtonEntity(0)
        , m_pauseBackgroundEntity(0)
        , m_pausedTextEntity(0)
        , m_returnToMenuButtonEntity(0)
        , m_touchPressed(false)
        , m_touchReleased(false)
        , m_touchPosition(0.0f, 0.0f)
        , m_initialized(false)
        , m_isPaused(false)
        , m_finished(false)
        , m_tutorialCoins(999)
        , m_jumpButtonHeld(false)
        , m_jumpStartTime(0)
        , m_jumpCooldown(0.0f)
    {
        GN_LOG_INFO("TutorialState: Constructor called");
        m_game = GameCore::GetGame();
    }

    TutorialState::~TutorialState() {
        GN_LOG_INFO("TutorialState: Destructor called");
    }

    void TutorialState::Enter() {
        GN_LOG_INFO("TutorialState: Entering tutorial state");
        
        // 1. Initialize systems
        InitializeSystems();
        
        // 2. Cache screen dimensions
        CacheScreenDimensions();
        
        // 3. Create background
        CreateBackground();
        
        // 4. Create player
        CreatePlayer();
        
        // 5. Create UI
        CreateUI();
        
        // 6. Create pause menu
        CreatePauseMenu();
        
        // 7. Reset state
        m_finished = false;
        m_isPaused = false;
        m_tutorialCoins = 999;
        m_initialized = true;
        
        // 8. Play tutorial theme music
        if (m_platformDelegates && m_platformDelegates->audio.playMusic) {
            m_platformDelegates->audio.playMusic("TutorialTheme", 0.7f, -1);  // 70% volume, loop infinitely
            GN_LOG_INFO("TutorialState: Started TutorialTheme music");
        }
        
        GN_LOG_INFO("TutorialState: Tutorial state entered successfully");
    }

    void TutorialState::Exit() {
        GN_LOG_INFO("TutorialState: Exiting");
        
        // Stop tutorial theme music
        if (m_platformDelegates && m_platformDelegates->audio.stopMusic) {
            m_platformDelegates->audio.stopMusic();
            GN_LOG_INFO("TutorialState: Stopped TutorialTheme music");
        }
        
        // Clean up all entities
        DestroyUI();
        DestroyEntities();
        
        // Cleanup ProjectileSystem
        if (m_projectileSystem) {
            m_projectileSystem->Cleanup();
            delete m_projectileSystem;
            m_projectileSystem = nullptr;
        }
        
        m_initialized = false;
        GN_LOG_INFO("TutorialState: Tutorial state exited");
    }
    
    void TutorialState::InitializeSystems() {
        GN_LOG_INFO("TutorialState: Initializing systems (using existing shared systems from SystemManager)");
        
        if (!m_ecsSystem || !m_ecsSystem->GetSystemManager()) {
            GN_LOG_ERROR("TutorialState: ECS or SystemManager is null!");
            return;
        }
        
        auto* systemManager = m_ecsSystem->GetSystemManager();
        
        // Get existing SpriteSystem from SystemManager (shared, not owned)
        m_spriteSystem = systemManager->GetSpriteSystem();
        if (m_spriteSystem) {
            GN_LOG_INFO("TutorialState: Using existing SpriteSystem from SystemManager");
        } else {
            GN_LOG_ERROR("TutorialState: Could not get SpriteSystem from SystemManager!");
        }
        
        // Get existing RenderSystem from SystemManager (shared, not owned)
        m_renderSystem = systemManager->GetRenderSystem();
        if (m_renderSystem) {
            GN_LOG_INFO("TutorialState: Using existing RenderSystem from SystemManager");
        } else {
            GN_LOG_ERROR("TutorialState: Could not get RenderSystem from SystemManager!");
        }
        
        // Create ProjectileSystem for tutorial (not in SystemManager)
        m_projectileSystem = new ProjectileSystem(m_ecsSystem);
        m_projectileSystem->Initialize();
        GN_LOG_INFO("TutorialState: Created ProjectileSystem for tutorial");
        
        // PlayerControllerSystem is NOT in SystemManager - tutorial handles input directly
        m_playerControllerSystem = nullptr;
        GN_LOG_INFO("TutorialState: Tutorial manages player input directly (no PlayerControllerSystem)");
        
        GN_LOG_INFO("TutorialState: Systems initialized (all shared from SystemManager)");
    }
    
    void TutorialState::CacheScreenDimensions() {
        if (m_renderSystem) {
            const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
            m_cachedScreenWidth = screenInfo.pixelWidth;
            m_cachedScreenHeight = screenInfo.pixelHeight;
            GN_LOG_INFO("TutorialState: Cached screen dimensions: " + std::to_string((int)m_cachedScreenWidth) + "x" + std::to_string((int)m_cachedScreenHeight));
        } else {
            m_cachedScreenWidth = 1179.0f;  // Fallback
            m_cachedScreenHeight = 2556.0f;
            GN_LOG_WARN("TutorialState: Using fallback screen dimensions");
        }
    }

    void TutorialState::Pause() {
        GN_LOG_INFO("TutorialState: Paused");
    }

    void TutorialState::Resume() {
        GN_LOG_INFO("TutorialState: Resumed");
    }

    void TutorialState::Update(float deltaTime) {
        if (!m_initialized) return;
        
        // Update InputManager singleton
        InputManager* inputManager = InputManager::GetInstance();
        if (inputManager) {
            inputManager->Update(deltaTime);
        }
        
        // If paused, don't update game systems
        if (m_isPaused) {
            return;
        }
        
        // Update sprite system (updates sprite states)
        if (m_spriteSystem) {
            m_spriteSystem->Update(deltaTime);
        }
        
        // Check for animation completion immediately after sprite update (prevents flash)
        UpdatePlayerAnimation();
        
        // Update physics (player gravity)
        UpdatePhysics(deltaTime);
        
        // Constrain player AFTER physics (so gravity doesn't push past constraint)
        ConstrainPlayer();
        
        // Update projectile system
        if (m_projectileSystem) {
            m_projectileSystem->Update(deltaTime);
        }
        
        // Update jump mechanics
        UpdateJumpMechanics(deltaTime);
    }
    
    void TutorialState::UpdatePlayerAnimation() {
        // Check for animation completion and return to idle (like PlayerControllerSystem)
        // This runs immediately after SpriteSystem::Update to prevent visual flash
        auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        if (sprite) {
            // If animation completed (non-looping animations like jump/shoot), return to idle
            // Only transition if we're not already idle to prevent flash
            if (sprite->textureId != "TurdletIdle" && sprite->hasCompleted) {
                sprite->textureId = "TurdletIdle";
                sprite->frameCount = 1;
                sprite->isAnimated = false;
                sprite->playing = false;
                sprite->loop = false;
                sprite->currentFrame = 0;
                sprite->currentFrameTime = 0.0f;
                sprite->hasCompleted = false;
                GN_LOG_INFO("TutorialState: Animation completed, returned to TurdletIdle");
            }
        }
    }

    void TutorialState::Render() {
        // Rendering handled by ECS systems
    }

    void TutorialState::HandleInput() {
        if (!m_initialized) return;
        
        UpdateInput();
        
        // Check for settings button press
        if (m_touchPressed) {
            if (CheckSettingsButtonClick(m_touchPosition.x, m_touchPosition.y)) {
                TogglePause();
                return;
            }
        }
        
        // If paused, check for "Main Menu" button press
        if (m_isPaused) {
            if (m_touchPressed) {
                if (CheckReturnToMenuButtonClick(m_touchPosition.x, m_touchPosition.y)) {
                    m_finished = true;
                    return;
                }
            }
            return;  // Don't process other input while paused
        }
        
        // Handle player input
        if (m_touchPressed) {
            // Check if touch is in shooting zone (bottom right)
            if (CheckShootingZoneClick(m_touchPosition.x, m_touchPosition.y)) {
                HandleShooting();
            } else {
                // Any other tap starts variable jump
                HandleJump();
            }
        }
        
        // Handle jump release
        if (m_touchReleased) {
            HandleJumpRelease();
        }
    }
    
    void TutorialState::HandleJump() {
        // Start variable jump - record timestamp
        if (!m_jumpButtonHeld && m_jumpCooldown <= 0.0f) {
            m_jumpButtonHeld = true;
            m_jumpStartTime = GameCore::GetCurrentTimestamp();
            GN_LOG_INFO("TutorialState: Jump button pressed - starting variable jump");
        }
    }
    
    void TutorialState::HandleJumpRelease() {
        if (m_jumpButtonHeld) {
            // Calculate hold duration
            uint64_t currentTime = GameCore::GetCurrentTimestamp();
            uint64_t holdDurationMs = currentTime - m_jumpStartTime;
            float holdDurationSeconds = holdDurationMs / 1000.0f;
            
            GN_LOG_INFO("TutorialState: Jump released after " + std::to_string(holdDurationMs) + "ms");
            
            // EXACT variable jump like PlayerControllerSystem
            const float JUMP_FORCE = 2600.0f;
            const float VARIABLE_JUMP_THRESHOLD = 0.8f;
            const float EARLY_RELEASE_MULTIPLIER = 0.5f;
            
            float jumpForce = JUMP_FORCE;
            if (holdDurationSeconds < VARIABLE_JUMP_THRESHOLD) {
                // Early release - reduce jump force
                float holdRatio = holdDurationSeconds / VARIABLE_JUMP_THRESHOLD;
                jumpForce = JUMP_FORCE * (EARLY_RELEASE_MULTIPLIER + holdRatio * (1.0f - EARLY_RELEASE_MULTIPLIER));
                GN_LOG_INFO("TutorialState: Variable jump - early release, force=" + std::to_string(jumpForce));
            }
            
            // Apply jump
            auto* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
            if (physics) {
                physics->velocity.y = -jumpForce;  // Negative = up
                
                // Set cooldown
                m_jumpCooldown = 0.15f;
                
                // Change to jump animation (6 frames, fully animated like GameplayState)
                auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
                if (sprite && sprite->textureId != "TurdletJump") {
                    sprite->textureId = "TurdletJump";
                    sprite->frameCount = 6;  // 6 frames
                    sprite->isAnimated = true;  // Animated
                    sprite->playing = true;
                    sprite->loop = false;  // Don't loop
                    sprite->frameTime = 0.08f;  // Frame timing
                    sprite->currentFrame = 0;
                    sprite->hasCompleted = false;  // Reset completion flag
                    GN_LOG_INFO("TutorialState: Changed to TurdletJump animation (6 frames)");
                }
            }
            
            m_jumpButtonHeld = false;
        }
    }
    
    void TutorialState::UpdateJumpMechanics(float deltaTime) {
        // Update jump cooldown
        if (m_jumpCooldown > 0.0f) {
            m_jumpCooldown -= deltaTime;
            if (m_jumpCooldown < 0.0f) m_jumpCooldown = 0.0f;
        }
        
        // Update jump hold for variable jump (timestamp-based like PlayerControllerSystem)
        if (m_jumpButtonHeld && m_jumpStartTime > 0) {
            uint64_t currentTime = GameCore::GetCurrentTimestamp();
            uint64_t holdDurationMs = currentTime - m_jumpStartTime;
            
            if (holdDurationMs >= 150) {  // AUTO_JUMP_THRESHOLD
                GN_LOG_INFO("TutorialState: Auto-jump triggered at " + std::to_string(holdDurationMs) + "ms");
                
                // Apply max force jump (2600.0f)
                auto* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
                if (physics) {
                    physics->velocity.y = -2600.0f;
                    m_jumpCooldown = 0.15f;
                    
                    // Change to jump animation (6 frames, fully animated)
                    auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
                    if (sprite && sprite->textureId != "TurdletJump") {
                        sprite->textureId = "TurdletJump";
                        sprite->frameCount = 6;
                        sprite->isAnimated = true;
                        sprite->playing = true;
                        sprite->loop = false;
                        sprite->frameTime = 0.08f;
                        sprite->currentFrame = 0;
                        sprite->hasCompleted = false;
                    }
                }
                
                m_jumpButtonHeld = false;
                m_jumpStartTime = 0;
            }
        }
    }
    
    void TutorialState::HandleShooting() {
        // Fire projectile using ProjectileSystem (proper pooling)
        if (m_tutorialCoins <= 0) {
            GN_LOG_INFO("TutorialState: No coins to shoot!");
            return;
        }
        
        if (!m_projectileSystem) {
            GN_LOG_ERROR("TutorialState: ProjectileSystem not available!");
            return;
        }
        
        // Decrement coins
        m_tutorialCoins--;
        UpdateCoinCounterUI();
        
        // Get player position
        auto* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        if (!playerTransform) return;
        
        // Fire projectile from player position (adjusted 8px left and 8px up from GameplayState)
        float playerScale = playerTransform->scale.x;  // Assuming uniform scaling
        float scaledOffsetX = 34.0f * playerScale;  // 42.0 - 8.0 (8px left)
        float scaledOffsetY = 24.0f * playerScale;  // 32.0 - 8.0 (8px up)
        Gnosis::GNVector2 spawnPosition = playerTransform->position + Gnosis::GNVector2(scaledOffsetX, scaledOffsetY);
        Gnosis::GNVector2 direction(1.0f, 0.0f);  // Fire right
        
        m_projectileSystem->SpawnPlayerProjectile(spawnPosition, direction, ProjectileType::POOP_BALL);
        
        // Play random spit sound (spit1 - spit6)
        int randomSpit = (rand() % 6) + 1;  // Random number 1-6
        std::string spitSound = "spit" + std::to_string(randomSpit);
        if (GameCore::GetGame()) {
            GameCore::GetGame()->PlaySFX(spitSound);
        }
        
        // Change to shooting animation (5 frames - TurdletShoot spritesheet is 320x64 = 5 frames)
        auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_playerEntity);
        if (sprite && sprite->textureId != "TurdletShoot") {
            sprite->textureId = "TurdletShoot";
            sprite->frameCount = 5;  // 5 frames (320 width / 64 per frame)
            sprite->isAnimated = true;  // Animated
            sprite->playing = true;
            sprite->loop = false;  // Don't loop
            sprite->frameTime = 0.1f;  // Frame timing (shoot is 0.1f, not 0.08f)
            sprite->currentFrame = 0;
            sprite->hasCompleted = false;  // Reset completion flag
            GN_LOG_INFO("TutorialState: Changed to TurdletShoot animation (5 frames)");
        }
        
        GN_LOG_INFO("TutorialState: Fired projectile! Coins remaining: " + std::to_string(m_tutorialCoins));
    }
    
    bool TutorialState::CheckShootingZoneClick(float x, float y) {
        // Shooting zone is bottom right area
        auto* transform = m_ecsSystem->GetComponent<Transform>(m_shootingZoneEntity);
        auto* shape = m_ecsSystem->GetComponent<UIShape>(m_shootingZoneEntity);
        if (!transform || !shape) return false;
        
        return (x >= transform->position.x &&
                x <= transform->position.x + shape->width &&
                y >= transform->position.y &&
                y <= transform->position.y + shape->height);
    }
    
    void TutorialState::UpdatePhysics(float deltaTime) {
        // Apply gravity and update positions for player (EXACT same as GameplayState with interpolation)
        auto* playerTransform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        auto* playerPhysics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
        
        if (playerTransform && playerPhysics) {
            // Apply variable gravity like GameplayState with smooth interpolation
            if (playerPhysics->useGravity) {
                // Check if ascending (negative velocity = going up)
                bool isAscending = (playerPhysics->velocity.y < 0.0f);
                
                if (isAscending) {
                    // Interpolate gravity from GRAVITY_UP to GRAVITY_DOWN as we approach apex
                    // This creates a smooth transition instead of abrupt switching (like PlayerControllerSystem)
                    float velocityMagnitude = std::abs(playerPhysics->velocity.y);
                    
                    // Interpolation zone: 0-200 velocity range
                    // At high velocity (200+): use GRAVITY_UP (2400)
                    // At low velocity (0-50): use GRAVITY_DOWN (8000) - apex
                    // In between: smooth interpolation
                    float t = 1.0f - std::min(velocityMagnitude / 200.0f, 1.0f); // 0.0 at high speed, 1.0 at low speed
                    float interpolatedGravity = 2400.0f + (8000.0f - 2400.0f) * t;
                    
                    playerPhysics->acceleration.y = interpolatedGravity;
                } else {
                    // Heavier gravity while falling (GRAVITY_DOWN = 8000)
                    playerPhysics->acceleration.y = 8000.0f;
                }
                
                // Update velocity with gravity
                playerPhysics->velocity.y += playerPhysics->acceleration.y * deltaTime;
                
                // Terminal velocity
                if (playerPhysics->velocity.y > 1800.0f) {
                    playerPhysics->velocity.y = 1800.0f;
                }
            }
            
            // Apply drag (only to Y velocity like PlayerControllerSystem)
            playerPhysics->velocity.y = playerPhysics->velocity.y * playerPhysics->drag;
            
            // Update position
            playerTransform->position.x += playerPhysics->velocity.x * deltaTime;
            playerTransform->position.y += playerPhysics->velocity.y * deltaTime;
        }
    }

    void TutorialState::CreateBackground() {
        GN_LOG_INFO("TutorialState: Creating Park Level background layers");
        
        // EXACT Park Level textures from LevelConfig.cpp
        // Each layer on SEPARATE render layer to avoid layering issues
        struct SimpleBgLayer {
            std::string textureId;
            float textureWidth;
            float textureHeight;
            int renderLayer;
        };
        
        std::vector<SimpleBgLayer> layers = {
            {"Level1BackLayerBackground", 1024.0f, 512.0f, 0},   // Back - layer 0
            {"Level1MidLayerBackground", 1024.0f, 512.0f, 1},    // Mid - layer 1
            {"Level1Clouds", 512.0f, 180.0f, 2},                 // Clouds - layer 2 (SEPARATE!)
            {"Level1FrontLayerBackground", 2048.0f, 480.0f, 3}   // Front - layer 3
        };
        
        for (const auto& layer : layers) {
            Gnosis::Entity bgEntity = m_ecsSystem->CreateEntity();
            
            // Scale to fit screen height
            float heightScale = m_cachedScreenHeight / layer.textureHeight;
            
            // Position at (0, 0) - top-left
            Transform bgTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(heightScale, heightScale));
            m_ecsSystem->AddComponent<Transform>(bgEntity, bgTransform);
            
            // Create Sprite component (RenderSystem will render it)
            Sprite bgSprite(layer.textureId, layer.textureWidth, layer.textureHeight);
            bgSprite.color = Gnosis::GNColor(255, 255, 255, 255);
            bgSprite.visible = true;
            bgSprite.layer = layer.renderLayer;
            m_ecsSystem->AddComponent<Sprite>(bgEntity, bgSprite);
            
            m_backgroundEntities.push_back(bgEntity);
            
            GN_LOG_INFO("TutorialState: Created background layer '" + layer.textureId + "' on render layer " + std::to_string(layer.renderLayer));
        }
        
        GN_LOG_INFO("TutorialState: Created " + std::to_string(m_backgroundEntities.size()) + " background layers");
    }

    void TutorialState::CreatePlayer() {
        // Create player EXACTLY like GameplayState does
        m_playerEntity = m_ecsSystem->CreateEntity();
        if (m_playerEntity == 0) {
            GN_LOG_ERROR("TutorialState: Failed to create player entity!");
            return;
        }
        
        // Position player in center-left of screen
        float playerScale = 8.0f;  // Same as GameplayState base scale
        float playerX = m_cachedScreenWidth * 0.25f;  // 25% from left
        float playerY = m_cachedScreenHeight * 0.50f; // Center vertically
        
        Transform playerTransform(Gnosis::GNVector2(playerX, playerY), 0.0f, Gnosis::GNVector2(playerScale, playerScale));
        m_ecsSystem->AddComponent<Transform>(m_playerEntity, playerTransform);
        
        // Add sprite component with Turdlet idle animation (EXACT same as GameplayState)
        // TurdletIdle.png is 64x64 pixels, single frame
        Sprite playerSprite("TurdletIdle", 64.0f, 64.0f, 64, 64, 1, 0.1f);
        playerSprite.color = Gnosis::GNColor(255, 255, 255, 255);
        playerSprite.visible = true;
        playerSprite.layer = 6; // Player layer (above obstacles which are layers 3-5)
        
        // Configure as single-frame idle (like PlayerControllerSystem)
        playerSprite.frameCount = 1;
        playerSprite.isAnimated = false;  // Not animated for idle
        playerSprite.playing = false;     // Not playing
        playerSprite.loop = false;        // No loop for idle
        playerSprite.currentFrame = 0;
        playerSprite.currentFrameTime = 0.0f;
        playerSprite.hasCompleted = false;
        
        m_ecsSystem->AddComponent<Sprite>(m_playerEntity, playerSprite);
        
        // Add physics component
        Physics playerPhysics;
        playerPhysics.useGravity = true;
        playerPhysics.mass = 1.0f;
        playerPhysics.drag = 0.95f;
        m_ecsSystem->AddComponent<Physics>(m_playerEntity, playerPhysics);
        
        // Add hitbox component (circle)
        Hitbox playerHitbox;
        playerHitbox.type = ColliderType::Circle;
        playerHitbox.radius = 12.0f;
        playerHitbox.offsetX = 0.0f;
        playerHitbox.offsetY = 0.0f;
        playerHitbox.isTrigger = false;
        m_ecsSystem->AddComponent<Hitbox>(m_playerEntity, playerHitbox);
        
        // Add player component (simplified - no coin tracking)
        PlayerComponent playerData;
        m_ecsSystem->AddComponent<PlayerComponent>(m_playerEntity, playerData);
        
        GN_LOG_INFO("TutorialState: Player created with TurdletIdle sprite - static for tutorial");
    }
    
    void TutorialState::ConstrainPlayer() {
        auto* transform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
        auto* physics = m_ecsSystem->GetComponent<Physics>(m_playerEntity);
        
        if (transform && physics) {
            // Keep player between top and center (top half of screen)
            float topBoundary = 50.0f;  // Don't let player go off top
            float centerY = m_cachedScreenHeight * 0.50f;  // Center line (bottom boundary)
            
            // If player tries to go above top, stop upward movement
            if (transform->position.y < topBoundary) {
                transform->position.y = topBoundary;
                physics->velocity.y = 0.0f;  // Stop upward movement, gravity will pull down
                GN_LOG_INFO("TutorialState: Player hit top boundary, velocity reset");
            }
            
            // If player tries to go below center, snap back and reset velocity
            if (transform->position.y > centerY) {
                transform->position.y = centerY;
                physics->velocity.y = 0.0f;  // Stop downward movement
            }
        }
    }

    void TutorialState::CreateUI() {
        GN_LOG_INFO("TutorialState: Creating UI (matching GameplayState layout)");
        
        if (!m_ecsSystem) return;
        
        // 1. PIPE COUNTER (top center)
        m_pipeCounterEntity = m_ecsSystem->CreateEntity();
        if (m_pipeCounterEntity != 0) {
            float centerX = m_cachedScreenWidth * 0.50f;
            float pipeY = m_cachedScreenHeight * 0.10f;  // 10% from top
            
            Transform pipeTransform(Gnosis::GNVector2(centerX, pipeY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_pipeCounterEntity, pipeTransform);
            
            // Dynamic font scaling for tablet
            bool isTablet = (m_cachedScreenHeight > 0) && (m_cachedScreenWidth / m_cachedScreenHeight > 0.6f);
            float tabletScale = isTablet ? 0.7f : 1.0f;

            UIElement pipeCounter;
            pipeCounter.buttonText = "0";  // NON-FUNCTIONAL - stays at 0
            pipeCounter.fontSize = 120.0f * tabletScale;
            pipeCounter.textOutlineWidth = 18.0f * tabletScale;
            pipeCounter.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White
            pipeCounter.centerTextHorizontally = true;
            pipeCounter.centerTextVertically = true;
            pipeCounter.visible = true;
            pipeCounter.isEnabled = true;
            pipeCounter.textLayer = 10;
            m_ecsSystem->AddComponent<UIElement>(m_pipeCounterEntity, pipeCounter);
        }
        
        // Match GameplayState EXACTLY: 1% from left, 87% from top, 8.0f scale
        float iconX = m_cachedScreenWidth * 0.01f;   // 1% from left
        float iconY = m_cachedScreenHeight * 0.87f;  // 87% from top
        float bagScale = 8.0f;  // Fixed 8.0f scale to match GameplayState
        
        m_coinBagEntity = m_ecsSystem->CreateEntity();
        if (m_coinBagEntity != 0) {
            Transform tr(Gnosis::GNVector2(iconX, iconY), 0.0f, Gnosis::GNVector2(bagScale, bagScale));
            m_ecsSystem->AddComponent<Transform>(m_coinBagEntity, tr);
            
            UIElement bag;
            bag.normalTextureId = "CoinBag";
            bag.visible = true;
            bag.isEnabled = true;
            bag.textLayer = 10;
            m_ecsSystem->AddComponent<UIElement>(m_coinBagEntity, bag);
        }
        
        // 3. COIN COUNTER TEXT (next to bag)
        m_coinsTextEntity = m_ecsSystem->CreateEntity();
        if (m_coinsTextEntity != 0) {
            float textX = iconX + (32.0f * bagScale) + 8.0f;
            float textY = iconY + (32.0f * bagScale * 0.5f) + 24.0f;
            
            Transform tr(Gnosis::GNVector2(textX, textY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_coinsTextEntity, tr);
            
            // Dynamic font scaling for tablet
            bool isTablet = (m_cachedScreenHeight > 0) && (m_cachedScreenWidth / m_cachedScreenHeight > 0.6f);
            float tabletScale = isTablet ? 0.7f : 1.0f;

            UIElement ui;
            ui.buttonText = "999";  // FUNCTIONAL - decrements when shooting
            ui.fontSize = 64.0f * tabletScale;
            ui.textOutlineWidth = 10.0f;
            ui.textColor = Gnosis::GNColor(255, 215, 0, 255);  // Gold
            ui.centerTextHorizontally = false;
            ui.centerTextVertically = true;
            ui.visible = true;
            ui.isEnabled = true;
            ui.textLayer = 10;
            m_ecsSystem->AddComponent<UIElement>(m_coinsTextEntity, ui);
        }
        
        // 4. SHOOTING ZONE VISUAL (bottom right)
        m_shootingZoneEntity = m_ecsSystem->CreateEntity();
        if (m_shootingZoneEntity != 0) {
            float textX = iconX + (32.0f * bagScale) + 8.0f;  // Calculate textX for shooting zone position
            float shootingZoneLeftX = textX + 200.0f + 8.0f;
            float shootingZoneRightX = m_cachedScreenWidth * 0.95f;
            float shootingZoneTopY = m_cachedScreenHeight * 0.80f;
            float shootingZoneBottomY = m_cachedScreenHeight * 0.95f;
            
            float width = shootingZoneRightX - shootingZoneLeftX;
            float height = shootingZoneBottomY - shootingZoneTopY;
            
            Transform shootTransform(Gnosis::GNVector2(shootingZoneLeftX, shootingZoneTopY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(m_shootingZoneEntity, shootTransform);
            
            UIElement shootUI;
            shootUI.visible = true;
            shootUI.isEnabled = true;
            shootUI.textLayer = 10;
            m_ecsSystem->AddComponent<UIElement>(m_shootingZoneEntity, shootUI);
            
            UIShape shootShape;
            shootShape.type = UIShapeType::Rectangle;
            shootShape.width = width;
            shootShape.height = height;
            shootShape.cornerRadius = 20.0f;  // Rounded corners
            shootShape.color = Gnosis::GNColor(128, 128, 128, 64);  // Semi-transparent gray
            shootShape.layer = 10;
            shootShape.visible = true;
            m_ecsSystem->AddComponent<UIShape>(m_shootingZoneEntity, shootShape);
        }
        
        // 5. SETTINGS BUTTON (top right)
        CreateSettingsButton();
        
        // 6. INSTRUCTION TEXT (center)
        CreateInstructionText();
        
        GN_LOG_INFO("TutorialState: UI created (pipe counter, coin bag+text, shooting zone, settings button, instructions)");
    }
    
    void TutorialState::CreateInstructionText() {
        // Create instruction text entities explaining controls
        std::vector<std::string> instructions = {
            "How to Play",
            "",
            "Tap shooting zone (bottom right)",
            "to shoot projectiles",
            "",
            "Tap anywhere else to jump!",
            "Hold longer for higher jumps",
            "",
            "Settings button pauses",
            "To return to menu, pause the game"
        };
        
        float centerX = m_cachedScreenWidth * 0.5f;
        float startY = m_cachedScreenHeight * 0.18f;  // Start slightly higher to fit more text
        float lineSpacing = 80.0f;  // Increased spacing for larger text
        
        for (size_t i = 0; i < instructions.size(); ++i) {
            if (instructions[i].empty()) continue;  // Skip empty lines
            
            Gnosis::Entity textEntity = m_ecsSystem->CreateEntity();
            Transform textTransform(Gnosis::GNVector2(centerX, startY + (i * lineSpacing)), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(textEntity, textTransform);
            
            // Tablet scaling
            bool isTablet = (m_cachedScreenHeight > 0) && (m_cachedScreenWidth / m_cachedScreenHeight > 0.6f);
            float tabletScale = isTablet ? 0.6f : 1.0f; // More aggressive reduction for instruction text block

            UIElement textUI;
            textUI.buttonText = instructions[i];
            // Increased font sizes as requested (was 64/48, now 80/60)
            textUI.fontSize = ((i == 0) ? 80.0f : 60.0f) * tabletScale;
            textUI.textColor = (i == 0) ? Gnosis::GNColor(255, 215, 0, 255) : Gnosis::GNColor(255, 255, 255, 255);  // Gold title, white text
            textUI.centerTextHorizontally = true;
            textUI.centerTextVertically = true;
            textUI.visible = true;
            textUI.textLayer = 11;  // Above UI elements
            m_ecsSystem->AddComponent<UIElement>(textEntity, textUI);
            
            m_instructionTextEntities.push_back(textEntity);
        }
        
        GN_LOG_INFO("TutorialState: Created " + std::to_string(m_instructionTextEntities.size()) + " instruction text entities");
    }
    
    void TutorialState::CreateSettingsButton() {
        float settingsX = m_cachedScreenWidth * 0.85f;   // 85% from left (Matches GameplayState PORTRAIT_SETTINGS_X)
        float settingsY = m_cachedScreenHeight * 0.05f;  // 5% from top (Matches GameplayState PORTRAIT_SETTINGS_Y)
        
        // Match GameplayState scale exactly (fixed 8.0f)
        float buttonScale = 8.0f;
        
        m_settingsButtonEntity = m_ecsSystem->CreateEntity();
        if (m_settingsButtonEntity != 0) {
            Transform settingsTransform(Gnosis::GNVector2(settingsX, settingsY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            m_ecsSystem->AddComponent<Transform>(m_settingsButtonEntity, settingsTransform);
            
            // Use correct texture ID (no .png extension, lowercase) - EXACT match to GameplayState
            Sprite settingsSprite;
            settingsSprite.textureId = "settingsbutton";  // EXACT match to GameplayState
            settingsSprite.width = 16.0f;
            settingsSprite.height = 16.0f;
            settingsSprite.layer = 10;
            settingsSprite.visible = true;
            m_ecsSystem->AddComponent<Sprite>(m_settingsButtonEntity, settingsSprite);
            
            UIElement settingsUI;
            settingsUI.normalTextureId = "settingsbutton";  // EXACT match to GameplayState
            settingsUI.visible = true;
            settingsUI.isEnabled = true;
            settingsUI.textLayer = 10;
            m_ecsSystem->AddComponent<UIElement>(m_settingsButtonEntity, settingsUI);
            
            // Add Bounds for consistency with GameplayState
            float scaledWidth = 16.0f * buttonScale;
            float scaledHeight = 16.0f * buttonScale;
            Bounds buttonBounds(scaledWidth, scaledHeight, 0.0f, 0.0f, false);
            m_ecsSystem->AddComponent<Bounds>(m_settingsButtonEntity, buttonBounds);
            
            GN_LOG_INFO("TutorialState: Settings button created at top right");
        }
    }
    
    void TutorialState::UpdateInput() {
        // Get InputManager singleton
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) return;
        
        m_touchPressed = false;
        m_touchReleased = false;
        
        // Get active touches from InputManager
        auto touches = inputManager->GetActiveTouches();
        
        if (!touches.empty()) {
            const TouchData& touch = touches[0];  // Use first touch
            m_touchPosition = GNVector2(touch.rawX, touch.rawY);  // Use pixel coordinates
            
            if (touch.state == TouchState::PRESSED) {
                m_touchPressed = true;
            } else if (touch.state == TouchState::RELEASED) {
                m_touchReleased = true;
            }
        }
    }
    

    void TutorialState::DestroyUI() {
        if (!m_ecsSystem) return;
        
        if (m_pipeCounterEntity != 0) {
            m_ecsSystem->DestroyEntity(m_pipeCounterEntity);
            m_pipeCounterEntity = 0;
        }
        if (m_coinBagEntity != 0) {
            m_ecsSystem->DestroyEntity(m_coinBagEntity);
            m_coinBagEntity = 0;
        }
        if (m_coinsTextEntity != 0) {
            m_ecsSystem->DestroyEntity(m_coinsTextEntity);
            m_coinsTextEntity = 0;
        }
        if (m_shootingZoneEntity != 0) {
            m_ecsSystem->DestroyEntity(m_shootingZoneEntity);
            m_shootingZoneEntity = 0;
        }
        if (m_settingsButtonEntity != 0) {
            m_ecsSystem->DestroyEntity(m_settingsButtonEntity);
            m_settingsButtonEntity = 0;
        }
        
        // Destroy instruction text entities
        for (Gnosis::Entity entity : m_instructionTextEntities) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_instructionTextEntities.clear();
        
        // Destroy pause menu entities
        if (m_pauseBackgroundEntity != 0) {
            m_ecsSystem->DestroyEntity(m_pauseBackgroundEntity);
            m_pauseBackgroundEntity = 0;
        }
        if (m_pausedTextEntity != 0) {
            m_ecsSystem->DestroyEntity(m_pausedTextEntity);
            m_pausedTextEntity = 0;
        }
        if (m_returnToMenuButtonEntity != 0) {
            m_ecsSystem->DestroyEntity(m_returnToMenuButtonEntity);
            m_returnToMenuButtonEntity = 0;
        }
        
        GN_LOG_INFO("TutorialState: UI destroyed");
    }
    
    void TutorialState::DestroyEntities() {
        if (!m_ecsSystem) return;
        
        // Destroy player
        if (m_playerEntity != 0) {
            m_ecsSystem->DestroyEntity(m_playerEntity);
            m_playerEntity = 0;
        }
        
        // Destroy backgrounds
        for (Gnosis::Entity entity : m_backgroundEntities) {
            if (entity != 0) {
                m_ecsSystem->DestroyEntity(entity);
            }
        }
        m_backgroundEntities.clear();
        
        // Projectiles managed by ProjectileSystem - no manual cleanup needed
        
        GN_LOG_INFO("TutorialState: Entities destroyed");
    }
    
    void TutorialState::CreatePauseMenu() {
        // Simple pause overlay matching PauseSystem visual style
        
        // 1. Pause menu background sprite (like PauseSystem)
        m_pauseBackgroundEntity = m_ecsSystem->CreateEntity();
        
        float centerX = m_cachedScreenWidth * 0.5f;
        float centerY = m_cachedScreenHeight * 0.5f + 32.0f;  // Offset like PauseSystem
        
        // Dynamic scaling logic below (replacing hardcoded 7.0f)
        
        // Use portrait or landscape background based on aspect ratio
        bool isLandscape = (m_cachedScreenWidth / m_cachedScreenHeight) > 1.0f;
        std::string bgTextureId = isLandscape ? "PauseMenuBackground" : "PauseMenuBackgroundMobile";
        float textureWidth = isLandscape ? 300.0f : 160.0f;
        float textureHeight = isLandscape ? 160.0f : 300.0f;
        
        // Dynamic scaling: Fit within 95% of width for portrait, or appropriate height for landscape
        float maxW = m_cachedScreenWidth * 0.95f;
        float maxH = m_cachedScreenHeight * 0.90f; // Increased to 90% height as requested
        
        float scaleX = maxW / textureWidth;
        float scaleY = maxH / textureHeight;
        
        float bgScale = std::min(scaleX, scaleY);
        
        // Calculate scaled dimensions and use CenterObjectAtPosition
        float scaledWidth = textureWidth * bgScale;
        float scaledHeight = textureHeight * bgScale;
        Gnosis::GNVector2 bgPosition = CenterObjectAtPosition(centerX, centerY, scaledWidth, scaledHeight);
        
        Transform bgTransform(bgPosition, 0.0f, Gnosis::GNVector2(bgScale, bgScale));
        m_ecsSystem->AddComponent<Transform>(m_pauseBackgroundEntity, bgTransform);
        
        Sprite bgSprite(bgTextureId, textureWidth, textureHeight);
        bgSprite.layer = 80;  // Same as PauseSystem
        bgSprite.visible = false;
        m_ecsSystem->AddComponent<Sprite>(m_pauseBackgroundEntity, bgSprite);
        
        UIElement bgUI;
        bgUI.normalTextureId = bgTextureId;
        bgUI.visible = false;
        bgUI.textLayer = 80;
        m_ecsSystem->AddComponent<UIElement>(m_pauseBackgroundEntity, bgUI);
        
        // 2. "PAUSED" text
        m_pausedTextEntity = m_ecsSystem->CreateEntity();
        Transform pausedTransform(Gnosis::GNVector2(m_cachedScreenWidth * 0.5f, m_cachedScreenHeight * 0.3f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_pausedTextEntity, pausedTransform);
        
        UIElement pausedText;
        pausedText.buttonText = "PAUSED";
        pausedText.fontSize = 80.0f;
        pausedText.textColor = Gnosis::GNColor(255, 255, 255, 255);
        pausedText.centerTextHorizontally = true;
        pausedText.centerTextVertically = true;
        pausedText.visible = false;
        pausedText.textLayer = 201;  // Above pause background (layer 200)
        m_ecsSystem->AddComponent<UIElement>(m_pausedTextEntity, pausedText);
        
        // 3. "Main Menu" button with FloppyButtonBlue sprite
        m_returnToMenuButtonEntity = m_ecsSystem->CreateEntity();
        
        float buttonCenterX = centerX;
        float buttonCenterY = centerY + 64.0f;  // Below center (pause menu is at centerY)
        
        // Dynamic scaling: Target 75% of screen width (Portrait reference)
        float buttonTexWidth = 90.0f;
        float buttonTexHeight = 16.0f;
        
        float targetButtonWidth = m_cachedScreenWidth * 0.75f;
        // Limit height just in case
        float targetMaxHeight = m_cachedScreenHeight * 0.12f;
        
        float scaleXButton = targetButtonWidth / buttonTexWidth;
        float scaleYButton = targetMaxHeight / buttonTexHeight;
        
        float buttonScale = std::min(scaleXButton, scaleYButton);
        
        // Calculate button dimensions and use CenterObjectAtPosition
        float buttonWidth = 90.0f * buttonScale;  // 900 pixels
        float buttonHeight = 16.0f * buttonScale;  // 160 pixels
        Gnosis::GNVector2 buttonPosition = CenterObjectAtPosition(buttonCenterX, buttonCenterY, buttonWidth, buttonHeight);
        
        Transform buttonTransform(buttonPosition, 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        m_ecsSystem->AddComponent<Transform>(m_returnToMenuButtonEntity, buttonTransform);
        
        // FloppyButtonBlue sprite (no .png extension)
        Sprite buttonSprite("FloppyButtonBlue", 90.0f, 16.0f);
        buttonSprite.layer = 90;  // Above everything
        buttonSprite.visible = false;
        m_ecsSystem->AddComponent<Sprite>(m_returnToMenuButtonEntity, buttonSprite);
        
        // Button text (EXACT match to PauseSystem font size)
        UIElement buttonUI;
        buttonUI.buttonText = "Main Menu";
        buttonUI.fontSize = 62.0f;  // Match PauseSystem main menu button
        buttonUI.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White
        buttonUI.normalTextureId = "FloppyButtonBlue";
        buttonUI.centerTextHorizontally = true;
        buttonUI.centerTextVertically = true;
        buttonUI.visible = false;
        buttonUI.textLayer = 91;  // Above button sprite
        m_ecsSystem->AddComponent<UIElement>(m_returnToMenuButtonEntity, buttonUI);
        
        // Add Bounds component for proper collision detection (covering entire button)
        Bounds buttonBounds(buttonWidth, buttonHeight, 0.0f, 0.0f, false);  // Top-left aligned
        m_ecsSystem->AddComponent<Bounds>(m_returnToMenuButtonEntity, buttonBounds);
        
        GN_LOG_INFO("TutorialState: Simple pause menu created");
    }
    
    void TutorialState::ShowPauseMenu() {
        if (m_pauseBackgroundEntity != 0) {
            auto* bgSprite = m_ecsSystem->GetComponent<Sprite>(m_pauseBackgroundEntity);
            if (bgSprite) bgSprite->visible = true;
            auto* bgUI = m_ecsSystem->GetComponent<UIElement>(m_pauseBackgroundEntity);
            if (bgUI) bgUI->visible = true;
        }
        if (m_pausedTextEntity != 0) {
            auto* text = m_ecsSystem->GetComponent<UIElement>(m_pausedTextEntity);
            if (text) text->visible = true;
        }
        if (m_returnToMenuButtonEntity != 0) {
            auto* buttonSprite = m_ecsSystem->GetComponent<Sprite>(m_returnToMenuButtonEntity);
            if (buttonSprite) buttonSprite->visible = true;
            auto* buttonUI = m_ecsSystem->GetComponent<UIElement>(m_returnToMenuButtonEntity);
            if (buttonUI) buttonUI->visible = true;
        }
    }
    
    void TutorialState::HidePauseMenu() {
        if (m_pauseBackgroundEntity != 0) {
            auto* bgSprite = m_ecsSystem->GetComponent<Sprite>(m_pauseBackgroundEntity);
            if (bgSprite) bgSprite->visible = false;
            auto* bgUI = m_ecsSystem->GetComponent<UIElement>(m_pauseBackgroundEntity);
            if (bgUI) bgUI->visible = false;
        }
        if (m_pausedTextEntity != 0) {
            auto* text = m_ecsSystem->GetComponent<UIElement>(m_pausedTextEntity);
            if (text) text->visible = false;
        }
        if (m_returnToMenuButtonEntity != 0) {
            auto* buttonSprite = m_ecsSystem->GetComponent<Sprite>(m_returnToMenuButtonEntity);
            if (buttonSprite) buttonSprite->visible = false;
            auto* buttonUI = m_ecsSystem->GetComponent<UIElement>(m_returnToMenuButtonEntity);
            if (buttonUI) buttonUI->visible = false;
        }
    }
    
    void TutorialState::TogglePause() {
        m_isPaused = !m_isPaused;
        
        if (m_isPaused) {
            ShowPauseMenu();
        } else {
            HidePauseMenu();
        }
    }
    
    void TutorialState::UpdateCoinCounterUI() {
        if (m_coinsTextEntity != 0) {
            auto* ui = m_ecsSystem->GetComponent<UIElement>(m_coinsTextEntity);
            if (ui) {
                ui->buttonText = std::to_string(m_tutorialCoins);
            }
        }
    }
    
    bool TutorialState::CheckSettingsButtonClick(float x, float y) {
        auto* transform = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);
        if (!transform) return false;
        
        // Use fixed scale 8.0f as defined in CreateSettingsButton
        float buttonSize = 16.0f * 8.0f; 
        
        // Check bounds (top-left positioning)
        if (x >= transform->position.x && x <= transform->position.x + buttonSize &&
            y >= transform->position.y && y <= transform->position.y + buttonSize) {
            
            // Return true, caller will handle TogglePause()
            return true;
        }
        return false;
    }
    
    bool TutorialState::CheckReturnToMenuButtonClick(float x, float y) {
        // Use proper bounds checking like PauseSystem
        if (m_returnToMenuButtonEntity == 0) return false;
        
        auto* transform = m_ecsSystem->GetComponent<Transform>(m_returnToMenuButtonEntity);
        auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_returnToMenuButtonEntity);
        auto* uiElement = m_ecsSystem->GetComponent<UIElement>(m_returnToMenuButtonEntity);
        
        if (!transform || !sprite || !uiElement || !uiElement->visible) return false;
        
        // Calculate button bounds (top-left positioned)
        float width = sprite->width * transform->scale.x;
        float height = sprite->height * transform->scale.y;
        float left = transform->position.x;
        float top = transform->position.y;
        float right = left + width;
        float bottom = top + height;
        
        GN_LOG_INFO("TutorialState: Button click check - touch(" + std::to_string(x) + "," + std::to_string(y) +
                   ") vs bounds(" + std::to_string(left) + "," + std::to_string(top) + "," +
                   std::to_string(right) + "," + std::to_string(bottom) + ")");
        
        return (x >= left && x <= right && y >= top && y <= bottom);
    }

} // namespace GameCore
