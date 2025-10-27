# Implementation Phases - Haptics, Serialization & Game Center

**Project**: Floppy Turd  
**Systems**: Haptic Feedback, Save/Load Persistence, Game Center Leaderboards  
**Platform**: iOS 10.0+  
**Date**: 2024  
**Status**: Planning Phase

---

## Overview

This document provides a week-by-week implementation plan for three major systems:
1. **Haptic Feedback System** (VibrationSystem)
2. **Serialization/Deserialization System** (Data Persistence)
3. **Game Center Integration** (Leaderboards with Anti-Cheat)

**Total Timeline**: 6 weeks (can be parallelized)  
**Team Size**: 1 developer  
**Risk Level**: Medium (Game Center testing requires physical devices)

---

## Table of Contents

1. [Week-by-Week Breakdown](#week-by-week-breakdown)
2. [Dependency Graph](#dependency-graph)
3. [Critical Path](#critical-path)
4. [Testing Milestones](#testing-milestones)
5. [Risk Mitigation](#risk-mitigation)
6. [Rollout Strategy](#rollout-strategy)

---

## Week-by-Week Breakdown

### Week 1: Foundations

**Focus**: Core infrastructure for all three systems

#### Serialization System (3 days)
- **Day 1-2**: Create SaveManager.swift
  - [ ] Define Codable data models (GameSaveData, ProgressData, etc.)
  - [ ] Implement JSON encoder/decoder with pretty printing
  - [ ] Add checksum generation (SHA256)
  - [ ] Implement atomic file writes (temp → main)
  - [ ] Add automatic backup (.bak) creation
  - [ ] Test save/load roundtrip
  
- **Day 3**: Legacy Migration
  - [ ] Implement binary file reader for old format
  - [ ] Create migration logic (binary → JSON)
  - [ ] Test with sample legacy save files
  - [ ] Add migration logging

**Deliverable**: SaveManager that reads legacy saves and writes new JSON format

#### Haptic System (2 days)
- **Day 4-5**: Create HapticManager.swift
  - [ ] Implement generator pooling (Impact, Selection, Notification)
  - [ ] Add device capability detection
  - [ ] Create HapticDelegate struct in PlatformDelegates.h
  - [ ] Bridge C++ → Swift haptic calls
  - [ ] Test on iPhone (verify no-op on iPad)

**Deliverable**: Working haptic infrastructure with C++ bridge

**Testing**:
- [ ] Unit tests for SaveManager encode/decode
- [ ] Migration test with 10 legacy files
- [ ] Haptic test on 2+ devices (iPhone, iPad)

**Risks**:
- ⚠️ Legacy save format might have undocumented fields
- ⚠️ Haptic generators might not initialize on all devices

---

### Week 2: Settings & Basic Integration

**Focus**: Move settings to UserDefaults, integrate basic haptics

#### Settings Migration (2 days)
- **Day 1**: UserDefaults Implementation
  - [ ] Create GameSettings wrapper for UserDefaults
  - [ ] Migrate floppyturd_settings.cfg → UserDefaults
  - [ ] Register default values
  - [ ] Add haptics toggle setting
  - [ ] Add difficulty setting (Runny/Regular/Rough)
  - [ ] Update C++ to read settings via delegate
  
- **Day 2**: Settings UI
  - [ ] Add haptics toggle to Options menu
  - [ ] Add difficulty selector to Options menu
  - [ ] Test settings persistence across app restarts
  - [ ] Remove legacy settings.cfg file after migration

**Deliverable**: All settings in UserDefaults, legacy file deleted

#### Player Action Haptics (3 days)
- **Day 3**: PlayerControllerSystem Integration
  - [ ] Add haptic calls to Jump() (Light impact, 0.4 intensity)
  - [ ] Add haptic calls to Shoot() (Rigid impact, 0.6 intensity)
  - [ ] Add haptic calls to OnDeath() (Heavy impact, 1.0 intensity)
  - [ ] Add prepare() calls to reduce latency
  
- **Day 4-5**: Collision & Pickup Haptics
  - [ ] ObstacleSystem: Death collision (Heavy impact)
  - [ ] PickupSystem: Coin pickup (Light impact, 0.3)
  - [ ] PickupSystem: Heart pickup (Medium impact, 0.6)
  - [ ] Add debouncing for rapid pickups
  - [ ] Profile performance (should be < 1ms per haptic)

**Deliverable**: Core gameplay haptics working

**Testing**:
- [ ] Settings persist correctly
- [ ] Jump feels responsive (< 20ms latency)
- [ ] Death haptic unmistakable
- [ ] Coin pickups satisfying (not overwhelming after 50 coins)
- [ ] Frame rate stays 60 FPS with haptics enabled

**Risks**:
- ⚠️ Haptic latency might be noticeable without prepare()
- ⚠️ Rapid coin collection might create vibration noise

---

### Week 3: Game Center Setup & Hat Persistence

**Focus**: Get Game Center working, unify hat saves

#### Game Center Foundation (3 days)
- **Day 1**: App Store Connect Setup
  - [ ] Create/verify App Store Connect account
  - [ ] Enable Game Center for Floppy Turd app
  - [ ] Create 6 leaderboards:
    - com.floppyturd.level1 (Level 1: Tutorial)
    - com.floppyturd.level2 (Level 2: Gold Flush)
    - com.floppyturd.level3 (Level 3: Ice Throne)
    - com.floppyturd.level4 (Level 4: Sewer)
    - com.floppyturd.level5 (Level 5: Sky Castle)
    - com.floppyturd.level6 (Level 6: Rat King)
  - [ ] Configure leaderboards (Integer, High-to-Low, Best Score)
  - [ ] Create 2 sandbox test accounts
  
- **Day 2**: GameCenterManager.swift
  - [ ] Implement authentication flow
  - [ ] Add isAuthenticated() check
  - [ ] Handle authentication declined gracefully
  - [ ] Test authentication on device (simulator won't work)
  
- **Day 3**: GameCenterDelegate Bridge
  - [ ] Add GameCenterDelegate to PlatformDelegates.h
  - [ ] Implement authenticate(), submitScore(), showLeaderboards()
  - [ ] Bridge C++ → Swift Game Center calls
  - [ ] Test authentication from C++ code

**Deliverable**: Game Center authentication working

#### Hat System Unification (2 days)
- **Day 4**: Audit Current Hat Saves
  - [ ] Find HatsSystem SaveHatStatus() implementation
  - [ ] Document current hat save format
  - [ ] Extract hat data (15 unlocked bools, equipped index)
  
- **Day 5**: Merge into Main Save
  - [ ] Add CustomizationData to GameSaveData
  - [ ] Migrate existing hat data to new format
  - [ ] Update HatsSystem to use SaveManager
  - [ ] Test hat unlock/equip persistence
  - [ ] Delete legacy hat save file (if exists)

**Deliverable**: All game data in single JSON file

**Testing**:
- [ ] Authenticate with Game Center on device
- [ ] Create sandbox account and sign in
- [ ] Verify isAuthenticated() returns true
- [ ] Hat unlocks persist across app restarts
- [ ] Equipped hat loads correctly

**Risks**:
- ⚠️ App Store Connect approval can take 24-48 hours
- ⚠️ Game Center sandbox behaves differently than production
- ⚠️ Hat save location might be in unexpected place

---

### Week 4: Score Submission & Leaderboard UI

**Focus**: Submit scores, display leaderboards, anti-cheat basics

#### Score Submission (2 days)
- **Day 1**: Basic Submission
  - [ ] Implement submitScore() in GameCenterManager
  - [ ] Add score submission to LevelCompleteState
  - [ ] Handle offline caching (automatic via GameKit)
  - [ ] Test score submission in sandbox
  - [ ] Verify scores appear in Game Center dashboard
  
- **Day 2**: iOS Version Compatibility
  - [ ] Implement iOS 14+ API (GKLeaderboard.submitScore)
  - [ ] Implement iOS 10-13 fallback (GKScore.report)
  - [ ] Test on both iOS versions if possible
  - [ ] Add error handling and logging

**Deliverable**: Scores submit to Game Center

#### Leaderboard UI (3 days)
- **Day 3**: Main Menu Integration
  - [ ] Remove "Quit" button from MainMenuState
  - [ ] Add "Leaderboard" button in its place
  - [ ] Implement showLeaderboards() (all levels)
  - [ ] Test button opens Game Center UI
  
- **Day 4**: Level-Specific Leaderboards
  - [ ] Implement showLeaderboard(levelId)
  - [ ] Add "View Leaderboard" button to LevelCompleteState
  - [ ] Add leaderboard option to PauseSystem (optional)
  - [ ] Test individual level leaderboard display
  
- **Day 5**: Polish & Edge Cases
  - [ ] Handle "not authenticated" gracefully (show prompt)
  - [ ] Test with no internet (should show cached data)
  - [ ] Test with no friends (should show global only)
  - [ ] Add loading indicators if needed
  - [ ] Test GKGameCenterViewControllerDelegate dismiss

**Deliverable**: Full leaderboard UI working

**Testing**:
- [ ] Submit score from Level 1 → appears in leaderboard
- [ ] Offline score caches and submits when online
- [ ] Higher score replaces lower score
- [ ] Lower score doesn't replace higher score
- [ ] Leaderboard button opens correct view
- [ ] Dismiss leaderboard returns to game

**Risks**:
- ⚠️ Sandbox scores don't appear in production
- ⚠️ GKGameCenterViewController might crash on some iOS versions

---

### Week 5: Anti-Cheat Implementation

**Focus**: Multi-layer score validation

#### Checkpoint Validation (3 days)
- **Day 1**: Session Hashing
  - [ ] Generate random session seed on level start
  - [ ] Store session seed in Keychain
  - [ ] Create KeychainManager.swift wrapper
  - [ ] Test Keychain persistence across app restarts
  
- **Day 2**: Checkpoint Collection
  - [ ] Add OnObstaclePassed() to ObstacleSystem
  - [ ] Generate checkpoint hash (seed|index|playerY|obstacleX|time)
  - [ ] Collect hashes into vector
  - [ ] Create GetValidationProof() method
  - [ ] Test proof generation during gameplay
  
- **Day 3**: Proof Validation
  - [ ] Implement proof length validation (score × 8 chars)
  - [ ] Add spot-check verification (10 random samples)
  - [ ] Reject scores with invalid proofs
  - [ ] Clear session seed after successful submission
  - [ ] Test with tampered proofs

**Deliverable**: Checkpoint-based validation working

#### Additional Anti-Cheat Layers (2 days)
- **Day 4**: Rate Limiting & Bounds
  - [ ] Implement RateLimiter class (1 minute between submissions)
  - [ ] Add reasonable bounds checking (score < 1000, time > 0.5s/pipe)
  - [ ] Add device fingerprinting (store in Keychain)
  - [ ] Include fingerprint in GKScore.context field
  
- **Day 5**: Validation Flags
  - [ ] Create ValidationFlags class
  - [ ] Track: jumpedOnce, inputRatio, totalFrames
  - [ ] Generate flags hash
  - [ ] Validate flags before submission
  - [ ] Test with bot-like behavior (100% input) → rejected

**Deliverable**: Multi-layer anti-cheat system

**Testing**:
- [ ] Normal gameplay generates valid proof
- [ ] Modified save file score rejected
- [ ] Rapid resubmission blocked by rate limiter
- [ ] Impossible score (10,000 pipes) rejected
- [ ] Bot behavior (100% input frames) flagged
- [ ] Legitimate scores still submit successfully

**Risks**:
- ⚠️ Checkpoint hash algorithm could be reverse-engineered
- ⚠️ False positives might reject legitimate scores
- ⚠️ Keychain data persists across app reinstalls (could be issue)

---

### Week 6: Advanced Haptics, Polish & Testing

**Focus**: Boss haptics, Core Haptics, comprehensive testing

#### Core Haptics (2 days)
- **Day 1**: Boss Death Pattern
  - [ ] Create CHHapticEngine setup in HapticManager
  - [ ] Design 3-stage boss death pattern:
    - Stage 1: Heavy impact (0.0s)
    - Stage 2: Sustained rumble (0.2s - 1.0s)
    - Stage 3: Final explosion (1.0s)
  - [ ] Pre-load pattern on app launch
  - [ ] Test on iOS 13+ device
  
- **Day 2**: Boss System Integration
  - [ ] Add haptic to BossSystem OnTakeDamage (Medium impact)
  - [ ] Add boss death pattern to OnBossDeath
  - [ ] Add haptic to boss attack events (Heavy impact)
  - [ ] Test boss fight with haptics
  - [ ] Verify iOS 10-12 graceful degradation

**Deliverable**: Boss haptics with custom patterns

#### UI Haptics (1 day)
- **Day 3**: Menu & Shop Haptics
  - [ ] UISystem: Button press (Selection feedback)
  - [ ] HatsSystem: Hat selection (Selection feedback)
  - [ ] HatsSystem: Hat purchase (Success notification)
  - [ ] HatsSystem: Hat equip (Medium impact)
  - [ ] GameplayState: Level complete (Success notification)
  - [ ] GameplayState: Game over (Error notification)

**Deliverable**: Complete haptic coverage

#### Comprehensive Testing (2 days)
- **Day 4**: System Testing
  - [ ] Full save/load cycle test (20 iterations)
  - [ ] Game Center submission test (all 6 levels)
  - [ ] Haptic performance profiling (Instruments Time Profiler)
  - [ ] Battery drain test (30 min session with/without haptics)
  - [ ] Thread safety stress test (concurrent saves)
  - [ ] Migration test (10 legacy saves)
  
- **Day 5**: Edge Case Testing
  - [ ] No internet → offline caching
  - [ ] Low storage → graceful save failure
  - [ ] Jailbroken device → still works
  - [ ] iPad → haptics no-op
  - [ ] iOS 10.0 → legacy APIs work
  - [ ] iOS 17.0 → modern APIs work
  - [ ] App backgrounding during save
  - [ ] Force quit during level → data saved

**Deliverable**: Production-ready systems

**Final Testing Checklist**:
- [ ] All haptics feel good (subjective playtesting)
- [ ] No frame drops during haptics
- [ ] Battery impact < 5% per 30min session
- [ ] Save/load < 50ms average
- [ ] Game Center auth works 100% on device
- [ ] Leaderboards display correctly
- [ ] Anti-cheat blocks obvious cheats
- [ ] No crashes in 100+ test sessions
- [ ] Memory usage stable (no leaks)
- [ ] All legacy data migrated successfully

**Risks**:
- ⚠️ Boss death pattern might feel awkward (needs iteration)
- ⚠️ Battery drain might exceed 5% (optimize if needed)
- ⚠️ Edge cases might reveal bugs

---

## Dependency Graph

```
Week 1: Foundations
├── SaveManager.swift ──────────┐
│   ├── Codable Models          │
│   ├── JSON Encoder            │
│   └── Legacy Migration        │
│                                │
└── HapticManager.swift         │
    ├── Generator Pooling        │
    └── C++ Bridge               │
                                 │
Week 2: Settings & Basic Haptics│
├── UserDefaults Migration ◄────┘
│   └── Settings UI             
│                                
└── PlayerController Haptics    
    ├── Jump/Shoot/Death        
    └── Collision/Pickup         
                                 
Week 3: Game Center & Hats      
├── App Store Connect ──────────┐
│   └── Leaderboard Config      │
│                                │
├── GameCenterManager ◄─────────┘
│   └── Authentication          
│                                
└── Hat System Unification      
    └── Merge into SaveManager   
                                 
Week 4: Leaderboards            
├── Score Submission ◄──────────┐ (requires Week 3)
│   ├── iOS 14+ API             │
│   └── iOS 10-13 Fallback      │
│                                │
└── Leaderboard UI ◄────────────┘
    ├── Main Menu Button        
    └── Level Complete Button    
                                 
Week 5: Anti-Cheat              
├── Checkpoint Validation ◄─────┐ (requires Week 4)
│   ├── Session Hashing         │
│   └── Proof Generation        │
│                                │
└── Advanced Validation ◄───────┘
    ├── Rate Limiting           
    ├── Bounds Checking         
    └── Validation Flags         
                                 
Week 6: Polish                  
├── Core Haptics ◄──────────────┐ (requires Week 2)
│   └── Boss Death Pattern      │
│                                │
├── UI Haptics ◄────────────────┘
│   └── Menu/Shop Feedback      
│                                
└── Comprehensive Testing ◄─────┐ (requires ALL)
    ├── System Tests            │
    └── Edge Case Tests ◄───────┘
```

**Critical Dependencies**:
1. SaveManager must exist before Hat migration (Week 3)
2. Game Center auth must work before score submission (Week 4)
3. Score submission must work before anti-cheat (Week 5)
4. All systems must exist before final testing (Week 6)

---

## Critical Path

**Longest dependency chain**: 6 weeks (sequential)

### Can Be Parallelized

If working with 2+ developers:

**Developer A** (Serialization & Game Center):
- Week 1: SaveManager + Legacy Migration
- Week 2: Settings Migration
- Week 3: Game Center Setup + Hat Unification
- Week 4: Score Submission + Leaderboard UI
- Week 5: Anti-Cheat Implementation
- Week 6: Testing

**Developer B** (Haptics):
- Week 1: HapticManager Foundation
- Week 2: Player Action Haptics + Collision/Pickup
- Week 3: (Help with Game Center testing)
- Week 4: (Help with Leaderboard UI)
- Week 5: (Help with Anti-Cheat)
- Week 6: Core Haptics + UI Haptics + Boss Integration

**Time Savings**: 2-3 weeks (down to 3-4 weeks total)

---

## Testing Milestones

### Milestone 1: Week 2 - Basic Functionality
**Acceptance Criteria**:
- ✅ Legacy saves migrate to JSON format
- ✅ Settings stored in UserDefaults
- ✅ Jump haptic feels responsive
- ✅ No data loss in 20 consecutive save/load cycles
- ✅ Frame rate stays 60 FPS

**Go/No-Go Decision**: If data loss occurs, STOP and fix before proceeding.

---

### Milestone 2: Week 4 - Game Center Live
**Acceptance Criteria**:
- ✅ Game Center authentication 100% on device
- ✅ Scores submit and appear in leaderboard
- ✅ Leaderboard UI opens from main menu
- ✅ Offline scores cache and submit later
- ✅ All 6 level leaderboards functional

**Go/No-Go Decision**: If Game Center unreliable, STOP and debug.

---

### Milestone 3: Week 6 - Production Ready
**Acceptance Criteria**:
- ✅ All systems working together
- ✅ Zero crashes in 100+ test sessions
- ✅ Battery drain acceptable (< 5% per 30min)
- ✅ Anti-cheat blocks obvious cheats
- ✅ Haptics enhance experience (playtester feedback)

**Go/No-Go Decision**: If crashes persist, delay launch.

---

## Risk Mitigation

### High-Priority Risks

#### Risk 1: Game Center Sandbox Issues
**Probability**: High  
**Impact**: Medium  
**Mitigation**:
- Create 3+ sandbox accounts early
- Test on physical device daily
- Have fallback: disable Game Center if broken
- Monitor Apple Developer Forums for known issues

#### Risk 2: Legacy Save Migration Failures
**Probability**: Medium  
**Impact**: High (data loss)  
**Mitigation**:
- Test with 50+ real legacy save files
- Always backup before migration
- Add rollback mechanism (restore from .v1_backup)
- Log all migration attempts for debugging

#### Risk 3: Haptic Performance Impact
**Probability**: Low  
**Impact**: Medium  
**Mitigation**:
- Profile early with Instruments
- Add settings toggle to disable
- Optimize generator pooling
- Debounce rapid events

#### Risk 4: Anti-Cheat False Positives
**Probability**: Medium  
**Impact**: High (blocking legitimate players)  
**Mitigation**:
- Start with lenient validation
- Log all rejected scores (don't just silently drop)
- Allow score submission without proof (warning only)
- Iterate based on real-world data

---

## Rollout Strategy

### Phase 1: Internal Alpha (Week 6)
**Audience**: Developer + 2 testers  
**Features**: All systems enabled  
**Duration**: 3 days  
**Goal**: Find critical bugs

**Exit Criteria**:
- No crashes in 10 hours of playtime
- Save/load reliable
- Haptics feel good (subjective)

---

### Phase 2: TestFlight Beta (Week 7-8)
**Audience**: 20-50 beta testers  
**Features**: All systems enabled  
**Duration**: 2 weeks  
**Goal**: Real-world validation

**Monitoring**:
- Crash reports (TestFlight Analytics)
- Save/load success rate
- Game Center authentication rate
- Battery drain reports (via feedback form)
- Anti-cheat rejection logs

**Exit Criteria**:
- < 1% crash rate
- > 95% authentication success
- < 5% battery drain per 30min (median)
- Zero data loss reports

---

### Phase 3: Production Launch (Week 9)
**Audience**: All users (App Store)  
**Features**: All systems enabled  
**Monitoring**: Same as beta + App Store reviews

**Rollback Plan**:
- If > 5% crash rate: disable new systems, revert to v1
- If data loss: emergency hotfix + compensation (coins)
- If Game Center broken: disable leaderboards, keep rest

---

## Post-Launch

### Week 10-12: Monitoring & Iteration

**Monitor**:
- [ ] Leaderboard top 100 for suspicious scores
- [ ] Anti-cheat rejection rate (should be < 1%)
- [ ] User reviews mentioning haptics/saves/leaderboards
- [ ] Crash analytics for new code paths

**Iterate**:
- [ ] Adjust haptic intensities based on feedback
- [ ] Tune anti-cheat thresholds (reduce false positives)
- [ ] Fix any edge case bugs discovered
- [ ] Optimize performance if needed

**Future Enhancements** (Phase 2):
- [ ] iCloud save sync (optional)
- [ ] Server-side score validation
- [ ] Admin dashboard for leaderboard moderation
- [ ] Achievements (if requested)
- [ ] Replay system (record gameplay proof)

---

## Success Metrics

**Quantitative**:
- Crash-free rate > 99%
- Save/load success rate > 99.9%
- Game Center auth rate > 90% (of eligible devices)
- Battery drain < 5% per 30min (median)
- Frame rate maintained at 60 FPS (95th percentile)
- Leaderboard engagement: > 20% of players view at least once

**Qualitative**:
- App Store reviews mention haptics positively
- No user complaints about lost data
- Competitive leaderboard participation
- Minimal cheating reports

---

## Contingency Plans

### If Week 1 Runs Over (SaveManager Complex)
- **Action**: Skip hat migration (Week 3), do in Phase 2
- **Impact**: Hats save separately (not ideal but works)

### If Week 3 Game Center Blocked (App Store Connect Delay)
- **Action**: Continue with other work, mock Game Center for testing
- **Impact**: 1-2 day delay to Week 4

### If Week 5 Anti-Cheat Too Complex
- **Action**: Ship with basic validation only (checksum + rate limit)
- **Impact**: More cheating initially, iterate post-launch

### If Week 6 Testing Reveals Critical Bug
- **Action**: Delay launch 1 week, fix bug, retest
- **Impact**: Launch delayed but quality maintained

---

## Developer Checklist

### Before Starting Week 1
- [ ] Read all three research docs thoroughly
- [ ] Set up physical iOS device for testing
- [ ] Create App Store Connect account
- [ ] Install Xcode Instruments (for profiling)
- [ ] Back up current working build
- [ ] Create feature branch: `feature/haptics-saves-gamecenter`

### Before Starting Week 3
- [ ] Register Game Center leaderboards (24hr approval)
- [ ] Create 2 sandbox test accounts
- [ ] Verify physical device available for testing

### Before Week 6 Testing
- [ ] Gather 50+ legacy save files for migration test
- [ ] Recruit 2-3 playtesters for subjective feedback
- [ ] Set up TestFlight app
- [ ] Create test plan document

### Before Production Launch
- [ ] All tests passing
- [ ] Code reviewed
- [ ] Analytics integrated
- [ ] Crash reporting enabled (Firebase/Sentry)
- [ ] Support email set up for bug reports
- [ ] Rollback plan documented

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Next Review**: End of each week during implementation

---

## Quick Reference

### Key Files to Create
- `FloppyTurd/src/iOS/Managers/SaveManager.swift`
- `FloppyTurd/src/iOS/Managers/HapticManager.swift`
- `FloppyTurd/src/iOS/Managers/GameCenterManager.swift`
- `FloppyTurd/src/iOS/Managers/KeychainManager.swift`
- `FloppyTurd/src/Engine/Platform/PlatformDelegates.h` (modify)

### Key Files to Modify
- `FloppyTurd/src/FloppyTurd/Game/FloppyTurdGame.cpp` (save/load delegation)
- `FloppyTurd/src/FloppyTurd/States/MainMenuState.cpp` (Quit → Leaderboard)
- `FloppyTurd/src/FloppyTurd/Systems/PlayerControllerSystem.cpp` (haptics)
- `FloppyTurd/src/FloppyTurd/Systems/ObstacleSystem.cpp` (checkpoints)
- `FloppyTurd/src/FloppyTurd/Systems/HatsSystem.cpp` (use SaveManager)

### Test Commands
```bash
# Build for simulator
cmake -B build_ios26 -G Xcode -DCMAKE_TOOLCHAIN_FILE=ios-cmake-master/ios.toolchain.cmake -DPLATFORM=SIMULATOR64

# Build for device
cmake -B build_ios_device -G Xcode -DCMAKE_TOOLCHAIN_FILE=ios-cmake-master/ios.toolchain.cmake -DPLATFORM=OS64

# Run tests (if using XCTest)
xcodebuild test -scheme FloppyTurd -destination 'platform=iOS Simulator,name=iPhone 16'

# Profile with Instruments
# 1. Build in Xcode
# 2. Product → Profile (⌘I)
# 3. Choose "Time Profiler" or "Energy Log"
```

### Useful iOS Settings Paths
```
Game Center Sandbox: Settings → Game Center → Sandbox Account
Developer Options: Settings → Developer (requires dev profile)
Storage: Settings → General → iPhone Storage → FloppyTurd
```
