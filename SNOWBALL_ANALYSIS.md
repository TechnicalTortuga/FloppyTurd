# SNOWBALL THROW ANALYSIS - Frame-by-Frame Report

## 🚨 CRITICAL ISSUES IDENTIFIED

### **Issue 1: ALL Snowballs Going UP-LEFT (Top-Left, Not Top-Right)**
- **Every single velocity has NEGATIVE X and NEGATIVE Y**
- This means: LEFT (negative X) and UP (negative Y)
- User reports they go "top right" but data shows top-LEFT

### **Issue 2: Player Position STUCK at X=332**
- Player X is ALWAYS 332 (300 + 32 center offset)
- Player Y varies: 840, 662, 986, 681, 423, 212, etc.
- This seems correct IF player is stationary horizontally

### **Issue 3: Upward Boost Making Throws Too Vertical**
- Original angles: -117° to -133° (downward-left)
- Boosted angles: -103° to -116° (more upward-left)
- Boost of -150px is making snowballs go MORE upward when they should aim at player

---

## 📊 FRAME-BY-FRAME DATA ANALYSIS

### **Frame 1**
```
Player CENTER: (332, 1070.94)
Snowman pos: (1245.64, 2236)
Spawn pos: (1295.64, 2336)

Delta Calculation:
dx = 332 - 1295.64 = -913.64  (Player is 913px to the LEFT)
dy = 1070.94 - 2336 = -1165.94  (Player is 1165px ABOVE spawn)

Original Angle: -128.08°  (pointing down-left toward player)
Boosted Angle: -112.78°  (rotated more upward by boost)

Final Velocity: (-135.70, -323.17)
  X: -135.70 = Going LEFT at 135 px/s  ✅ Correct direction
  Y: -323.17 = Going UP at 323 px/s   ⚠️ TOO STEEP/VERTICAL
```

### **Frame 2**
```
Player CENTER: (332, 1059.65)
Snowman pos: (1244.94, 2236)
Spawn pos: (1294.94, 2336)

Delta Calculation:
dx = 332 - 1294.94 = -912.94
dy = 1059.65 - 2336 = -1276.35

Original Angle: -125.58°
Boosted Angle: -111.26°

Final Velocity: (-127.99, -328.94)
  X: -127.99 = Going LEFT ✅
  Y: -328.94 = Going UP ⚠️ EVEN STEEPER
```

### **Frame 7**
```
Player CENTER: (332, 660.74)
Snowman pos: (1137.77, 2236)
Spawn pos: (1187.77, 2336)

Delta Calculation:
dx = 332 - 1187.77 = -855.77
dy = 660.74 - 2336 = -1675.26

Original Angle: -117.06°
Boosted Angle: -106.14°

Final Velocity: (-100.08, -345.92)
  X: -100.08 = Going LEFT ✅
  Y: -345.92 = Going UP ⚠️ EXTREMELY VERTICAL
```

### **Frame 8**
```
Player CENTER: (332, 1028.39)
Snowman pos: (1145.65, 2236)
Spawn pos: (1195.65, 2336)

Delta Calculation:
dx = 332 - 1195.65 = -863.65
dy = 1028.39 - 2336 = -1307.61

Original Angle: -123.44°
Boosted Angle: -109.98°

Final Velocity: (-121.25, -333.57)
  X: -121.25 = Going LEFT ✅
  Y: -333.57 = Going UP ⚠️ TOO VERTICAL
```

### **Frame 9 (FACING RIGHT - FLIPPED)**
```
Player CENTER: (332, 662.70)
Snowman pos: (1620.67, 2236) ⚠️ VERY FAR RIGHT
Spawn pos: (1570.67, 2336)  ⚠️ Note: Facing RIGHT, so spawn is to LEFT of snowman

Delta Calculation:
dx = 332 - 1570.67 = -1238.67  (Massive distance!)
dy = 662.70 - 2336 = -1673.30

Original Angle: -126.51°
Boosted Angle: -111.83°

Final Velocity: (-130.89, -326.82)
  X: -130.89 = Going LEFT ✅
  Y: -326.82 = Going UP ⚠️ TOO VERTICAL
```

### **Frame 10 (FACING RIGHT)**
```
Player CENTER: (332, 986.86)
Snowman pos: (1628.67, 2236)
Spawn pos: (1578.67, 2336)

Delta Calculation:
dx = 332 - 1578.67 = -1246.67
dy = 986.86 - 2336 = -1349.14

Original Angle: -132.74°
Boosted Angle: -115.60°

Final Velocity: (-149.31, -311.58)
  X: -149.31 = Going LEFT ✅
  Y: -311.58 = Going UP ⚠️ TOO VERTICAL
```

---

## 🔍 PATTERN ANALYSIS

### **Consistent Behaviors:**
1. **Player X is ALWAYS 332** - Player doesn't move horizontally (or we're always reading same value)
2. **Player Y varies** - 212 to 1070 (player bobbing/moving vertically)
3. **Snowman positions vary** - 646 to 1628 (scrolling across screen)
4. **Spawn Y is ALWAYS 2336** - Consistent spawn height (hand position)
5. **All dx are NEGATIVE** - Snowballs always need to go LEFT to hit player
6. **All dy are NEGATIVE** - Player is always ABOVE spawn point

### **Velocity Analysis:**
| Frame | Velocity X | Velocity Y | Ratio Y/X | Direction |
|-------|-----------|-----------|-----------|-----------|
| 1 | -135.70 | -323.17 | 2.38 | UP-LEFT, steep |
| 2 | -127.99 | -328.94 | 2.57 | UP-LEFT, steeper |
| 7 | -100.08 | -345.92 | 3.46 | UP-LEFT, very steep |
| 8 | -121.25 | -333.57 | 2.75 | UP-LEFT, steep |
| 9 | -130.89 | -326.82 | 2.50 | UP-LEFT, steep |
| 10 | -149.31 | -311.58 | 2.09 | UP-LEFT, less steep |

**Average Y/X Ratio: 2.63** - This means for every 1 pixel left, goes 2.63 pixels up!
This is EXTREMELY vertical/steep trajectory.

---

## 🎯 ROOT CAUSE ANALYSIS

### **Problem 1: Upward Boost Direction is WRONG**
```cpp
// Current code:
finalDirection.y += upwardBoost;  // upwardBoost = -150

// What happens:
// dy is already negative (player above spawn)
// After normalization: direction.y is negative (going up)
// Adding -150 makes it MORE negative (even more upward!)
// Result: Too steep, almost vertical throws
```

**The boost should ADD to the magnitude, not change direction!**

### **Problem 2: Snowballs Should Arc DOWN, Not Up**
The snowballs spawn at Y=2336 (bottom) and player is at Y=200-1000 (top).
They need to:
1. Go UP initially to reach player height
2. Arc DOWN due to gravity
3. Hit player

BUT: With negative Y velocity (-300 to -350) and gravity pulling DOWN (+200 px/s²),
the snowball goes UP very fast, slows down, then falls. By the time it comes down,
it's way past the player horizontally!

### **Problem 3: No Gravity Compensation in Angle**
We're aiming directly at player's current position, but not accounting for:
- Gravity pulling down at 200 px/s²
- Flight time to reach player
- Parabolic trajectory needed

---

## 🔧 WHAT THE OLD SCRIPT DID DIFFERENTLY

Looking at `SnowmanEnemy.cpp` line 78:
```cpp
Vector2 vel = Vector2Scale(Vector2Normalize(delta), 220.0f);
```

**No boost at all!** Just:
1. Calculate delta (player - spawn)
2. Normalize it
3. Scale by 220

**But they had line 73-75:**
```cpp
// skip overly-vertical throws
if (fabsf(delta.x) < fabsf(delta.y) * 0.3f)
    return;
```

This REJECTS throws where horizontal < 30% of vertical!
In our case, ALL throws would be rejected because:
- Frame 1: |dx|=913, |dy|=1165, ratio = 913/1165 = 0.78 ✅ PASS
- Frame 2: |dx|=912, |dy|=1276, ratio = 912/1276 = 0.71 ✅ PASS
- Frame 7: |dx|=855, |dy|=1675, ratio = 855/1675 = 0.51 ✅ PASS

Wait, they all pass the 0.3 threshold. So that's not the issue.

---

## 💡 RECOMMENDED FIXES

### **Fix 1: REMOVE THE UPWARD BOOST**
```cpp
// DELETE THIS:
const float upwardBoost = -150.0f;
finalDirection.y += upwardBoost;

// Just use normalized direction * speed:
finalDirection.x = (dx / length) * 220.0f;
finalDirection.y = (dy / length) * 220.0f;
// Let gravity handle the arc naturally!
```

### **Fix 2: Reset Flip State on Wrap**
In `LevelManager::UpdateEnemyPooling`, you already have:
```cpp
// Reset flip state to default (facing left)
t->scale.x = std::abs(t->scale.x);
enemyComp->isFacingRight = false;
```
✅ This is already implemented!

### **Fix 3: Fix Flip Position Compensation**
The compensation is BACKWARDS! When flipping:
- Sprite anchor is top-left
- When scale.x flips from +1 to -1, sprite renders flipped around left edge
- This DOESN'T need position adjustment for visual consistency

**REMOVE flip compensation entirely** - it's causing the jump!

---

## 📈 PREDICTIONS AFTER FIXES

**Without upward boost (-150px):**
- Frame 1: velocity = (-135.70, -173.17) instead of (-135.70, -323.17)
- Frame 7: velocity = (-100.08, -195.92) instead of (-100.08, -345.92)

**Y/X Ratios would improve:**
- From 2.38 → 1.28 (Frame 1)
- From 3.46 → 1.96 (Frame 7)

**Result:** More horizontal throws, less vertical, better chance of hitting player!
