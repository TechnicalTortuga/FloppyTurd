# Landscape UI Analysis and Implementation Plan

## Current UI Elements and Locations

### GameplayState UI Elements

#### 1. Pipe Counter
- **Location**: `GameplayState.cpp` lines 1163-1216
- **Position**: Centered horizontally, 10% from top (`screenW * 0.50f`, `screenH * 0.10f`)
- **Texture**: Text-only (no background)
- **Size**: Font size 120, outline width 18
- **Entity**: `m_pipeCounterEntity`

#### 2. Coin Bag Icon
- **Location**: `GameplayState.cpp` lines 1224-1244
- **Position**: 1% from left, 15% from bottom (`screenW * 0.01f`, `screenH * 0.85f`)
- **Texture**: `CoinBag.png` (32x32, scaled 8x to 256x256)
- **Entity**: `m_coinBagEntity`

#### 3. Coins Text
- **Location**: `GameplayState.cpp` lines 1246-1265
- **Position**: Right of coin bag (`iconX + (32.0f * bagScale) + 8.0f`, `iconY + (32.0f * bagScale * 0.5f) + 24.0f`)
- **Font**: Size 64, gold color
- **Entity**: `m_coinsTextEntity`

#### 4. Shooting Zone Visual
- **Location**: `GameplayState.cpp` lines 1268-1358
- **Position**: Above coin bag area (80% to 95% from top, 5% from right edge)
- **Size**: Calculated relative to coin bag position
- **Entity**: `m_shootingZoneEntity`
- **Components**: UIElement (visual) + UIShape (rectangle rendering)

#### 5. Settings Button (Pause Menu)
- **Location**: `GameplayState.cpp` lines 1801-1828
- **Position**: Top-right corner (85% from left, 5% from top)
- **Texture**: `settingsbutton.png`
- **Scale**: 8.0x
- **Entity**: `m_settingsButtonEntity`

#### 6. Heart UI System
- **Location**: `HeartSystem.cpp` lines 157-181 (CreateHeartUI)
- **Position**: Same X as coin bag, starting from menu button Y level
- **Textures**: Multiple heart states (`TurdHeart*.png`)
- **Container Entity**: `m_heartUIEntity`

### Pause Menu System Elements

#### 7. Pause Menu Background
- **Location**: `PauseSystem.cpp` lines 274-333
- **Position**: Centered on screen with 32px Y offset
- **Texture**: `PauseMenuBackgroundMobile.png` (160x300, scaled 7x)
- **Entity**: `m_pauseMenuBackgroundEntity`

#### 8. Ribbon Buttons
- **Location**: `PauseSystem.cpp` (CreateRibbonButtons method)
- **Texture**: `PauseMenuRibbonButton.png`
- **Position**: Along top of pause menu

### BOSS Level Specific Elements

#### 9. BOSS Background
- **Textures**: `BossLevelBackgroundMobile.png`, `BossLevelPillarMobile.png`
- **Position**: Full screen background layers
- **System**: `LevelManager.cpp` (background creation)

#### 10. Rat King (BOSS)
- **System**: `BossSystem.cpp`
- **Position**: Dynamic movement between `walkRangeMin` and `walkRangeMax`
- **Textures**: Multiple sprite sheets for different states

## Current Orientation Detection

### ScreenInfo Structure
- **Location**: `PlatformDelegates.h` lines 10-22
- **Fields**:
  - `pixelWidth/pixelHeight`: Actual screen dimensions
  - `logicalWidth/logicalHeight`: Logical coordinate space
  - `isPortrait`: Boolean orientation flag
  - `scaleFactor`: Pixel-to-logical ratio

### Orientation Detection
- **Location**: `MetalRenderer.swift` lines 1686-1744
- **Method**: Uses `UIDevice.current.orientation` and viewport dimensions
- **Landscape Swap**: Dimensions are swapped for landscape to ensure width > height

## Landscape/Portrait Separation Strategy

### 1. Orientation-Aware UI Positioning

#### Helper Functions to Implement:
```cpp
// In GameplayState.h
bool IsLandscapeMode() const { return !m_renderSystem->GetScreenInfo().isPortrait; }
void UpdateUILayoutForOrientation();

// In GameplayState.cpp
void GameplayState::UpdateUILayoutForOrientation() {
    if (IsLandscapeMode()) {
        // Landscape-specific positioning
        RepositionUIElementsLandscape();
    } else {
        // Portrait positioning (current implementation)
        RepositionUIElementsPortrait();
    }
}
```

#### Position Constants for Each Mode:
```cpp
// Portrait mode (current)
static constexpr float PORTRAIT_SETTINGS_X = 0.85f;  // 85% from left
static constexpr float PORTRAIT_SETTINGS_Y = 0.05f;  // 5% from top
static constexpr float PORTRAIT_COINBAG_X = 0.01f;   // 1% from left
static constexpr float PORTRAIT_COINBAG_Y = 0.85f;   // 15% from bottom
static constexpr float PORTRAIT_PIPE_Y = 0.10f;      // 10% from top

// Landscape mode (new)
static constexpr float LANDSCAPE_SETTINGS_X = 0.90f; // Further right
static constexpr float LANDSCAPE_SETTINGS_Y = 0.08f; // Slightly lower
static constexpr float LANDSCAPE_COINBAG_X = 0.10f;  // 10% from left (user requirement)
static constexpr float LANDSCAPE_COINBAG_Y = 0.95f;  // 5% from bottom (user requirement)
static constexpr float LANDSCAPE_PIPE_Y = 0.12f;     // Slightly lower
static constexpr float LANDSCAPE_PLAYER_OFFSET_X = 100.0f; // Further into screen
```

### 2. Texture Selection Based on Orientation

#### Pause Menu Background:
```cpp
// In PauseSystem::CreatePauseMenuBackground()
std::string bgTextureId = IsLandscapeMode() ?
    "PauseMenuBackgroundLandscape" : "PauseMenuBackgroundMobile";
```

#### Button Textures:
```cpp
// In PauseSystem::CreateRibbonButtons()
std::string buttonTextureId = IsLandscapeMode() ?
    "FloppyButtonBlue" : "PauseMenuRibbonButton";
```

### 3. Shooting Bar Input Handling

#### Current Issue:
- Shooting zone exists but input handling may not work properly in landscape
- Need to verify touch coordinates are properly mapped

#### Solution:
```cpp
// In PlayerControllerSystem::HandleInput()
if (IsLandscapeMode()) {
    // Adjust touch coordinates for landscape orientation
    float adjustedTouchX = landscapeTouchX;
    float adjustedTouchY = landscapeTouchY;
    // Apply landscape-specific shooting zone logic
}
```

### 4. Background Texture Stretching

#### Current Implementation:
- Backgrounds use parallax components (except BOSS level)
- BOSS level has static background

#### Landscape Adjustments:
```cpp
// In LevelManager::CreateBackgroundLayer()
if (IsLandscapeMode()) {
    // Ensure textures stretch to full screen dimensions
    scaledWidth = screenInfo.pixelWidth;
    scaledHeight = screenInfo.pixelHeight;
}
```

### 5. Player Position Adjustments

#### Current Position:
- Fixed at X=400, Y=639 (portrait coordinates)

#### Landscape Position:
```cpp
// In PlayerControllerSystem::ResetPlayerPosition()
if (IsLandscapeMode()) {
    playerTransform->position = GNVector2(400.0f + LANDSCAPE_PLAYER_OFFSET_X, 639.0f);
}
```

## Implementation Order

1. **Phase 1**: Add orientation detection helpers and constants
2. **Phase 2**: Implement UI repositioning functions
3. **Phase 3**: Add texture selection based on orientation
4. **Phase 4**: Fix shooting bar input for landscape
5. **Phase 5**: Adjust background stretching
6. **Phase 6**: Test and refine positioning

## Files to Modify

### Core Gameplay Files:
- `GameplayState.h/cpp`: Add orientation helpers, repositioning logic
- `PlayerControllerSystem.h/cpp`: Player position and shooting input
- `PauseSystem.h/cpp`: Background and button texture selection
- `HeartSystem.h/cpp`: Heart UI positioning
- `LevelManager.h/cpp`: Background stretching

### Configuration Files:
- Add landscape-specific texture assets
- Update asset references in code

## Testing Strategy

1. **Orientation Change Testing**: Verify UI repositions when device rotates
2. **Input Testing**: Ensure touch areas work correctly in landscape
3. **Visual Testing**: Check that all elements fit properly on screen
4. **Performance Testing**: Monitor for orientation change performance impact

## Asset Requirements

### New Textures Needed:
- `PauseMenuBackgroundLandscape.png`: Landscape version of pause menu background
- Landscape-optimized button textures (if needed)

### Existing Assets to Reuse:
- `FloppyButtonBlue.png`: For landscape button replacement
- `BossLevelBackgroundMobile.png`: May work for landscape or needs landscape version

---

## Implementation Status

- [x] Document all UI elements and current locations
- [x] Analyze current orientation handling
- [ ] Add orientation detection helpers
- [ ] Implement landscape UI positioning
- [ ] Add texture selection logic
- [ ] Fix shooting bar input
- [ ] Adjust background stretching
- [ ] Test orientation changes
