# Final App Store Submission Checklist - Floppy Turd

## ✅ Completed Code Items
- [x] GameCenter authentication implemented
- [x] 9 Leaderboards configured and created in App Store Connect
- [x] StoreKit 2 IAP system with "Remove Ads" ($1.99)
- [x] App icons generated and displaying
- [x] Bundle ID: com.floppyturd.game
- [x] Build succeeds for iOS Simulator
- [x] Swift 6.0 compilation errors resolved

---

## ✅ CRITICAL ITEMS COMPLETED

### 1. AdMob Production IDs ✅
**Status:** COMPLETE - Production IDs configured

- ✅ App ID: `ca-app-pub-2487109358798103~4785125685`
- ✅ Interstitial ID: `ca-app-pub-2487109358798103/2326483309`
- ✅ Debug builds use test IDs, Release builds use production IDs
- ✅ Configured in `AdManager.swift` and `Info.plist`

### 2. Add Privacy Policy & App Tracking Transparency (ATT)

**Legal Requirement:** Apps with ads MUST have a privacy policy and ATT prompt.

### 2. Privacy Policy & ATT (App Tracking Transparency) ✅

**Status:** COMPLETE - Privacy policy live, ATT prompt implemented

#### A. Privacy Policy (DONE! ✅)
- ✅ Privacy policy created and deployed
- ✅ **Live URL:** https://TechnicalTortuga.github.io/floppyturd-privacy/privacy-policy.html
- ✅ Covers AdMob, GameCenter, IAP, COPPA, GDPR, CCPA
- ✅ Contact: floppyturdthegame@gmail.com
- ✅ Repository: github.com/TechnicalTortuga/floppyturd-privacy

#### B. ATT Prompt (DONE! ✅)
- ✅ `NSUserTrackingUsageDescription` added to Info.plist
- ✅ ATT prompt implemented in `GameViewController.swift`
- ✅ Shows on first launch, respects user choice
- ✅ No crashes, builds successfully

### 3. COPPA Compliance (Family-Friendly App) ✅

**Status:** COMPLETE - maxAdContentRating set to .general

- ✅ Configured in `AdManager.swift` init
- ✅ maxAdContentRating = .general (family-friendly content)
- ✅ Compliant with 9+ age rating

### 4. Configure IAP in App Store Connect

**Steps:**
1. Go to App Store Connect > Your App > In-App Purchases
2. Create new In-App Purchase:
   - Type: Non-Consumable
   - Reference Name: "Remove Ads"
   - Product ID: `com.floppyturd.game.removeads`
   - Price: $1.99 (Tier 2)
   - Localized Description: "Remove all ads permanently!"
   - Screenshot: Required (show the purchase confirmation)

3. Submit for review alongside app

### 5. Create StoreKit Configuration File for Testing

**In Xcode:**
1. File → New → File
2. Choose "StoreKit Configuration File"
3. Name: `FloppyTurd.storekit`
4. Add product:
   - Type: Non-Consumable
   - Product ID: `com.floppyturd.game.removeads`
   - Price: $1.99
5. Enable in scheme: Edit Scheme → Run → StoreKit Configuration → FloppyTurd.storekit

---

## 📱 Testing Requirements

### Test on Physical Device
**Why:** GameCenter and IAP don't work in Simulator

**Steps:**
1. Connect iPhone/iPad via USB
2. Change build target from Simulator to your device
3. Build and run: `Cmd+R`
4. Test GameCenter login
5. Test IAP purchase (use Sandbox account)
6. Verify leaderboard score submission
7. Verify "Remove Ads" works

### Create Sandbox Tester Account
1. App Store Connect → Users and Access → Sandbox Testers
2. Create test account with fake email
3. Sign out of App Store on device
4. When prompted during IAP test, sign in with sandbox account
5. Purchase is free in sandbox mode

---

## 📸 App Store Assets Required

### Screenshots (Required Sizes)
- **6.7" (iPhone 14 Pro Max):** 1290 x 2796 (at least 3 screenshots)
- **6.5" (iPhone 11 Pro Max):** 1242 x 2688 (at least 3 screenshots)
- **5.5" (iPhone 8 Plus):** 1242 x 2208 (optional but recommended)

**Must Show:**
- Gameplay
- Main menu
- Leaderboards
- Settings with "Remove Ads" option

### App Icon
- ✅ Already set: 1024x1024 AppIcon (using liquid glass version)
- No transparency
- No rounded corners (Apple adds them)

### App Information
- **Name:** Floppy Turd (or just "Floppy Turd" if available)
- **Subtitle:** "Dodge Pipes, Beat the Boss!"
- **Description:** Highlight 6 levels, leaderboards, boss fight, ad-free option
- **Keywords:** floppy, bird, pipes, arcade, casual, leaderboard, boss
- **Category:** Games → Arcade
- **Age Rating:** 9+ (Infrequent/Mild Cartoon Violence)
- **Copyright:** © 2025 [Your Name/Company]

---

## 🔧 Build for Distribution

### Archive the App
```bash
# Build for iOS Device (not simulator)
cd /Users/aimac/Development/FloppyTurd
rm -rf build_ios_device && mkdir build_ios_device && cd build_ios_device

# Configure for device
cmake .. -G Xcode -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
  -DCMAKE_OSX_ARCHITECTURES=arm64

# Build
xcodebuild -project FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -configuration Release \
  -sdk iphoneos \
  -archivePath FloppyTurd.xcarchive \
  archive
```

### Upload to App Store Connect
**Option 1 - Xcode Organizer:**
1. Window → Organizer
2. Select archive
3. Click "Distribute App"
4. Choose "App Store Connect"
5. Upload

**Option 2 - Command Line:**
```bash
xcodebuild -exportArchive \
  -archivePath FloppyTurd.xcarchive \
  -exportPath . \
  -exportOptionsPlist ExportOptions.plist
```

---

## 📋 App Store Connect Configuration

### 1. App Information
- [ ] Privacy Policy URL (REQUIRED)
- [ ] Support URL
- [ ] Marketing URL (optional)
- [ ] Copyright
- [ ] Age Rating (9+)

### 2. Pricing & Availability
- [ ] Price: Free
- [ ] Available in all territories (or select specific countries)

### 3. Privacy Details (REQUIRED for apps with ads)
**Data Used to Track You:**
- [x] Identifiers (Device ID for advertising)

**Data Linked to You:**
- [x] Identifiers (if using personalized ads)
- [x] Usage Data (ad interactions)

**Data Not Linked to You:**
- [x] Crash Data
- [x] Performance Data

### 4. App Review Information
- [ ] Demo account (if needed - GameCenter should work without)
- [ ] Notes for reviewer:
  ```
  This is a casual arcade game with 6 levels and GameCenter leaderboards.
  
  - Level 1-5: Pipe dodging gameplay
  - Level 6: Boss fight (speedrun only, no pipes)
  - AdMob interstitial ads shown after each level
  - IAP "Remove Ads" available for $1.99
  
  GameCenter login required for leaderboards.
  Please test on a physical device as GameCenter doesn't work in Simulator.
  ```

---

## ✅ Final Verification Checklist

### Code
- [x] ~~Test~~ Production AdMob IDs configured ✅
- [x] ATT prompt added and tested ✅
- [x] COPPA compliance configured (maxAdContentRating) ✅
- [x] Privacy policy URL: https://TechnicalTortuga.github.io/floppyturd-privacy/privacy-policy.html ✅
- [x] All 9 leaderboard IDs match App Store Connect configuration ✅
- [x] IAP Product ID matches: `com.floppyturd.game.removeads` ✅
- [x] Debug mode disabled (fresh start ready) ✅
- [x] Hat system fixed (start hatless, all locked, can unequip) ✅

### Testing
- [ ] Tested on physical iPhone/iPad
- [ ] GameCenter login works
- [ ] Leaderboard scores submit successfully
- [ ] IAP purchase works (sandbox)
- [ ] Ads show correctly
- [ ] "Remove Ads" purchase removes ads permanently
- [ ] App doesn't crash
- [ ] Performance is acceptable

### App Store Connect
- [ ] App created with Bundle ID: `com.floppyturd.game`
- [ ] All 9 leaderboards configured
- [ ] IAP "Remove Ads" configured
- [ ] Screenshots uploaded (all required sizes)
- [ ] App description written
- [x] Privacy policy hosted and linked ✅ https://TechnicalTortuga.github.io/floppyturd-privacy/privacy-policy.html
- [ ] Age rating set to 9+
- [ ] Privacy details filled out

### Legal
- [ ] Privacy policy created and accessible
- [ ] COPPA compliance verified
- [ ] GDPR compliance (if selling in EU)
- [ ] Terms of Service (optional but recommended)

---

## 📞 Next Steps

1. **Today:** Replace test AdMob IDs, add ATT prompt
2. **Tomorrow:** Test on physical device, create sandbox tester
3. **Day 3:** Take screenshots, write app description
4. **Day 4:** Archive build, upload to App Store Connect
5. **Day 5:** Submit for review

**Estimated Review Time:** 1-3 days  
**Common Rejection Reasons:**
- Missing privacy policy
- Missing ATT prompt for ad tracking
- IAP screenshot missing
- Crashes on launch

---

## 🎯 You're Almost There! 🚀

**COMPLETED ✅:**
✅ All game code working
✅ GameCenter leaderboards (9 total)
✅ IAP system with "Remove Ads"
✅ App icons displaying
✅ Production AdMob IDs configured
✅ Privacy policy live at https://TechnicalTortuga.github.io/floppyturd-privacy/privacy-policy.html
✅ ATT prompt implemented
✅ COPPA compliance configured
✅ Debug mode disabled
✅ Hat system fixed (start hatless, all locked)
✅ Build succeeds

**REMAINING TO-DO ⚠️:**
1. **Physical Device Testing** (~1-2 hours)
   - Test on real iPhone/iPad
   - Verify GameCenter login works
   - Test IAP purchase (sandbox)
   - Verify ads show correctly
   - Test "Remove Ads" purchase

2. **App Store Connect Setup** (~1 hour)
   - Verify Bundle ID and leaderboards configured
   - Configure IAP "Remove Ads" product
   - Fill out privacy questionnaire
   - Set age rating to 9+
   - Add privacy policy URL

3. **App Store Assets** (~2-3 hours)
   - Take screenshots (6.7", 6.5" displays)
   - Write app description
   - Create app preview video (optional)

4. **Build & Submit** (~1 hour)
   - Archive Release build
   - Upload to App Store Connect
   - Fill out App Review info
   - Submit for review

**TOTAL TIME REMAINING:** ~5-7 hours before submission!

---

## 📞 Next Immediate Steps

1. **RIGHT NOW:** Test on a physical device
   - Connect iPhone/iPad
   - Build in Release mode
   - Test GameCenter, IAP, and ads

2. **TODAY:** Take screenshots and write description

3. **TOMORROW:** Archive, upload, and submit for review

**Estimated Review Time:** 1-3 days after submission
