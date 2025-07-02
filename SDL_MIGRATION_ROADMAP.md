# SDL Migration & iOS Platform Abstraction Roadmap

## Overview
This document outlines the step-by-step plan for migrating Floppy Turd from raylib to SDL2 (and related SDL libraries), with a focus on iOS support and platform abstraction. It includes a detailed checklist of all files in the codebase to track refactor progress.

---

## 1. Project Setup & Build System
- [ ] Set up SDL2, SDL2_image, SDL2_mixer, SDL2_ttf for all platforms.
- [ ] Update CMakeLists.txt to use SDL instead of raylib.
- [ ] Remove raylib from dependencies and package configs.

## 2. Platform Abstraction Layer
- [ ] Expand `PlatformLayer` for all platform-specific functionality (window, input, audio, file paths, vibration, etc.).
- [ ] Implement iOS-specific code using SDL's iOS backend and native APIs as needed.
- [ ] Use SDL's cross-platform APIs for desktop.

## 3. Window & Graphics
- [ ] Replace all raylib window and screen functions with SDL equivalents.
- [ ] Refactor `Window.cpp` and `Window.h` for SDL window/context creation, resizing, fullscreen.
- [ ] Replace all raylib drawing calls with SDL rendering and texture APIs.
- [ ] Update `TextureAtlas`, `Sprite`, and all rendering code to use SDL textures and renderers.

## 4. Input Handling
- [ ] Replace all raylib input functions with SDL input events and state queries.
- [ ] Refactor `PlatformLayer` and `TouchControls` to use SDL's input system.
- [ ] Implement gesture recognition (swipe, pinch, long press) using SDL touch/multitouch APIs and iOS-specific hooks if needed.

## 5. Audio
- [ ] Replace all raylib audio functions with SDL_mixer or SDL's audio APIs.
- [ ] Refactor `AudioManager`, `AudioClip`, `SoundManager`, and related classes to use SDL audio objects.

## 6. Fonts & Text
- [ ] Replace raylib font/text rendering with SDL_ttf.
- [ ] Update `AIGUI` and all UI code to use SDL_ttf for text rendering.

## 7. Resource Management
- [ ] Refactor `ResourceManager`, `ResourceCompat`, and all resource loading to use SDL's file APIs and SDL_image/SDL_mixer/SDL_ttf for loading assets.
- [ ] Update all texture, sound, and font loading to use SDL's APIs.

## 8. Game Loop & Timing
- [ ] Replace raylib's drawing/timing functions with SDL's event loop and timing functions.
- [ ] Ensure the main loop is compatible with iOS's requirements (may need to use SDL's iOS main loop integration).

## 9. Platform-Specific Features
- [ ] Implement iOS-specific features in `PlatformLayer` (safe area, orientation, virtual keyboard, vibration, etc.) using SDL and native iOS APIs as needed.
- [ ] For desktop, use SDL's cross-platform features.

## 10. Testing & Validation
- [ ] Build and test on all target platforms (iOS, macOS, Windows, Linux).
- [ ] Use SweetPad and real devices for gesture and touch testing on iOS.

---

# Detailed Checklist: All Files

## Core Game Files
- [ ] FloppyTurd/main.cpp
- [ ] FloppyTurd/Game.cpp
- [ ] FloppyTurd/Game.h
- [ ] FloppyTurd/Window.cpp
- [ ] FloppyTurd/Window.h
- [ ] FloppyTurd/PlatformLayer.cpp
- [ ] FloppyTurd/PlatformLayer.h
- [ ] FloppyTurd/GameSettings.h
- [ ] FloppyTurd/QuickplaySettings.h

## Rendering & UI
- [ ] FloppyTurd/AIGUI.cpp
- [ ] FloppyTurd/AIGUI.h
- [ ] FloppyTurd/TextureAtlas.cpp
- [ ] FloppyTurd/TextureAtlas.h
- [ ] FloppyTurd/Sprite.cpp
- [ ] FloppyTurd/Sprite.h
- [ ] FloppyTurd/StaticLayer.cpp
- [ ] FloppyTurd/StaticLayer.h
- [ ] FloppyTurd/AnimatedLayer.cpp
- [ ] FloppyTurd/AnimatedLayer.h
- [ ] FloppyTurd/AnimatedParallaxLayer.cpp
- [ ] FloppyTurd/AnimatedParallaxLayer.h
- [ ] FloppyTurd/ParallaxLayer.cpp
- [ ] FloppyTurd/ParallaxLayer.h
- [ ] FloppyTurd/SnowOverlay.cpp
- [ ] FloppyTurd/SnowOverlay.h

## Input & Touch
- [ ] FloppyTurd/TouchControls.cpp
- [ ] FloppyTurd/TouchControls.h

## Audio
- [ ] FloppyTurd/AudioManager.cpp
- [ ] FloppyTurd/AudioManager.h
- [ ] FloppyTurd/AudioClip.cpp
- [ ] FloppyTurd/AudioClip.h
- [ ] FloppyTurd/SoundManager.cpp
- [ ] FloppyTurd/SoundManager.h
- [ ] FloppyTurd/SoundEffect.cpp
- [ ] FloppyTurd/SoundEffect.h

## Resource Management
- [ ] FloppyTurd/ResourceManager.cpp
- [ ] FloppyTurd/ResourceManager.h
- [ ] FloppyTurd/ResourceCompat.h
- [ ] FloppyTurd/TextureCache.cpp
- [ ] FloppyTurd/TextureCache.h

## Gameplay & Entities
- [ ] FloppyTurd/Player.cpp
- [ ] FloppyTurd/Player.h
- [ ] FloppyTurd/Bird.cpp
- [ ] FloppyTurd/Bird.h
- [ ] FloppyTurd/Boss.cpp
- [ ] FloppyTurd/Boss.h
- [ ] FloppyTurd/BossHealthBar.cpp
- [ ] FloppyTurd/BossHealthBar.h
- [ ] FloppyTurd/Enemy.cpp
- [ ] FloppyTurd/Enemy.h
- [ ] FloppyTurd/Projectile.cpp
- [ ] FloppyTurd/Projectile.h
- [ ] FloppyTurd/Explosion.cpp
- [ ] FloppyTurd/Explosion.h
- [ ] FloppyTurd/Cactus.cpp
- [ ] FloppyTurd/Cactus.h
- [ ] FloppyTurd/Hat.cpp
- [ ] FloppyTurd/Hat.h
- [ ] FloppyTurd/PoopHeart.cpp
- [ ] FloppyTurd/PoopHeart.h
- [ ] FloppyTurd/PoopHeartType.h
- [ ] FloppyTurd/GoldToilets.cpp
- [ ] FloppyTurd/GoldToilets.h
- [ ] FloppyTurd/ToiletPaper.cpp
- [ ] FloppyTurd/ToiletPaper.h
- [ ] FloppyTurd/ToiletPaperProjectile.cpp
- [ ] FloppyTurd/ToiletPaperProjectile.h
- [ ] FloppyTurd/ToiletPair.cpp
- [ ] FloppyTurd/ToiletPair.h
- [ ] FloppyTurd/SpikeBall.cpp
- [ ] FloppyTurd/SpikeBall.h
- [ ] FloppyTurd/PickUp.cpp
- [ ] FloppyTurd/PickUp.h

## Levels & Managers
- [ ] FloppyTurd/Level.cpp
- [ ] FloppyTurd/Level.h
- [ ] FloppyTurd/LevelManager.cpp
- [ ] FloppyTurd/LevelManager.h
- [ ] FloppyTurd/DesertLevel.cpp
- [ ] FloppyTurd/DesertLevel.h
- [ ] FloppyTurd/ParkLevel.cpp
- [ ] FloppyTurd/ParkLevel.h
- [ ] FloppyTurd/SewerLevel.cpp
- [ ] FloppyTurd/SewerLevel.h
- [ ] FloppyTurd/SnowLevel.cpp
- [ ] FloppyTurd/SnowLevel.h
- [ ] FloppyTurd/CastleLevel.cpp
- [ ] FloppyTurd/CastleLevel.h
- [ ] FloppyTurd/BossLevel.cpp
- [ ] FloppyTurd/BossLevel.h

## Menus & UI
- [ ] FloppyTurd/MainMenu.cpp
- [ ] FloppyTurd/MainMenu.h
- [ ] FloppyTurd/MenuButton.cpp
- [ ] FloppyTurd/MenuButton.h
- [ ] FloppyTurd/Loading.cpp
- [ ] FloppyTurd/Loading.h
- [ ] FloppyTurd/Credits.cpp
- [ ] FloppyTurd/Credits.h

## Performance & Profiling
- [ ] FloppyTurd/PerformanceProfiler.cpp
- [ ] FloppyTurd/PerformanceProfiler.h

## Miscellaneous
- [ ] FloppyTurd/GameStats.cpp
- [ ] FloppyTurd/GameStats.h
- [ ] FloppyTurd/ResourceCompat.h
- [ ] FloppyTurd/ResourceManager.h.bak
- [ ] FloppyTurd/Packages.config
- [ ] FloppyTurd/CMakeLists.txt
- [ ] FloppyTurd/FloppyTurd.sln
- [ ] FloppyTurd/FloppyTurd.vcxproj
- [ ] FloppyTurd/FloppyTurd.vcxproj.filters
- [ ] FloppyTurd/BUILD.md
- [ ] FloppyTurd/MOBILE_REFACTORING_ROADMAP.md

---

# Platform Abstraction & SDL Migration Notes

- For each file, replace all raylib includes and functions with SDL equivalents.
- Use `#ifdef` or platform abstraction in `PlatformLayer` for iOS-specific code.
- For iOS, ensure the main loop and event handling are compatible with SDL's iOS integration.
- For rendering, use SDL textures, renderers, and surfaces.
- For input, use SDL's event system for keyboard, mouse, and touch/multitouch.
- For audio, use SDL_mixer for music and sound effects.
- For fonts, use SDL_ttf for all text rendering.
- For resource management, use SDL's file APIs and SDL_image/SDL_mixer/SDL_ttf for loading assets.
- For timing and game loop, use SDL's timing and event functions.
- For iOS-specific features (safe area, orientation, vibration, etc.), use SDL's APIs and native iOS code as needed.

---

**Check off each file as you refactor and test it.**

---

*This document will guide your migration and serve as a progress tracker as you port Floppy Turd from raylib to SDL for full cross-platform and iOS support.* 