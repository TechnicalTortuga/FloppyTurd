# Tutorial State Implementation Tracker

## Overview
Refactor TutorialState to properly use GameplayState patterns WITHOUT modifying GameplayState or PauseSystem.

**Key Understanding:**
- ✅ RenderSystem handles actual rendering
- ✅ SpriteSystem updates sprite states
- ✅ We create Sprite components that RenderSystem will render
- ✅ TutorialState is STANDALONE - NOT a subclass of GameplayState
- ✅ NO modifications to GameplayState or PauseSystem
- ✅ Simple pause menu built directly into TutorialState

---

## Phase 1: System Initialization

### ✅ Systems to Include (from GameplayState patterns)

| System | Purpose | Copy From | Notes |
|--------|---------|-----------|-------|
| `RenderSystem*` | Screen info & rendering | Line 1150-1163 | Borrowed from SystemManager, not owned |
| `SpriteSystem` | Sprite updates | Line 1110 | Needed for sprite management |
| `PlayerControllerSystem` | Player entity & behavior | Line 1139 | Handles jump, shoot, animations |

### ❌ Systems to SKIP

| System | Reason |
|--------|--------|
| `HatsSystem` | Not needed for tutorial (use default player) |
| `SkillSystem` | Not needed for tutorial |
| `UISystem` | Manual UI creation instead |
| `PauseSystem` | Simple pause menu built into TutorialState |
| `LevelManager` | No obstacles/enemies in tutorial |
| `CameraSystem` | No scrolling needed |
| `PickupSystem` | No pickups in tutorial |
| `EnemySystem` | No enemies in tutorial |
| `ProjectileSystem` | No actual shooting in tutorial |
| `BossSystem` | Not a boss level |
| `OverlaySystem` | No snow/effects in tutorial |

### Implementation Plan:

```cpp
// TutorialState.h - Add member variables
private:
    // Core systems
    Gnosis::ECS* m_ecsSystem;
    PlatformDelegates* m_platformDelegates;
    
    // Game systems (minimal set - only 3 systems!)
    std::unique_ptr<SpriteSystem> m_spriteSystem;
    std::unique_ptr<PlayerControllerSystem> m_playerControllerSystem;
    RenderSystem* m_renderSystem;  // Borrowed from SystemManager
    
    // Cached screen dimensions
    float m_cachedScreenWidth;
    float m_cachedScreenHeight;
```

**Function to Implement:**
- `TutorialState::InitializeSystems()` - Simplified version
  - Create SpriteSystem (Line 1110 pattern)
  - Create PlayerControllerSystem (Line 1139 pattern, pass nullptr for HatsSystem and SkillSystem)
  - Get RenderSystem from SystemManager (Lines 1150-1163)
  - That's it! Only 3 systems needed

---

## Phase 2: Background Rendering

### Pattern to Copy: Background Layer System

**Source:** 
- `GameplayState::CreateBackgroundLayers()` (Lines 1550-1717)
- `LevelConfigFactory::AddParkLevelLayers()` (LevelConfig.cpp Lines 171-193)

**What it does:**
1. Creates background layer entities
2. Adds Transform component with proper position and scaling
3. Adds Sprite component (RenderSystem will render it)
4. Calculates proper scaling to fit screen height
5. Positions layers for proper rendering

**For Tutorial:**
- Use **EXACT Park Level (Level 1) background layers**
- Keep static (no scrolling) - NO Parallax components
- Only need 1 instance per layer (no wrapping needed)
- Use EXACT texture IDs from Park Level config

### Park Level Background Layers (from LevelConfig.cpp):

```cpp
// Exact textures from Level 1:
1. "Level1BackLayerBackground"  - Back layer (render layer 0)
2. "Level1MidLayerBackground"   - Mid layer (render layer 1)
3. "Level1Clouds"               - Clouds (render layer 0)
4. "Level1FrontLayerBackground" - Front layer (render layer 2)
```

### Implementation Plan:

```cpp
// TutorialState.cpp
void TutorialState::CreateBackground() {
    // Use EXACT Park Level background layers (Level 1)
    // Simplified version - NO parallax scrolling
    // Static background - tutorial doesn't scroll
    
    struct SimpleBgLayer {
        std::string textureId;
        float textureWidth;
        float textureHeight;
        int renderLayer;
    };
    
    // EXACT Park Level textures from LevelConfig.cpp
    // Each layer on SEPARATE render layer to avoid layering issues
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
        Transform bgTransform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(heightScale, heightScale));
        m_ecsSystem->AddComponent<Transform>(bgEntity, bgTransform);
        
        // Create Sprite component (RenderSystem will render it)
        Sprite bgSprite(layer.textureId, layer.textureWidth, layer.textureHeight);
        bgSprite.color = GNColor(255, 255, 255, 255);
        bgSprite.visible = true;
        bgSprite.layer = layer.renderLayer;
        m_ecsSystem->AddComponent<Sprite>(bgEntity, bgSprite);
        
        m_backgroundEntities.push_back(bgEntity);
    }
}
```

**Member variables needed:**
```cpp
std::vector<Gnosis::Entity> m_backgroundEntities;
```

**Note:** These are the EXACT same textures used in Park Level gameplay.

---

## Phase 3: Player Entity

### Pattern to Copy: Player Creation + PlayerControllerSystem

**Source:** 
- `GameplayState::CreateGameEntities()` Lines 1418-1550 (player creation)
- `PlayerControllerSystem` handles all sprite loading, animations, input

**What it does:**
1. Creates player entity
2. Adds Transform component with initial position
3. Adds PlayerComponent with stats
4. PlayerControllerSystem loads sprites and handles rendering
5. PlayerControllerSystem handles jump input
6. PlayerControllerSystem handles shoot input (we'll disable actual projectiles)

**For Tutorial:**
- Position player at center-left (not scrolling, so center of screen)
- Restrict Y movement to bottom 50% of screen
- Disable actual projectile spawning

### Implementation Plan:

```cpp
// TutorialState.cpp
void TutorialState::CreatePlayer() {
    m_playerEntity = m_ecsSystem->CreateEntity();
    if (m_playerEntity == 0) return;
    
    // Position player in center-left of screen
    float playerX = m_cachedScreenWidth * 0.25f;  // 25% from left
    float playerY = m_cachedScreenHeight * 0.50f; // Center vertically
    
    Transform playerTransform(GNVector2(playerX, playerY), 0.0f, GNVector2(6.0f, 6.0f));
    m_ecsSystem->AddComponent<Transform>(m_playerEntity, playerTransform);
    
    // Create player component with basic stats
    PlayerComponent playerData;
    playerData.velocity = GNVector2(0.0f, 0.0f);
    playerData.maxHealth = 3;
    playerData.currentHealth = 3;
    playerData.isAlive = true;
    playerData.isHurt = false;
    playerData.invulnerable = false;
    playerData.totalCoins = 999;  // Fixed for tutorial
    playerData.sessionCoins = 0;
    playerData.state = PlayerState::Idle;
    m_ecsSystem->AddComponent<PlayerComponent>(m_playerEntity, playerData);
    
    // PlayerControllerSystem will:
    // - Load FloppyTurd sprites
    // - Handle animations
    // - Process jump input
    // - Update player state
}
```

**Constraint player to bottom half:**
```cpp
// In TutorialState::Update()
void TutorialState::ConstrainPlayer() {
    auto* transform = m_ecsSystem->GetComponent<Transform>(m_playerEntity);
    auto* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
    
    if (transform && player) {
        // Constrain to bottom 50% of screen
        float minY = m_cachedScreenHeight * 0.50f;  // Top boundary (center)
        float maxY = m_cachedScreenHeight * 0.90f;  // Bottom boundary
        
        if (transform->position.y < minY) {
            transform->position.y = minY;
            player->velocity.y = 0.0f;
        }
        if (transform->position.y > maxY) {
            transform->position.y = maxY;
            player->velocity.y = 0.0f;
        }
    }
}
```

---

## Phase 4: UI Creation

### Pattern to Copy: GameplayState UI Layout

**Source:** `GameplayState::CreateUI()` Lines 1797-2054

**What it creates:**
1. **Pipe Counter** - Top center, large white text with outline
2. **Coin Bag Icon** - Bottom left (1% from left, 87% from top)
3. **Coin Counter Text** - Next to coin bag, gold color
4. **Shooting Zone Visual** - Bottom right, semi-transparent gray rectangle
5. **Settings Button** - Top right corner

**For Tutorial - Exact Same Layout:**

### Implementation Plan:

```cpp
// TutorialState.cpp - Copy exact pattern from GameplayState
void TutorialState::CreateUI() {
    if (!m_ecsSystem) return;
    
    // 1. PIPE COUNTER (top center)
    m_pipeCounterEntity = m_ecsSystem->CreateEntity();
    if (m_pipeCounterEntity != 0) {
        float centerX = m_cachedScreenWidth * 0.50f;
        float pipeY = m_cachedScreenHeight * 0.10f;  // 10% from top
        
        Transform pipeTransform(GNVector2(centerX, pipeY), 0.0f, GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_pipeCounterEntity, pipeTransform);
        
        UIElement pipeCounter;
        pipeCounter.buttonText = "0";  // NON-FUNCTIONAL - stays at 0 (tutorial explains controls)
        pipeCounter.fontSize = 120.0f;
        pipeCounter.textOutlineWidth = 18.0f;
        pipeCounter.textColor = GNColor(255, 255, 255, 255);  // White
        pipeCounter.centerTextHorizontally = true;
        pipeCounter.centerTextVertically = true;
        pipeCounter.visible = true;
        pipeCounter.isEnabled = true;
        pipeCounter.textLayer = 10;
        m_ecsSystem->AddComponent<UIElement>(m_pipeCounterEntity, pipeCounter);
    }
    
    // 2. COIN BAG ICON (bottom left)
    float iconX = m_cachedScreenWidth * 0.01f;   // 1% from left
    float iconY = m_cachedScreenHeight * 0.87f;  // 87% from top
    float bagScale = 8.0f;  // 32x32 → 256x256
    
    m_coinBagEntity = m_ecsSystem->CreateEntity();
    if (m_coinBagEntity != 0) {
        Transform tr(GNVector2(iconX, iconY), 0.0f, GNVector2(bagScale, bagScale));
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
        
        Transform tr(GNVector2(textX, textY), 0.0f, GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_coinsTextEntity, tr);
        
        UIElement ui;
        ui.buttonText = "999";  // FUNCTIONAL - decrements when player shoots
        ui.fontSize = 64.0f;
        ui.textOutlineWidth = 10.0f;
        ui.textColor = GNColor(255, 215, 0, 255);  // Gold
        ui.centerTextHorizontally = false;
        ui.centerTextVertically = true;
        ui.visible = true;
        ui.isEnabled = true;
        ui.textLayer = 10;
        m_ecsSystem->AddComponent<UIElement>(m_coinsTextEntity, ui);
    }
    
    // 4. SHOOTING ZONE VISUAL (bottom right)
    // TODO: Add rounded rectangle support to Metal renderer for nicer appearance
    m_shootingZoneEntity = m_ecsSystem->CreateEntity();
    if (m_shootingZoneEntity != 0) {
        // Position to right of coin counter
        float shootingZoneLeftX = textX + 200.0f + 8.0f;  // After coin counter
        float shootingZoneRightX = m_cachedScreenWidth * 0.95f;  // 5% from right
        float shootingZoneTopY = m_cachedScreenHeight * 0.80f;   // 80% from top
        float shootingZoneBottomY = m_cachedScreenHeight * 0.95f; // 95% from top
        
        float width = shootingZoneRightX - shootingZoneLeftX;
        float height = shootingZoneBottomY - shootingZoneTopY;
        
        Transform shootTransform(GNVector2(shootingZoneLeftX, shootingZoneTopY), 0.0f, GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_shootingZoneEntity, shootTransform);
        
        UIElement shootUI;
        shootUI.visible = true;
        shootUI.isEnabled = true;
        shootUI.textLayer = 10;
        m_ecsSystem->AddComponent<UIElement>(m_shootingZoneEntity, shootUI);
        
        UIShape shootShape;
        shootShape.type = UIShapeType::Rectangle;  // Currently rectangle, TODO: add RoundedRectangle type
        shootShape.width = width;
        shootShape.height = height;
        shootShape.color = GNColor(128, 128, 128, 64);  // Semi-transparent gray
        shootShape.layer = 10;
        shootShape.visible = true;
        m_ecsSystem->AddComponent<UIShape>(m_shootingZoneEntity, shootShape);
    }
    
    // 5. SETTINGS BUTTON (top right)
    CreateSettingsButton();
    
    // 6. Initialize PauseSystem
    if (m_pauseSystem && !m_pauseSystem->IsVisible()) {
        m_pauseSystem->Initialize();
    }
}
```

**Function to copy:**
- `GameplayState::CreateSettingsButton()` - for settings button creation

**Member variables needed:**
```cpp
Gnosis::Entity m_pipeCounterEntity;
Gnosis::Entity m_coinBagEntity;
Gnosis::Entity m_coinsTextEntity;
Gnosis::Entity m_shootingZoneEntity;
Gnosis::Entity m_settingsButtonEntity;
```

---

## Phase 5: Settings Button & Simple Pause Menu

### Pattern to Copy: Settings Button + Simple Pause Overlay

**Source:** 
- `GameplayState::CreateSettingsButton()` - Settings button creation pattern

**For Tutorial:**
- Same settings button (top right)
- **NO PauseSystem** - build simple pause overlay directly in TutorialState
- Simple pause overlay with:
  - Semi-transparent dark background
  - "PAUSED" text
  - "Return to Main Menu" button only
- No ribbons, audio options, or other controls from PauseSystem

### Implementation Plan:

```cpp
// TutorialState.cpp
void TutorialState::CreateSettingsButton() {
    // Copy exact implementation from GameplayState
    // Settings button at top-right corner
    float settingsX = m_cachedScreenWidth * 0.85f;   // 85% from left
    float settingsY = m_cachedScreenHeight * 0.05f;  // 5% from top
    float settingsScale = 1.5f;
    
    m_settingsButtonEntity = m_ecsSystem->CreateEntity();
    if (m_settingsButtonEntity != 0) {
        Transform settingsTransform(GNVector2(settingsX, settingsY), 0.0f, GNVector2(settingsScale, settingsScale));
        m_ecsSystem->AddComponent<Transform>(m_settingsButtonEntity, settingsTransform);
        
        Sprite settingsSprite("ButtonSettings", 128.0f, 128.0f);
        settingsSprite.layer = 10;
        settingsSprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_settingsButtonEntity, settingsSprite);
        
        UIElement settingsUI;
        settingsUI.normalTextureId = "ButtonSettings";
        settingsUI.visible = true;
        settingsUI.isEnabled = true;
        settingsUI.textLayer = 10;
        m_ecsSystem->AddComponent<UIElement>(m_settingsButtonEntity, settingsUI);
    }
}
```

**Pause Menu Handling:**
```cpp
// TutorialState.cpp
void TutorialState::HandleInput() {
    if (!m_initialized) return;
    
    // Update input first
    UpdateInput();
    
    // Check for settings button press
    if (m_touchPressed) {
        if (CheckSettingsButtonClick(m_touchPosition.x, m_touchPosition.y)) {
            TogglePause();
            return;
        }
    }
    
    // If paused, let PauseSystem handle input
    if (m_isPaused) {
        m_pauseSystem->HandleInput();
        return;
    }
    
    // Otherwise, handle tutorial input (jump on tap)
    HandleTutorialInput();
}

void TutorialState::TogglePause() {
    m_isPaused = !m_isPaused;
    
    if (m_isPaused) {
        m_pauseSystem->Show();
    } else {
        m_pauseSystem->Hide();
    }
}

bool TutorialState::CheckSettingsButtonClick(float x, float y) {
    auto* transform = m_ecsSystem->GetComponent<Transform>(m_settingsButtonEntity);
    if (!transform) return false;
    
    float buttonSize = 128.0f * 1.5f;  // 128px * scale
    float halfSize = buttonSize * 0.5f;
    
    return (x >= transform->position.x - halfSize &&
            x <= transform->position.x + halfSize &&
            y >= transform->position.y - halfSize &&
            y <= transform->position.y + halfSize);
}
```

**Member variables needed:**
```cpp
bool m_isPaused;
```

**Simple Pause Menu Implementation:**
Create a minimal pause overlay directly in TutorialState (NO modifications to PauseSystem needed):

```cpp
// TutorialState.cpp
void TutorialState::CreatePauseMenu() {
    // Create simple pause overlay entities
    
    // 1. Dark semi-transparent background
    m_pauseBackgroundEntity = m_ecsSystem->CreateEntity();
    Transform bgTransform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_pauseBackgroundEntity, bgTransform);
    
    UIShape bgShape;
    bgShape.type = UIShapeType::Rectangle;
    bgShape.width = m_cachedScreenWidth;
    bgShape.height = m_cachedScreenHeight;
    bgShape.color = GNColor(0, 0, 0, 180);  // Dark semi-transparent
    bgShape.layer = 100;  // High layer to cover everything
    bgShape.visible = false;  // Hidden until pause
    m_ecsSystem->AddComponent<UIShape>(m_pauseBackgroundEntity, bgShape);
    
    // 2. "PAUSED" text
    m_pausedTextEntity = m_ecsSystem->CreateEntity();
    Transform pausedTransform(GNVector2(m_cachedScreenWidth * 0.5f, m_cachedScreenHeight * 0.3f), 0.0f, GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_pausedTextEntity, pausedTransform);
    
    UIElement pausedText;
    pausedText.buttonText = "PAUSED";
    pausedText.fontSize = 80.0f;
    pausedText.textColor = GNColor(255, 255, 255, 255);
    pausedText.centerTextHorizontally = true;
    pausedText.centerTextVertically = true;
    pausedText.visible = false;
    pausedText.textLayer = 101;
    m_ecsSystem->AddComponent<UIElement>(m_pausedTextEntity, pausedText);
    
    // 3. "Return to Main Menu" button (text only)
    m_returnToMenuButtonEntity = m_ecsSystem->CreateEntity();
    Transform buttonTransform(GNVector2(m_cachedScreenWidth * 0.5f, m_cachedScreenHeight * 0.5f), 0.0f, GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_returnToMenuButtonEntity, buttonTransform);
    
    UIElement buttonText;
    buttonText.buttonText = "Main Menu";  // Shorter text for better fit
    buttonText.fontSize = 56.0f;  // Larger since text is shorter
    buttonText.textColor = GNColor(255, 215, 0, 255);  // Gold
    buttonText.centerTextHorizontally = true;
    buttonText.centerTextVertically = true;
    buttonText.visible = false;
    buttonText.textLayer = 101;
    m_ecsSystem->AddComponent<UIElement>(m_returnToMenuButtonEntity, buttonText);
}

void TutorialState::ShowPauseMenu() {
    if (m_pauseBackgroundEntity != 0) {
        auto* bgShape = m_ecsSystem->GetComponent<UIShape>(m_pauseBackgroundEntity);
        if (bgShape) bgShape->visible = true;
    }
    if (m_pausedTextEntity != 0) {
        auto* text = m_ecsSystem->GetComponent<UIElement>(m_pausedTextEntity);
        if (text) text->visible = true;
    }
    if (m_returnToMenuButtonEntity != 0) {
        auto* button = m_ecsSystem->GetComponent<UIElement>(m_returnToMenuButtonEntity);
        if (button) button->visible = true;
    }
}

void TutorialState::HidePauseMenu() {
    if (m_pauseBackgroundEntity != 0) {
        auto* bgShape = m_ecsSystem->GetComponent<UIShape>(m_pauseBackgroundEntity);
        if (bgShape) bgShape->visible = false;
    }
    if (m_pausedTextEntity != 0) {
        auto* text = m_ecsSystem->GetComponent<UIElement>(m_pausedTextEntity);
        if (text) text->visible = false;
    }
    if (m_returnToMenuButtonEntity != 0) {
        auto* button = m_ecsSystem->GetComponent<UIElement>(m_returnToMenuButtonEntity);
        if (button) button->visible = false;
    }
}
```

**Member variables needed:**
```cpp
Gnosis::Entity m_pauseBackgroundEntity;
Gnosis::Entity m_pausedTextEntity;
Gnosis::Entity m_returnToMenuButtonEntity;
```

---

## Phase 6: Update Loop & Cleanup

### Functions to Implement:

```cpp
// TutorialState.cpp

void TutorialState::Enter() {
    GN_LOG_INFO("Entering TutorialState");
    
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
    
    // 6. Reset state
    m_finished = false;
    m_isPaused = false;
    m_initialized = true;
    
    GN_LOG_INFO("TutorialState entered successfully");
}

void TutorialState::Exit() {
    GN_LOG_INFO("Exiting TutorialState");
    
    // 1. Cleanup UI
    DestroyUI();
    
    // 2. Cleanup entities
    DestroyEntities();
    
    // 3. Cleanup pause system
    if (m_pauseSystem) {
        m_pauseSystem->Cleanup();
    }
    
    GN_LOG_INFO("TutorialState exited");
}

void TutorialState::Update(float deltaTime) {
    if (!m_initialized) return;
    
    // 1. Update InputManager singleton
    InputManager* inputManager = InputManager::GetInstance();
    if (inputManager) {
        inputManager->Update(deltaTime);
    }
    
    // 2. If paused, only update pause system
    if (m_isPaused) {
        m_pauseSystem->Update(deltaTime);
        return;
    }
    
    // 3. Update sprite system (updates sprite states)
    if (m_spriteSystem) {
        m_spriteSystem->Update(deltaTime);
    }
    
    // 4. Update player controller (handles physics, animations, input)
    if (m_playerControllerSystem) {
        m_playerControllerSystem->Update(deltaTime);
    }
    
    // 5. Constrain player to bottom half
    ConstrainPlayer();
    
    // 6. Update UI system
    if (m_uiSystem) {
        m_uiSystem->Update(deltaTime);
    }
}

void TutorialState::Render() {
    // RenderSystem handles all rendering automatically
    // No explicit render calls needed
}

void TutorialState::CacheScreenDimensions() {
    if (m_renderSystem) {
        const ScreenInfo& screenInfo = m_renderSystem->GetScreenInfo();
        m_cachedScreenWidth = screenInfo.pixelWidth;
        m_cachedScreenHeight = screenInfo.pixelHeight;
        GN_LOG_INFO("Cached screen dimensions: " + std::to_string((int)m_cachedScreenWidth) + "x" + std::to_string((int)m_cachedScreenHeight));
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
}
```

---

## Phase 7: Input Handling

### Pattern to Copy: Input with PauseSystem integration

```cpp
// TutorialState.cpp
void TutorialState::HandleInput() {
    if (!m_initialized) return;
    
    // Update input
    UpdateInput();
    
    // Check for settings button press
    if (m_touchPressed) {
        if (CheckSettingsButtonClick(m_touchPosition.x, m_touchPosition.y)) {
            TogglePause();
            return;
        }
    }
    
    // If paused, check for "Return to Main Menu" button press
    if (m_isPaused) {
        if (m_touchPressed) {
            if (CheckReturnToMenuButtonClick(m_touchPosition.x, m_touchPosition.y)) {
                m_finished = true;
                return;
            }
        }
        return;  // Don't process other input while paused
    }
    
    // Tutorial input: any tap makes player jump
    // (PlayerControllerSystem handles the actual jump)
    // Shooting is handled by PlayerControllerSystem automatically
}

bool TutorialState::CheckReturnToMenuButtonClick(float x, float y) {
    auto* transform = m_ecsSystem->GetComponent<Transform>(m_returnToMenuButtonEntity);
    if (!transform) return false;
    
    // Simple hit test - large hit area for text button
    float buttonWidth = 600.0f;   // Generous width
    float buttonHeight = 100.0f;  // Generous height
    float halfWidth = buttonWidth * 0.5f;
    float halfHeight = buttonHeight * 0.5f;
    
    return (x >= transform->position.x - halfWidth &&
            x <= transform->position.x + halfWidth &&
            y >= transform->position.y - halfHeight &&
            y <= transform->position.y + halfHeight);
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
```

---

## Summary Checklist

### Files to Modify:

- [x] `/src/FloppyTurd/States/MainMenuState.cpp` - Add How To button to visibility ✅ DONE
- [ ] `/src/FloppyTurd/States/TutorialState.h` - Refactor header with new systems
- [ ] `/src/FloppyTurd/States/TutorialState.cpp` - Reimplement using GameplayState patterns

**NO modifications needed to:**
- ❌ GameplayState (not a subclass)
- ❌ PauseSystem (simple pause built into TutorialState)
- ❌ Any other existing systems

### New Member Variables (TutorialState.h):

```cpp
// Systems (ONLY 3 systems!)
std::unique_ptr<SpriteSystem> m_spriteSystem;
std::unique_ptr<PlayerControllerSystem> m_playerControllerSystem;
RenderSystem* m_renderSystem;  // Borrowed from SystemManager

// Cached dimensions
float m_cachedScreenWidth;
float m_cachedScreenHeight;

// Game entities
Gnosis::Entity m_playerEntity;
std::vector<Gnosis::Entity> m_backgroundEntities;

// UI entities
Gnosis::Entity m_pipeCounterEntity;
Gnosis::Entity m_coinBagEntity;
Gnosis::Entity m_coinsTextEntity;
Gnosis::Entity m_shootingZoneEntity;
Gnosis::Entity m_settingsButtonEntity;

// Simple pause menu entities (NO PauseSystem)
Gnosis::Entity m_pauseBackgroundEntity;
Gnosis::Entity m_pausedTextEntity;
Gnosis::Entity m_returnToMenuButtonEntity;

// Input state
bool m_touchPressed;
bool m_touchReleased;
GNVector2 m_touchPosition;

// State
bool m_initialized;
bool m_isPaused;
```

### Functions to Implement:

1. **InitializeSystems()** - Initialize 3 systems: SpriteSystem, PlayerControllerSystem, RenderSystem
2. **CreateBackground()** - Create 4 static Park Level background layers (NO parallax)
3. **CreatePlayer()** - Create player entity, let PlayerControllerSystem handle sprites
4. **CreateUI()** - Pipe counter, coin bag+text, shooting zone, settings button
5. **CreateSettingsButton()** - Settings button (top right)
6. **CreatePauseMenu()** - Simple pause overlay (background, "PAUSED" text, return button)
7. **ShowPauseMenu()** - Show pause overlay
8. **HidePauseMenu()** - Hide pause overlay
9. **CacheScreenDimensions()** - Cache screen width/height
10. **ConstrainPlayer()** - Restrict player to bottom 50% of screen
11. **DestroyUI()** - Cleanup UI entities
12. **DestroyEntities()** - Cleanup game entities
13. **UpdateInput()** - Process touch input
14. **HandleInput()** - Handle settings button and pause menu input
15. **TogglePause()** - Show/hide pause menu
16. **CheckSettingsButtonClick()** - Settings button hit test
17. **CheckReturnToMenuButtonClick()** - Return button hit test
18. **Enter()** - Initialize everything
19. **Exit()** - Cleanup everything
20. **Update()** - Update loop (systems + player constraint)

### Estimated Lines of Code:
- TutorialState.h: ~120 lines (member variables + 20 function declarations)
- TutorialState.cpp: ~800 lines (20 functions implemented)
- **Total:** ~920 lines, **2 files only**

### Testing Checklist After Implementation:
- [ ] **Background** - Park Level background renders correctly (all 4 layers visible)
- [ ] **Player** - Player turd renders correctly in center-left
- [ ] **Player Jump** - Player can jump on tap
- [ ] **Player Constraint** - Player constrained to bottom 50% of screen
- [ ] **Pipe Counter** - White "0" visible at top center with outline
- [ ] **Coin Bag** - Coin bag icon visible at bottom left
- [ ] **Coin Count** - Gold "999" visible next to coin bag (decrements on shoot)
- [ ] **Shooting Zone** - Semi-transparent gray rectangle visible at bottom right
- [ ] **Settings Button** - Settings button visible at top right
- [ ] **Pause Toggle** - Settings button click shows pause overlay
- [ ] **Pause Overlay** - Dark background, "PAUSED" text, "Return to Main Menu" button visible
- [ ] **Return Button** - "Return to Main Menu" returns to main menu
- [ ] **How To Button** - Hides when entering other menus ✅ DONE

---

## Additional Notes

### Rounded Rectangle for Shooting Pads (Future Enhancement):
Currently using `UIShapeType::Rectangle` for shooting zone visual. To make it look nicer:
1. Add `UIShapeType::RoundedRectangle` to `UIShape` component
2. Add `cornerRadius` field to `UIShape`
3. Update Metal renderer to draw rounded rectangles
4. Apply to BOTH TutorialState and GameplayState shooting zones

This can be done as a separate enhancement after tutorial is working.

---

## Ready for Green Light! 🚦

Review this tracker and confirm:
1. ✅ All systems correctly identified (only 3 systems needed)
2. ✅ NO modifications to GameplayState or PauseSystem
3. ✅ EXACT Park Level background textures specified
4. ✅ All functions and patterns properly mapped (20 functions)
5. ✅ Member variables complete
6. ✅ Coin counter starts at 999, decrements when shooting (functional)
7. ✅ Simple pause menu built directly into TutorialState
8. ✅ Implementation approach is sound

Once you give the green light, I'll proceed with the implementation in order:
1. Update TutorialState.h with new members (120 lines)
2. Implement InitializeSystems() 
3. Implement CreateBackground() with Park Level textures
4. Implement CreatePlayer()
5. Implement CreateUI() matching GameplayState layout
6. Implement CreatePauseMenu() - simple overlay
7. Implement Update() loop
8. Implement input handling
9. Build and test! 🎮
