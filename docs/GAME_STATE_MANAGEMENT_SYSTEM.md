# Game State Management System Documentation

## Current Status: ⚠️ ARCHITECTURE NEEDS REFINEMENT

The game state management system exists but needs architectural improvements based on Apple best practices, Swift 6.0+ C++ interop patterns, and proper ECS management.

## Research Findings & Best Practices

### iOS Game Loop (Apple Recommended - 2024)
**✅ Current Implementation is Correct**: Using `MTKViewDelegate.draw(in:)` is the Apple-recommended approach for Metal-based games. This supersedes manual `CADisplayLink` loops and provides:
- Automatic V-Sync timing
- Optimal frame pacing
- Metal-optimized rendering pipeline
- Built-in back-pressure handling

### Swift 6.0+ C++ Interoperability (WWDC23/24)
**✅ Current Implementation is Robust**: The project correctly uses:
- Native Swift-C++ interop (no `extern "C"` bridging)
- Direct C++ class instantiation from Swift
- Proper module mapping and namespace usage
- Clean ownership boundaries between Swift and C++

### ECS Architecture Pattern
**⚠️ Needs Improvement**: Research shows the **Coordinator Pattern** is recommended:
- Single ECS instance shared across all game states
- States should **reference** the ECS, not own separate instances
- Coordinator mediates between EntityManager, ComponentManager, SystemManager

## Refined Architecture Overview

### Recommended Game Loop Flow (iOS)
```
iOS GameViewController (Swift)
    ↓ MTKViewDelegate.draw(in:)           // ✅ Apple recommended entry point
    ↓ gameEngine.update(deltaTime:)       // Swift → C++ interop
    ↓ gameEngine.render()
        ↓ GameEngine.swift
        ↓ cppGame?.Update(deltaTime)       // Native Swift-C++ call
        ↓ cppGame?.Render()
            ↓ FloppyTurdGame.cpp
            ↓ m_stateManager->Update(deltaTime)  // ❌ COMMENTED OUT
            ↓ m_stateManager->Render()           // ❌ COMMENTED OUT
            ↓ m_ecsCoordinator->Update()         // ⚠️ Single ECS instance
```

## State Management Components

### 1. GameState (Base Class)
**Location**: `src/FloppyTurd/States/GameState.h`

```cpp
class GameState {
public:
    virtual void Enter() = 0;
    virtual void Exit() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void HandleInput() = 0;
    virtual bool IsFinished() const = 0;
    virtual const char* GetStateName() const = 0;
};
```

### 2. GameStateManager
**Location**: `src/FloppyTurd/States/GameStateManager.cpp`

**Features**:
- Stack-based state management
- Deferred state changes (safe for mid-update transitions)
- Push/Pop/Change operations
- Automatic state lifecycle management

**Key Methods**:
```cpp
void Update(float deltaTime) {
    ProcessPendingChanges();
    if (!m_stateStack.empty()) {
        m_stateStack.back()->Update(deltaTime);  // Updates current state
    }
}

void Render() {
    if (!m_stateStack.empty()) {
        m_stateStack.back()->Render();  // Renders current state
    }
}

void HandleInput() {
    if (!m_stateStack.empty()) {
        m_stateStack.back()->HandleInput();  // Handles input for current state
    }
}
```

### 3. State Architecture Recommendations

#### ✅ Core Game States (Top-Level)
- **LoadingState**: Initial game loading
- **MainMenuState**: Main menu with substates
- **GameplayState**: Active gameplay with substates
- **PausedState**: Game paused overlay

#### ⚠️ Substates (Not Top-Level States)
**Options should NOT be a top-level state**. Instead:
- **MainMenu → OptionsSubstate**: Settings accessible from main menu
- **Gameplay → PauseSubstate → OptionsSubstate**: Settings accessible when paused

#### Current Implementation Status

##### LoadingState ✅ IMPLEMENTED
**Location**: `src/FloppyTurd/States/LoadingState.h/.cpp`
- Rotating poop hat loading icon
- 3-second loading duration
- Smooth transition to main menu
- Progress tracking (0.0 to 1.0)

##### MainMenuState ✅ IMPLEMENTED  
**Location**: `src/FloppyTurd/States/MainMenuState.h/.cpp`
- Platform-aware layouts (desktop/mobile)
- Floppy Turd logo with interactive F button
- Menu options: Playing, Options, Quick Play, Quit
- Asset path integration
- **TODO**: Implement options as substate, not separate state

## Current Problems: Multiple Architectural Issues

### 1. Commented Out Integration
**In FloppyTurdGame.cpp**:
```cpp
void FloppyTurdGame::Update(float deltaTime) {
    // Update state manager
    if (m_stateManager) {
        // m_stateManager->Update(deltaTime);  // ❌ COMMENTED OUT
    }
}

void FloppyTurdGame::Render() {
    // Render state manager
    if (m_stateManager) {
        // m_stateManager->Render();  // ❌ COMMENTED OUT
    }
}
```

### 2. ECS Ownership Anti-Pattern
**Current Issue**: Each state creates its own ECS instance:
```cpp
// ❌ WRONG: Each state owns separate ECS
LoadingState::LoadingState(Gnosis::ECS* ecsSystem) : m_ecsSystem(ecsSystem)
MainMenuState::MainMenuState(Gnosis::ECS* ecsSystem) : m_ecsSystem(ecsSystem)
```

**Problem**: This creates multiple ECS instances, breaking entity management and system coherence.

### 3. Missing Input Forwarding
**iOS Touch → C++ State**: No input forwarding from iOS touch events to state manager.

### 4. State Abstraction Over-Engineering
**Options as Top-Level State**: Should be substates of MainMenu/Gameplay, not separate states.

## Recommended Architecture Fixes

### Fix 1: Implement ECS Coordinator Pattern

**Create Single ECS Coordinator**:
```cpp
// In FloppyTurdGame.h
class FloppyTurdGame {
private:
    std::unique_ptr<ECSCoordinator> m_ecsCoordinator;  // ✅ Single instance
    std::unique_ptr<GameStateManager> m_stateManager;
};

// In FloppyTurdGame.cpp
bool FloppyTurdGame::Initialize() {
    // Initialize ECS coordinator FIRST
    m_ecsCoordinator = std::make_unique<ECSCoordinator>();
    m_ecsCoordinator->Init();
    
    // Initialize state manager with ECS reference
    m_stateManager = std::make_unique<GameStateManager>();
    
    // States get ECS reference, not ownership
    auto loadingState = std::make_unique<LoadingState>(m_ecsCoordinator.get());
    m_stateManager->PushState(std::move(loadingState));
    
    return true;
}
```

### Fix 2: Proper Update/Render Order

**In `FloppyTurdGame::Update()`**:
```cpp
void FloppyTurdGame::Update(float deltaTime) {
    if (!m_initialized || !m_running || m_paused) {
        return;
    }

    // 1. Update current game state FIRST
    if (m_stateManager) {
        m_stateManager->Update(deltaTime);  // ✅ UNCOMMENT THIS
    }

    // 2. Update ECS systems (entities created by states)
    if (m_ecsCoordinator) {
        m_ecsCoordinator->Update(deltaTime);  // ✅ Single ECS update
    }

    // 3. Update debug info
    if (m_showDebugInfo) {
        UpdateDebugInfo(deltaTime);
    }
}
```

**In `FloppyTurdGame::Render()`**:
```cpp
void FloppyTurdGame::Render() {
    if (!m_initialized || !m_running) {
        return;
    }

    // Begin frame and clear screen
    if (m_platformDelegates.renderer.beginFrame) {
        m_platformDelegates.renderer.beginFrame();
    }
    if (m_platformDelegates.renderer.clearScreen) {
        m_platformDelegates.renderer.clearScreen(0.2f, 0.3f, 0.3f, 1.0f);
    }

    // 1. Render current game state
    if (m_stateManager) {
        m_stateManager->Render();  // ✅ UNCOMMENT THIS
    }

    // 2. Render ECS systems (entities managed by states)
    if (m_ecsCoordinator) {
        m_ecsCoordinator->Render();  // ✅ Single ECS render
    }

    // 3. Render debug info
    if (m_showDebugInfo) {
        RenderDebugInfo();
    }

    // End frame
    if (m_platformDelegates.renderer.endFrame) {
        m_platformDelegates.renderer.endFrame();
    }
    if (m_platformDelegates.renderer.present) {
        m_platformDelegates.renderer.present();
    }
}
```

### Fix 3: Add Input Forwarding (iOS → C++)

**In GameViewController.swift**:
```swift
public func touchInputHandler(_ handler: TouchInputHandler, didReceiveInput input: Any) {
    // Forward to C++ game for state-specific handling
    gameEngine.handleTouchInput(input)  // ✅ Add this method
}
```

**In GameEngine.swift**:
```swift
public func handleTouchInput(_ input: Any) {
    guard let cppGame = cppGame else { return }
    cppGame.HandleInput()  // ✅ Forward to C++ state manager
}
```

**In FloppyTurdGame.cpp**:
```cpp
void FloppyTurdGame::HandleInput() {
    if (m_stateManager) {
        m_stateManager->HandleInput();  // ✅ Add this method
    }
}
```

### Fix 4: Refactor State Constructors

**Change from ECS ownership to ECS reference**:
```cpp
// ❌ OLD: Each state owns ECS
LoadingState::LoadingState(Gnosis::ECS* ecsSystem) : m_ecsSystem(ecsSystem)

// ✅ NEW: Each state references shared ECS coordinator
LoadingState::LoadingState(ECSCoordinator* ecsCoordinator) 
    : m_ecsCoordinator(ecsCoordinator)

// States use coordinator to create/manage entities
void LoadingState::CreateLoadingEntities() {
    m_poopHatEntity = m_ecsCoordinator->CreateEntity();
    m_ecsCoordinator->AddComponent<Transform>(m_poopHatEntity, transform);
    m_ecsCoordinator->AddComponent<Sprite>(m_poopHatEntity, sprite);
}
```

### Step 4: State Transitions

**In LoadingState** (when loading completes):
```cpp
void LoadingState::Update(float deltaTime) {
    // ... existing update code ...
    
    // Check if loading is complete
    if (m_loadingTimer >= LOADING_DURATION) {
        // Transition to main menu
        // This would be handled by the game's state management logic
        m_finished = true;
    }
}
```

**In FloppyTurdGame** (check for state transitions):
```cpp
void FloppyTurdGame::Update(float deltaTime) {
    // ... existing code ...
    
    // Check for state transitions
    if (m_stateManager && !m_stateManager->IsEmpty()) {
        GameState* currentState = m_stateManager->GetCurrentState();
        if (currentState && currentState->IsFinished()) {
            HandleStateTransition(currentState);
        }
    }
}

void FloppyTurdGame::HandleStateTransition(GameState* finishedState) {
    const char* stateName = finishedState->GetStateName();
    
    if (strcmp(stateName, "Loading") == 0) {
        // Transition from loading to main menu
        auto mainMenuState = std::make_unique<MainMenuState>(m_ecsSystem.get());
        m_stateManager->ChangeState(std::move(mainMenuState));
    }
    else if (strcmp(stateName, "MainMenu") == 0) {
        // Handle main menu selections
        // This would be determined by the menu state's selection
        // For now, just log
        GN_LOG_INFO("Main menu finished - implement game state transition");
    }
}
```

## iOS Integration Points

### Touch Input Forwarding
**In GameViewController.swift**:
```swift
public func touchInputHandler(_ handler: TouchInputHandler, didReceiveInput input: Any) {
    // Forward to C++ game for state-specific handling
    gameEngine.handleTouchInput(input)
}
```

**In GameEngine.swift**:
```swift
public func handleTouchInput(_ input: Any) {
    cppGame?.HandleInput()  // This will call the state manager's HandleInput
}
```

## State Lifecycle

```
Application Start
    ↓
GameStateManager::Initialize()
    ↓
PushState(LoadingState)
    ↓
LoadingState::Enter()
    ↓ (3 seconds of rotating poop hat)
LoadingState::Update() → IsFinished() = true
    ↓
ChangeState(MainMenuState)
    ↓
LoadingState::Exit()
MainMenuState::Enter()
    ↓ (user interaction)
MainMenuState::Update() → menu selection
    ↓
Transition to GameplayState/OptionsState/Quit
```

## Benefits of Proper Architecture

### ECS Coordinator Pattern Benefits
1. **Single Source of Truth**: One ECS instance manages all entities
2. **Entity Persistence**: Entities can persist across state transitions
3. **System Coherence**: All systems operate on the same entity pool
4. **Memory Efficiency**: No duplicate ECS instances
5. **Clear Ownership**: Coordinator owns ECS, states reference it

### State Management Benefits
1. **Clean Separation**: Each game screen/mode is isolated
2. **Easy Transitions**: Smooth state changes with proper cleanup
3. **Input Handling**: Each state handles its own input appropriately
4. **Substate Support**: Options/pause as substates, not top-level states
5. **Debugging**: Clear state tracking and logging

### iOS Integration Benefits
1. **Apple Best Practice**: MTKViewDelegate.draw(in:) is the recommended entry point
2. **Swift 6.0+ Interop**: Native C++ calls without bridging overhead
3. **Metal Optimization**: Automatic V-Sync and frame pacing
4. **Touch Input**: Direct forwarding from iOS to C++ state manager

## State Lifecycle

```
Application Start
    ↓
GameStateManager::Initialize()
    ↓
PushState(LoadingState)
    ↓
LoadingState::Enter()
    ↓ (3 seconds of rotating poop hat)
LoadingState::Update() → IsFinished() = true
    ↓
ChangeState(MainMenuState)
    ↓
LoadingState::Exit()
MainMenuState::Enter()
    ↓ (user interaction)
MainMenuState::Update() → menu selection
    ↓
Transition to GameplayState/OptionsState/Quit
```

## Next Steps to Enable

1. **Implement ECS Coordinator Pattern**
2. **Enable State Manager Integration**
3. **Implement Input Forwarding**
4. **Test State Transitions**
5. **Add Missing States**

The foundation is solid - we just need to implement the ECS coordinator pattern, enable state manager integration, and add input forwarding to make it work!
