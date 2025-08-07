# Dynamic System Refactor Plan - Eliminating Hardcoded Values

## Executive Summary

This plan outlines the complete refactoring of FloppyTurd's codebase to eliminate all hardcoded values and create a fully dynamic, platform-agnostic system. The primary goals are:

1. **Dynamic Screen Resolution Detection** - Using platform delegates to get actual device bounds
2. **Dynamic Texture Dimension Retrieval** - Loading texture metadata dynamically without fallbacks
3. **Platform-Specific Code Segregation** - Clear separation between iOS Metal and Desktop Raylib implementations
4. **Robust Configuration System** - Centralized configuration management
5. **Future-Proof Architecture** - Prepared for easy Raylib reintegration

## Current Hardcoded Issues Identified

### Screen Dimensions
- **iPhone 16 Portrait**: Hardcoded `1179×2556` pixels throughout codebase
- **iPhone 16 Logical**: Hardcoded `393×852` points in fallbacks
- **Desktop Default**: Hardcoded `800×600` in Raylib implementation
- **Mixed Usage**: Inconsistent use of pixels vs logical points

### Texture Dimensions
- **Background Textures**: Hardcoded sizes (Front: 2048×480, Clouds: 512×180, Others: 1024×480)
- **Sprite Textures**: Hardcoded fallbacks (coins: 16×16, enemies: 32×32, etc.)
- **UI Elements**: Hardcoded texture sizes with fallback values
- **Painting Textures**: Hardcoded 96×96 fallbacks in MainMenu

### Platform-Specific Code Issues
- **iOS-Specific Logic**: Embedded in supposedly platform-agnostic files
- **Metal Renderer**: iOS-specific calls in shared systems
- **Threading**: iOS-specific threading mixed with general logic
- **Asset Loading**: Platform-specific asset catalog calls in shared code

## Proposed Solution Architecture

### 1. Dynamic Screen Resolution System

#### 1.1 Enhanced Platform Delegates
```cpp
// Enhanced PlatformDelegates.h
struct ScreenInfo {
    float pixelWidth;        // Actual pixel dimensions
    float pixelHeight;
    float logicalWidth;      // Logical coordinate space
    float logicalHeight;
    float scaleFactor;       // Pixel to logical ratio
    bool isPortrait;
    std::string deviceModel; // For device-specific optimizations
};

struct PlatformDelegates {
    struct RendererDelegates {
        void (*getScreenInfo)(ScreenInfo* info);
        void (*getScreenSize)(float* width, float* height); // Legacy support
        // ... existing methods
    } renderer;
};
```

#### 1.2 iOS Implementation
```swift
// MetalRenderer.swift
public func getScreenInfo() -> ScreenInfo {
    let bounds = UIScreen.main.bounds
    let nativeBounds = UIScreen.main.nativeBounds
    let scale = UIScreen.main.scale
    
    return ScreenInfo(
        pixelWidth: Float(nativeBounds.width),
        pixelHeight: Float(nativeBounds.height),
        logicalWidth: Float(bounds.width),
        logicalHeight: Float(bounds.height),
        scaleFactor: Float(scale),
        isPortrait: nativeBounds.height > nativeBounds.width,
        deviceModel: UIDevice.current.model
    )
}
```

#### 1.3 Desktop/Raylib Implementation
```cpp
// RaylibRenderer.cpp
void GetScreenInfo(ScreenInfo* info) {
    info->pixelWidth = static_cast<float>(GetScreenWidth());
    info->pixelHeight = static_cast<float>(GetScreenHeight());
    info->logicalWidth = info->pixelWidth;   // 1:1 on desktop
    info->logicalHeight = info->pixelHeight;
    info->scaleFactor = 1.0f;
    info->isPortrait = info->pixelHeight > info->pixelWidth;
    info->deviceModel = "Desktop";
}
```

### 2. Dynamic Texture Dimension System

#### 2.1 Enhanced Texture Metadata
```cpp
// TextureManager.h
struct TextureMetadata {
    uint32_t handle;
    int width;
    int height;
    int channels;
    std::string format;
    size_t dataSize;
    bool isLoaded;
    std::string assetPath;
};

class TextureManager {
public:
    static TextureManager& Instance();
    
    // No fallbacks - must succeed or fail cleanly
    TextureMetadata GetTextureMetadata(const std::string& textureId);
    bool LoadTextureMetadata(const std::string& textureId);
    void CacheTextureMetadata(const std::string& textureId, const TextureMetadata& metadata);
    
private:
    std::unordered_map<std::string, TextureMetadata> m_metadataCache;
};
```

#### 2.2 iOS Texture Metadata Loading
```swift
// AssetManager.swift
public func getTextureMetadata(name: String) -> TextureMetadata? {
    guard let image = UIImage(named: name) else { return nil }
    
    let scale = image.scale
    let size = image.size
    let pixelWidth = Int(size.width * scale)
    let pixelHeight = Int(size.height * scale)
    
    return TextureMetadata(
        width: pixelWidth,
        height: pixelHeight,
        channels: 4, // RGBA
        format: "RGBA8",
        dataSize: pixelWidth * pixelHeight * 4
    )
}
```

#### 2.3 Platform-Agnostic Texture Loading Interface
```cpp
// Enhanced PlatformDelegates.h
struct PlatformDelegates {
    struct AssetDelegates {
        bool (*getTextureMetadata)(const char* textureId, TextureMetadata* metadata);
        void (*loadTexture)(const char* path, TextureLoadCallback callback, void* userData);
        // ... existing methods
    } asset;
};
```

### 3. Configuration Management System

#### 3.1 Dynamic Configuration Manager
```cpp
// ConfigManager.h
class ConfigManager {
public:
    static ConfigManager& Instance();
    
    void LoadConfiguration();
    void SaveConfiguration();
    
    // Screen configuration
    ScreenInfo GetCurrentScreenInfo() const { return m_screenInfo; }
    void UpdateScreenInfo();
    
    // Scaling configuration
    float GetUIScale() const;
    float GetTextScale() const;
    float GetSpriteScale() const;
    
    // Platform detection
    bool IsIOS() const { return m_platform == Platform::iOS; }
    bool IsDesktop() const { return m_platform == Platform::Desktop; }
    
private:
    enum class Platform { iOS, Desktop, Unknown };
    
    ScreenInfo m_screenInfo;
    Platform m_platform;
    std::unordered_map<std::string, float> m_scaleFactors;
};
```

### 4. Platform Code Segregation Strategy

#### 4.1 Preprocessor Definitions
```cpp
// Platform detection macros
#ifdef __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_IOS
        #define PLATFORM_IOS 1
        #define PLATFORM_DESKTOP 0
    #else
        #define PLATFORM_IOS 0
        #define PLATFORM_DESKTOP 1
    #endif
#else
    #define PLATFORM_IOS 0
    #define PLATFORM_DESKTOP 1
#endif
```

#### 4.2 Platform-Specific Headers
```cpp
// PlatformSpecific.h
#if PLATFORM_IOS
    #include "iOS/iOSRenderer.h"
    #include "iOS/iOSInputHandler.h"
    #include "iOS/iOSAudioHandler.h"
    using PlatformRenderer = iOSRenderer;
    using PlatformInputHandler = iOSInputHandler;
    using PlatformAudioHandler = iOSAudioHandler;
#elif PLATFORM_DESKTOP
    #include "Raylib/RaylibRenderer.h"
    #include "Raylib/RaylibInputHandler.h"
    #include "Raylib/RaylibAudioHandler.h"
    using PlatformRenderer = RaylibRenderer;
    using PlatformInputHandler = RaylibInputHandler;
    using PlatformAudioHandler = RaylibAudioHandler;
#endif
```

#### 4.3 Shared Interface Pattern
```cpp
// RendererInterface.h
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Initialize() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void DrawSprite(uint32_t textureHandle, float x, float y, float scaleX, float scaleY) = 0;
    virtual ScreenInfo GetScreenInfo() = 0;
};

// Platform factories
std::unique_ptr<IRenderer> CreatePlatformRenderer();
std::unique_ptr<IInputHandler> CreatePlatformInputHandler();
std::unique_ptr<IAudioHandler> CreatePlatformAudioHandler();
```

## Implementation Tasks

### Phase 1: Foundation Systems ✅ PLANNED

#### Task 1.1: Create Enhanced PlatformDelegates
- [ ] Add `ScreenInfo` struct with comprehensive screen data
- [ ] Add `TextureMetadata` struct for dynamic texture information
- [ ] Extend existing delegates with new metadata functions
- [ ] Maintain backward compatibility with existing `getScreenSize`

#### Task 1.2: Implement ConfigManager
- [ ] Create centralized configuration management
- [ ] Add platform detection logic
- [ ] Implement screen info caching and updating
- [ ] Add configuration file support for scaling factors

#### Task 1.3: Create TextureManager
- [ ] Implement dynamic texture metadata loading
- [ ] Add caching system for texture information
- [ ] Remove all hardcoded texture dimension fallbacks
- [ ] Add error handling for missing textures

### Phase 2: iOS Platform Updates ✅ PLANNED

#### Task 2.1: Enhanced iOS Screen Detection
- [ ] Update MetalRenderer to use UIScreen.main.nativeBounds
- [ ] Add device model detection for optimizations
- [ ] Implement orientation change handling
- [ ] Add scale factor management

#### Task 2.2: iOS Texture Metadata System
- [ ] Update AssetManager to provide texture metadata
- [ ] Add asset catalog metadata extraction
- [ ] Remove hardcoded iOS-specific texture sizes
- [ ] Implement proper error handling for missing assets

#### Task 2.3: iOS Threading Updates
- [ ] Update ThreadingSystem to use new delegates
- [ ] Remove hardcoded screen dimensions from Swift code
- [ ] Add proper metadata passing between threads

### Phase 3: Desktop/Raylib Platform Updates ✅ PLANNED

#### Task 3.1: Raylib Screen Detection
- [ ] Implement dynamic window size detection
- [ ] Add multi-monitor support
- [ ] Implement resolution change handling
- [ ] Add fullscreen mode support

#### Task 3.2: Raylib Texture System
- [ ] Implement Raylib texture metadata loading
- [ ] Add texture dimension caching
- [ ] Remove hardcoded desktop texture sizes
- [ ] Add file system-based texture loading

#### Task 3.3: Platform Segregation
- [ ] Move iOS-specific code out of shared systems
- [ ] Create proper platform abstraction layers
- [ ] Add compile-time platform selection
- [ ] Ensure clean Raylib integration paths

### Phase 4: Core Systems Refactoring ✅ PLANNED

#### Task 4.1: LevelManager Dynamic Updates
- [ ] Remove all hardcoded screen dimensions
- [ ] Implement dynamic background scaling
- [ ] Use TextureManager for background dimensions
- [ ] Add real-time resolution change support

#### Task 4.2: GameplayState Dynamic Updates
- [ ] Remove hardcoded screen and texture dimensions
- [ ] Implement dynamic UI scaling
- [ ] Use ConfigManager for scaling factors
- [ ] Add platform-agnostic rendering paths

#### Task 4.3: MainMenuState Dynamic Updates
- [ ] Remove hardcoded painting dimensions
- [ ] Implement dynamic UI element scaling
- [ ] Use platform-agnostic screen detection
- [ ] Add responsive layout system

#### Task 4.4: SpriteSystem Updates
- [ ] Remove hardcoded sprite dimensions
- [ ] Implement dynamic frame calculation
- [ ] Use TextureManager for all texture metadata
- [ ] Add runtime texture validation

### Phase 5: Testing and Validation ✅ PLANNED

#### Task 5.1: Multi-Device Testing
- [ ] Test on various iOS devices (iPhone SE, Pro, Max, iPad)
- [ ] Test on various desktop resolutions
- [ ] Validate orientation changes
- [ ] Test texture loading edge cases

#### Task 5.2: Performance Validation
- [ ] Benchmark dynamic vs hardcoded performance
- [ ] Optimize metadata caching
- [ ] Validate memory usage patterns
- [ ] Test startup time impact

#### Task 5.3: Robustness Testing
- [ ] Test missing texture handling
- [ ] Test invalid screen configurations
- [ ] Validate error recovery paths
- [ ] Test platform switching (future iOS desktop support)

## Files Requiring Updates

### Core Engine Files
- [ ] `src/Engine/Platform/PlatformDelegates.h` - Add new delegate structures
- [ ] `src/FloppyTurd/Systems/LevelManager.cpp` - Remove hardcoded dimensions
- [ ] `src/FloppyTurd/Systems/SpriteSystem.cpp` - Dynamic texture metadata
- [ ] `src/FloppyTurd/States/GameplayState.cpp` - Dynamic screen handling
- [ ] `src/FloppyTurd/States/MainMenuState.cpp` - Dynamic UI scaling

### iOS Platform Files
- [ ] `src/iOS/Rendering/MetalRenderer.swift` - Enhanced screen detection
- [ ] `src/iOS/Assets/AssetManager.swift` - Texture metadata extraction
- [ ] `src/iOS/Threading/ThreadingSystem.swift` - Remove hardcoded values
- [ ] `src/iOS/Threading/ThreadingProxy.cpp` - Updated delegate implementation
- [ ] `src/iOS/GameEngine.swift` - Dynamic coordinate conversion

### Desktop Platform Files
- [ ] `src/Raylib/Rendering/RaylibRenderer.cpp` - Dynamic screen detection
- [ ] `src/Engine/Platform/RaylibPlatformImpl.cpp` - Enhanced delegates
- [ ] `src/Engine/Platform/RaylibPlatformImpl.h` - New interface methods

### New Files to Create
- [ ] `src/Engine/Configuration/ConfigManager.h`
- [ ] `src/Engine/Configuration/ConfigManager.cpp`
- [ ] `src/Engine/Assets/TextureManager.h`
- [ ] `src/Engine/Assets/TextureManager.cpp`
- [ ] `src/Engine/Platform/PlatformFactory.h`
- [ ] `src/Engine/Platform/PlatformFactory.cpp`

## Benefits of This Refactor

### 1. Device Compatibility
- **Universal iOS Support**: Works on any current or future iOS device
- **Desktop Flexibility**: Supports any screen resolution or configuration
- **Orientation Support**: Proper handling of device rotations
- **Multi-Monitor**: Desktop support for various display configurations

### 2. Maintainability
- **No Hardcoded Values**: All dimensions retrieved dynamically
- **Platform Separation**: Clear iOS vs Desktop code boundaries
- **Future-Proof**: Easy to add new platforms or device types
- **Error Handling**: Robust fallback systems without hardcoded assumptions

### 3. Performance
- **Cached Metadata**: Efficient texture and screen information caching
- **Lazy Loading**: Load texture metadata only when needed
- **Platform Optimization**: Platform-specific optimizations without code duplication
- **Memory Efficiency**: No unnecessary texture dimension storage

### 4. Developer Experience
- **Single Configuration**: Centralized configuration management
- **Clear Abstractions**: Well-defined interfaces between platforms
- **Easy Testing**: Platform-agnostic test suites
- **Clean Separation**: iOS developers can focus on Metal, Desktop developers on Raylib

## Risk Mitigation

### 1. Performance Risks
- **Mitigation**: Comprehensive caching of frequently accessed metadata
- **Testing**: Benchmark all dynamic lookups vs previous hardcoded values
- **Fallback**: Keep performance-critical paths optimized

### 2. Compatibility Risks
- **Mitigation**: Extensive testing on various devices and configurations
- **Testing**: Automated tests for all supported device types
- **Fallback**: Graceful degradation for unsupported configurations

### 3. Complexity Risks
- **Mitigation**: Clean, well-documented interfaces and abstractions
- **Testing**: Unit tests for all configuration and metadata systems
- **Fallback**: Clear error messages and debugging information

## Implementation Timeline

### Week 1: Foundation (Phase 1)
- Enhanced PlatformDelegates and ConfigManager
- TextureManager implementation
- Basic platform detection

### Week 2: iOS Updates (Phase 2)
- iOS screen detection enhancements
- iOS texture metadata system
- Threading system updates

### Week 3: Desktop Updates (Phase 3)
- Raylib platform enhancements
- Desktop texture system
- Platform code segregation

### Week 4: Core Refactoring (Phase 4)
- LevelManager and GameplayState updates
- MainMenuState and SpriteSystem updates
- Integration testing

### Week 5: Testing and Polish (Phase 5)
- Multi-device testing
- Performance optimization
- Documentation and cleanup

## Conclusion

This refactor will transform FloppyTurd from a hardcoded, iOS-specific codebase into a truly dynamic, platform-agnostic game engine. The separation of iOS Metal and Desktop Raylib code will make future development much more maintainable, while the dynamic configuration system will ensure compatibility with any device or screen configuration.

The investment in this refactor will pay dividends in reduced maintenance burden, improved device compatibility, and cleaner architecture for future development.
