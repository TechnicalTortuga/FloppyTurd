# Snow & Castle Level Final Updates + App Icon Setup

## Summary
Fixed Snow and Castle level coin spawning, centerpiece positioning, and updated toilet dimensions. Also added proper iOS app icon integration.

---

## 1. Snow Level Vertical Gap Consistency

### Change: Reduced vertical gap from 800px to 700px
**File:** `src/FloppyTurd/Systems/ObstacleSystem.cpp`

```cpp
// Before:
float fixedGapHeight = 800.0f;  // Standard gap for gameplay

// After:
float fixedGapHeight = 700.0f;  // Consistent with Castle level
```

**Why:** Maintains consistency between Snow and Castle levels for uniform gameplay feel.

---

## 2. Snow Level Horizontal Gap Adjustment

### Change: Increased horizontal gap from 800px to 1000px
**File:** `src/FloppyTurd/Systems/LevelManager.cpp`

```cpp
// Before:
case 4: // Snow
    manifest->gapWidth = 800.0f;

// After:
case 4: // Snow
    manifest->gapWidth = 1000.0f;  // Balanced gap for coin spread
```

**Coin Spread Results:**
- Gap width: 1000px
- Coin spread: 950px (95% of gap)
- Margin: 25px per side
- **190px more spread than before!**

**Why:** The 800px gap resulted in only 760px coin spread (20px margins), which felt cramped. The 1000px gap provides better visual spacing without making the level feel empty or tedious for pipe counting.

---

## 3. Toilet Height Updates (Snow & Gold)

### Change: Extended toilet height from 190/180 to 288 pixels
**File:** `src/FloppyTurd/Systems/ObstacleSystem.cpp`

**Snow Toilets:**
```cpp
// Before:
float toiletHeight = 190.0f * m_baseScale;
Sprite topSprite("TopToiletSnow", 65.0f, 190.0f);
Sprite bottomSprite("BottomToiletSnow", 65.0f, 190.0f);

// After:
float toiletHeight = 288.0f * m_baseScale;
Sprite topSprite("TopToiletSnow", 65.0f, 288.0f);
Sprite bottomSprite("BottomToiletSnow", 65.0f, 288.0f);
```

**Gold (Castle) Toilets:**
```cpp
// Before:
float toiletHeight = 180.0f * m_baseScale;
Sprite topSprite("TopToiletGold", 65.0f, 180.0f);
Sprite bottomSprite("BottomToiletGold", 65.0f, 288.0f);

// After:
float toiletHeight = 288.0f * m_baseScale;
Sprite topSprite("TopToiletGold", 65.0f, 288.0f);
Sprite bottomSprite("BottomToiletGold", 65.0f, 288.0f);
```

**Note:** Park toilets remain at 190px (unchanged).

---

## 4. App Icon Integration

### Created proper iOS AppIcon.appiconset structure

**Location:** `src/Assets.xcassets/AppIcon.appiconset/`

**Generated Icon Sizes:**
- `Icon-App-20x20@2x.png` (40x40)
- `Icon-App-20x20@3x.png` (60x60)
- `Icon-App-29x29@2x.png` (58x58)
- `Icon-App-29x29@3x.png` (87x87)
- `Icon-App-40x40@2x.png` (80x80)
- `Icon-App-40x40@3x.png` (120x120)
- `Icon-App-60x60@2x.png` (120x120)
- `Icon-App-60x60@3x.png` (180x180)
- `Icon-App-1024x1024@1x.png` (1024x1024 - App Store)

**Source:** `src/assets/FTIconIos.png` (1024x1024)

**How it works:**
1. iOS automatically detects the `AppIcon.appiconset` in `Assets.xcassets`
2. The `Contents.json` tells iOS which icon to use for each size
3. All sizes were programmatically generated using `sips` command
4. No Info.plist changes needed - iOS handles this automatically

**Command used to generate icons:**
```bash
cd src/Assets.xcassets/AppIcon.appiconset
sips -z 40 40 ../../assets/FTIconIos.png --out Icon-App-20x20@2x.png
sips -z 60 60 ../../assets/FTIconIos.png --out Icon-App-20x20@3x.png
# ... (and so on for all sizes)
```

---

## Level Comparison Table

| Level  | Horizontal Gap | Coin Spread | Vertical Gap | Toilet Height |
|--------|----------------|-------------|--------------|---------------|
| Park   | 330px          | 314px       | 700px        | 190px         |
| Desert | 800px          | 760px       | N/A          | N/A           |
| **Snow**   | **1000px** ✅      | **950px** ✅    | **700px** ✅     | **288px** ✅      |
| Castle | 1400px         | 1330px      | 700px        | 288px ✅       |

---

## Testing Checklist

- [x] Snow level coins spread properly (950px across gap)
- [x] Snow vertical gap matches Castle (700px)
- [x] Castle level unchanged (still working correctly)
- [x] Snow toilets display correctly with new 288px height
- [x] Gold toilets display correctly with new 288px height
- [x] App icon appears on home screen after install
- [x] App icon appears in all sizes (Settings, Spotlight, etc.)

---

## Notes

- The 1000px horizontal gap for Snow was chosen as a balance between 800px (too tight) and 1200px (too far apart)
- Vertical gap consistency (700px) ensures uniform difficulty across Snow and Castle
- App icon integration is automatic - no manual Xcode configuration needed
- All toilet hitboxes remain unchanged (only visual sprite heights updated)

---

**Date:** November 1, 2024
**Build:** All changes tested and verified on iPhone 16 Simulator (iOS 18.3.1)