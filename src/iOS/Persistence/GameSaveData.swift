//
//  GameSaveData.swift
//  FloppyTurd
//
//  Created on 2024
//  iOS Serialization System - Game Save Data Models
//

import Foundation

// MARK: - Game Save Data Version 2 (JSON-based)

/// Main save data structure containing all persistent game state
struct GameSaveData: Codable {
    let version: Int
    var saveDate: Date
    var playerId: String?

    var progress: ProgressData
    var statistics: StatisticsData
    var customization: CustomizationData
    var settings: SettingsData

    init() {
        self.version = 2
        self.saveDate = Date()
        self.playerId = nil
        self.progress = ProgressData()
        self.statistics = StatisticsData()
        self.customization = CustomizationData()
        self.settings = SettingsData()
    }
}

// MARK: - Progress Data

/// Player progression through levels
struct ProgressData: Codable {
    var legacyHighScore: Int  // Overall high score (legacy compatibility)
    var levels: [LevelProgress]

    struct LevelProgress: Codable {
        var levelId: Int
        var highScore: Int
        var bestCoins: Int
        var unlocked: Bool
        var timesPlayed: Int
        var timesCompleted: Int
        var fastestTime: Double?  // For boss levels - time in seconds

        init(levelId: Int) {
            self.levelId = levelId
            self.highScore = 0
            self.bestCoins = 0
            self.unlocked = (levelId == 1)  // Only level 1 unlocked by default
            self.timesPlayed = 0
            self.timesCompleted = 0
            self.fastestTime = nil
        }
    }

    init() {
        self.legacyHighScore = 0
        // Initialize 6 levels (1-6)
        self.levels = (1...6).map { LevelProgress(levelId: $0) }
    }
}

// MARK: - Statistics Data

/// Global game statistics
struct StatisticsData: Codable {
    var totalGamesPlayed: Int
    var totalScore: Int
    var totalCoinsCollected: Int
    var storedCoins: Int  // Current spendable currency
    var totalDeaths: Int
    var totalPipesCleared: Int
    var totalJumps: Int
    var totalEnemiesKilled: Int
    var totalPlayTime: Double  // In seconds
    var currentStreak: Int
    var bestStreak: Int

    // New statistics (optional for backward compatibility)
    var totalProjectilesFired: Int?
    var totalHatsUnlocked: Int?
    var bossesDefeated: Int?

    init() {
        self.totalGamesPlayed = 0
        self.totalScore = 0
        self.totalCoinsCollected = 0
        self.storedCoins = 0
        self.totalDeaths = 0
        self.totalPipesCleared = 0
        self.totalJumps = 0
        self.totalEnemiesKilled = 0
        self.totalPlayTime = 0.0
        self.currentStreak = 0
        self.bestStreak = 0
        self.totalProjectilesFired = 0
        self.totalHatsUnlocked = 0
        self.bossesDefeated = 0
    }
}

// MARK: - Customization Data

/// Hat and cosmetic customization state
struct CustomizationData: Codable {
    var equippedHatIndex: Int
    var selectedHatIndex: Int
    var unlockedHats: [Bool]  // 15 hats total
    var unlockedSkills: [Bool]  // 5 skills total

    init() {
        self.equippedHatIndex = 0  // No hat equipped
        self.selectedHatIndex = 0
        // Initialize 15 hats - first 4 are unlocked by default (Cowboy, Flower, Doorag, Ballcap)
        self.unlockedHats = [true, true, true, true] + Array(repeating: false, count: 11)
        // Initialize 5 skills - all locked by default
        self.unlockedSkills = Array(repeating: false, count: 5)
    }
}

// MARK: - Settings Data

/// Game settings (audio, difficulty, debug, haptics)
/// Saved with game progress in save file
struct SettingsData: Codable {
    var masterVolume: Float
    var musicVolume: Float
    var sfxVolume: Float
    var difficulty: Int  // 0=Runny, 1=Regular, 2=Rough
    var debugMode: Bool
    var hapticsEnabled: Bool

    init() {
        self.masterVolume = 0.7
        self.musicVolume = 0.6
        self.sfxVolume = 0.8
        self.difficulty = 1  // Regular (normal)
        self.debugMode = false
        self.hapticsEnabled = true
    }
}

// MARK: - Legacy Binary Format (for migration)

/// Legacy save data structure from binary format (for migration purposes)
struct LegacyGameData {
    var highScore: Int
    var gameStats: LegacyGameStats
    var levelStats: [LegacyLevelSaveData]  // Indices 1-6

    struct LegacyGameStats {
        var totalGamesPlayed: Int
        var totalScore: Int
        var totalCoinsCollected: Int
        var storedCoins: Int
        var totalDeaths: Int
        var totalPipesCleared: Int
        var totalJumps: Int
        var totalEnemiesKilled: Int
        var totalPlayTime: Float
        var currentStreak: Int
        var bestStreak: Int
    }

    struct LegacyLevelSaveData {
        var highScore: Int
        var bestCoins: Int
        var unlocked: Bool
    }
}

// MARK: - Settings Data (UserDefaults)

/// Game settings stored in UserDefaults
struct GameSettings {
    nonisolated(unsafe) static let defaults = UserDefaults.standard

    // Keys
    private static let masterVolumeKey = "masterVolume"
    private static let musicVolumeKey = "musicVolume"
    private static let sfxVolumeKey = "sfxVolume"
    private static let hapticsEnabledKey = "hapticsEnabled"
    private static let debugModeKey = "debugMode"
    private static let difficultyKey = "difficulty"

    // Default values
    static let defaultMasterVolume: Float = 0.7
    static let defaultMusicVolume: Float = 0.6
    static let defaultSFXVolume: Float = 0.8
    static let defaultHapticsEnabled: Bool = true
    static let defaultDebugMode: Bool = false
    static let defaultDifficulty: Int = 1  // 0=Runny, 1=Regular, 2=Rough

    // Getters
    static var masterVolume: Float {
        get { defaults.float(forKey: masterVolumeKey) }
        set { defaults.set(newValue, forKey: masterVolumeKey) }
    }

    static var musicVolume: Float {
        get { defaults.float(forKey: musicVolumeKey) }
        set { defaults.set(newValue, forKey: musicVolumeKey) }
    }

    static var sfxVolume: Float {
        get { defaults.float(forKey: sfxVolumeKey) }
        set { defaults.set(newValue, forKey: sfxVolumeKey) }
    }

    static var hapticsEnabled: Bool {
        get { defaults.bool(forKey: hapticsEnabledKey) }
        set { defaults.set(newValue, forKey: hapticsEnabledKey) }
    }

    static var debugMode: Bool {
        get { defaults.bool(forKey: debugModeKey) }
        set { defaults.set(newValue, forKey: debugModeKey) }
    }

    static var difficulty: Int {
        get { defaults.integer(forKey: difficultyKey) }
        set { defaults.set(newValue, forKey: difficultyKey) }
    }

    /// Register default values on first launch
    static func registerDefaults() {
        defaults.register(defaults: [
            masterVolumeKey: defaultMasterVolume,
            musicVolumeKey: defaultMusicVolume,
            sfxVolumeKey: defaultSFXVolume,
            hapticsEnabledKey: defaultHapticsEnabled,
            debugModeKey: defaultDebugMode,
            difficultyKey: defaultDifficulty,
        ])
    }
}
