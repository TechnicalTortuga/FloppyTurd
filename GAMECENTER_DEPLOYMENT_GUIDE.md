# 🎮 GameCenter Deployment Guide - Floppy Turd

**Date**: November 7, 2025  
**Status**: ✅ Code Complete - Ready for App Store Connect Setup  
**Next Step**: Register Leaderboards in App Store Connect

---

## ✅ What's Already Implemented

### **1. GameCenter Authentication - COMPLETE**
- ✅ **Auto-authentication on app launch** (`GameViewController.swift` lines 93-107)
- ✅ **Graceful failure handling** (user can play without Game Center)
- ✅ **Player info retrieval** (display name, player ID)
- ✅ **Entitlements configured** (`FloppyTurd.entitlements`)

### **2. Score Submission - COMPLETE**
- ✅ **Automatic submission on level completion** (`FloppyTurdGame.cpp`)
- ✅ **Offline caching** (Game Center API handles this automatically)
- ✅ **Error handling and logging**
- ✅ **Score type: Pipes cleared** (primary skill metric)

### **3. Leaderboard Display - COMPLETE**
- ✅ **Native Apple Game Center UI**
- ✅ **Per-level leaderboard access** (`LeaderboardState.cpp`)
- ✅ **"Show All Leaderboards" option**
- ✅ **Authentication state checking before display**

### **4. Platform Integration - COMPLETE**
- ✅ **Swift ↔ C++ bridging** (`ThreadingProxy.cpp`)
- ✅ **Command queue architecture** (thread-safe)
- ✅ **GameCenterManager.swift** (singleton pattern)
- ✅ **Platform delegates** (`PlatformDelegates.h`)

---

## 🚨 CRITICAL FIX APPLIED

### **Leaderboard ID Standardization**
**Problem Found**: Inconsistent leaderboard IDs between submission and display  
**Fix Applied**: Standardized to simple format (`com.floppyturd.level1` instead of `level1.park`)

**Changed File**: `src/FloppyTurd/Game/FloppyTurdGame.cpp` line 1042-1047

---

## 📊 Leaderboard Configuration for App Store Connect

### **REQUIRED: Create These 9 Leaderboards**

Use these EXACT IDs when setting up in App Store Connect:

| # | Leaderboard ID | Display Name | Score Type | Sort | Leaderboard Type |
|---|---|---|---|---|---|
| 1 | `com.floppyturd.park` | Park - Pipes Cleared | Integer | High→Low | Classic |
| 2 | `com.floppyturd.sewer` | Sewer - Pipes Cleared | Integer | High→Low | Classic |
| 3 | `com.floppyturd.desert` | Desert - Pipes Cleared | Integer | High→Low | Classic |
| 4 | `com.floppyturd.snow` | Snow - Pipes Cleared | Integer | High→Low | Classic |
| 5 | `com.floppyturd.castle` | Castle - Pipes Cleared | Integer | High→Low | Classic |
| 6 | `com.floppyturd.ratking.time` | Rat King - Speedrun | Time (Elapsed) | Low→High | Classic |
| 7 | `com.floppyturd.totalenemies` | Total Enemies Killed | Integer | High→Low | Classic |
| 8 | `com.floppyturd.totalcoins` | Total Coins Collected | Integer | High→Low | Classic |
| 9 | `com.floppyturd.totalpipes` | Total Pipes Cleared | Integer | High→Low | Classic |

⚠️ **CRITICAL**: IDs must match EXACTLY (case-sensitive, no typos)

**Note**: 
- Theme-based IDs (park, sewer, desert, etc.) allow for level reordering in future updates without breaking leaderboards!
- **Rat King boss has NO pipes** - only speedrun time leaderboard
- **Classic leaderboards never reset** - permanent high scores for player retention

---

## 🛠️ App Store Connect Setup (Step-by-Step)

### **Step 1: Access Game Center**
1. Go to [https://appstoreconnect.apple.com](https://appstoreconnect.apple.com)
2. Sign in with your Apple Developer account
3. Navigate to: **My Apps** → **Floppy Turd** (or create if not exists)
4. Click **Game Center** tab (left sidebar)

### **Step 2: Enable Game Center**
If this is your first time:
1. Click **Enable Game Center**
2. Confirm Bundle ID: `com.floppyturd.game`
3. Accept Apple's Game Center terms

### **Step 3: Create Each Leaderboard**
For each of the 9 leaderboards above:

1. Click **Leaderboards** → **+ (Add Leaderboard)**
2. Select **Single Leaderboard**
3. Fill in:
   - **Leaderboard Reference Name**: (same as Display Name)
   - **Leaderboard ID**: (from table above - EXACT MATCH)
   - **Score Format Type**: 
     - `Integer` for levels 1-6 and totals
     - `Time (Elapsed)` for boss speedrun
   - **Score Submission Type**: `Best Score`
   - **Sort Order**: 
     - `High to Low` for pipes/enemies/coins
     - `Low to High` for boss time
   - **Score Range Minimum**: `0`
   - **Score Range Maximum**: Leave blank (unlimited)

4. Click **Save**

### **Step 4: Localize Each Leaderboard**
For each leaderboard:
1. Click **Localization** → **+ (Add Language)**
2. Select **English (U.S.)**
3. Enter:
   - **Name**: (Display Name from table)
   - **Score Format**: 
     - `%d pipes` for level leaderboards
     - `%d ms` for boss time
     - `%d enemies` for total enemies
     - `%d coins` for total coins
   - **Score Format (Plural)**: (same as above)
4. Click **Save**

### **Step 5: Add Leaderboard Images (Optional but Recommended)**
For each leaderboard:
1. Create 512x512 PNG image representing the level/stat
2. Upload to **Leaderboard Image** section
3. This appears in Game Center app

### **Step 6: Set as Default Leaderboard Set**
1. Click **Leaderboard Sets** → **Default**
2. Add all 9 leaderboards to the default set
3. This makes them appear in "Show All Leaderboards" view

---

## 📱 Testing on Physical Device

### **Prerequisites**
- ✅ Physical iOS device (iPhone/iPad)
- ✅ Device signed into Game Center (Settings → Game Center)
- ✅ Leaderboards created in App Store Connect (above)
- ✅ App built with correct bundle ID (`com.floppyturd.game`)
- ✅ Provisioning profile with Game Center capability

### **Build & Deploy**
```bash
# Option 1: Build via Xcode
# Open FloppyTurd.xcworkspace → Select device → Build (⌘B) → Run (⌘R)

# Option 2: Build via command line
cd /Users/aimac/Development/FloppyTurd
xcodebuild -workspace FloppyTurd.xcworkspace \
  -scheme FloppyTurd \
  -configuration Debug \
  -destination 'platform=iOS,id=YOUR_DEVICE_UDID' \
  build
```

### **Testing Checklist**

#### **1. Authentication Test**
- [ ] Launch app on device
- [ ] Check Xcode console for logs:
  ```
  🎮 [GameCenter] Starting authentication...
  ✅ [GameCenter] Authentication successful
  👤 [GameCenter] Player: YourName
  ```
- [ ] If prompted, sign into Game Center
- [ ] App should work even if authentication fails

#### **2. Score Submission Test**
- [ ] Play any level (easiest: Level 1)
- [ ] Complete the level (clear at least 1 pipe)
- [ ] Check console for:
  ```
  📊 [GameCenter] Submitting score: X to leaderboard: com.floppyturd.level1
  ✅ [GameCenter] Score submitted successfully
  ```
- [ ] If offline, score should cache and submit when online

#### **3. Leaderboard Display Test**
- [ ] From main menu, tap **Leaderboard** button
- [ ] Should show native Game Center UI
- [ ] Navigate between level pages
- [ ] Verify your submitted score appears
- [ ] Check "Friends" vs "Global" tabs work

#### **4. Boss Speedrun Test**
- [ ] Complete Level 6 (Boss)
- [ ] Check console for TWO submissions:
  ```
  📊 [GameCenter] Submitting score: X to leaderboard: com.floppyturd.level6
  📊 [GameCenter] Submitting score: Y to leaderboard: com.floppyturd.level6.boss.time
  ```
- [ ] Verify both scores appear in Game Center

#### **5. Multi-Account Test**
- [ ] Sign out of Game Center (Settings app)
- [ ] Sign in with different Apple ID
- [ ] Launch app, play level
- [ ] Verify scores submit to different account
- [ ] Check leaderboards show different player names

---

## 🐛 Troubleshooting

### **"Authentication failed - player not authenticated"**
**Cause**: Device not signed into Game Center  
**Fix**: Settings → Game Center → Sign In

### **"Cannot show leaderboard - no view controller set"**
**Cause**: View controller not passed to GameCenterManager  
**Fix**: Already handled in `GameViewController.swift` line 95

### **Scores submit but don't appear in leaderboard**
**Cause**: Leaderboard ID mismatch between code and App Store Connect  
**Fix**: Double-check IDs match exactly (case-sensitive)

### **Leaderboards not visible in Game Center app**
**Cause**: Leaderboards not published yet  
**Fix**: In App Store Connect, ensure leaderboards are:
1. Added to Default Leaderboard Set
2. Localized for at least one language
3. Saved (not in draft state)

### **Sandbox vs Production Leaderboards**
- **Development**: Uses Sandbox Game Center (separate leaderboards)
- **TestFlight/Production**: Uses Production Game Center
- Scores do NOT transfer between sandbox and production

---

## 📈 Post-Launch Monitoring

### **Analytics to Track**
- **Authentication Rate**: % of players who successfully authenticate
- **Score Submission Rate**: % of completed levels that submit scores
- **Leaderboard View Rate**: % of players who view leaderboards
- **Return Rate**: Do leaderboards increase player retention?

### **Where to View Stats**
1. **App Store Connect** → **App Analytics** → **Game Center**
2. View:
   - Leaderboard participation
   - Player engagement
   - Score distribution

### **Common Issues to Watch**
- High authentication failure rate → investigate entitlements/provisioning
- Low leaderboard views → consider adding UI prompts
- Score spam/cheating → implement server-side validation (future)

---

## 🎯 What's Next After Game Center

Based on `DEPLOYMENT_PREPARATION.md` Phase 7-9:

### **Phase 7.1: In-App Purchase (IAP) - Remove Ads**
- [ ] Create `FloppyTurd.storekit` configuration file
- [ ] Define product: `com.floppyturd.game.removeads` ($2.00 USD)
- [ ] Implement `StoreManager.swift` (similar to `GameCenterManager.swift`)
- [ ] Wire purchase → `AdManager.setAdsEnabled(false)`
- [ ] Add restore purchases functionality
- [ ] Test with Sandbox accounts

### **Phase 8: Pre-Deployment Checklist**
- [ ] Verify all features work on physical device
- [ ] Test fresh install (no save data)
- [ ] Test app update scenario (preserve save data)
- [ ] Verify Bundle ID: `com.floppyturd.game` everywhere
- [ ] Update version to 1.0.0
- [ ] Disable debug logging
- [ ] Configure Release build settings
- [ ] Generate provisioning profiles

### **Phase 9: App Center Beta Testing**
- [ ] Upload build to App Center
- [ ] Distribute to beta testers (5-10 people)
- [ ] Monitor crash reports
- [ ] Collect feedback on Game Center experience
- [ ] Test leaderboards with real users
- [ ] Verify ad revenue tracking
- [ ] Test IAP purchases in production sandbox

### **Phase 10: App Store Submission**
- [ ] Prepare screenshots (include leaderboard UI)
- [ ] Write app description (mention leaderboards)
- [ ] Submit for review
- [ ] Monitor review status
- [ ] Launch! 🚀

---

## 📝 Quick Reference

### **Bundle ID**
```
com.floppyturd.game
```

### **Leaderboard ID Format**
```
com.floppyturd.level{N}              // Levels 1-6
com.floppyturd.level6.boss.time      // Boss speedrun
com.floppyturd.total{stat}           // Cumulative stats
```

### **Files Modified Today**
- ✅ `src/FloppyTurd/Game/FloppyTurdGame.cpp` - Fixed leaderboard IDs

### **Files Already Complete**
- ✅ `src/iOS/GameCenter/GameCenterManager.swift`
- ✅ `src/iOS/GameViewController.swift`
- ✅ `src/iOS/Threading/ThreadingProxy.cpp`
- ✅ `src/iOS/Threading/ThreadingSystem.swift`
- ✅ `src/FloppyTurd/States/LeaderboardState.cpp`
- ✅ `src/Engine/Platform/PlatformDelegates.h`
- ✅ `FloppyTurd.entitlements`
- ✅ `Info.plist`

### **Console Log Keywords to Watch**
```
🎮 [GameCenter] Starting authentication
✅ [GameCenter] Authentication successful
📊 [GameCenter] Submitting score
📱 [GameCenter] Showing leaderboard
⚠️ [GameCenter] Cannot X - not authenticated
❌ [GameCenter] Failed to X
```

---

## ✨ Summary

**You are 95% ready for Game Center deployment!**

**What you have:**
- ✅ Complete authentication system
- ✅ Automatic score submission
- ✅ Native leaderboard UI
- ✅ Proper error handling
- ✅ Thread-safe Swift↔C++ bridge

**What you need to do:**
1. ⏰ **NOW**: Create 9 leaderboards in App Store Connect (30 min)
2. ⏰ **TODAY**: Test on physical device (30 min)
3. ⏰ **THIS WEEK**: Implement IAP for "Remove Ads" (Phase 7.1)
4. ⏰ **NEXT WEEK**: Beta testing via App Center (Phase 9)
5. ⏰ **WEEK 3**: App Store submission! 🚀

**Estimated time to App Store submission:** 2-3 weeks

Good luck! Let me know if you need help with any of these steps! 💩🎮
