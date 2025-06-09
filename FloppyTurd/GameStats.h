#pragma once
#include <string>
#include <raylib.h>

struct GameStats {
	int totalPipes = 0;          // Total pipes passed across all sessions
	int totalCoins = 0;          // Total coins collected
	int totalEnemiesKilled = 0;  // Total enemies killed
	int totalJumps = 0;          // Total jumps performed
	int totalFlops = 0;          // Total deaths
	int totalLevelTries = 0;     // Total attempts across all levels

	// Per-level high scores (pipes)
	int levelHighScores[6] = { 0, 0, 0, 0, 0, 0 };

	// Unlocked states
	bool hatUnlocked[18] = { false };  // 18 hats (4 default unlocked, 14 locked)
	bool skillUnlocked[5] = { false }; // 5 skills
	bool levelUnlocked[6] = { false }; // 6 levels (Park always true by default)

	// Load stats from binary file
	void Load();

	// Save stats to binary file
	void Save() const;
};