# Metal Migration & Platform Abstraction Summary

## Overview
This document summarizes the ongoing effort to migrate the FloppyTurd codebase to support both Raylib (for non-iOS platforms) and Metal (for iOS), with a robust platform abstraction layer (PAL). It details the steps taken, recurring issues, and recommendations for future architecture improvements to bridge Objective-C++ (Metal) and C++ (Raylib) effectively.

---

## Migration Steps & Issues Encountered

### 1. Initial State
- Raylib was deeply integrated into the rendering pipeline.
- Early attempts to add Metal for iOS led to a mix of `.cpp`, `.mm`, and `.h` files with platform-specific hacks.
- Compile errors arose from mixing C++ and Objective-C++ code, especially with header inclusions.

### 2. Platform Abstraction Layer (PAL)
- Created a PAL to abstract rendering, allowing the game to use Raylib or Metal based on platform.
- Implemented conditional compilation and compatibility layers (`RaylibCompat.h`, `MetalRaylibCompat.h`).
- Noted that `MetalRaylibCompat.h` provides a Raylib-like API for iOS, but must only be included in Objective-C++ files.

### 3. File Organization & Compilation Issues
- Many headers (e.g., `AIGUI.h`, `PlatformLayer.h`) included `MetalRaylibCompat.h` directly or indirectly, causing Objective-C code to leak into C++ translation units.
- This led to build failures with errors about Objective-C types (e.g., `NSString`) in C++ files.
- Repeatedly refactored includes so that:
  - Only `.mm` files include `MetalRaylibCompat.h`.
  - All headers included by `.cpp` files use only `RaylibCompat.h`.
- Cleaned up and removed experimental or redundant files (e.g., `RenderInterface.h`, `RendererFactory.cpp`).

### 4. Build System & CMake
- Ensured CMake only builds Raylib for non-iOS, and Metal for iOS.
- Cleaned build directories and reconfigured to avoid stale includes.
- Still encountered issues with header inclusion order and stale build artifacts.

---

## Recurring Woes
- **Objective-C++ Leakage:**
  - Including Objective-C headers in C++ files causes cryptic build errors.
  - Must strictly separate C++ and Objective-C++ code at the header level.
- **Header Inclusion Order:**
  - Some headers transitively include forbidden files, making it hard to track down the source of errors.
- **Build System Staleness:**
  - CMake or Xcode may use cached headers or object files, requiring full clean builds after header changes.
- **Complexity of PAL:**
  - The PAL must be simple and only expose C++-safe interfaces to the game code.

---

## Suggestions for Future Architecture Refactors

1. **Strict Header Policy:**
   - Never include Objective-C or Objective-C++ headers in any file that might be included by a `.cpp` file.
   - Use pure C/C++ headers for all interfaces exposed to the game logic.
   - Only `.mm` files (or `.h` files included exclusively by `.mm` files) should include Objective-C headers.

2. **Opaque Handles & PIMPL:**
   - Use opaque pointers or the PIMPL idiom to hide Objective-C types from C++ code.
   - Example: `struct Texture2D { void* impl; int width; int height; };` where `impl` is only used in Objective-C++.
   - Apply PIMPL to resource classes like `Texture2D` and `Sprite`, with implementations in `Texture2D.cpp` (Raylib) and `Texture2D.mm` (Metal).

3. **Unified Render API:**
   - Define a minimal, platform-agnostic rendering API in C++ (`Renderer.h`).
   - Implement this API in `RendererRaylib.cpp` for non-iOS and `RendererMetal.mm` for iOS, using a factory pattern (`RendererFactory.cpp`) to instantiate the correct implementation.
   - Replace `PlatformLayer` rendering calls with `Renderer` methods in game code.

4. **Build System Hygiene:**
   - Always perform a full clean build after changing header inclusion logic using a `clean_all` CMake target.
   - Use CMake `target_sources` to separate `.cpp` and `.mm` files into distinct build targets.
   - Restrict `MetalRaylibCompat.h` to iOS-specific include directories (e.g., `include/ios`).
   - Set `COMPILE_FLAGS` to `-x objective-c++` for `.mm` files in CMake.

5. **Testing & CI:**
   - Add CI steps to build for both iOS and desktop to catch header inclusion issues early.
   - Test builds after each header or implementation change to ensure Metal and Raylib parity.

6. **Documentation:**
   - Maintain up-to-date documentation on header inclusion policies and platform abstraction boundaries.
   - Add comments to headers clarifying inclusion rules, e.g., “SAFE for .cpp and .mm” for `RaylibCompat.h` and “ONLY include in .mm files” for `MetalRaylibCompat.h`.

---

## Next Steps
- Audit all headers for accidental Objective-C++ inclusions, using forward declarations and PIMPL to eliminate leakage.
- Implement the `Renderer` interface and factory, replacing `PlatformLayer` rendering calls.
- Refactor resource classes (`Texture2D`, `Sprite`) to use PIMPL, with platform-specific implementations.
- Update `CMakeLists.txt` to separate `.cpp` and `.mm` files and add a `clean_all` target.
- Test builds on iOS (Metal) and desktop (Raylib) incrementally.
- Add documentation to headers and update this file with new conventions.

---

## File Relationship Tracker

### Core Game Files (C++)
| File | Type | Dependencies | Platform-Specific | Notes |
|------|------|--------------|-------------------|-------|
| `main.cpp` | C++ | PlatformLayer, Game | No | Entry point, platform-agnostic |
| `Game.cpp` | C++ | Playing, Menu, etc. | No | Main game loop |
| `Playing.cpp` | C++ | Renderer, RaylibCompat | No | Game state, uses Renderer API |
| `Menu.cpp` | C++ | Renderer, RaylibCompat | No | Menu system |
| `Bird.cpp` | C++ | Sprite, RaylibCompat | No | Player character |
| `Boss.cpp` | C++ | Sprite, RaylibCompat | No | Boss entities |
| `AudioManager.cpp` | C++ | AIGUI, PlatformLayer | No | Audio system |
| `AudioClip.cpp` | C++ | RaylibCompat | No | Audio resource |
| `AnimatedLayer.cpp` | C++ | Sprite, RaylibCompat | No | Background layers |
| `AnimatedParallaxLayer.cpp` | C++ | AnimatedLayer | No | Parallax backgrounds |
| `BossHealthBar.cpp` | C++ | RaylibCompat | No | UI element |
| `BossLevel.cpp` | C++ | Boss, RaylibCompat | No | Boss level logic |
| `TouchControls.cpp` | C++ | PlatformLayer | No | Touch input handling |

### Platform Abstraction Files
| File | Type | Dependencies | Platform-Specific | Notes |
|------|------|--------------|-------------------|-------|
| `PlatformLayer.h` | Header | RaylibCompat | No | Platform abstraction interface, uses forward declarations |
| `PlatformLayer.cpp` | C++ | RaylibCompat | No | Raylib implementation for non-iOS |
| `PlatformLayer.mm` | ObjC++ | MetalRaylibCompat | Yes | Metal implementation for iOS |
| `RaylibCompat.h` | Header | None | No | C++-safe Raylib compatibility |
| `MetalRaylibCompat.h` | Header | Metal, Foundation | Yes | iOS-only, ObjC++ only, in `include/ios` |
| `AIGUI.h` | Header | RaylibCompat | No | GUI system interface |
| `AIGUI.cpp` | C++ | RaylibCompat | No | GUI implementation for non-iOS |
| `AIGUI.mm` | ObjC++ | MetalRaylibCompat | Yes | GUI implementation for iOS |
| `Renderer.h` | Header | RaylibCompat | No | Unified rendering interface |
| `RendererRaylib.cpp` | C++ | RaylibCompat | No | Raylib rendering implementation |
| `RendererMetal.mm` | ObjC++ | MetalRaylibCompat | Yes | Metal rendering implementation |
| `RendererFactory.cpp` | C++ | Renderer | Yes | Factory for platform-specific renderers |

### Metal Rendering Files (iOS Only)
| File | Type | Dependencies | Platform-Specific | Notes |
|------|------|--------------|-------------------|-------|
| `MetalRenderer.h` | Header | MetalRaylibCompat | Yes | Metal renderer interface |
| `MetalRenderer.mm` | ObjC++ | Metal, MetalRaylibCompat | Yes | Metal 2D rendering |
| `MetalTexture.h` | Header | MetalRaylibCompat | Yes | Metal texture interface |
| `MetalTexture.mm` | ObjC++ | Metal, MetalRaylibCompat | Yes | Metal texture handling |
| `MetalShader.h` | Header | MetalRaylibCompat | Yes | Metal shader interface |
| `MetalShader.mm` | ObjC++ | Metal, MetalRaylibCompat | Yes | Metal shader management |
| `MetalTextRenderer.h` | Header | MetalRaylibCompat | Yes | Metal text rendering |
| `MetalTextRenderer.mm` | ObjC++ | Metal, MetalRaylibCompat | Yes | Metal text rendering |

### iOS Platform Files
| File | Type | Dependencies | Platform-Specific | Notes |
|------|------|--------------|-------------------|-------|
| `iOS/AppDelegate.mm` | ObjC++ | UIKit, MetalRaylibCompat | Yes | iOS app lifecycle |
| `iOS/GameViewController.mm` | ObjC++ | UIKit, MetalRaylibCompat | Yes | iOS view controller |
| `iOS/HapticsManager.mm` | ObjC++ | CoreHaptics | Yes | iOS haptic feedback |

### Resource Management
| File | Type | Dependencies | Platform-Specific | Notes |
|------|------|--------------|-------------------|-------|
| `ResourceManager.h` | Header | PlatformLayer, RaylibCompat | No | Resource management interface |
| `ResourceManager.cpp` | C++ | PlatformLayer, RaylibCompat | No | Resource loading for non-iOS |
| `ResourceManager.mm` | ObjC++ | MetalRaylibCompat | Yes | Resource loading for iOS |
| `ResourceCompat.h` | Header | RaylibCompat | No | Resource compatibility layer |
| `Sprite.h` | Header | RaylibCompat | No | Sprite interface, uses PIMPL |
| `Sprite.cpp` | C++ | RaylibCompat | No | Sprite implementation for non-iOS |
| `Sprite.mm` | ObjC++ | MetalRaylibCompat | Yes | Sprite implementation for iOS |
| `Texture2D.h` | Header | RaylibCompat | No | Texture interface, uses PIMPL |
| `Texture2D.cpp` | C++ | RaylibCompat | No | Texture implementation for non-iOS |
| `Texture2D.mm` | ObjC++ | MetalRaylibCompat | Yes | Texture implementation for iOS |

### Build System Files
| File | Type | Dependencies | Platform-Specific | Notes |
|------|------|--------------|-------------------|-------|
| `CMakeLists.txt` | CMake | Platform detection | Yes | Build configuration, separates .cpp and .mm files |
| `Info.plist` | XML | None | Yes | iOS app metadata |
| `Shaders2D.metal` | Metal | None | Yes | Metal shader source |

### Critical Dependencies to Monitor
1. **RaylibCompat.h** - Safe for all C++ files, provides platform-agnostic API.
2. **MetalRaylibCompat.h** - ONLY for .mm files, contains Objective-C code, located in `include/ios`.
3. **PlatformLayer.h** - Must only include `RaylibCompat.h`, never `MetalRaylibCompat.h`.
4. **AIGUI.h** - Must only include `RaylibCompat.h`, never `MetalRaylibCompat.h`.
5. **Renderer.h** - Unified rendering interface, safe for all files.
6. **Texture2D.h**, **Sprite.h** - Use PIMPL to hide platform-specific details.

### Refactoring Priority
1. **High Priority**: Ensure no .cpp files transitively include `MetalRaylibCompat.h`, using forward declarations and PIMPL.
2. **Medium Priority**: Consolidate platform-specific code into .mm files, implement `Renderer` interface.
3. **Low Priority**: Optimize header inclusion order and reduce compilation time.

---

*This file should be referenced at the start of future migration or refactor sessions to avoid repeating past mistakes and to guide clean architecture decisions.*