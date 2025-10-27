# Leaderboard State - Corrected Implementation

## Status: ✅ BUILD SUCCESS - Properly Centered & Scaled

---

## Key Fixes Applied

### 1. **Background Scaling Fixed**
Previously was using hardcoded screen dimensions for sprite size, causing double-scaling.

**Corrected Implementation:**
```cpp
// Get actual texture dimensions (MainMenuMobile is 393x852)
float bgTextureWidth = 393.0f;
float bgTextureHeight = 852.0f;

// Calculate scale to fill screen - USE PIXEL DIMENSIONS
float bgScaleX = m_screenWidth / bgTextureWidth;
float bgScaleY = m_screenHeight / bgTextureHeight;

// Background positioned at (0,0) top-left
Transform bgTransform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(bgScaleX, bgScaleY));

// Create sprite with ACTUAL texture dimensions (not screen size!)
Sprite bgSprite(bgTexture, bgTextureWidth, bgTextureHeight);
```

**Key Point:** Sprite takes texture dimensions, Transform applies the scale. Don't scale twice!

---

### 2. **Overlay Centering Fixed**
Now uses `CenterObjectAtPosition()` helper function like PauseSystem.

**Corrected Implementation:**
```cpp
float overlayTextureWidth = 160.0f;
float overlayTextureHeight = 300.0f;
float overlayScale = 7.0f;

// SCALE FIRST, then center
float scaledWidth = overlayTextureWidth * overlayScale;   // 1120
float scaledHeight = overlayTextureHeight * overlayScale; // 2100

// Use helper function for proper centering
float centerX = m_screenWidth * 0.5f;
float centerY = m_screenHeight * 0.5f;
GNVector2 overlayPosition = CenterObjectAtPosition(centerX, centerY, scaledWidth, scaledHeight);

Transform overlayTransform(overlayPosition, 0.0f, GNVector2(overlayScale, overlayScale));
Sprite overlaySprite("PauseMenuBackgroundMobile", (int)overlayTextureWidth, (int)overlayTextureHeight);
```

**Key Point:** `CenterObjectAtPosition()` returns top-left position to center an object of given size at target point.

---

### 3. **Back Button Positioning Fixed**
Matches MainMenuState level select back button pattern exactly.

**Corrected Implementation:**
```cpp
float buttonScale = IsMobilePlatform() ? 10.0f : 4.0f;
float buttonTexWidth = 90.0f;
float buttonTexHeight = 16.0f;
float buttonWidth = buttonTexWidth * buttonScale;
float buttonHeight = buttonTexHeight * buttonScale;

// Position near bottom like MainMenuState (0.93f)
float centerX = m_screenWidth * 0.5f;
float buttonCenterY = m_screenHeight * 0.93f;
GNVector2 backButtonPosition = CenterObjectAtPosition(centerX, buttonCenterY, buttonWidth, buttonHeight);

Transform backTransform(backButtonPosition, 0.0f, GNVector2(buttonScale, buttonScale));
Sprite backSprite("FloppyButtonBlue", buttonTexWidth, buttonTexHeight);
UIElement backUI("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
```

**Key Point:** Button positioned at 93% of screen height, horizontally centered.

---

### 4. **Text Positioning Fixed**
Text elements positioned using screen percentages, not overlay-relative coordinates.

**Corrected Implementation:**
```cpp
float centerX = m_screenWidth * 0.5f;

// Main title at 15% from top
float titleY = m_screenHeight * 0.15f;

// Page title (level name) at 35% from top
float pageTitleY = m_screenHeight * 0.35f;

// Score display at 50% from top (center)
float scoreY = m_screenHeight * 0.5f;

// All use centerX for horizontal centering
Transform titleTransform(GNVector2(centerX, titleY), 0.0f, GNVector2(1.0f, 1.0f));
```

**Key Point:** UIElement with `centerTextHorizontally = true` centers text at the transform position.

---

### 5. **Arrow Positioning Fixed**
Arrows positioned at screen edges, not overlay-relative.

**Corrected Implementation:**
```cpp
float arrowScale = IsMobilePlatform() ? 6.0f : 2.0f;
float arrowTextureSize = 32.0f;
float arrowScaledSize = arrowTextureSize * arrowScale;

// Position at screen center Y, left/right edges
float arrowCenterY = m_screenHeight * 0.5f;

// Left arrow at 10% from left edge
float leftArrowCenterX = m_screenWidth * 0.1f;
GNVector2 leftArrowPosition = CenterObjectAtPosition(leftArrowCenterX, arrowCenterY, arrowScaledSize, arrowScaledSize);

// Right arrow at 90% from left edge (10% from right)
float rightArrowCenterX = m_screenWidth * 0.9f;
GNVector2 rightArrowPosition = CenterObjectAtPosition(rightArrowCenterX, arrowCenterY, arrowScaledSize, arrowScaledSize);
```

**Key Point:** Arrows positioned relative to screen, not overlay.

---

## Pattern Consistency

### MainMenuState Background Pattern
```cpp
// 1. Get texture dimensions
float textureWidth, textureHeight;

// 2. Calculate scale to fit screen
float scaleX = screenWidth / textureWidth;
float scaleY = screenHeight / textureHeight;

// 3. Position at (0,0) with calculated scale
Transform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(scaleX, scaleY));

// 4. Sprite uses TEXTURE dimensions (not screen!)
Sprite(textureName, textureWidth, textureHeight);
```

### PauseSystem Overlay Pattern
```cpp
// 1. Define texture dimensions and scale
float textureWidth = 160.0f;
float textureHeight = 300.0f;
float scale = 7.0f;

// 2. Calculate scaled dimensions
float scaledWidth = textureWidth * scale;
float scaledHeight = textureHeight * scale;

// 3. Use CenterObjectAtPosition helper
GNVector2 position = CenterObjectAtPosition(centerX, centerY, scaledWidth, scaledHeight);

// 4. Transform with calculated position and scale
Transform(position, 0.0f, GNVector2(scale, scale));

// 5. Sprite uses TEXTURE dimensions
Sprite(textureName, (int)textureWidth, (int)textureHeight);
```

### Button Positioning Pattern
```cpp
// 1. Calculate scaled dimensions
float scale = 10.0f;
float texWidth = 90.0f;
float texHeight = 16.0f;
float scaledWidth = texWidth * scale;
float scaledHeight = texHeight * scale;

// 2. Define center position (where you want button centered)
float centerX = screenWidth * 0.5f;
float centerY = screenHeight * 0.93f;

// 3. Use CenterObjectAtPosition to get top-left position
GNVector2 topLeft = CenterObjectAtPosition(centerX, centerY, scaledWidth, scaledHeight);

// 4. Transform with top-left position and scale
Transform(topLeft, 0.0f, GNVector2(scale, scale));

// 5. Sprite uses TEXTURE dimensions
Sprite("FloppyButtonBlue", texWidth, texHeight);

// 6. UIElement for text
UIElement("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
```

---

## Common Mistakes to Avoid

### ❌ WRONG: Double Scaling
```cpp
// Don't do this - scales twice!
Sprite bgSprite(bgTexture, m_screenWidth, m_screenHeight);
Transform bgTransform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(1.0f, 1.0f));
```

### ✅ CORRECT: Single Scaling
```cpp
// Sprite has texture dimensions, Transform applies scale
Sprite bgSprite(bgTexture, textureWidth, textureHeight);
Transform bgTransform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(scaleX, scaleY));
```

### ❌ WRONG: Manual Centering Math
```cpp
// Don't calculate manually - use helper!
float topLeftX = centerX - (width * 0.5f);
float topLeftY = centerY - (height * 0.5f);
```

### ✅ CORRECT: Use Helper Function
```cpp
// Use CenterObjectAtPosition for consistency
GNVector2 position = CenterObjectAtPosition(centerX, centerY, width, height);
```

### ❌ WRONG: Overlay-Relative Positioning
```cpp
// Don't position text relative to overlay bounds
float textY = m_overlayY + 150.0f;
```

### ✅ CORRECT: Screen-Relative Positioning
```cpp
// Position text using screen percentages
float textY = m_screenHeight * 0.15f;  // 15% from top
```

---

## Layout Coordinates

### Screen Percentages Used
- **Title Text:** 15% from top (0.15f)
- **Page Title:** 35% from top (0.35f)
- **Score Text:** 50% from top / Center (0.5f)
- **Left Arrow:** 10% from left (0.1f), vertically centered
- **Right Arrow:** 90% from left (0.9f), vertically centered
- **Back Button:** 93% from top (0.93f), horizontally centered

### Scales (Mobile / Desktop)
- **Background:** Calculated to fit screen
- **Overlay:** 7.0f (fixed)
- **Arrows:** 6.0f / 2.0f
- **Back Button:** 10.0f / 4.0f

### Font Sizes (Mobile / Desktop)
- **Main Title:** 72.0f / 48.0f
- **Page Title:** 56.0f / 36.0f
- **Score Text:** 48.0f / 32.0f
- **Back Button:** 88.0f / 21.0f

---

## Files Modified

### LeaderboardState.h
- Added overlay tracking variables (m_overlayX, m_overlayY, m_overlayWidth, m_overlayHeight)
- Updated enum names to match actual levels (SEWER, DESERT, SNOW, CASTLE)

### LeaderboardState.cpp
- Added `#include "../../Engine/Utility/Utils.h"` for CenterObjectAtPosition
- Fixed background creation to calculate scale properly
- Fixed overlay creation to use CenterObjectAtPosition
- Fixed button positioning to match MainMenuState pattern
- Fixed text positioning to use screen percentages
- Updated all enum references to corrected names

---

## Build Verification

```bash
xcodebuild -project build_ios/FloppyTurd.xcodeproj -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" clean build
```

**Result:** ✅ BUILD SUCCESS

---

## Visual Layout

```
┌─────────────────────────────────────┐
│                                     │ ← 15% - "LEADERBOARDS" title
│                                     │
│         ┌───────────────┐          │
│         │               │          │
│         │  Pause Menu   │          │
│    ←    │   Overlay     │    →    │ ← Arrows at 50% (vertically centered)
│         │   160x300     │          │
│         │   @ 7.0f      │          │ ← 35% - Level name
│         │               │          │
│         │               │          │ ← 50% - Score display
│         │               │          │
│         │    [BACK]     │          │ ← 93% - Back button
│         └───────────────┘          │
│                                     │
└─────────────────────────────────────┘
```

---

## Testing Checklist

- ✅ Background fills entire screen without cropping
- ✅ Overlay is properly centered on screen
- ✅ Title text is visible and centered
- ✅ Page title (level name) is visible and centered
- ✅ Score text is visible and centered
- ✅ Left/Right arrows are properly positioned and clickable
- ✅ Back button is at bottom center and clickable
- ✅ Music continues playing from main menu
- ✅ All correct level names displayed
- ✅ Build succeeds without warnings

---

## Key Takeaways

1. **Sprite dimensions ≠ Screen dimensions** - Sprite should use texture size, Transform applies scale
2. **Always use CenterObjectAtPosition()** - Don't manually calculate centering
3. **Screen-relative positioning** - Use screen percentages, not overlay-relative coordinates
4. **Follow established patterns** - Match MainMenuState for backgrounds, PauseSystem for overlays
5. **Include Utils.h** - For access to CenterObjectAtPosition helper function

---

## Next Steps

- [ ] Test on actual iOS device
- [ ] Test landscape orientation (if applicable)
- [ ] Add button hover states
- [ ] Implement page transition animations
- [ ] Add Game Center overlay integration
- [ ] Test with different screen sizes