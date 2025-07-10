
# Touch Control Refactoring & Consolidation Plan

## 1. Summary of Work Completed

Our efforts to resolve the iOS touch input issues have involved a deep refactoring and consolidation of the input handling pipeline. Here is a summary of our journey:

*   **Initial Diagnosis:** We began by diagnosing a complete lack of touch input on the iOS simulator. Initial investigation confirmed that UIKit touch events (`touchesBegan`, `touchesEnded`) were being received by `GameView.mm` but were not being processed correctly by the game logic.
*   **Synchronization Issues:** We identified and fixed a critical frame-synchronization bug. The game loop was processing input *after* the game state had already updated, causing a one-frame lag that made transient inputs like presses and releases impossible to detect. We rectified this by introducing a dedicated `HandleInputFrame()` call at the beginning of the main game loop.
*   **State Management Flaw:** We found that the `pressed` and `released` flags were being cleared in the same frame they were set, preventing any other system (like `AIGUI`) from ever reading them. We corrected this by moving the state-clearing logic to the beginning of the next input frame.
*   **Root Cause Discovery: Dual Input Systems:** The most significant breakthrough was the discovery of two conflicting touch input systems running in parallel:
    1.  A legacy, instance-based `_touchControls` system.
    2.  A newer, static `TouchControls` system intended for a frame-accurate buffer.
    These two systems were corrupting each other's state, leading to unpredictable behavior.
*   **Consolidation and Refactoring:** We deprecated the instance-based system and refactored all input handling to use the static `TouchControls` class as the single source of truth. This involved routing all touch events from `GameView.mm` exclusively to the static methods.
*   **Multi-Touch Implementation:** To support gameplay actions like jumping and shooting simultaneously, we enhanced the static `TouchControls` system by adding a `MultiTouchBuffer` to track up to three concurrent touch points and calculate pinch gestures.

## 2. Intended Input Pipeline Architecture

The goal is a clean, single-source-of-truth pipeline that reliably captures and processes input once per frame.

1.  **Entry Point (OS Layer):** `GameView.mm` receives raw touch events from iOS (UIKit) via `touchesBegan`, `touchesMoved`, and `touchesEnded`.
2.  **Data Forwarding (Bridge):** For each event, `GameView.mm` immediately calls the appropriate static C++ method (`TouchControls::SetTouchState` or `TouchControls::SetMultiTouchState`), passing the touch coordinates and down status. This acts as a simple bridge, placing raw data into a buffer.
3.  **Frame Processing (Game Loop):** At the beginning of each frame, the `Game::Update` method calls `HandleInputFrame()`. This function orchestrates the entire input update.
4.  **State Update (Core Logic):** Inside `HandleInputFrame()`, the first step is to call `TouchControls::UpdateTouchState()`. This is the critical function that:
    *   Compares the current raw input state with the processed state from the *previous frame*.
    *   Sets the `pressed` and `released` flags based on this comparison. These flags will remain true for exactly one game frame.
5.  **Polling (Game Logic):** Throughout the rest of the game loop, various systems poll the now-stable state from the static `TouchControls` class:
    *   **UI:** The `AIGUI_UpdateInput()` function reads the touch state to determine button clicks, calling `TouchControls::IsTouchPressed()`, `TouchControls::GetTouchPosition()`, etc.
    *   **Gameplay States:** The `HandleInput()` method within each game state (e.g., `Gameplay`, `MainMenu`) checks for player actions by calling methods like `TouchControls::IsTouchDown()` (for continuous actions) or `TouchControls::IsTouchPressed()` (for single-fire events).
6.  **State Cleanup:** The final step within `HandleInputFrame()` is to call `TouchControls::ClearTransientStates()` to reset the `pressed` and `released` flags, preparing them for the next frame.

## 3. Current Issue & The Fix

**The Final Bug:** The `touchPressed` flag is never being set. Log analysis shows that if `touchesBegan` and `touchesEnded` events are sent by the OS in rapid succession (within the same game frame), our logic updates the `previousDown` state prematurely. This causes the crucial `!previousDown && isDown` condition for a "press" to be missed.

**Actionable Steps to Fix:**

1.  **Introduce a Raw Input Buffer:**
    *   In `TouchControls.h`, modify the `TouchInputBuffer` struct to include a separate set of "raw" variables: `bool rawIsDown;` and `Vector2 rawTouchPosition;`.
    *   `SetTouchState` (called from `GameView.mm`) will **only** write to these `raw` variables. It should no longer modify the main `isDown` or `touchPosition` variables.

2.  **Isolate State Processing Logic:**
    *   The `TouchControls::UpdateTouchState()` function (called once per game frame from `HandleInputFrame`) will now be the *only* place where the main input state is calculated.
    *   Implement the following logic inside `UpdateTouchState()`:
        ```cpp
        // 1. Latch the state from the previous game frame.
        touchBuffer.previousDown = touchBuffer.isDown;

        // 2. Consume the latest raw input from the OS events.
        touchBuffer.isDown = touchBuffer.rawIsDown;
        touchBuffer.touchPosition = touchBuffer.rawTouchPosition;

        // 3. Calculate the transient states for this frame.
        touchBuffer.pressed = touchBuffer.isDown && !touchBuffer.previousDown;
        touchBuffer.released = !touchBuffer.isDown && touchBuffer.previousDown;
        ```

3.  **Add Debug Logging:**
    *   To verify the fix, add `tracelog` statements at key points:
        *   In `GameView.mm`, to confirm `touchesBegan/Ended` are firing.
        *   In `TouchControls::SetTouchState`, to see the raw values being set.
        *   At the beginning and end of `TouchControls::UpdateTouchState`, to log `isDown`, `previousDown`, `rawIsDown`, `pressed`, and `released` to see the state transition in action.

4.  **Apply to Multi-Touch:**
    *   Repeat the same "raw buffer" logic for the `MultiTouchBuffer` to ensure it is also robust.

## 4. Best Practices & Supporting Research

The proposed fix aligns with established best practices for handling input in games. The core problem is a race condition between asynchronous OS events and the synchronous game loop. The industry-standard solution is to decouple them.

*   **The Decoupled Update Pattern:** Research confirms that a robust input system should not process logic directly in OS-driven event handlers (`touchesBegan`, etc.). Instead, these handlers should do the absolute minimum: capture the raw input data and place it into a simple buffer. A separate function, called once per frame from the main game loop, is then responsible for processing that buffer, updating the game's official input state, and calculating transient states like `pressed` or `released`.
*   **Supporting Examples:**
    *   A post on the **"The Brain Dump"** blog about cross-platform touch input describes an almost identical architecture for unifying iOS, Android, and Emscripten. It defines a platform-agnostic `touchEvent` struct that is populated by platform-specific listeners and then fed into gesture recognizers.
    *   Microsoft's own UWP documentation for adding touch controls to a DirectX game demonstrates the same pattern. Event handlers like `OnPointerPressed` only record state (`m_panInUse = TRUE`, `m_panFirstDown = position`). A separate `Update` method, called from the main loop, reads this state and computes the camera movement.
    *   The "Cowboy Programming" blog's article on Stylus Control emphasizes visualizing input and accounting for "sloppy" user actions by processing raw stroke data into a clean, intended direction—a process that can only happen in a controlled, per-frame update, not in a flurry of raw events.

This research validates our approach: `GameView.mm` becomes a simple data forwarder, and `TouchControls::UpdateTouchState` becomes the single source of truth for what the input state is *for that frame*.

## 5. Prompt for Next AI Chat Session

> The iOS touch input system has been refactored to a single, static `TouchControls` manager, but it currently fails to register `touchPressed` events due to a race condition. As detailed in **Touch_Input_Refactor_Plan.md**, the definitive fix is to decouple the raw OS events from the synchronous game loop by using a raw input buffer.
>
> Your task is to implement this fix by following the plan in the markdown file:
> 1.  Modify `TouchInputBuffer` and `MultiTouchBuffer` in `TouchControls.h` to include "raw" state variables (e.g., `rawIsDown`, `rawTouchPosition`).
> 2.  Update `TouchControls::SetTouchState` and `SetMultiTouchState` in the Objective-C++ implementation to only write to these new `raw` variables.
> 3.  Rewrite the logic in the C++ `TouchControls::UpdateTouchState` and `UpdateMultiTouchState` methods to consume the raw state, correctly latch the `previousDown` state from the last frame, and reliably calculate the `pressed` and `released` flags for the current frame.
> 4.  Ensure debug logs are in place to verify the state transitions. 