#pragma once
#include <string>
#include "PlatformAPI.h"

struct GameStats {
    int version = 1;             // File format version
    int totalPipes = 0;          // Total pipes passed across all sessions
    int totalCoins = 0;          // Total coins collected
    int totalEnemiesKilled = 0;  // Total enemies killed
    int totalJumps = 0;          // Total jumps performed
    int totalFlops = 0;          // Total deaths
    int totalLevelTries = 0;     // Total attempts across all levels

    // Per-level high scores (pipes)
    int levelHighScores[6] = { 0, 0, 0, 0, 0, 0 };

    // Unlocked states
    bool hatUnlocked[15] = { false }; // 15 hats
    bool skillUnlocked[6] = { false }; // 6 skills
    bool levelUnlocked[6] = { false }; // 6 levels (Park always true)

    void Load();
    void Save() const;
};