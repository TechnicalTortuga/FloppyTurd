 # PlatformAPI Audit Report & Action Plan

## Executive Summary

This audit identifies all files in the FloppyTurd codebase that use Raylib types and functions, ensuring proper header inclusion and clean architecture. The goal is to maintain a unified API while properly separating Raylib (desktop) and Metal (iOS) implementations.

**Audit Status:** COMPLETED
**Total Files Analyzed:** 187+ source files
**Files Requiring Updates:** 50+ files need raylib.h includes

---

## Current State Analysis

### ✅ CORRECTLY CONFIGURED FILES
These files already have proper raylib.h includes:

**Header Files:**
- `PlatformAPI.h` - ✅ Includes raylib.h
- `PlatformSpecific.h` - ✅ Includes raylib.h  
- `FontCache.h` - ✅ Includes raylib.h
- `AIGUI.h` - ✅ Includes raylib.h

**Source Files:**
- `PlatformAPI.cpp` - ✅ Includes PlatformRaylib.h

### ⚠️ FILES REQUIRING UPDATES
These files use Raylib types but don't include raylib.h:

---

## Detailed File Audit

### 1. Core Game Files (HIGH PRIORITY)

#### `Game.cpp`
**Raylib Types Used:** Vector2, Color, Rectangle, Texture2D, Font, Sound, Music
**Current Includes:** PlatformAPI.h, PlatformTypes.h
**Action Required:** Add `#include "raylib.h"`

#### `Playing.cpp` 
**Raylib Types Used:** Vector2, Color, Rectangle, Texture2D, Font
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `MainMenu.cpp`
**Raylib Types Used:** Vector2, Color, Rectangle, Texture2D, Font, Sound, Music
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `Loading.cpp`
**Raylib Types Used:** Vector2, Color, Rectangle, Texture2D, Font
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

### 2. Resource Management Files

#### `ResourceManager.cpp`
**Raylib Types Used:** Texture2D, Image, Font, Sound, Music
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `TextureAtlas.cpp`
**Raylib Types Used:** Texture2D, Image, Rectangle, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `TextureCache.cpp`
**Raylib Types Used:** Texture2D, Image
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

### 3. Audio System Files

#### `AudioClip.h`
**Raylib Types Used:** Music, Sound
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `SoundManager.cpp`
**Raylib Types Used:** Sound
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `AudioStateManager.cpp`
**Raylib Types Used:** Music, Sound
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

### 4. Rendering & Graphics Files

#### `Sprite.cpp`
**Raylib Types Used:** Texture2D, Rectangle, Vector2, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `AnimatedLayer.cpp`
**Raylib Types Used:** Texture2D, Rectangle, Vector2, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `StaticLayer.cpp`
**Raylib Types Used:** Texture2D, Rectangle, Vector2, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `ParallaxLayer.cpp`
**Raylib Types Used:** Texture2D, Rectangle, Vector2, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

### 5. UI System Files

#### `TouchControls.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `UICoordinateSystem.cpp`
**Raylib Types Used:** Rectangle, Vector2
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `PerformanceProfiler.cpp`
**Raylib Types Used:** Texture2D, Rectangle, Vector2, Color, Font
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

### 6. Game Object Files

#### `Player.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D, Sound
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `Enemy.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `SnowmanEnemy.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `Projectile.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

### 7. Level System Files

#### `LevelManager.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `BossLevel.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D, Font
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `DesertLevel.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `SnowLevel.cpp`
**Raylib Types Used:** Vector2, Rectangle, Color, Texture2D
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

### 8. Utility Files

#### `Window.cpp`
**Raylib Types Used:** Rectangle, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

#### `FontAtlasGenerator.mm`
**Raylib Types Used:** Image, Rectangle, Color
**Current Includes:** PlatformAPI.h
**Action Required:** Add `#include "raylib.h"`

---

## Action Plan

### Phase 1: Header Inclusion Fixes (IMMEDIATE)

**Priority 1 - Core Game Files:**
```bash
# Add raylib.h to core game files
echo '#include "raylib.h"' >> Game.cpp
echo '#include "raylib.h"' >> Playing.cpp  
echo '#include "raylib.h"' >> MainMenu.cpp
echo '#include "raylib.h"' >> Loading.cpp
```

**Priority 2 - Resource Management:**
```bash
# Add raylib.h to resource files
echo '#include "raylib.h"' >> ResourceManager.cpp
echo '#include "raylib.h"' >> TextureAtlas.cpp
echo '#include "raylib.h"' >> TextureCache.cpp
```

**Priority 3 - Audio System:**
```bash
# Add raylib.h to audio files
echo '#include "raylib.h"' >> AudioClip.h
echo '#include "raylib.h"' >> SoundManager.cpp
echo '#include "raylib.h"' >> AudioStateManager.cpp
```

### Phase 2: iOS Implementation Organization

**Current Issues in PlatformAPI.h:**
1. Mixed desktop and iOS code
2. Inline implementations scattered throughout
3. Some function signatures may not match Raylib exactly

**Action Items:**
1. Move all iOS-specific inline implementations to PlatformIOS.cpp
2. Clean up PlatformAPI.h to only contain:
   - Function declarations (Raylib-compatible signatures)
   - Platform detection macros
   - Platform-specific implementation access
3. Ensure all function signatures match Raylib exactly

### Phase 3: Metal Implementation Cleanup

**PlatformIOS.cpp Organization:**
```cpp
// ============================================================================
// iOS METAL IMPLEMENTATION
// ============================================================================

class PlatformIOS : public PlatformSpecific {
private:
    // Metal-specific members
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLRenderPipelineState> m_pipelineState;
    
    // Audio members
    NSMutableDictionary<NSString*, AVAudioPlayer*>* m_soundPlayers;
    AVAudioPlayer* m_musicPlayer;
    
public:
    // Window management
    void Initialize(void* nativeView) override;
    void Initialize() override;
    void Shutdown() override;
    
    // Drawing functions (Metal implementations)
    void DrawRectangle(int posX, int posY, int width, int height, Color color) override;
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint) override;
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color) override;
    
    // Audio functions (AVAudioPlayer implementations)
    void* LoadSound(const char* fileName) override;
    void PlaySound(void* sound) override;
    void* LoadMusic(const char* fileName) override;
    void PlayMusic(void* music) override;
    
    // Input functions (Touch to Mouse/Keyboard mapping)
    bool IsPrimaryInputDown() override;
    Vector2 GetPrimaryInputPosition() override;
};
```

### Phase 4: Function Coverage Analysis

**Raylib Functions Used in FloppyTurd:**
Based on the audit, these are the main Raylib functions used:

**Window Management:**
- `InitWindow`, `CloseWindow`, `WindowShouldClose`
- `GetScreenWidth`, `GetScreenHeight`
- `SetTargetFPS`, `GetFrameTime`

**Drawing:**
- `BeginDrawing`, `EndDrawing`, `ClearBackground`
- `DrawRectangle`, `DrawRectangleRec`, `DrawRectangleLinesEx`
- `DrawTexture`, `DrawTextureEx`, `DrawTextureRec`, `DrawTexturePro`
- `DrawText`, `DrawTextEx`, `MeasureText`, `MeasureTextEx`
- `DrawCircle`, `DrawLine`, `DrawLineEx`

**Textures:**
- `LoadTexture`, `UnloadTexture`, `SetTextureWrap`
- `LoadImage`, `UnloadImage`, `ImageResize`, `ImageDraw`
- `LoadTextureFromImage`

**Audio:**
- `LoadSound`, `UnloadSound`, `PlaySound`, `SetSoundVolume`
- `LoadMusic`, `UnloadMusic`, `PlayMusic`, `StopMusic`, `UpdateMusic`
- `IsMusicPlaying`, `SetMusicVolume`, `PauseMusic`, `ResumeMusic`

**Input:**
- `IsMouseButtonDown`, `IsMouseButtonReleased`
- `IsKeyPressed`, `IsKeyDown`
- `GetMousePosition`, `GetMouseDelta`

**Math & Utilities:**
- `Vector2Add`, `V