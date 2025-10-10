# Performance Optimization Plan - Phase 2
**Date:** October 10, 2025  
**Current Status:** 40-50 FPS (Target: 60+ FPS consistently)

---

## 🎯 Current Performance Status

### FPS by Level
- **Level 1:** 60 FPS with **drops to 50 FPS when crossing pipes** ⚠️
- **Level 2:** 45-50 FPS ⚠️
- **Level 5:** 40-50 FPS with dips below 40 ⚠️

### C++ Profiling Results (COMPLETED ✅)

**GameplayState Update Loop:** ~4-5ms total
```
UpdateSpawning:          1.86-1.99ms  ⚠️ HEAVIEST SYSTEM
CheckToiletCollisions:   0.93-1.00ms  ⚠️ PIPE CROSSING LAG
CameraSystem:            0.41ms       ✅
SpriteSystem:            0.11ms       ✅
All other systems:       <0.5ms       ✅
```

**RenderSystem:** ~0.3-1.0ms total
```
CollectRenderItems:   0.20-0.47ms  ✅
RenderWorldSpace:     0.07-0.19ms  ✅
RenderScreenSpace:    0.09-0.30ms  ✅
SortRenderQueue:      0.01-0.03ms  ✅
```

**Total C++ Time:** ~4-6ms (only 30-36% of 16.67ms budget!)

### The Mystery: Where are the other 10-12ms going?

Since C++ only uses 4-6ms, the remaining **10-12ms bottleneck is in**:
1. 🔴 **Metal Renderer (Swift/GPU side)** - NOT PROFILED YET
2. 🔴 **Texture operations** - Loading/uploading to GPU
3. 🟡 **Swift/Objective-C bridging overhead**
4. 🟡 **File I/O for logging** (partially fixed)

---

## 🔥 Priority 1: CRITICAL ISSUES (Do First)

### 1.1 Profile Metal Renderer ⚡ HIGHEST PRIORITY
**Problem:** Metal rendering pipeline is NOT profiled. This is likely where 10-12ms is hiding.

**Where to Profile:**
- `MetalRenderer.swift` - All draw calls
  - `drawSprite()` / `drawSpriteScaled()`
  - `drawText()` / `drawTextCentered()` / `drawTextCenteredOutlined()`
  - `drawRectangle()`
  - Texture uploads / GPU operations
  - Command buffer encoding
  - Present() call

**Action Items:**
- [ ] Add timing to `MetalRenderer.drawSprite()` and variants
- [ ] Add timing to `MetalRenderer.drawText()` and variants
- [ ] Add timing to command buffer encoding
- [ ] Add timing to GPU texture operations
- [ ] Add timing to present() call
- [ ] Log aggregate stats every 1 second (like C++ profiler)

**Expected Findings:**
- Likely culprits: Texture uploads, excessive draw calls, or GPU stalls

---

### 1.2 Fix CheckToiletCollisions - PIPE CROSSING LAG 🚨
**Problem:** FPS drops from 60→50 when crossing pipes (Level 1)

**Current Cost:** 0.93-1.00ms avg, 2.01ms max

**Root Cause Analysis:**
- **Brute force collision detection** - Checking player against ALL pipes every frame
- **No spatial partitioning** - Pipes far off-screen still checked
- **Heavy math on pipe crossing** - Likely scoring/audio/particles triggered

**Optimization Plan:**
```cpp
// BEFORE (checking all pipes):
for (all pipes in level) {
    if (playerCollidesWith(pipe)) { ... }
}

// AFTER (spatial partitioning):
for (pipes within 200px of player) {  // Only check nearby
    if (playerCollidesWith(pipe)) { ... }
}
```

**Action Items:**
- [ ] Add spatial grid to only check pipes near player (±200px X range)
- [ ] Cache pipe positions in sorted array by X coordinate
- [ ] Binary search for nearby pipes instead of linear scan
- [ ] Profile the collision detection vs the "on collision" logic separately
- [ ] Investigate what happens when pipes are crossed (scoring, audio, particles)

**Expected Improvement:** 0.93ms → 0.2ms (75% reduction)

---

### 1.3 Optimize UpdateSpawning - HEAVIEST SYSTEM 🔥
**Problem:** Consistently 1.86-1.99ms (40% of gameplay update time!)

**Current Cost:** 1.86-1.99ms avg, 3.78ms max

**Investigation Needed:**
> User Note: "All entities should be POOLED already"

**Questions to Answer:**
1. Are pools actually being used or are we doing `new` allocations?
2. Are we calling `CreateEntity()` every spawn or reusing from pool?
3. Are we doing expensive setup on spawn (texture loads, component setup)?
4. What causes the 3.78ms spike? (Double spawn burst?)

**Action Items:**
- [ ] Profile individual spawn types:
  - `SpawnObstacle()` timing
  - `SpawnEnemy()` timing
  - `SpawnPickup()` timing
  - `ActivatePooledEntity()` timing
- [ ] Verify object pooling is ACTUALLY used (add debug counter)
- [ ] Check if we're loading textures synchronously on spawn
- [ ] Spread spawns across multiple frames if needed
- [ ] Pre-initialize pool entities with components on level load

**Expected Improvement:** 1.9ms → 0.5ms (75% reduction if pooling is fixed)

---

## 🟡 Priority 2: IMPORTANT (Do Soon)

### 2.1 Add Granular Metal Profiling
**File:** `/Users/aimac/Development/FloppyTurd/src/Raylib/Rendering/MetalRenderer.swift`

**Sections to Profile:**
```swift
// 1. Sprite rendering
func drawSprite(...) {
    startTiming("DrawSprite")
    // ... texture lookup, pipeline setup, encoding
    endTiming("DrawSprite")
}

// 2. Text rendering (likely expensive with outlined text)
func drawTextCenteredOutlined(...) {
    startTiming("DrawTextOutlined")
    // ... glyph rendering, outline passes
    endTiming("DrawTextOutlined")
}

// 3. Command buffer submission
func present() {
    startTiming("GPUPresent")
    commandBuffer?.present(drawable)
    commandBuffer?.commit()
    endTiming("GPUPresent")
}
```

**Expected Findings:**
- Text rendering with outlines may be doing **multiple passes per glyph**
- Texture lookups may be synchronous and blocking
- GPU wait time may be significant

---

### 2.2 Investigate Texture System Overhead
**Problem:** Every `GetOrLoadTexture()` call may be triggering async operations

**Questions:**
1. Are textures being loaded **on-demand during rendering**?
2. Are we caching texture handles properly?
3. Are we calling `getTexture()` multiple times per frame for the same texture?

**Action Items:**
- [ ] Add profiling to `GetOrLoadTexture()` in RenderSystem
- [ ] Verify texture cache is hit (not miss) during normal gameplay
- [ ] Pre-load all textures on level start (no on-demand loading)
- [ ] Add metrics: Cache hits vs misses per frame

---

### 2.3 Clean Up Debug Logging (User Request)
**Goal:** "Make debug logs more precise to the areas we are debugging"

**Strategy:**
- ✅ Already removed hot-path logs from RenderSystem
- [ ] Add logging categories (RENDER, SPAWN, COLLISION, AUDIO, etc.)
- [ ] Add runtime log filtering (only log SPAWN during spawn debugging)
- [ ] Keep ERROR/WARN logs always enabled
- [ ] Remove INFO logs from hot paths

**Action Items:**
- [ ] Create `GN_LOG_CATEGORY` macro system
- [ ] Wrap remaining hot-path logs in debug flags
- [ ] Add command-line arg to enable specific categories

---

## 🟢 Priority 3: NICE TO HAVE (Do Later)

### 3.1 Batch Rendering Optimizations
**Problem:** Each sprite/text draw may be a separate GPU command

**Optimization:**
- Batch identical textures into single draw call
- Use instanced rendering for repeated sprites (pipes, coins)
- Sort render queue by texture to minimize state changes

**Expected Improvement:** 20-30% reduction in GPU time

---

### 3.2 Reduce Sprite Animation Overhead
**Current:** SpriteSystem updates all animated sprites every frame

**Optimization:**
- Update animations at 30Hz instead of 60Hz
- Skip animation updates for off-screen sprites
- Use simpler animation math (pre-calculate frame times)

**Expected Improvement:** 0.11ms → 0.05ms

---

### 3.3 Optimize Camera System Spikes
**Problem:** 0.41ms avg but 2.23ms max spike

**Investigation:**
- What causes the 2.23ms spike?
- Camera shake calculations?
- Viewport transform updates?

**Action Items:**
- [ ] Profile camera calculations separately
- [ ] Cache transform matrices
- [ ] Investigate spike cause

---

## 📊 Performance Targets

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| **Level 1 FPS** | 60 (drops to 50) | 60+ stable | 🟡 |
| **Level 2 FPS** | 45-50 | 60+ stable | 🔴 |
| **Level 5 FPS** | 40-50 | 60+ stable | 🔴 |
| **C++ Update Time** | 4-6ms | <6ms | ✅ |
| **Metal Render Time** | ??? | <8ms | ⚠️ NOT PROFILED |
| **Total Frame Time** | 20-25ms | <16.67ms | 🔴 |

---

## 🎯 Action Plan Summary

### Week 1: Profiling & Critical Fixes
1. **Day 1:** Add Metal renderer profiling (Swift side)
2. **Day 2:** Analyze Metal profiling results, identify GPU bottleneck
3. **Day 3:** Fix CheckToiletCollisions with spatial partitioning
4. **Day 4:** Profile & optimize UpdateSpawning
5. **Day 5:** Test all levels, measure FPS improvements

### Expected Results After Week 1:
- **Metal profiling:** Identify 10-12ms bottleneck source
- **CheckToiletCollisions:** 0.9ms → 0.2ms (-75%)
- **UpdateSpawning:** 1.9ms → 0.5ms (-75%)
- **Total savings:** ~2.2ms from C++ side
- **FPS improvement:** Depends on Metal optimizations

---

## 🔍 Specific Investigation Tasks

### Task 1: Metal Renderer Profiling Setup
**File:** `src/Raylib/Rendering/MetalRenderer.swift`

**Add timing infrastructure:**
```swift
class MetalRenderer {
    private var frameTimes: [String: (total: Double, count: Int, max: Double)] = [:]
    private var lastFlushTime = CACurrentMediaTime()
    
    func startTiming(_ section: String) {
        // Store start time
    }
    
    func endTiming(_ section: String) {
        // Calculate duration, aggregate stats
    }
    
    func flushTimings() {
        // Log stats every ~1 second
    }
}
```

### Task 2: CheckToiletCollisions Spatial Grid
**File:** `src/FloppyTurd/States/GameplayState.cpp`

**Add spatial optimization:**
```cpp
// Cache pipes sorted by X position
std::vector<Gnosis::Entity> m_sortedPipes;
float m_lastPipeSortX = 0.0f;

void CheckToiletCollisions() {
    float playerX = playerTransform->position.x;
    
    // Only check pipes within ±200px
    auto startIt = std::lower_bound(m_sortedPipes.begin(), m_sortedPipes.end(), 
                                     playerX - 200.0f, [](Entity e, float x) {
        return GetComponent<Transform>(e)->position.x < x;
    });
    
    auto endIt = std::upper_bound(m_sortedPipes.begin(), m_sortedPipes.end(),
                                   playerX + 200.0f, [](float x, Entity e) {
        return x < GetComponent<Transform>(e)->position.x;
    });
    
    // Only check nearby pipes (typically 2-3 instead of 50+)
    for (auto it = startIt; it != endIt; ++it) {
        CheckCollision(*it);
    }
}
```

### Task 3: UpdateSpawning Deep Profiling
**File:** `src/FloppyTurd/States/GameplayState.cpp`

**Add sub-section timing:**
```cpp
void UpdateSpawning(float deltaTime) {
    m_frameProfiler.StartSection("UpdateSpawning");
    
    m_frameProfiler.StartSection("SpawnObstacles");
    // Spawn obstacles logic
    m_frameProfiler.EndSection("SpawnObstacles");
    
    m_frameProfiler.StartSection("SpawnEnemies");
    // Spawn enemies logic
    m_frameProfiler.EndSection("SpawnEnemies");
    
    m_frameProfiler.StartSection("SpawnPickups");
    // Spawn pickups logic
    m_frameProfiler.EndSection("SpawnPickups");
    
    m_frameProfiler.EndSection("UpdateSpawning");
}
```

---

## 📈 Success Criteria

**Minimum Viable Performance (MVP):**
- ✅ Level 1: Stable 60 FPS (no drops on pipe crossing)
- ✅ Level 2: Stable 60 FPS
- ✅ Level 5: Stable 55-60 FPS

**Stretch Goals:**
- ✅ All levels: Stable 60 FPS
- ✅ Frame time budget <14ms (headroom for physics spikes)
- ✅ No frame drops during gameplay events (pipes, enemies, pickups)

---

## 🚀 Next Steps

1. **IMMEDIATE:** Profile Metal renderer (add timing infrastructure)
2. **NEXT:** Analyze Metal profiling results in gameplay
3. **THEN:** Fix CheckToiletCollisions (spatial partitioning)
4. **THEN:** Investigate UpdateSpawning (verify pooling is used)
5. **FINALLY:** Test and validate 60 FPS on all levels

**Let's start with Metal profiling - that's where the mystery 10-12ms is hiding!**
