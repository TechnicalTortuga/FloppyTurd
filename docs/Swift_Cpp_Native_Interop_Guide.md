# Swift/C++ Native Interoperability Guide
## Modern Approach for Floppy Turd iOS Development

*Created by Carl the Code-Conjuring Turdsmith*

---

## Overview

Swift 5.9+ and Xcode 15+ introduced **native bidirectional C++/Swift interoperability**, eliminating the need for Objective-C++ bridging layers. This guide covers everything you need to know for implementing clean, modern interop in Floppy Turd.

## Key Benefits

✅ **No Objective-C++ Required** - Direct Swift ↔ C++ communication  
✅ **Type Safety** - Swift compiler understands C++ types natively  
✅ **Performance** - Zero-overhead abstraction, no bridging costs  
✅ **Concurrency Safe** - Works seamlessly with Swift 6+ strict concurrency  
✅ **Maintainable** - Single source of truth, fewer files  

---

## Setup Requirements

### Minimum Versions
- **Swift**: 5.9+
- **Xcode**: 15.0+
- **iOS Deployment**: 13.0+ (no additional requirements for interop)
- **C++ Standard**: C++14 minimum, C++17/20 recommended

### Xcode Project Configuration

1. **Enable C++ Interoperability**
   ```
   Build Settings → Swift Compiler → Custom Flags
   Other Swift Flags: -cxx-interoperability-mode=default
   ```

2. **Set C++ Standard**
   ```
   Build Settings → Apple Clang - Language - C++
   C++ Language Dialect: C++17 (or C++20)
   ```

3. **Import Paths**
   ```
   Build Settings → Search Paths
   Import Paths: Add paths to your C++ headers
   ```

---

## Official Swift/C++ Interoperability Patterns

### 1. Importing C++ into Swift

**Setup**: Enable C++ interoperability and create module maps

```cpp
// C++ Header (GameEntity.h)
class GameEntity {
public:
    GameEntity(float x, float y);
    void update(float deltaTime);
    float getX() const { return m_x; }
    float getY() const { return m_y; }
private:
    float m_x, m_y;
};
```

```
// module.modulemap
module GameEntityModule {
    header "GameEntity.h"
    requires cplusplus
    export *
}
```

```swift
// Swift Usage - Direct C++ API calls
import GameEntityModule

let entity = GameEntity(10.0, 20.0)  // C++ constructor
entity.update(0.016)                 // C++ member function
let position = (entity.getX(), entity.getY())  // C++ getters
```

### 2. Exposing Swift APIs to C++

**Automatic Header Generation**: Swift compiler generates C++ headers

```swift
// Swift Class (SwiftGameManager.swift)
public class SwiftGameManager {
    public init() {}
    
    public func processInput(_ deltaTime: Float) -> Bool {
        // Swift implementation
        return true
    }
}
```

```cpp
// C++ Usage (uses auto-generated header)
#include "MyModule-Swift.h"

void gameLoop() {
    auto manager = MyModule::SwiftGameManager::init();
    bool result = manager.processInput(0.016f);
}
```

### 3. Working with C++ Containers

**Automatic Collection Conformance**: C++ containers become Swift collections

```cpp
// C++ API
std::vector<int> getScores();
void setScores(const std::vector<int>& scores);
```

```swift
// Swift Usage - std::vector becomes Swift Collection
let scores: [Int32] = getScores()  // Automatic conversion
for score in scores {              // Swift iteration
    print(score)
}
setScores([100, 200, 300])         // Swift array to std::vector
```

---

## Official Type Mapping

### Automatic Type Conversions

| C++ Type | Swift Type | Notes |
|----------|------------|-------|
| `bool` | `Bool` | Direct mapping |
| `int`, `int32_t` | `Int32` | Automatic conversion |
| `float` | `Float` | Direct mapping |
| `double` | `Double` | Direct mapping |
| `std::string` | `String` | **Supported in Swift 5.9+** |
| `std::vector<T>` | `[T]` | **Automatic collection conformance** |
| `std::span<T>` | Safe in Swift 6.2+ | **New safe interoperability mode** |
| C++ references | Unsafe by default | **Dependent lifetime handling** |
| C++ pointers | Unsafe by default | **Safe mode in Swift 6.2+** |

### Reference Types and Memory Safety

Swift 6.2 introduces **safe interoperability mode** for C++ pointers and view types:

- **Dependent References**: Member functions returning references are considered unsafe
- **View Types**: Structures containing references are treated as view types
- **Lifetime Management**: Swift assumes returned references depend on `this` object lifetime
- **Safety Annotations**: Use `SWIFT_RETURNS_INDEPENDENT_VALUE` for independent references

---

## Best Practices (Official Guidelines)

### 1. C++ API Design for Swift

✅ **DO**: Use standard C++ types (they map automatically)  
✅ **DO**: Design with value semantics when possible  
✅ **DO**: Use `std::string` and `std::vector` (fully supported)  
✅ **DO**: Annotate independent references with `SWIFT_RETURNS_INDEPENDENT_VALUE`  

❌ **DON'T**: Return dependent references without annotations  
❌ **DON'T**: Use C++ iterators in Swift (use range-based loops)  
❌ **DON'T**: Assume all C++ features are available in Swift  

### 2. Reference Type Mapping

```cpp
// C++ Class with reference semantics
class SWIFT_SHARED_REFERENCE(retain, release) GameEngine {
public:
    void update(float deltaTime);
    std::string_view getName() const SWIFT_RETURNS_INDEPENDENT_VALUE;
private:
    std::string m_name;
};
```

```swift
// Swift Usage - Automatic reference management
let engine = GameEngine()  // Shared reference type
engine.update(0.016)       // Direct method call
let name = engine.getName()  // Safe independent reference
// ARC handles cleanup automatically
```

### 3. Working with C++ Containers

```cpp
// C++ API with containers
class GameData {
public:
    std::vector<int> getScores() const;
    void setScores(const std::vector<int>& scores);
    std::map<std::string, int> getSettings() const;
};
```

```swift
// Swift Usage - Automatic collection conformance
let gameData = GameData()

// std::vector becomes Swift Array
let scores: [Int32] = gameData.getScores()
for score in scores {  // Swift iteration
    print("Score: \(score)")
}

// Swift Array to std::vector
gameData.setScores([100, 200, 300])

// std::map becomes Swift Dictionary
let settings = gameData.getSettings()
print(settings["difficulty"] ?? 0)
```

### 4. Safe Interoperability Mode (Swift 6.2+)

```cpp
// C++ with safe annotations
class DataProcessor {
public:
    // Unsafe by default - returns dependent reference
    const std::string& getCurrentData() const;
    
    // Safe with annotation - returns independent value
    SWIFT_RETURNS_INDEPENDENT_VALUE
    std::string_view getProcessorName() const;
    
    // Safe container access
    const std::vector<int>& getResults() const;
};
```

```swift
// Swift 6.2+ safe usage
let processor = DataProcessor()

// Unsafe method - renamed to emphasize danger
let data = processor.__getCurrentDataUnsafe()

// Safe method - normal name
let name = processor.getProcessorName()

// Container access - automatically safe
let results = processor.getResults()
```

---

## Module Map Configuration

### Standard C++ Module Map

```
// module.modulemap
module FloppyTurdCore {
    header "GameEngine.h"
    header "PlatformAPI.h"
    requires cplusplus
    export *
}
```

### Xcode Project Setup

```
// Build Settings
OTHER_SWIFT_FLAGS = "-cxx-interoperability-mode=default"
SWIFT_OBJC_INTERFACE_HEADER_NAME = "$(SWIFT_MODULE_NAME)-Swift.h"
```

```
// Package.swift
let package = Package(
    name: "FloppyTurd",
    platforms: [.iOS(.v13)],
    targets: [
        .target(
            name: "FloppyTurdCore",
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        )
    ]
)
```

---

## Common Pitfalls & Solutions

### 1. String Handling

❌ **Wrong (Old Approach)**:
```cpp
void old_function(const char* str); // Outdated C-style approach
```

✅ **Correct (Swift 5.9+ Official)**:
```cpp
void modern_function(const std::string& str); // std::string fully supported!
```

### 2. Object Lifetime

❌ **Wrong**:
```swift
func badExample() {
    let tempString = "Hello"
    tempString.withCString { ptr in
        // ptr becomes invalid after this block!
        someAsyncFunction(ptr)
    }
}
```

✅ **Correct**:
```swift
func goodExample() {
    let tempString = "Hello"
    tempString.withCString { ptr in
        // Use ptr synchronously only
        someSyncFunction(ptr)
    }
}
```

### 3. Reference Type Safety

❌ **Wrong (Unsafe References)**:
```cpp
class DataAPI {
public:
    // Unsafe - returns dependent reference
    const std::string& getCurrentData() const;
};
```

✅ **Correct (Safe with Annotations)**:
```cpp
class DataAPI {
public:
    // Safe - annotated independent value
    SWIFT_RETURNS_INDEPENDENT_VALUE
    std::string getCurrentData() const;
    
    // Safe - containers are automatically handled
    std::vector<std::string> getAllData() const;
};
```

---

## Performance Considerations

### 1. Minimize Boundary Crossings

```cpp
// ❌ Bad - Multiple calls
for (int i = 0; i < 1000; ++i) {
    swift_process_item(i);
}

// ✅ Good - Batch processing
swift_process_items(items, 1000);
```

### 2. Leverage Automatic Type Mapping

```cpp
// ✅ Excellent - Direct C++ types map automatically
struct Point2D {
    float x, y;
};
void update_position(Point2D position);           // Value type
void update_positions(std::vector<Point2D> points); // Container type

// ✅ Also Good - Reference types with proper annotations
SWIFT_RETURNS_INDEPENDENT_VALUE
Point2D get_player_position() const;
```

### 3. Use Native Swift Collections

```swift
// ✅ Excellent - Direct Swift Array to std::vector
func processData() {
    let data: [UInt8] = Array(repeating: 0, count: 1024)
    cpp_process_buffer(data)  // Automatic conversion!
}

// ✅ Also Good - std::string support
func processText() {
    let message = "Hello from Swift!"
    cpp_process_string(message)  // Direct std::string conversion
}
```

---

## Debugging Tips

### 1. Enable Verbose Logging

```bash
# Xcode build settings
OTHER_SWIFT_FLAGS = -cxx-interoperability-mode=default -v
```

### 2. Check Symbol Visibility

```bash
# Verify exported symbols
nm -D your_binary | grep your_function
```

### 3. Use Static Analysis

```bash
# Enable all warnings
WARNING_CFLAGS = -Wall -Wextra -Wpedantic
```

---

## Migration Strategy

### Phase 1: Enable Swift 5.9+ Interop
1. Update Xcode project settings for C++ interoperability
2. Create module maps for existing C++ headers
3. Remove `@_cdecl` and `extern "C"` patterns

### Phase 2: Modernize C++ APIs
1. Use `std::string` and `std::vector` directly
2. Add `SWIFT_RETURNS_INDEPENDENT_VALUE` annotations
3. Design with value semantics when possible

### Phase 3: Update Swift Code
1. Remove manual C-style bridging
2. Use direct C++ API calls
3. Leverage automatic type conversions

### Phase 4: Clean Up
1. Remove unnecessary bridge files
2. Update build configurations
3. Test with Swift 6.2+ safe interoperability mode

---

## Conclusion

Swift 5.9+ native C++ interop is a game-changer for iOS development. By following these patterns and best practices, you can create clean, efficient, and maintainable code that leverages the best of both Swift and C++.

**Remember**: Keep interfaces simple, manage memory carefully, and always test across the language boundary!

---

*This guide is specifically tailored for Floppy Turd development. For the latest official documentation, visit [Swift.org C++ Interoperability](https://www.swift.org/documentation/cxx-interop/).*