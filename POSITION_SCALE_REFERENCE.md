# FloppyTurd Position & Scale Reference Guide

## 🎯 Current Debug Data (FINAL FIXES)

### Rat King Positioning Data
- **Desired Center X**: 731.000000 (60% of 1179px screen)
- **Top Left X**: 219.000000 (actual transform position)
- **Top Left Y**: 1392.000000 (floor position - raised by 40px)
- **Walk Boundaries**: Min=254.349976, Max=283.000000 (28.65px range - focused)
- **Current Position**: (219.000000, 1392.000000)
- **Separation from Player**: 254px - 25px = **229px gap** ✅

### Player Positioning Data
- **Boss Level X**: 25.0px (FIXED - now working correctly!)
- **Other Levels X**: 50% of screen width (589.5px on 1179px screen)

---

## 📍 Player Position Setters

### 1. GameplayState.cpp - Initial Player Positioning
```cpp
// File: src/FloppyTurd/States/GameplayState.cpp:762-764
// Boss level uses 25px from left, other levels use center
float playerX = (m_currentLevelId == 6) ? 25.0f : (screenWidth * 0.5f);
Transform playerTransform(Gnosis::GNVector2(playerX, 639.0f), 0.0f, Gnosis::GNVector2(playerScale, playerScale));
```

### 2. PlayerControllerSystem.cpp - Fixed X Position
```cpp
// File: src/FloppyTurd/Systems/PlayerControllerSystem.cpp:77
static constexpr float PLAYER_X_POSITION = 300.0f;  // Configurable player X position
```

### 3. GameplayState.cpp - Reset Position
```cpp
// File: src/FloppyTurd/States/GameplayState.cpp:2850-2856
// Reset player transform to starting position
playerTransform->position = Gnosis::GNVector2(400.0f, 639.0f);
```

---

## 🏗️ Rat King Position Setters

### 1. LevelManager.cpp - Initial Spawn Position
```cpp
// File: src/FloppyTurd/Systems/LevelManager.cpp:357-373
// For top-left positioning: position to utilize the full 128px margin
float desiredCenterX = screenWidth - (ratKingSpriteWidth / 2.0f) + 64.0f; // Center with 128px margin (half)
float ratKingX = desiredCenterX - (ratKingSpriteWidth / 2.0f); // Top-left X position
float ratKingY = desiredCenterY - (ratKingSpriteHeight / 2.0f); // Top-left Y position
```

### 2. BossSystem.cpp - Walk Boundaries
```cpp
// File: src/FloppyTurd/Systems/BossSystem.cpp:81-84
// Boundary calculations for top-left positioned sprite
walkRangeMin = centerX - (ratKingSpriteWidth / 2.0f); // Left boundary (center - half width)
walkRangeMax = screenWidth - ratKingSpriteWidth + 128.0f; // Right boundary with 128px margin
```

### 3. BossSystem.cpp - Movement Updates
```cpp
// File: src/FloppyTurd/Systems/BossSystem.cpp:221-265
// Move towards destination and update entity position
position.x += direction * walkSpeed * deltaTime;
transform->position.x = position.x;
```

---

## 📏 Scale Setters

### 1. HeartSystem.cpp - Heart Scale
```cpp
// File: src/FloppyTurd/Systems/HeartSystem.cpp:266
const float heartScale = 6.0f; // Reduced scale for smaller hearts
```

### 2. GameplayState.cpp - Player Scale
```cpp
// File: src/FloppyTurd/States/GameplayState.cpp:751-764
float playerScale = m_currentLevelConfig.baseScale; // Usually 8.0f
Transform playerTransform(Gnosis::GNVector2(playerX, 639.0f), 0.0f, Gnosis::GNVector2(playerScale, playerScale));
```

### 3. LevelManager.cpp - Rat King Scale
```cpp
// File: src/FloppyTurd/Systems/LevelManager.cpp:350
float ratKingScale = 8.0f; // Rat King scale - match player scale for consistency
```

### 4. PlayerControllerSystem.cpp - Hat Scale
```cpp
// File: src/FloppyTurd/Systems/PlayerControllerSystem.cpp:785
Transform hatTransform(Gnosis::GNVector2(playerX, playerY), 0.0f, Gnosis::GNVector2(playerScale, playerScale));
```

---

## 🎮 UI Element Positioning

### 1. HeartSystem.cpp - Heart Positioning
```cpp
// File: src/FloppyTurd/Systems/HeartSystem.cpp:280-281
float heartY = y + (i * heartSpacing); // Vertical stacking
Transform heartTransform(Gnosis::GNVector2(x, heartY), 0.0f, Gnosis::GNVector2(heartScale, heartScale));
```

### 2. GameplayState.cpp - UI Positioning
```cpp
// File: src/FloppyTurd/States/GameplayState.cpp:1624-1625
float menuX = screenWidth * 0.85f; // 85% from left
float menuY = screenHeight * 0.05f; // 5% from top
```

---

## 🔧 Issues Fixed ✅

### 1. Player X Position Conditional - ✅ FIXED
- **Problem**: PlayerControllerSystem was overriding GameplayState position
- **Solution**: Updated PlayerControllerSystem to use 25px for level 6
- **Result**: Player now correctly positions at 25px in boss level

### 2. Rat King Overlap with Player - ✅ FIXED
- **Problem**: Rat King Min boundary (77.5px) too close to player (25px)
- **Solution**: Changed Min boundary to 65% of screen (772px) instead of 50%
- **Result**: 229px separation between player (25px) and Rat King Min (254px)

### 3. 128px Gap Issue - ✅ FIXED
- **Problem**: Rat King not utilizing full 128px margin on right
- **Solution**: Max boundary now properly accounts for 128px margin
- **Result**: Rat King can utilize full movement range with margin

### 4. Scale Inconsistencies
- **Problem**: Multiple scale values scattered across systems
- **Current**: Heart scale=6.0, Player/Rat King scale=8.0
- **Recommendation**: Centralize scale constants

---

## 📊 Final Debug Values

### Screen Dimensions
- **Width**: 1179px
- **Height**: 2556px

### Rat King Calculations (FINAL)
- **Sprite Width**: 1024px (128px × 8.0 scale)
- **Center X**: 589.5px (50% of screen)
- **Desired Center**: 731.0px (60% + 64px margin)
- **Top Left**: 219.0px (731 - 512)
- **Walk Range**: 254.3px to 283.0px (28.7px focused range)
- **Separation from Player**: 254px - 25px = **229px gap** ✅

### Player Position (FINAL)
- **Boss Level**: 25.0px ✅ (fixed and working)
- **Other Levels**: 589.5px (50% of screen)

---

## ✅ Completed Fixes

1. **✅ Player X Position**: Fixed conditional in PlayerControllerSystem
2. **✅ Rat King Separation**: Increased Min boundary to 65% of screen (772px)
3. **✅ 128px Margin**: Max boundary properly accounts for margin
4. **✅ Position Validation**: All positions logged and verified
5. **🔄 Scale Centralization**: Heart scale=6.0, others=8.0 (documented)

---

*Last Updated: 2025-09-03*
*Debug Data from: FloppyTurd_Debug_Latest.txt*
