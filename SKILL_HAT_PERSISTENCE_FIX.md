# Skill and Hat Persistence Fix

## Summary of Changes

This document outlines the fixes made to ensure that skills and hats are properly persisted across game sessions.

## Issues Fixed

### 1. Skills Not Persisting
**Problem:** Skills purchased in the game were not being saved. The `SaveSkillProgress()` and `LoadSkillProgress()` methods in `SkillSystem.cpp` were empty TODOs.

**Solution:**
- Added `unlockedSkills` array to the game's `CustomizationData` structure
- Implemented skill persistence methods in `SkillSystem.cpp` to sync with the game's save system
- Updated serialization to include skill unlock status in the save file

### 2. Hat Purchase Status Not Syncing
**Problem:** Hats stayed equipped visually, but the system would ask to buy them again when selected because the unlock status wasn't properly synchronized between the legacy file system and the unified game save system.

**Solution:**
- Updated `HatsSystem` to primarily use the game's unified save system
- Maintained legacy `hat_status.txt` file for backward compatibility
- Ensured all hat operations (buy, equip, select) sync to the game save system

## Files Modified

### C++ Game Code

#### `src/FloppyTurd/Game/FloppyTurdGame.h`
- Added `unlockedSkills` vector to `CustomizationData` struct
- Added methods: `IsSkillUnlocked()` and `UnlockSkill()`

#### `src/FloppyTurd/Game/FloppyTurdGame.cpp`
- Implemented `IsSkillUnlocked()` method to check if a skill is unlocked by index
- Implemented `UnlockSkill()` method to unlock a skill and trigger save

#### `src/FloppyTurd/Systems/SkillSystem.cpp`
- Fully implemented `SaveSkillProgress()` to save all 5 skill unlock states to game save system
- Fully implemented `LoadSkillProgress()` to restore skill unlock states from game save system
- Skills are mapped by their enum values:
  - HalfHearts = 0
  - ThirdHearts = 1
  - CoinMagnet = 2
  - HeartMagnet = 3
  - CoinSafetyNet = 4

#### `src/FloppyTurd/Systems/HatsSystem.cpp`
- Updated `SelectHat()` to sync selected hat index to game save
- Updated `BuySelectedHat()` to sync unlocked hat to game save
- Updated `EquipSelectedHat()` to sync equipped hat to game save
- Updated `SaveHatStatus()` to prioritize game save system over legacy file
- Updated `LoadHatStatus()` to prioritize game save system over legacy file

### Serialization Layer

#### `src/Engine/Platform/SaveGameHelpers.h`
- Added `unlockedSkills` array serialization to JSON output
- Added parsing for `CUSTOM_SKILL_X_UNLOCKED` keys in deserialization
- Skills are stored as 5 boolean values (one per skill type)

### Swift/iOS Persistence

#### `src/iOS/Persistence/GameSaveData.swift`
- Added `unlockedSkills: [Bool]` property to `CustomizationData` struct
- Initialized with 5 skills, all locked by default

#### `src/iOS/Persistence/SaveManager.swift`
- Added skill unlock data to the KEY:VALUE output format for C++ bridge
- Format: `CUSTOM_SKILL_0_UNLOCKED:1` (where 0 is skill index, 1 means unlocked)

## Data Flow

### Saving Skills
1. User purchases skill in pause menu
2. `SkillSystem::UnlockSkill()` marks skill as unlocked and calls `SaveSkillProgress()`
3. `SaveSkillProgress()` calls `FloppyTurdGame::UnlockSkill(skillIndex)` for each unlocked skill
4. `FloppyTurdGame` updates `m_customizationData.unlockedSkills[index]` and triggers `SaveGameData()`
5. Save data is serialized to JSON via `SaveGameHelpers::serializeGameData()`
6. Swift `SaveManager` receives JSON and persists to binary plist with HMAC

### Loading Skills
1. Game starts, `SkillSystem` constructor calls `LoadSkillProgress()`
2. `LoadSkillProgress()` queries `FloppyTurdGame::IsSkillUnlocked(skillIndex)` for each skill
3. If unlocked, skill is marked as unlocked and auto-activated (for passive skills)
4. Game data was previously loaded from Swift save file via `deserializeGameData()`

### Saving Hats
1. User selects/buys/equips hat in pause menu
2. `HatsSystem` calls appropriate game method:
   - `SelectHat()` → `SetSelectedHatIndex()`
   - `BuySelectedHat()` → `UnlockHat()`
   - `EquipSelectedHat()` → `SetEquippedHatIndex()`
3. Each game method updates `m_customizationData` and triggers `SaveGameData()`
4. `HatsSystem` also calls `SaveHatStatus()` which saves to both game save and legacy file

### Loading Hats
1. Game starts, `HatsSystem` constructor loads from game save
2. `LoadHatStatus()` queries game for:
   - `GetEquippedHatIndex()`
   - `GetSelectedHatIndex()`
   - `IsHatUnlocked(index)` for each hat
3. Hat status is updated based on save data
4. Legacy file is maintained for backward compatibility but not prioritized

## Save File Format

The unified save file uses a structured KEY:VALUE format (one per line) for C++ parsing:

```
# Skills (5 total)
CUSTOM_SKILL_0_UNLOCKED:1
CUSTOM_SKILL_1_UNLOCKED:0
CUSTOM_SKILL_2_UNLOCKED:1
CUSTOM_SKILL_3_UNLOCKED:0
CUSTOM_SKILL_4_UNLOCKED:1

# Hats (15 total)
CUSTOM_EQUIPPED_HAT:5
CUSTOM_SELECTED_HAT:5
CUSTOM_HAT_0_UNLOCKED:1
CUSTOM_HAT_1_UNLOCKED:1
# ... (remaining hats)
CUSTOM_HAT_14_UNLOCKED:0
```

## Build Status

✅ **Build Successful** - All changes compiled successfully on iOS Simulator (iPhone 16, iOS 18.3.1)

### Compiler Errors Fixed
1. Fixed `std::map::operator[]` requiring default constructor by using `find()` instead in `LoadSkillProgress()`
2. Fixed hat unlock status not loading properly - added unlock status loading in HatsSystem constructor
3. Fixed locked frame overlays not hiding after loading - added refresh loop after LoadHatStatus()

### Hat System Bugs Fixed
1. **Hat unlock status not persisting**: Fixed by loading unlock status from game save in HatsSystem constructor
2. **Locked frame overlays not disappearing**: Fixed by adding visibility update after LoadHatStatus() in CreateHatsGrid()
3. **Wrong default unlock state**: Fixed by unlocking first 4 hats by default in both Swift and C++ CustomizationData

## Testing Checklist

- [x] Skills persist across game sessions ✅
- [x] Skills remain unlocked after restart ✅
- [x] Purchased hats remain unlocked after restart ✅
- [x] Equipped hat remains equipped after restart ✅
- [x] Selected hat in menu is remembered after restart ✅
- [x] Hat unlock status properly syncs between purchase and display ✅
- [x] Locked frame overlays disappear when hats are unlocked ✅
- [x] First 4 hats are unlocked by default ✅
- [x] No duplicate purchases required for already-owned items ✅

## Migration Notes

- Existing players with unlocked skills will need to re-unlock them (fresh start for skills)
- Existing hat data from `hat_status.txt` will be migrated to the unified save on first load
- The game maintains both the legacy file and unified save for one version cycle
- **Important**: First 4 hats (Cowboy, Flower, Doorag, Ballcap) are now unlocked by default for all players

## Future Improvements

- Consider removing legacy `hat_status.txt` file after migration period
- Add validation to ensure skill indices match enum values
- Add logging for debugging save/load issues
- Consider adding a "restore purchases" button for recovery

## Technical Notes

### Compiler Fixes Applied
1. **Skills**: Changed `m_skills[SkillType::X]` to `m_skills.find(SkillType::X)` to avoid requiring default constructor
2. **Hats**: No compiler errors, but logic errors fixed

### Hat System Logic Fixes

#### Problem 1: Hat Unlock Status Not Loading
**Root Cause**: `InitializeHats()` hardcoded hat status (first 4 UNLOCKED, rest LOCKED), but this happened before loading from save system. The HatsSystem constructor loaded equipped/selected indices but NOT unlock status.

**Solution**: Added unlock status loading in HatsSystem constructor after `InitializeHats()`:
```cpp
// Load unlock status for all hats from game save
for (size_t i = 0; i < m_hats.size(); ++i) {
    if (GameCore::GetGame()->IsHatUnlocked(static_cast<int>(i))) {
        m_hats[i].status = HatStatus::UNLOCKED;
    }
}
```

#### Problem 2: Locked Frame Overlays Not Disappearing
**Root Cause**: In `CreateHatsGrid()`, locked frame overlays were created based on initial hat status (LOCKED), then `LoadHatStatus()` updated the status to UNLOCKED, but the visual overlays were never updated.

**Solution**: Added visibility update loop after `LoadHatStatus()` in `CreateHatsGrid()`:
```cpp
// After loading, hide locked frames for unlocked hats
for (size_t i = 0; i < m_hats.size(); ++i) {
    if (m_hats[i].status == HatStatus::UNLOCKED) {
        // Hide sprite and UI element for locked frame overlay
    }
}
```

#### Problem 3: Default Unlock State Mismatch
**Root Cause**: Swift `CustomizationData` only unlocked first hat, but HatsSystem expected first 4 hats unlocked.

**Solution**: Updated both Swift and C++ `CustomizationData` to unlock first 4 hats by default:
- Swift: `self.unlockedHats = [true, true, true, true] + Array(repeating: false, count: 11)`
- C++: `unlockedHats[0..3] = true`

### Key Implementation Details
- Skills are saved immediately when unlocked via `SaveSkillProgress()`
- Skills are loaded once during `SkillSystem` construction via `LoadSkillProgress()`
- Passive skills (all current skills) are auto-activated when loaded
- Hat operations sync to game save on every change (select, buy, equip)
- Hat unlock status is loaded in HatsSystem constructor (BEFORE grid creation)
- Locked frame overlays are hidden after LoadHatStatus() (AFTER grid creation)
- Legacy file system maintained for backward compatibility but not prioritized