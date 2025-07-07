e# Roadmap: Adaptive Mobile UI Implementation

This document outlines the plan to refactor the game's UI from a fixed-resolution, letterboxed design to a responsive, adaptive layout that works natively on mobile devices.

---

### Phase 1: Establish the UI Management Foundation

This phase is about creating the core tools for a responsive layout without changing any existing game states yet.

- [ ] **1.1. Create `UIManager` Class:**
    - A central singleton class responsible for all layout calculations.
    - It will store screen dimensions and, crucially, the **safe area** rectangle provided by the OS.
    - It will provide functions to convert abstract positions (e.g., "top-center," "bottom-left with a 10px margin") into absolute pixel coordinates for drawing.

- [ ] **1.2. Define UI Anchors & Data Structures:**
    - Create an `enum UIAnchor` with values like `TOP_LEFT`, `CENTER`, `BOTTOM_RIGHT`, etc.
    - Create a simple struct to hold layout properties for a UI element, including its anchor point and pixel offsets.

---

### Phase 2: Refactor the Rendering Pipeline

Here, we'll split the rendering path. Desktop will continue as-is, while mobile will draw directly to the screen.

- [ ] **2.1. Conditional Rendering Path:**
    - In `Game::RenderFrame()`, use `#ifdef PLATFORM_MOBILE` to create two distinct rendering paths.
    - **Desktop (`#else`):** Will continue to use the existing `BeginTextureMode(renderTarget)`.
    - **Mobile (`#ifdef`):** Will **bypass** the `RenderTexture`. All draw calls will go directly to the main framebuffer.

- [ ] **2.2. Update Mobile Projection Matrix:**
    - The graphics projection matrix (for Metal and OpenGL) is currently hardcoded for a 320x180 world.
    - For mobile, change this to an orthographic projection that matches the device's actual screen dimensions.

---

### Phase 3: Implement the New Layout (Proof of Concept)

We'll start with the `Loading` screen to prove the new system works before refactoring the entire game.

- [ ] **3.1. Refactor `Loading` State:**
    - Modify `Loading::Draw()` to use the new `UIManager` for positioning.
    - It will ask the `UIManager` for the coordinates corresponding to the `CENTER` anchor.

- [ ] **3.2. Refactor Input Handling:**
    - On mobile, touch coordinates are in the screen's coordinate space. The current system scales touch input to match the 320x180 canvas.
    - Find this logic and wrap it in an `#ifndef PLATFORM_MOBILE` block so that mobile input is handled directly.

---

### Phase 4: Full UI Migration (Iterative)

Once the proof of concept is working, we will roll out the changes to the rest of the UI.

- [ ] **4.1. Refactor `MainMenu`:**
    - Each button and text element will be updated to use the `UIManager` for positioning.

- [ ] **4.2. Refactor `Playing` (HUD):**
    - The in-game HUD (score, health, pause button) will be refactored to use the `UIManager` for positioning within the safe area. 