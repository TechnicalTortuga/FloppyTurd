# iOS Game Loop Implementation Plan for FloppyTurd

## Current Issue

The game is installing on iOS devices but immediately exiting on launch due to improper integration with the iOS app lifecycle and event system. Key issues identified:

1. The main iOS entry point in `main.cpp` directly calls `game_main()` without using `UIApplicationMain()`
2. `game_main()` is called asynchronously on a background queue in `GameViewController.mm`
3. The entire game loop runs once and exits, not responding to iOS display refresh events
4. `WindowShouldClose()` is not integrated with iOS lifecycle events

## Solution Architecture

We need to restructure how the game integrates with iOS, separating the game initialization from the game loop and synchronizing with the iOS display system, while maintaining a platform-agnostic core game logic.

### 1. Platform-Agnostic Game Architecture

**Current Structure:**
- `Game` constructor calls `RunGame()`
- `RunGame()` contains the entire game loop in a `while (!WindowShouldClose())` structure
- Core game logic is tightly coupled with the loop structure

**New Platform-Agnostic Structure:**
- `Game` class provides platform-independent interface:
  - `Initialize()`: One-time setup (platform-independent)
  - `Update(float deltaTime)`: Single frame update (platform-independent)
  - `Render()`: Single frame render (platform-independent)
  - `Shutdown()`: Resource cleanup (platform-independent)

**Platform-Specific Loop Implementations:**
- **Desktop/Raylib:** Uses traditional while loop in `RunGameDesktop()`
- **iOS:** Uses event-based frame updates via `CADisplayLink`

### 2. Platform-Specific Main Loop Implementations

#### Desktop Loop (Raylib-based)

```cpp
// In Game.cpp
void Game::RunGameDesktop()
{
    Initialize(); // One-time initialization
    
    // Traditional desktop game loop using Raylib
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        Update(deltaTime); // Core game logic (platform-agnostic)
        Render();          // Core rendering logic (platform-agnostic)
    }
    
    Shutdown(); // Cleanup
}
```

#### iOS Loop (CADisplayLink-based)

Update `main.cpp` to use `UIApplicationMain()` for iOS:

```objc
#if defined(PLATFORM_IOS)
int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
#endif
```

Modify `GameViewController.mm` to drive the game loop using iOS display system:

```objc
// In GameViewController.mm
- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Create and initialize game instance on appropriate queue
    _game = new Game(); // No auto-start of game loop
    
    dispatch_async(_gameQueue, ^{
        // Initialize game without starting main loop
        _game->Initialize(); // Platform-agnostic initialization
        
        dispatch_async(dispatch_get_main_queue(), ^{
            // Set up display link on main thread after initialization
            [self setupDisplayLink];
            _gameInitialized = YES;
        });
    });
}

- (void)setupDisplayLink {
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(gameLoopTick:)];
    [self.displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)gameLoopTick:(CADisplayLink *)sender {
    if (_gameInitialized && _game) {
        // Run a single iteration of game logic (platform-agnostic)
        float deltaTime = sender.targetTimestamp - sender.timestamp;
        _game->Update(deltaTime);
        
        // Rendering will trigger via MTKView's drawInMTKView method
        // Which will call _game->Render()
    }
}
```

### 3. Platform-Agnostic Game Interface

Modify `Game.h` to provide a clean platform-agnostic interface:

```cpp
// Game.h
class Game {
public:
    Game(); // Constructor no longer auto-starts game
    ~Game();
    
    // Platform-agnostic game interface
    void Initialize();          // Setup game systems and resources
    void Update(float deltaTime); // Process game logic for one frame
    void Render();             // Render a single frame
    void Shutdown();           // Cleanup resources
    
    // Platform-specific runners
    void RunGameDesktop();      // Traditional loop for desktop platforms
    
    // iOS lifecycle hooks
    void OnPause();  // App going to background
    void OnResume(); // App returning to foreground
    
private:
    // Game state and implementation details
};
```

### 4. iOS Lifecycle Integration

Integrate the game with iOS app lifecycle events via AppDelegate:

```cpp
// In RaylibCompat.h/cpp
extern "C" {
    void OnAppPause() {
        // Get game instance and pause it
        Game* gameInstance = GetGameInstance();
        if (gameInstance) gameInstance->OnPause();
    }
    
    void OnAppResume() {
        // Get game instance and resume it
        Game* gameInstance = GetGameInstance();
        if (gameInstance) gameInstance->OnResume();
    }
}
```

### 5. Metal Rendering Integration

Integrate with Metal rendering system in iOS:

```objc
// In GameViewController.mm
- (void)drawInMTKView:(MTKView *)view {
    if (_gameInitialized && _game) {
        // This is called by the display system when ready to render
        _game->Render(); // Calls platform-agnostic render code
    }
}
```

## Implementation Steps

1. **Create Game Loop Adapter Functions**
   - Extract initialization code from `game_main()`
   - Extract single-frame update code from `RunGame()`
   - Create C linkage functions for Objective-C to call

2. **Update Game Class**
   - Modify Game constructor to not automatically start game loop
   - Add methods to support frame-by-frame execution

3. **Update iOS Entry Point**
   - Modify `main.cpp` to use `UIApplicationMain()` for iOS

4. **Enhance GameViewController**
   - Implement `CADisplayLink`-based game loop
   - Properly connect to game update functions

5. **Connect AppDelegate Lifecycle Events**
   - Ensure `OnAppPause()` and `OnAppResume()` properly pause/resume game

6. **Test and Debug**
   - Build and test on iOS device
   - Monitor for crashes and lifecycle issues
   - Verify performance and smooth frame rate

## Key Challenges

- **State Management**: Ensuring game state is properly preserved between frames
- **Timing**: Handling delta time and frame pacing correctly with `CADisplayLink`
- **Resource Management**: Properly handling iOS memory warnings and app state transitions
- **Input Handling**: Ensuring touch events map correctly to game input
- **Render Synchronization**: Coordinating Metal rendering with game state updates

## Success Criteria

- Game launches and runs continuously on iOS device
- Game properly handles being sent to background and resumed
- Game properly handles device rotation and resizing
- Game maintains consistent frame rate
- No memory leaks or resource issues
