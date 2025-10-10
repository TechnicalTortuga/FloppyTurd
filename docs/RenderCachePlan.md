# Render Cache Implementation Plan

## Current Rendering Flow
- **Render bootstrap** `RenderSystem::Render()` rebuilds `m_renderQueue` every frame, running `CollectRenderItems()`, `SortRenderQueue()`, `RenderWorldSpace()`, and `RenderScreenSpace()` in sequence.
- **Per-frame ECS scans** `CollectRenderItems()` executes five separate `GetEntitiesWithComponents<>` sweeps (sprites, text, UI, debug, shapes), creating temporary vectors each frame for the same entity sets.
- **UI duplication** UI entities are queried twice, injecting duplicate `RenderItem`s and doubling ECS iteration cost for screen-space elements.
- **Texture resolution** `RenderSingleItem()` calls `GetOrLoadTexture()` on every draw, performing string lookups in `m_textureCache` and potentially re-queueing async loads even when textures are already resident on the Swift `AssetManager` side.
- **Draw submission** All renderables are submitted one-by-one to the Metal renderer delegates (`drawSpriteScaled`, `drawSpriteScaledWithSource`, etc.) without batching despite repeated textures across obstacles and enemies.

## Pain Points to Address
- **[ECS iteration cost]** O(N) entity scans per component mix inflate CPU time as obstacle/enemy counts grow.
- **[Duplicate UI work]** Double collection of `UIElement` entities and repeated component fetches in `RenderScreenSpace()` waste cycles and inflate render queue size.
- **[Texture handle churn]** String hashing and cache lookups occur every frame because `Sprite` stores only the texture ID, not the resolved handle supplied by Swift.
- **[Logging overhead]** High-volume `GN_LOG_INFO/DEBUG` statements inside render loops trigger formatting and delegate calls every frame.
- **[Missing batching]** Without grouping by texture/Material, Metal incurs one draw call per sprite, limiting achievable FPS once entities exceed ~200.

## Proposed Architecture
- **[Persistent entity caches]** Add member vectors (`m_cachedSprites`, `m_cachedTexts`, `m_cachedUI`, `m_cachedDebug`, `m_cachedShapes`) plus a `m_renderCacheDirty` flag. Rebuild caches only when entities with relevant component signatures are created/destroyed.
- **[Dirty notifications]** Introduce `RenderSystem::NotifyEntityChanged(Entity)` / `NotifyEntityRemoved(Entity)` so systems that mutate renderable signatures (Obstacle spawning/wrapping, UI creation, etc.) can invalidate caches immediately. Provide helper wrappers in shared factory code to keep call sites consistent.
- **[Frame usage]** During each `Render()`, iterate cached vectors, pull components directly, and skip entities that lost visibility instead of re-querying the ECS.
- **[Sprite handle caching]** Extend `GameCore::Sprite` with `uint32_t cachedTextureHandle` and `bool handleValid`. Cache the resolved handle after the first successful `GetOrLoadTexture()`; refresh only when `textureId` changes or Swift signals an eviction.
- **[Swift/Metal alignment]** Keep all positions in pixel space using `ScreenInfo.pixelWidth/pixelHeight` to match the Metal renderer contract (per project rule). Cached data stores entity IDs only; actual component data stays authoritative in the ECS to avoid stale transforms.
- **[Logging controls]** Gate existing render-loop logging behind `m_renderDebugLogging` (default `false`) so intensive formatting can be toggled on only when diagnosing issues.

## Integration with Existing Managers
- **[TextureManager ↔ AssetManager]** Continue funneling unresolved texture IDs through `PlatformDelegates.asset.loadTexture`. Cached handles simply skip redundant lookups when `handleValid` is true. Swift’s `AssetManager` remains the source of truth for residency.
- **[ThreadingProxy callbacks]** `HandleTextureLoaded()` should set `sprite.cachedTextureHandle`, flip `handleValid`, and update the metadata cache; no change to callback wiring is required.
- **[SystemManager orchestration]** Cache rebuild uses the existing shared ECS instance (per architecture rule) and remains transparent to `SystemManager::Render()`—only `RenderSystem` internals change.

## Actionable Phase Tracker
- [ ] **Phase 1 – Cache scaffolding**: Add cache member vectors/flags to `RenderSystem`, implement `RebuildRenderCaches()` invoked on startup and when dirty.
- [ ] **Phase 2 – Dirty hooks**: Create notification APIs, retrofit spawning/destruction sites (`LevelManager`, `ObstacleSystem`, UI factories) to flag the render cache.
- [ ] **Phase 3 – Texture handle caching**: Extend `Sprite` component, update texture load path to validate and reuse cached handles, ensure Swift callbacks populate metadata consistently.
- [ ] **Phase 4 – Logging controls**: Introduce `m_renderDebugLogging` guard, trim or wrap high-frequency logs in render loops, and add targeted counters for profiling.
- [ ] **Phase 5 – Pre-batching groundwork**: After caches stabilize, design texture-based batching structs in `RenderSystem` and Metal renderer to collapse identical texture draws into fewer calls.

## Deliverables
- **[Documentation]** Update `RENDERING_ANALYSIS.md` with cache-specific notes once implemented.
- **[Testing]** Capture FPS metrics in Levels 2 and 5 to validate gains, and confirm no regressions in UI rendering or asset loading on iPhone 16 simulator.
