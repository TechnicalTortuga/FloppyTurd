# FloppyTurd Architecture Audit (Swift Bridge & Game Integration)

## Overview
This document tracks the current state of the FloppyTurd game architecture, focusing on the Swift bridge, major managers, game states, and the migration away from legacy Objective-C/C++ (.mm) files. It highlights relationships, throughlines, missing links, and provides actionable recommendations for a robust, maintainable system.

---

## 1. Swift Bridge Throughline
- **PlatformAPI**: All platform calls route through PlatformAPI.h, which conditionally forwards to Cpp2Swift (iOS) or raylib (desktop).
- **Cpp2Swift**: C++ namespace that bridges to Swift via `@_silgen_name` functions.
- **Swift Bridge**: All platform, rendering, input, resource, and audio calls are stubbed or implemented in Swift, grouped by subsystem.
- **ResourceManagerSwift**: Handles all resource loading (textures, fonts, audio) for iOS, using Metal and async/await where possible.
- **GameEngine/Game**: Main game loop and state management, now expected to call into PlatformAPI for all platform-specific needs.

---

## 2. Key Managers & Game States
### ResourceManager
- **Current**: `ResourceManagerSwift` is the canonical resource loader for iOS, integrated with Metal and async/await.
- **Linkage**: All resource requests from game code should route through PlatformAPI → Cpp2Swift → Swift ResourceManager.
- **TODO**: Audit all usages of ResourceManager in C++ and ensure they are routed through the bridge for iOS.

### Game Class
- **Current**: Multiple versions (`GameEngine.swift`, `GameEngineClean.swift`, `GameEngineOld.swift`).
- **Linkage**: Initialization and main loop should call PlatformAPI for window, input, and rendering setup.
- **TODO**: Ensure only one canonical GameEngine is used; remove or archive old versions.

### AIGUI
- **Current**: Updated for platform-agnostic input and touch-friendly UI.
- **Linkage**: All input and rendering should go through PlatformAPI.
- **TODO**: Verify all AIGUI input/rendering is routed through the bridge.

### MainMenu, Playing, Loading, Credits
- **Current**: Each state should use PlatformAPI for input, rendering, and resource access.
- **TODO**: Audit each state for direct platform calls; refactor to use PlatformAPI/Swift bridge as needed.

---

## 3. .mm Files (Objective-C++/Legacy)
### Files to Remove or Refactor

**Action:**


## 4. Stub Implementation Checklist
- [ ] All Swift bridge stubs implemented and connected to real engine logic.

- [x] MetalRenderer.mm → MetalRendererSwift.swift (MetalKit)
- [x] MetalTextRenderer.mm → MetalTextRendererSwift.swift (MetalKit)
- [x] AudioStateManager.mm → AudioManagerSwift.swift (AVFoundation)
- [x] ResourceManager.mm/FontCache.mm → ResourceManagerSwift.swift (Foundation, MetalKit)
- [x] LogManager.mm → LogManagerSwift.swift (Foundation, os.log) (**TraceLog integration next**)
- [ ] GameLog.mm → GameLogSwift.swift (Foundation, os.log)
- [x] HapticsManager.mm → HapticsManagerSwift.swift (CoreHaptics)
- [ ] PlatformTraitsIOS.mm → PlatformTraitsIOSSwift.swift (UIKit)
- [ ] UICoordinateSystem.mm → UICoordinateSystemSwift.swift (UIKit)
- [ ] WindowIOS.mm → WindowIOSSwift.swift (UIKit)
- [ ] MetalTextureCache.mm → MetalTextureCacheSwift.swift (MetalKit)
- [ ] MetalTexture.mm → MetalTextureSwift.swift (MetalKit)
- [ ] MetalFrameResources.mm → MetalFrameResourcesSwift.swift (MetalKit)
- [ ] MetalShader.mm → MetalShaderSwift.swift (MetalKit)
- [ ] test_parameter_passing.mm → (remove)
- [ ] ios-signing-test/main.mm → (remove)

**Audit Notes:**
- Metal, font, audio, resource management, logging, and haptics are now implemented in Swift.
- Next: Integrate TraceLog with LogManagerSwift, audit and consolidate GameEngine versions, and continue refactoring remaining systems.
- [ ] MetalShader.mm → MetalShaderSwift.swift (MetalKit)
- [ ] LogManager.mm → LogManagerSwift.swift (Foundation, os.log)
- [ ] GameLog.mm → GameLogSwift.swift (Foundation, os.log)
- [ ] PlatformTraitsIOS.mm → PlatformTraitsIOSSwift.swift (UIKit)
- [ ] UICoordinateSystem.mm → UICoordinateSystemSwift.swift (UIKit)
- [ ] WindowIOS.mm → WindowIOSSwift.swift (UIKit)
- [ ] HapticsManager.mm → HapticsManagerSwift.swift (CoreHaptics)
- [ ] test_parameter_passing.mm → (remove)
- [ ] ios-signing-test/main.mm → (remove)

**Migration Notes:**
- All Metal, AVFoundation, CoreGraphics, UIKit, and Foundation logic should be implemented in Swift using native frameworks.
- Remove all Objective-C++ (.mm) files after migration.

---

## 5. Notes & Concerns
- Multiple GameEngine versions may cause confusion—consolidate.
- Ensure all C++ code uses PlatformAPI for platform calls, not direct iOS/Metal/ObjC.
- Test struct round-tripping between C++ and Swift for all bridged types.
- Document any remaining direct platform calls for future refactor.

---

## 6. Recommendations
- Prioritize removal of .mm files and migration to Swift/C++.
- Complete all stub implementations in Swift and connect to real logic.
- Keep this audit up to date as you refactor and extend the bridge.
