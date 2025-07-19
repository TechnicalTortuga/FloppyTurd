# MINIMAL ABI FIX: Header/Implementation Signature Alignment

## Problem Identified
The issue is **NOT** the architecture - it's a simple header/implementation mismatch:

- **Header declares**: `Texture2D GetOrLoadTexture(const char* fileName);`
- **Implementation has**: `Texture2D MetalTextureCache::GetOrLoadTexture(NSString* fileName)`
- **Caller converts**: `const char*` → `NSString*` → passes `NSString*` to a function expecting `const char*`

This creates ABI corruption because the caller and implementation disagree on parameter types.

## Minimal Fix (No Architecture Changes)

### Option 1: Align Implementation to Header (Recommended)
Keep the C++ interface, do NSString conversion **inside** the .mm file:

```objc++
// MetalTextureCache.mm - Update implementation to match header
Texture2D MetalTextureCache::GetOrLoadTexture(const char* fileName) {
    // Convert to NSString safely within the .mm file
    NSString* nsFileName = [NSString stringWithUTF8String:fileName];
    if (!nsFileName) {
        TraceLog(LOG_ERROR, "[ERROR] GetOrLoadTexture: Invalid UTF-8 string: %s", fileName);
        return CreateFallbackTexture();
    }
    
    // Rest of existing logic using nsFileName...
    std::string fileNameStr = [nsFileName UTF8String];
    // ... existing implementation unchanged
}
```

### Option 2: Align Header to Implementation 
Update header to match the NSString implementation:

```cpp
// MetalTextureCache.h - Add conditional compilation
class MetalTextureCache {
public:
#ifdef __OBJC__
    Texture2D GetOrLoadTexture(NSString* fileName);
#else
    Texture2D GetOrLoadTexture(const char* fileName);
#endif
    // ... rest unchanged
};
```

## Why This Works (No Performance Loss)

### Current Hot Path:
1. `const char*` → `NSString*` conversion in PlatformTraitsIOS.mm
2. Pass `NSString*` to method expecting `const char*` → **ABI CORRUPTION**
3. String gets corrupted during parameter passing

### Fixed Hot Path (Option 1):
1. `const char*` passed directly to MetalTextureCache
2. `const char*` → `NSString*` conversion **inside** MetalTextureCache.mm
3. **No ABI boundary crossing with Objective-C types**

### Performance Impact: **ZERO**
- Same number of string conversions
- Same cache lookup logic  
- Same Metal API calls
- **No virtual calls, no PIMPL overhead, no indirection**

## Implementation (5-Minute Fix)

Just update the MetalTextureCache.mm implementation:

```objc++
// Change this line in MetalTextureCache.mm:
Texture2D MetalTextureCache::GetOrLoadTexture(NSString* fileName) {

// To this:
Texture2D MetalTextureCache::GetOrLoadTexture(const char* fileName) {
    NSString* nsFileName = [NSString stringWithUTF8String:fileName];
    if (!nsFileName) {
        TraceLog(LOG_ERROR, "[ERROR] Invalid UTF-8 string: %s", fileName);
        return CreateFallbackTexture();
    }
    
    // Replace all uses of 'fileName' with 'nsFileName' in the rest of the function
    // The rest of the logic stays exactly the same
```

And update the caller to pass `const char*` directly:

```objc++
// PlatformTraitsIOS.mm - remove NSString conversion
Texture2D IOSTraits::LoadTexture(const char* fileName) {
    MetalTextureCache& cache = MetalTextureCache::GetInstance();
    return cache.GetOrLoadTexture(fileName);  // Pass const char* directly
}
```

## Why This is Better Than PIMPL

1. **Zero Performance Impact**: No indirection, virtual calls, or heap allocations
2. **Minimal Code Changes**: Just fix the signature mismatch
3. **Maintains Hot Path**: Direct access to cache and Metal APIs
4. **ABI Safe**: All Objective-C operations confined to .mm file
5. **No Architecture Changes**: Keeps current singleton design

This fixes the root cause (ABI boundary violation) with minimal surgery while preserving all performance characteristics.
