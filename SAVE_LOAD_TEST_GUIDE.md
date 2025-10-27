# Save/Load System Test Guide

## Purpose
This guide helps you verify that the critical save/load bug is fixed and player progress now persists correctly across app launches.

---

## Quick Test (5 minutes)

### 1. Fresh Install Test
```bash
# Clean install - delete app from simulator first
xcrun simctl uninstall booted com.floppyturd.game

# Build and run
xcodebuild -project build_ios/FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -configuration Debug \
  -destination 'platform=iOS Simulator,id=BC7474BC-046F-4717-91E6-A65DBC6D56B5' \
  build
```

**Expected Console Output:**
```
📖 Loading game data...
🔄 [SaveManager C++ Interop] Synchronous load requested from C++
📖 [SaveManager] Processing SYNCHRONOUS load game command...
📖 Loading game data...
ℹ️ No save file found, starting fresh
ℹ️ [SaveManager C++ Interop] No save data to return to C++
ℹ️ No save file found, using defaults
Game data reset to defaults
```

### 2. Play and Save Test
1. **Play the game** - collect some coins (e.g., 50 coins)
2. **Check level unlock** - try to unlock level 2
3. **Equip a hat** (if implemented)
4. **Note your stats**:
   - Coins: ______
   - High Score: ______
   - Levels Unlocked: ______

5. **Exit the app** - force quit from simulator (Cmd+Shift+H twice, swipe up)

### 3. Reload Test
1. **Relaunch the app** from simulator
2. **Check console logs**:

**Expected Console Output (Success):**
```
📖 Loading game data...
🔄 [SaveManager C++ Interop] Synchronous load requested from C++
📖 [SaveManager] Processing SYNCHRONOUS load game command...
📖 Loading game data...
✅ Game data loaded successfully (version 2)
✅ [SaveManager C++ Interop] Returning JSON to C++ (XXXX chars)
✅ Game data loaded and deserialized successfully
📊 Loaded - Coins: 50, High Score: 1250
```

3. **Verify your progress was restored**:
   - ✅ Same coin count
   - ✅ Same high score
   - ✅ Same unlocked levels
   - ✅ Same equipped hat

---

## Detailed Test Scenarios

### Scenario 1: First Time User
**Steps:**
1. Delete app from simulator
2. Launch fresh install
3. Play one game
4. Exit and relaunch

**Expected:**
- First launch: No save data, starts with defaults
- After game: Progress is saved
- Relaunch: Progress is restored

### Scenario 2: Returning User
**Steps:**
1. Launch app with existing save
2. Play more games, earn more coins
3. Exit and relaunch

**Expected:**
- Launch: Old progress restored
- After playing: New progress saved
- Relaunch: Latest progress restored

### Scenario 3: Level Progression
**Steps:**
1. Start fresh
2. Unlock level 2 by meeting requirements
3. Exit and relaunch
4. Verify level 2 is still unlocked

**Expected:**
- Level unlock persists across restarts

### Scenario 4: Hat Inventory
**Steps:**
1. Purchase/unlock a hat
2. Equip the hat
3. Exit and relaunch
4. Verify hat is still owned and equipped

**Expected:**
- Owned hats persist
- Equipped hat persists

---

## Console Log Checklist

When app launches, look for these logs in sequence:

### ✅ Successful Load
```
[1] 📖 Loading game data...
[2] 🔄 [SaveManager C++ Interop] Synchronous load requested from C++
[3] 📖 [SaveManager] Processing SYNCHRONOUS load game command...
[4] 📖 Loading game data...
[5] ✅ Game data loaded successfully (version 2)
[6] ✅ [SaveManager C++ Interop] Returning JSON to C++ (XXXX chars)
[7] ✅ Game data loaded and deserialized successfully
[8] 📊 Loaded - Coins: XXX, High Score: XXX
```

### ✅ No Save File (Fresh Start)
```
[1] 📖 Loading game data...
[2] 🔄 [SaveManager C++ Interop] Synchronous load requested from C++
[3] 📖 [SaveManager] Processing SYNCHRONOUS load game command...
[4] 📖 Loading game data...
[5] ℹ️ No save file found, starting fresh
[6] ℹ️ [SaveManager C++ Interop] No save data to return to C++
[7] ℹ️ No save file found, using defaults
[8] Game data reset to defaults
```

### ❌ Bug Still Present (Should NOT See)
```
[1] 📖 Loading game data...
[2] Game data reset to defaults  ← This should only appear if NO save exists
```

---

## Save File Location

The save file is stored here:
```
~/Library/Developer/CoreSimulator/Devices/[DEVICE_ID]/data/Containers/Data/Application/[APP_ID]/Documents/floppyturd_save_v2.json
```

To inspect the save file manually:
```bash
# Find the app's documents directory
xcrun simctl get_app_container booted com.floppyturd.game data

# View the save file
cat [PATH_FROM_ABOVE]/Documents/floppyturd_save_v2.json | jq .
```

---

## What Data Should Persist?

### Player Progress
- ✅ Coins (stored)
- ✅ High score (legacy)
- ✅ Level stats (all 6 levels):
  - High score per level
  - Best coins collected
  - Best boss time
  - Unlocked status
  - Unlock requirements

### Player Inventory
- ✅ Owned hats
- ✅ Equipped hat
- ✅ Unlocked hats

### Game Statistics
- ✅ Total games played
- ✅ Total score
- ✅ Total coins collected
- ✅ Total deaths
- ✅ Total pipes cleared
- ✅ Total jumps
- ✅ Total enemies killed
- ✅ Total play time
- ✅ Current streak
- ✅ Best streak

### Settings (Separate System)
- ✅ Master volume
- ✅ Music volume
- ✅ SFX volume
- ✅ Debug mode

---

## Troubleshooting

### Issue: "Game data reset to defaults" on every launch
**Diagnosis:** The bug is NOT fixed
**Check:**
1. Verify `FloppyTurdGame::LoadGameData()` does NOT always call `ResetGameData()`
2. Check that the delegate is returning `true` when save data exists
3. Look for error logs during deserialization

### Issue: Save file exists but not loading
**Diagnosis:** Deserialization may be failing
**Check:**
1. Look for "Failed to deserialize" error
2. Inspect JSON file for corruption
3. Check JSON schema matches expected format

### Issue: Data saves but gets overwritten immediately
**Diagnosis:** Another code path may be calling `ResetGameData()`
**Check:**
1. Search for all calls to `ResetGameData()`
2. Ensure it's only called when load fails or no save exists

### Issue: Build fails with linking error
**Diagnosis:** C++ interop not working
**Check:**
1. Swift function must be `public`
2. Function must be at module/global level (not inside a class)
3. C++ must call it from `FloppyTurd::` namespace

---

## Migration Test

If you have old binary save files (`.gamesave`), test migration:

**Steps:**
1. Place old `.gamesave` file in Documents directory
2. Launch app
3. Check for migration logs

**Expected Logs:**
```
🔄 Legacy save file detected - migrating...
✅ Migration successful
✅ Game data loaded successfully (version 2)
```

---

## Success Criteria

The fix is successful if:

1. ✅ Fresh install shows "No save file found, using defaults"
2. ✅ After playing, progress is saved (check console for save logs)
3. ✅ App relaunch shows "Game data loaded and deserialized successfully"
4. ✅ Coin count, high score, and unlocked levels match what you had before exit
5. ✅ No "Game data reset to defaults" unless it's a fresh install
6. ✅ Save file exists in Documents directory and contains valid JSON

---

## Performance Notes

- Save/load is **synchronous** during app initialization
- Typical load time: **< 10ms**
- This is acceptable because it only happens at startup
- The app waits for load to complete before showing UI

---

## Related Documentation

- See `SAVE_LOAD_CRITICAL_FIX.md` for technical details
- See `SERIALIZATION_SYSTEM_COMPLETE.md` for JSON schema
- See `BUILD_INSTRUCTIONS_SERIALIZATION.md` for build info

---

**Last Updated:** 2024
**Status:** ✅ Fix implemented and building successfully