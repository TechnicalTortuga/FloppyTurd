# FloppyTurd Orientation & Rendering Pipeline Analysis

## Executive Summary

**Problem:** Game renders incorrectly when starting in landscape mode, showing UI positioned for landscape even though orientation locks to portrait. Boss level can self-correct when rotated 180°, but portrait mode cannot.

**Root Cause:** Race condition between orientation lock request and MTKView drawable size update, combined with ConfigManager not being updated at the right time.

---

## Current Pipeline Flow

### 1. App Launch Sequence

```
AppDelegate.didFinishLaunchingWithOptions()
├─> Create GameViewController
├─> gameViewController.lockToPortrait()  ← FIRST LOCK
│   ├─> Sets orientationLocked = true
│   ├─> Sets lockedOrientation = [.portrait, .portraitUpsideDown]
│   ├─> Calls setNeedsUpdateOfSupportedInterfaceOrientations()
│   └─> Calls requestGeometryUpdate(.portrait) ← ASYNC!
├─> window?.makeKeyAndVisible()
└─> Enable device orientation monitoring
```

**Timeline:**
- T+0ms: `lockToPortrait()` called
- T+0ms: Orientation lock requested (async)
- T+???ms: iOS processes orientation change
- T+???ms: MTKView size updates
- T+???ms: `mtkView(_:drawableSizeWillChange:)` fires

**Problem:** GameEngine initializes and MainMenuState enters **before** MTKView size has updated!

---

### 2. MetalRenderer Initialization

```
GameViewController.setupGameEngine()
├─> metalRenderer = MetalRenderer()
├─> metalRenderer.setMetalView(metalView)
│   ├─> viewportSize = view.drawableSize  ← READS CURRENT SIZE
│   └─> updateProjectionMatrix()
└─> gameEngine.setMetalRenderer(metalRenderer)
```

**Current State at This Point:**
- If started in landscape: `drawableSize = (2556, 1179)` ❌
- Portrait lock requested but not applied yet!

---

### 3. Screen Info Acquisition Flow

#### Path A: getScreenInfo() - Used During Initialization

```swift
MetalRenderer.getScreenInfo()
├─> if metalView exists:
│   ├─> pixelWidth = metalView.drawableSize.width
│   ├─> pixelHeight = metalView.drawableSize.height
│   └─> logicalWidth/Height from metalView.bounds
├─> else (fallback):
│   └─> Use UIScreen.nativeBounds + windowScene orientation
│
├─> Detect orientation from UIDevice.current.orientation
│
└─> Adjust dimensions based on isPortrait flag:
    ├─> if landscape: ensure width > height
    └─> if portrait: ensure height > width
```

**Issues:**
1. `drawableSize` reflects **current** MTKView size, not requested orientation
2. `UIDevice.current.orientation` = `.unknown` at launch (returns 0)
3. Fallback uses `windowScene.interfaceOrientation` but MTKView may not match yet

---

#### Path B: mtkView(_:drawableSizeWillChange:) - Orientation Change Callback

```swift
mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize)
├─> metalRenderer.updateViewportSize(size)
│   ├─> viewportSize = size
│   └─> updateProjectionMatrix()
│
├─> screenInfo = metalRenderer.getScreenInfo()
│   └─> Now reads NEW drawableSize
│
└─> gameEngine.cppGame?.UpdateScreenInfo(screenInfo)
    └─> ConfigManager.UpdateScreenInfo()
        ├─> Calls renderer.getScreenInfo() delegate
        ├─> CalculateScaleFactors()
        └─> Triggers screen info callback
```

**When This Fires:**
- ✅ When MTKView size actually changes (device rotation, window resize)
- ✅ After `viewWillTransition(to:with:)` completes
- ❌ NOT immediately after `lockToPortrait()` request
- ❌ NOT during app initialization (no size change yet)

---

### 4. ConfigManager Update Paths

#### Path 1: Direct Set (Used at Initialization)
```cpp
setScreenInfoDirect(screenInfo)
├─> m_screenInfo = screenInfo
├─> m_pixelWidth = screenInfo.pixelWidth
├─> m_pixelHeight = screenInfo.pixelHeight
├─> NO CalculateScaleFactors() call!
└─> NO callback trigger!
```

#### Path 2: Full Update (Used on Orientation Change)
```cpp
UpdateScreenInfo()
├─> renderer.getScreenInfo(&m_screenInfo)  // Delegate call
├─> CalculateScaleFactors()                // Recalculates!
├─> Triggers m_screenInfoUpdateCallback    // Notifies systems!
└─> UI systems reposition elements
```

**Critical Difference:**
- `setScreenInfoDirect()` = Just sets values
- `UpdateScreenInfo()` = Sets values + recalculates + notifies

---

## Race Condition Timeline

### Scenario: App Starts in Landscape

```
T=0ms    AppDelegate: lockToPortrait() called
         │
         ├─> requestGeometryUpdate(.portrait) ← Async request
         │
T=10ms   GameEngine.initialize() called
         │
         ├─> MetalRenderer reads drawableSize
         │   └─> Still landscape: 2556x1179 ❌
         │
         ├─> setScreenInfoDirect(2556x1179)
         │   └─> ConfigManager gets WRONG dimensions ❌
         │
T=50ms   MainMenuState.Enter()
         │
         ├─> lockToPortrait() again (redundant)
         ├─> Reads screen dimensions: 2556x1179 ❌
         └─> Creates UI with LANDSCAPE layout ❌
         │
T=150ms  (iOS processes orientation change)
         │
T=200ms  MTKView drawable size changes to 1179x2556 ✅
         │
T=201ms  mtkView(_:drawableSizeWillChange:) fires
         │
         ├─> UpdateScreenInfo(1179x2556) ✅
         └─> ConfigManager now has correct dimensions
         
T=202ms  BUT: UI already created with wrong layout! ❌
         MainMenuState already created buttons at wrong positions!
```

**Result:** Loading screen and main menu render at landscape positions/sizes even though device is portrait.

---

### Scenario: Boss Level 180° Rotation (Why It Works)

```
T=0ms    User physically rotates device 180° in landscape
         │
T=50ms   MTKView drawable size changes (still landscape, flipped)
         │
T=51ms   mtkView(_:drawableSizeWillChange:) fires ✅
         │
         ├─> UpdateScreenInfo() called
         ├─> ConfigManager recalculates
         └─> UI systems notified
         │
T=52ms   GameplayState repositions UI ✅
         │
Result:  UI adjusts correctly!
```

**Why This Works:**
1. MTKView size actually changed (even if same dimensions, different bounds)
2. `drawableSizeWillChange` callback fired
3. `UpdateScreenInfo()` called with correct dimensions
4. UI systems repositioned via callback

---

## Key System Components

### 1. MTKView (iOS System)
- **Owns:** drawable size, view bounds
- **Updates:** Asynchronously after orientation changes
- **Callback:** `mtkView(_:drawableSizeWillChange:)`
- **Issue:** Timing is unpredictable, can be 100-300ms after orientation request

### 2. MetalRenderer (Swift)
- **Reads:** MTKView.drawableSize
- **Provides:** getScreenInfo() method
- **Issue:** Returns current size, not requested size
- **No State:** Doesn't know what orientation was requested

### 3. ConfigManager (C++)
- **Stores:** Screen dimensions, scale factors
- **Methods:**
  - `setScreenInfoDirect()` - Direct set (no callbacks)
  - `UpdateScreenInfo()` - Full update (with callbacks)
- **Callback:** `m_screenInfoUpdateCallback` notifies UI systems
- **Issue:** Gets wrong dimensions during initialization

### 4. MainMenuState (C++)
- **Reads:** ConfigManager screen dimensions
- **Creates:** UI layout based on those dimensions
- **Issue:** Reads dimensions before they're correct
- **No Update:** Doesn't reposition UI when dimensions change later

---

## Research Findings

### Known iOS/MTKView Issues

1. **drawableSizeWillChange Timing**
   - Not called immediately after orientation request
   - Can report incorrect sizes temporarily (iOS bug, fixed in iOS 13+)
   - Delay varies: 100-300ms after `viewWillTransition`

2. **Device Orientation at Launch**
   - `UIDevice.current.orientation` = `.unknown` (rawValue: 0)
   - Can't rely on device orientation during app initialization
   - Must use `UIWindowScene.interfaceOrientation` instead

3. **Race Conditions**
   - Common problem in Metal apps
   - Solutions: Use `viewWillTransition(to:with:)` coordinator, or manual drawable size management

4. **Alternative Approaches**
   - Use `CAMetalLayer` directly with `presentsWithTransaction`
   - Manual drawable size calculation instead of relying on MTKView
   - Override `viewWillTransition` and update synchronously

---

## Current Fix Attempts (And Why They Failed)

### Attempt 1: AppDelegate lockToPortrait() ❌
**What:** Lock portrait before window appears
**Issue:** MTKView not created yet, lock has no effect on drawable size

### Attempt 2: Timed Callback After lockToPortrait() ❌
**What:** Wait 0.15s after lock, then update ConfigManager
**Issue:** MTKView size still hasn't changed, update happens with wrong dimensions

### Attempt 3: Rely on mtkView(_:drawableSizeWillChange:) ❌
**What:** Remove timed callbacks, let MTKView callback handle it
**Issue:** Callback fires AFTER UI is already created with wrong layout

### Attempt 4: MainMenuState Sleep Wait ❌
**What:** Sleep 150ms in MainMenuState.Enter() after lockToPortrait()
**Issue:** Blocks main thread, still no guarantee MTKView updated

---

## Proposed Solutions

### Solution 1: Synchronous Orientation in viewWillTransition ⭐⭐⭐⭐⭐

**Concept:** Use `viewWillTransition(to:with:)` to update ConfigManager synchronously BEFORE view rotates.

```swift
override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    
    // BEFORE rotation animation
    let newOrientation = size.width > size.height ? "landscape" : "portrait"
    log("viewWillTransition: BEFORE rotation to \(newOrientation), size: \(size)")
    
    // Update viewport size immediately (before MTKView updates)
    metalRenderer?.updateViewportSize(width: Float(size.width), height: Float(size.height))
    
    // Force ConfigManager update with new size
    if let renderer = metalRenderer {
        var manualScreenInfo = renderer.getScreenInfo()
        // Override with transition size
        manualScreenInfo.pixelWidth = Float(size.width)
        manualScreenInfo.pixelHeight = Float(size.height)
        manualScreenInfo.isPortrait = size.height > size.width
        
        gameEngine?.cppGame?.UpdateScreenInfo(manualScreenInfo)
        log("✅ ConfigManager updated BEFORE rotation animation")
    }
    
    // DURING rotation animation
    coordinator.animate(alongsideTransition: { context in
        // Animation block
    }) { context in
        // AFTER rotation completes
        log("viewWillTransition: Rotation animation complete")
    }
}
```

**Pros:**
- ✅ Synchronous, no race condition
- ✅ Called BEFORE rotation animation
- ✅ Gets final size as parameter
- ✅ Works for all orientation changes

**Cons:**
- ❌ Doesn't fire on app launch (no transition yet)
- ❌ Still need to handle initial layout

---

### Solution 2: Force MTKView Size in setupMetalView ⭐⭐⭐⭐

**Concept:** Manually set MTKView drawable size after creating it.

```swift
private func setupMetalView() {
    // ... existing setup ...
    
    // FORCE portrait drawable size immediately
    let portraitWidth: CGFloat = min(view.bounds.width, view.bounds.height)
    let portraitHeight: CGFloat = max(view.bounds.width, view.bounds.height)
    
    metalView.drawableSize = CGSize(
        width: portraitWidth * UIScreen.main.nativeScale,
        height: portraitHeight * UIScreen.main.nativeScale
    )
    
    log("✅ Forced MTKView drawable size to portrait: \(metalView.drawableSize)")
}
```

**Pros:**
- ✅ Immediate effect
- ✅ Guarantees portrait size at initialization
- ✅ Simple, one-time fix

**Cons:**
- ❌ Might conflict with auto-resizing
- ❌ May cause visual glitch during transition

---

### Solution 3: Defer MainMenuState Entry Until Correct Size ⭐⭐⭐

**Concept:** Don't enter MainMenuState until MTKView has correct dimensions.

```cpp
// In LoadingState.cpp
void LoadingState::Update(float deltaTime) {
    if (m_loadingComplete) {
        // Check if screen is portrait before transitioning
        auto& config = ConfigManager::Instance();
        if (config.GetScreenInfo().isPortrait) {
            // Safe to transition
            m_finished = true;
        } else {
            // Wait for orientation change
            GN_LOG_INFO("Waiting for portrait orientation before showing main menu...");
        }
    }
}
```

**Pros:**
- ✅ Ensures MainMenuState always gets correct dimensions
- ✅ Loading screen stays until ready

**Cons:**
- ❌ User sees loading screen longer
- ❌ What if orientation never updates?

---

### Solution 4: MainMenuState Dynamic Relayout on Update ⭐⭐⭐⭐

**Concept:** Make MainMenuState check for dimension changes and recreate layout.

```cpp
void MainMenuState::Update(float deltaTime) {
    // Check for screen dimension changes
    if (m_renderSystem) {
        ScreenInfo current = m_renderSystem->GetScreenInfo();
        if (current.pixelWidth != m_screenWidth || current.pixelHeight != m_screenHeight) {
            GN_LOG_INFO("Screen dimensions changed during main menu!");
            GN_LOG_INFO("Old: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
            GN_LOG_INFO("New: " + std::to_string(current.pixelWidth) + "x" + std::to_string(current.pixelHeight));
            
            // Update stored dimensions
            m_screenWidth = current.pixelWidth;
            m_screenHeight = current.pixelHeight;
            
            // Recreate entire UI layout
            DestroyUIElements();
            CreateUIElements();
            
            GN_LOG_INFO("✅ Main menu UI recreated for new dimensions");
        }
    }
    
    // ... rest of update ...
}
```

**Pros:**
- ✅ Self-correcting
- ✅ Works even if orientation changes after entering
- ✅ No timing dependencies

**Cons:**
- ❌ Performance cost of recreating UI
- ❌ Visual glitch during recreation

---

### Solution 5: Screen Prompt for Main Menu (Last Resort) ⭐⭐

**Concept:** Like boss level, show "Please rotate to portrait" prompt.

**Pros:**
- ✅ Guaranteed correct orientation
- ✅ User-driven, reliable

**Cons:**
- ❌ Poor UX on main menu
- ❌ Annoying for users
- ❌ Doesn't fix the underlying issue

---

## Recommended Approach

### Hybrid Solution: Combine #1, #2, and #4

1. **At Launch (Solution #2):** Force MTKView portrait size in `setupMetalView()`
2. **During Transitions (Solution #1):** Use `viewWillTransition` to update before rotation
3. **Safety Net (Solution #4):** MainMenuState checks dimensions in `Update()` and relayouts if needed

### Implementation Priority

**Phase 1: Critical Fixes**
- ✅ Implement `viewWillTransition` update (Solution #1)
- ✅ Force MTKView portrait size at creation (Solution #2)

**Phase 2: Robustness**
- ✅ Add dynamic relayout to MainMenuState (Solution #4)
- ✅ Add dynamic relayout to GameplayState

**Phase 3: Polish**
- ⚠️ Only add screen prompt if all else fails (Solution #5)

---

## Code Locations to Modify

### 1. GameViewController.swift
```swift
// ADD viewWillTransition override
override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator)

// MODIFY setupMetalView()
// Force drawable size to portrait
```

### 2. MainMenuState.cpp
```cpp
// MODIFY Update()
// Add dimension change detection and relayout
```

### 3. GameplayState.cpp
```cpp
// MODIFY Update() 
// Add dimension change detection and relayout
```

### 4. MetalRenderer.swift
```swift
// ADD method to override drawable size
public func forceDrawableSize(width: Float, height: Float)
```

---

## Testing Checklist

- [ ] Boot app in portrait → Main menu renders correctly
- [ ] Boot app in landscape → Main menu renders correctly (after auto-rotate to portrait)
- [ ] Main menu → Rotate device → UI adjusts
- [ ] Main menu → Boss level → Rotates to landscape correctly
- [ ] Boss level → Main menu → Rotates back to portrait correctly
- [ ] Boss level → Rotate 180° → UI adjusts correctly
- [ ] Regular level → Stays portrait, UI correct
- [ ] App suspend/resume → Orientation maintained

---

## Additional Notes

### Why Boss Level Works
- Boss level allows rotation
- When user rotates 180°, MTKView size "changes" (even if same dimensions)
- This triggers `drawableSizeWillChange` callback
- ConfigManager gets updated
- UI repositions via callback

### Why Main Menu Doesn't Self-Correct
- Main menu locks to portrait immediately
- No rotation allowed, so no `drawableSizeWillChange` callback
- UI created once with wrong dimensions
- No mechanism to detect and fix later

### The Core Issue
**MTKView drawable size is asynchronous and unpredictable. We need a synchronous way to set/get dimensions during initialization and orientation changes.**

---

## Questions for Further Investigation

1. Can we override MTKView.drawableSize setter to be synchronous?
2. Does `setNeedsLayout()` force immediate MTKView size update?
3. Should we use CAMetalLayer directly instead of MTKView?
4. Can ConfigManager subscribe to NotificationCenter for orientation changes?
5. Should we poll MTKView size in a loop until it matches expected orientation?

---

*Document created: 2025-10-02*
*Last updated: 2025-10-02*
