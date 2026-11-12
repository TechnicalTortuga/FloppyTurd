# Floppy Turd — restored raylib edition

Restored from `53636de6298fcca93fef470b2efb481defe05322` (July 2, 2025, v0.902), the last complete raylib checkpoint previously selected for the portfolio website. The last June checkpoint is `cf21d1d` (June 10, 2025). Work is on `revisit/cross-platform`; upstream master is preserved.

- [Build, package, and browser integration](BUILD.md)
- [Stable browser/revision contract](docs/BROWSER_CONTRACT.md)
- [Gameplay differences, asset provenance, and code review](docs/REVISIT.md)
- [Asset comparison](docs/asset-comparison.json)
- [Pinned dependencies](dependencies.lock.json)

The current game uses raylib **6.2**, pinned to a full commit. Desktop and the inline website TV use a **400×225** world. Expanded touch presentation uses **225×400** for normal levels and **400×225** for the final boss. The canvas fills the available arena with crisp bitmap text and nearest filtering; menus and HUD respect browser safe areas.

Controls: **Space / click** jump, **S** shoot after unlocking the skill, **Tab** pause. Desktop also retains **F** for shooting, **Esc** for pause, and **F11** for fullscreen. In a website embed, **F** requests host fullscreen and **Shift+Esc** returns to the website. Standalone browser fullscreen uses its own button or F; platform support may limit native fullscreen.

Original raylib balancing remains the starting point. The first explicit iOS/Pooper Trooper port adds spit, enemy-defeat and purchase sounds, two ranks each of Homing and Split Shot, bird hover behavior and safer defeated-enemy handling. Existing paid skills remain intact. See [port provenance and remaining scope](docs/ios-port-provenance.json). Broader playtesting of all six levels is still required after the camera changes.

Touch-capable browsers automatically show mobile controls. Tap the gameplay canvas or Jump, hold Shoot after unlocking it, and use Pause/Resume or the embedded Back button. Normal mobile levels request portrait; the last level requests landscape. Rotating to the wrong orientation pauses the session until corrected. Add `?touch=1` to preview controls on desktop; this does not replace real iOS/Android device testing.

The portfolio can run the same session live on its TV with `embed=1&autostart=1`. The shared `portfolio-input` v1 pointer/key/capability schema and `window.portfolioGame` API keep future revisions and other games independent of the website scene. Embedded audio stays muted until a genuine activation gesture.

The browser starts with a small core pack. Level scenery/enemies and their defeat sounds load when entering that level; only the selected music track downloads when played. Content-addressed assets use a bounded 64 MB IndexedDB cache and HTTP caching, with storage/network failure recovery. The full server-side asset library is still about 124 MB. Exact core sizes and hashes are recorded in each distribution manifest. The first Play tap activates Safari audio; Sound retries after an interruption.
