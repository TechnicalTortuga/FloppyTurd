# Modern PlatformAPI Refactor Guide

## Overview
This document outlines the complete refactoring process from legacy C-style bridging to native Swift 5.9+ C++ interoperability for Floppy Turd's PlatformAPI system.

## Current State Analysis

### ✅ Already Implemented (Direct Swift-C++ Interop)

#### InputEngine Functions (InputEngineSwift.swift)
All functions are properly exposed with `@_expose(Cxx)` and `nonisolated public static func`:
- `isMouseButtonPressed(button: Int32) -> Bool`
- `getMousePosition() -> Vector2`
- `getMouseX() -> Float`
- `getMouseY() -> Float`
- `getTouchPosition(index: Int32) -> Vector2`
- `getTouchX(index: Int32) -> Float`
- `getTouchY(index: Int32) -> Float`
- `getTouchCount() -> Int32`

#### UICoordinateSystem Functions (UICoordinateSystemBridgeSwift.swift)
All functions are public and working with direct interop:
- `UICoordinateSystem_GetPixelScreenRect() -> Rectangle`
- `UICoordinateSystem_GetPointScreenRect() -> Rectangle`
- `UICoordinateSystem_GetSafeAreaRect() -> Rectangle`

### 🔄 Needs @_expose(Cxx) Annotations

#### MetalRenderer Functions (MetalRendererSwift.swift)
Currently public but missing `@_expose(Cxx)` annotations:
- `updateViewport(width: Float, height: Float)`
- `initialize()` (Note: @MainActor - needs bridge function)
- `shutdown()`
- Potential candidates for FPS/performance metrics

#### AudioManager Functions (AudioManagerSwift.swift)
Need to identify and expose audio-related functions:
- Audio playback controls
- Volume management
- Audio state queries

#### Platform-Specific Functions
Need to identify Swift implementations for:
- `GetFPS()` - Performance monitoring
- `SetOrientation()` - Device orientation
- `ShowVirtualKeyboard()` / `HideVirtualKeyboard()` - iOS keyboard
- `Vibrate()` - Haptic feedback
- `GetResourcePath()` / `GetSaveDataPath()` - File system
- `GetScreenCenter()` / `GetRenderScale()` - Display metrics
- `GetScreenDensity()` / `GetSafeArea()` - iOS-specific UI

## Implementation Strategy

### Phase 1: Documentation and Discovery
1. ✅ Create this comprehensive guide
2. 🔄 Search for existing Swift implementations of missing functions
3. 🔄 Identify which functions need new Swift implementations
4. 🔄 Document the complete function mapping

### Phase 2: Add @_expose(Cxx) Annotations
1. 🔄 Add annotations to MetalRenderer functions
2. 🔄 Add annotations to AudioManager functions
3. 🔄 Add annotations to platform-specific functions
4. 🔄 Create bridge functions for @MainActor functions that can't be directly exposed

### Phase 3: Update PlatformAPI_Modern.h
1. 🔄 Replace all TODO comments with actual Swift function calls
2. 🔄 Ensure all iOS implementations use `FloppyTurd::` namespace calls
3. 🔄 Verify Desktop implementations remain using raylib directly
4. 🔄 Add proper error handling and fallbacks

### Phase 4: Testing and Validation
1. 🔄 Build iOS simulator target
2. 🔄 Test all input functions
3. 🔄 Test all rendering functions
4. 🔄 Test all audio functions
5. 🔄 Test all platform-specific functions
6. 🔄 Performance comparison with legacy system

### Phase 5: Migration and Cleanup
1. 🔄 Replace PlatformAPI.h with PlatformAPI_Modern.h
2. 🔄 Remove CppInteropBridgeSwift.swift (legacy)
3. 🔄 Update all C++ files to use new API
4. 🔄 Remove deprecated C-style bridge functions

## Technical Notes

### Swift-C++ Interop Requirements
- Functions must be `nonisolated public static func`
- Must have `@_expose(Cxx)` annotation
- @MainActor functions need bridge functions
- Use Swift namespace: `FloppyTurd::ClassName::functionName()`

### Performance Benefits
- Direct function calls (no C-style bridging overhead)
- Type safety at compile time
- Better optimization opportunities
- Reduced memory allocations

### Compatibility
- Maintains same API surface for C++ code
- Platform detection ensures correct implementation
- Fallback mechanisms for unsupported functions

## Current PlatformAPI_Modern.h Status

### ✅ Implemented Functions
- Input system (mouse, touch, keyboard)
- UI coordinate system (screen rects, safe area)
- Vector math operations
- Collision detection
- Basic platform detection

### 🔄 TODO Functions (Need Swift Implementations)
- `GetFPS()` - Line 132
- `SetOrientation()`
- `ShowVirtualKeyboard()` / `HideVirtualKeyboard()`
- `Vibrate()`
- `GetResourcePath()` / `GetSaveDataPath()`
- Audio management functions
- Rendering control functions
- Platform-specific UI functions

## Next Steps
1. Search for existing Swift implementations of missing functions
2. Add @_expose(Cxx) annotations to identified functions
3. Create new Swift implementations for missing functionality
4. Update PlatformAPI_Modern.h with actual function calls
5. Test iOS build with new system

---

*This refactor represents a major step toward modern, type-safe, and performant C++ ↔ Swift interoperability in Floppy Turd. Once complete, we'll have eliminated all legacy C-style bridging in favor of native Swift 5.9+ interop.*