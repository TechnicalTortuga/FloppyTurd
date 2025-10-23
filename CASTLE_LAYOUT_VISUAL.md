# Castle Level Layout - Visual Reference

## Overview
This document provides a visual representation of how elements are positioned in the Castle level.

---

## Top-Down View (Horizontal Layout)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│  Screen Edge                    2000px GAP                Screen Edge           │
│                                                                                 │
│     ┌────┐                                                    ┌────┐           │
│     │    │                                                    │    │           │
│     │Top │              ╔════════════╗                       │Top │           │
│     │Toi │              ║            ║                       │Toi │           │
│  🔥 │let │              ║ CURTAIN    ║                    🔥 │let │ 🔥        │
│     │    │              ║ (256x512)  ║                       │    │           │
│     └────┘              ║  @8x Scale ║                       └────┘           │
│                         ║            ║                                         │
│       │                 ║            ║                         │               │
│       │                 ║            ║                         │               │
│       │   1000px        ║  Centered  ║        1000px          │               │
│       │                 ║   on Pair  ║                         │               │
│       │                 ║            ║                         │               │
│       ├─────────────────╫────────────╫─────────────────────────┤               │
│       │                 ║            ║                         │               │
│       │                 ║            ║                         │               │
│       │                 ║            ║                         │               │
│       │                 ║            ║                         │               │
│       │    🕯️ or ⚔️    ║            ║     🕯️ or ⚔️          │               │
│       │                 ║            ║                         │               │
│       │   CENTERPIECE   ║            ║   CENTERPIECE          │               │
│       │   (pillar or    ║            ║   (next group)         │               │
│       │    spikeball)   ║            ║                         │               │
│       │                 ║            ║                         │               │
│     ┌────┐              ║            ║                       ┌────┐           │
│     │    │              ║            ║                       │    │           │
│  🔥 │Bot │              ║            ║                    🔥 │Bot │ 🔥        │
│     │Toi │              ║            ║                       │Toi │           │
│     │let │              ║            ║                       │let │           │
│     │    │              ╚════════════╝                       │    │           │
│     └────┘                                                    └────┘           │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘

Legend:
🔥 = Floor Torch (80px from toilet)
🕯️ = Chandelier (400px from center)
⚔️ = Pillar or SpikeBall (TRUE CENTER of gap)
```

---

## Vertical Layout (Side View)

```
0px ─────────────────────────────────────────────────────────────
     │                                                           │
     │  ╔══════════════════════════════════════════════════╗    │
     │  ║                                                  ║    │
     │  ║           CURTAIN (spans full height)           ║    │
     │  ║                                                  ║    │
     │  ║              256x512 @ 8x scale                  ║    │
     │  ║              = 2048 x 4096 pixels                ║    │
     │  ║                                                  ║    │
25%  │  ║    ╭───────╮  ← RatCopter Top Range            ║    │
     │  ║    │  🐀   │                                     ║    │
     │  ║    ╰───────╯                                     ║    │
     │  ║                                                  ║    │
     │  ║                  🏛️ PILLAR                       ║    │
     │  ║              (centered vertically)               ║    │
50%  │  ║                     OR                           ║    │ Screen
     │  ║                  ⚔️ SPIKEBALL                    ║    │ Height
     │  ║                   +  BASE                        ║    │
     │  ║             (both centered)                      ║    │
     │  ║                                                  ║    │
60%  │  ║    ╭───────╮  ← RatCopter Bottom Range         ║    │
     │  ║    │  🐀   │                                     ║    │
     │  ║    ╰───────╯                                     ║    │
     │  ║                                                  ║    │
     │  ║                                                  ║    │
     │  ║                                                  ║    │
     │  ║                                                  ║    │
     │  ╚══════════════════════════════════════════════════╝    │
100% ─────────────────────────────────────────────────────────────
```

---

## Centerpiece Positioning Details

### OLD (INCORRECT) Calculation
```
centerX = gapStartX + (gapWidth * 0.5) - 16.0 - 128.0
        = toiletX + 1000.0 - 16.0 - 128.0
        = toiletX + 856.0    ← OFF-CENTER by 144px!
```

### NEW (CORRECT) Calculation
```
centerX = gapStartX + (gapWidth * 0.5)
        = toiletX + 1000.0    ← TRUE CENTER of 2000px gap
```

---

## Curtain Positioning Details

### Curtain Center on Toilet Pair
```
Toilet Position:     x
Toilet Width:        65px (raw) * 8.0 scale = 520px
Curtain Width:       256px (raw) * 8.0 scale = 2048px

Curtain X = toiletX - (curtainWidth * 0.5) + (toiletWidth * 0.5)
          = toiletX - 1024.0 + 260.0
          = toiletX - 764.0

This centers the curtain on the toilet pair!
```

### Curtain Vertical Span
```
Curtain Y:           0.0 (top of screen)
Curtain Height:      512px (raw) * 8.0 scale = 4096px
Screen Height:       2556px (iPhone 16 Portrait)

Result: Curtain extends beyond screen (4096 > 2556)
        → Fully covers from top to bottom ✓
```

---

## SpikeBall + Base Centering

### Base Positioning
```
Base Size:           10x10 (raw) * 8.0 scale = 80x80px
Gap Center X:        toiletX + 1000.0
Base Width:          80px

Base Center X = gapCenterX - (baseWidth * 0.5) + (5.0 * scale)
              = (toiletX + 1000.0) - 40.0 + 40.0
              = toiletX + 1000.0    ← TRUE CENTER

Base Top-Left X = baseCenterX - (5.0 * scale)
                = (toiletX + 1000.0) - 40.0
                = toiletX + 960.0
```

### SpikeBall Positioning
```
SpikeBall Size:      64x90 (raw)
Position:            Same as base center (pivot rotation handles offset)
Rotation Pivot:      0.0, -45.0 (from sprite center to chain connection)

Result: SpikeBall rotates around base center ✓
```

---

## Pillar Centering

### Pillar Positioning
```
Pillar Size:         96x512 (raw) * 8.0 scale = 768x4096px
Gap Center X:        toiletX + 1000.0
Pillar Width:        768px

Pillar X = gapCenterX - (pillarWidth * 0.5)
         = (toiletX + 1000.0) - 384.0
         = toiletX + 616.0    ← Horizontally centered

Pillar Y = (screenHeight / 2) - (pillarHeight / 2)
         = 1278.0 - 2048.0
         = -770.0    ← Vertically centered (top half offscreen)
```

---

## RatCopter Y-Range

### OLD (FIXED PIXELS - WRONG)
```
minY = 600.0
maxY = 1400.0
Range = 800px

On iPhone 16 (2556px height):
  600 / 2556 = 23.5%  ← Too variable
 1400 / 2556 = 54.8%  ← Not consistent across devices
```

### NEW (PERCENTAGE - CORRECT)
```
minY = screenHeight * 0.25  (25%)
maxY = screenHeight * 0.60  (60%)
Range = screenHeight * 0.35 (35%)

On iPhone 16 (2556px height):
  minY = 639px   ← Lifted bottom row up
  maxY = 1534px  ← Brought top row down
  Range = 895px  ← Good spread

On iPad (2732px height):
  minY = 683px   ← Scales correctly
  maxY = 1639px  ← Scales correctly
  Range = 956px  ← Proportional
```

---

## Layer Z-Ordering

```
Layer 0: Castle Background
         ↓
Layer 2: Curtains, Pillar, SpikeBall Base, Chandeliers
         ↓ (decorative elements behind gameplay)
         ↓
Layer 3: Floor Torches, SpikeBalls
         ↓ (interactive elements)
         ↓
Layer 4: Enemies (RatCopters)
         ↓
Layer 5: Player
         ↓ (always on top)
```

---

## Complete Toilet Group Composition

Each toilet group spawns:
1. **Top Toilet** (oscillates vertically)
2. **Bottom Toilet** (oscillates vertically)
3. **Left Floor Torch** (animated, 4 frames)
4. **Right Floor Torch** (animated, 4 frames)
5. **Curtain** (centered on pair, full height) ← NEW
6. **Centerpiece** (in gap, rotates through 3 types):
   - Torch Pillar (animated, 4 frames)
   - Decorative Painting (static)
   - SpikeBall + Base (rotating)
7. **Left Chandelier** (animated, 4 frames)
8. **Right Chandelier** (animated, 4 frames)

---

## Spacing Summary

```
Toilet Group Width:        ~520px (toilet pair)
Gap Between Groups:        2000px
Total Pattern Width:       ~2520px

Elements in Gap:
- Chandeliers at ±400px from center
- Centerpiece at TRUE CENTER (0px offset)

Distance Calculations:
- Toilet to Left Chandelier:   1000 - 400 = 600px
- Toilet to Centerpiece:        1000px
- Toilet to Right Chandelier:   1000 + 400 = 1400px
- Centerpiece to Left Chand:    400px
- Centerpiece to Right Chand:   400px
```

---

## Testing Coordinates

For a toilet at `x = 1000.0`:

| Element | Expected X | Expected Y | Notes |
|---------|-----------|-----------|-------|
| Curtain | 236.0 | 0.0 | Centered on toilet |
| Left Torch | 920.0 | ~2330 | 80px left of toilet |
| Right Torch | 1600.0 | ~2330 | 80px right + toilet width |
| Centerpiece | 2000.0 | ~1278 | TRUE center of gap |
| Left Chandelier | 1600.0 | 0.0 | 400px left of center |
| Right Chandelier | 2400.0 | 0.0 | 400px right of center |
| SpikeBall Base | 1960.0 | ~1278 | Centered (top-left coord) |
| RatCopter Top | varies | 639.0 | 25% screen height |
| RatCopter Bottom | varies | 1534.0 | 60% screen height |

---

## Assets Reference

All textures in: `src/assets/graphics/environment/backgrounds/castle/`

| Asset | Size | Frames | Purpose |
|-------|------|--------|---------|
| curtains.png | 256x512 | 1 | Decorative backdrop |
| TorchPillar | 96x512 | 4 | Centerpiece (animated) |
| SpikeBall | 64x90 | 1 | Centerpiece (rotating) |
| SpikeBallBase | 10x10 | 1 | Rotation anchor |
| castlelevelfloortorch | 32x32 | 4 | Side decoration |
| castlelevelchandelier | 32x34 | 4 | Top decoration |
| PaintingA/B/C/D | 32x64 | 1 | Centerpiece (static) |

---