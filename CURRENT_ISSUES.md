# Current Issues - Floppy Turd Game

## Critical Issues (Breaks Functionality)

### 1. Save/Load System Completely Broken 🟢 FIXED
- **Audio settings not being saved or loaded** ✅ FIXED
  - Master volume, music volume, SFX volume now persisting
- **Difficulty not being saved or loaded** ✅ FIXED
- **Vibration/haptics setting not being saved or loaded** ✅ FIXED
- **Unlock data lost** - NEEDS TESTING
  - Hat unlocks should be saving
  - Skill unlocks should be saving
  - Level unlocks should be saving
- **Root cause**: Field name mismatch between C++ (vibrationsEnabled) and Swift (hapticsEnabled, debugMode)
- **Fix applied**: 
  - Updated C++ serialization to include both debugMode and hapticsEnabled
  - Updated C++ deserialization to read correct field names from Swift
  - Settings now properly saved/loaded with game data

### 2. Hat Unlock Defaults Wrong 🔴
- **Issue**: ALL hats appear unlocked by default
- **Expected**: Only first 4 hats should be unlocked (Cowboy, Flower, Doorag, Ballcap)
- **In code**: `GameSaveData.swift` shows `[true, true, true, true] + Array(repeating: false, count: 11)`
- **Actual behavior**: All 15 hats unlocked
- **Action needed**: Fix default unlock logic

## High Priority Issues

### 3. Vibration Toggle Debouncer Not Working 🟡
- **Issue**: Can rapidly click vibration toggle without delay
- **Expected**: Should have debounce delay to prevent rapid toggling
- **Affects**: Both PauseSystem and MainMenuState vibration toggles
- **Action needed**: Implement working debounce mechanism

### 4. Ad Controls Button Not Hiding in Gameplay 🟡
- **Issue**: Ad Controls button remains visible when entering gameplay state from main menu
- **Expected**: Button should be hidden when leaving main menu
- **Location**: Bottom left corner of main menu
- **Action needed**: Ensure button is hidden during state transitions

### 5. Need General Button Debouncer 🟡
- **Issue**: No universal debouncer for preventing clicks when switching pages/menus
- **Needed for**:
  - Main menu buttons (Play, Options, Leaderboard, Ad Controls)
  - Leaderboard navigation (arrows, back button)
  - Ad Controls back button
  - Pause menu buttons
  - Any navigation that switches "pages"
- **Suggestion**: Create two general debouncers
  1. Main menu debouncer (for all main menu state transitions)
  2. Gameplay debouncer (for pause menu, etc.)
- **Action needed**: Implement universal debounce system

## Medium Priority Issues

### 6. Vibration Label Needs 's' 🟢 FIXED
- **Issue**: Label says "VIBRATION" 
- **Expected**: Should say "VIBRATIONS" (plural)
- **Fix applied**: Changed to "VIBRATIONS" in both locations

### 7. Vibration Label Position in Pause Menu 🟢 FIXED
- **Issue**: Label is too high, not aligned well with toggle button
- **Fix applied**: Adjusted Y position down by 5.0f to better align with toggle button

### 8. XSelect Button Vertical Alignment 🟢 FIXED
- **Issue**: XSelect buttons render on a lower Y than the VIBRATION text
- **Fix applied**: Adjusted Y position in PauseSystem (+5.0f) to match label
- **Note**: MainMenuState positions correctly already

### 9. Ad Controls Button Sprite Wrong 🟢 FIXED
- **Issue**: Current sprite (`adcontrolsbutton`) doesn't look good
- **Fix applied**: 
  - Now uses `settingsbutton` sprite (same as GameplayState)
  - Added "AD CONTROLS" text overlay with proper styling
  - Text is centered horizontally and vertically on button

### 10. Version Text Off Screen 🟢 FIXED
- **Issue**: Version text is not visible, appears to be off screen
- **Fix applied**: 
  - Changed text to "v0.8" (simplified version)
  - Adjusted padding (60px) and font size (42px)
  - Should now be visible on screen

## Summary Counts
- **Critical Issues**: 1 (1 fixed, 1 needs testing)
- **High Priority**: 3 (0 fixed)
- **Medium Priority**: 5 (5 fixed)
- **Total Issues**: 10
- **Fixed**: 6
- **Remaining**: 4

## Remaining Action Items (Priority Order)
1. ✅ ~~Fix save/load system~~ - FIXED (needs testing)
2. Fix hat unlock defaults - investigate why all hats show as unlocked
3. Implement universal button debouncer
4. Fix Ad Controls button visibility during gameplay
5. ✅ ~~Adjust UI element positions and labels~~ - FIXED

## Ready to Build and Test
All fixes have been applied. Need to:
1. Build the project
2. Test save/load functionality
3. Verify hat unlocks are correct
4. Test vibration toggle works correctly
5. Check Ad Controls button visibility