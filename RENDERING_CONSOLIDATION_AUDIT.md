# Rendering Consolidation Audit Report (2025-08-08)

This report documents concrete, code-cited findings about the current rendering flow, duplicate paths, coordinate usage, and debug overlays. No code changes were made.

## Findings

- __[SystemManager fallback enables duplicate rendering]__
  - File: `src/Engine/Core/SystemManager.cpp`, function: `SystemManager::Render()`
  - If `m_renderSystem` is null, it falls back to calling `m_spriteSystem->Render()` and `m_uiSystem->Render()`.
  - Impact: Keeps legacy parallel pipelines alive and can cause duplicate UI/sprite rendering when unified pipeline is intended.

- __[GameplayState renders UI twice (duplication)]__
  - File: `src/FloppyTurd/States/GameplayState.cpp`, function: `GameplayState::Render()`
  - Calls `m_renderSystem->Render();` and then `m_uiSystem->Render();` (see grep results around lines ~181–189).
  - Impact: `RenderSystem` already renders `UIElement` entities; calling `UISystem::Render()` duplicates UI.

- __[State-local systems create redundant/parallel pipelines]__
  - File: `src/FloppyTurd/States/MainMenuState.cpp`, constructor
    - Instantiates local `SpriteSystem` and local `RenderSystem` (for queries and screen info), but `MainMenuState::Render()` uses `m_ecsCoordinator->Render();` which delegates to `SystemManager::Render()`.
    - Impact: Redundant systems risk resource duplication or desync; unnecessary once `SystemManager` orchestrates rendering.
  - File: `src/FloppyTurd/States/GameplayState.cpp`
    - Uses member `m_renderSystem` and `m_uiSystem` directly for rendering instead of delegating to `SystemManager`.
    - Impact: Bypasses orchestration, fragments the pipeline.

- __[Direct renderer calls from states bypass RenderSystem]__
  - File: `src/FloppyTurd/States/GameplayState.cpp`, function: `DrawDebugRectangles()`
    - Uses `delegates.renderer.drawRectangle(...)` directly in multiple places.
  - File: `src/FloppyTurd/States/MainMenuState.cpp`, function: `DrawButtonDebugRectangles()` and other debug helpers
    - Uses `delegates.renderer.drawRectangle(...)` directly.
  - Impact: Bypasses `RenderSystem` layering/sorting. Debug overlays should be ECS-driven (e.g., via `DebugDraw`) and rendered inside `RenderSystem` for consistent ordering.

- __[UISystem performs independent rendering (confirmed duplication)]__
  - File: `src/FloppyTurd/Systems/UISystem.cpp`, function: `UISystem::Render()`
  - Independently collects `Transform + UIElement` and renders them.
  - Impact: Conflicts with `RenderSystem::CollectRenderItems()` which already gathers and renders UI; causes double-render.

- __[Coordinate usage: logical dimensions used in UI responsiveness]__
  - File: `src/FloppyTurd/Systems/UISystem.cpp`
    - `UpdateScreenInfo()` logs and stores both logical and pixel dimensions.
    - `GetResponsiveScale()` and `CalculateResponsivePosition()` explicitly use `logicalWidth`/`logicalHeight` as references.
  - Memory constraints require using pixel coordinates for rendering/positioning. Using logical values for layout/position risks misplacement under Metal.
  - Note: `RenderSystem` also computes `GetDynamicScale()` based on logical height (reference 852). While used for layout/scaling logs, ensure any final positions sent to renderer are in pixels.

- __[RenderSystem collection includes DebugDraw but no circle draw usage found]__
  - File: `src/FloppyTurd/Systems/RenderSystem.cpp`, function: `CollectRenderItems()` shows collection of `Transform + DebugDraw` with flags like `showBounds` and `showCollider`.
  - Platform supports circles: `PlatformDelegates.renderer.drawCircle` is wired (`iOSPlatformImpl`, `RaylibPlatformImpl`, `ThreadingProxy`, `MetalRenderer`).
  - No explicit circle debug rendering path was found in `RenderSystem` (e.g., to draw collider circles). Current debug items appear rectangle-oriented.
  - Impact: Circle colliders (e.g., player/projectiles) can’t be visualized yet via the unified pipeline.

- __[States call into ECS/engine render inconsistently]__
  - `MainMenuState::Render()` calls `m_ecsCoordinator->Render();` (which should route to `SystemManager::Render()`), aligning with centralization.
  - `LoadingState.cpp` also calls `m_ecsCoordinator->Render();` consistently.
  - `GameplayState::Render()` diverges (calls state-local systems).

## Risks

- __Duplicate UI rendering__ leads to Z-order conflicts, inconsistent visuals, and extra GPU work.
- __Parallel pipelines__ (state-local vs SystemManager) complicate lifecycle, initialization, and platform resource handling.
- __Direct draw calls__ from states can violate pixel coordinate invariants and skip sorting/layering, causing overlays to appear above/below incorrectly.
- __Logical dimension usage__ in positioning risks invisible/misaligned UI on Metal, per critical constraints.
- __Missing circle debug rendering__ hinders debugging of circle colliders.

## Recommendations (no code changes yet)

- __Make RenderSystem the sole renderer__
  - In `SystemManager::Render()`, remove legacy fallbacks to `SpriteSystem::Render()` and `UISystem::Render()`; log and skip when `RenderSystem` missing.
  - Ensure all states call `m_ecsCoordinator->Render()` (or `SystemManager::Render()`) only; remove state-local direct calls to system `Render()`.

- __Eliminate duplicate UI rendering__
  - Stop calling `UISystem::Render()` from `GameplayState::Render()`.
  - Keep `UISystem` focused on UI entity creation, input, and state management; rendering belongs in `RenderSystem`.

- __Unify debug overlays in RenderSystem__
  - Replace direct `drawRectangle` calls in states with ECS-driven `DebugDraw` data.
  - Add circle support: extend `DebugDraw` to describe circle bounds (center, radius, color, alpha) and implement corresponding render path using `renderer.drawCircle`.

- __Enforce pixel-based positioning__
  - In `UISystem`, avoid using `logicalWidth`/`logicalHeight` for final positions/sizes. Use `ScreenInfo.pixelWidth/pixelHeight` and raw pixel coordinates when computing UI element transforms/safe areas.
  - Audit `RenderSystem` to ensure any scaling factors do not convert to logical space when emitting draw calls; keep final coordinates as pixels.

- __Remove state-local redundant systems__
  - From `MainMenuState` and `GameplayState`, remove construction/use of local `RenderSystem`/`SpriteSystem` for runtime rendering.
  - If needed for queries (e.g., texture dimensions), factor those helpers into non-render utilities to avoid initializing full systems.

- __Test plan__
  - Verify no duplicate UI with on/off toggles for `UISystem::Render()` calls removed.
  - Validate z-ordering: backgrounds, world sprites, UI, then debug overlays.
  - Confirm pixel-accurate placement across iPhone 16, iPad, and desktop; compare against `ScreenInfo.pixelWidth/pixelHeight`.
  - Enable new circle debug overlay and visually confirm collider alignment.

## Notable Code References

- `src/Engine/Core/SystemManager.cpp`: `SystemManager::Render()`, `InitializeSystems()`
- `src/FloppyTurd/Systems/RenderSystem.cpp`: `Render()`, `CollectRenderItems()`, `RenderWorldSpace()`, `RenderScreenSpace()`, `UpdateScreenInfo()`, `GetDynamicScale()`
- `src/FloppyTurd/Systems/UISystem.cpp`: `Render()`, `UpdateScreenInfo()`, `GetResponsiveScale()`, `CalculateResponsivePosition()`
- `src/FloppyTurd/Systems/SpriteSystem.cpp`: `Render()`
- `src/FloppyTurd/States/MainMenuState.cpp`: constructor, `Render()`, debug helpers
- `src/FloppyTurd/States/GameplayState.cpp`: `Render()`, `DrawDebugRectangles()`
- Circle support plumbing: `PlatformDelegates.h` (drawCircle), `iOSPlatformImpl.*`, `RaylibPlatformImpl.*`, `ThreadingProxy.*`, `MetalRenderer.swift`

## Open Questions (to confirm before changes)

- Should `UISystem` be retained solely for creation/input and completely detached from rendering responsibilities? (Recommended yes.)
- Any non-ECS UI drawing intentionally left out of `RenderSystem` (e.g., platform-native overlays) that must remain direct? If so, document expected layering.
- Are there existing `DebugDraw` fields intended to represent circles, or should new fields be added (center/radius)?

---

# Comprehensive System Inventories and Connections

This section enumerates what UISystem and SpriteSystem process, update, and render; their helpers; and how everything is initialized/connected.

## UISystem Inventory (`src/FloppyTurd/Systems/UISystem.cpp`)

- __[Initialization]__
  - `UISystem::UISystem(...)`
    - Stores `m_ecsCoordinator`, `m_delegates`.
    - Initializes screen info and layout: `UpdateScreenInfo()`, `SetupLayout()`.
    - Tracks `m_screenInfoValid`.

- __[Update Loop]__
  - `Update(float deltaTime)`
    - Currently no per-frame logic except placeholder for future animations/hover effects.

- __[Render Path]__
  - `Render()`
    - Queries `GetEntitiesWithComponents<Transform, UIElement>()`.
    - Filters via `IsEntityVisible(entity)` and `UIElement.isEnabled && UIElement.visible`.
    - Sorts by UI layer (uses `uiElement.textLayer` in the visible list, lower renders first).
    - For each, calls `RenderUIElement(entity, *transform, *uiElement)`.
  - `RenderUIElement(entity, transform, uiElement)`
    - Requires `delegates.renderer.drawText`.
    - Chooses font size from `uiElement.fontSize` (no extra scaling inside this function).
    - Chooses text color based on `uiElement.isHovered` (uses `textColor` vs `textHoverColor`, RGBA normalized by 255.0f).
    - Computes text position from `transform.position` (screen space).
    - Draws button text via `drawText` (implied by naming; exact call in this function) at pixel coordinates.

- __[UI Creation & Utilities]__
  - `CreateButton(text, x, y, scale, normalTex, hoverTex, pressedTex, fontSize)`
    - Creates an entity and adds `Transform`, `Sprite`, and `UIElement` with button state.
  - `CreateButtonWithBounds(text, x, y, boundsWidth, boundsHeight, normalTex, hoverTex, pressedTex, fontSize)`
    - Variant that sets explicit bounds for hit testing/layout.
  - `UpdateButtonSprite(entity)` / `ResetAllButtonStates()`
    - Manages sprite visual based on button state (normal/hover/pressed) by setting `Sprite.textureId` etc.
  - `IsPointInBounds(entity, x, y)` / `GetEntityBounds(entity, left, right, top, bottom)`
    - Hit testing using `Transform` and `UIElement` bounds (screen space).

- __[Screen/Layout & Responsiveness]__
  - `UpdateScreenInfo()`
    - If available: `delegates.renderer.getScreenInfo(&m_screenInfo)`; logs both logical and pixel dimensions; sets `m_screenInfoValid`.
    - Legacy fallback: `getScreenSize(&logicalWidth, &logicalHeight)` and mirrors logical → pixel; sets `scaleFactor`, `isPortrait`, `deviceModel`.
  - `GetResponsiveScale()`
    - Uses `m_screenInfo.logicalHeight / 852.0f` (iPhone 16 logical reference).
  - `GetUIScale()`
    - Clamps `GetResponsiveScale()` to [0.8, 2.0].
  - `SetupLayout()` with platform branches:
    - `SetupIOSLayout()` / `SetupDesktopLayout()`
    - Logs scale, intended for safe area and platform-specific tweaks.
  - `GetSafeArea(left, top, right, bottom)`
    - Default safe area uses logical dimensions; iOS heuristic: top=44, bottom=logicalHeight-34 for iPhone models.
  - `CalculateResponsivePosition(base, isHorizontal)`
    - Scales positions using logical width/height references (393/852 for iPhone 16).
  - `CalculateResponsiveSize(base)`
    - Multiplies by `GetUIScale()`.

- __[Data/Components Touched]__
  - Reads: `Transform`, `UIElement`.
  - Writes: `UIElement` (buttonText updates elsewhere), `Sprite` in `UpdateButtonSprite`.
  - Delegates: `renderer.drawText`, `renderer.getScreenInfo` or `renderer.getScreenSize`.

- __[Important Notes for Migration]__
  - Rendering duplicates `RenderSystem` and must be disabled/removed.
  - Logical-dimension-based layout helpers exist; final rendering coordinates must remain in pixels for Metal.
  - Creation, input, hit testing should remain in `UISystem`.

## SpriteSystem Inventory (`src/FloppyTurd/Systems/SpriteSystem.cpp`)

- __[Initialization]__
  - `SpriteSystem::SpriteSystem(...)`
    - Stores `m_ecsCoordinator`, `m_delegates`.
    - Initializes `m_textureBasePath`, texture caches (`m_textureCache`, `m_textureDimensions`), pending set.

- __[Update Loop]__
  - `Update(float deltaTime)`
    - Iterates `GetEntitiesWithComponents<Sprite>()`.
    - For animated sprites (`sprite.isAnimated && sprite.playing`), advances animation via `UpdateSpriteAnimation(sprite, deltaTime)`.
  - `UpdateSpriteAnimation(Sprite* sprite, float dt)`
    - Handles frame timing, wrap/clamp, and frame index updates.

- __[Render Path]__
  - `Render()`
    - Queries `GetEntitiesWithComponents<Transform, Sprite>()`.
    - Filters via `IsEntityVisible(entity)` (checks `sprite->visible`).
    - Collects `(entity, sprite.layer)`, sorts ascending by layer.
    - For each, calls `RenderSprite(entity, *transform, *sprite)`.
  - `RenderSprite(entity, transform, sprite)`
    - Obtains or loads texture handle via `GetOrLoadTexture(sprite.textureId, entity)`.
    - Computes source rect via `CalculateSourceRect(sprite)` (frameX/frameY/frameW/frameH for animated sheets or full texture).
    - Uses `delegates.renderer` draw call (implementation specific; consistent with pixel coordinates) to draw at `transform.position` with `transform.scale` and sprite width/height.
    - Handles centered vs top-left origin if a `RotationRenderer` or similar component is present (referenced in other files), ensuring correct rotation pivot.

- __[Animation & Control APIs]__
  - `PlayAnimation(entity)`, `PauseAnimation(entity)`, `StopAnimation(entity)`, `SetAnimationFrame(entity, frame)`
    - Mutate `Sprite` state flags/frame.

- __[Texture & Asset Handling]__
  - `SetTextureBasePath(basePath)`
    - For desktop; iOS uses asset catalog IDs directly.
  - `GetOrLoadTexture(textureId, entity)`
    - Returns cached handle or triggers async load.
  - `HandleTextureLoaded(TextureData*, const char* error, void* userData)`
    - Completion callback that stores platform texture handle and dimensions; removes from `m_pendingTextures`.
  - `LoadTexture(textureId, filePath)`
    - Validates existence via `delegates.asset.fileExists`; logs and returns if missing.
  - `GetFullTexturePath(textureId)`
    - Returns resolved path or passes-through for iOS.
  - `GetTextureDimensions(textureId)`
    - Returns cached `(width, height)`; `{0,0}` if unknown.

- __[Visibility & Helpers]__
  - `IsEntityVisible(entity)`
    - Returns `sprite && sprite->visible`; can be extended for culling.

- __[Data/Components Touched]__
  - Reads: `Transform`, `Sprite` (and possibly `RotationRenderer` in render step).
  - Writes: `Sprite` (animation state), caches.
  - Delegates: `asset.fileExists`, async texture pipeline, `renderer` draw APIs.

- __[Important Notes for Migration]__
  - Rendering duplicates `RenderSystem` and must be disabled/removed.
  - Keep animation update and texture management; rendering moves to `RenderSystem`.

## RenderSystem Collection (for context)

- `CollectRenderItems()` currently gathers:
  - `Transform + Sprite` → sprite items (with `layer`, `depth = layer*1000 + y`).
  - `Transform + Text` → text items.
  - `Transform + UIElement` → UI items.
  - `Transform + DebugDraw` → debug items (rectangles today; circle support pending).

## Initialization & Connection Map

- __[Central Orchestration]__
  - `Gnosis::SystemManager` (`src/Engine/Core/SystemManager.cpp`)
    - `InitializeSystems()` creates `SpriteSystem`, `UISystem`, and `RenderSystem`.
    - Wires subsystems into `RenderSystem` via `SetSubsystems(sprite, ui)` (header indicates method exists; used for coordination only, not for duplicate rendering).
    - `Render()` currently prefers `RenderSystem` but has legacy fallback to `SpriteSystem::Render()` and `UISystem::Render()` when `RenderSystem` is null (to be removed in migration).
    - `UpdateSystems(deltaTime)` updates `SpriteSystem` and `UISystem` (UI input/animations, sprite animations).

- __[States]__
  - `MainMenuState` calls `m_ecsCoordinator->Render();` (routes to `SystemManager`). Verified at `src/FloppyTurd/States/MainMenuState.cpp:248`.
  - `GameplayState` directly calls `m_renderSystem->Render();` and `m_uiSystem->Render();`, then `DrawDebugRectangles()` (duplicates UI and bypasses `SystemManager`). Verified at `src/FloppyTurd/States/GameplayState.cpp:181-194`.

- __[ECS]__
  - `ECS` provides `CreateEntity`, `AddComponent<T>`, `GetComponent<T>`, and queries `GetEntitiesWithComponents<...>()` used by all systems.

- __[Platform Delegates]__
  - `PlatformDelegates.renderer`: `drawText`, `drawRectangle`, `drawCircle`, `getScreenInfo`, `getScreenSize`.
  - Asset and audio delegates used by systems for loading and effects.

## Graph Diagram (overview)

```text
GameStateManager
  └─ Active GameState (MainMenu / Gameplay / Loading)
       ├─ calls ECS.Update(dt)
       └─ calls ECS.Render()  ───────────────┐
                                             │
ECS ─────────────────────────────────────────┤
  ├─ SystemManager.InitializeSystems()       │
  │    ├─ SpriteSystem (update anim, assets) │
  │    ├─ UISystem    (create/input/helpers) │
  │    └─ RenderSystem (unified renderer)    │
  │         └─ SetSubsystems(Sprite, UI)     │
  │
  ├─ SystemManager.Update(dt)
  │    ├─ SpriteSystem.Update (animations)
  │    └─ UISystem.Update (input/hover/future)
  │
  └─ SystemManager.Render()
       └─ RenderSystem.Render()
            ├─ CollectRenderItems():
            │    - Transform+Sprite
            │    - Transform+Text
            │    - Transform+UIElement
            │    - Transform+DebugDraw
            ├─ Sort (layer, depth)
            ├─ RenderWorldSpace()
            └─ RenderScreenSpace()
                 └─ delegates.renderer (Metal/Raylib)
```

## Verification Notes (file and line references)

- __UISystem draw calls__
  - `src/FloppyTurd/Systems/UISystem.cpp:80-81` checks `m_delegates.renderer.drawText` not null.
  - `src/FloppyTurd/Systems/UISystem.cpp:174` uses `drawTextCentered(...)`.
  - `src/FloppyTurd/Systems/UISystem.cpp:178` uses `drawText(...)`.
  - Sorting by text layer: `src/FloppyTurd/Systems/UISystem.cpp:52` uses `uiElement->textLayer`.

- __SpriteSystem texture + rendering helpers__
  - Texture acquisition: `src/FloppyTurd/Systems/SpriteSystem.cpp:159` `GetOrLoadTexture(...)`.
  - Source rect calc usage: `:170` and definition at `:290` (`CalculateSourceRect`).
  - Rotation-centered rendering toggle: `:205` checks `HasComponent<RotationRenderer>`.

- __GameplayState rendering path__
  - Direct system renders and debug overlay: `src/FloppyTurd/States/GameplayState.cpp:181-194`.

- __MainMenu/Loading states central render__
  - `src/FloppyTurd/States/MainMenuState.cpp:248` `m_ecsCoordinator->Render();`
  - `src/FloppyTurd/States/LoadingState.cpp:114` `m_ecsCoordinator->Render();`

- __Direct drawRectangle calls in states__
  - `src/FloppyTurd/States/GameplayState.cpp:1279-1337` (`drawRectangle` overlays).
  - `src/FloppyTurd/States/MainMenuState.cpp:984-1010` (`drawRectangle` for button debug).

- __DebugDraw component fields__
  - Located in `src/FloppyTurd/Components/GameComponents.h:543-558` (flags, colors, alpha, debugLayer).

- __RenderSystem subsystem wiring__
  - `src/Engine/Core/SystemManager.cpp:123` calls `m_renderSystem->SetSubsystems(m_spriteSystem.get(), m_uiSystem.get());`

# Migration Plan (Detailed, Non-Destructive)

- __[1] Route all rendering through RenderSystem (keep legacy systems compiled)__
  - Remove/disable call-sites that invoke `UISystem::Render()` and `SpriteSystem::Render()`.
  - Keep `UISystem` and `SpriteSystem` source code intact for reference and rollback until migration is validated.
  - Ensure all states call `ECS.Render()` (or `SystemManager.Render()`) only.

- __[2] Prevent invocation of legacy render paths (do not delete yet)__
  - Do not delete `UISystem::Render()` and `SpriteSystem::Render()`; just ensure they are not called from anywhere.
  - Keep `UISystem` for creation/input/hit-testing; keep `SpriteSystem` for animation and assets during migration.

- __[3] Centralize debug overlays__
  - Extend `DebugDraw` to support circles (center, radius, color, alpha).
  - Implement circle rendering in `RenderSystem` via `delegates.renderer.drawCircle`.
  - Replace state-level direct `drawRectangle` calls with `DebugDraw` components.

- __[4] Enforce pixel coordinate policy__
  - Audit `UISystem` responsive helpers; for final positions/sizes used in rendering, convert to pixel-based using `ScreenInfo.pixelWidth/pixelHeight` and avoid logical units.
  - Verify `RenderSystem` emits pixel coordinates only.

- __[5] Clean up state-local systems (calls only)__
  - Remove/disable any direct calls to state-local `RenderSystem`/`SpriteSystem` (retain members temporarily if needed for queries).

- __[6] Validation & perf__
  - Visual regression tests on iPhone 16, iPad, desktop.
  - Confirm z-ordering: background → world → UI → debug.
  - Profile to ensure reduced draw submissions and no duplication.

## Call-Site Checklist (disable legacy rendering calls)

- __SystemManager__
  - `src/Engine/Core/SystemManager.cpp` → In `SystemManager::Render()`, remove/guard the fallback calls to `SpriteSystem::Render()` and `UISystem::Render()` (keep log warning if `RenderSystem` missing).

- __GameplayState__
  - `src/FloppyTurd/States/GameplayState.cpp:181-194` → Remove/disable `m_uiSystem->Render();` and rely on `RenderSystem` only. Keep `DrawDebugRectangles()` until debug overlays are migrated.

- __Other States__
  - Confirm only `m_ecsCoordinator->Render();` is used. Already verified:
    - MainMenu: `src/FloppyTurd/States/MainMenuState.cpp:248`
    - Loading: `src/FloppyTurd/States/LoadingState.cpp:114`

## Optional Kill-Switches (for A/B verification)

- Add a runtime flag (e.g., `g_EnableLegacyUIRender`, `g_EnableLegacySpriteRender`) to temporarily re-enable old paths for investigative comparisons. Defaults: OFF. Use only during migration testing.

## Final Cleanup (post-validation)

- After visual/perf verification, delete legacy render code paths from `UISystem` and `SpriteSystem` and remove now-unused helpers that moved into `RenderSystem`.

---
Prepared without modifying source code. This document is exhaustive for UISystem and SpriteSystem responsibilities and provides a topology diagram and step-by-step migration plan.

---

# Function-by-Function Index (for Migration)

This index is derived from file outlines and verified code reads. Use it to plan exactly what to migrate into `RenderSystem` and what to retain in update-only systems.

## UISystem (`src/FloppyTurd/Systems/UISystem.cpp`)

- __UISystem::UISystem(Gnosis::ECS*, const PlatformDelegates&)__ `[7–21]`
  - Initializes `m_ecsCoordinator`, `m_delegates`, sets `m_screenInfoValid=false`, calls `UpdateScreenInfo()` and `SetupLayout()`.

- __Update(float deltaTime)__ `[23–30]`
  - Currently no per-frame logic. Safe to keep as-is for future UI animations/hover effects.

- __Render()__ `[32–77]`
  - Collects `Transform+UIElement`, filters visibility, sorts by `uiElement.textLayer`, calls `RenderUIElement(...)`.
  - To be removed/disabled after migration; functionality moves to `RenderSystem::RenderScreenSpace()`.

- __RenderUIElement(Entity, const Transform&, const UIElement&)__ `[79–180]`
  - Issues text draw via `delegates.renderer.drawText/Centered` using pixel coordinates. Chooses hover/normal colors and font size.
  - To be migrated into `RenderSystem`’s UI text draw pass.

- __IsEntityVisible(Entity)__ `[182–192]`
  - Checks `UIElement` enabled/visible flags; helper for render filtering.
  - Keep as shared helper (or fold into `RenderSystem` collection logic).

- __CreateButton(...)__ `[194–244]`
  - Creates ECS entity; adds `Transform`, `Sprite`, `UIElement` configured for a button.
  - Keep in `UISystem` (creation API). No rendering side-effects.

- __CreateButtonWithBounds(...)__ `[246–298]`
  - Variant with explicit bounds for hit testing.
  - Keep in `UISystem`.

- __IsPointInBounds(Entity, x, y)__ `[300–321]`
  - Hit testing based on `Transform` and `UIElement` bounds.
  - Keep in `UISystem` (input subsystem).

- __GetEntityBounds(Entity, left, right, top, bottom)__ `[323–343]`
  - Computes bounds (screen space/pixels).
  - Keep in `UISystem`.

- __UpdateButtonSprite(Entity)__ `[345–370]`
  - Updates `Sprite` based on UI state (normal/hover/pressed).
  - Keep; used by UI interaction state changes.

- __ResetAllButtonStates()__ `[372–388]`
  - Resets UI button states.
  - Keep.

- __UpdateScreenInfo()__ `[390–416]`
  - Populates `m_screenInfo` via delegates; logs logical/pixel dimensions; sets validity.
  - Keep; ensure downstream uses pixelWidth/pixelHeight for final positions.

- __GetResponsiveScale() const__ `[418–427]`
  - Returns scale based on logical height reference (852). For layout only.
  - Keep but avoid using for final pixel emission.

- __GetUIScale() const__ `[429–437]`
  - Clamps responsive scale to [0.8, 2.0].
  - Keep (layout).

- __SetupLayout()__ `[439–450]`, __SetupIOSLayout()__ `[452–460]`, __SetupDesktopLayout()__ `[462–469]`
  - Platform layout hooks; currently logging + placeholders.
  - Keep; extend for safe areas (using pixel dimensions per policy).

- __GetSafeArea(float&, float&, float&, float&) const__ `[471–491]`
  - Returns safe area using logical sizes; iOS heuristics for notch/home-indicator.
  - Keep but migrate to pixel dimensions for correctness.

- __CalculateResponsivePosition(float, bool) const__ `[493–506]`
  - Scales positions relative to logical 393/852 references.
  - Keep for layout math only; ensure final render uses pixels.

- __CalculateResponsiveSize(float) const__ `[508–514]`
  - Scales size by UI scale.
  - Keep for layout math only; final draw sizes must be pixels.

## SpriteSystem (`src/FloppyTurd/Systems/SpriteSystem.cpp`)

- __SpriteSystem::SpriteSystem(Gnosis::ECS*, const PlatformDelegates&)__ `[7–17]`
  - Initializes ECS, delegates, texture base path.

- __Update(float deltaTime)__ `[19–33]`
  - Iterates `Sprite` entities; advances animation when playing.
  - Keep in SpriteSystem (update responsibility).

- __Render()__ `[35–79]`
  - Collects `Transform+Sprite`, filters by `visible`, sorts by layer, calls `RenderSprite(...)`.
  - To be removed/disabled; rendering moves to `RenderSystem`.

- __PlayAnimation(Entity)__ `[81–90]`, __PauseAnimation(Entity)__ `[92–101]`, __StopAnimation(Entity)__ `[103–112]`
  - Control animation state on `Sprite`.
  - Keep.

- __SetAnimationFrame(Entity, int)__ `[114–123]`
  - Sets current frame.
  - Keep.

- __SetTextureBasePath(const std::string&)__ `[125–128]`
  - Sets base path for desktop texture loading.
  - Keep.

- __UpdateSpriteAnimation(Sprite*, float)__ `[130–155]`
  - Updates frame timers and indices.
  - Keep.

- __RenderSprite(Entity, const Transform&, const Sprite&)__ `[157–243]`
  - Gets/loads texture, computes source rect, selects centered vs top-left rendering based on `RotationRenderer` presence, issues renderer draw.
  - To be migrated into `RenderSystem`’s sprite draw path.

- __GetOrLoadTexture(const std::string&, Entity)__ `[245–288]`
  - Texture cache lookup and platform load wiring.
  - Keep in SpriteSystem (asset responsibility), callable by `RenderSystem`.

- __CalculateSourceRect(const Sprite&)__ `[290–316]`
  - Computes frame rectangle (animated sheets or full texture).
  - Keep; `RenderSystem` may call this or we can lift a copy.

- __GetFullTexturePath(const std::string&)__ `[318–322]`
  - iOS returns ID as path; desktop resolves full path.
  - Keep.

- __HandleTextureLoaded(TextureData*, const char*, void*)__ `[324–342]`
  - Stores texture handle and dimensions on load completion; clears pending.
  - Keep.

- __LoadTexture(const std::string&, const std::string&)__ `[344–358]`
  - Logs, dedups, verifies existence via delegate.
  - Keep.

- __IsEntityVisible(Entity) const__ `[361–369]`
  - Checks `sprite->visible`.
  - Keep (helper).

- __GetTextureDimensions(const std::string&) const__ `[371–377]`
  - Returns cached width/height.
  - Keep; useful for layout.

---

With this index, we can precisely route rendering responsibilities to `RenderSystem` while preserving update/asset responsibilities in `SpriteSystem` and creation/input/layout responsibilities in `UISystem`.
