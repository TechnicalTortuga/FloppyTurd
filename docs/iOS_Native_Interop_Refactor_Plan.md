# iOS Native Interop Refactor Plan

> **Note:** This plan focuses on the clean `src/` directory structure. The old codebase has been removed from the workspace to prevent confusion.

## Complete Migration to Swift 5.9+ Official C++ Interoperability

*Created by Carl the Code-Conjuring Turdsmith*

---

## Executive Summary

This plan outlines the complete refactoring of Floppy Turd's iOS systems to use **official Swift 5.9+ C++ interoperability** instead of the current mix of `@_cdecl`, `extern "C"`, and manual bridging patterns. The goal is to achieve direct, type-safe, zero-overhead communication between C++ and Swift.

**IMPORTANT**: Based on the current UML architecture in `/Users/aimac/Development/FloppyTurd/GnosisUML.md`, we ARE using PlatformInterfaces with abstract base classes:
- `Renderer` (abstract) → `MetalRenderer` (Swift implementation)
- `InputHandler` (abstract) → `TouchInputHandler` (Swift implementation)  
- `AudioHandler` (abstract) → `AVAudioHandler` (Swift implementation)

The Engine takes `unique_ptr<PlatformInterfaces>` containing these implementations.

### Current State Analysis

**Problems with Current Implementation:**
1. **Inconsistent Patterns**: Mix of `@_cdecl`, `@objc`, and manual bridging
2. **Outdated Approaches**: Using undocumented `@_cdecl` instead of official Swift 5.9+ features
3. **Performance Overhead**: Manual memory management and type conversions
4. **Maintenance Burden**: Multiple bridge files and C-style interfaces
5. **Type Safety Issues**: Manual pointer casting and unsafe operations

**Control Flow Divergence Point:**

### Platform Entry Points (src/ directory):
- **iOS Entry Point**: `src/iOS/main.swift` - Creates iOS platform implementations
- **Desktop Entry Point**: `src/Desktop/main.cpp` - Creates Raylib platform implementations

### Platform Implementations (src/ directory):
- `src/iOS/MetalRenderer.swift` - iOS rendering implementation
- `src/iOS/TouchInputHandler.swift` - iOS input implementation  
- `src/iOS/AVAudioHandler.swift` - iOS audio implementation
- `src/Desktop/RaylibRenderer.cpp` - Desktop rendering implementation
- `src/Desktop/RaylibInputHandler.cpp` - Desktop input implementation
- `src/Desktop/RaylibAudioHandler.cpp` - Desktop audio implementation

### Abstract Interfaces (src/Engine/):
- `src/Engine/Renderer.h` - Abstract renderer interface
- `src/Engine/InputHandler.h` - Abstract input interface
- `src/Engine/AudioHandler.h` - Abstract audio interface
- `src/Engine/PlatformInterfaces.h` - Container struct

---

## Implementation Strategy

### Architecture Based on GnosisUML.md
We will **FOLLOW** the clean UML architecture with proper control flow divergence:
- ✅ Keep abstract base classes: `Renderer`, `InputHandler`, `AudioHandler` (in src/Engine/)
- ✅ Keep `PlatformInterfaces` struct as the divergence point
- ✅ Engine constructor: `Engine(platform: unique_ptr<PlatformInterfaces>)`
- ✅ Platform-specific entry points decide which implementations to instantiate:
  - **iOS Entry Point**: Creates `MetalRenderer`, `TouchInputHandler`, `AVAudioHandler`
  - **Desktop Entry Point**: Creates `RaylibRenderer`, `RaylibInputHandler`, `RaylibAudioHandler`
- ✅ Use Swift 5.9+ native C++ interop for iOS implementations
- ✅ Zero bridging overhead with automatic type conversion

This maintains clean separation while leveraging modern interop capabilities.

## Phase 1: Foundation Setup

### 1.1 Xcode Project Configuration

**Update Build Settings:**
```bash
# Add to Xcode project build settings
OTHER_SWIFT_FLAGS = "-cxx-interoperability-mode=default"
SWIFT_OBJC_INTERFACE_HEADER_NAME = "FloppyTurd-Swift.h"
CLANG_CXX_LANGUAGE_STANDARD = "c++17"
```

**Update CMakeLists.txt:**
```cmake
# Enable C++ interoperability for Swift
set_target_properties(FloppyTurd PROPERTIES
    SWIFT_OBJC_INTERFACE_HEADER_NAME "FloppyTurd-Swift.h"
    SWIFT_CXX_INTEROPERABILITY_MODE "default"
)
```

### 1.2 Module Map Creation ✅ COMPLETED

**Created `src/Engine/module.modulemap`:**
```
module GnosisEngine {
    header "Core/GNLog.h"
    header "Game/Game.h"
    header "Game/Player.h"
    header "Game/Level.h"
    header "Game/GameState.h"
    header "Game/GameConfig.h"
    requires cplusplus
    export *
}
```

**Updated `FloppyTurd/module.modulemap`:**
```
module FloppyTurd {
    header "Game.h"
    header "Player.h"
    header "Level.h"
    requires cplusplus
    export *
}
```

**Benefits:**
- Exports all C++ game classes directly to Swift
- Includes `requires cplusplus` for Swift 5.9+ interop
- Covers all game entities, systems, and utilities
- Eliminates need for PlatformInterfaces abstraction

---

## Phase 2: Platform Entry Points Setup

### 2.1 Create iOS Entry Point ✅ PLANNED
```swift
// src/iOS/AppDelegate.swift
import UIKit
import GnosisEngine

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?
    private var gameEngine: Engine?
    
    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        
        // Create platform implementations
        let metalRenderer = MetalRenderer()
        let touchInput = TouchInputHandler()
        let avAudio = AVAudioHandler()
        
        // Package into PlatformInterfaces
        let platformInterfaces = PlatformInterfaces(
            renderer: metalRenderer,
            inputHandler: touchInput,
            audioHandler: avAudio
        )
        
        // Initialize game engine with platform implementations
        gameEngine = Engine(platform: platformInterfaces)
        
        // Setup window and view controller
        window = UIWindow(frame: UIScreen.main.bounds)
        let gameViewController = GameViewController(engine: gameEngine!)
        window?.rootViewController = gameViewController
        window?.makeKeyAndVisible()
        
        return true
    }
}
```

### 2.2 Create Desktop Entry Point ✅ PLANNED
```cpp
// src/Desktop/main.cpp
#include "Engine/Engine.h"
#include "Engine/PlatformInterfaces.h"
#include "Desktop/RaylibRenderer.h"
#include "Desktop/RaylibInputHandler.h"
#include "Desktop/RaylibAudioHandler.h"

int main() {
    // Create desktop platform implementations
    auto renderer = std::make_unique<RaylibRenderer>();
    auto inputHandler = std::make_unique<RaylibInputHandler>();
    auto audioHandler = std::make_unique<RaylibAudioHandler>();
    
    // Package into PlatformInterfaces
    auto platformInterfaces = std::make_unique<PlatformInterfaces>();
    platformInterfaces->renderer = std::move(renderer);
    platformInterfaces->inputHandler = std::move(inputHandler);
    platformInterfaces->audioHandler = std::move(audioHandler);
    
    // Initialize game engine with platform implementations
    Engine gameEngine(std::move(platformInterfaces));
    gameEngine.run();
    
    return 0;
}
```

---

## Phase 3: iOS Platform Implementations

### 3.1 Create MetalRenderer.swift ✅ PLANNED
```swift
// src/iOS/MetalRenderer.swift
import Metal
import MetalKit
import GnosisEngine

public class MetalRenderer: Renderer {
    private var device: MTLDevice
    private var commandQueue: MTLCommandQueue
    private var renderPipelineState: MTLRenderPipelineState
    
    public init() {
        // Initialize Metal resources
    }
    
    public func submitBatch(commands: [DrawCommand]) {
        // Implement batch rendering
    }
    
    public func beginFrame() {
        // Start frame rendering
    }
    
    public func endFrame() {
        // End frame rendering
    }
    
    public func clearScreen() {
        // Clear screen
    }
    
    public func present() {
        // Present frame
    }
}
```

### 3.2 Create TouchInputHandler.swift ✅ PLANNED
```swift
// src/iOS/TouchInputHandler.swift
import UIKit
import GnosisEngine

public class TouchInputHandler: InputHandler {
    private var activeTouches: Set<UITouch> = []
    private var currentActions: Set<InputAction> = []
    
    public func pollEvents() {
        // Process touch events
    }
    
    public func isActionPressed(action: InputAction) -> Bool {
        return currentActions.contains(action)
    }
    
    public func getMousePosition() -> GNVector2 {
        // Return touch position as mouse equivalent
    }
    
    public func isKeyPressed(key: Int32) -> Bool {
        // Map touch gestures to key equivalents
        return false
    }
}
```

### 3.3 Create AVAudioHandler.swift ✅ PLANNED
```swift
// src/iOS/AVAudioHandler.swift
import AVFoundation
import GnosisEngine

public class AVAudioHandler: AudioHandler {
    private var audioEngine: AVAudioEngine
    private var playerNodes: [GNAudioHandle: AVAudioPlayerNode] = [:]
    
    public func playSound(handle: GNAudioHandle) {
        // Play sound effect
    }
    
    public func playMusic(handle: GNAudioHandle) {
        // Play background music
    }
    
    public func stopAudio(handle: GNAudioHandle) {
        // Stop audio
    }
    
    public func setVolume(volume: Float) {
        // Set master volume
    }
    
    public func pauseAll() {
        // Pause all audio
    }
    
    public func resumeAll() {
        // Resume all audio
    }
    
    public func isMusic(handle: GNAudioHandle) -> Bool {
        // Check if handle is music
    }
    
    public func loopAudio(handle: GNAudioHandle, loop: Bool) {
        // Set audio looping
    }
}
```

---

## Phase 4: Desktop Platform Implementations

### 4.1 Create RaylibRenderer.cpp ✅ PLANNED
```cpp
// src/Desktop/RaylibRenderer.cpp
#include "RaylibRenderer.h"
#include <raylib.h>

void RaylibRenderer::submitBatch(const std::vector<DrawCommand>& commands) {
    for (const auto& cmd : commands) {
        // Implement Raylib rendering
    }
}

void RaylibRenderer::beginFrame() {
    BeginDrawing();
}

void RaylibRenderer::endFrame() {
    EndDrawing();
}

void RaylibRenderer::clearScreen() {
    ClearBackground(BLACK);
}

void RaylibRenderer::present() {
    // Raylib handles presentation automatically
}
```

### 4.2 Create RaylibInputHandler.cpp ✅ PLANNED
```cpp
// src/Desktop/RaylibInputHandler.cpp
#include "RaylibInputHandler.h"
#include <raylib.h>

void RaylibInputHandler::pollEvents() {
    // Raylib handles events automatically
}

bool RaylibInputHandler::isActionPressed(InputAction action) {
    switch (action) {
        case InputAction::JUMP:
            return IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        case InputAction::SHOOT:
            return IsKeyPressed(KEY_X) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
        // ... other actions
    }
    return false;
}

GNVector2 RaylibInputHandler::getMousePosition() {
    Vector2 pos = GetMousePosition();
    return {pos.x, pos.y};
}

bool RaylibInputHandler::isKeyPressed(int key) {
    return IsKeyPressed(key);
}
```

### 4.3 Create RaylibAudioHandler.cpp ✅ PLANNED
```cpp
// src/Desktop/RaylibAudioHandler.cpp
#include "RaylibAudioHandler.h"
#include <raylib.h>

void RaylibAudioHandler::playSound(GNAudioHandle handle) {
    // Play sound using Raylib
}

void RaylibAudioHandler::playMusic(GNAudioHandle handle) {
    // Play music using Raylib
}

// ... implement other audio methods
```

---

## Phase 5: Engine Integration

### 5.1 Update Engine Constructor
```cpp
// src/Engine/Engine.cpp
Engine::Engine(std::unique_ptr<PlatformInterfaces> platform)
    : m_platform(std::move(platform)) {
    // Initialize engine with platform implementations
}

void Engine::render() {
    m_platform->renderer->beginFrame();
    m_platform->renderer->clearScreen();
    
    // Render game objects
    for (const auto& cmd : m_renderCommands) {
        m_platform->renderer->submitBatch({cmd});
    }
    
    m_platform->renderer->endFrame();
    m_platform->renderer->present();
}

void Engine::processInput() {
    m_platform->inputHandler->pollEvents();
    
    if (m_platform->inputHandler->isActionPressed(InputAction::JUMP)) {
        // Handle jump
    }
}
```

### 5.2 Platform Interface Updates
```cpp
// src/Engine/PlatformInterfaces.h
struct PlatformInterfaces {
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputHandler> inputHandler;
    std::unique_ptr<AudioHandler> audioHandler;
    
    PlatformInterfaces() = default;
    PlatformInterfaces(std::unique_ptr<Renderer> r, 
                      std::unique_ptr<InputHandler> i,
                      std::unique_ptr<AudioHandler> a)
        : renderer(std::move(r))
        , inputHandler(std::move(i))
        , audioHandler(std::move(a)) {}
};
```

---

## Phase 6: Build System Integration

### 6.1 CMake Updates

**Add to main CMakeLists.txt:**

```cmake
# Enable Swift/C++ interoperability
if(APPLE)
    set_target_properties(FloppyTurd PROPERTIES
        SWIFT_OBJC_INTERFACE_HEADER_NAME "FloppyTurd-Swift.h"
        SWIFT_CXX_INTEROPERABILITY_MODE "default"
    )
    
    # Ensure Swift header is generated before C++ compilation
    add_dependencies(FloppyTurd_cpp FloppyTurd_swift)
endif()
```

### 6.2 Xcode Project Updates

**Build Phases:**
1. Swift Compilation (generates `FloppyTurd-Swift.h`)
2. C++ Compilation (uses generated header)
3. Linking

**Build Settings:**
```
SWIFT_OBJC_INTERFACE_HEADER_NAME = FloppyTurd-Swift.h
OTHER_SWIFT_FLAGS = -cxx-interoperability-mode=default
CLANG_CXX_LANGUAGE_STANDARD = c++17
```

---

## Phase 7: Testing and Validation

### 7.1 Incremental Testing Strategy

1. **Logging First**: Test new logging system in isolation
2. **Game Engine**: Test basic initialization and lifecycle
3. **Rendering**: Test Metal renderer integration
4. **Input**: Test touch input handling
5. **Full Integration**: Test complete game loop

### 7.2 Validation Checklist

- [ ] No `@_cdecl` functions remain
- [ ] No `extern "C"` bridge functions
- [ ] No manual pointer casting
- [ ] Direct Swift class instantiation works
- [ ] Automatic type conversion works (String, Array, etc.)
- [ ] Performance is equal or better
- [ ] Memory management is automatic (ARC)
- [ ] Build system generates Swift headers correctly

---

## Implementation Timeline

### Week 1: Foundation
- [ ] Update build settings and module maps
- [ ] Test basic Swift/C++ interop with simple example
- [ ] Verify auto-generated headers work

### Week 2: Logging System
- [ ] Refactor logging to direct Swift integration
- [ ] Remove old bridge files
- [ ] Test logging across C++/Swift boundary

### Week 3: Core Systems
- [ ] Refactor GameEngine to direct class usage
- [ ] Update MetalRenderer integration
- [ ] Modernize TouchInputHandler

### Week 4: Integration & Testing
- [ ] Full system integration testing
- [ ] Performance validation
- [ ] Documentation updates

---

## Benefits of New Approach

### Performance
- **Zero Bridging Overhead**: Direct function calls
- **Automatic Type Conversion**: No manual string/array copying
- **ARC Integration**: Automatic memory management

### Maintainability
- **Single Source of Truth**: No duplicate bridge functions
- **Type Safety**: Compiler-enforced type checking
- **Modern Swift**: Uses official, documented features

### Developer Experience
- **IntelliSense Support**: Full IDE integration
- **Debugging**: Native debugging across languages
- **Future-Proof**: Uses official Apple-supported features

---

## Risk Mitigation

### Potential Issues
1. **Build Complexity**: Auto-generated headers dependency
2. **Debugging**: Cross-language debugging setup
3. **Performance**: Verify no regressions

### Mitigation Strategies
1. **Incremental Migration**: One system at a time
2. **Comprehensive Testing**: Automated test suite
3. **Rollback Plan**: Keep old code in branches until validated

---

## Conclusion

This refactor will modernize Floppy Turd's iOS integration to use official Swift 5.9+ C++ interoperability, eliminating technical debt and improving performance, maintainability, and developer experience. The direct class usage approach provides the cleanest, most efficient integration possible.

**Next Steps:**
1. Start with Phase 1 (Foundation Setup)
2. Implement Phase 2 (Logging System) as proof of concept
3. Proceed incrementally through remaining phases
4. Validate each phase before proceeding

*Let's polish this turd to a mirror shine with modern, official Swift/C++ interop!* 🚀