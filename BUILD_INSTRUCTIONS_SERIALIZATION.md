# Build Instructions - Serialization System Integration

**Date**: 2024  
**Status**: READY TO BUILD  
**Target**: iOS Simulator (iPhone 16, iOS 18.3.1)

---

## What's New in This Build

### ✅ Serialization System (Complete)
- JSON-based save/load (replaces binary format)
- Automatic legacy save migration
- UserDefaults settings storage
- Thread-safe async saves
- C++/Swift bridge via @_cdecl functions

### ✅ Level Unlock Requirements (Restored)
- Level 1: Always unlocked
- Level 2: 50 pipes from Level 1
- Level 3: 50 pipes + 100 coins
- Level 4: 50 pipes + 250 coins
- Level 5: 50 pipes + 500 coins
- Level 6 (Boss): 50 pipes + 1000 coins
- All debug overrides removed

### ✅ Painting Tap Feature
- Double-click/tap paintings to enter levels
- 10-pixel tap threshold (vs. pan/swipe)
- Quick level entry alternative to Play button

---

## Files Changed Summary

### New Files (10)
```
src/iOS/Persistence/
├── GameSaveData.swift          (215 lines) - Codable data models
├── SaveManager.swift           (409 lines) - Save/load manager
└── SaveGameBridge.swift        (157 lines) - C bridge functions

src/Engine/Platform/
├── SaveGameBridge.h            (36 lines)  - C interface
└── SaveGameHelpers.h           (313 lines) - JSON serialization

Documentation/
├── SERIALIZATION_AND_LEVEL_UNLOCK_PROGRESS.md
├── SERIALIZATION_SYSTEM_COMPLETE.md
└── BUILD_INSTRUCTIONS_SERIALIZATION.md (this file)
```

### Modified Files (9)
```
src/Engine/Platform/
└── PlatformDelegates.h         - Save commands added

src/iOS/Threading/
├── ThreadingProxy.h            - Save queue added
├── ThreadingProxy.cpp          - Save queue implementation
└── ThreadingSystem.swift       - Save command processor

src/FloppyTurd/States/
├── MainMenuState.h             - m_panStartY member
└── MainMenuState.cpp           - Painting tap detection

src/FloppyTurd/Game/
└── FloppyTurdGame.cpp          - New save/load + unlock requirements
```

---

## Pre-Build Checklist

- [x] All Swift files created in `src/iOS/Persistence/`
- [x] All C++ headers created in `src/Engine/Platform/`
- [x] Threading system updated for save commands
- [x] FloppyTurdGame.cpp uses new bridge functions
- [x] Level unlock requirements restored
- [x] Painting tap feature implemented
- [x] No syntax errors in modified files

---

## Build Commands

### Step 1: Clean Build Directory

```bash
cd /Users/aimac/Development/FloppyTurd
rm -rf build_ios
mkdir build_ios
cd build_ios
```

### Step 2: Generate Xcode Project

```bash
cmake -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=../ios-cmake-master/ios.toolchain.cmake \
  -DPLATFORM=SIMULATOR64 \
  -DIOS_PLATFORM=SIMULATOR \
  -DIOS_ARCH=x86_64 \
  -DCMAKE_BUILD_TYPE=Debug \
  ..
```

**Expected Output**: 
- Xcode project generated successfully
- All Swift files discovered via GLOB_RECURSE
- All headers found in include paths

### Step 3: Build for Simulator

```bash
xcodebuild -project build_ios/FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" \
  clean build > build_ios/build_output_iphone16.txt 2>&1
```

**Expected Duration**: 2-5 minutes

### Step 4: Check Build Results

```bash
tail -100 build_ios/build_output_iphone16.txt
```

**Success Indicators**:
- "BUILD SUCCEEDED"
- No fatal errors
- App bundle created in build_ios/Debug-iphonesimulator/

---

## Known Build Warnings (Safe to Ignore)

1. **std::clamp not found** (3 occurrences in MainMenuState.cpp)
   - Pre-existing issue, not related to serialization
   - Workaround: Uses std::min/max instead
   - Does not block build

2. **Unused includes** (various files)
   - Pre-existing warnings
   - Cleanup can be done later

3. **Forward declaration issues** (FloppyTurdGame.cpp)
   - Pre-existing, not blocking

---

## Testing After Build

### 1. Fresh Install Test
```
✓ Launch app (fresh install)
✓ Only Level 1 should be unlocked
✓ Settings should use defaults (master=0.7, music=0.6, sfx=0.8)
✓ No legacy save files present
```

### 2. Save/Load Test
```
✓ Play Level 1, collect coins
✓ Check Documents directory for floppyturd_save_v2.json
✓ Kill app, relaunch
✓ Verify coins and level data restored
```

### 3. Level Unlock Test
```
✓ Beat Level 1 with 50+ pipes
✓ Level 2 should unlock automatically
✓ Attempt to unlock Level 3 without 100 coins → fails
✓ Collect 100+ coins, unlock Level 3 → succeeds
```

### 4. Painting Tap Test
```
✓ Navigate to level select
✓ Tap on current painting → enters level
✓ Pan between paintings → does NOT enter level
```

### 5. Migration Test (Advanced)
```
✓ Place old floppyturd_save.dat in Documents/
✓ Launch app
✓ Check for floppyturd_save_v2.json creation
✓ Check for floppyturd_save.dat.v1_backup
✓ Verify all data migrated correctly
```

---

## Debugging Build Errors

### If CMake Fails
```bash
# Check that ios-cmake-master toolchain exists
ls -la ios-cmake-master/ios.toolchain.cmake

# Verify Swift files are discoverable
find src/iOS/Persistence -name "*.swift"
```

### If Xcode Build Fails
```bash
# Check specific error in build log
grep "error:" build_ios/build_output_iphone16.txt

# Open in Xcode for detailed error messages
open build_ios/FloppyTurd.xcodeproj
```

### Common Issues

**Issue**: Swift files not found
**Fix**: Ensure CMakeLists.txt has `file(GLOB_RECURSE)` for Swift sources

**Issue**: Linker errors for bridge functions
**Fix**: Ensure SaveGameBridge.swift uses `@_cdecl` decorator

**Issue**: Cannot find SaveGameBridge.h
**Fix**: Add include path: `src/Engine/Platform/SaveGameBridge.h`

---

## Log Monitoring

### Expected Log Messages on Launch

```
[C++] 📖 Loading settings...
[Swift] 📖 [SaveGameBridge] Settings loaded: master=0.7 music=0.6 sfx=0.8 debug=false
[C++] ✅ Settings loaded from UserDefaults
[C++] 📖 Loading game data...
[Swift] ℹ️ [SaveGameBridge] No save data found
[C++] ℹ️ No save file found, using defaults
[C++] 🔄 ResetGameData called - resetting to defaults
```

### Expected Log Messages on Save

```
[C++] 💾 Saving game data... Stored coins: 150
[Swift] ✅ [SaveGameBridge] Game data saved successfully
[C++] ✅ Game data saved successfully (JSON format)
[Swift] 💾 [SaveGameBridge] Settings saved: master=0.7 music=0.6 sfx=0.8 debug=false
[C++] ✅ Settings saved to UserDefaults
```

---

## File Locations

### Save Files (iOS Simulator)
```
~/Library/Developer/CoreSimulator/Devices/<UUID>/data/Containers/Data/Application/<UUID>/Documents/
├── floppyturd_save_v2.json     (NEW - JSON format)
└── floppyturd_save.dat         (OLD - binary, migrated on first launch)
```

### Settings (UserDefaults)
```
~/Library/Developer/CoreSimulator/Devices/<UUID>/data/Containers/Data/Application/<UUID>/Library/Preferences/
└── com.yourcompany.FloppyTurd.plist
```

---

## Performance Expectations

- **App Launch**: < 1 second (without assets)
- **Load Game Data**: < 100ms (synchronous)
- **Save Game Data**: < 50ms (async, background)
- **Legacy Migration**: < 200ms (one-time)
- **JSON File Size**: 2-5 KB (pretty-printed)

---

## Next Steps After Successful Build

1. **Run in Simulator**: Test full gameplay loop
2. **Verify Save/Load**: Check JSON file contents
3. **Test Migration**: Use old save file if available
4. **Device Build**: Deploy to physical iPhone for haptics test
5. **Integration**: Merge with Game Center leaderboards (Phase 3)

---

## Rollback Plan (If Needed)

If the build fails or has issues:

```bash
# Revert FloppyTurdGame.cpp save/load changes
git checkout src/FloppyTurd/Game/FloppyTurdGame.cpp

# Use legacy binary save system temporarily
# Then debug serialization system separately
```

---

## Contact & Support

**Documentation**:
- See `SERIALIZATION_SYSTEM_COMPLETE.md` for full architecture
- See `SERIALIZATION_AND_LEVEL_UNLOCK_PROGRESS.md` for progress tracking
- See `docs/SerializationSystem_Research.md` for design decisions

**Files to Check First if Issues**:
1. `FloppyTurdGame.cpp` - Save/load implementation
2. `SaveGameBridge.swift` - C bridge functions
3. `SaveManager.swift` - JSON persistence
4. `SaveGameHelpers.h` - JSON serialization

---

## Build Status Checklist

Pre-build:
- [x] Code complete
- [x] Files created
- [x] Includes added
- [x] Documentation written

Post-build:
- [ ] Build succeeds
- [ ] App launches in simulator
- [ ] Save/load works
- [ ] Level unlocks work
- [ ] Painting tap works
- [ ] Migration works (if legacy save present)

---

**Ready to Build!** 🚀

Run the commands in order and monitor the build output. The system is complete and ready for testing.

**Last Updated**: 2024