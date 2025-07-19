# IMPLEMENTATION PLAN: ABI-Safe Metal Texture Cache

## Overview
Implement Apple's recommended PIMPL pattern to eliminate ABI boundary violations in MetalTextureCache. This plan addresses the root cause of parameter corruption: NSString* in C++ headers.

## Pre-Implementation Analysis

### Current Problem
- **File**: MetalTextureCache.h contains NSString* in C++ header
- **Symptom**: "environment/Level1BackLayerBackground.png" → "ı§k^D^A" corruption
- **Root Cause**: ABI mismatch between C++ and Objective-C++ compilation units
- **Apple's Diagnosis**: "undefined behavior to access ownership-qualified object through lvalue of differently-qualified type"

### Solution Architecture
- **Pattern**: PIMPL (Pointer to Implementation) with opaque Objective-C++ implementation
- **Interface**: Pure C++ header with const char* parameters
- **Implementation**: Objective-C++ .mm file with full NSString/Metal support
- **Memory Management**: __bridge_retained for C++/Objective-C ownership transfer

## Step-by-Step Implementation

### Step 1: Backup Current Implementation
```bash
# Create backup of current files
cp MetalTextureCache.h MetalTextureCache.h.backup
cp MetalTextureCache.mm MetalTextureCache.mm.backup
```

### Step 2: Refactor Header (MetalTextureCache.h)
**Goal**: Remove all Objective-C types, use opaque implementation

```cpp
// MetalTextureCache.h - Pure C++ interface
#ifndef METAL_TEXTURE_CACHE_H
#define METAL_TEXTURE_CACHE_H

class MetalTextureCache {
public:
    MetalTextureCache();
    ~MetalTextureCache();
    
    // ABI-safe interface using C types only
    void* GetOrLoadTexture(const char* texturePath);
    void ClearCache();
    
private:
    void* m_impl;  // Opaque pointer to Objective-C++ implementation
    
    // Prevent copying (contains Objective-C objects)
    MetalTextureCache(const MetalTextureCache&) = delete;
    MetalTextureCache& operator=(const MetalTextureCache&) = delete;
};

#endif // METAL_TEXTURE_CACHE_H
```

### Step 3: Create Objective-C++ Implementation Interface
**Goal**: Define internal Objective-C class for actual implementation

```objc++
// MetalTextureCache.mm - Objective-C++ implementation
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#import "MetalTextureCache.h"

@interface MetalTextureCacheImpl : NSObject {
    @private
    NSMutableDictionary<NSString*, id<MTLTexture>>* _textureCache;
    id<MTLDevice> _device;
}

- (instancetype)initWithDevice:(id<MTLDevice>)device;
- (id<MTLTexture>)getOrLoadTexture:(NSString*)texturePath;
- (void)clearCache;

@end
```

### Step 4: Implement Objective-C++ Core Logic
**Goal**: Move all Metal/NSString operations to type-safe environment

```objc++
@implementation MetalTextureCacheImpl

- (instancetype)initWithDevice:(id<MTLDevice>)device {
    if (self = [super init]) {
        _device = device;
        _textureCache = [[NSMutableDictionary alloc] init];
    }
    return self;
}

- (id<MTLTexture>)getOrLoadTexture:(NSString*)texturePath {
    // Check cache first
    id<MTLTexture> cachedTexture = _textureCache[texturePath];
    if (cachedTexture) {
        return cachedTexture;
    }
    
    // Load texture using existing Metal loading logic
    id<MTLTexture> newTexture = [self loadTextureFromPath:texturePath];
    if (newTexture) {
        _textureCache[texturePath] = newTexture;
    }
    
    return newTexture;
}

- (void)clearCache {
    [_textureCache removeAllObjects];
}

// Private helper method - move existing texture loading code here
- (id<MTLTexture>)loadTextureFromPath:(NSString*)texturePath {
    // Move existing texture loading implementation here
    // This code was previously in the C++ portion
    // ...existing Metal texture loading logic...
}

@end
```

### Step 5: Implement C++ Wrapper with Bridge Casts
**Goal**: Provide ABI-safe C++ interface that delegates to Objective-C++

```cpp
// C++ wrapper implementation in MetalTextureCache.mm

MetalTextureCache::MetalTextureCache() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        MetalTextureCacheImpl* impl = [[MetalTextureCacheImpl alloc] initWithDevice:device];
        m_impl = (__bridge_retained void*)impl;  // Transfer ownership to C++
    }
}

MetalTextureCache::~MetalTextureCache() {
    if (m_impl) {
        CFRelease(m_impl);  // Release Objective-C object
        m_impl = nullptr;
    }
}

void* MetalTextureCache::GetOrLoadTexture(const char* texturePath) {
    if (!m_impl || !texturePath) return nullptr;
    
    @autoreleasepool {
        // Convert C string to NSString within safe Objective-C++ context
        NSString* nsPath = [NSString stringWithUTF8String:texturePath];
        if (!nsPath) return nullptr;
        
        // Call Objective-C++ implementation
        MetalTextureCacheImpl* impl = (__bridge MetalTextureCacheImpl*)m_impl;
        id<MTLTexture> texture = [impl getOrLoadTexture:nsPath];
        
        // Return opaque pointer - caller treats as void*
        return (__bridge void*)texture;
    }
}

void MetalTextureCache::ClearCache() {
    if (!m_impl) return;
    
    MetalTextureCacheImpl* impl = (__bridge MetalTextureCacheImpl*)m_impl;
    [impl clearCache];
}
```

### Step 6: Update Caller (PlatformTraitsIOS.mm)
**Goal**: Use C string interface consistently

```objc++
// PlatformTraitsIOS.mm - simplified caller
void* IOSTraits::LoadTexture(const char* path) {
    // Direct call with no string conversion needed here
    // String conversion happens safely within MetalTextureCache implementation
    return g_metalTextureCache->GetOrLoadTexture(path);
}
```

### Step 7: Error Handling and Validation
**Goal**: Add robust error handling for string conversion and Metal operations

```objc++
void* MetalTextureCache::GetOrLoadTexture(const char* texturePath) {
    @try {
        if (!m_impl || !texturePath) {
            NSLog(@"MetalTextureCache: Invalid parameters");
            return nullptr;
        }
        
        @autoreleasepool {
            NSString* nsPath = [NSString stringWithUTF8String:texturePath];
            if (!nsPath) {
                NSLog(@"MetalTextureCache: Failed to convert path to NSString: %s", texturePath);
                return nullptr;
            }
            
            MetalTextureCacheImpl* impl = (__bridge MetalTextureCacheImpl*)m_impl;
            id<MTLTexture> texture = [impl getOrLoadTexture:nsPath];
            
            if (!texture) {
                NSLog(@"MetalTextureCache: Failed to load texture: %@", nsPath);
                return nullptr;
            }
            
            return (__bridge void*)texture;
        }
    }
    @catch (NSException* exception) {
        NSLog(@"MetalTextureCache: Exception during texture loading: %@", exception.reason);
        return nullptr;
    }
}
```

## Validation Plan

### Step 8: Test ABI Safety
1. **Build Test**: Verify clean compilation with no ABI warnings
2. **String Integrity Test**: Log input/output strings to verify no corruption
3. **Memory Management Test**: Use Instruments to verify no leaks
4. **Crash Test**: Verify no crashes during texture loading

### Step 9: Integration Testing
1. **Load Test**: Load multiple textures and verify cache functionality
2. **Performance Test**: Measure texture loading performance vs. current implementation
3. **Memory Test**: Verify cache clearing works correctly

## Benefits of This Implementation

### ABI Safety ✅
- **No Objective-C types in C++ headers**
- **All string conversion within Objective-C++ compilation unit**
- **Opaque pointers eliminate type mismatch issues**

### Memory Management ✅
- **Clear ownership semantics with __bridge_retained**
- **ARC manages Objective-C object lifecycle**
- **RAII pattern for C++ resource management**

### Performance ✅
- **Single string conversion per texture load**
- **Efficient caching with NSMutableDictionary**
- **No ABI overhead in common paths**

### Maintainability ✅
- **Clean separation of C++ and Objective-C++ concerns**
- **Type safety within each compilation unit**  
- **Standard PIMPL pattern familiar to C++ developers**

This implementation follows Apple's documented ABI safety guidelines and eliminates the root cause of parameter corruption.
