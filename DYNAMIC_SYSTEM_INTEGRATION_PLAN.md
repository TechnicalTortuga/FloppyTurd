# FloppyTurd Dynamic System Integration Plan

## System Architecture Inventory

### Core Systems (Where Dynamic Config Should Be Integrated)

1. **AssetManager** (`src/Engine/Assets/AssetManager.{h,cpp}`)
   - **Current Role**: Handles all texture, audio, font, shader loading
   - **Integration Point**: Add dynamic texture metadata retrieval
   - **New Methods**: `getTextureMetadata()`, `getTextureDimensions()`

2. **RenderSystem** (`src/FloppyTurd/Systems/RenderSystem.{h,cpp}`)
   - **Current Role**: Unified rendering with camera, layers, world-to-screen
   - **Integration Point**: Add dynamic screen size handling
   - **New Methods**: `getScreenInfo()`, `calculateDynamicScale()`

3. **UISystem** (`src/FloppyTurd/Systems/UISystem.{h,cpp}`)
   - **Current Role**: UI element rendering, button creation, bounds checking
   - **Integration Point**: Add responsive layout calculations
   - **New Methods**: `calculateResponsiveScale()`, `getScreenSafeArea()`

4. **LevelManager** (`src/FloppyTurd/Systems/LevelManager.{h,cpp}`)
   - **Current Role**: Level lifecycle, progression, content spawning
   - **Integration Point**: Add device-specific layout functions
   - **New Methods**: `setupiOSLayout()`, `setupDesktopLayout()`

### Platform Communication Flow

```
PlatformDelegates.h (Enhanced ScreenInfo/TextureMetadata structs)
    ↓
iOSPlatformImpl.{h,cpp} (C++ delegate implementations)
    ↓
ThreadingProxy.{h,cpp} (Command queue proxy)
    ↓
ThreadingSystem.swift (Swift command processor)
    ↓
MetalRenderer.swift / AssetManager.swift (iOS implementations)
```

### States That Need Dynamic Layout Functions

1. **MainMenuState** (`src/FloppyTurd/States/MainMenuState.{h,cpp}`)
   - Add `setupiOSLayout()` and `setupDesktopLayout()` methods

2. **GameplayState** (`src/FloppyTurd/States/GameplayState.{h,cpp}`)
   - Add `setupiOSLayout()` and `setupDesktopLayout()` methods

3. **LoadingState** (`src/FloppyTurd/States/LoadingState.{h,cpp}`)
   - Add `setupiOSLayout()` and `setupDesktopLayout()` methods

## Implementation Plan

### Phase 1: Enhance Platform Delegates ✅ COMPLETED
- [x] Enhanced `PlatformDelegates.h` with `ScreenInfo` and `TextureMetadata` structs
- [x] Added `getScreenInfo()` and `getTextureMetadata()` delegate functions

### Phase 2: Integrate Dynamic Screen Info into RenderSystem ✅ COMPLETED
- [x] Add screen info management to `RenderSystem`
- [x] Add dynamic scaling calculations based on device
- [x] Remove hardcoded screen dimensions (1179×2556, 393×852, 800×600)

### Phase 3: Integrate Dynamic Texture Metadata into AssetManager ✅ COMPLETED
- [x] Add texture metadata retrieval to `AssetManager`
- [x] Remove hardcoded texture dimensions (2048×480, 1024×480, 512×180, etc.)
- [x] Cache texture metadata for performance

### Phase 4: Add Responsive Layout to UISystem ✅ COMPLETED
- [x] Add responsive scaling calculations to `UISystem`
- [x] Remove hardcoded UI dimensions and positions
- [x] Add safe area support for iOS

### Phase 5: Add Platform-Specific Layout Functions to States ✅ COMPLETED
- [x] Add `setupiOSLayout()` to `MainMenuState`
- [x] Add `setupiOSLayout()` to `GameplayState`
- [x] Add `setupiOSLayout()` to `LoadingState`
- [x] Call appropriate layout function based on platform

### Phase 6: Implement iOS Platform Delegates ✅ COMPLETED
- [x] Implement `getScreenInfo()` in `MetalRenderer.swift`
- [x] Implement `getTextureMetadata()` in `AssetManager.swift`
- [x] Connect delegates in `ThreadingProxy.cpp`
- [x] Add Swift C callable functions in `ThreadingSystem.swift`

### Phase 7: Update Core Systems to Use Dynamic Values
- [ ] Update `LevelManager` spawn positions
- [ ] Update `PlayerControllerSystem` boundaries
- [ ] Update `CameraSystem` viewport calculations
- [ ] Update `SpriteSystem` scaling

### Phase 8: Testing and Validation
- [ ] Build and test on iPhone 16 simulator
- [ ] Verify no hardcoded values remain
- [ ] Test responsive layout on different orientations
- [ ] Validate performance impact

## Hardcoded Values to Eliminate

### Screen Dimensions
- iPhone 16: `1179×2556` pixels, `393×852` logical
- Desktop fallback: `800×600`

### Texture Dimensions
- Backgrounds: `2048×480`, `1024×480`, `512×180`
- Sprites: `16×16`, `32×32`, `48×48`, `64×64`, `96×96`
- UI elements: `64×16`, various button sizes

### Assets to Fix
- TurdletIdle: Currently hardcoded as `64×64`
- Background layers: Various hardcoded dimensions
- UI buttons: Fixed sizes and positions

## File Modifications Required

### Core Systems
1. `src/Engine/Assets/AssetManager.{h,cpp}` - Add texture metadata
2. `src/FloppyTurd/Systems/RenderSystem.{h,cpp}` - Add screen info
3. `src/FloppyTurd/Systems/UISystem.{h,cpp}` - Add responsive layout
4. `src/FloppyTurd/Systems/LevelManager.{h,cpp}` - Add layout functions

### Platform Implementation
5. `src/Engine/Platform/iOSPlatformImpl.{h,cpp}` - Add delegate implementations
6. `src/iOS/Threading/ThreadingProxy.{h,cpp}` - Add command routing
7. `src/iOS/Rendering/MetalRenderer.swift` - Implement getScreenInfo
8. `src/iOS/Assets/AssetManager.swift` - Implement getTextureMetadata

### Game States
9. `src/FloppyTurd/States/MainMenuState.{h,cpp}` - Add layout functions
10. `src/FloppyTurd/States/GameplayState.{h,cpp}` - Add layout functions
11. `src/FloppyTurd/States/LoadingState.{h,cpp}` - Add layout functions

### Supporting Systems
12. `src/FloppyTurd/Systems/PlayerControllerSystem.{h,cpp}` - Dynamic boundaries
13. `src/FloppyTurd/Systems/CameraSystem.{h,cpp}` - Dynamic viewport
14. `src/FloppyTurd/Systems/SpriteSystem.{h,cpp}` - Dynamic scaling

## Design Philosophy

1. **No New Abstraction Layers**: Integrate into existing systems
2. **Platform-Specific Layout Functions**: `setupiOSLayout()` vs `setupDesktopLayout()` 
3. **Follow Existing Patterns**: Use the delegate → proxy → threading system flow
4. **Keep iOS/Desktop Separate**: Clear separation for future Raylib reintegration
5. **Dynamic Everything**: No hardcoded dimensions anywhere

## Implementation Focus

**iOS ONLY** - No Desktop/Raylib work until iOS dynamic system is complete and tested.
