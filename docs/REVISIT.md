# Floppy Turd revisit — November 11, 2026

## Recovered checkpoints and assets

Repository: https://github.com/TechnicalTortuga/FloppyTurd

| Checkpoint | Evidence and role |
| --- | --- |
| `a605e55`, June 10, 2025 | Commit labeled Version 1.0 Release. |
| `cf21d1d`, June 10, 2025 | Last June commit: added loading state and fixed total pipes. Closest exact match to the remembered June baseline. |
| `53636de6298fcca93fef470b2efb481defe05322`, July 2, 2025 | v0.902, macOS build complete, road to iOS. The user explicitly identified this complete raylib checkpoint in the website chat; selected for the restoration. |
| `a99e834`, July 24, 2025 | Replacement engine/rebuild checkpoint; later Swift/Metal work follows. |
| `ea665c2f220ea50f52636ec862cfbe3fe30c7f00`, March 8, 2026 | Current master; cleanup after merging SwifttoSuccess and renaming to Pooper Trooper. |

The supplied `C:/Users/aeist/Downloads/FloppyTurd-master.zip` has the exact current-master SHA in its ZIP comment. Its 1,185 source/asset files were preserved under `.local/master-snapshot` for comparison. This local reference copy is ignored by Git. The restored game's matching historical resources remain under `FloppyTurd/resources`; current-master assets do not overwrite old files blindly.

The baseline contains 327 resource files. Current master has 352 files in `src/Assets`: **241 have byte-identical baseline counterparts; 111 do not**. The latter include both additions and changed assets, not necessarily 111 new game features. `asset-comparison.json` records SHA-256, sizes and matching baseline paths. `src/Assets.xcassets` is a separate iOS packaging tree, not another gameplay asset set to merge wholesale.

## Gameplay differences: raylib checkpoint versus later iOS/master

These are source comparisons, not an assertion that every iOS feature was shipped or works on a device today.

| Area | Restored raylib baseline | Later iOS/master |
| --- | --- | --- |
| Architecture | C++ classes, raylib rendering/audio, ResourceManager/compatibility layer, AIGUI menus. | C++ ECS and systems with Swift 6 platform delegates, Metal batching/instancing, AVAudio and CADisplayLink/threading. |
| Jumping | Instant impulse on Space/click; upward velocity cap; stronger gravity after apex. | Touch hold/release force with an automatic jump around 150 ms, a short jump cooldown, early release handling, separately tuned rising/falling/death gravity and terminal speeds. Numeric values use a different screen coordinate scale. |
| Shooting | Already costs one session coin; shooting skill unlock; hold-to-repeat. | Still one session coin, with dedicated touch zones, startup input suppression, about 0.3 s cooldown, and haptics. This was refined, not invented by the iOS port. |
| Park | First available level with the old progression flow. | No enemies, pickups or shooting in its configuration; later progression treats Park as Legacy Mode, unlocked after the boss. |
| Levels | Six named levels already exist, including sewer, desert, snow, castle and Rat King boss. Coins unlock levels; Quickplay is available. | Same six themes, but data-driven layouts, obstacle/coin grouping and more enemy tuning. Legacy Mode replaces the old Quickplay presentation and is boss-gated. |
| Difficulty | Runny/Regular/Rough; 60/80/120 world pan speeds in the old level setup. | Config-driven speed multipliers 0.75/1/1.5 and retuned base speeds. Compare feel after normalizing coordinates, not raw constants. |
| Skills | Six one-time flags: shooting, half/third hearts, coin magnet, heart magnet, safety net. | Ranked health precision, coin/heart magnets, homing shots, split shots and a one-rank safety net, with explicit rank costs. |
| Hats | Fifteen hats, purchase/equip UI and alternate form/animation sprites already exist. Unlock flags are stored; equipped identity is not in the old save struct. | Same collection with revised pricing tiers (50/150/250), per-state textures and equipped/selected hat persistence. |
| Boss | Rat King, projectiles, health bar, music/death sequence and credits already exist. | Extensive later collision/projectile/wave/layout tuning; portrait/landscape transitions and safe-area adaptations. |
| Persistence | Old binary counters/high scores/unlock flags. | Swift Codable JSON schema with level completion/best runs, stored coins, playtime, ranks/equipment/settings and migration support. |
| Presentation | Landscape 320×180 design, desktop window controls, keyboard/mouse; preparatory touch abstractions. | Portrait normal levels, landscape boss/orientation prompts, tutorials, haptic settings and mobile controls. |
| Services | Local stats and menus. | Game Center/leaderboard, StoreKit and advertising integrations exist in source. These require platform/product decisions; they are not part of the desktop/browser port. |
| Naming | Floppy Turd and the separate interactive F logo. | Later commits rebrand the game Pooper Trooper. The restored edition keeps the requested Floppy Turd identity. |

The July 2 checkpoint itself adds substantial resource caching, atlases/profiling, touch abstraction and platform work over June 10. Those are pre-Swift additions and should not be misattributed to iOS.

## Changes made in this revisit

- Pin official raylib **6.2** to `02566453880e06bec97f5b8160fa6fe1e98397ca`; retain Emscripten 4.0.23 for repeatability. Adapt the renamed image drawing API and prevent raylib's transitive export flag from replacing the browser bridge exports.
- Replace the blocking browser loop with a persistent single-frame callback. Move input handling before frame-event reset; this fixes missed browser Space presses. S shoots; desktop retains F as an alias. Embedded F requests fullscreen; Shift+Escape returns to the room while plain Escape/Tab pause.
- Show a 400×225 world instead of 320×180: 25% more world extent on each axis, with an 800×450 render buffer. Anchor the HUD to the viewport corners, center the pipe count by visible glyph bounds, and shrink modal panels 10%. Center the original 320×180 UI region; reduce main-menu type from 24 to 20 logical pixels. Update key spawns/culling/ground anchors, parallax coverage and snow coverage. Fix the previously inconsistent GameSettings values of 1920×1080.
- Generate Whacky Joe through raylib's monochrome FONT_BITMAP path; use point filtering, stable font metrics and a denser render target. Correct scissor coordinates for the denser target and centered origin; align button outlines with fills. Shared font ownership replaces duplicate credits loading and per-frame filtering changes.
- Hide desktop resolution/fullscreen options in browser menus. Keep desktop F11 and window options; standalone browser has a direct fullscreen button and the website owns embedded fullscreen.
- Fix cached loading/score texture or sound ownership, release scene resources before closing graphics/audio, and avoid playing credits music during construction. Initialize touchControls before reading it through skill restoration.
- Correct exact-case asset paths, missing castle-music names, icon format, snowball/UI paths, and remove registrations for nonexistent unused menu textures. The registered-asset audit now passes on a case-sensitive name inventory.
- Fix double-counting session currency on run completion and in purchase/display totals. Shooting and purchasing update the same total once; tests cover insufficient funds and bank-versus-session spending.
- Use per-user save locations and browser persistence. Save v2 explicitly encodes numbers/flags and reads old v1 saves; corruption cannot partially overwrite live progress. Exclude developer saves, editor source files, build trees and duplicate iOS catalogs from runnable packages.
- Add desktop packaging, static Web packaging, a four-platform CI configuration, and the origin/session-checked website lifecycle bridge agreed with the website chat.

## Validation and remaining review work

Windows Release and Emscripten Release were compiled locally. Native regression checks cover v1 migration, v2 round-trip, malformed/truncated saves, coin conservation and viewport mapping. Eight browser bridge checks cover lifecycle, invalid senders and keyboard ownership. All 308 resource registrations resolve with exact filename case.

See the final chat update for the most recent interactive browser checks. These tests do not constitute complete playthroughs of all six levels or native Linux/macOS runtime validation.

Next priorities:

1. Play through every level and the boss after the wider framing change; review spawn rhythm, gaps, camera anchors and first-frame input together. Keep the original balance available in Git for comparison.
2. Measure cold/warm loads and cache eviction on physical browsers after the deferred-loading changes below. Consider re-encoding/deduplicating the remaining server-side music library separately from startup optimization.
3. Consolidate TextureCache, ResourceManager and the currently underused atlas system. Borrowed raw texture handles must not outlive cache eviction; the present interfaces do not encode that guarantee.
4. Persist equipped hat, settings and richer statistics in a subsequent shared schema, then deliberately choose which iOS rank/progression changes to port. The new desktop v2 binary is not the Swift JSON format.
5. Review mixed Update/Draw responsibilities and audio ownership in level constructors; eliminate eager loading of every level once behavior is covered. Some legacy warnings and raw-pointer ownership remain.
6. Continue the Windows/Linux/macOS CI matrix and perform real macOS/Linux audio, controller/window and packaging checks. Decide signing/notarization separately from building.

The restoration is committed and pushed on `revisit/cross-platform` for its CI builds. The first Windows/Linux/macOS/Web matrix passed (run 43124791254). No game or website has been published, upstream master is unchanged, and the iOS snapshot has not been modified.

Mobile follow-up: the browser shell detects touch/pen/coarse-pointer capabilities and offers Jump, held Shoot, Pause/Resume and embedded Back controls; it clears held actions on cancellation/backgrounding. The protocol remains v1. `docs/BROWSER_CONTRACT.md` and hash-verified distribution manifests make future revision replacement repeatable. Game-over score/coin rows are raised and aligned by visible glyph bounds.

Live-TV follow-up: optional embedded muted autostart, preserved WebGL drawing buffers, one persistent session, and `window.portfolioGame` / `window.floppyEmbed` same-origin presentation/audio/input API. The shared `portfolio-input` v1 pointer/key/reset/capability schema is independent of Floppy Turd actions. Input is latched in C++ so rapid gestures survive frame boundaries; pointer cancellation does not click a menu.

Deferred-loading follow-up: replace the 123,670,167-byte preload with an 806,904-byte core asset pack (about 2.2 MB for all core runtime files). Level visual groups load before constructing that scene; only the selected music clip downloads when played. Remove eager construction of every level and the web atlas sweep. Music stop releases WASM decoder/MEMFS ownership; content-addressed compressed files use a bounded 64 MB persistent cache and immutable HTTP caching. Manifest schema 2 lists all deferred blobs/catalog, while lifecycle/input remain v1. The website retains immutable prior releases for open sessions.

Physical iPhone 16/Safari testing reached the LAN preview and confirmed audio after Sound. It exposed a reproducible DOM-focus blackout: GLFW canvas blur was mistaken for page backgrounding. The browser shell now owns page visibility/focus and frozen frames retain the prior image. Play selects Safari's playback route so Sound is recovery rather than a second activation step. Broader physical touch, interruption, performance and all-level playtesting remain ongoing.
