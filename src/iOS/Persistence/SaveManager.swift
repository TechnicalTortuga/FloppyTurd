//
//  SaveManager.swift
//  FloppyTurd
//
//  Created on 2024
//  iOS Serialization System - Main Save/Load Manager
//

import CryptoKit  // For HMAC validation (anti-tamper)
import Foundation
import os.log

/// Thread-safe manager for game data persistence
/// Note: Some methods are nonisolated for C++ bridge compatibility
final class SaveManager: @unchecked Sendable {
    static let shared = SaveManager()

    // MARK: - Properties

    private let logger = Logger(subsystem: "com.floppyturd.ios", category: "SaveManager")
    private let saveQueue = DispatchQueue(label: "com.floppyturd.savequeue", qos: .utility)
    private var isSaving = false

    private let saveFileName = "floppyturd_save_v3.plist"  // Binary plist (not human-readable)
    private let legacySaveFileName = "floppyturd_save.dat"
    private let legacySettingsFileName = "floppyturd_settings.cfg"

    private var documentsURL: URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
    }

    private var saveFileURL: URL {
        documentsURL.appendingPathComponent(saveFileName)
    }

    private var legacySaveFileURL: URL {
        documentsURL.appendingPathComponent(legacySaveFileName)
    }

    private var legacySettingsFileURL: URL {
        documentsURL.appendingPathComponent(legacySettingsFileName)
    }

    // MARK: - Initialization

    private init() {
        logger.info("SaveManager initialized")
        logger.debug("Documents directory: \(self.documentsURL.path)")

        // Register default settings
        GameSettings.registerDefaults()

        // Migrate legacy settings if present
        migrateSettingsIfNeeded()
    }

    // MARK: - Public API

    /// Load game data from disk (migrates legacy format if needed)
    nonisolated func load() -> GameSaveData? {
        logger.info("Loading game data...")

        // Check if we need to migrate from legacy format
        if FileManager.default.fileExists(atPath: legacySaveFileURL.path) {
            logger.info("Legacy save file detected - migrating...")
            if let migrated = migrateLegacySave() {
                logger.info("Migration successful")
                return migrated
            } else {
                logger.warning("Migration failed, starting fresh")
                return nil
            }
        }

        // Load from binary plist format
        guard FileManager.default.fileExists(atPath: saveFileURL.path) else {
            logger.info("No save file found, starting fresh")
            return nil
        }

        do {
            let combinedData = try Data(contentsOf: saveFileURL)

            // File format: [HMAC (32 bytes)] + [PropertyList Data]
            guard combinedData.count > 32 else {
                logger.warning("Save file too small, possibly corrupted")
                return nil
            }

            let hmacData = combinedData.prefix(32)
            let plistData = combinedData.suffix(from: 32)

            // Verify HMAC (detect tampering)
            let expectedHMAC = HMAC<SHA256>.authenticationCode(for: plistData, using: Self.hmacKey)
            guard hmacData == Data(expectedHMAC) else {
                logger.warning("Save file HMAC mismatch - file may have been tampered with!")
                logger.warning("Starting fresh to prevent cheating")
                return nil
            }

            // Decode binary plist
            let decoder = PropertyListDecoder()
            var saveData = try decoder.decode(GameSaveData.self, from: plistData)

            // Validate loaded data
            if validateLoadedData(&saveData) {
                logger.info(
                    "Game data loaded successfully (version \(saveData.version), HMAC verified)")
                return saveData
            } else {
                logger.warning("Loaded data failed validation, starting fresh")
                return nil
            }
        } catch {
            logger.error("Failed to load game data: \(error.localizedDescription)")
            return nil
        }
    }

    /// Save game data to disk (async, thread-safe)
    func save(_ data: GameSaveData, completion: (@Sendable (Bool) -> Void)? = nil) {
        guard !isSaving else {
            logger.warning("Save already in progress, skipping...")
            completion?(false)
            return
        }

        isSaving = true

        saveQueue.async { [weak self] in
            guard let self = self else {
                DispatchQueue.main.async {
                    completion?(false)
                }
                return
            }

            let success = self.performSave(data)

            DispatchQueue.main.async {
                self.isSaving = false
                completion?(success)
            }
        }
    }

    /// Synchronous save (use sparingly, prefer async save)
    nonisolated func saveSync(_ data: GameSaveData) -> Bool {
        return performSave(data)
    }

    /// Check if legacy save file exists
    nonisolated func hasLegacySaveFile() -> Bool {
        return FileManager.default.fileExists(atPath: legacySaveFileURL.path)
    }

    /// Delete all save data (use with caution!)
    nonisolated func deleteSaveData() {
        do {
            if FileManager.default.fileExists(atPath: saveFileURL.path) {
                try FileManager.default.removeItem(at: saveFileURL)
                logger.info("Save data deleted")
            }
        } catch {
            logger.error("Failed to delete save data: \(error.localizedDescription)")
        }
    }

    // MARK: - Command Processing (for ThreadingProxy integration)

    /// Process a save game command with JSON data
    /// - Parameter jsonString: JSON string containing game save data
    /// - Returns: true if save succeeded, false otherwise
    static func processSaveGameCommand(_ jsonString: String) -> Bool {
        shared.logger.info("Processing save game command...")

        // Still receive JSON from C++ for now (but save as binary plist)
        guard let jsonData = jsonString.data(using: .utf8) else {
            shared.logger.error("Failed to convert JSON string to Data")
            return false
        }

        do {
            let decoder = JSONDecoder()
            decoder.dateDecodingStrategy = .iso8601
            let saveData = try decoder.decode(GameSaveData.self, from: jsonData)

            // Save using SaveManager (will save as BINARY PLIST with HMAC)
            let success = SaveManager.shared.saveSync(saveData)
            if success {
                shared.logger.info("Game data saved successfully (binary plist with HMAC)")
            } else {
                shared.logger.error("Failed to save game data")
            }
            return success
        } catch {
            shared.logger.error("Failed to decode JSON: \(error.localizedDescription)")
            return false
        }
    }

    /// Process a load game command (async version for command queue)
    /// - Returns: JSON string containing game save data, or nil if no save exists
    static func processLoadGameCommand() -> String? {
        shared.logger.info("Processing load game command...")

        guard let saveData = SaveManager.shared.load() else {
            shared.logger.info("No save data found")
            return nil
        }

        // Encode to JSON string
        do {
            let encoder = JSONEncoder()
            encoder.dateEncodingStrategy = .iso8601
            encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
            let jsonData = try encoder.encode(saveData)

            guard let jsonString = String(data: jsonData, encoding: .utf8) else {
                shared.logger.error("Failed to convert JSON data to string")
                return nil
            }

            shared.logger.info("Game data loaded successfully (\(jsonData.count) bytes)")
            return jsonString
        } catch {
            shared.logger.error("Failed to encode save data: \(error.localizedDescription)")
            return nil
        }
    }

    /// Process a synchronous load game command (for C++ bridge)
    /// - Returns: Simple key:value format string, or nil if no save exists
    /// - Note: This is the synchronous version called directly from C++ during initialization
    static func processLoadGameCommandSync() -> String? {
        shared.logger.info("Processing SYNCHRONOUS load game command...")

        guard let saveData = SaveManager.shared.load() else {
            shared.logger.info("No save data found (sync load)")
            return nil
        }

        // Create simple KEY:VALUE format (one per line) for C++ to parse
        // This is much simpler than JSON and Swift handles all the validation
        var output = "VERSION:\(saveData.version)\n"

        // Statistics
        output += "STATS_TOTAL_GAMES:\(saveData.statistics.totalGamesPlayed)\n"
        output += "STATS_TOTAL_SCORE:\(saveData.statistics.totalScore)\n"
        output += "STATS_TOTAL_COINS:\(saveData.statistics.totalCoinsCollected)\n"
        output += "STATS_STORED_COINS:\(saveData.statistics.storedCoins)\n"
        output += "STATS_TOTAL_DEATHS:\(saveData.statistics.totalDeaths)\n"
        output += "STATS_TOTAL_PIPES:\(saveData.statistics.totalPipesCleared)\n"
        output += "STATS_TOTAL_JUMPS:\(saveData.statistics.totalJumps)\n"
        output += "STATS_TOTAL_ENEMIES:\(saveData.statistics.totalEnemiesKilled)\n"
        output += "STATS_PLAY_TIME:\(saveData.statistics.totalPlayTime)\n"
        output += "STATS_CURRENT_STREAK:\(saveData.statistics.currentStreak)\n"
        output += "STATS_BEST_STREAK:\(saveData.statistics.bestStreak)\n"

        // Progress
        output += "PROGRESS_LEGACY_HIGH_SCORE:\(saveData.progress.legacyHighScore)\n"

        // Levels
        for level in saveData.progress.levels {
            output += "LEVEL_\(level.levelId)_HIGH_SCORE:\(level.highScore)\n"
            output += "LEVEL_\(level.levelId)_BEST_COINS:\(level.bestCoins)\n"
            output += "LEVEL_\(level.levelId)_UNLOCKED:\(level.unlocked ? 1 : 0)\n"
            if let fastestTime = level.fastestTime {
                output += "LEVEL_\(level.levelId)_FASTEST_TIME:\(fastestTime)\n"
            }
        }

        // Customization
        output += "CUSTOM_EQUIPPED_HAT:\(saveData.customization.equippedHatIndex)\n"
        output += "CUSTOM_SELECTED_HAT:\(saveData.customization.selectedHatIndex)\n"
        for (index, unlocked) in saveData.customization.unlockedHats.enumerated() {
            output += "CUSTOM_HAT_\(index)_UNLOCKED:\(unlocked ? 1 : 0)\n"
        }
        for (index, unlocked) in saveData.customization.unlockedSkills.enumerated() {
            output += "CUSTOM_SKILL_\(index)_UNLOCKED:\(unlocked ? 1 : 0)\n"
        }

        shared.logger.info("Game data loaded successfully (SYNC): \(output.count) bytes")
        shared.logger.debug(
            "Sample data - Coins: \(saveData.statistics.storedCoins), Level 2 unlocked: \(saveData.progress.levels[1].unlocked)"
        )

        return output
    }

    /// Process a save settings command
    /// - Parameters:
    ///   - masterVolume: Master volume (0.0 - 1.0)
    ///   - musicVolume: Music volume (0.0 - 1.0)
    ///   - sfxVolume: SFX volume (0.0 - 1.0)
    ///   - debugMode: Debug mode enabled
    static func processSaveSettingsCommand(
        masterVolume: Float,
        musicVolume: Float,
        sfxVolume: Float,
        debugMode: Bool
    ) {
        shared.logger.info("Processing save settings command...")

        GameSettings.masterVolume = masterVolume
        GameSettings.musicVolume = musicVolume
        GameSettings.sfxVolume = sfxVolume
        GameSettings.debugMode = debugMode

        shared.logger.info(
            "Settings saved: master=\(masterVolume) music=\(musicVolume) sfx=\(sfxVolume) debug=\(debugMode)"
        )
    }

    /// Process a load settings command
    /// - Returns: Tuple of (masterVolume, musicVolume, sfxVolume, debugMode, wasLoaded)
    static func processLoadSettingsCommand() -> (Float, Float, Float, Bool, Bool) {
        shared.logger.info("Processing load settings command...")

        let masterVolume = GameSettings.masterVolume
        let musicVolume = GameSettings.musicVolume
        let sfxVolume = GameSettings.sfxVolume
        let debugMode = GameSettings.debugMode

        // Check if we're using default values (first launch)
        let isFirstLaunch = !UserDefaults.standard.bool(forKey: "hasLaunchedBefore")
        if isFirstLaunch {
            UserDefaults.standard.set(true, forKey: "hasLaunchedBefore")
            shared.logger.info("First launch detected - using default settings")
        }

        shared.logger.info(
            "Settings loaded: master=\(masterVolume) music=\(musicVolume) sfx=\(sfxVolume) debug=\(debugMode)"
        )

        return (masterVolume, musicVolume, sfxVolume, debugMode, !isFirstLaunch)
    }

    // MARK: - Private Helpers

    /// Secret key for HMAC validation (prevents save file tampering)
    /// In production, this should be obfuscated or derived from device-specific data
    private static let hmacKey = SymmetricKey(
        data: "FloppyTurd-SaveFile-Key-2025".data(using: .utf8)!)

    nonisolated private func performSave(_ data: GameSaveData) -> Bool {
        do {
            var saveData = data
            saveData.saveDate = Date()

            // Use BINARY PropertyList format (not human-readable, harder to hack)
            let encoder = PropertyListEncoder()
            encoder.outputFormat = .binary  // NOT XML - binary is not human-readable

            let plistData = try encoder.encode(saveData)

            // Calculate HMAC for tamper detection
            let hmac = HMAC<SHA256>.authenticationCode(for: plistData, using: Self.hmacKey)
            let hmacData = Data(hmac)

            // Combine: [HMAC (32 bytes)] + [PropertyList Data]
            var combinedData = Data()
            combinedData.append(hmacData)
            combinedData.append(plistData)

            // Atomic write using temporary file
            let tempURL = saveFileURL.appendingPathExtension("tmp")
            try combinedData.write(to: tempURL, options: .atomic)

            // Replace old file with new file
            if FileManager.default.fileExists(atPath: saveFileURL.path) {
                try FileManager.default.removeItem(at: saveFileURL)
            }
            try FileManager.default.moveItem(at: tempURL, to: saveFileURL)

            logger.info(
                "Game data saved successfully (\(combinedData.count) bytes, binary plist with HMAC)"
            )
            return true
        } catch {
            logger.error("Failed to save game data: \(error.localizedDescription)")
            return false
        }
    }

    private func validateLoadedData(_ data: inout GameSaveData) -> Bool {
        // Validate version
        let version = data.version
        guard version == 2 else {
            logger.warning("Unsupported save version: \(version)")
            return false
        }

        // Validate level count
        let levelCount = data.progress.levels.count
        guard levelCount == 6 else {
            logger.warning("Invalid level count: \(levelCount)")
            return false
        }

        // Validate hat count
        let hatCount = data.customization.unlockedHats.count
        guard hatCount == 15 else {
            logger.warning("Invalid hat count: \(hatCount)")
            return false
        }

        // Clamp values to reasonable bounds
        data.statistics.storedCoins = max(0, min(data.statistics.storedCoins, 999999))
        data.statistics.totalScore = max(0, min(data.statistics.totalScore, 999999))
        data.statistics.totalPlayTime = max(0, min(data.statistics.totalPlayTime, 999999))

        // Validate equipped hat index
        data.customization.equippedHatIndex = max(0, min(data.customization.equippedHatIndex, 14))
        data.customization.selectedHatIndex = max(0, min(data.customization.selectedHatIndex, 14))

        // Ensure level 1 is always unlocked
        if data.progress.levels.count > 0 {
            data.progress.levels[0].unlocked = true
        }

        return true
    }

    // MARK: - Legacy Migration

    private func migrateLegacySave() -> GameSaveData? {
        guard let legacyData = loadLegacyBinaryFormat() else {
            logger.error("Failed to load legacy binary data")
            return nil
        }

        let newData = convertToV2(legacy: legacyData)

        // Save in new format
        if saveSync(newData) {
            // Backup legacy file
            let backupURL = legacySaveFileURL.appendingPathExtension("v1_backup")
            do {
                if FileManager.default.fileExists(atPath: backupURL.path) {
                    try FileManager.default.removeItem(at: backupURL)
                }
                try FileManager.default.copyItem(at: legacySaveFileURL, to: backupURL)
                logger.info("Legacy save backed up to: \(backupURL.lastPathComponent)")

                // Delete original legacy file
                try FileManager.default.removeItem(at: legacySaveFileURL)
                logger.info("Legacy save file removed")
            } catch {
                logger.warning("Failed to backup/remove legacy save: \(error.localizedDescription)")
            }

            return newData
        }

        return nil
    }

    private func loadLegacyBinaryFormat() -> LegacyGameData? {
        guard let data = try? Data(contentsOf: legacySaveFileURL) else {
            return nil
        }

        var offset = 0

        // Read high score (Int32, 4 bytes)
        guard offset + 4 <= data.count else { return nil }
        let highScore = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4

        // Read GameStats structure (44 bytes total)
        // totalGamesPlayed, totalScore, totalCoinsCollected, storedCoins, totalDeaths,
        // totalPipesCleared, totalJumps, totalEnemiesKilled (8 × Int32 = 32 bytes)
        // totalPlayTime (Float = 4 bytes)
        // currentStreak, bestStreak (2 × Int32 = 8 bytes)
        guard offset + 44 <= data.count else { return nil }

        let totalGamesPlayed = data.withUnsafeBytes {
            $0.load(fromByteOffset: offset, as: Int32.self)
        }
        offset += 4
        let totalScore = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        let totalCoinsCollected = data.withUnsafeBytes {
            $0.load(fromByteOffset: offset, as: Int32.self)
        }
        offset += 4
        let storedCoins = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        let totalDeaths = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        let totalPipesCleared = data.withUnsafeBytes {
            $0.load(fromByteOffset: offset, as: Int32.self)
        }
        offset += 4
        let totalJumps = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        let totalEnemiesKilled = data.withUnsafeBytes {
            $0.load(fromByteOffset: offset, as: Int32.self)
        }
        offset += 4
        let totalPlayTime = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Float.self) }
        offset += 4
        let currentStreak = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4
        let bestStreak = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
        offset += 4

        let gameStats = LegacyGameData.LegacyGameStats(
            totalGamesPlayed: Int(totalGamesPlayed),
            totalScore: Int(totalScore),
            totalCoinsCollected: Int(totalCoinsCollected),
            storedCoins: Int(storedCoins),
            totalDeaths: Int(totalDeaths),
            totalPipesCleared: Int(totalPipesCleared),
            totalJumps: Int(totalJumps),
            totalEnemiesKilled: Int(totalEnemiesKilled),
            totalPlayTime: totalPlayTime,
            currentStreak: Int(currentStreak),
            bestStreak: Int(bestStreak)
        )

        // Read 6 level stats (each: Int32, Int32, Bool = 9 bytes × 6 = 54 bytes)
        var levelStats: [LegacyGameData.LegacyLevelSaveData] = []
        for _ in 1...6 {
            guard offset + 9 <= data.count else { return nil }

            let levelHighScore = data.withUnsafeBytes {
                $0.load(fromByteOffset: offset, as: Int32.self)
            }
            offset += 4
            let bestCoins = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Int32.self) }
            offset += 4
            let unlocked = data.withUnsafeBytes { $0.load(fromByteOffset: offset, as: Bool.self) }
            offset += 1

            levelStats.append(
                LegacyGameData.LegacyLevelSaveData(
                    highScore: Int(levelHighScore),
                    bestCoins: Int(bestCoins),
                    unlocked: unlocked
                ))
        }

        return LegacyGameData(
            highScore: Int(highScore),
            gameStats: gameStats,
            levelStats: levelStats
        )
    }

    private func convertToV2(legacy: LegacyGameData) -> GameSaveData {
        var newData = GameSaveData()

        // Convert progress
        newData.progress.legacyHighScore = legacy.highScore
        for (index, legacyLevel) in legacy.levelStats.enumerated() {
            if index < newData.progress.levels.count {
                newData.progress.levels[index].highScore = legacyLevel.highScore
                newData.progress.levels[index].bestCoins = legacyLevel.bestCoins
                newData.progress.levels[index].unlocked = legacyLevel.unlocked
            }
        }

        // Convert statistics
        newData.statistics.totalGamesPlayed = legacy.gameStats.totalGamesPlayed
        newData.statistics.totalScore = legacy.gameStats.totalScore
        newData.statistics.totalCoinsCollected = legacy.gameStats.totalCoinsCollected
        newData.statistics.storedCoins = legacy.gameStats.storedCoins
        newData.statistics.totalDeaths = legacy.gameStats.totalDeaths
        newData.statistics.totalPipesCleared = legacy.gameStats.totalPipesCleared
        newData.statistics.totalJumps = legacy.gameStats.totalJumps
        newData.statistics.totalEnemiesKilled = legacy.gameStats.totalEnemiesKilled
        newData.statistics.totalPlayTime = Double(legacy.gameStats.totalPlayTime)
        newData.statistics.currentStreak = legacy.gameStats.currentStreak
        newData.statistics.bestStreak = legacy.gameStats.bestStreak

        logger.info(
            "Converted legacy data - High score: \(legacy.highScore), Coins: \(legacy.gameStats.storedCoins)"
        )

        return newData
    }

    private func migrateSettingsIfNeeded() {
        guard FileManager.default.fileExists(atPath: legacySettingsFileURL.path) else {
            return
        }

        logger.info("Migrating legacy settings...")

        guard let contents = try? String(contentsOf: legacySettingsFileURL) else {
            logger.error("Failed to read legacy settings")
            return
        }

        let values = contents.split(separator: " ").compactMap { Float($0) }

        if values.count >= 3 {
            GameSettings.masterVolume = values[0]
            GameSettings.musicVolume = values[1]
            GameSettings.sfxVolume = values[2]

            if values.count >= 4 {
                GameSettings.debugMode = (values[3] > 0)
            }

            logger.info("Settings migrated successfully")

            // Delete legacy settings file
            do {
                try FileManager.default.removeItem(at: legacySettingsFileURL)
                logger.info("Legacy settings file removed")
            } catch {
                logger.warning("Failed to remove legacy settings: \(error.localizedDescription)")
            }
        }
    }
}

// MARK: - C++ Interop Functions

/// Load game data synchronously for C++ interop
/// Called directly from C++ during game initialization via Swift C++ interop
/// - Returns: JSON string containing game save data, or empty string if no save exists
public func loadGameDataSync() -> String {
    let logger = Logger(subsystem: "com.floppyturd.ios", category: "SaveManager")
    logger.info("Synchronous load requested from C++")

    guard let jsonString = SaveManager.processLoadGameCommandSync() else {
        logger.info("No save data to return to C++")
        return ""
    }

    logger.info("Returning JSON to C++ (\(jsonString.count) chars)")
    return jsonString
}
