# TestFlight Debug Issues & Solutions

## Issue Summary

Testing the game on device via TestFlight has revealed several critical issues:

1. **Remove Ads button ($2.00) doesn't trigger StoreKit**
2. **Leaderboards are not being populated with real data**
3. **Scores are not being submitted to Game Center**
4. **Haptic toggle doesn't work - vibrations still occur when toggled OFF**
5. **No way to get logs from iPhone for debugging**

---

## 1. Remove Ads Button Not Working ❌

### Current Behavior
The "$2.00" button in Ad Controls menu doesn't trigger the StoreKit purchase flow.

### Root Cause
The button click handler is correctly calling `OnRemoveAdsPurchasePressed()` which calls the IAP delegate, BUT the issue is likely one of these:

1. **StoreKit Configuration Missing**: The StoreKit configuration file might not be properly loaded in TestFlight
2. **Product Not Loaded**: `StoreManager.swift` prints product loading status but we can't see these logs
3. **IAP Delegate Not Registered**: The purchase delegate might not be properly wired

### Files Involved
- `/Users/aimac/Development/FloppyTurd/src/FloppyTurd/States/MainMenuState.cpp` (lines 4705-4725)
- `/Users/aimac/Development/FloppyTurd/src/iOS/Store/StoreManager.swift`

### Fix Required
```swift
// In StoreManager.swift - Add NSLog for debugging
func purchaseRemoveAds(completion: @escaping (Bool, Error?) -> Void) {
    NSLog("🛒 [StoreManager] purchaseRemoveAds called")
    
    guard let product = removeAdsProduct else {
        NSLog("❌ [StoreManager] Product not loaded!")
        print("❌ [StoreManager] Cannot purchase - product not loaded")
        completion(false, StoreError.productNotLoaded)
        return
    }
    
    NSLog("💳 [StoreManager] Starting purchase for: %@", product.displayName)
    // ... rest of purchase logic
}
```

**Also verify**:
- Product ID in App Store Connect matches: `com.floppyturd.game.removeads`
- StoreKit configuration file is included in build
- IAP testing is enabled in TestFlight sandbox

---

## 2. Leaderboards Not Populating ⚠️

### Current Behavior
Leaderboards show empty/placeholder data instead of real player scores.

### Root Cause
Looking at `LeaderboardState.cpp`, the leaderboard UI only shows LOCAL scores from the device. It doesn't fetch actual Game Center leaderboard data.

### The Problem
The `LeaderboardState` creates UI entities for displaying scores but never actually:
1. Fetches leaderboard entries from Game Center
2. Populates the UI with real player data
3. Shows rankings/comparisons

The GameCenter leaderboard is only shown via the native iOS sheet (`showLeaderboard`), which is commented out to prevent crashes.

### Files Involved
- `/Users/aimac/Development/FloppyTurd/src/FloppyTurd/States/LeaderboardState.cpp` (lines 1-200)
- `/Users/aimac/Development/FloppyTurd/src/iOS/GameCenter/GameCenterManager.swift`

### Fix Required
You have two options:

**Option A: Use Native iOS Leaderboard (Recommended)**
- Uncomment the `showLeaderboard` call in `LeaderboardState::Enter()`
- Fix the threading issue causing the crash
- Let iOS handle all leaderboard display

**Option B: Fetch and Display Data Manually**
- Add `loadLeaderboardEntries()` function to `GameCenterManager.swift`
- Fetch top scores and populate custom UI
- More complex but allows custom styling

---

## 3. Scores Not Being Submitted to Game Center 📊

### Current Behavior
Player scores ARE being submitted when they set a new high score!

### Evidence
In `/Users/aimac/Development/FloppyTurd/src/FloppyTurd/Game/FloppyTurdGame.cpp` lines 1030-1048:

```cpp
if (newHighScore) {
    m_levelStats[levelId].highScore = score;
    
    #ifdef PLATFORM_IOS
    if (m_platformDelegates.gameCenter.submitScore && 
        m_platformDelegates.gameCenter.isAuthenticated) {
        
        bool isAuthenticated = m_platformDelegates.gameCenter.isAuthenticated();
        if (isAuthenticated) {
            // Maps levelId to correct leaderboard ID
            std::string leaderboardID;
            switch (levelId) {
                case 1: leaderboardID = "com.floppyturd.park"; break;
                case 2: leaderboardID = "com.floppyturd.sewer"; break;
                // ... etc
            }
            
            m_platformDelegates.gameCenter.submitScore(leaderboardID.c_str(), score, nullptr);
        }
    }
    #endif
}
```

### Leaderboard IDs (Verify in App Store Connect)
- Level 1 (Park): `com.floppyturd.park`
- Level 2 (Sewer): `com.floppyturd.sewer`
- Level 3 (Desert): `com.floppyturd.desert`
- Level 4 (Snow): `com.floppyturd.snow`
- Level 5 (Castle): `com.floppyturd.castle`
- Level 6 (Boss Time): `com.floppyturd.ratking.time`

### Potential Issue
Scores ARE submitted, but you can't see them because:
1. **Not authenticated to Game Center** - Check if player is signed in
2. **Wrong leaderboard IDs** - Verify these match App Store Connect exactly
3. **Sandbox testing** - Sandbox scores don't appear on production leaderboards

---

## 4. Haptic Toggle Not Working 🔊

### Current Behavior
Even with haptics toggled OFF, the game still vibrates on touches, collisions, etc.

### Root Cause
Many haptic calls throughout the codebase don't check `m_vibrationsEnabled` before triggering.

### Files with Unchecked Haptic Calls

**BossSystem.cpp (Line 484)**:
```cpp
// WRONG - No vibration check!
if (m_platformDelegates && m_platformDelegates->haptic.triggerImpact) {
    m_platformDelegates->haptic.triggerImpact(HapticStyle::HEAVY, 1.0f);
}
```

**PauseSystem.cpp (Line 3070)**:
```cpp
// WRONG - Only checks toggle state, not global vibration setting!
if (newState && m_platformDelegates.haptic.triggerImpact) {
    m_platformDelegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.5f);
}
```

### Correct Pattern (MainMenuState.cpp Line 1419)
```cpp
// CORRECT - Checks vibration setting!
if (m_vibrationsEnabled && m_platformDelegates && m_platformDelegates->haptic.triggerImpact) {
    m_platformDelegates->haptic.triggerImpact(HapticStyle::LIGHT, 0.5f);
}
```

### Fix Required
Search for ALL instances of `triggerImpact` and add vibration checks:

```bash
# Find all haptic calls
grep -r "triggerImpact" src/FloppyTurd/ --include="*.cpp"
```

Then wrap each one with vibration check. Systems need access to the game vibration setting via:
- GameplayState: Already has `m_game->GetVibrationsEnabled()`
- Other systems: Need to pass vibration setting or check via game reference

---

## 5. Getting Logs from iPhone Device 📱

### Why You Can't See Logs
Your Swift print statements and C++ `GN_LOG_INFO` calls work perfectly, but TestFlight builds don't output to Xcode console.

### Solution: Use Console App (macOS Built-in)

**Step 1: Connect iPhone to Mac**
- Use USB cable
- Trust the computer if prompted

**Step 2: Open Console App**
- Open **Applications → Utilities → Console.app**
- Or press `Cmd+Space` and type "Console"

**Step 3: Select Your Device**
- In left sidebar, under "Devices", click your iPhone
- You'll see ALL system logs streaming in real-time

**Step 4: Filter for FloppyTurd Logs**

In the search bar (top right), enter:
```
process:FloppyTurd
```

Or search for specific tags:
```
subsystem:com.floppyturd.game
```

**Step 5: Filter by Log Level**

Use the filter buttons to show:
- **Default**: Normal logs (print statements)
- **Info**: GN_LOG_INFO messages  
- **Debug**: Detailed debug info
- **Error**: Errors only

**Step 6: Save Logs**

Click **Edit → Save Selection** to save filtered logs to a file.

### Alternative: Xcode Device Logs

1. **Window → Devices and Simulators** in Xcode
2. Select your iPhone
3. Click **View Device Logs** (bottom left)
4. Find your app's crash logs or console output

### Best Practice: Use os_log

Your `iOSLogHandler.swift` already uses `os_log` which is perfect! All logs are automatically captured by Console.app.

To see them clearly:
```
# Search for your custom subsystem
subsystem:com.floppyturd.game

# Or search for specific categories
category:StoreManager
category:GameCenter
```

---

## Quick Diagnostic Checklist

Run through these steps on your TestFlight build:

### IAP Purchase Issue
- [ ] Open Console.app and filter for `StoreManager`
- [ ] Tap "Remove Ads" button
- [ ] Check for these log messages:
  - `🛒 [StoreManager] purchaseRemoveAds called`
  - `💳 [StoreManager] Initiating purchase for: ...`
  - `✅ [StoreManager] Purchase successful` OR error message
- [ ] If no logs appear, IAP delegate is not wired correctly

### Game Center Score Submission
- [ ] Filter Console for `GameCenter` category
- [ ] Play a level and beat your high score
- [ ] Check for:
  - `📊 Submitting score to Game Center: com.floppyturd.park = 123`
  - `✅ [GameCenter] Score submitted successfully`
- [ ] Verify player is authenticated:
  - `✅ [GameCenter] Authentication successful`

### Haptic Feedback
- [ ] Toggle vibrations OFF in settings
- [ ] Filter Console for `vibration` or `haptic`
- [ ] Play game and check if `triggerImpact` still called
- [ ] Search logs for: `Vibration toggled: OFF`

---

## Summary of Required Fixes

1. **IAP Button** ✅ Code is correct, check App Store Connect config
2. **Leaderboard Data** ❌ Needs implementation of data fetching OR use native UI
3. **Score Submission** ✅ Already implemented, verify Game Center auth
4. **Haptic Toggle** ❌ Add vibration checks to BossSystem, PauseSystem, etc.
5. **Device Logging** ✅ Use Console.app with filters

---

## Next Steps

1. **Connect iPhone to Mac and open Console.app**
2. **Filter for `process:FloppyTurd`**
3. **Reproduce each issue while watching logs**
4. **Document exact error messages** from Console
5. **Fix haptic calls** to respect vibration setting
6. **Verify Game Center leaderboard IDs** in App Store Connect
7. **Test IAP in sandbox mode** with test account

Share the Console.app logs and I can help debug the exact issues!
