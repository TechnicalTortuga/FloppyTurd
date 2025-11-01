# Monday Deployment Guide - FloppyTurd iOS

## Pre-Deployment Cleanup

### 1. Clean Up Build Directories
You currently have 4 different iOS build directories which can cause confusion in Xcode. For a clean Monday deployment, use only ONE build directory.

**Recommended approach:**
```bash
cd /Users/aimac/Development/FloppyTurd

# Back up your current build directories (optional)
mv build_ios build_ios_backup_$(date +%Y%m%d)
mv build_ios26 build_ios26_backup_$(date +%Y%m%d)
mv build_ios26_device build_ios26_device_backup_$(date +%Y%m%d)
mv build_ios_device build_ios_device_backup_$(date +%Y%m%d)

# Create ONE fresh build directory
mkdir build_ios_production
```

### 2. Target Schema - Current Status
Your CMakeLists.txt is **correctly structured** with conditional compilation:
- **iOS builds**: Creates `FloppyTurd` app bundle + `FloppyTurdGame` static library
- **Desktop builds**: Creates standalone `FloppyTurd` executable

The confusion comes from having multiple build directories. Once you use a single build directory, Xcode will show a clean target schema.

## Latest Fixes Applied (Ready for Monday)

### ✅ Boss Health Bar
- **White hurt bar**: Now trims with 0.3s delay after damage, then smoothly interpolates to current health
- **Red health bar**: Position locked to prevent any movement/transforms
- **Effect**: Clean visual feedback when boss takes damage, white bar shows briefly then shrinks

### ✅ Wave Amplitudes Reduced
- **Boss level coins**: Reduced from 400 to 100 amplitude (smoother, less dramatic)
- **Rainbow heart pickup**: Reduced from 800 to 400 amplitude (still special but more controlled)

### ✅ Previously Fixed (From Boss Level Tweaks)
- Lock-on dots target player center ✅
- Lock-on dots flash red/white before throw ✅
- Enemy pools clean on Try Again (except Rat King) ✅
- Boss health resets properly on Try Again ✅
- Player projectiles do 10 damage to Rat King ✅
- Snowball hitbox reduced to 4px radius ✅

## Clean Build Process for Monday

### Step 1: Generate Clean Xcode Project
```bash
cd /Users/aimac/Development/FloppyTurd

# Generate iOS build with clean directory
cmake -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=ios-cmake-master/ios.toolchain.cmake \
  -DPLATFORM=OS64 \
  -DDEPLOYMENT_TARGET=17.0 \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=86886N72J8 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="Apple Development" \
  -B build_ios_production
```

### Step 2: Build for Simulator (Testing)
```bash
# Build for iOS Simulator
xcodebuild -project build_ios_production/FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "platform=iOS Simulator,name=iPhone 16,OS=18.3.1" \
  -configuration Debug \
  clean build

# Install to simulator
xcrun simctl install booted build_ios_production/Debug-iphonesimulator/FloppyTurd.app

# Launch on simulator
xcrun simctl launch booted com.floppyturd.engine
```

### Step 3: Build for Device (Deployment)
```bash
# Build for iOS Device
xcodebuild -project build_ios_production/FloppyTurd.xcodeproj \
  -scheme FloppyTurd \
  -destination "generic/platform=iOS" \
  -configuration Release \
  -archivePath build_ios_production/FloppyTurd.xcarchive \
  archive

# Export IPA (for TestFlight or App Store)
xcodebuild -exportArchive \
  -archivePath build_ios_production/FloppyTurd.xcarchive \
  -exportPath build_ios_production/export \
  -exportOptionsPlist export_options.plist
```

### Step 4: Upload to App Store Connect
```bash
# Upload to TestFlight/App Store using Xcode's built-in uploader
xcrun altool --upload-app \
  --type ios \
  --file build_ios_production/export/FloppyTurd.ipa \
  --username "your-apple-id@email.com" \
  --password "@keychain:AC_PASSWORD"
```

Or use Xcode GUI:
1. Open `build_ios_production/FloppyTurd.xcodeproj` in Xcode
2. Product → Archive
3. Window → Organizer → Archives
4. Select your archive → Distribute App → App Store Connect

## Target Schema in Xcode

After clean rebuild, you should see in Xcode:
- **Targets**: 
  - `FloppyTurd` (main app target)
  - `FloppyTurdGame` (static library with game logic)
- **Schemes**: 
  - `FloppyTurd` (use this for building/running)

**There is NO duplicate target issue** - the CMakeLists conditionally creates iOS OR Desktop targets, never both simultaneously.

## Pre-Deployment Testing Checklist

### Boss Level (Level 6) Tests
- [ ] Boss health bar appears when Rat King activates
- [ ] Health bar shows full health at start
- [ ] White hurt bar appears briefly when boss takes damage
- [ ] White bar trims smoothly after 0.3s delay
- [ ] Red health bar stays in position (no movement)
- [ ] Boss dies after 20 projectile hits (10 damage each × 20 = 200 health)
- [ ] "Try Again" resets boss to full health
- [ ] "Try Again" removes rat minions from previous attempt
- [ ] Lock-on dots aim at player center
- [ ] Lock-on dots flash red/white rapidly before throwing

### Wave Motion Tests
- [ ] Boss level coins wave smoothly with 100 amplitude
- [ ] Coins are easier to collect (less vertical movement)
- [ ] Rainbow heart waves with 400 amplitude (still special feeling)
- [ ] Wave motion doesn't feel too slow or too fast

### Snowball Hitbox Tests
- [ ] Snowballs have small hitbox (4px radius)
- [ ] Snowballs can be dodged more easily than before
- [ ] Hitbox feels fair and precise

## Version Info

- **Build Version**: 1.0.0
- **Deployment Target**: iOS 17.0+
- **Swift Version**: 6.0
- **C++ Standard**: C++20
- **Bundle ID**: com.floppyturd.engine
- **Team ID**: 86886N72J8

## Files Modified in Latest Session

1. `src/FloppyTurd/Systems/PickupSystem.cpp`
   - Boss coin wave amplitude: 400 → 100
   - Rainbow heart amplitude: 800 → 400

2. `src/FloppyTurd/Systems/BossHealthBar.cpp`
   - Added delayed white bar trimming (0.3s delay)
   - Linear interpolation for smooth trim
   - Locked red bar position (no transforms)

3. `src/FloppyTurd/Systems/BossHealthBar.h`
   - Added `m_whiteTrimDelay` member variable

## Quick Reference Commands

```bash
# View simulator logs
xcrun simctl spawn booted log stream --level debug --predicate 'subsystem contains "com.floppyturd"'

# Clean derived data
rm -rf ~/Library/Developer/Xcode/DerivedData/FloppyTurd-*

# List available simulators
xcrun simctl list devices available

# Uninstall from simulator
xcrun simctl uninstall booted com.floppyturd.engine
```

## Deployment Day Checklist

- [ ] Run clean build from scratch (Step 1-2 above)
- [ ] Test on simulator - verify all boss level fixes
- [ ] Test on physical device (if available)
- [ ] Run through all 6 levels to ensure no regressions
- [ ] Verify game center integration works
- [ ] Verify save/load system works
- [ ] Check credits screen
- [ ] Archive and export Release build (Step 3)
- [ ] Upload to App Store Connect (Step 4)
- [ ] Submit for review with release notes

## Emergency Rollback

If issues arise:
```bash
# Revert to backup build directory
mv build_ios_backup_YYYYMMDD build_ios

# Build from known-good state
xcodebuild -project build_ios/FloppyTurd.xcodeproj -scheme FloppyTurd ...
```

## Support Notes

- All boss bar changes are isolated to `BossHealthBar.cpp` and `.h`
- Wave changes are isolated to `PickupSystem.cpp`
- No changes to core systems (ECS, rendering, collision)
- All changes are additive/refinements, no breaking changes
- Previous boss level fixes remain intact

**Ready for Monday deployment! 🚀**