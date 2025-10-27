# Save/Load System Debug Checklist

## Current Status: Still Not Working ❌

**Problem**: Game data is still being reset to defaults on every launch.

---

## Evidence from Logs (Latest Run)

### What the Logs Show:
```
2025-10-26 19:44:43.207 [INFO] 📖 Loading game data...
2025-10-26 19:44:43.207 [INFO] 💾 Saving game data... Stored coins: 0
2025-10-26 19:44:43.207 [INFO] ✅ Game data loaded and deserialized successfully
2025-10-26 19:44:43.208 [INFO] ✅ Unlock requirements set for all levels after load
```

### What This Means:
1. ✅ LoadGameData() is called
2. ❌ SaveGameData() is called IMMEDIATELY (with 0 coins!)
3. ✅ Deserialization claims success
4. ✅ Unlock requirements are set

**CRITICAL**: The save happens BETWEEN load and deserialize, overwriting the file with zeros!

---

## JSON File Contents (After Save)

Location: `Documents/floppyturd_save_v2.json`

```json
{
  "statistics": {
    "storedCoins": 0,           // ❌ Should have actual coins
    "totalGamesPlayed": 0       // ❌ Should have actual games
  },
  "progress": {
    "levels": [
      {
        "levelId": 1,
        "unlocked": false,        // ❌ Level 1 should ALWAYS be true!
        "highScore": 0
      }
    ]
  }
}
```

**Problem**: Everything is 0 and level 1 is locked!

---

## Root Cause Analysis

### Question 1: Who is calling SaveGameData during load?

**Suspects**:
1. ❌ `UpdateLevelStats()` - Should be blocked by `m_isLoadingGameData` flag
2. ❌ `UpdateGameStats()` - Need to check if this triggers a save
3. ❌ Swift SaveManager - Might be saving after load
4. ❌ Some other code path during initialization

**Action**: Added detailed logging to SaveGameData() to show:
- When it's called
- Value of `m_isLoadingGameData` flag
- Caller identification

### Question 2: Is the data being loaded correctly?

**Check**:
- [ ] Swift SaveManager.loadGameDataSync() is returning valid JSON
- [ ] JSON string is being passed to C++ correctly
- [ ] Deserialization is parsing all fields correctly
- [ ] Game state variables are being updated

**Test**: Add logging in deserializeGameData() to show what values are being read

### Question 3: Is the loading flag working?

**Timeline**:
```
LoadGameData() starts
  └─> Set m_isLoadingGameData = true
      └─> Call loadGameData delegate
          └─> Swift returns JSON
              └─> Call deserializeGameData()
                  └─> Call UpdateLevelStats() (6 times)
                      └─> Check m_isLoadingGameData
                          └─> If true: skip SaveGameData() ✅
                          └─> If false: call SaveGameData() ❌
```

**Status**: Flag SHOULD be working, but SaveGameData is still being called

---

## Possible Issues

### Issue A: SaveGameData called from deserializeGameData
**Hypothesis**: The deserialize helper might be calling SaveGameData directly

**Check**:
```cpp
// In SaveGameHelpers.h deserializeGameData()
game.UpdateGameStats(stats);      // Does this trigger a save?
game.UpdateLevelStats(i, stats);  // Does this trigger a save?
game.UpdateCustomizationData(customization);  // Does this trigger a save?
```

**Action**: Check if UpdateGameStats and UpdateCustomizationData also call SaveGameData

### Issue B: Multiple SaveGameData implementations
**Hypothesis**: There might be two SaveGameData functions and we're modifying the wrong one

**Check**:
- Search entire codebase for `void.*SaveGameData`
- Verify there's only ONE implementation

### Issue C: Swift is saving after load
**Hypothesis**: Swift SaveManager might save the data after loading it

**Check**:
- Review Swift SaveManager.processLoadGameCommandSync()
- Look for any save calls after load

### Issue D: UpdateGameStats triggers save
**Hypothesis**: UpdateGameStats might call SaveGameData (like UpdateLevelStats does)

**Check**:
```cpp
void FloppyTurdGame::UpdateGameStats(const GameStats& stats) {
    m_gameStats = stats;
    SaveGameData();  // ❌ If this exists, it needs the same flag check!
}
```

---

## Fix Applied (Latest)

### Added Early Exit in SaveGameData():
```cpp
void FloppyTurdGame::SaveGameData() {
    GN_LOG_INFO("💾 SaveGameData() CALLED - Stored coins: " + 
                std::to_string(m_gameStats.storedCoins) + 
                ", Loading flag: " + std::to_string(m_isLoadingGameData));
    
    // Skip saving if we're currently loading
    if (m_isLoadingGameData) {
        GN_LOG_WARN("⚠️ SaveGameData() skipped - currently loading game data");
        return;
    }
    
    // ... rest of save logic
}
```

**Expected Result**: 
- SaveGameData should be skipped during load
- No save should happen until after deserialization completes

---

## Testing Steps

### Step 1: Clean Install
```bash
# Delete app from simulator
xcrun simctl uninstall booted com.floppyturd.game

# Delete any cached data
rm -rf ~/Library/Developer/CoreSimulator/Devices/*/data/Containers/Data/Application/*/Documents/floppyturd_save_v2.json
```

### Step 2: First Run (Fresh)
**Expected Logs**:
```
📖 Loading game data...
💾 SaveGameData() CALLED - Stored coins: 0, Loading flag: 1
⚠️ SaveGameData() skipped - currently loading game data
ℹ️ No save file found, using defaults
🔄 ResetGameData called - resetting to defaults
```

**Check**:
- [ ] No save file exists yet
- [ ] Defaults are applied
- [ ] Level 1 is unlocked
- [ ] SaveGameData was skipped during load

### Step 3: Play Game
**Actions**:
- Play one game
- Collect coins (e.g., 50 coins)
- Complete level
- Exit to main menu

**Expected**:
- SaveGameData should be called AFTER gameplay
- JSON file should have 50 coins
- Level 1 should be unlocked: true

### Step 4: Reload (Exit and Relaunch)
**Expected Logs**:
```
📖 Loading game data...
💾 SaveGameData() CALLED - Stored coins: 0, Loading flag: 1
⚠️ SaveGameData() skipped - currently loading game data
🔄 [SaveManager C++ Interop] Synchronous load requested from C++
📖 [SaveManager] Processing SYNCHRONOUS load game command...
✅ Game data loaded successfully (version 2)
✅ [SaveManager C++ Interop] Returning JSON to C++ (XXXX chars)
✅ Game data loaded and deserialized successfully
📊 Loaded - Coins: 50, High Score: XXX
✅ Unlock requirements set for all levels after load
```

**Check**:
- [ ] SaveGameData was skipped (flag = 1)
- [ ] Swift loaded the JSON successfully
- [ ] Deserialization shows 50 coins
- [ ] Game state shows 50 coins in UI

---

## Functions That Need Loading Flag Check

All these functions call SaveGameData and need the flag check:

1. ✅ `UpdateLevelStats()` - Already has check
2. ❓ `UpdateGameStats()` - NEED TO CHECK
3. ❓ `UpdateCustomizationData()` - NEED TO CHECK
4. ❓ `SetEquippedHatIndex()` - NEED TO CHECK
5. ❓ `SetSelectedHatIndex()` - NEED TO CHECK
6. ❓ `UnlockLevel()` - NEED TO CHECK
7. ❓ `UpdateLevelHighScore()` - NEED TO CHECK

**Action**: Search for all functions that call SaveGameData() and add the flag check to each

---

## Next Debug Steps

### 1. Add Logging to UpdateGameStats
```cpp
void FloppyTurdGame::UpdateGameStats(const GameStats& stats) {
    GN_LOG_INFO("📊 UpdateGameStats called - storedCoins: " + 
                std::to_string(stats.storedCoins));
    m_gameStats = stats;
    // Does this call SaveGameData?
}
```

### 2. Add Logging to deserializeGameData
```cpp
// In SaveGameHelpers.h
inline bool deserializeGameData(FloppyTurdGame& game, const std::string& jsonString) {
    GN_LOG_INFO("🔍 Deserialize: Parsing statistics section...");
    stats.storedCoins = parseJSONInt(statsSection, "storedCoins");
    GN_LOG_INFO("🔍 Deserialize: Read storedCoins = " + std::to_string(stats.storedCoins));
    
    game.UpdateGameStats(stats);
    GN_LOG_INFO("🔍 Deserialize: UpdateGameStats called");
    
    // ... parse levels ...
    game.UpdateLevelStats(i, levelStats);
    GN_LOG_INFO("🔍 Deserialize: UpdateLevelStats(" + std::to_string(i) + ") called");
}
```

### 3. Verify Swift is returning data
```swift
public func loadGameDataSync() -> String {
    guard let jsonString = SaveManager.processLoadGameCommandSync() else {
        print("ℹ️ [SaveManager C++ Interop] No save data to return to C++")
        return ""
    }
    
    print("✅ [SaveManager C++ Interop] Returning JSON to C++:")
    print(jsonString)  // Print the ACTUAL JSON being returned
    
    return jsonString
}
```

---

## Success Criteria

The system is working correctly when:

1. ✅ Fresh install shows defaults with Level 1 unlocked
2. ✅ Playing games updates stats and coins
3. ✅ SaveGameData is called AFTER gameplay, not during load
4. ✅ JSON file contains actual game data (not zeros)
5. ✅ App relaunch loads the saved data correctly
6. ✅ Coins, scores, and unlocked levels persist across restarts
7. ✅ Level unlock requirements are enforced (not all 0)

---

## Current Build Status

- ✅ Code compiles successfully
- ✅ SaveGameData has early exit if loading flag is set
- ❓ Need to test if this fixes the issue
- ❓ Need to check other functions that call SaveGameData

**Next**: Run the app and check the new logs to see if SaveGameData is being skipped during load.