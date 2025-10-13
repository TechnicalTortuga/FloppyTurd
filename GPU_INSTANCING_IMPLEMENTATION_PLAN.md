# GPU Instancing Implementation Plan for FloppyTurd

## 📚 Research Summary

### Sources Analyzed:
1. **Metal by Example - Instanced Rendering**
   - Official Metal instancing patterns
   - How to use `[[instance_id]]` attribute
   - Per-instance uniform buffer structure

2. **Cocos2D Metal Shaders**
   - Real-world 2D engine implementation
   - Vertex shader patterns for sprites
   - Buffer management strategies

3. **Gabriel's Virtual Tavern - Vulkan Sprite Batching**
   - Modern bindless approach
   - Instance data structures
   - One-draw-call pattern

---

## 🎯 Implementation Strategy

### Current State (Broken):
- **270 draw calls** per frame (one per sprite)
- **540 buffer allocations** per frame (uniform + vertex per sprite)
- **20-30 FPS**

### Target State (GPU Instancing):
- **1 draw call** per texture batch (~6-8 batches total)
- **0 per-sprite allocations**
- **60 FPS** 🎯

---

## 📐 Data Structures

### 1. Per-Instance Sprite Data (Swift)

```swift
// Pack all per-sprite data into one structure
struct SpriteInstanceData {
    var modelMatrix: simd_float4x4      // Transform (position, scale, rotation)
    var uvRect: SIMD4<Float>            // (u0, v0, u1, v1) for sprite sheets
    var color: SIMD4<Float>             // Tint color (r, g, b, a)
    
    // Alignment: 64 + 16 + 16 = 96 bytes per sprite
}
```

**Key Points:**
- `modelMatrix`: Contains position, scale, and rotation baked into one matrix
- `uvRect`: Handles sprite sheet frames (animated sprites)
- `color`: For tinting/alpha effects

### 2. Shared Frame Uniforms (Swift)

```swift
// Shared across all sprites in a batch
struct FrameUniforms {
    var projectionMatrix: simd_float4x4  // Orthographic projection
}
```

---

## 🎨 Metal Shader Code

### Shader Structs (Shaders.metal)

```metal
#include <metal_stdlib>
using namespace metal;

// Vertex input (from static vertex buffer - the quad)
struct VertexIn {
    float2 position [[attribute(0)]];   // Local quad position (0-1)
    float2 texCoord [[attribute(1)]];   // Base texture coordinates (0-1)
};

// Per-instance data (from instance buffer)
struct InstanceData {
    float4x4 modelMatrix;               // Transform for this sprite
    float4 uvRect;                      // (u0, v0, u1, v1) for sprite sheet
    float4 color;                       // Tint/alpha
};

// Frame-level uniforms
struct FrameUniforms {
    float4x4 projectionMatrix;
};

// Vertex shader output / Fragment shader input
struct VertexOut {
    float4 position [[position]];       // Clip space position
    float2 texCoord;                    // Interpolated UV
    float4 color;                       // Interpolated color
};
```

### Vertex Shader

```metal
vertex VertexOut spriteVertexInstanced(
    VertexIn in [[stage_in]],
    constant FrameUniforms& frameUniforms [[buffer(1)]],
    constant InstanceData* instances [[buffer(2)]],
    uint instanceID [[instance_id]],     // ← GPU provides this automatically!
    uint vertexID [[vertex_id]])
{
    // Get this sprite's instance data
    InstanceData instance = instances[instanceID];
    
    // Transform vertex position by instance's model matrix
    float4 worldPosition = instance.modelMatrix * float4(in.position, 0.0, 1.0);
    
    // Project to clip space
    float4 clipPosition = frameUniforms.projectionMatrix * worldPosition;
    
    // Calculate UV coordinates from sprite sheet
    // Mix between uvRect.xy (top-left) and uvRect.zw (bottom-right) based on texCoord
    float2 uv = mix(instance.uvRect.xy, instance.uvRect.zw, in.texCoord);
    
    // Output
    VertexOut out;
    out.position = clipPosition;
    out.texCoord = uv;
    out.color = instance.color;
    return out;
}
```

### Fragment Shader

```metal
fragment float4 spriteFragmentInstanced(
    VertexOut in [[stage_in]],
    texture2d<float> texture [[texture(0)]],
    sampler textureSampler [[sampler(0)]])
{
    // Sample texture
    float4 texColor = texture.sample(textureSampler, in.texCoord);
    
    // Apply tint/alpha
    return texColor * in.color;
}
```

---

## 🔧 Swift Implementation

### Step 1: Create Instance Buffer

```swift
// In MetalRenderer setup
private var spriteInstanceBuffer: MTLBuffer?
private let maxSpritesPerBatch: Int = 512

func setupInstanceBuffer() {
    let bufferSize = MemoryLayout<SpriteInstanceData>.stride * maxSpritesPerBatch
    spriteInstanceBuffer = device.makeBuffer(
        length: bufferSize,
        options: .storageModeShared  // CPU writable, GPU readable
    )
    spriteInstanceBuffer?.label = "Sprite Instance Buffer"
}
```

### Step 2: Fill Instance Data Per Frame

```swift
func drawSpriteBatch(textureHandle: UInt32, 
                    sprites: UnsafeBufferPointer<GameCore.SpriteBatchData>) {
    
    guard let texture = textures[textureHandle],
          let instanceBuffer = spriteInstanceBuffer,
          sprites.count > 0 else { return }
    
    // Get pointer to instance buffer
    let instancePointer = instanceBuffer.contents()
        .bindMemory(to: SpriteInstanceData.self, capacity: maxSpritesPerBatch)
    
    // Fill instance data for each sprite
    for (i, sprite) in sprites.enumerated() {
        var instanceData = SpriteInstanceData()
        
        // Build model matrix
        let spriteWidth = sprite.sourceWidth > 0 ? sprite.sourceWidth * sprite.scaleX : Float(texture.width) * sprite.scaleX
        let spriteHeight = sprite.sourceHeight > 0 ? sprite.sourceHeight * sprite.scaleY : Float(texture.height) * sprite.scaleY
        
        instanceData.modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: sprite.x, y: sprite.y),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: sprite.rotation
        )
        
        // Set UV rectangle for sprite sheet
        if sprite.sourceWidth > 0 && sprite.sourceHeight > 0 {
            let texWidth = Float(texture.width)
            let texHeight = Float(texture.height)
            instanceData.uvRect = SIMD4<Float>(
                sprite.sourceX / texWidth,          // u0
                sprite.sourceY / texHeight,         // v0
                (sprite.sourceX + sprite.sourceWidth) / texWidth,   // u1
                (sprite.sourceY + sprite.sourceHeight) / texHeight  // v1
            )
        } else {
            // Full texture
            instanceData.uvRect = SIMD4<Float>(0, 0, 1, 1)
        }
        
        // Color/alpha (default white/opaque)
        instanceData.color = SIMD4<Float>(1, 1, 1, 1)
        
        // Write to buffer
        instancePointer[i] = instanceData
    }
    
    // Get projection matrix
    let projectionMatrix = uniformBuffer.contents()
        .bindMemory(to: simd_float4x4.self, capacity: 1).pointee
    
    // Create frame uniforms
    var frameUniforms = FrameUniforms(projectionMatrix: projectionMatrix)
    let frameUniformsSize = MemoryLayout<FrameUniforms>.stride
    
    // Set pipeline and buffers
    renderEncoder.setRenderPipelineState(instancedPipelineState!)  // New instanced pipeline
    renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)  // Quad vertices
    renderEncoder.setVertexBytes(&frameUniforms, length: frameUniformsSize, index: 1)  // Frame uniforms
    renderEncoder.setVertexBuffer(instanceBuffer, offset: 0, index: 2)  // Instance data
    renderEncoder.setFragmentTexture(texture, index: 0)
    renderEncoder.setFragmentSamplerState(samplerState, index: 0)
    
    // ★ ONE DRAW CALL FOR ALL SPRITES! ★
    renderEncoder.drawIndexedPrimitives(
        type: .triangle,
        indexCount: 6,                  // 6 indices for quad
        indexType: .uint16,
        indexBuffer: indexBuffer!,
        indexBufferOffset: 0,
        instanceCount: sprites.count    // ← Draw N instances of the quad!
    )
}
```

### Step 3: Create Pipeline State for Instancing

```swift
func setupInstancedPipeline() {
    let library = device.makeDefaultLibrary()
    let vertexFunction = library?.makeFunction(name: "spriteVertexInstanced")
    let fragmentFunction = library?.makeFunction(name: "spriteFragmentInstanced")
    
    let pipelineDescriptor = MTLRenderPipelineDescriptor()
    pipelineDescriptor.label = "Instanced Sprite Pipeline"
    pipelineDescriptor.vertexFunction = vertexFunction
    pipelineDescriptor.fragmentFunction = fragmentFunction
    pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
    
    // Vertex descriptor for quad vertices
    let vertexDescriptor = MTLVertexDescriptor()
    // Position (x, y)
    vertexDescriptor.attributes[0].format = .float2
    vertexDescriptor.attributes[0].offset = 0
    vertexDescriptor.attributes[0].bufferIndex = 0
    // TexCoord (u, v)
    vertexDescriptor.attributes[1].format = .float2
    vertexDescriptor.attributes[1].offset = 8
    vertexDescriptor.attributes[1].bufferIndex = 0
    // Layout
    vertexDescriptor.layouts[0].stride = 16  // 2 floats (pos) + 2 floats (uv)
    vertexDescriptor.layouts[0].stepFunction = .perVertex
    
    pipelineDescriptor.vertexDescriptor = vertexDescriptor
    
    // Blending for transparency
    pipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
    
    do {
        instancedPipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
    } catch {
        fatalError("Failed to create instanced pipeline: \(error)")
    }
}
```

---

## 📊 Expected Performance Impact

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| **Draw Calls/Frame** | 270 | 6-8 | **97% reduction** |
| **Buffer Allocations/Frame** | 540 | 0 | **100% elimination** |
| **GPU Efficiency** | Low (overhead) | High (instancing) | **Massive gain** |
| **FPS (Estimated)** | 20-30 | **60** 🎯 | **2-3x faster** |

---

## 🔄 Migration Path

### Phase 1: Create New Shaders
1. ✅ Research complete
2. ⏳ Create `Shaders.metal` with instanced vertex/fragment shaders
3. ⏳ Add `SpriteInstanceData` and `FrameUniforms` structs to Swift

### Phase 2: Setup Pipeline
1. ⏳ Create instance buffer in `setupBuffers()`
2. ⏳ Create instanced pipeline state
3. ⏳ Update vertex descriptor

### Phase 3: Update Batch Rendering
1. ⏳ Modify `drawSpriteBatch()` to fill instance buffer
2. ⏳ Replace per-sprite draw calls with one instanced call
3. ⏳ Test with simple sprites first

### Phase 4: Handle Edge Cases
1. ⏳ Non-batchable sprites (rotation, pivot) - use old path temporarily
2. ⏳ Animated sprites - verify UV rect calculations
3. ⏳ Debug sprites - keep separate rendering

### Phase 5: Cleanup
1. ⏳ Remove old per-sprite buffer allocation code
2. ⏳ Profile and verify 60 FPS
3. ⏳ Document changes

---

## ⚠️ Important Considerations

### Instance ID Auto-Increment
```metal
uint instanceID [[instance_id]]  // GPU automatically sets: 0, 1, 2, ...
```
The GPU handles this - you don't manage it!

### Buffer Alignment
- `SpriteInstanceData` is 96 bytes (well-aligned for Metal)
- Projection matrix is 64 bytes (aligned)

### Sprite Sheet UVs
For animated sprites, calculate UV rect from source rectangle:
```swift
uvRect = (sourceX/texWidth, sourceY/texHeight, 
          (sourceX+sourceWidth)/texWidth, (sourceY+sourceHeight)/texHeight)
```

### Triple Buffering (Still Valid!)
You can triple-buffer the instance buffer if needed:
```swift
var instanceBuffers: [MTLBuffer] = []  // 3 buffers
var currentFrameIndex = 0
// Use instanceBuffers[currentFrameIndex] each frame
```

---

## 🎯 Success Criteria

- ✅ All sprites render correctly (position, scale, rotation)
- ✅ Sprite sheet animations work (correct UV calculations)
- ✅ Transparency/blending works
- ✅ Consistent 60 FPS on Level 1, Sewer, Castle
- ✅ No visual glitches
- ✅ Clean code, no technical debt

---

## 🚀 Ready to Implement!

With this research complete, we now have:
1. ✅ Understanding of Metal instancing patterns
2. ✅ Real-world shader examples
3. ✅ Complete implementation strategy
4. ✅ Performance targets

**Next Step:** Create the shaders and start implementation!
