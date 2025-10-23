# Boss Level Fixes Gameplan

## Overview
Based on the updated fix report and analysis of old raylib implementation in /oldscripts/BossHealthBar.cpp (uses DrawTextureRec for partial bar drawing), we'll implement fixes in sequence. Prioritize health bar partial rendering as it's foundational. Use edit_file for changes, testing after each major step. No state machine refactor yet.

## Step 1: Partial Rendering for Boss Health Bar
- **Goal**: Enable drawing partial widths of health/hurt textures like old DrawTextureRec.
- **Changes**:
  - In BossHealthBar::UpdateUIEntities, for m_healthFillEntity and m_hurtEffectEntity (UIShape? But old uses textures), adjust to use Sprite components instead if needed.
  - Switch BossHealthBar to use Sprite for bars, set sourceWidth = healthPixelWidth * scale.
  - Call MetalRenderer's drawSpriteScaledWithSource with source rect {0,0, healthPercent * originalWidth, originalHeight}.
  - Position via dest rect in renderer.
- **Files**: src/FloppyTurd/Systems/BossHealthBar.cpp, possibly add Sprite components in constructor.
- **Test**: Simulate health changes, verify bars draw partially without full texture.

## Step 2: Fix Arms Rotation Inconsistency
- **Goal**: Align with spikeball pivot rotation.
- **Investigation**: Compare BossSystem arm setup to ObstacleSystem::SpawnCastleSpikeBall (uses PivotRotationRenderer with offsets).
- **Changes**:
  - In BossSystem::InitializeForLevel, add PivotRotationRenderer to back/frontArmEntity with correct shoulder offsets (from old RatKing: shoulder at {position.x +60, y+38}, origin {64,64}).
  - Ensure UpdateArmRotations sets rotation correctly, matching UpdateSpikeBallRotations.
  - Log pivot values and compare to spikeballs.
- **Files**: src/FloppyTurd/Systems/BossSystem.cpp
- **Test**: Run aiming state, log rotations/positions; compare visuals to spikeballs.

## Step 3: Fix Dots Placement and Visibility
- **Goal**: Position dots correctly, not too far right.
- **Changes**:
  - In UpdateLockOnIndicator, adjust shoulderPivot for landscape (use screenWidth from UpdateScreenDimensions).
  - Reduce spacing or clamp x > screenWidth - margin.
  - Ensure visibility in AIMING; add debug draw if needed.
- **Files**: src/FloppyTurd/Systems/BossSystem.cpp
- **Test**: Log dot positions; build/run to visually confirm.

## Step 4: Ensure Hurt Animation Plays
- **Goal**: Verify SpriteSystem advances hurtSprite frames.
- **Changes**:
  - In ChangeState(HURT), set sprite.isAnimated = true, reset currentFrame=0, frameTime=0.15f (match old 0.2f?).
  - If not advancing, add manual update in HandleHurt similar to old.
  - Check StateAnimation integration as per report.
- **Files**: src/FloppyTurd/Systems/BossSystem.cpp
- **Test**: Apply damage, check if 6 frames play over 0.9s.

## Step 5: Add Ticking/Smooth Update to Boss Bar
- **Goal**: Smooth health depletion like old fade, add ticking if meant gradual.
- **Changes**:
  - In UpdateHealthValues, instead of instant set, store targetHealthPercent, lerp current in Update (e.g., current += (target - current) * dt * 5.0f).
  - Enhance hurt fade as in old (alpha pulse).
- **Files**: src/FloppyTurd/Systems/BossHealthBar.cpp
- **Test**: Damage boss, verify smooth bar reduction.

## Overall Testing
- After each step: Build with xcodebuild, install/run on simulator, grab logs from app container.
- Full test: Play boss level, verify all fixes.

Let's begin with Step 1.
