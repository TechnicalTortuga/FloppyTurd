# MetalRenderer Screen Information System

## Overview

The MetalRenderer system in FloppyTurd handles screen information management, caching, and updates during orientation changes. This document explains how screen information flows from the iOS device through MetalRenderer to ConfigManager and ultimately to game states.

## Key Components

### 1. MetalRenderer (Swift)
**File:** `src/iOS/Rendering/MetalRenderer.swift`
- Primary interface between iOS Metal framework and C++ game engine
- Manages screen information collection and viewport updates

### 2. ConfigManager (C++)
**File:** `src/Engine/Configuration/ConfigManager.cpp`
- Singleton that caches screen information for the entire application
- Provides screen info to game states and systems

### 3. GameViewController (Swift)
**File:** `src/iOS/GameViewController.swift`
- iOS UIViewController that manages the Metal view
- Handles orientation change events and forwards them to MetalRenderer

## Screen Information Flow

### 1. Initialization Phase

#### MetalRenderer Initialization
```swift
// MetalRenderer.initialize() - Called during app startup
// File: src/iOS/Rendering/MetalRenderer.swift, line ~446
public func initialize() -> Bool {
    // ... Metal setup code ...

    // Set initial viewport size from MTKView
    viewportSize = view.drawableSize  // Line 488

    // ... more initialization ...
}
```

#### ConfigManager Initialization
```cpp
// ConfigManager.Initialize() - Called during app startup
// File: src/Engine/Configuration/ConfigManager.cpp, line ~12
void ConfigManager::Initialize(const PlatformDelegates& delegates) {
    m_delegates = delegates;
    DetectPlatform();
    UpdateScreenInfo();  // Line 15 - Gets initial screen info
    LoadDefaultConfiguration();
    CalculateScaleFactors();
}
```

### 2. Screen Information Collection

#### getScreenInfo() Method
```swift
// MetalRenderer.getScreenInfo() - Main screen info collection method
// File: src/iOS/Rendering/MetalRenderer.swift, line ~1686
public func getScreenInfo() -> GameCore.ScreenInfo {
    // Get logical bounds (in points)
    let logicalBounds = mainScreen.bounds
    let logicalWidth = Float(logicalBounds.width)
    let logicalHeight = Float(logicalBounds.height)

    // Get pixel bounds (native scale)
    let pixelBounds = mainScreen.nativeBounds
    let pixelWidth = Float(pixelBounds.width)
    let pixelHeight = Float(pixelBounds.height)

    // Calculate scale factor
    let scaleFactor = Float(mainScreen.nativeScale)

    // Determine orientation using device orientation, not just pixel dimensions
    // This is important for iOS Simulator where screen dimensions don't change on rotation
    let deviceOrientation = UIDevice.current.orientation
    let isPortrait = deviceOrientation.isPortrait ||
                    (pixelHeight > pixelWidth && deviceOrientation == .unknown)

    // Create and return ScreenInfo struct
    var screenInfo = GameCore.ScreenInfo()
    screenInfo.logicalWidth = logicalWidth
    screenInfo.logicalHeight = logicalHeight
    screenInfo.pixelWidth = pixelWidth
    screenInfo.pixelHeight = pixelHeight
    screenInfo.scaleFactor = scaleFactor
    screenInfo.isPortrait = isPortrait
    screenInfo.deviceModel = std.string(deviceModel)

    return screenInfo
}
```

### 3. Screen Information Storage/Caching

#### ConfigManager Storage
```cpp
// ConfigManager - Member variables for caching screen info
// File: src/Engine/Configuration/ConfigManager.h
private:
    ScreenInfo m_screenInfo;  // Cached screen information
    PlatformDelegates m_delegates;  // Platform function pointers
```

#### UpdateScreenInfo() Method
```cpp
// ConfigManager.UpdateScreenInfo() - Updates cached screen info
// File: src/Engine/Configuration/ConfigManager.cpp, line ~38
void ConfigManager::UpdateScreenInfo() {
    // Prefer enhanced info; do not clobber cached valid info with legacy fallbacks
    if (m_delegates.renderer.getScreenInfo) {
        m_delegates.renderer.getScreenInfo(&m_screenInfo);
        CalculateScaleFactors(); // Recalculate when screen changes
        GN_LOG_INFO("Screen info updated: " + std::to_string(m_screenInfo.pixelWidth) + "x" +
                   std::to_string(m_screenInfo.pixelHeight) + " pixels");
        return;
    }
    // ... fallback logic ...
}
```

### 4. Orientation Change Handling

#### UIViewController Orientation Changes
```swift
// GameViewController.viewWillTransition() - Handles orientation changes
// File: src/iOS/GameViewController.swift, line ~338
override public func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)

    log("View will transition to size: \(size)")

    // Update screen info when view transitions (orientation change)
    coordinator.animate(alongsideTransition: { _ in
        // This is called during the animation
    }) { _ in
        // This is called after the transition completes
        DispatchQueue.main.async {
            // Force MetalRenderer to update screen info after orientation change
            if let renderer = self.metalRenderer {
                let screenInfo = renderer.getScreenInfo()
                self.log("Orientation changed - New screen info: \(screenInfo.pixelWidth)x\(screenInfo.pixelHeight), portrait: \(screenInfo.isPortrait)", level: .info)

                // ConfigManager will automatically update its screen info through the MetalRenderer
                // The singleton pattern ensures it's updated when the MetalRenderer changes
                self.log("ConfigManager screen info will be updated through MetalRenderer", level: .debug)
            }
            self.log("View transition completed", level: .debug)
        }
    }
}
```

#### MTKView Delegate Methods
```swift
// GameViewController.mtkView() - Handles Metal view size changes
// File: src/iOS/GameViewController.swift, line ~295
public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
    log("Metal view size changed to \(size)")
    metalRenderer?.updateViewportSize(width: Float(size.width), height: Float(size.height))

    // Screen info is automatically updated by MetalRenderer when viewport size changes
    log("Viewport size updated", level: .debug)
}
```

#### MetalRenderer Viewport Updates
```swift
// MetalRenderer.updateViewportSize() - Updates Metal viewport
// File: src/iOS/Rendering/MetalRenderer.swift, line ~570
public func updateViewportSize(width: Float, height: Float) {
    viewportSize = CGSize(width: CGFloat(width), height: CGFloat(height))
    updateProjectionMatrix()
    log("Viewport size updated to (\(width), \(height))", level: .debug)
}
```

### 5. Game State Access to Screen Info

#### ScreenPromptState Usage
```cpp
// ScreenPromptState.CheckOrientation() - How game states access screen info
// File: src/FloppyTurd/States/ScreenPromptState.cpp
void ScreenPromptState::CheckOrientation() {
    // Get screen info from ConfigManager singleton
    auto& configManager = ConfigManager::Instance();
    const auto& screenInfo = configManager.GetScreenInfo();

    // Use screen info for orientation detection
    bool isCurrentlyPortrait = screenInfo.isPortrait;

    // ... orientation logic ...
}
```

## Key Design Patterns

### 1. Singleton Pattern (ConfigManager)
- **Purpose:** Global access to screen information across the entire application
- **Implementation:** Static instance method returns single shared instance
- **Benefits:** Consistent screen info across all systems, automatic updates

### 2. Platform Abstraction (PlatformDelegates)
- **Purpose:** Decouple platform-specific code from game logic
- **Implementation:** Function pointers in PlatformDelegates struct
- **Benefits:** Clean separation between iOS and C++ code

### 3. Observer Pattern (Orientation Changes)
- **Purpose:** React to orientation changes without tight coupling
- **Implementation:** UIViewController notifies MetalRenderer of size changes
- **Benefits:** Loose coupling between UI and rendering systems

## Current Issues and Limitations

### 1. ConfigManager Update Timing
**Problem:** ConfigManager doesn't automatically update when MetalRenderer detects orientation changes
**Current Workaround:** Manual calls to ConfigManager.UpdateScreenInfo() are needed
**Potential Solution:** Add automatic update mechanism

### 2. iOS Simulator Orientation Detection
**Problem:** iOS Simulator doesn't change pixel dimensions on rotation
**Current Fix:** Use UIDevice.current.orientation for reliable detection
**Implementation:** Lines 1705-1706 in getScreenInfo()

### 3. Thread Safety
**Problem:** Screen info updates happen on main thread, accessed from game thread
**Current Mitigation:** Use atomic operations and careful synchronization
**Note:** ConfigManager is a singleton and should be thread-safe

## Architecture Analysis: Redundancy Issues

### Current System Architecture

There are **THREE separate screen information systems** creating redundancy and synchronization issues:

#### 1. MetalRenderer (Swift) - Data Collection
- **Purpose:** Collect raw screen information from iOS APIs
- **Storage:** Returns `ScreenInfo` struct by value
- **Scope:** Platform-specific, iOS-only
- **Update Trigger:** iOS orientation notifications

#### 2. ConfigManager (C++) - Global Caching
- **Purpose:** Singleton cache for application-wide screen information
- **Storage:** `ScreenInfo m_screenInfo` member variable
- **Scope:** Global singleton, accessible from anywhere
- **Update Trigger:** Manual calls to `UpdateScreenInfo()`

#### 3. RenderSystem (C++) - Rendering Caching
- **Purpose:** Cache screen info for rendering calculations
- **Storage:** `ScreenInfo m_screenInfo` member variable
- **Scope:** Render system specific
- **Update Trigger:** Calls to `UpdateScreenInfo()` which reads from ConfigManager

### Current Data Flow Problems

```
iOS Device Orientation Change
        ↓
GameViewController.viewWillTransition()
        ↓
MetalRenderer.getScreenInfo() ← ✅ Works correctly
        ↓
ConfigManager.UpdateScreenInfo() ← ✅ Now has callback mechanism
        ↓
RenderSystem.UpdateScreenInfo() ← ❌ Manual sync required
        ↓
ScreenPromptState.CheckOrientation() ← ❌ Uses stale data
```

### Redundancy Analysis

| System | Purpose | Storage | Synchronization | Access Pattern |
|--------|---------|---------|------------------|----------------|
| **MetalRenderer** | Raw data collection | None (returns by value) | N/A | Direct iOS API calls |
| **ConfigManager** | Global application cache | Singleton member | Manual updates | `ConfigManager::Instance()` |
| **RenderSystem** | Render-specific cache | System member | Reads from ConfigManager | ECS system access |

## Recommendation: Clean Architecture

### Option 1: ConfigManager-Only (RECOMMENDED) ⭐

**Pros:**
- ✅ Single source of truth
- ✅ Automatic updates via callback mechanism
- ✅ Global accessibility
- ✅ No synchronization issues
- ✅ Thread-safe singleton pattern

**Cons:**
- ❌ Requires callback mechanism implementation

**Implementation:**
```cpp
// ScreenPromptState should use:
auto& configManager = ConfigManager::Instance();
const auto& screenInfo = configManager.GetCurrentScreenInfo();
```

### Option 2: RenderSystem-Only

**Pros:**
- ✅ ECS-integrated access pattern
- ✅ Automatic synchronization with ConfigManager

**Cons:**
- ❌ Still depends on ConfigManager updates
- ❌ More complex data flow
- ❌ ECS dependency for screen info access

### Option 3: MetalRenderer + ConfigManager (Hybrid)

**Pros:**
- ✅ Direct iOS API access
- ✅ Global caching

**Cons:**
- ❌ Platform-specific code in game states
- ❌ More complex than ConfigManager-only

## Clean Solution Implementation

The **ConfigManager-only approach** is the cleanest because:

1. **Single Responsibility:** ConfigManager handles all screen information caching
2. **Global Access:** Available from any system without ECS dependencies
3. **Automatic Updates:** Callback mechanism ensures real-time updates
4. **Thread Safety:** Singleton pattern with proper synchronization
5. **Platform Independence:** Abstracts away platform-specific details

### Required Changes

1. **Update ScreenPromptState** to use ConfigManager directly:
```cpp
// Replace RenderSystem calls with ConfigManager
auto& configManager = ConfigManager::Instance();
const auto& screenInfo = configManager.GetCurrentScreenInfo();
```

2. **Keep MetalRenderer → ConfigManager flow** for data collection
3. **Remove RenderSystem screen info duplication** - let it read from ConfigManager when needed

This eliminates the redundant caching and synchronization issues while maintaining clean separation of concerns.

## Critical Functions to Monitor

1. **`MetalRenderer.getScreenInfo()`** - Core screen info collection
2. **`ConfigManager.UpdateScreenInfo()`** - Screen info caching
3. **`GameViewController.viewWillTransition()`** - Orientation change handling
4. **`GameViewController.mtkView(_:drawableSizeWillChange:)`** - Metal view updates
5. **`ScreenPromptState.CheckOrientation()`** - Game state orientation logic

## Recommended Fixes

### 1. Automatic ConfigManager Updates
Add a mechanism for ConfigManager to automatically update when MetalRenderer detects changes:

```cpp
// In ConfigManager.h
void SetScreenInfoCallback(std::function<void()> callback);

// In MetalRenderer (Swift)
configManager.SetScreenInfoCallback {
    // Callback that updates ConfigManager
    ConfigManager::Instance().UpdateScreenInfo();
}
```

### 2. Thread-Safe Screen Info Updates
Implement thread-safe updates using atomic operations:

```cpp
// In ConfigManager.h
std::atomic<bool> m_screenInfoDirty;
std::mutex m_screenInfoMutex;
```

This ensures that screen information updates are thread-safe and don't cause race conditions between the main thread (iOS) and game thread (C++).
