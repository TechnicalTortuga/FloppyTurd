# Floppy Turd Mobile Refactoring Roadmap

## Overview
This document outlines the refactoring plan for porting Floppy Turd to mobile platforms (Android & iOS) while maintaining desktop compatibility.

## ✅ Completed
1. **CMake Build System** - Cross-platform build configuration
2. **Platform Abstraction Layer** - `PlatformLayer.h/cpp` for platform-specific code
3. **Touch Controls System** - `TouchControls.h/cpp` for mobile input
4. **Build Documentation** - `BUILD.md` with platform-specific instructions

## 🚧 High Priority Refactoring

### 1. Input System Integration ✅ COMPLETED
- [x] Modify `Player.cpp` to use `PlatformLayer` for input
- [x] Update `MainMenu.cpp` to support touch navigation
- [x] Integrate `TouchControls` into `Playing.cpp`
- [x] Update `AIGUI` to use platform-agnostic input

### 2. Resource Management 🔄 IN PROGRESS  
- [x] Create `ResourceManager` class with modern caching system
- [x] Implement platform-specific resource loading with quality tiers
- [x] Add intelligent LRU cache and memory management
- [x] Create hybrid compatibility system (`ResourceCompat.h`)
- [ ] **Next Phase**: Gradually migrate components from `Resources.h` to `ResourceManager`
- [ ] **Future**: Convert large textures to mobile-friendly formats

### 3. Screen Adaptation
- [ ] Update `Window.cpp` to handle mobile screen sizes
- [ ] Implement safe area handling for notched devices
- [ ] Add orientation lock support
- [ ] Scale UI elements based on screen density

### 4. Performance Optimization
- [ ] Profile and optimize particle effects
- [ ] Implement texture atlasing for sprites
- [ ] Add quality settings for different device tiers
- [ ] Optimize audio loading (streaming vs preload)

## 📱 Mobile-Specific Features

### 1. Touch Gestures
- [ ] Swipe up/down for menu navigation
- [ ] Pinch to zoom (for menus/credits)
- [ ] Long press for alternate actions
- [ ] Haptic feedback for collisions/actions

### 2. Platform Integration
- [ ] Game Center/Google Play Games integration
- [ ] Cloud save support
- [ ] Achievement system adaptation
- [ ] In-app purchases for cosmetics (optional)

### 3. Mobile UI/UX
- [ ] Larger touch targets for buttons
- [ ] Touch-friendly pause menu
- [ ] Mobile-optimized settings screen
- [ ] Tutorial for touch controls

## 🛠️ Code Quality Improvements

### 1. Memory Management
- [ ] Replace raw pointers with smart pointers
- [ ] Fix memory leaks in level transitions
- [ ] Implement proper RAII for resources
- [ ] Add memory pooling for projectiles/particles

### 2. Architecture Improvements
- [ ] Implement Entity Component System (ECS) for game objects
- [ ] Create proper State Machine for game states
- [ ] Separate rendering from game logic
- [ ] Add dependency injection for better testing

### 3. Error Handling
- [ ] Add proper exception handling
- [ ] Implement logging system with levels
- [ ] Add crash reporting for mobile
- [ ] Graceful fallbacks for missing resources

## 📊 Specific File Refactoring

### High Impact Files:
1. **Game.cpp/h**
   - Remove platform-specific code
   - Use PlatformLayer for window creation
   - Add mobile lifecycle handling (pause/resume)

2. **Player.cpp/h**
   - Abstract input handling
   - Add touch-specific controls
   - Optimize collision detection

3. **Resources.h**
   - Convert to ResourceManager class
   - Add platform-specific path resolution
   - Implement async loading

4. **Window.cpp/h**
   - Handle multiple screen sizes
   - Add orientation support
   - Implement safe area handling

5. **MainMenu.cpp**
   - Touch-friendly button sizes
   - Swipe navigation support
   - Mobile-optimized layout

## 🔧 Build System Enhancements

### Android
- [ ] Generate APK signing configuration
- [ ] Add ProGuard rules
- [ ] Configure different build flavors
- [ ] Set up CI/CD with GitHub Actions

### iOS
- [ ] Create Info.plist template
- [ ] Configure app icons and launch screens
- [ ] Set up provisioning profiles
- [ ] Handle App Store requirements

## 📅 Implementation Timeline

### Phase 1 (Weeks 1-2): Core Systems
- Input system integration
- Resource management refactoring
- Basic touch controls

### Phase 2 (Weeks 3-4): Mobile Adaptation
- Screen handling
- Mobile UI/UX
- Performance optimization

### Phase 3 (Weeks 5-6): Platform Features
- Platform-specific integrations
- Testing and debugging
- Performance profiling

### Phase 4 (Week 7-8): Polish
- Bug fixes
- Final optimizations
- Store preparation

## 🎮 Gameplay Improvements

### Mobile-Specific Gameplay
- [ ] Auto-shoot option for easier mobile play
- [ ] Difficulty adjustment for touch controls
- [ ] Mobile-exclusive power-ups
- [ ] Daily challenges system

### General Improvements
- [ ] Better collision feedback
- [ ] Smoother difficulty curve
- [ ] More varied enemy patterns
- [ ] Boss fight improvements

## 📝 Notes for Implementation

1. **Maintain Desktop Compatibility** - All changes should work on both mobile and desktop
2. **Test Early and Often** - Set up device testing lab
3. **Performance First** - Mobile devices have limited resources
4. **Battery Life** - Optimize for power efficiency
5. **Network Awareness** - Handle offline mode gracefully

## 🚀 Next Steps

1. Start with input system integration
2. Test basic functionality on Android/iOS
3. Iterate based on performance metrics
4. Gather user feedback through beta testing

This roadmap is a living document and should be updated as development progresses. 