# Sprite Batching Implementation Plan
**Date:** October 10, 2025  
**Goal:** Reduce draw calls from 1340 → 50-100 per frame (+20 FPS gain)

---

## 🎯 Strategy: Minimal Invasive Changes

We'll add batching while preserving your delegate/command/proxy architecture:
1. **Add batch delegates** to `PlatformDelegates`
2. **Group sprites by texture** in `RenderSystem::RenderWorldSpace()`
3. **Implement batch renderer** in `MetalRenderer.swift`
4. **Wire through ThreadingProxy** for thread safety

**NO breaking changes to existing code - batching is additive!**

---

## 📋 Implementation Steps

### Phase 1: Add Batch Support to Delegates (30 min)

**File:** `src/Engine/Platform/PlatformDelegates.h`

**Changes:**
1. Add `SpriteBatchData` struct
2. Add `drawSpriteBatch` delegate function
3. Add `CMD_DRAW_SPRITE_BATCH` command type

```cpp
// Add after line 171 (after text outline commands)
CMD_DRAW_SPRITE_BATCH = 44,  // NEW: Batch rendering

// Add new batch data structure after line 202
struct SpriteBatchData {
    uint32_t textureHandle;
    float x, y;
    float scaleX, scaleY;
    float rotation;
    float sourceX, sourceY, sourceWidth, sourceHeight;  // For sprite sheets
    
    SpriteBatchData() : textureHandle(0), x(0), y(0), scaleX(1), scaleY(1), rotation(0),
                        sourceX(0), sourceY(0), sourceWidth(0), sourceHeight(0) {}
};

// Modify RenderCommandData to support batch (after line 202)
std::vector<SpriteBatchData> batchData;  // For CMD_DRAW_SPRITE_BATCH

// Add to RendererDelegate (after line 309)
void (*drawSpriteBatch)(const std::vector<SpriteBatchData>& sprites);
```

**Why this works:** Batching is optional - existing single-sprite code still works!

---

### Phase 2: Implement Batch Grouping in RenderSystem (45 min)

**File:** `src/FloppyTurd/Systems/RenderSystem.cpp`

**Current RenderWorldSpace (SLOW):**
```cpp
void RenderSystem::RenderWorldSpace() {
    for (const RenderItem& item : m_renderQueue) {
        if (m_ecsSystem->HasComponent<UIElement>(item.entity)) continue;
        if (item.sprite && !m_platformDelegates.renderer.drawSprite) continue;
        
        RenderSingleItem(item);  // 1340 individual draw calls
    }
}
```

**NEW Optimized RenderWorldSpace (FAST):**
```cpp
void RenderSystem::RenderWorldSpace() {
    // Group sprites by texture for batching
    std::unordered_map<uint32_t, std::vector<const RenderItem*>> spriteBatches;
    std::vector<const RenderItem*> nonBatchableItems;  // Debug rects, special cases
    
    // Collect and group
    for (const RenderItem& item : m_renderQueue) {
        // Skip UI elements
        if (m_ecsSystem->HasComponent<UIElement>(item.entity)) {
            continue;
        }
        
        // Group sprites by texture handle
        if (item.sprite && item.sprite->textureHandleValid) {
            spriteBatches[item.sprite->cachedTextureHandle].push_back(&item);
        }
        // Non-sprites (debug, shapes) rendered individually
        else {
            nonBatchableItems.push_back(&item);
        }
    }
    
    // Render batches (NEW!)
    if (m_platformDelegates.renderer.drawSpriteBatch) {
        for (const auto& [textureHandle, items] : spriteBatches) {
            RenderSpriteBatch(textureHandle, items);
        }
    }
    // Fallback: Render individually if batching not available
    else {
        for (const auto& [textureHandle, items] : spriteBatches) {
            for (const RenderItem* item : items) {
                RenderSingleItem(*item);
            }
        }
    }
    
    // Render non-batchable items individually
    for (const RenderItem* item : nonBatchableItems) {
        RenderSingleItem(*item);
    }
}
```

**Add new helper function:**
```cpp
void RenderSystem::RenderSpriteBatch(uint32_t textureHandle, 
                                     const std::vector<const RenderItem*>& items) {
    if (items.empty()) return;
    
    // Build batch data
    std::vector<GameCore::SpriteBatchData> batchData;
    batchData.reserve(items.size());
    
    for (const RenderItem* item : items) {
        GameCore::SpriteBatchData data;
        data.textureHandle = textureHandle;
        
        // World to screen transform
        Gnosis::GNVector2 screenPos = WorldToScreen(item->transform->position);
        data.x = screenPos.x;
        data.y = screenPos.y;
        
        // Scale
        data.scaleX = item->sprite->width * item->transform->scale.x * GetCameraScale();
        data.scaleY = item->sprite->height * item->transform->scale.y * GetCameraScale();
        
        // Rotation
        data.rotation = item->transform->rotation;
        
        // Source rect (for sprite sheets)
        data.sourceX = item->sprite->frameX;
        data.sourceY = item->sprite->frameY;
        data.sourceWidth = item->sprite->frameWidth;
        data.sourceHeight = item->sprite->frameHeight;
        
        batchData.push_back(data);
    }
    
    // Single batch draw call for all sprites with this texture
    m_platformDelegates.renderer.drawSpriteBatch(batchData);
}
```

**Add to RenderSystem.h:**
```cpp
private:
    void RenderSpriteBatch(uint32_t textureHandle, 
                          const std::vector<const RenderItem*>& items);
```

---

### Phase 3: Implement Metal Batch Renderer (60 min)

**File:** `src/iOS/Rendering/MetalRenderer.swift`

**Add instanced rendering support:**

```swift
// Add after line 163 (profiling variables)

// MARK: - Batch Rendering State
private var instanceBuffer: MTLBuffer?
private var maxBatchInstances: Int = 2048  // Support up to 2048 sprites per batch

// Batch instance structure (matches what GPU expects)
struct SpriteInstanceData {
    var modelMatrix: simd_float4x4
    var texCoords: SIMD4<Float>  // (u0, v0, u1, v1)
}
```

**Add batch drawing function (add after `present()` function ~line 664):**

```swift
public func drawSpriteBatch(_ sprites: [GameCore.SpriteBatchData]) {
    startTiming("DrawSpriteBatch")
    defer { endTiming("DrawSpriteBatch") }
    
    guard !sprites.isEmpty else { return }
    guard let device = device else { return }
    guard let renderEncoder = ensureRenderEncoder() else { return }
    guard let pipelineState = texturedPipelineState else { return }
    
    // Ensure instance buffer is large enough
    let requiredSize = sprites.count * MemoryLayout<SpriteInstanceData>.stride
    if instanceBuffer == nil || (instanceBuffer?.length ?? 0) < requiredSize {
        let bufferSize = max(requiredSize, maxBatchInstances * MemoryLayout<SpriteInstanceData>.stride)
        instanceBuffer = device.makeBuffer(length: bufferSize, options: .storageModeShared)
    }
    
    guard let instanceBuffer = instanceBuffer else { return }
    
    // Fill instance buffer
    let instancePtr = instanceBuffer.contents().bindMemory(
        to: SpriteInstanceData.self,
        capacity: sprites.count
    )
    
    // Get projection matrix once
    guard let uniformBuffer = uniformBuffer else { return }
    let projectionMatrix = uniformBuffer.contents().bindMemory(
        to: simd_float4x4.self, capacity: 1
    ).pointee
    
    for (i, sprite) in sprites.enumerated() {
        // Create model matrix for this sprite
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: sprite.x, y: sprite.y),
            scale: (x: sprite.scaleX, y: sprite.scaleY),
            rotation: sprite.rotation
        )
        
        // Combine with projection
        let mvpMatrix = projectionMatrix * modelMatrix
        
        // Texture coordinates (full texture or sprite sheet region)
        var texCoords = SIMD4<Float>(0, 0, 1, 1)
        
        // If source rect specified, calculate UV coords
        if sprite.sourceWidth > 0 && sprite.sourceHeight > 0 {
            // Get texture to calculate UV
            if let texture = textures[sprite.textureHandle] {
                let texWidth = Float(texture.width)
                let texHeight = Float(texture.height)
                texCoords = SIMD4<Float>(
                    sprite.sourceX / texWidth,
                    sprite.sourceY / texHeight,
                    (sprite.sourceX + sprite.sourceWidth) / texWidth,
                    (sprite.sourceY + sprite.sourceHeight) / texHeight
                )
            }
        }
        
        instancePtr[i] = SpriteInstanceData(
            modelMatrix: mvpMatrix,
            texCoords: texCoords
        )
    }
    
    // Get the first sprite's texture (all sprites in batch use same texture)
    guard let texture = textures[sprites[0].textureHandle] else { return }
    
    // Bind texture once for entire batch
    renderEncoder.setFragmentTexture(texture, index: 0)
    renderEncoder.setFragmentSamplerState(samplerState, index: 0)
    renderEncoder.setRenderPipelineState(pipelineState)
    
    // Set vertex buffer (quad vertices)
    renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
    
    // Set instance buffer
    renderEncoder.setVertexBuffer(instanceBuffer, offset: 0, index: 1)
    
    // Draw all instances in ONE call!
    renderEncoder.drawPrimitives(
        type: .triangle,
        vertexStart: 0,
        vertexCount: 6,  // 2 triangles = 6 vertices per quad
        instanceCount: sprites.count  // Draw N instances
    )
}
```

**Note:** This requires updating the Metal shader to support instancing, but we can do that in Phase 4.

---

### Phase 4: Wire Through ThreadingProxy (30 min)

**File:** `src/Engine/Threading/ThreadingProxy.h` & `.mm`

**Add batch command enqueueing:**

```cpp
// In ThreadingProxy.h (add to public methods)
static void enqueueDrawSpriteBatch(const std::vector<GameCore::SpriteBatchData>& sprites);

// In ThreadingProxy.mm implementation
void ThreadingProxy::enqueueDrawSpriteBatch(const std::vector<GameCore::SpriteBatchData>& sprites) {
    GameCore::RenderCommand cmd(GameCore::CommandType::CMD_DRAW_SPRITE_BATCH);
    cmd.data.batchData = sprites;  // Copy batch data
    
    enqueueRenderCommand(cmd);
}
```

**Update CommandProcessor to handle batch commands:**

```swift
// In CommandProcessor.swift, add to processRenderCommand:
case .CMD_DRAW_SPRITE_BATCH:
    if let batchData = cmd.data.batchData {
        metalRenderer.drawSpriteBatch(batchData)
    }
```

---

### Phase 5: Update iOS Platform Setup (15 min)

**File:** `src/Engine/Platform/iOSPlatformImpl.cpp`

**Add batch delegate:**

```cpp
void SetupDelegates(PlatformDelegates& delegates) {
    // ... existing code ...
    
    // Add batch rendering support
    delegates.renderer.drawSpriteBatch = [](const std::vector<GameCore::SpriteBatchData>& sprites) {
        GameCore::ThreadingProxy::enqueueDrawSpriteBatch(sprites);
    };
    
    // ... rest of setup ...
}
```

---

## 🔧 Metal Shader Update (Optional - Phase 6)

**File:** `src/iOS/Rendering/Shaders.metal`

**Current vertex shader:**
```metal
vertex VertexOut vertexShader(
    const device Vertex* vertices [[buffer(0)]],
    constant float4x4& mvpMatrix [[buffer(1)]],
    uint vertexID [[vertex_id]]
) {
    VertexOut out;
    out.position = mvpMatrix * float4(vertices[vertexID].position, 0.0, 1.0);
    out.texCoord = vertices[vertexID].texCoord;
    return out;
}
```

**NEW instanced vertex shader:**
```metal
struct InstanceData {
    float4x4 mvpMatrix;
    float4 texCoords;  // (u0, v0, u1, v1)
};

vertex VertexOut vertexShaderInstanced(
    const device Vertex* vertices [[buffer(0)]],
    const device InstanceData* instances [[buffer(1)]],
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]]
) {
    VertexOut out;
    
    // Get per-instance data
    InstanceData instance = instances[instanceID];
    
    // Transform vertex position
    out.position = instance.mvpMatrix * float4(vertices[vertexID].position, 0.0, 1.0);
    
    // Apply per-instance texture coordinates
    float2 baseUV = vertices[vertexID].texCoord;
    float2 uvMin = instance.texCoords.xy;
    float2 uvMax = instance.texCoords.zw;
    out.texCoord = uvMin + baseUV * (uvMax - uvMin);
    
    return out;
}
```

---

## 🎯 Testing Plan

### Test 1: Verify Batching Works
```cpp
// Add debug logging to RenderSpriteBatch
GN_LOG_INFO("Batching " + std::to_string(items.size()) + " sprites for texture " + std::to_string(textureHandle));
```

**Expected:** See logs like "Batching 50 sprites for texture 12345"

### Test 2: Verify Draw Call Reduction
**Before:** MetalProfiler shows `DrawSpriteScaled calls=1340`  
**After:** MetalProfiler should show `DrawSpriteBatch calls=50-100`

### Test 3: Verify Visual Correctness
- All sprites render correctly
- No flickering or missing sprites
- Animations still work
- Rotation/scale correct

### Test 4: Performance Gain
**Before:** 40-50 FPS on Level 5  
**After:** 55-65 FPS on Level 5 (**+15-20 FPS expected**)

---

## ⚡ Quick Win: Frustum Culling Fix (10 min)

While implementing batching, also fix the incomplete culling:

**File:** `src/FloppyTurd/Systems/RenderSystem.cpp` (line 164-169)

**Current (BROKEN):**
```cpp
float entityScreenX = WorldToScreen(transform->position).x;
if (entityScreenX > screenWidth + cullMargin) {
    continue;  // Only culls RIGHT side!
}
```

**Fixed (PROPER 4-SIDE CULLING):**
```cpp
Gnosis::GNVector2 screenPos = WorldToScreen(transform->position);
float halfWidth = (sprite->width * transform->scale.x) / 2.0f;
float halfHeight = (sprite->height * transform->scale.y) / 2.0f;

// Cull all 4 directions
if (screenPos.x + halfWidth < -cullMargin ||                           // Left
    screenPos.x - halfWidth > screenWidth + cullMargin ||              // Right  
    screenPos.y + halfHeight < -cullMargin ||                          // Top
    screenPos.y - halfHeight > m_screenInfo.pixelHeight + cullMargin)  // Bottom
{
    continue;
}
```

**Expected gain:** Additional +8-10 FPS (30-40% fewer sprites rendered)

---

## 📊 Expected Results

| Optimization | Draw Calls Before | Draw Calls After | FPS Gain |
|-------------|------------------|------------------|----------|
| **Sprite Batching** | 1340 | 50-100 | +15-20 |
| **Frustum Culling Fix** | 100% rendered | 60-70% rendered | +8-10 |
| **TOTAL** | 1340 calls | 30-70 calls | **+23-30 FPS** |

**Target Performance:**
- Level 1: 60 FPS → **60 FPS** (stable, no pipe drops)
- Level 2: 45-50 FPS → **65-70 FPS**
- Level 5: 40-50 FPS → **63-73 FPS** ✅ **TARGET ACHIEVED!**

---

## 🚀 Implementation Order

### Day 1 (2-3 hours):
1. ✅ Phase 1: Add delegates (30 min)
2. ✅ Phase 2: RenderSystem batching (45 min)
3. ✅ Phase 5: iOS platform setup (15 min)
4. ✅ Quick Win: Fix frustum culling (10 min)
5. ✅ **Test & verify visual correctness** (30 min)

**Expected result:** Batching infrastructure in place, fallback to individual rendering

### Day 2 (2-3 hours):
6. ✅ Phase 3: MetalRenderer batch implementation (60 min)
7. ✅ Phase 4: ThreadingProxy wiring (30 min)
8. ✅ Phase 6: Update Metal shaders (30 min)
9. ✅ **Test & measure FPS gains** (45 min)

**Expected result:** Full batching working, 60 FPS on all levels

---

## 🎯 Text Optimization (OPTIONAL - Later)

Text is simpler to optimize and can wait:

**Quick Fix (5 min):**
- Cache score/pipe counter textures (don't re-rasterize every frame)
- Combine multi-line text into single texture

**Expected gain:** +3-5 FPS

**Full Fix (2 hours):**
- Pre-render UI text atlas on level load
- Batch text drawing like sprites

**Expected gain:** +10-15 FPS additional

---

## ✅ Success Criteria

1. **Build succeeds** with no errors
2. **Visual correctness** - all sprites render properly
3. **FPS improvement** - +20-30 FPS gain on Level 5
4. **Stable 60 FPS** on all levels
5. **No regressions** - existing features still work

---

## 🎮 Ready to Start?

**Recommended approach:**
1. Start with **Phase 1 & 2** (delegates + RenderSystem grouping)
2. Test with fallback to individual rendering (visual verification)
3. Then add **Phase 3** (Metal batch renderer)
4. Test and measure performance gains
5. Iterate if needed

**This plan maintains your architecture while adding batching as an optimization layer!**
