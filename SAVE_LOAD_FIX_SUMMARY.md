# Save/Load System Fix - Complete Summary

## Issues Found and Resolved

### 1. **CRITICAL: Missing V2 to V3 Migration (USER DATA LOSS)**
**Problem**: The save system was upgraded from V2 (JSON format) to V3 (binary plist with HMAC), but there was NO migration path from V2 to V3. When users launched the app after the upgrade:
- The V2 JSON file (`floppyturd_save_v2.json`) was ignored
- The app looked for V3 plist file, didn't find it, and started fresh
- User lost all progress: coins, unlocked levels, high scores, etc.

**Evidence**: 
- Found `floppyturd_save_v2.json` in Documents folder with 8 stored coins, Level 2 unlocked, Level 1 high score of 51
- Logs showed "No save file found, using defaults" at launch
- V3 plist file was created AFTER launch (from fresh saves), not migrated from V2

**Fix**: Added `migrateV2JSONSave()` function in `SaveManager.swift`:
- Detects V2 JSON file on load
- Decodes V2 JSON format
- Converts to V3 `GameSaveData` structure
- Saves as V3 binary plist with HMAC
- Deletes old V2 file after successful migration
- Restores user's progress automatically on next launch

### 2. **CRITICAL: Settings Not Persisting (Audio, Difficulty, Haptics)**
**Problem**: Settings changes in C++ were being lost because Swift was overwriting them during save.

**Flow Before (BROKEN)**:
1. User changes volume in C++ → C++ calls `SaveGameData()`
2. C++ serializes settings from game state → JSON with new volume
3. Swift receives JSON, decodes it
4. Swift **OVERWRITES** JSON settings with OLD UserDefaults values
5. Swift saves the OLD values, ignoring C++ changes

**Flow After (FIXED)**:
1. User changes volume in C++ → C++ calls `SaveGameData()`
2. C++ serializes settings from game state → JSON with new volume
3. Swift receives JSON, decodes it
4. Swift **UPDATES** UserDefaults to match C++ values (keeping in sync)
5. Swift saves the C++ values (authoritative source)

**Fix**: Changed `processSaveGameCommand()` in `SaveManager.swift`:
- Changed from `saveData.settings.masterVolume = GameSettings.masterVolume` (overwrite)
- To `GameSettings.masterVolume = saveData.settings.masterVolume` (sync)
- C++ is now the single source of truth for settings
- UserDefaults is just kept in sync for Swift-side access

### 3. **UI: Entity Cleanup in Exit()**
**Problem**: When exiting MainMenuState, many entities were not being destroyed:
- Ad Controls button (`m_adControlsButtonEntity`)
- Version text (`m_versionTextEntity`)
- All Options menu entities (knobs, tracks, labels, arrows, vibration toggle)
- All Ad Controls menu entities (title, back button, price button, label)

**Result**: Entities persisted across state changes, stayed visible, caused confusion

**Fix**: Updated `MainMenuState::Exit()`:
- Added destruction of all missing entities
- Reset entity IDs to 0 after destruction
- Added comprehensive logging

### 4. **UI: Version Text Off-Screen**
**Problem**: Version text positioned at `m_screenWidth - 60` without accounting for text width, causing it to extend off-screen right

**Fix**: Updated version text positioning in `CreateMobileLayout()`:
- Estimated text width (~50px for "v0.8")
- Changed to `m_screenWidth - versionPaddingRight - estimatedTextWidth`
- Now properly visible in bottom right corner

### 5. **UI: Ad Controls Layout Issues**
**Problem**: 
- Back button used generic `button_normal` texture (inconsistent with other menus)
- $2.00 button used generic texture and was positioned in bottom right
- Not using FloppyButtonBlue like other menu buttons

**Fix**: Updated `CreateAdControlsLayout()`:
- Back button: Changed to `floppybuttonblue`, centered at bottom
- $2.00 button: Changed to `floppybuttonblue`, centered UNDER "Remove Ads" label
- Consistent with other menu layouts

### 6. **UX: Missing Button Debouncing**
**Problem**: When transitioning between menus (e.g., Ad Controls → Main Menu), the "back" button release would immediately trigger a click on the underlying button (e.g., Leaderboard), causing unintended navigation.

**Fix**: Added comprehensive debouncing system:
- Added `m_lastMenuButtonPressTime` timer with 300ms debounce
- Set timer in ALL button press handlers (Play, Options, QuickPlay, Leaderboard, AdControls, Back)
- Set timer in `Resume()` when returning from other states
- Set timer in `Enter()` on state entry
- Check timer in `CheckMenuButtonClicks()` to block rapid clicks

### 7. **Logging: Enhanced Save/Load Diagnostics**
**Problem**: Insufficient logging made it hard to diagnose save/load issues

**Fix**: Added comprehensive logging throughout `SaveManager.swift`:
- File existence checks with full paths
- File size and modification date
- HMAC verification status
- Decode/encode success/failure with error details
- Settings values at save and load time
- Migration detection and progress
- Data snapshots (coins, games played, level unlocks)

---

## Files Modified

### Swift Files
- **`FloppyTurd/src/iOS/Persistence/SaveManager.swift`**:
  - Added `v2SaveFileName` and `v2SaveFileURL` properties
  - Added `migrateV2JSONSave()` function for V2 → V3 migration
  - Fixed `processSaveGameCommand()` to use C++ settings as source of truth
  - Enhanced logging throughout load/save/migration paths

### C++ Files
- **`FloppyTurd/src/FloppyTurd/States/MainMenuState.cpp`**:
  - Fixed `Exit()` to destroy all entities (lines 257-361)
  - Fixed version text positioning (lines 1714-1725)
  - Fixed Ad Controls layout with FloppyButtonBlue (lines 4304-4380)
  - Added debouncing to all button handlers (multiple locations)
  - Set debounce timer in `Resume()` (line 420)
  - Set debounce timer in `Enter()` (line 95)
  - Added debounce check in `CheckMenuButtonClicks()` (line 2201)

---

## How to Avoid Version Increments

To make changes to the save file structure WITHOUT incrementing the version:

1. **Add fields only** - Add new optional fields to `GameSaveData.swift` structs
2. **Use default values** - New fields should have defaults in `init()`
3. **Keep encoding compatible** - Don't change existing field names or types
4. **Test migration** - Verify old saves still load correctly

Example of safe change:
```swift
struct StatisticsData: Codable {
    var totalGamesPlayed: Int
    var totalScore: Int
    // NEW FIELD - safe to add
    var totalBossesDefeated: Int? // Optional, defaults to nil
    
    init() {
        self.totalGamesPlayed = 0
        self.totalScore = 0
        self.totalBossesDefeated = 0 // Default value
    }
}
```

**When you MUST increment version**:
- Removing fields
- Renaming fields
- Changing field types
- Changing structure hierarchy
- Changing binary format (plist → something else)

---

## Testing Checklist

✅ **V2 Migration**: Verified V2 JSON file detected and migrated to V3
✅ **Settings Persistence**: Audio, difficulty, and haptics now save/load correctly
✅ **Entity Cleanup**: All UI entities properly destroyed on state exit
✅ **Version Text**: Visible in bottom right, not off-screen
✅ **Ad Controls UI**: Back and price buttons on FloppyButtonBlue
✅ **Debouncing**: No accidental double-clicks when switching menus

## Current Save File Structure

**Location**: `~/Library/Developer/CoreSimulator/Devices/<UUID>/data/Containers/Data/Application/<UUID>/Documents/`

**Files**:
- `floppyturd_save_v3.plist` - Current save (binary plist with HMAC)
- `floppyturd_save_v2.json` - Legacy V2 (auto-migrated and deleted)
- `floppyturd_save.dat` - Legacy binary (auto-migrated if found)

**Version History**:
- V1: Legacy binary `.dat` format
- V2: JSON format (`floppyturd_save_v2.json`)
- V3: Binary plist with HMAC (current)

---

## Architecture Summary

### Save Flow
```
[C++ Game State Change]
    ↓
SaveGameData() in C++
    ↓
serializeGameData() → JSON with settings from C++ state
    ↓
Swift: processSaveGameCommand(jsonString)
    ↓
Decode JSON → GameSaveData
    ↓
Update UserDefaults FROM GameSaveData (keep in sync)
    ↓
Encode as binary plist + HMAC
    ↓
Write to floppyturd_save_v3.plist
```

### Load Flow
```
[App Launch]
    ↓
LoadGameData() in C++
    ↓
Swift: loadGameDataSync()
    ↓
Check for V2 JSON → migrate if found
Check for legacy .dat → migrate if found
Load V3 plist if exists
    ↓
Verify HMAC
    ↓
Decode binary plist → GameSaveData
    ↓
Apply settings TO both C++ and UserDefaults
    ↓
Convert to KEY:VALUE string
    ↓
Return to C++
    ↓
deserializeGameData() → update game state
```

### Source of Truth
- **Game Progress**: C++ game state (serialized to/from save file)
- **Settings**: C++ game state (UserDefaults kept in sync)
- **Persistence**: Binary plist with HMAC (tamper-proof)

---

## Key Insights

1. **C++ is authoritative** - Swift is the persistence layer, not the source of truth
2. **UserDefaults is a mirror** - Kept in sync for Swift-side convenience, not the primary store
3. **Migration is critical** - Always provide migration path when changing save format
4. **HMAC prevents cheating** - Binary plist with HMAC stops save file tampering
5. **Debouncing prevents UX bugs** - Essential for touch-based navigation between states

---

## Future Improvements

- Add save file backup/restore functionality
- Add save file validation UI (show what will be loaded)
- Add settings reset button (restore defaults)
- Consider cloud save sync (iCloud integration)
- Add save file compression (reduce size)