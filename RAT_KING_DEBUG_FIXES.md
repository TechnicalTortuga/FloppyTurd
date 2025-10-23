# Rat King Level Debug Fixes

## Date: 2025-01-XX
## Issues Addressed

### 1. **Rats Wrapping from Bottom Edge (CRITICAL)**
**Problem**: RatCopters were spawning from the bottom edge of the screen after their initial spawn, despite being configured for a middle band (30%-50%).

**Root Cause**: The boss level (Level 6) runs in **LANDSCAPE mode** (2556x1179), but the rat spawning code was using portrait-oriented percentage ranges (30%-50%). When applied to the smaller landscape height (1179px instead of 2556px), these percentages were being calculated correctly, BUT when rats went off-screen during BEELINE state and wrapped around, they could exit from ANY edge (including bottom), and the wrap logic wasn't constraining their Y position properly.

**Fixes Applied**:
- **LevelManager.cpp** (Lines 590-602, 717-730, 1795-1809, 2070-2081):
  - Added landscape-aware Y band calculation: **50%-70% in landscape** vs **30%-50% in portrait**
  - In landscape (1179px height): 50%-70% = 589-825px (middle-lower band)
  - In portrait (2556px height): 30%-50% = 766-1278px (middle band)
  - Added detailed logging showing screen orientation, height, and calculated Y percentages
  
- **EnemySystem.cpp** (Lines 1046-1073):
  - Added Y boundary constraint checking when rats wrap horizontally
  - If rat's Y position is outside valid bounds (topPadding to screenHeight-bottomPadding), reset to safe middle band
  - Uses landscape-aware percentages (50%-70% or 30%-50% depending on orientation)
  - Prevents rats from wrapping back with invalid Y coordinates

### 2. **Hitbox Center Calculation Inconsistency**
**Problem**: Player could pass through obstacles (like TP from below) because hitbox center calculations were inconsistent across different entity types.

**Root Cause**: 
- Player/Enemy/Obstacle centers calculated as: `position + spriteHalfDimensions + (offset * scale)`
- Projectile centers calculated as: `position + (offset * scale)` (missing sprite dimensions!)
- This caused misalignment in collision detection, especially for circle-vs-circle checks

**Fix Applied**:
- **GameplayState.cpp** (Lines 3036-3046):
  - Fixed projectile center calculation to include sprite half-dimensions
  - Now consistent with player/enemy calculations
  - Formula: `projCenterX = position.x + spriteHalfWidth + (offset * scale.x)`

### 3. **Boss Targeting Accuracy (Player Lock-On)**
**Problem**: Rat King's projectiles were either overshooting or undershooting the player.

**Root Cause**: Boss was targeting pure sprite center without accounting for hitbox offset, leading to aiming inaccuracy.

**Fix Applied**:
- **GameplayState.cpp** (Lines 349-374):
  - Changed boss targeting to aim at **AVERAGE** of sprite center and hitbox center
  - Old: `playerCenter = position + halfWidth/Height` (sprite center only)
  - New: `playerCenter = (spriteCenter + hitboxCenter) * 0.5f` (split the difference)
  - This provides better targeting accuracy that accounts for player's actual collision area

### 4. **Enhanced Logging for Debug Analysis**
**Problem**: Insufficient logging made it difficult to diagnose spawn position issues in landscape mode.

**Fixes Applied**:
- Added `[RAT_WRAP]` logs showing screen height, orientation, and Y position percentages
- Added `[RATCOPTER_SPAWN]` logs with detailed spawn calculations
- Added `[RESET]` logs for level retry repositioning with orientation awareness
- Added `[RAT_Y_CONSTRAINT]` logs when rats are repositioned due to invalid Y bounds

## Testing Checklist

### Expected Behaviors After Fixes:
1. ✅ **Rats spawn in correct middle band** (50%-70% of landscape screen height)
2. ✅ **Rats do NOT enter from bottom edge** after initial spawn
3. ✅ **Player cannot pass through obstacles from below** (TP, walls, etc.)
4. ✅ **Boss projectiles aim accurately at player** (not over/undershooting)
5. ✅ **Collision detection works consistently** for all entity types
6. ✅ **Detailed logs show orientation-aware calculations**

### How to Verify:
1. Launch game and navigate to Boss Level (Level 6)
2. Observe rat spawn positions - should be in middle-lower band (50%-70% of screen height)
3. Let rats complete their attack cycle (FLY_IN → HOVER → PULLBACK → BEELINE → wrap)
4. Watch for rats re-entering from right side - they should maintain middle-band Y positions
5. Check logs for `[RAT_WRAP]` entries showing landscape orientation detection
6. Test player collision with obstacles from below - should collide properly
7. Observe boss aiming - projectiles should track toward player accurately

## Technical Details

### Landscape Mode Screen Dimensions:
- Portrait: 1179w × 2556h
- Landscape: 2556w × 1179h (swapped for boss level)

### Rat Y Position Bands:
| Mode | Min % | Max % | Min Y (px) | Max Y (px) | Visual Position |
|------|-------|-------|------------|------------|-----------------|
| Portrait | 30% | 50% | 767 | 1278 | Middle band |
| Landscape | 50% | 70% | 589 | 825 | Middle-lower band |

### Collision Center Formulas:
**Player/Enemy/Obstacle**:
```cpp
centerX = position.x + (spriteWidth * scale.x * 0.5f) + (hitbox.offsetX * scale.x)
centerY = position.y + (spriteHeight * scale.y * 0.5f) + (hitbox.offsetY * scale.y)
```

**Projectile** (NOW FIXED):
```cpp
centerX = position.x + (spriteWidth * scale.x * 0.5f) + (hitbox.offsetX * scale.x)
centerY = position.y + (spriteHeight * scale.y * 0.5f) + (hitbox.offsetY * scale.y)
```

**Boss Target Lock-On** (NOW USES AVERAGE):
```cpp
spriteCenterX = position.x + halfWidth
hitboxCenterX = spriteCenterX + (hitbox.offsetX * scale.x)
targetX = (spriteCenterX + hitboxCenterX) * 0.5f  // Split the difference
```

## Files Modified

1. **src/FloppyTurd/Systems/LevelManager.cpp**
   - Lines 590-602: Landscape-aware rat wrap positioning
   - Lines 717-730: Landscape-aware rat wrap with detailed logging
   - Lines 1795-1809: Initial spawn landscape awareness
   - Lines 2070-2081: Level retry reset landscape awareness

2. **src/FloppyTurd/Systems/EnemySystem.cpp**
   - Lines 1046-1073: Y boundary constraint checking on wrap

3. **src/FloppyTurd/States/GameplayState.cpp**
   - Lines 349-374: Boss targeting accuracy (split difference)
   - Lines 3036-3046: Projectile hitbox center calculation fix

## Notes

- All changes maintain backward compatibility with portrait mode levels
- Landscape-specific logic only activates when `screenInfo.isPortrait == false`
- ConfigManager properly updates screen info when orientation changes
- Rat state machine (FLY_IN → HOVER → PULLBACK → BEELINE) remains unchanged
- Only spawn positioning and wrap logic were adjusted for landscape awareness

## Future Improvements

Consider for future iterations:
1. Make Y band percentages configurable per level instead of hardcoded
2. Add visual debug overlay showing spawn zones and boundaries
3. Consider rectangular hitboxes for obstacles if circle collisions still prove problematic
4. Add enemy-specific Y constraint configurations (some enemies might need different bands)