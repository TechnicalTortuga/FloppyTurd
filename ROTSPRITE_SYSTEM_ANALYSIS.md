# RotSprite System Comprehensive Analysis & Redesign

## Executive Summary

Our current RotSprite implementation for the spikeball has multiple fundamental issues:
1. **Incorrect coordinate system handling** between CPU positioning, GPU rendering, and collision detection
2. **Broken pivot rotation logic** causing visual clipping and positioning errors
3. **Inconsistent texture bounds calculation** during rotation
4. **Misaligned hitbox and visual representation**

This document provides a complete analysis and redesign based on research of authoritative sources and best practices.

---

## 1. RESEARCH FINDINGS

### 1.1 RotSprite Algorithm Overview
RotSprite, developed by Xenowhirl, is designed to rotate pixel art without introducing anti-aliasing artifacts that distort the original color palette. The algorithm:

1. **Upscales** the original sprite using Scale2x/hq2x (typically 8x)
2. **Rotates** the upscaled version using traditional bilinear/nearest-neighbor sampling
3. **Downscales** back to target size
4. **Optionally applies detail restoration** to preserve single-pixel features

### 1.2 Pivot Rotation Mathematics
Standard pivot rotation requires three transformations in sequence:

```
T_final = T_back × R(θ) × T_origin

Where:
- T_origin: Translate to origin (subtract pivot)
- R(θ): Rotation matrix
- T_back: Translate back (add pivot)
```

### 1.3 Metal Shader Coordinate Systems
Metal uses:
- **Texture coordinates**: (0,0) at top-left, (1,1) at bottom-right
- **NDC coordinates**: (-1,-1) to (1,1) with (0,0) at center
- **Pixel coordinates**: (0,0) at top-left, (width-1, height-1) at bottom-right

---

## 2. CURRENT SYSTEM PROBLEMS ANALYSIS

### 2.1 Coordinate System Mismatches

**Problem**: Multiple coordinate system conversions are happening inconsistently:

1. **ObstacleSystem**: Sets spikeball position in world coordinates
2. **RenderSystem**: Converts to screen coordinates and normalizes pivot
3. **MetalRenderer**: Converts again to texture coordinates
4. **RotSprite shader**: Expects specific coordinate ranges

**Root Cause**: Each stage makes assumptions about the input coordinate system without proper validation.

### 2.2 Broken Pivot Logic

**Current Implementation Issues**:
```cpp
// ObstacleSystem.cpp - WRONG: Moving sprite position instead of just pivot
float spikeBallY = baseY + (45.0f * m_baseScale);

// RenderSystem.cpp - WRONG: Double normalization
float normalizedPivotY = pivotRenderer->pivotY / item.sprite->height;

// MetalRenderer.swift - WRONG: Incorrect coordinate conversion
let normalizedPivotY = (Float(texture.height) * 0.5 + pivotY) / Float(texture.height)
```

### 2.3 Texture Bounds Issues

**Problem**: Creating padded textures without proper coordinate mapping
```metal
// RotSprite.metal - WRONG: Inconsistent coordinate scaling
float2 pivotInOutput = pivotInOriginal * (outputSizeF / originalSize);
```

---

## 3. PROPER SYSTEM DESIGN

### 3.1 Coordinate System Standardization

**World Space** → **Screen Space** → **Texture Space** → **Shader Space**

```
Spikeball Entity:
- Position: World coordinates (e.g., 1000, 1278)
- Pivot: Relative to sprite center in pixels (e.g., 0, -45)

Rendering Pipeline:
1. Convert world position to screen position
2. Keep pivot in sprite-relative pixels
3. Convert to normalized texture coordinates only in final shader
4. Apply rotation in shader space
```

### 3.2 Correct Pivot Implementation

**Entity Definition**:
```cpp
// ObstacleSystem.cpp
Transform spikeBallTransform;
spikeBallTransform.position = {baseCenterX, baseCenterY};  // World position at base center

PivotRotationRenderer pivotRenderer;
pivotRenderer.pivotX = 0.0f;    // Sprite center X
pivotRenderer.pivotY = -45.0f;  // 45 pixels above sprite center (chain connection)
```

**Rendering Conversion**:
```cpp
// RenderSystem.cpp - NO normalization here, pass raw pixel values
float pivotX = pivotRenderer->pivotX;  // Keep in pixels
float pivotY = pivotRenderer->pivotY;  // Keep in pixels
```

**Metal Shader Conversion**:
```swift
// MetalRenderer.swift - Convert to normalized coordinates for shader
let normalizedPivotX = (pivotX + Float(texture.width) * 0.5) / Float(texture.width)
let normalizedPivotY = (pivotY + Float(texture.height) * 0.5) / Float(texture.height)
```

### 3.3 Proper RotSprite Bounds Calculation

**Padding Calculation**:
```swift
// Calculate exact bounds needed for any rotation around the pivot
let pivotOffsetX = pivotX  // Relative to sprite center
let pivotOffsetY = pivotY
let spriteWidth = Float(texture.width)
let spriteHeight = Float(texture.height)

// Calculate maximum distance from pivot to any corner
let corners = [
    (-spriteWidth/2 - pivotOffsetX, -spriteHeight/2 - pivotOffsetY),
    ( spriteWidth/2 - pivotOffsetX, -spriteHeight/2 - pivotOffsetY),
    (-spriteWidth/2 - pivotOffsetX,  spriteHeight/2 - pivotOffsetY),
    ( spriteWidth/2 - pivotOffsetX,  spriteHeight/2 - pivotOffsetY)
]

let maxDistance = corners.map { sqrt($0.0*$0.0 + $0.1*$0.1) }.max()!
let paddedSize = UInt32(ceil(maxDistance * 2))
```

### 3.4 Correct Shader Implementation

**RotSprite Shader**:
```metal
kernel void rotsprite_rotate_and_downscale(
    texture2d<float, access::read> upscaledTexture [[texture(0)]],
    texture2d<float, access::write> outputTexture [[texture(1)]],
    constant RotSpriteParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 outputSize = uint2(outputTexture.get_width(), outputTexture.get_height());
    uint2 upscaledSize = uint2(upscaledTexture.get_width(), upscaledTexture.get_height());
    
    // Output pixel position (center of pixel)
    float2 outputPixel = float2(gid) + 0.5;
    
    // Convert pivot from normalized to pixel coordinates in both spaces
    float2 originalSize = float2(params.originalSize);
    float2 outputSizeF = float2(outputSize);
    float2 upscaledSizeF = float2(upscaledSize);
    
    // Pivot in original space (0.5, 0.5) = center
    float2 pivotInOriginal = (params.rotationCenter * originalSize);
    
    // Scale pivot to output space (maintaining center relationship)
    float2 outputCenter = outputSizeF * 0.5;
    float2 originalCenter = originalSize * 0.5;
    float2 pivotOffsetFromOriginalCenter = pivotInOriginal - originalCenter;
    float2 pivotInOutput = outputCenter + pivotOffsetFromOriginalCenter;
    
    // Scale pivot to upscaled space
    float2 upscaledCenter = upscaledSizeF * 0.5;
    float2 pivotInUpscaled = upscaledCenter + (pivotOffsetFromOriginalCenter * ROTSPRITE_SCALE_FACTOR);
    
    // Standard pivot rotation: translate → rotate → translate back
    float2 translatedPixel = outputPixel - pivotInOutput;
    float2x2 invRotMatrix = rotationMatrix(-params.rotationAngle);
    float2 rotatedPixel = invRotMatrix * translatedPixel;
    float2 sourceCoord = rotatedPixel + pivotInUpscaled;
    
    // Sample with bounds checking
    float4 outputColor = float4(0.0);
    if (sourceCoord.x >= 0.0 && sourceCoord.x < upscaledSizeF.x &&
        sourceCoord.y >= 0.0 && sourceCoord.y < upscaledSizeF.y) {
        outputColor = upscaledTexture.sample(textureSampler, sourceCoord / upscaledSizeF);
    }
    
    outputTexture.write(outputColor, gid);
}
```

---

## 4. IMPLEMENTATION PLAN

### Phase 1: Fix Coordinate System (High Priority)
1. Remove all coordinate conversions except in final shader stage
2. Keep pivot values in sprite-relative pixels throughout pipeline
3. Fix spikeball positioning to maintain collision alignment

### Phase 2: Implement Proper Bounds Calculation (High Priority)
1. Calculate exact padding needed based on pivot and sprite dimensions
2. Ensure output texture accommodates full rotation range
3. Fix coordinate mapping between different texture sizes

### Phase 3: Validate Shader Implementation (Medium Priority)
1. Verify rotation matrix calculations
2. Test pivot transformations with known values
3. Add debug visualization for pivot points

### Phase 4: Optimize and Polish (Low Priority)
1. Implement detail restoration pass
2. Add caching for computed bounds
3. Performance optimization

---

## 5. TESTING STRATEGY

### 5.1 Unit Tests
- Test coordinate conversions with known values
- Verify rotation matrix calculations
- Validate bounds calculations for various pivot positions

### 5.2 Visual Validation
- Add debug visualization showing:
  - Actual pivot point
  - Rotation bounds
  - Texture borders
- Compare with simple matrix rotation for accuracy

### 5.3 Integration Tests
- Verify collision detection matches visual rotation
- Test edge cases (pivot at corners, large rotations)
- Performance benchmarking

---

## 6. CONCLUSION

The current system suffers from fundamental design issues rather than implementation bugs. A complete redesign focusing on coordinate system consistency and proper mathematical foundations is required. The proposed solution maintains simplicity while ensuring correctness and provides a solid foundation for future enhancements.

**Key Success Metrics**:
- ✅ Spikeball rotates around base center (chain connection)
- ✅ No visual clipping during rotation
- ✅ Collision detection matches visual representation
- ✅ Pixel-perfect quality maintained
- ✅ Performance remains acceptable

**Estimated Implementation Time**: 2-3 days for complete redesign and testing.
