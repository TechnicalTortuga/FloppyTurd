# Leaderboard State Fixes Summary

## Status: ✅ COMPLETED - Build Successful

---

## Issues Identified
1. **No backgrounds showing** - Using incorrect texture names and not properly scaled
2. **Everything stuck in top-left** - Not using proper centering like PauseSystem and MainMenuState
3. **Music stops** - Exit() should not stop music to let it continue in main menu
4. **No working back button** - Using non-existent "BackButton" texture instead of FloppyButtonBlue
5. **Wrong level names** - Using made-up names instead of actual level names from LevelConfig

## Correct Level Names (from LevelConfig.cpp)
- Level 1: "A Flop in the Park"
- Level 2: "Home Sweet Home" (Sewer theme)
- Level 3: "The Good, The Bad, and the Stinky" (Desert theme)
- Level 4: "Polar Pandemonium" (Snow theme)
- Level 5: "Dung in the Dungeon" (Castle theme)
- Level 6: "Curtains for Crap" (Boss level)

## Key Changes Needed

### 1. Update LeaderboardState.h

Add overlay position tracking variables:
```cpp
// Overlay position tracking (for positioning UI within the centered overlay)
float m_overlayX;
float m_overlayY;
float m_overlayWidth;
float m_overlayHeight;
```

Update enum names to match actual level themes:
```cpp
enum class LeaderboardPage {
    LEVEL_1_PARK = 0,           // "A Flop in the Park"
    LEVEL_2_SEWER = 1,          // "Home Sweet Home"
    LEVEL_3_DESERT = 2,         // "The Good, The Bad, and the Stinky"
    LEVEL_4_SNOW = 3,           // "Polar Pandemonium"
    LEVEL_5_CASTLE = 4,         // "Dung in the Dungeon"
    LEVEL_6_BOSS = 5,           // "Curtains for Crap"
    TOTAL_ENEMIES = 6,
    TOTAL_COINS = 7,
    TOTAL_PIPES = 8,
    COUNT = 9
};
```

### 2. Update Enter() Function

Use pixel dimensions instead of logical dimensions:
```cpp
void LeaderboardState::Enter() {
    GN_LOG_INFO("Entering Leaderboard State");
    m_finished = false;

    // Get enhanced screen info using pixel dimensions
    ScreenInfo screenInfo;
    if (m_platformDelegates && m_platformDelegates->renderer.getScreenInfo) {
        m_platformDelegates->renderer.getScreenInfo(&screenInfo);
        m_screenWidth = screenInfo.pixelWidth;   // Use pixel dimensions!
        m_screenHeight = screenInfo.pixelHeight; // Use pixel dimensions!
        GN_LOG_INFO("LeaderboardState: Screen info - pixel: " + 
                   std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight));
    } else if (m_platformDelegates && m_platformDelegates->renderer.getScreenSize) {
        m_platformDelegates->renderer.getScreenSize(&m_screenWidth, &m_screenHeight);
    } else {
        // Default to iPhone 16 pixel dimensions
        m_screenWidth = 1179.0f;
        m_screenHeight = 2556.0f;
    }

    // NOTE: We do NOT start music here - let it continue from main menu
    CreateUI();
    m_initialized = true;
}
```

### 3. Update Exit() Function

Remove music stop:
```cpp
void LeaderboardState::Exit() {
    GN_LOG_INFO("Exiting Leaderboard State");
    
    // Clean up all entities...
    
    // NOTE: We do NOT stop music here - let it continue playing in main menu
    m_initialized = false;
}
```

### 4. Update CreateBackground() Function

Use proper scaling like PauseSystem and MainMenuState:
```cpp
void LeaderboardState::CreateBackground() {
    GN_LOG_INFO("LeaderboardState: Creating backgrounds");

    // 1. Create full-screen main menu background (like MainMenuState)
    m_backgroundEntity = m_ecsSystem->CreateEntity();
    
    // Use MainMenuMobile for mobile platforms
    std::string bgTexture = IsMobilePlatform() ? "MainMenuMobile" : "MainMenu";
    
    // Background positioned at (0,0) and scaled to fill screen
    Transform bgTransform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_backgroundEntity, bgTransform);

    Sprite bgSprite;
    bgSprite.textureId = bgTexture;
    bgSprite.width = static_cast<int>(m_screenWidth);
    bgSprite.height = static_cast<int>(m_screenHeight);
    bgSprite.layer = 0;
    bgSprite.visible = true;
    m_ecsSystem->AddComponent<Sprite>(m_backgroundEntity, bgSprite);

    // 2. Create centered pause menu overlay background (like PauseSystem)
    // PauseMenuBackgroundMobile is 160x300 and uses 7.0f scale
    m_overlayBackgroundEntity = m_ecsSystem->CreateEntity();
    
    float bgTextureWidth = 160.0f;
    float bgTextureHeight = 300.0f;
    float bgScale = 7.0f;
    
    // Calculate final rendered dimensions
    float scaledWidth = bgTextureWidth * bgScale;   // 160 * 7 = 1120
    float scaledHeight = bgTextureHeight * bgScale; // 300 * 7 = 2100
    
    // Center on screen
    float centerX = m_screenWidth * 0.5f;
    float centerY = m_screenHeight * 0.5f;
    
    // Calculate top-left position for centered overlay
    float bgX = centerX - (scaledWidth * 0.5f);
    float bgY = centerY - (scaledHeight * 0.5f);
    
    Transform overlayTransform(GNVector2(bgX, bgY), 0.0f, GNVector2(bgScale, bgScale));
    m_ecsSystem->AddComponent<Transform>(m_overlayBackgroundEntity, overlayTransform);

    Sprite overlaySprite;
    overlaySprite.textureId = "PauseMenuBackgroundMobile";
    overlaySprite.width = static_cast<int>(bgTextureWidth);
    overlaySprite.height = static_cast<int>(bgTextureHeight);
    overlaySprite.layer = 5;
    overlaySprite.visible = true;
    m_ecsSystem->AddComponent<Sprite>(m_overlayBackgroundEntity, overlaySprite);
    
    // Store overlay bounds for positioning UI elements within it
    m_overlayX = bgX;
    m_overlayY = bgY;
    m_overlayWidth = scaledWidth;
    m_overlayHeight = scaledHeight;
}
```

### 5. Update CreateNavigationButtons() Function

Position buttons within the centered overlay and use FloppyButtonBlue for back button:
```cpp
void LeaderboardState::CreateNavigationButtons() {
    GN_LOG_INFO("LeaderboardState: Creating navigation buttons");

    // Calculate positions within the overlay
    float overlayLeft = m_overlayX;
    float overlayRight = m_overlayX + m_overlayWidth;
    float overlayTop = m_overlayY;
    float overlayBottom = m_overlayY + m_overlayHeight;
    float overlayCenterY = m_overlayY + (m_overlayHeight * 0.5f);
    float overlayCenterX = m_overlayX + (m_overlayWidth * 0.5f);

    // Arrow button scale (larger for mobile)
    float arrowScale = IsMobilePlatform() ? 6.0f : 2.0f;
    float arrowWidth = 32.0f * arrowScale;
    float arrowHeight = 32.0f * arrowScale;

    // Left arrow - positioned on left side of overlay, vertically centered
    m_leftArrowEntity = m_ecsSystem->CreateEntity();
    float leftArrowX = overlayLeft + 40.0f;
    float leftArrowY = overlayCenterY - (arrowHeight * 0.5f);
    
    Transform leftTransform(GNVector2(leftArrowX, leftArrowY), 0.0f, GNVector2(arrowScale, arrowScale));
    m_ecsSystem->AddComponent<Transform>(m_leftArrowEntity, leftTransform);

    Sprite leftSprite;
    leftSprite.textureId = "LeftArrow";
    leftSprite.width = 32;
    leftSprite.height = 32;
    leftSprite.layer = 15;
    leftSprite.visible = true;
    m_ecsSystem->AddComponent<Sprite>(m_leftArrowEntity, leftSprite);

    // Right arrow - positioned on right side of overlay, vertically centered
    m_rightArrowEntity = m_ecsSystem->CreateEntity();
    float rightArrowX = overlayRight - arrowWidth - 40.0f;
    float rightArrowY = overlayCenterY - (arrowHeight * 0.5f);
    
    Transform rightTransform(GNVector2(rightArrowX, rightArrowY), 0.0f, GNVector2(arrowScale, arrowScale));
    m_ecsSystem->AddComponent<Transform>(m_rightArrowEntity, rightTransform);

    Sprite rightSprite;
    rightSprite.textureId = "RightArrow";
    rightSprite.width = 32;
    rightSprite.height = 32;
    rightSprite.layer = 15;
    rightSprite.visible = true;
    m_ecsSystem->AddComponent<Sprite>(m_rightArrowEntity, rightSprite);

    // Back button - positioned at bottom center of overlay using FloppyButtonBlue
    m_backButtonEntity = m_ecsSystem->CreateEntity();
    
    float backButtonScale = IsMobilePlatform() ? 10.0f : 4.0f;
    float backButtonTexWidth = 90.0f;
    float backButtonTexHeight = 16.0f;
    float backButtonWidth = backButtonTexWidth * backButtonScale;
    float backButtonHeight = backButtonTexHeight * backButtonScale;
    
    float backButtonX = overlayCenterX - (backButtonWidth * 0.5f);
    float backButtonY = overlayBottom - backButtonHeight - 80.0f;
    
    Transform backTransform(GNVector2(backButtonX, backButtonY), 0.0f, GNVector2(backButtonScale, backButtonScale));
    m_ecsSystem->AddComponent<Transform>(m_backButtonEntity, backTransform);

    Sprite backSprite("FloppyButtonBlue", backButtonTexWidth, backButtonTexHeight);
    backSprite.layer = 15;
    backSprite.visible = true;
    m_ecsSystem->AddComponent<Sprite>(m_backButtonEntity, backSprite);
    
    UIElement backUI("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
    backUI.fontSize = IsMobilePlatform() ? 88.0f : 21.0f;
    backUI.textColor = GNColor(255, 255, 255, 255);
    backUI.centerTextHorizontally = true;
    backUI.centerTextVertically = true;
    backUI.visible = true;
    m_ecsSystem->AddComponent<UIElement>(m_backButtonEntity, backUI);
}
```

### 6. Update CreatePageContent() Function

Position content within the overlay:
```cpp
void LeaderboardState::CreatePageContent() {
    DestroyPageContent();
    
    // Calculate positions within the overlay
    float overlayCenterX = m_overlayX + (m_overlayWidth * 0.5f);
    float overlayTop = m_overlayY;
    
    // Title at top of overlay
    m_titleEntity = m_ecsSystem->CreateEntity();
    float titleY = overlayTop + 150.0f;
    
    Transform titleTransform(GNVector2(overlayCenterX, titleY), 0.0f, GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_titleEntity, titleTransform);

    UIElement titleUI;
    titleUI.buttonText = "LEADERBOARDS";
    titleUI.fontSize = IsMobilePlatform() ? 72.0f : 48.0f;
    titleUI.textColor = GNColor(255, 255, 255, 255);
    titleUI.centerTextHorizontally = true;
    titleUI.visible = true;
    titleUI.textLayer = 10;
    m_ecsSystem->AddComponent<UIElement>(m_titleEntity, titleUI);

    // Page title (level name)
    m_pageTitleEntity = m_ecsSystem->CreateEntity();
    float pageTitleY = titleY + 200.0f;
    
    Transform pageTitleTransform(GNVector2(overlayCenterX, pageTitleY), 0.0f, GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_pageTitleEntity, pageTitleTransform);

    UIElement pageTitleUI;
    pageTitleUI.buttonText = GetPageTitle(m_currentPage);
    pageTitleUI.fontSize = IsMobilePlatform() ? 56.0f : 36.0f;
    pageTitleUI.textColor = GNColor(255, 215, 0, 255); // Gold
    pageTitleUI.centerTextHorizontally = true;
    pageTitleUI.visible = true;
    pageTitleUI.textLayer = 12;
    m_ecsSystem->AddComponent<UIElement>(m_pageTitleEntity, pageTitleUI);

    // Local score display
    m_localScoreEntity = m_ecsSystem->CreateEntity();
    float scoreY = pageTitleY + 150.0f;
    
    Transform scoreTransform(GNVector2(overlayCenterX, scoreY), 0.0f, GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_localScoreEntity, scoreTransform);

    UIElement scoreUI;
    scoreUI.buttonText = GetLocalScoreText(m_currentPage);
    scoreUI.fontSize = IsMobilePlatform() ? 48.0f : 32.0f;
    scoreUI.textColor = GNColor(255, 255, 255, 255);
    scoreUI.centerTextHorizontally = true;
    scoreUI.visible = true;
    scoreUI.textLayer = 12;
    m_ecsSystem->AddComponent<UIElement>(m_localScoreEntity, scoreUI);
}
```

### 7. Update GetPageTitle() Function

Use correct level names:
```cpp
std::string LeaderboardState::GetPageTitle(LeaderboardPage page) const {
    switch (page) {
        case LeaderboardPage::LEVEL_1_PARK:
            return "A Flop in the Park";
        case LeaderboardPage::LEVEL_2_SEWER:
            return "Home Sweet Home";
        case LeaderboardPage::LEVEL_3_DESERT:
            return "The Good, The Bad, and the Stinky";
        case LeaderboardPage::LEVEL_4_SNOW:
            return "Polar Pandemonium";
        case LeaderboardPage::LEVEL_5_CASTLE:
            return "Dung in the Dungeon";
        case LeaderboardPage::LEVEL_6_BOSS:
            return "Curtains for Crap";
        case LeaderboardPage::TOTAL_ENEMIES:
            return "Total Enemies Defeated";
        case LeaderboardPage::TOTAL_COINS:
            return "Total Coins Collected";
        case LeaderboardPage::TOTAL_PIPES:
            return "Total Pipes Cleared";
        default:
            return "Leaderboard";
    }
}
```

### 8. Update HandleInput() Function

Fix button hit detection to use top-left based positioning:
```cpp
// Check back button (top-left based positioning)
if (m_backButtonEntity != 0) {
    auto* transform = m_ecsSystem->GetComponent<Transform>(m_backButtonEntity);
    auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_backButtonEntity);
    
    if (transform && sprite) {
        float btnX = transform->position.x;  // Top-left based
        float btnY = transform->position.y;  // Top-left based
        float btnW = sprite->width * transform->scale.x;
        float btnH = sprite->height * transform->scale.y;

        if (touchX >= btnX && touchX <= btnX + btnW &&
            touchY >= btnY && touchY <= btnY + btnH) {
            GN_LOG_INFO("✅ Back button clicked!");
            OnBackButtonPressed();
            return;
        }
    }
}
```

## Summary

The main issues were:
1. Not using pixel dimensions from ScreenInfo
2. Not centering the PauseMenuBackgroundMobile overlay properly (160x300 at 7.0f scale)
3. Not positioning UI elements within the centered overlay bounds
4. Using wrong texture names and level names
5. Not using FloppyButtonBlue with UIElement for the back button
6. Stopping music on exit (should continue playing)

The fixes follow the same patterns as PauseSystem (for the overlay background) and MainMenuState (for full-screen background and button creation).

---

## Implementation Complete

All fixes have been successfully implemented and tested:
- ✅ Backgrounds properly displayed and centered
- ✅ UI elements positioned within portrait frame overlay
- ✅ Music continues playing between main menu and leaderboard
- ✅ Back button functional with FloppyButtonBlue texture
- ✅ All level names corrected to match LevelConfig
- ✅ Enum names updated (SEWER, DESERT, SNOW, CASTLE)
- ✅ Build successful on iOS Simulator (iPhone 16)

**Build Command:**
```bash
xcodebuild -project build_ios/FloppyTurd.xcodeproj -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" clean build
```

**Result:** BUILD SUCCESS