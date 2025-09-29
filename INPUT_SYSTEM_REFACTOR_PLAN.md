# Input System Refactor Plan & Landscape Touch Fix

## Executive Summary

The current input system has multiple issues:
1. **Landscape Touch Bug**: Touch coordinates are not transformed for orientation changes, causing UI elements to be unresponsive in landscape mode
2. **Tight Coupling**: Input handling is tightly coupled to GameplayState, making it hard to extend or modify
3. **Multiple Coordinate Transformations**: Coordinates are transformed at multiple levels without consistent orientation awareness
4. **Lack of Separation of Concerns**: Input polling, processing, and game state integration are all mixed together

## Root Cause Analysis

### Current Input Flow
```
iOS Touch Events → GameViewController → TouchInputHandler (Swift)
    ↓
TouchInputHandler normalizes by view size → GameEngine
    ↓
GameEngine converts back to pixels using nativeBounds (no orientation) → C++
    ↓
GameplayState.HandleInput() polls platform delegates
    ↓
NormalizeCoordinates() converts to normalized (0.0-1.0)
    ↓
PlayerControllerSystem receives normalized coordinates
```

### The Landscape Bug
- **TouchInputHandler**: Normalizes coordinates by dividing by current view size
- **GameEngine**: Converts back using `UIScreen.main.nativeBounds` (always returns portrait dimensions)
- **Renderer**: Correctly transforms viewport for landscape (swaps dimensions)
- **Result**: Touch coordinates remain in portrait space while UI is rendered in landscape space

### Coordinate Transformation Issue
```cpp
// Current: No orientation awareness in GameEngine
let pixelBounds = UIScreen.main.nativeBounds  // Always portrait dimensions
let gameX = Float(normalizedPosition.x) * Float(pixelBounds.width)   // Wrong for landscape
let gameY = Float(normalizedPosition.y) * Float(pixelBounds.height)  // Wrong for landscape
```

## Proposed Solution Architecture

### Phase 1: Fix Landscape Coordinate Transformation

#### 1.1 Update TouchInputHandler (Swift)
**File**: `src/iOS/Input/TouchInputHandler.swift`

**Changes**:
- Add orientation detection in touch coordinate processing
- Transform coordinates to match renderer's coordinate space
- Pass orientation-aware coordinates to GameEngine

**Implementation**:
```swift
// Add orientation detection
private func getCurrentInterfaceOrientation() -> UIInterfaceOrientation {
    return UIApplication.shared.windows.first?.windowScene?.interfaceOrientation ?? .portrait
}

// Transform coordinates for orientation
private func transformCoordinatesForOrientation(_ point: CGPoint, _ viewSize: CGSize) -> CGPoint {
    let orientation = getCurrentInterfaceOrientation()

    switch orientation {
    case .landscapeLeft:
        return CGPoint(x: point.y, y: viewSize.width - point.x)
    case .landscapeRight:
        return CGPoint(x: viewSize.height - point.y, y: point.x)
    case .portraitUpsideDown:
        return CGPoint(x: viewSize.width - point.x, y: viewSize.height - point.y)
    default: // portrait
        return point
    }
}
```

#### 1.2 Update GameEngine Touch Delegate
**File**: `src/iOS/GameEngine.swift`

**Changes**:
- Remove hardcoded `UIScreen.main.nativeBounds` usage
- Accept pre-transformed coordinates from TouchInputHandler
- Pass orientation information to C++ side

#### 1.3 Update C++ Platform Delegates
**File**: `src/FloppyTurd/Game/PlatformDelegates.h/cpp`

**Changes**:
- Add orientation-aware coordinate transformation
- Update input polling to handle orientation changes

### Phase 2: Create Centralized InputManager

#### 2.1 Create InputManager Class
**File**: `src/FloppyTurd/Input/InputManager.h/cpp`

**Purpose**: Central hub for all input processing, decoupling from GameplayState

**Features**:
- Input state management (pressed, released, held)
- Coordinate transformation pipeline
- Orientation change handling
- Input device abstraction
- Event queuing system

**Interface**:
```cpp
class InputManager {
public:
    struct InputState {
        bool isPressed;
        bool isReleased;
        bool isHeld;
        Vector2 position;
        Vector2 normalizedPosition;
    };

    // Core methods
    void Update();
    InputState GetTouchState(int touchId = 0);
    InputState GetMouseState();
    bool IsKeyPressed(int keyCode);

    // Orientation handling
    void SetScreenOrientation(bool isLandscape);
    Vector2 TransformCoordinatesForOrientation(const Vector2& coords);

    // Touch-specific
    int GetTouchCount();
    Vector2 GetTouchPosition(int touchId);
};
```

#### 2.2 Create InputContext System
**File**: `src/FloppyTurd/Input/InputContext.h/cpp`

**Purpose**: Game state-aware input processing

**Features**:
- Context-based input mapping (menu vs gameplay vs paused)
- Input filtering based on game state
- Configurable input sensitivity
- Input event delegation

### Phase 3: Refactor GameplayState Input Handling

#### 3.1 Remove Direct Input Polling
**File**: `src/FloppyTurd/States/GameplayState.cpp`

**Changes**:
- Remove direct platform delegate polling from `HandleInput()`
- Replace with InputManager queries
- Simplify coordinate normalization logic

**Before**:
```cpp
void GameplayState::HandleInput() {
    // 50+ lines of platform delegate polling
    if (m_platformDelegates->input.getTouchCount && ...) {
        // Complex nested logic for each input type
    }
}
```

**After**:
```cpp
void GameplayState::HandleInput() {
    // Query InputManager for processed input
    auto touchState = m_inputManager->GetTouchState();
    if (touchState.isPressed) {
        HandleTouchPress(touchState.position);
    }
}
```

#### 3.2 Consolidate Input Methods
**Consolidate these methods**:
- `HandleSettingsButtonClick()` → `HandleUIInput()`
- `CheckSettingsButtonClick()` → `ProcessUIButton()`
- `HandleGameplayInput()` → `ProcessGameplayInput()`
- `NormalizeCoordinates()` → Move to InputManager

### Phase 4: Implement Proper Separation of Concerns

#### 4.1 Input Processing Pipeline
```
Raw Input → InputManager → Coordinate Transformation → InputContext → GameState
```

#### 4.2 Platform Abstraction Layer
**File**: `src/FloppyTurd/Input/PlatformInputAdapter.h/cpp`

**Purpose**: Abstract platform-specific input implementations

**Benefits**:
- Easy to add new platforms
- Consistent interface across platforms
- Platform-specific optimizations

#### 4.3 Event-Driven Input System
**File**: `src/FloppyTurd/Input/InputEventSystem.h/cpp`

**Purpose**: Event-based input processing

**Features**:
- Input event queuing
- Asynchronous event processing
- Input gesture recognition
- Event filtering and prioritization

## Implementation Timeline

### Phase 1: Landscape Fix (High Priority)
1. **Day 1**: Update TouchInputHandler coordinate transformation
2. **Day 1**: Update GameEngine touch delegate methods
3. **Day 2**: Test landscape touch functionality
4. **Day 2**: Verify settings button and shooting bar responsiveness

### Phase 2: InputManager Creation (Medium Priority)
1. **Day 3**: Create InputManager class structure
2. **Day 3**: Implement basic input state management
3. **Day 4**: Add coordinate transformation pipeline
4. **Day 4**: Integrate with existing platform delegates

### Phase 3: GameplayState Refactor (Medium Priority)
1. **Day 5**: Remove direct platform polling from GameplayState
2. **Day 5**: Replace with InputManager queries
3. **Day 6**: Consolidate input handling methods
4. **Day 6**: Test gameplay input functionality

### Phase 4: Advanced Features (Low Priority)
1. **Day 7**: Implement InputContext system
2. **Day 7**: Add PlatformInputAdapter
3. **Day 8**: Implement event-driven system
4. **Day 8**: Comprehensive testing and optimization

## Testing Strategy

### Landscape Touch Testing
1. **Automated Tests**: Create unit tests for coordinate transformation
2. **Integration Tests**: Test touch input across orientation changes
3. **Manual Testing**: Verify UI responsiveness in landscape mode

### Input System Testing
1. **Unit Tests**: Test InputManager functionality
2. **Integration Tests**: Test input flow from platform to gameplay
3. **Performance Tests**: Ensure input processing doesn't impact frame rate

## Benefits of This Refactor

### 1. **Fixes Landscape Bug**
- Touch coordinates properly transformed for orientation
- Consistent coordinate space between input and rendering
- Settings button and UI elements work in landscape mode

### 2. **Improved Maintainability**
- Centralized input logic in InputManager
- Clear separation of concerns
- Easier to debug input issues

### 3. **Enhanced Extensibility**
- Easy to add new input devices (game controllers, etc.)
- Simple to modify input mappings
- Platform abstraction for future ports

### 4. **Better Performance**
- Reduced redundant coordinate transformations
- More efficient input polling
- Event-driven processing reduces CPU usage

### 5. **Cleaner Code Architecture**
- GameplayState focuses on game logic, not input plumbing
- Input system is self-contained and testable
- Clear interfaces between components

## Risk Assessment

### Low Risk
- Coordinate transformation fix (isolated changes)
- InputManager creation (additive changes)

### Medium Risk
- GameplayState refactor (changes existing logic)
- Platform delegate integration (affects multiple systems)

### Mitigation Strategies
1. **Incremental Implementation**: Implement and test each phase separately
2. **Backward Compatibility**: Maintain existing interfaces during transition
3. **Comprehensive Testing**: Test all input scenarios before/after changes
4. **Rollback Plan**: Keep backup of working input system

## Conclusion

This refactor addresses both the immediate landscape touch bug and the broader architectural issues with input handling. By implementing a centralized, orientation-aware input system, we create a more maintainable, extensible, and robust input architecture that will serve the project well into the future.

The phased approach ensures minimal disruption while delivering incremental improvements and bug fixes.

