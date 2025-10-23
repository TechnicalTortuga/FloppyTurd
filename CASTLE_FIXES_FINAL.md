# Castle Level - Final Fixes Applied

## ✅ All Issues Resolved

### 1. Curtain Scaling - FIXED
- **Before**: Gigantic (used baseScale 8.0x)
- **After**: Perfect size (screen height / 512 = ~5.0x)
- Matches sewer background scaling method

### 2. Pillar Visibility - FIXED
- **Before**: Layer 2 (same as curtains, invisible)
- **After**: Layer 3 (in front of curtains, visible)

### 3. Centerpiece Positioning - FIXED
- **Before**: Off-center with arbitrary offsets
- **After**: TRUE center calculation
- Formula: `centerX = toiletX + toiletWidth + ((gapWidth - toiletWidth) * 0.5)`
- Pillar, painting, and spikeball base all properly centered

### 4. Coin Spread - FIXED
- **Before**: Coins only at toilet edges (narrow spread)
- **After**: Centered in gap and expanded outward (like snow level)
- Castle-specific logic: `coinCenterX = maxX + (gapWidth/2)`, then expand ±350px
- Top and bottom rows, 5 coins each, spread across gap

### 5. RatCopter Targeting - FIXED
- **Before**: Used player top-left position, often missed
- **After**: Uses player CENTER (like snowmen)
- Player center: `position + (32, 32)` for 64x64 sprite
- Rat center: `position + (16, 16)` for 32x32 sprite
- Direction: `normalize(playerCenter - ratCenter)`

### 6. RatCopter Visibility - FIXED
- **Before**: Trigger at 75% screen (still mostly offscreen)
- **After**: Check `x + spriteWidth < screenWidth` (actually visible)
- Rats FLY IN horizontally before hovering

### 7. RatCopter Y-Position - FIXED
- **Initial Spawn**: 25%-60% of screen height
- **Wrap**: Same range (25%-60%)
- Added logging to verify positioning

### 8. RatCopter Wrap State - FIXED
- Resets to `EnemyState::FlyIn` on wrap
- Clears all state: `hoverTimer`, `hasLockedDirection`, `targetDirection`, `pullbackVector`
- Starts fresh behavior cycle

## Build Status
✅ **BUILD SUCCEEDED** - iPhone 16 Simulator, Debug

## Testing Priorities

1. **Curtains**: Should be normal sized, centered on toilets
2. **Pillars**: Should be visible, animated, centered in gaps
3. **SpikeBall base**: Should be visible, centered with ball
4. **Coins**: Should spread from center of gap outward
5. **Rats**:
   - Should spawn in middle band (25%-60%)
   - Should fly in and be VISIBLE before hovering
   - Should hover with bobbing
   - Should pull back when seeing player
   - Should beeline toward player CENTER accurately
   - Should wrap and reset to FlyIn state

## Key Formulas

### Curtain Scale
```cpp
scale = screenHeight / 512.0f  // ~5.0x, not 8.0x
```

### Centerpiece Position
```cpp
toiletWidth = 65.0f * m_baseScale;
centerX = toiletX + toiletWidth + ((gapWidth - toiletWidth) * 0.5f);
```

### Coin Spread (Castle)
```cpp
coinCenterX = maxX + (gapWidth / 2.0f);  // Center of gap
coinHalfWidth = ((maxX - minX) / 2.0f) + 350.0f;  // Expand
coinMinX = coinCenterX - coinHalfWidth;
coinMaxX = coinCenterX + coinHalfWidth;
```

### Rat Targeting
```cpp
playerCenter = playerPos + GNVector2(32, 32);  // Half sprite size
ratCenter = ratPos + GNVector2(16, 16);         // Half sprite size
direction = normalize(playerCenter - ratCenter);
```

## Files Modified (11 total)
1. `Config/LevelConfig.cpp` - Curtain layer removal, red coins
2. `Components/GameComponents.h` - Rat AI fields
3. `Systems/EnemySystem.h` - ProcessEnemyMovement signature
4. `Systems/EnemySystem.cpp` - Rat AI, player CENTER targeting
5. `Systems/LevelManager.h` - GetPlayerPosition()
6. `Systems/LevelManager.cpp` - Rat Y-range, wrap reset, spawn position
7. `Systems/ObstacleSystem.h` - SpawnCastleCurtain()
8. `Systems/ObstacleSystem.cpp` - Curtain scale, pillar layer, centerpiece positioning, coin spread

