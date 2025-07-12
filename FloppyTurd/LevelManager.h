#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <iostream>
#include "PlatformAPI.h"
#include "Level.h"
#include "Enemy.h"
#include "Boss.h"
#include <set>
#include "SewerPipe.h"
#include "SnowmanEnemy.h"
#include "Bird.h"
#include "QuickplaySettings.h"

class LevelManager {
public:
    LevelManager(std::shared_ptr<Level> level);
    ~LevelManager();

    void Update(float deltaTime);
    void Draw() const;

    Vector2 GetPlayerPosition() const;
    void SetEnemySpawnInterval(float interval);
    std::shared_ptr<Level> GetCurrentLevel() const;
    std::vector<std::shared_ptr<Enemy>>& GetEnemies();
    std::shared_ptr<Boss> GetBoss();
    void SetPlayerPosition(Vector2 pos) { lastPlayerPosition = pos; }
    static LevelManager* GetInstance();
    void SetPipePanSpeedMultiplier(float multiplier, float duration);
    void SetLevel(std::shared_ptr<Level> newLevel, int difficulty); // Updated to accept difficulty
    void SetQuickplaySettings(const QuickplaySettings& settings);
    float GetPickupPanSpeed() const; // Get pan speed for enemies and pickups
    int GetDifficultyIndex() const;  // Method to get difficulty index
    bool hasPassedFirstToilet = false;

private:
    void SpawnEnemy();
    bool IsSpawnPositionValid(const Rectangle& enemyHitbox) const;
    void UpdateEnemyFactory(); // Added to match .cpp usage
    std::shared_ptr<Level> currentLevel;
    std::vector<std::shared_ptr<Enemy>> enemies;
    std::shared_ptr<Boss> boss;
    float enemySpawnInterval;
    float enemySpawnTimer;
    std::function<std::shared_ptr<Enemy>(Vector2 spawnPos)> enemyFactory;
    Vector2 lastPlayerPosition = { 150, 90 };
    static LevelManager* instance;
    int difficultyIndex; // Member to store difficulty

    QuickplaySettings quickplaySettings;
};