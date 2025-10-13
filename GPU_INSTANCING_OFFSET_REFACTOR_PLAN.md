# GPU Instancing Per-Batch Offset Refactor Plan

**Date**: October 13, 2025  
**Author**: Cascade AI Assistant  
**Status**: Planning → Implementation

---

## Executive Summary

**Problem**: All instanced sprites render at the same position because multiple batches per frame overwrite the same buffer offset (0), stomping previously written instance data before the GPU reads it.

**Root Cause**: `drawSpriteBatch()` always writes to offset 0 of the current triple-buffered instance buffer, even when multiple batches are submitted within the same frame.

**Solution**: Introduce per-frame instance offset tracking, write to sequential buffer regions, and bind buffers with proper byte offsets for each batch.

---

## Systems & Files to Modify

### 1. **MetalRenderer.swift** (`src/iOS/Rendering/MetalRenderer.swift`)
Primary rendering system that manages Metal pipelines, buffers, and draw calls.

#### New State Variables
```swift
// Add to class properties around line 140-150
private var currentInstanceOffset: Int = 0  // Current write position in instance buffer (in sprite count)
private var instancedDrawCallsThisFrame: Int = 0  // Diagnostic counter
```

#### Functions to Modify

**`beginFrame()` (lines 707-745)**
- **Change**: Reset `currentInstanceOffset = 0` and `instancedDrawCallsThisFrame = 0` when rotating buffers
- **Reason**: Fresh frame = fresh buffer region
- **Hook**: Already rotates `currentBufferIndex`, now also clears offset

**`drawSpriteBatch()` (lines 1431-1565)**
- **Change**: Major refactor - core instancing logic
- **New Logic**:
  1. Compute available capacity: `remainingCapacity = maxSpritesPerBatch - currentInstanceOffset`
  2. Check if batch fits: if `remainingCapacity == 0`, handle buffer overflow
  3. Write instance data at offset: `instancePointer + currentInstanceOffset`
  4. Bind buffer with byte offset: `offset: currentInstanceOffset * MemoryLayout<SpriteInstanceData>.stride`
  5. Advance offset: `currentInstanceOffset += writtenSprites`
  6. Handle large batches: split into sub-batches if needed

**`endFrame()` (lines 747-754)**
- **Change**: Optional - add diagnostic logging for instance buffer usage
- **Hook**: Log `currentInstanceOffset` and `instancedDrawCallsThisFrame` at frame end

---

## Data Flow Diagram

### Before (Broken)
```
Frame Start
  ↓
currentBufferIndex = (currentBufferIndex + 1) % 3
  ↓
Batch 1: Write to instanceBuffer[0...N] at offset 0  ← sprites 0-N
  ↓                                                      
Batch 2: Write to instanceBuffer[0...M] at offset 0  ← OVERWRITES sprites 0-M (stomps Batch 1!)
  ↓
Batch 3: Write to instanceBuffer[0...K] at offset 0  ← OVERWRITES sprites 0-K (stomps all!)
  ↓
GPU reads: Only sees Batch 3 data for ALL draws
  ↓
Result: All sprites render at same position 😭
```

### After (Fixed)
```
Frame Start
  ↓
currentBufferIndex = (currentBufferIndex + 1) % 3
currentInstanceOffset = 0
  ↓
Batch 1 (10 sprites): 
  - Check capacity: 512 - 0 = 512 ✓
  - Write to instanceBuffer[0...9]
  - Bind buffer with offset: 0 * 96 = 0 bytes
  - Draw 10 instances
  - Advance offset: 0 + 10 = 10
  ↓
Batch 2 (5 sprites):
  - Check capacity: 512 - 10 = 502 ✓
  - Write to instanceBuffer[10...14]
  - Bind buffer with offset: 10 * 96 = 960 bytes
  - Draw 5 instances
  - Advance offset: 10 + 5 = 15
  ↓
Batch 3 (8 sprites):
  - Check capacity: 512 - 15 = 497 ✓
  - Write to instanceBuffer[15...22]
  - Bind buffer with offset: 15 * 96 = 1440 bytes
  - Draw 8 instances
  - Advance offset: 15 + 8 = 23
  ↓
GPU reads: Each draw sees correct region of instance buffer
  ↓
Result: All sprites render at independent positions! 🎉
```

---

## Implementation Plan

### Phase 1: Add Offset Tracking State
**File**: `MetalRenderer.swift`  
**Location**: Class properties section (around line 140-150)

```swift
// GPU Instancing - per-frame offset tracking
private var currentInstanceOffset: Int = 0  // Sprites written to current buffer
private var instancedDrawCallsThisFrame: Int = 0  // Draw calls this frame
private var instanceBufferOverflowCount: Int = 0  // Lifetime overflow counter
```

---

### Phase 2: Reset Offset in beginFrame()
**File**: `MetalRenderer.swift`  
**Function**: `beginFrame()` (line 707)

**Add after buffer rotation (line 712)**:
```swift
// Rotate triple buffer index for instance data
currentBufferIndex = (currentBufferIndex + 1) % maxBuffersInFlight

// CRITICAL: Reset instance offset for fresh buffer
currentInstanceOffset = 0
instancedDrawCallsThisFrame = 0
```

---

### Phase 3: Refactor drawSpriteBatch() - Core Logic
**File**: `MetalRenderer.swift`  
**Function**: `drawSpriteBatch()` (lines 1431-1565)

#### 3.1 Check Capacity Before Writing
**Insert after sprite count validation (line 1474)**:
```swift
// Check remaining capacity in current instance buffer
let remainingCapacity = maxSpritesPerBatch - currentInstanceOffset

// Handle buffer overflow: if no space left, log warning and skip
// (Alternative: flush encoder + rotate buffer, but adds complexity)
guard remainingCapacity > 0 else {
    log("⚠️ Instance buffer FULL (offset=\(currentInstanceOffset)/\(maxSpritesPerBatch)) - skipping batch of \(sprites.count) sprites", level: .warning)
    instanceBufferOverflowCount += 1
    return
}

// Clamp batch size to available capacity
let spritesToDraw = min(sprites.count, remainingCapacity)
if spritesToDraw < sprites.count {
    log("⚠️ Batch truncated: requested \(sprites.count) sprites, only \(spritesToDraw) fit in remaining capacity", level: .warning)
}
```

#### 3.2 Write Instance Data at Offset
**Replace lines 1481-1543 (instance data fill loop)**:
```swift
// Calculate write offset in instance buffer
let writeOffset = currentInstanceOffset
let instancePointer = instanceBuffer.contents().bindMemory(
    to: SpriteInstanceData.self, capacity: maxSpritesPerBatch
)

// DEBUG: Log offset and batch info
if shouldLog || writeOffset == 0 {
    log("📦 BATCH #\(instancedDrawCallsThisFrame): offset=\(writeOffset) count=\(spritesToDraw)/\(sprites.count) texture=\(firstSprite.textureHandle)", level: .info)
}

// Fill instance data starting at writeOffset
for (i, sprite) in sprites.prefix(spritesToDraw).enumerated() {
    let bufferIndex = writeOffset + i  // CRITICAL: Write at offset, not i!
    
    // ... existing sprite dimension calculation ...
    let spriteWidth: Float
    let spriteHeight: Float
    if sprite.sourceWidth > 0 && sprite.sourceHeight > 0 {
        spriteWidth = sprite.sourceWidth * sprite.scaleX
        spriteHeight = sprite.sourceHeight * sprite.scaleY
    } else {
        spriteWidth = Float(texture.width) * sprite.scaleX
        spriteHeight = Float(texture.height) * sprite.scaleY
    }
    
    // Build model matrix
    let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
        position: (x: sprite.x, y: sprite.y),
        scale: (x: spriteWidth, y: spriteHeight),
        rotation: sprite.rotation
    )
    
    // Calculate UV rectangle
    let uvRect: SIMD4<Float>
    if sprite.sourceWidth > 0 && sprite.sourceHeight > 0 {
        let texWidth = Float(texture.width)
        let texHeight = Float(texture.height)
        uvRect = SIMD4<Float>(
            sprite.sourceX / texWidth,
            sprite.sourceY / texHeight,
            (sprite.sourceX + sprite.sourceWidth) / texWidth,
            (sprite.sourceY + sprite.sourceHeight) / texHeight
        )
    } else {
        uvRect = SIMD4<Float>(0, 0, 1, 1)
    }
    
    // Write to instance buffer at offset position
    instancePointer[bufferIndex] = SpriteInstanceData(
        modelMatrix: modelMatrix,
        uvRect: uvRect,
        color: SIMD4<Float>(1, 1, 1, 1)
    )
    
    // DEBUG: Log first sprite transform of each batch
    if i == 0 {
        let translationX = modelMatrix.columns.3.x
        let translationY = modelMatrix.columns.3.y
        log("  🔬 Batch[\(instancedDrawCallsThisFrame)] Sprite[0 @ buffer[\(bufferIndex)]]: translate=(\(translationX), \(translationY)) input=(\(sprite.x),\(sprite.y))", level: .info)
    }
}
```

#### 3.3 Bind Buffer with Byte Offset
**Replace lines 1548-1554 (encoder setup)**:
```swift
// Calculate byte offset for this batch
let byteOffset = writeOffset * MemoryLayout<SpriteInstanceData>.stride

// Create frame uniforms
var frameUniforms = FrameUniforms(projectionMatrix: projectionMatrix)

// Set up render encoder with OFFSET binding
renderEncoder.setRenderPipelineState(pipelineState)
renderEncoder.setVertexBuffer(instancedVB, offset: 0, index: 0)  // Quad vertices
renderEncoder.setVertexBytes(&frameUniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
renderEncoder.setVertexBuffer(instanceBuffer, offset: byteOffset, index: 2)  // ← CRITICAL: Use byte offset!
renderEncoder.setFragmentTexture(texture, index: 0)
renderEncoder.setFragmentSamplerState(samplerState, index: 0)

// DEBUG: Log buffer binding
if shouldLog || writeOffset == 0 {
    log("  🎯 Binding buffer: offset=\(byteOffset) bytes (\(writeOffset) sprites * \(MemoryLayout<SpriteInstanceData>.stride) bytes/sprite)", level: .info)
}
```

#### 3.4 Advance Offset After Draw
**Add after draw call (line 1564)**:
```swift
// ★★★ ONE DRAW CALL FOR ALL SPRITES IN THIS BATCH! ★★★
renderEncoder.drawIndexedPrimitives(
    type: .triangle,
    indexCount: 6,
    indexType: .uint16,
    indexBuffer: indexBuffer,
    indexBufferOffset: 0,
    instanceCount: spritesToDraw  // Draw only sprites that fit
)

// CRITICAL: Advance instance offset for next batch
currentInstanceOffset += spritesToDraw
instancedDrawCallsThisFrame += 1

// DEBUG: Log buffer state after draw
if shouldLog {
    log("  ✅ Batch complete: new offset=\(currentInstanceOffset)/\(maxSpritesPerBatch) usage=\(Int(Float(currentInstanceOffset)/Float(maxSpritesPerBatch)*100))%", level: .info)
}
```

---

### Phase 4: Add Diagnostics & Guard Rails

#### 4.1 Frame-End Logging
**File**: `MetalRenderer.swift`  
**Function**: `endFrame()` (line 747)

**Add before ending encoder**:
```swift
// Log instance buffer usage stats
if instancedDrawCallsThisFrame > 0 {
    let usagePercent = Int(Float(currentInstanceOffset) / Float(maxSpritesPerBatch) * 100)
    log("📊 Frame buffer usage: \(currentInstanceOffset)/\(maxSpritesPerBatch) sprites (\(usagePercent)%) across \(instancedDrawCallsThisFrame) batches", level: .debug)
}

if instanceBufferOverflowCount > 0 && frameCount % 60 == 0 {
    log("⚠️ Instance buffer overflow count: \(instanceBufferOverflowCount) (lifetime)", level: .warning)
}
```

#### 4.2 Capacity Overflow Handling (Future Enhancement)
**Option A - Skip batch** (implemented above)  
**Option B - Flush & rotate** (more complex, implement if needed):
```swift
// If buffer is full, flush current encoder and rotate
if remainingCapacity == 0 {
    log("⚠️ Instance buffer full - flushing encoder and rotating buffer", level: .warning)
    currentRenderEncoder?.endEncoding()
    currentRenderEncoder = nil
    currentInstanceOffset = 0
    // Re-try draw with fresh buffer
}
```

---

## Validation & Testing

### Test Cases

1. **Single batch per frame** (baseline)
   - Verify offset starts at 0
   - Verify single batch renders correctly
   - Verify offset advances to batch size

2. **Multiple small batches** (core fix validation)
   - Submit 3-5 batches of different sizes
   - Verify each uses sequential offsets
   - Verify all sprites render at independent positions
   - Check logs for correct offset progression

3. **Large batch edge case**
   - Submit batch with 500+ sprites
   - Verify truncation/splitting if needed
   - Check capacity warnings

4. **Buffer near-full scenario**
   - Submit batches totaling near 512 sprites
   - Verify final batch truncates or skips gracefully
   - Check overflow logging

5. **Cross-frame validation**
   - Verify offset resets to 0 at frame start
   - Verify triple-buffering still works (no tearing)

### Debug Logging Checklist

- [x] Offset value at batch start
- [x] Byte offset calculation
- [x] Buffer write index for first sprite
- [x] Transform values (compare to input)
- [x] Capacity remaining
- [x] Batch truncation warnings
- [x] Frame-end usage stats

---

## Shader Verification (No Changes Needed)

The Metal shader `spriteVertexInstanced()` already correctly uses `instanceID` to index the instance buffer:

```metal
vertex InstancedVertexOut spriteVertexInstanced(
    InstancedVertexIn in [[stage_in]],
    constant FrameUniforms& frameUniforms [[buffer(1)]],
    constant SpriteInstanceData* instances [[buffer(2)]],  // ← Buffer bound with offset
    uint instanceID [[instance_id]])  // ← GPU provides 0, 1, 2, ...
{
    SpriteInstanceData instance = instances[instanceID];  // ← Correct indexing!
    // ... transform logic ...
}
```

**Why this works with offsets**:
- Metal's `instanceID` is relative to the draw call (0, 1, 2, ...)
- When we bind `buffer(2)` with byte offset `N * 96`, the GPU sees `instances[0]` at buffer byte `N * 96`
- So `instances[instanceID]` correctly reads from our offset region
- **No shader changes needed!** ✅

---

## Memory Layout Validation

**SpriteInstanceData size**: 96 bytes (verified)
- `modelMatrix`: `simd_float4x4` = 64 bytes
- `uvRect`: `SIMD4<Float>` = 16 bytes
- `color`: `SIMD4<Float>` = 16 bytes
- **Total**: 96 bytes ✅

**Buffer capacity**: 512 sprites * 96 bytes = 49,152 bytes per buffer  
**Triple buffering**: 3 buffers * 49,152 = 147,456 bytes total  
**Alignment**: 96 bytes is 16-byte aligned ✅

---

## Rollback Plan

If issues arise:
1. Revert `drawSpriteBatch()` to always write at offset 0
2. Set `currentInstanceOffset = 0` before each batch (temporary fix)
3. Keep diagnostic logging to analyze multi-batch scenarios
4. Original code preserved in git history

---

## Success Criteria

✅ Multiple batches per frame render at independent positions  
✅ No visual artifacts or overlapping sprites  
✅ Instance buffer offset advances correctly  
✅ No buffer overflows or crashes  
✅ 60 FPS maintained (no performance regression)  
✅ Debug logs confirm correct offset progression  
✅ Existing single-batch rendering still works  

---

## Implementation Sequence

1. ✅ Create this plan document
2. ⏳ Add state variables (`currentInstanceOffset`, counters)
3. ⏳ Modify `beginFrame()` to reset offset
4. ⏳ Refactor `drawSpriteBatch()` core logic
5. ⏳ Add capacity guards and diagnostics
6. ⏳ Build and test on iOS Simulator
7. ⏳ Verify multi-batch rendering via logs
8. ⏳ Visual validation in-game
9. ⏳ Performance profiling
10. ⏳ Clean up debug logs (keep essential ones)

---

## Notes & Assumptions

- **Triple buffering preserved**: Still rotating `currentBufferIndex` per frame
- **No shader changes**: GPU `instanceID` indexing already correct
- **Coordinate system unchanged**: Still using pixel coordinates (per memory guidance)
- **Single shared ECS**: Per established architecture (memory guidance)
- **Max batch size**: 512 sprites sufficient for current game workload
- **Overflow strategy**: Start with skip/truncate, implement flush if needed

---

**END OF PLAN**  
Ready to proceed with implementation.
