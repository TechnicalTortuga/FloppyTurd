# Metal Migration Roadmap for FloppyTurd iOS

## Introduction
This roadmap outlines the remaining tasks and future goals for completing the Metal migration of FloppyTurd on iOS. The goal is to achieve a fully functional Metal-based rendering pipeline for improved performance and compatibility.

## Phase 1: Build Stabilization (Current)
- [x] Centralize raylib compatibility in `RaylibCompat.h/cpp`.
- [x] Retire `MetalRaylibCompat.h` and update all references.
- [x] Add necessary API shims and stubs for raylib functions.
- [ ] Achieve a clean build with no errors or warnings.

## Phase 2: Metal Integration
- [ ] Implement full Metal rendering for core graphics functions (e.g., `DrawTexture`, `DrawText`).
- [ ] Integrate Metal-based audio processing or provide suitable stubs.
- [ ] Optimize rendering performance using Metal's capabilities.

## Phase 3: Input and Interaction
- [ ] Fully implement touch input handling for iOS using `UpdateTouchState` and related functions.
- [ ] Test and refine safe area insets for proper UI layout on different iPhone models.

## Phase 4: Testing and Deployment
- [ ] Conduct extensive testing on physical iOS devices (various models and OS versions).
- [ ] Address device-specific issues (e.g., performance, rendering glitches).
- [ ] Prepare for App Store submission with Metal backend.

## Phase 5: Documentation and Maintenance
- [ ] Document Metal-specific code and compatibility layers for future developers.
- [ ] Update project README and wiki with Metal migration details.
- [ ] Establish a maintenance plan for future iOS SDK updates.

## Timeline
- **Phase 1 Completion**: Target end of July 2025 (or sooner with iterative build fixes).
- **Phase 2-3 Completion**: Target August 2025.
- **Phase 4-5 Completion**: Target September 2025 for full deployment readiness.

*Last Updated: July 4, 2025*
