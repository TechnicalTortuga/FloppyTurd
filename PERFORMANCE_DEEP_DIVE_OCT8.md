# Performance Deep Dive - October 8, 2025

## 📊 FPS Measurements

| Scene | FPS | Analysis |
|-------|-----|----------|
| Main Menu | ~120 FPS | ✅ Good - minimal rendering |
| Level 1 (No enemies) | 40-60 FPS | ⚠️ Already slow with just obstacles |
| Level 2 (With enemies) | ~22 FPS | 🔴 CRITICAL - Unplayable |

**Drop Analysis:**
- Menu → Level 1: **50% FPS loss** (obstacles + parallax)
- Level 1 → Level 2: **60% FPS loss** (enemies added)

This suggests **multiple** performance bottlenecks, not just enemies.

---

## 🔍 RENDERER ANALYSIS

### Current Render Path (Per Frame):

```cpp
RenderSystem::Render() {
    1. GetEntitiesWithComponents<Transform, Sprite>()     // ALL entities
    2. GetEntitiesWithComponents<Transform, Text>()       // ALL text
    3. GetEntitiesWithComponents<Transform, UIElement>()  // ALL UI
    4. GetEntitiesWithComponents<Transform, DebugDraw>()  // ALL debug
    5. GetEntitiesWithComponents<Transform, UIShape>()    // ALL shapes
    
    // For EACH entity found:
    6. GetComponent calls (Transform, Sprite, etc.)
    7. Culling checks
    8. World-to-screen transform
    9. Texture loading/caching
    10. Draw call to Metal
}
```

### 🔥 CRITICAL ISSUES:

#### Issue 1: ECS Query Cost (Lines 58, 147, 158, 196, 326)
```cpp
auto renderableEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite>();
```

**Problem:** This iterates through **ALL entities in the ECS** to find matches!
- With 200 obstacles + enemies + UI = **500+ entity checks**
- Done **5 times per frame** (sprites, text, UI, debug, shapes)
- = **2500+ entity type checks per frame!**

**Evidence:** Level 1 (no enemies) already at 40-60 FPS suggests obstacle count is the issue.

#### Issue 2: Individual Draw Calls
Every sprite = separate Metal draw call
- **No batching**
- **No instancing**
- Each call has overhead: state changes, buffer bindings, shader setup

With 200 obstacles + 5 enemies + backgrounds:
- **~210 draw calls per frame**
- At 22 FPS = **4,620 draw calls/second** (way too many!)

#### Issue 3: Texture Handle Lookups (Line 751+)
```cpp
uint32_t GetOrLoadTexture(const std::string& textureId, Entity entity)
```

For each sprite:
1. String lookup in `m_textureCache` (hash map)
2. If not found, async load + string operations
3. Multiple map lookups per frame

**Cost:** String hashing + map lookups for every sprite, every frame

---

## 🎯 ROOT CAUSES

### 1. **ECS GetEntitiesWithComponents is O(n)**
The ECS iterates ALL entities checking component masks.
- 500 entities × 5 queries = 2,500 checks **before** any rendering!

### 2. **No Render Batching**
Modern renderers batch sprites with same texture/state:
```
Instead of: Draw(sprite1), Draw(sprite2), Draw(sprite3)...
Do: DrawBatch([sprite1, sprite2, sprite3...])
```

We're doing individual draws = massive overhead.

### 3. **No Spatial Culling Until Collection**
We collect ALL entities, then cull during iteration.
Should: Only collect visible entities.

### 4. **String-Based Texture Lookups**
Every frame, every sprite: hash string → lookup map
Should: Cache texture handle in Sprite component.

---

## 💡 OPTIMIZATION STRATEGIES

### Immediate Wins (Should get to 50+ FPS):

#### A. Cache Renderable Entities
```cpp
class RenderSystem {
    // Cache these, rebuild only when entities added/removed
    std::vector<Entity> m_cachedRenderableEntities;
    std::vector<Entity> m_cachedUIEntities;
    std::vector<Entity> m_cachedTextEntities;
    bool m_renderCacheDirty = true;
    
    void RebuildRenderCache() {
        // Only call when entities change, not every frame!
    }
};
```

**Savings:** 2,500 entity checks → 0 per frame

#### B. Cache Texture Handles in Sprite Component
```cpp
struct Sprite {
    std::string textureId;
    uint32_t cachedTextureHandle = 0;  // NEW: Cache the handle!
    bool handleValid = false;
};
```

When rendering:
```cpp
if (!sprite->handleValid) {
    sprite->cachedTextureHandle = GetOrLoadTexture(sprite->textureId);
    sprite->handleValid = true;
}
// Use sprite->cachedTextureHandle (no lookup!)
```

**Savings:** 210 string hashes + map lookups → 0 per frame

#### C. Spatial Partitioning (Grid/Quadtree)
Only check entities in visible grid cells.

**Savings:** Check 50 entities instead of 500

---

### Advanced Wins (Should get to 60 FPS):

#### D. Sprite Batching by Texture
Group sprites with same texture into single draw call:
```cpp
struct SpriteBatch {
    uint32_t textureHandle;
    std::vector<SpriteVertex> vertices;  // All sprites with this texture
};
```

**Metal Call:**
```cpp
// Instead of 210 individual draws:
for (auto& batch : batches) {
    drawBatch(batch.textureHandle, batch.vertices);  // 1 call for all sprites!
}
```

**Savings:** 210 draw calls → ~20 draw calls (grouped by texture)

#### E. Instanced Rendering
For repeated sprites (obstacles), use Metal instancing:
```cpp
drawInstanced(pipeTexture, instances, count);  // Draw 50 pipes in 1 call
```

**Savings:** Massive GPU/CPU overhead reduction

---

## 🔬 SPECIFIC BOTTLENECK LOCATIONS

### File: `RenderSystem.cpp`

**Lines 58-342: CollectRenderItems()**
```cpp
// PROBLEM: 5 separate ECS queries every frame
auto renderableEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Sprite>();  // Line 58
auto textEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, Text>();           // Line 147
auto uiElementEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, UIElement>(); // Line 158
auto debugEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, DebugDraw>();     // Line 196
auto shapeEntities = m_ecsSystem->GetEntitiesWithComponents<Transform, UIShape>();       // Line 326
```

**FIX:** Cache these lists, rebuild only on entity create/destroy.

**Lines 751-850: GetOrLoadTexture()**
```cpp
// PROBLEM: String lookup every frame for every sprite
auto it = m_textureCache.find(textureId);  // Hash + lookup!
```

**FIX:** Cache handle in Sprite component.

**Lines 360-375: RenderWorldSpace()**
```cpp
// PROBLEM: 210 individual draw calls
for (const RenderItem& item : m_renderQueue) {
    RenderSingleItem(item);  // Separate draw call each time!
}
```

**FIX:** Batch by texture before drawing.

---

### File: `EnemySystem.cpp`

**Lines 529-545: ProcessEnemyCollision()**
```cpp
// For EACH enemy, for EACH projectile:
Transform* projTransform = m_ecsSystem->GetComponent<Transform>(projEntity);
Hitbox* projHitbox = m_ecsSystem->GetComponent<Hitbox>(projEntity);
Projectile* proj = m_ecsSystem->GetComponent<Projectile>(projEntity);
```

**Analysis:** 5 enemies × 8 projectiles × 3 GetComponent = 120 calls/frame

**Status:** ✅ Already optimized (we pass sprite/stateAnim now)

But projectile components should also be cached!

---

### File: `CameraSystem.cpp`

**Lines 101-323: UpdateParallaxLayers()**
```cpp
auto parallaxEntities = m_ecsSystem->GetEntitiesWithComponents<...>();  // Every frame!
std::map<int, std::vector<Entity>> layerEntities;  // Allocated every frame!
```

**FIX:** Member variables, rebuild only when parallax added/removed.

---

## 📋 IMPLEMENTATION PRIORITY

### Phase 1: Cache ECS Queries (Biggest win)
**Files:** `RenderSystem.cpp`, `CameraSystem.cpp`
**Estimated gain:** 40-60 FPS → 50-70 FPS
**Effort:** Medium (需要 add/remove hooks)

### Phase 2: Cache Texture Handles
**Files:** `RenderSystem.cpp`, `GameComponents.h`
**Estimated gain:** +5-10 FPS
**Effort:** Low

### Phase 3: Sprite Batching
**Files:** `RenderSystem.cpp`, `MetalRenderer` (Swift)
**Estimated gain:** +10-15 FPS
**Effort:** High (requires Metal buffer management)

### Phase 4: Instanced Rendering
**Files:** `RenderSystem.cpp`, Metal shaders
**Estimated gain:** +5-10 FPS
**Effort:** Very High (new shader pipeline)

---

## 🚨 SNOW BACKGROUNDS ISSUE

**Status:** Files ARE in xcassets with `.png` extension
**Texture IDs in Code:**
```cpp
config.backgroundLayers.emplace_back("SnowLevelBackLayerBackground.png", ...);
config.backgroundLayers.emplace_back("SnowLevelMidLayerBackground.png", ...);
config.backgroundLayers.emplace_back("SnowLevelFrontLayerBackground.png", ...);
config.backgroundLayers.emplace_back("SnowLevelFrontLayerTrees.png", ...);
```

**Problem:** RenderSystem might be stripping `.png` or adding path prefix

**Debug:** Check `GetFullTexturePath()` and texture loading logs

---

## 🎯 RECOMMENDED IMMEDIATE ACTION

1. **Implement Phase 1** (Cache ECS queries) - Will have biggest impact
2. **Implement Phase 2** (Cache texture handles) - Easy win
3. **Profile again** with FPS counter
4. **Debug snow backgrounds** with targeted logging

This should get us from 22 FPS → 60+ FPS.

Then Phase 3 (batching) for further optimization.



