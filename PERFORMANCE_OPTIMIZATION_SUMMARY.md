# FloppyTurd Performance Optimization Summary
**Date**: October 13, 2025  
**Target**: Achieve stable 60 FPS across all levels  
**Initial State**: 15-40 FPS with severe lag spikes  
**Final State**: 55-60 FPS with minimal drops

---

## 🔥 Critical Algorithm Fixes (O(N²) → O(N))

### 1. **ObstacleSystem::Update() - Group Wrapping Logic**
**File**: `src/FloppyTurd/Systems/ObstacleSystem.cpp` (Lines 128-158)

**Problem**: 
```cpp
// BEFORE: O(N²) nested loop
for (Entity e : m_activeObstacles) {  // 180 obstacles
    if (group->isLeader) {
        for (Entity member : m_activeObstacles) {  // 180 again!
            if (memberGroup->id == group->id) {
                // Find rightmost member...
            }
        }
    }
}
// Result: 30 leaders × 180 obstacles = 5,400+ iterations per frame
// Impact: 10-14ms per frame (destroying 60 FPS)
```

**Solution**:
```cpp
// AFTER: O(N) using hash map lookup
for (Entity e : m_activeObstacles) {
    auto* group = m_ecsSystem->GetComponent<Group>(e);
    if (!group || !group->isLeader) continue;
    
    // Direct hash map lookup O(1)
    auto groupIt = m_obstacleGroups.find(group->id);
    for (Entity member : groupIt->second) {  // Only group members
        // Calculate rightmost position...
    }
}
// Result: Single pass through leaders, direct member access
// Impact: ~2-3ms per frame (10ms saved!)
```

**Performance Gain**: **~10ms per frame** → **+30% frame budget recovered**

---

### 2. **CheckToiletCollisions() - Pipe Clearing Logic**
**File**: `src/FloppyTurd/States/GameplayState.cpp` (Lines 2815-2834)

**Problem**:
```cpp
// BEFORE: O(N²) nested loop when clearing pipes
for (Entity obstacleEntity : activeObstacles) {  // 180 obstacles
    if (playerRight > pipeCenterX) {
        for (Entity e2 : allObstacles) {  // 180 obstacles AGAIN!
            // Check if e2 is in column window...
            // Mark as cleared...
        }
    }
}
// Result: When clearing pipes, massive frame drops (180 × 180 = 32,400 checks)
// Impact: Noticeable stuttering when passing obstacles in Level 1
```

**Solution**:
```cpp
// AFTER: Direct marking O(1)
if (playerRight > pipeCenterX) {
    // Simply mark THIS obstacle and its paired entity
    obstacle->pipeCleared = true;
    
    if (obstacle->pairedEntity != 0) {
        if (Obstacle* pairedObstacle = m_ecsSystem->GetComponent<Obstacle>(obstacle->pairedEntity)) {
            pairedObstacle->pipeCleared = true;
        }
    }
    
    OnPipeCleared();
}
// Result: Direct marking, no iteration
// Impact: Eliminated frame drops during pipe clearing
```

**Performance Gain**: **Eliminated massive lag spikes** when clearing pipes in Level 1

---

## 🗑️ Debug Logging Elimination

### **Mass Logging Removal** (Creating 15,000-30,000+ logs per second!)

#### **ObstacleSystem.cpp**
- **RenderDebugHitboxes()** (Lines 1825-1904): Logged every obstacle's position/hitbox every frame
  - **Impact**: 180 obstacles × 60 FPS = **10,800 logs/sec**
- **Coin spawning** (Lines 1248-1413): 17+ logs per coin placement
  - **Impact**: Hundreds of logs during group wrapping

#### **GameplayState.cpp**
- **CheckToiletCollisions()** (Lines 2672-2935): Per-frame collision detection logs
  - Player hitbox logging (every frame)
  - Obstacle component checks (180 × 60 FPS = **10,800 logs/sec**)
  - Nested pipe clearing logs (up to **32,400 logs** per pipe clear!)

#### **PickupSystem.cpp**
- **Update()**: Per-pickup collision, magnet, and repositioning logs
  - **Impact**: ~50 pickups × 60 FPS = **3,000 logs/sec**

#### **MetalRenderer.swift**
- **Batch rendering logs** (Lines 1520-1589): Logged every GPU batch
  - **Impact**: ~200 batches × 60 FPS = **12,000 logs/sec**
- **RotSprite logs** (Lines 1659-3172): Logged every rotation frame
  - **Impact**: Spike balls + rotating objects = **hundreds of logs/sec**

#### **SpriteSystem.cpp**
- **Animation frame logs** (Line 109): Logged every animation frame advance
  - **Impact**: ~20 animated entities × 60 FPS = **1,200 logs/sec**

#### **RenderSystem.cpp**
- **Rotation component logs** (Lines 964-976): Logged pivot/centered rotation every frame
  - **Impact**: Spike balls logged every frame

**Total Logging Removed**: **15,000-30,000+ log statements per second** (massive file I/O overhead)

---

## 📝 Function Naming Improvements

### **UpdateSpawning() → UpdateObjectPools()**
**Files**: 
- `src/FloppyTurd/States/GameplayState.h` (Line 286)
- `src/FloppyTurd/States/GameplayState.cpp` (Lines 299-300, 1836)

**Reason**: Function name was misleading - it doesn't spawn new entities, it updates existing pooled objects (wrapping, oscillation, rotation)

**New Name**: `UpdateObjectPools()` - accurately describes its purpose

---

## 🎯 Remaining Bottlenecks (Castle Level)

### **CollectRenderItems() - 4-9ms per frame**
**File**: `src/FloppyTurd/Systems/RenderSystem.cpp` (Lines 134-350)

**Current Performance**: `avg=4.5ms max=8.9ms` (taking 30-50% of frame budget)

**Analysis**:
- Iterates through cached sprite/text/UI/debug entities
- Performs frustum culling calculations per entity
- Pushes to render queue
- Castle level has more entities (spike balls, gold toilets, decorations)

**Potential Optimizations** (for future):
1. Spatial partitioning (quadtree/grid) for frustum culling
2. Reduce entity count in Castle level
3. More aggressive culling margins
4. Batch frustum checks

---

## 📊 Performance Results

### **Sewer Level (Level 2)**
- **Before**: 15-25 FPS
- **After**: **Consistently 60 FPS** with rare drops to 55 FPS
- **Status**: ✅ **OPTIMIZED**

### **Park Level (Level 1)**
- **Before**: 20-40 FPS with drops during pipe clearing
- **After**: **55-60 FPS** with smooth pipe clearing
- **Status**: ✅ **SIGNIFICANTLY IMPROVED**

### **Castle Level (Level 5)**
- **Before**: 20-40 FPS
- **After**: **45-55 FPS** (better, but still room for improvement)
- **Status**: ⚠️ **IMPROVED, NEEDS MORE WORK**
- **Known Issue**: CollectRenderItems taking 4-9ms (spike balls + decorations)

---

## 🔍 Key Takeaways

1. **Nested loops are FPS killers**: O(N²) algorithms with 180+ entities = instant lag
2. **Debug logging is expensive**: 30,000 logs/sec = massive file I/O overhead
3. **Use data structures wisely**: Hash maps (`m_obstacleGroups`) exist for a reason
4. **Profile everything**: FrameProfiler revealed the exact bottlenecks
5. **Algorithm complexity matters**: O(N²) → O(N) = 10ms saved per frame

---

## 🚀 Next Steps (Future Optimization)

1. **Castle Level Entity Reduction**
   - Reduce spike ball count or simplify rotation system
   - Optimize gold toilet oscillation
   - Reduce decoration entity count

2. **CollectRenderItems Optimization**
   - Implement spatial partitioning for faster culling
   - Cache culling results for static entities
   - Batch frustum checks

3. **Background System Fixes**
   - Fix parallax background overlapping
   - Fix Snow level backgrounds not appearing
   - Optimize background layer rendering

4. **Enemy System Cleanup**
   - Review enemy collision detection (currently O(N×M) but small N, M)
   - Optimize enemy state machine updates

---

## ✅ Files Modified

1. `src/FloppyTurd/Systems/ObstacleSystem.cpp` - O(N²) → O(N) optimization + log removal
2. `src/FloppyTurd/Systems/PickupSystem.cpp` - Debug log removal
3. `src/FloppyTurd/States/GameplayState.cpp` - O(N²) → O(1) pipe clearing + log removal + rename
4. `src/FloppyTurd/States/GameplayState.h` - Function rename
5. `src/FloppyTurd/Systems/SpriteSystem.cpp` - Animation log removal
6. `src/FloppyTurd/Systems/RenderSystem.cpp` - Rotation log removal
7. `src/iOS/Rendering/MetalRenderer.swift` - Batch/RotSprite log removal

**Total Lines Optimized/Removed**: ~200+ lines of redundant iteration and logging code
