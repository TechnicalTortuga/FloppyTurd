# Critical Fixes Needed - Multiple Issues

## Issues Identified:

1. **Rat King spawning incorrectly** - Multiple copies, not animating, no boss AI
2. **Snowmen not spawning** in snow level
3. **Rats not spawning** in castle level  
4. **Snow/Castle backgrounds missing**
5. **Desert/Castle debug hitboxes** still visible (should be off)
6. **Birds spawn too low**

---

## Root Causes:

### 1. Rat King Issue ❌ CRITICAL

**Problem:** Rat King is being added to the regular enemy pool and spawned multiple times like a regular enemy

**Root Cause:**
- `AddBossEnemies()` adds Rat King to `config.enemies` list
- `InitializeEnemyPool()` creates 10 pooled Rat Kings
- `SpawnInitialEnemies()` spawns multiple instances
- `BossSystem` expects ONE special boss entity, not pooled enemies

**Solution:**
- Rat King should NOT be in the enemy pool
- Boss enemies need special initialization in `BossSystem`
- Skip "boss_idle" movement pattern in `InitializeEnemyPool()`

### 2. Multi-Enemy Type Levels ❌

**Problem:** `SpawnInitialEnemies()` only spawns ONE enemy type

**Current Code:**
```cpp
void LevelManager::SpawnInitialEnemies() {
    if (m_currentLevelConfig.enemies.empty()) return;
    
    // BUG: Only uses FIRST enemy config
    const EnemyConfig& config = m_currentLevelConfig.enemies[0];  // ← ONLY FIRST!
    
    int count = std::min(5, GetAvailableEnemyCount());
    for (int i = 0; i < count; ++i) {
        // Spawns only first type...
    }
}
```

**Issue:** Snow level has 4 enemy types, Castle has 1, but only first type spawns

**Solution:** Distribute spawns across ALL enemy types in rotation

### 3. Missing Backgrounds ❌

**Problem:** Snow (level 4) and Castle (level 5) have no backgrounds rendering

**Possible Causes:**
- Background layers not configured in `AddSnowLevelLayers()` / `AddCastleLevelLayers()`
- Textures not loading
- Layer ordering incorrect

**Solution:** Check `LevelConfig.cpp` layer setup functions

### 4. Debug Hitboxes ❌

**Problem:** Debug rendering still showing for obstacles

**Need to find:** Where `debugHitboxes` or `showDebugHitboxes` flag is set

---

## Fix Priority:

1. **Rat King (Critical)** - Boss completely broken
2. **Multi-enemy spawning** - Levels incomplete
3. **Missing backgrounds** - Visual bug
4. **Debug hitboxes** - Polish issue
5. **Bird height** - Minor adjustment

---

## Implementation Plan:

### Fix 1: Rat King Separate from Pool

```cpp
// In LevelManager::InitializeEnemyPool()
for (const EnemyConfig& config : m_currentLevelConfig.enemies) {
    // Skip boss enemies - they're handled by BossSystem
    if (config.movementPattern == "boss_idle" || 
        config.textureId == "Ratking" ||
        config.textureId == "RatKing") {
        GN_LOG_INFO("Skipping boss enemy in pool: " + config.textureId);
        continue;
    }
    
    // Pool regular enemies...
}
```

### Fix 2: Multi-Enemy Spawning

```cpp
void LevelManager::SpawnInitialEnemies() {
    if (m_currentLevelConfig.enemies.empty() || m_enemyPool.inactiveEnemies.empty()) return;
    
    int maxSpawns = std::min(5, (int)m_enemyPool.inactiveEnemies.size());
    
    // Get screen dimensions
    const ScreenInfo& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
    
    // Distribute spawns across all available enemy types in pool
    for (int i = 0; i < maxSpawns; ++i) {
        Gnosis::Entity enemy = GetInactiveEnemy();
        if (!enemy) break;
        
        // Get the enemy type from the entity
        Enemy* enemyComp = m_ecsSystem->GetComponent<Enemy>(enemy);
        if (!enemyComp) continue;
        
        // Position based on enemy type
        float x = screenInfo.pixelWidth + 400.0f + (i * 1200.0f);
        float y = screenInfo.pixelHeight * 0.3f + (i * 200.0f);
        
        // Spawn with proper config
        // (Need to find which config matches this enemy...)
    }
}
```

### Fix 3: Check Background Layers

Need to review `AddSnowLevelLayers()` and `AddCastleLevelLayers()` in `LevelConfig.cpp`

---

## Files to Modify:

1. `src/FloppyTurd/Systems/LevelManager.cpp` - Enemy pool init, spawning
2. `src/FloppyTurd/Config/LevelConfig.cpp` - Background layers
3. `src/FloppyTurd/Systems/ObstacleSystem.cpp` - Debug hitbox flags
4. `src/FloppyTurd/Config/EnemyConfigs.cpp` - Bird spawn height

---

**Status:** Ready to implement fixes

