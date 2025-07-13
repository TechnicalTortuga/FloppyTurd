# Audit & Refactor Report: iOS-Specific Code in `Window.cpp`/`Window.h`

## Summary

The `Window` class currently contains a significant amount of iOS-specific and mobile-specific code, scattered throughout both the implementation and header files. This includes preprocessor guards, direct UIKit/Objective-C references, and platform checks. This approach makes the codebase harder to maintain, less readable, and more error-prone as platform logic is not cleanly separated.

## Issues Identified

- **Heavy Use of Preprocessor Guards:**
  - `#ifdef PLATFORM_MOBILE`, `#ifdef __APPLE__`, `#if !defined(__APPLE__) || !TARGET_OS_IPHONE`, etc. are scattered throughout the files.
- **Direct iOS/Objective-C References:**
  - UIKit types (`UIView`, `UIScreen`, `CGRect`, etc.) and iOS-specific APIs are referenced directly in the C++ code.
- **Platform-Specific Methods in Interface:**
  - Methods like `GetSafeArea()`, `SetPreferredOrientation(bool)`, and `ShouldUseLargerTouchTargets()` are present in the cross-platform interface, but only make sense on mobile/iOS.
- **Duplication and Indirection:**
  - Some methods simply forward to platform-specific implementations, adding unnecessary indirection.
- **Obscured Platform Boundaries:**
  - Platform logic is not clearly separated, making it difficult to reason about or extend for new platforms.

## Refactor Plan

### 1. **Extract Platform-Specific Logic**
- Move all iOS-specific code into a dedicated implementation file (e.g., `WindowIOS.mm` or `PlatformIOSWindow.mm`).
- Use a platform abstraction layer (e.g., `PlatformAPI` or traits) to expose only the necessary hooks to the cross-platform `Window` class.

### 2. **Minimize Preprocessor Guards in Core Files**
- Replace scattered preprocessor checks with a single, well-defined interface boundary.
- Use virtual methods or traits to provide platform-specific behavior.

### 3. **Refine the Window Interface**
- Remove or hide methods from the cross-platform interface that are not relevant to all platforms.
- Use composition or delegation to provide platform-specific features only where needed.

### 4. **Objective-C/C++ Boundary Management**
- Ensure all Objective-C code is isolated in `.mm` files and accessed via C++-friendly interfaces.
- Use `#ifdef __APPLE__` and `TARGET_OS_IOS` only in platform-specific files.

### 5. **Documentation and Comments**
- Clearly document which files are platform-specific and how the abstraction boundary is maintained.

## Implementation Tracker

- [x] **Audit all iOS-specific code in `Window.cpp`/`Window.h`**
- [x] **Create `WindowIOS.mm` for iOS-specific implementation**
- [x] **Move all UIKit/Objective-C code to `.mm` file**
- [x] **Refactor `Window` interface to remove mobile-only methods**
- [x] **Update platform abstraction layer to provide necessary hooks**
- [x] **Replace scattered preprocessor guards with clean boundaries**
- [ ] **Test on iOS and desktop to ensure no regressions**
- [ ] **Document the new structure and boundaries**

### Completed Tasks:
1. ✅ **Created `WindowIOS.mm`** with full iOS-specific window management including:
   - Safe area handling for notches and home indicators
   - Screen density detection
   - Orientation switching with proper UIKit integration
   - Touch target sizing
   - Font size recommendations
   - Render scaling

2. ✅ **Created `WindowIOS.h`** with proper C++ interface declarations

3. ✅ **Updated `PlatformTraits.h`** to include window-specific functions in both IOSTraits and RaylibTraits

4. ✅ **Refactored `Window.h`** to remove mobile-specific methods from the cross-platform interface

5. ✅ **Refactored `Window.cpp`** to:
   - Remove all iOS-specific implementations
   - Use platform abstraction (CurrentTraits) for remaining functions
   - Remove scattered preprocessor guards
   - Clean up the codebase

6. ✅ **Updated `CMakeLists.txt`** to include the new WindowIOS.mm file

### Next Steps:
- Test the build on both iOS and desktop
- Verify orientation switching works correctly
- Ensure all window functionality is preserved

---

**Notes:**
- This refactor will improve maintainability, readability, and extensibility for future platforms.
- All platform-specific code should be isolated and clearly marked, with minimal impact on the cross-platform core. 