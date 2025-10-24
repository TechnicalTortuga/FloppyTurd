# Boss Level Tweaks Summary

## Overview
This document summarizes the tweaks made to the Rat King boss level (Level 6) to improve gameplay, visual feedback, and reset behavior.

## Changes Made

### 1. Lock-On Dots Target Player Center ✅
**File:** `src/FloppyTurd/Systems/BossSystem.cpp`
- **Issue:** Lock-on dots were aiming at player X position instead of full center
- **Fix:** Removed Y-offset adjustment in `UpdateLockOnIndicator()` - dots now aim directly at `aimingData.playerPosition` which is already the player center (verified in `GameplayState.cpp` where it calculates sprite center + hitbox center / 2)
- **Lines:** 760-763

### 2. Lock-On Dots Flash Red/White Before Throw ✅
**File:** `src/FloppyTurd/Systems/BossSystem.cpp`
- **Issue:** Dots were missing the pre-throw flashing effect from the old implementation
- **Fix:** Added flashing effect in final 20% of aiming duration:
  - Progress >= 80% triggers flash mode
  - Flashes at 10Hz (10 times per second)
  - Alternates between pure red (255,0,0) and pure white (255,255,255)
  - Before 80%: continues yellow-to-red gradient as before
- **Lines:** 767-819

### 3. Clean Enemy Pools on Try Again (Except Rat King) ✅
**File:** `src/FloppyTurd/Systems/LevelManager.cpp`
- **Issue:** Rat minions from previous attempt were persisting and appearing in the next attempt
- **Fix:** Modified `ResetEnemiesForRetry()`:
  - Skip Rat King boss entity (managed by BossSystem, not EnemySystem)
  - Identify and remove rat minions (Rat enemy type with "grounded" movement pattern)
  - Remove minions from active enemies list and move them offscreen
  - Other enemies are reset normally with proper spacing
- **Lines:** 2054-2216

### 4. Reset Boss Health and Health Bar on Try Again ✅
**Files:** 
- `src/FloppyTurd/Systems/BossSystem.h` - Added `Reset()` method declaration
- `src/FloppyTurd/Systems/BossSystem.cpp` - Implemented `Reset()` method
- `src/FloppyTurd/Systems/BossHealthBar.h` - Added `Reset()` method declaration
- `src/FloppyTurd/Systems/BossHealthBar.cpp` - Implemented `Reset()` method
- `src/FloppyTurd/States/GameplayState.cpp` - Call reset methods in `TryAgain()`

**BossSystem::Reset()** resets:
- Health to maxHealth (200)
- State to IDLE
- All timers (idle, walk, hurt, death)
- Animation state flags
- Minion spawn threshold to 185
- Aiming data and lock-on indicators
- Destroys and prepares lock-on dot entities for clean state

**BossHealthBar::Reset()** resets:
- Current health percent to 1.0
- Shadow health percent to 1.0 (removes hurt fade effect)
- Displayed health percent to 1.0
- Hurt fade timer to 0
- Updates UI entities to show full health

**GameplayState::TryAgain()** calls both reset methods when level 6 is active.

### 5. Player Projectiles Do 10 Damage to Rat King ✅
**File:** `src/FloppyTurd/States/GameplayState.cpp`
- **Issue:** Player projectiles were doing 1 damage per hit (would take 200 hits to defeat boss)
- **Fix:** Changed damage from `proj->damage` (1) to hardcoded 10 damage per hit
- **Result:** Boss with 200 health now takes 20 hits to defeat (200 / 10 = 20 hits)
- **Lines:** 442-443

### 6. Snowball Hitbox Radius Reduced to 4 Pixels ✅
**File:** `src/FloppyTurd/Systems/ProjectileSystem.cpp`
- **Issue:** Snowball hitboxes were too large (14 pixels), making them hard to dodge
- **Fix:** Changed `SNOWBALL` case in `ConfigureProjectileSprite()`:
  - Reduced radius from 14.0f to 4.0f pixels (unscaled)
  - Updated comment to reflect small precision radius
  - Note: This is the UNSCALED radius - collision system applies Transform.scale to get effective radius
- **Lines:** 545-556

## Technical Notes

### Collision System
- All hitbox radii are stored as **unscaled values** in the base sprite coordinate system
- The collision detection code in `GameplayState::CheckToiletCollisions()` applies `Transform.scale` when calculating effective radius:
  ```cpp
  float effectiveRadius = hitbox.radius * ((transform.scale.x + transform.scale.y) * 0.5f);
  ```
- This ensures consistent collision behavior regardless of sprite scaling

### Boss System Architecture
- Rat King boss is managed by `BossSystem`, not `EnemySystem`
- Rat minions spawned during battle are standard enemies in `EnemySystem`
- Reset logic must handle both systems appropriately
- Boss health bar is a separate UI system that mirrors boss health

### Lock-On Indicator
- Uses `DebugDraw` components for rendering colored dots
- Dots are created once and reused (entities stored in `aimingData.dotEntities`)
- Color and visibility updated each frame based on aiming progress
- Flashing effect uses simple modulo math on scaled timer: `(int)(timer * 10.0f) % 2`

## Testing Checklist

When testing these changes, verify:

- [ ] Lock-on dots aim toward player's center (not just horizontally)
- [ ] Dots flash red/white rapidly in the final 20% of aiming duration
- [ ] Dots transition smoothly from yellow→red gradient to flashing mode
- [ ] On Try Again, no rat minions from previous attempt appear
- [ ] On Try Again, Rat King's health bar shows full health
- [ ] On Try Again, boss starts in IDLE state with full health
- [ ] Player projectiles do 10 damage per hit to Rat King
- [ ] Rat King dies after 20 projectile hits (200 health / 10 damage)
- [ ] Snowballs are easier to dodge (smaller hitbox)
- [ ] Snowball hitbox feels precise and fair (4px radius)

## Build Info

- **Build Command:** 
  ```bash
  xcodebuild -project build_ios/FloppyTurd.xcodeproj -scheme FloppyTurd \
    -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" clean build
  ```
- **Install Command:**
  ```bash
  xcrun simctl install booted build_ios/Debug-iphonesimulator/Debug/FloppyTurd.app
  ```
- **Build Status:** ✅ BUILD SUCCEEDED

## Files Modified

1. `src/FloppyTurd/Systems/BossSystem.h` - Added Reset() method
2. `src/FloppyTurd/Systems/BossSystem.cpp` - Implemented lock-on targeting, flashing, and Reset()
3. `src/FloppyTurd/Systems/BossHealthBar.h` - Added Reset() method
4. `src/FloppyTurd/Systems/BossHealthBar.cpp` - Implemented health bar reset
5. `src/FloppyTurd/Systems/LevelManager.cpp` - Modified enemy reset logic
6. `src/FloppyTurd/Systems/ProjectileSystem.cpp` - Reduced snowball radius
7. `src/FloppyTurd/States/GameplayState.cpp` - Increased boss damage, added reset calls

## Related Documentation

- See `Rat King Level Hitbox Debugging` thread for hitbox collision system details
- See `LANDSCAPE_GAMEOVER_IMPLEMENTATION_GUIDE.md` for boss level UI/layout information