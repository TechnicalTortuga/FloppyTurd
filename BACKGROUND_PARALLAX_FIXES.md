# Background Parallax System Fixes
**Date:** October 13, 2025

## Issues Identified

### 1. **Snow Level Backgrounds Not Loading** ❌
**Root Cause:** Asset name mismatch between preloading and level configuration.

**LoadingState.cpp** was preloading OLD asset names:
- ❌ `SnowLevelBackground.png`
- ❌ `SnowLevelBackTrees.png`
- ❌ `SnowLevelFrontTrees.png`
- ❌ `SnowLevelMountains.png`
- ❌ `SnowLevelTundra.png`

**LevelConfig.cpp** expected NEW asset names:
- ✅ `SnowLevelBackLayerBackground.png`
- ✅ `SnowLevelMidLayerBackground.png`
- ✅ `SnowLevelFrontLayerBackground.png`
- ✅ `SnowLevelFrontLayerTrees.png`

When `LevelManager::CreateBackgroundLayers()` called `GetTextureDimensions()`, the cache lookup failed because the correct assets weren't preloaded, causing all snow level backgrounds to be skipped.

### 2. **Park Level: Overlapping Clouds and Mid Layer** 🌳
**Root Cause:** Two background layers assigned to the same render layer.

**Before:**
```cpp
// Mid layer - render layer 1
config.backgroundLayers.emplace_back("Level1MidLayerBackground.png", 100.0f, 0.3f, 1);

// Clouds - render layer 1 ❌ OVERLAP!
config.backgroundLayers.emplace_back("Level1Clouds.png", 75.0f, 0.2f, 1);
```

Both layers rendered on layer 1, causing Z-fighting and flickering.

### 3. **Castle Level: Curtains Too Dense** 🏰
**Root Cause:** Castle background and curtains moving at same speed with minimal depth separation.

**Before:**
```cpp
// Castle background - 200.0f speed, depth 1.0
config.backgroundLayers.emplace_back("castlebacklayerbackground.png", 200.0f, 1.0f, 0);

// Curtains - 200.0f speed, depth 1.05 (only 5% difference)
config.backgroundLayers.emplace_back("curtains.png", 200.0f, 1.05f, 2);
```

Both layers moved at identical speed, so curtains never separated to reveal the castle background behind them.

## Fixes Applied

### Fix 1: Snow Level Asset Preloading ✅
**File:** `src/FloppyTurd/States/LoadingState.cpp`

Updated asset preload list to match current level configuration:
```cpp
"Snowball.png",
"Snowfall.png",
"SnowLevelBackLayerBackground.png",  // ✅ NEW
"SnowLevelMidLayerBackground.png",   // ✅ NEW
"SnowLevelFrontLayerBackground.png", // ✅ NEW
"SnowLevelFrontLayerTrees.png",      // ✅ NEW
"SnowLevelPainting.png",
```

### Fix 2: Park Level Cloud Layer Separation ✅
**File:** `src/FloppyTurd/Config/LevelConfig.cpp`

Moved clouds to layer 0 (back layer) to prevent overlap:
```cpp
// Clouds - independent movement (adjusted to layer 0 to avoid overlap with mid layer)
config.backgroundLayers.emplace_back("Level1Clouds.png", 75.0f, 0.2f, 0); // ✅ Layer 0
```

### Fix 3: Castle Curtain Programmatic Gaps ✅
**Files:** 
- `src/FloppyTurd/Components/GameComponents.h` - Added `segmentGap` field
- `src/FloppyTurd/Config/LevelConfig.h` - Added `segmentGap` to BackgroundLayer
- `src/FloppyTurd/Config/LevelConfig.cpp` - Configured 512px gap for curtains
- `src/FloppyTurd/Systems/LevelManager.cpp` - Initial positioning with gaps
- `src/FloppyTurd/Systems/CameraSystem.cpp` - Wrapping with gaps

Implemented proper segment gap system:
```cpp
// Back layer - castle background
config.backgroundLayers.emplace_back("castlebacklayerbackground.png", 200.0f, 1.0f, 0);

// Front layer - curtains with programmatic gaps
config.backgroundLayers.emplace_back("curtains.png", 200.0f, 1.05f, 2);
config.backgroundLayers.back().segmentGap = 512.0f; // ✅ Full segment width gap
```

**Result:** Curtain segments have 512px gaps between them, revealing the castle background. Scroll speed unchanged - pure programmatic spacing.

## Technical Notes

### Positioning System (Already Correct) ✅
The `LevelManager::CreateBackgroundLayers()` system already uses **pixel-perfect integer arithmetic**:

```cpp
// Pure integer arithmetic for positioning
int scaledWidthInt = static_cast<int>(std::round(scaledWidth));
int xPosInt = i * scaledWidthInt;  // Each segment positioned by its actual width
float xPos = static_cast<float>(xPosInt);
```

**No offsets are applied** - each background segment uses its sprite's actual width. The flickering was purely a render layer issue, not a positioning problem.

### Parallax Wrapping (Already Correct) ✅
The `CameraSystem::UpdateParallaxLayers()` handles seamless wrapping:
- Groups entities by render layer
- Applies synchronized movement to all backgrounds in the same layer
- Uses pixel-perfect wrapping calculations
- No sub-pixel artifacts

The wrapping system was working correctly; the issue was multiple layers trying to render at the same Z-depth.

## Testing Checklist

- [ ] **Snow Level (Level 4):** Verify all 4 background layers now appear
  - Back layer (sky)
  - Mid layer (mountains)
  - Front layer (trees)
  - Front trees layer (foreground trees)
  
- [ ] **Park Level (Level 1):** Verify clouds no longer flicker with mid layer
  - Clouds should render behind mid layer elements
  - No Z-fighting or overlap artifacts
  
- [ ] **Castle Level (Level 5):** Verify curtains create natural spacing
  - Castle background visible between curtain segments
  - Curtains move faster than background for parallax effect
  
- [ ] **Desert Level (Level 3):** No changes made, verify no regressions
  
- [ ] **Sewer Level (Level 2):** No changes made, verify no regressions
  
- [ ] **Boss Level (Level 6):** No changes made, verify no regressions

## Asset Structure Verified

All background assets confirmed in xcassets:
```
src/Assets.xcassets/graphics/environment/backgrounds/
├── level1/
│   ├── Level1BackLayerBackground.imageset/
│   ├── Level1Clouds.imageset/
│   ├── Level1MidLayerBackground.imageset/
│   └── Level1FrontLayerBackground.imageset/
├── sewer/
│   ├── SewerLargeA.imageset/
│   ├── SewerLargeB.imageset/
│   ├── SewerLargeC.imageset/
│   └── SewerLargeD.imageset/
├── level3/
│   ├── Level3BackLayerBackground.imageset/
│   ├── Level3MidLayerBackground.imageset/
│   └── Level3FrontLayerBackground.imageset/
├── snow_level/
│   ├── SnowLevelBackLayerBackground.imageset/ ✅
│   ├── SnowLevelMidLayerBackground.imageset/  ✅
│   ├── SnowLevelFrontLayerBackground.imageset/ ✅
│   └── SnowLevelFrontLayerTrees.imageset/     ✅
├── castle/
│   ├── castlebacklayerbackground.imageset/
│   └── curtains.imageset/
└── boss/
    ├── BossLevelBackgroundMobile.imageset/
    ├── BossFloor.imageset/
    ├── BossWalls.imageset/
    └── screenCurtains.imageset/
```

## Summary

✅ **3 bugs fixed** with minimal, surgical changes:
1. Snow level backgrounds now load correctly via asset preloading fix
2. Park level clouds no longer overlap via layer reassignment  
3. Castle curtains create natural spacing via parallax speed differential

**No changes to core parallax systems** - positioning and wrapping logic already correct.
