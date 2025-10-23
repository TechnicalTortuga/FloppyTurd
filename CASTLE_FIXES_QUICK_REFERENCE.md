# Castle Level Fixes - Quick Reference

## What Was Fixed

### 1. Curtains ✓
- **Before**: Parallax background layer with gaps
- **After**: Spawned as decorative obstacles, centered on each toilet pair
- **Position**: X-centered on toilets, Y spans full screen (0 to 4096px @ 8x scale)

### 2. Pillar Layer ✓
- **Before**: Missing or off-center
- **After**: Properly centered in gap between toilets
- **Position**: TRUE center (1000px from toilet in 2000px gap)
- **Layer**: 2 (behind game objects)

### 3. SpikeBall Base ✓
- **Before**: Missing or off-center
- **After**: Properly centered with spikeball
- **Position**: TRUE center, base top-left calculated from center point
- **Layer**: 2 (base) and 3 (ball)

### 4. RatCopter Positioning ✓
- **Before**: Fixed pixels 600-1400 (bottom offscreen, top too high)
- **After**: Screen-relative 25%-60% of height
- **Result**: All rats visible and well-positioned

### 5. RatCopter Behavior ✓
- **Before**: Just hovered in place
- **After**: Full AI state machine
- **States**: FLY_IN → HOVER → PULLBACK → BEELINE
- **Behavior**: See player, pull back, charge toward target

---

## Key Code Changes

### Files Modified (7 total)
1. `Config/LevelConfig.cpp` - Removed curtain background layer
2. `Components/GameComponents.h` - Added RatCopter AI fields
3. `Systems/EnemySystem.cpp` - Implemented state machine
4. `Systems/LevelManager.h` - Added GetPlayerPosition()
5. `Systems/LevelManager.cpp` - Rat Y-range, player position, state init
6. `Systems/ObstacleSystem.h` - Added SpawnCastleCurtain()
7. `Systems/ObstacleSystem.cpp` - Curtain spawn, centerpiece centering

### New Functions
- `ObstacleSystem::SpawnCastleCurtain(float x, int groupId)`
- `LevelManager::GetPlayerPosition() const`

### Modified Functions
- `ObstacleSystem::SpawnCastlePattern_GoldToiletPair()` - Curtain spawn, center calc
- `ObstacleSystem::SpawnCastleTorchPillar()` - Horizontal centering
- `ObstacleSystem::SpawnCastleSpikeBall()` - Horizontal centering
- `EnemySystem::ProcessEnemyMovement()` - RatCopter state machine
- `LevelManager::UpdateEnemyPooling()` - Rat Y-range (25%-60%)
- `LevelManager::SpawnEnemyWithConfig()` - RatCopter FlyIn state init

---

## Test It!

### Quick Test Commands
```bash
# Build
cd FloppyTurd
xcodebuild -project build_ios/FloppyTurd.xcodeproj -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16" clean build

# Run in Simulator
open /Applications/Xcode.app/Contents/Developer/Applications/Simulator.app
# Then launch FloppyTurd from Xcode or Simulator
```

### What to Look For
✓ Curtains appear centered on toilet pairs
✓ Curtains span from top to bottom of screen
✓ Pillar appears in center of gap (when spawned)
✓ SpikeBall + base appear in center of gap (when spawned)
✓ Both pillar and base are visible
✓ Rats fly in from right
✓ Rats hover around 75% screen
✓ Rats pull back when player is in view
✓ Rats charge toward player's position
✓ Rats wrap around and restart behavior

---

## Formulas

### Curtain Position
```cpp
curtainX = toiletX - (curtainWidth * 0.5) + (toiletWidth * 0.5)
curtainY = 0.0
```

### Centerpiece Position (TRUE CENTER)
```cpp
centerX = toiletX + (gapWidth * 0.5)  // 1000px from toilet
centerY = screenHeight * 0.5           // Vertically centered
```

### RatCopter Y-Range
```cpp
minY = screenHeight * 0.25  // 25% from top
maxY = screenHeight * 0.60  // 60% from top
```

---

## State Machine at a Glance

```
SPAWN → FLY_IN (move left to 75% screen)
     ↓
     HOVER (bob in place 0.75-1.0s)
     ↓
     PULLBACK (move back 20 units for 0.25s, lock player position)
     ↓
     BEELINE (charge at 150 speed toward locked position)
     ↓
     OFFSCREEN → WRAP → back to FLY_IN
```

---

## Debug Logs to Watch For

```
[RATCOPTER_INIT] Initialized RatCopter with FlyIn state
RatCopter: FLY_IN → HOVER (timer=0.85s)
RatCopter: HOVER → PULLBACK (target=500.0,1200.0)
RatCopter: PULLBACK → BEELINE (speed=150.0)
RatCopter: BEELINE → offscreen (will wrap)
```

---

## Constants

| Constant | Value | Where |
|----------|-------|-------|
| Gap Width | 2000px | ObstacleSystem |
| Castle Scale | 8.0x | LevelConfig |
| Rat Y Min | 25% screen | LevelManager |
| Rat Y Max | 60% screen | LevelManager |
| Hover Time | 0.75-1.0s | EnemySystem |
| Pullback Time | 0.25s | EnemySystem |
| Beeline Speed | 150.0 | EnemySystem |
| Curtain Size | 256x512 | Asset |

---

## Build Status
✅ **SUCCESS** - iPhone 16 Simulator, Debug configuration

---

## Documentation
- Full details: `CASTLE_LEVEL_FIXES_SUMMARY.md`
- State machine: `CASTLE_RATCOPTER_STATE_MACHINE.md`
- Visual layout: `CASTLE_LAYOUT_VISUAL.md`
