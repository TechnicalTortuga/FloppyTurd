# FloppyTurd iOS Render Loop Refactor Plan

## Overview
The current issue is that `GameView` subclasses `MTKView` but also creates an internal `_mtkView`, causing confusion and preventing the render loop from running. We need to refactor to use a single MTKView pattern.

## Refactor Strategy
We'll use **Option 1: Subclass MTKView** approach, where `GameView` is a proper MTKView subclass and acts as its own delegate.

---

## Step-by-Step Refactor Plan

### Step 1: Update GameView.h
**File:** `FloppyTurd/iOS/GameView.h`

**Changes:**
- Remove the `initWithFrame:device:` method (we'll use the standard MTKView init)
- Add `<MTKViewDelegate>` to the interface declaration
- Remove any references to internal MTKView

**New GameView.h:**
```objc
#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#import <MetalKit/MetalKit.h>
#import "PlatformAPI.h"

// Forward declarations
class MetalRenderer;

@interface GameView : MTKView <MTKViewDelegate>

// Initialization
- (instancetype)initWithFrame:(CGRect)frame game:(Game *)game;

// Rendering
- (void)renderFrame; // Called by Game::RenderFrame

// Input querying (for Game/TouchControls)
- (Vector2)getPrimaryTouchPosition;
- (BOOL)isPrimaryTouchDown;
- (BOOL)isPrimaryTouchPressed;
- (BOOL)isPrimaryTouchReleased;
- (std::vector<Vector2>)getTouchPoints;

// Internal MetalRenderer access (if needed)
- (MetalRenderer *)getMetalRenderer;

// Metal device and command queue access
- (id<MTLDevice>)getMetalDevice;
- (id<MTLCommandQueue>)getMetalCommandQueue;

@end

#endif // GAME_VIEW_H
```

### Step 2: Refactor GameView.mm - Remove Internal MTKView
**File:** `FloppyTurd/iOS/GameView.mm`

**Changes:**
- Remove `_mtkView` property and all references to it
- Make `GameView` act as its own MTKView delegate
- Update initialization to use standard MTKView init
- Move all MTKView setup to the main view

**Key Changes:**
1. Remove `MTKView *_mtkView;` from the interface
2. Update `initWithFrame:game:` to call `[super initWithFrame:frame device:device]`
3. Set `self.delegate = self;` instead of `_mtkView.delegate = self;`
4. Remove `[self addSubview:_mtkView];`
5. Update all `_mtkView` references to `self`

### Step 3: Update GameViewController.mm
**File:** `FloppyTurd/iOS/GameViewController.mm`

**Changes:**
- Update `GameView` initialization to pass the Metal device
- Ensure the view is properly configured

**In `-loadView`:**
```objc
- (void)loadView {
    // Create Metal device first
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"[ERROR] Metal is not supported on this device");
        return;
    }
    
    // Create and configure the GameView with device
    _gameView = [[GameView alloc] initWithFrame:[[UIScreen mainScreen] bounds] game:GetGameInstance()];
    self.view = _gameView;
    
    // Set the global GameView pointer for RaylibCompat_iOS performance optimization
    SetGlobalGameView(_gameView);
    NSLog(@"[INIT] SetGlobalGameView called with GameView: %p", _gameView);
}
```

### Step 4: Update MetalRenderer Initialization
**File:** `FloppyTurd/MetalRenderer.mm`

**Changes:**
- Update `MetalRenderer::Initialize()` to work with the refactored GameView
- Ensure it uses the correct MTKView instance

### Step 5: Update Safe Area Handling
**File:** `FloppyTurd/iOS/GameView.mm`

**Changes:**
- Remove `updateSafeArea` method that was positioning the internal `_mtkView`
- The main view will handle safe area automatically
- Update `layoutSubviews` to handle safe area if needed

### Step 6: Update Touch Handling
**File:** `FloppyTurd/iOS/GameView.mm`

**Changes:**
- Ensure touch handling works with the main view
- Update touch coordinate calculations if needed

### Step 7: Update Render Method
**File:** `FloppyTurd/iOS/GameView.mm`

**Changes:**
- Update `-render` method to call `[self draw]` instead of `[_mtkView draw]`
- Ensure `drawInMTKView:` is properly implemented

**New render method:**
```objc
- (void)render {
    if (_isInitialized) {
        [self draw];
    }
}
```

### Step 8: Clean Up Dealloc
**File:** `FloppyTurd/iOS/GameView.mm`

**Changes:**
- Remove `_mtkView = nil;` from dealloc
- Keep other cleanup code

---

## Testing Steps

### Step 1: Build Test
```bash
xcodebuild -project FloppyTurd.xcodeproj -scheme FloppyTurd -destination 'platform=iOS Simulator,name=iPhone 16' build
```

### Step 2: Install and Launch
```bash
xcrun simctl install booted Debug-iphonesimulator/FloppyTurd.app
xcrun simctl launch booted com.floppyturd.game
```

### Step 3: Check Logs
Look for these TraceLog entries:
- `[GameView] drawInMTKView: Starting frame render`
- `[GameView] drawInMTKView: Frame render completed`
- Metal renderer debug logs

### Step 4: Visual Test
- App should display content (even if textures are missing)
- Should see some rendering output
- Touch input should work

---

## Expected Results After Refactor

1. **TraceLogs should appear:** `[GameView] drawInMTKView` logs should be visible
2. **Rendering should work:** Even with missing textures, basic shapes should render
3. **Game loop should run:** CADisplayLink should trigger the render loop
4. **Touch input should work:** Touch events should be processed

---

## Rollback Plan

If issues arise:
1. Keep backup of original files
2. Revert changes one step at a time
3. Test after each reversion to identify the problematic change

---

## Files to Modify (in order)

1. `FloppyTurd/iOS/GameView.h` - Interface changes
2. `FloppyTurd/iOS/GameView.mm` - Implementation refactor
3. `FloppyTurd/iOS/GameViewController.mm` - Initialization updates
4. `FloppyTurd/MetalRenderer.mm` - Renderer updates (if needed)

---

## Success Criteria

- [ ] App builds without errors
- [ ] App launches without crashes
- [ ] `[GameView] drawInMTKView` TraceLogs appear in logs
- [ ] Some visual output is displayed (even if textures are missing)
- [ ] Touch input responds
- [ ] Game loop runs continuously (60 FPS)

---

## Next Steps After Successful Refactor

1. Fix texture loading issues (asset catalog vs file paths)
2. Implement proper resource management
3. Add error handling and recovery
4. Optimize performance
5. Add debug overlays and visual feedback 