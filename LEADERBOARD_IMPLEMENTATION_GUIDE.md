# Leaderboard System Implementation Guide

**Project**: Floppy Turd  
**Feature**: Game Center Leaderboard Integration  
**Date**: 2024  
**Status**: In Progress - Code Infrastructure Complete, Swift/iOS Integration Pending

---

## Table of Contents
1. [Overview](#overview)
2. [What's Been Implemented](#whats-been-implemented)
3. [What Still Needs To Be Done](#what-still-needs-to-be-done)
4. [Architecture](#architecture)
5. [Leaderboard Categories](#leaderboard-categories)
6. [Implementation Steps](#implementation-steps)
7. [Testing Guide](#testing-guide)

---

## Overview

This document outlines the implementation of the leaderboard system for Floppy Turd. The system replaces the "Quit" button in the Main Menu with a "Leaderboard" button that provides access to:

- **5 Level-based leaderboards** (Levels 1-5): Highest pipes cleared in one session
- **1 Boss leaderboard** (Level 6): Fastest completion time
- **3 Global stat leaderboards**: Total enemies defeated, total coins, total pipes

The UI uses the pause menu background mobile overlay on top of the main menu background, with left/right arrows to navigate between different leaderboard pages.

---

## What's Been Implemented

### 1. Core Data Structures ✅

#### GameCenterDelegate (PlatformDelegates.h)
```cpp
struct GameCenterDelegate {
    void (*authenticate)(void (*completion)(bool success));
    bool (*isAuthenticated)();
    void (*submitScore)(const char* leaderboardID, int64_t score, void (*completion)(bool success));
    void (*showLeaderboard)(const char* leaderboardID);
    void (*showAllLeaderboards)();
    const char* (*getPlayerName)();
    const char* (*getPlayerID)();
    void* platformContext;
};
```

Added to `PlatformDelegates` struct as `gameCenter` member.

#### Game Statistics Tracking

**FloppyTurdGame.h Updates:**
- Added `sessionEnemiesKilled` to `GameStats` struct
- Added `bestBossTime` (float) to `LevelStats` struct
- Added methods:
  - `IncrementSessionEnemyKills()`
  - `ResetSessionEnemyKills()`
  - `GetSessionEnemiesKilled()`
  - `UpdateLevelHighScore()` now accepts optional `bossTime` parameter

**FloppyTurdGame.cpp Updates:**
- Implemented enemy kill tracking methods
- Updated `UpdateLevelHighScore()` to save boss completion times

### 2. Serialization Updates ✅

**SaveGameHelpers.h Updates:**
- Added `bestBossTime` serialization in level data
- Added `sessionEnemiesKilled` to statistics serialization
- Updated `parseLevelData()` to include `bestBossTime` parameter
- Updated deserialization to load boss times

### 3. LeaderboardState ✅

**Files Created:**
- `FloppyTurd/src/FloppyTurd/States/LeaderboardState.h`
- `FloppyTurd/src/FloppyTurd/States/LeaderboardState.cpp`

**Features:**
- 9 different leaderboard pages (enum `LeaderboardPage`)
- Navigation with left/right arrows
- Back button to return to main menu
- "View Global Leaderboard" button that calls Game Center
- Displays local high scores/stats
- Format time display for boss level (MM:SS.ms)

**Leaderboard Pages:**
1. Level 1 - Park (pipes)
2. Level 2 - Gold Flush (pipes)
3. Level 3 - Ice Throne (pipes)
4. Level 4 - Sewer (pipes)
5. Level 5 - Sky Castle (pipes)
6. Level 6 - Rat King Boss (time)
7. Total Enemies Defeated
8. Total Coins Collected
9. Total Pipes Cleared

### 4. Main Menu Updates ✅

**MainMenuState.h & .cpp Changes:**
- Replaced `MenuOption::QUIT` with `MenuOption::LEADERBOARD`
- Renamed `m_quitButtonEntity` to `m_leaderboardButtonEntity`
- Renamed `OnQuitButtonPressed()` to `OnLeaderboardButtonPressed()`
- Updated all button arrays and visibility logic
- Changed button text from "QUIT" to "LEADERBOARD"

---

## What Still Needs To Be Done

### 1. Swift-Side Game Center Integration ✅ COMPLETE

**File to Create/Update:** `FloppyTurd/ios/FloppyTurd/GameCenterManager.swift`

```swift
import GameKit

/// Game Center Manager for iOS only
/// Handles authentication, score submission, and leaderboard display
class GameCenterManager: NSObject {
    static let shared = GameCenterManager()
    
    private(set) var isAuthenticated = false
    private weak var viewController: UIViewController?
    
    // Authentication - called automatically on app launch
    func authenticate(completion: @escaping (Bool) -> Void) {
        let localPlayer = GKLocalPlayer.local
        
        localPlayer.authenticateHandler = { [weak self] viewController, error in
            if let vc = viewController {
                // Present auth UI if needed
                self?.viewController?.present(vc, animated: true)
                completion(false)
            } else if localPlayer.isAuthenticated {
                self?.isAuthenticated = true
                print("✅ Game Center authenticated: \(localPlayer.displayName)")
                completion(true)
            } else {
                self?.isAuthenticated = false
                if let error = error {
                    print("❌ Game Center auth error: \(error.localizedDescription)")
                }
                completion(false)
            }
        }
    }
    
    // Submit score via ThreadingProxy for thread safety
    func submitScore(_ score: Int64, leaderboardID: String, completion: @escaping (Bool) -> Void) {
        guard isAuthenticated else {
            print("⚠️ Cannot submit score - not authenticated")
            completion(false)
            return
        }
        
        if #available(iOS 14.0, *) {
            GKLeaderboard.submitScore(Int(score), context: 0, player: GKLocalPlayer.local, 
                                     leaderboardIDs: [leaderboardID]) { error in
                if let error = error {
                    print("❌ Score submission error: \(error.localizedDescription)")
                    completion(false)
                } else {
                    print("✅ Score submitted: \(score) to \(leaderboardID)")
                    completion(true)
                }
            }
        } else {
            // Fallback for iOS 13 and below
            let scoreReporter = GKScore(leaderboardIdentifier: leaderboardID)
            scoreReporter.value = score
            GKScore.report([scoreReporter]) { error in
                completion(error == nil)
            }
        }
    }
    
    // Show leaderboard for specific level/stat
    func showLeaderboard(_ leaderboardID: String) {
        guard let vc = viewController else {
            print("⚠️ No view controller set for Game Center")
            return
        }
        
        let gcVC = GKGameCenterViewController(leaderboardID: leaderboardID, 
                                             playerScope: .global, 
                                             timeScope: .allTime)
        gcVC.gameCenterDelegate = self
        vc.present(gcVC, animated: true)
    }
    
    func setViewController(_ vc: UIViewController) {
        self.viewController = vc
    }
}

extension GameCenterManager: GKGameCenterControllerDelegate {
    func gameCenterViewControllerDidFinish(_ gameCenterViewController: GKGameCenterViewController) {
        gameCenterViewController.dismiss(animated: true)
    }
}
```

### 2. ThreadingProxy Integration 🚧 IN PROGRESS

**File to Update:** `FloppyTurd/src/iOS/Threading/ThreadingProxy.h`

Add Game Center command types:

```cpp
// In ThreadingProxy.h command enum
enum class CommandType {
    // ... existing commands ...
    
    // Game Center commands
    GAME_CENTER_AUTHENTICATE,
    GAME_CENTER_IS_AUTHENTICATED,
    GAME_CENTER_SUBMIT_SCORE,
    GAME_CENTER_SHOW_LEADERBOARD,
};
```

**File to Update:** `FloppyTurd/src/iOS/Threading/ThreadingProxy.cpp`

Add Game Center command handlers in `ProcessCommand()`:

```cpp
case CommandType::GAME_CENTER_AUTHENTICATE:
    // Queue on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        GameCenterManager.shared.authenticate { success in
            // Callback handled by completion
        }
    });
    break;

case CommandType::GAME_CENTER_IS_AUTHENTICATED:
    return GameCenterManager.shared.isAuthenticated;

case CommandType::GAME_CENTER_SUBMIT_SCORE:
    // Extract leaderboardID and score from command data
    dispatch_async(dispatch_get_main_queue(), ^{
        GameCenterManager.shared.submitScore(score, leaderboardID: leaderboardID) { success in
            // Callback handled
        }
    });
    break;

case CommandType::GAME_CENTER_SHOW_LEADERBOARD:
    // Extract leaderboardID from command data
    dispatch_async(dispatch_get_main_queue(), ^{
        GameCenterManager.shared.showLeaderboard(leaderboardID)
    });
    break;
```

### 3. Platform Delegate Setup ⚠️ PRIORITY

**File to Update:** Wherever PlatformDelegates are initialized (likely in iOS platform setup)

Set up Game Center delegates to use ThreadingProxy:

```cpp
// In iOS platform initialization
platformDelegates.gameCenter.authenticate = [](void (*completion)(bool)) {
    ThreadingProxy::QueueCommand(CommandType::GAME_CENTER_AUTHENTICATE, nullptr);
};

platformDelegates.gameCenter.isAuthenticated = []() -> bool {
    return ThreadingProxy::ExecuteSync<bool>(CommandType::GAME_CENTER_IS_AUTHENTICATED);
};

platformDelegates.gameCenter.submitScore = [](const char* leaderboardID, int64_t score, void (*completion)(bool)) {
    // Package data and queue command
    ThreadingProxy::QueueGameCenterScore(leaderboardID, score, completion);
};

platformDelegates.gameCenter.showLeaderboard = [](const char* leaderboardID) {
    ThreadingProxy::QueueCommand(CommandType::GAME_CENTER_SHOW_LEADERBOARD, leaderboardID);
};

platformDelegates.gameCenter.getPlayerName = []() -> const char* {
    if (!GameCenterManager.shared.isAuthenticated) return nullptr;
    static std::string name;
    name = [GKLocalPlayer.local.displayName UTF8String];
    return name.c_str();
};

platformDelegates.gameCenter.getPlayerID = []() -> const char* {
    if (!GameCenterManager.shared.isAuthenticated) return nullptr;
    static std::string playerID;
    playerID = [GKLocalPlayer.local.gamePlayerID UTF8String];
    return playerID.c_str();
};
```

## Platform Differences

### iOS (Game Center Available)
- Native Game Center authentication and leaderboards
- Automatic score submission after level completion
- Shows Apple's native leaderboard UI with global rankings
- Handles offline caching automatically
- Shows friend comparisons if available

### Android (No Game Center)
- LeaderboardState shows local stats only
- No online leaderboards (future: could integrate Google Play Games)
- Still tracks all stats locally
- UI shows "Local Stats" instead of "Global Leaderboard"
- No authentication needed

### Implementation Check
```cpp
#ifdef PLATFORM_IOS
    // Use Game Center delegates
    if (m_platformDelegates->gameCenter.isAuthenticated && 
        m_platformDelegates->gameCenter.isAuthenticated()) {
        // Authenticated - can submit scores
    }
#else
    // Android or desktop - no online leaderboards
    // Just show local stats
#endif
```

### 4. Game State Manager Integration ⚠️ PRIORITY

**File to Update:** `FloppyTurd/src/FloppyTurd/States/MainMenuState.cpp`

Update the leaderboard button handler:

```cpp
void MainMenuState::OnLeaderboardButtonPressed() {
    GN_LOG_INFO("Leaderboard button pressed - opening leaderboards");
    
    #ifdef PLATFORM_IOS
    // On iOS, open Game Center leaderboard directly
    if (m_platformDelegates && m_platformDelegates->gameCenter.showLeaderboard) {
        // Start with first level leaderboard
        m_platformDelegates->gameCenter.showLeaderboard("com.floppyturd.level1");
    } else {
        GN_LOG_WARN("Game Center not available");
    }
    #else
    // On Android or other platforms, create LeaderboardState for local stats view
    auto leaderboardState = std::make_shared<LeaderboardState>(m_ecsCoordinator, m_platformDelegates);
    // TODO: Add to state manager
    #endif
}
```

**Note:** 
- iOS: Shows native Game Center UI immediately (no custom state needed)
- Android: Would show LeaderboardState with local stats only (no online leaderboards)
- LeaderboardState on iOS auto-opens Game Center UI and shows local scores as backup

### 5. Enemy Kill Tracking Integration 🔨 NEEDED

**Files to Update:** 
- `FloppyTurd/src/FloppyTurd/States/GameplayState.cpp`
- Any enemy/collision system files

**Where to Add:**

Find where enemies are defeated (likely in collision detection or enemy health systems) and add:

```cpp
// When any enemy is defeated (not just bosses):
if (m_game) {
    m_game->IncrementSessionEnemyKills();
}
```

**When to Reset:**

In `GameplayState::Enter()` or level start:

```cpp
void GameplayState::Enter() {
    // ... existing code ...
    
    // Reset session enemy kills at start of level
    if (m_game) {
        m_game->ResetSessionEnemyKills();
    }
}
```

### 6. Boss Timer Tracking 🔨 NEEDED

**File to Update:** `FloppyTurd/src/FloppyTurd/States/GameplayState.h` and `.cpp`

**Add to GameplayState.h:**

```cpp
// For Level 6 (Boss) timer
float m_bossLevelStartTime;
bool m_isBossLevel;
```

**Add to GameplayState::Enter():**

```cpp
void GameplayState::Enter() {
    // ... existing code ...
    
    // Track if this is boss level and start timer
    m_isBossLevel = (m_levelId == 6);
    if (m_isBossLevel) {
        m_bossLevelStartTime = 0.0f; // Will track elapsed time
        GN_LOG_INFO("Boss level started - timer initialized");
    }
}
```

**Add to GameplayState::Update():**

```cpp
void GameplayState::Update(float deltaTime) {
    // ... existing code ...
    
    // Track boss level time
    if (m_isBossLevel) {
        m_bossLevelStartTime += deltaTime;
    }
}
```

**Update level completion to pass boss time:**

```cpp
// When level 6 is completed (in boss death or level complete logic):
if (m_isBossLevel && m_game) {
    m_game->UpdateLevelHighScore(m_levelId, m_pipesCleared, sessionCoins, m_bossLevelStartTime);
    GN_LOG_INFO("Boss defeated in " + std::to_string(m_bossLevelStartTime) + " seconds");
}
```

### 7. Score Submission Hooks 🔨 NEEDED

**Where to Submit Scores:**

After level completion or when high score is achieved:

```cpp
// In GameplayState or level complete logic:
void SubmitScoreToGameCenter(int levelId, int score) {
    if (!m_platformDelegates || !m_platformDelegates->gameCenter.submitScore) {
        return;
    }
    
    // Check if authenticated
    if (m_platformDelegates->gameCenter.isAuthenticated && 
        !m_platformDelegates->gameCenter.isAuthenticated()) {
        GN_LOG_INFO("Not authenticated with Game Center - skipping score submission");
        return;
    }
    
    // Build leaderboard ID
    std::string leaderboardID = "com.floppyturd.level" + std::to_string(levelId);
    
    // Submit score
    m_platformDelegates->gameCenter.submitScore(
        leaderboardID.c_str(), 
        static_cast<int64_t>(score),
        [](bool success) {
            if (success) {
                GN_LOG_INFO("✅ Score submitted to Game Center");
            } else {
                GN_LOG_WARN("⚠️ Failed to submit score to Game Center");
            }
        }
    );
}
```

Call this after updating high scores in `UpdateLevelHighScore()`.

### 8. App Store Connect Setup 📱 REQUIRED (BEFORE DEPLOYMENT)

**Steps:**

1. **Login to App Store Connect**
   - Go to https://appstoreconnect.apple.com
   - Select Floppy Turd app

2. **Create Leaderboards**
   - Navigate to: Features → Game Center → Leaderboards
   - Click "+" → "Classic Leaderboard"

3. **Create Each Leaderboard:**

   **Levels 1-5 (Pipes):**
   - Reference ID: `com.floppyturd.level1` (2, 3, 4, 5)
   - Title: "Level 1: Park" (etc.)
   - Score Format: Integer
   - Sort Order: High to Low
   - Score Submission: Best Score
   - Score Range: 0 to 999999

   **Level 6 (Boss Time):**
   - Reference ID: `com.floppyturd.level6`
   - Title: "Level 6: Rat King Boss"
   - Score Format: Elapsed Time (Centiseconds)
   - Sort Order: Low to High (fastest time wins)
   - Score Range: 0 to 99999999

   **Total Stats:**
   - `com.floppyturd.totalenemies` - "Total Enemies Defeated"
   - `com.floppyturd.totalcoins` - "Total Coins Collected"
   - `com.floppyturd.totalpipes` - "Total Pipes Cleared"
   - All: Integer, High to Low, Best Score

4. **Save and Submit for Review**
   - Leaderboards require approval (usually 24-48 hours)

5. **Add Game Center Capability**
   - In Xcode: Target → Signing & Capabilities → "+ Capability" → Game Center

### 9. CMakeLists.txt Update 🔨 NEEDED

**File to Update:** `FloppyTurd/src/FloppyTurd/CMakeLists.txt`

Add the new LeaderboardState files:

```cmake
# In the list of source files, add:
States/LeaderboardState.cpp
States/LeaderboardState.h
```

### 10. Info.plist Update 📱 REQUIRED

**File to Update:** `FloppyTurd/ios/FloppyTurd/Info.plist`

No special keys needed for Game Center, but ensure:
- App has valid Bundle ID
- Game Center capability is enabled in Xcode

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Main Menu State                        │
│  [Play] [Options] [Quick Play] [Leaderboard ← NEW]         │
└────────────────────────┬────────────────────────────────────┘
                         │ Click "Leaderboard"
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                   Leaderboard State                         │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Pause Menu Background Mobile (Overlay)               │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │  [←]        LEVEL 1: PARK              [→]      │  │  │
│  │  │                                                  │  │  │
│  │  │        Your Best: 127 pipes                     │  │  │
│  │  │                                                  │  │  │
│  │  │        [View Global Leaderboard]                │  │  │
│  │  │                                                  │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  │                      [Back]                           │  │
│  └───────────────────────────────────────────────────────┘  │
│  Main Menu Background                                       │
└──────────────────────┬──────────────────────────────────────┘
                       │ Click "View Global Leaderboard"
                       ▼
           ┌──────────────────────────────┐
           │   iOS Game Center View       │
           │   (Native Apple UI)          │
           │   - Global rankings          │
           │   - Friend comparisons       │
           │   - Player profiles          │
           └──────────────────────────────┘
```

---

## Leaderboard Categories

| # | Category | Type | Leaderboard ID | Metric |
|---|----------|------|----------------|--------|
| 1 | Level 1: Park | Level | com.floppyturd.level1 | Pipes (High→Low) |
| 2 | Level 2: Gold Flush | Level | com.floppyturd.level2 | Pipes (High→Low) |
| 3 | Level 3: Ice Throne | Level | com.floppyturd.level3 | Pipes (High→Low) |
| 4 | Level 4: Sewer | Level | com.floppyturd.level4 | Pipes (High→Low) |
| 5 | Level 5: Sky Castle | Level | com.floppyturd.level5 | Pipes (High→Low) |
| 6 | Level 6: Rat King Boss | Boss | com.floppyturd.level6 | Time (Low→High) |
| 7 | Total Enemies | Global | com.floppyturd.totalenemies | Count (High→Low) |
| 8 | Total Coins | Global | com.floppyturd.totalcoins | Count (High→Low) |
| 9 | Total Pipes | Global | com.floppyturd.totalpipes | Count (High→Low) |

---

## Implementation Steps

### Phase 1: Complete C++ Integration ✅ DONE
- [x] Add GameCenterDelegate to PlatformDelegates
- [x] Update GameStats with sessionEnemiesKilled
- [x] Update LevelStats with bestBossTime
- [x] Update serialization
- [x] Create LeaderboardState
- [x] Update MainMenuState (Quit → Leaderboard)

### Phase 2: Swift/iOS Integration 🚧 IN PROGRESS
- [ ] Create GameCenterManager.swift
- [ ] Add Game Center commands to ThreadingProxy
- [ ] Hook up delegates via ThreadingProxy (iOS only)
- [ ] Test authentication flow
- [ ] Ensure Android builds skip Game Center code

### Phase 3: Gameplay Tracking 🔨 TODO
- [ ] Add enemy kill tracking to collision/enemy systems
- [ ] Add boss timer tracking to GameplayState
- [ ] Add score submission hooks
- [ ] Reset session stats on level start

### Phase 4: State Management 🔨 TODO
- [ ] Add LeaderboardState to GameStateManager
- [ ] Handle transitions to/from LeaderboardState
- [ ] Update CMakeLists.txt

### Phase 5: App Store Setup 📱 TODO
- [ ] Create leaderboards in App Store Connect
- [ ] Configure score formats and ranges
- [ ] Submit for review
- [ ] Test with TestFlight

### Phase 6: Testing & Polish 🧪 TODO
- [ ] Test all 9 leaderboard pages
- [ ] Test score submission
- [ ] Test offline behavior
- [ ] Test authentication flow
- [ ] Visual polish (arrows, fonts, spacing)

---

## Testing Guide

### Local Testing (No Game Center)

1. **Test Leaderboard UI:**
   - Launch game
   - Click "Leaderboard" button in main menu
   - Navigate through all 9 pages with arrows
   - Verify local scores display correctly
   - Click "Back" to return to main menu

2. **Test Data Tracking:**
   - Play through levels and verify pipes are tracked
   - Defeat enemies and check session counter
   - Complete boss level and verify timer
   - Check save file for correct data

### Game Center Testing (Sandbox)

1. **Setup Sandbox Account:**
   - Settings → App Store → Sandbox Account
   - Create test account in App Store Connect

2. **Test Authentication:**
   - Launch game
   - Verify Game Center login prompt
   - Authenticate with test account

3. **Test Score Submission:**
   - Complete a level with high score
   - Verify score submission log messages
   - Open Game Center app
   - Verify score appears in leaderboard

4. **Test Leaderboard Display:**
   - Click "View Global Leaderboard"
   - Verify native Game Center view opens
   - Check scores appear correctly
   - Test friend comparisons (if available)

### Edge Cases

- [ ] Test with Game Center disabled
- [ ] Test offline score submission
- [ ] Test rapid score submissions
- [ ] Test timer accuracy for boss level
- [ ] Test enemy kill counter doesn't count bosses twice
- [ ] Test navigation at leaderboard boundaries (first/last page)

**Note:** Android testing will be done after iOS version is completely finished.

---

## Notes

- **iOS Guidelines:** Apps should not have a "Quit" button - this is against Apple's HIG. Replacing with "Leaderboard" is appropriate.
- **Offline Behavior:** Game Center handles offline scores - they'll be submitted when connection is restored.
- **Time Format:** Boss times are stored in seconds (float) but submitted to Game Center as centiseconds (int64).
- **Enemies:** Make sure to differentiate between minion kills and boss kills if needed.
- **Privacy:** Game Center authentication is optional - game should work without it.

---

## File Checklist

### Created ✅
- [x] `FloppyTurd/src/FloppyTurd/States/LeaderboardState.h`
- [x] `FloppyTurd/src/FloppyTurd/States/LeaderboardState.cpp`
- [x] `FloppyTurd/src/iOS/GameCenter/GameCenterManager.swift` ✅

### Modified ✅
- [x] `FloppyTurd/src/FloppyTurd/Game/FloppyTurdGame.h` (added m_isIOSPlatform bool)
- [x] `FloppyTurd/src/FloppyTurd/Game/FloppyTurdGame.cpp` (platform detection)
- [x] `FloppyTurd/src/FloppyTurd/States/MainMenuState.cpp` (removed macros)
- [x] `FloppyTurd/src/Engine/Platform/PlatformDelegates.h`
- [x] `FloppyTurd/src/FloppyTurd/Game/FloppyTurdGame.h`
- [x] `FloppyTurd/src/FloppyTurd/Game/FloppyTurdGame.cpp`
- [x] `FloppyTurd/src/Engine/Platform/SaveGameHelpers.h`
- [x] `FloppyTurd/src/FloppyTurd/States/MainMenuState.h`
- [x] `FloppyTurd/src/FloppyTurd/States/MainMenuState.cpp`

### To Create 🔨
- [x] `FloppyTurd/src/iOS/GameCenter/GameCenterManager.swift` ✅

### To Modify 🔨
- [ ] `FloppyTurd/ios/FloppyTurd/GameBridge.swift` (add bridge functions)
- [ ] `FloppyTurd/ios/FloppyTurd/ViewController.swift` (setup delegates)
- [ ] `FloppyTurd/src/FloppyTurd/States/GameStateManager.cpp` (add state)
- [ ] `FloppyTurd/src/FloppyTurd/States/GameplayState.h` (add timer/tracking)
- [ ] `FloppyTurd/src/FloppyTurd/States/GameplayState.cpp` (implement tracking)
- [ ] `FloppyTurd/src/FloppyTurd/CMakeLists.txt` (add new files)

---

## Next Steps

**Priority 1 (Critical):**
1. Create `GameCenterManager.swift`
2. Add bridge functions to `GameBridge.swift`
3. Hook up delegates in initialization code
4. Add LeaderboardState to CMakeLists.txt and rebuild

**Priority 2 (High):**
1. Add enemy kill tracking to gameplay code
2. Add boss timer to GameplayState
3. Add score submission hooks
4. Update GameStateManager for transitions

**Priority 3 (Before Deployment):**
1. Set up leaderboards in App Store Connect
2. Test with sandbox accounts
3. Visual polish and UX refinements
4. Full QA testing

---

**Document Version:** 1.0  
**Last Updated:** 2024  
**Author:** AI Assistant