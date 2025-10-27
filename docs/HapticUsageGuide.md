# Haptic Feedback System - Usage Guide

## Overview

The haptic feedback system provides a simple, thread-safe API for triggering haptic feedback from C++ game code. Commands are queued through the threading system and executed on the main thread by the Swift HapticManager.

## Architecture

```
C++ Game Code
    ↓ (calls haptic delegates)
PlatformDelegates.haptic.*
    ↓ (enqueues commands)
ThreadingProxy
    ↓ (command queue)
CommandProcessor (Swift)
    ↓ (executes on main thread)
HapticManager (Swift)
    ↓ (triggers feedback)
iOS UIFeedbackGenerator / Core Haptics
```

## Available Haptic Types

### 1. Impact Feedback
Used for physical interactions, collisions, and taps.

**Styles:**
- `LIGHT` - Subtle, gentle feedback (UI taps, light touches)
- `MEDIUM` - Moderate feedback (standard collisions)
- `HEAVY` - Strong feedback (major impacts, boss attacks)
- `RIGID` - Sharp, precise feedback (iOS 13+)
- `SOFT` - Cushioned, gentle feedback (iOS 13+)

### 2. Selection Feedback
Used for UI navigation and menu scrolling.

### 3. Notification Feedback
Used for important game events.

**Types:**
- `SUCCESS` - Positive events (level complete, pickup collected)
- `WARNING` - Cautionary events (low health)
- `ERROR` - Negative events (game over, failed action)

### 4. Pattern Feedback
Complex haptic sequences for special events.

**Built-in Patterns:**
- `boss_death` - Dramatic escalating sequence for boss defeats
- `level_unlock` - Celebratory ascending pattern
- `hat_unlock` - Playful double-tap pattern

## Usage Examples

### Basic Impact Feedback

```cpp
#include "Engine/Platform/PlatformDelegates.h"

// In your game system (e.g., CollisionSystem, PlayerController)
void handlePlayerJump() {
    // Trigger light haptic feedback for jump
    if (m_delegates.haptic.triggerImpact) {
        m_delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.8f);
    }
}

void handleCollision(float impactForce) {
    // Scale haptic intensity based on collision force
    HapticStyle style = impactForce > 100.0f ? HapticStyle::HEAVY : HapticStyle::MEDIUM;
    float intensity = std::min(impactForce / 150.0f, 1.0f);
    
    if (m_delegates.haptic.triggerImpact) {
        m_delegates.haptic.triggerImpact(style, intensity);
    }
}
```

### Selection Feedback

```cpp
void handleMenuNavigation() {
    // Trigger selection feedback when scrolling through menu items
    if (m_delegates.haptic.triggerSelection) {
        m_delegates.haptic.triggerSelection();
    }
}
```

### Notification Feedback

```cpp
void handleLevelComplete() {
    // Trigger success notification
    if (m_delegates.haptic.triggerNotification) {
        m_delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
    }
}

void handleGameOver() {
    // Trigger error notification
    if (m_delegates.haptic.triggerNotification) {
        m_delegates.haptic.triggerNotification(HapticNotificationType::ERROR);
    }
}

void handleLowHealth() {
    // Trigger warning notification
    if (m_delegates.haptic.triggerNotification) {
        m_delegates.haptic.triggerNotification(HapticNotificationType::WARNING);
    }
}
```

### Pattern Feedback

```cpp
void handleBossDeath() {
    // Trigger dramatic boss death haptic sequence
    if (m_delegates.haptic.triggerPattern) {
        m_delegates.haptic.triggerPattern("boss_death");
    }
}

void handleLevelUnlock() {
    // Trigger celebratory level unlock pattern
    if (m_delegates.haptic.triggerPattern) {
        m_delegates.haptic.triggerPattern("level_unlock");
    }
}

void handleHatUnlock() {
    // Trigger playful hat unlock pattern
    if (m_delegates.haptic.triggerPattern) {
        m_delegates.haptic.triggerPattern("hat_unlock");
    }
}
```

### Preparation (Performance Optimization)

```cpp
void prepareForBossEncounter() {
    // Pre-warm haptic generator before boss battle
    // This reduces latency for the first haptic trigger
    if (m_delegates.haptic.prepare) {
        m_delegates.haptic.prepare(HapticStyle::HEAVY);
    }
}
```

## Integration Points

### PlayerControllerSystem

```cpp
// src/FloppyTurd/Systems/PlayerControllerSystem.cpp

void PlayerControllerSystem::handleJump() {
    // ... existing jump logic ...
    
    // Add haptic feedback
    if (m_delegates.haptic.triggerImpact) {
        m_delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.7f);
    }
}
```

### ObstacleSystem (Collision Detection)

```cpp
// src/FloppyTurd/Systems/ObstacleSystem.cpp

void ObstacleSystem::onCollision(Entity player, Entity obstacle) {
    // ... existing collision logic ...
    
    // Add haptic feedback based on obstacle type
    if (m_delegates.haptic.triggerImpact) {
        HapticStyle style = isHardObstacle ? HapticStyle::HEAVY : HapticStyle::MEDIUM;
        m_delegates.haptic.triggerImpact(style, 1.0f);
    }
}
```

### PickupSystem

```cpp
// src/FloppyTurd/Systems/PickupSystem.cpp

void PickupSystem::collectPickup(Entity pickup) {
    // ... existing pickup logic ...
    
    // Add haptic feedback for pickup collection
    if (m_delegates.haptic.triggerNotification) {
        m_delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
    }
}
```

### UISystem (Menu Interactions)

```cpp
// src/FloppyTurd/Systems/UISystem.cpp

void UISystem::onButtonPress() {
    // ... existing button logic ...
    
    // Add haptic feedback for button press
    if (m_delegates.haptic.triggerImpact) {
        m_delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.8f);
    }
}

void UISystem::onMenuScroll() {
    // ... existing scroll logic ...
    
    // Add haptic feedback for menu navigation
    if (m_delegates.haptic.triggerSelection) {
        m_delegates.haptic.triggerSelection();
    }
}
```

### BossSystem

```cpp
// src/FloppyTurd/Systems/BossSystem.cpp

void BossSystem::onBossAttack() {
    // ... existing attack logic ...
    
    // Add haptic feedback for boss attack
    if (m_delegates.haptic.triggerImpact) {
        m_delegates.haptic.triggerImpact(HapticStyle::HEAVY, 1.0f);
    }
}

void BossSystem::onBossDeath() {
    // ... existing death logic ...
    
    // Trigger dramatic boss death haptic sequence
    if (m_delegates.haptic.triggerPattern) {
        m_delegates.haptic.triggerPattern("boss_death");
    }
}
```

### GameplayState (Game State Changes)

```cpp
// src/FloppyTurd/States/GameplayState.cpp

void GameplayState::onLevelComplete() {
    // ... existing level complete logic ...
    
    // Add haptic feedback for level completion
    if (m_delegates.haptic.triggerNotification) {
        m_delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
    }
}

void GameplayState::onGameOver() {
    // ... existing game over logic ...
    
    // Add haptic feedback for game over
    if (m_delegates.haptic.triggerNotification) {
        m_delegates.haptic.triggerNotification(HapticNotificationType::ERROR);
    }
}
```

## Best Practices

### DO's ✅

1. **Match haptics to visuals/audio** - Trigger haptics at the same time as visual/audio feedback
2. **Use appropriate intensity** - Scale intensity based on event magnitude
3. **Be consistent** - Use the same haptic type for similar interactions
4. **Prepare for known events** - Use `prepare()` before predictable haptic triggers
5. **Test on real devices** - Haptics feel different on different devices

### DON'Ts ❌

1. **Don't overuse** - Too many haptics become annoying and drain battery
2. **Don't use for decoration** - Only trigger haptics for meaningful events
3. **Don't use random haptics** - Use consistent patterns for similar events
4. **Don't ignore user preferences** - Respect system haptic settings (handled automatically)
5. **Don't use for continuous events** - Haptics should be discrete, not continuous

## Performance Considerations

### Battery Impact
- Haptic feedback uses the Taptic Engine, which consumes battery
- Limit haptic frequency to important events only
- Typical usage: 10-20 haptic events per minute is acceptable

### Latency
- Haptic commands are queued and processed on next frame
- Use `prepare()` to reduce first-trigger latency
- Typical latency: < 16ms (1 frame at 60 FPS)

### Thread Safety
- All haptic calls are thread-safe (queued through ThreadingProxy)
- Commands execute on main thread automatically
- No manual synchronization needed

## Device Support

### Supported Devices (Full Taptic Engine)
- iPhone 7 and later
- iPhone SE (2nd gen) and later
- All iPhone X series and later

### Limited Support (Vibration Motor)
- iPhone 6s and earlier
- iPad (all models) - No haptic support

### Checking Support
The system automatically checks device support at initialization. On unsupported devices, haptic calls are silently ignored.

## Debugging

### Logging
Enable haptic logging in debug builds:
```cpp
// Haptic commands are logged at TRACE level
// Check logs for:
// - "[CommandProcessor] Haptic impact triggered"
// - "[CommandProcessor] Haptic selection triggered"
// - "[CommandProcessor] Haptic notification triggered"
// - "[CommandProcessor] Haptic pattern triggered"
```

### Testing Without Device
The iOS Simulator does not support haptic feedback. You must test on a real device with Taptic Engine support.

### Console Output
```
[HapticManager] [INFO] Device haptic support: true
[HapticManager] [INFO] Core Haptics engine started
[HapticManager] [INFO] Common haptic patterns preloaded
[CommandProcessor] Haptic impact triggered: style=light intensity=0.8
```

## Common Issues

### Haptics Not Working
1. Check device support (iPhone 7+)
2. Check system Settings → Sounds & Haptics → System Haptics is enabled
3. Check device is not in Low Power Mode
4. Verify code is calling delegates (not null)

### Weak or Delayed Haptics
1. Use `prepare()` before triggering
2. Reduce command queue depth (process commands more frequently)
3. Check for main thread blocking

### Battery Drain
1. Reduce haptic frequency
2. Use lighter haptic styles
3. Only trigger on important events

## Additional Resources

- [Apple Human Interface Guidelines - Haptics](https://developer.apple.com/design/human-interface-guidelines/playing-haptics)
- [Core Haptics Documentation](https://developer.apple.com/documentation/corehaptics)
- [UIFeedbackGenerator Documentation](https://developer.apple.com/documentation/uikit/uifeedbackgenerator)

## Future Enhancements

### Custom Patterns
To add custom haptic patterns, modify `HapticManager.swift`:

```swift
@available(iOS 13.0, *)
private func preloadCustomPattern() throws {
    let events: [CHHapticEvent] = [
        // Define your custom haptic sequence
        CHHapticEvent(
            eventType: .hapticTransient,
            parameters: [
                CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.8),
                CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5),
            ], relativeTime: 0.0),
        // Add more events...
    ]
    
    let pattern = try CHHapticPattern(events: events, parameters: [])
    let player = try hapticEngine?.makePlayer(with: pattern)
    patternPlayers["my_custom_pattern"] = player
}
```

Then call from C++:
```cpp
m_delegates.haptic.triggerPattern("my_custom_pattern");
```

## Summary

The haptic system provides a simple, performant way to add tactile feedback to your game. Use it thoughtfully to enhance the player experience without overwhelming them. Remember to test on real devices and respect system settings for the best user experience.