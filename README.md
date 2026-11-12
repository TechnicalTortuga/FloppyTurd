# Floppy Turd — restored raylib edition

Restored from `53636de6298fcca93fef470b2efb481defe05322` (July 2, 2025, v0.902), the last complete raylib checkpoint previously selected for the portfolio website. The last June checkpoint is `cf21d1d` (June 10, 2025). Work is on `revisit/cross-platform`; upstream master is preserved.

- [Build, package, and browser integration](BUILD.md)
- [Stable browser/revision contract v1](docs/BROWSER_CONTRACT.md)
- [Gameplay differences, asset provenance, and code review](docs/REVISIT.md)
- [Asset comparison](docs/asset-comparison.json)
- [Pinned dependencies](dependencies.lock.json)

The current game uses raylib **6.2**, pinned to a full commit. The playable world is **400×225**, rendered to **800×450**. A centered 320×180 UI layout keeps the original menus while reducing their relative size. Text uses monochrome bitmap glyphs and nearest filtering. Display resizing preserves aspect ratio and uses integer enlargement whenever it fits.

Controls: **Space / click** jump, **S** shoot after unlocking the skill, **Tab** pause. Desktop also retains **F** for shooting, **Esc** for pause, and **F11** for fullscreen. In a website embed, **F** requests host fullscreen and **Esc** returns to the website. Standalone browser fullscreen uses its own button or F; platform support may limit native fullscreen.

Original raylib balancing remains the starting point. The later iOS/Pooper Trooper gameplay and services have been compared, not silently merged into this edition. Broader playtesting of all six levels is still required after the wider framing change.

Touch-capable browsers automatically show mobile controls. Tap the gameplay canvas or Jump, hold Shoot after unlocking it, and use Pause/Resume or the embedded Back button. Portrait and landscape use the same letterboxed game. Add `?touch=1` to preview controls on desktop; this does not replace real iOS/Android device testing.
