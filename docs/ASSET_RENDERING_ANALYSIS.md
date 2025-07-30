# Asset Management and Rendering Pipeline Analysis

## Current Architecture Overview

### 1. Asset Loading Flow
```
C++ SpriteSystem → ThreadingSystem → AssetManager.swift → MetalRenderer.swift
```

### 2. Key Components

#### A. C++ Side (Engine)
- **SpriteSystem.cpp**: Creates sprites and requests texture loading
- **ThreadingSystem.cpp**: Bridges C++ to Swift via platform interface
- **iOSPlatformImpl.cpp**: Platform-specific implementation for iOS

#### B. Swift Side (iOS)
- **ThreadingSystem.swift**: Receives commands from C++ and delegates to AssetManager
- **AssetManager.swift**: Loads textures from iOS asset catalog
- **MetalRenderer.swift**: Renders textures using Metal

## Current Issues Identified

### 1. Texture Loading Pipeline Disconnect

**Problem**: AssetManager and MetalRenderer are separate systems that aren't properly connected.

**Current Flow**:
1. C++ requests texture via `loadTextureSync`
2. ThreadingSystem calls AssetManager.loadTextureSync
3. AssetManager loads texture and stores in `textureCache`
4. AssetManager returns texture handle (pointer to MTLTexture)
5. C++ receives handle but MetalRenderer doesn't know about it
6. MetalRenderer tries to render with unknown handle → fails

**Evidence from logs**:
```
SpriteSystem: Rendering sprite entity 1 with texture handle 992210096
MetalRenderer: drawSpriteScaled: Invalid sprite handle 992210096
```

### 2. Rotation and Rendering Issues

**Problem**: The sprite might be rotating incorrectly or being rendered off-screen.

**Current rotation code in LoadingState.cpp**:
```cpp
m_rotationAngle += 0.02f; // Increments every frame
if (m_rotationAngle > 2.0f * M_PI) {
    m_rotationAngle -= 2.0f * M_PI;
}
```

**Potential issues**:
- Rotation might be happening around wrong axis
- Sprite might be rendered outside visible area
- Z-ordering/layering issues

### 3. Asset Path Resolution

**Problem**: Inconsistent asset path handling between systems.

**Current state**:
- C++ uses: `"poophat"` (asset name only)
- AssetManager looks for: `"poophat.png"` in asset catalog
- MetalRenderer expects: Registered texture handle

## Detailed Component Analysis

### AssetManager.swift Analysis

**Current Implementation**:
```swift
public func loadTextureSync(name: String, extension: String = "png") -> UnsafeMutableRawPointer? {
    // Loads texture from asset catalog
    // Stores in textureCache
    // Returns pointer to MTLTexture
}
```

**Issues**:
1. Returns raw pointer to MTLTexture, not MetalRenderer handle
2. No connection to MetalRenderer's texture registry
3. Texture cache is separate from MetalRenderer's texture dictionary

### MetalRenderer.swift Analysis

**Current Implementation**:
```swift
private var textures: [UInt32: MTLTexture] = [:]
private var nextTextureHandle: UInt32 = 1

public func drawSpriteScaled(textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float) {
    if let texture = textures[textureHandle] {
        // Render texture
    } else {
        log("drawSpriteScaled: Invalid sprite handle \(textureHandle)", level: .warning)
    }
}
```

**Issues**:
1. Has its own texture registry separate from AssetManager
2. No method to register textures from AssetManager
3. Expects texture handles from its own registry

### ThreadingSystem.swift Analysis

**Current Implementation**:
```swift
private func loadTextureWithMetalRenderer(name: String, extension: String, callback: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
    do {
        let texture = try await assetManager.loadTexture(name: name, extension: extension)
        let handle = metalRenderer.registerTexture(texture)
        // Call C++ callback with handle
    } catch {
        // Handle error
    }
}
```

**Issues**:
1. Uses async `loadTexture` instead of sync `loadTextureSync`
2. Registers texture with MetalRenderer but C++ doesn't know about it
3. Callback mechanism might not be working correctly

## Proposed Solution Architecture

### 1. Unified Texture Management

**Goal**: Single source of truth for texture management.

**Proposed Flow**:
```
C++ SpriteSystem → ThreadingSystem → AssetManager → MetalRenderer → C++ Callback
```

**Implementation**:
1. AssetManager loads texture from asset catalog
2. AssetManager registers texture with MetalRenderer
3. MetalRenderer returns handle to AssetManager
4. AssetManager returns handle to C++ via callback
5. C++ uses handle for rendering

### 2. Fixed AssetManager.swift

```swift
public func loadTextureSync(name: String, extension: String = "png") -> UnsafeMutableRawPointer? {
    // 1. Load texture from asset catalog
    // 2. Register with MetalRenderer
    // 3. Return MetalRenderer handle (not texture pointer)
}
```

### 3. Enhanced MetalRenderer.swift

```swift
public func registerTexture(_ texture: MTLTexture) -> UInt32 {
    let handle = nextTextureHandle
    textures[handle] = texture
    nextTextureHandle += 1
    return handle
}
```

### 4. Fixed ThreadingSystem.swift

```swift
private func loadTextureWithMetalRenderer(name: String, extension: String, callback: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
    // Use sync loading and proper handle registration
}
```

## Debugging Steps

### 1. Verify Texture Loading
- Check if texture is actually loaded from asset catalog
- Verify texture handle is properly registered with MetalRenderer
- Confirm C++ receives correct handle

### 2. Verify Rendering
- Check sprite position and rotation
- Verify texture handle is valid in MetalRenderer
- Check if sprite is within visible area

### 3. Verify Asset Catalog
- Confirm "poophat" asset exists in asset catalog
- Check asset dimensions and format
- Verify asset is properly compiled into app bundle

## Next Steps

1. **Fix AssetManager-MetalRenderer connection**
2. **Implement proper texture handle flow**
3. **Debug sprite positioning and rotation**
4. **Verify asset catalog integration**
5. **Test end-to-end texture loading and rendering**

## Questions to Investigate

1. Is the "poophat" asset actually in the asset catalog?
2. What are the actual dimensions of the poophat texture?
3. Is the sprite being positioned within the visible screen area?
4. Is the rotation happening around the correct axis?
5. Are there any Z-ordering issues preventing visibility? 