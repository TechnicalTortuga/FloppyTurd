# Landscape Game Over Screen - Implementation Guide

## 🎯 Objective
Implement a landscape-specific layout for the game over screen that displays:
- Dead turd sprite on the LEFT side
- Scoreboard and buttons on the RIGHT side
- Properly sized buttons matching the landscape pause menu

---

## 📋 Phase-by-Phase Implementation

### Phase 1: Locate the Code
**File**: `src/FloppyTurd/States/GameplayState.cpp`
**Method**: `CreateGameOverUI()`
**Line**: ~3293 (search for `void GameplayState::CreateGameOverUI()`)

### Phase 2: Add Landscape Detection
**At the very start of the method** (after the null check), add:

```cpp
void GameplayState::CreateGameOverUI() {
    if (!m_ecsSystem) {
        return;
    }
    
    // ADD THIS LINE:
    bool isLandscape = IsLandscapeMode();
    
    // Then wrap ALL existing code in an if/else:
    if (isLandscape) {
        // NEW LANDSCAPE LAYOUT (Phase 3)
    } else {
        // MOVE ALL EXISTING CODE HERE (keep portrait layout as-is)
    }
}
```

### Phase 3: Implement Landscape Layout
**Inside the `if (isLandscape)` block**, add this complete layout:

```cpp
if (isLandscape) {
    // LANDSCAPE LAYOUT: Dead turd on left, scoreboard + buttons on right
    
    // 1. Background (full screen)
    m_gameOverBackgroundEntity = m_ecsSystem->CreateEntity();
    if (m_gameOverBackgroundEntity != Gnosis::INVALID_ENTITY) {
        float backgroundScale = m_cachedScreenWidth / 64.0f;
        Transform bgTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, 
                             Gnosis::GNVector2(backgroundScale, backgroundScale));
        m_ecsSystem->AddComponent<Transform>(m_gameOverBackgroundEntity, bgTransform);
        
        Sprite bgSprite("GameOverBackground", 64, 64);
        bgSprite.layer = 100;
        bgSprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_gameOverBackgroundEntity, bgSprite);
    }
    
    // 2. Morte sprite (LEFT side, vertically centered)
    m_morteEntity = m_ecsSystem->CreateEntity();
    if (m_morteEntity != Gnosis::INVALID_ENTITY) {
        float morteScale = 6.0f;
        float morteWidth = 64.0f * morteScale;
        float morteHeight = 64.0f * morteScale;
        
        // Position on left side at 25% from left, vertically centered
        Gnosis::GNVector2 mortePosition = CenterObjectAtPosition(
            m_cachedScreenWidth * 0.25f, 
            m_cachedScreenHeight * 0.5f, 
            morteWidth, 
            morteHeight
        );
        
        Transform morteTransform(mortePosition, 0.0f, Gnosis::GNVector2(morteScale, morteScale));
        m_ecsSystem->AddComponent<Transform>(m_morteEntity, morteTransform);
        
        Sprite morteSprite("FloppyTurdMorte", 64, 64);
        morteSprite.layer = 102;
        morteSprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_morteEntity, morteSprite);
    }
    
    // 3. Death message (top center)
    m_deathMessageEntity = m_ecsSystem->CreateEntity();
    if (m_deathMessageEntity != Gnosis::INVALID_ENTITY) {
        float messageX = m_cachedScreenWidth * 0.5f;
        float messageY = m_cachedScreenHeight * 0.15f;
        Transform messageTransform(Gnosis::GNVector2(messageX, messageY), 0.0f, 
                                  Gnosis::GNVector2(6.0f, 6.0f));
        m_ecsSystem->AddComponent<Transform>(m_deathMessageEntity, messageTransform);
        
        UIElement messageUI;
        messageUI.buttonText = GetRandomDeathMessage();
        messageUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        messageUI.visible = true;
        messageUI.isEnabled = true;
        messageUI.textLayer = 104;
        messageUI.fontSize = 60.0f;
        messageUI.centerTextHorizontally = true;
        messageUI.centerTextVertically = true;
        messageUI.textOutlineWidth = 6.0f;
        messageUI.normalTextureId = "";
        m_ecsSystem->AddComponent<UIElement>(m_deathMessageEntity, messageUI);
    }
    
    // 4. Score display background (RIGHT side, upper area)
    m_gameOverScoreEntity = m_ecsSystem->CreateEntity();
    if (m_gameOverScoreEntity != Gnosis::INVALID_ENTITY) {
        float scoreScale = (m_cachedScreenWidth * 0.35f) / 64.0f;
        float scoreWidth = 64.0f * scoreScale;
        float scoreHeight = 32.0f * scoreScale;
        
        Gnosis::GNVector2 scorePosition = CenterObjectAtPosition(
            m_cachedScreenWidth * 0.70f, 
            m_cachedScreenHeight * 0.30f, 
            scoreWidth, 
            scoreHeight
        );
        
        Transform scoreTransform(scorePosition, 0.0f, Gnosis::GNVector2(scoreScale, scoreScale));
        m_ecsSystem->AddComponent<Transform>(m_gameOverScoreEntity, scoreTransform);
        
        UIElement scoreUI;
        scoreUI.buttonText = "";
        scoreUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        scoreUI.visible = true;
        scoreUI.isEnabled = true;
        scoreUI.textLayer = 101;
        scoreUI.normalTextureId = "GameOverScore";
        scoreUI.fontSize = 60.0f;
        scoreUI.centerTextHorizontally = true;
        scoreUI.centerTextVertically = true;
        m_ecsSystem->AddComponent<UIElement>(m_gameOverScoreEntity, scoreUI);
    }
    
    // 5. Pipes label (RIGHT side, above scoreboard)
    m_pipesLabelEntity = m_ecsSystem->CreateEntity();
    if (m_pipesLabelEntity != Gnosis::INVALID_ENTITY) {
        float pipesX = m_cachedScreenWidth * 0.70f;
        float pipesY = m_cachedScreenHeight * 0.30f - 60.0f;
        Transform pipesTransform(Gnosis::GNVector2(pipesX, pipesY), 0.0f, 
                                Gnosis::GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_pipesLabelEntity, pipesTransform);
        
        UIElement pipesUI;
        pipesUI.buttonText = "Pipes: " + std::to_string(m_pipesCleared);
        pipesUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        pipesUI.visible = true;
        pipesUI.isEnabled = true;
        pipesUI.textLayer = 102;
        pipesUI.fontSize = 60.0f;
        pipesUI.centerTextHorizontally = true;
        pipesUI.textOutlineWidth = 6.0f;
        pipesUI.normalTextureId = "";
        m_ecsSystem->AddComponent<UIElement>(m_pipesLabelEntity, pipesUI);
    }
    
    // 6. Coins label (RIGHT side, below scoreboard)
    m_coinsLabelEntity = m_ecsSystem->CreateEntity();
    if (m_coinsLabelEntity != Gnosis::INVALID_ENTITY) {
        PlayerComponent* player = m_ecsSystem->GetComponent<PlayerComponent>(m_playerEntity);
        int totalCoins = player ? player->sessionCoins : 0;
        
        float coinsX = m_cachedScreenWidth * 0.70f;
        float coinsY = m_cachedScreenHeight * 0.30f + 40.0f;
        Transform coinsTransform(Gnosis::GNVector2(coinsX, coinsY), 0.0f, 
                                Gnosis::GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_coinsLabelEntity, coinsTransform);
        
        UIElement coinsUI;
        coinsUI.buttonText = "Coins: " + std::to_string(totalCoins);
        coinsUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        coinsUI.visible = true;
        coinsUI.isEnabled = true;
        coinsUI.textLayer = 102;
        coinsUI.fontSize = 60.0f;
        coinsUI.centerTextHorizontally = true;
        coinsUI.textOutlineWidth = 6.0f;
        coinsUI.normalTextureId = "";
        m_ecsSystem->AddComponent<UIElement>(m_coinsLabelEntity, coinsUI);
    }
    
    // 7. Try Again button (RIGHT side, lower area - matching pause menu size)
    m_tryAgainButtonEntity = m_ecsSystem->CreateEntity();
    if (m_tryAgainButtonEntity != Gnosis::INVALID_ENTITY) {
        // Use same button scale as landscape pause menu
        float buttonScale = 5.0f; // Same as landscape pause menu buttons
        float buttonWidth = 64.0f * buttonScale;
        float buttonHeight = 32.0f * buttonScale;
        
        Gnosis::GNVector2 tryAgainPosition = CenterObjectAtPosition(
            m_cachedScreenWidth * 0.70f, 
            m_cachedScreenHeight * 0.65f, 
            buttonWidth, 
            buttonHeight
        );
        
        Transform tryAgainTransform(tryAgainPosition, 0.0f, 
                                    Gnosis::GNVector2(buttonScale, buttonScale));
        m_ecsSystem->AddComponent<Transform>(m_tryAgainButtonEntity, tryAgainTransform);
        
        UIElement tryAgainUI;
        tryAgainUI.buttonText = "Try Again";
        tryAgainUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        tryAgainUI.visible = true;
        tryAgainUI.isEnabled = true;
        tryAgainUI.isButton = true;
        tryAgainUI.textLayer = 103;
        tryAgainUI.normalTextureId = "ButtonBlue";
        tryAgainUI.hoverTextureId = "ButtonBlueHover";
        tryAgainUI.pressedTextureId = "ButtonBluePressed";
        tryAgainUI.fontSize = 60.0f;
        tryAgainUI.centerTextHorizontally = true;
        tryAgainUI.centerTextVertically = true;
        m_ecsSystem->AddComponent<UIElement>(m_tryAgainButtonEntity, tryAgainUI);
        
        Hitbox buttonHitbox(ColliderType::Rectangle, buttonWidth, buttonHeight);
        m_ecsSystem->AddComponent<Hitbox>(m_tryAgainButtonEntity, buttonHitbox);
    }
    
    // 8. Quit button (RIGHT side, bottom - matching pause menu size)
    m_quitButtonEntity = m_ecsSystem->CreateEntity();
    if (m_quitButtonEntity != Gnosis::INVALID_ENTITY) {
        float buttonScale = 5.0f; // Same as landscape pause menu buttons
        float buttonWidth = 64.0f * buttonScale;
        float buttonHeight = 32.0f * buttonScale;
        
        Gnosis::GNVector2 quitPosition = CenterObjectAtPosition(
            m_cachedScreenWidth * 0.70f, 
            m_cachedScreenHeight * 0.80f, 
            buttonWidth, 
            buttonHeight
        );
        
        Transform quitTransform(quitPosition, 0.0f, 
                               Gnosis::GNVector2(buttonScale, buttonScale));
        m_ecsSystem->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
        
        UIElement quitUI;
        quitUI.buttonText = "Quit";
        quitUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
        quitUI.visible = true;
        quitUI.isEnabled = true;
        quitUI.isButton = true;
        quitUI.textLayer = 103;
        quitUI.normalTextureId = "ButtonRed";
        quitUI.hoverTextureId = "ButtonRedHover";
        quitUI.pressedTextureId = "ButtonRedPressed";
        quitUI.fontSize = 60.0f;
        quitUI.centerTextHorizontally = true;
        quitUI.centerTextVertically = true;
        m_ecsSystem->AddComponent<UIElement>(m_quitButtonEntity, quitUI);
        
        Hitbox buttonHitbox(ColliderType::Rectangle, buttonWidth, buttonHeight);
        m_ecsSystem->AddComponent<Hitbox>(m_quitButtonEntity, buttonHitbox);
    }
}
```

### Phase 4: Preserve Portrait Layout
**Inside the `else` block**, move ALL the existing code (currently in CreateGameOverUI):

```cpp
else {
    // PORTRAIT LAYOUT (existing code - keep as-is)
    
    // Move the entire existing implementation here:
    // - Background creation
    // - Morte sprite
    // - Score display
    // - Pipes label
    // - Coins label
    // - Death message
    // - Try Again button
    // - Quit button
    
    // DO NOT MODIFY - just move it into the else block
}
```

---

## 🔍 Testing Checklist

After implementation, test the following:

### Portrait Mode (iOS Device Held Vertically):
- [ ] Background displays correctly
- [ ] Dead turd sprite is centered above scoreboard
- [ ] Death message shows at top
- [ ] Scoreboard shows pipes and coins
- [ ] Try Again button is centered and clickable
- [ ] Quit button is centered and clickable
- [ ] All text is readable

### Landscape Mode (iOS Device Held Horizontally):
- [ ] Background displays correctly
- [ ] Dead turd sprite is on LEFT side, vertically centered
- [ ] Death message shows at top center
- [ ] Scoreboard is on RIGHT side with pipes/coins
- [ ] Try Again button is on RIGHT side, properly sized
- [ ] Quit button is on RIGHT side below Try Again
- [ ] All buttons are clickable
- [ ] All text is readable (60pt font)
- [ ] Layout doesn't overlap or clip

---

## 📐 Layout Reference

### Landscape Layout (2556 x 1179 - iPhone in landscape):
```
┌──────────────────────────────────────────────────────────────┐
│              [Death Message - centered at 15%]               │
│                                                               │
│   ┌────────────┐                  ┌─────────────────┐       │
│   │            │                  │  Pipes: 42      │       │
│   │   Dead     │                  │                 │       │
│   │   Turd     │                  │  ┌───────────┐  │       │
│   │   Morte    │                  │  │Scoreboard │  │       │
│   │            │                  │  └───────────┘  │       │
│   │  (6.0x)    │                  │                 │       │
│   │            │                  │  Coins: 15      │       │
│   │            │                  │                 │       │
│   └────────────┘                  │  ┌───────────┐  │       │
│    25% from L                     │  │Try Again  │  │       │
│    Centered V                     │  └───────────┘  │       │
│                                   │                 │       │
│                                   │  ┌───────────┐  │       │
│                                   │  │   Quit    │  │       │
│                                   │  └───────────┘  │       │
│                                   └─────────────────┘       │
│                                    70% from L                │
└──────────────────────────────────────────────────────────────┘
```

### Portrait Layout (1179 x 2556 - iPhone normal):
```
┌─────────────────────────┐
│  [Death Message 25%]    │
│                         │
│     ┌────────────┐      │
│     │   Morte    │      │
│     │   Sprite   │      │
│     │   (6.0x)   │      │
│     └────────────┘      │
│      30% from top       │
│                         │
│    Pipes: 42            │
│   ┌──────────────┐      │
│   │  Scoreboard  │      │
│   └──────────────┘      │
│    Coins: 15            │
│      60% from top       │
│                         │
│   ┌──────────────┐      │
│   │  Try Again   │      │
│   └──────────────┘      │
│                         │
│   ┌──────────────┐      │
│   │    Quit      │      │
│   └──────────────┘      │
└─────────────────────────┘
```

---

## 🚨 Common Pitfalls to Avoid

1. **Don't modify portrait layout** - It's working fine, just move it into the `else` block
2. **Use CenterObjectAtPosition()** - Don't manually calculate centers, use the helper
3. **Match pause menu button scale** - Use 5.0f for landscape buttons (same as pause menu)
4. **Use smaller fonts in landscape** - 60.0f instead of 80.0f to fit more content
5. **Test both orientations** - Device rotation should switch layouts automatically

---

## 📝 Additional Context

**Related Files** (for reference):
- `src/FloppyTurd/Systems/PauseSystem.cpp` - Landscape pause menu buttons (line ~200+)
- `src/FloppyTurd/States/GameplayState.cpp` - `IsLandscapeMode()` helper (line ~180)
- `src/FloppyTurd/States/GameplayState.cpp` - `CenterObjectAtPosition()` helper (line ~3100)

**Boss Battle Features** (already completed):
- Player projectiles hit Rat King ✅
- Boss health bar has smooth damage fade ✅
- Player only hurt by boss projectiles (not boss bounds) ✅

**This Implementation Completes**:
The final piece of the boss battle UI - a proper landscape game over screen that matches the quality of the landscape pause menu.

---

**Implementation Time Estimate**: 15-20 minutes
**Difficulty**: Medium (mostly copy-paste with coordinate adjustments)
**Risk Level**: Low (portrait layout unchanged, landscape is additive)

Good luck! 🚀