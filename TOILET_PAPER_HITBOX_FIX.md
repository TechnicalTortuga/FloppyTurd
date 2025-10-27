# Toilet Paper Enemy Hitbox Fix

## Date: October 26, 2024

## Problem
The Toilet Paper enemies had hitboxes that were WAY too large - players were getting hit by "empty air" above and below the toilet paper sprites. The hitbox was approximately **25.6 pixels radius** (51.2px diameter) for a visual sprite that appears much smaller.

## Investigation

### Key Discovery: Two Codebases
The project has two separate codebases:
1. **`oldscripts/`** - Legacy Raylib-based code (NOT used in iOS build)
2. **`src/`** - New ECS-based code (ACTUALLY used in iOS build)

Initially modified `oldscripts/ToiletPaper.cpp` but this had **zero effect** on the iOS build because those files aren't compiled for iOS.

### iOS Build Architecture
- Uses **CMake** with platform detection
- iOS builds use **Metal/SpriteKit** (no Raylib)
- Sources compiled from `src/` directory only
- Entity Component System (ECS) architecture via Gnosis

### Hitbox System Location
The actual hitbox for Toilet Paper enemies is set in:
- **Config file**: `src/FloppyTurd/Config/EnemyConfigs.cpp`
- **Function**: `EnemyConfigRegistry::CreateToiletPaperConfig()`
- **Calculation**: Hitbox radius = `config.width * 0.4f`

### Original Values
```cpp
EnemyConfig config("ToiletPaperFlap", 64.0f, 64.0f, 6.0f, 25.0f, 3.0f, 1, 64, 64, 4, 0.30f, true, "horizontal");
```
- Sprite size: 64x64 pixels
- Config width: **64.0f**
- Calculated hitbox radius: **64.0f × 0.4f = 25.6px**
- Effective diameter: **51.2px** (huge!)

## Solution

### Changed Values
```cpp
EnemyConfig config("ToiletPaperFlap", 15.0f, 64.0f, 6.0f, 25.0f, 3.0f, 1, 64, 64, 4, 0.30f, true, "horizontal");
```
- Sprite size: Still 64x64 pixels (visual unchanged)
- Config width: **15.0f** (only used for hitbox calculation)
- New hitbox radius: **15.0f × 0.4f = 6.0px**
- Effective diameter: **12.0px** (much tighter!)

### Comparison to Player Hitbox
- Player hitbox radius: **11.0px**
- Toilet Paper hitbox radius: **6.0px** (now smaller than player!)
- This creates fair, skill-based gameplay

### Comparison to Original Request
- Requested: 12px wide × 8px tall rectangle
- Implemented: 12px diameter circle (6px radius)
- Note: iOS uses circular hitboxes (ColliderType::Circle) for enemies, not rectangles

## Files Modified

1. **`src/FloppyTurd/Config/EnemyConfigs.cpp`** (Line 162)
   - Changed first parameter from `64.0f` to `15.0f`
   - Added comment explaining hitbox calculation

## Testing
Build command used:
```bash
xcodebuild -project build_ios/FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" \
  build
```

**Result**: ✅ BUILD SUCCEEDED

## Further Adjustments
If the hitbox is still too large, you can reduce it further by lowering the width value:
- `width = 10.0f` → radius = 4.0px (8px diameter)
- `width = 7.5f` → radius = 3.0px (6px diameter)
- `width = 5.0f` → radius = 2.0px (4px diameter)

The formula is always: **hitbox_radius = width × 0.4**

## Additional Notes

### Debug Logging (Not Added to iOS Build)
While investigating, we prepared debug logging code for the old codebase, but it's not relevant since oldscripts aren't used. The iOS platform uses Swift/Metal for rendering and doesn't use the same logging infrastructure.

### Log Monitoring Script
Created `watch_logs.sh` to help monitor iOS Simulator logs if needed for future debugging:
```bash
chmod +x watch_logs.sh
./watch_logs.sh
```

### Architecture Lessons Learned
- Always check CMakeLists.txt to understand what's actually being compiled
- Platform-specific builds may use completely different source trees
- Don't assume file modifications will affect all builds without verification
- iOS build = `src/` folder (ECS architecture)
- Desktop/other = potentially `oldscripts/` folder (Raylib architecture)