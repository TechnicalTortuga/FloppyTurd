# FloppyTurd Coin System Documentation

## Overview
This document details all coin-related values in the FloppyTurd game, their purposes, reset behavior, storage locations, and persistence characteristics.

## Core Coin Values

### 1. **PlayerComponent.sessionCoins**
- **Purpose**: Coins collected during the current level attempt
- **Data Type**: `int` (PlayerComponent member)
- **Reset Behavior**:
  - ✅ **Resets to 0** when player dies and chooses "Try Again" (`ResetPlayerEntity()`)
  - ✅ **Resets to 0** when starting a new level (`SetLevel()`)
  - ✅ **Resets to 0** at game initialization (`Initialize()`)
- **Storage**: Transient (not saved to disk)
- **Persistence**: Lost when app closes or level ends
- **Usage**: Displayed as "Session Coins" in pause menu stats

### 2. **PlayerComponent.totalCoins** (STORED COINS)
- **Purpose**: Coins stored between sessions (persistent spendable coins)
- **Data Type**: `int` (PlayerComponent member)
- **Reset Behavior**:
  - ❌ **NEVER resets** during gameplay
  - ❌ **NEVER resets** when player dies
  - ❌ **NEVER resets** when starting new level
  - ✅ **ONLY resets** during `ResetPlayerEntity()` was a BUG (FIXED)
- **Storage**: Loaded from `GameStats.storedCoins` on game start
- **Persistence**: Saved to disk in `GameStats.storedCoins`
- **Usage**: Combined with session coins for "Current Total Coins" display

### 3. **GameplayState.m_sessionCoinsCollected**
- **Purpose**: Counter for coins collected in current level attempt (used for stats tracking)
- **Data Type**: `int` (GameplayState member)
- **Reset Behavior**:
  - ✅ **Resets to 0** in `TryAgain()` method
  - ✅ **Resets to 0** in `SetLevel()` method (new level)
  - ✅ **Resets to 0** in `Initialize()` method (game start)
- **Storage**: Transient (not saved to disk)
- **Persistence**: Lost when app closes
- **Usage**: Passed to PauseSystem for stats display

### 4. **GameStats.storedCoins**
- **Purpose**: Persistent storage of player's stored coins between sessions
- **Data Type**: `int` (GameStats struct member)
- **Reset Behavior**:
  - ❌ **NEVER resets** during normal gameplay
  - ✅ **Increases** when session coins are converted after death
  - ✅ **Decreases** when coins are spent on hats/skills
- **Storage**: Saved/loaded via binary serialization in `SaveGameData`/`LoadGameData`
- **Persistence**: Persists between app sessions
- **Usage**: Source of truth for stored coins, synced with `PlayerComponent.totalCoins`

### 5. **GameStats.totalCoinsCollected** (GROSS TOTAL)
- **Purpose**: Lifetime total of all coins ever collected by player
- **Data Type**: `int` (GameStats struct member)
- **Reset Behavior**:
  - ❌ **NEVER resets**
  - ✅ **Increases** every time ANY coin is collected
  - ✅ **Increases** when session coins are converted to stored after death
  - ✅ **Increases** when returning to menu after quitting
- **Storage**: Saved/loaded via binary serialization in `SaveGameData`/`LoadGameData`
- **Persistence**: Persists between app sessions
- **Usage**: Displayed as "Gross Total Coins" in pause menu stats

### 6. **FloppyTurdGame.m_playerCoins** (LEGACY - REMOVED)
- **Purpose**: Legacy coin storage - **REMOVED** from save/load operations
- **Data Type**: `int` (FloppyTurdGame member)
- **Reset Behavior**: **NO LONGER USED**
- **Storage**: **REMOVED** from save/load operations
- **Persistence**: **NO LONGER PERSISTS**
- **Usage**: **DEPRECATED** - `GetPlayerCoins()` now returns `GameStats.storedCoins`

### 7. **GameplayState.m_sessionCoinsCollected**
- **Purpose**: Counter for coins collected in current level attempt (for stats display)
- **Data Type**: `int` (GameplayState member)
- **Reset Behavior**:
  - ✅ **Resets to 0** in `TryAgain()` method
  - ✅ **Resets to 0** in `SetLevel()` method (new level)
  - ✅ **Resets to 0** at game initialization (`Initialize()`)
- **Storage**: Transient (not saved to disk)
- **Persistence**: Lost when app closes
- **Usage**: Passed to PauseSystem for session coins display

## Pipe-Related Values

### 7. **GameplayState.m_pipesCleared**
- **Purpose**: Pipes cleared in current level attempt
- **Data Type**: `int` (GameplayState member)
- **Reset Behavior**:
  - ✅ **Resets to 0** in `TryAgain()` method
  - ✅ **Resets to 0** in `SetLevel()` method (new level)
  - ✅ **Resets to 0** in `Initialize()` method (game start)
- **Storage**: Transient (not saved to disk)
- **Persistence**: Lost when app closes
- **Usage**: Displayed as "Session Pipes" in pause menu stats

### 8. **GameStats.totalPipesCleared**
- **Purpose**: Lifetime total of all pipes cleared across all sessions
- **Data Type**: `int` (GameStats struct member)
- **Reset Behavior**:
  - ❌ **NEVER resets**
  - ✅ **Increases** every time a pipe is cleared (in `OnPipeCleared()`)
- **Storage**: Saved/loaded via binary serialization in `SaveGameData`/`LoadGameData`
- **Persistence**: Persists between app sessions
- **Usage**: Displayed as "Total Pipes" in pause menu stats

## Other Values

### 9. **GameStats.totalDeaths**
- **Purpose**: Total number of player deaths across all sessions
- **Data Type**: `int` (GameStats struct member)
- **Reset Behavior**:
  - ❌ **NEVER resets**
  - ✅ **Increases** by 1 every time player dies
- **Storage**: Saved/loaded via binary serialization
- **Persistence**: Persists between app sessions
- **Usage**: Displayed as "Total Flops" in pause menu stats

### 10. **GameStats.totalEnemiesKilled**
- **Purpose**: Total enemies defeated across all sessions
- **Data Type**: `int` (GameStats struct member)
- **Reset Behavior**:
  - ❌ **NEVER resets**
  - ✅ **Increases** when enemies are defeated
- **Storage**: Saved/loaded via binary serialization
- **Persistence**: Persists between app sessions
- **Usage**: Displayed as "Enemies Defeated" in pause menu stats

## Reset Timing & Triggers

### Level Reset (Try Again / New Level)
**Triggered by**: Player death → Game Over → Try Again button, OR starting new level
**Values that reset to 0**:
- ✅ `PlayerComponent.sessionCoins`
- ✅ `GameplayState.m_sessionCoinsCollected`
- ✅ `GameplayState.m_pipesCleared`
- ✅ `GameplayState.m_currentScore`
- ✅ Various game state timers and counters

**Values that DO NOT reset**:
- ❌ `PlayerComponent.totalCoins` (stored coins)
- ❌ `GameStats.storedCoins`
- ❌ `GameStats.totalCoinsCollected` (gross total)
- ❌ `GameStats.totalPipesCleared`
- ❌ `GameStats.totalDeaths`
- ❌ `GameStats.totalEnemiesKilled`

### Game Start
**Triggered by**: App launch
**Behavior**: All persistent values loaded from disk, session values initialized to 0

## Purchasing Logic

### Coin Availability Check
```cpp
// Total coins available for purchasing = stored + session
int totalAvailableCoins = player->totalCoins + player->sessionCoins;
```

### Deduction Priority (UPDATED)
1. **First**: Use ALL available `PlayerComponent.sessionCoins` (session coins)
2. **Then**: Use `PlayerComponent.totalCoins` (stored coins) for remaining cost
3. **Sync**: Update `GameStats.storedCoins` to match `PlayerComponent.totalCoins`

### Example Purchase Flow
```
Player has: 5 stored coins + 3 session coins = 8 total available
Current Total displays: 8 (stored + session)
Purchasing item costing 7 coins:
- Step 1: Use ALL 3 session coins (session = 0, remaining cost = 4)
- Step 2: Use 4 stored coins (stored = 1, remaining cost = 0)
- Update GameStats.storedCoins = 1
- Current Total immediately updates to: 1 (1 stored + 0 session)
- Result: Purchase successful, stats update instantly
```

### Edge Case Handling
```
Player has: 2 stored coins + 6 session coins = 8 total available
Purchasing item costing 7 coins:
- Step 1: Use ALL 6 session coins (session = 0, remaining cost = 1)
- Step 2: Use 1 stored coin (stored = 1, remaining cost = 0)
- Result: 1 stored + 0 session = 1 total remaining

If purchasing item costing 10 coins (more than available):
- Step 1: Use ALL 6 session coins (session = 0, remaining cost = 4)
- Step 2: Use ALL 2 stored coins (stored = 0, remaining cost = 2)
- ERROR: Purchase incomplete - 2 coins short
```

## Critical Bugs Fixed

### 1. **Stored Coins Reset Bug** (FIXED)
- **Problem**: `ResetPlayerEntity()` was resetting `player->totalCoins = 0`
- **Impact**: Stored coins disappeared when trying again after death
- **Fix**: Removed the line that resets stored coins

### 1.5 **Session Coins Reset Consistency** (FIXED)
- **Problem**: `SetLevel()` was not resetting `player->sessionCoins` when starting new levels
- **Impact**: Session coins could carry over between levels unintentionally
- **Fix**: Added `player->sessionCoins = 0` in `SetLevel()` method

### 2. **Double Deduction Bug** (FIXED)
- **Problem**: Both `SkillSystem.UnlockSkill()` and `PauseSystem` were deducting coins
- **Impact**: Players lost twice as many coins as expected
- **Fix**: Removed coin deduction from `SkillSystem.UnlockSkill()`

### 3. **Stats Display Bug** (FIXED)
- **Problem**: "Current Total" showed session + stored instead of just stored
- **Impact**: Confusing stats display
- **Fix**: Changed to pass only stored coins to stats display

### 4. **Purchasing Priority Reversal** (FIXED)
- **Problem**: Purchasing logic used stored coins first, then session coins
- **Impact**: Session coins accumulated instead of being spent first
- **Fix**: Reversed priority to use ALL session coins first, then stored coins

### 5. **Legacy Coin System Cleanup** (FIXED)
- **Problem**: `FloppyTurdGame.m_playerCoins` was redundant and caused confusion
- **Impact**: Multiple coin sources, potential sync issues
- **Fix**: Removed from save/load, redirected methods to use stored coins

### 6. **Double Counting Prevention** (FIXED)
- **Problem**: Session coins added to both stored and legacy coin pools
- **Impact**: Coins counted twice, inflated totals
- **Fix**: Unified all coin operations to use stored coins as single source of truth

### 7. **Gross Total Double Counting** (FIXED)
- **Problem**: Death finality and menu return both added to gross total
- **Impact**: Gross total showed 8 instead of 5, 11 instead of 7
- **Fix**: Only OnCoinCollected updates gross total; death/menu finality only converts to stored

### 8. **Stats Display Logic Gates** (FIXED)
- **Problem**: Pipes/stats only updated when coins collected, not when pipes cleared
- **Impact**: Session pipes/total pipes not updating in real-time
- **Fix**: OnPipeCleared now calls UpdateStatsData to refresh pause menu

### 9. **Session Reset Logic** (FIXED)
- **Problem**: TryAgain didn't update pause system with reset values
- **Impact**: Session values didn't appear to reset in pause menu
- **Fix**: Added UpdateStatsData call in TryAgain after all resets complete

### 10. **Real-Time Stats Updates** (FIXED)
- **Problem**: Current Total coins didn't update immediately after purchases
- **Impact**: Users had to exit/re-enter pause menu to see updated coin counts
- **Fix**: Added immediate UpdateStatsData calls after all purchases with tab-specific refresh logic

### 11. **Cross-Tab Synchronization** (FIXED)
- **Problem**: Purchasing on one tab didn't update displays on other tabs
- **Impact**: Switching tabs showed stale coin information
- **Fix**: Added tab-aware refresh logic that updates current tab immediately after purchases

### 12. **Session Coin Display Updates** (FIXED)
- **Problem**: Session coin count in stats didn't update immediately after purchases
- **Impact**: Stats showed old session coin values even after spending them
- **Fix**: Post-purchase updates now use `playerComp->sessionCoins` instead of cached PauseSystem values

## Data Flow Diagram

```
Coin Collection:
OnCoinCollected(value) ──► player->sessionCoins += value
                      ├─► m_sessionCoinsCollected += value
                      ├─► GameStats.totalCoinsCollected += value (GROSS TOTAL)
                      └─► UpdateStatsData(totalSpendableCoins) → Refresh "Current Total"

Pipe Clearing:
OnPipeCleared() ──► m_pipesCleared += 1
               ├─► GameStats.totalPipesCleared += 1
               └─► UpdateStatsData(totalSpendableCoins) → Refresh "Current Total"

Death Finality:
TriggerGameOver() ──► GameStats.storedCoins += sessionCoins (convert to stored)
                  └─► player->sessionCoins = 0 (after conversion)
                  ❌ NO gross total update (already counted when collected)

Menu Return:
ReturnToMainMenu() ──► GameStats.storedCoins += sessionCoins (convert to stored)
                   └─► player->sessionCoins = 0 (after conversion)
                   ❌ NO gross total update (already counted when collected)

Purchasing (Session-First Priority):
PauseSystem ──► Use ALL sessionCoins first, then storedCoins
           ├─► player->sessionCoins -= amount
           ├─► player->totalCoins -= remainder
           ├─► GameStats.storedCoins = player->totalCoins
           └─► UpdateStatsData(updatedSessionCoins, totalSpendableCoins) + Refresh Tab → All displays update immediately

Level Reset:
TryAgain() ──► player->sessionCoins = 0
          ├─► m_sessionCoinsCollected = 0
          ├─► m_pipesCleared = 0
          ├─► player->totalCoins (UNCHANGED - stored persist)
          └─► UpdateStatsData() → Refresh pause menu with zeros

Real-Time Updates:
Purchase Success ──► UpdateStatsData(updatedSessionCoins, updatedTotal) immediately
                 ├─► RefreshStatsDisplay() (if on stats tab - shows correct session coins)
                 ├─► UpdateHatDisplay() (if on hats tab)
                 └─► UpdateSkillDisplay() (if on skills tab)

Pause Menu Open ──► UpdateStatsData(totalSpendableCoins) with current values
               └─► Show "Current Total" = stored + session immediately
```

## Recommendations

1. **Remove `FloppyTurdGame.m_playerCoins`** - It's redundant and causes confusion
2. **Always use `PlayerComponent.totalCoins`** for stored coins display
3. **Always sync `GameStats.storedCoins`** with `PlayerComponent.totalCoins` after purchases
4. **Never reset stored coins** during level resets or try-again scenarios
5. **Test purchasing** with combinations of stored + session coins to ensure correct deduction priority

---

*Last Updated: $(date)*
*Document Version: 1.0*
