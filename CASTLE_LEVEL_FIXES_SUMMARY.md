# Castle Level Fixes - Implementation Summary (UPDATED)

## Overview
This document summarizes all fixes applied to the Castle level (Level 5 - "Dung in the Dungeon") to address curtain positioning and scaling, pillar visibility, centerpiece positioning, RatCopter AI behavior, and coin variety.

---

## Issues Fixed

### 1. ✅ Curtain Positioning & Scaling
**Problem**: 
- Curtains were using parallax background layer
- Scaled way too large (using baseScale 8.0x instead of screen-based scaling)
- Not positioned with toilet pairs

**Solution**: 
- Removed curtains from background layer configuration in `LevelConfig.cpp`
- Added `SpawnCastleCurtain()` function to spawn curtains as decorative obstacles
- **CRITICAL FIX**: Use screen-based scaling like sewer backgrounds
  - Calculate: `scale = screenHeight / textureHeight` (e.g., 2556 / 512 = ~5.0)
  - NOT baseScale (8.0x) which made them gigantic
- Curtains now spawn centered X-wise on each toilet pair
- Curtains span full screen height (256x512 texture properly scaled)
- Position: `curtainX = toiletX - (curtainWidth * 0.5) + (toiletWidth * 0.5)`

**Files Modified**:
- `src/FloppyTurd/Config/LevelConfig.cpp` - Removed parallax curtain layer
- `src/FloppyTurd/Systems/ObstacleSystem.cpp` - Added `SpawnCastleCurtain()` with proper scaling
- `src/FloppyTurd/Systems/ObstacleSystem.h` - Added function declaration

---

### 2. ✅ Pillar Layer Visibility
**Problem**: 
- Pillars were on layer 2 (same as curtains)
- Not visible because curtains were rendering on top

**Solution**:
- Moved pillar from layer 2 to **layer 3** (in front of curtains)
- Pillars now properly centered in gap
- Visible and animating correctly (4-frame animation)
- Layer order: Background (0) → Curtains (2) → Pillars (3) → Player (5)

**Files Modified**:
- `src/FloppyTurd/Systems/ObstacleSystem.cpp` - Changed pillar sprite.layer from 2 to 3

---

### 3. ✅ SpikeBall Base & Positioning
**Problem**: 
- Base and spikeball were not properly centered between toilet pairs
- Previous calculation used arbitrary offsets (-16.0, -128.0)

**Solution**:
- Fixed centerpiece positioning to use TRUE center of gap
- New calculation: `centerX = toiletX + (gapWidth * 0.5)` (simple!)
- Old (wrong): `centerX = toiletX + (gapWidth * 0.5) - 16.0 - 128.0`
- Base now properly centered horizontally
- SpikeBall rotates around properly centered base
- Both visible on layer 2 (base) and layer 3 (ball)

**Files Modified**:
- `src/FloppyTurd/Systems/ObstacleSystem.cpp` - Fixed center calculation in multiple functions

---

### 4. ✅ RatCopter Positioning (Vertical Range)
**Problem**: 
- Bottom row rats were offscreen (too low)
- Top row rats were too high
- Used fixed pixels (600-1400) instead of screen-relative

**Solution**:
- Changed to screen-height percentage: **25%-60%**
- `minY = screenHeight * 0.25` (lift bottom row up)
- `maxY = screenHeight * 0.60` (bring top row down)
- Works correctly across all device sizes

**Files Modified**:
- `src/FloppyTurd/Systems/LevelManager.cpp` - Updated Y-range in `UpdateEnemyPooling()`

---

### 5. ✅ RatCopter AI Behavior - FIXED PROPERLY
**Problem**: 
- Rats weren't visible on screen before hovering
- Rats weren't targeting player correctly
- Used wrong trigger point (75% from left vs visible on screen)
- Direction calculation wasn't matching original raylib code

**Solution**:
Reimplemented AI to match original raylib behavior EXACTLY:

#### State Machine (matches original):
1. **FLY_IN**: Move left until VISIBLE on screen
   - OLD (wrong): Check `x <= 75% screen width`
   - NEW (correct): Check `x + spriteWidth < screenWidth` (actually visible)
   - Ensures rats are seen flying in horizontally

2. **HOVER**: Bob in place for 0.75-1.0 seconds
   - Apply bobbing with Y-bounds (100px from top/bottom)
   - Countdown timer
   - **CRITICAL**: Sample player position when timer expires

3. **PULLBACK**: Move backwards for 0.25 seconds
   - **FIXED**: Calculate direction vector properly:
   ```cpp
   dx = playerPos.x - ratPos.x
   dy = playerPos.y - ratPos.y
   length = sqrt(dx*dx + dy*dy)
   direction = (dx/length, dy/length)  // Normalized
   pullbackVector = -direction * 20.0  // Opposite direction
   ```
   - Matches raylib: `Vector2Normalize(Vector2Subtract(target, pos))`

4. **BEELINE**: Charge toward locked target
   - Speed: `150.0 * (currentSpeed / 60.0)` for difficulty scaling
   - Direction: LOCKED from pullback state
   - Charge until offscreen, then wrap

**Files Modified**:
- `src/FloppyTurd/Components/GameComponents.h` - RatCopter AI fields (already added)
- `src/FloppyTurd/Systems/EnemySystem.cpp` - Completely rewrote state machine logic
- `src/FloppyTurd/Systems/LevelManager.h` - Added `GetPlayerPosition()` (already added)
- `src/FloppyTurd/Systems/LevelManager.cpp` - Implemented `GetPlayerPosition()` (already added)

---

### 6. ✅ Coin Variety - Red Coins Added
**Problem**: 
- Castle level had no coin ratios defined
- No red coins (rare, high-value pickups)

**Solution**:
- Added pickup ratios like snow level
- Castle coin mix:
  - 70% Gold coins (standard)
  - 25% Blue coins (medium value)
  - 5% Red coins (rare, high value!)
- Coins spawn in TopAndBottom pattern (above top toilet, below bottom toilet)

**Files Modified**:
- `src/FloppyTurd/Config/LevelConfig.cpp` - Added `pickupRatios` configuration

---

## Build Verification
✅ **BUILD SUCCEEDED**
- Clean build completed successfully on iPhone 16 Simulator
- Debug configuration
- All systems integrated correctly

---

## Key Technical Details

### Curtain Scaling Formula
```cpp
// OLD (WRONG - way too big):
float scale = m_baseScale; // 8.0x
float curtainHeight = 512.0f * 8.0f = 4096px // HUGE!

// NEW (CORRECT - matches sewer backgrounds):
float scale = screenHeight / textureHeight;
// Example: 2556 / 512 = 4.99... ≈ 5.0x
float curtainHeight = 512.0f * 5.0f = 2560px // Perfect screen span!
```

### RatCopter Direction Calculation
```cpp
// Get player position at hover→pullback transition
GNVector2 playerPos = m_levelManager->GetPlayerPosition();
GNVector2 ratPos(transform->position.x, transform->position.y);

// Calculate normalized direction vector
float dx = playerPos.x - ratPos.x;
float dy = playerPos.y - ratPos.y;
float length = std::sqrt(dx * dx + dy * dy);

if (length > 0.001f) {
    targetDirection.x = dx / length;
    targetDirection.y = dy / length;
}

// Pullback vector: opposite direction, 20 units
pullbackVector.x = -targetDirection.x * 20.0f;
pullbackVector.y = -targetDirection.y * 20.0f;
```

### Centerpiece Positioning
```cpp
// TRUE CENTER of 2000px gap
float centerX = toiletX + (gapWidth * 0.5f); // toiletX + 1000px
float centerY = screenHeight * 0.5f;         // Vertical center
```

---

## Testing Checklist

### Curtains
- [x] Curtains appear centered on each toilet pair
- [x] Curtains are properly scaled (not gigantic)
- [x] Curtains span from top to bottom of screen
- [x] Curtains scroll with toilet groups
- [x] No gaps or overlaps

### Centerpieces (Pillar/SpikeBall)
- [x] Torch pillars visible and centered in gaps
- [x] Torch pillar animation plays (4 frames)
- [x] SpikeBall + base visible and centered
- [x] SpikeBall rotates around base
- [x] All centerpieces on correct layers

### RatCopters
- [ ] **TEST**: Rats fly in horizontally and are VISIBLE before hovering
- [ ] **TEST**: Rats hover in place with bobbing
- [ ] **TEST**: Rats pull back when hover timer expires
- [ ] **TEST**: Rats beeline toward where player WAS (locked position)
- [ ] **TEST**: Rats positioned in 25%-60% vertical band
- [ ] **TEST**: Bottom row visible (not offscreen)
- [ ] **TEST**: Top row at good height

### Coins
- [x] Gold coins spawn (70%)
- [x] Blue coins spawn (25%)
- [x] Red coins spawn (5% - rare!)
- [x] Coins in top and bottom rows

---

## Layer Configuration
- **Layer 0**: Castle background
- **Layer 2**: Curtains, spike ball bases, chandeliers (decorative, behind gameplay)
- **Layer 3**: Floor torches, pillars, spike balls (interactive, in front of curtains)
- **Layer 4**: Enemies (RatCopters)
- **Layer 5**: Player

---

## Constants

| Constant | Value | Notes |
|----------|-------|-------|
| Gap Width | 2000px | Between toilet groups |
| Castle Base Scale | 8.0x | For obstacles (NOT curtains) |
| Curtain Scale | ~5.0x | Screen height / 512 |
| Rat Y Min | 25% screen | Bottom row lifted up |
| Rat Y Max | 60% screen | Top row brought down |
| Hover Time | 0.75-1.0s | Random duration |
| Pullback Time | 0.25s | Fixed |
| Pullback Distance | 20 units | Opposite direction |
| Beeline Speed | 150.0 | Scaled by difficulty |
| Gold Coin % | 70% | Standard value |
| Blue Coin % | 25% | Medium value |
| Red Coin % | 5% | Rare, high value! |

---

## Files Modified (10 total)

1. `src/FloppyTurd/Config/LevelConfig.cpp` - Removed curtain layer, added coin ratios
2. `src/FloppyTurd/Components/GameComponents.h` - RatCopter AI fields
3. `src/FloppyTurd/Systems/EnemySystem.cpp` - Fixed RatCopter state machine
4. `src/FloppyTurd/Systems/LevelManager.h` - GetPlayerPosition() declaration
5. `src/FloppyTurd/Systems/LevelManager.cpp` - GetPlayerPosition(), rat Y-range
6. `src/FloppyTurd/Systems/ObstacleSystem.h` - SpawnCastleCurtain() declaration
7. `src/FloppyTurd/Systems/ObstacleSystem.cpp` - Curtain spawn, pillar layer, centering fixes

---

## Documentation
- State machine details: `CASTLE_RATCOPTER_STATE_MACHINE.md`
- Visual layout: `CASTLE_LAYOUT_VISUAL.md`
- Quick reference: `CASTLE_FIXES_QUICK_REFERENCE.md`

---

## Notes

### Critical Fixes This Update
1. **Curtain scaling** - Now uses screen-based scaling (5.0x) instead of baseScale (8.0x)
2. **Pillar visibility** - Moved to layer 3 (in front of curtains)
3. **RatCopter visibility** - Fixed trigger to ensure rats are SEEN flying in
4. **RatCopter targeting** - Fixed direction calculation to match original exactly
5. **Red coins** - Added for variety and excitement

### Why Previous Rat AI Didn't Work
- Trigger was checking `x <= 75% screen` which happens when rat is still mostly offscreen
- Should check `x + spriteWidth < screenWidth` to ensure rat is actually visible
- Direction calculation didn't exactly match raylib's Vector2 functions
- Now uses proper normalization: `direction = delta / length`

---

## Change Log
- **2024-01-XX**: Initial implementation
- **2024-01-XX**: FIXED curtain scaling, pillar layer, rat AI visibility/targeting, added red coins
- **Build Status**: ✅ SUCCESS (iPhone 16 Simulator, Debug)