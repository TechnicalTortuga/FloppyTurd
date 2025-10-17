# COMPREHENSIVE SNOWBALL SYSTEM ANALYSIS

## 🚨 CRITICAL DISCOVERY: SNOWBALLS ARE GOING RIGHT!

### **User Report vs. Logs Contradiction**
- **User says:** "Snowballs ONLY going to the RIGHT"
- **Logs show:** velocity.x is NEGATIVE (-100 to -150)
- **Conclusion:** There's a SIGN FLIP somewhere in the rendering or coordinate system!

---

## 📐 COORDINATE SYSTEM ANALYSIS

### **Screen Coordinates (Need Verification):**
```
Standard iOS/Metal (Top-Left Origin):
  0,0 ----------> +X (RIGHT)
   |
   |
   v
  +Y (DOWN)

Flappy Bird Style:
  Player X = 332 (left side, FIXED)
  Player Y varies (0-2556, flapping)
  Snowman X varies (scrolling LEFT from right)
  Snowman Y = 2236 (near bottom, FIXED)
```

### **Current Calculations:**
```cpp
// Player CENTER: (332, varies)
// Spawn position: (snowmanX + offset, 2336)

dx = 332 - (snowmanX + offset)  // NEGATIVE when player is LEFT
dy = playerY - 2336              // NEGATIVE when player is ABOVE spawn

// Example:
// Snowman at X=1200, player at Y=800
dx = 332 - 1200 = -868  (player 868px to the LEFT)
dy = 800 - 2336 = -1536 (player 1536px ABOVE spawn)
```

**✅ This math is CORRECT for aiming LEFT and UP!**

### **🚨 PROBLEM: Velocity Assignment**
```cpp
// ProjectileSystem.cpp line 241:
physics->velocity = direction; // Direction is (-868, -1536) normalized * 220

// ProjectileSystem.cpp line 414-415:
transform->position.x += physics->velocity.x * deltaTime;
transform->position.y += physics->velocity.y * deltaTime;
```

**If velocity.x is NEGATIVE, position.x DECREASES → moves LEFT** ✅
**If velocity.y is NEGATIVE, position.y DECREASES → moves UP** ✅

**BUT USER SEES IT GOING RIGHT!** 🚨

---

## 🔍 POSSIBLE CAUSES OF RIGHT-WARD MOVEMENT

### **Hypothesis 1: Camera Scroll Interference**
```cpp
// Line 247: ScrollSpeed(0.0f) added to prevent camera scroll
// BUT: Is this being applied AFTER velocity?
```

**Check needed:** Does CameraSystem run AFTER ProjectileSystem?

### **Hypothesis 2: Sprite Flip Affects Projectile**
```cpp
// When snowman flips, does projectile inherit flip?
// Snowman scale.x = -1 when facing right
// Does this flip the projectile spawn direction?
```

###Hypothesis 3: Renderer Coordinate Transform**
**Metal renderer might flip X axis!**

---

## 🎯 APEX PHYSICS CALCULATION

### **User Requirement:**
> "The Apex of the snowball throw should ALWAYS be where the player position WAS when the snowball was thrown."

This requires **BALLISTIC TRAJECTORY PHYSICS**, not simple aim!

### **Current (Wrong) Approach:**
```cpp
// Aim directly at player
velocity = normalize(player - spawn) * 220
// Result: Straight line with gravity pulling down
// Apex is NOT at player position!
```

### **Correct Ballistic Approach:**

Given:
- Start: (sx, sy) = spawn position
- Target: (px, py) = player position
- Gravity: g = 200 px/s² (downward = +Y direction)
- We want apex at player's Y position

**Physics Equations:**
```
Vertical motion (reaching apex):
  v_y_initial² = 2 * g * (py - sy)  // Need to travel (py - sy) upward
  v_y_initial = sqrt(2 * g * abs(py - sy))
  
  Since py < sy (player above spawn):
    v_y_initial = -sqrt(2 * 200 * (sy - py))  // Negative = upward
  
Horizontal motion:
  Time to apex: t_apex = v_y_initial / g
  Horizontal velocity: v_x = (px - sx) / t_apex
  
Example (Frame 1 from logs):
  sx = 1295.64, sy = 2336
  px = 332, py = 1070.94
  
  Vertical distance: sy - py = 2336 - 1070.94 = 1265.06
  v_y = -sqrt(2 * 200 * 1265.06) = -sqrt(506024) = -711.5 px/s
  
  Time to apex: t = 711.5 / 200 = 3.56 seconds
  
  Horizontal distance: px - sx = 332 - 1295.64 = -963.64
  v_x = -963.64 / 3.56 = -270.6 px/s
  
Final velocity: (-270.6, -711.5)
```

**This is VERY different from current (-135, -173)!**

---

## 🔧 FLIP SYSTEM ANALYSIS

### **Current Flip Logic:**
```cpp
// EnemySystem.cpp line 105-108:
float playerCenterX = playerTransform->position.x + 32.0f;  // 332 + 32 = 364
float snowmanCenterX = transform->position.x + 32.0f;
float horizontalOffset = snowmanCenterX - playerCenterX;

if (horizontalOffset > 0) {
    // Snowman RIGHT of player → face LEFT (normal)
    isFacingRight = false;
    scale.x = +abs(scale.x);  // Positive scale
} else {
    // Snowman LEFT of player → face RIGHT (flipped)
    isFacingRight = true;
    scale.x = -abs(scale.x);  // Negative scale
}
```

### **Problem: Snowman Spawns from RIGHT edge**
```
Screen layout:
0 -------- 332(player) ----------- 1170(screen edge)

Snowman spawns at X = 1170+ (off-screen right)
First throw: snowmanX > 332, so horizontalOffset > 0
  → isFacingRight = false (facing LEFT) ✅ CORRECT!

As snowman scrolls left past player:
snowmanX < 332, so horizontalOffset < 0
  → isFacingRight = true (facing RIGHT) ✅ Should turn around!
```

**BUT USER SAYS:** "Snowmen come on screen already flipped"

**Possible issue:** On spawn, are they being set to isFacingRight=true by default?

---

## 🎬 STATE MACHINE ANALYSIS

### **Snowman States:**
1. **Spawn** → Y=2236 (ground), X=1170+ (off-screen right)
2. **Scroll Left** → X decreases, approaching screen
3. **Enter Screen** → When X < 1170, visible
4. **Throw (Initial)** → When close to screen edge
5. **Keep Scrolling** → Move past player
6. **Throw (Proximity)** → If player nearby
7. **Exit Screen** → X < 0
8. **Wrap** → Reset to X=1170+, reset flip state

### **Flip State Lifecycle:**
```
SPAWN:
  isFacingRight = false (default, should face LEFT)
  ✅ Correct initial state

RESET ON WRAP (LevelManager.cpp):
  t->scale.x = std::abs(t->scale.x);
  enemyComp->isFacingRight = false;
  ✅ Should reset to facing LEFT
```

**IF snowmen spawn already flipped:**
- Check spawn initialization in LevelManager::SpawnEnemies
- Maybe isFacingRight is not being initialized?

---

## 📊 PROJECTILE TRACKING PLAN

Add comprehensive logging to track snowball positions every frame:

```cpp
// In ProjectileSystem::UpdateActiveProjectiles (enemy projectiles)
if (projectileData->projectileType == ProjectileType::SNOWBALL) {
    static std::map<Entity, int> snowballFrames;
    snowballFrames[projectile]++;
    
    GN_LOG_INFO("[SNOWBALL " + std::to_string(projectile) + "] Frame " + 
               std::to_string(snowballFrames[projectile]) + 
               ": pos=(" + std::to_string(transform->position.x) + ", " + 
               std::to_string(transform->position.y) + ")" +
               ", vel=(" + std::to_string(physics->velocity.x) + ", " + 
               std::to_string(physics->velocity.y) + ")" +
               ", lifetime=" + std::to_string(projectileData->currentLifetime));
}
```

---

## 🎯 RECOMMENDED FIXES

### **Fix 1: Implement Ballistic Trajectory**
```cpp
// Calculate velocities to hit player Y at apex
float dy = spawnPosition.y - playerSnapshotPos.y;  // Distance to climb
float dx = playerSnapshotPos.x - spawnPosition.x;  // Horizontal distance

// Initial vertical velocity to reach player Y
float v_y = -std::sqrt(2.0f * 200.0f * std::abs(dy));  // Upward

// Time to reach apex
float timeToApex = std::abs(v_y) / 200.0f;

// Horizontal velocity to reach player X at apex
float v_x = dx / timeToApex;

finalDirection.x = v_x;
finalDirection.y = v_y;
```

### **Fix 2: Add Per-Frame Snowball Logging**
Track every snowball position, velocity, and lifetime.

### **Fix 3: Investigate Coordinate System**
- Check if Metal renderer flips X axis
- Check if camera scroll is being applied despite ScrollSpeed(0)
- Check if sprite flip affects child projectiles

### **Fix 4: Verify Flip Initialization**
```cpp
// In LevelManager::SpawnEnemies, after creating enemy:
enemyComp->isFacingRight = false;  // Explicitly set default
```

### **Fix 5: Remove Center Offset from Flip Decision**
```cpp
// Don't add 32px offset for flip decision
// Snowman position IS already the relevant point
float horizontalOffset = transform->position.x - playerTransform->position.x;
```

The +32 offset might be causing flip inconsistencies!

---

## 📈 EXPECTED RESULTS AFTER FIXES

### **With Ballistic Trajectory:**
- Snowballs arc to player's Y position
- Apex occurs at (playerX, playerY) from spawn moment
- Much higher initial velocity.y (-700 instead of -173)
- Flight time ~3-4 seconds to reach player

### **With Better Flip Logic:**
- Snowmen always spawn facing LEFT
- Only flip when passing player
- No flip position jump (we removed compensation)

### **With Coordinate System Fix:**
- Negative velocity.x → moves LEFT (not right!)
- Snowballs actually hit player

---

## 🔬 DEBUGGING STEPS

1. **Add snowball frame-by-frame logging**
2. **Log spawn flip state explicitly**  
3. **Check if velocity.x sign matches movement direction**
4. **Implement ballistic trajectory**
5. **Test and compare logs to expected behavior**
