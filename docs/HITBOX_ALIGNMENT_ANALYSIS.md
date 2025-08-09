# Hitbox Alignment Analysis for Toilet Obstacles

## Scope
- Creation of toilet obstacle hitboxes (top and bottom)
- How transforms/scales/offsets are applied over time (spawning and wrapping)
- How collision rectangles are computed vs. how debug rectangles are drawn
- Why overlays appear offset from intentions despite correct math
- Concrete next steps to make overlays and gameplay collision match visual intention

## Key Files and Concepts
- `src/FloppyTurd/Systems/LevelManager.cpp`
  - Spawns toilet pairs; sets `Transform`, `Sprite`, `Physics`, `Hitbox`, `Obstacle`, `DebugDraw`.
  - Two paths exist:
    - `SpawnToiletPair(...)`: gap-based using provided `config.gapHeight`.
    - `SpawnToiletPairWithGap(...)`: pool-based randomized top Y with fixed gap (now 309).
  - Hitbox semantics: offsets are CENTER-based; `Transform.position` is TOP-LEFT of sprite.
- `src/FloppyTurd/States/GameplayState.cpp`
  - Collision checks for player vs obstacle rectangles (circle-rect), computed center-based from top-left transform.
- `src/FloppyTurd/Systems/RenderSystem.cpp`
  - Collects `RenderItem`s for sprites and debug overlays.
  - Debug overlays are drawn using CENTER-based computation (sprite half-dimensions + hitbox offsets), then converted to top-left for rectangle draw call.
- `src/FloppyTurd/Components/GameComponents.h`
  - `Hitbox` (width/height/offsetX/offsetY, center semantics implied by usage).
  - `DebugDraw` controls visibility and colors.

## Creation: Where and How Hitboxes Are Built
- Top toilet (both spawn paths):
  - Texture: 64x256.
  - Scale: `m_currentLevelConfig.baseScale` (8 on iPhone 16 logs) → sprite world size 512x2048.
  - Hitbox width = 20 (unscaled), height = `config.height - 30` → 256 - 30 = 226 (unscaled).
  - Offsets:
    - `offsetX = 0` (centered horizontally by logic, see notes below).
    - `offsetY = -15` so that collider top aligns with sprite top (center shifted up by 15).
  - Scaled rectangle dimensions: 20*8=160 (W), 226*8=1808 (H).
- Bottom toilet:
  - Same texture and scale.
  - Hitbox width = 20; height = 226.
  - `offsetX = 0`.
  - `offsetY = +15` so that collider top starts 30px below sprite top.
- Verified by pool-spawn logs we added:
  - Example
    - Top spriteTL Y = -885.84, top hitbox T = -885.84, B = 922.16; size=(160x1808).
    - Bottom spriteTL Y = 1471.16, bottom hitbox T = 1711.16 (= 1471.16 + 240), size=(160x1808).
    - spriteGap = 309; colliderGap = 309 + 480 = 789 (matches expected 309 + 2*30*8).

Conclusion: Creation math is correct and consistent with the requirement.

## Updates: Movement and Wrapping
- `LevelManager::WrapObstacleAroundScreen(...)`:
  - Repositions X to queue to the right when leaving screen; re-randomizes topY and computes bottomY = topY + toiletHeight + fixedGapHeight (309) for the pair.
  - Resets `Obstacle::pipeCleared` so scoring works.
  - Updates `basePosition` fields.
- No additional changes to hitbox offsets/size during update; `Transform.position` changes, scale stays constant.

Conclusion: Updates preserve the creation semantics. Gap is identical between initial spawn and wrap after our change.

## Collision Computation (Gameplay)
- In `GameplayState::CheckToiletCollisions()`:
  - For each obstacle:
    - Compute rectW/H = hitbox.width/height * transform.scale.
    - Compute sprite half-dimensions = sprite.width/height * scale * 0.5.
    - Compute rectangle center = transform.topLeft + spriteHalf + hitbox.offset*scale.
    - Convert to top-left (rectX/Y) by subtracting half width/height.
  - Player circle center computed similarly from player top-left + player sprite half + player hitbox offset.

Conclusion: Collision rectangles match hitbox creation semantics (center-based). Player collisions occur where the overlays should be if overlays match this math.

## Debug Overlay Rendering
- `RenderSystem::CollectRenderItems()` for debug colliders:
  - Takes the entity `Hitbox` and sets `debugWidth/Height` and `debugOffsetX/Y` = hitbox.*.
- `RenderSystem::Render()` (debug branch):
  - Uses CENTER-based placement:
    - `center = transform.topLeft + spriteHalf + (offset*scale)`.
    - Converts to top-left for the rectangle draw call (`debugX = centerX - width/2`).
  - This exactly mirrors collision computation.

Conclusion: If rectangles appear horizontally off-center visually while logs/numbers are correct, the error is likely in visual expectations (texture art center vs math center), not in the math used by both collision and overlay.

## Why It Still Looks Offset On Screen
From the captured screenshots and your observation:
1) Horizontal misalignment (hitboxes appear left of the shaft center):
   - The texture’s visible shaft (white pipe column) is not centered in the full 64px texture; the art likely has padding or asymmetric pixels.
   - Since we center the 20px-wide collider on the sprite’s mathematical center (i.e., full 64px), any asymmetry in the art center vs texture center yields a visible offset.
   - Our pool logs confirm the hitbox horizontal center equals `(sprite left) + (sprite width/2)` (e.g., 1279 + 256 = 1535), and hitbox L/R are 1535±80, which is correct mathematically.
   - Therefore, to visually center on the shaft we either:
     - add a small `offsetX` to the hitbox (in unscaled pixels) to match the art’s shaft center, or
     - shift the actual sprite rect (not recommended), or
     - trim/realign the art so the shaft truly sits at 32px center of the 64px texture.

2) Vertical misalignment (top/bottom rectangles don’t visually meet where expected):
   - The creation math uses 30px TRIMs (scaled to 240) and matches perfectly per logs.
   - If the pipe’s visible lip/rounded edge is not exactly at the texture’s top/bottom bounds, the collider top/bottom will not align with perceived edges in the art. This is an art/texture bounding vs mathematical bounding mismatch.
   - Additionally, the vertical “gap” you visually perceive is the art gap; the collider gap adds 30px trim on both sides by design, so the colliders will be farther apart than sprite edges by 60px scaled.

3) “Overlays match the actual collision”
   - You noted the player triggers only within overlays; that means overlay math and collision math are aligned (good). The disagreement is between the art’s perceived center/edge and the texture’s true 64x256 bounds.

## Minimum-Change Remedies (preserve gameplay feel)
- Add a small `offsetX` to both top and bottom toilet colliders to match the art shaft center. Start with 2–4 unscaled px and tune by logs/screenshot alignment.
  - Example: `offsetX = +2.0f` (unscaled) → +16px at scale 8. This will visibly shift the 20px bar toward the perceived shaft center.
  - We temporarily tried +2 and reverted at your request; we can parameterize this per texture if desired.
- If the pipe lip needs to be tighter to the art edge, reduce trim from 30 to, say, 24 (or change only top or bottom). But you confirmed 30 is desired; keep at 30.

## Structural Remedies (art and data correctness)
- Re-center the pipe art so the shaft is visually centered in 64px.
- Or define per-texture metadata (JSON) with `colliderOffsetX` to accommodate asymmetric art without code changes.
- If multiple device scales exist, ensure the scale factor is uniform; logs show it is (`baseScale` used everywhere).

## Why your simulator view still looks far off
- The phone screenshot shows the overlays significantly left of the shaft. Given logs prove mathematical centering, the underlying cause is almost certainly the pipe column art not being centered within the 64px texture bounds.
- Because both collision and overlays use the same math and match each other (and the player collision), the only remaining mismatch is between the math center and the art’s perceived center.

## Proposed next steps
1) Quick test: introduce a temp per-texture `offsetX` in `ObstacleConfig` for `TopToilet`/`BottomToilet` and set it to, e.g., +3.0 (unscaled). Apply to both top/bottom hitboxes and rebuild. If it visually centers, we lock it as data.
2) If you prefer no code changes: adjust the art to center the shaft inside the 64px width. Then revert `offsetX = 0` and everything will be perfectly centered.
3) Keep 30px trims; the collider gap will remain larger than sprite gap by 60px scaled, as intended.

## Appendix: Verifications from logs
- Example (12:54:29):
  - Top spriteTL=(1279,-885.84) worldSize=(512x2048)
  - Top hitbox LRTB: L=1455 R=1615 T=-885.84 B=922.16 size=(160x1808)
  - Bottom spriteTL=(1279,1471.16); Bottom hitbox T=1711.16 (= +240), size=(160x1808)
  - spriteGap=309; colliderGap=789 (= 309 + 480)
- All numbers are consistent with the configuration and scaling.

---
If you want, I can implement option (1) by adding `colliderOffsetX` to `ObstacleConfig` and using it for `TopToilet` and `BottomToilet` only (no changes to the player). This keeps gameplay logic intact and fixes the visual offset without touching art.
