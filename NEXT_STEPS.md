# Next Steps - Game Center Implementation

**Current Status**: ✅ C++ side complete and building successfully  
**What's Left**: Swift-side command processing and UI integration

---

## Immediate Next Steps

### Step 1: Process GameCenter Commands in Swift ⏳

**File**: `FloppyTurd/src/iOS/Threading/ThreadingSystem.swift`

**Add this method to CommandProcessor class**:

```swift
private func processGameCenterCommands() {
    let commands = GameCorePlatform.GameCore.getAndClearGameCenterCommandsFromProxy()
    
    for command in commands {
        switch command.type {
        case .CMD_GAME_CENTER_AUTHENTICATE:
            Task { @MainActor in
                GameCenterManager.shared.authenticate { success, error in
                    if success {
                        print("✅ Game Center authenticated")
                    } else {
                        print("❌ Game Center auth failed: \(error?.localizedDescription ?? "Unknown")")
                    }
                }
            }
            
        case .CMD_GAME_CENTER_SUBMIT_SCORE:
            Task { @MainActor in
                let leaderboardID = String(cString: command.data.leaderboardID)
                GameCenterManager.shared.submitScore(
                    command.data.score,
                    leaderboardID: leaderboardID
                ) { success, error in
                    if success {
                        print("✅ Score submitted: \(command.data.score) to \(leaderboardID)")
                    } else {
                        print("❌ Score submission failed: \(error?.localizedDescription ?? "Unknown")")
                    }
                }
            }
            
        case .CMD_GAME_CENTER_SHOW_LEADERBOARD:
            Task { @MainActor in
                let leaderboardID = String(cString: command.data.leaderboardID)
                GameCenterManager.shared.showLeaderboard(leaderboardID)
            }
            
        case .CMD_GAME_CENTER_SHOW_ALL_LEADERBOARDS:
            Task { @MainActor in
                GameCenterManager.shared.showAllLeaderboards()
            }
            
        default:
            break
        }
    }
}
```

**Add this call to processCommands() method**:

```swift
func processCommands() {
    processRenderCommands()
    processAudioCommands()
    processLogCommands()
    processAssetCommands()
    processHapticCommands()
    processSaveCommands()
    processGameCenterCommands()  // <-- ADD THIS LINE
}
```

---

### Step 2: Set Up View Controller Reference

**File**: `FloppyTurd/src/iOS/GameViewController.swift`

**Add to viewDidLoad()**:

```swift
override func viewDidLoad() {
    super.viewDidLoad()
    
    // ... existing setup code ...
    
    // Set Game Center view controller reference
    GameCenterManager.shared.setViewController(self)
    
    // Optional: Auto-authenticate on launch
    GameCenterManager.shared.authenticate { success, error in
        if success {
            print("✅ Game Center ready")
        } else if let error = error {
            print("⚠️ Game Center authentication failed: \(error.localizedDescription)")
        }
    }
}
```

---

### Step 3: Test Basic Functionality

**Build and Run**:
```bash
cd /Users/aimac/Development/FloppyTurd/build_ios
xcodebuild -project FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" \
  build
```

**What to Check**:
1. Game launches without crashing
2. Console shows "✅ Game Center ready" (if authenticated)
3. No errors about missing GameCenter methods

---

### Step 4: Connect LeaderboardState to Game Center

**File**: `FloppyTurd/src/FloppyTurd/States/LeaderboardState.cpp`

**In Enter() method, add**:

```cpp
void LeaderboardState::Enter() {
    // ... existing code ...
    
    // On iOS, show Game Center leaderboard if authenticated
    #ifdef PLATFORM_IOS
    if (m_platformDelegates->gameCenter.isAuthenticated()) {
        // Show the current leaderboard
        const char* leaderboardID = GetLeaderboardID(m_currentPage);
        m_platformDelegates->gameCenter.showLeaderboard(leaderboardID);
    }
    #endif
}
```

**Add helper method**:

```cpp
const char* LeaderboardState::GetLeaderboardID(int page) {
    switch (page) {
        case 0: return "com.floppyturd.level1";
        case 1: return "com.floppyturd.level2";
        case 2: return "com.floppyturd.level3";
        case 3: return "com.floppyturd.level4";
        case 4: return "com.floppyturd.level5";
        case 5: return "com.floppyturd.level6";
        case 6: return "com.floppyturd.totalenemies";
        case 7: return "com.floppyturd.totalcoins";
        case 8: return "com.floppyturd.totalpipes";
        default: return "com.floppyturd.level1";
    }
}
```

---

### Step 5: Submit Scores After Level Completion

**File**: `FloppyTurd/src/FloppyTurd/States/GameplayState.cpp`

**When level is completed**:

```cpp
void GameplayState::OnLevelComplete(int score, int coins) {
    // ... existing code to save score ...
    
    // Submit to Game Center on iOS
    if (m_game->IsIOSPlatform()) {
        if (m_platformDelegates->gameCenter.isAuthenticated()) {
            const char* leaderboardID = GetLeaderboardIDForLevel(m_levelId);
            int64_t scoreToSubmit = static_cast<int64_t>(score);
            
            // For boss level, submit time in centiseconds
            if (m_levelId == 6) {
                scoreToSubmit = static_cast<int64_t>(m_bossTime * 100.0f);
            }
            
            m_platformDelegates->gameCenter.submitScore(
                leaderboardID, 
                scoreToSubmit,
                nullptr  // No completion callback needed
            );
        }
    }
}
```

---

### Step 6: Boss Timer Implementation (Level 6 Only)

**File**: `FloppyTurd/src/FloppyTurd/States/GameplayState.h`

**Add member**:
```cpp
float m_bossLevelStartTime;  // Timer for boss level completion
```

**File**: `FloppyTurd/src/FloppyTurd/States/GameplayState.cpp`

**In Enter() for level 6**:
```cpp
if (m_levelId == 6) {
    m_bossLevelStartTime = 0.0f;
}
```

**In Update() for level 6**:
```cpp
if (m_levelId == 6 && !m_levelComplete) {
    m_bossLevelStartTime += deltaTime;
}
```

**When boss dies**:
```cpp
// Update high score with boss time
m_game->UpdateLevelHighScore(6, score, coins, m_bossLevelStartTime);
```

---

## Testing Checklist

### Local Testing (No Game Center)
- [ ] Build succeeds
- [ ] Game launches
- [ ] Leaderboard button appears in main menu
- [ ] Clicking leaderboard shows local stats
- [ ] Navigation between pages works
- [ ] Back button returns to main menu

### iOS Sandbox Testing
- [ ] Game Center authentication prompt appears
- [ ] Authentication succeeds
- [ ] Native leaderboard UI opens when clicking leaderboard button
- [ ] Score submission works (check console logs)
- [ ] Leaderboard shows submitted scores
- [ ] Offline scores queue and submit later

---

## App Store Connect Setup (Before TestFlight)

1. Log into App Store Connect
2. Go to your app → Features → Game Center
3. Create 9 leaderboards with these IDs:

```
com.floppyturd.level1        - "A Flop in the Park" (Integer, High to Low)
com.floppyturd.level2        - "Home Sweet Home" (Integer, High to Low)
com.floppyturd.level3        - "The Good, The Bad, and the Stinky" (Integer, High to Low)
com.floppyturd.level4        - "Polar Pandemonium" (Integer, High to Low)
com.floppyturd.level5        - "Dung in the Dungeon" (Integer, High to Low)
com.floppyturd.level6        - "Curtains for Crap" (Elapsed Time, Low to High, format: centiseconds)
com.floppyturd.totalenemies  - "Total Enemies Defeated" (Integer, High to Low)
com.floppyturd.totalcoins    - "Total Coins Collected" (Integer, High to Low)
com.floppyturd.totalpipes    - "Total Pipes Cleared" (Integer, High to Low)
```

4. Enable Game Center capability in Xcode project settings
5. Submit for review (leaderboards need approval)

---

## Common Issues & Solutions

### Issue: "Game Center not available"
**Solution**: Make sure you're signed into iCloud on the simulator/device

### Issue: Leaderboard shows empty
**Solution**: Leaderboards need to be approved by Apple first (use TestFlight sandbox)

### Issue: Scores not submitting
**Solution**: Check console logs for error messages, verify leaderboard IDs match App Store Connect

### Issue: Authentication fails
**Solution**: Sign out and back into iCloud, restart simulator

---

## Build Commands Reference

**Clean rebuild**:
```bash
cd /Users/aimac/Development/FloppyTurd
rm -rf build_ios && mkdir build_ios && cd build_ios
cmake -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=../ios-cmake-master/ios.toolchain.cmake \
  -DPLATFORM=SIMULATOR64 \
  -DIOS_PLATFORM=SIMULATOR \
  -DIOS_ARCH=x86_64 \
  -DCMAKE_BUILD_TYPE=Debug ..

xcodebuild -project build_ios/FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" \
  clean build
```

**Incremental build**:
```bash
cd /Users/aimac/Development/FloppyTurd/build_ios
xcodebuild -project FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" \
  build
```

---

## What's Already Done ✅

- ✅ Platform detection (bool instead of macros)
- ✅ GameCenterManager.swift created
- ✅ Command queue system implemented
- ✅ ThreadingProxy delegates wired up
- ✅ Build successful on iOS Simulator
- ✅ All C++ infrastructure in place

## What's Left ⏳

- ⏳ Process GameCenter commands in Swift (Step 1)
- ⏳ Set view controller reference (Step 2)
- ⏳ Connect LeaderboardState (Step 4)
- ⏳ Submit scores after level completion (Step 5)
- ⏳ Boss timer implementation (Step 6)
- ⏳ Create leaderboards in App Store Connect
- ⏳ Test with TestFlight

---

**You're 80% done!** The hard part (C++ infrastructure) is complete. Now just wire up the Swift side and test! 🚀💩