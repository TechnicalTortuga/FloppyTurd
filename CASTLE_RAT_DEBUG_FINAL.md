# Castle Level: Rat Behavior & Decoration Debug Guide

## Date: October 18, 2024

## Overview
This document provides a comprehensive debugging guide for Castle level (Level 5) issues, including detailed logging locations and what to look for in the debug file.

---

## 🔍 Debug Log File Location

**Path**: `~/Library/Developer/CoreSimulator/Devices/[DEVICE_ID]/data/Containers/Data/Application/[APP_ID]/Documents/FloppyTurd_Debug.txt`

**Quick Access**:
1. Run the app in iPhone 16 Simulator
2. Navigate to Castle level (Level 5)
3. Check Xcode console for log file path, or use:
   ```bash
   xcrun simctl get_app_container booted com.yourcompany.FloppyTurd data
   ```
4. Open `Documents/FloppyTurd_Debug.txt`

**Note**: Logging is enabled for Simulator builds only (see `iOSLogHandler.swift:335-348`)

---

## 🐀 Issue 1: Rat Erratic Behavior

### Symptoms Reported:
- Rats flying all over the place
- Movement not predictable or robust
- Still aiming behind player

### Debug Logging Added:

#### A. **Initial Spawn** (`LevelManager.cpp:1650-1670`)
```
[RATCOPTER_INIT] ===== SPAWN =====
[RATCOPTER_INIT] Pos=(X, Y), BaseY=Y, State=FLY_IN
[RATCOPTER_INIT] Speed=120, BobAmplitude=XXX, BobSpeed=XXX
[RATCOPTER_INIT] =================
```

**What to check**:
- ✅ `BaseY` should be between 25%-60% of screen height (639-1534 for iPhone 16)
- ✅ `Speed` should be 120.0
- ✅ `BobAmplitude` should be reasonable (20-40 pixels)
- ⚠️ If `BaseY` is outside range, rats will hit screen bounds during hover

#### B. **FLY_IN State** (`EnemySystem.cpp:799-817`)
```
[RATCOPTER FLY_IN] Pos=(X,Y), Speed=120, Delta=0.016, MovedBy=1.92
```
*Logged every 100 frames*

**What to check**:
- ✅ `MovedBy` should be ~1.92 pixels/frame (120 * 0.016)
- ✅ X should decrease steadily (moving left)
- ✅ Y should stay CONSTANT (no bobbing during fly-in)
- ⚠️ If Y changes during FLY_IN, bobbing is still active (BUG!)

#### C. **FLY_IN → HOVER Transition** (`EnemySystem.cpp:820-825`)
```
[RATCOPTER FLY_IN→HOVER] At 85.2% screen, timer=0.83s, Y=1024
```

**What to check**:
- ✅ Screen position should be ~85% (within 84-86%)
- ✅ Timer should be 0.75-1.0 seconds
- ✅ Y position should be valid (639-1534 for iPhone 16)
- ⚠️ If triggering at wrong position, rats will be too far left or right

#### D. **HOVER State** (`EnemySystem.cpp:820-862`)
```
[RATCOPTER HOVER] Pos=(X,Y), BaseY=1024, Timer=0.45s, BobAmplitude=30
```
*Logged every 30 frames*

**Warning logs if Y gets constrained**:
```
[RATCOPTER HOVER] Y constrained! baseY=1024, bobOffset=50, unconstrained=1574, final=1456
```

**What to check**:
- ✅ X should stay CONSTANT (not moving left during hover)
- ✅ Y should oscillate around BaseY within ±BobAmplitude
- ✅ Timer should count down from initial value to 0
- ⚠️ If seeing "Y constrained!" warnings, BaseY is wrong or amplitude too large
- ⚠️ If X is changing, velocity is not zeroed during hover (BUG!)

#### E. **HOVER → PULLBACK Transition** (`EnemySystem.cpp:840-926`)
```
[RATCOPTER TARGET] RatCenter(950,1024) → PlayerCenter(200,1200) | Dir(-0.85,0.53)
[RATCOPTER HOVER→PULLBACK] Pullback vector=(68,-42), Duration=0.25s
```

**What to check**:
- ✅ `RatCenter` should be rat's CENTER (not top-left)
- ✅ `PlayerCenter` should be player's CENTER (not top-left)
- ✅ `Dir` should be normalized (sqrt(x²+y²) ≈ 1.0)
- ✅ Direction should point LEFT and TOWARD player Y (negative X is correct)
- ✅ `Pullback vector` should be OPPOSITE of Dir, scaled by 80
- ⚠️ If Dir is positive X or wrong Y, targeting calculation is broken!
- ⚠️ If PlayerCenter is behind rat, player is wrapping or position is stale

#### F. **PULLBACK State** (`EnemySystem.cpp:930-957`)
```
[RATCOPTER PULLBACK] Pos=(1018,982), MovedBy=(68,-42), Timer=0.12s
```
*Logged every 5 frames*

**What to check**:
- ✅ `MovedBy` should match pullback vector scaled by (deltaTime * 4.0)
- ✅ Should move for exactly 0.25 seconds (timer 0.25 → 0.0)
- ✅ Should move AWAY from player (opposite of target direction)
- ⚠️ If MovedBy is erratic, deltaTime is unstable or calculation is wrong

#### G. **PULLBACK → BEELINE Transition** (`EnemySystem.cpp:950-957`)
```
[RATCOPTER PULLBACK→BEELINE] Speed=300, Dir=(-0.85,0.53), StartPos=(1086,940)
```

**What to check**:
- ✅ Speed should be 300.0 (fast aggressive charge)
- ✅ Dir should be SAME as locked direction from hover
- ✅ StartPos should be pullback end position
- ⚠️ If Dir changed, direction was not locked properly!

#### H. **BEELINE State** (`EnemySystem.cpp:963-986`)
```
[RATCOPTER BEELINE] Pos=(831,1108), Speed=300, Dir=(-0.85,0.53), MovedBy=(-4.08,2.54)
```
*Logged every 20 frames*

**What to check**:
- ✅ `MovedBy` should be Dir * Speed * deltaTime (~-4.08, 2.54 per frame)
- ✅ Should charge in STRAIGHT LINE toward locked position
- ✅ Dir should NEVER change during beeline
- ⚠️ If MovedBy varies wildly, speed or direction is changing (BUG!)
- ⚠️ If rat curves, direction is being recalculated (BUG!)

#### I. **BEELINE → OFFSCREEN** (`EnemySystem.cpp:983-986`)
```
[RATCOPTER BEELINE→OFFSCREEN] At (-150,1450), will wrap
```

**What to check**:
- ✅ Should trigger when X < -100 or Y < -100 or X > screenWidth+100 or Y > screenHeight+100
- ⚠️ If doesn't trigger, rats will accumulate offscreen!

#### J. **Wrap/Reset** (`LevelManager.cpp:676-692`)
```
[RATCOPTER_WRAP] ===== COMPLETE RESET =====
[RATCOPTER_WRAP] NewPos=(1829,856), BaseY=856, State=FLY_IN
[RATCOPTER_WRAP] BobAmplitude=30, BobSpeed=1.5, BobPhase=3.14
[RATCOPTER_WRAP] ===========================
```

**What to check**:
- ✅ NewPos.X should be ~1829 (screenWidth + 650)
- ✅ NewPos.Y and BaseY should be 25%-60% of screen (639-1534)
- ✅ State should reset to FLY_IN
- ✅ All timers should be zeroed
- ✅ BobPhase should be randomized (prevents sync)
- ⚠️ If BaseY not updated, rat will bob to wrong Y during next hover!

---

## 🪙 Issue 2: Coin Spread Too Narrow

### Symptoms Reported:
- Coins not spread enough for 2000px castle gap
- Should match pillar/spikeball spread pattern

### Current Configuration (`ObstacleSystem.cpp:1388-1403`):

```cpp
// Castle: 800px expansion (was 350px)
float coinHalfWidth = ((maxX - minX) / 2.0f) + 800.0f;
```

**Debug Logging**:
```
[ObstacleSystem::Castle TopAndBottom [SCREEN EDGES] TOP row at Y=200, BOTTOM row at Y=2306, 
 X span CENTERED+WIDENED=[500 to 2100], total coins=10
```

**What to check**:
- ✅ Coins should span ~1600px (800px on each side of center)
- ✅ Y positions: Top=200, Bottom=2306 (screen edges like Snow level)
- ✅ Total coins should be 10 (5 top, 5 bottom)
- ⚠️ If span is < 1600px, expansion is too small
- ⚠️ If Y not at screen edges, wrong pattern selected

---

## 🕯️ Issue 3: Chandelier & Centerpiece Positioning

### Symptoms Reported:
- Right chandelier needs to move left
- Paintings not centered

### Debug Logging Added (`ObstacleSystem.cpp:2610-2630`):

```
[CASTLE_CENTERPIECE] Calculating center position:
[CASTLE_CENTERPIECE]   Toilet at X=1000, toiletWidth=520px
[CASTLE_CENTERPIECE]   Gap: start=1000, end=3000, width=2000px
[CASTLE_CENTERPIECE]   TRUE CENTER X = 1740 (gapStart + toiletWidth + (gapWidth - toiletWidth) * 0.5)

[CASTLE_CHANDELIER] Left at X=1340 (center - 400px)
[CASTLE_CHANDELIER] Right at X=2040 (center + 300px)  ← MOVED LEFT from +400
[CASTLE_CHANDELIER] Offsets: left=584, right=1284
```

**What to check**:
- ✅ TRUE CENTER should be: toiletX + toiletWidth + ((gapWidth - toiletWidth) / 2)
- ✅ Left chandelier: CENTER - 400px
- ✅ Right chandelier: CENTER + 300px (asymmetric per user request)
- ⚠️ If centerpieces (pillar/painting/spikeball) not at TRUE CENTER, calculation is wrong

### Centerpiece-Specific Logs:

#### Torch Pillar (`ObstacleSystem.cpp:2757`):
```
[CASTLE_PILLAR] Spawned at X=1692 (center=1740, pillarWidth=768), Y=1278, group=5
```
**Note**: X is top-left, so centerX - (pillarWidth/2) = displayed X

#### Painting (`ObstacleSystem.cpp:2890-2893`):
```
[CASTLE_PAINTING] Spawned CENTERED at X=1740 (sprite top-left=1452, width=576), Y=990, group=5
```
**Note**: Now properly centered! Sprite top-left = centerX - (width/2)

#### Spike Ball (`ObstacleSystem.cpp:3069`):
```
Spawned castle spike ball obstacle CENTERED at x=1740 y=1278 with group 5
```

---

## 🎯 Expected Behavior Summary

### RatCopter State Machine Flow:
```
1. SPAWN at (screenWidth + 650, 25%-60% height)
2. FLY_IN: Move left at 120 u/s, NO BOBBING, Y stays constant
3. HOVER at 85% screen: Bob ±amplitude for 0.75-1.0s, lock player center
4. PULLBACK: Move opposite direction 80 units over 0.25s
5. BEELINE: Charge at 300 u/s in locked direction until offscreen
6. WRAP: Reset to step 1 with new random Y
```

### Coin Layout:
- **Top row**: Y=200 (near top)
- **Bottom row**: Y=2306 (near bottom)
- **Horizontal**: Centered in 2000px gap, spread ±800px from center

### Centerpiece Positioning:
- **Calculate**: centerX = toiletX + toiletWidth + ((gapWidth - toiletWidth) / 2)
- **Pillar/Painting/SpikeBall**: All at centerX (true center of gap)
- **Left Chandelier**: centerX - 400px
- **Right Chandelier**: centerX + 300px

---

## 🔧 Quick Debug Checklist

When debugging rats:
- [ ] Check spawn Y is in 639-1534 range (25-60% of 2556px screen)
- [ ] Verify Y stays CONSTANT during FLY_IN (no bobbing)
- [ ] Confirm hover triggers at ~85% screen position
- [ ] Watch for "Y constrained!" warnings (indicates baseY problem)
- [ ] Verify target direction points LEFT (negative X)
- [ ] Check pullback vector is OPPOSITE of target direction
- [ ] Confirm beeline direction NEVER changes
- [ ] Verify wrap resets baseY to new spawn position

When debugging positioning:
- [ ] Check [CASTLE_CENTERPIECE] logs for TRUE CENTER calculation
- [ ] Verify all centerpieces spawn at same centerX
- [ ] Confirm chandeliers are -400px (left) and +300px (right) from center
- [ ] Check coin span is ~1600px total (±800 from center)

---

## 📊 Performance Notes

All rat logging uses `GN_LOG_INFO` level, which:
- ✅ Enabled in Simulator builds
- ✅ Writes to `FloppyTurd_Debug.txt`
- ❌ Disabled in Device builds (performance)

Logs are throttled:
- FLY_IN: Every 100 frames (~6 logs/second)
- HOVER: Every 30 frames (~20 logs/second)
- PULLBACK: Every 5 frames (~60 logs/second)
- BEELINE: Every 20 frames (~30 logs/second)

This prevents log spam while still capturing state transitions and issues.

---

## 🐛 Known Issues to Watch For

1. **BaseY Not Updated on Wrap**: Rat bobs to wrong Y after wrap
   - **Symptom**: Rat spawns at Y=1000, wraps, then bobs toward old Y=1000 instead of new Y
   - **Check**: `[RATCOPTER_WRAP]` log shows `BaseY` matches `NewPos.Y`

2. **Direction Not Locked**: Rat curves during beeline
   - **Symptom**: MovedBy values change direction during BEELINE state
   - **Check**: `Dir` stays constant across all BEELINE logs

3. **Y Constrained During Hover**: Amplitude or baseY wrong
   - **Symptom**: `[RATCOPTER HOVER] Y constrained!` warnings
   - **Check**: baseY + bobAmplitude fits in 100px top/bottom padding

4. **Velocity Not Zeroed During Hover**: Rat drifts left while hovering
   - **Symptom**: X position changes in HOVER logs
   - **Check**: X stays constant across all HOVER logs for same rat

5. **Targeting Uses Top-Left Instead of Center**: Rats aim behind player
   - **Symptom**: Direction vector points wrong way
   - **Check**: `[RATCOPTER TARGET]` shows sensible RatCenter and PlayerCenter values

---

## 📝 Files Modified

1. **EnemySystem.cpp**: Added comprehensive rat state machine logging
2. **LevelManager.cpp**: Added spawn and wrap logging
3. **ObstacleSystem.cpp**: 
   - Increased coin spread (350 → 800px)
   - Fixed painting centering
   - Moved right chandelier left (400 → 300px from center)
   - Added positioning debug logs

**Build Status**: ✅ **BUILD SUCCEEDED**

---

## 🎮 Testing Instructions

1. **Launch iPhone 16 Simulator**
2. **Start Castle Level (Level 5)**
3. **Play for 30-60 seconds** (let rats spawn, hover, attack, wrap)
4. **Locate log file**:
   ```bash
   find ~/Library/Developer/CoreSimulator -name "FloppyTurd_Debug.txt" -mtime -1
   ```
5. **Search for issues**:
   ```bash
   grep "RATCOPTER" FloppyTurd_Debug.txt | grep -E "(constrained|ERROR|WARN)"
   grep "CASTLE_" FloppyTurd_Debug.txt
   ```

6. **Look for patterns**:
   - Are rats completing full state cycle? (FLY_IN → HOVER → PULLBACK → BEELINE → WRAP)
   - Are Y positions consistent? (no jumping between states)
   - Are directions locked? (same Dir throughout PULLBACK and BEELINE)
   - Are centerpieces aligned? (all at same centerX)

---

## 📧 Reporting Issues

When reporting rat behavior issues, please include:
1. **Log excerpt** showing full state cycle for ONE rat
2. **Screen recording** showing the erratic behavior
3. **Device info** (simulator model and iOS version)
4. **Specific observation** (e.g., "rat curves during beeline" or "rat spawns offscreen")

This will help correlate visual behavior with logged state data.