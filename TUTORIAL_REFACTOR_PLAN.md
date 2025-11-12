# Tutorial State Refactor Plan

## Issues Identified

1. **How To button not hidden** when other menu buttons are hidden
2. **UI layout mismatch** with GameplayState:
   - Need pipe counter at top center
   - Need coin bag + coin count at bottom left
   - Need shooting pad at bottom right  
3. **Player turd not visible** - likely not using proper sprite/texture loading
4. **Background not visible** - not using background layer system
5. **Settings button missing** - need top right corner settings button
6. **Pause menu missing** - need minimal pause menu with "Return to Main Menu" only

## Root Cause Analysis

The TutorialState is trying to manually create entities instead of leveraging the existing GameplayState systems:

- **Background**: GameplayState uses `CreateBackgroundLayers()` with `LevelConfig`
- **Player**: GameplayState uses `PlayerControllerSystem` with proper sprite loading
- **UI**: GameplayState has established patterns for all UI elements
- **Pause**: GameplayState uses `PauseSystem` for pause functionality

## Recommended Solution

**Make TutorialState a minimal implementation of GameplayState** with these differences:

### What to Keep from GameplayState:
1. ✅ **PlayerControllerSystem** - handles player entity, sprites, animations, jump, shoot
2. ✅ **Background layers** - use level config to load proper background
3. ✅ **UI System** - reuse CreateUI() pattern for consistent layout
4. ✅ **PauseSystem** - for settings button and minimal pause menu
5. ✅ **SpriteSystem** - for sprite rendering
6. ✅ **RenderSystem** - for screen dimensions

### What to Restrict/Modify:
1. ⚠️ **Player movement** - constrain to bottom 1/2 or 1/4 of screen
2. ⚠️ **No enemies** - tutorial doesn't need enemy spawning
3. ⚠️ **No obstacles** - tutorial doesn't need pipe obstacles
4. ⚠️ **No actual shooting** - just show the shooting pad
5. ⚠️ **Simplified pause menu** - just "Return to Main Menu" button
6. ⚠️ **Fixed coins** - show "999" coins always

### Implementation Approach:

**Option A: Reuse GameplayState with Tutorial Mode**
- Add `m_isTutorialMode` flag to GameplayState
- Skip enemy/obstacle spawning when in tutorial mode
- Constrain player movement when in tutorial mode
- Show simplified UI when in tutorial mode

**Option B: TutorialState inherits GameplayState patterns**
- Copy GameplayState initialization code
- Use same systems (PlayerControllerSystem, SpriteSystem, etc.)
- Implement simplified Update() that skips spawning
- Override pause menu to show minimal version

**Option C: Create Tutorial "Level"**  
- Create a special level config (Level 0 or Level 99)
- Set it up with tutorial-specific settings
- Use GameplayState as-is with this special level
- Level config controls what systems are active

## Recommended: Option C - Tutorial Level

This is the cleanest approach that avoids code duplication:

```cpp
// In LevelManager or level configs:
LevelConfig tutorialLevel;
tutorialLevel.levelId = 0; // Special tutorial level
tutorialLevel.levelName = "Tutorial";
tutorialLevel.spawnObstacles = false; // No obstacles
tutorialLevel.spawnEnemies = false; // No enemies
tutorialLevel.coinsEnabled = true; // Show coins (fixed at 999)
tutorialLevel.shootingEnabled = true; // Show shooting pad
tutorialLevel.backgroundLayers = { /* use level 1 background */ };
tutorialLevel.playerConstraints.minY = screenHeight * 0.50f; // Bottom half only
tutorialLevel.playerConstraints.maxY = screenHeight * 0.90f;
```

Then TutorialState becomes:
```cpp
class TutorialState : public GameplayState {
public:
    TutorialState(ECS* ecs, PlatformDelegates* pd) 
        : GameplayState(ecs, pd, 0) { // Level 0 = tutorial
        m_isTutorialMode = true;
    }
    
    void Enter() override {
        GameplayState::Enter();
        // Override coin count to show 999
        FixCoinCountAt999();
        // Override pause menu to be minimal
        SetPauseMenuMinimal(true);
    }
};
```

## Next Steps

1. **Fix MainMenuState** - Add `m_howToButtonEntity` to `SetMainMenuVisible()`
2. **Create Tutorial Level Config** - Define level 0 with tutorial settings  
3. **Refactor TutorialState** - Inherit from or reuse GameplayState systems
4. **Test rendering** - Ensure background and player are visible
5. **Test UI** - Ensure all UI elements match GameplayState layout
6. **Test pause menu** - Ensure minimal pause menu works

## Files to Modify

1. `/src/FloppyTurd/States/MainMenuState.cpp` - Add How To button to visibility
2. `/src/FloppyTurd/States/TutorialState.h` - Refactor to use GameplayState systems
3. `/src/FloppyTurd/States/TutorialState.cpp` - Reimplement using GameplayState patterns
4. `/src/FloppyTurd/Systems/LevelManager.cpp` - Add tutorial level config
5. `/src/FloppyTurd/Systems/PauseSystem.h` - Add minimal mode flag
