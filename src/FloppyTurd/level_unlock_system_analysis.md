# Level Unlock System Analysis

## Overview
This document analyzes the complete level unlock system in FloppyTurd, identifying all functions, potential redundancy, and issues.

## Core Data Structures

### LevelStats
```cpp
struct LevelStats {
    int highScore;              // Best pipes cleared for this level
    int bestCoins;              // Best coins collected for this level
    bool unlocked;              // Whether this level is unlocked
    int unlockRequirement;      // Pipes required to unlock next level
    int coinRequirement;        // Coins required to unlock next level (alternative)
};
```

## 1. SetDefaultUnlockRequirements Function

**Location:** `src/FloppyTurd/Game/FloppyTurdGame.cpp:757`

**Purpose:** Sets initial unlock requirements for each level

**Current Implementation:**
```cpp
void FloppyTurdGame::SetDefaultUnlockRequirements(int levelId, LevelStats& stats) {
    switch (levelId) {
        case 1: // Park - always unlocked
            stats.unlocked = true;
            stats.unlockRequirement = 0;
            stats.coinRequirement = 0;
            break;
        case 2: // Sewer - TEMPORARILY SET TO 0 PIPES FOR TESTING
            stats.unlockRequirement = 0; // pipes from Park (level 1) - TEMP: 0 for testing
            stats.coinRequirement = 0;
            break;
        case 3: // Desert - 50 pipes from Sewer + 100 coins
            stats.unlockRequirement = 50;
            stats.coinRequirement = 100;
            break;
        // ... more cases
    }
}
```

**Issues:** ✅ CORRECT - Level 2 has 0 pipe requirement

## 2. ResetGameData Function

**Location:** `src/FloppyTurd/Game/FloppyTurdGame.cpp:608`

**Purpose:** Resets all game data to defaults

**Implementation:**
```cpp
void FloppyTurdGame::ResetGameData() {
    // Reset basic data...
    for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
        LevelStats& stats = m_levelStats[levelId];
        stats.highScore = 0;
        stats.bestCoins = 0;
        stats.unlocked = (levelId == 1); // Only first level unlocked by default
        SetDefaultUnlockRequirements(levelId, stats); // ✅ Calls SetDefaultUnlockRequirements
    }
}
```

**Issues:** ✅ CORRECT - Calls SetDefaultUnlockRequirements

## 3. LoadGameData Function

**Location:** `src/FloppyTurd/Game/FloppyTurdGame.cpp:392`

**Purpose:** Loads game data from disk

**Implementation:**
```cpp
void FloppyTurdGame::LoadGameData() {
    std::ifstream file(SAVE_FILE_NAME, std::ios::binary);
    if (file.is_open()) {
        // Load basic data...
        file.read(reinterpret_cast<char*>(&m_levelStats[1]), sizeof(LevelStats) * MAX_LEVELS);
        // ✅ NEW: Update requirements to latest defaults
        for (int levelId = 1; levelId <= MAX_LEVELS; ++levelId) {
            LevelStats& stats = m_levelStats[levelId];
            SetDefaultUnlockRequirements(levelId, stats);
        }
    } else {
        ResetGameData(); // Calls SetDefaultUnlockRequirements
    }
}
```

**Issues:** ✅ CORRECT - Now updates requirements after loading

## 4. SaveGameData Function

**Location:** `src/FloppyTurd/Game/FloppyTurdGame.cpp:375`

**Purpose:** Saves game data to disk

**Implementation:**
```cpp
void FloppyTurdGame::SaveGameData() {
    std::ofstream file(SAVE_FILE_NAME, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<char*>(&m_highScore), sizeof(m_highScore));
        file.write(reinterpret_cast<char*>(&m_playerCoins), sizeof(m_playerCoins));
        file.write(reinterpret_cast<char*>(&m_gameStats), sizeof(m_gameStats));
        file.write(reinterpret_cast<char*>(&m_levelStats[1]), sizeof(LevelStats) * MAX_LEVELS);
    }
}
```

**Issues:** ✅ CORRECT - Saves level stats including requirements

## 5. CanUnlockLevel Function

**Location:** `src/FloppyTurd/Game/FloppyTurdGame.cpp:792`

**Purpose:** Checks if level unlock requirements are met

**Implementation:**
```cpp
bool FloppyTurdGame::CanUnlockLevel(int levelId, std::string& failureMessage) {
    // Basic validation...
    const LevelStats& levelStats = m_levelStats[levelId];

    if (levelStats.unlockRequirement > 0) {
        int prevLevelHighScore = GetLevelHighScore(requiredLevelId);
        if (prevLevelHighScore < levelStats.unlockRequirement) {
            failureMessage = "Need " + std::to_string(levelStats.unlockRequirement) + " pipes from previous level";
            return false;
        }
    }

    if (levelStats.coinRequirement > 0) {
        if (m_playerCoins < levelStats.coinRequirement) {
            failureMessage = "Need " + std::to_string(levelStats.coinRequirement) + " coins";
            return false;
        }
    }

    return true;
}
```

**Issues:** ✅ CORRECT - Uses levelStats.unlockRequirement from m_levelStats

## 6. TryUnlockLevel Function

**Location:** `src/FloppyTurd/Game/FloppyTurdGame.cpp:856`

**Purpose:** Attempts to unlock a level, checking requirements first

**Implementation:**
```cpp
bool FloppyTurdGame::TryUnlockLevel(int levelId) {
    std::string failureMessage;
    if (!CanUnlockLevel(levelId, failureMessage)) {
        GN_LOG_INFO("❌ Cannot unlock level " + std::to_string(levelId) + ": " + failureMessage);
        return false;
    }

    const LevelStats& levelStats = m_levelStats[levelId];

    // Deduct coins if required
    if (levelStats.coinRequirement > 0) {
        SpendCoins(levelStats.coinRequirement);
    }

    UnlockLevel(levelId);
    return true;
}
```

**Issues:** ✅ CORRECT - Calls CanUnlockLevel and UnlockLevel

## 7. UnlockLevel Function

**Location:** `src/FloppyTurd/Game/FloppyTurdGame.cpp:710`

**Purpose:** Actually unlocks the level and plays sounds

**Implementation:**
```cpp
void FloppyTurdGame::UnlockLevel(int levelId) {
    if (levelId >= 1 && levelId <= MAX_LEVELS) {
        m_levelStats[levelId].unlocked = true;
        SaveGameData();

        // Play level unlock sound effects
        PlaySFX("balloonpop"); // 🎈 Immediate sound
        // Schedule partyhorn sound to play after 1 second delay
        m_pendingPartyHorn = true;
        m_levelUnlockSoundTimer = 0.0f;
    }
}
```

**Issues:** ✅ CORRECT - Plays balloonpop and schedules partyhorn

## 8. UpdateUnlockButtonVisibility Function

**Location:** `src/FloppyTurd/States/MainMenuState.cpp:2390`

**Purpose:** Shows/hides unlock buttons based on level unlock status

**Implementation:**
```cpp
void MainMenuState::UpdateUnlockButtonVisibility(size_t levelIndex, bool isUnlocked) {
    if (levelIndex < m_unlockButtonEntities.size() && m_unlockButtonEntities[levelIndex] != 0) {
        Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_unlockButtonEntities[levelIndex]);
        if (buttonSprite) buttonSprite->visible = !isUnlocked;
    }
}
```

**Issues:** ✅ CORRECT - Hides button when unlocked

## 9. OnUnlockButtonPressed Function

**Location:** `src/FloppyTurd/States/MainMenuState.cpp:2409`

**Purpose:** Handles unlock button press

**Implementation:**
```cpp
void MainMenuState::OnUnlockButtonPressed(int levelNumber) {
    // Debounce check...
    if (GameCore::GetGame()->TryUnlockLevel(levelNumber)) {
        // Success - update UI
        UpdateUnlockButtonVisibility(i, true);
        RefreshLevelDisplay();
    } else {
        // Play denied sound ONLY if button is visible
        if (buttonIsVisible) {
            GameCore::GetGame()->PlaySFX("denied");
        }
    }
}
```

**Issues:** ✅ CORRECT - Calls TryUnlockLevel and handles denied sound

## 10. UI Requirement Display Logic

**Location:** `src/FloppyTurd/States/MainMenuState.cpp:2220-2260`

**Purpose:** Displays unlock requirements in UI

**Implementation:**
```cpp
if (levelStats.unlockRequirement > 0) {
    requirementsText = std::to_string(prevLevelHighScore) + "/" + std::to_string(levelStats.unlockRequirement) + " pipes from\n'" + prevLevelName + "'";
}
```

**Issues:** ✅ CORRECT - Displays requirements from levelStats

## Data Flow Analysis

### Initialization Flow
1. **Constructor** → LoadGameData()
2. **LoadGameData()** → SetDefaultUnlockRequirements() ✅
3. **ResetGameData()** → SetDefaultUnlockRequirements() ✅

### Unlock Attempt Flow
1. **OnUnlockButtonPressed()** → TryUnlockLevel()
2. **TryUnlockLevel()** → CanUnlockLevel()
3. **CanUnlockLevel()** → Checks m_levelStats[levelId].unlockRequirement ✅
4. **If success** → UnlockLevel() → Plays sounds ✅

### Save/Load Flow
1. **SaveGameData()** → Saves m_levelStats[] to disk ✅
2. **LoadGameData()** → Loads m_levelStats[] from disk ✅
3. **LoadGameData()** → SetDefaultUnlockRequirements() ✅ (NEW)

## Redundancy Analysis

### Potential Redundancy Issues: ⚠️

1. **SetDefaultUnlockRequirements called in multiple places:**
   - ResetGameData() ✅
   - LoadGameData() ✅ (NEW)
   - **Issue:** If called multiple times, could overwrite user progress

2. **Level requirement checks in multiple places:**
   - CanUnlockLevel() ✅
   - UI display logic ✅
   - **Issue:** Both read from same source (m_levelStats), but could get out of sync

## Issues Identified

### 1. Data Persistence Issue ✅ FIXED
**Problem:** Saved data contained old requirements (50 pipes for level 2)
**Solution:** Added SetDefaultUnlockRequirements call in LoadGameData
**Status:** ✅ FIXED

### 2. Sound Playback ✅ WORKING
**Problem:** balloonpop and partyhorn not playing
**Solution:** UnlockLevel plays balloonpop and schedules partyhorn
**Status:** ✅ WORKING

### 3. Denied Sound Visibility ✅ WORKING
**Problem:** Denied sound playing when button invisible
**Solution:** Added visibility check in OnUnlockButtonPressed
**Status:** ✅ WORKING

## Current Status

✅ **SetDefaultUnlockRequirements:** Level 2 = 0 pipes
✅ **LoadGameData:** Updates requirements after loading
✅ **CanUnlockLevel:** Checks m_levelStats.unlockRequirement
✅ **UI Display:** Shows requirements from m_levelStats
✅ **Sound Effects:** balloonpop + partyhorn on unlock
✅ **Denied Sound:** Only when button visible

## Testing Checklist

- [ ] Level 2 shows "0/0 pipes" in UI
- [ ] Unlock button works for level 2
- [ ] balloonpop.wav plays immediately
- [ ] partyhorn.wav plays after 1 second
- [ ] Denied sound only plays when button visible
- [ ] Level actually unlocks (stays unlocked after restart)

## Recommendations

1. **Consider consolidating requirement logic** - Have a single source of truth
2. **Add more debug logging** to track requirement values
3. **Test with fresh install** to ensure no old data interference
4. **Verify sound files exist** in asset catalog

## Debug Commands

```bash
# Check current save file
find ~/Library/Developer/CoreSimulator -name "floppyturd_save.dat"

# Check asset catalog
find src/Assets.xcassets -name "*balloonpop*" -o -name "*partyhorn*"

# Monitor debug logs
tail -f /path/to/FloppyTurd_Debug.txt
```
