#include "GameStats.h"
#include <raylib.h>
#include <fstream>

void GameStats::Load() {
	const char* filename = "floppy_turd_stats.bin";
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		// File doesn't exist, create default on first run
		Save();
		return;
	}

	// Read the entire struct in one go
	file.read(reinterpret_cast<char*>(this), sizeof(GameStats));
	file.close();

	// Ensure Park level is always unlocked
	levelUnlocked[0] = true;
}

void GameStats::Save() const {
	const char* filename = "floppy_turd_stats.bin";
	std::ofstream file(filename, std::ios::binary);
	if (file.is_open()) {
		// Write the entire struct in one go
		file.write(reinterpret_cast<const char*>(this), sizeof(GameStats));
		file.close();
	}
	else {
		TraceLog(LOG_WARNING, "Failed to save file: %s", filename);
	}

	// Ensure Park level is always unlocked after save
	GameStats temp = *this;
	temp.levelUnlocked[0] = true;
	std::ofstream fixFile(filename, std::ios::binary);
	if (fixFile.is_open()) {
		fixFile.write(reinterpret_cast<const char*>(&temp), sizeof(GameStats));
		fixFile.close();
	}
}