# iOS Serialization/Deserialization System - Research & Design Document

**Project**: Floppy Turd  
**System**: Data Persistence & Serialization  
**Platform**: iOS 10.0+  
**Date**: 2024  
**Status**: Research Phase

---

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Current State Analysis](#current-state-analysis)
3. [iOS Persistence Options](#ios-persistence-options)
4. [Data Architecture](#data-architecture)
5. [Security Considerations](#security-considerations)
6. [Migration & Versioning](#migration--versioning)
7. [Implementation Strategy](#implementation-strategy)
8. [Testing & Validation](#testing--validation)

---

## Executive Summary

### What We're Building
A robust, secure, and future-proof data persistence system that saves and restores:
- Player progress (unlocked levels, high scores per level)
- Inventory & customization (equipped hats, purchased items)
- Game statistics (total games, coins collected, deaths, etc.)
- User settings (volume, haptics, difficulty)
- Anti-cheat validation data (for Game Center leaderboard integrity)

### Current State
**Files**: 
- `FloppyTurdGame.cpp` lines 28-38, 409-485, 788-840
- Binary file: `floppyturd_save.dat`
- Text file: `floppyturd_settings.cfg`

**Data Saved**:
- High scores (legacy + per-level)
- Player coins (stored & total collected)
- Game stats (games played, deaths, jumps, etc.)
- Level stats (high score, best coins, unlocked status)
- Settings (master/music/sfx volume, debug mode)
- Hat status (via HatsSystem - file unknown)

**Issues**:
1. **No versioning** - Can't migrate data when adding new fields
2. **Binary format** - Hard to debug, not human-readable
3. **No validation** - Data can be tampered with (cheating)
4. **No encryption** - Plain binary data easily modified
5. **Thread safety unclear** - Save/load might conflict
6. **No cloud sync** - Progress lost if device replaced
7. **No corruption recovery** - One bad write loses all data
8. **Hat data path unknown** - Unclear where hats are saved

### Goals
- **Reliability**: Never lose player data
- **Security**: Prevent cheating for leaderboards
- **Performance**: Fast save/load (< 50ms)
- **Maintainability**: Easy to add new fields
- **iCloud Ready**: Architecture supports sync (Phase 2)
- **Debug-Friendly**: Human-readable backup format
- **Migration**: Seamless upgrades from current binary format

---

## Current State Analysis

### Existing Save System Deep Dive

#### SaveGameData() - Lines 409-434
```cpp
void FloppyTurdGame::SaveGameData() {
    GN_LOG_INFO("💰 Saving game data... Player coins: " + std::to_string(m_playerCoins));
    
    std::ofstream file(SAVE_FILE_NAME, std::ios::binary);
    if (file.is_open()) {
        // Save basic game data
        file.write(reinterpret_cast<const char*>(&m_highScore), sizeof(m_highScore));
        file.write(reinterpret_cast<const char*>(&m_playerCoins), sizeof(m_playerCoins));
        
        // Save game stats
        file.write(reinterpret_cast<const char*>(&m_gameStats), sizeof(m_gameStats));
        
        // Save level stats for levels 1-6
        for (int i = 1; i <= MAX_LEVELS; ++i) {
            LevelSaveData saveData = {
                m_levelStats[i].highScore,
                m_levelStats[i].bestCoins,
                m_levelStats[i].unlocked
            };
            file.write(reinterpret_cast<const char*>(&saveData), sizeof(LevelSaveData));
        }
        
        file.close();
        GN_LOG_INFO("✅ Game data saved successfully");
    } else {
        GN_LOG_ERROR("❌ Failed to save game data");
    }
}
```

**Analysis**:
- ✅ Simple, fast binary write
- ✅ Uses dedicated save struct (LevelSaveData) to avoid saving runtime fields
- ❌ No version header - can't detect old save format
- ❌ No checksum - file corruption undetected
- ❌ Direct struct write - breaks if struct layout changes (padding, alignment)
- ❌ No atomic write - crash during save = corrupted file
- ❌ No backup - one bad write loses everything

#### LoadGameData() - Lines 435-485
```cpp
void FloppyTurdGame::LoadGameData() {
    std::ifstream file(SAVE_FILE_NAME, std::ios::binary);
    if (file.is_open()) {
        // Load basic game data
        file.read(reinterpret_cast<char*>(&m_highScore), sizeof(m_highScore));
        file.read(reinterpret_cast<char*>(&m_playerCoins), sizeof(m_playerCoins));
        
        // Load game stats
        file.read(reinterpret_cast<char*>(&m_gameStats), sizeof(m_gameStats));
        
        // Load level stats
        for (int i = 1; i <= MAX_LEVELS; ++i) {
            LevelSaveData saveData;
            file.read(reinterpret_cast<char*>(&saveData), sizeof(LevelSaveData));
            
            m_levelStats[i].highScore = saveData.highScore;
            m_levelStats[i].bestCoins = saveData.bestCoins;
            m_levelStats[i].unlocked = saveData.unlocked;
            
            // Runtime fields set elsewhere
            SetDefaultUnlockRequirements(i, m_levelStats[i]);
        }
        
        file.close();
    } else {
        GN_LOG_INFO("No save file found, using defaults");
        ResetGameData();
    }
}
```

**Analysis**:
- ✅ Separates persistent data from runtime data (unlock requirements)
- ✅ Falls back to defaults if file missing
- ❌ No version check - loads blindly
- ❌ No validation - doesn't check if values are reasonable
- ❌ Silent corruption - bad data just loads
- ❌ No migration path - can't upgrade old saves

#### SaveSettings() - Lines 815-827
```cpp
void FloppyTurdGame::SaveSettings() {
    std::ofstream file(SETTINGS_FILE_NAME);
    if (file.is_open()) {
        file << m_masterVolume << " " << m_musicVolume << " " << m_sfxVolume << " " << m_showDebugInfo;
        file.close();
    }
}
```

**Analysis**:
- ✅ Human-readable text format
- ✅ Easy to debug
- ✅ Has versioning (v1 vs v2 detected in LoadSettings)
- ❌ Still no corruption detection
- ❌ No validation (could load negative volume)

#### HatsSystem Persistence
**File**: `src/FloppyTurd/Systems/HatsSystem.h` (lines 106-107)

```cpp
void SaveHatStatus();
void LoadHatStatus();
```

**Issue**: Implementation not found in visible code. Need to search for:
- Where are hat unlock/equip states saved?
- Same file as game data or separate?
- Binary or text format?

### Data Inventory (What Needs Saving)

#### 1. Player Progress
```cpp
struct LevelStats {
    int highScore;           // ✅ Currently saved
    int bestCoins;          // ✅ Currently saved
    bool unlocked;          // ✅ Currently saved
    // Runtime only (not saved):
    int unlockRequirement;  
    int coinRequirement;    
};
// Saved for levels 1-6
```

#### 2. Game Statistics
```cpp
struct GameStats {
    int totalGamesPlayed;       // ✅ Saved
    int totalScore;             // ✅ Saved
    int totalCoinsCollected;    // ✅ Saved
    int storedCoins;            // ✅ Saved (spendable currency)
    int totalDeaths;            // ✅ Saved
    int totalPipesCleared;      // ✅ Saved
    int totalJumps;             // ✅ Saved
    int totalEnemiesKilled;     // ✅ Saved
    float totalPlayTime;        // ✅ Saved (in seconds)
    int currentStreak;          // ✅ Saved
    int bestStreak;             // ✅ Saved
};
```

#### 3. Customization (Hats)
```cpp
struct HatData {
    std::string name;
    // ... texture paths ...
    int cost;
    HatStatus status;  // LOCKED or UNLOCKED
};

// What needs saving per hat:
int m_equippedHatIndex;     // Which hat is equipped
int m_selectedHatIndex;     // Which hat is selected in UI
// For each hat (15 total):
bool unlocked;              // Is hat unlocked?
```

#### 4. Settings
```cpp
float m_masterVolume;   // ✅ Saved (0.0 - 1.0)
float m_musicVolume;    // ✅ Saved (0.0 - 1.0)
float m_sfxVolume;      // ✅ Saved (0.0 - 1.0)
bool m_showDebugInfo;   // ✅ Saved

// Missing (should save):
bool hapticsEnabled;    // From HapticManager (future)
int difficulty;         // Difficulty enum (0=Runny, 1=Regular, 2=Rough)
```

#### 5. Anti-Cheat Data (New - for Game Center)
```cpp
// Per level playthrough validation
struct PlaythroughValidation {
    uint64_t sessionHash;       // Random hash per session
    std::vector<uint32_t> checkpoints; // Hidden checkpoint hashes
    uint64_t finalScoreHash;    // Checksum of score + session
    int64_t startTimestamp;     // Server time at start
    int64_t endTimestamp;       // Server time at end
};

// This data is NOT directly editable by user
// Stored separately from main save file
```

### File Size Analysis

**Current Save File Size** (estimated):
```
m_highScore:         4 bytes (int)
m_playerCoins:       4 bytes (int)
m_gameStats:        44 bytes (11 fields)
LevelSaveData × 6:  36 bytes (6 levels × 6 bytes each)
-----------------
Total:             ~88 bytes
```

**Projected Size with New Features**:
```
Version header:      4 bytes
Checksum:            8 bytes (uint64_t)
Legacy data:        88 bytes
Hat data:           ~40 bytes (15 hats × 1 bit/hat packed + 2 int indices)
Settings expansion: ~8 bytes (haptics + difficulty)
Anti-cheat hash:    16 bytes (per-session, not in main save)
-----------------
Total:            ~164 bytes
```

**Conclusion**: Even with all features, save file is < 200 bytes. Size is not a concern.

---

## iOS Persistence Options

### Option 1: UserDefaults (Recommended for Settings)

**What It Is**: Key-value store backed by property list (plist) file.

**Pros**:
- ✅ Built-in to iOS, no external dependencies
- ✅ Automatic thread safety
- ✅ iCloud sync support (NSUbiquitousKeyValueStore)
- ✅ Type-safe Swift API
- ✅ Instant writes (no file handle management)
- ✅ Perfect for settings/preferences

**Cons**:
- ❌ Not designed for large data (Apple recommends < 1MB total)
- ❌ No transactions (can't save multiple values atomically)
- ❌ No versioning built-in
- ❌ String keys (typo-prone)

**Best For**: Settings (volume, haptics, debug flags)

**Example**:
```swift
// Save
UserDefaults.standard.set(0.8, forKey: "masterVolume")
UserDefaults.standard.set(true, forKey: "hapticsEnabled")

// Load
let volume = UserDefaults.standard.float(forKey: "masterVolume") // default: 0.0
let haptics = UserDefaults.standard.bool(forKey: "hapticsEnabled") // default: false
```

**Migration from Current**:
```cpp
// Current: floppyturd_settings.cfg (text file)
// New: UserDefaults with structured keys

// Migration code (run once):
if (legacySettingsFileExists) {
    loadLegacySettings();
    saveToUserDefaults();
    deleteLegacyFile();
}
```

---

### Option 2: Codable + JSON (Recommended for Game Data)

**What It Is**: Swift's native serialization protocol with JSON encoding.

**Pros**:
- ✅ Type-safe (compiler checks field names)
- ✅ Human-readable (JSON format)
- ✅ Easy debugging (can inspect files)
- ✅ Built-in versioning support (optional fields)
- ✅ Automatic encoding/decoding
- ✅ Cross-platform (JSON works everywhere)
- ✅ Easy to extend (add new fields without breaking)

**Cons**:
- ❌ Larger file size than binary (164 bytes → ~500 bytes)
- ❌ Slightly slower than binary (negligible for our size)
- ❌ Requires Swift (can't use directly in C++)

**Best For**: Game progress, stats, unlocks

**Example**:
```swift
struct GameSaveData: Codable {
    let version: Int = 2
    var highScore: Int
    var storedCoins: Int
    var gameStats: GameStats
    var levelStats: [LevelSaveData]
    var hatData: HatSaveData
    
    struct GameStats: Codable {
        var totalGamesPlayed: Int
        var totalScore: Int
        var totalCoinsCollected: Int
        var storedCoins: Int
        var totalDeaths: Int
        var totalPipesCleared: Int
        var totalJumps: Int
        var totalEnemiesKilled: Int
        var totalPlayTime: Double
        var currentStreak: Int
        var bestStreak: Int
    }
    
    struct LevelSaveData: Codable {
        var highScore: Int
        var bestCoins: Int
        var unlocked: Bool
    }
    
    struct HatSaveData: Codable {
        var equippedHatIndex: Int
        var selectedHatIndex: Int
        var unlockedHats: [Bool] // 15 elements
    }
}

// Save
let saveData = GameSaveData(/* ... */)
let encoder = JSONEncoder()
encoder.outputFormatting = .prettyPrinted // Human-readable
let jsonData = try encoder.encode(saveData)
try jsonData.write(to: saveFileURL)

// Load
let jsonData = try Data(contentsOf: saveFileURL)
let decoder = JSONDecoder()
let saveData = try decoder.decode(GameSaveData.self, from: jsonData)
```

**Migration from Current Binary**:
```swift
// 1. Detect if old binary file exists
// 2. If yes, read using old C++ binary format
// 3. Convert to new Codable struct
// 4. Save as JSON
// 5. Delete old binary file
```

---

### Option 3: Binary Plist (Alternative)

**What It Is**: Apple's binary property list format.

**Pros**:
- ✅ Compact (similar to custom binary)
- ✅ Fast read/write
- ✅ Built-in iOS support
- ✅ Works with Codable

**Cons**:
- ❌ Not human-readable (harder to debug)
- ❌ Slightly more complex than JSON
- ❌ Apple-specific format

**Best For**: Not recommended (JSON is better for debugging, binary gains minimal)

**Example**:
```swift
let encoder = PropertyListEncoder()
encoder.outputFormat = .binary // vs .xml
let plistData = try encoder.encode(saveData)
```

---

### Option 4: Keychain (Recommended for Anti-Cheat)

**What It Is**: Secure encrypted storage for sensitive data.

**Pros**:
- ✅ Encrypted by iOS (can't be read even with jailbreak)
- ✅ Survives app reinstall (optional)
- ✅ Survives device backup/restore
- ✅ Perfect for anti-cheat validation hashes
- ✅ Not synced to iCloud (device-specific)

**Cons**:
- ❌ More complex API
- ❌ Not designed for large data
- ❌ Slower than file I/O (encryption overhead)

**Best For**: Anti-cheat session hashes, device fingerprints

**Example**:
```swift
import Security

// Save to Keychain
let sessionHash = "abc123...".data(using: .utf8)!
let query: [String: Any] = [
    kSecClass as String: kSecClassGenericPassword,
    kSecAttrAccount as String: "session_hash_level_1",
    kSecValueData as String: sessionHash
]
SecItemAdd(query as CFDictionary, nil)

// Load from Keychain
let loadQuery: [String: Any] = [
    kSecClass as String: kSecClassGenericPassword,
    kSecAttrAccount as String: "session_hash_level_1",
    kSecReturnData as String: true
]
var result: AnyObject?
SecItemCopyMatching(loadQuery as CFDictionary, &result)
let hash = result as? Data
```

---

### Option 5: Core Data (NOT Recommended)

**What It Is**: Apple's object-graph persistence framework (ORM).

**Why NOT to Use**:
- ❌ Massive overkill for our simple data
- ❌ Steep learning curve
- ❌ Thread safety complexity
- ❌ Migration complexity
- ❌ Designed for relational data (we have flat structs)

**When to Use**: Apps with complex relationships (100+ entity types, queries, etc.)

---

### Option 6: SQLite (NOT Recommended)

**What It Is**: Embedded relational database.

**Why NOT to Use**:
- ❌ Overkill for our data size
- ❌ Requires SQL knowledge
- ❌ Migration complexity
- ❌ No type safety without wrapper

**When to Use**: Apps with complex queries or 10,000+ records

---

### Recommended Hybrid Approach

**Settings** → `UserDefaults`
- Volume levels (master, music, sfx)
- Haptics enabled
- Debug mode
- Difficulty preference

**Game Data** → `Codable + JSON` in Documents directory
- High scores
- Level unlocks
- Hat ownership
- Statistics

**Anti-Cheat** → `Keychain`
- Session validation hashes
- Device fingerprints
- Leaderboard submission tokens

**Rationale**:
1. Each storage type optimized for its use case
2. Settings sync to iCloud automatically (UserDefaults)
3. Game data human-readable for debugging (JSON)
4. Anti-cheat data secure and persistent (Keychain)

---

## Data Architecture

### File Structure

```
/Documents/
  floppyturd_save_v2.json        ← Main save file (Codable/JSON)
  floppyturd_save_v2.json.bak    ← Automatic backup (previous save)

/Library/Preferences/
  com.floppyturd.game.plist      ← UserDefaults (settings)

/Keychain/
  floppyturd.session.level_1     ← Anti-cheat hash (level 1)
  floppyturd.session.level_2     ← Anti-cheat hash (level 2)
  ...
  floppyturd.device.fingerprint  ← Device ID

/Legacy (to delete after migration)/
  floppyturd_save.dat            ← Old binary save
  floppyturd_settings.cfg        ← Old text settings
```

### Data Models

#### Swift Side (New)

```swift
// MARK: - Top-Level Save Data
struct GameSaveData: Codable {
    let version: Int // Always check this first!
    var saveDate: Date
    var playerId: String? // Optional: for Game Center
    
    var progress: ProgressData
    var statistics: StatisticsData
    var customization: CustomizationData
    
    init() {
        version = 2
        saveDate = Date()
        progress = ProgressData()
        statistics = StatisticsData()
        customization = CustomizationData()
    }
}

// MARK: - Progress Data
struct ProgressData: Codable {
    var legacyHighScore: Int // Keep for backwards compat
    var levels: [LevelProgress] // 6 levels
    
    struct LevelProgress: Codable {
        var levelId: Int
        var highScore: Int
        var bestCoins: Int
        var unlocked: Bool
        var timesPlayed: Int = 0      // NEW
        var timesCompleted: Int = 0   // NEW
        var fastestTime: Double? = nil // NEW (in seconds)
    }
    
    init() {
        legacyHighScore = 0
        levels = (1...6).map { id in
            LevelProgress(levelId: id, highScore: 0, bestCoins: 0, 
                         unlocked: id == 1) // Level 1 always unlocked
        }
    }
}

// MARK: - Statistics Data
struct StatisticsData: Codable {
    var totalGamesPlayed: Int = 0
    var totalScore: Int = 0
    var totalCoinsCollected: Int = 0
    var storedCoins: Int = 0 // Spendable currency
    var totalDeaths: Int = 0
    var totalPipesCleared: Int = 0
    var totalJumps: Int = 0
    var totalEnemiesKilled: Int = 0
    var totalPlayTime: Double = 0.0 // seconds
    var currentStreak: Int = 0
    var bestStreak: Int = 0
    
    // NEW fields (future):
    var totalProjectilesFired: Int? = nil
    var totalHatsUnlocked: Int? = nil
    var bossesDefeated: Int? = nil
}

// MARK: - Customization Data
struct CustomizationData: Codable {
    var equippedHatIndex: Int = 0 // 0 = no hat
    var selectedHatIndex: Int = 0
    var unlockedHats: [Bool] // 15 elements (index 0 = hat 1)
    
    init() {
        equippedHatIndex = 0
        selectedHatIndex = 0
        unlockedHats = Array(repeating: false, count: 15)
        unlockedHats[0] = true // First hat always unlocked
    }
}

// MARK: - Settings (UserDefaults, not Codable)
struct GameSettings {
    static var masterVolume: Float {
        get { UserDefaults.standard.float(forKey: "masterVolume") }
        set { UserDefaults.standard.set(newValue, forKey: "masterVolume") }
    }
    
    static var musicVolume: Float {
        get { UserDefaults.standard.float(forKey: "musicVolume") }
        set { UserDefaults.standard.set(newValue, forKey: "musicVolume") }
    }
    
    static var sfxVolume: Float {
        get { UserDefaults.standard.float(forKey: "sfxVolume") }
        set { UserDefaults.standard.set(newValue, forKey: "sfxVolume") }
    }
    
    static var hapticsEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: "hapticsEnabled") }
        set { UserDefaults.standard.set(newValue, forKey: "hapticsEnabled") }
    }
    
    static var debugMode: Bool {
        get { UserDefaults.standard.bool(forKey: "debugMode") }
        set { UserDefaults.standard.set(newValue, forKey: "debugMode") }
    }
    
    static var difficulty: Int {
        get { UserDefaults.standard.integer(forKey: "difficulty") }
        set { UserDefaults.standard.set(newValue, forKey: "difficulty") }
    }
    
    // Set defaults on first launch
    static func registerDefaults() {
        let defaults: [String: Any] = [
            "masterVolume": 1.0,
            "musicVolume": 0.7,
            "sfxVolume": 0.8,
            "hapticsEnabled": true,
            "debugMode": false,
            "difficulty": 1 // Regular
        ]
        UserDefaults.standard.register(defaults: defaults)
    }
}
```

#### C++ Side (Bridge Interface)

```cpp
// Add to FloppyTurdGame.h

struct SaveGameDelegate {
    // Save/Load main game data
    void (*saveGameData)(const void* cppGameInstance);
    void (*loadGameData)(void* cppGameInstance);
    
    // Save/Load settings
    void (*saveSettings)();
    void (*loadSettings)();
    
    // Anti-cheat
    void (*saveSessionHash)(int levelId, const char* hash);
    const char* (*loadSessionHash)(int levelId);
    
    // Migration
    bool (*hasLegacySaveFile)();
    void (*migrateLegacyData)();
    
    void* platformContext; // Swift SaveManager instance
    
    SaveGameDelegate()
        : saveGameData(nullptr)
        , loadGameData(nullptr)
        , saveSettings(nullptr)
        , loadSettings(nullptr)
        , saveSessionHash(nullptr)
        , loadSessionHash(nullptr)
        , hasLegacySaveFile(nullptr)
        , migrateLegacyData(nullptr)
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
    SaveGameDelegate save; // NEW
    // ...
};
```

### Thread Safety Strategy

**Problem**: Save can be called from multiple places (pause, quit, background, etc.)

**Solution**: All save operations go through main thread

```swift
class SaveManager {
    private let saveQueue = DispatchQueue(label: "com.floppyturd.save", qos: .userInitiated)
    private var isSaving = false
    
    func saveGameData(_ data: GameSaveData, completion: ((Bool) -> Void)? = nil) {
        saveQueue.async { [weak self] in
            guard let self = self, !self.isSaving else {
                completion?(false)
                return
            }
            
            self.isSaving = true
            defer { self.isSaving = false }
            
            let success = self.performSave(data)
            
            DispatchQueue.main.async {
                completion?(success)
            }
        }
    }
}
```

### Atomic Write Strategy

**Problem**: Crash during save = corrupted file

**Solution**: Write to temporary file, then atomic rename

```swift
func performSave(_ data: GameSaveData) -> Bool {
    let fileURL = saveFileURL()
    let tempURL = fileURL.appendingPathExtension("tmp")
    let backupURL = fileURL.appendingPathExtension("bak")
    
    do {
        // 1. Encode to JSON
        let encoder = JSONEncoder()
        encoder.outputFormatting = .prettyPrinted
        let jsonData = try encoder.encode(data)
        
        // 2. Write to temp file
        try jsonData.write(to: tempURL, options: .atomic)
        
        // 3. If main file exists, copy to backup
        if FileManager.default.fileExists(atPath: fileURL.path) {
            try? FileManager.default.removeItem(at: backupURL) // Remove old backup
            try? FileManager.default.copyItem(at: fileURL, to: backupURL)
        }
        
        // 4. Atomic rename temp → main (this is the critical moment)
        try FileManager.default.replaceItem(at: fileURL, 
                                           withItemAt: tempURL, 
                                           backupItemName: nil, 
                                           options: .usingNewMetadataOnly, 
                                           resultingItemURL: nil)
        
        return true
        
    } catch {
        print("Save failed: \(error)")
        return false
    }
}
```

**Why This Works**:
- `replaceItem(at:withItemAt:)` is atomic on iOS
- If crash during write, temp file corrupted (not main file)
- If crash during rename, main file still intact
- Backup exists if main file corrupted

---

## Security Considerations

### Threat Model

**Attackers Want To**:
1. Inflate high scores for leaderboards
2. Give themselves unlimited coins
3. Unlock all hats without payment
4. Unlock all levels without progression
5. Modify stats for bragging rights

**Attack Vectors**:
1. **Direct File Editing**: Modify JSON file on jailbroken device or via backup
2. **Memory Editing**: Use tools like GameGem to modify runtime values
3. **Backup Manipulation**: Edit save files in iTunes/Finder backup, then restore
4. **Time Manipulation**: Change device clock to exploit time-based logic
5. **Replay Attacks**: Submit same valid score multiple times

### Defense Strategy

#### 1. Checksum Validation
**Purpose**: Detect file tampering

```swift
import CryptoKit

func calculateChecksum(_ data: GameSaveData) -> String {
    let encoder = JSONEncoder()
    encoder.outputFormatting = .sortedKeys // Deterministic encoding
    let jsonData = try! encoder.encode(data)
    
    let hash = SHA256.hash(data: jsonData)
    return hash.compactMap { String(format: "%02x", $0) }.joined()
}

func saveWithChecksum(_ data: GameSaveData) {
    var dataWithChecksum = data
    dataWithChecksum.checksum = calculateChecksum(data)
    
    // Save to file
    let encoder = JSONEncoder()
    let jsonData = try! encoder.encode(dataWithChecksum)
    try! jsonData.write(to: saveFileURL)
}

func loadAndValidate() -> GameSaveData? {
    let jsonData = try! Data(contentsOf: saveFileURL)
    var data = try! JSONDecoder().decode(GameSaveData.self, from: jsonData)
    
    let savedChecksum = data.checksum
    data.checksum = nil // Remove before recalculating
    let calculatedChecksum = calculateChecksum(data)
    
    guard savedChecksum == calculatedChecksum else {
        print("⚠️ Save file tampered! Checksum mismatch")
        return nil // Or reset to defaults
    }
    
    return data
}
```

#### 2. Keychain Session Hashes
**Purpose**: Validate leaderboard submissions

```swift
// On level start
func startLevel(levelId: Int) -> String {
    let sessionId = UUID().uuidString
    let timestamp = Date().timeIntervalSince1970
    
    let hashInput = "\(sessionId)|\(levelId)|\(timestamp)|\(deviceFingerprint)"
    let hash = SHA256.hash(data: hashInput.data(using: .utf8)!)
    let hashString = hash.compactMap { String(format: "%02x", $0) }.joined()
    
    // Store in Keychain (secure, survives app restart)
    KeychainManager.save(key: "session_\(levelId)", value: hashString)
    
    return sessionId
}

// On level complete (before submitting to Game Center)
func validateScore(levelId: Int, sessionId: String, score: Int) -> Bool {
    guard let storedHash = KeychainManager.load(key: "session_\(levelId)") else {
        return false // No session hash = invalid
    }
    
    let timestamp = Date().timeIntervalSince1970
    let expectedHash = SHA256.hash(data: "\(sessionId)|\(levelId)|\(timestamp)|\(deviceFingerprint)".data(using: .utf8)!)
    
    // Hash won't match exactly (timestamp different), but we can validate sessionId
    // More complex validation needed - see Game Center section
    return true
}
```

#### 3. Reasonable Bounds Validation
**Purpose**: Reject obviously cheated values

```swift
func validateLoadedData(_ data: GameSaveData) -> Bool {
    // Coins can't be negative or absurdly high
    guard data.statistics.storedCoins >= 0 && data.statistics.storedCoins < 1_000_000 else {
        return false
    }
    
    // Scores must be reasonable per level
    for level in data.progress.levels {
        if level.highScore < 0 || level.highScore > 10_000 {
            return false
        }
        if level.bestCoins < 0 || level.bestCoins > level.highScore * 2 {
            return false // Can't have more coins than reasonable for score
        }
    }
    
    // Play time can't exceed app install time
    let appInstallDate = getAppInstallDate()
    let maxPossiblePlayTime = Date().timeIntervalSince(appInstallDate)
    guard data.statistics.totalPlayTime <= maxPossiblePlayTime else {
        return false
    }
    
    return true
}
```

#### 4. Obfuscation (Light)
**Purpose**: Make casual editing harder

```swift
// XOR obfuscation (NOT encryption, just makes hex editing harder)
func obfuscate(_ value: Int, key: UInt8 = 0x42) -> Int {
    var bytes = withUnsafeBytes(of: value) { Array($0) }
    for i in 0..<bytes.count {
        bytes[i] ^= key
    }
    return bytes.withUnsafeBytes { $0.load(as: Int.self) }
}

// Use for critical values
var storedCoins: Int {
    get { obfuscate(_storedCoins, key: 0x42) }
    set { _storedCoins = obfuscate(newValue, key: 0x42) }
}
private var _storedCoins: Int
```

**Note**: This is security through obscurity. Real protection is server-side validation (Game Center).

---

## Migration & Versioning

### Version Strategy

**Semantic Versioning for Save Format**:
- Version 1: Legacy binary format (current)
- Version 2: JSON with checksums (new)
- Version 3+: Future expansions

**Version Field**:
```swift
struct GameSaveData: Codable {
    let version: Int = 2 // Increment when adding breaking changes
    // ...
}
```

### Migration Path: Binary → JSON

```swift
class SaveMigrator {
    func migrate() -> GameSaveData? {
        // 1. Check if legacy file exists
        let legacyURL = documentsURL.appendingPathComponent("floppyturd_save.dat")
        guard FileManager.default.fileExists(atPath: legacyURL.path) else {
            return nil // No migration needed
        }
        
        // 2. Load legacy binary data
        guard let legacyData = loadLegacyBinaryFormat(from: legacyURL) else {
            print("Failed to load legacy save")
            return nil
        }
        
        // 3. Convert to new format
        let newData = convertToV2(legacy: legacyData)
        
        // 4. Save in new format
        SaveManager.shared.save(newData)
        
        // 5. Backup legacy file (don't delete yet, in case of bugs)
        let backupURL = legacyURL.appendingPathExtension("v1_backup")
        try? FileManager.default.copyItem(at: legacyURL, to: backupURL)
        
        // 6. Delete legacy file (after confirmed new save works)
        try? FileManager.default.removeItem(at: legacyURL)
        
        return newData
    }
    
    private func loadLegacyBinaryFormat(from url: URL) -> LegacyGameData? {
        guard let data = try? Data(contentsOf: url) else { return nil }
        
        var offset = 0
        
        // Read in same order as SaveGameData()
        let highScore = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        
        let playerCoins = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        
        // GameStats (11 fields × 4 bytes each + 1 float)
        let totalGamesPlayed = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        // ... read rest of GameStats ...
        
        // LevelSaveData × 6
        var levels: [LevelData] = []
        for _ in 1...6 {
            let levelHighScore = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
            offset += 4
            let bestCoins = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
            offset += 4
            let unlocked = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Bool.self) }
            offset += 1
            
            levels.append(LevelData(highScore: Int(levelHighScore), bestCoins: Int(bestCoins), unlocked: unlocked))
        }
        
        return LegacyGameData(highScore: Int(highScore), coins: Int(playerCoins), levels: levels)
    }
    
    private func convertToV2(legacy: LegacyGameData) -> GameSaveData {
        var newData = GameSaveData()
        newData.progress.legacyHighScore = legacy.highScore
        newData.statistics.storedCoins = legacy.coins
        
        for (index, legacyLevel) in legacy.levels.enumerated() {
            newData.progress.levels[index].highScore = legacyLevel.highScore
            newData.progress.levels[index].bestCoins = legacyLevel.bestCoins
            newData.progress.levels[index].unlocked = legacyLevel.unlocked
        }
        
        return newData
    }
}
```

### Settings Migration: Text → UserDefaults

```swift
func migrateSettings() {
    let legacyURL = documentsURL.appendingPathComponent("floppyturd_settings.cfg")
    guard FileManager.default.fileExists(atPath: legacyURL.path) else { return }
    
    guard let contents = try? String(contentsOf: legacyURL) else { return }
    let values = contents.split(separator: " ").compactMap { Float($0) }
    
    if values.count >= 3 {
        // Legacy v2 format: master music sfx debug
        UserDefaults.standard.set(values[0], forKey: "masterVolume")
        UserDefaults.standard.set(values[1], forKey: "musicVolume")
        UserDefaults.standard.set(values[2], forKey: "sfxVolume")
        if values.count >= 4 {
            UserDefaults.standard.set(values[3] > 0, forKey: "debugMode")
        }
    }
    
    // Backup and delete
    try? FileManager.default.removeItem(at: legacyURL)
}
```

---

## Implementation Strategy

### Phase 1: Foundation (Week 1)
**Goal**: New save system with backwards compatibility

**Tasks**:
1. Create `SaveManager.swift` with Codable models
2. Implement JSON save/load with atomic writes
3. Add checksum validation
4. Implement legacy binary migration
5. Add SaveGameDelegate to PlatformDelegates.h
6. Bridge C++ ↔ Swift save calls

**Success Criteria**:
- New saves write to JSON format
- Legacy saves migrate automatically
- No data loss during migration
- Checksums detect tampering

### Phase 2: Settings Migration (Week 1)
**Goal**: Move settings to UserDefaults

**Tasks**:
1. Migrate `floppyturd_settings.cfg` → UserDefaults
2. Add haptics toggle to settings
3. Add difficulty selection to settings
4. Update C++ code to read settings via delegate
5. Test settings persistence across app restarts

**Success Criteria**:
- All settings in UserDefaults
- Legacy settings file deleted after migration
- C++ can read/write settings via delegate

### Phase 3: Hat System Integration (Week 2)
**Goal**: Unified hat persistence

**Tasks**:
1. Find current HatsSystem save implementation
2. Merge hat data into main save file (CustomizationData)
3. Remove separate hat file (if exists)
4. Test hat unlock/equip persistence

**Success Criteria**:
- Hats save/load correctly
- Equipped hat persists across sessions
- Purchased hats remain unlocked

### Phase 4: Anti-Cheat Foundation (Week 2)
**Goal**: Keychain session validation

**Tasks**:
1. Create `KeychainManager.swift` wrapper
2. Implement session hash generation
3. Store hashes in Keychain on level start
4. Add validation data to save file
5. Test hash persistence across app restarts

**Success Criteria**:
- Session hashes stored securely
- Hashes survive app restart
- Invalid hashes rejected

### Phase 5: Testing & Validation (Week 2)
**Goal**: Bulletproof reliability

**Tasks**:
1. Test migration on 100+ legacy save files
2. Test corruption recovery (backup restore)
3. Test concurrent save calls (thread safety)
4. Test app backgrounding during save
5. Performance profiling (save < 50ms)

**Success Criteria**:
- Zero data loss in tests
- Saves complete in < 50ms
- Thread-safe under stress test
- Backups restore successfully

---

## Testing & Validation

### Unit Tests

```swift
class SaveManagerTests: XCTestCase {
    func testSaveAndLoad() {
        let saveData = GameSaveData()
        saveData.statistics.storedCoins = 1000
        
        SaveManager.shared.save(saveData)
        let loaded = SaveManager.shared.load()
        
        XCTAssertEqual(loaded?.statistics.storedCoins, 1000)
    }
    
    func testChecksumValidation() {
        var saveData = GameSaveData()
        saveData.statistics.storedCoins = 1000
        SaveManager.shared.save(saveData)
        
        // Manually tamper with file
        let fileURL = SaveManager.saveFileURL()
        var jsonString = try! String(contentsOf: fileURL)
        jsonString = jsonString.replacingOccurrences(of: "1000", with: "9999")
        try! jsonString.write(to: fileURL, atomically: true, encoding: .utf8)
        
        // Load should fail validation
        let loaded = SaveManager.shared.load()
        XCTAssertNil(loaded) // Tampered file rejected
    }
    
    func testLegacyMigration() {
        // Create legacy binary file
        createLegacyBinaryFile(highScore: 500, coins: 100)
        
        // Migration should convert to new format
        let migrated = SaveMigrator().migrate()
        
        XCTAssertNotNil(migrated)
        XCTAssertEqual(migrated?.progress.legacyHighScore, 500)
        XCTAssertEqual(migrated?.statistics.storedCoins, 100)
    }
    
    func testAtomicWrite() {
        // Save should not corrupt existing file if crash during write
        // (Hard to test automatically - use manual crash injection)
    }
    
    func testThreadSafety() {
        let expectation = XCTestExpectation(description: "Concurrent saves")
        expectation.expectedFulfillmentCount = 10
        
        for i in 0..<10 {
            DispatchQueue.global().async {
                var data = GameSaveData()
                data.statistics.storedCoins = i * 100
                SaveManager.shared.save(data)
                expectation.fulfill()
            }
        }
        
        wait(for: [expectation], timeout: 5.0)
        
        // Final load should have one of the saved values (no corruption)
        let loaded = SaveManager.shared.load()
        XCTAssertNotNil(loaded)
    }
}
```

### Integration Tests

1. **Full Game Cycle**: Play level 1 → quit → relaunch → verify progress saved
2. **Hat Purchase**: Buy hat → quit → relaunch → verify hat unlocked
3. **Settings Persistence**: Change volume → quit → relaunch → verify volume saved
4. **Migration**: Install old version → play → update to new version → verify data migrated

### Performance Benchmarks

```swift
func benchmarkSavePerformance() {
    let data = GameSaveData()
    
    measure {
        SaveManager.shared.save(data)
    }
    
    // Target: < 50ms average
}

func benchmarkLoadPerformance() {
    measure {
        _ = SaveManager.shared.load()
    }
    
    // Target: < 30ms average
}
```

### Edge Cases

1. ✅ No save file exists (first launch) → defaults loaded
2. ✅ Corrupted save file → backup restored or defaults used
3. ✅ Partial write (app crash mid-save) → temp file discarded, main file intact
4. ✅ Jailbroken device → save still works (no special handling needed)
5. ✅ Device low on storage → save fails gracefully with error
6. ✅ iCloud restore → save file restored correctly
7. ✅ App reinstall → Keychain data persists (if configured)

---

## Appendix

### File Locations Reference

```swift
// Documents directory (backed up to iCloud/iTunes)
let documentsURL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
let saveFileURL = documentsURL.appendingPathComponent("floppyturd_save_v2.json")

// Library directory (not backed up)
let libraryURL = FileManager.default.urls(for: .libraryDirectory, in: .userDomainMask)[0]
let cacheURL = libraryURL.appendingPathComponent("Caches")

// UserDefaults location (automatic)
// ~/Library/Preferences/com.floppyturd.game.plist
```

### Useful Resources

1. **Apple Documentation**:
   - [File System Basics](https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/)
   - [Codable](https://developer.apple.com/documentation/swift/codable)
   - [UserDefaults](https://developer.apple.com/documentation/foundation/userdefaults)
   - [Keychain Services](https://developer.apple.com/documentation/security/keychain_services)

2. **Best Practices**:
   - WWDC 2018: "Data You Can Trust"
   - WWDC 2019: "Cryptography and Your Apps"

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Next Review**: After Phase 3 implementation