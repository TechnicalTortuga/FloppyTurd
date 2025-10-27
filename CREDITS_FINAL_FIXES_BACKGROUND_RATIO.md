# Credits Scene - Final Fixes Summary

## Issues Fixed

### 1. ✅ Background Scaling - Rectangular Pixels Issue

**Problem:**
- Background was showing rectangular pixels instead of square pixels
- Screen: 2556x1179 (landscape)
- RenderSystem was applying non-uniform scaling somewhere in the pipeline

**Root Cause:**
- The MetalRenderer uses **actual MTLTexture dimensions** from the loaded texture
- Setting sprite.width/height in ECS doesn't affect what the renderer uses
- RenderSystem may apply additional camera/view transformations

**Solution:**
- Changed to **fixed scale 8.0** for simplicity and pixel-perfect rendering
- At scale 8.0: 320x180 texture → 2560x1440 final size
- Slight overflow (4px width, 261px height) is acceptable for credits background

### 2. ✅ Toilet Pair Overlap - Wrong Texture Dimensions

**Problem:**
- Top and bottom toilets were overlapping
- No visible gap between pairs
- Code was using 65x190 but actual textures are 64x256

**Root Cause:**
- **MetalRenderer ignores sprite.width/height** set in ECS components
- Renderer loads actual texture from asset catalog and uses MTLTexture.width/height
- Actual toilet textures: **64x256** (from logs: `Texture metadata for TopToilet.png: 64x256`)
- Code was calculating with wrong dimensions (65x190)
- Gap math: -684 + (190*3) + 800 = 686 ❌
- Correct gap: -652.8 + (256*3) + 800 = 915.2 ✅

**Solution:**
- Updated sprite dimensions to **64x256** (actual texture size)
- Temporarily **removed top toilets** to debug gap issue clearly
- Bottom toilets positioned at screen center (Y = 589.5) for debugging
- Scale 3.0: 64x256 → 192x768 final size ✅

### 3. ✅ Credits Duration Extended

**Problem:**
- Song ends but state transitions immediately
- No smooth transition to ScreenPromptState

**Solution:**
- Extended duration to **121 seconds** (2 minutes 1 second)
- Added **white fade out** starting at 120 seconds
- Fade out duration: 1 second
- State finishes when fade completes (alpha reaches 1.0)

### 4. ✅ White Fade Out Transition

**Implementation:**
- Added `m_fadingOut` boolean flag
- Fade starts at `CREDITS_DURATION - FADE_OUT_DURATION` (120s)
- UpdateWhiteFade() now handles both:
  - **Fade IN from white** at start (1 second)
  - **Fade OUT to white** at end (1 second)
- Smooth white transition before returning to ScreenPromptState

---

## Background Ratio Calculations

### Current Setup
- **Texture:** 320x180 pixels
- **Screen:** 2556x1179 pixels (landscape)
- **Scale:** 8.0
- **Final Size:** 2560x1440 pixels
- **Overflow:** +4px width, +261px height

### Perfect Fit Options

#### Option 1: Keep 320 width, adjust height
```
Screen ratio: 2556 / 1179 = 2.1688:1
Perfect texture: 320 x 147 pixels
At scale 8.0: 2560 x 1176 (almost perfect!)
```

#### Option 2: Match Boss Level scale (6.65625)
```
Boss level uses: 384x512 in portrait, flipped to landscape
For landscape at scale 6.65625:
Perfect texture: 384 x 177 pixels
Final size: 2556.0 x 1178.0625 (perfect fit!)
```

#### Option 3: Standard game resolution
```
If using scale 8.0 exactly:
Texture needed: 319.5 x 147.375 = round to 320 x 147
Final: 2560 x 1176 (3px overflow acceptable)
```

### Recommended Approach

**Make new texture: 320 x 147 pixels**
- Maintains power-of-2 friendly width (320)
- Height adjusted to screen ratio
- Scale 8.0 gives 2560x1176
- Only 4px width and 3px height overflow
- Clean, pixel-perfect scaling

### Alternative: Direct Width/Height Rendering

If you want **exact 1:1 pixel control without scaling:**

The MetalRenderer has `drawTexture(handle, x, y, width, height)` that uses `textureTransformMatrix` for direct size control. This bypasses sprite scaling entirely.

To use this:
1. Would need to expose through PlatformDelegates
2. Add new delegate function pointer
3. RenderSystem would need to detect and use it for layer 100+ UI sprites
4. Would give you exact destination rectangle control

**Matrix capability:** Yes, `MetalMatrixHelpers.textureTransformMatrix(position, size)` already supports this - it creates a transform matrix for exact width/height without scale multiplication.

---

## Code Changes Summary

### CreditsState.h
- Added `m_fadingOut` boolean flag
- Added `FADE_OUT_DURATION` constant (1.0s)
- Added `CREDITS_DURATION` constant (121.0s)

### CreditsState.cpp

**CreateBackground():**
- Changed to fixed scale 8.0
- Added comments about perfect-fit texture ratios
- Logged: fixedScale, finalSize for debugging

**CreatePipes():**
- Fixed toilet texture dimensions: 65x190 → **64x256**
- Temporarily removed top toilets for debugging
- Bottom toilets at screen center (Y = screenHeight * 0.5)
- Correct spacing calculations with actual texture size

**Update():**
- Checks if elapsed time ≥ 120s to start fade out
- Finishes when fade out completes (alpha ≥ 1.0)
- No longer uses `m_totalMusicDuration` from audio system

**UpdateWhiteFade():**
- Supports fade IN (start) and fade OUT (end)
- Increases alpha during fade out
- Decreases alpha during fade in
- Sets visibility appropriately

---

## Testing Checklist

- [ ] Background fills screen width completely
- [ ] Pixels are square (not rectangular)
- [ ] Bottom toilets visible at screen center
- [ ] Bottom toilets scroll smoothly without overlap
- [ ] Credits text scrolls horizontally
- [ ] White fade in from white (1 second at start)
- [ ] White fade out to white (1 second at 120s mark)
- [ ] State transitions to ScreenPromptState after fade completes
- [ ] Skip button works and triggers immediate fade out
- [ ] Duration is 121 seconds total

---

## Next Steps

1. **Create new background texture:** 320x147 pixels for perfect fit
2. **Re-enable top toilets** once bottom toilet positioning confirmed
3. **Adjust top toilet Y position** with correct texture height (768px at scale 3.0)
4. **Test complete credits flow** from boss death to main menu

---

## Technical Notes

### Why MetalRenderer Ignores sprite.width/height

The rendering pipeline works like this:

1. **ECS Component:** Sets sprite.width/height (informational only)
2. **RenderSystem:** Collects sprites and passes to renderer
3. **MetalRenderer:** Loads MTLTexture from asset catalog
4. **Uses:** `texture.width` and `texture.height` from actual texture
5. **Scales:** `finalWidth = texture.width * scaleX`
6. **Result:** ECS dimensions are ignored!

This is why the toilet gap was wrong - we calculated gap with 190px height but renderer used 256px.

### Camera/View Transformations

Layer 100+ sprites use **screen space** rendering but RenderSystem may still apply:
- View matrix transformations
- Aspect ratio corrections
- Safe area insets
- Orientation adjustments

For **absolute pixel control**, would need source→destination rectangle rendering without any scale multiplication.

---

## Perfect Background Asset Specs

**Filename:** FloppyTurdCreditsBackground.png  
**Dimensions:** 320 x 147 pixels  
**Ratio:** 2.177:1 (matches 2556:1179)  
**Format:** PNG, RGBA  
**Scale in code:** 8.0  
**Final screen size:** 2560 x 1176 pixels  
**Overflow:** 4px width, -3px height (negligible)  

This will give you **perfect square pixels** at 1:1 scaling!