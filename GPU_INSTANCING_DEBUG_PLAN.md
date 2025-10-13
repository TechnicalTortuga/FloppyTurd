# GPU Instancing Debug Plan

## Current Problem
All sprites are rendering stacked at the same position, even though logs show different data.

## What We've Verified
1. ✅ Pipeline created: `✅ Instanced sprite pipeline created successfully`
2. ✅ Shaders exist: `spriteVertexInstanced`, `spriteFragmentInstanced`
3. ✅ Data is correct in C++: positions vary (375→391 for Y)
4. ✅ Data reaches Swift: ThreadingSystem receives correct values
5. ✅ Instance buffer created: 512 sprite capacity

## Possible Root Causes

### 1. Matrix Transformation Bug
**Theory:** Model matrix not transforming vertices correctly
**Test:** Log the actual matrix values in the instance buffer
**Evidence:** Need to see matrix.columns.3 (translation component)

### 2. Vertex Buffer Mismatch
**Theory:** instancedVertexBuffer has wrong format
**Current:** `[x, y, u, v]` = 16 bytes per vertex
**Shader expects:** `float2 position [[attribute(0)]], float2 texCoord [[attribute(1)]]`
**Status:** Should be correct, but need to verify vertex descriptor

### 3. Instance ID Not Incrementing
**Theory:** All instances read `instances[0]`
**Test:** Add shader debugging or GPU capture
**Likelihood:** Low (Metal handles this automatically)

### 4. Buffer Binding Wrong
**Theory:** Instance buffer bound to wrong index
**Current:** `setVertexBuffer(instanceBuffer, offset: 0, index: 2)`
**Shader:** `constant SpriteInstanceData* instances [[buffer(2)]]`
**Status:** Matches correctly

## Next Steps

### Option A: Revert to Non-Instanced (Immediate Fix)
- Remove GPU instancing
- Go back to individual draw calls
- Fixes rendering, loses performance gains
- **USER WOULD HATE THIS** (aggressive refactoring preference)

### Option B: Debug Shader with GPU Capture
1. Use Xcode's Metal Frame Debugger
2. Capture a frame
3. Inspect instance buffer contents
4. Verify shader is reading correct data

### Option C: Simplify to Minimal Test
1. Create 2 sprites at (100, 100) and (200, 200)
2. Same texture, different positions
3. Draw with count=2
4. If they stack → shader bug
5. If they separate → batching bug

### Option D: Check Online Research Again
Look for "Metal 2D sprite instancing tutorial" examples to compare our implementation.

## Questions to Resolve
1. Is the quad (0,0)→(1,1) or (0,0)→(1,1) in which coordinate space?
2. Does the model matrix need to include the sprite dimensions?
3. Is the projection matrix correct for our coordinate system?

## Recommended Next Action
**Try Option C** - Create minimal test with 2 hardcoded sprites to isolate the bug.
