# Boss Arms Not Rotating - Root Cause Analysis

## Critical New Information
The arms are **NOT rotating at all** - they're completely static. This is not a pivot offset issue, it's a fundamental rendering or visibility problem.

## Hypothesis: Arms May Not Be Rendering

### Possible Causes

1. **Arms might not be visible at all**
   - Layer 7 (back arm) and Layer 9 (front arm) might not be rendered
   - Sprites might not have valid texture IDs
   - Entities might not be in the render queue

2. **Pivot Rotation Not Being Applied by Renderer**
   - RenderSystem checks for `PivotRotationRenderer` component
   - If component exists, calls `drawSpriteScaledPivoted`
   - If renderer doesn't support this function, falls back to centered or basic rendering
   - **Critical**: Need to verify `drawSpriteScaledPivoted` exists in MetalRenderer

3. **Transform Rotation Not Being Set**
   - `UpdateArmRotations()` is called every frame in AIMING state
   - Sets `transform->rotation = aimingData.currentArmAngle`
   - But if renderer ignores rotation without pivot component, nothing happens

## Investigation Steps

### Step 1: Check if Arms Are Being Rendered At All
Look for Layer 7 and Layer 9 in the render batch logs:
```
Layer 7: should have back arm
Layer 9: should have front arm
```

From the logs provided, we see:
- Layer 6: TurdletIdle (player)
- Layer 8: RatkingAimTorsoOnly (boss torso)
- **NO Layer 7 or 9 visible!**

**CONCLUSION**: Arms are NOT being rendered at all!

### Step 2: Why Aren't Arms Rendering?

Possible reasons:
1. **Sprites not visible**: Check if `visible = true` when arms should show
2. **Invalid texture IDs**: Arms might have empty or wrong texture names
3. **Entities not in ECS**: Arms might not be properly created
4. **Layer filtering**: Renderer might skip certain layers

### Step 3: Check Arm Entity Creation

From code analysis:
- `CreateBodyPartEntities()` creates `backArmEntity` and `frontArmEntity`
- Adds Transform, Sprite, and PivotRotationRenderer components
- Initial sprites have `visible = false`

In `SetArmSpriteVisibility(true, true)`:
- Copies sprite properties from `sprites.backArmSprite` and `sprites.frontArmSprite`
- Sets `visible = true`
- Updates transform position to shoulder

**Critical Check**: Are `sprites.backArmSprite` and `sprites.frontArmSprite` loaded correctly in `LoadSprites()`?

### Step 4: Texture ID Verification

From `LoadSprites()`:
```cpp
sprites.backArmSprite = new Sprite("RatkingAimBackArmOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
sprites.frontArmSprite = new Sprite("RatkingAimTossArmOnly", 128.0f, 128.0f, 128, 128, 7, 0.16f);
```

These texture IDs need to match actual asset files. If files don't exist or names don't match, sprites won't render.

## Root Cause Suspect: Arms Not Rendering Because...

### Most Likely: Invalid Texture IDs or Assets Not Found
The arm sprites might be trying to load textures that don't exist in the assets. When a texture fails to load, the sprite becomes invisible.

**Evidence**:
- Layer 7 and 9 completely missing from render batches
- Logs show Layer 6 (player) and Layer 8 (torso) but skip 7 and 9
- This suggests entities exist but aren't being rendered

### Secondary Suspect: Visibility Not Set Properly
Even though `SetArmSpriteVisibility` sets `visible = true`, something might be overriding it or the copy operation might be failing.

## Next Steps to Debug

1. **Check Asset Files**: Verify these files exist:
   - `RatkingAimBackArmOnly.png`
   - `RatkingAimTossArmOnly.png`

2. **Add Debug Logging**: In next build, check:
   - Are arm entities created? (check entity IDs)
   - Are textures loaded? (check texture handles)
   - Are sprites visible? (check visible flag)
   - What layers are in render queue?

3. **Verify Render Path**: Check if `drawSpriteScaledPivoted` exists in MetalRenderer and is being called for pivot rotation entities.

## Quick Fix to Try

If assets don't exist or have wrong names, the simplest fix is to use the full Ratking sprite for arms temporarily while we debug, or verify the correct asset names from the asset folder.

##Confirmed: Arms Are NOT Visible in Render Queue

The logs show no Layer 7 or 9, confirming arms aren't being rendered. The rotation code is irrelevant if sprites aren't visible.

**Priority**: Fix arm visibility FIRST, then worry about rotation.

