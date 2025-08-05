# Animation Logic Gates Documentation

## Overview

This document describes the animation logic gates implemented in the FloppyTurd game to ensure proper animation behavior, particularly preventing infinite looping of non-looping animations like jump and shoot.

## System Architecture

The animation system consists of two main components:
1. **SpriteSystem** - Handles frame advancement and animation completion
2. **PlayerControllerSystem** - Manages animation state transitions and input handling

## SpriteSystem Animation Logic

### Frame Advancement Logic (UpdateSpriteAnimation)

```cpp
// In SpriteSystem::UpdateSpriteAnimation()
if (sprite->currentFrame >= sprite->frameCount) {
    if (sprite->loop) {
        sprite->currentFrame = 0;  // Loop back to start
    } else {
        sprite->currentFrame = sprite->frameCount - 1;  // Stay on last frame
        sprite->playing = false;   // Stop animation
        sprite->hasCompleted = true;  // Mark as completed
    }
}
```

**Purpose**: Controls how animations behave when they reach the last frame
- **Looping animations** (like idle): Reset to frame 0 and continue
- **Non-looping animations** (like jump/shoot): Stop on last frame and mark as completed

## PlayerControllerSystem Logic Gates

### Logic Gate 1: Same Animation Check
```cpp
if (m_currentAnimation == animationName) {
    // Continue to additional checks...
}
```
**Purpose**: Prevents unnecessary processing when trying to change to the same animation

### Logic Gate 2: Currently Playing Check
```cpp
if (sprite->playing) {
    GN_LOG_DEBUG("Animation already playing, skipping restart");
    return;
}
```
**Purpose**: Prevents restarting an animation that is currently playing

### Logic Gate 3: Non-loopable Completed Check
```cpp
if (!sprite->loop && sprite->hasCompleted && animationName != m_idleAnimation) {
    GN_LOG_DEBUG("Non-loopable animation has completed, skipping restart");
    return;
}
```
**Purpose**: Prevents restarting a non-looping animation that has already completed (except for idle)

### Logic Gate 4: Non-loopable Stopped Check
```cpp
if (!sprite->loop && !sprite->playing && animationName != m_idleAnimation) {
    GN_LOG_DEBUG("Non-loopable animation has stopped, skipping restart");
    return;
}
```
**Purpose**: Prevents restarting a non-looping animation that has stopped (except for idle)

## Input Handler Logic Gates

### Jump Input Handler
```cpp
if (sprite && m_currentAnimation != m_idleAnimation && (sprite->playing || sprite->hasCompleted)) {
    GN_LOG_DEBUG("Jump input ignored - currently in animation");
    return;
}
```
**Purpose**: Prevents jump input when any non-idle animation is playing OR has completed

### Shoot Input Handler
```cpp
if (sprite && m_currentAnimation != m_idleAnimation && (sprite->playing || sprite->hasCompleted)) {
    GN_LOG_DEBUG("Shoot input ignored - currently in animation");
    return;
}
```
**Purpose**: Prevents shoot input when any non-idle animation is playing OR has completed

## Animation Completion Detection

### UpdatePlayerAnimation Method
```cpp
// Check if current animation has completed - if so, return to idle
if (m_currentAnimation != m_idleAnimation && sprite->hasCompleted) {
    // Clear states and return to idle
    ChangePlayerAnimation(m_idleAnimation);
    return;
}

// Check if current animation has stopped unexpectedly
if (m_currentAnimation != m_idleAnimation && !sprite->playing && !sprite->hasCompleted) {
    // Handle unexpected stop
    ChangePlayerAnimation(m_idleAnimation);
    return;
}
```

**Purpose**: 
1. Detects when animations complete and automatically returns to idle
2. Handles unexpected animation stops (error recovery)

## Animation Properties by Type

### Idle Animation (TurdletIdle)
- **Frame Count**: 1 (single frame)
- **Frame Time**: 0.1f (not used)
- **Loop**: true
- **Is Animated**: false
- **Behavior**: Always playing, always restartable

### Jump Animation (TurdletJump)
- **Frame Count**: 6
- **Frame Time**: 0.2f
- **Loop**: false
- **Is Animated**: true
- **Behavior**: Play once, stop on last frame, mark as completed

### Shoot Animation (TurdletShoot)
- **Frame Count**: 5
- **Frame Time**: 0.15f
- **Loop**: false
- **Is Animated**: true
- **Behavior**: Play once, stop on last frame, mark as completed

### Hurt Animation (TurdletHurt)
- **Frame Count**: 6
- **Frame Time**: 0.18f
- **Loop**: false
- **Is Animated**: true
- **Behavior**: Play once, stop on last frame, mark as completed

## State Management

### Cooldown System
- **Jump Cooldown**: 0.1f seconds (prevents rapid jumping)
- **Shoot Cooldown**: 0.3f seconds (prevents rapid shooting)
- **Purpose**: Provides buffer time between actions, independent of animation status

### State Variables
- `m_isJumping`: Set when jump starts, cleared when jump animation completes
- `m_isShooting`: Set when shoot starts, cleared when shoot animation completes
- `m_currentAnimation`: Tracks the currently playing animation name

## Logic Flow Summary

1. **Input Received**: Touch input triggers jump or shoot
2. **Input Validation**: Check cooldowns and current animation state
3. **Animation Start**: If valid, start appropriate animation
4. **Frame Advancement**: SpriteSystem advances frames based on frameTime
5. **Completion Detection**: When last frame reached, mark as completed and stop
6. **State Cleanup**: PlayerControllerSystem detects completion and returns to idle
7. **Ready for Next Input**: System ready for next action

## Key Design Principles

1. **Animation-Driven State**: Animation completion drives state transitions, not timers
2. **Single Responsibility**: SpriteSystem handles frame logic, PlayerControllerSystem handles state logic
3. **Defensive Programming**: Multiple checks prevent edge cases and infinite loops
4. **Idle Priority**: Idle animation can always restart, other animations cannot restart once completed
5. **Cooldown Independence**: Cooldowns provide input buffering independent of animation status

## Debugging

The system includes extensive logging to track:
- Animation state changes
- Input handling decisions
- Frame advancement
- Completion detection
- State transitions

Use these logs to diagnose animation issues and verify logic gate behavior. 