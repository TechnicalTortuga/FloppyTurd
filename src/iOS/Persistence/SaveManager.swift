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
    private let v2SaveFileName = "floppyturd_save_v2.json"  // V2 JSON format
    private let legacySaveFileName = "floppyturd_save.dat"
    private let legacySettingsFileName = "floppyturd_settings.cfg"

    private var documentsURL: URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
    }

    private var saveFileURL: URL {
        documentsURL.appendingPathComponent(saveFileName)
    }

    private var v2SaveFileURL: URL {
        documentsURL.appendingPathComponent(v2SaveFileName)
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
        logger.info("📁 Documents directory: \(self.documentsURL.path)")
        logger.info("📁 Save file path: \(self.saveFileURL.path)")
        logger.info(
            "📁 Save file exists: \(FileManager.default.fileExists(atPath: self.saveFileURL.path))")

        // List all files in Documents directory
        do {
            let files = try FileManager.default.contentsOfDirectory(atPath: documentsURL.path)
            logger.info("📁 Files in Documents: \(files)")
        } catch {
            logger.error("📁 Failed to list Documents directory: \(error)")
        }

        // Register default settings
        GameSettings.registerDefaults()

        // Migrate legacy settings if present
        migrateSettingsIfNeeded()
    }

    // MARK: - Public API

    /// Load game data from disk (migrates legacy format if needed)
    nonisolated func load() -> GameSaveData? {
        logger.info("🔄 Loading game data...")
        logger.info("🔍 Documents directory: \(self.documentsURL.path)")
        logger.info("🔍 Save file path: \(self.saveFileURL.path)")
        logger.info("🔍 V2 JSON save path: \(self.v2SaveFileURL.path)")
        logger.info("🔍 Legacy save path: \(self.legacySaveFileURL.path)")

        // Check if we need to migrate from v2 JSON format
        if FileManager.default.fileExists(atPath: v2SaveFileURL.path) {
            logger.info("📦 V2 JSON save file detected - migrating to V3...")
            if let migrated = migrateV2JSONSave() {
                logger.info("✅ V2 to V3 migration successful")
                // Save the migrated data in v3 format
                if saveSync(migrated) {
                    logger.info("✅ Migrated data saved to V3 format")
                    // Delete old v2 file after successful migration
                    try? FileManager.default.removeItem(at: v2SaveFileURL)
                    logger.info("✅ Old V2 JSON file deleted")
                }
                return migrated
            } else {
                logger.warning("⚠️ V2 migration failed, trying legacy migration...")
            }
        }

        // Check if we need to migrate from legacy binary format
        if FileManager.default.fileExists(atPath: legacySaveFileURL.path) {
            logger.info("📦 Legacy binary save file detected - migrating...")
            if let migrated = migrateLegacySave() {
                logger.info("✅ Legacy migration successful")
                return migrated
            } else {
                logger.warning("⚠️ Legacy migration failed, starting fresh")
                return nil
            }
        }

        // Load from binary plist format
        guard FileManager.default.fileExists(atPath: saveFileURL.path) else {
            logger.info("ℹ️ No save file found at path, starting fresh")
            return nil
        }

        logger.info("✅ Save file exists, attempting to load...")

        logger.info("✅ Save file exists at: \(self.saveFileURL.path)")

        do {
            let combinedData = try Data(contentsOf: saveFileURL)
            logger.info("📊 Loaded \(combinedData.count) bytes from save file")

            // File format: [HMAC (32 bytes)] + [PropertyList Data]
            guard combinedData.count > 32 else {
                logger.warning(
                    "⚠️ Save file too small (\(combinedData.count) bytes), possibly corrupted")
                return nil
            }

            let hmacData = combinedData.prefix(32)
            let plistData = combinedData.suffix(from: 32)
            logger.info(
                "🔐 Verifying HMAC (\(hmacData.count) bytes HMAC, \(plistData.count) bytes data)...")

            // Verify HMAC (detect tampering)
            let expectedHMAC = HMAC<SHA256>.authenticationCode(for: plistData, using: Self.hmacKey)
            guard hmacData == Data(expectedHMAC) else {
                logger.warning("⚠️ Save file HMAC mismatch - file may have been tampered with!")
                logger.warning("⚠️ Starting fresh to prevent cheating")
                return nil
            }
            logger.info("✅ HMAC verification passed")

            // Decode binary plist
            logger.info("🔄 Decoding binary plist...")
            let decoder = PropertyListDecoder()
            var saveData = try decoder.decode(GameSaveData.self, from: plistData)
            logger.info("✅ Binary plist decoded successfully")

            // Validate loaded data
            logger.info("🔄 Validating loaded data...")
            if validateLoadedData(&saveData) {
                logger.info(
                    "✅ Game data loaded successfully (version \(saveData.version), HMAC verified)")
                logger.info(
                    "📊 Loaded stats - Coins: \(saveData.statistics.storedCoins), Games: \(saveData.statistics.totalGamesPlayed)"
                )

                // Apply settings from save data to GameSettings (UserDefaults)
                GameSettings.masterVolume = saveData.settings.masterVolume
                GameSettings.musicVolume = saveData.settings.musicVolume
                GameSettings.sfxVolume = saveData.settings.sfxVolume
                GameSettings.difficulty = saveData.settings.difficulty
                GameSettings.debugMode = saveData.settings.debugMode
                GameSettings.hapticsEnabled = saveData.settings.hapticsEnabled
                logger.info(
                    "✅ Settings applied from save data: master=\(saveData.settings.masterVolume) music=\(saveData.settings.musicVolume) sfx=\(saveData.settings.sfxVolume)"
                )
                logger.info(
                    "✅ Settings applied: difficulty=\(saveData.settings.difficulty) debug=\(saveData.settings.debugMode) haptics=\(saveData.settings.hapticsEnabled)"
                )

                return saveData
            } else {
                logger.warning("⚠️ Loaded data failed validation, starting fresh")
                return nil
            }
        } catch {
            logger.error("❌ Failed to load game data: \(error.localizedDescription)")
            logger.error("❌ Error details: \(error)")
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
        shared.logger.info("💾 Processing save game command...")
        shared.logger.debug("💾 JSON length: \(jsonString.count) characters")

        // Still receive JSON from C++ for now (but save as binary plist)
        guard let jsonData = jsonString.data(using: .utf8) else {
            shared.logger.error("❌ Failed to convert JSON string to Data")
            return false
        }

        do {
            let decoder = JSONDecoder()
            decoder.dateDecodingStrategy = .iso8601
            let saveData = try decoder.decode(GameSaveData.self, from: jsonData)

            // Settings come from C++ (in the JSON), so update GameSettings (UserDefaults) to match
            // This ensures UserDefaults stays in sync with the authoritative C++ game state
            GameSettings.masterVolume = saveData.settings.masterVolume
            GameSettings.musicVolume = saveData.settings.musicVolume
            GameSettings.sfxVolume = saveData.settings.sfxVolume
            GameSettings.difficulty = saveData.settings.difficulty
            GameSettings.debugMode = saveData.settings.debugMode
            GameSettings.hapticsEnabled = saveData.settings.hapticsEnabled
            shared.logger.info(
                "💾 Settings from C++ saved: master=\(saveData.settings.masterVolume) music=\(saveData.settings.musicVolume) sfx=\(saveData.settings.sfxVolume) difficulty=\(saveData.settings.difficulty) debug=\(saveData.settings.debugMode) haptics=\(saveData.settings.hapticsEnabled)"
            )

            // Save using SaveManager (will save as BINARY PLIST with HMAC)
            shared.logger.info("💾 Saving to: \(shared.saveFileURL.path)")
            shared.logger.info(
                "💾 Data snapshot - storedCoins: \(saveData.statistics.storedCoins), totalGames: \(saveData.statistics.totalGamesPlayed)"
            )

            let success = SaveManager.shared.saveSync(saveData)
            if success {
                shared.logger.info("✅ Game data saved successfully (binary plist with HMAC)")
                shared.logger.info(
                    "✅ File exists after save: \(FileManager.default.fileExists(atPath: shared.saveFileURL.path))"
                )
            } else {
                shared.logger.error("❌ Failed to save game data")
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
    /// Process a load game command (synchronous version for C++ during initialization)
    /// - Returns: Simple key:value format string, or nil if no save exists
    /// - Note: This is the synchronous version called directly from C++ during initialization
    static func processLoadGameCommandSync() -> String? {
        shared.logger.info("🔍 Processing SYNCHRONOUS load game command...")
        shared.logger.info("🔍 Save file path: \(shared.saveFileURL.path)")
        shared.logger.info(
            "🔍 Save file exists: \(FileManager.default.fileExists(atPath: shared.saveFileURL.path))"
        )

        if FileManager.default.fileExists(atPath: shared.saveFileURL.path) {
            do {
                let attrs = try FileManager.default.attributesOfItem(
                    atPath: shared.saveFileURL.path)
                if let size = attrs[.size] as? Int {
                    shared.logger.info("🔍 Save file size: \(size) bytes")
                }
                if let modDate = attrs[.modificationDate] as? Date {
                    shared.logger.info("🔍 Save file modified: \(modDate)")
                }
            } catch {
                shared.logger.error("🔍 Failed to get file attributes: \(error)")
            }
        }

        guard let saveData = SaveManager.shared.load() else {
            shared.logger.warning("⚠️ No save data found (sync load) - load() returned nil")
            return nil
        }

        shared.logger.info("✅ SaveData loaded successfully from file")

        shared.logger.info("✅ Save data loaded, converting to KEY:VALUE format")

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

        // Settings (audio, difficulty, debug, haptics)
        output += "SETTINGS_MASTER_VOLUME:\(saveData.settings.masterVolume)\n"
        output += "SETTINGS_MUSIC_VOLUME:\(saveData.settings.musicVolume)\n"
        output += "SETTINGS_SFX_VOLUME:\(saveData.settings.sfxVolume)\n"
        output += "SETTINGS_DIFFICULTY:\(saveData.settings.difficulty)\n"
        output += "SETTINGS_DEBUG_MODE:\(saveData.settings.debugMode ? 1 : 0)\n"
        output += "SETTINGS_HAPTICS_ENABLED:\(saveData.settings.hapticsEnabled ? 1 : 0)\n"

        shared.logger.info("✅ Game data loaded successfully (SYNC): \(output.count) bytes")
        shared.logger.info(
            "📊 Sample data - Coins: \(saveData.statistics.storedCoins), Total games: \(saveData.statistics.totalGamesPlayed)"
        )
        shared.logger.info("📊 Level 2 unlocked: \(saveData.progress.levels[1].unlocked)")
        shared.logger.info(
            "📊 Settings - Master: \(saveData.settings.masterVolume), Music: \(saveData.settings.musicVolume), SFX: \(saveData.settings.sfxVolume)"
        )
        shared.logger.info(
            "📊 Settings - Difficulty: \(saveData.settings.difficulty), Debug: \(saveData.settings.debugMode), Haptics: \(saveData.settings.hapticsEnabled)"
        )

        return output
    }

    /// Process a load settings command
    /// - Returns: Tuple of (masterVolume, musicVolume, sfxVolume, difficulty, debugMode, hapticsEnabled, wasLoaded)
    static func processLoadSettingsCommand() -> (Float, Float, Float, Int, Bool, Bool, Bool) {
        shared.logger.info("Processing load settings command...")

        let masterVolume = GameSettings.masterVolume
        let musicVolume = GameSettings.musicVolume
        let sfxVolume = GameSettings.sfxVolume
        let difficulty = GameSettings.difficulty
        let debugMode = GameSettings.debugMode
        let hapticsEnabled = GameSettings.hapticsEnabled

        // Check if we're using default values (first launch)
        let isFirstLaunch = !UserDefaults.standard.bool(forKey: "hasLaunchedBefore")
        if isFirstLaunch {
            UserDefaults.standard.set(true, forKey: "hasLaunchedBefore")
            shared.logger.info("First launch detected - using default settings")
        }

        shared.logger.info(
            "Settings loaded: master=\(masterVolume) music=\(musicVolume) sfx=\(sfxVolume) difficulty=\(difficulty) debug=\(debugMode) haptics=\(hapticsEnabled)"
        )

        return (
            masterVolume, musicVolume, sfxVolume, difficulty, debugMode, hapticsEnabled,
            !isFirstLaunch
        )
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

    /// Migrate from V2 JSON format to V3 binary plist format
    func migrateV2JSONSave() -> GameSaveData? {
        logger.info("🔄 Attempting to migrate V2 JSON save...")

        guard FileManager.default.fileExists(atPath: v2SaveFileURL.path) else {
            logger.warning("⚠️ V2 JSON save file does not exist")
            return nil
        }

        do {
            let jsonData = try Data(contentsOf: v2SaveFileURL)
            logger.info("📊 Loaded V2 JSON file: \(jsonData.count) bytes")

            let decoder = JSONDecoder()
            decoder.dateDecodingStrategy = .iso8601
            var saveData = try decoder.decode(GameSaveData.self, from: jsonData)

            logger.info("✅ V2 JSON decoded successfully")
            logger.info(
                "📊 V2 data - storedCoins: \(saveData.statistics.storedCoins), totalGames: \(saveData.statistics.totalGamesPlayed)"
            )
            logger.info(
                "📊 V2 data - Level 1 score: \(saveData.progress.levels[0].highScore), Level 2 unlocked: \(saveData.progress.levels[1].unlocked)"
            )

            // Update version to 3
            saveData.saveDate = Date()

            // Validate the loaded data
            if validateLoadedData(&saveData) {
                logger.info("✅ V2 migration successful - data validated")
                return saveData
            } else {
                logger.warning("⚠️ V2 migrated data failed validation")
                return nil
            }
        } catch {
            logger.error("❌ Failed to migrate V2 JSON save: \(error.localizedDescription)")
            logger.error("❌ Error details: \(error)")
            return nil
        }
    }

    func migrateLegacySave() -> GameSaveData? {
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
    logger.info("🔍 Synchronous load requested from C++")

    guard let jsonString = SaveManager.processLoadGameCommandSync() else {
        logger.warning("⚠️ No save data to return to C++ - processLoadGameCommandSync returned nil")
        return ""
    }

    logger.info("✅ Returning KEY:VALUE data to C++ (\(jsonString.count) chars)")
    return jsonString
}
