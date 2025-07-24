# Floppy Turd - Revised Architecture Design
## Composition-Based Platform Agnostic System

## Core Insight
Instead of inheritance-based IPlatform interfaces (which don't work well with C++/Swift interop), we'll use **composition with function delegates** that can point to either iOS Swift implementations or Raylib implementations.

## New Architecture Overview

### 1. Platform-Agnostic Game Systems

```cpp
// Core game systems that don't care about platform
class FloppyTurdGame {
private:
    // Platform-agnostic delegates
    RendererDelegate m_renderer;
    InputDelegate m_input;
    AudioDelegate m_audio;
    
    // Core game logic
    std::unique_ptr<ECS> m_ecsSystem;
    std::unique_ptr<GameStateManager> m_stateManager;
    
public:
    bool Initialize();  // Single initialization point
    void Update(float deltaTime);
    void Render();
};
```

### 2. Delegate System (Function Pointer Approach)

```cpp
// Renderer delegate - can point to iOS or Raylib functions
struct RendererDelegate {
    void (*beginFrame)();
    void (*endFrame)();
    void (*clearScreen)(float r, float g, float b, float a);
    void (*drawSprite)(const Sprite& sprite, float x, float y);
    void (*drawText)(const char* text, float x, float y, float size);
    
    // Platform-specific context (iOS: Swift objects, Raylib: native)
    void* platformContext;
};

struct InputDelegate {
    bool (*isActionPressed)(int action);
    void (*getMousePosition)(float* x, float* y);
    void (*getTouchPosition)(float* x, float* y);  // iOS specific
    bool (*isKeyPressed)(int key);  // Desktop specific
    
    void* platformContext;
};

struct AudioDelegate {
    void (*playSound)(const char* soundName);
    void (*playMusic)(const char* musicName);
    void (*setVolume)(float volume);
    void (*stopMusic)();
    
    void* platformContext;
};
```

### 3. Platform-Specific Implementations

#### iOS Implementation (C++ wrapper functions)
```cpp
// iOS wrapper functions that call Swift
namespace iOSPlatform {
    void BeginFrame() {
        // Call Swift MetalRenderer.beginFrame() via platformContext
        auto* renderer = static_cast<void*>(g_iosRenderer);
        ios_metal_begin_frame(renderer);
    }
    
    void DrawSprite(const Sprite& sprite, float x, float y) {
        auto* renderer = static_cast<void*>(g_iosRenderer);
        ios_metal_draw_sprite(renderer, sprite.textureId, x, y, sprite.width, sprite.height);
    }
    
    bool IsActionPressed(int action) {
        auto* input = static_cast<void*>(g_iosInput);
        return ios_touch_is_action_pressed(input, action);
    }
    
    void PlaySound(const char* soundName) {
        auto* audio = static_cast<void*>(g_iosAudio);
        ios_audio_play_sound(audio, soundName);
    }
}
```

#### Raylib Implementation
```cpp
// Raylib wrapper functions
namespace RaylibPlatform {
    void BeginFrame() {
        BeginDrawing();
    }
    
    void DrawSprite(const Sprite& sprite, float x, float y) {
        DrawTexture(sprite.raylibTexture, x, y, WHITE);
    }
    
    bool IsActionPressed(int action) {
        switch(action) {
            case ACTION_JUMP: return IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            default: return false;
        }
    }
    
    void PlaySound(const char* soundName) {
        // Load and play Raylib sound
        PlaySound(GetSound(soundName));
    }
}
```

### 4. Platform Detection and Delegate Setup

```cpp
bool FloppyTurdGame::Initialize() {
    GN_LOG_INFO("Initializing Floppy Turd Game...");
    
    // Platform detection and delegate setup
#ifdef __APPLE__
#if TARGET_OS_IPHONE
    SetupIOSDelegates();
#else
    SetupRaylibDelegates();  // macOS
#endif
#else
    SetupRaylibDelegates();  // Desktop
#endif
    
    // Initialize core systems (platform-agnostic)
    if (!InitializeECS()) return false;
    if (!InitializeGameStates()) return false;
    
    // Test platform delegates
    m_renderer.beginFrame();
    m_renderer.clearScreen(0.2f, 0.3f, 0.3f, 1.0f);
    m_renderer.endFrame();
    
    m_initialized = true;
    return true;
}

void FloppyTurdGame::SetupIOSDelegates() {
    // iOS: Point delegates to iOS wrapper functions
    m_renderer.beginFrame = iOSPlatform::BeginFrame;
    m_renderer.endFrame = iOSPlatform::EndFrame;
    m_renderer.clearScreen = iOSPlatform::ClearScreen;
    m_renderer.drawSprite = iOSPlatform::DrawSprite;
    m_renderer.platformContext = g_iosRenderer;  // Swift object pointer
    
    m_input.isActionPressed = iOSPlatform::IsActionPressed;
    m_input.getTouchPosition = iOSPlatform::GetTouchPosition;
    m_input.platformContext = g_iosInput;
    
    m_audio.playSound = iOSPlatform::PlaySound;
    m_audio.playMusic = iOSPlatform::PlayMusic;
    m_audio.platformContext = g_iosAudio;
}

void FloppyTurdGame::SetupRaylibDelegates() {
    // Raylib: Point delegates to Raylib wrapper functions
    m_renderer.beginFrame = RaylibPlatform::BeginFrame;
    m_renderer.endFrame = RaylibPlatform::EndFrame;
    m_renderer.clearScreen = RaylibPlatform::ClearScreen;
    m_renderer.drawSprite = RaylibPlatform::DrawSprite;
    m_renderer.platformContext = nullptr;  // Raylib is global
    
    m_input.isActionPressed = RaylibPlatform::IsActionPressed;
    m_input.getMousePosition = RaylibPlatform::GetMousePosition;
    m_input.platformContext = nullptr;
    
    m_audio.playSound = RaylibPlatform::PlaySound;
    m_audio.playMusic = RaylibPlatform::PlayMusic;
    m_audio.platformContext = nullptr;
}
```

### 5. Game Logic (Platform-Agnostic)

```cpp
void FloppyTurdGame::Update(float deltaTime) {
    // Game logic doesn't care about platform
    if (m_input.isActionPressed(ACTION_JUMP)) {
        // Make turd jump
        m_turd.Jump();
    }
    
    // Update physics, collision, etc.
    m_ecsSystem->Update(deltaTime);
}

void FloppyTurdGame::Render() {
    m_renderer.beginFrame();
    m_renderer.clearScreen(0.5f, 0.8f, 1.0f, 1.0f);  // Sky blue
    
    // Render game objects
    m_renderer.drawSprite(m_turdSprite, m_turd.x, m_turd.y);
    m_renderer.drawSprite(m_pipeSprite, m_pipe.x, m_pipe.y);
    
    // Render UI
    char scoreText[32];
    sprintf(scoreText, "Score: %d", m_score);
    m_renderer.drawText(scoreText, 10, 10, 24);
    
    m_renderer.endFrame();
}
```

### 6. iOS Integration

#### Swift to C++ Bridge Functions
```swift
// In GameEngine.swift
class GameEngine {
    private var metalRenderer: MetalRenderer
    private var touchInputHandler: TouchInputHandler
    private var audioHandler: AudioManagerSwift
    private var cppGame: FloppyTurdGame?
    
    func initialize() {
        // Create Swift components
        metalRenderer = MetalRenderer()
        touchInputHandler = TouchInputHandler()
        audioHandler = AudioManagerSwift()
        
        // Set global pointers for C++ to access
        SetIOSComponents(
            Unmanaged.passUnretained(metalRenderer).toOpaque(),
            Unmanaged.passUnretained(touchInputHandler).toOpaque(),
            Unmanaged.passUnretained(audioHandler).toOpaque()
        )
        
        // Create and initialize C++ game
        cppGame = FloppyTurdGame()
        cppGame?.Initialize()  // Will auto-detect iOS and setup delegates
    }
}
```

#### C++ Bridge Functions
```cpp
// Global iOS component pointers
void* g_iosRenderer = nullptr;
void* g_iosInput = nullptr;
void* g_iosAudio = nullptr;

// Called from Swift to set component pointers
extern "C" void SetIOSComponents(void* renderer, void* input, void* audio) {
    g_iosRenderer = renderer;
    g_iosInput = input;
    g_iosAudio = audio;
}

// iOS wrapper functions that call Swift
extern "C" void ios_metal_begin_frame(void* renderer) {
    // Call Swift MetalRenderer.beginFrame()
    // Implementation depends on Swift interop method
}

extern "C" void ios_metal_draw_sprite(void* renderer, int textureId, float x, float y, float w, float h) {
    // Call Swift MetalRenderer.drawSprite()
}

extern "C" bool ios_touch_is_action_pressed(void* input, int action) {
    // Call Swift TouchInputHandler.isActionPressed()
    return false; // Placeholder
}
```

## Benefits of This Approach

### 1. **Platform Agnostic Game Logic**
- Game code doesn't know or care about platform
- Same Update() and Render() logic for all platforms
- Easy to test and debug

### 2. **No Inheritance Issues**
- Uses composition instead of inheritance
- Function pointers work perfectly with C++/Swift interop
- No virtual function table complications

### 3. **Single Initialization Point**
- One Initialize() method that auto-detects platform
- Clean, predictable initialization flow
- Easy to add new platforms

### 4. **Performance**
- Direct function calls (no virtual dispatch)
- Minimal overhead
- Platform-specific optimizations possible

### 5. **Maintainability**
- Clear separation of concerns
- Platform-specific code isolated in wrapper functions
- Easy to modify or extend platform support

## Implementation Plan

1. **Define delegate structures** in new header file
2. **Implement iOS wrapper functions** that call Swift
3. **Implement Raylib wrapper functions** that call Raylib
4. **Refactor FloppyTurdGame** to use delegates instead of direct calls
5. **Update iOS GameEngine.swift** to set global component pointers
6. **Test on both iOS and desktop** to ensure consistency

This approach gives us the best of both worlds: platform abstraction without inheritance complexity, and direct performance without virtual function overhead!