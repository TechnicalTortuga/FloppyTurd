#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include "../../FloppyTurd/Game/FloppyTurdGame.h"

namespace GameCore {

    /**
     * @brief Helper functions for serializing/deserializing game data to/from JSON
     * 
     * These functions convert between C++ game data structures and JSON strings
     * that can be passed to the Swift SaveManager via the C bridge.
     */
    namespace SaveGameHelpers {

        /**
         * @brief Escape a string for JSON
         */
        inline std::string escapeJSON(const std::string& str) {
            std::ostringstream oss;
            for (char c : str) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\b': oss << "\\b"; break;
                    case '\f': oss << "\\f"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default:
                        if (c < 0x20) {
                            oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                        } else {
                            oss << c;
                        }
                }
            }
            return oss.str();
        }

        /**
         * @brief Serialize game data to JSON string
         * 
         * Converts FloppyTurdGame data to JSON format matching Swift's GameSaveData structure.
         * 
         * @param game Reference to the game instance
         * @return JSON string ready to pass to saveGameDataJSON()
         */
        inline std::string serializeGameData(const FloppyTurdGame& game) {
            std::ostringstream json;
            json << std::fixed << std::setprecision(2);

            json << "{\n";
            json << "  \"version\": 2,\n";
            json << "  \"saveDate\": \"" << "2024-01-01T00:00:00Z" << "\",\n";  // Will be updated by SaveManager
            json << "  \"playerId\": null,\n";

            // Progress data
            json << "  \"progress\": {\n";
            json << "    \"legacyHighScore\": " << game.GetHighScore() << ",\n";
            json << "    \"levels\": [\n";

            const int maxLevels = 6;
            for (int i = 1; i <= maxLevels; ++i) {
                const auto& stats = game.GetLevelStats(i);
                json << "      {\n";
                json << "        \"levelId\": " << i << ",\n";
                json << "        \"highScore\": " << stats.highScore << ",\n";
                json << "        \"bestCoins\": " << stats.bestCoins << ",\n";
                json << "        \"unlocked\": " << (stats.unlocked ? "true" : "false") << ",\n";
                json << "        \"timesPlayed\": 0,\n";
                json << "        \"timesCompleted\": 0,\n";
                json << "        \"fastestTime\": " << (stats.bestBossTime > 0 ? std::to_string(stats.bestBossTime) : "null") << "\n";
                json << "      }";
                if (i < maxLevels) json << ",";
                json << "\n";
            }

            json << "    ]\n";
            json << "  },\n";

            // Statistics data
            const auto& gameStats = game.GetGameStats();
            json << "  \"statistics\": {\n";
            json << "    \"totalGamesPlayed\": " << gameStats.totalGamesPlayed << ",\n";
            json << "    \"totalScore\": " << gameStats.totalScore << ",\n";
            json << "    \"totalCoinsCollected\": " << gameStats.totalCoinsCollected << ",\n";
            json << "    \"storedCoins\": " << gameStats.storedCoins << ",\n";
            json << "    \"totalDeaths\": " << gameStats.totalDeaths << ",\n";
            json << "    \"totalPipesCleared\": " << gameStats.totalPipesCleared << ",\n";
            json << "    \"totalJumps\": " << gameStats.totalJumps << ",\n";
            json << "    \"totalEnemiesKilled\": " << gameStats.totalEnemiesKilled << ",\n";
            json << "    \"sessionEnemiesKilled\": " << gameStats.sessionEnemiesKilled << ",\n";
            json << "    \"totalPlayTime\": " << gameStats.totalPlayTime << ",\n";
            json << "    \"currentStreak\": " << gameStats.currentStreak << ",\n";
            json << "    \"bestStreak\": " << gameStats.bestStreak << ",\n";
            json << "    \"totalProjectilesFired\": 0,\n";
            json << "    \"totalHatsUnlocked\": 0,\n";
            json << "    \"bossesDefeated\": 0\n";
            json << "  },\n";

            // Customization data - get from game's customization data
            const auto& customization = game.GetCustomizationData();
            json << "  \"customization\": {\n";
            json << "    \"equippedHatIndex\": " << customization.equippedHatIndex << ",\n";
            json << "    \"selectedHatIndex\": " << customization.selectedHatIndex << ",\n";
            json << "    \"unlockedHats\": [";
            
            // Serialize unlocked hats array
            for (size_t i = 0; i < customization.unlockedHats.size(); ++i) {
                if (i > 0) json << ", ";
                json << (customization.unlockedHats[i] ? "true" : "false");
            }

            json << "],\n";
            json << "    \"unlockedSkills\": [";
            
            // Serialize unlocked skills array
            for (size_t i = 0; i < customization.unlockedSkills.size(); ++i) {
                if (i > 0) json << ", ";
                json << (customization.unlockedSkills[i] ? "true" : "false");
            }

            json << "]\n";
            json << "  },\n";

            // Settings data - audio, difficulty, debug, and haptics
            json << "  \"settings\": {\n";
            json << "    \"masterVolume\": " << game.GetMasterVolume() << ",\n";
            json << "    \"musicVolume\": " << game.GetMusicVolume() << ",\n";
            json << "    \"sfxVolume\": " << game.GetSFXVolume() << ",\n";
            json << "    \"difficulty\": " << static_cast<int>(GameCore::LevelManager::GetGlobalDifficulty()) << ",\n";
            json << "    \"debugMode\": " << (game.GetDebugMode() ? "true" : "false") << ",\n";
            json << "    \"hapticsEnabled\": " << (game.GetVibrationsEnabled() ? "true" : "false") << "\n";
            json << "  }\n";
            json << "}";

            return json.str();
        }

        /**
         * @brief Parse JSON value as int
         * 
         * Simple parser for extracting integer values from JSON.
         * Format: "key": value
         */
        inline int parseJSONInt(const std::string& json, const std::string& key) {
            // Try multiple patterns to handle Swift's flexible spacing
            std::string patterns[] = {
                "\"" + key + "\" : ",   // Swift format: "key" : value
                "\"" + key + "\":",     // Compact format: "key":value
                "\"" + key + "\": "     // Standard format: "key": value
            };
            
            size_t pos = std::string::npos;
            size_t keyLen = 0;
            
            for (const auto& pattern : patterns) {
                pos = json.find(pattern);
                if (pos != std::string::npos) {
                    keyLen = pattern.length();
                    break;
                }
            }
            
            if (pos == std::string::npos) {
                GN_LOG_WARN("🔍 parseJSONInt: Key '" + key + "' not found in JSON");
                return 0;
            }

            pos += keyLen;
            // Skip any additional whitespace
            while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
                pos++;
            }

            // Parse number
            std::string numStr;
            while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '-')) {
                numStr += json[pos++];
            }

            if (numStr.empty()) {
                GN_LOG_WARN("🔍 parseJSONInt: Failed to parse number for key '" + key + "'");
                return 0;
            }

            return std::stoi(numStr);
        }

        /**
         * @brief Parse JSON value as bool
         */
        inline bool parseJSONBool(const std::string& json, const std::string& key) {
            // Try multiple patterns to handle Swift's flexible spacing
            std::string patterns[] = {
                "\"" + key + "\" : ",   // Swift format: "key" : value
                "\"" + key + "\":",     // Compact format: "key":value
                "\"" + key + "\": "     // Standard format: "key": value
            };
            
            size_t pos = std::string::npos;
            size_t keyLen = 0;
            
            for (const auto& pattern : patterns) {
                pos = json.find(pattern);
                if (pos != std::string::npos) {
                    keyLen = pattern.length();
                    break;
                }
            }
            
            if (pos == std::string::npos) {
                GN_LOG_WARN("🔍 parseJSONBool: Key '" + key + "' not found in JSON");
                return false;
            }

            pos += keyLen;
            // Skip any additional whitespace
            while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
                pos++;
            }

            // Check if the next characters are "true"
            if (pos + 4 <= json.length() && json.substr(pos, 4) == "true") {
                return true;
            }
            
            // Check if the next characters are "false"
            if (pos + 5 <= json.length() && json.substr(pos, 5) == "false") {
                return false;
            }

            GN_LOG_WARN("🔍 parseJSONBool: Failed to parse boolean for key '" + key + "'");
            return false; // Default to false if neither found
        }

        /**
         * @brief Parse JSON value as float
         */
        inline float parseJSONFloat(const std::string& json, const std::string& key) {
            // Try multiple patterns to handle Swift's flexible spacing
            std::string patterns[] = {
                "\"" + key + "\" : ",   // Swift format: "key" : value
                "\"" + key + "\":",     // Compact format: "key":value
                "\"" + key + "\": "     // Standard format: "key": value
            };
            
            size_t pos = std::string::npos;
            size_t keyLen = 0;
            
            for (const auto& pattern : patterns) {
                pos = json.find(pattern);
                if (pos != std::string::npos) {
                    keyLen = pattern.length();
                    break;
                }
            }
            
            if (pos == std::string::npos) {
                // Don't log warning for optional fields like bestBossTime/fastestTime
                return 0.0f;
            }

            pos += keyLen;
            // Skip any additional whitespace
            while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
                pos++;
            }

            // Parse number
            std::string numStr;
            while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '-' || json[pos] == '.')) {
                numStr += json[pos++];
            }

            return numStr.empty() ? 0.0f : std::stof(numStr);
        }

        /**
         * @brief Parse level data from JSON array
         * 
         * Extracts a specific level's data from the "levels" array in the JSON.
         */
        inline void parseLevelData(const std::string& json, int levelId, int& highScore, int& bestCoins, float& bestBossTime, bool& unlocked) {
            // Find the levels array
            size_t levelsPos = json.find("\"levels\":");
            if (levelsPos == std::string::npos) return;

            // Find the specific level object (levelId - 1 because array is 0-indexed but levels are 1-indexed)
            // Swift outputs "levelId" : X with spaces around the colon
            std::string levelIdStr = "\"levelId\" : " + std::to_string(levelId);
            size_t levelPos = json.find(levelIdStr, levelsPos);
            if (levelPos == std::string::npos) return;

            // Find the start of this level's object
            size_t objStart = json.rfind('{', levelPos);
            size_t objEnd = json.find('}', levelPos);
            if (objStart == std::string::npos || objEnd == std::string::npos) {
                GN_LOG_WARN("🔍 parseLevelData: Failed to find level object boundaries for level " + std::to_string(levelId));
                return;
            }

            std::string levelJSON = json.substr(objStart, objEnd - objStart + 1);

            highScore = parseJSONInt(levelJSON, "highScore");
            bestCoins = parseJSONInt(levelJSON, "bestCoins");
            bestBossTime = parseJSONFloat(levelJSON, "fastestTime");  // Swift uses "fastestTime"
            unlocked = parseJSONBool(levelJSON, "unlocked");
            
            GN_LOG_INFO("🔍 parseLevelData: Level " + std::to_string(levelId) + 
                       " - unlocked=" + std::to_string(unlocked) +
                       ", highScore=" + std::to_string(highScore));
        }

        /**
         * @brief Parse hat unlock status from JSON array
         */
        inline bool parseHatUnlocked(const std::string& json, int hatIndex) {
            size_t hatsPos = json.find("\"unlockedHats\":");
            if (hatsPos == std::string::npos) return (hatIndex == 0); // Default: only first hat unlocked

            size_t arrayStart = json.find('[', hatsPos);
            if (arrayStart == std::string::npos) return (hatIndex == 0);

            // Count to the hatIndex-th value
            int currentIndex = 0;
            size_t pos = arrayStart + 1;
            while (pos < json.length() && currentIndex < hatIndex) {
                if (json[pos] == ',') currentIndex++;
                pos++;
            }

            // Find the next "true" or "false"
            size_t truePos = json.find("true", pos);
            size_t falsePos = json.find("false", pos);
            size_t commaPos = json.find(',', pos);
            size_t bracketPos = json.find(']', pos);

            // Check which comes first
            if (truePos != std::string::npos && 
                truePos < std::min(falsePos, std::min(commaPos, bracketPos))) {
                return true;
            }

            return false;
        }

        /**
         * @brief Parse simple key:value format from Swift
         * 
         * Swift now sends structured data in KEY:VALUE format (one per line)
         * instead of complex JSON. This is simpler, more reliable, and harder to manually edit.
         */
        inline std::string parseKeyValue(const std::string& data, const std::string& key) {
            std::string searchKey = key + ":";
            size_t pos = data.find(searchKey);
            if (pos == std::string::npos) {
                return "";
            }
            
            // Move past the key and colon
            pos += searchKey.length();
            
            // Find the end of the line
            size_t endPos = data.find('\n', pos);
            if (endPos == std::string::npos) {
                endPos = data.length();
            }
            
            return data.substr(pos, endPos - pos);
        }

        /**
         * @brief Deserialize game data from structured format
         * 
         * Parses the structured KEY:VALUE data returned from Swift and updates the game state.
         * This replaces the fragile JSON parser with a simple, reliable format.
         * 
         * @param game Reference to the game instance to update
         * @param dataString Structured data string from Swift (KEY:VALUE format)
         * @return true if parsing succeeded, false otherwise
         */
        inline bool deserializeGameData(FloppyTurdGame& game, const std::string& dataString) {
            if (dataString.empty()) {
                GN_LOG_ERROR("🔍 Deserialize: Data string is empty!");
                return false;
            }

            GN_LOG_INFO("=== DESERIALIZE START ===");
            GN_LOG_INFO("🔍 Deserialize: Data length: " + std::to_string(dataString.length()));

            // Parse statistics
            FloppyTurdGame::GameStats stats;
            stats.totalGamesPlayed = std::stoi(parseKeyValue(dataString, "STATS_TOTAL_GAMES"));
            stats.totalScore = std::stoi(parseKeyValue(dataString, "STATS_TOTAL_SCORE"));
            stats.totalCoinsCollected = std::stoi(parseKeyValue(dataString, "STATS_TOTAL_COINS"));
            stats.storedCoins = std::stoi(parseKeyValue(dataString, "STATS_STORED_COINS"));
            stats.totalDeaths = std::stoi(parseKeyValue(dataString, "STATS_TOTAL_DEATHS"));
            stats.totalPipesCleared = std::stoi(parseKeyValue(dataString, "STATS_TOTAL_PIPES"));
            stats.totalJumps = std::stoi(parseKeyValue(dataString, "STATS_TOTAL_JUMPS"));
            stats.totalEnemiesKilled = std::stoi(parseKeyValue(dataString, "STATS_TOTAL_ENEMIES"));
            stats.sessionEnemiesKilled = 0; // Not persisted
            
            std::string playTimeStr = parseKeyValue(dataString, "STATS_PLAY_TIME");
            stats.totalPlayTime = playTimeStr.empty() ? 0.0f : std::stof(playTimeStr);
            
            stats.currentStreak = std::stoi(parseKeyValue(dataString, "STATS_CURRENT_STREAK"));
            stats.bestStreak = std::stoi(parseKeyValue(dataString, "STATS_BEST_STREAK"));

            GN_LOG_INFO("🔍 Deserialize: Parsed stats - storedCoins=" + std::to_string(stats.storedCoins) + 
                       ", totalGamesPlayed=" + std::to_string(stats.totalGamesPlayed) +
                       ", totalPipesCleared=" + std::to_string(stats.totalPipesCleared));

            game.UpdateGameStats(stats);

            // Parse level data
            const int maxLevels = 6;
            for (int i = 1; i <= maxLevels; ++i) {
                std::string levelPrefix = "LEVEL_" + std::to_string(i) + "_";
                
                FloppyTurdGame::LevelStats levelStats;
                
                std::string highScoreStr = parseKeyValue(dataString, levelPrefix + "HIGH_SCORE");
                levelStats.highScore = highScoreStr.empty() ? 0 : std::stoi(highScoreStr);
                
                std::string bestCoinsStr = parseKeyValue(dataString, levelPrefix + "BEST_COINS");
                levelStats.bestCoins = bestCoinsStr.empty() ? 0 : std::stoi(bestCoinsStr);
                
                std::string unlockedStr = parseKeyValue(dataString, levelPrefix + "UNLOCKED");
                levelStats.unlocked = (unlockedStr == "1");
                
                std::string fastestTimeStr = parseKeyValue(dataString, levelPrefix + "FASTEST_TIME");
                levelStats.bestBossTime = fastestTimeStr.empty() ? 0.0f : std::stof(fastestTimeStr);
                
                // Requirements will be set by SetDefaultUnlockRequirements
                levelStats.unlockRequirement = 0;
                levelStats.coinRequirement = 0;

                GN_LOG_INFO("🔍 Deserialize: Level " + std::to_string(i) + 
                           " parsed - unlocked=" + std::to_string(levelStats.unlocked) +
                           ", highScore=" + std::to_string(levelStats.highScore) +
                           ", bestCoins=" + std::to_string(levelStats.bestCoins));

                game.UpdateLevelStats(i, levelStats);
            }

            // Parse customization
            FloppyTurdGame::CustomizationData customization;
            
            std::string equippedHatStr = parseKeyValue(dataString, "CUSTOM_EQUIPPED_HAT");
            customization.equippedHatIndex = equippedHatStr.empty() ? -1 : std::stoi(equippedHatStr);
            
            std::string selectedHatStr = parseKeyValue(dataString, "CUSTOM_SELECTED_HAT");
            customization.selectedHatIndex = selectedHatStr.empty() ? -1 : std::stoi(selectedHatStr);
            
            // Parse unlocked hats
            customization.unlockedHats.clear();
            for (int i = 0; i < 15; ++i) {
                std::string hatKey = "CUSTOM_HAT_" + std::to_string(i) + "_UNLOCKED";
                std::string hatStr = parseKeyValue(dataString, hatKey);
                customization.unlockedHats.push_back(hatStr == "1");
            }
            
            // Parse unlocked skills
            customization.unlockedSkills.clear();
            for (int i = 0; i < 5; ++i) {
                std::string skillKey = "CUSTOM_SKILL_" + std::to_string(i) + "_UNLOCKED";
                std::string skillStr = parseKeyValue(dataString, skillKey);
                customization.unlockedSkills.push_back(skillStr == "1");
            }
            
            game.UpdateCustomizationData(customization);

            // Parse settings (audio and difficulty)
            std::string masterVolStr = parseKeyValue(dataString, "SETTINGS_MASTER_VOLUME");
            if (!masterVolStr.empty()) {
                game.SetMasterVolume(std::stof(masterVolStr));
            }
            
            std::string musicVolStr = parseKeyValue(dataString, "SETTINGS_MUSIC_VOLUME");
            if (!musicVolStr.empty()) {
                game.SetMusicVolume(std::stof(musicVolStr));
            }
            
            std::string sfxVolStr = parseKeyValue(dataString, "SETTINGS_SFX_VOLUME");
            if (!sfxVolStr.empty()) {
                game.SetSFXVolume(std::stof(sfxVolStr));
            }
            
            std::string difficultyStr = parseKeyValue(dataString, "SETTINGS_DIFFICULTY");
            if (!difficultyStr.empty()) {
                int difficultyValue = std::stoi(difficultyStr);
                GameCore::LevelManager::SetGlobalDifficulty(static_cast<GameCore::Difficulty>(difficultyValue));
                GN_LOG_INFO("🔍 Deserialize: Loaded difficulty = " + std::to_string(difficultyValue));
            }
            
            std::string debugModeStr = parseKeyValue(dataString, "SETTINGS_DEBUG_MODE");
            if (!debugModeStr.empty()) {
                bool debugMode = (debugModeStr == "true" || debugModeStr == "1");
                game.SetDebugMode(debugMode);
                GN_LOG_INFO("🔍 Deserialize: Loaded debug mode = " + std::string(debugMode ? "enabled" : "disabled"));
            }
            
            std::string hapticsStr = parseKeyValue(dataString, "SETTINGS_HAPTICS_ENABLED");
            if (!hapticsStr.empty()) {
                bool hapticsEnabled = (hapticsStr == "true" || hapticsStr == "1");
                game.SetVibrationsEnabled(hapticsEnabled);
                GN_LOG_INFO("🔍 Deserialize: Loaded haptics = " + std::string(hapticsEnabled ? "enabled" : "disabled"));
            }

            GN_LOG_INFO("=== DESERIALIZE END - SUCCESS ===");
            GN_LOG_INFO("🔍 Final parsed values - storedCoins=" + std::to_string(stats.storedCoins) + 
                       ", totalPipesCleared=" + std::to_string(stats.totalPipesCleared));

            return true;
        }

    } // namespace SaveGameHelpers

} // namespace GameCore