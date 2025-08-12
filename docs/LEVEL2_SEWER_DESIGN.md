## Level 2 – Sewer Environment (Implementation Notes)

- Backgrounds: uses `SewerLargeA/B/C/D` as variant textures. Each parallax instance randomly swaps to a variant whenever it wraps. Configured in `LevelConfigFactory::AddSewerLevelLayers()` using `variantTextureIds` and handled at runtime by `ParallaxVariants` + `CameraSystem::UpdateParallaxLayers()`.

- Obstacles: single-piece sewer segments (not hoop pairs). Patterns are spawned via the pooled system in `LevelManager::InitializeObstaclePool()` for Level 2 only:
  - Top-only triplet
  - Bottom-only triplet
  - Pyramid 3-high (3-2-1)
  - Pyramid 4-high (4-3-2-1)
  - Within each pattern, segment colors alternate between orange and blue vertically/horizontally.
  - Non-overlapping: each pattern is spawned as its own `Group` with fixed offsets; wrapping respects group spacing.

- Collision: rectangle hitboxes inset by 6 px on width/height (`Hitbox.width/height = sprite - 12`).

- Pooling/wrapping: groups are wrapped as a unit using `WrapGroupAroundScreen()` so spacing and shape are preserved.

- NPC: Janitor appears periodically, sweeps by default, and switches to `JanitorSurprise` after the player passes. Managed by `UpdateNPCSpawning()` and `UpdateNPCStates()` for Level 2.


