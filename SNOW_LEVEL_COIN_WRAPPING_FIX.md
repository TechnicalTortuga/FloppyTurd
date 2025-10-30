# Snow Level Coin Wrapping Fix

**Date**: October 29, 2025  
**Status**: ✅ Complete  
**File Modified**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`

---

## Problem Summary

The snow level had two critical issues with obstacle groups and coins:

### Issue 1: Visible Coin Disappearing During Wrap
Coins were positioned **AFTER** (to the right of) each toilet pair. When a toilet group wrapped around the screen, the coins that belonged to that group were still visible to the right, causing them to suddenly disappear when the group wrapped.

### Issue 2: Gap Narrowing After 30 Groups
After the initial 32 obstacle groups wrapped around, the spacing between groups narrowed significantly. This was caused by a mismatch between initialization spacing and wrapping spacing:
- **During initialization**: Groups spawned with 1500px gaps
- **During wrapping**: Groups repositioned with only 900px gaps
- **Result**: After 32 wraps, gaps became progressively narrower

---

## Solutions Implemented

### Fix 1: Coin Positioning (Line 1531)
**Changed coin positioning from AFTER to BEFORE toilet pairs:**

```cpp
// OLD: Coins positioned AFTER obstacles
float coinCenterX = maxX + (gapSize / 2.0f);  // Position in gap AFTER obstacles

// NEW: Coins positioned BEFORE obstacles
float coinCenterX = minX - (gapSize / 2.0f);  // Position in gap BEFORE obstacles
```

**Why this works:**
- Coins now naturally scroll off-screen FIRST
- Then the toilet pair follows
- Only then does the group wrap around
- No visible disappearing effect!

### Fix 2: Consistent Gap Spacing (Lines 1530, 1767, 2448)
**Synchronized all gap values to 1500px:**

**Coin positioning gap** (line 1530):
```cpp
// OLD: const float gapSize = 900.0f;
// NEW:
const float gapSize = 1500.0f; // Must match initialization spacing for consistent gaps
```

**Wrapping gap** (line 1767):
```cpp
// OLD: newX = rightmostOriginX + 900.0f;
// NEW:
newX = rightmostOriginX + 1500.0f;  // MUST MATCH INITIALIZATION SPACING
```

**GroupGap component** (line 2448):
```cpp
// OLD: GroupGap groupGap(900.0f, false, "snow_toilet_spacing");
// NEW:
GroupGap groupGap(1500.0f, false, "snow_toilet_spacing");
```

---

## Technical Details

### Obstacle Pool Size
- **OBSTACLE_POOL_SIZE**: 32 groups (defined in `ObstacleSystem.h`)
- This means 32 toilet pairs exist simultaneously on screen
- After a group wraps, it must maintain the same spacing as initialization

### Gap Spacing Breakdown
- **Initialization** (line 110): `nextWorldX += 1500.0f`
- **Wrapping** (line 1767): `newX = rightmostOriginX + 1500.0f`
- **Coins** (line 1530): `const float gapSize = 1500.0f`
- **GroupGap** (line 2448): `GroupGap groupGap(1500.0f, ...)`

All four locations now use **1500px** for consistency.

---

## Testing Recommendations

1. **Play snow level for extended period** (30+ obstacle groups passed)
2. **Verify gaps remain consistent** throughout gameplay
3. **Confirm coins don't visibly disappear** when groups wrap
4. **Check coin positioning** - should appear BEFORE toilet pairs
5. **Monitor gap width** - should stay at 1500px even after wrapping

---

## Related Systems

### Park Level Pattern
The park level (Level 1) uses a different pattern:
- No fixed gaps during initialization
- Uses `groupWidth` for spacing (line 123)
- Wrapping uses `rightmostOriginX + rightmostGroupWidth` (line 1781)
- This works because the pattern is self-consistent

### Castle Level
Castle level uses 2000px spacing consistently:
- Initialization: 2000px (line 115)
- Wrapping: 2000px (line 1774)
- Already correct, no changes needed

---

## Key Takeaway

**For infinite-scrolling obstacle systems with fixed gaps:**
- Initialization spacing MUST match wrapping spacing
- Coin positioning MUST match gap spacing
- GroupGap component MUST match both
- Any mismatch will cause progressive narrowing after wrapping cycles



