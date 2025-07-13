# iOS Game Loop & Rendering Pipeline Analysis

## Overview
This document tracks the current state of the iOS game loop, rendering pipeline, and logging for the FloppyTurd project. It identifies missing or incomplete connections between the game logic and Metal rendering, and provides a checklist for full, non-stubbed implementation.

---

## 1. Game Loop & Rendering Flow (iOS)

### **Current Flow:**
- **GameViewController**
  - Owns a `CADisplayLink` that calls `gameLoopTick:` at 60 FPS.
  - In `gameLoopTick:`, calls `game->UpdateFrame(deltaTime)` (game logic update).
  - Then triggers `[_gameView render]` on the main thread.
- **GameView**
  - `render` method calls `[_mtkView draw]`.
  - This triggers the MetalKit delegate method `drawInMTKView:`.
- **drawInMTKView:**
  - Calls `_renderer->BeginFrame()`
  - **(MISSING)**: The actual game drawing (e.g., `Game::Draw()` or `Game::RenderFrame()`) is **not called here**.
  - Calls `_renderer->EndFrame()` and `_renderer->Present()`

### **Key Problem:**
- The MetalRenderer is setting up and presenting frames, but the game’s draw logic is never invoked on iOS. This results in a running game loop with no visible rendering.

---

## 2. Logging Situation
- **MetalRenderer** and **PlatformTraitsIOS** use a mix of `NSLog` and `TraceLog`.
- Only `TraceLog` writes to the FloppyTurdLogs file, which is essential for debugging on device/simulator.
- **Action Needed:** Replace all `NSLog` calls in MetalRenderer and PlatformTraitsIOS with `TraceLog` for unified, file-based logging.

---

## 3. Action Items & Checklist

### **A. Draw Loop Fixes**
- [ ] Ensure `Game::Draw()` (or equivalent) is called from within the MetalKit `drawInMTKView:` delegate method.
- [ ] Verify that all rendering state is properly set up before and after the draw call.
- [ ] Ensure frame timing and synchronization are correct.

### **B. Logging Consistency**
- [ ] Replace all `NSLog` calls in MetalRenderer and PlatformTraitsIOS with `TraceLog`.
- [ ] Add `TraceLog` calls for all major rendering, frame, and draw events.

### **C. Full, Non-Stubbed Implementation**
- [ ] Remove all stubbed or no-op functions in MetalRenderer and PlatformTraitsIOS.
- [ ] Implement all rendering, texture, and input functions fully.
- [ ] Ensure all platform-specific code paths are exercised and tested.

---

## 4. Next Steps
1. **Fix the draw loop:** Wire up the game’s draw logic to the MetalKit rendering delegate.
2. **Unify logging:** Replace all `NSLog` with `TraceLog` and add detailed logs for rendering events.
3. **Audit and implement:** Systematically remove all stubs and implement missing platform features.
4. **Test and verify:** Run the app, check logs, and ensure the game is rendering and updating as expected.

---

**This document will be updated as we proceed with implementation and fixes.** 