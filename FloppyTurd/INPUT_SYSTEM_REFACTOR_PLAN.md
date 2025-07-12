# Input System Refactor Plan

## Current Problem
- Input handling is scattered across multiple components (GameView, PlatformLayer, TouchControls)
- Inconsistent APIs and redundant forwarding of touch events
- Platform-specific input code mixed with game logic
- Multiple input sources causing confusion and potential bugs
- **CRITICAL**: We're migrating FROM RaylibCompat_iOS TO PlatformIOS, not the other way around!

## Target Architecture

### Layer 1: Game Code
- Calls PlatformAPI functions for input operations
- Doesn't know about platform-specific details
- Uses unified input interface (IsPrimaryInputDown, GetPrimaryInputPosition, etc.)

### Layer 2: PlatformAPI (Unified Interface)
- Provides platform-agnostic input function names
- Delegates to platform-specific implementations (PlatformIOS)
- Handles input state management and coordinate conversion

### Layer 3: PlatformIOS (Platform-Specific Implementation)
- Provides low-level platform-specific input functions
- Uses iOS touch events and gesture recognition
- Manages input state and coordinate systems
- **REPLACES RaylibCompat_iOS input handling**

### Layer 4: InputManager (Input State Management)
- Manages input state, gesture recognition, and input mapping
- Provides high-level input abstractions (jump, shoot, menu navigation)
- Handles input caching and lifecycle

## Implementation Plan

### Phase 1: Complete PlatformIOS Input Implementation ✅ COMPLETED
- [x] PlatformIOS has basic input function stubs
- [x] PlatformIOS delegates to PlatformLayer for now
- [ ] **COMPLETE**: Implement direct iOS touch handling in PlatformIOS
- [ ] **COMPLETE**: Remove dependency on PlatformLayer for input
- [ ] **COMPLETE**: Add gesture recognition to PlatformIOS

### Phase 2: PlatformAPI Input Functions ✅ COMPLETED
- [x] PlatformAPI provides unified input function names
- [x] PlatformAPI delegates to platform-specific implementations
- [x] PlatformAPI includes all necessary input functions

### Phase 3: Game Code Migration (IN PROGRESS)
- [ ] Update all game code to use PlatformAPI input functions
- [ ] Remove direct TouchControls usage
- [ ] Remove direct PlatformLayer input calls
- [ ] Remove RaylibCompat_iOS input calls
- [ ] Ensure consistent input handling across all game states

### Phase 4: Remove RaylibCompat_iOS Input Handling
- [ ] Remove input functions from RaylibCompat_iOS.mm
- [ ] Remove UpdateTouchState, ClearAllTouchStates from RaylibCompat_iOS
- [ ] Remove GetMousePosition, IsMouseButtonDown from RaylibCompat_iOS
- [ ] Ensure all input goes through PlatformAPI → PlatformIOS

### Phase 5: Cleanup and Optimization
- [ ] Remove redundant input forwarding
- [ ] Remove TouchControls class (functionality moved to InputManager)
- [ ] Clean up PlatformLayer input handling
- [ ] Optimize input performance
- [ ] Add input debugging and logging

## Migration Strategy

### **Step 1: Complete PlatformIOS Input Implementation**
```cpp
// PlatformIOS.cpp - Complete the input implementation
bool PlatformIOS::IsPrimaryInputDown() {
    // Get touch state directly from GameView, not PlatformLayer
    GameView* gameView = GetGameView();
    if (gameView) {
        return [gameView isPrimaryTouchDown];
    }
    return false;
}

Vector2 PlatformIOS::GetPrimaryInputPosition() {
    // Get touch position directly from GameView
    GameView* gameView = GetGameView();
    if (gameView) {
        CGPoint location = [gameView getPrimaryTouchLocation];
        return Vector2{(float)location.x, (float)location.y};
    }
    return {0, 0};
}
```

### **Step 2: Update Game Code**
Replace all input calls with PlatformAPI equivalents:

```cpp
// OLD: Direct TouchControls usage
if (touchControls->IsJumpPressed()) {
    player->Jump();
}

// NEW: PlatformAPI usage
if (PlatformAPI::IsPrimaryInputPressed()) {
    player->Jump();
}
```

### **Step 3: Update AIGUI System**
Replace AIGUI input handling with PlatformAPI:

```cpp
// OLD: Mixed input sources
bool touchActive = PlatformLayer::GetInstance().IsPrimaryInputDown();

// NEW: Unified PlatformAPI
bool touchActive = PlatformAPI::IsPrimaryInputDown();
```

### **Step 4: Remove RaylibCompat_iOS Input Functions**
Remove these functions from RaylibCompat_iOS.mm:
- `UpdateTouchState()`
- `ClearAllTouchStates()`
- `GetMousePosition()`
- `IsMouseButtonDown()`
- `IsMouseButtonPressed()`
- `IsMouseButtonReleased()`

### **Step 5: Remove TouchControls Class**
- Move gesture recognition to InputManager
- Move touch zone logic to PlatformIOS
- Remove TouchControls.h/cpp files

## Benefits of This Architecture

### **Unified Interface**
- Single source of truth for input operations
- Consistent API across all game code
- Platform-agnostic input handling

### **Proper Layering**
- Clear separation of concerns
- Platform-specific code isolated in PlatformIOS
- Easy to test and maintain

### **Performance**
- Reduced input forwarding overhead
- Efficient coordinate conversion
- Optimized gesture recognition

### **Maintainability**
- Centralized input logic
- Easy to add new input types
- Clear debugging and logging

### **Extensibility**
- Easy to add new platforms
- Support for new input devices
- Advanced input features (haptic feedback, etc.)

## Testing Strategy

### **Input Responsiveness**
- Test touch response time
- Verify gesture recognition accuracy
- Check coordinate conversion precision

### **Cross-Platform Compatibility**
- Test on different iOS devices
- Verify safe area handling
- Check orientation changes

### **Game Integration**
- Test all game states
- Verify input consistency
- Check for input conflicts

## Success Criteria

### **Performance**
- Input response time < 16ms (60 FPS)
- No input lag or missed inputs
- Smooth gesture recognition

### **Functionality**
- All existing input features work
- No regression in input behavior
- Consistent input across all game states

### **Code Quality**
- Clean, maintainable code
- Proper error handling
- Comprehensive logging and debugging

## Next Steps

1. **Complete Phase 1**: Finish PlatformIOS input implementation
2. **Test thoroughly**: Verify input functionality and performance
3. **Optimize**: Fine-tune input responsiveness and accuracy
4. **Document**: Update documentation with new input architecture
5. **Move to next system**: Begin refactoring file system 