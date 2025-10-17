# Snow Level Comprehensive Fixes & Debugging Guide

## Issues Addressed

### 1. ✅ **Horizontal Spacing Between Toilet Pairs** (FIXED)
**Problem**: Spacing between toilet PAIRS horizontally was too small - you wanted more room for coins BETWEEN the pairs.

**Solution**: 
- Changed snow level pattern spacing from `0.0f` to `1800.0f` in `ObstacleSystem.cpp` line 68
- Updated `GroupGap` component from `1500.0f` to `1800.0f` for snow toilet pairs (line 2246)
- This creates 1800px horizontal spacing between each toilet pair group

**Location**: `ObstacleSystem.cpp:67-68`, `ObstacleSystem.cpp:2246`

---

### 2. ✅ **Snowman Ground Positioning** (FIXED)
**Problem**: Snowmen were floating too high - not properly grounded at bottom of screen.

**Root Cause**: Positioning was incorrectly using SCALED height instead of RAW sprite height. Transform positioning uses raw pixel coordinates, then rendering applies scale separately.

**Solution**:
- Changed from: `float snowmanHeight = matchingConfig->frameHeight * matchingConfig->scale;` 
- Changed to: `float rawSpriteHeight = matchingConfig->frameHeight;  // 64px`
- Position calculation: `y = screenHeight - rawSpriteHeight` (e.g., 2556 - 64 = 2492px)
- **Rendering applies the 8x scale separately!**

**Location**: `LevelManager.cpp:1507-1521`

**Debug Logs Added**:
```cpp
GN_LOG_INFO("LevelManager: Grounding snowman '" + enemyComp->enemyType + "' at Y=" + std::to_string(y) + 
           " (screenHeight=" + std::to_string(screenInfo.pixelHeight) + 
           ", rawSpriteHeight=" + std::to_string(rawSpriteHeight) + "px" +
           ", renderScale=" + std::to_string(matchingConfig->scale) + "x" +
           ", bottomEdge=" + std::to_string(y + rawSpriteHeight) + "px = GROUND)");
```

---

### 3. ✅ **Snowman Spawn Ratios** (FIXED)
**Problem**: All 4 snowman types spawning equally (25% each), making red thrower too common.

**Solution**:
- Implemented custom spawn ratios for snow level (ID 4):
  - **Red Snowman (SnowManIdle)**: 15% - RARE attacker
  - **SnowManChill**: ~28% - Common decorative
  - **SnowManGreen**: ~28% - Common decorative
  - **SnowManChad**: ~29% - Common decorative

**Location**: `LevelManager.cpp:307-350`

---

### 4. ✅ **Snowman Throwing Behavior** (FIXED + EXTENSIVE LOGGING)
**Problem**: Red snowman wasn't throwing snowballs - the `UpdateSnowmanThrower()` function was never being called!

**Solution**:
- Integrated `UpdateSnowmanThrower()` into main `EnemySystem::Update()` loop
- Added comprehensive logging with `[SNOWMAN]` prefix for easy filtering

**Location**: `EnemySystem.cpp:63-66`

**Debug Logs Added**:
```cpp
// When snowman comes on screen:
GN_LOG_INFO("[SNOWMAN] Red snowman X came ON SCREEN at x=Y, starting throw timer");

// Player range checking:
GN_LOG_INFO("[SNOWMAN] Checking player range - playerEntity=X, levelManager=VALID/NULL");
GN_LOG_INFO("[SNOWMAN] Player at x=X, enemy at x=Y, distance=Zpx, throwRange=400px");

// When in range:
GN_LOG_INFO("[SNOWMAN] ✅ PLAYER IN RANGE! Distance=X <= throwRange=Y");
GN_LOG_INFO("[SNOWMAN] Player in range - throw timer=Xs / cooldown=2.0s");

// When throwing:
GN_LOG_INFO("[SNOWMAN] 🎯 THROWING SNOWBALL! Timer reached: Xs");
```

---

### 5. ✅ **Coin Positioning Between Pipes** (FIXED + LOGGING)
**Problem**: Coins weren't appearing at top and bottom of screen between toilet pairs.

**Solution**:
- Already implemented `TopAndBottom` pattern for snow level toilets
- Added extensive logging to verify coin placement

**Location**: `ObstacleSystem.cpp:1337-1364`

**Debug Logs Added**:
```cpp
GN_LOG_INFO("ObstacleSystem::TopAndBottom 🪙 TOP coin row at Y=X (topBandBottom=Y + verticalPad=Z), X span=[A to B]");
GN_LOG_INFO("ObstacleSystem::TopAndBottom 🪙 BOTTOM coin row at Y=X (bottomBandTop=Y - verticalPad=Z), X span=[A to B]");
GN_LOG_INFO("ObstacleSystem::TopAndBottom SUMMARY for groupId=X: TOP row Y=A, BOTTOM row Y=B, GAP=Cpx, total coins=D");
```

---

### 6. ✅ **Vertical Gap Within Toilet Pair** (ALREADY CORRECT)
**Current Setting**: 1100px vertical gap between top and bottom toilet in a pair (set in previous fix)
- This provides ample room for coin rows between the pipes
- Top toilet oscillates with its pair, maintaining the gap

**Location**: `ObstacleSystem.cpp:2142`

---

## Architecture Notes

### Scale vs. Position
**CRITICAL UNDERSTANDING**:
- **Transform.position**: Raw pixel coordinates (e.g., sprite is 64px tall)
- **Transform.scale**: Multiplier applied during rendering (e.g., 8.0x)
- **Rendering**: Takes position + applies scale (64px × 8.0 = 512px on screen)

**Snowman Example**:
```
Raw sprite: 64×64 pixels
Scale: 8.0x
Screen height: 2556px

Position Y calculation:
y = 2556 - 64 = 2492px (TOP-LEFT corner of sprite)
Bottom edge: 2492 + 64 = 2556px (GROUND)

Rendered on screen:
- Top-left at Y=2492
- Sprite renders as 512×512 (64×8.0)
- Bottom edge still at 2556px (ground)
```

### Player Entity Reference Flow
```
GameplayState::OnEnter()
  └─> Creates player entity
      └─> m_playerControllerSystem->SetPlayerEntity(player)
      └─> m_levelManager->SetPlayerEntity(player)
      └─> m_pickupSystem->SetPlayerEntity(player)

EnemySystem::UpdateSnowmanThrower()
  └─> playerEntity = m_levelManager->GetPlayerEntity()
      └─> If valid, get Transform and check distance
      └─> If in range, increment throw timer
      └─> If timer >= cooldown, throw snowball
```

---

## Debugging Commands

### Search Logs for Snowman Behavior
```bash
# All snowman-related logs
grep "\[SNOWMAN\]" path/to/logfile.txt

# Snowman coming on screen
grep "came ON SCREEN" path/to/logfile.txt

# Player range checks
grep "PLAYER IN RANGE" path/to/logfile.txt

# Throwing events
grep "THROWING SNOWBALL" path/to/logfile.txt

# Snowman spawn ratios
grep "Snow level spawn ratios" path/to/logfile.txt
```

### Search Logs for Coin Positioning
```bash
# Coin row placement
grep "🪙 TOP coin row" path/to/logfile.txt
grep "🪙 BOTTOM coin row" path/to/logfile.txt

# Coin positioning summary
grep "TopAndBottom SUMMARY" path/to/logfile.txt
```

### Search Logs for Enemy Positioning
```bash
# Snowman grounding
grep "Grounding snowman" path/to/logfile.txt

# Enemy spawn positions
grep "Spawned enemy type" path/to/logfile.txt
```

---

## Expected Behavior After Fixes

### Snow Level Gameplay
1. **Snowman Variety**: 
   - Mostly see blue/green/chad decorative snowmen (~85%)
   - Occasional red thrower snowman (~15%)

2. **Red Snowman Behavior**:
   - Slides from right to left on ground
   - When player gets within 400px, starts throw timer
   - Throws snowball every 2 seconds when in range
   - 6-frame throw animation plays
   - Snowball spawns at frame 3 of animation

3. **Pipe Spacing**:
   - **Vertical gap within pair**: 1100px (ample room for coins)
   - **Horizontal gap between pairs**: 1800px (plenty of space to navigate)

4. **Coin Patterns**:
   - **Top row**: Below top pipe, with vertical padding
   - **Bottom row**: Above bottom pipe, with vertical padding
   - **5 coins per row** (or fewer if space is tight)
   - Coins spawn horizontally across pipe width

5. **Snowman Positioning**:
   - All snowmen (decorative and thrower) ground at bottom of screen
   - Raw position: Y = 2556 - 64 = 2492px
   - Rendered position: Bottom edge at 2556px (perfect ground alignment)

---

## Files Modified

1. **LevelManager.cpp**:
   - Lines 307-350: Custom spawn ratios for snow level
   - Lines 1507-1521: Fixed snowman ground positioning with raw sprite height

2. **EnemySystem.cpp**:
   - Lines 63-66: Integrated UpdateSnowmanThrower into main loop
   - Lines 81-154: Added extensive logging for snowman behavior

3. **ObstacleSystem.cpp**:
   - Line 68: Set horizontal spacing to 1800px for snow level
   - Line 2142: Vertical gap within pair already at 1100px
   - Line 2246: GroupGap updated to 1800px
   - Lines 2253-2254: Added logging for toilet pair spawning
   - Lines 1337-1364: Added coin positioning logs for TopAndBottom pattern

---

## Testing Checklist

- [ ] Snowmen appear grounded at bottom of screen (not floating)
- [ ] Mostly see decorative snowmen (blue, green, chad)
- [ ] Rarely see red thrower snowman (~15% spawn rate)
- [ ] Red snowman throws snowballs when player approaches
- [ ] Check logs for `[SNOWMAN]` entries showing player detection
- [ ] Check logs for `🎯 THROWING SNOWBALL` when in range
- [ ] Horizontal spacing between toilet pairs is generous (~1800px)
- [ ] Vertical gap within toilet pair allows easy navigation (~1100px)
- [ ] Coins appear in TWO horizontal rows (top and bottom of gap)
- [ ] Check logs for `🪙 TOP coin row` and `🪙 BOTTOM coin row`
- [ ] Coins don't overlap with pipes

---

## Next Steps If Issues Persist

1. **Check log file** in app Documents folder:
   ```bash
   # Find app container
   xcrun simctl get_app_container booted com.floppyturd.game
   
   # View logs
   cat /path/to/container/Documents/game_log.txt | grep "\[SNOWMAN\]"
   ```

2. **Verify player entity**:
   - Look for "playerEntity=0" warnings in logs
   - Should see "playerEntity=X" where X > 0

3. **Verify snowman detection**:
   - Look for "came ON SCREEN" logs
   - Should see distance calculations when snowman is visible

4. **Verify coin spawning**:
   - Look for "TOP coin row" and "BOTTOM coin row" logs
   - Check Y coordinates match expected screen positions

---

**All fixes completed with extensive logging for debugging! 🎮❄️**
