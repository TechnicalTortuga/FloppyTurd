# Performance Issues & Remaining Bugs - October 8, 2025

## 🔥 CRITICAL PERFORMANCE ISSUES

### 1. CameraSystem::UpdateParallaxLayers() - EVERY FRAME ALLOCATIONS

**File:** `src/FloppyTurd/Systems/CameraSystem.cpp` lines 99-323

**Problems:**
```cpp
// Line 101: Vector copy EVERY FRAME
auto parallaxEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite, Parallax, ParallaxInstance>();

// Line 111: Another vector copy EVERY FRAME (for debug logging)
auto basicParallaxEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite, Parallax>();

// Lines 118-119: Map allocations EVERY FRAME
std::map<int, std::vector<Gnosis::Entity>> layerEntities;
std::map<int, float> layerMovementDeltas;
```

**Impact:**
- 2 vector allocations/copies per frame
- 2 map allocations per frame
- 3 iterations through parallaxEntities (lines 122, 152, 176)
- This happens EVERY FRAME (60 times per second!)

**Solution:**
Make these member variables in `CameraSystem.h`:
```cpp
class CameraSystem {
private:
    // Cache these to avoid per-frame allocations
    std::vector<Gnosis::Entity> m_cachedParallaxEntities;
    std::map<int, std::vector<Gnosis::Entity>> m_layerEntities;
    std::map<int, float> m_layerMovementDeltas;
    
    // Rebuild cache only when needed (level load, not every frame)
    void RebuildParallaxCache();
};
```

Update cache only on level load, not every frame.

---

## 🐛 REMAINING BUGS

### 2. Toilet Paper Enemies Still Too Fast

**Status:** Config is correct (25.0f speed), but enemies appear fast in-game

**Possible Causes:**
1. Movement speed being overridden somewhere after spawn
2. Delta time scaling issue
3. Config not being applied correctly during spawn

**Investigation Needed:**
- Check `LevelManager::SpawnEnemyWithConfig()` - is speed being set?
- Check `EnemySystem::ProcessEnemyMovement()` - is it using `enemy->speed`?
- Add logging to confirm actual speed value at runtime

---

### 3. Snowmen Not Properly Grounded

**Status:** Still hovering above ground

**Possible Causes:**
1. Ground calculation using wrong height value
2. Y position being overridden after initial spawn
3. `GroundEnemy()` not being called

**Files to Check:**
- `LevelManager::SpawnInitialEnemies()` - snowman Y calculation
- `EnemySystem::GroundEnemy()` - grounding logic
- `EnemySystem::ProcessEnemyMovement()` - might be overriding Y position

---

### 4. Projectile Collision Still Not Working

**Status:** Projectiles spawn and move, but don't collide with enemies

**Current Logs Show:**
- `ProjectileSystem::Update` shows active projectiles when shooting
- But `EnemySystem` sees 0 projectiles during collision checks

**Theory:** Timing issue - projectiles might be in different phase of update cycle

**Debug Steps:**
1. Add frame number logging to both systems
2. Log exact timing when projectiles are added/removed from active list
3. Verify `GetActivePlayerProjectiles()` returns correct list

**Potential Issue:**
The `const auto&` references might be evaluated at different times?

---

### 5. Rat King Moving While Aiming

**Status:** Still moving during throw animation

**Root Cause:** Unknown - need to check `BossSystem` state transitions

**Files:**
- `src/FloppyTurd/Systems/BossSystem.cpp`

---

### 6. Background Flickering (Desert/Castle)

**Status:** Curtain layer adjustment made, needs testing

**Previous Fix:**
- Changed curtains.png layer from 1 to 2
- Changed scrollSpeed from 1.0f to 1.05f

**May Need:** More robust parallax positioning (related to issue #1)

---

## 📊 SYSTEM UPDATE ORDER (FIXED)

**Old (Broken):**
```
1. UpdateSpawning() → EnemySystem::Update() → checks collisions (0 projectiles!)
2. ProjectileSystem::Update() → adds projectiles to active list (too late!)
```

**New (Fixed):**
```
1. ProjectileSystem::Update() → updates active projectile list
2. EnemySystem::Update() → checks collisions with current list
```

---

## 🎯 ACTION PLAN

### Priority 1: Performance (CameraSystem)
- [ ] Move parallax vectors/maps to member variables
- [ ] Cache parallax entities on level load, not every frame
- [ ] Reduce from 3 passes to 1 pass through entities

### Priority 2: Collision Detection
- [ ] Add frame number logging to debug timing
- [ ] Verify `GetActivePlayerProjectiles()` timing
- [ ] Check if reference caching is causing stale data

### Priority 3: Enemy Speed/Grounding
- [ ] Add runtime logging for enemy speed values
- [ ] Verify grounding calculation for snowmen
- [ ] Check if movement is overriding spawn positions

### Priority 4: Rat King & Other Issues
- [ ] Debug Rat King state transitions during aiming
- [ ] Test background flickering with new settings
- [ ] Implement snowman throw animation (pending feature)

---

## 🔍 DEBUGGING IMPROVEMENTS MADE

1. ✅ Reduced log spam - only log when projectiles/collisions exist
2. ✅ Fixed system update order
3. ✅ Added targeted logging for collision checks
4. ⏳ Need frame-by-frame timing logs to debug collision issue

---

## 💡 OTHER POTENTIAL OPTIMIZATIONS

1. **RenderSystem**: Check if it's caching textures properly
2. **ObstacleSystem**: Check if checking 199 obstacles every frame is necessary
3. **ECS GetComponent calls**: Could be cached per entity during single-pass loop

---

## 📝 NOTES

- Collisions worked before refactor - regression bug
- CameraSystem performance issue existed whole time
- Need to verify actual runtime values vs config values
- Many issues might be interconnected (performance affecting timing)

