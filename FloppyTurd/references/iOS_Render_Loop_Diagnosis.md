# FloppyTurd iOS Render Loop Diagnosis & Step-Through

## Relevant Files for Chief Examination

- `FloppyTurd/main_ios.mm` (iOS entry point)
- `FloppyTurd/iOS/AppDelegate.h/.mm` (App delegate, window & root controller setup)
- `FloppyTurd/iOS/GameViewController.h/.mm` (View controller, game loop, Metal device)
- `FloppyTurd/iOS/GameView.h/.mm` (MTKView subclass, Metal renderer, delegate logic)
- `FloppyTurd/MetalRenderer.h/.mm` (Metal rendering backend)
- `FloppyTurd/Game.h/.cpp` (Game logic and render entry)

---

## Full Logic Step-Through: iOS Initialization to Render

### 1. **Entry Point: `main_ios.mm`**
- Calls `UIApplicationMain` with `AppDelegate` as the delegate.

### 2. **App Delegate: `AppDelegate.mm`**
- In `application:didFinishLaunchingWithOptions:`:
  - Creates a `UIWindow`.
  - Instantiates `GameViewController` and sets as root view controller.
  - Makes window key and visible.
  - Starts haptics manager.

### 3. **Game View Controller: `GameViewController.mm`**
- `-loadView`:
  - Creates a `GameView` with screen bounds and the current `Game` instance.
  - Sets `self.view = _gameView`.
- `-viewDidLoad`:
  - Initializes Metal device.
  - Initializes UIManager with screen and safe area info.
  - Calls `game_main(0, nullptr)` to initialize the game.
  - If successful, calls `[self startGameLoop]`.
- `-startGameLoop`:
  - Calls `setupDisplayLink`.
- `-setupDisplayLink`:
  - Creates a `CADisplayLink` targeting `self` and selector `gameLoopTick:`.
  - Sets preferred FPS to 60.
  - Adds the display link to the run loop.
- `-gameLoopTick:`:
  - Checks if the game is initialized.
  - Gets the `Game` instance.
  - Calculates delta time.
  - Calls `game->UpdateFrame(deltaTime)` on a background queue.
  - On the main queue, calls `[_gameView render]` to trigger a render.

### 4. **Game View: `GameView.mm`**
- Subclasses `MTKView` but also creates an internal `_mtkView` (another `MTKView`).
- `_mtkView.delegate = self;` — GameView acts as the MTKViewDelegate for the internal view.
- Metal device and command queue are created.
- A `MetalRenderer` is created and initialized with `_mtkView`.
- `-render` calls `[_mtkView draw]`, which triggers the delegate method.
- `-drawInMTKView:` (MTKViewDelegate):
  - If initialized, calls:
    - `_renderer->BeginFrame();`
    - `_game->RenderFrame();`
    - `_renderer->EndFrame();`
    - `_renderer->Present();`
  - TraceLogs at start/end.

### 5. **Game/Renderer**
- `Game` instance is responsible for game logic and rendering.
- `MetalRenderer` handles Metal draw calls.

---

## Key Findings

- **Render loop is set up via CADisplayLink in `GameViewController`, which calls `[_gameView render]` on each tick.**
- **GameView is the MTKViewDelegate for its internal `_mtkView`.**
- **`drawInMTKView:` is implemented and should be called by MetalKit when `[_mtkView draw]` is called.**
- **TraceLogs are present in `drawInMTKView:` and MetalRenderer, but none are appearing in the logs, indicating the delegate method is not being called.**
- **No `setDelegate` for the top-level `GameView` itself, but `_mtkView.delegate = self` is set in `setupView`.**
- **There is a possible confusion: `GameView` is a subclass of `MTKView`, but also creates and manages its own `_mtkView` subview, which is not standard practice.**

---

## Identified Issues

- **Double MTKView:** `GameView` subclasses `MTKView` but also creates a `_mtkView` property that is another `MTKView` instance. This can cause confusion and may prevent the correct delegate from being called, or the correct view from being displayed.
- **Delegate Not Called:** If the visible view is the outer `GameView` (a `MTKView`), but the delegate is set on the internal `_mtkView`, the render loop will not run.
- **No TraceLogs:** The absence of `[GameView] drawInMTKView` logs confirms the delegate is not being triggered.

---

## Recommendations

1. **Refactor `GameView` to use a single `MTKView`:**
   - Either subclass `MTKView` and use `self` as the delegate, or
   - Use a contained `MTKView` and do not subclass `MTKView`.
   - Do not mix both patterns.
2. **Ensure the visible view is the one with the delegate set:**
   - If `GameView` is a subclass of `MTKView`, set its delegate to `self` and do not create an internal `_mtkView`.
   - If using a contained `_mtkView`, ensure it is the one added to the view hierarchy and is visible.
3. **Add TraceLogs to confirm which view is being displayed and which delegate is being called.**

---

## Example: Correct Pattern

```objc
// Option 1: Subclass MTKView
@interface GameView : MTKView <MTKViewDelegate>
@end

@implementation GameView
- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device {
    self = [super initWithFrame:frame device:device];
    if (self) {
        self.delegate = self;
        // ... other setup ...
    }
    return self;
}
@end
```

---

## Conclusion

- The current setup creates two MTKViews, which likely prevents the render loop from running.
- Refactor to use a single MTKView and ensure the delegate is set on the visible view.
- This should restore the render loop and allow rendering and TraceLogs to function as expected. 