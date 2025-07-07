# FloppyTurd iOS Main Menu Refactor & Tracking

## Analysis: Current State

### UI Rendering: Metal (iOS) vs. Raylib (Desktop)
- **Platform-Agnostic Drawing:** Uses `RaylibCompat_iOS` to route draw calls to Metal on iOS, raylib elsewhere.
- **Main Menu Drawing:**
  - Desktop: `MainMenu::DrawDesktopUI()` uses fixed 320x180 layout.
  - Mobile: `MainMenu::DrawMobileUI()` uses `UIManager` for safe area, scaling, and anchors. Draws background, logo, and buttons for mobile.
- **Logo Handling:** Supports both "FinLogo" (the "F") and full "FloppyTurdLogo.png". The "F" is interactive for the fart sound.

### Input Handling: Touch (iOS) vs. Mouse/Keyboard (Desktop)
- **iOS:** Touch events in `GameViewController.mm` update input state via `UpdateTouchState`, used by platform-agnostic UI logic.
- **Desktop:** Uses raylib's mouse/keyboard input directly.

### Current Issues
- Buttons are misaligned and not covering the screen.
- No text on buttons (Metal text rendering is a stub).
- No button functionality (possible input mapping issue).
- Background image not filling the screen.

## Key Files
- `MainMenu.cpp` (UI logic)
- `UIManager.cpp`/`.h` (safe area, scaling)
- `RaylibCompat_iOS.mm`, `PlatformLayerDelegate.mm`, `MetalRenderer.mm` (drawing abstraction)
- `GameViewController.mm`, `PlatformLayer.mm`/`.h` (input handling)

## Refactor & Fix Plan

### 1. Implement Metal Text Rendering
- [ ] Implement real text rendering in `MetalRenderer::DrawText` (replace rectangle stub).
- [ ] Ensure button labels and UI text appear.

### 2. Button Layout & Hitboxes
- [ ] Verify `UIManager` safe area/scaling on iOS devices.
- [ ] Adjust button sizes/positions for mobile (80% width, proper spacing).
- [ ] Ensure button hitboxes match visuals and respond to touch.

### 3. Background Image
- [ ] Use the correct mobile main menu background asset.
- [ ] Scale background to fill the entire screen (respect safe area).

### 4. Logo Placement & Fart Sound
- [ ] Place logo at top center.
- [ ] Draw "F" as a separate interactive button.
- [ ] Play fart sound when "F" is pressed.

### 5. Input Mapping
- [ ] Ensure touch input is mapped to UI buttons.
- [ ] Test on multiple iOS devices and orientations.

### 6. Polish & Test
- [ ] Test layout and input on iPhone 16 and other devices.
- [ ] Polish spacing, scaling, and responsiveness.
- [ ] Verify all main menu functionality (navigation, sound, etc).

---

**Progress Tracking:**
- [ ] Metal text rendering
- [ ] Button layout
- [ ] Background image
- [ ] Logo/fart sound
- [ ] Input mapping
- [ ] Final polish/testing 