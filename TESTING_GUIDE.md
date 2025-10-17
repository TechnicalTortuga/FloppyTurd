# Background Parallax Testing Guide
**Date:** October 13, 2025  
**Build:** iPhone 16 Simulator

## Changes Implemented

### 1. Snow Level Background Loading Fix ✅
**Issue:** Snow level backgrounds not appearing  
**Root Cause:** Asset preload mismatch in LoadingState.cpp  
**Fix:** Updated asset names to match current configuration

### 2. Park Level Cloud Layer Fix ✅
**Issue:** Clouds flickering/overlapping with mid layer  
**Root Cause:** Both on render layer 1 (Z-fighting)  
**Fix:** Moved clouds to layer 0

### 3. Castle Curtain Segment Gap System ✅
**Issue:** No castle background visible through curtains  
**Root Cause:** No spacing between curtain segments  
**Fix:** Implemented programmatic gap system (512px gaps)

## Testing Checklist

### Snow Level (Level 4) - PRIORITY
Navigate to Level 4 "Polar Pandemonium" and verify:

- [ ] **Background Layer 1 (Back)** - Sky/distant mountains visible
- [ ] **Background Layer 2 (Mid)** - Middle mountains visible
- [ ] **Background Layer 3 (Front)** - Trees and foreground visible
- [ ] **Background Layer 4 (Front Trees)** - Foreground trees visible
- [ ] All 4 layers scroll at different speeds (parallax effect)
- [ ] No gaps or black spaces in backgrounds
- [ ] No flickering or Z-fighting
- [ ] Smooth scrolling with no jitter

**Expected Result:** All 4 background layers now appear correctly. Previously showed no backgrounds at all.

### Castle Level (Level 5) - PRIORITY
Navigate to Level 5 "Dung in the Dungeon" and verify:

- [ ] **Castle background** visible (layer 0)
- [ ] **Curtain segments** visible (layer 2)
- [ ] **512px gaps between curtain segments**
- [ ] Castle background visible through gaps
- [ ] No overlapping curtain segments
- [ ] Curtains wrap correctly maintaining gaps
- [ ] Both layers scroll at same speed (200.0f)
- [ ] No speed differential (not faster/slower)

**Expected Result:** Curtain segments have clear gaps revealing castle background behind. Previously curtains were tightly packed.

### Park Level (Level 1) - REGRESSION TEST
Navigate to Level 1 "A Flop in the Park" and verify:

- [ ] **Back layer** visible (sky)
- [ ] **Mid layer** visible (buildings/ground)
- [ ] **Clouds** visible and no longer flickering
- [ ] **Front layer** visible (foreground elements)
- [ ] No Z-fighting or overlap artifacts
- [ ] All layers seamlessly tiled
- [ ] Smooth parallax scrolling

**Expected Result:** Clouds no longer flicker with mid layer. Previously had Z-fighting on layer 1.

### Desert Level (Level 3) - REGRESSION TEST
Navigate to Level 3 "The Good, The Bad, and the Stinky" and verify:

- [ ] **Back layer** visible (distant desert)
- [ ] **Mid layer** visible (cacti/dunes)
- [ ] **Front layer** visible (foreground)
- [ ] No overlapping or flickering
- [ ] All layers seamlessly tiled
- [ ] Proper parallax speed differences

**Expected Result:** No changes made, verify no regressions.

### Sewer Level (Level 2) - REGRESSION TEST
Navigate to Level 2 "Home Sweet Home" and verify:

- [ ] Sewer backgrounds visible
- [ ] Variant cycling works (A, B, C, D)
- [ ] Janitor NPC moves with background
- [ ] No gaps or overlaps
- [ ] Seamless wrapping

**Expected Result:** No changes made, verify no regressions.

### Boss Level (Level 6) - REGRESSION TEST
Navigate to Level 6 "Curtains for Crap" and verify:

- [ ] Boss background visible
- [ ] Boss floor positioned correctly
- [ ] Boss walls visible
- [ ] Screen curtains on sides
- [ ] Background is STATIC (not scrolling)
- [ ] Rat King boss visible and grounded

**Expected Result:** No changes made, verify no regressions.

## Technical Verification

### Log Monitoring
Watch for these log messages:

**Snow Level Loading:**
```
Loading level 4: Polar Pandemonium
Creating background layers...
📋 Processing layer: 'SnowLevelBackLayerBackground.png'
📋 Processing layer: 'SnowLevelMidLayerBackground.png'
📋 Processing layer: 'SnowLevelFrontLayerBackground.png'
📋 Processing layer: 'SnowLevelFrontLayerTrees.png'
```

**Castle Level Gap Application:**
```
📋 Processing layer: 'castlebacklayerbackground.png'
📋 Processing layer: 'curtains.png'
Added Parallax component... with segmentGap=512.000000
🎭 SEGMENT GAP APPLIED: 'curtains.png' wrapped with 512px gap
```

### Performance Check
Monitor frame rate during gameplay:
- [ ] No frame drops in snow level
- [ ] No frame drops in castle level
- [ ] Gap calculations don't impact performance
- [ ] Smooth 60fps gameplay maintained

### Edge Case Testing

#### Castle Curtain Wrapping
1. Play castle level for 30+ seconds
2. Verify gaps remain consistent after multiple wraps
3. Check no gap accumulation or drift
4. Ensure pixel-perfect positioning maintained

#### Snow Level Asset Loading
1. Exit and re-enter snow level
2. Verify backgrounds load every time
3. Check no black flashes or delays
4. Confirm consistent rendering

#### Multiple Level Transitions
1. Play Level 1 → Level 4 → Level 5
2. Verify each level's backgrounds load correctly
3. Check no asset bleeding between levels
4. Confirm proper cleanup on transitions

## Known Limitations

### Segment Gap System
- **Gap is fixed** - doesn't vary per segment
- **All gaps same size** - can't alternate gap sizes
- **No gap animation** - gaps are static

### Future Enhancements
- Randomized gap sizes within range
- Per-segment visibility control
- Dynamic gap sizing based on events

## Debugging Tips

### If Snow Level Still Shows No Backgrounds
1. Check LoadingState.cpp preload list
2. Verify asset names match xcassets exactly
3. Check GetTextureDimensions() cache lookups
4. Monitor metadata retrieval logs

### If Castle Gaps Not Appearing
1. Check segmentGap value in config (should be 512.0f)
2. Verify Parallax component receives gap value
3. Monitor wrapping logic in CameraSystem
4. Check integer arithmetic in positioning

### If Flickering Persists
1. Verify render layers are unique
2. Check Z-depth values don't overlap
3. Ensure sprite visibility flags correct
4. Monitor synchronized layer movement

## Success Criteria

✅ **Snow level displays all 4 background layers**  
✅ **Castle curtains have visible 512px gaps**  
✅ **Park clouds don't flicker with mid layer**  
✅ **No performance degradation**  
✅ **No regressions in other levels**  
✅ **Smooth parallax scrolling maintained**

## Testing Notes
_Add your observations below:_

---

**Tester:**  
**Date:**  
**Build:**  

**Level 4 (Snow):**  
- Backgrounds visible: [ ] Yes [ ] No
- Issues found:

**Level 5 (Castle):**  
- Gaps visible: [ ] Yes [ ] No
- Gap size looks correct: [ ] Yes [ ] No
- Issues found:

**Level 1 (Park):**  
- Clouds fixed: [ ] Yes [ ] No
- Issues found:

**Other Levels:**  
- Regressions: [ ] Yes [ ] No
- Issues found:

**Overall Status:**  
[ ] All tests passed  
[ ] Some issues found  
[ ] Major issues found
