#include "LevelManager.h"
#include "BossLevel.h"
#include "SewerLevel.h"
#include "CastleLevel.h"
#include "ToiletPaper.h"
#include "RatCopter.h"
#include <raymath.h>
#include "SnowLevel.h"
#include "DesertLevel.h"

LevelManager* LevelManager::instance = nullptr;

LevelManager::LevelManager(std::shared_ptr<Level> level)
    : currentLevel(level), enemySpawnInterval(2.0f), enemySpawnTimer(0.0f), hasPassedFirstToilet(false)
{
    LevelManager::instance = this;

    if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(currentLevel.get())) {
        boss = bossLevel->GetBoss();
    }
    else {
        if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
            enemyFactory = [sewer](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                return std::make_shared<ToiletPaper>(spawnPos, 120.0f);
                };
        }
        else if (CastleLevel* castle = dynamic_cast<CastleLevel*>(currentLevel.get())) {
            enemyFactory = [castle](Vector2 /*ignored*/) -> std::shared_ptr<Enemy> {
                float randomY = (float)GetRandomValue(0, 180 - 32);
                Vector2 startPos = { 320.0f, randomY };
                return std::make_shared<RatCopter>(startPos, 80.0f);
                };
        }
        else if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(currentLevel.get())) {
            enemyFactory = [bossLevel](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                float randomY = (float)GetRandomValue(0, 180 - 32);
                Vector2 startPos = { 320.0f, randomY };
                return std::make_shared<RatCopter>(startPos, 80.0f);
                };
        }
        else if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
            enemyFactory = [snow](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                int typeRoll = GetRandomValue(0, 99);
                if (typeRoll < 5) return std::make_shared<SnowmanEnemy>(spawnPos, SnowmanType::sCHAD);        // 5%
                else if (typeRoll < 35) return std::make_shared<SnowmanEnemy>(spawnPos, SnowmanType::sGREEN);   // 30%
                else if (typeRoll < 65) return std::make_shared<SnowmanEnemy>(spawnPos, SnowmanType::sBLUE);  // 30%
                else return std::make_shared<SnowmanEnemy>(spawnPos, SnowmanType::sRed);                    // 35%
                };
        }
        else if (DesertLevel* desert = dynamic_cast<DesertLevel*>(currentLevel.get())) {
            std::cout << "[SetCurrentLevel] ✅ DesertLevel enemyFactory assigned\n";
            enemyFactory = [desert](Vector2) -> std::shared_ptr<Enemy> {
                static float lastBirdX = -1000.0f;
                const float screenWidth = 320.0f;

                if (desert->outhouses.size() < 2) {
                    std::cout << "[BirdSpawn] ⚠️ Not enough outhouses to spawn Bird\n";
                    float spawnX = screenWidth + 20.0f;
                    float spawnY = (float)GetRandomValue(40, 140);
                    Vector2 spawnPos = { spawnX, spawnY };
                    std::cout << "[BirdSpawn] ✅ Fallback Bird at (" << spawnX << ", " << spawnY << ")\n";
                    return std::make_shared<Bird>(spawnPos, 80.0f);
                }

                std::sort(desert->outhouses.begin(), desert->outhouses.end(),
                    [](const auto& a, const auto& b) {
                        return a->GetOuthouseHitbox().x < b->GetOuthouseHitbox().x;
                    });

                for (size_t i = 0; i + 1 < desert->outhouses.size(); ++i) {
                    Rectangle leftHit = desert->outhouses[i]->GetOuthouseHitbox();
                    Rectangle rightHit = desert->outhouses[i + 1]->GetOuthouseHitbox();

                    std::cout << "[BirdSpawn] Outhouse " << i << ": x=" << leftHit.x << ", Outhouse " << i + 1 << ": x=" << rightHit.x << "\n";

                    if (leftHit.x < 100.0f) continue;

                    float leftEdge = leftHit.x + leftHit.width + 10.0f;
                    float rightEdge = rightHit.x - 10.0f;

                    if ((rightEdge - leftEdge) < 32.0f) {
                        std::cout << "[BirdSpawn] ⚠️ Gap too narrow between outhouses\n";
                        continue;
                    }

                    float centerX = (leftEdge + rightEdge - 24.0f) / 2.0f;
                    float spawnX = (float)GetRandomValue((int)(centerX - 40), (int)(centerX + 40));
                    spawnX = std::clamp(spawnX, leftEdge, rightEdge - 24.0f);
                    float spawnY = (float)GetRandomValue(40, 140);

                    if (spawnX - lastBirdX < 80.0f) {
                        std::cout << "[BirdSpawn] ⚠️ Too close to last Bird (lastX: " << lastBirdX << ", newX: " << spawnX << ")\n";
                        continue;
                    }

                    lastBirdX = spawnX;
                    Vector2 spawnPos = { spawnX, spawnY };
                    std::cout << "[BirdSpawn] ✅ (" << spawnX << ", " << spawnY << ")\n";
                    return std::make_shared<Bird>(spawnPos, 80.0f);
                }

                std::cout << "[BirdSpawn] ⚠️ No valid gap found for Bird\n";
                float spawnX = screenWidth + 20.0f;
                float spawnY = (float)GetRandomValue(40, 140);
                Vector2 spawnPos = { spawnX, spawnY };
                std::cout << "[BirdSpawn] ✅ Fallback Bird at (" << spawnX << ", " << spawnY << ")\n";
                return std::make_shared<Bird>(spawnPos, 80.0f);
                };
        }
    }
}

LevelManager::~LevelManager() {}

LevelManager* LevelManager::GetInstance() {
    return instance;
}

void LevelManager::Update(float deltaTime) {
    if (currentLevel) currentLevel->Update(deltaTime);

    // Update hasPassedFirstToilet for CastleLevel
    if (CastleLevel* castle = dynamic_cast<CastleLevel*>(currentLevel.get())) {
        const auto& toilets = castle->getObjLoc();
        if (!hasPassedFirstToilet && toilets.size() > 0) {
            float firstToiletRightEdge = toilets[0]->pos.x + 45.f;
            if (lastPlayerPosition.x > firstToiletRightEdge) {
                hasPassedFirstToilet = true;
                std::cout << "[LevelManager] ✅ hasPassedFirstToilet set for CastleLevel\n";
            }
        }
    }
    // Add support for DesertLevel
    else if (DesertLevel* desert = dynamic_cast<DesertLevel*>(currentLevel.get())) {
        const auto& outhouses = desert->outhouses;
        if (!hasPassedFirstToilet && outhouses.size() > 0) {
            float firstOuthouseRightEdge = outhouses[0]->GetOuthouseHitbox().x + outhouses[0]->GetOuthouseHitbox().width;
            if (lastPlayerPosition.x > firstOuthouseRightEdge) {
                hasPassedFirstToilet = true;
                std::cout << "[LevelManager] ✅ hasPassedFirstToilet set for DesertLevel\n";
            }
        }
    }
    // Add support for SnowLevel
    else if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
        const auto& toilets = snow->getObjLoc();
        if (!hasPassedFirstToilet && toilets.size() > 0) {
            float firstToiletRightEdge = toilets[0]->pos.x + 45.f; // Assuming similar width as CastleLevel
            if (lastPlayerPosition.x > firstToiletRightEdge) {
                hasPassedFirstToilet = true;
                std::cout << "[LevelManager] ✅ hasPassedFirstToilet set for SnowLevel\n";
            }
        }
    }

    if (!boss) {
        for (auto it = enemies.begin(); it != enemies.end(); ) {
            (*it)->Update(deltaTime);
            Rectangle hitbox = (*it)->GetHitbox();
            if (hitbox.x + hitbox.width < 0) it = enemies.erase(it);
            else ++it;
        }

        for (auto it = enemies.begin(); it != enemies.end(); ) {
            if ((*it)->ShouldBeRemoved()) it = enemies.erase(it);
            else ++it;
        }

        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= enemySpawnInterval && hasPassedFirstToilet) {
            SpawnEnemy();
            enemySpawnTimer = 0.0f;
        }
    }
    else {
        if (boss && boss->isActive) {
            boss->SetPlayerPosition(GetPlayerPosition());
            boss->Update(deltaTime);
        }
        else boss.reset();

        for (auto it = enemies.begin(); it != enemies.end(); ) {
            if ((*it)->ShouldBeRemoved()) it = enemies.erase(it);
            else ++it;
        }

        for (int i = 0; i < enemies.size(); i++) {
            enemies[i]->Update(deltaTime);
        }
    }
}

void LevelManager::Draw() const {
    if (currentLevel) currentLevel->Draw();
    if (!boss) {
        for (const auto& enemy : enemies) enemy->Draw();
    }
    else if (boss && boss->isActive) {
        for (const auto& enemy : enemies)
            if (enemy->ShouldBeRemoved() == false)
                enemy->Draw();

        boss->Draw();
    }
}

Vector2 LevelManager::GetPlayerPosition() const {
    return lastPlayerPosition;
}

void LevelManager::SetEnemySpawnInterval(float interval) {
    enemySpawnInterval = interval;
}

std::shared_ptr<Level> LevelManager::GetCurrentLevel() const {
    return currentLevel;
}

std::vector<std::shared_ptr<Enemy>>& LevelManager::GetEnemies() {
    return enemies;
}

std::shared_ptr<Boss> LevelManager::GetBoss() {
    return boss;
}

void LevelManager::SpawnEnemy() {
    if (!enemyFactory) {
        std::cout << "[SpawnEnemy] ⚠️ No enemyFactory defined\n";
        return;
    }

    // SnowLevel: Spawn Snowmen dynamically between toilets
    if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
        const auto& toilets = snow->getObjLoc();
        const float screenWidth = 320.0f; // Adjust to your game’s screen width
        if (toilets.size() < 2) {
            std::cout << "[SnowSpawn] ⚠️ Not enough toilets to spawn Snowman\n";
            // Fallback: Spawn near right edge of screen
            float spawnX = screenWidth + 20.0f; // Dynamic for visibility
            float spawnY = 180.0f - 58.0f; // Align bottom with ground
            Vector2 spawnPos = { spawnX, spawnY };
            std::shared_ptr<Enemy> enemy = enemyFactory(spawnPos);
            if (enemy && IsSpawnPositionValid(enemy->GetHitbox())) {
                enemies.push_back(enemy);
                std::cout << "[SnowSpawn] ✅ Fallback Snowman spawned at (" << spawnX << ", " << spawnY << ")\n";
                enemySpawnInterval = GetRandomValue(100, 200) / 100.0f; // 1.0 to 2.0s for fun
            }
            else {
                std::cout << "[SnowSpawn] ⚠️ Fallback Snowman invalid at (" << spawnX << ", " << spawnY << ")\n";
            }
            return;
        }

        for (size_t i = 0; i < toilets.size() - 1; ++i) {
            float leftX = toilets[i]->pos.x + 45.0f; // Right edge of current toilet
            float rightX = toilets[i + 1]->pos.x;   // Left edge of next toilet
            std::cout << "[SnowSpawn] Toilet " << i << ": x=" << toilets[i]->pos.x << ", Toilet " << i + 1 << ": x=" << toilets[i + 1]->pos.x << "\n";
            if (rightX - leftX < 32.0f) {
                std::cout << "[SnowSpawn] ⚠️ Gap too narrow between toilets\n";
                continue;
            }

            float spawnX = (leftX + rightX) / 2.0f; // Center between toilets
            float spawnY = 180.0f - 58.0f;          // Align bottom with ground
            Vector2 spawnPos = { spawnX, spawnY };

            std::shared_ptr<Enemy> enemy = enemyFactory(spawnPos);
            if (enemy && IsSpawnPositionValid(enemy->GetHitbox())) {
                enemies.push_back(enemy);
                std::cout << "[SnowSpawn] ✅ Snowman spawned at (" << spawnX << ", " << spawnY << ")\n";
                enemySpawnInterval = GetRandomValue(100, 200) / 100.0f; // Random 1.0 to 2.0s
                return;
            }
            else {
                std::cout << "[SnowSpawn] ⚠️ Invalid spawn position at (" << spawnX << ", " << spawnY << ")\n";
            }
        }
        // Fallback if no valid gap
        float spawnX = screenWidth + 20.0f;
        float spawnY = 180.0f - 58.0f;
        Vector2 spawnPos = { spawnX, spawnY };
        std::shared_ptr<Enemy> enemy = enemyFactory(spawnPos);
        if (enemy && IsSpawnPositionValid(enemy->GetHitbox())) {
            enemies.push_back(enemy);
            std::cout << "[SnowSpawn] ✅ Fallback Snowman spawned at (" << spawnX << ", " << spawnY << ")\n";
            enemySpawnInterval = GetRandomValue(100, 200) / 100.0f;
        }
        else {
            std::cout << "[SnowSpawn] ⚠️ Fallback Snowman invalid at (" << spawnX << ", " << spawnY << ")\n";
        }
        return;
    }

    // SewerLevel: Spawn ToiletPaper between pipes
    if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
        auto& pipes = sewer->GetPipes();
        for (size_t i = 0; i < pipes.size() - 1; ++i) {
            PipeType type = pipes[i]->GetPipeType();
            PipeType nextType = pipes[i + 1]->GetPipeType();

            if ((type == PipeType::TopOrange || type == PipeType::TopBlue) &&
                (nextType == PipeType::BottomOrange || nextType == PipeType::BottomBlue)) {

                float topEdge = pipes[i]->GetEdge();
                float bottomEdge = pipes[i + 1]->GetEdge();

                float minY = topEdge + 10.0f;
                float maxY = bottomEdge - 32.0f;
                if (maxY < minY + 4.0f) maxY = minY + 4.0f;

                float spawnX = 340.0f;
                float spawnY = GetRandomValue((int)minY, (int)maxY);

                std::shared_ptr<ToiletPaper> tp = std::make_shared<ToiletPaper>(Vector2{ spawnX, spawnY }, 120.0f);

                if (GetRandomValue(0, 1) == 0)
                    tp->SetInverted(true); // Invert sine wave randomly for fun

                bool tooCloseToExisting = false;
                for (const auto& e : enemies) {
                    if (CheckCollisionRecs(tp->GetHitbox(), e->GetHitbox())) {
                        tooCloseToExisting = true;
                        break;
                    }
                }

                if (!tooCloseToExisting) {
                    enemies.push_back(tp);
                    std::cout << "[SewerSpawn] ✅ ToiletPaper spawned at (" << spawnX << ", " << spawnY << ")\n";
                    enemySpawnInterval = GetRandomValue(180, 260) / 100.0f;
                    return;
                }
                else {
                    std::cout << "[SewerSpawn] ⚠️ ToiletPaper collision at (" << spawnX << ", " << spawnY << ")\n";
                }
            }
        }
        std::cout << "[SewerSpawn] ⚠️ No valid pipe gap for ToiletPaper\n";
        return;
    }

    // Default spawn for other levels (Castle, etc.)
    Vector2 spawnPos = { 320.0f + 20, 0.0f };
    std::shared_ptr<Enemy> enemy = enemyFactory(spawnPos);
    if (enemy && IsSpawnPositionValid(enemy->GetHitbox())) {
        enemies.push_back(enemy);
        std::cout << "[DefaultSpawn] ✅ Enemy spawned at (" << spawnPos.x << ", " << spawnPos.y << ")\n";
    }
    else {
        std::cout << "[DefaultSpawn] ⚠️ Invalid or null enemy at (" << spawnPos.x << ", " << spawnPos.y << ")\n";
    }
}

bool LevelManager::IsSpawnPositionValid(const Rectangle& enemyHitbox) const {
    for (const auto& enemy : enemies) {
        if (CheckCollisionRecs(enemyHitbox, enemy->GetHitbox()))
            return false;
    }
    return true;
}

void LevelManager::SetPipePanSpeedMultiplier(float multiplier, float duration) {
    if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
        sewer->SetPanSpeedMultiplier(multiplier, duration);
    }
}

void LevelManager::SetLevel(std::shared_ptr<Level> newLevel)
{
    // Reset all state as needed (enemies, boss, pickups, etc.)
    currentLevel = std::move(newLevel);
    hasPassedFirstToilet = false;
    enemies.clear();
    enemySpawnTimer = 0.0f;
    // Reset DesertLevel-specific state
    if (dynamic_cast<DesertLevel*>(currentLevel.get())) {
        std::cout << "[SetLevel] ✅ Resetting DesertLevel Bird spawn state\n";
        enemyFactory = [this](Vector2) -> std::shared_ptr<Enemy> {
            static float lastBirdX = -1000.0f;
            const float screenWidth = 320.0f;
            DesertLevel* desert = dynamic_cast<DesertLevel*>(currentLevel.get());
            if (!desert) return nullptr;

            if (desert->outhouses.size() < 2) {
                std::cout << "[BirdSpawn] ⚠️ Not enough outhouses to spawn Bird\n";
                float spawnX = screenWidth + 20.0f;
                float spawnY = (float)GetRandomValue(40, 140);
                Vector2 spawnPos = { spawnX, spawnY };
                std::cout << "[BirdSpawn] ✅ Fallback Bird at (" << spawnX << ", " << spawnY << ")\n";
                return std::make_shared<Bird>(spawnPos, 80.0f);
            }

            std::sort(desert->outhouses.begin(), desert->outhouses.end(),
                [](const auto& a, const auto& b) {
                    return a->GetOuthouseHitbox().x < b->GetOuthouseHitbox().x;
                });

            for (size_t i = 0; i + 1 < desert->outhouses.size(); ++i) {
                Rectangle leftHit = desert->outhouses[i]->GetOuthouseHitbox();
                Rectangle rightHit = desert->outhouses[i + 1]->GetOuthouseHitbox();

                std::cout << "[BirdSpawn] Outhouse " << i << ": x=" << leftHit.x << ", Outhouse " << i + 1 << ": x=" << rightHit.x << "\n";

                if (leftHit.x < 100.0f) continue;

                float leftEdge = leftHit.x + leftHit.width + 10.0f;
                float rightEdge = rightHit.x - 10.0f;

                if ((rightEdge - leftEdge) < 32.0f) {
                    std::cout << "[BirdSpawn] ⚠️ Gap too narrow between outhouses\n";
                    continue;
                }

                float centerX = (leftEdge + rightEdge - 24.0f) / 2.0f;
                float spawnX = (float)GetRandomValue((int)(centerX - 40), (int)(centerX + 40));
                spawnX = std::clamp(spawnX, leftEdge, rightEdge - 24.0f);
                float spawnY = (float)GetRandomValue(40, 140);

                if (spawnX - lastBirdX < 80.0f) {
                    std::cout << "[BirdSpawn] ⚠️ Too close to last Bird (lastX: " << lastBirdX << ", newX: " << spawnX << ")\n";
                    continue;
                }

                lastBirdX = spawnX;
                Vector2 spawnPos = { spawnX, spawnY };
                std::cout << "[BirdSpawn] ✅ (" << spawnX << ", " << spawnY << ")\n";
                return std::make_shared<Bird>(spawnPos, 80.0f);
            }

            std::cout << "[BirdSpawn] ⚠️ No valid gap found for Bird\n";
            float spawnX = screenWidth + 20.0f;
            float spawnY = (float)GetRandomValue(40, 140);
            Vector2 spawnPos = { spawnX, spawnY };
            std::cout << "[BirdSpawn] ✅ Fallback Bird at (" << spawnX << ", " << spawnY << ")\n";
            return std::make_shared<Bird>(spawnPos, 80.0f);
            };
    }
}

void LevelManager::SetQuickplaySettings(const QuickplaySettings& settings)
{
}
