# Boss Arm Positioning Fix

## Problem Summary

The Rat King boss's rotating arms (front and back) were appearing **above and to the left** of where they should be, instead of being properly attached to the boss's shoulder position during aiming and throwing animations.

## Root Cause Analysis

The issue stemmed from a **coordinate system mismatch** between different sprite rendering modes:

### The Rat King Torso (Static Sprite)
- Rendered as a **static sprite without rotation**
- Uses **top-left positioning** (standard sprite rendering)
- Position represents the **top-left corner** of the 128x128 sprite
- Scale factor: 8.0x (so 1024x1024 pixels on screen)

### The Rat King Arms (Rotating Animated Sprites)
- Rendered as **animated sprites WITH rotation** via `PivotRotationRenderer`
- Uses **center-based positioning** (for proper rotation)
- Position represents the **center point** of the 128x128 sprite
- Scale factor: 8.0x (so 1024x1024 pixels on screen)
- Pivot point: (0, 0) - rotation around sprite center

### The Mismatch

The code was calculating the shoulder position correctly:
```cpp
GNVector2 GetShoulderPosition() const {
    return {
        position.x + (60.0f * scale),  // 60 pixels offset in sprite space
        position.y + (38.0f * scale)   // 38 pixels offset in sprite space
    };
}
```

However, in `UpdateSprites()`, the arm positions were being adjusted with an incorrect offset:
```cpp
// ❌ INCORRECT - Treating centered sprite as if top-left positioned
backArmTransform->position.x = shoulder.x - (64.0f * scale);
backArmTransform->position.y = shoulder.y - (64.0f * scale);
```

This `-64 * scale` offset was attempting to compensate as if the arm sprite was rendered from its top-left corner (moving it 64 pixels = half of 128px sprite size). But since `PivotRotationRenderer` renders from the **center**, this offset was wrong.

Additionally, the shoulder calculation gives a reference point on the torso, but the actual rotation joint visible in the arm sprite is offset from that reference point by **16 pixels right** and **32 pixels down** in sprite space.

### Visual Impact

With scale = 8.0:
- Incorrect offset: -64 * 8 = **-512 pixels in both X and Y**
- This moved the arms **512 pixels left** and **512 pixels up** from where they should be
- Result: Arms appeared floating above and to the left of the boss's shoulder

## The Fix

**Apply the correct joint offset** - position arm transforms at the actual rotation joint:

```cpp
// ✅ CORRECT - Position at arm joint (16px right, 32px down from shoulder reference)
backArmTransform->position.x = shoulder.x + (16.0f * scale);
backArmTransform->position.y = shoulder.y + (32.0f * scale);
```

This accounts for:
1. The shoulder reference point from the torso sprite
2. The actual joint location in the arm sprite (16px right, 32px down in sprite space)
3. Proper scaling of the offset by the boss's scale factor (8.0x = 128px right, 256px down on screen)

## Rendering Pipeline Confirmation

### Static Sprites (Torso)
1. `RenderSystem` uses `drawSpriteScaled()` or `drawSpriteScaledWithSource()`
2. `MetalMatrixHelpers::spriteTransformMatrix()` - top-left positioning
3. Metal shader renders quad with position as **top-left corner**

### Pivoted Rotating Sprites (Arms)
1. `RenderSystem` detects `PivotRotationRenderer` component
2. Uses `drawSpriteScaledWithSourcePivoted()` for animated sprites
3. `MetalMatrixHelpers::spriteTransformMatrixPivoted()` with pivot (0,0)
4. Transformation order:
   - Center unit quad at origin (-0.5, -0.5 translation)
   - Apply pivot offset (0, 0 in this case)
   - Rotate around pivot point
   - Scale to final size
   - Translate to final position
5. Metal shader renders with **center at transform position**

## Files Modified

- `src/FloppyTurd/Systems/BossSystem.cpp`
  - `CreateBodyPartEntities()` - Updated initial arm positions with joint offset (lines ~870, ~905)
  - `SetArmSpriteVisibility()` - Updated arm positions with joint offset when showing arms (lines ~1188, ~1226)
  - `UpdateSprites()` - Applied joint offset in AIMING state (lines ~1338, ~1362)
  - `UpdateSprites()` - Applied joint offset in THROWING state (lines ~1395, ~1422)
  - `UpdateSprites()` - Applied joint offset in default state (lines ~1459, ~1473)

## Testing Verification

After the fix, verify:
1. ✅ Boss arms are visually attached at the shoulder during AIMING
2. ✅ Arms rotate smoothly while tracking the player
3. ✅ Arms animate correctly during THROWING (7-frame toss animation)
4. ✅ Both back arm (layer 7) and front arm (layer 9) render in correct order
5. ✅ Projectiles spawn from the correct hand position

## Key Takeaway

**When using `PivotRotationRenderer`:**
- Position the entity transform at the **intended rotation center point**
- Do NOT apply offsets to compensate for sprite dimensions
- The renderer automatically handles centering the sprite at the transform position
- Pivot offsets are relative to sprite center in sprite-space pixels

**Pattern to follow (like spike balls):**
```cpp
// Position entity at the base center (where chain connects)
transform.position = baseCenter;

// Pivot offset in sprite pixels (e.g., -45 pixels up for chain connection)
PivotRotationRenderer pivot(true, 0.0f, -45.0f, rotationSpeed, false);
```

For the boss arms, we want rotation around the arm joint:
```cpp
// Position entity at arm joint (offset from shoulder reference)
GNVector2 shoulder = GetShoulderPosition();
transform.position = GNVector2(
    shoulder.x + (16.0f * scale),  // 16px right in sprite space
    shoulder.y + (32.0f * scale)   // 32px down in sprite space
);

// No pivot offset within sprite - rotate around sprite center which is now at the joint
PivotRotationRenderer pivot(true, 0.0f, 0.0f, 0.0f, true); // manual control
```
