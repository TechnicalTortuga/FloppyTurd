# ABI Boundary Analysis: String Parameter Corruption

## Current Problem

The fundamental issue is that string parameters are getting corrupted when passed between compilation units in the iOS Metal texture loading system.

### Evidence from Logs

```
[IOSTraits] LoadTexture: ENTRY POINT - fileName: environment/Level1BackLayerBackground.png
[IOSTraits] LoadTexture: Using NSString interface to avoid ABI boundary issues
[IOSTraits] LoadTexture: Calling MetalTextureCache with NSString interface
[MetalTextureCache] NSString fileName: ı§k^D^A
[MetalTextureCache] std::string conversion: ∞¥¬
```

**Key Observation**: The string `environment/Level1BackLayerBackground.png` becomes `ı§k^D^A` when it reaches MetalTextureCache.

## Analysis of Attempted Solutions

### 1. Initial std::string Approach (FAILED)
- **Method**: Pass `std::string` by reference across compilation units
- **Result**: Parameter corruption
- **Reason**: C++ ABI instability across Objective-C++ compilation units

### 2. const char* Approach (FAILED)
- **Method**: Pass `const char*` across compilation units
- **Result**: Parameter corruption
- **Reason**: Still crossing ABI boundaries unsafely

### 3. NSString* Interface Approach (FAILED)
- **Method**: Convert to NSString in PlatformTraitsIOS.mm, pass NSString* to MetalTextureCache
- **Result**: NSString itself gets corrupted (`environment/Level1BackLayerBackground.png` → `ı§k^D^A`)
- **Reason**: NSString* declared in C++ header causes ABI issues

## Root Cause Analysis

### The Real Problem
The issue is NOT just about string types. The problem is about **how parameters are passed across compilation unit boundaries** when those compilation units have different compilation contexts.

### Compilation Context Mismatch
- **PlatformTraitsIOS.mm**: Compiled as Objective-C++ (.mm)
- **MetalTextureCache.h**: Included in both C++ and Objective-C++ contexts
- **MetalTextureCache.mm**: Compiled as Objective-C++ (.mm)

### ABI Boundary Issues
When `NSString*` is declared in a C++ header:
1. C++ compilation units see it as an undefined pointer type
2. Objective-C++ compilation units see it as a proper NSString pointer
3. This creates ABI mismatches when calling across compilation boundaries

## Current Architecture Issues

### File Structure Analysis
```
PlatformTraitsIOS.mm (Obj-C++)
    ↓ calls
MetalTextureCache::GetOrLoadTexture(NSString* fileName)
    ↓ declared in
MetalTextureCache.h (C++ header with NSString*)
    ↓ implemented in
MetalTextureCache.mm (Obj-C++)
```

### The Fundamental ABI Problem
- NSString* in C++ header creates undefined behavior
- Parameter passing becomes unreliable
- Memory layout mismatches cause corruption

## What We Need to Understand

### 1. Why are ALL approaches failing?
- Every string-passing method we've tried fails
- This suggests the problem is deeper than string type choice
- The issue is likely in the interface design itself

### 2. What is the actual calling convention?
- How are parameters actually passed between .mm files?
- What does the compiler generate for these calls?
- Are we dealing with name mangling issues?

### 3. What is the proper iOS/Metal pattern?
- How do professional iOS apps handle this?
- What is Apple's recommended approach?
- Are we missing fundamental iOS development patterns?

## Questions That Need Answers

1. **Interface Design**: Should we eliminate C++ headers with Objective-C types entirely?

2. **Compilation Strategy**: Should we separate pure C++ and pure Objective-C++ interfaces?

3. **Parameter Passing**: Should we use C-compatible interfaces only?

4. **Architecture Pattern**: Should we use a different design pattern entirely?

## Potential Solutions to Investigate

### Option A: Pure C Interface
- Use only C-compatible types in headers
- Handle all Objective-C conversions internally
- Avoid any Objective-C types in cross-compilation-unit interfaces

### Option B: Separate Interface Layers
- Create a pure C++ interface layer
- Create a separate Objective-C++ implementation layer
- Use only POD types for communication

### Option C: Complete Architecture Redesign
- Move texture loading entirely into a single compilation unit
- Eliminate cross-compilation-unit calls entirely
- Use callbacks or delegates for communication

## Next Steps (DO NOT IMPLEMENT YET)

1. **Research Phase**: Investigate Apple's recommended patterns
2. **Analysis Phase**: Understand the exact ABI mechanism
3. **Design Phase**: Choose the most robust solution
4. **Implementation Phase**: Implement with full understanding

## Critical Insight

The problem is NOT about finding the "right" string type. The problem is about **designing ABI-safe interfaces** for iOS development with mixed C++/Objective-C++ compilation units.

We need to solve the interface design problem, not just the string passing problem.
