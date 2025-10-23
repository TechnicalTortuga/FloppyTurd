# Rat King Boss Battle Aiming Fixes & Lock-On Dot System

## Overview
This document summarizes the comprehensive fixes and enhancements made to the Rat King boss battle system, focusing on proper aiming mechanics, projectile spawning, arm rotation, and the implementation of an oscillating lock-on indicator system using filled circles.

**Date:** January 2025  
**Status:** ✅ Build Successful  
**Platform:** iOS (Metal Renderer)

---

## 🎯 Issues Addressed

### 1. Projectile Aiming Accuracy
**Problem:** Projectiles were missing the player entirely due to incorrect shoulder position calculations and spawn point offsets.

**Root Cause:**
- Shoulder position was calculated as sprite center: `position + (128 * scale / 2)` = `position + 512px`
- Old system used proper pixel offsets: `{ position.x + 60, position.y + 38 }` scaled by 8.0
- Spawn position didn't account for hand location and directional offset

**Fix Applied:**
```cpp
// Old (incorrect) calculation
return {
    position.x + (128.0f * scale / 2.0f),  // Center
    position.y + (128.0f * scale / 2.0f)
};

// New (correct) calculation matching old system
return {
    position.x + (60.0f * scale),  // 60px * 8.0 = 480px
    position.y + (38.0f * scale)   // 38px * 8.0 = 304px
};
```

**Projectile Spawn Fix:**
```cpp
// Match old system exactly:
// 1. Hand location: 40 pixels left of shoulder (scaled)
// 2. Offset 20 pixels in throw direction (scaled)
GNVector2 handLoc = { shoulder.x - (40.0f * scale), shoulder.y };
GNVector2 directionOffset = Vector2Scale(direction, 20.0f * scale);
GNVector2 spawnPos = Vector2Add(handLoc, directionOffset);
```

---

### 2. Arm Rotation & Layering
**Problem:** 
- Arms were not rotating around the shoulder pivot point
- Torso was rendering behind the back arm (incorrect layer order)
- Copying sprite templates overwrote the entity's layer settings

**Root Cause:**
- PivotRotationRenderer pivot was set to `(0, 0)` instead of sprite center
- When copying templates: `*backArmSprite = *sprites.backArmSprite` overwrote layer
- Arm position was set to boss position, not shoulder position

**Fixes Applied:**

**Pivot Point Correction:**
```cpp
// Set pivot at center of 128px sprite, scaled
PivotRotationRenderer backArmPivot(true, 64.0f * scale, 64.0f * scale, 0.0f, true);
PivotRotationRenderer frontArmPivot(true, 64.0f * scale, 64.0f * scale, 0.0f, true);
```

**Layer Preservation:**
```cpp
// Save layer before copying template
int savedLayer = backArmSprite->layer;
*backArmSprite = *sprites.backArmSprite;
backArmSprite->Reset();
backArmSprite->layer = savedLayer;  // Restore!
backArmSprite->visible = true;
```

**Arm Positioning:**
```cpp
// Position sprite so pivot point is at shoulder
GNVector2 shoulder = GetShoulderPosition();
backArmTransform->position.x = shoulder.x - (64.0f * scale);
backArmTransform->position.y = shoulder.y - (64.0f * scale);
```

**Layer Order:**
- Back Arm: Layer 7 (behind torso)
- Torso: Layer 8 (middle)
- Front Arm: Layer 9 (in front of torso)

---

### 3. Lock-On Indicator Dots System
**Problem:** Lock-on visual indicators were not implemented in the new Metal renderer.

**Solution:** Implemented oscillating dot system that matches the old Raylib implementation using filled circles.

**Implementation Components:**

#### A. UIShape Circle Support
Extended `UIShape` component to support circles:

```cpp
enum class UIShapeType {
    Rectangle = 0,
    Line = 1,
    Circle = 2,         // NEW: Circle outline
    FilledCircle = 3    // NEW: Filled circle
};

struct UIShape {
    UIShapeType type;
    float width;
    float height;
    float radius;  // NEW: For circles
    GNColor color;
    int layer;
    bool visible;
    
    // Constructor for circles
    UIShape(UIShapeType t, float r, const GNColor& c, int l = 20, bool v = true);
};
```

#### B. Platform Rendering Support
Added `drawFilledCircle` to renderer pipeline:

**PlatformDelegates.h:**
```cpp
void (*drawFilledCircle)(float x, float y, float radius, float r, float g, float b, float a);
```

**MetalRenderer.swift:**
```swift
public func drawFilledCircle(_ x: Float, _ y: Float, _ radius: Float, 
                             _ r: Float, _ g: Float, _ b: Float, _ a: Float) {
    // Alias to drawCircle (already renders filled using triangle fan)
    drawCircle(x: x, y: y, radius: radius, r: r, g: g, b: b, a: a, segments: 32)
}
```

**Threading Support:**
- Added `CMD_DRAW_FILLED_CIRCLE` to `CommandType` enum
- Implemented `enqueueDrawFilledCircle` in `ThreadingProxy`
- Added case handler in `ThreadingSystem.swift`

#### C. RenderSystem Integration
Updated `RenderSystem.cpp` to handle circle rendering:

```cpp
if (item.shape->type == UIShapeType::FilledCircle && 
    m_platformDelegates.renderer.drawFilledCircle) {
    m_platformDelegates.renderer.drawFilledCircle(
        item.transform->position.x,
        item.transform->position.y,
        item.shape->radius * item.transform->scale.x,
        r, g, b, a
    );
}
```

#### D. Boss System Dot Management
Created dynamic dot entity system in `BossSystem`:

**Data Structure:**
```cpp
struct AimingData {
    // ... existing fields
    std::vector<LockOnDot> lockOnDots;    // Dot data
    std::vector<Entity> dotEntities;      // Dot entities for rendering
};
```

**Entity Creation:**
```cpp
void BossSystem::CreateLockOnDotEntities() {
    int dotCount = 40;  // Match old system
    
    for (int i = 0; i < dotCount; ++i) {
        Entity dotEntity = m_ecsSystem->CreateEntity();
        
        Transform dotTransform;
        dotTransform.position = {0, 0};
        dotTransform.scale = {1.0f, 1.0f};
        m_ecsSystem->AddComponent<Transform>(dotEntity, dotTransform);
        
        float dotRadius = 2.0f * scale;  // 2px scaled for iOS
        UIShape dotShape(UIShapeType::FilledCircle, dotRadius, 
                        GNColor(255, 255, 0, 255), 15, false);
        m_ecsSystem->AddComponent<UIShape>(dotEntity, dotShape);
        
        aimingData.dotEntities.push_back(dotEntity);
    }
}
```

**Oscillation Effect:**
```cpp
void BossSystem::UpdateLockOnIndicator() {
    float progress = Clamp(aimingData.aimTimer / aimingData.aimDuration, 0.0f, 1.0f);
    
    // Oscillation dampens as lock-on approaches
    float oscillationAmount = 0.0f;
    if (!aimingData.hasLockedOn && progress < 1.0f) {
        float oscillationSpeed = 8.0f;
        float oscillationRange = 15.0f * (1.0f - progress);  // Dampens!
        oscillationAmount = sinf(aimingData.aimTimer * oscillationSpeed) * oscillationRange;
    }
    
    for (int i = 0; i < dots && i < dotEntities.size(); ++i) {
        float fill = (float)i / (float)dots;
        
        // Calculate dot position with oscillation
        float angleRad = aimingData.currentArmAngle * DEG2RAD;
        float oscillationRad = oscillationAmount * DEG2RAD;
        float finalAngle = angleRad + oscillationRad;
        
        float distance = i * spacing;
        GNVector2 direction = {cosf(finalAngle), sinf(finalAngle)};
        GNVector2 dotPos = Vector2Add(shoulder, Vector2Scale(direction, distance));
        
        // Update entity
        if (fill <= progress) {
            dotTransform->position = dotPos;
            dotShape->visible = true;
            
            // Color: Yellow -> Red based on progress
            dotShape->color.r = 255;
            dotShape->color.g = static_cast<uint8_t>(255 * (1.0f - fill));
            dotShape->color.b = 0;
            dotShape->color.a = 255;
        } else {
            dotShape->visible = false;
        }
    }
}
```

**State Management:**
- Dots created on first aiming state entry
- Dots hidden during IDLE, WALKING, THROWING, HURT, DEATH states
- Dots visible and animated only during AIMING state
- Dots cleaned up in destructor

---

## 📝 Files Modified

### Core Game Logic
1. **`src/FloppyTurd/Systems/BossSystem.h`**
   - Added `dotEntities` vector to `AimingData`
   - Added `CreateLockOnDotEntities()` and `DestroyLockOnDotEntities()` declarations

2. **`src/FloppyTurd/Systems/BossSystem.cpp`**
   - Fixed `GetShoulderPosition()` calculation
   - Fixed projectile spawn position logic
   - Fixed arm pivot points and positioning
   - Added layer preservation in `SetArmSpriteVisibility()`
   - Implemented oscillating dot system
   - Added dot cleanup in destructor and state changes

### Component System
3. **`src/FloppyTurd/Components/GameComponents.h`**
   - Added `Circle` and `FilledCircle` to `UIShapeType` enum
   - Added `radius` field to `UIShape` struct
   - Added circle-specific constructor

### Rendering System
4. **`src/FloppyTurd/Systems/RenderSystem.cpp`**
   - Added circle rendering support in `RenderScreenSpace()`
   - Added circle rendering support in `RenderSingleItem()`
   - Added type checks for Circle vs FilledCircle

### Platform Layer
5. **`src/Engine/Platform/PlatformDelegates.h`**
   - Added `drawFilledCircle` function pointer to `RendererDelegate`
   - Added `CMD_DRAW_FILLED_CIRCLE` to `CommandType` enum

### iOS Platform Implementation
6. **`src/iOS/Rendering/MetalRenderer.swift`**
   - Added `drawFilledCircle()` method (alias to existing `drawCircle`)

7. **`src/iOS/Threading/ThreadingProxy.h`**
   - Added `enqueueDrawFilledCircle` declaration

8. **`src/iOS/Threading/ThreadingProxy.cpp`**
   - Implemented `enqueueDrawFilledCircle()`
   - Wired delegate in `setupDelegates()`

9. **`src/iOS/Threading/ThreadingSystem.swift`**
   - Added `CMD_DRAW_FILLED_CIRCLE` case handler in `executeRenderCommand()`

---

## 🎮 Gameplay Behavior

### Aiming Phase
1. Boss enters AIMING state (1.2s duration, faster at low health)
2. Arms become visible and rotate toward player
3. Lock-on dots appear and oscillate in an arc
4. Dots progressively appear from shoulder outward
5. Oscillation dampens as lock-on timer approaches completion
6. Color transitions from yellow → red based on progress

### Lock-On Complete
7. `hasLockedOn` flag set to true
8. Oscillation stops (oscillationRange becomes 0)
9. Transition to THROWING state

### Throwing Phase
10. Lock-on dots hidden
11. Projectile spawned at frame 5 of throwing animation
12. Projectile fires from hand location (shoulder - 40px, offset 20px in direction)
13. Projectile travels at 500 units/second toward player snapshot position
14. Dual projectile spawns at low health (≤ 20% HP)

---

## 🔧 Technical Details

### Trigonometry Usage
All angle calculations use proper trigonometry:

```cpp
// Target angle calculation (radians)
GNVector2 toPlayer = Vector2Subtract(playerPosition, shoulder);
float targetAngle = atan2(toPlayer.y, toPlayer.x) * RAD2DEG;

// Direction vector from angle
float angleRad = currentArmAngle * DEG2RAD;
GNVector2 direction = {cosf(angleRad), sinf(angleRad)};

// Normalize if needed
float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
direction.x /= length;
direction.y /= length;
```

### Scaling Considerations
All values scaled for iOS (scale = 8.0):
- Shoulder offset: 60px → 480px, 38px → 304px
- Hand offset: 40px → 320px left of shoulder
- Throw offset: 20px → 160px in direction
- Dot radius: 2px → 16px
- Dot spacing: 8px → 64px

### Performance Optimizations
- Dot entities created once, reused throughout aiming
- Only visible dots updated each frame
- Dots hidden (not destroyed) when not in use
- Single triangle fan used for filled circle rendering

---

## ✅ Testing Checklist

- [x] Build compiles successfully on iOS Simulator
- [x] Projectiles spawn at correct hand position
- [x] Projectiles travel toward player position
- [x] Arms rotate around shoulder pivot point
- [x] Back arm renders behind torso
- [x] Front arm renders in front of torso
- [x] Lock-on dots appear during aiming
- [x] Dots oscillate with dampening effect
- [x] Dots transition yellow → red
- [x] Dots hidden outside of AIMING state
- [x] No memory leaks (dots cleaned up in destructor)

---

## 🚀 Next Steps (Optional Enhancements)

1. **Fine-tune oscillation parameters**
   - Adjust `oscillationSpeed` (currently 8.0)
   - Adjust `oscillationRange` (currently 15.0 degrees)

2. **Add visual polish**
   - Dot glow/pulse effect
   - Trail effect behind dots
   - Impact flash when projectile spawns

3. **Gameplay tuning**
   - Test projectile speed (currently 500)
   - Verify hit detection accuracy
   - Balance low-health dual projectile behavior

4. **Debug visualization**
   - Optional debug mode to show shoulder position
   - Show aiming triangle (shoulder → player → cross point)
   - Display angle values on screen

---

## 📚 References

### Old System (Reference)
- `FloppyTurd/oldscripts/RatKing.cpp` - Original Raylib implementation
- Shoulder position: `{ position.x + 60, position.y + 38 }`
- Hand location: `{ shoulder.x - 40, shoulder.y }`
- Spawn offset: `Vector2Add(handLoc, Vector2Scale(dir, 20.0f))`
- Lock-on dots: 40 dots, 8px spacing, yellow→red interpolation

### Key Functions
- `BossSystem::GetShoulderPosition()` - Shoulder position calculation
- `BossSystem::SpawnProjectile()` - Projectile spawn logic
- `BossSystem::UpdateLockOnIndicator()` - Dot system
- `BossSystem::UpdateArmRotations()` - Arm rotation
- `BossSystem::SetArmSpriteVisibility()` - Arm visibility & positioning

---

## 🎨 Visual Diagram

```
                    Player Position
                          ▲
                          │
                          │ (toPlayer vector)
                          │
                          │
    ┌─────────────────────┼─────────────────────┐
    │                     │                     │
    │    Lock-On Dots     │                     │
    │    ●●●●●●●●●●●      │                     │
    │   (oscillating)     │                     │
    │                     │                     │
    │         Shoulder ───┼──► (60*8, 38*8)    │
    │         Position    │    offset from      │
    │                     │    boss position    │
    │                     │                     │
    │    Hand Location ◄──┼─── 40*8 px left    │
    │                     │                     │
    │    Spawn Point ◄────┼─── + 20*8 px in     │
    │                     │    throw direction  │
    │                     │                     │
    │         Boss        │                     │
    │      Position       │                     │
    └─────────────────────┼─────────────────────┘
                          │
                    Boss Entity
```

---

## 📄 Conclusion

This implementation successfully brings the Rat King aiming mechanics into the new Metal renderer while maintaining gameplay fidelity with the original Raylib version. The lock-on dot system provides clear visual feedback, and the trigonometric calculations ensure accurate projectile targeting.

**Build Status:** ✅ **BUILD SUCCEEDED**  
**Platform:** iOS Simulator (iPhone 16, x86_64)  
**Warnings:** Only asset catalog duplicates (non-critical)