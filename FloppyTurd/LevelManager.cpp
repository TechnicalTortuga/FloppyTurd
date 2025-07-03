#include "LevelManager.h"
#include "BossLevel.h"
#include "SewerLevel.h"
#include "CastleLevel.h"
#include "ToiletPaper.h"
#include "RatCopter.h"

#include "SnowLevel.h"
#include "DesertLevel.h"
#include "ParkLevel.h"
#include "Game.h" // For Game instance access

LevelManager* LevelManager::instance = nullptr;

LevelManager::LevelManager(std::shared_ptr<Level> level)
    : currentLevel(level), enemySpawnInterval(2.0f), enemySpawnTimer(0.0f), hasPassedFirstToilet(false), difficultyIndex(1) // Default to Regular
{
    LevelManager::instance = this;

    if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(currentLevel.get())) {
        boss = bossLevel->GetBoss();
        TraceLog(LOG_INFO, "[LevelManager] Initialized boss from BossLevel, ref count: %d", boss.use_count());
    }
    else {
        UpdateEnemyFactory();
    }
}

void LevelManager::UpdateEnemyFactory()
{
    if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
        enemyFactory = [sewer](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
            float pipeSpeed = sewer->GetPipePanSpeed();
            float extra = GetRandomValue(15, 35); // optional range
            return std::make_shared<ToiletPaper>(spawnPos, pipeSpeed, extra);
            };
    }
    else if (CastleLevel* castle = dynamic_cast<CastleLevel*>(currentLevel.get())) {
        enemyFactory = [castle, this](Vector2 /*ignored*/) -> std::shared_ptr<Enemy> {
            float randomY = (float)GetRandomValue(0, 180 - 32);
            Vector2 startPos = { 320.0f, randomY };
            return std::make_shared<RatCopter>(startPos, GetPickupPanSpeed());
            };
    }
    else if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(currentLevel.get())) {
        enemyFactory = [bossLevel, this](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
            float randomY = (float)GetRandomValue(0, 180 - 32);
            Vector2 startPos = { 320.0f, randomY };
            return std::make_shared<RatCopter>(startPos, GetPickupPanSpeed());
            };
    }
    else if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
        enemyFactory = [snow, this](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
            int typeRoll = GetRandomValue(0, 99);
            SnowmanType type;
            if (typeRoll < 5) type = SnowmanType::sCHAD;
            else if (typeRoll < 35) type = SnowmanType::sGREEN;
            else if (typeRoll < 65) type = SnowmanType::sBLUE;
            else type = SnowmanType::sRed;
            return std::make_shared<SnowmanEnemy>(spawnPos, type, GetPickupPanSpeed(), difficultyIndex);
            };
    }
    else if (DesertLevel* desert = dynamic_cast<DesertLevel*>(currentLevel.get())) {
        enemyFactory = [desert, this](Vector2) -> std::shared_ptr<Enemy> {
            static float lastBirdX = -1000.0f;
            const float screenWidth = 320.0f;

            if (desert->outhouses.size() < 2) {
                std::cout << "[BirdSpawn] ⚠️ Not enough outhouses to spawn Bird\n";
                float spawnX = screenWidth + 20.0f;
                float spawnY = (float)GetRandomValue(40, 140);
                Vector2 spawnPos = { spawnX, spawnY };
                std::cout << "[BirdSpawn] ✅ Fallback Bird at (" << spawnX << ", " << spawnY << ")\n";
                return std::make_shared<Bird>(spawnPos, GetPickupPanSpeed());
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
                return std::make_shared<Bird>(spawnPos, GetPickupPanSpeed());
            }

            std::cout << "[BirdSpawn] ⚠️ No valid gap found for Bird\n";
            float spawnX = screenWidth + 20.0f;
            float spawnY = (float)GetRandomValue(40, 140);
            Vector2 spawnPos = { spawnX, spawnY };
            std::cout << "[BirdSpawn] ✅ Fallback Bird at (" << spawnX << ", " << spawnY << ")\n";
            return std::make_shared<Bird>(spawnPos, GetPickupPanSpeed());
            };
    }
    else {
        enemyFactory = nullptr;
    }
}

LevelManager::~LevelManager()
{
    TraceLog(LOG_INFO, "[LevelManager] Destroying LevelManager, boss ref count: %d", boss.use_count());
    boss.reset();
}

LevelManager* LevelManager::GetInstance()
{
    return instance;
}

void LevelManager::Update(float deltaTime)
{
    if (currentLevel) {
        currentLevel->Update(deltaTime);
    }
    
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
    else if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
        const auto& toilets = snow->getObjLoc();
        if (!hasPassedFirstToilet && toilets.size() > 0) {
            float firstToiletRightEdge = toilets[0]->pos.x + 45.f;
            if (lastPlayerPosition.x > firstToiletRightEdge) {
                hasPassedFirstToilet = true;
                std::cout << "[LevelManager] ✅ hasPassedFirstToilet set for SnowLevel\n";
            }
        }
    }
    else if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
        const auto& pipes = sewer->GetPipes();
        if (!hasPassedFirstToilet && pipes.size() > 0) {
            float firstPipeRightEdge = pipes[0]->GetHitbox().x + pipes[0]->GetHitbox().width;
            if (lastPlayerPosition.x > firstPipeRightEdge) {
                hasPassedFirstToilet = true;
                std::cout << "[LevelManager] ✅ hasPassedFirstToilet set for SewerLevel\n";
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

        if (quickplaySettings.enableEnemies && hasPassedFirstToilet) {
            enemySpawnTimer += deltaTime;
            if (enemySpawnTimer >= enemySpawnInterval) {
                SpawnEnemy();
                enemySpawnTimer = 0.0f;
            }
        }
    }
    else {
        if (boss && boss->isActive) {
            boss->SetPlayerPosition(GetPlayerPosition());
            boss->Update(deltaTime);
        }
        else {
            boss.reset();
            TraceLog(LOG_INFO, "[LevelManager] Cleared inactive boss");
        }

        for (auto it = enemies.begin(); it != enemies.end(); ) {
            if ((*it)->ShouldBeRemoved()) it = enemies.erase(it);
            else ++it;
        }

        for (auto& enemy : enemies) {
            enemy->Update(deltaTime);
        }
    }
}

void LevelManager::Draw() const
{
    if (currentLevel) currentLevel->Draw();
    if (!boss) {
        for (const auto& enemy : enemies) enemy->Draw();
    }
    else if (boss && boss->isActive) {
        for (const auto& enemy : enemies)
            if (!enemy->ShouldBeRemoved())
                enemy->Draw();
        boss->Draw();
    }
}

Vector2 LevelManager::GetPlayerPosition() const
{
    return lastPlayerPosition;
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
    if (!enemyFactory) {
        std::cout << "[SpawnEnemy] ⚠️ No enemyFactory defined\n";
        return;
    }

    if (dynamic_cast<ParkLevel*>(currentLevel.get())) {
        std::cout << "[SpawnEnemy] ⚠️ Enemies not supported in Park Level\n";
        return;
    }

    if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
        const float screenWidth = 320.0f;
        float spawnX = screenWidth + 20.0f;
        float spawnY = 180.0f - 58.0f; // Fixed Y position at ground level
        Vector2 spawnPos = { spawnX, spawnY };
        std::shared_ptr<Enemy> enemy = enemyFactory(spawnPos);
        if (enemy && IsSpawnPositionValid(enemy->GetHitbox())) {
            enemies.push_back(enemy);
            std::cout << "[SnowSpawn] ✅ Snowman spawned at (" << spawnX << ", " << spawnY << ")\n";
            enemySpawnInterval = 1.5f; // Fixed interval for consistent spawning
        }
        else {
            std::cout << "[SnowSpawn] ⚠️ Invalid spawn position at (" << spawnX << ", " << spawnY << ")\n";
        }
        return;
    }

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

                std::shared_ptr<ToiletPaper> tp = std::make_shared<ToiletPaper>(Vector2{ spawnX, spawnY }, sewer->GetPipePanSpeed());

                if (GetRandomValue(0, 1) == 0)
                    tp->SetInverted(true);

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

bool LevelManager::IsSpawnPositionValid(const Rectangle& enemyHitbox) const
{
    for (const auto& enemy : enemies) {
        if (CheckCollisionRecs(enemyHitbox, enemy->GetHitbox()))
            return false;
    }
    return true;
}

void LevelManager::SetPipePanSpeedMultiplier(float multiplier, float duration)
{
    if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
        sewer->SetPanSpeedMultiplier(multiplier, duration);
    }
}

void LevelManager::SetLevel(std::shared_ptr<Level> newLevel, int difficulty)
{
    TraceLog(LOG_INFO, "[LevelManager] Setting new level, clearing old state. Old boss ref count: %d", boss.use_count());
    boss.reset();
    enemies.clear();
    currentLevel = std::move(newLevel);
    hasPassedFirstToilet = false;
    enemySpawnTimer = 0.0f;
    difficultyIndex = difficulty; // Set difficulty from Playing

    if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(currentLevel.get())) {
        bossLevel->Reset();
        boss = bossLevel->GetBoss();
        TraceLog(LOG_INFO, "[LevelManager] Initialized new boss for BossLevel, ref count: %d", boss.use_count());
    }

    UpdateEnemyFactory();
}

void LevelManager::SetQuickplaySettings(const QuickplaySettings& settings)
{
    quickplaySettings = settings;

    if (currentLevel) {
        if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
            auto& pipes = sewer->GetPipes();
            for (auto& pipe : pipes) {
                pipe->SetCollisionEnabled(quickplaySettings.enableObstacles);
            }
        }
        else if (ParkLevel* park = dynamic_cast<ParkLevel*>(currentLevel.get())) {
            auto& toilets = park->getObjLoc();
            for (auto& toilet : toilets) {
                toilet->SetCollisionEnabled(quickplaySettings.enableObstacles);
            }
        }
        else if (DesertLevel* desert = dynamic_cast<DesertLevel*>(currentLevel.get())) {
            for (auto& outhouse : desert->outhouses) {
                outhouse->SetCollisionEnabled(quickplaySettings.enableObstacles);
            }
        }
        else if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
            auto& toilets = snow->getObjLoc();
            for (auto& toilet : toilets) {
                toilet->SetCollisionEnabled(quickplaySettings.enableObstacles);
            }
        }
        else if (CastleLevel* castle = dynamic_cast<CastleLevel*>(currentLevel.get())) {
            auto& toilets = castle->getObjLoc();
            for (auto& toilet : toilets) {
                toilet->SetCollisionEnabled(quickplaySettings.enableObstacles);
            }
        }
    }

    if (quickplaySettings.swingingPipes) {
        if (currentLevel) {
            currentLevel->SetSwingingPipes(true);
        }
    }
}

float LevelManager::GetPickupPanSpeed() const
{
    if (!currentLevel) return 80.0f; // Default Regular speed
    if (SewerLevel* sewer = dynamic_cast<SewerLevel*>(currentLevel.get())) {
        return sewer->GetPipePanSpeed();
    }
    else if (SnowLevel* snow = dynamic_cast<SnowLevel*>(currentLevel.get())) {
        return snow->pickupPanSpeed; // Directly access SnowLevel's pickupPanSpeed
    }
    else if (DesertLevel* desert = dynamic_cast<DesertLevel*>(currentLevel.get())) {
        return 80.0f; // Default speed (no specific pan speed method yet)
    }
    else if (CastleLevel* castle = dynamic_cast<CastleLevel*>(currentLevel.get())) {
        return 80.0f; // Default speed (no specific pan speed method yet)
    }
    return 80.0f; // Fallback for unsupported levels
}

int LevelManager::GetDifficultyIndex() const
{
    return difficultyIndex;
}