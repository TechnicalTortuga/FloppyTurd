#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <iostream>
#include "raylib.h"
#include "Level.h"
#include "Enemy.h"
#include "Boss.h"
#include <set>
#include "SewerPipe.h"
#include "SnowmanEnemy.h"
#include "Bird.h"
#include "QuickplaySettings.h" // Replace MainMenu.h with new header

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
    void SetLevel(std::shared_ptr<Level> newLevel);

    // New method to apply Quickplay settings
    void SetQuickplaySettings(const QuickplaySettings& settings);

private:
    void SpawnEnemy();
    bool IsSpawnPositionValid(const Rectangle& enemyHitbox) const;
    std::shared_ptr<Level> currentLevel;
    std::vector<std::shared_ptr<Enemy>> enemies;
    std::shared_ptr<Boss> boss;
    float enemySpawnInterval;
    float enemySpawnTimer;
    std::function<std::shared_ptr<Enemy>(Vector2 spawnPos)> enemyFactory;
    Vector2 lastPlayerPosition = { 150, 90 };
    bool hasPassedFirstToilet = false;
    static LevelManager* instance;

    // Quickplay settings
    QuickplaySettings quickplaySettings;
};