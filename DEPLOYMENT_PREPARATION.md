# FloppyTurd - Deployment Preparation Guide

## Current Version: 1.0.0
## Timeline: Phase 4 - Ad System Complete, UI/IAP Features Next

---

## Phase 1: Gameplay & Functionality Fixes (PRIORITY) ✅ COMPLETE

### 1.1 Audio Settings Persistence ✅
- [x] Save audio volume settings to persistent storage
- [x] Restore audio settings on game launch
- [x] Test audio persistence across app restarts

### 1.2 Difficulty Settings Persistence ✅
- [x] Save difficulty selection to persistent storage
- [x] Restore difficulty setting on game launch
- [x] Test difficulty persistence across app restarts

### 1.3 Difficulty Enum Cleanup ✅
- [x] **CRITICAL:** Fix difficulty enum glitch (briefly switching to normal/easy/hard)
- [x] Ensure all difficulty references use correct enums: `Runny`, `Regular`, `Rough`
- [x] Search codebase for any hardcoded "normal", "easy", "hard" strings
- [x] Replace all old enum references with established enums
- [x] Test difficulty selection stability across all menus

### 1.4 Pause Menu Audio Track Handling ✅
- [x] Implement smooth audio scrubbing in pause menu (match main menu behavior)
- [x] Fix "sticky" audio track behavior in pause menu
- [x] Enable click-to-position functionality on audio track
- [x] Ensure pause menu audio controls match main menu options logic
- [x] Test audio track interaction in both menus for parity

### 1.5 Visual Polish & Rendering Issues ✅
- [x] **Remove debug rectangles from main menu** (leftover from early development)
- [x] **Fix state transition rendering glitches:**
  - [x] Ensure all elements load before presenting any state
  - [x] Fix "domino effect" where background appears before UI elements
  - [x] Prevent frame flashing during pause menu open
  - [x] Prevent frame flashing during game over state
  - [x] Prevent frame flashing during gameplay state entry
  - [x] Implement proper state pre-loading/buffering
  - [x] Ensure atomic presentation of all state elements
- [x] Test all state transitions for visual smoothness
- [x] Verify no debug visuals remain in any state

---

## Phase 2: Economy & Unlocks Configuration ✅ COMPLETE

### 2.1 Skill Pricing ✅
Update skill prices to:
- [x] **Half Hearts:** 200 coins ✅
- [x] **Third Hearts:** 400 coins ✅
- [x] **Coin Magnet:** 200 coins ✅
- [x] **Heart Magnet:** 200 coins ✅
- [x] **Coin Safety Net:** 300 coins ✅

### 2.2 Hat System Configuration ✅
- [x] **Lock ALL hats by default** (no hats unlocked at start) ✅
- [x] **First 6 hats:** 50 coins each ✅
- [x] **Next 6 hats:** 150 coins each ✅
- [x] **Last 3 hats:** 250 coins each ✅
  - [x] Reorder hats: Gold Crown, Pharaoh Hat, Poop Hat should be the LAST 3 ✅
  - [x] Move these from their current middle positions to the end ✅
- [x] **FIX:** Hat locked icon must disappear immediately upon purchase ✅

---

## Phase 3: Project Structure Consolidation ✅ COMPLETE

### 3.1 Module Consolidation ✅
- [x] Consolidate separate FloppyTurdGame Library into single module ✅
- [x] Remove FloppyTurdGame Library target ✅
- [x] Update all imports and references ✅
- [x] Verify all code compiles with consolidated module ✅
- [x] Update build configuration ✅
- [ ] Test build on device

### 3.2 Bundle ID Standardization ✅
- [x] **Search entire codebase for bundle ID references** ✅
- [x] Replace ALL instances of `com.floppyturd.engine` with `com.floppyturd.game` ✅
- [x] Update Info.plist ✅ (already had correct bundle ID)
- [x] Update project settings ✅ (CMakeLists.txt updated)
- [x] Update capabilities/entitlements ✅ (no bundle ID references)
- [ ] Update provisioning profiles (requires Xcode build)
- [ ] Update Game Center configuration (if bundle-specific)
- [x] Verify no mixed bundle IDs remain ✅

---

## Phase 4: Memory & Performance Audit

### 4.1 Memory Access Review
- [ ] Audit codebase for unsafe memory accesses
- [ ] Check for retain cycles
- [ ] Review texture/sprite loading and disposal
- [ ] Check for memory leaks in state transitions
- [ ] Profile memory usage during gameplay
- [ ] Check audio resource management
- [ ] Review particle system cleanup
- [ ] Verify proper deallocation in scene transitions

### 4.2 Performance Optimization
- [ ] Profile frame rate during boss fights
- [ ] Check for stuttering during level transitions
- [ ] Verify smooth performance on target devices
- [ ] Test on older hardware if applicable

---

## Phase 5: AdMob Integration ✅ COMPLETE

### 5.1 Ad System Architecture ✅
- [x] Platform delegates for ad commands ✅
- [x] ThreadingProxy extensions for ad lifecycle ✅
- [x] Swift AdManager with Google Mobile Ads SDK ✅
- [x] Ad-ready state synchronization (C++ ↔ Swift) ✅
- [x] Command processing in ThreadingSystem ✅
- [x] C++ AdSystem for game logic ✅

### 5.2 Ad Presentation & Flow ✅
- [x] Interstitial ads on death (configurable frequency) ✅
- [x] Ad shows after game over UI is visible ✅
- [x] Game pause/resume around ad presentation ✅
- [x] Proper state synchronization (GameViewController) ✅
- [x] Modal lifecycle handling (prevent shutdown during ad) ✅
- [x] Ad preloading on dismiss ✅

### 5.3 Death Physics & Anti-Cheese ✅
- [x] Increased death gravity (GRAVITY_DEATH = 29400) ✅
- [x] Separate terminal velocity for death (5000) ✅
- [x] Rapid fall to game over screen ✅
- [x] Anti-cheese: No pipe clear during invulnerability ✅

### 5.4 Ad Configuration ✅
- [x] Test ad unit IDs for development ✅
- [x] Production ad unit IDs configured ✅
- [x] Ad frequency: Every 5 deaths after learning period ✅
- [x] Info.plist GADApplicationIdentifier ✅
- [x] CMakeLists.txt XCFramework integration ✅

### 5.5 Testing & Validation ✅
- [x] Ads load and show correctly ✅
- [x] Ads present at correct timing ✅
- [x] Game pauses during ad ✅
- [x] Game resumes after ad dismissal ✅
- [x] No game shutdown during modal ad ✅
- [x] Death counter and frequency logic ✅
- [x] Anti-cheese pipe clearing ✅

### 5.6 Documentation ✅
- [x] AdMob_Integration_Complete.md ✅
- [x] AdMob_Debug_Fix.md ✅
- [x] AdMob_Final_Fixes.md ✅
- [x] AdMob_Resume_Fix.md ✅
- [x] Swift_Cpp_Native_Interop_Guide.md ✅

**Status**: Ad system fully functional. Ready for IAP "Remove Ads" integration in Phase 6.

---

## Phase 6: UI Enhancements & IAP Preparation (IN PROGRESS)

### 6.1 New Asset Integration
- [ ] Add `adcontrolsbutton.png` to main menu (bottom left)
  - [ ] Scale 6x
  - [ ] Position bottom left corner of main menu
  - [ ] Add input handler to open Ad Controls menu
- [ ] Add `xbuttonselected.png` to options menu
  - [ ] Vibration toggle ON state
- [ ] Add `xbuttonunselected.png` to options menu
  - [ ] Vibration toggle OFF state
- [ ] Verify all new assets load correctly
- [ ] Test asset scaling on different screen sizes

### 6.2 Ad Controls Menu Implementation
- [ ] Create new menu state/mode for Ad Controls
- [ ] Layout similar to Leaderboard menu structure
- [ ] Add "Remove Ads" label/title
- [ ] Create "$2.00" purchase button (bottom right)
  - [ ] Position opposite of ad controls button
  - [ ] Match button style with existing menu buttons
  - [ ] Add placeholder handler (IAP integration in Phase 7)
- [ ] Add back button functionality
- [ ] Test menu navigation flow

### 6.3 Version Number Display
- [ ] Add version text entity to main menu
- [ ] Position: Bottom right corner (opposite ad controls button)
- [ ] Display current version: "1.0.0"
- [ ] Scale appropriately for mobile
- [ ] White text, readable font size
- [ ] Test on different screen sizes/orientations

### 6.4 Options Menu - Vibration Toggle
- [ ] Add vibration toggle row to options menu
- [ ] Use `xbuttonselected.png` for ON state
- [ ] Use `xbuttonunselected.png` for OFF state
- [ ] Position below difficulty section
- [ ] Add "VIBRATION" label
- [ ] Implement toggle logic (prevent haptic commands when OFF)
- [ ] Save vibration preference to persistent storage
- [ ] Load vibration preference on startup
- [ ] Test toggle functionality
- [ ] Verify haptics disabled when toggled OFF

### 6.5 PlatformDelegates Integration
- [ ] Add vibration enabled/disabled state query
- [ ] Check vibration preference before sending haptic commands
- [ ] Update haptic trigger points to respect toggle
- [ ] Ensure no refactoring needed (simple gate on commands)
- [ ] Test haptic feedback respects toggle state

---

## Phase 7: Game Center & IAP Integration

### 7.1 StoreKit 2 - Remove Ads IAP
- [ ] Create StoreKit configuration file
- [ ] Define "Remove Ads" product (com.floppyturd.game.removeads)
- [ ] Set price point: $2.00 USD
- [ ] Implement StoreKit 2 purchase flow
- [ ] Add purchase restoration
- [ ] Handle purchase errors/cancellations
- [ ] Persist "ads removed" state locally
- [ ] Verify purchase across app restarts
- [ ] Sync with AdManager.setAdsEnabled(false)
- [ ] Test purchase flow end-to-end
- [ ] Test restore purchases
- [ ] Add purchase confirmation UI feedback

### 7.2 Game Center Initialization
- [ ] Implement Game Center login on game launch
- [ ] Add authentication state handling
- [ ] Handle authentication errors gracefully
- [ ] Show appropriate UI feedback during login
- [ ] Cache authentication state
- [ ] Test with logged-in user
- [ ] Test with logged-out user
- [ ] Test with Game Center disabled

### 7.3 Game Center API Stability
- [ ] Verify stable connection to Game Center API
- [ ] Implement retry logic for failed connections
- [ ] Handle network disconnections gracefully
- [ ] Add proper error logging for debugging
- [ ] Test achievement posting
- [ ] Verify proper async handling

### 7.4 Leaderboard Integration (Beta Testing Phase)
- [ ] Complete leaderboard UI integration
- [ ] Implement score submission
- [ ] Implement leaderboard fetching
- [ ] Test leaderboard display
- [ ] Add loading states for leaderboard
- [ ] Handle leaderboard errors
- [ ] Test during App Center beta deployment
- [ ] Verify score persistence and display

---

---

## Phase 8: Pre-Deployment Checklist

### 8.1 Build Configuration
- [ ] Set correct bundle ID: `com.floppyturd.game`
- [ ] Update version number
- [ ] Update build number
- [ ] Set Release configuration
- [ ] Disable debug logging
- [ ] Remove test code/cheats
- [ ] Verify signing certificates
- [ ] Configure provisioning profiles

### 8.2 Asset Review
- [ ] Verify all assets are production-ready
- [ ] Check for placeholder graphics
- [ ] Verify audio quality
- [ ] Check app icons (all sizes)
- [ ] Verify launch screen

### 8.3 Testing Matrix
- [ ] Test full gameplay loop
- [ ] Test ad presentation flow (death → game over → ad → resume)
- [ ] Test "Remove Ads" IAP purchase
- [ ] Test "Remove Ads" restoration
- [ ] Test vibration toggle functionality
- [ ] Test all difficulty levels (Runny, Regular, Rough)
- [ ] Test skill purchases
- [ ] Test hat purchases
- [ ] Test save/load functionality
- [ ] Test audio settings persistence
- [ ] Test difficulty settings persistence
- [ ] Test Game Center integration
- [ ] Test leaderboards
- [ ] Test achievements (if implemented)
- [ ] Test on multiple devices
- [ ] Test fresh install
- [ ] Test update scenario (if applicable)

### 8.4 App Store Requirements
- [ ] Prepare app description
- [ ] Prepare keywords
- [ ] Prepare screenshots (all required sizes)
- [ ] Include Ad Controls menu in screenshots
- [ ] Show IAP "Remove Ads" option
- [ ] Prepare promotional text
- [ ] Review App Store guidelines compliance
- [ ] Verify age rating requirements
- [ ] Check privacy policy requirements
- [ ] Review in-app purchase guidelines
- [ ] Submit IAP for review alongside app
- [ ] Verify AdMob compliance and privacy disclosures

---

## Phase 9: App Center Beta Deployment

### 9.1 App Center Setup
- [ ] Configure App Center project
- [ ] Set up distribution groups
- [ ] Configure build automation
- [ ] Set up crash reporting
- [ ] Configure analytics

### 9.2 Beta Release
- [ ] Build beta version
- [ ] Upload to App Center
- [ ] Distribute to beta testers
- [ ] Monitor crash reports
- [ ] Gather feedback
- [ ] Test leaderboard integration in production environment
- [ ] Monitor ad impressions and revenue
- [ ] Test IAP purchases with real Apple Sandbox accounts
- [ ] Verify ads do not show after "Remove Ads" purchase

---

## Current Sprint: UI & IAP Setup

### Phase 6 - UI Enhancements (Current Priority):
1. [ ] Integrate `adcontrolsbutton.png` into main menu (bottom left, 6x scale)
2. [ ] Add version number display (bottom right, "1.0.0")
3. [ ] Create Ad Controls menu structure
4. [ ] Add "Remove Ads" label and "$2.00" button
5. [ ] Integrate `xbuttonselected.png` and `xbuttonunselected.png`
6. [ ] Add vibration toggle to options menu
7. [ ] Implement vibration toggle state persistence
8. [ ] Gate haptic commands based on toggle state
9. [ ] Test all new UI elements and navigation

### Phase 7 - IAP Integration (Next Sprint):
10. [ ] Create StoreKit configuration for "Remove Ads"
11. [ ] Implement StoreKit 2 purchase flow
12. [ ] Wire IAP to AdManager.setAdsEnabled(false)
13. [ ] Add purchase restoration flow
14. [ ] Persist "ads removed" state
15. [ ] Complete Game Center initialization
16. [ ] Test IAP end-to-end

### Phase 8 & 9 - Testing & Deployment:
17. [ ] Complete pre-deployment checklist
18. [ ] Full testing matrix execution
19. [ ] App Center beta deployment
20. [ ] Monitor ad revenue and IAP purchases
21. [ ] Leaderboard testing in production
22. [ ] Performance and crash monitoring

---

## Known Issues to Watch

- ~~Debug rectangles visible on main menu before buttons populate~~ ✅ FIXED
- ~~State transition rendering glitches (domino effect - background loads before UI)~~ ✅ FIXED
- ~~Difficulty enum appearing as normal/easy/hard instead of Runny/Regular/Rough~~ ✅ FIXED
- ~~Pause menu audio track is "sticky" vs smooth main menu behavior~~ ✅ FIXED
- ~~Some hats currently unlocked by default~~ ✅ FIXED
- ~~Hat order incorrect (premium hats in middle instead of end)~~ ✅ FIXED
- ~~Mixed bundle IDs (engine vs game)~~ ✅ FIXED
- ~~Separate module library needs consolidation~~ ✅ FIXED
- Need to add Ad Controls menu UI
- Need to add vibration toggle to options
- Need to implement "Remove Ads" IAP
- Need version number display on main menu

---

## Post-Deployment Tasks

- [ ] Monitor App Center crash reports
- [ ] Review user feedback
- [ ] Monitor ad impressions and eCPM
- [ ] Track "Remove Ads" IAP conversion rate
- [ ] Monitor leaderboard functionality
- [ ] Check Game Center integration stability
- [ ] Performance metrics review
- [ ] Plan updates based on feedback
- [ ] A/B test ad frequency if needed

---

## Notes

- All difficulty references must use: `Runny`, `Regular`, `Rough`
- Bundle ID must be: `com.floppyturd.game` everywhere
- No hats should be unlocked by default
- Last 3 hats (Gold Crown, Pharaoh, Poop) = 250 coins
- Pause menu audio should match main menu behavior exactly
- Ad controls button: bottom left, 6x scale
- Version number: bottom right, opposite ad controls button
- IAP price: exactly $2.00 (no $1.99) - "number 2 is for poop" 💩
- Vibration toggle: simple gate on PlatformDelegates, no major refactoring
- Ad Controls menu layout: similar to Leaderboard menu

---

## Implementation Reference

### Ad System Files (Complete)
- `src/iOS/Advertising/AdManager.swift` ✅
- `src/iOS/Threading/ThreadingProxy.cpp/.h` ✅
- `src/FloppyTurd/Systems/AdSystem.cpp/.h` ✅
- `src/FloppyTurd/States/GameplayState.cpp` ✅
- `src/FloppyTurd/Systems/PlayerControllerSystem.cpp/.h` ✅
- `src/iOS/GameViewController.swift` ✅
- `Info.plist` (GADApplicationIdentifier) ✅
- `CMakeLists.txt` (XCFramework links) ✅

### Files to Modify for Phase 6 UI
- `src/FloppyTurd/States/MainMenuState.h` - Add ad controls button, version text, menu mode
- `src/FloppyTurd/States/MainMenuState.cpp` - Implement ad controls menu, vibration toggle
- `src/assets/graphics/ui/hud/adcontrolsbutton.png` - ✅ Asset exists
- `src/assets/graphics/ui/hud/xbuttonselected.png` - ✅ Asset exists
- `src/assets/graphics/ui/hud/xbuttonunselected.png` - ✅ Asset exists

### Files to Create for Phase 7 IAP
- `FloppyTurd.storekit` - StoreKit configuration file
- `src/iOS/Store/StoreManager.swift` - StoreKit 2 purchase logic
- `src/Engine/Platform/PlatformDelegates.h` - Add IAP delegates (if needed)

---

**Current Phase**: 6 - UI Enhancements & IAP Preparation  
**Target**: Complete Phase 6-7 before App Store submission
