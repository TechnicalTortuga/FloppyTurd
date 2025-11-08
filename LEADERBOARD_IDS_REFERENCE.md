# 🏆 Leaderboard IDs Quick Reference

**App**: Floppy Turd iOS  
**Bundle ID**: `com.floppyturd.game`  
**Updated**: November 7, 2025  

---

## 📋 Complete Leaderboard List (9 Total)

### Level-Based Leaderboards (Pipes Cleared) - 5 Total

| Level | Theme | Leaderboard ID | Reference Name |
|-------|-------|----------------|----------------|
| 1 | Park | `com.floppyturd.park` | Park - Pipes Cleared |
| 2 | Sewer | `com.floppyturd.sewer` | Sewer - Pipes Cleared |
| 3 | Desert | `com.floppyturd.desert` | Desert - Pipes Cleared |
| 4 | Snow | `com.floppyturd.snow` | Snow - Pipes Cleared |
| 5 | Castle | `com.floppyturd.castle` | Castle - Pipes Cleared |

**Score Format**: Integer  
**Sort Order**: High to Low (most pipes wins)  
**Submission**: Automatic on level completion  
**Leaderboard Type**: Classic (never resets)

---

### Boss Speedrun Leaderboard (Time-Based) - 1 Total

| Leaderboard ID | Reference Name | Score Format | Sort Order |
|----------------|----------------|--------------|------------|
| `com.floppyturd.ratking.time` | Rat King - Speedrun | Time (Elapsed) | Low to High |

**Score Metric**: Milliseconds  
**Submission**: Automatic on Rat King boss defeat  
**Goal**: Fastest time wins!  
**Leaderboard Type**: Classic (never resets)  
**Note**: ⚠️ Rat King has **NO PIPES** - only speedrun time!

---

### Cumulative Stats Leaderboards (All-Time) - 3 Total

| Stat | Leaderboard ID | Reference Name |
|------|----------------|----------------|
| Enemies Killed | `com.floppyturd.totalenemies` | Total Enemies Killed |
| Coins Collected | `com.floppyturd.totalcoins` | Total Coins Collected |
| Pipes Cleared | `com.floppyturd.totalpipes` | Total Pipes Cleared |

**Score Format**: Integer  
**Sort Order**: High to Low  
**Submission**: Updated continuously during gameplay  
**Leaderboard Type**: Classic (never resets)

---

## 🎯 Why Theme-Based IDs?

**Future-Proofing**: Using theme names (`park`, `sewer`, etc.) instead of level numbers (`level1`, `level2`) allows you to:

✅ Reorder levels in future updates without breaking leaderboards  
✅ Add new levels between existing ones  
✅ Change difficulty progression  
✅ Keep leaderboard history intact even if level order changes

Example: If you swap Level 1 (Park) and Level 2 (Sewer) in version 2.0, the leaderboards stay correct because they're tied to themes, not level numbers!

---

## 📝 App Store Connect Configuration

### For Each Leaderboard:

**Type**: Single Leaderboard  

**Score Settings**:
- **Levels 1-6 + Totals**:
  - Format: Integer
  - Sort: High to Low
  - Range: 0 to unlimited
  
- **Rat King Speedrun ONLY**:
  - Format: Time (Elapsed)
  - Sort: Low to High
  - Range: 0 to unlimited

**Localization** (English - Required):
- Name: (from Reference Name column above)
- Score Format: 
  - Pipes: `%d pipes`
  - Time: `%d ms`
  - Enemies: `%d enemies`
  - Coins: `%d coins`

**Image**: 512x512 PNG (optional but recommended)

---

## 💻 Code References

### Score Submission (Automatic)
**File**: `src/FloppyTurd/Game/FloppyTurdGame.cpp`

```cpp
// Level completion (pipes cleared)
case 1: leaderboardID = "com.floppyturd.park"; break;
case 2: leaderboardID = "com.floppyturd.sewer"; break;
case 3: leaderboardID = "com.floppyturd.desert"; break;
case 4: leaderboardID = "com.floppyturd.snow"; break;
case 5: leaderboardID = "com.floppyturd.castle"; break;
case 6: leaderboardID = "com.floppyturd.ratking"; break;

// Boss speedrun (time)
m_platformDelegates.gameCenter.submitScore("com.floppyturd.ratking.time", timeInMs, nullptr);
```

### Leaderboard Display
**File**: `src/FloppyTurd/States/LeaderboardState.cpp`

```cpp
case LeaderboardPage::LEVEL_1_PARK:   return "com.floppyturd.park";
case LeaderboardPage::LEVEL_2_SEWER:  return "com.floppyturd.sewer";
case LeaderboardPage::LEVEL_3_DESERT: return "com.floppyturd.desert";
case LeaderboardPage::LEVEL_4_SNOW:   return "com.floppyturd.snow";
case LeaderboardPage::LEVEL_5_CASTLE: return "com.floppyturd.castle";
case LeaderboardPage::LEVEL_6_BOSS:   return "com.floppyturd.ratking";
```

---

## ✅ Setup Checklist

- [ ] Created all 10 leaderboards in App Store Connect
- [ ] Used EXACT IDs (case-sensitive!)
- [ ] Set correct score format for each (Integer vs Time)
- [ ] Set correct sort order (High→Low vs Low→High)
- [ ] Added English localization for each
- [ ] Added all to Default Leaderboard Set
- [ ] Tested on physical device (not simulator)
- [ ] Verified scores submit correctly
- [ ] Confirmed leaderboards display in Game Center app

---

## 🚨 Common Mistakes to Avoid

❌ Using level numbers in IDs (`level1` instead of `park`)  
❌ Typos in leaderboard IDs (must match code EXACTLY)  
❌ Wrong score format (using Integer for boss time)  
❌ Wrong sort order (High→Low for speedrun time)  
❌ Testing in Simulator (Game Center doesn't work there)  
❌ Forgetting to localize leaderboards  
❌ Not adding to Default Leaderboard Set

---

**Last Updated**: Code updated with theme-based IDs on November 7, 2025 ✅
