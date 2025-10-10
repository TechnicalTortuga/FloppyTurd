# Remaining Issues - October 8, 2025

## Status Update
✅ **FIXED**: No crashes on scene transitions  
✅ **FIXED**: Rat King appears on boss level  
✅ **PARTIALLY FIXED**: RatCopters spawn but behavior needs work  

---

## 🔴 Critical Issues Still Needed

### 1. Snow Level Backgrounds Not Rendering
**Problem:** Snow level backgrounds still not appearing despite adding `.png` extensions
**Assets Available:**
- `SnowLevelBackLayerBackground.png`
- `SnowLevelMidLayerBackground.png`
- `SnowLevelFrontLayerBackground.png`
- `SnowLevelFrontLayerTrees.png`

**Action:** Need to check if texture loading is failing. Verify asset catalog has these textures.

---

### 2. Only One Type of Snowman Appearing
**Problem:** Not seeing variety of snowmen despite pool initialization
**Assets Available:**
1. `SnowManChill.png` - Decorative
2. `SnowManGreen.png` - Decorative
3. `SnowManChad.png` - Decorative
4. `SnowManIdle.png` - The red one (throws snowballs)
5. `SnowManThrow.png` - Throw animation

**Current Config:** 
- SnowManChill, SnowManGreen, SnowManChad are decorative (no behavior)
- SnowManIdle is the enemy that throws

**Action:** Verify all 4 configs are in the level config and being added to pool

---

### 3. Castle Curtains Overlapping/Flickering
**Problem:** Curtains overlapping castle background and flickering
**Cause:** Both layers at same scroll speed with same render layer?
**Action:** Check render layers and positioning for castle background layers

---

### 4. Grounding Issues

#### 4.1 Rat King Too Low
**Problem:** Rat King positioned too far down
**Current:** Spawns at `screenHeight * 0.40f` (40% from top)
**Needed:** `screenHeight - (128 * scale)` to ground him properly
**Action:** Fix boss spawn Y position in `LevelManager::InitializeEnemyPool()`

#### 4.2 Snowmen Too High
**Problem:** Snowmen floating, need to be grounded
**Current:** Spawning at `screenHeight * 0.3f + offset`
**Needed:** `screenHeight - (enemyHeight)` for grounded enemies
**Action:** Fix `SpawnInitialEnemies()` to ground snowmen properly

---

### 5. RatCopter Behavior Wrong
**Problem:** RatCopters "zipping really weird"
**Current Behavior:** Moving left at constant speed
**Needed Behavior:**
1. **Hover phase:** Move left slowly while hovering vertically
2. **Position constraint:** Stay between `(0 + padding)` and `(screenHeight - height - padding)`
3. **Trigger at 75% screen:** When reaching 75% of screen width, begin beeline
4. **Beeline phase:** Shoot toward player position

**Speed:** Currently 60.0f, may need adjustment for hover vs beeline

**Action:** Implement two-phase behavior in `EnemySystem` or specialized RatCopter movement

---

### 6. Rat King Not Animating/Moving
**Problem:** Rat King static, not walking or throwing projectiles
**Expected Behavior:**
- **Idle/Walk:** Periodically walk left and right within boundaries
- **Aim:** Lock onto player position
- **Throw:** Fire projectile after aiming

**Current:** BossSystem expects to manage this, but may not be triggering states
**Action:** Verify BossSystem is updating and changing states correctly

---

### 7. Enemy Pool & Patterns

#### 7.1 Pool Too Small
**Problem:** Only 10 enemies in pool, not enough for patterns
**Requested:** 16 enemies for better pattern variety
**Action:** Change `MAX_ENEMY_POOL_SIZE` from 10 to 16

#### 7.2 No Pattern Randomization
**Problem:** Enemies wrap around but don't form new patterns
**Current:** Enemies just wrap to right side individually
**Needed:** 
- Group enemies into patterns when spawning
- Randomize Y positions on wrap
- Create formations (V-shape for birds, clusters for snowmen, etc.)

**Action:** Implement pattern system similar to obstacle groups

---

## Implementation Priority

1. **HIGH**: Fix grounding for Rat King and Snowmen
2. **HIGH**: Fix snow level backgrounds
3. **HIGH**: Increase enemy pool to 16
4. **MEDIUM**: Fix RatCopter behavior (hover + beeline)
5. **MEDIUM**: Verify all snowman types are spawning
6. **MEDIUM**: Fix castle curtain flickering
7. **LOW**: Rat King animation/movement (BossSystem already handles, just verify)
8. **LOW**: Enemy pattern randomization on wrap

---

## Technical Notes

### Grounding Formula
```cpp
// For grounded enemies:
float enemyHeight = spriteHeight * transform->scale.y;
float groundY = screenHeight - enemyHeight;
transform->position.y = groundY;
```

### RatCopter Behavior States
```cpp
enum class RatCopterState {
    Hovering,    // Move left while bobbing, constrained Y
    Beelining    // Lock onto player, move fast
};

// Trigger condition:
if (transform->position.x <= screenWidth * 0.75f) {
    state = Beelining;
}
```

### Enemy Pool Distribution
With 16 enemies and 4 snowman types:
- 4 of each type in pool
- Spawn 5-6 at a time
- On wrap, reposition with pattern logic


