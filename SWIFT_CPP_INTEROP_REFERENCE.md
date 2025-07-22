# Swift C++ Interoperability Reference for Floppy Turd

## 🚨 CRITICAL REMINDER: WE USE NATIVE C++ INTEROP, NOT C-STYLE BRIDGING

**This document serves as a permanent reference to prevent reverting to outdated C-style interop patterns.**

---

## Our Architecture: Modern Swift 5.9+ C++ Interoperability

### ✅ What We ARE Using (Correct Approach)

- **Native Swift 5.9+ C++ Interoperability**
- **Direct C++ class instantiation in Swift**: `let engine = CppGameEngine()`
- **Bidirectional interop**: C++ ↔ Swift without manual bridging
- **Automatic type bridging** by Swift compiler
- **Bridging headers** for C++ class exposure
- **Swift annotations** for enhanced interop (`SWIFT_SHARED_REFERENCE`, etc.)

### ❌ What We Are NOT Using (Outdated Approach)

- ~~`@_cdecl` wrapper functions~~
- ~~`void*` pointer casting~~
- ~~Manual C-style bridging~~
- ~~`extern "C"` function wrappers~~
- ~~`UnsafeMutableRawPointer` conversions~~

---

## Project Configuration

### Xcode Build Settings
```
C++ and Objective-C Interoperability: C++/Objective-C++
C++ Language Dialect: C++17 (or newer)
C++ Standard Library: libc++
```

### Required Files
- **FloppyTurd-Bridging-Header.h**: Includes C++ headers for Swift
- **module.modulemap**: Defines C++ modules (if needed)
- **GameEngine-Swift.h**: Auto-generated Swift→C++ interface

---

## Code Patterns

### ✅ Correct: Direct C++ Usage in Swift
```swift
// Direct C++ class instantiation
let gameEngine = CppGameEngine()
gameEngine.initialize()

// Direct method calls
let screenWidth = gameEngine.getScreenWidth()
gameEngine.setTargetFPS(60)

// C++ containers as Swift collections
for texture in gameEngine.getLoadedTextures() {
    // Process texture
}
```

### ✅ Correct: Swift Usage in C++
```cpp
#include "GameEngine-Swift.h"

// Direct Swift class usage
auto swiftView = GameEngine::SwiftUIView::init();
swiftView.present(viewController);
```

### ❌ Incorrect: Manual C-Style Bridging
```swift
// DON'T DO THIS!
let bridge = getCppInteropBridge() // Returns void*
bridge_initializeEngine(bridge)    // Manual wrapper
```

---

## Current Architecture Components

### Core C++ Game Engine
- **Game.h/cpp**: Main game logic
- **PlatformAPI.h**: Platform abstraction (should expose C++ classes directly)
- **Raylib Integration**: C++ game engine core

### Swift iOS Layer
- **GameEngine.swift**: Swift wrapper around C++ engine
- **UIFrameworkSwift.swift**: iOS-specific UI components
- **CppInteropBridgeSwift.swift**: ⚠️ **LEGACY - Should be refactored to native interop**

### Interop Interface
- **FloppyTurd-Bridging-Header.h**: C++ headers exposed to Swift
- **Auto-generated headers**: Swift compiler creates C++ bindings

---

## Refactoring Guidelines

### When You See These Patterns, STOP and Refactor:

1. **`@_cdecl` functions** → Use direct C++ class methods
2. **`void*` returns** → Use proper C++ class references
3. **Manual bridging** → Let Swift compiler handle it
4. **C-style wrappers** → Expose C++ classes directly

### Refactoring Steps:

1. **Identify C++ classes** that need Swift access
2. **Add to bridging header**: `#include "YourCppClass.h"`
3. **Use Swift annotations** for better integration:
   ```cpp
   struct SWIFT_SHARED_REFERENCE(retain, release) GameEngine {
       SWIFT_COMPUTED_PROPERTY int getScreenWidth() const;
       void setTargetFPS(int fps);
   };
   ```
4. **Remove manual bridging code**
5. **Update Swift code** to use C++ classes directly

---

## Swift Annotations for C++ Classes

### Memory Management
```cpp
// Shared reference type (reference counted)
struct SWIFT_SHARED_REFERENCE(retain, release) GameEngine;

// Immortal reference (never deallocated)
struct SWIFT_IMMORTAL_REFERENCE GameSingleton;
```

### Properties
```cpp
// Computed properties
int getValue() const SWIFT_COMPUTED_PROPERTY;
void setValue(int value);

// Independent return values
SWIFT_RETURNS_INDEPENDENT_VALUE
std::string_view getName() const;
```

### Self-Contained Types
```cpp
// Safe to copy/move
struct SWIFT_SELF_CONTAINED Vector2 {
    float x, y;
};
```

---

## Common Mistakes to Avoid

### ❌ Creating Unnecessary Wrappers
```swift
// DON'T create manual wrappers like this:
func bridge_initializeEngine(_ ptr: UnsafeMutableRawPointer) {
    // Manual bridging code
}
```

### ✅ Use Direct Interop Instead
```swift
// DO use direct C++ classes:
let engine = CppGameEngine()
engine.initialize()
```

### ❌ Void Pointer Casting
```cpp
// DON'T return void pointers:
void* getCppInteropBridge() {
    return static_cast<void*>(&bridge);
}
```

### ✅ Return Proper References
```cpp
// DO return proper C++ references:
CppGameEngine& getGameEngine() {
    return CppGameEngine::getInstance();
}
```

---

## Performance Benefits of Native Interop

- **Zero-cost abstractions**: No manual bridging overhead
- **Type safety**: Compile-time type checking
- **Automatic memory management**: Swift handles C++ object lifetimes
- **Optimized calls**: Direct function calls without wrapper layers

---

## Debugging Tips

### Enable C++ Interop Debugging
```bash
# Add to build flags
-Xfrontend -enable-cxx-interop
-Xcc -std=c++17
```

### Common Build Errors
- **"Cannot find type in scope"**: Add to bridging header
- **"Unsupported C++ feature"**: Check Swift C++ interop status page
- **Memory issues**: Use proper Swift annotations

---

## Resources

- [Swift.org C++ Interoperability Guide](https://www.swift.org/documentation/cxx-interop/)
- [WWDC 2023: Mix Swift and C++](https://developer.apple.com/videos/play/wwdc2023/10172/)
- [Swift C++ Interop Status](https://www.swift.org/documentation/cxx-interop/status/)

---

## 🎯 Action Items for Floppy Turd

1. **Refactor CppInteropBridgeSwift.swift** to use native interop
2. **Update PlatformAPI.h** to expose C++ classes directly
3. **Remove all `@_cdecl` wrapper functions**
4. **Add proper Swift annotations** to C++ classes
5. **Update Swift code** to use C++ classes directly

---

**Remember: We're building a modern Swift/C++ hybrid, not a legacy C bridge!**

*This document should be referenced whenever working on interop code to maintain consistency and prevent regression to outdated patterns.*