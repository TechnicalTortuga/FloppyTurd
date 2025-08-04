#include "LevelManager.h"
#include "BossLevel.h"
#include "SewerLevel.h"
#include "CastleLevel.h"
#include "ToiletPaper.h"
#include "RatCopter.h"

LevelManager::LevelManager(std::shared_ptr<Level> level)
    : currentLevel(level), enemySpawnInterval(2.0f), enemySpawnTimer(0.0f)
{
    if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(currentLevel.get()))
    {
        boss = bossLevel->GetBoss();
    }
    else
    {
        // Non-boss level: set up enemy factory (e.g., ToiletPaper or RatCopter)
        if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get()))
        {
            enemyFactory = [sewer](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                return std::make_shared<ToiletPaper>(spawnPos, 80.0f);
                };
        }
        else if (CastleLevel* castle = dynamic_cast<CastleLevel*>(currentLevel.get()))
        {
            enemyFactory = [castle](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                return std::make_shared<RatCopter>(spawnPos, 80.0f);
                };
        }
        else if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(currentLevel.get()))
        {
            // Spawn RatCopters in BossLevel with random y-position, veering toward player
            enemyFactory = [bossLevel](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                float randomY = (float)GetRandomValue(0, GameSettings::GameHeight - 32);  // Ensure within screen height, 32 for sprite size
                Vector2 startPos = { 320.0f, randomY };  // Spawn from right edge
                return std::make_shared<RatCopter>(startPos, 80.0f);  // Initial speed left
                };
        }
    }
}

LevelManager::~LevelManager()
{
    // Clean up any dynamically allocated resources if needed
}

void LevelManager::Update(float deltaTime)
{
    if (currentLevel) currentLevel->Update(deltaTime);

    // Update enemies (if not a boss level)
    if (!boss)
    {
        for (auto it = enemies.begin(); it != enemies.end(); )
        {
            (*it)->Update(deltaTime);
            Rectangle hitbox = (*it)->GetHitbox();
            if (hitbox.x + hitbox.width < 0) it = enemies.erase(it);
            else ++it;
        }

        for (auto it = enemies.begin(); it != enemies.end(); )
        {
            if ((*it)->ShouldBeRemoved()) it = enemies.erase(it);
            else ++it;
        }

        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= enemySpawnInterval)
        {
            SpawnEnemy();
            enemySpawnTimer = 0.0f;
        }
    }
    else
    {
        // Update boss
        if (boss && boss->isActive) boss->Update(deltaTime);
        else boss.reset();  // Remove boss when inactive
    }
}

void LevelManager::Draw() const
{
    if (currentLevel) currentLevel->Draw();
    if (!boss)
    {
        for (const auto& enemy : enemies) enemy->Draw();
    }
    else if (boss && boss->isActive)
    {
        boss->Draw();
    }
}

Vector2 LevelManager::GetPlayerPosition() const
{
    return Vector2();
}

void LevelManager::SetEnemySpawnInterval(float interval)
{
    enemySpawnInterval = interval;
}

std::shared_ptr<Level> LevelManager::GetCurrentLevel() const
{
    return currentLevel;
}

std::vector<std::shared_ptr<Enemy>>& LevelManager::GetEnemies()
{
    return enemies;
}

std::shared_ptr<Boss> LevelManager::GetBoss()
{
    return boss;
}

void LevelManager::SpawnEnemy()
{
    if (!enemyFactory) return;

    Vector2 spawnPos = { 320.0f, 0.0f };  // Spawn at right edge, adjust as needed
    std::shared_ptr<Enemy> enemy = enemyFactory(spawnPos);
    Rectangle enemyHitbox = enemy->GetHitbox();

    if (IsSpawnPositionValid(enemyHitbox))
    {
        enemies.push_back(enemy);
    }
}

bool LevelManager::IsSpawnPositionValid(const Rectangle& enemyHitbox) const
{
    // Simple check: ensure no overlap with existing enemies
    for (const auto& enemy : enemies)
    {
        if (CheckCollisionRecs(enemyHitbox, enemy->GetHitbox()))
            return false;
    }
    return true;
}