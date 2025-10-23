# RatCopter AI State Machine

## Visual State Diagram

```
                    ┌─────────────────────────────────────────────────────────┐
                    │                                                         │
                    │                   SPAWN/WRAP                           │
                    │                                                         │
                    └──────────────────────┬──────────────────────────────────┘
                                           │
                                           │ Initialize at x = screenWidth + 650
                                           │ Y = 25%-60% of screen height
                                           ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                                                          │
                    │                    ① FLY_IN STATE                       │
                    │                                                          │
                    │  • Move left at normal speed                            │
                    │  • Apply bobbing for visual interest                    │
                    │  • Continue until X ≤ 75% of screen width               │
                    │                                                          │
                    └──────────────────────┬───────────────────────────────────┘
                                           │
                                           │ X ≤ 75% screen
                                           │ Set hover timer (0.75-1.0 seconds)
                                           ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                                                          │
                    │                   ② HOVER STATE                         │
                    │                                                          │
                    │  • Stay in place (no horizontal movement)               │
                    │  • Apply bobbing for hover effect                       │
                    │  • Countdown hover timer                                │
                    │  • Constrain Y to safe bounds                           │
                    │                                                          │
                    └──────────────────────┬───────────────────────────────────┘
                                           │
                                           │ Hover timer expires
                                           │ Get player position
                                           │ Calculate direction to player
                                           ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                                                          │
                    │                  ③ PULLBACK STATE                       │
                    │                                                          │
                    │  • Lock direction toward player                         │
                    │  • Calculate pullback vector (opposite * 20 units)      │
                    │  • Move backward for 0.25 seconds                       │
                    │  • Prepare for charge                                   │
                    │                                                          │
                    └──────────────────────┬───────────────────────────────────┘
                                           │
                                           │ Pullback timer expires
                                           │ Calculate beeline speed (150 * difficulty)
                                           │
                                           ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                                                          │
                    │                  ④ BEELINE STATE                        │
                    │                                                          │
                    │  • Charge toward locked direction                       │
                    │  • High speed (150.0 base * difficulty multiplier)      │
                    │  • Continue until offscreen                             │
                    │  • Y locked (no vertical adjustment)                    │
                    │                                                          │
                    └──────────────────────┬───────────────────────────────────┘
                                           │
                                           │ Exits screen bounds
                                           │ Wrap system resets position
                                           │
                                           └─────────────► Back to FLY_IN STATE
```

---

## State Details

### ① FLY_IN
**Purpose**: Entry animation from spawn or wrap
**Duration**: Until X position ≤ 75% of screen width
**Movement**:
- Horizontal: `-speed * deltaTime`
- Vertical: Bobbing enabled (sine wave)
**Transition**: X ≤ 75% screen → HOVER

---

### ② HOVER
**Purpose**: Pause before targeting player
**Duration**: Random 0.75-1.0 seconds
**Movement**:
- Horizontal: None (stays in place)
- Vertical: Bobbing enabled (sine wave with bounds)
**Action**: When timer expires, sample player position
**Transition**: Timer expires → PULLBACK

---

### ③ PULLBACK
**Purpose**: Wind-up animation before charge
**Duration**: Fixed 0.25 seconds
**Movement**:
- Direction: Opposite of player direction
- Distance: 20 units * 4.0 speed multiplier
- Formula: `position += pullbackVector * deltaTime * 4.0`
**Calculation**:
```cpp
toPlayer = playerPos - ratPos
direction = normalize(toPlayer)
pullbackVector = -direction * 20.0
```
**Transition**: Timer expires → BEELINE

---

### ④ BEELINE
**Purpose**: High-speed charge toward target
**Duration**: Until offscreen
**Movement**:
- Direction: Locked toward player's position from PULLBACK state
- Speed: `150.0 * (currentSpeed / 60.0)` (difficulty scaling)
- Formula: `position += targetDirection * beelineSpeed * deltaTime`
**Exit Condition**: Any of:
- `X < -100`
- `X > screenWidth + 100`
- `Y < -100`
- `Y > screenHeight + 100`
**Transition**: Offscreen → Wrap → FLY_IN

---

## Code Integration Points

### Component Fields (Enemy)
```cpp
// State tracking
EnemyState currentState;  // FlyIn, Hover, Pullback, Beeline

// Hover state
float hoverTimer;         // Countdown timer

// Pullback state
GNVector2 targetDirection; // Locked direction to player
bool hasLockedDirection;   // Direction is locked
GNVector2 pullbackVector;  // Pullback movement vector
float pullbackTimer;       // Pullback duration timer

// Beeline state
float beelineSpeed;        // Final charge speed
```

### Initialization (on spawn)
```cpp
if (config.movementPattern == "flying") {
    enemyComp->currentState = EnemyState::FlyIn;
    enemyComp->hoverTimer = 0.0f;
    enemyComp->hasLockedDirection = false;
    enemyComp->pullbackTimer = 0.0f;
    enemyComp->beelineSpeed = 150.0f;
}
```

### State Machine (in ProcessEnemyMovement)
```cpp
if (enemy->movementPattern == "flying") {
    switch (enemy->currentState) {
        case EnemyState::FlyIn:
            // Move left, check trigger
            break;
        case EnemyState::Hover:
            // Bob in place, countdown
            break;
        case EnemyState::Pullback:
            // Move backwards, countdown
            break;
        case EnemyState::Beeline:
            // Charge toward target
            break;
    }
}
```

---

## Behavior Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Spawn X | `screenWidth + 650` | Offscreen right |
| Spawn Y Range | `25%-60%` of screen | Middle band |
| Hover Trigger | `75%` screen width | Three-quarters across |
| Hover Duration | `0.75-1.0` seconds | Random |
| Pullback Distance | `20` units | Opposite direction |
| Pullback Duration | `0.25` seconds | Fixed |
| Pullback Speed Mult | `4.0x` | Fast pullback |
| Beeline Base Speed | `150.0` | Scaled by difficulty |
| Difficulty Scaling | `speed / 60.0` | Current speed reference |
| Bobbing Enabled | All states except Pullback/Beeline | Visual interest |
| Safe Y Bounds | `100px` from top/bottom | Hover state only |

---

## Comparison to Original (Raylib)

### Preserved
✅ 4-state machine (FLY_IN → HOVER → PULLBACK → BEELINE)
✅ Hover duration randomization (0.75-1.0s)
✅ Pullback distance (20 units)
✅ Pullback duration (0.25s)
✅ Beeline speed calculation (150.0 base)
✅ Player position sampling on hover→pullback transition
✅ Direction locking

### Adapted
🔄 Screen coordinates (320x180 → device-specific via ConfigManager)
🔄 Trigger point (240px → 75% screen width)
🔄 Speed scaling (80.0 reference → 60.0 reference)
🔄 Offscreen check (bounds → screen + margin)

### Enhanced
✨ Bobbing during FLY_IN (not in original)
✨ Safe Y bounds during HOVER (prevents top/bottom escape)
✨ Wrap reset (returns to FLY_IN state cleanly)
✨ State logging for debugging

---

## Debugging Tips

### Log State Transitions
```cpp
GN_LOG_DEBUG("RatCopter: FLY_IN → HOVER (timer=" + std::to_string(hoverTimer) + "s)");
GN_LOG_DEBUG("RatCopter: HOVER → PULLBACK (target=" + std::to_string(playerX) + "," + std::to_string(playerY) + ")");
GN_LOG_DEBUG("RatCopter: PULLBACK → BEELINE (speed=" + std::to_string(beelineSpeed) + ")");
GN_LOG_DEBUG("RatCopter: BEELINE → offscreen (will wrap)");
```

### Visual State Indicators
Consider adding debug visualization:
- FLY_IN: Green tint
- HOVER: Yellow tint
- PULLBACK: Orange tint
- BEELINE: Red tint

### Common Issues
1. **Rat doesn't hover**: Check trigger point calculation (75% screen)
2. **Rat doesn't beeline**: Verify player position is being sampled correctly
3. **Rat moves wrong direction**: Check direction normalization and sign
4. **Rat gets stuck**: Ensure state transitions are not blocked
5. **Rat doesn't wrap**: Check offscreen bounds and wrap reset logic

---

## Performance Notes

- State machine adds minimal overhead (single switch statement)
- Direction calculation happens once per HOVER→PULLBACK transition
- Bobbing calculation shared with other enemy types
- No expensive operations in tight loops
- Player position lookup cached per frame in EnemySystem

---

## Future Enhancements

### Possible Improvements
1. **Variable beeline duration**: Add timeout to force wrap if stuck
2. **Evasion behavior**: Add small random offset to beeline direction
3. **Formation flying**: Coordinate multiple rats for group attacks
4. **Sound effects**: Add audio cues for each state transition
5. **Particle effects**: Smoke trail during beeline, dust on pullback
6. **Difficulty variants**: Different hover durations per difficulty level

### Configuration Suggestions
Consider making these values configurable in EnemyConfig:
- Hover duration range (min/max)
- Pullback distance
- Pullback duration
- Beeline speed multiplier
- Bobbing amplitude during states

---