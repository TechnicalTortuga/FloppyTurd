# Swift-C++ Interop Functions Audit

## Overview
This document provides a comprehensive audit of all Swift functions that need to be exposed to C++ using `@_expose(Cxx)` annotations as part of the modernization from legacy C-style bridging to native Swift 5.9+ C++ interoperability.

## ✅ Already Implemented (Direct Swift-C++ Interop)

### InputEngine Functions
**File:** `GameEngine/Input/InputEngineSwift.swift`
**Status:** ✅ Complete - All functions have `@_expose(Cxx)` annotations

- `@_expose(Cxx) nonisolated public static func isMouseButtonPressed(_ button: Int32) -> Bool`
- `@_expose(Cxx) nonisolated public static func getMousePosition() -> Vector2`
- `@_expose(Cxx) nonisolated public static func getMouseX() -> Float`
- `@_expose(Cxx) nonisolated public static func getMouseY() -> Float`
- `@_expose(Cxx) nonisolated public static func getTouchPosition(_ index: Int32) -> Vector2`
- `@_expose(Cxx) nonisolated public static func getTouchX(_ index: Int32) -> Float`
- `@_expose(Cxx) nonisolated public static func getTouchY(_ index: Int32) -> Float`
- `@_expose(Cxx) nonisolated public static func getTouchCount() -> Int32`

**C++ Usage:**
```cpp
// Direct calls - no bridge needed!
FloppyTurd::InputEngine::isMouseButtonPressed(0)
FloppyTurd::InputEngine::getMousePosition()
FloppyTurd::InputEngine::getTouchCount()
```

### UICoordinateSystem Functions
**File:** `GameEngine/UI/UICoordinateSystemSwift.swift`
**Status:** ✅ Complete - Functions exposed via global scope

- `@_expose(Cxx) public func UICoordinateSystem_GetSafeAreaRect(inPixels: Bool) -> Rectangle`
- `@_expose(Cxx) public func UICoordinateSystem_GetPixelScreenRect() -> Rectangle`

**C++ Usage:**
```cpp
// Direct calls - no bridge needed!
FloppyTurd::UICoordinateSystem_GetSafeAreaRect(true)
FloppyTurd::UICoordinateSystem_GetPixelScreenRect()
```

## 🚧 Needs Implementation (`@_expose(Cxx)` Required)

### 1. MetalRenderer Functions
**File:** `GameEngine/Rendering/MetalRendererSwift.swift`
**Current Status:** ❌ No `@_expose(Cxx)` annotations
**Priority:** High

**Functions to Expose:**
- `updateViewport(width: Int, height: Int)`
- `getCurrentFPS() -> Int32` (needs implementation)
- `setTargetFPS(_ fps: Int32)` (needs implementation)
- `getFrameTime() -> Float` (needs implementation)

**Implementation Notes:**
- `MetalRendererSwift` has `targetFrameTime` (1.0/60.0) and `frameStartTime` properties
- Need to add FPS calculation logic based on frame timing
- Functions should be `nonisolated public static func` for C++ compatibility

### 2. AudioManager Functions
**File:** `GameEngine/Audio/AudioManagerSwift.swift`
**Current Status:** ❌ No `@_expose(Cxx)` annotations
**Priority:** Medium

**Functions to Expose:**
- `playSound(_ soundId: String) -> Bool`
- `playMusic(_ musicId: String) -> Bool`
- `stopMusic()`
- `setMasterVolume(_ volume: Float)`
- `setSoundVolume(_ volume: Float)`
- `setMusicVolume(_ volume: Float)`
- `isMusicPlaying() -> Bool`

### 3. HapticsManager Functions
**File:** `GameEngine/Haptics/HapticsManagerSwift.swift`
**Current Status:** ❌ No `@_expose(Cxx)` annotations
**Priority:** Medium

**Functions to Expose:**
- `playImpact(_ style: UIImpactFeedbackGenerator.FeedbackStyle)`
- `playNotification(_ type: UINotificationFeedbackGenerator.FeedbackType)`
- `playSelection()`

**Implementation Notes:**
- Need to create C++-compatible enums for feedback types
- Consider simplified interface: `vibrate(_ intensity: Int32)` for C++ ease

### 4. ResourceManager Path Functions
**File:** `GameEngine/Resources/ResourceManagerSwift.swift`
**Current Status:** ❌ No `@_expose(Cxx)` annotations
**Priority:** High

**Functions to Expose:**
- `getResourcePath(_ resourceName: String) -> String`
- `getSaveDataPath(_ filename: String) -> String`
- `getDocumentsDirectory() -> String` (needs implementation)
- `getApplicationSupportDirectory() -> String` (needs implementation)

**Implementation Notes:**
- Current path resolution is private (`resolveResourcePath`)
- Need public static functions for C++ access
- Should leverage existing `resourcesPath` property

### 5. Platform-Specific Functions
**Current Status:** ❌ Only in legacy bridge
**Priority:** Medium

**Functions Needed:**
- `setOrientation(_ landscape: Bool)`
- `showVirtualKeyboard(_ show: Bool)`
- `isMobilePlatform() -> Bool`
- `preferLowPowerMode() -> Bool`
- `getRecommendedTextureSize() -> Int32`

**Implementation Notes:**
- Currently only exist in `CppInteropBridgeSwift.swift`
- Need proper Swift classes/managers for these functions
- Consider creating `IOSPlatformManagerSwift` class

## 📋 Implementation Strategy

### Phase 1: Core Rendering & Performance
1. **MetalRenderer FPS Functions**
   - Add FPS calculation logic to `MetalRendererSwift`
   - Expose `getCurrentFPS()`, `setTargetFPS()`, `getFrameTime()`
   - Update `PlatformAPI_Modern.h` TODO markers

### Phase 2: Audio System
2. **AudioManager Exposure**
   - Add `@_expose(Cxx)` to existing public functions
   - Ensure `nonisolated` where appropriate
   - Test audio playback from C++

### Phase 3: Resource Management
3. **ResourceManager Path Functions**
   - Create public static path resolution functions
   - Add `@_expose(Cxx)` annotations
   - Implement missing directory path functions

### Phase 4: Platform Features
4. **Haptics & Platform Functions**
   - Expose haptics functions with simplified C++ interface
   - Create dedicated platform manager for orientation/keyboard
   - Add `@_expose(Cxx)` annotations

### Phase 5: Testing & Migration
5. **Validation & Cleanup**
   - Test all new direct Swift-C++ calls
   - Update `PlatformAPI_Modern.h` implementations
   - Remove legacy bridge dependencies
   - Delete `CppInteropBridgeSwift.swift`

## 🔧 Technical Requirements

### For `@_expose(Cxx)` Compatibility:
1. **Function Signature Requirements:**
   - Use C++-compatible types (Int32, Float, Bool, String)
   - Avoid Swift-specific types (CGFloat, NSString)
   - Use `nonisolated` for thread-safe functions
   - Prefer `public static func` for utility functions

2. **Class Requirements:**
   - Classes should be `public final class`
   - Use `nonisolated public static let shared` for singletons
   - Avoid `@MainActor` on exposed functions (use internal dispatch)

3. **Type Mapping:**
   ```swift
   // ✅ Good - C++ compatible
   @_expose(Cxx) nonisolated public static func getVolume() -> Float
   @_expose(Cxx) nonisolated public static func isPlaying() -> Bool
   @_expose(Cxx) nonisolated public static func playSound(_ id: String) -> Bool
   
   // ❌ Bad - Swift-specific types
   @_expose(Cxx) func getVolume() -> CGFloat  // Use Float instead
   @_expose(Cxx) func playSound(_ id: NSString) -> Bool  // Use String instead
   ```

## 🎯 Success Criteria

### When Complete:
1. **All C++ code uses direct Swift calls:**
   ```cpp
   // Instead of: FloppyTurd::getCppInteropBridge().playSound("jump")
   // We use: FloppyTurd::AudioManager::playSound("jump")
   ```

2. **No more legacy bridge dependencies:**
   - `CppInteropBridgeSwift.swift` can be deleted
   - All `@_cdecl` functions removed
   - `PlatformAPI.h` replaced with `PlatformAPI_Modern.h`

3. **Performance improvements:**
   - Reduced function call overhead
   - Better type safety
   - Cleaner C++ code

4. **Maintainability:**
   - Clear separation of concerns
   - Each Swift manager handles its domain
   - Direct, obvious function calls from C++

## 🚀 Ready to Polish This Turd!

Carl's ready to make these Swift functions shine brighter than a freshly polished turd! Each `@_expose(Cxx)` annotation brings us closer to a cleaner, faster, and more maintainable codebase. Let's start with the high-priority items and work our way through this magnificent modernization!

**Next Step:** Add `@_expose(Cxx)` annotations to the functions identified above, starting with MetalRenderer FPS functions and ResourceManager path functions.