#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <iostream>
#include "raylib.h"
#include "Level.h"
#include "Enemy.h"
#include "Boss.h"

class LevelManager {
public:
    LevelManager(std::shared_ptr<Level> level);
    ~LevelManager();  // Added destructor for cleanup

    void Update(float deltaTime);
    void Draw() const;

    Vector2 GetPlayerPosition() const;

    void SetEnemySpawnInterval(float interval);
    std::shared_ptr<Level> GetCurrentLevel() const;
    std::vector<std::shared_ptr<Enemy>>& GetEnemies();
    std::shared_ptr<Boss> GetBoss();  // Ensure this is defined here

private:
    void SpawnEnemy();
    bool IsSpawnPositionValid(const Rectangle& enemyHitbox) const;

private:
    std::shared_ptr<Level> currentLevel;
    std::vector<std::shared_ptr<Enemy>> enemies;
    std::shared_ptr<Boss> boss;  // For boss levels
    float enemySpawnInterval;
    float enemySpawnTimer;
    std::function<std::shared_ptr<Enemy>(Vector2 spawnPos)> enemyFactory;
};