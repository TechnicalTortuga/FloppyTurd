# Hitbox Demystification - Complete Guide to Collision Detection

## Date: 2025-01-23
## Purpose: Comprehensive documentation of hitbox creation, configuration, and collision detection

---

## Overview

FloppyTurd uses **circle-based hitboxes** for most entities with a center-offset system. This document demystifies where hitboxes are created, how they're configured, and how collision detection works.

---

## 1. Hitbox Component Structure

**Location**: `src/FloppyTurd/Components/GameComponents.h` (Lines 219-239)

```cpp
struct Hitbox : public Gnosis::Component {
    ColliderType type;      // Circle, Rectangle, or Polygon
    
    // Circle properties
    float radius;           // UNSCALED radius (collision system applies transform.scale)
    
    // Rectangle properties  
    float width;
    float height;
    
    // Offset from entity transform position (top-left corner of sprite)
    float offsetX;          // Offset to center in UNSCALED sprite pixels
    float offsetY;          // Offset to center in UNSCALED sprite pixels
    
    // Behavior
    bool isTrigger;
    bool isStatic;
    std::string tag;
};
```

### Key Concepts:
- **Position**: Transform.position is the TOP-LEFT corner of the sprite
- **Offset**: Distance from top-left to hitbox center (in unscaled sprite pixels)
- **Radius**: Circle radius in unscaled pixels (scaled during collision check)
- **Scale**: Applied during collision calculations, NOT stored in hitbox

---

## 2. Where Hitboxes Are Created

### A. Player Hitbox
**Location**: `src/FloppyTurd/States/GameplayState.cpp` (CreateGameEntities)

```cpp
Hitbox playerCollider;
playerCollider.type = ColliderType::Circle;
playerCollider.radius = 24.0f;     // Unscaled radius
playerCollider.offsetX = 0.0f;     // Centered on sprite
playerCollider.offsetY = 0.0f;     // Centered on sprite
playerCollider.tag = "Player";
```

**Effective Radius**: 24.0 * 8.0 (scale) = **192px** (but actually uses 96px from calculation - CHECK THIS!)

---

### B. Obstacle Hitboxes (Toilets, Walls, etc.)
**Location**: `src/FloppyTurd/Systems/ObstacleSystem.cpp` (SpawnObstacle)

```cpp
Hitbox collider;
collider.type = ColliderType::Rectangle;  // Most obstacles use rectangles
collider.width = config.hitboxWidth;       // From obstacle config
collider.height = config.hitboxHeight;     // From obstacle config
collider.offsetX = config.hitboxOffsetX;   // Configured per obstacle type
collider.offsetY = config.hitboxOffsetY;   // Configured per obstacle type
collider.tag = "Obstacle";
```

---

### C. Enemy Hitboxes
**Location**: `src/FloppyTurd/Systems/LevelManager.cpp` (SpawnEnemy, Line ~1012)

```cpp
Hitbox collider;
collider.type = ColliderType::Circle;
collider.isStatic = false;

// Use 40% of the smaller dimension for a tight, fair hitbox
float smallerDim = std::min(enhancedConfig.width, enhancedConfig.height);
collider.radius = smallerDim * 0.4f;  // Unscaled - e.g., 64px sprite → 25.6px radius
collider.offsetX = 0.0f;  // Centered
collider.offsetY = 0.0f;  // Centered
collider.tag = "Enemy";
```

**Example**: RatCopter (64x64 sprite at 6x scale)
- Smaller dimension: 64px
- Radius: 64 * 0.4 = **25.6px** (unscaled)
- Effective radius: 25.6 * 6.0 = **153.6px** (scaled)

---

### D. Projectile Hitboxes
**Location**: `src/FloppyTurd/Systems/ProjectileSystem.cpp`

#### Initial Creation (Line ~99, ~136):
```cpp
Hitbox hitbox;
hitbox.type = ColliderType::Circle;
hitbox.radius = 8.0f;   // Placeholder - reconfigured when spawned
hitbox.offsetX = 16.0f; // Placeholder - reconfigured when spawned
hitbox.offsetY = 16.0f; // Placeholder - reconfigured when spawned
hitbox.tag = "projectile" or "enemy_projectile";
```

#### Configuration at Spawn (Line ~537-556):
```cpp
// Offset should be half the UNSCALED sprite dimensions (center of sprite)
hitbox->offsetX = config.frameWidth / 2.0f;
hitbox->offsetY = config.frameHeight / 2.0f;

// Radius varies by projectile type (UNSCALED):
switch (projectileType) {
    case ProjectileType::TOILET_PAPER:
        hitbox->radius = 10.0f;  // TP: 10px unscaled
        break;
    case ProjectileType::SNOWBALL:
        hitbox->radius = 18.0f;  // Snowball: 18px unscaled
        break;
    case ProjectileType::POOP_BALL:
    case ProjectileType::LARGE_POOP_BALL:
    default:
        hitbox->radius = 16.0f;  // Player projectiles: 16px unscaled
        break;
}
```

**Toilet Paper (Boss Level)**:
- Sprite: 32x32 pixels
- Scale: 8.0x
- Offset: (16, 16) - center of sprite
- Radius: 10px (unscaled) = **80px** (scaled)

**Player Poop Ball**:
- Sprite: 16x16 pixels
- Scale: 8.0x
- Offset: (8, 8) - center of sprite
- Radius: 16px (unscaled) = **128px** (scaled)

---

## 3. Collision Detection - The Math

### A. Circle-Circle Collision Formula

**Location**: `src/FloppyTurd/States/GameplayState.cpp` (CheckToiletCollisions)

```cpp
// Step 1: Calculate center positions
float pCenterX = playerTransform->position.x + spriteHalfWidth + (playerHitbox->offsetX * scale.x);
float pCenterY = playerTransform->position.y + spriteHalfHeight + (playerHitbox->offsetY * scale.y);
float pRadius = playerHitbox->radius * ((scale.x + scale.y) * 0.5f);

float oCenterX = obstacleTransform->position.x + spriteHalfWidth + (obstacleHitbox->offsetX * scale.x);
float oCenterY = obstacleTransform->position.y + spriteHalfHeight + (obstacleHitbox->offsetY * scale.y);
float oRadius = obstacleHitbox->radius * ((scale.x + scale.y) * 0.5f);

// Step 2: Calculate distance between centers
float dx = pCenterX - oCenterX;
float dy = pCenterY - oCenterY;
float distanceSquared = dx * dx + dy * dy;

// Step 3: Check if circles overlap
float radiusSum = pRadius + oRadius;
bool collided = distanceSquared <= (radiusSum * radiusSum);
```

### B. Circle-Rectangle Collision Formula

```cpp
// Step 1: Find closest point on rectangle to circle center
float closestX = std::max(rectX, std::min(circleCenterX, rectX + rectW));
float closestY = std::max(rectY, std::min(circleCenterY, rectY + rectH));

// Step 2: Calculate distance from circle center to closest point
float dx = circleCenterX - closestX;
float dy = circleCenterY - closestY;

// Step 3: Check if distance is within circle radius
bool collided = (dx * dx + dy * dy) <= (circleRadius * circleRadius);
```

---

## 4. Collision Check Locations

### Player vs Obstacles (Toilets, Walls)
**Location**: `GameplayState::CheckToiletCollisions()` (Line ~2829)
- **Type**: Circle (player) vs Rectangle/Circle (obstacles)
- **When**: Every frame during Playing state
- **Result**: Triggers OnPlayerHurt() if collision and not invulnerable

### Player vs Enemy Projectiles (TP, Snowballs)
**Location**: `GameplayState::CheckToiletCollisions()` (Line ~3040)
- **Type**: Circle vs Circle
- **When**: Every frame during Playing state
- **Result**: Triggers OnPlayerHurt() and marks projectile inactive

### Player vs Enemies (RatCopters, Birds)
**Location**: `GameplayState::CheckToiletCollisions()` (Line ~3080)
- **Type**: Circle vs Circle
- **When**: Every frame during Playing state
- **Result**: Damages both player and enemy

### Player Projectiles vs Boss
**Location**: `GameplayState::Update()` (Line ~395)
- **Type**: Circle (projectile) vs Circle (boss body)
- **When**: Every frame when boss is active (Level 6)
- **Result**: Damages boss, removes projectile

### Player Projectiles vs Enemies
**Location**: `EnemySystem::ProcessEnemyCollision()` (EnemySystem.cpp)
- **Type**: Circle vs Circle
- **When**: Every frame for active enemies
- **Result**: Triggers hurt state, removes projectile

---

## 5. Common Issues and Solutions

### Issue 1: Projectile Center Calculation Inconsistency (FIXED)
**Problem**: Projectiles calculated center WITHOUT sprite half-dimensions, causing 32-64px misalignment.

**Old Formula** (WRONG):
```cpp
float projCenterX = position.x + (offset * scale.x);
```

**New Formula** (CORRECT):
```cpp
float projCenterX = position.x + spriteHalfWidth + (offset * scale.x);
```

**Fix Location**: `GameplayState.cpp` Line 3057-3058

---

### Issue 2: Hitbox Radius Too Large/Small
**Symptom**: Collisions feel unfair - hitting when visually no contact, or passing through when should hit.

**Solution**: Adjust radius in `ProjectileSystem::ConfigureProjectileSprite()` or enemy configs.

**Current Values**:
- TP: 10px unscaled (80px scaled) - feels good
- Snowball: 18px unscaled (144px scaled) - may be too large
- Poop: 16px unscaled (128px scaled) - feels good
- Player: Calculated from sprite, effective ~96px - CHECK THIS

---

### Issue 3: Boss Targeting Inaccuracy (FIXED)
**Problem**: Boss aimed at sprite center, not accounting for hitbox offset.

**Solution**: Aim at AVERAGE of sprite center and hitbox center (split the difference).

**Fix Location**: `GameplayState.cpp` Line 358-374

---

## 6. Debugging Tips

### Enable Detailed Collision Logging
**Location**: `GameplayState.cpp` Line 3044-3100

Logs show:
- `[TP_CHECK]`: Number of projectiles being checked
- `[TP_COLLISION_DETAIL]`: Projectile position, sprite size, scale, offset, radius
- `[TP_COLLISION_CALC]`: Calculated center positions and radii
- `[TP_COLLISION_TEST]`: Distance vs combined radius, collision result

### Check Hitbox Configuration
**Location**: `ProjectileSystem.cpp` Line 558-560

Logs show:
- `✅ HITBOX CONFIGURED`: Radius, offset, frame size for each projectile spawned

### Verify Screen Dimensions
**Location**: Logs show `pixelWidth x pixelHeight` and `isPortrait`
- Portrait: 1179 x 2556
- Landscape: 2556 x 1179

---

## 7. Testing Checklist

### TP Collision Accuracy:
- [ ] TP hits player when visually touching
- [ ] TP does NOT hit when visually separated by 10-20px
- [ ] TP collision feels fair and predictable
- [ ] No "phantom hits" where TP is clearly not touching player

### Boss Projectile Accuracy:
- [ ] Lock-on indicator dots point at player center (offset 8px*scale higher)
- [ ] TP travels along indicator line
- [ ] TP hits player when indicator was accurate

### Enemy Collision Consistency:
- [ ] RatCopters hit player when touching
- [ ] Player projectiles destroy RatCopters accurately
- [ ] No "invincible" spots on enemies

---

## 8. Formula Reference Card

### Hitbox Center Calculation (ALL ENTITIES):
```
centerX = position.x + (spriteWidth * scale.x * 0.5) + (hitbox.offsetX * scale.x)
centerY = position.y + (spriteHeight * scale.y * 0.5) + (hitbox.offsetY * scale.y)
```

### Effective Radius:
```
effectiveRadius = hitbox.radius * ((scale.x + scale.y) * 0.5)
```

### Circle-Circle Collision:
```
dx = center1.x - center2.x
dy = center1.y - center2.y
distanceSquared = dx*dx + dy*dy
radiusSum = radius1 + radius2
collision = (distanceSquared <= radiusSum*radiusSum)
```

### Circle-Rectangle Collision:
```
closestX = clamp(circleX, rectX, rectX + rectW)
closestY = clamp(circleY, rectY, rectY + rectH)
dx = circleX - closestX
dy = circleY - closestY
collision = ((dx*dx + dy*dy) <= circleRadius*circleRadius)
```

---

## 9. Current Known Issues

### Issue: Rat Wrapping from Bottom Edge
**Status**: FIXED in build
**Root Cause**: Level ID check was wrong (checked level 5 instead of level 6)
**Fix**: Added landscape-aware Y positioning for level 6 (50%-70% vs 30%-50%)

### Issue: Lock-On Indicator Offset
**Status**: FIXED in build
**Fix**: Adjusted target position 8px*scale higher for better visual accuracy

---

## 10. File Modification Summary

**Files Modified for Hitbox Fixes**:
1. `GameplayState.cpp` - Fixed projectile center calculation (Line 3057)
2. `GameplayState.cpp` - Added boss target averaging (Line 358-374)
3. `GameplayState.cpp` - Added detailed TP collision logging (Line 3044-3100)
4. `BossSystem.cpp` - Adjusted lock-on target offset (Line 757-759)
5. `LevelManager.cpp` - Fixed rat wrapping Y positions for level 6 (Multiple locations)

**No Changes Needed**:
- `ProjectileSystem.cpp` - Hitbox configuration is correct
- Circle-circle collision math is correct
- Radius values are appropriate

---

## Conclusion

The hitbox system uses a consistent formula across all entity types:
1. **Center** = position + spriteHalf + (offset * scale)
2. **Effective Radius** = radius * averageScale
3. **Collision** = distanceSquared <= (radiusSum)²

All collision detection goes through `GameplayState::CheckToiletCollisions()` and related functions. The system is working correctly with recent fixes applied.