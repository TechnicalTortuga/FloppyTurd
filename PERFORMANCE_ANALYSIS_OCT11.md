# Performance Analysis & Optimization Report
**Date:** October 11, 2025  
**Status:** In Progress - Major Allocations Fixed

---

## 🔍 Profiler Analysis

### Current FPS Range
- **Main Menu:** ~60 FPS
- **Level 1:** 46-61 FPS (variable)
- **Sewer/Castle Levels:** 20-40 FPS ⚠️

### Metal Profiler Data (Per Second)
```
DrawSpriteBatch: avg=0.55ms, calls=367 (~6 batches/frame) ✅
DrawSpriteScaled: avg=0.41ms, calls=270 (~5 individual sprites/frame) ✅
DrawTextRaster: avg=0.18ms, MAX=19.45ms ⚠️ HUGE SPIKES!
BeginFrame: avg=0.06ms, MAX=8.09ms ⚠️ SPIKES!
```

---

## 🚨 BOTTLENECKS IDENTIFIED

### 1. **TEXT RENDERING - PRIMARY CULPRIT** 💥
**Impact:** Max spike of **19.45ms** (blocks entire frame!)
- FPS counter updates every frame
- Score/UI text re-rendering
- String allocations for text updates

**Fix Priority:** HIGH
- Cache text rendering to textures
- Only re-render text when it changes
- Reduce FPS counter update frequency

### 2. **PER-FRAME HEAP ALLOCATIONS** 💥
**Found in `RenderSystem::RenderWorldSpace()`:**

**BEFORE (Lines 432-433):**
```cpp
// Created EVERY FRAME (60 allocations/sec!)
std::map<int, std::unordered_map<uint32_t, std::vector<const RenderItem*>>> layeredBatches;
std::map<int, std::vector<const RenderItem*>> layeredNonBatchable;
```

**AFTER (Fixed):**
```cpp
// Now member variables - cleared and reused each frame
m_layeredBatches  // Reuse map/vectors, preserve capacity
m_layeredNonBatchable
m_currentFrameLayers // Also reused
```

**Impact:** Eliminated ~60 complex nested container allocations per second

### 3. **Multiple HasComponent Lookups**
**Lines 445-448:**
```cpp
bool usesCenteredRendering = m_ecsSystem->HasComponent<RotationRenderer>(entity);
bool usesPivotRotation = m_ecsSystem->HasComponent<PivotRotationRenderer>(entity);
bool isAnimated = item.sprite->isAnimated;
bool hasStateAnimation = m_ecsSystem->HasComponent<StateAnimation>(entity);
```
4 hash map lookups **per sprite per frame**

**Potential Fix:** Cache component flags in RenderItem or Sprite

### 4. **Culling Calculations**
**Lines 200-228:** WorldToScreen + bounds calculation for EVERY sprite
- Could cache screen positions if camera isn't moving much
- Could use spatial partitioning (grid/quadtree)

---

## ✅ OPTIMIZATIONS COMPLETED

### Phase 1: Sprite Batching ✅
- **Status:** Complete
- **Result:** ~500 draw calls → ~6 batches per frame
- **FPS Impact:** Enabled 60 FPS (was impossible before)

### Phase 2: Frustum Culling ✅
- **Status:** Complete with conservative margins
- **Result:** Off-screen sprites not rendered
- **FPS Impact:** Moderate improvement (especially in wide levels)

### Phase 3: Memory Allocations ✅ **NEW**
- **Status:** Just completed
- **Result:** Eliminated per-frame allocations of complex containers
- **Expected Impact:** 5-10 FPS improvement, reduced GC pressure

---

## 🎯 NEXT OPTIMIZATION PRIORITIES

### Priority 1: TEXT RENDERING CACHE 🔥
**Problem:** Text re-rendered every frame causing 19ms spikes  
**Solution:**
1. Render text to MTLTexture once
2. Cache texture, only regenerate when text changes
3. Draw cached texture (fast blit operation)

**Expected Impact:** +10-15 FPS, eliminate frame drops

### Priority 2: Component Flag Caching
**Problem:** Multiple HasComponent calls per sprite  
**Solution:** Add flags to RenderItem:
```cpp
struct RenderItem {
    // ... existing fields ...
    bool needsCenteredRendering = false;
    bool needsPivotRotation = false;
    bool needsStateAnimation = false;
};
```
Set once during collection, check cheaply during batching

**Expected Impact:** +2-5 FPS

### Priority 3: Spatial Partitioning (If Still Needed)
**Problem:** Culling checks every sprite against viewport  
**Solution:** Grid-based spatial hash
- Only check sprites in grid cells overlapping viewport
- Update grid when entities move significantly

**Expected Impact:** +5-10 FPS in dense levels

---

## 🔮 FUTURE CONSIDERATIONS

### GPU Instancing
**When:** If still bottlenecked after above fixes  
**What:** Render all instances of same sprite in ONE draw call
- Requires shader modifications
- Pass transform matrices as instance buffer
**Expected Impact:** Marginal (already batching well)

### Render to Texture (Single Draw Call)
**Complexity:** HIGH  
**Benefit:** LOW (Metal already handles batching efficiently)

**Why NOT recommended:**
- Still need to draw all sprites to intermediate texture
- Added complexity of render target management
- Memory overhead of full-screen textures
- No real benefit over current batching

---

## 📊 ALGORITHMIC COMPLEXITY

### Current Per-Frame (N = visible sprites):
```
CollectRenderItems:  O(N)           ✅ Optimal
WorldToScreen:       O(N)           ✅ Required
Culling checks:      O(N)           ✅ Required
Component lookups:   O(4N)          ⚠️ Can reduce to O(N)
Sorting:             O(N log N)     ✅ Once per frame
Batching grouping:   O(N)           ✅ Linear
Layer iteration:     O(L * B)       ✅ L=layers, B=batches (both small)
```

**Overall:** O(N log N) which is optimal for sorted rendering

---

## 🎮 TESTING CHECKLIST

After each optimization, test:
- ✅ Level 1 (pipes, coins, backgrounds)
- ✅ Sewer level (large backgrounds, scrolling)
- ✅ Castle level (dense pickups)
- ✅ Boss level (complex animations)
- ✅ All sprite animations working
- ✅ Culling not premature
- ✅ No visual artifacts

---

## 📈 TARGET METRICS

**Goal:** Consistent 60 FPS across all levels

**Current:**
- Main Menu: ~60 FPS ✅
- Level 1: 46-61 FPS (close!)
- Sewer: 20-40 FPS ⚠️
- Castle: 20-40 FPS ⚠️

**After Text Caching (Predicted):**
- Main Menu: ~60 FPS ✅
- Level 1: ~60 FPS ✅
- Sewer: 50-60 FPS ✅
- Castle: 50-60 FPS ✅

---

## 🛠️ IMPLEMENTATION NOTES

### Container Reuse Pattern
```cpp
// Clear vectors but preserve capacity
for (auto& [layer, batches] : m_layeredBatches) {
    for (auto& [handle, items] : batches) {
        items.clear(); // Capacity preserved
    }
}
// Don't clear maps - preserve structure
```

This pattern avoids:
- Map node deallocations
- Vector capacity reallocations
- Fragmentation

### Why Not Single-Draw-Call Approach?

**Render Target Method:**
1. Create offscreen MTLTexture (1179x2556 pixels)
2. Render all sprites to texture (still N draw calls!)
3. Draw texture to screen (1 draw call)

**Problems:**
- Memory: 12MB per frame for full-res texture
- Still N draw calls (to render target)
- No performance benefit
- Added complexity

**Current batching is superior:** Same few draw calls, no memory overhead

---

## 💡 CONCLUSION

**Batching + Culling + Allocation Fixes** have brought us close to 60 FPS.

**Next critical fix:** Text rendering cache will likely get us to consistent 60 FPS.

**GPU instancing** not needed - current batching already efficient.

**Render targets** not recommended - no real benefit over batching.
