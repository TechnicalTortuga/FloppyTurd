# Clipped Sprite Rendering Implementation Plan

## Overview
Implement a new rendering method that allows sprites to be rendered at a **fixed destination size** while clipping the source texture via UV coordinates. This solves the boss health bar issue where changing `sourceWidth` causes the rendered sprite size to change, making it appear to move.

## Problem Statement

### Current Behavior
Our Metal renderer calculates sprite size as:
```swift
spriteWidth = sprite.sourceWidth * absScaleX
spriteHeight = sprite.sourceHeight * absScaleY
```

This means:
- When `sourceWidth` changes, the rendered width changes
- Position appears to shift even though transform position is locked
- Impossible to "clip" a sprite without changing its visual size

### Desired Behavior
- Render sprite at a **fixed destination rectangle** (constant size)
- Change **source UV rectangle** to clip the texture
- Sprite stays in exact same position/size, only texture content changes
- Perfect for health bars, progress bars, loading bars, etc.

## Technical Research

### UV Coordinate Basics (from OpenGL/Metal standards)
- UV coordinates range from `(0, 0)` to `(1, 1)` across the entire texture
- `(0, 0)` = bottom-left corner (OpenGL convention) or top-left (Metal convention)
- `(1, 1)` = top-right corner (OpenGL) or bottom-right (Metal)
- Metal uses **normalized coordinates** where texture dimensions don't matter

### Clipping Formula
To render only the left portion of a texture (e.g., 75% health):
```
// Full texture UV rect
u0 = 0.0, v0 = 0.0
u1 = 1.0, v1 = 1.0

// 75% health (clip from right side)
u0 = 0.0, v0 = 0.0
u1 = 0.75, v1 = 1.0  // Only sample left 75% of texture

// Destination quad stays at full size (e.g., 1280 pixels wide)
// Texture is stretched/repeated to fill the quad
```

### Key Insight
The destination quad size is **independent** of the UV rectangle. We can:
1. Create a quad of fixed size (e.g., 1280x32 pixels)
2. Change UV coordinates to sample different portions of the texture
3. Texture sampling will stretch/compress to fit the destination quad

## Implementation Plan

### Phase 1: Add New Component Flag
**File:** `src/FloppyTurd/Components/GameComponents.h`

Add new field to `Sprite` struct:
```cpp
struct Sprite {
    // ... existing fields ...
    
    // NEW: Use fixed-destination rendering (for clipping effects)
    bool useFixedDestination = false;
    float fixedWidth = 0.0f;   // If > 0, use this as destination width
    float fixedHeight = 0.0f;  // If > 0, use this as destination height
    
    // sourceX, sourceY, sourceWidth, sourceHeight still used for UV clipping
};
```

**Rationale:** Non-breaking change. Existing sprites with `useFixedDestination = false` render normally.

### Phase 2: Update RenderSystem
**File:** `src/FloppyTurd/Systems/RenderSystem.cpp`

In `RenderSpriteBatch()`, check for fixed-destination flag:
```cpp
if (item->sprite->useFixedDestination && 
    item->sprite->fixedWidth > 0.0f && 
    item->sprite->fixedHeight > 0.0f) {
    
    // Use fixed destination size (already in pixels, apply camera scale only)
    float cameraScale = GetCameraScale();
    data.scaleX = (item->sprite->fixedWidth * cameraScale) / static_cast<float>(item->sprite->frameWidth);
    data.scaleY = (item->sprite->fixedHeight * cameraScale) / static_cast<float>(item->sprite->frameHeight);
    
    // Mark this sprite for fixed-destination rendering
    data.useFixedDestination = true;
    
} else {
    // Existing behavior - scale based on sprite width/height
    float cameraScale = GetCameraScale();
    float scaleX = item->sprite->width / static_cast<float>(item->sprite->frameWidth);
    float scaleY = item->sprite->height / static_cast<float>(item->sprite->frameHeight);
    data.scaleX = scaleX * item->transform->scale.x * cameraScale;
    data.scaleY = scaleY * item->transform->scale.y * cameraScale;
    
    data.useFixedDestination = false;
}
```

### Phase 3: Update SpriteBatchData Structure
**File:** `src/Engine/Platform/PlatformDelegates.h`

Add flag to batch data:
```cpp
struct SpriteBatchData {
    uint32_t textureHandle;
    float x, y;
    float scaleX, scaleY;
    float rotation;
    float sourceX, sourceY;
    float sourceWidth, sourceHeight;
    float pivotX, pivotY;
    
    // NEW: Fixed-destination rendering flag
    bool useFixedDestination = false;
};
```

### Phase 4: Update MetalRenderer
**File:** `src/iOS/Rendering/MetalRenderer.swift`

In `drawSpriteBatch()`, check the flag:
```swift
for sprite in sprites {
    // ... existing code ...
    
    let spriteWidth: Float
    let spriteHeight: Float
    
    if sprite.useFixedDestination {
        // FIXED DESTINATION: Use scale values directly as pixel dimensions
        // The scale already represents the desired destination size
        spriteWidth = sprite.scaleX * Float(sprite.sourceWidth)
        spriteHeight = sprite.scaleY * Float(sprite.sourceHeight)
    } else if sprite.sourceWidth > 0 && sprite.sourceHeight > 0 {
        // NORMAL: Animated/sprite sheet rendering
        spriteWidth = sprite.sourceWidth * absScaleX
        spriteHeight = sprite.sourceHeight * absScaleY
    } else {
        // NORMAL: Static sprite rendering
        spriteWidth = Float(texture.width) * absScaleX
        spriteHeight = Float(texture.height) * absScaleY
    }
    
    // UV coordinates are calculated the same way regardless of destination size
    // This is the key: UV rect is independent of destination quad size
    if sprite.sourceWidth > 0 && sprite.sourceHeight > 0 {
        let texWidth = Float(texture.width)
        let texHeight = Float(texture.height)
        
        let halfPixelU = 0.5 / texWidth
        let halfPixelV = 0.5 / texHeight
        
        var u0 = max(0.0, min(1.0, (sprite.sourceX / texWidth) + halfPixelU))
        var v0 = max(0.0, min(1.0, (sprite.sourceY / texHeight) + halfPixelV))
        var u1 = max(0.0, min(1.0, ((sprite.sourceX + sprite.sourceWidth) / texWidth) - halfPixelU))
        var v1 = max(0.0, min(1.0, ((sprite.sourceY + sprite.sourceHeight) / texHeight) - halfPixelV))
        
        uvRect = SIMD4<Float>(u0, v0, u1, v1)
    }
    
    // Model matrix uses spriteWidth/Height calculated above
    // Destination quad size is NOW independent of source UV rect size
}
```

### Phase 5: Update BossHealthBar to Use New System
**File:** `src/FloppyTurd/Systems/BossHealthBar.cpp`

In `CreateUIEntities()`:
```cpp
// Create health sprite with FIXED destination
Sprite healthSprite("BossBarHealth", ORIGINAL_WIDTH * scale, ORIGINAL_HEIGHT * scale);
healthSprite.visible = true;
healthSprite.isAnimated = false;
healthSprite.frameCount = 1;
healthSprite.frameWidth = ORIGINAL_WIDTH;
healthSprite.frameHeight = ORIGINAL_HEIGHT;
healthSprite.layer = 13;

// NEW: Enable fixed-destination rendering
healthSprite.useFixedDestination = true;
healthSprite.fixedWidth = ORIGINAL_WIDTH * scale;   // Always 1280 pixels
healthSprite.fixedHeight = ORIGINAL_HEIGHT * scale; // Always 256 pixels

// Source rect will be changed each frame for clipping
healthSprite.sourceX = 0;
healthSprite.sourceY = 0;
healthSprite.sourceWidth = ORIGINAL_WIDTH;  // Full width initially
healthSprite.sourceHeight = ORIGINAL_HEIGHT;
```

In `UpdateUIEntities()`:
```cpp
// Update health sprite - ONLY change sourceWidth for clipping
if (m_healthFillEntity != 0) {
    Sprite* health = m_ecsSystem->GetComponent<Sprite>(m_healthFillEntity);
    if (health) {
        health->visible = shouldBeVisible && (m_displayedHealthPercent > 0.0f);
        
        // FIXED: Destination size never changes (stays at fixedWidth x fixedHeight)
        // Only UV coordinates change via sourceWidth
        health->sourceX = 0;
        health->sourceY = 0;
        health->sourceWidth = ORIGINAL_WIDTH * m_displayedHealthPercent;  // Clip texture
        health->sourceHeight = ORIGINAL_HEIGHT;
        
        // Position stays locked (no adjustment needed!)
        // The sprite renders at the exact same screen position/size always
    }
}
```

## Benefits of This Approach

### 1. Clean Architecture
- ✅ Non-breaking change (opt-in via flag)
- ✅ Existing sprites continue to work normally
- ✅ No changes to shader code
- ✅ Works within existing batch rendering system

### 2. Performance
- ✅ No additional draw calls
- ✅ Still uses efficient batch rendering
- ✅ Same vertex/fragment shader pipeline
- ✅ No CPU overhead (simple flag check)

### 3. Flexibility
- ✅ Perfect for health bars, progress bars, loading bars
- ✅ Can clip from left, right, top, or bottom via sourceX/Y/Width/Height
- ✅ Supports rotation and all existing sprite features
- ✅ Easy to understand and maintain

### 4. Correctness
- ✅ Sprite position truly never moves
- ✅ No need for position adjustment hacks
- ✅ Destination size is explicit and constant
- ✅ UV clipping is mathematically correct

## Testing Strategy

### Test 1: Boss Health Bar
- Create boss health bar with fixed destination
- Damage boss repeatedly
- Verify red bar stays in exact same position
- Verify red bar clips smoothly from right side
- Verify white hurt bar also stays position-locked

### Test 2: Other UI Elements
- Test with pause menu elements
- Test with animated sprites (should ignore flag)
- Test with regular gameplay sprites (should work normally)

### Test 3: Edge Cases
- Test with sourceWidth = 0 (full texture)
- Test with sourceWidth = texture width (full texture)
- Test with rotation applied
- Test with very small clip percentages (1-5%)
- Test with camera scaling

### Test 4: Performance
- Measure frame time before/after change
- Verify batch rendering still efficient
- Check for any GPU bottlenecks
- Profile draw call count (should be same)

## Rollout Plan

### Step 1: Implement Component Changes
- Add `useFixedDestination`, `fixedWidth`, `fixedHeight` to Sprite struct
- Verify no compilation errors
- Default values ensure backward compatibility

### Step 2: Implement RenderSystem Changes
- Update `RenderSpriteBatch()` to handle flag
- Add `useFixedDestination` to SpriteBatchData
- Test with existing sprites (should work normally)

### Step 3: Implement MetalRenderer Changes
- Update batch rendering to check flag
- Calculate spriteWidth differently for fixed-destination sprites
- Test rendering with flag enabled

### Step 4: Update BossHealthBar
- Enable fixed-destination mode
- Set fixedWidth/Height to full bar size
- Remove position adjustment code
- Test boss level thoroughly

### Step 5: Adjust Wave Amplitudes & Pickup Timing
- While we're here, also fix:
  - Boss coin wave amplitude: 100 → 200
  - Pickup spawn timer: increase delay between groups
  - Pickup scroll speed: reduce to slow down coin acquisition

### Step 6: Build & Deploy
- Clean build from scratch
- Test on simulator
- Test on device (if available)
- Deploy Monday with confidence

## Alternative Approaches Considered

### Alternative 1: Position Adjustment (Quick Hack)
```cpp
// Adjust X position to compensate for width change
float widthDiff = (1.0f - m_displayedHealthPercent) * (ORIGINAL_WIDTH * scale) * 0.5f;
healthTransform->position.x = m_barX - widthDiff;
```
**Rejected because:** Fragile, requires constant recalculation, error-prone

### Alternative 2: New Dedicated Rendering Function
```cpp
drawSpriteClipped(handle, destRect, sourceRect);
```
**Rejected because:** More invasive, duplicates existing batch system, harder to maintain

### Alternative 3: Shader-Based Clipping
Use discard in fragment shader based on UV coordinates.
**Rejected because:** Requires new shader, breaks batching, GPU overhead

## Success Criteria

✅ Boss health bars stay in exact same position as health changes
✅ Both red and white bars render correctly with clipping
✅ No visual "jumping" or position shifting
✅ Performance remains the same (no frame drops)
✅ All existing sprites continue to work normally
✅ Code is clean, maintainable, and well-documented

## Files to Modify

1. ✅ `src/FloppyTurd/Components/GameComponents.h` - Add sprite flags
2. ✅ `src/Engine/Platform/PlatformDelegates.h` - Update batch data
3. ✅ `src/FloppyTurd/Systems/RenderSystem.cpp` - Handle fixed-destination logic
4. ✅ `src/iOS/Rendering/MetalRenderer.swift` - Implement Metal rendering
5. ✅ `src/FloppyTurd/Systems/BossHealthBar.cpp` - Use new rendering mode
6. ✅ `src/FloppyTurd/Systems/BossHealthBar.h` - Update if needed
7. ✅ `src/FloppyTurd/Systems/PickupSystem.cpp` - Adjust wave/timing (while we're here)

## Estimated Implementation Time
- Component changes: 10 minutes
- RenderSystem changes: 20 minutes
- MetalRenderer changes: 30 minutes
- BossHealthBar updates: 15 minutes
- Pickup tuning: 10 minutes
- Testing & debugging: 30 minutes
- **Total: ~2 hours**

## Post-Implementation Enhancements
Consider for future versions:
- Add clipping direction flag (left/right/top/bottom)
- Add animated clipping support (smooth transitions)
- Add rounded corners for clipped sprites
- Add border rendering for clipped regions

---

**Ready to implement! Let's make boss health bars pixel-perfect! 🎯**