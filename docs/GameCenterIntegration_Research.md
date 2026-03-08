# Game Center Integration & Leaderboards - Research & Design Document

**Project**: Floppy Turd  
**System**: Game Center Integration with Anti-Cheat Leaderboards  
**Platform**: iOS 10.0+ (GameKit)  
**Date**: 2024  
**Status**: Research Phase

---

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Game Center Overview](#game-center-overview)
3. [Leaderboard Architecture](#leaderboard-architecture)
4. [Anti-Cheat Strategy](#anti-cheat-strategy)
5. [UI Integration](#ui-integration)
6. [Implementation Strategy](#implementation-strategy)
7. [Testing & Validation](#testing--validation)

---

## Executive Summary

### What We're Building
A secure, per-level leaderboard system integrated with iOS Game Center that:
- Tracks high scores for each of the 6 levels
- Prevents cheating through client-side validation
- Replaces the "Quit" button in Main Menu with "Leaderboard" access
- Validates scores before submission using hidden checkpoints
- Provides smooth authentication and offline support

### Current State
**Game Center Integration**: None  
**Leaderboards**: None  
**Authentication**: None  
**Anti-Cheat**: Basic checksum on save files only

**Main Menu Quit Button** (`MainMenuState.cpp`):
- Currently has "Quit" button that exits app
- On iOS, quitting apps is discouraged (users use home button)
- Prime candidate for replacement with "Leaderboard" button

### Goals
- **Competitive**: Per-level leaderboards encourage replayability
- **Secure**: Multi-layer anti-cheat prevents fake scores
- **Seamless**: Auto-authentication, graceful offline handling
- **Social**: Compare with friends, see global rankings
- **Privacy**: Respect player choice (can play without Game Center)
- **UX**: Native iOS leaderboard UI (no custom implementation needed)

---

## Game Center Overview

### What is Game Center?

Apple's social gaming network built into iOS, providing:
- **Player Authentication**: Single sign-on with Apple ID
- **Leaderboards**: Global and friend rankings
- **Achievements**: Not used in Floppy Turd (per requirements)
- **Multiplayer**: Not applicable
- **Cloud Save**: Not used (we handle our own saves)

### GameKit Framework

**Import**: `import GameKit`

**Key Classes**:
1. `GKLocalPlayer` - The authenticated player
2. `GKLeaderboard` - Leaderboard data and queries
3. `GKScore` - Individual score submissions
4. `GKGameCenterViewController` - Pre-built leaderboard UI
5. `GKLeaderboardEntry` - Player rank/score data (iOS 14+)

### Device Requirements

| Feature | iOS Version | Notes |
|---------|-------------|-------|
| Basic Game Center | iOS 4.1+ | Legacy API |
| Modern GKLeaderboard | iOS 14.0+ | Recommended API |
| Legacy Compatibility | iOS 10.0-13.9 | Fallback needed |
| Game Center App | Removed iOS 10 | Now in Settings app |

**Strategy**: Support iOS 10.0+ using availability checks for iOS 14+ features.

---

## Leaderboard Architecture

### Leaderboard Structure

#### Per-Level Leaderboards

**6 Leaderboards** (one per level):

```
Leaderboard ID               Display Name           Score Type    Sort Order
──────────────────────────────────────────────────────────────────────────────
com.floppyturd.level1       "Level 1: Tutorial"    Integer       High to Low
com.floppyturd.level2       "Level 2: Gold Flush"  Integer       High to Low
com.floppyturd.level3       "Level 3: Ice Throne"  Integer       High to Low
com.floppyturd.level4       "Level 4: Sewer"       Integer       High to Low
com.floppyturd.level5       "Level 5: Sky Castle"  Integer       High to Low
com.floppyturd.level6       "Level 6: Rat King"    Integer       High to Low
```

**Score Metric**: Pipes cleared (not coins, not time)

**Rationale**:
- Pipes cleared = primary skill metric
- Consistent across all levels
- Easy to understand ("I got to pipe 127!")
- Prevents confusion (coins can be missed)

#### Leaderboard Configuration (App Store Connect)

```json
{
  "leaderboards": [
    {
      "referenceID": "com.floppyturd.level1",
      "title": {
        "en": "Level 1: Tutorial"
      },
      "scoreFormat": "integer",
      "sortOrder": "highToLow",
      "scoreSubmissionType": "bestScore",
      "scoreRangeMin": 0,
      "scoreRangeMax": 999999,
      "recurring": false,
      "archived": false
    }
    // ... repeat for levels 2-6
  ]
}
```

**Setup Steps** (App Store Connect):
1. Login to [appstoreconnect.apple.com](https://appstoreconnect.apple.com)
2. Select Floppy Turd app
3. Features → Game Center → Leaderboards
4. Click "+" → "Single Leaderboard"
5. Configure each of 6 leaderboards
6. Save and wait for approval (~24 hours for first submission)

---

### Authentication Flow

#### Silent Authentication (Recommended)

```swift
import GameKit

class GameCenterManager {
    static let shared = GameCenterManager()
    
    private(set) var isAuthenticated = false
    private(set) var localPlayer: GKLocalPlayer?
    
    func authenticatePlayer(completion: @escaping (Bool) -> Void) {
        localPlayer = GKLocalPlayer.local
        
        localPlayer?.authenticateHandler = { [weak self] viewController, error in
            if let viewController = viewController {
                // Player needs to sign in - present modal
                self?.presentAuthenticationViewController(viewController)
                completion(false)
                
            } else if self?.localPlayer?.isAuthenticated == true {
                // Success!
                self?.isAuthenticated = true
                print("✅ Game Center authenticated: \(self?.localPlayer?.displayName ?? "Unknown")")
                completion(true)
                
            } else {
                // Failed or user declined
                self?.isAuthenticated = false
                if let error = error {
                    print("❌ Game Center auth failed: \(error.localizedDescription)")
                }
                completion(false)
            }
        }
    }
    
    private func presentAuthenticationViewController(_ viewController: UIViewController) {
        // Present from root view controller
        if let rootVC = UIApplication.shared.windows.first?.rootViewController {
            rootVC.present(viewController, animated: true)
        }
    }
}
```

**When to Authenticate**:
- ✅ On app launch (silent, non-blocking)
- ✅ When user taps "Leaderboard" (if not already authenticated)
- ❌ NOT on every level start (too intrusive)

**Handling Declined Authentication**:
- Allow gameplay without Game Center
- Show "Sign in to Game Center" prompt in leaderboard screen
- Don't block any features (leaderboards are optional)

---

### Score Submission

#### Basic Score Submission

```swift
func submitScore(_ score: Int, for levelId: Int, completion: @escaping (Bool, Error?) -> Void) {
    guard isAuthenticated else {
        print("⚠️ Not authenticated, skipping score submission")
        completion(false, NSError(domain: "GameCenter", code: -1, userInfo: [NSLocalizedDescriptionKey: "Not authenticated"]))
        return
    }
    
    let leaderboardID = "com.floppyturd.level\(levelId)"
    
    if #available(iOS 14.0, *) {
        // Modern API
        GKLeaderboard.submitScore(score, context: 0, player: GKLocalPlayer.local, leaderboardIDs: [leaderboardID]) { error in
            if let error = error {
                print("❌ Score submission failed: \(error.localizedDescription)")
                completion(false, error)
            } else {
                print("✅ Score submitted: \(score) to \(leaderboardID)")
                completion(true, nil)
            }
        }
        
    } else {
        // Legacy API (iOS 10-13)
        let scoreReporter = GKScore(leaderboardIdentifier: leaderboardID)
        scoreReporter.value = Int64(score)
        
        GKScore.report([scoreReporter]) { error in
            if let error = error {
                print("❌ Score submission failed: \(error.localizedDescription)")
                completion(false, error)
            } else {
                print("✅ Score submitted: \(score) to \(leaderboardID)")
                completion(true, nil)
            }
        }
    }
}
```

#### Offline Score Caching

**Problem**: Score submission fails if no internet  
**Solution**: GameKit automatically caches scores and retries

```swift
// No special handling needed - GameKit handles this automatically
// Scores are cached locally and submitted when connection restored
```

**Verification**:
- Scores submitted offline will appear with delay
- Check `GKScore.isUploaded` property (iOS 7+)
- Retry manually if needed (optional)

---

### Leaderboard Display

#### Pre-Built UI (Recommended)

```swift
func showLeaderboard(for levelId: Int) {
    guard isAuthenticated else {
        showAuthenticationPrompt()
        return
    }
    
    let leaderboardID = "com.floppyturd.level\(levelId)"
    
    if #available(iOS 14.0, *) {
        // Modern API - single leaderboard
        let viewController = GKGameCenterViewController(leaderboardID: leaderboardID, playerScope: .global, timeScope: .allTime)
        viewController.gameCenterDelegate = self
        presentViewController(viewController)
        
    } else {
        // Legacy API
        let viewController = GKGameCenterViewController()
        viewController.gameCenterDelegate = self
        viewController.viewState = .leaderboards
        viewController.leaderboardIdentifier = leaderboardID
        presentViewController(viewController)
    }
}

// Delegate method - called when user closes leaderboard
extension GameCenterManager: GKGameCenterControllerDelegate {
    func gameCenterViewControllerDidFinish(_ gameCenterViewController: GKGameCenterViewController) {
        gameCenterViewController.dismiss(animated: true)
    }
}
```

**UI Features** (provided by Apple):
- Global rankings (top 100)
- Friend rankings (if player has Game Center friends)
- Player's rank and score
- Time scopes: Today, Week, All Time (not applicable for us - use All Time only)
- Smooth scrolling, animations
- Automatic localization
- Dark mode support

#### All Levels View (Leaderboard Hub)

```swift
func showAllLeaderboards() {
    guard isAuthenticated else {
        showAuthenticationPrompt()
        return
    }
    
    if #available(iOS 14.0, *) {
        // Show Game Center dashboard with all leaderboards
        let viewController = GKGameCenterViewController(state: .leaderboards)
        viewController.gameCenterDelegate = self
        presentViewController(viewController)
        
    } else {
        // Legacy - defaults to all leaderboards
        let viewController = GKGameCenterViewController()
        viewController.gameCenterDelegate = self
        viewController.viewState = .leaderboards
        presentViewController(viewController)
    }
}
```

---

## Anti-Cheat Strategy

### Threat Landscape

**Attack Vectors**:
1. **Memory Hacking**: Modify score in RAM during gameplay (GameGem, iGameGuardian)
2. **Save File Editing**: Inflate high score in save file, then submit
3. **Network MitM**: Intercept and modify score submission packets
4. **Time Manipulation**: Slow down game time to achieve impossible scores
5. **Replay Attacks**: Submit same legitimate score multiple times
6. **Bot Scripts**: Automated perfect gameplay
7. **Jailbreak Tweaks**: Runtime manipulation of game logic

### Defense Layers

#### Layer 1: Client-Side Validation (Hidden Checkpoints)

**Concept**: During gameplay, collect cryptographic proofs that the player actually played the level.

**Implementation**:

```cpp
// In ObstacleSystem.cpp - when player passes obstacle

class ObstacleSystem {
private:
    std::vector<uint32_t> m_checkpointHashes;
    uint64_t m_sessionSeed;
    
public:
    void OnLevelStart(int levelId) {
        // Generate random session seed
        m_sessionSeed = GenerateRandomSeed();
        m_checkpointHashes.clear();
        
        // Store seed in Keychain (Swift delegate)
        if (m_platformDelegates && m_platformDelegates->save.saveSessionHash) {
            char seedStr[32];
            snprintf(seedStr, sizeof(seedStr), "%llu", m_sessionSeed);
            m_platformDelegates->save.saveSessionHash(levelId, seedStr);
        }
    }
    
    void OnObstaclePassed(int obstacleIndex, float playerY, float obstacleX) {
        // Create checkpoint hash from:
        // - Session seed (secret)
        // - Obstacle index (which pipe)
        // - Player Y position (proves player was actually there)
        // - Obstacle X position (proves timing)
        // - Game time (proves sequential passage)
        
        char hashInput[256];
        snprintf(hashInput, sizeof(hashInput), "%llu|%d|%.2f|%.2f|%.3f",
                 m_sessionSeed, obstacleIndex, playerY, obstacleX, GetGameTime());
        
        uint32_t checkpointHash = SimpleHash(hashInput);
        m_checkpointHashes.push_back(checkpointHash);
    }
    
    std::string GetValidationProof() {
        // Concatenate all checkpoint hashes
        std::string proof;
        for (uint32_t hash : m_checkpointHashes) {
            char hashStr[16];
            snprintf(hashStr, sizeof(hashStr), "%08x", hash);
            proof += hashStr;
        }
        return proof;
    }
};
```

**Validation on Score Submission**:

```swift
func submitScoreWithValidation(levelId: Int, score: Int, proof: String) {
    // 1. Retrieve session seed from Keychain
    guard let sessionSeed = KeychainManager.load(key: "session_\(levelId)") else {
        print("❌ No session seed - rejecting score")
        return
    }
    
    // 2. Verify proof length matches score
    // Each obstacle passed = 8 hex chars
    let expectedProofLength = score * 8
    guard proof.count == expectedProofLength else {
        print("❌ Proof length mismatch - rejecting score")
        return
    }
    
    // 3. Spot-check random checkpoints (expensive to verify all)
    let samplesToVerify = min(10, score) // Verify up to 10 random checkpoints
    var isValid = true
    
    for _ in 0..<samplesToVerify {
        let randomIndex = Int.random(in: 0..<score)
        let checkpointHashStr = proof.substring(start: randomIndex * 8, length: 8)
        
        // Recompute expected hash (would need game state - complex)
        // For now, just check it's valid hex
        if checkpointHashStr.range(of: "^[0-9a-f]{8}$", options: .regularExpression) == nil {
            isValid = false
            break
        }
    }
    
    guard isValid else {
        print("❌ Proof validation failed - rejecting score")
        return
    }
    
    // 4. Submit to Game Center
    submitScore(score, for: levelId) { success, error in
        if success {
            // Clear session seed (prevent resubmission)
            KeychainManager.delete(key: "session_\(levelId)")
        }
    }
}
```

**Limitations**:
- Sophisticated attacker can generate fake proofs if they reverse-engineer the hash algorithm
- Solution: Obfuscate hash algorithm, use multiple secret salts

#### Layer 2: Rate Limiting

**Concept**: Limit score submissions to prevent spam/brute-force

```swift
class RateLimiter {
    private var lastSubmissionTimes: [Int: Date] = [:] // levelId -> last submission
    
    func canSubmitScore(for levelId: Int) -> Bool {
        guard let lastTime = lastSubmissionTimes[levelId] else {
            return true // First submission
        }
        
        let timeSinceLastSubmission = Date().timeIntervalSince(lastTime)
        let minimumInterval: TimeInterval = 60 // 1 minute between submissions
        
        return timeSinceLastSubmission >= minimumInterval
    }
    
    func recordSubmission(for levelId: Int) {
        lastSubmissionTimes[levelId] = Date()
    }
}

// Usage
if !RateLimiter.shared.canSubmitScore(for: levelId) {
    print("⚠️ Rate limit exceeded - wait before resubmitting")
    return
}
```

#### Layer 3: Reasonable Bounds Checking

**Concept**: Reject obviously impossible scores

```swift
func isScoreReasonable(score: Int, levelId: Int, playTime: TimeInterval) -> Bool {
    // 1. Score can't be negative
    guard score >= 0 else { return false }
    
    // 2. Score can't exceed theoretical maximum
    // Assumption: Max ~500 pipes per level in 10 minutes
    guard score <= 1000 else { return false }
    
    // 3. Play time must be reasonable
    // Minimum: 0.5 seconds per pipe (superhuman)
    // Maximum: No limit (player can play forever)
    let minimumPlayTime = Double(score) * 0.5
    guard playTime >= minimumPlayTime else {
        print("❌ Score too high for play time - likely cheated")
        return false
    }
    
    // 4. First-time player can't get perfect score
    // (Check if this is player's first submission for this level)
    if isFirstSubmission(levelId) && score > 100 {
        // Suspicious but not impossible - flag for review
        print("⚠️ Unusually high first score: \(score)")
    }
    
    return true
}
```

#### Layer 4: Statistical Anomaly Detection

**Concept**: Flag scores that are statistical outliers

```swift
func detectAnomalies(score: Int, levelId: Int) -> Bool {
    // Fetch top scores from Game Center
    loadTopScores(levelId: levelId) { topScores in
        guard let topScores = topScores, topScores.count >= 10 else {
            return // Not enough data
        }
        
        // Calculate mean and standard deviation
        let mean = topScores.reduce(0, +) / topScores.count
        let variance = topScores.map { pow(Double($0 - mean), 2) }.reduce(0, +) / Double(topScores.count)
        let stdDev = sqrt(variance)
        
        // Flag if score is > 3 standard deviations above mean
        if Double(score) > Double(mean) + (3 * stdDev) {
            print("⚠️ Statistical anomaly detected: \(score) >> mean: \(mean)")
            // Don't reject, but log for manual review
        }
    }
    
    return true // Don't auto-reject based on stats alone
}
```

#### Layer 5: Device Fingerprinting

**Concept**: Track submissions per device to detect account farming

```swift
import UIKit

func getDeviceFingerprint() -> String {
    let deviceId = UIDevice.current.identifierForVendor?.uuidString ?? "unknown"
    let deviceModel = UIDevice.current.model
    let osVersion = UIDevice.current.systemVersion
    
    let fingerprint = "\(deviceId)|\(deviceModel)|\(osVersion)"
    
    // Hash for privacy
    let hash = fingerprint.data(using: .utf8)!.sha256()
    return hash.hexString()
}

// Store in Keychain on first launch
func setupDeviceFingerprint() {
    if KeychainManager.load(key: "device_fingerprint") == nil {
        let fingerprint = getDeviceFingerprint()
        KeychainManager.save(key: "device_fingerprint", value: fingerprint)
    }
}

// Include in score submission metadata
func submitScoreWithFingerprint(score: Int, levelId: Int) {
    guard let fingerprint = KeychainManager.load(key: "device_fingerprint") else {
        return
    }
    
    // GKScore.context field can store custom data (64-bit integer)
    // We can pack device fingerprint hash (first 8 bytes)
    let contextValue = fingerprint.prefix(16).hexToInt64() ?? 0
    
    if #available(iOS 14.0, *) {
        GKLeaderboard.submitScore(score, context: contextValue, player: GKLocalPlayer.local, leaderboardIDs: ["com.floppyturd.level\(levelId)"]) { error in
            // Handle submission
        }
    }
}
```

#### Layer 6: Flag-Based Validation

**Concept**: Hidden validation flags set during gameplay

```cpp
class ValidationFlags {
private:
    bool m_jumpedAtLeastOnce = false;
    bool m_collidedWithObstacle = false; // Suspicious if never hit anything
    int m_totalFrames = 0;
    int m_inputFrames = 0; // Frames with player input
    
public:
    void RecordJump() { m_jumpedAtLeastOnce = true; }
    void RecordCollision() { m_collidedWithObstacle = true; }
    void RecordFrame(bool hasInput) {
        m_totalFrames++;
        if (hasInput) m_inputFrames++;
    }
    
    bool IsValid() {
        // Must have jumped at least once
        if (!m_jumpedAtLeastOnce) return false;
        
        // Input must be present in at least 10% of frames (bots have 100%)
        float inputRatio = float(m_inputFrames) / float(m_totalFrames);
        if (inputRatio < 0.1 || inputRatio > 0.95) return false;
        
        // More checks...
        return true;
    }
    
    uint32_t GetFlagsHash() {
        char flagStr[128];
        snprintf(flagStr, sizeof(flagStr), "%d|%d|%d|%d",
                 m_jumpedAtLeastOnce, m_collidedWithObstacle, m_totalFrames, m_inputFrames);
        return SimpleHash(flagStr);
    }
};
```

### Combined Validation Flow

```
Player Completes Level
         │
         ▼
Collect Validation Data:
  - Checkpoint hashes (proof of playthrough)
  - Session seed (from Keychain)
  - Play time
  - Validation flags hash
  - Device fingerprint
         │
         ▼
Client-Side Checks:
  ✓ Proof length matches score
  ✓ Rate limit not exceeded
  ✓ Score is reasonable for play time
  ✓ Validation flags pass
         │
         ▼
Submit to Game Center:
  - Score value
  - Context field (device fingerprint)
  - Optional: Store proof in iCloud (future)
         │
         ▼
Game Center Records Score
         │
         ▼
Optional: Server-Side Validation (Future Phase)
  - Fetch proof from iCloud
  - Verify checkpoint hashes
  - Flag suspicious scores
  - Ban repeat offenders
```

### Manual Review System (Future)

For top scores, implement manual review:

1. **Top 10 Scores**: Require video proof (not implemented in v1)
2. **Suspicious Scores**: Admin dashboard to review flagged scores
3. **Community Reports**: Allow players to report cheaters
4. **Ban System**: Blacklist device fingerprints of confirmed cheaters

---

## UI Integration

### Main Menu Changes

**Current**: Main Menu has "Quit" button  
**New**: Replace "Quit" with "Leaderboard"

#### Rationale for Removing Quit Button

**iOS Human Interface Guidelines**:
> "Don't quit programmatically. An iOS app never quits because users press the Home button to leave an app."

**Best Practices**:
- iOS users expect to press Home button to exit
- Quit button is unnecessary and goes against platform conventions
- Better use of space: Leaderboard promotes engagement

#### UI Layout Change

**Before**:
```
┌─────────────────────────┐
│      FLOPPY TURD        │
├─────────────────────────┤
│      [  PLAY  ]         │
│      [ SHOP   ]         │
│      [OPTIONS ]         │
│      [ CREDITS]         │
│      [  QUIT  ]  ← Remove
└─────────────────────────┘
```

**After**:
```
┌─────────────────────────┐
│      FLOPPY TURD        │
├─────────────────────────┤
│      [  PLAY  ]         │
│      [ SHOP   ]         │
│      [OPTIONS ]         │
│      [ CREDITS]         │
│      [LEADERBOARD] ← New
└─────────────────────────┘
```

#### Implementation

**File**: `src/FloppyTurd/States/MainMenuState.cpp`

```cpp
// In CreateMenuButtons()

// Remove Quit button creation code
// Replace with:

CreateLeaderboardButton(centerX, quitY, buttonWidth, buttonHeight);

void MainMenuState::CreateLeaderboardButton(float x, float y, float width, float height) {
    Gnosis::Entity leaderboardButton = m_ecsCoordinator->CreateEntity();
    
    // ... set up button sprite, text, etc. (same as other buttons) ...
    
    uiElement.type = UIElementType::BUTTON;
    uiElement.text = "LEADERBOARD";
    uiElement.onClick = []() {
        // Call Swift delegate to show leaderboards
        if (g_Game && g_Game->GetPlatformDelegates().gameCenter.showLeaderboards) {
            g_Game->GetPlatformDelegates().gameCenter.showLeaderboards();
        }
    };
    
    m_ecsCoordinator->AddComponent(leaderboardButton, uiElement);
}
```

### Level Complete Screen

**Add**: "View Leaderboard" button after completing level

```cpp
// In LevelCompleteState.cpp

void LevelCompleteState::CreateUI() {
    // ... existing Continue/Retry buttons ...
    
    // New: View Leaderboard button
    CreateLeaderboardButtonForLevel(m_levelId, centerX, leaderboardY, buttonWidth, buttonHeight);
}

void LevelCompleteState::OnLeaderboardButtonPressed() {
    // Show leaderboard for this specific level
    if (m_platformDelegates.gameCenter.showLeaderboard) {
        m_platformDelegates.gameCenter.showLeaderboard(m_levelId);
    }
}
```

### In-Game Pause Menu

**Optional**: Add "View Leaderboard" to pause menu

```cpp
// In PauseSystem.cpp

// Add leaderboard option to pause menu (below Resume/Restart/Quit)
CreateLeaderboardButton();
```

---

### Platform Delegates Addition

**File**: `src/Engine/Platform/PlatformDelegates.h`

```cpp
struct GameCenterDelegate {
    // Authentication
    void (*authenticate)(void (*callback)(bool success));
    bool (*isAuthenticated)();
    
    // Score submission
    void (*submitScore)(int score, int levelId, const char* validationProof);
    
    // UI display
    void (*showLeaderboards)();  // All leaderboards
    void (*showLeaderboard)(int levelId);  // Specific level
    
    // Player info
    const char* (*getPlayerName)();
    const char* (*getPlayerId)();
    
    void* platformContext;
    
    GameCenterDelegate()
        : authenticate(nullptr)
        , isAuthenticated(nullptr)
        , submitScore(nullptr)
        , showLeaderboards(nullptr)
        , showLeaderboard(nullptr)
        , getPlayerName(nullptr)
        , getPlayerId(nullptr)
        , platformContext(nullptr) {}
};

// Add to PlatformDelegates
struct PlatformDelegates {
    RendererDelegate renderer;
    InputDelegate input;
    AudioDelegate audio;
    AssetDelegate asset;
    LogDelegate log;
    HapticDelegate haptic;
    SaveGameDelegate save;
    GameCenterDelegate gameCenter;  // NEW
    // ...
};
```

---

## Implementation Strategy

### Phase 1: Game Center Setup (Week 1)
**Goal**: Authentication and basic infrastructure

**Tasks**:
1. Create App Store Connect account (if not exists)
2. Register 6 leaderboards in App Store Connect
3. Create `GameCenterManager.swift` singleton
4. Implement authentication flow
5. Add GameCenterDelegate to PlatformDelegates.h
6. Test authentication on device (simulator doesn't support Game Center)

**Success Criteria**:
- Player can authenticate with Game Center
- Leaderboards visible in App Store Connect
- isAuthenticated() returns correct state

### Phase 2: Score Submission (Week 1)
**Goal**: Submit scores to Game Center

**Tasks**:
1. Implement submitScore() in GameCenterManager
2. Add score submission call to LevelCompleteState
3. Add offline caching handling
4. Test score submission and verify on Game Center dashboard
5. Test offline submission (Airplane mode)

**Success Criteria**:
- Scores submit successfully when online
- Scores cache and submit when connection restored
- Scores visible in Game Center app/settings

### Phase 3: Leaderboard UI (Week 2)
**Goal**: Display leaderboards

**Tasks**:
1. Replace Quit button with Leaderboard in MainMenuState
2. Add showLeaderboards() and showLeaderboard() implementations
3. Add "View Leaderboard" button to LevelCompleteState
4. Test leaderboard display (iOS 14+ and iOS 10-13 compatibility)
5. Handle "not authenticated" gracefully

**Success Criteria**:
- Leaderboard button opens Game Center UI
- All 6 level leaderboards accessible
- Works on iOS 10.0+ devices

### Phase 4: Anti-Cheat - Checkpoints (Week 2)
**Goal**: Basic validation

**Tasks**:
1. Add checkpoint hash generation to ObstacleSystem
2. Store session seed in Keychain on level start
3. Collect checkpoint hashes during gameplay
4. Pass validation proof to submitScore()
5. Validate proof before Game Center submission

**Success Criteria**:
- Checkpoints generated for each obstacle passed
- Proof string length matches score
- Invalid proofs rejected

### Phase 5: Anti-Cheat - Advanced (Week 3)
**Goal**: Multi-layer validation

**Tasks**:
1. Implement rate limiting
2. Add reasonable bounds checking
3. Add device fingerprinting
4. Implement validation flags (jump, input, etc.)
5. Test with modified save files (should reject)

**Success Criteria**:
- Rate limiter prevents spam submissions
- Impossible scores rejected
- Modified scores detected and blocked

### Phase 6: Testing & Polish (Week 3)
**Goal**: Production ready

**Tasks**:
1. Test on multiple devices (iPhone 8, 14, 16)
2. Test with jailbroken device (ensure still works)
3. Test offline/online transitions
4. Add analytics for submission success rate
5. Test with Game Center sandbox (TestFlight)
6. Submit for App Store review

**Success Criteria**:
- Works on all supported iOS versions
- No crashes in 100+ test sessions
- Leaderboards populated with test scores
- Anti-cheat passes basic tampering tests

---

## Testing & Validation

### Test Environment Setup

**Sandbox Testing**:
1. Go to Settings → Game Center → Sandbox Account
2. Create test accounts: testplayer1@example.com, testplayer2@example.com
3. Sign out of real Game Center account
4. Sign in with test account
5. Run app in Xcode or TestFlight

**Important**: Sandbox scores don't appear in production Game Center!

### Test Cases

#### Functional Tests
1. ✅ Authentication succeeds on first launch
2. ✅ Player can view all leaderboards
3. ✅ Player can view individual level leaderboard
4. ✅ Score submits after completing level
5. ✅ Offline score cached and submitted later
6. ✅ Higher score replaces lower score
7. ✅ Lower score doesn't replace higher score
8. ✅ Leaderboard shows player's rank
9. ✅ Leaderboard shows friend rankings

#### Anti-Cheat Tests
1. ✅ Score with no proof rejected
2. ✅ Score with wrong proof length rejected
3. ✅ Score exceeding max value rejected
4. ✅ Score submitted too quickly (rate limit) rejected
5. ✅ Modified save file score rejected
6. ✅ Impossible play time rejected

#### Edge Cases
1. ✅ No internet connection → score caches
2. ✅ User declines Game Center → game still playable
3. ✅ User has no friends → leaderboard shows global only
4. ✅ Player ranks outside top 100 → shows "Unranked"
5. ✅ Leaderboard empty (no scores yet) → shows placeholder

### Performance Benchmarks

```swift
func benchmarkScoreSubmission() {
    measure {
        submitScore(100, for: 1) { _, _ in }
    }
    // Target: < 500ms average
}

func benchmarkLeaderboardLoad() {
    measure {
        showLeaderboard(for: 1)
    }
    // Target: < 1000ms to present UI
}
```

### Monitoring & Analytics

**Track These Metrics**:
- Authentication success rate
- Score submission success rate
- Leaderboard view count
- Offline submission count
- Rejected score count (anti-cheat)
- Average time to authenticate

**Example** (using Firebase Analytics or similar):
```swift
Analytics.logEvent("gc_score_submitted", parameters: [
    "level_id": levelId,
    "score": score,
    "online": isOnline,
    "validated": isValidated
])
```

---

## Appendix

### App Store Connect Setup Checklist

- [ ] Login to App Store Connect
- [ ] Navigate to My Apps → Floppy Turd
- [ ] Features → Game Center
- [ ] Enable Game Center for app
- [ ] Create 6 leaderboards with IDs:
  - [ ] com.floppyturd.level1
  - [ ] com.floppyturd.level2
  - [ ] com.floppyturd.level3
  - [ ] com.floppyturd.level4
  - [ ] com.floppyturd.level5
  - [ ] com.floppyturd.level6
- [ ] Configure each leaderboard:
  - [ ] Title (e.g., "Level 1: Tutorial")
  - [ ] Score format: Integer
  - [ ] Sort order: High to Low
  - [ ] Score submission type: Best Score
- [ ] Wait for approval (24-48 hours first time)
- [ ] Create sandbox test accounts
- [ ] Test with TestFlight build

### Useful Resources

1. **Apple Documentation**:
   - [Game Center Programming Guide](https://developer.apple.com/documentation/gamekit)
   - [GKLeaderboard](https://developer.apple.com/documentation/gamekit/gkleaderboard)
   - [App Store Connect Game Center Setup](https://help.apple.com/app-store-connect/#/dev84b80958f)

2. **WWDC Sessions**:
   - WWDC 2020: "What's New in Game Center"
   - WWDC 2021: "Tap into Game Center: Leaderboards, Achievements, and Multiplayer"

3. **Anti-Cheat Resources**:
   - [Client-Side Anti-Cheat Best Practices](https://developer.apple.com/forums/tags/game-center)
   - [GameKit Security Overview](https://developer.apple.com/documentation/gamekit/secure-game-center-submissions)

### Known Limitations

1. **No Server-Side Validation**: Game Center doesn't provide server-side validation hooks. All validation is client-side.
2. **Context Field Limited**: Only 64-bit integer, can't store complex proof data.
3. **Sandbox Isolation**: Sandbox and production leaderboards are separate (can't test real rankings).
4. **Device Requirement**: Game Center doesn't work in iOS Simulator (must test on device).
5. **iCloud Dependency**: Requires Apple ID with iCloud enabled.

### Future Enhancements

1. **Phase 2: iCloud Proof Storage**: Store full checkpoint proofs in iCloud for server verification
2. **Phase 3: Admin Dashboard**: Web dashboard to review flagged scores
3. **Phase 4: Community Reporting**: Allow players to report suspicious scores
4. **Phase 5: Achievements**: Add achievements for milestones (if requested later)

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Next Review**: After Phase 3 implementation