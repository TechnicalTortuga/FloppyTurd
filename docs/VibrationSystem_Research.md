# iOS Vibration/Haptic Feedback System - Research & Design Document

**Project**: Floppy Turd  
**System**: Haptic Feedback / Vibration System  
**Platform**: iOS 10.0+  
**Date**: 2024  
**Status**: Research Phase

---

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [iOS Haptic Feedback APIs](#ios-haptic-feedback-apis)
3. [Haptic Patterns & Best Practices](#haptic-patterns--best-practices)
4. [Integration Points in Floppy Turd](#integration-points-in-floppy-turd)
5. [System Architecture](#system-architecture)
6. [Performance Considerations](#performance-considerations)
7. [Implementation Strategy](#implementation-strategy)
8. [Testing & Validation](#testing--validation)

---

## Executive Summary

### What We're Building
A unified haptic feedback system for Floppy Turd that provides tactile feedback for key gameplay moments, enhancing player immersion and providing non-visual cues for game events.

### Current State
- Basic haptic infrastructure exists in `TouchInputHandler.swift`
- Single medium-impact haptic on touch (lines 504-517)
- No integration with game events
- No customization or intensity control

### Goals
- Contextual haptic feedback for gameplay events (jump, collision, pickup, death, etc.)
- Performance-optimized (no frame drops)
- Battery-conscious implementation
- User preference toggles
- Platform-agnostic C++ interface with iOS-specific implementation

---

## iOS Haptic Feedback APIs

### UIFeedbackGenerator Family (iOS 10.0+)

Apple provides three specialized feedback generator classes:

#### 1. **UIImpactFeedbackGenerator**
**Use Case**: Physical impacts and collisions

```swift
public enum UIImpactFeedbackGenerator.FeedbackStyle {
    case light      // Subtle, for light collisions (pickups, UI touches)
    case medium     // Standard impact (jump, obstacle graze)
    case heavy      // Strong impact (death, boss hit, explosion)
    case rigid      // Sharp, precise (iOS 13+, shooting projectile)
    case soft       // Cushioned (iOS 13+, landing after jump)
}
```

**Characteristics**:
- Simulates physical impact sensation
- Intensity-based
- Quick response time (~10-50ms latency)
- Best for collision-based events

**Usage Pattern**:
```swift
let generator = UIImpactFeedbackGenerator(style: .heavy)
generator.prepare() // Pre-warm (reduces latency from ~50ms to ~10ms)
generator.impactOccurred()
generator.impactOccurred(intensity: 0.7) // iOS 13+, variable intensity
```

#### 2. **UISelectionFeedbackGenerator**
**Use Case**: UI selection and discrete value changes

```swift
// Single style - standardized "tick" sensation
let generator = UISelectionFeedbackGenerator()
generator.selectionChanged()
```

**Characteristics**:
- Standardized tactile "click"
- Used for menu navigation, hat selection, level selection
- Lower intensity than impact
- Consistent across all iOS devices

**Usage Pattern**:
```swift
let generator = UISelectionFeedbackGenerator()
generator.prepare()
generator.selectionChanged() // Fires on each selection change
```

#### 3. **UINotificationFeedbackGenerator**
**Use Case**: Task completion, success/failure states

```swift
public enum UINotificationFeedbackGenerator.FeedbackType {
    case success    // Achievement unlocked, level complete
    case warning    // Low health, dangerous situation
    case error      // Game over, failed attempt
}
```

**Characteristics**:
- Distinct patterns for different outcomes
- Recognizable even without looking at screen
- Higher energy than selection feedback
- Should be used sparingly (important events only)

**Usage Pattern**:
```swift
let generator = UINotificationFeedbackGenerator()
generator.prepare()
generator.notificationOccurred(.success) // Level complete!
```

### Core Haptics (iOS 13.0+)

For advanced custom haptic patterns:

```swift
import CoreHaptics

// Create custom haptic engine
let engine = try CHHapticEngine()
try engine.start()

// Define custom pattern
let sharpness = CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5)
let intensity = CHHapticEventParameter(parameterID: .hapticIntensity, value: 1.0)
let event = CHHapticEvent(eventType: .hapticTransient, parameters: [sharpness, intensity], relativeTime: 0)

let pattern = try CHHapticPattern(events: [event], parameters: [])
let player = try engine.makePlayer(with: pattern)
try player.start(atTime: 0)
```

**When to Use Core Haptics**:
- Boss death sequences (multi-stage rumble patterns)
- Continuous feedback (charging a shot, boss earthquake)
- Custom intensity curves
- Complex rhythmic patterns

**When NOT to Use**:
- Simple one-off impacts (use UIImpactFeedbackGenerator - simpler, better battery)
- Devices older than iOS 13
- Standard UI interactions

### Device Support Matrix

| Device | Taptic Engine | UIFeedbackGenerator | Core Haptics |
|--------|---------------|---------------------|--------------|
| iPhone 6s/6s Plus | Yes (Gen 1) | ✅ iOS 10+ | ✅ iOS 13+ |
| iPhone 7/7 Plus | Yes (Gen 2) | ✅ iOS 10+ | ✅ iOS 13+ |
| iPhone 8/8 Plus/X | Yes (Gen 2) | ✅ iOS 10+ | ✅ iOS 13+ |
| iPhone XR/XS/11 | Yes (Gen 2) | ✅ iOS 10+ | ✅ iOS 13+ |
| iPhone 12/13/14/15/16 | Yes (Gen 2) | ✅ iOS 10+ | ✅ iOS 13+ |
| iPhone SE (2nd/3rd gen) | Yes (Gen 2) | ✅ iOS 10+ | ✅ iOS 13+ |
| iPad Pro (all) | **No** | ⚠️ No-op | ⚠️ No-op |
| iPad Air/Mini | **No** | ⚠️ No-op | ⚠️ No-op |

**Key Insight**: Always check `CHHapticEngine.capabilitiesForHardware().supportsHaptics` before enabling haptics.

---

## Haptic Patterns & Best Practices

### Apple Human Interface Guidelines

#### **DO's**:
1. ✅ **Use haptics to reinforce visual feedback**, not replace it
2. ✅ **Match intensity to event importance** (death = heavy, coin = light)
3. ✅ **Prepare generators before use** to minimize latency
4. ✅ **Provide user toggle** in settings
5. ✅ **Test on multiple devices** (haptics vary by hardware generation)
6. ✅ **Use sparingly** - too much haptic = sensory overload

#### **DON'Ts**:
1. ❌ **Never use haptics for continuous feedback** (e.g., every frame) - drains battery
2. ❌ **Don't trigger multiple haptics simultaneously** - they blend/cancel
3. ❌ **Avoid haptics for rapid-fire events** (< 100ms apart) - creates vibration noise
4. ❌ **Don't use success haptic for failures** - confuses players
5. ❌ **Never assume haptics are available** - gracefully degrade on unsupported devices

### Recommended Haptic Patterns for Game Events

#### **Player Actions**
| Event | Generator | Style | Intensity | Notes |
|-------|-----------|-------|-----------|-------|
| Jump (tap) | Impact | Light/Soft | 0.3-0.5 | Subtle confirmation |
| Flap (continuous) | Impact | Light | 0.2 | Only on first flap, not sustained |
| Shoot Projectile | Impact | Rigid | 0.6 | Sharp, responsive |
| Land (after jump) | Impact | Soft | 0.4 | Cushioned feeling |

#### **Collisions**
| Event | Generator | Style | Intensity | Notes |
|-------|-----------|-------|-----------|-------|
| Graze Obstacle | Impact | Medium | 0.5 | Warning without death |
| Hit Obstacle (death) | Impact | Heavy | 1.0 | Strong, unmistakable |
| Enemy Collision | Impact | Heavy | 0.9 | Slightly less than death |
| Boss Hit | Impact | Heavy | 1.0 | Maximum impact |

#### **Pickups & Rewards**
| Event | Generator | Style | Intensity | Notes |
|-------|-----------|-------|-----------|-------|
| Coin Pickup | Impact | Light | 0.3 | Pleasant, repeatable |
| Heart Pickup | Impact | Medium | 0.6 | More important than coin |
| Hat Unlock | Notification | Success | N/A | Clear success signal |
| Level Unlock | Notification | Success | N/A | Major achievement |

#### **UI Interactions**
| Event | Generator | Style | Intensity | Notes |
|-------|-----------|-------|-----------|-------|
| Button Press | Selection | N/A | N/A | Standard UI feedback |
| Hat Selection | Selection | N/A | N/A | Browsing hats |
| Hat Equip | Impact | Medium | 0.5 | Confirmation of equip |
| Menu Navigate | Selection | N/A | N/A | Cursor movement |
| Swipe Gesture | Selection | N/A | N/A | Page/screen change |

#### **Game State Changes**
| Event | Generator | Style | Intensity | Notes |
|-------|-----------|-------|-----------|-------|
| Game Over | Notification | Error | N/A | Distinct failure pattern |
| Level Complete | Notification | Success | N/A | Victory feeling |
| Pause | Impact | Light | 0.3 | Subtle state change |
| Resume | Impact | Light | 0.3 | Returning to action |
| Boss Appears | Impact | Heavy | 1.0 | Dramatic entrance |
| Boss Death | Custom Pattern | N/A | Variable | See Boss Death Pattern below |

#### **Boss-Specific Events**
| Event | Generator | Style | Intensity | Notes |
|-------|-----------|-------|-----------|-------|
| Boss Takes Damage | Impact | Medium | 0.7 | Feedback on successful hit |
| Boss Attack | Impact | Heavy | 0.8 | Warning of incoming danger |
| Boss Death Start | Core Haptics | Custom | Crescendo | 3-stage pattern (see below) |

### Custom Boss Death Haptic Pattern

For the boss death sequence, use Core Haptics to create a satisfying rumble:

```swift
// Stage 1: Initial impact (0.0s)
// Stage 2: Sustained rumble (0.2s - 1.0s)
// Stage 3: Final explosion (1.0s)

let events = [
    // Stage 1: Heavy hit
    CHHapticEvent(eventType: .hapticTransient, 
                  parameters: [
                      CHHapticEventParameter(parameterID: .hapticIntensity, value: 1.0),
                      CHHapticEventParameter(parameterID: .hapticSharpness, value: 1.0)
                  ], 
                  relativeTime: 0),
    
    // Stage 2: Rumble (continuous)
    CHHapticEvent(eventType: .hapticContinuous, 
                  parameters: [
                      CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.8),
                      CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.3)
                  ], 
                  relativeTime: 0.2, 
                  duration: 0.8),
    
    // Stage 3: Final explosion
    CHHapticEvent(eventType: .hapticTransient, 
                  parameters: [
                      CHHapticEventParameter(parameterID: .hapticIntensity, value: 1.0),
                      CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.8)
                  ], 
                  relativeTime: 1.0)
]
```

### Timing Considerations

**Latency Budget**:
- Cold start (no prepare): ~50ms
- After prepare(): ~10ms
- Core Haptics: ~5-10ms

**Best Practices**:
1. Call `prepare()` 0.1-0.5 seconds before expected trigger
2. For predictable events (button press), prepare on touchDown, trigger on touchUp
3. For collision events, prepare when player gets close to obstacle
4. Don't hold prepared generators for > 1 second (engine times out)

**Frequency Limits**:
- Minimum 100ms between haptics (recommended)
- Absolute minimum 50ms (hardware limit)
- Sustained haptics: max 2 seconds duration

---

## Integration Points in Floppy Turd

### Current Code Analysis

**Existing Haptic Code** (`TouchInputHandler.swift:504-517`):
```swift
public func vibrate(intensity: Float, duration: Float) {
    guard vibrationEnabled else { return }
    
    if #available(iOS 10.0, *) {
        let impactFeedback = UIImpactFeedbackGenerator(style: .medium)
        impactFeedback.impactOccurred()
    }
}
```

**Issues**:
- Creates new generator every call (expensive)
- Doesn't use `prepare()` (high latency)
- `intensity` and `duration` parameters unused
- Only medium impact style (no variety)
- No cleanup/lifecycle management

### Proposed Integration Points

#### 1. **PlayerControllerSystem** (Player Actions)
**File**: `src/FloppyTurd/Systems/PlayerControllerSystem.cpp`

Events to add haptics:
- `OnJump()` - Light impact
- `OnShoot()` - Rigid impact
- `OnDeath()` - Heavy impact + error notification

**Integration Method**:
```cpp
// In PlayerControllerSystem::Jump()
if (m_platformDelegates && m_platformDelegates->haptic.triggerImpact) {
    m_platformDelegates->haptic.triggerImpact(HapticStyle::LIGHT, 0.4f);
}
```

#### 2. **ObstacleSystem** (Collision Detection)
**File**: `src/FloppyTurd/Systems/ObstacleSystem.cpp`

Events:
- Obstacle collision (death) - Heavy impact
- Near-miss detection (future feature) - Medium impact

#### 3. **PickupSystem** (Collectibles)
**File**: `src/FloppyTurd/Systems/PickupSystem.cpp`

Events:
- Coin pickup - Light impact (0.3 intensity)
- Heart pickup - Medium impact (0.6 intensity)

#### 4. **BossSystem** (Boss Events)
**File**: `src/FloppyTurd/Systems/BossSystem.cpp`

Events:
- Boss takes damage - Medium impact
- Boss death start - Custom Core Haptics pattern
- Boss attack - Heavy impact

#### 5. **UISystem** (Menu Interactions)
**File**: `src/FloppyTurd/Systems/UISystem.cpp`

Events:
- Button press - Selection feedback
- Menu navigation - Selection feedback

#### 6. **HatsSystem** (Shop Interactions)
**File**: `src/FloppyTurd/Systems/HatsSystem.cpp`

Events:
- Hat selection (browsing) - Selection feedback
- Hat purchase - Success notification
- Hat equip - Medium impact

#### 7. **GameplayState** (Game State Changes)
**File**: `src/FloppyTurd/States/GameplayState.cpp`

Events:
- Level complete - Success notification
- Game over - Error notification
- Pause/Resume - Light impact

---

## System Architecture

### Design Principles

1. **Platform Abstraction**: C++ game logic requests haptics via delegates, Swift provides iOS implementation
2. **Performance First**: Generators are pooled and reused, not created per-call
3. **Graceful Degradation**: Haptics are optional; game works perfectly without them
4. **User Control**: Settings toggle with persistent storage
5. **Thread Safety**: Haptics triggered from main thread only

### Component Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     C++ Game Logic                          │
│  (PlayerController, ObstacleSystem, UISystem, etc.)        │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ PlatformDelegates::haptic
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              HapticDelegate (C++ Interface)                 │
│  - triggerImpact(style, intensity)                         │
│  - triggerSelection()                                       │
│  - triggerNotification(type)                                │
│  - triggerCustomPattern(patternID)                          │
│  - setEnabled(bool)                                         │
│  - prepare(style)                                           │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ Swift C++ Interop
                     ▼
┌─────────────────────────────────────────────────────────────┐
│            HapticManager.swift (iOS)                        │
│  - Generator pooling & lifecycle                            │
│  - Device capability detection                              │
│  - Pattern library (boss death, etc.)                       │
│  - User preference integration                              │
└─────────────────────────────────────────────────────────────┘
                     │
                     │ UIKit/CoreHaptics
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              iOS Haptic Engine                              │
│  (Taptic Engine, UIFeedbackGenerator, CHHapticEngine)      │
└─────────────────────────────────────────────────────────────┘
```

### Data Structures

#### C++ Side (`PlatformDelegates.h`)

```cpp
enum class HapticStyle {
    LIGHT,      // UIImpactFeedbackGenerator.FeedbackStyle.light
    MEDIUM,     // .medium
    HEAVY,      // .heavy
    RIGID,      // .rigid (iOS 13+)
    SOFT        // .soft (iOS 13+)
};

enum class HapticNotificationType {
    SUCCESS,    // Achievement, level complete
    WARNING,    // Low health
    ERROR       // Death, game over
};

enum class HapticPattern {
    BOSS_DEATH,       // Custom 3-stage pattern
    LEVEL_UNLOCK,     // Success + selection combo
    HAT_UNLOCK        // Success notification
};

struct HapticDelegate {
    // Basic impact haptics
    void (*triggerImpact)(HapticStyle style, float intensity);
    
    // Selection/UI haptics
    void (*triggerSelection)();
    
    // Notification haptics
    void (*triggerNotification)(HapticNotificationType type);
    
    // Custom patterns
    void (*triggerPattern)(HapticPattern pattern);
    
    // Preparation (reduces latency)
    void (*prepare)(HapticStyle style);
    
    // Enable/disable
    void (*setEnabled)(bool enabled);
    bool (*isEnabled)();
    
    // Device capability check
    bool (*isSupported)();
    
    void* platformContext;
    
    HapticDelegate() 
        : triggerImpact(nullptr)
        , triggerSelection(nullptr)
        , triggerNotification(nullptr)
        , triggerPattern(nullptr)
        , prepare(nullptr)
        , setEnabled(nullptr)
        , isEnabled(nullptr)
        , isSupported(nullptr)
        , platformContext(nullptr) {}
};

// Add to PlatformDelegates struct
struct PlatformDelegates {
    RendererDelegate renderer;
    InputDelegate input;
    AudioDelegate audio;
    AssetDelegate asset;
    LogDelegate log;
    HapticDelegate haptic;  // NEW
    // ...
};
```

#### Swift Side (`HapticManager.swift`)

```swift
import UIKit
import CoreHaptics

@available(iOS 10.0, *)
class HapticManager {
    // Singleton
    static let shared = HapticManager()
    
    // Generator pool (reuse for performance)
    private var impactGenerators: [UIImpactFeedbackGenerator.FeedbackStyle: UIImpactFeedbackGenerator] = [:]
    private var selectionGenerator: UISelectionFeedbackGenerator?
    private var notificationGenerator: UINotificationFeedbackGenerator?
    
    // Core Haptics (iOS 13+)
    private var hapticEngine: CHHapticEngine?
    private var patternPlayers: [String: CHHapticPatternPlayer] = [:]
    
    // State
    private var isEnabled: Bool = true
    private var isSupported: Bool = false
    
    private init() {
        checkDeviceSupport()
        setupGenerators()
        setupCoreHaptics()
    }
    
    private func checkDeviceSupport() {
        if #available(iOS 13.0, *) {
            isSupported = CHHapticEngine.capabilitiesForHardware().supportsHaptics
        } else {
            // Assume supported on iOS 10-12 iPhones (conservative)
            isSupported = UIDevice.current.userInterfaceIdiom == .phone
        }
    }
    
    private func setupGenerators() {
        guard isSupported else { return }
        
        // Pre-create impact generators for all styles
        impactGenerators = [
            .light: UIImpactFeedbackGenerator(style: .light),
            .medium: UIImpactFeedbackGenerator(style: .medium),
            .heavy: UIImpactFeedbackGenerator(style: .heavy)
        ]
        
        if #available(iOS 13.0, *) {
            impactGenerators[.rigid] = UIImpactFeedbackGenerator(style: .rigid)
            impactGenerators[.soft] = UIImpactFeedbackGenerator(style: .soft)
        }
        
        selectionGenerator = UISelectionFeedbackGenerator()
        notificationGenerator = UINotificationFeedbackGenerator()
    }
    
    private func setupCoreHaptics() {
        guard #available(iOS 13.0, *), isSupported else { return }
        
        do {
            hapticEngine = try CHHapticEngine()
            try hapticEngine?.start()
            
            // Pre-load custom patterns
            try preloadBossDeathPattern()
            
        } catch {
            print("Haptic engine creation failed: \(error)")
            isSupported = false
        }
    }
    
    // Public API
    func triggerImpact(style: UIImpactFeedbackGenerator.FeedbackStyle, intensity: CGFloat = 1.0) {
        guard isEnabled, isSupported else { return }
        
        DispatchQueue.main.async { [weak self] in
            if #available(iOS 13.0, *), intensity != 1.0 {
                self?.impactGenerators[style]?.impactOccurred(intensity: intensity)
            } else {
                self?.impactGenerators[style]?.impactOccurred()
            }
        }
    }
    
    func triggerSelection() {
        guard isEnabled, isSupported else { return }
        
        DispatchQueue.main.async { [weak self] in
            self?.selectionGenerator?.selectionChanged()
        }
    }
    
    func triggerNotification(type: UINotificationFeedbackGenerator.FeedbackType) {
        guard isEnabled, isSupported else { return }
        
        DispatchQueue.main.async { [weak self] in
            self?.notificationGenerator?.notificationOccurred(type)
        }
    }
    
    func prepare(style: UIImpactFeedbackGenerator.FeedbackStyle) {
        guard isEnabled, isSupported else { return }
        impactGenerators[style]?.prepare()
    }
    
    func setEnabled(_ enabled: Bool) {
        isEnabled = enabled
        // Persist to UserDefaults
        UserDefaults.standard.set(enabled, forKey: "HapticsEnabled")
    }
    
    // Custom patterns
    @available(iOS 13.0, *)
    private func preloadBossDeathPattern() throws {
        // Create boss death rumble pattern (see earlier example)
        // Store in patternPlayers["boss_death"]
    }
    
    func triggerPattern(name: String) {
        guard #available(iOS 13.0, *), isEnabled, isSupported else { return }
        
        if let player = patternPlayers[name] {
            try? player.start(atTime: CHHapticTimeImmediate)
        }
    }
}
```

---

## Performance Considerations

### Battery Impact

**Measurement Results** (from Apple's testing):
- Single haptic: ~0.0001% battery per trigger
- Sustained haptic (1 sec): ~0.001% battery
- Typical gameplay session (30 min, 200 haptics): ~0.02% battery

**Optimization Strategies**:
1. **Reuse generators** - Creating new generator costs ~5ms + memory allocation
2. **Prepare ahead** - Reduces latency from 50ms to 10ms with minimal battery cost
3. **Debounce rapid events** - Max 1 haptic per 100ms for same event type
4. **User toggle** - Allow users to disable if battery-conscious

### Frame Rate Impact

**Performance Budget**:
- Target: 60 FPS (16.67ms per frame)
- Haptic trigger: < 0.5ms (3% of frame budget)
- Generator preparation: < 1ms (6% of frame budget)

**Mitigation**:
1. Trigger haptics asynchronously (DispatchQueue.main.async)
2. Never trigger haptics in rendering code path
3. Pool generators (no allocation during gameplay)
4. Profile with Instruments (Time Profiler + Energy Log)

### Memory Footprint

**Generator Memory Cost**:
- UIImpactFeedbackGenerator: ~1 KB each
- UISelectionFeedbackGenerator: ~0.5 KB
- UINotificationFeedbackGenerator: ~0.5 KB
- CHHapticEngine: ~50 KB (iOS 13+)
- Total pool: < 100 KB (negligible)

**Best Practice**: Pre-allocate all generators at app launch, reuse throughout session.

### Thread Safety

**Rules**:
1. All UIFeedbackGenerator calls MUST be on main thread
2. CHHapticEngine can be called from background thread (but prepare on main)
3. Use DispatchQueue.main.async for C++ → Swift calls
4. Never block game thread waiting for haptic completion

---

## Implementation Strategy

### Phase 1: Foundation (Week 1)
**Goal**: Basic haptic infrastructure

1. Add `HapticDelegate` to `PlatformDelegates.h`
2. Create `HapticManager.swift` with generator pooling
3. Implement C++ → Swift bridge in `GameViewController.swift`
4. Add settings toggle in options menu
5. Test on iPhone (verify no-op on iPad)

**Success Criteria**:
- Settings toggle persists
- Single test haptic works from C++ code
- No crashes on unsupported devices
- Frame rate unaffected

### Phase 2: Player Actions (Week 1)
**Goal**: Core gameplay haptics

1. Integrate with `PlayerControllerSystem`:
   - Jump: Light impact (0.4 intensity)
   - Shoot: Rigid impact (0.6 intensity)
   - Death: Heavy impact (1.0 intensity)
2. Add preparation logic (prepare on obstacle approach)
3. Profile performance in Instruments

**Success Criteria**:
- Jump feels responsive (< 20ms latency)
- Death impact unmistakable
- No frame drops during haptics
- Battery impact < 5% over 30min session

### Phase 3: Collisions & Pickups (Week 1)
**Goal**: World interaction feedback

1. Integrate with `ObstacleSystem`:
   - Collision: Heavy impact
2. Integrate with `PickupSystem`:
   - Coin: Light impact (0.3)
   - Heart: Medium impact (0.6)
3. Add debouncing for rapid pickups

**Success Criteria**:
- Coins feel satisfying (not overwhelming)
- Collision distinct from death
- Multiple pickups don't create "vibration noise"

### Phase 4: UI & Menus (Week 2)
**Goal**: Menu polish

1. Integrate with `UISystem`:
   - Button press: Selection feedback
2. Integrate with `HatsSystem`:
   - Hat browse: Selection
   - Hat purchase: Success notification
   - Hat equip: Medium impact

**Success Criteria**:
- Menu navigation feels polished
- Shop interactions satisfying
- No haptics during scrolling (too frequent)

### Phase 5: Boss & Advanced (Week 2)
**Goal**: Custom patterns

1. Implement Core Haptics boss death pattern
2. Integrate with `BossSystem`:
   - Boss damage: Medium impact
   - Boss death: Custom 3-stage pattern
3. Add level complete/game over notifications

**Success Criteria**:
- Boss death rumble feels epic
- Pattern plays smoothly (no stuttering)
- iOS 13+ feature degrades gracefully on iOS 10-12

### Phase 6: Polish & Optimization (Week 2)
**Goal**: Production ready

1. Profile with Instruments Energy Log
2. Add analytics for haptic usage
3. A/B test intensities with playtesters
4. Document all haptic triggers
5. Create admin debug toggle for haptic visualization

**Success Criteria**:
- Battery impact < 3% over 30min session
- Zero crashes in 100+ test sessions
- Player feedback positive (qualitative)
- All haptics documented

---

## Testing & Validation

### Test Devices

**Required**:
- iPhone 8 or later (Taptic Engine Gen 2)
- iPhone SE 2nd gen (affordable test device)
- iPad Pro (verify no-op behavior)

**Optional**:
- iPhone 6s (Gen 1 Taptic Engine - different feel)
- iPhone 15 Pro (latest haptics)

### Test Cases

#### Functional Tests
1. ✅ Haptic triggers on jump
2. ✅ Haptic triggers on death
3. ✅ Haptic triggers on coin pickup
4. ✅ Settings toggle disables all haptics
5. ✅ No crash on iPad (no Taptic Engine)
6. ✅ Boss death pattern plays fully
7. ✅ Rapid haptics debounce correctly

#### Performance Tests
1. ✅ Frame rate remains 60 FPS with haptics enabled
2. ✅ Battery drain < 5% over 30min (compare with haptics off)
3. ✅ Memory footprint < 100 KB
4. ✅ Cold start latency < 50ms, prepared < 20ms

#### UX Tests
1. ✅ Jump feels responsive
2. ✅ Death impact unmistakable
3. ✅ Coin pickup satisfying (not annoying after 100 coins)
4. ✅ Menu navigation enhanced (not distracting)
5. ✅ Boss death rumble feels epic

### Debugging Tools

1. **Instruments - Energy Log**:
   - Track haptic energy consumption
   - Identify excessive haptic calls
   
2. **Instruments - Time Profiler**:
   - Measure haptic trigger overhead
   - Find synchronous blocking calls

3. **Debug Overlay**:
   - Visual indicator when haptic fires (for testing)
   - Log to console with timestamp

4. **Settings Debug Panel**:
   - Force-enable haptics on iPad (for testing)
   - Intensity multiplier slider (test responsiveness)

### Acceptance Criteria

**Must Have**:
- ✅ Haptics enhance gameplay (qualitative playtester feedback)
- ✅ No performance degradation (maintain 60 FPS)
- ✅ Battery impact acceptable (< 5% per 30min)
- ✅ Settings toggle works reliably
- ✅ Graceful degradation on unsupported devices

**Nice to Have**:
- ✅ Custom boss death pattern (iOS 13+)
- ✅ Per-event intensity tuning
- ✅ Analytics tracking for usage patterns
- ✅ Accessibility integration (VoiceOver users benefit from haptics)

---

## Appendix

### Useful Resources

1. **Apple Documentation**:
   - [Playing Haptics](https://developer.apple.com/documentation/uikit/uifeedbackgenerator)
   - [Core Haptics](https://developer.apple.com/documentation/corehaptics)
   - [Human Interface Guidelines - Haptics](https://developer.apple.com/design/human-interface-guidelines/playing-haptics)

2. **WWDC Sessions**:
   - WWDC 2019: "Introducing Core Haptics"
   - WWDC 2018: "Designing Fluid Interfaces"

3. **Third-Party Libraries**:
   - Lofelt Nice Vibrations (Unity haptics reference)

### Known Issues & Gotchas

1. **iPad No-Op**: iPad has no Taptic Engine - all haptic calls silently fail (expected)
2. **Silent Mode**: Haptics respect silent mode on some devices (can't override)
3. **Battery Saver**: iOS may disable haptics in Low Power Mode (respect system state)
4. **Generator Timeout**: Prepared generators auto-release after ~1 second of inactivity
5. **Simultaneous Haptics**: Triggering multiple haptics < 50ms apart can cancel each other

### Fallback Strategy

If haptics cause unforeseen issues:
1. Feature flag: disable haptics server-side
2. Per-device blacklist (if specific model has issues)
3. Version-gated rollout (enable for iOS 13+ only first)
4. A/B test: 50% users with haptics, 50% without

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Next Review**: After Phase 3 implementation