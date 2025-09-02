# Rat King Boss Implementation Plan

## 🎯 **Overview**
Implement a complex boss fight with Rat King featuring multiple animation states, aiming mechanics, and projectile attacks. Based on analysis of the old desktop implementation.

## 📋 **Analysis of Old Desktop Implementation**

### **Key Insights from Old System:**
- **State Machine**: IDLE → WALKING → PREPARING_ATTACK → ATTACKING → HURT → DEATH
- **Health System**: 200 HP with minion spawning at thresholds (185, 155, 125, etc.)
- **Animation System**: Multi-sprite with torso, front arm, back arm layers
- **Aiming System**: 3-second lock-on with smooth angle interpolation
- **Projectile System**: Toilet paper projectiles spawned on frame 7 of attack
- **Movement**: Left-right walking within screen bounds with smooth transitions
- **Sound Integration**: Dynamic music changes based on health (low health music)
- **Visual Effects**: Lock-on indicator dots, hurt flashing, death explosions

### **Technical Architecture:**
- **Sprites**: 7 separate animations (idle, walk, hurt, death, torso, front arm, back arm)
- **State Timing**: 2s idle, 3s walking, 1.2-2.0s aiming (health-scaled), 7-frame attack
- **Positioning**: Shoulder pivot at (position.x + 64, position.y + 64)
- **Projectile Spawn**: Frame 6 of torso animation, dual projectiles below 10% HP

## 🏗️ **iOS Implementation Architecture**

### **1. Enhanced State System**
```cpp
enum class RatKingState {
    IDLE,           // 2 second timer, single frame idle
    WALKING,        // 3 second timer, 8-frame walk animation
    AIMING,         // 1.2-2.0s (health-scaled), multi-sprite aiming
    THROWING,       // 7-frame torso animation, projectile on frame 6
    HURT,           // 6-frame hurt animation
    DEATH           // 6-frame death animation
};
```

### **2. Multi-Sprite Animation System**
- **Base Sprite**: `Ratking.png` (128×128) - idle/walking base
- **Torso Layer**: `RatkingTorsoOnly.png` (128×128, 7 frames)
- **Back Arm Layer**: `RatkingBackArmOnly.png` (128×128, 7 frames)
- **Front Arm Layer**: `RatkingAimFrontArmOnly.png` (128×128, 7 frames)
- **Hurt Animation**: `RatkingHurt.png` (128×128, 6 frames)
- **Death Animation**: `RatkingDeath.png` (128×128, 6 frames)

### **3. Enhanced Enemy Configuration**
```cpp
// Add to EnemyConfigRegistry
EnemyConfig config("Ratking", 128.0f, 128.0f, 100.0f, 6.0f, 1, 128, 128, 1, 0.16f, true, "boss_idle");
config.hitPoints = 20;  // Boss has 20 HP (scaled down from 200)
config.isBoss = true;
// Multiple animation states as found in old system
```

### **4. Aiming & Projectile System**
```cpp
struct AimingData {
    float aimDuration = 1.2f;    // Base aiming time (health scales this)
    float aimTimer = 0.0f;
    Vector2 playerPosition;
    float currentArmAngle = 180.0f;  // Start facing down
    Vector2 shoulderPivot;           // Rotation point
    bool hasLockedOn = false;
    float lockOnAngle = 0.0f;        // Final locked angle
};
```

### **5. Health & Minion System**
- **Health**: 20 HP (scaled down from desktop's 200)
- **Minion Thresholds**: Spawn RatCopters at 80%, 50%, 20% health
- **Low Health Music**: Trigger at 40% health remaining

## 🏗️ **Implementation Architecture**

### **1. Boss State System**
```cpp
enum class RatKingState {
    IDLE,           // Single frame: Ratking.png
    WALKING_LEFT,   // 8 frames: RatkingWalk.png
    WALKING_RIGHT,  // 8 frames: RatkingWalk.png (flipped)
    AIMING,         // 1 frame: RatkingTorsoOnly.png + arms
    THROWING,       // 7 frames: RatkingTossArmOnly.png
    HURT,           // 6 frames: RatkingHurt.png
    DEATH           // 6 frames: RatkingDeath.png
};
```

### **2. Animation System Overhaul**
- **Primary Sprite**: `Ratking.png` (128×128) - base idle/walking
- **Torso Layer**: `RatkingTorsoOnly.png` (128×128, 7 frames) - aiming state
- **Back Arm Layer**: `RatkingBackArmOnly.png` (128×128, 7 frames) - aiming rotation
- **Toss Arm Layer**: `RatkingTossArmOnly.png` (128×128, 7 frames) - throw animation
- **Hurt Animation**: `RatkingHurt.png` (128×128, 6 frames)
- **Death Animation**: `RatkingDeath.png` (128×128, 6 frames)

### **3. Positioning & Movement**
- **Screen Bounds**: Always on-screen, never behind player
- **Floor Distance**: 20px × 5 scale (100px) from bottom
- **Movement Range**: Left-right movement within screen bounds
- **Player Offset**: Player positioned more left for boss arena

### **4. Aiming System**
```cpp
struct AimingData {
    float aimDuration = 3.0f;    // 3 seconds to aim
    float aimTimer = 0.0f;
    Vector2 playerPosition;
    float backArmAngle = 0.0f;
    float tossArmAngle = 0.0f;
    Vector2 shoulderPivot;       // Rotation point
};
```

### **5. State Machine Logic**
```
IDLE → WALKING → AIMING (3s) → THROWING (7 frames) → IDLE
    ↓
HURT (when damaged) → IDLE
    ↓
DEATH (when HP = 0)
```

## 📁 **File Structure Changes**

### **New Files:**
- `src/FloppyTurd/Systems/RatKingBossSystem.h/cpp` - Dedicated boss system
- `src/FloppyTurd/Components/RatKingBossComponent.h` - Boss-specific components

### **Modified Files:**
- `src/FloppyTurd/Config/EnemyConfigs.cpp` - Add Rat King configurations
- `src/FloppyTurd/Systems/EnemySystem.cpp` - Integrate boss system
- `src/FloppyTurd/Systems/ProjectileSystem.cpp` - Add boss projectiles

## 🔧 **Implementation Steps**

### **Phase 1: Basic Setup**
1. ✅ Set world speed to 0 for boss level
2. ✅ Add Rat King to enemy registry with proper configurations
3. ✅ Position Rat King 100px from floor, centered horizontally
4. ✅ Scale Rat King to ~6x (768×768 pixels)

### **Phase 2: Animation System**
1. Create multi-sprite animation system for Rat King
2. Implement state transitions (idle → walking → aiming → throwing)
3. Add hurt and death animations
4. Integrate with existing EnemySystem

### **Phase 3: Aiming Mechanics**
1. Track player position during aiming state
2. Rotate back arm and toss arm around shoulder pivot
3. Maintain aiming for 3 seconds
4. Transition to throwing animation

### **Phase 4: Projectile System**
1. Spawn projectile on frame 7 of throwing animation
2. Calculate trajectory based on final aiming direction
3. Integrate with existing projectile system

### **Phase 5: Movement & AI**
1. Implement left-right walking within screen bounds
2. Ensure Rat King never goes behind player
3. Add screen boundary constraints
4. Balance attack patterns

## 🎨 **Visual Design**

### **Layering System:**
```
Background Layer (static)
├── Rat King Torso (aiming state)
├── Rat King Back Arm (rotating)
├── Rat King Toss Arm (rotating)
└── Pillar (decorative, center)
```

### **Animation Timing:**
- **Idle**: Static frame
- **Walking**: 8 frames, smooth left-right movement
- **Aiming**: 3 seconds with arm rotation tracking player
- **Throwing**: 7 frames, projectile spawns on frame 7
- **Hurt**: 6 frames, temporary invincibility
- **Death**: 6 frames, level completion

## 🧪 **Testing Checklist**

- [ ] Boss level loads with static background
- [ ] Rat King appears at correct position (100px from floor)
- [ ] Rat King scales correctly (~6x, 768×768px)
- [ ] Walking animation works left/right within bounds
- [ ] Aiming state tracks player for 3 seconds
- [ ] Arm rotation follows player position
- [ ] Throwing animation completes in 7 frames
- [ ] Projectile spawns on correct frame with proper trajectory
- [ ] Hurt animation triggers on damage
- [ ] Death animation triggers when HP = 0

## 🚀 **Success Criteria**

✅ **Functional Boss Fight**: Rat King moves, aims, throws projectiles  
✅ **Visual Polish**: Smooth animations, proper layering  
✅ **Gameplay Balance**: Challenging but fair difficulty  
✅ **Performance**: No frame drops during complex animations  

---

## 📝 **Next Steps**

1. **Immediate**: Fix current Rat King positioning and basic animations
2. **Short-term**: Implement aiming system with arm rotation
3. **Medium-term**: Add projectile spawning and trajectory
4. **Long-term**: Polish AI, balance difficulty, add sound effects

**Priority**: Get Rat King visible and moving correctly first, then build complexity.

## 🎮 **State Machine Implementation (Based on Old System)**

### **State Flow (from Desktop Analysis):**
```
IDLE (2s) → WALKING (3s) → AIMING (1.2-2.0s) → THROWING (7 frames) → IDLE
    ↓
HURT (when damaged) → IDLE
    ↓
DEATH (when HP = 0)
```

### **State Handler Methods (from Old System):**
- `HandleIdle()`: Timer-based transition to walking or aiming
- `HandleWalking()`: Left-right movement within bounds (100-220 range)
- `HandlePreparingAttack()`: Player tracking with arm rotation, 3-second lock-on
- `HandleAttacking()`: 7-frame animation, projectile spawn on frame 6
- `HandleHurt()`: 6-frame hurt animation with invincibility
- `HandleDeath()`: 6-frame death animation, level completion

## 🖼️ **Animation System Details (Enhanced from Old System)**

### **Sprite Management (7 Separate Sprites):**
- **idleSprite**: 1 frame, static - `Ratking.png`
- **walkSprite**: 8 frames, 0.15s per frame - `RatkingWalk.png`
- **torsoSprite**: 7 frames, 0.2s per frame - `RatkingTorsoOnly.png`
- **backArmSprite**: 7 frames, 0.2s per frame - `RatkingBackArmOnly.png`
- **frontArmSprite**: 7 frames, 0.2s per frame - `RatkingAimFrontArmOnly.png`
- **hurtSprite**: 6 frames, 0.2s per frame - `RatkingHurt.png`
- **deathSprite**: 6 frames, 0.3s per frame - `RatkingDeath.png`

### **Layered Rendering System:**
```
Background Layer (static)
├── Rat King Base (idle/walking)
├── Rat King Torso (aiming/throwing)
├── Rat King Back Arm (rotating during aim)
├── Rat King Front Arm (rotating during aim)
└── Pillar (decorative, center)
```

## 🎯 **Positioning & Movement (from Old System)**

### **Screen Bounds:**
- **Floor Distance**: 100px from screen bottom (20px × 5 scale)
- **Movement Range**: 100px to 220px horizontal movement
- **Walk Speed**: 20px/second
- **Shoulder Pivot**: `position + (64, 64)` for arm rotation

### **Movement Logic:**
```cpp
// From old system
position.x += dir * walkSpeed * dt;
if (position.x < walkRangeMin) dir = 1.0f;
else if (position.x > walkRangeMax) dir = -1.0f;
```

## 🚀 **Projectile System (from Old System)**

### **Spawn Timing:**
- **Frame 6** of 7-frame throwing animation
- **Dual Projectiles**: Below 10% HP (2 HP remaining)
- **Spawn Location**: `shoulder + (-40, 0)` offset from shoulder
- **Trajectory**: Based on final aiming angle

### **Projectile Properties:**
```cpp
// From old system
ToiletPaperProjectile(spawn, dir, 100.0f, 1.0f);
```

## 🎵 **Audio Integration (from Old System)**

### **Dynamic Music:**
- **Boss Music**: `BossLevel.mp3` on level start
- **Low Health**: `BossThemeLowHealth.mp3` at 40% HP
- **Dynamic Timing**: Music changes based on health percentage

### **Sound Effects:**
- **Aiming**: Lock-on sound effects
- **Throwing**: Projectile launch sounds
- **Hurt**: Damage sound effects
- **Death**: Death sound effects

## 👾 **Minion Spawning System (from Old System)**

### **Health Thresholds:**
- **80% HP**: Spawn 4 RatCopters
- **50% HP**: Spawn 4 RatCopters
- **20% HP**: Spawn 6 RatCopters

### **Spawn Logic (from Old System):**
```cpp
// Health-based minion spawning
while (health <= nextMinionHealthThreshold && nextMinionHealthThreshold > 0) {
    if (health >= 50) SpawnMinionWave(4);
    else SpawnMinionWave(6);
    nextMinionHealthThreshold -= 30;
}
```

## 🔧 **Implementation Steps (Updated)**

### **Phase 1: Core Setup ✅**
1. ✅ Set world speed to 0 for boss level
2. ✅ Add Rat King to enemy registry with proper configurations
3. ✅ Position Rat King at correct floor distance (100px)
4. ✅ Scale Rat King to 6x (768×768px final size)

### **Phase 2: Animation System (Enhanced)**
1. Implement 7-sprite management system (base + 6 animations)
2. Create layered rendering for multi-sprite boss
3. Add state-based sprite switching
4. Implement smooth transitions between animations

### **Phase 3: Aiming Mechanics (from Old System)**
1. Implement 3-second lock-on system
2. Add smooth angle interpolation (SmoothAngleLerp)
3. Create arm rotation around shoulder pivot
4. Add visual lock-on indicator

### **Phase 4: Projectile Integration**
1. Spawn projectiles on frame 6 of throwing animation
2. Calculate trajectory from final aiming angle
3. Implement dual projectile system for low health
4. Add projectile cleanup and collision

### **Phase 5: Minion System**
1. Add health threshold detection (80%, 50%, 20%)
2. Implement RatCopter spawning waves
3. Handle minion AI and player targeting
4. Add spawn position randomization

### **Phase 6: Audio & Polish**
1. Integrate dynamic music system (BossLevel.mp3 → BossThemeLowHealth.mp3)
2. Add sound effects for all boss actions
3. Implement hurt flashing and visual effects
4. Add boss health bar integration

## 🧪 **Testing Checklist (Enhanced)**

### **Basic Functionality:**
- [ ] Rat King spawns at 100px from floor
- [ ] Rat King scales to 768×768px (6x scaling)
- [ ] Boss music plays on level start
- [ ] Background remains static (world speed = 0)
- [ ] Pillar animation plays in center

### **State Machine (from Old System):**
- [ ] IDLE → WALKING transition (2s timer)
- [ ] WALKING → IDLE transition (3s timer)
- [ ] WALKING movement within 100-220px bounds
- [ ] IDLE → AIMING transition (random)

### **Aiming System (from Old System):**
- [ ] 3-second lock-on duration
- [ ] Player position tracking
- [ ] Smooth arm rotation (SmoothAngleLerp)
- [ ] Lock-on visual indicator dots
- [ ] Health-scaled aiming time (faster at low health)

### **Combat System:**
- [ ] Projectile spawning on frame 6 of 7-frame animation
- [ ] Correct trajectory calculation from final angle
- [ ] Dual projectiles below 10% HP
- [ ] Hurt animation (6 frames) on damage
- [ ] Death animation (6 frames) at 0 HP

### **Advanced Features:**
- [ ] Minion spawning at health thresholds
- [ ] Low health music transition at 40% HP
- [ ] Screen boundary constraints
- [ ] Proper multi-sprite layering
- [ ] Boss health bar integration

## 🚀 **Success Criteria (Enhanced)**

✅ **Faithful Recreation**: Matches desktop implementation behavior  
✅ **Visual Polish**: 7-sprite layered rendering system  
✅ **Audio Integration**: Dynamic music and sound effects  
✅ **Gameplay Balance**: Challenging boss fight with proper difficulty scaling  
✅ **Performance**: Smooth 60fps with complex multi-sprite animations  

---

## 📝 **Current Implementation Status**

🔄 **Phase 1 Complete**: Basic setup, positioning, scaling, world speed  
⏳ **Phase 2 In Progress**: Multi-sprite animation system implementation  
⏳ **Phase 3 Pending**: Aiming mechanics with arm rotation  
⏳ **Phase 4 Pending**: Projectile spawning system  
⏳ **Phase 5 Pending**: Minion spawning system  
⏳ **Phase 6 Pending**: Audio integration and polish  

**Next Priority**: Complete the multi-sprite animation system and state machine implementation.
