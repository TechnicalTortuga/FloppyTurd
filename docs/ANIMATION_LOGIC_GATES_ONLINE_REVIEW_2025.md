# Animation State Logic Gates: Best Practices & Findings (2025)

## 1. General Principles
- **Animation Data vs Playback:**  
  Separate animation data (frames, duration, loop flag) from playback logic. Each entity should have its own animation player/controller that tracks current frame, time, and state.
- **Frame Advancement:**  
  Advance frames based on elapsed time (`deltaTime`). Use a frame timer and only increment the frame when the timer exceeds the frame duration.
- **Looping vs One-Shot:**  
  - **Looping:** Reset to frame 0 when reaching the end.
  - **One-Shot (Jump/Shoot):** Stop at the last frame, set a "completed" flag, and halt playback.

## 2. Single-Loop Animations (Jump/Shoot)
- **Best Practice:**  
  For non-looping animations, when the last frame is reached:
  - Set `currentFrame = frameCount - 1`
  - Set `playing = false`
  - Set `hasCompleted = true`
- **Triggering Logic:**  
  Use the `hasCompleted` flag to trigger game logic (e.g., allow next jump, fire, etc.) only after the animation finishes.
- **State Machine Integration:**  
  Use animation state machines (Unity, Godot) to manage transitions. For jump/shoot, transition out of the animation only when `hasCompleted` is true.

## 3. Callback/Events
- **Best Practice:**  
  Use callbacks or event systems to notify when an animation finishes (especially for one-shot actions).  
  Example:  
  - Godot: `animation_finished` signal.
  - Unity: Animation Events or StateMachineBehaviour.
  - Playdate: Use a sequence library or check `animator:ended()`.

## 4. Edge Cases
- **Frame Overflow:**  
  Always clamp `currentFrame` to `[0, frameCount-1]` for one-shot.
- **Interruptions:**  
  If a new animation is triggered before the previous one finishes, reset all relevant flags and timers.

## 5. References
- [Godot Docs: 2D Sprite Animation](https://docs.godotengine.org/en/stable/tutorials/2d/2d_sprite_animation.html)
- [Unity Manual: Animation State Machines](https://docs.unity3d.com/Manual/AnimationStateMachines.html)
- [GameDev StackExchange: Sprite Animation Best Practices](https://gamedev.stackexchange.com/questions/1434/sprite-animation-best-practices)
- [Playdate Dev Forum: Animation Completion Logic](https://devforum.play.date/t/multiple-animator-animations-on-one-sprite-run-logic-after-animation-finishes-whats-the-best-practice/14129)

---

## Your Implementation Review

- Your current logic for single-loop animations (jump/shoot) matches best practices:
  - Stops at last frame.
  - Sets `playing = false` and `hasCompleted = true`.
- **Recommendation:**  
  - Ensure your player controller/game logic checks `hasCompleted` before allowing new actions.
  - Consider adding callback/event support for animation completion if not present.
  - Double-check that `loop` is set correctly for jump/shoot animations.

---

## Sample Pseudocode for Single-Loop Animation

```cpp
if (sprite->currentFrameTime >= sprite->frameTime) {
    sprite->currentFrameTime = 0.0f;
    sprite->currentFrame++;
    if (sprite->currentFrame >= sprite->frameCount) {
        if (sprite->loop) {
            sprite->currentFrame = 0;
        } else {
            sprite->currentFrame = sprite->frameCount - 1;
            sprite->playing = false;
            sprite->hasCompleted = true;
            // Optionally: trigger callback/event here
        }
    }
}
```

---

## Conclusion

Your SpriteSystem logic gates for jump/shoot animations are now in line with current best practices.  
If you want to further modernize, consider adding event/callback support for animation completion and ensure your game logic uses the `hasCompleted` flag for state transitions.
