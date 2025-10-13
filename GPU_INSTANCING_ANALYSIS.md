# GPU Instancing Implementation Analysis

## Research Sources

### 1. Metal by Example - Instanced Rendering
**URL:** https://metalbyexample.com/instanced-rendering/

**Key Takeaways:**
- Instanced rendering uses `drawIndexedPrimitives` with `instanceCount` parameter
- Per-instance data stored in a buffer bound to vertex shader
- Shader accesses instance data via `[[instance_id]]` attribute
- Buffer layout: `buffer(0)` = vertices, `buffer(1)` = shared uniforms, `buffer(2)` = per-instance uniforms

**Their Code Structure:**
```objective-c
// Setup buffers
[commandEncoder setVertexBuffer:mesh.vertexBuffer offset:0 atIndex:0];
[commandEncoder setVertexBuffer:sharedUniformBuffer offset:0 atIndex:1];
[commandEncoder setVertexBuffer:perInstanceBuffer offset:0 atIndex:2];

// Draw call
[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                           indexCount:indexCount
                            indexType:MTLIndexTypeUInt16
                          indexBuffer:mesh.indexBuffer
                    indexBufferOffset:0
                        instanceCount:instanceCount];
```

**Their Shader:**
```metal
vertex ProjectedVertex vertex_project(
    constant InVertex *vertices [[buffer(0)]],
    constant Uniforms &uniforms [[buffer(1)]],
    constant PerInstanceUniforms *perInstanceUniforms [[buffer(2)]],
    ushort vid [[vertex_id]],
    ushort iid [[instance_id]])
{
    float4x4 instanceModelMatrix = perInstanceUniforms[iid].modelMatrix;
    // Transform with instance-specific matrix
    float4 worldPos = instanceModelMatrix * float4(vertex.position, 1.0);
    // Then apply shared projection
    return uniforms.projectionMatrix * worldPos;
}
```

---

## Our Implementation Comparison

### Buffer Setup (MetalRenderer.swift:1528-1535)

**Our Code:**
```swift
renderEncoder.setRenderPipelineState(pipelineState)
renderEncoder.setVertexBuffer(instancedVB, offset: 0, index: 0)  // Quad vertices (pos+uv)
renderEncoder.setVertexBytes(&frameUniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
renderEncoder.setVertexBuffer(instanceBuffer, offset: 0, index: 2)  // Per-instance data
renderEncoder.setFragmentTexture(texture, index: 0)
renderEncoder.setFragmentSamplerState(samplerState, index: 0)
```

**Comparison:**
- ✅ **MATCHES:** Buffer indices (0=vertices, 1=shared, 2=per-instance)
- ✅ **MATCHES:** Using `setVertexBuffer` for instance data
- ⚠️ **DIFFERENCE:** We use `setVertexBytes` for shared uniforms (should be fine for small data)

### Draw Call (MetalRenderer.swift:1523-1531)

**Our Code:**
```swift
renderEncoder.drawIndexedPrimitives(
    type: .triangle,
    indexCount: 6,
    indexType: .uint16,
    indexBuffer: indexBuffer,
    indexBufferOffset: 0,
    instanceCount: spriteCount  // Multiple instances
)
```

**Comparison:**
- ✅ **MATCHES:** Using `drawIndexedPrimitives` with `instanceCount`
- ✅ **MATCHES:** 6 indices for quad (2 triangles)

### Shader (Shaders2D.metal:110-135)

**Our Code:**
```metal
vertex InstancedVertexOut spriteVertexInstanced(
    InstancedVertexIn in [[stage_in]],
    constant FrameUniforms& frameUniforms [[buffer(1)]],
    constant SpriteInstanceData* instances [[buffer(2)]],
    uint instanceID [[instance_id]])
{
    SpriteInstanceData instance = instances[instanceID];
    
    // Transform vertex by instance model matrix
    float4 worldPosition = instance.modelMatrix * float4(in.position, 0.0, 1.0);
    
    // Project to clip space
    float4 clipPosition = frameUniforms.projectionMatrix * worldPosition;
    
    // Calculate UVs
    float2 uv = mix(instance.uvRect.xy, instance.uvRect.zw, in.texCoord);
    
    InstancedVertexOut out;
    out.position = clipPosition;
    out.texCoord = uv;
    out.color = instance.color;
    return out;
}
```

**Comparison:**
- ✅ **MATCHES:** Using `[[instance_id]]` to index into instances array
- ✅ **MATCHES:** Fetching per-instance data via `instances[instanceID]`
- ✅ **MATCHES:** Applying model matrix first, then projection
- ⚠️ **DIFFERENCE:** We use `[[stage_in]]` for vertex input (requires vertex descriptor)

---

## CRITICAL FINDING: Vertex Descriptor Issue

### The Problem

When using `[[stage_in]]`, Metal uses the **vertex descriptor** to fetch vertex data. Our vertex buffer has stride=16 bytes (4 floats: x, y, u, v).

**Our Vertex Descriptor Setup (MetalRenderer.swift:682-695):**
```swift
let instancedVertexDescriptor = MTLVertexDescriptor()
// Position (float2)
instancedVertexDescriptor.attributes[0].format = .float2
instancedVertexDescriptor.attributes[0].offset = 0
instancedVertexDescriptor.attributes[0].bufferIndex = 0
// TexCoord (float2)
instancedVertexDescriptor.attributes[1].format = .float2
instancedVertexDescriptor.attributes[1].offset = 8
instancedVertexDescriptor.attributes[1].bufferIndex = 0
// Layout
instancedVertexDescriptor.layouts[0].stride = 16  // ✅ CORRECT
instancedVertexDescriptor.layouts[0].stepFunction = .perVertex  // ✅ CORRECT
```

**Vertex Buffer Data (MetalRenderer.swift:551-557):**
```swift
let instancedVertices: [Float] = [
    // Position (x, y), TexCoord (u, v)
    0.0, 1.0, 0.0, 1.0,  // Bottom-left
    1.0, 1.0, 1.0, 1.0,  // Bottom-right
    1.0, 0.0, 1.0, 0.0,  // Top-right
    0.0, 0.0, 0.0, 0.0,  // Top-left
]
```

✅ **This is CORRECT!** Stride=16, 4 vertices, proper layout.

---

## ACTUAL BUG DISCOVERED

### Issue: Matrix Column-Major vs Row-Major

Looking at our matrix creation (MetalMatrixHelpers.swift:23-30):

```swift
static func translationMatrix(x: Float, y: Float, z: Float = 0.0) -> simd_float4x4 {
    return simd_float4x4(
        simd_float4(1.0, 0.0, 0.0, 0.0),  // Column 0
        simd_float4(0.0, 1.0, 0.0, 0.0),  // Column 1
        simd_float4(0.0, 0.0, 1.0, 0.0),  // Column 2
        simd_float4(x, y, z, 1.0)         // Column 3 (translation)
    )
}
```

✅ This is correct - `simd_float4x4` is **column-major**.

### Log Analysis

From logs:
```
Matrix[0]: translate=(141.5, 376.35928) input=(141.5,376.35928) scale=(215.80731,308.29617)
Matrix[0]: translate=(141.5, 376.26648) input=(141.5,376.26648) scale=(896.0,640.0)
```

The translation component is CORRECT (column 3 of matrix = position).

---

## ACTUAL PROBLEM: Vertex Position Range

### The Critical Issue

Our vertex buffer defines a quad from (0,0) to (1,1):
```
0.0, 1.0  → bottom-left
1.0, 1.0  → bottom-right
1.0, 0.0  → top-right
0.0, 0.0  → top-left
```

**The shader does:**
```metal
float4 worldPosition = instance.modelMatrix * float4(in.position, 0.0, 1.0);
```

If `in.position` = (0, 0), and modelMatrix has translation (141.5, 376.0):
- Result: worldPosition = (141.5, 376.0, 0, 1) ✅ CORRECT

If `in.position` = (1, 1), and modelMatrix has scale (216, 308) and translation (141.5, 376.0):
- Result: worldPosition = (141.5 + 216, 376.0 + 308, 0, 1) = (357.5, 684.0) ✅ CORRECT

**This should work!** The quad spans from (141.5, 376) to (357.5, 684).

---

## HYPOTHESIS: Projection Matrix Issue

Let me check the projection matrix setup (MetalRenderer.swift:645-649):

```swift
let projectionMatrix = MetalMatrixHelpers.viewportProjectionMatrix(
    width: width, height: height)
```

**Implementation (MetalMatrixHelpers.swift:214-223):**
```swift
static func viewportProjectionMatrix(width: Float, height: Float) -> simd_float4x4 {
    return orthographicMatrix(
        left: 0.0,
        right: width,
        bottom: height,   // ← Bottom is HEIGHT
        top: 0.0,         // ← Top is 0
        near: -1.0,
        far: 1.0
    )
}
```

**Orthographic Matrix (MetalMatrixHelpers.swift:106-113):**
```swift
static func orthographicMatrix(left: Float, right: Float, bottom: Float, top: Float, near: Float, far: Float) -> simd_float4x4 {
    return simd_float4x4(
        simd_float4(2.0 / (right - left), 0, 0, 0),
        simd_float4(0, 2.0 / (top - bottom), 0, 0),
        simd_float4(0, 0, -2.0 / (far - near), 0),
        simd_float4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1)
    )
}
```

✅ This is the standard orthographic projection formula.

---

## ROOT CAUSE IDENTIFIED

### The Actual Bug: ALL Sprites Render at Same Position Due to REUSED BUFFER

**The issue is NOT the shader or matrices - it's that we're drawing SINGLE sprites per batch!**

From logs:
```
Layer 0: 1 batches
  Batch: 'MainMenuMobile' (handle=156), 1 sprites
Layer 1: 1 batches
  Batch: 'FloppyLogo' (handle=75), 1 sprites
```

Each sprite gets its own batch with `instanceCount=1`. When `instanceCount=1`, instancing doesn't provide any benefit, and we're essentially doing individual draw calls.

**But why do they STACK visually?**

Because the instance buffer is REUSED across multiple draw calls. Each draw call writes to `instancePointer[0]`, then immediately draws. The GPU sees different data each time, but if there's a synchronization issue or buffer coherency problem, it might read stale data.

### Metal Buffer Coherency

From Apple's Metal Best Practices:
> When using MTLResourceStorageModeShared, modifications made by the CPU are automatically visible to the GPU. However, you should avoid modifying a buffer that is currently being read by an in-flight command buffer.

**Our code (MetalRenderer.swift:1462-1467):**
```swift
let instancePointer = instanceBuffer.contents().bindMemory(
    to: SpriteInstanceData.self, capacity: maxSpritesPerBatch
)

let spriteCount = min(sprites.count, maxSpritesPerBatch)

for (i, sprite) in sprites.prefix(spriteCount).enumerated() {
    // Fill instancePointer[i]
}
```

We fill the buffer, then immediately issue the draw call. This SHOULD work with `.storageModeShared`.

---

## SOLUTION: Triple Buffering

The standard solution in Metal is **triple buffering** - maintain 3 instance buffers and rotate between them per frame.

**Why 3 buffers?**
- CPU can write to buffer N while GPU reads buffers N-1 and N-2
- Prevents CPU/GPU synchronization stalls
- Ensures data coherency

**Implementation:**
```swift
private var instanceBuffers: [MTLBuffer] = []
private var currentBufferIndex = 0
private let maxBuffersInFlight = 3

// In beginFrame():
currentBufferIndex = (currentBufferIndex + 1) % maxBuffersInFlight

// In drawSpriteBatch():
let instanceBuffer = instanceBuffers[currentBufferIndex]
```

This is mentioned in Apple's Metal documentation and is standard practice for dynamic buffers.

---

## Action Plan

1. **Implement Triple Buffering** for instance buffer
2. **Keep instancing enabled** - the shader is correct
3. **Fix batching** - ensure sprites with same texture actually batch together (separate issue)
4. **Use semaphores** to wait for GPU to finish with old buffers

This matches industry standard Metal rendering practices.
