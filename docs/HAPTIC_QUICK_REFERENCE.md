# Haptic System - Quick Reference Card

## 🎯 Quick Start (Copy & Paste Ready)

### Include Headers
```cpp
#include "Engine/Platform/PlatformDelegates.h"
#include "Engine/Platform/HapticHelpers.h"
```

### Most Common Usage
```cpp
// Jump
HapticHelpers::TriggerJump(m_delegates);

// Collision
HapticHelpers::TriggerCollision(m_delegates, impactForce);

// Button Press
HapticHelpers::TriggerButtonPress(m_delegates);

// Level Complete
HapticHelpers::TriggerLevelComplete(m_delegates);

// Boss Death (custom pattern)
HapticHelpers::TriggerBossDeath(m_delegates);
```

---

## 📋 All Helper Functions

### Player Actions
```cpp
HapticHelpers::TriggerJump(delegates);
HapticHelpers::TriggerLanding(delegates, landingForce);  // force: 0.0-1.0
HapticHelpers::TriggerDash(delegates);
```

### Collisions
```cpp
HapticHelpers::TriggerCollision(delegates, impactForce);  // auto-scales style
HapticHelpers::TriggerLightCollision(delegates);
HapticHelpers::TriggerHeavyCollision(delegates);
```

### Pickups
```cpp
HapticHelpers::TriggerCoinPickup(delegates);
HapticHelpers::TriggerPowerupPickup(delegates);
HapticHelpers::TriggerRareItemPickup(delegates);
```

### UI Interactions
```cpp
HapticHelpers::TriggerButtonPress(delegates);
HapticHelpers::TriggerMenuNavigation(delegates);
HapticHelpers::TriggerToggle(delegates);
HapticHelpers::TriggerValueChange(delegates);
```

### Game State
```cpp
HapticHelpers::TriggerLevelStart(delegates);
HapticHelpers::TriggerLevelComplete(delegates);
HapticHelpers::TriggerLevelUnlock(delegates);
HapticHelpers::TriggerGameOver(delegates);
HapticHelpers::TriggerPause(delegates);
HapticHelpers::TriggerResume(delegates);
```

### Boss Events
```cpp
HapticHelpers::TriggerBossAppearance(delegates);
HapticHelpers::TriggerBossAttack(delegates);
HapticHelpers::TriggerBossDamage(delegates);
HapticHelpers::TriggerBossDeath(delegates);  // Custom pattern!
HapticHelpers::TriggerBossPhaseChange(delegates);
```

### Shop/Hats
```cpp
HapticHelpers::TriggerHatUnlock(delegates);  // Custom pattern!
HapticHelpers::TriggerHatEquip(delegates);
HapticHelpers::TriggerPurchaseSuccess(delegates);
HapticHelpers::TriggerPurchaseFailure(delegates);
```

### Warnings/Alerts
```cpp
HapticHelpers::TriggerLowHealth(delegates);
HapticHelpers::TriggerDangerWarning(delegates);
HapticHelpers::TriggerCountdownTick(delegates);
```

### Preparation (Performance)
```cpp
HapticHelpers::PrepareBossBattle(delegates);    // Before boss fight
HapticHelpers::PrepareUIInteraction(delegates); // Before opening menu
HapticHelpers::PrepareGameplay(delegates);      // At level start
```

---

## 🔧 Direct API (Advanced)

### Impact Feedback
```cpp
if (m_delegates.haptic.triggerImpact) {
    m_delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.8f);
    m_delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 1.0f);
    m_delegates.haptic.triggerImpact(HapticStyle::HEAVY, 1.0f);
    m_delegates.haptic.triggerImpact(HapticStyle::RIGID, 0.9f);  // iOS 13+
    m_delegates.haptic.triggerImpact(HapticStyle::SOFT, 0.7f);   // iOS 13+
}
```

### Selection Feedback
```cpp
if (m_delegates.haptic.triggerSelection) {
    m_delegates.haptic.triggerSelection();
}
```

### Notification Feedback
```cpp
if (m_delegates.haptic.triggerNotification) {
    m_delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
    m_delegates.haptic.triggerNotification(HapticNotificationType::WARNING);
    m_delegates.haptic.triggerNotification(HapticNotificationType::ERROR);
}
```

### Pattern Playback
```cpp
if (m_delegates.haptic.triggerPattern) {
    m_delegates.haptic.triggerPattern("boss_death");    // ~2 second sequence
    m_delegates.haptic.triggerPattern("level_unlock");  // ~0.4 second
    m_delegates.haptic.triggerPattern("hat_unlock");    // ~0.4 second
}
```

### Preparation
```cpp
if (m_delegates.haptic.prepare) {
    m_delegates.haptic.prepare(HapticStyle::HEAVY);  // Before triggering
}
```

---

## 📊 Haptic Styles Guide

| Style | Feel | Use Case | Example |
|-------|------|----------|---------|
| LIGHT | Subtle, gentle | UI taps, light touches | Button press, menu scroll |
| MEDIUM | Moderate | Standard collisions | Wall hit, enemy collision |
| HEAVY | Strong, intense | Major impacts | Boss attack, death |
| RIGID | Sharp, precise | Quick actions | Dash, parry (iOS 13+) |
| SOFT | Cushioned, gentle | Soft landings | Cloud bounce (iOS 13+) |

---

## ⚡ Integration Examples

### PlayerControllerSystem
```cpp
void handleJump() {
    // Player jumped
    HapticHelpers::TriggerJump(m_delegates);
}

void handleLanding(float velocity) {
    // Scale haptic by fall speed
    float force = std::min(std::abs(velocity) / 100.0f, 1.0f);
    HapticHelpers::TriggerLanding(m_delegates, force);
}
```

### ObstacleSystem
```cpp
void onCollisionWithObstacle(float damage) {
    // Scale by damage amount
    float impact = std::min(damage / 50.0f, 1.0f);
    HapticHelpers::TriggerCollision(m_delegates, impact);
}
```

### UISystem
```cpp
void onMenuItemChanged() {
    // Menu navigation feedback
    HapticHelpers::TriggerMenuNavigation(m_delegates);
}

void onButtonPressed() {
    // Button press feedback
    HapticHelpers::TriggerButtonPress(m_delegates);
}
```

### BossSystem
```cpp
void onBossEncounterStart() {
    // Prepare for heavy haptics
    HapticHelpers::PrepareBossBattle(m_delegates);
    HapticHelpers::TriggerBossAppearance(m_delegates);
}

void onBossDeath() {
    // Dramatic 2-second haptic sequence
    HapticHelpers::TriggerBossDeath(m_delegates);
}
```

---

## ✅ Best Practices

### DO ✅
- Match haptics to visuals/audio timing
- Scale intensity based on event magnitude
- Use consistent haptics for similar events
- Prepare before known haptic triggers
- Test on real devices (iPhone 7+)

### DON'T ❌
- Overuse haptics (max ~20/minute)
- Use haptics just for decoration
- Trigger on every frame/continuous events
- Ignore device capabilities
- Use random intensities

---

## 🎮 Pattern Descriptions

### boss_death (~2 seconds)
```
Phase 1: Warning tremors (0-0.5s) - 3 escalating impacts
Phase 2: Massive impact (0.5s) - Maximum intensity
Phase 3: Rumble aftermath (0.55-1.2s) - Continuous fade
Phase 4: Decay pulses (1.2-2.0s) - 3 decreasing impacts
```

### level_unlock (~0.4 seconds)
```
Celebratory ascending: 4 impacts with increasing intensity
```

### hat_unlock (~0.4 seconds)
```
Playful double-tap: 2 quick taps + strong finale
```

---

## 🔍 Debugging

### Check Console Logs
```
[HapticManager] [INFO] Device haptic support: true
[HapticManager] [INFO] Core Haptics engine started
[CommandProcessor] Haptic impact triggered: style=light intensity=0.8
```

### Common Issues
```
No haptics? → Check device (iPhone 7+), Settings, Low Power Mode
Weak haptics? → Use prepare(), increase intensity
Too many haptics? → Reduce frequency, lighter styles
```

---

## 📱 Device Support

| Device | Support | Notes |
|--------|---------|-------|
| iPhone 7+ | ✅ Full | Taptic Engine |
| iPhone 6s | ⚠️ Limited | Vibration only |
| iPad | ❌ None | No haptics |
| Simulator | ❌ None | Test on device |

---

## 🚀 Quick Integration Steps

1. **Include headers** in your system
2. **Add haptic calls** at event points
3. **Test on real device** (iPhone 7+)
4. **Fine-tune** intensity/style
5. **User test** for feedback

---

## 📚 Full Documentation

- **Usage Guide**: `docs/HapticUsageGuide.md`
- **Examples**: `docs/examples/HapticIntegrationExample.cpp`
- **Implementation**: `docs/HapticSystemImplementation.md`
- **Research**: `docs/VibrationSystem_Research.md`

---

**Last Updated**: 2024  
**Status**: ✅ Ready for Use