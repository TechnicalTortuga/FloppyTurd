#include "GameStats.h"
#include "RaylibCompat.h"
#include <fstream>
#include <cstring>

void GameStats::Load() {
    const char* filename = "floppy_turd_stats.bin";
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        TraceLog(LOG_INFO, "No stats file found, creating default: %s", filename);
        levelUnlocked[0] = true; // Park is always open for business!
        Save();
        return;
    }

    // Check file size to avoid reading garbage
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    if (fileSize < sizeof(int)) {
        TraceLog(LOG_WARNING, "Stats file too small: %lld bytes, expected at least %zu", fileSize, sizeof(int));
        file.close();
        Save(); // Create fresh file
        return;
    }

    // Read version
    int fileVersion;
    file.read(reinterpret_cast<char*>(&fileVersion), sizeof(int));
    if (fileVersion != 1) {
        TraceLog(LOG_WARNING, "Unknown stats file version: %d, resetting file", fileVersion);
        file.close();
        Save();
        return;
    }

    // Validate file size for full struct
    if (fileSize != sizeof(GameStats)) {
        TraceLog(LOG_WARNING, "Stats file size mismatch: %lld bytes, expected %zu", fileSize, sizeof(GameStats));
        file.close();
        Save();
        return;
    }

    // Read the struct (excluding version, already read)
    file.read(reinterpret_cast<char*>(this) + sizeof(int), sizeof(GameStats) - sizeof(int));
    file.close();

    levelUnlocked[0] = true; // Ensure Park is always unlocked
    TraceLog(LOG_INFO, "Loaded stats: version=%d, pipes=%d, coins=%d, highScore[3]=%d, levelUnlocked[3]=%d",
        version, totalPipes, totalCoins, levelHighScores[3], levelUnlocked[3]);
}

void GameStats::Save() const {
    const char* filename = "floppy_turd_stats.bin";
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open stats file for writing: %s", filename);
        return;
    }

    // Write the entire struct
    file.write(reinterpret_cast<const char*>(this), sizeof(GameStats));
    if (!file.good()) {
        TraceLog(LOG_ERROR, "Failed to write stats file: %s", filename);
        file.close();
        return;
    }
    file.close();
    TraceLog(LOG_INFO, "Saved stats: version=%d, pipes=%d, coins=%d, highScore[3]=%d, levelUnlocked[3]=%d",
        version, totalPipes, totalCoins, levelHighScores[3], levelUnlocked[3]);
}