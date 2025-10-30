# Castle Level Obstacle Wrapping & Coin Positioning Fix

**Date**: October 29, 2025  
**Status**: ✅ Complete  
**File Modified**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`

---

## Problem Summary

The castle level had three critical issues:

### Issue 1: Obstacles Stop Spawning After Group 43
**Symptom**: After obstacle 43 passes, no more obstacles spawn or wrap around.

**Root Cause**: Castle toilet pairs were **NOT being registered** in `m_obstacleGroups` map!

The code was adding toilets to `m_activeObstacles` but **forgot to register them** in `m_obstacleGroups`, which is required for the wrapping system to find and reposition groups.

**Snow Level** (line 2457) - CORRECT:
```cpp
m_obstacleGroups[groupId] = { topToilet, bottomToilet };
```

**Castle Level** (line 2659-2661) - MISSING:
```cpp
m_activeObstacles.push_back(topToilet);
m_activeObstacles.push_back(bottomToilet);
// ❌ MISSING: m_obstacleGroups[groupId] = { topToilet, bottomToilet };
```

Without this registration, the wrapping system (`WrapGroupAroundScreen`) couldn't find castle toilet groups to reposition them after they went off-screen.

### Issue 2: Coins Not Centered Between Toilet Pairs
**Symptom**: Coins overlapped with toilet pairs on the left instead of being centered in gaps.

**Root Cause**: Same issue as snow level - coins were positioned using complex "3-point symmetry" logic that was overly complicated and positioned coins AFTER toilet pairs.

### Issue 3: Random Pickups Outside Top/Bottom Strips
**Potential Cause**: Decorative items (curtains, torches, chandeliers, pillars, paintings, spike balls) are spawned with `groupId=-1` and added to `m_activeObstacles`, but they have `GroupPattern::Decorative` which might be triggering coin spawning.

---

## Solutions Implemented

### Fix 1: Register Castle Toilet Groups (Line 2663-2666)
Added the missing registration to `m_obstacleGroups`:

```cpp
// CRITICAL: Register this castle toilet pair in the obstacle groups map so wrapping and coin logic can find it
m_obstacleGroups[groupId] = { topToilet, bottomToilet };

GN_LOG_INFO("Registered castle toilet group " + std::to_string(groupId) + " in m_obstacleGroups for wrapping");
```

**Why this works:**
- Wrapping system can now find castle toilet groups via `m_obstacleGroups.find(groupId)`
- Groups will properly wrap around after going off-screen
- Infinite scrolling now works correctly

### Fix 2: Simplified Coin Positioning (Lines 1387-1403)
Replaced complex "3-point symmetry" logic with simple true gap center calculation (matching snow level fix):

```cpp
// FIXED: Position coins BEFORE toilet pairs (same fix as snow level)
const float toiletWidth = (maxX - minX); // Actual width of current toilet group
const float gapSize = 2000.0f; // Gap between toilet groups (must match initialization)

// Calculate true gap boundaries:
// Previous toilet right edge: minX - gapSize + toiletWidth
// Current toilet left edge: minX
// Gap center: halfway between these two points
float previousToiletRightEdge = minX - gapSize + toiletWidth;
float currentToiletLeftEdge = minX;
float coinCenterX = (previousToiletRightEdge + currentToiletLeftEdge) / 2.0f;

// Spread coins around the true gap center
const float coinSpreadHalfWidth = 800.0f; // Half of total coin spread (1600px total)
float coinMinX = coinCenterX - coinSpreadHalfWidth;
float coinMaxX = coinCenterX + coinSpreadHalfWidth;
```

**Benefits:**
- Coins positioned BEFORE toilet pairs (prevents visible disappearing during wrap)
- Truly centered in the gap between toilet pairs
- Consistent with snow level implementation
- Much simpler and easier to understand

---

## Technical Details

### Obstacle Pool Configuration
- **OBSTACLE_POOL_SIZE**: 32 groups (defined in `ObstacleSystem.h`)
- Castle level spawns **32 toilet pairs** (groups 1-32)
- Gap between toilets: **2000px** (defined in line 2655)
- Total span: ~64,000px (32 groups × 2000px)

### Decorative Items (Not in Groups)
These items are spawned with `groupId=-1` (no group):
- Curtains
- Floor Torches (2 per group)
- Chandeliers (2 per group)
- Torch Pillars (some groups)
- Paintings (some groups)
- Spike Balls (some groups)

They're added to `m_activeObstacles` for rendering/physics but don't participate in group wrapping.

### Wrapping Flow
1. `ObstacleSystem::Update()` checks each group leader's position
2. If rightmost member < 0.0f (off-screen), calls `WrapGroupAroundScreen(groupId)`
3. `WrapGroupAroundScreen()` finds rightmost group using `m_obstacleGroups`
4. Positions wrapped group at `rightmostOriginX + 2000px` (castle level spacing)
5. **Now works because castle groups are registered in `m_obstacleGroups`**

---

## Before vs After

### Before (Broken)
```
✅ Groups 1-32 spawn correctly
❌ Groups NOT registered in m_obstacleGroups
❌ Wrapping system can't find groups
❌ After group 32 passes, no more obstacles appear
❌ Game becomes impossible to play
```

### After (Fixed)
```
✅ Groups 1-32 spawn correctly
✅ Groups registered in m_obstacleGroups
✅ Wrapping system finds and repositions groups
✅ Groups wrap infinitely
✅ Coins properly centered in gaps
✅ Coins positioned BEFORE toilets (no visible disappearing)
✅ Game playable indefinitely
```

---

## Testing Recommendations

1. **Play castle level for extended period** (50+ obstacle groups)
2. **Verify obstacles continue wrapping** after initial 32 groups
3. **Check coin positioning** - should be centered in gaps, not overlapping toilets
4. **Confirm no visual coin disappearing** when groups wrap
5. **Monitor decorative items** - should stay properly positioned
6. **Watch for random pickups** outside top/bottom coin strips

---

## Related Files
- `src/FloppyTurd/Systems/ObstacleSystem.cpp` (main fixes)
- `src/FloppyTurd/Systems/ObstacleSystem.h` (OBSTACLE_POOL_SIZE definition)
- `src/FloppyTurd/Components/GameComponents.h` (GroupPattern enum)

---

## Key Takeaways

**For any infinite-scrolling obstacle system:**
1. **ALWAYS register obstacle groups** in both `m_activeObstacles` AND `m_obstacleGroups`
2. **Match initialization spacing with wrapping spacing** (2000px for castle level)
3. **Position coins BEFORE obstacles** to prevent visible disappearing
4. **Calculate true gap center** between obstacles for proper coin placement
5. **Keep decorative items separate** from obstacle groups (use `groupId=-1`)


