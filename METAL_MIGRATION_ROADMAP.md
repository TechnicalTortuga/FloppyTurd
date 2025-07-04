# Metal/SpriteKit Migration & iOS Platform Abstraction Roadmap

## Overview
This document outlines the step-by-step plan for migrating Floppy Turd from SDL2 to Metal/SpriteKit on iOS, while maintaining Raylib support for all other platforms. The migration will use Objective-C++ wrappers to ensure compatibility with our C++ codebase and leverage the existing RaylibCompat abstraction layer.

## Key Principles
- **Platform Conditional**: Metal/SpriteKit only on iOS, Raylib everywhere else
- **C++ Compatibility**: Use Objective-C++ wrappers to interface with C++ code
- **Minimal Disruption**: Leverage existing RaylibCompat interface
- **AIGUI Preservation**: Maintain and enhance our custom immediate mode GUI system [[memory:823465]]

---

## Phase 1: Architecture & Setup

### 1.1 Create MetalRaylibCompat Layer
- [ ] Create `MetalRaylibCompat.h` with same interface as `RaylibCompat.h`
- [ ] Create `MetalRaylibCompat.mm` (Objective-C++) implementation
- [ ] Design Metal rendering context management
- [ ] Setup MTKView integration with UIKit

### 1.2 Objective-C++ Wrapper Design
- [ ] Create `MetalRenderer.h/.mm` for Metal pipeline management
- [ ] Create `MetalTexture.h/.mm` for texture management
- [ ] Create `MetalShader.h/.mm` for shader management
- [ ] Create `SpriteKitBridge.h/.mm` for SpriteKit features

### 1.3 Build System Updates
- [ ] Update CMakeLists.txt to conditionally use Metal/SpriteKit on iOS
- [ ] Remove SDL2 dependencies for iOS builds
- [ ] Add Metal and SpriteKit framework linking
- [ ] Configure Objective-C++ compilation flags

### 1.4 Remove SDL Dependencies
- [ ] Remove SDL includes from iOS codepaths
- [ ] Update iOS-specific code in main.cpp
- [ ] Clean up SDL initialization/shutdown code
- [ ] Remove SDL2 from iOS build dependencies

---

## Phase 2: Core Rendering System

### 2.1 Window & View Management
- [ ] Implement `InitWindow()` using UIWindow/MTKView
- [ ] Implement `CloseWindow()` with proper cleanup
- [ ] Handle window state (fullscreen, orientation)
- [ ] Implement framebuffer management

### 2.2 Metal Renderer Implementation  
- [ ] Create command buffer management
- [ ] Implement render pass descriptors
- [ ] Setup vertex/fragment shader pipeline
- [ ] Implement texture sampling and binding

### 2.3 Drawing Commands
- [ ] Implement `BeginDrawing()`/`EndDrawing()` with Metal
- [ ] Implement `ClearBackground()` 
- [ ] Create vertex buffer management for primitives
- [ ] Implement batch rendering for performance

### 2.4 Coordinate System
- [ ] Handle Metal's NDC coordinate system
- [ ] Implement projection matrix for 2D rendering
- [ ] Ensure compatibility with existing game coordinates

---

## Phase 3: Texture & Image Management

### 3.1 Texture Loading
- [ ] Implement `LoadTexture()` using MTLTexture
- [ ] Implement `LoadTextureFromImage()` 
- [ ] Handle texture formats (RGBA, etc.)
- [ ] Implement texture caching system

### 3.2 Texture Drawing
- [ ] Implement `DrawTexture()` variants
- [ ] Implement `DrawTextureEx()` with rotation/scale
- [ ] Implement `DrawTexturePro()` with source/dest rects
- [ ] Handle texture tinting and alpha blending

### 3.3 Image Processing
- [ ] Implement `LoadImage()` using UIImage/CGImage
- [ ] Implement `GenImageColor()`
- [ ] Implement `ImageResize()` using Core Graphics
- [ ] Handle image format conversions

---

## Phase 4: Primitive Rendering

### 4.1 Shape Drawing
- [ ] Implement `DrawRectangle()` variants
- [ ] Implement `DrawRectangleRounded()` for AIGUI [[memory:823465]]
- [ ] Implement `DrawCircle()` and `DrawCircleV()`
- [ ] Implement `DrawLine()` 

### 4.2 Batching & Performance
- [ ] Create vertex buffer batching system
- [ ] Optimize draw call submission
- [ ] Implement instanced rendering where applicable
- [ ] Profile and optimize rendering pipeline

---

## Phase 5: Text Rendering

### 5.1 Font Management
- [ ] Implement `LoadFont()` using Core Text
- [ ] Implement `LoadFontEx()` with size options
- [ ] Create font atlas generation
- [ ] Handle font caching

### 5.2 Text Drawing
- [ ] Implement `DrawText()` and `DrawTextEx()`
- [ ] Implement `MeasureText()` and `MeasureTextEx()`
- [ ] Support text scaling and spacing
- [ ] Ensure AIGUI text rendering works [[memory:823465]]

---

## Phase 6: Input System

### 6.1 Touch Input
- [ ] Replace SDL touch with UITouch events
- [ ] Implement multi-touch support
- [ ] Map touch to mouse compatibility layer
- [ ] Handle touch begin/move/end states

### 6.2 Gesture Recognition
- [ ] Implement tap gestures
- [ ] Implement swipe gestures
- [ ] Implement pinch/zoom gestures
- [ ] Implement long press detection

### 6.3 Platform Layer Updates
- [ ] Update `PlatformLayer::GetTouchPoints()`
- [ ] Update `TouchControls` class for native touch
- [ ] Implement haptic feedback using UIKit

---

## Phase 7: Audio System

### 7.1 Audio Device Management
- [ ] Replace SDL_mixer with AVAudioEngine
- [ ] Implement `InitAudioDevice()`/`CloseAudioDevice()`
- [ ] Setup audio session configuration
- [ ] Handle audio interruptions

### 7.2 Sound Effects
- [ ] Implement `LoadSound()` using AVAudioPlayer
- [ ] Implement `PlaySound()` with mixing support
- [ ] Implement `SetSoundVolume()`
- [ ] Handle multiple simultaneous sounds

### 7.3 Music Streaming
- [ ] Implement `LoadMusicStream()` 
- [ ] Implement music playback controls
- [ ] Support background audio
- [ ] Handle audio format conversions

---

## Phase 8: Platform-Specific Features

### 8.1 iOS Integration
- [ ] Handle safe area for notched devices
- [ ] Implement orientation changes
- [ ] Support iOS app lifecycle events
- [ ] Handle memory warnings

### 8.2 Performance Features
- [ ] Implement Metal Performance Shaders where applicable
- [ ] Use texture compression (PVRTC/ASTC)
- [ ] Implement dynamic resolution scaling
- [ ] Profile with Instruments

### 8.3 SpriteKit Integration (Optional)
- [ ] Evaluate SpriteKit for particle effects
- [ ] Consider SKPhysicsBody for physics
- [ ] Assess SKAction for animations
- [ ] Determine hybrid rendering approach

---

## Phase 9: AIGUI Enhancement [[memory:823465]]

### 9.1 Touch Optimization
- [ ] Update AIGUI for native touch input
- [ ] Implement touch-friendly UI controls
- [ ] Add gesture support to AIGUI widgets
- [ ] Optimize hit testing for touch

### 9.2 Resolution Independence  
- [ ] Update AIGUI scaling for different devices
- [ ] Support dynamic type sizes
- [ ] Handle safe area in UI layout
- [ ] Test on various iOS devices

---

## Phase 10: Testing & Deployment

### 10.1 Component Testing
- [ ] Test each rendering function
- [ ] Verify texture loading/drawing
- [ ] Test audio playback
- [ ] Validate input handling

### 10.2 Integration Testing
- [ ] Test full game loop
- [ ] Verify AIGUI functionality [[memory:823465]]
- [ ] Test all game levels
- [ ] Performance benchmarking

### 10.3 Device Testing
- [ ] Test on iPhone (various models)
- [ ] Test on iPad
- [ ] Test different iOS versions
- [ ] Memory usage profiling

### 10.4 Xcode Project Setup
- [ ] Create proper Xcode project
- [ ] Configure signing and provisioning
- [ ] Setup asset catalogs
- [ ] Configure Info.plist

---

## File Migration Checklist

### Core Files (Highest Priority)
- [ ] FloppyTurd/RaylibCompat.cpp → MetalRaylibCompat.mm
- [ ] FloppyTurd/RaylibCompat.h (add iOS conditionals)
- [ ] FloppyTurd/main.cpp (iOS initialization)
- [ ] FloppyTurd/Window.cpp (Metal view setup)
- [ ] FloppyTurd/PlatformLayer.cpp (iOS native features)

### Rendering Files
- [ ] FloppyTurd/AIGUI.cpp (ensure Metal compatibility) [[memory:823465]]
- [ ] FloppyTurd/TextureAtlas.cpp
- [ ] FloppyTurd/Sprite.cpp
- [ ] FloppyTurd/AnimatedLayer.cpp
- [ ] FloppyTurd/ParallaxLayer.cpp
- [ ] All other rendering-related files

### Input Files
- [ ] FloppyTurd/TouchControls.cpp
- [ ] Input handling in Game.cpp

### Audio Files  
- [ ] FloppyTurd/AudioManager.cpp
- [ ] FloppyTurd/SoundManager.cpp
- [ ] FloppyTurd/AudioClip.cpp

### Resource Management
- [ ] FloppyTurd/ResourceManager.cpp
- [ ] FloppyTurd/TextureCache.cpp

---

## Implementation Notes

### Metal Specifics
- Use MTLPixelFormatBGRA8Unorm for textures
- Implement triple buffering for smooth rendering
- Use Metal 2 features where available
- Consider MetalKit helpers for common tasks

### Objective-C++ Guidelines
- Keep C++ interfaces in headers
- Implement Metal code in .mm files
- Use `__bridge` for Core Foundation conversions
- Minimize Objective-C exposure to C++ code

### Performance Considerations
- Batch draw calls aggressively
- Use texture atlases
- Implement frustum culling
- Profile with Metal System Trace

### Debugging Tools
- Use Metal Frame Capture
- Enable Metal API Validation
- Use Xcode Memory Graph
- Profile with Instruments

---

**This roadmap prioritizes maintaining compatibility with the existing C++ codebase while leveraging Apple's native technologies for optimal iOS performance.** 