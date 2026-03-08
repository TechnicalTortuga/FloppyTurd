# Google Mobile Ads SDK Installation Guide

## Overview
This guide explains how to add the Google Mobile Ads SDK to FloppyTurd using Swift Package Manager in Xcode.

**Time Required:** 2-3 minutes  
**Method:** Swift Package Manager (SPM) via Xcode  
**SDK Version:** 12.x or later (recommended)

---

## Prerequisites

- Xcode 15.0 or later
- CMake-generated Xcode project in `build_ios/FloppyTurd.xcodeproj`
- Internet connection (to download the SDK)

---

## Installation Steps

### Step 1: Open the Xcode Project

```bash
cd /Users/aimac/Development/FloppyTurd
open build_ios/FloppyTurd.xcodeproj
```

Or if CocoaPods generated a workspace:
```bash
open FloppyTurd.xcworkspace
```

### Step 2: Add Swift Package

1. In Xcode, select the **FloppyTurd** project in the Project Navigator (top of left sidebar)
2. Select the **FloppyTurd** target (under Targets)
3. Click the **"+"** button under "Frameworks, Libraries, and Embedded Content" section
4. Or go to **File** → **Add Package Dependencies...**

### Step 3: Enter Package URL

In the package search field, enter:
```
https://github.com/googleads/swift-package-manager-google-mobile-ads.git
```

### Step 4: Select Version

- **Dependency Rule:** Select "Up to Next Major Version"
- **Version:** Enter `12.0.0` (or the latest available)
- This ensures you get updates within the 12.x series

### Step 5: Add to Target

1. Click **"Add Package"**
2. In the "Choose Package Products" dialog:
   - Select **GoogleMobileAds**
   - Ensure it's added to the **FloppyTurd** target
3. Click **"Add Package"**

### Step 6: Verify Installation

Wait for Xcode to download and integrate the package (may take 1-2 minutes).

Once complete, you should see:
- `GoogleMobileAds` listed under "Package Dependencies" in the project navigator
- No build errors when you build the project

---

## Build and Test

### Rebuild the Project

```bash
cd /Users/aimac/Development/FloppyTurd
xcodebuild -project build_ios/FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" \
  clean build
```

Or build in Xcode:
- Press `⌘B` (Cmd+B) to build
- Or **Product** → **Build**

### Expected Result

✅ **Build succeeds** - All AdMob integration code compiles successfully

---

## Testing the Integration

### 1. Check SDK Initialization

Run the app and check the console logs:

```
[AdManager] Initializing Google Mobile Ads SDK...
[AdManager] Google Mobile Ads SDK initialized
[AdManager] Adapter statuses:
  - ...
```

### 2. Test Ad Preloading

The first ad should preload automatically on app launch:

```
[AdSystem] Initialized with learning period: 3 deaths, ad frequency: 5 deaths
[AdSystem] Preloading next ad...
[AdManager] Preloading interstitial ad...
[AdManager] Interstitial ad preloaded successfully
```

### 3. Test Ad Display

1. Play the game and die 3 times (learning period - no ads)
2. On the 5th death, an ad should appear
3. Check logs:

```
[AdSystem] Player died - Death count since last ad: 5, Total: 5
[AdSystem] Death threshold reached (5/5) - showing ad
[AdManager] Showing interstitial ad...
[AdManager] Ad will present full screen content
[AdManager] Ad dismissed - user closed the ad
[AdManager] Preloading next ad...
```

---

## Troubleshooting

### Issue: "No such module 'GoogleMobileAds'"

**Solution:** The package wasn't added correctly. Repeat steps 2-5.

### Issue: Package download fails

**Solution:**
- Check your internet connection
- Try again (Xcode sometimes times out)
- Or manually download: **File** → **Packages** → **Reset Package Caches**

### Issue: Build errors after adding package

**Solution:**
- Clean build folder: **Product** → **Clean Build Folder** (`⌘⇧K`)
- Quit Xcode and reopen
- Delete `DerivedData`: `rm -rf ~/Library/Developer/Xcode/DerivedData/FloppyTurd-*`

### Issue: Ads don't show

**Checklist:**
- ✅ SDK initialized? Check logs for "Google Mobile Ads SDK initialized"
- ✅ Ad preloaded? Check logs for "Interstitial ad preloaded successfully"
- ✅ Past learning period? First 3 deaths show no ads
- ✅ Test ad unit ID in use? (Default for development)

---

## Alternative: CocoaPods Installation

If you prefer CocoaPods, a `Podfile` is included in the project root:

```bash
cd /Users/aimac/Development/FloppyTurd
pod install
open FloppyTurd.xcworkspace  # Use workspace, not .xcodeproj!
```

**Note:** CocoaPods integration with CMake can be complex. Swift Package Manager is recommended.

---

## Configuration

### Ad Unit IDs

The project uses **test ad unit IDs** by default (in `src/iOS/Advertising/AdManager.swift`):

```swift
private let testAdUnitID = "ca-app-pub-3940256099942544/4411468910"  // Google's test ID
```

**Before releasing to TestFlight or App Store:**

1. Create real ad unit IDs in your [AdMob account](https://apps.admob.com/)
2. Replace test IDs in:
   - `src/iOS/Advertising/AdManager.swift` - Update `testAdUnitID`
   - `Info.plist` - Update `GADApplicationIdentifier`

### Ad Frequency

Default settings (in `src/FloppyTurd/Systems/AdSystem.cpp`):

```cpp
m_learningPeriodDeaths = 3;  // First 3 deaths = no ads
m_adFrequencyDeaths = 5;     // Show ad every 5 deaths
```

To adjust, modify `FloppyTurdGame.cpp` initialization:

```cpp
m_adSystem->SetLearningPeriod(5);  // First 5 deaths = no ads
m_adSystem->SetAdFrequency(3);     // Show ad every 3 deaths
```

---

## SDK Documentation

- **AdMob iOS Guide:** https://developers.google.com/admob/ios/quick-start
- **Swift Package:** https://github.com/googleads/swift-package-manager-google-mobile-ads
- **API Reference:** https://developers.google.com/admob/ios/reference
- **SKAdNetwork IDs:** https://developers.google.com/admob/ios/skadnetwork

---

## Next Steps

✅ **Installation Complete!**

1. ✅ Test with test ad IDs in simulator
2. ⏸️ Create real AdMob account and ad units
3. ⏸️ Replace test IDs before TestFlight
4. ⏸️ Monitor AdMob dashboard for performance
5. ⏸️ Implement "Remove Ads" IAP (optional)

---

*Last Updated: AdMob Integration Phase*  
*SDK Version: 12.x*  
*Method: Swift Package Manager*