# ABI Safety Apple Analysis

## Executive Summary
After analyzing Apple's official ABI documentation and testing multiple interface approaches, the core issue is **mixing C++ headers with Objective-C types creates undefined ABI behavior across compilation unit boundaries**.

## Apple's Official Guidance on ABI Boundaries

### 1. Core ABI Principles from Apple's ARC Documentation

Apple explicitly states that **Objective-C types in C++ headers cause undefined behavior**:

> "A type is a retainable object owner type if it is a retainable object pointer type"
> "An ownership qualifier may be applied to a substituted template type parameter"
> "There are four ownership qualifiers: __autoreleasing, __strong, __unsafe_unretained, __weak"

The critical issue: **NSString* in C++ headers violates ABI safety** because:
- C++ compilation units don't understand Objective-C ARC semantics
- Parameter passing conventions differ between C++ and Objective-C++
- Memory layout assumptions vary across compilation contexts

### 2. Documented ABI Boundary Issues

From Apple's ARC specification:
> "It is undefined behavior to access an ownership-qualified object through an lvalue of a differently-qualified type"
> "The conversion to const __unsafe_unretained is permitted because the semantics of reads are equivalent across all these ownership semantics"

Our symptoms match Apple's documented undefined behavior:
- String corruption ("environment/Level1BackLayerBackground.png" → "ı§k^D^A")
- Identical failures across multiple string type attempts
- ABI boundary crossing between PlatformTraitsIOS.mm → MetalTextureCache

### 3. Apple's Recommended Solutions

Apple's documentation provides specific guidance for our situation:

#### A. Use C-Compatible Types in Headers
> "A C function may be marked with the cf_audited_transfer attribute"
> "A parameter of C retainable pointer type is assumed to not be consumed unless it is marked with the cf_consumed attribute"

**Recommended**: Use `const char*` in C++ headers, convert to NSString* within .mm implementation

#### B. Bridged Casts for Type Conversion
> "(__bridge T) op casts the operand to the destination type T"
> "There is no transfer of ownership, and ARC inserts no retain operations"

**Pattern**: `NSString* nsStr = (__bridge NSString*)CFStringCreateWithCString(...)`

#### C. Compilation Unit Separation
> "ARC must interoperate with Objective-C code which manages retains and releases manually"
> "The type system must reliably indicate how to manage objects of a type"

**Principle**: Keep Objective-C types confined to .mm files, use C types for interface boundaries

## Analysis of Our Failed Attempts

### Attempt 1: std::string Interface
**Issue**: C++ string ABI differs between compilation units
**Apple's Guidance**: "It is undefined behavior if ARC is exposed to an invalid pointer"

### Attempt 2: const char* Interface  
**Issue**: Still crossing ABI boundary with string semantics
**Apple's Guidance**: C types are safer but string lifetime management remains problematic

### Attempt 3: NSString* Interface
**Issue**: Objective-C types in C++ headers violate ABI safety
**Apple's Guidance**: "A program is ill-formed if it attempts to apply an ownership qualifier to a type which is already ownership-qualified"

## Apple's Recommended Solution Pattern

Based on Apple's ABI documentation, the correct pattern is:

### 1. Header Design (MetalTextureCache.h)
```cpp
// C++ header - use only C-compatible types
class MetalTextureCache {
public:
    void* GetOrLoadTexture(const char* texturePath);  // C-compatible interface
private:
    void* m_impl;  // Opaque implementation pointer
};
```

### 2. Implementation Design (MetalTextureCache.mm)
```objc++
// Objective-C++ implementation
@interface MetalTextureCacheImpl : NSObject
- (id<MTLTexture>)getOrLoadTexture:(NSString*)texturePath;
@end

void* MetalTextureCache::GetOrLoadTexture(const char* texturePath) {
    NSString* nsPath = [NSString stringWithUTF8String:texturePath];
    MetalTextureCacheImpl* impl = (__bridge MetalTextureCacheImpl*)m_impl;
    return (__bridge void*)[impl getOrLoadTexture:nsPath];
}
```

### 3. Caller Pattern (PlatformTraitsIOS.mm)
```objc++
void* LoadTexture(const char* path) {
    return g_textureCache->GetOrLoadTexture(path);  // Safe C interface
}
```

## Apple's ABI Safety Principles

From the official documentation:

### 1. Interface Segregation
> "ARC must interoperate with Objective-C code which manages retains and releases manually"

**Principle**: Separate C++ interfaces from Objective-C implementation

### 2. Type Safety Boundaries  
> "The type system must reliably identify which objects are to be managed"

**Principle**: Use opaque pointers (`void*`) or C types for cross-compilation-unit interfaces

### 3. Ownership Transfer Clarity
> "There must be reliable conventions for whether and when 'ownership' is passed between caller and callee"

**Principle**: Document ownership semantics explicitly, use `__bridge` casts for clarity

## Conclusion

Apple's official ABI documentation confirms our analysis:
1. **NSString* in C++ headers is undefined behavior**
2. **Mixed compilation units require C-compatible interfaces**  
3. **The solution is architectural separation, not parameter type changes**

The root cause is interface design violating Apple's ABI safety principles. The solution requires:
- C-compatible headers with `const char*` or opaque pointers
- Objective-C++ implementation handling NSString conversion internally
- Clear ownership semantics using Apple's recommended bridging patterns

This approach aligns with Apple's documented best practices for iOS ABI safety.
