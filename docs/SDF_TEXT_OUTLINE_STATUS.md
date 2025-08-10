### SDF text outline: current state, issues, and options

This note summarizes requirements, what the codebase currently implements, what problems we still see (spacing/centering/jaggies), and pragmatic options to fix them. It also cites industry references and clarifies single‑pass vs two‑pass outline approaches.

---

### Requirements
- **Pixel-accurate placement**: all positioning and centering must use actual screen pixels (not logical points).
- **Clean, stable outlines**: no hairline jaggies or stray black pixels at glyph edges at gameplay/menu sizes.
- **Natural word spacing**: use proportional advances; no forced monospacing in rendering.
- **Correct centering**: both X and Y, including vertically within button rectangles.

---

### What we have in the codebase
- Atlas generation (`src/iOS/Rendering/MetalRenderer.swift`)
  - Generates a grayscale SDF atlas from the TTF at runtime.
  - Current params: atlas `1024×1024`, padding `12`, supersample factor `4`.
  - Fixed grid (16×6 cells) covering ASCII 32–127; each glyph is drawn into its cell and SDF is computed; UVs use the cell rectangle.
- Metrics and layout
  - CTFont metrics (ascent, descent, leading, lineHeight) captured per loaded font.
  - Rendering uses proportional advances from CTFont; an additional tracking term is applied.
  - Vertical placement snaps the baseline to integer pixels to avoid shimmer.
- Renderer/shaders
  - Linear sampler is used for SDF textures; sprites keep nearest.
  - Fragment `sdf_text_fragment`: standard SDF fill with `fwidth(distance)` anti‑aliasing.
  - Fragment `sdf_text_outline_fragment`: single‑pass composition computing fill and a stroke band around the edge, then compositing stroke behind fill. Tunable `aaScale` and `outlineWidth` are passed.
  - We are currently using a **single‑pass outline** (fill and outline in one fragment). We are not drawing text twice.

---

### What is still off (observed)
- **Jagged/“hairy” outlines** on high‑contrast edges, especially small text and thin strokes.
- **Horizontal centering occasionally off** after layout changes (string advance sum vs drawn quad width mismatch in edge cases).
- **Vertical centering in buttons off by a few px**; baseline vs visual centering mismatch for all‑caps/irregular shapes.
- **Word spacing too tight** after switching off monospaced advances.

---

### Why these happen (common causes)
- SDF outlines are sensitive to: atlas resolution, padding (linear sampling bleeds neighbors), incorrect distance polarity, and smoothing window tied to derivatives in screen space.
- Fixed‑cell atlases lose true bearings; without per‑glyph bearings/offsets the vertical “visual” centering can differ from baseline center, and punctuation can drift.
- Text measurement must mirror the exact rendering path (advances, tracking, lineHeight). Any mismatch causes visible centering drift.

References
- Valve paper on SDF text: “Improved Alpha‑Tested Magnification for Vector Text and Art” — see derivative‑based smoothing and thresholds. [`https://github.com/Chlumsky/msdfgen#distance-fields`](https://github.com/Chlumsky/msdfgen#distance-fields)
- MSDF (multi‑channel) for sharp corners (reduces outline artifacts compared to single‑channel SDF). [`https://github.com/Chlumsky/msdfgen`](https://github.com/Chlumsky/msdfgen)
- Unity TextMeshPro SDF shader notes (tunable outline softness/thickness, padding guidance). [`https://docs.unity3d.com/Packages/com.unity.textmeshpro@3.2/manual/ShadersDistanceField.html`](https://docs.unity3d.com/Packages/com.unity.textmeshpro@3.2/manual/ShadersDistanceField.html)

---

### Two‑pass vs single‑pass outline
- **Single‑pass (current)**
  - One draw, one fragment shader: compute fill alpha around `edge=0.5`, compute a second band offset by `outlineWidth`, composite stroke behind fill.
  - Pros: fewer draw calls, easy batching. Cons: tuning the band to avoid “hair” artifacts at small sizes can be fiddly.

- **Two‑pass (option)**
  - Pass 1: draw the stroke only (wider threshold, slightly softer AA) using the SDF texture.
  - Pass 2: draw the fill on top with a normal SDF fill shader.
  - Pros: simpler, robust composition; you can widen stroke without impacting fill sharpness. Cons: 2× overdraw/drawcalls for text.

Both approaches work in industry. If we want the quickest path to artifact‑free strokes, two‑pass is usually more forgiving.

---

### Concrete issues mapped to fixes
- Jaggy outlines
  - Ensure sufficient atlas padding (≥12 px; 16 px is safer at strong outlines).
  - Use a slightly wider AA window for the stroke than for the fill (already added; can expose `aaScaleStroke` separately).
  - Consider switching to a **two‑pass** draw (stroke→fill) to fully decouple stroke softness from fill sharpness.
  - Longer‑term: evaluate **MSDF** to preserve corners at tiny sizes.

- Horizontal centering not perfect
  - Make `measureText()` mirror draw: same per‑glyph advances, same tracking, same kerning (if applied). Today we mirror advances and tracking; we can add optional kerning pairs from CoreText shaping (see “Next steps”).
  - Use the measured width for centering; do not mix cell width with advance.

- Vertical centering in buttons
  - Baseline snap is good for stability; for visual centering in buttons, add a per‑font `visualCenterOffsetPx` (ascender heavy fonts appear high when centered on baseline box). Compute `visualCenterOffsetPx = 0.5*(ascender - (lineHeight - ascender)) * scale` and allow a small tweak (±2–4 px).
  - Optionally store per‑glyph `bearingY` during atlas gen and add it at draw time for punctuation.

- Word spacing too tight
  - Expose tracking as a parameter; default to `0.10×fontSize` for headings, `0.06–0.08×fontSize` for button labels. Ensure measurement uses the same tracking value.

---

### Minimal, low‑risk plan
1. Keep single‑pass for now; expose two knobs in renderer:
   - `outlineAAStrokeScale` (default 1.6) and `outlineAAFillScale` (default 1.0).
   - Tracking per draw call (`trackingPx`, default from UI element).
2. In `measureText()`, take `trackingPx` and advances from the same code path used by draw (no constants). Ensure the returned `height` uses CTFont lineHeight.
3. Add `visualCenterOffsetPx` for vertical centering in button labels; apply when computing the centered Y.
4. Bump atlas padding to 16 if we keep 6–8 px outlines frequently.

---

### If artifacts persist (robust option)
- Implement **two‑pass outline**:
  - Pass 1 shader: SDF stroke only (wider AA window, threshold `(edge - t)`), color = outline. No fill.
  - Pass 2 shader: SDF fill only (normal AA around `edge`).
  - Wire two public calls or one API that does both; batching still possible per pass.

---

### Where to change (file map)
- Atlas/metrics/tracking/measurement: `src/iOS/Rendering/MetalRenderer.swift`
- UI centering call sites: `src/FloppyTurd/Systems/RenderSystem.cpp` (drawTextCentered/Outlined)
- Shaders (AA windows, thresholds): `src/iOS/Shaders/Shaders2D.metal`

---

### Quick glossary
- **SDF**: single‑channel signed distance field. Great quality at moderate sizes; corners can round.
- **MSDF**: multi‑channel SDF using RGB to encode edge directions; preserves corners better, heavier atlas.
- **fwidth**: screen‑space derivative; drives smoothstep width for stable AA under scaling.

---

### Recommendation
- Short term: keep single‑pass but expose stroke AA separately, increase atlas padding, and add visual Y‑centering offset + configurable tracking so centering is pixel‑perfect.
- If we still see hairlines, switch to two‑pass stroke→fill. Longer‑term, consider MSDF if we need very crisp tiny text.


