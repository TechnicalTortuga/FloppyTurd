# Rat King Boss Level Fix Report

## Executive Summary
This report analyzes the current implementation of the Rat King boss in the FloppyTurd game, focusing on the reported issues: non-rotating arms, misplaced/invisible lock-on dots, non-playing hurt animation, boss health bar not updating properly, partial rendering support, and state machine robustness. It draws comparisons to working systems like castle spike ball rotations and provides detailed suggestions for fixes and improvements. No code changes are implemented here; this is purely advisory.

The analysis is based on semantic searches of the codebase, particularly in `BossSystem.cpp/h`, `BossHealthBar.cpp/h`, `ObstacleSystem.cpp` (for spike balls), and `MetalRenderer.swift` (for rendering).

Key findings:
- Arms rotation exists but may not be rendering correctly due to missing pivot rotation support.
- Dots visibility issues likely stem from positioning calculations and screen coordinate mismatches.
- Hurt animation sets the sprite but may not animate due to missing animation logic in hurt state.
- Boss bar updates health but lacks "ticking" animation; hurt effect is present but may need enhancement.
- Partial rendering (source/dest rectangles) is already supported in Metal renderer via `drawSpriteScaledWithSource` and is needed specifically for the boss health bar components (frame, red health fill, white hurt effect).
- State machine is basic enum-based; can be improved for better robustness.

Implementation plan prioritizes partial rendering confirmation, then fixes in order of dependency (e.g., state machine before specific behaviors).

## 1. Partial Rendering Setup (Source/Dest Rectangles)
### Current Status
- The Metal renderer in `src/iOS/Rendering/MetalRenderer.swift` already supports partial texture rendering:
  - Function `drawSpriteScaledWithSource` takes `sourceX`, `sourceY`, `sourceWidth`, `sourceHeight` parameters.
  - It calculates pixel-perfect UV coordinates to prevent bleeding artifacts.
  - Used for sprite sheets and animated sprites.
  - Example usage: Handles UV flipping for horizontal/vertical flips and half-pixel offsets for crisp rendering.
- Destination rectangles are handled via transformation matrices (model-view-projection) for positioning and scaling.
- No major gaps found; the system seems ready for partial rendering, but usage in BossHealthBar needs verification/implementation as per old scripts.

### Issues and Suggestions
- **Potential Issue**: Boss health bar (frame, red fill, white hurt) might need source rects for partial drawing, especially if textures are atlased. Confirm against old raylib implementation in /oldscripts.
- **Suggestions**:
  - Confirm integration: In `BossSystem::UpdateSprites`, ensure each sprite component uses source rects for frame selection (e.g., `sprite.sourceX = frameIndex * frameWidth`).
  - Extend if needed: If multi-frame textures are used, add source rect params to render calls.
  - Test: Build and run, log UV coords during aiming/throwing to verify partial rendering.
  - Why first? Many issues (animations, dots) depend on correct rendering pipeline.

## 2. Arms Not Rotating
### Current Status
- Arms are separate entities (`backArmEntity`, `frontArmEntity`) with their own sprites.
- Rotation logic in `BossSystem::UpdateArmRotations()` (called in AIMING state):
  - Calculates angle to player from shoulder pivot.
  - Sets `transform.rotation` on arm entities.
- Pivot-based rotation is implemented in the project (e.g., spikeballs), so arms should use similar setup.

### Comparison to Spike Balls
- In `ObstacleSystem::SpawnCastleSpikeBall` (castle level):
  - Uses `PivotRotationRenderer` component: Specifies pivot offsets (e.g., `PivotRotationRenderer(true, 0.0f, -45.0f, 180.0f)`).
  - Updates rotation in `UpdateSpikeBallRotations`: Increments `transform.rotation` and adjusts hitbox around pivot.
  - This allows oscillation around a base pivot, which works for spike balls.

### Issues and Suggestions
- **Potential Issue**: While pivot rotation is implemented, there might be an inconsistency in arm setup (e.g., wrong pivot offsets, missing component, or rendering ignoring pivot) compared to spikeballs.
- **Suggestions**:
  - Adopt pivot rotation: Add `PivotRotationRenderer` to arm entities in `BossSystem::InitializeForLevel`.
    - Set pivot to shoulder offset (e.g., from sprite analysis: ~center of torso sprite).
    - Example: `PivotRotationRenderer(true, shoulderOffsetX, shoulderOffsetY, rotationSpeed)`.
  - Update logic: In `UpdateArmRotations`, calculate angle and set `transform.rotation` relative to pivot.
  - Sync with state: Ensure arms visible only in AIMING/THROWING states (already handled in `ChangeState`).
  - Test: Compare to spike balls; log arm positions/rotations during aiming.

## 3. Dots Placed Too Far Right and Not Visible
### Current Status
- Lock-on dots: Vector of `LockOnDot` structs, rendered as `UIShape` entities.
- In `BossSystem::UpdateLockOnIndicator()` (called in AIMING):
  - Calculates positions along aiming line from shoulder pivot.
  - Adds oscillation for visual effect.
  - Sets color from yellow to red based on progress.
  - Visibility set based on aim progress.
- Dots created in `CreateLockOnDotEntities()`: 20 circle shapes, scaled small.

### Issues and Suggestions
- **Potential Issues**:
  - Positioning: Calculations use `shoulderPivot` which might be offset (e.g., too far right due to landscape mode or scale=8.0f).
  - Visibility: Dots hidden in non-AIMING states; might not show if aimTimer < aimDuration or progress calc off.
  - Screen coords: Assumes portrait; in landscape (boss level), X positions might exceed screen.
- **Suggestions**:
  - Adjust pivot: In `AimingData`, set `shoulderPivot` dynamically based on screenWidth (e.g., `shoulderPivot.x = position.x + (64.0f * scale)` for 128px sprite center).
  - Fix positioning: Clamp dot positions to screen bounds; reduce spacing if too far right.
  - Enhance visibility: Add alpha fade-in; ensure `dotShape->visible = true` when progress > 0.
  - Debug: Log dot positions during aiming; if off-screen, offset by -screenWidth/4 or similar.

## 4. Hurt Animation Not Playing
### Current Status
- In `HandleDamage`: If health >0, `ChangeState(RatKingState::HURT)`, sets `hurtBuffer=24`, `hurtFlashTimer=0.0f`.
- In `ChangeState(HURT)`: Sets sprite to `hurtSprite` (6-frame), hides arms/dots.
- In `HandleHurt`: Increments `hurtTimer`; after 0.9s (6 frames @0.15s), back to IDLE.
- Animation advancement is handled by the SpriteSystem, so setting the sprite should trigger it.

### Issues and Suggestions
- **Potential Issue**: Sprite is set but animation might not trigger if SpriteSystem requires additional flags (e.g., isAnimated=true, reset currentFrame) or if hurtSprite config is incorrect.
- **Suggestions**:
  - Add animation logic: In `HandleHurt`, manually advance `sprite.currentFrame` based on timer (e.g., `currentFrame = int(hurtTimer / 0.15f) % 6`).
  - Integrate with StateAnimation: If using `StateAnimation` component (seen in EnemySystem), add hurt clip and switch via `changeState("hurt")`. Ensure SpriteSystem advances frames during HURT state.
  - Flash effect: Ensure `IsHurtFlashing()` toggles color/tint in renderer.
  - Test: Force damage in debug mode, verify frames play.

## 5. Boss Bar Not Ticking Away
### Current Status
- `BossHealthBar` class: Updates width of `m_healthFillEntity` based on `m_currentHealthPercent`.
- Hurt effect: Fades a shadow bar from old to new health over `HURT_FADE_DURATION`.
- Updated in `UpdateHealthValues` when health decreases.

### Issues and Suggestions
- **Potential Issue**: No smooth "ticking" animation; instant width change with fade might not match old scripts' gradual depletion.
- **Suggestions**:
  - Add tweening: In `Update`, interpolate `displayedHealth` towards actual health over time (e.g., lerp by deltaTime * speed).
  - Enhance hurt: Pulse the fade or add particle effects on damage.
  - Match old: If old used per-frame decrement, add similar in `HandleDamage`.
  - Visibility: Ensure bar shown when boss active (already checks `bossSystem->IsActive()`).

## 6. Rat King State Machine Robustness
### Current Status
- Enum-based (`RatKingState`) with `ChangeState` switching sprites/visibility.
- Handlers in `Update` for each state (e.g., `HandleAiming`, `HandleHurt`).
- Transitions: Timer-based (idle/walk), condition-based (aim to throw).

### Issues and Suggestions
- **Potential Issues**: Basic switch-case prone to missed states; no sub-states (e.g., aiming phases); hurt interrupts poorly handled.
- **Suggestions**:
  - Enhance to FSM: Use a map of state objects with enter/update/exit methods for modularity.
  - Add safeguards: Prevent invalid transitions (e.g., can't aim while hurt).
  - Sub-states: For AIMING, add LOCKING_ON sub-state for dot progression.
  - Events: Use event system for damage/music changes instead of inline.
  - Robustness: Add logging on every state change; timeout safeguards.

## Implementation Plan
1. **Verify Partial Rendering**: Test `drawSpriteScaledWithSource` with boss assets.
2. **Fix Arms Rotation**: Implement pivot rotation like spike balls.
3. **Debug Dots**: Adjust positions, log and tweak.
4. **Hurt Animation**: Add frame advancement.
5. **Boss Bar Ticking**: Add interpolation.
6. **State Machine Refactor**: Modularize after fixes.
7. **Testing**: Build/run, grab logs from app container (`xcrun simctl get_app_container ...`), analyze.

If needed, we can build and run to get logs for dynamic issues.
