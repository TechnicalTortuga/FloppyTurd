# Parallax Segment Gap System Implementation
**Date:** October 13, 2025

## Overview
Implemented a proper **programmatic gap system** for parallax background segments, allowing configurable spacing between segments to reveal underlying layers (e.g., castle background visible through curtain gaps).

## Problem Statement
Castle level curtains were rendering tightly packed with no space between segments, completely obscuring the castle background behind them. The original workaround of changing scroll speeds was rejected as it didn't fit the game design intent.

## Solution: Segment Gap Configuration

### Architecture Changes

#### 1. **Parallax Component** (`GameComponents.h`)
Added `segmentGap` field to control spacing:
```cpp
struct Parallax : public Gnosis::Component {
    float scrollSpeed;
    float repeatWidth;
    bool autoScroll;
    float segmentGap;       // Gap in pixels between segments (0 = no gap, tight positioning)
    
    Parallax()
        : scrollSpeed(50.0f)
        , repeatWidth(800.0f)
        , autoScroll(true)
        , segmentGap(0.0f)  // Default: no gap for backward compatibility
    {}
};
```

#### 2. **BackgroundLayer Config** (`LevelConfig.h`)
Added `segmentGap` to layer configuration:
```cpp
struct BackgroundLayer {
    // ... existing fields ...
    float segmentGap;       // Gap in pixels between segments (0 = tight, use for castle curtains)
    
    BackgroundLayer(const std::string& texture, float speed, float layerDepth, int layer = 0)
        : // ... existing initializers ...
        , segmentGap(0.0f)  // Default: no gap
    {}
};
```

#### 3. **LevelManager Initial Positioning** (`LevelManager.cpp`)
Modified segment creation to apply gap during initial positioning:
```cpp
// Apply segment gap if configured (for castle curtains, etc.)
int gapInt = static_cast<int>(std::round(layerConfig.segmentGap));

for (int i = 0; i < instancesNeeded; ++i) {
    int scaledWidthInt = static_cast<int>(std::round(scaledWidth));
    // Apply segment gap: each segment is positioned at (width + gap) * index
    int xPosInt = i * (scaledWidthInt + gapInt);
    float xPos = static_cast<float>(xPosInt);
    // ...
}
```

And transfer gap to Parallax component:
```cpp
Parallax parallax;
parallax.scrollSpeed = layerConfig.scrollSpeed;
parallax.repeatWidth = static_cast<float>(scaledWidthInt);
parallax.autoScroll = true;
parallax.segmentGap = layerConfig.segmentGap; // Apply gap from config
```

#### 4. **CameraSystem Wrapping Logic** (`CameraSystem.cpp`)
Modified wrapping to maintain gap when segments wrap around:
```cpp
// Apply segment gap if configured (for castle curtains, etc.)
int segmentGapInt = static_cast<int>(std::round(parallax->segmentGap));

if (rightmostEntity != 0) {
    // Snap to exact pixel boundary and add segment gap
    transform->position.x = static_cast<float>(rightmostSegmentEnd + segmentGapInt);
}
```

### Castle Level Configuration
Applied 512px gap to curtains (full segment width):
```cpp
void LevelConfigFactory::AddCastleLevelLayers(LevelConfig& config) {
    // Back layer - castle background
    config.backgroundLayers.emplace_back("castlebacklayerbackground.png", 200.0f, 1.0f, 0);

    // Front layer - curtains with programmatic gaps to show castle background
    config.backgroundLayers.emplace_back("curtains.png", 200.0f, 1.05f, 2);
    
    // Set segment gap to create spacing between curtain segments
    // This reveals the castle background behind without changing scroll speed
    config.backgroundLayers.back().segmentGap = 512.0f; // Full segment width gap
}
```

## Technical Details

### Gap Application Points
1. **Initial Creation** (`LevelManager::CreateBackgroundLayers`)
   - Segments positioned at `index * (width + gap)`
   - Pure integer arithmetic for pixel-perfect positioning

2. **Wrapping** (`CameraSystem::UpdateParallaxLayers`)
   - Wrapped position = `rightmostSegmentEnd + gap`
   - Maintains consistent spacing across wraps

### Key Design Decisions

#### Why Not Change Scroll Speed?
- Changing speed alters gameplay feel and timing
- User requested **programmatic spacing**, not temporal separation
- Gaps should be static, not dynamic

#### Why Integer Arithmetic?
- Eliminates floating-point precision errors
- Ensures pixel-perfect alignment
- No sub-pixel artifacts or flickering

#### Why Per-Layer Configuration?
- Different layers may need different gap sizes
- Castle curtains: large gap (512px = full segment width)
- Other layers: no gap (0px = tight positioning)
- Flexible for future level designs

### Backward Compatibility
- Default `segmentGap = 0.0f` maintains existing behavior
- All existing levels unaffected (park, sewer, desert, snow, boss)
- Only castle level configured with gap

## Usage Examples

### Tight Positioning (Default)
```cpp
// No gap - segments touch seamlessly
config.backgroundLayers.emplace_back("Level1BackLayerBackground.png", 50.0f, 0.1f, 0);
// segmentGap defaults to 0.0f
```

### Castle Curtains with Full-Width Gap
```cpp
config.backgroundLayers.emplace_back("curtains.png", 200.0f, 1.05f, 2);
config.backgroundLayers.back().segmentGap = 512.0f; // Full segment width gap
```

### Custom Gap Size
```cpp
config.backgroundLayers.emplace_back("SomeTexture.png", 100.0f, 0.5f, 1);
config.backgroundLayers.back().segmentGap = 256.0f; // Half-width gap
```

## Testing Checklist

### Castle Level (Level 5)
- [ ] Curtain segments have visible gaps between them
- [ ] Castle background visible through gaps
- [ ] Gaps consistent across all segments
- [ ] No flickering or z-fighting
- [ ] Wrapping maintains gap spacing
- [ ] Scroll speed unchanged (200.0f for both layers)

### Other Levels (Regression Testing)
- [ ] **Park (Level 1):** No gaps, seamless backgrounds
- [ ] **Sewer (Level 2):** No gaps, seamless backgrounds
- [ ] **Desert (Level 3):** No gaps, seamless backgrounds  
- [ ] **Snow (Level 4):** No gaps, seamless backgrounds (after preload fix)
- [ ] **Boss (Level 6):** Static background, no parallax

### Edge Cases
- [ ] Wrapping maintains consistent gap
- [ ] Multiple wraps don't accumulate error
- [ ] Gap works with different screen resolutions
- [ ] No performance impact from gap calculations

## Future Enhancements

### Potential Extensions
1. **Per-Segment Gap Variation**
   - Randomize gap size within a range
   - Create organic, non-uniform spacing

2. **Segment Visibility Control**
   - Hide specific segment indices (e.g., every other segment)
   - More flexible than gaps alone

3. **Gap Animation**
   - Dynamic gap size based on gameplay events
   - Could expand/contract gaps for dramatic effect

### Gap Size Guidelines
- **Full Width (512px):** Large gaps, substantial reveal
- **Half Width (256px):** Moderate gaps, partial reveal
- **Quarter Width (128px):** Subtle gaps, peek-through effect
- **Custom:** Adjust based on texture design and desired aesthetic

## Summary

✅ **Proper programmatic gap system implemented**
- No workarounds or speed hacks
- Clean, configurable architecture
- Pixel-perfect positioning maintained
- Backward compatible with existing levels
- Ready for castle curtain spacing

The system now supports **true segment gaps** that reveal underlying background layers without affecting scroll speed or timing. Castle level will show the castle background through curtain gaps as designed.
