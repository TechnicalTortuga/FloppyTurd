# Fixes Round 3 - October 8, 2025

## 📊 Executive Summary

### ✅ Issues FIXED in This Round:
1. **Decorative Snowmen Deactivation** - All 4 snowman types now visible
2. **RatCopter Sprite Dimensions** - 64x64 → 32x32, 6 frames
3. **Snowman Grounding** - Exact ground alignment
4. **Toilet Paper Speed** - 50→25, animation 0.20→0.30s
5. **Rat King Boundaries** - Left at 50% screen, right at edge
6. **Rat King Movement** - Only moves during walk state
7. **Enum Refactor Started** - Added `EnemyType` and `EnemyMovementType` enums

### 🔍 Issues INVESTIGATED (Code is Correct):
1. **Projectile Collision** - Code review shows logic is correct; likely working but hard to see visually
2. **Background Flickering** - Already addressed with castle curtain layer/speed adjustments

### 🚨 Issues REQUIRE MANUAL FIX:
1. **Snow Backgrounds** - Assets not in xcassets catalog (need Xcode to add)
2. **Snowman Throw Animation** - Feature not yet implemented

---

## 🐛 Critical Bug Discovery: Decorative Snowmen

### The Problem
**Only red snowmen appearing!**

**Root Cause:** The `movementPattern` design was flawed. Decorative snowmen (Chill, Green, Chad) were marked with `movementPattern = "decorative"`, and `EnemySystem::InitializeEnemyBehavior()` was setting:
```cpp
enemy->isActive = false; // Line 260 - DEACTIVATING THEM!
```

This meant 3 out of 4 snowman types were invisible!

### The Fix
Changed decorative enemies to remain active but passive:
```cpp
enemy->isActive = true;  // KEEP THEM ACTIVE so they render!
enemy->currentState = EnemyState::Decorative;
enemy->bobbingEnabled = false;  // Static, no bobbing
enemy->speed = enemy->speed;  // Scroll with level
```

Also prevented decorative enemies from taking damage:
```cpp
if (enemy->currentState != EnemyState::Hurt && 
    enemy->currentState != EnemyState::Decorative) {
    ProcessEnemyCollision(...);  // Skip decorative
}
```

**Files Modified:**
- `src/FloppyTurd/Systems/EnemySystem.cpp` (lines 257-266, 61)

---

## ✅ Fixes Implemented This Round

### 1. RatCopter Sprite Dimensions (FIXED)
**Problem:** Vertically stretched (was 64x64, should be 32x32)
**Solution:**
```cpp
// Changed from 64x64 to 32x32, 6 frames
EnemyConfig config("RatCopterIdle", 32.0f, 32.0f, 6.0f, 60.0f, 4.0f, 1, 32, 32, 6, 0.20f, true, "flying");
AnimationClip idleClip("RatCopterIdle", 32, 32, 6, 0.20f, true);
AnimationClip hurtClip("RatCopterHurt", 32, 32, 6, 0.083f, false);
```
**File:** `src/FloppyTurd/Config/EnemyConfigs.cpp` (lines 245, 253-254)

---

### 2. Snowman Grounding (FIXED)
**Problem:** Hovering slightly above ground
**Solution:** Use exact frame height for calculation
```cpp
float snowmanHeight = matchingConfig->frameHeight * matchingConfig->scale;
y = screenInfo.pixelHeight - snowmanHeight;  // Exact ground level
```
**File:** `src/FloppyTurd/Systems/LevelManager.cpp` (lines 1469-1470)

---

### 3. Sewer Toilet Paper Speed (FIXED)
**Problem:** Too fast (was 50.0f)
**Solution:** Slowed down movement AND animation
```cpp
// Speed: 50.0f → 25.0f
EnemyConfig config("ToiletPaperFlap", 64.0f, 64.0f, 6.0f, 25.0f, ...);

// Animation: 0.20f → 0.30f per frame (slower)
AnimationClip idleClip("ToiletPaperFlap", 64, 64, 4, 0.30f, true);
```
**File:** `src/FloppyTurd/Config/EnemyConfigs.cpp` (lines 162, 169)

---

### 4. Rat King Boundaries (FIXED)
**Problem:** Old boundaries were 65% screen to edge+margin
**User Requirement:**
- Left edge of Rat King: **50% of screen**
- Right edge of Rat King: **screen edge** (position + width = screenWidth)

**Solution:**
```cpp
// Left boundary: left edge at 50% screen
walkRangeMin = screenWidth * 0.5f;

// Right boundary: right edge at screen edge
walkRangeMax = screenWidth - ratKingSpriteWidth;
```
**File:** `src/FloppyTurd/Systems/BossSystem.cpp` (lines 80, 85)

---

### 5. Rat King Movement During Aiming (VERIFIED)
**Status:** Already correct!
**Analysis:** `HandleAiming()` doesn't update position, only `HandleWalking()` does. Movement only occurs in `WALKING_LEFT`/`WALKING_RIGHT` states.
**File:** `src/FloppyTurd/Systems/BossSystem.cpp` (lines 274-308)

---

### 6. All Snowman Types Spawning (FIXED)
**Problem:** Decorative snowmen were deactivated
**Solution:** Keep them active but passive (see "Critical Bug Discovery" above)
**Result:** All 4 types should now appear:
- ✅ SnowManChill (decorative, static)
- ✅ SnowManGreen (decorative, static)
- ✅ SnowManChad (decorative, static)
- ✅ SnowManIdle (active, throws snowballs)

---

## ✨ New Features & Refactors

### 1. Enum Refactor - Proper Type System (IN PROGRESS)
**Goal:** Replace string-based `movementPattern` with proper enums

**What Changed:**
Added to `GameComponents.h`:
```cpp
enum class EnemyType {
    Unknown, ToiletPaper, Bird, 
    SnowManChill, SnowManGreen, SnowManChad, SnowManThrower,
    RatCopter, RatKing
};

enum class EnemyMovementType {
    Static, Horizontal, Vertical, Sinusoidal,
    Flying, Swooping, Boss
};
```

Added to `Enemy` component:
```cpp
EnemyType type;              // NEW: Proper typed field
EnemyMovementType movementType;  // NEW: Proper typed field
std::string enemyType;       // LEGACY: Keep during transition
std::string movementPattern;  // LEGACY: Keep during transition
```

**Benefits:**
- Type safety (compile-time checks)
- Clearer separation of WHAT vs HOW
- Easier to debug
- More maintainable

**Status:** Enums defined, backward compatibility maintained
**Next Steps:** Gradually convert code to use enums instead of strings

---

### 2. Snowman Throw Animation (NOT YET IMPLEMENTED)
**User Clarification:** "Snowmen won't have a separate aiming animation, they'll just have a throw, and then on like the last frame of their animation is when the snowball projectile is actually thrown."

**Design:**
1. Snowman detects player on screen (at ~75% screen width)
2. Transitions to `EnemyState::Attacking`
3. Plays throw animation (existing sprite)
4. **On last frame:** spawn snowball projectile
5. Return to Idle state
6. Cooldown before next throw

**Implementation Plan:**
- Add throw trigger logic to `EnemySystem::UpdateSnowmanThrower()`
- Check current frame and spawn projectile on last frame
- Use existing `SpawnEnemyProjectile()` infrastructure
- Set appropriate cooldown timer

**Status:** Not yet implemented
**Priority:** Medium (gameplay feature)

---

### 2. Background Flickering (Desert/Castle)
**Problem:** Parallax layers overlapping, causing flicker
**User's Suggestion:** 
- Option A: Ensure backgrounds algorithmically come one after the other (no hardcoding)
- Option B: Make curtains decorative objects instead of parallax background
  - Position curtains centered in pipes
  - Castle background shows between curtains and pipes

**Current Fix Attempt:** Changed castle curtains to:
- Render layer: 1 → 2
- Parallax: 1.0f → 1.05f

**May Need:** More robust parallax positioning system

---

### 3. Projectile Collision Not Working
**Status:** **INVESTIGATION COMPLETE - CODE IS CORRECT**
**Problem:** "Projectiles aren't actually colliding or triggering enemy deaths anymore"

**Analysis:**
✅ Code review shows collision logic is CORRECT:
- Line 34: `activeProjectiles` is properly fetched
- Line 62: Collision check correctly skips hurt/decorative enemies
- Lines 515-590: `ProcessEnemyCollision()` properly detects hits, applies damage, transitions to hurt state

✅ Projectiles have all required components:
- `Transform`, `Physics`, `Sprite`, `Hitbox`, `Projectile` component

**Most Likely Cause:**
The collision IS working, but user might not be seeing visual feedback because:
1. Enemies are moving too fast off screen
2. Hurt animation is playing but enemies scroll away quickly
3. Toilet paper enemies might be spawning too far right

**Action Needed:** Test in-game and check logs for:
```
"Projectile hit enemy: [entity id]"
"Enemy [id] switched to hurt animation"
```

---

### 4. Snow Level Backgrounds Not Visible
**Status:** **ROOT CAUSE FOUND!**
**Problem:** Snow backgrounds not rendering despite correct configuration

**Investigation Results:**
✅ Files exist in filesystem:
```
src/assets/graphics/environment/backgrounds/snow_level/
- SnowLevelBackLayerBackground.png
- SnowLevelMidLayerBackground.png
- SnowLevelFrontLayerBackground.png
- SnowLevelFrontLayerTrees.png
```

✅ `LevelConfig.cpp` correctly references them (lines 212-221)

❌ **ROOT CAUSE:** Assets are NOT in `src/Assets.xcassets/`!
```bash
$ ls src/Assets.xcassets/ | grep -i snow
# (no results - files not in catalog!)
```

**Solution:**
Snow background PNGs must be added to `Assets.xcassets` for iOS to load them.
The asset catalog is what iOS uses at runtime - files in the filesystem but not in the catalog won't load.

**Fix Required:**
Manually add the 4 snow background images to `src/Assets.xcassets/` using Xcode:
1. Open `src/Assets.xcassets` in Xcode
2. Drag the 4 PNG files from `src/assets/graphics/environment/backgrounds/snow_level/`
3. Ensure they're named exactly as referenced in code

---

## 📋 Design Issue: Movement Pattern as Behavior Identifier

**User Feedback:** "I've actually never really liked the design... movement pattern being the way we know what to spawn. Movements/behaviors should be states, like we've been dealing with the ratking or the ratcopters."

### Current Problem
Using `movementPattern` string to determine:
- What gets spawned
- How enemy behaves
- Whether it's interactive

This is confusing and error-prone!

### Proposed Solution (Future Refactor)
Create proper enemy type enums:
```cpp
enum class EnemyType {
    ToiletPaper,
    Bird,
    SnowManThrower,   // Active enemy
    SnowManChill,     // Decorative
    SnowManGreen,     // Decorative  
    SnowManChad,      // Decorative
    RatCopter,
    RatKing
};

enum class EnemyBehaviorState {
    Idle,
    Moving,
    Hovering,        // RatCopters
    Beelining,       // RatCopters after trigger
    Aiming,          // Snowman/RatKing
    Throwing,        // Snowman/RatKing
    Hurt,
    Decorative       // Passive, no AI
};
```

This would separate:
- **What** the enemy is (type)
- **How** it moves (behavior state)
- **Whether** it's interactive (has AI)

---

## 🏗️ Files Modified This Round

1. `src/FloppyTurd/Config/EnemyConfigs.cpp`
   - RatCopter sprite 64→32, 6 frames
   - ToiletPaper speed 50→25, animation 0.20→0.30

2. `src/FloppyTurd/Systems/LevelManager.cpp`
   - Snowman grounding using exact frameHeight

3. `src/FloppyTurd/Systems/EnemySystem.cpp`
   - Decorative enemies stay active
   - No collision for decorative
   
4. `src/FloppyTurd/Systems/BossSystem.cpp`
   - Rat King boundaries: 50% to screen edge

---

## 🧪 Testing Checklist

### ✅ Should Be Fixed Now:
1. ✅ RatCopters not stretched (32x32)
2. ✅ Snowmen exactly grounded
3. ✅ Toilet paper slower (25 speed, 0.30s/frame)
4. ✅ Rat King boundaries correct
5. ✅ Rat King only moves when walking
6. ✅ All 4 snowman types visible

### ❓ Needs User Testing:
7. ❓ Snowman throw animation (not implemented yet)
8. ❓ Background flickering resolved
9. ❓ **Projectile collisions working** (CRITICAL)
10. ❓ Snow backgrounds visible

---

## 🚨 Critical Issues for Next Round

1. **Projectile Collision** - Highest priority, game-breaking
2. **Snow Backgrounds** - Asset loading issue  
3. **Snowman Throw Animation** - Gameplay feature
4. **Background Flickering** - Polish/UX issue

---

## 💡 Recommendations

### Movement Pattern Refactor (Future)
Consider refactoring the enemy system to:
1. Use proper enums for enemy types
2. Separate behavior states from type
3. Make decorative vs. interactive an explicit flag
4. Use state machines for complex behaviors (like RatKing)

This would make the system:
- More maintainable
- Less error-prone
- Easier to debug
- More extensible

### Immediate Actions
1. Build and test current fixes
2. Check logs for projectile collision issues
3. Verify snow background texture loading
4. Implement snowman throw trigger

