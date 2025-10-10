# Enemy Implementation Gameplan

## ⚠️ CRITICAL REQUIREMENTS
- **NO HARDCODED DIMENSIONS**: All coordinates must use `ConfigManager::Instance().GetCurrentScreenInfo()`
- **NATIVE iOS DESIGN**: Design enemy behaviors directly for iOS device dimensions, not scaled from legacy code
- **ORIENTATION AWARENESS**: Handle portrait vs landscape positioning correctly
- **DEVICE AGNOSTIC**: Implementation must work on any iOS device resolution

## Overview
This document outlines the complete implementation plan for enemy elements in FloppyTurd, focusing on collision detection, hurt animations, and specialized enemy behaviors.

## Current Architecture Analysis

### ECS Components & Systems
- **EnemySystem**: Handles enemy state management, movement, and animations
- **ProjectileSystem**: Manages projectiles with Hitbox components
- **GameplayState**: Contains collision detection logic (currently only player vs obstacles)
- **Hitbox Component**: Supports Circle and Rectangle colliders with offsets and tags

### Enemy Configuration System
- **EnemyConfigRegistry**: Centralized configuration for all enemy types
- **Enemy Component**: Contains health, damage, movement patterns, state management, bobbing behavior

## iOS Dimensions & Orientation Context

### Screen Dimensions (Dynamic - No Hardcoding!)
- **Portrait Mode (Castle Level)**: ConfigManager::GetCurrentScreenInfo().pixelWidth × pixelHeight
- **Landscape Mode (Boss Level)**: pixelWidth × pixelHeight (swapped when orientation changes)
- **Scale Factor**: ConfigManager::GetCurrentScreenInfo().scaleFactor
- **Implementation**: Always use ConfigManager::Instance().GetCurrentScreenInfo() - never hardcode dimensions!

### Level-Specific Orientations
- **Castle Level**: Portrait mode (dynamic width × height from ConfigManager)
- **Boss Level**: Landscape mode (height × width from ConfigManager)
- **Desert Level**: Portrait mode (birds fly across current screen width)
- **Snow Level**: Portrait mode (snowmen positioned along current screen width)

## Legacy Implementation Reference (For Behavior Logic Only)

### Bird Behavior Reference
- **Movement**: Horizontal left movement with sine wave hover
- **Hurt Animation**: 4-frame hurt animation (0.4 seconds total)
- **Hitbox**: Zero-sized when hurt, normal rectangle otherwise
- **Lifecycle**: Removed when hurt animation completes

### RatCopter (Rat) Behavior Reference
- **States**: FLY_IN → HOVER → PULLBACK → BEELINE
- **FLY_IN**: Move left until trigger distance reached
- **HOVER**: Wait 0.75-1.0 seconds (random)
- **PULLBACK**: Move backward from player direction for 0.25 seconds
- **BEELINE**: Charge toward locked direction at increased speed
- **Hurt Animation**: 6-frame hurt animation (0.5 seconds total)

### ToiletPaper Behavior Reference
- **Movement**: Sinusoidal oscillation with phase offset
- **Idle Behavior**: 75% chance to be active initially, idle periods between active phases
- **Hurt Animation**: 4-frame hurt animation (user specification)

## Required Implementation

### 1. Collision Detection System Enhancement
**Current State**: Collision detection exists only for player vs obstacles
**Required**: Add projectile vs enemy collision detection

**Implementation Location**: EnemySystem::Update()
**Algorithm**:
- For each active projectile, check collision with all active enemies
- Use Hitbox components for collision detection (circle-circle, circle-rectangle, rectangle-rectangle)
- On collision: Apply damage, trigger hurt state, remove projectile

### 2. Hurt State Management
**Current State**: Basic EnemyState enum exists (Idle, Moving, Attacking, Hurt, Dead, Decorative)
**Required**: Implement hurt state transitions and animations

**Requirements**:
- Switch to hurt animation sprites when damaged
- Disable hitbox during hurt state
- Auto-remove enemy after hurt animation completes
- Reset hurt state after animation finishes (if not dead)

### 3. Enemy-Specific Behaviors

#### Bird Echelon Formations (Desert Level - Portrait Mode)
**Current Config**: Basic horizontal movement with optional bobbing
**Required**: Multiple formation types with spatial constraints
- **3-Bird Echelon**: 3 birds in diagonal line across current screen width
- **5-Bird V-Formation**: 5 birds in V formation (not diagonal line due to space constraints between toilet top and screen top)
- **Hovering Pairs**: 2-3 birds hovering up/down between pipe top and screen top

**Implementation**:
- Extend EnemyConfig with formation parameters
- Add formation spawning logic in LevelManager
- Implement vertical positioning based on formation type using ConfigManager dimensions
- **V-Formation Logic**: Center bird at middle height (screenHeight/2), birds to left/right progressively higher/lower to form V
- **Coordinate Scaling**: Use ConfigManager for all positioning - no hardcoded coordinates!

#### Rat Pullback & Beeline Behavior
**Current Config**: Basic flying behavior
**Required**: Different behavior per level type

**Boss Level (Landscape)**:
- **FLY_IN**: Move left until trigger distance (x ≤ scaled trigger distance)
- **HOVER**: Pause for random duration (0.75-1.0s)
- **PULLBACK**: Calculate direction to player, move backward for 0.25s
- **BEELINE**: Charge toward locked direction at increased speed
- **Orientation**: Landscape means rats fly across wider screen dimension

**Castle Level (Portrait)**:
- **Movement**: Move with level (horizontal scrolling) until sufficiently on screen
- **Trigger**: When on screen enough (x position check from left edge), switch to beeline
- **Rows**: Two distinct rows - one hugging screen top, one hugging screen bottom
- **Note**: Spike ball covers middle obstacles, so rats stay in top/bottom rows
- **Orientation**: Portrait means vertical separation is key

**State Management**:
- Add new EnemyState values: FlyingIn, Hovering, Pullback, Beeline, Scrolling
- Track target direction, pullback timer, hover timer
- Speed scaling during beeline phase
- Level-specific behavior initialization

#### ToiletPaper Sinusoidal Movement (Sewer Level - Portrait)
**Current Config**: Horizontal movement with bobbing
**Required**: True sinusoidal oscillation scaled for current screen dimensions
- **Formula**: `pos.y = startY + amplitude * sin(pos.x * frequency + phaseOffset)`
- **Parameters**: amplitude scaled appropriately, frequency=0.015f, randomized phaseOffset
- **Idle Behavior**: Periodic inactive periods (4-8 seconds)
- **Coordinate Scaling**: Use ConfigManager for all dimensions - startY relative to screenHeight, amplitude based on screen scale

### 4. Animation System Integration
**Current State**: StateAnimation component exists for complex animations
**Required**: Hurt animation support for all enemy types

**Bird Hurt Animation**:
- 4 frames at 0.1s per frame (0.4s total)
- Texture: "BirdHurt"
- Frame size: 32x32

**Rat Hurt Animation**:
- 6 frames at ~0.083s per frame (0.5s total)
- Texture: "RatCopterHurt"
- Frame size: 64x64

**ToiletPaper Hurt Animation**:
- 4 frames at 0.08s per frame (0.32s total) - **CONFIRM**: Old code shows 2 frames, user specified 4 frames
- Texture: "ToiletPaperHurt"
- Frame size: 64x64

### 5. Native iOS Coordinate Design
**Critical**: Design all enemy behaviors directly for current iOS device dimensions using ConfigManager

**Implementation Pattern**:
```cpp
const auto& screenInfo = ConfigManager::Instance().GetCurrentScreenInfo();
float screenWidth = screenInfo.pixelWidth;
float screenHeight = screenInfo.pixelHeight;
bool isPortrait = screenInfo.isPortrait;

// Design positions relative to current device dimensions
float enemyX = screenWidth * 0.8f;        // 80% across screen width
float enemyY = screenHeight * 0.5f;       // Center of screen height
float margin = screenWidth * 0.05f;       // 5% margin from edges
```

**Requirements**:
- **Proportional Positioning**: Use percentages of screen dimensions for all enemy positions
- **Safe Zone Margins**: Keep proportional margins from screen edges (5-10% of screen dimensions)
- **Orientation Awareness**: Handle portrait vs landscape positioning correctly
- **No Hardcoded Values**: Every coordinate calculated from ConfigManager screen info

### 7. Configuration Updates
**Files to Modify**: EnemyConfigs.cpp, EnemyConfigs.h

**New Configuration Parameters**:
```cpp
// Formation configuration for birds
struct FormationConfig {
    std::string type; // "echelon_3", "v_formation_5", "hovering_pairs"
    int birdCount;
    float verticalSpacing;
    float horizontalSpacing;
    float hoverRangeMin; // as percentage of screen height
    float hoverRangeMax;
    float vFormationAngle = 30.0f; // degrees for V formation spread
};

// Behavior parameters for rats - all values are base parameters, actual positions calculated dynamically
struct RatBehaviorConfig {
    // Timing parameters
    float hoverDurationMin = 0.75f;
    float hoverDurationMax = 1.0f;
    float pullbackDuration = 0.25f;
    float beelineSpeedMultiplier = 1.5f;
    float detectionRange = 300.0f;

    // Position parameters (as percentages of screen dimensions)
    float flyInTriggerXPercent = 0.75f;     // Trigger beeline at 75% across screen width
    float castleScrollTriggerXPercent = 0.2f; // Trigger at 20% from left edge in castle level
    float topRowYPercent = 0.05f;           // Top row at 5% from screen top
    float bottomRowYPercent = 0.95f;        // Bottom row at 95% from screen top
};

// Oscillation parameters for toilet paper (Sewer Level - Portrait)
struct OscillationConfig {
    // Oscillation parameters (relative to screen dimensions)
    float amplitudePercent = 0.06f;        // 6% of screen height
    float frequency = 0.015f;
    bool randomizePhase = true;
    float idleChance = 0.25f;            // 25% chance to start idle
    float idleDurationMin = 4.0f;
    float idleDurationMax = 8.0f;
    float startYCenterPercent = 0.5f;     // Start at 50% screen height (center)
    float startYVariancePercent = 0.08f;  // ±8% variance around center
};
```

### 8. Enemy Pooling & Lifecycle Management
**Current State**: Enemies are destroyed when defeated or offscreen
**Required**: Proper enemy pooling system with deactivation/reactivation

**Enemy Pool Architecture**:
- **Pool Size**: ~10 enemies maximum (configurable per level type)
- **States**: Active, Inactive (available for respawning)
- **Lifecycle**: Inactive → Active → Hurt → Inactive (reusable)

**Pooling Implementation**:
```cpp
struct EnemyPool {
    std::vector<Entity> allEnemies;           // All pre-allocated enemies
    std::vector<Entity> activeEnemies;        // Currently active/spawned
    std::vector<Entity> inactiveEnemies;      // Available for respawning (ALL can be reused)
};
```

**Dynamic Spawning Logic**:
- Check available inactive enemies
- Calculate possible patterns based on available count
- Spawn patterns that fit available enemy count
- Skip patterns that require more enemies than available

**Hurt Animation Timing Fix**:
- **Issue**: Enemies marked inactive immediately, preventing hurt animation
- **Fix**: Enemies stay active during hurt animation, returned to inactive pool after completion
- **Benefit**: All enemies remain in pool and can be reused indefinitely

## Implementation Phases

### Phase 1: Native iOS Coordinate System
1. Implement ConfigManager integration throughout enemy systems - no hardcoded dimensions anywhere
2. Create proportional positioning utilities that work with current iOS device dimensions
3. Add orientation-aware positioning logic (portrait vs landscape) using screenInfo.isPortrait
4. Update all EnemyConfig initialization to calculate positions as percentages of screen dimensions

### Phase 2: Enemy Pooling System
1. Implement EnemyPool structure with active/inactive/defeated states
2. Create pre-allocated enemy pool (~10 enemies) with proper component initialization
3. Add deactivation/reactivation logic instead of entity destruction
4. Implement dynamic pattern spawning based on available inactive enemies
5. Fix hurt animation timing - enemies stay active during hurt animation

### Phase 3: Core Collision System
1. Add projectile-enemy collision detection to EnemySystem
2. Implement hurt state transitions with proper timing
3. Update enemy lifecycle to use defeated state instead of immediate removal
4. Test collision detection across different orientations

### Phase 4: Animation Integration
1. Create StateAnimation configurations for hurt states
2. Update sprite switching logic
3. Verify animation timing and frame counts
4. Test hurt animations in both portrait and landscape orientations

### Phase 5: Specialized Behaviors
1. Implement Bird echelon formations (including V-formation logic) using proportional positioning
2. Add Rat AI state machine (Boss level landscape: fly-in/hover/pullback/beeline, Castle level portrait: scrolling/beeline with row positioning)
3. Create ToiletPaper oscillation movement using percentage-based parameters
4. Update EnemyConfig structures with formation and level-specific parameters as percentages

### Phase 6: Testing & Polish
1. Test all collision scenarios in both orientations
2. Verify enemy behaviors match legacy behavior logic (adapted for iOS dimensions)
3. Performance optimization for iOS
4. Edge case handling and orientation changes
5. Validate enemy pooling and dynamic spawning

## Dependencies & Prerequisites

### Asset Requirements
- **BirdHurt**: 4-frame hurt animation spritesheet
- **RatCopterHurt**: 6-frame hurt animation spritesheet
- **ToiletPaperHurt**: 4-frame hurt animation spritesheet (verify frame count)
- Confirm all hurt animation assets exist and match specifications

### Code Dependencies
- EnemySystem must access ProjectileSystem for collision detection
- LevelManager integration for enemy spawning and formation logic
- StateAnimation component for complex hurt animations

## Testing Strategy

### Collision Testing
- Player projectiles hitting each enemy type
- Verify hurt animations trigger correctly
- Confirm enemy removal after animation completion
- Test edge cases (multiple projectiles, rapid hits)

### Behavior Testing
- Bird formations spawn correctly
- Rat AI transitions work properly
- ToiletPaper oscillation matches legacy behavior
- Screen wrapping and boundary handling

### Performance Testing
- Enemy count limits with collision detection active
- Memory usage with hurt animation state switching
- Frame rate impact during intense combat scenarios

## Risk Assessment

### High Risk
- Collision detection performance with many enemies/projectiles
- Complex Rat AI state machine reliability
- Formation spawning logic complexity

### Medium Risk
- Hurt animation state management
- Enemy removal timing
- Configuration parameter tuning

### Low Risk
- ToiletPaper oscillation (mathematical implementation)
- Bird formation positioning (geometric calculations)
- Animation frame timing

## Success Criteria

1. **Collision Detection**: All enemy types take damage from player projectiles
2. **Hurt Animations**: Correct frame counts and timing for all enemy types
3. **Enemy Behaviors**: All specialized behaviors match legacy implementation
4. **Formation Logic**: Bird echelons and hovering pairs work correctly
5. **Performance**: No frame rate drops during normal gameplay
6. **Reliability**: No crashes or memory leaks during enemy lifecycles
