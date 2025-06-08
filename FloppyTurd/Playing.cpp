#include "Playing.h"
#include "AIGUI.h"
#include "raymath.h"
#include <string>
#include <functional>
#include "RatCopter.h"
#include "ToiletPaper.h"
#include "AudioManager.h"
#include "SnowLevel.h"
#include "BossLevel.h"
#include "RatKing.h"
#include "BossHealthBar.h"
#include "TextureCache.h"
#include "Coin.h"
#include "PoopHeart.h"
#include "SnowballProjectile.h"

struct SkillData {
    std::string name;
    std::string description;
    int x;
    int y;
    bool isUnlocked = false;
    std::string requirementText;
    std::function<bool(const Player&)> canUnlock;
    std::function<void(Player&)> applyEffect;
};

Playing::Playing(Game* game)
{
    using namespace Resources;
    this->game = game;
    player = new Player(game); // Pass game reference to Player
    pauseMenuBackground = LoadTexture(PauseMenuBackground);
    _TurdPointMenu = LoadTexture(TurdPointMenu);
    _TurdPointMenuBorder = LoadTexture(TurdPointMenuBorder);
    _TurdPointInfo = LoadTexture(TurdPointInfo);
    SetTextureWrap(_TurdPointMenu, TEXTURE_WRAP_CLAMP);
    Scoreboard = LoadTexture(ScoreBoard);
    _TurdHeart = LoadTexture(TurdHeart);
    _CoinBag = LoadTexture(coinbagtexture);
    ScoreSound = LoadSound(GotScore);
    SCORE = 0;
    TOTALSCORE = 0;
    TOTALCOINS = 0;

    gameOverMusic = new AudioClip(GameOverMusic);
    PreLoadLevels();
    SetCurrentLevel(0);

    skillNodeTextures[0] = LoadTexture(TPMenuButtonLocked);
    skillNodeTextures[1] = LoadTexture(TPMenuButtonAvailable);
    skillNodeTextures[2] = LoadTexture(TPMenuButtonSelected);
    skillNodeTextures[3] = LoadTexture(TPMenuButtonFocused);

    for (int i = 0; i < totalSkillNodes; ++i) skillUnlocked[i] = false;
    selectedNode = -1;
    turdPoints = 3;

    gameOverBackground = LoadTexture(GameOverBackground);
    tryAgainBackground = LoadTexture(TryAgainBackground);
    deadFloppy = LoadTexture(DeadFloppy);
    gameOverScore = LoadTexture(GameOverScore);

    InitializeSkillNodes();
    InitializeHats();

    floppyButtonBlue = LoadTexture(blueButton);
    floppyButtonBlueHover = LoadTexture(blueButtonHover);

    AudioManager::GetInstance().LoadSoundEffect("GotCoin", Resources::GotCoin);
    AudioManager::GetInstance().LoadSoundEffect("GotHealth", Resources::GotHealth);
    AudioManager::GetInstance().LoadSoundEffect("GotHealthBig", Resources::GotHealthBig);

    Texture2D rawSnowTexture = LoadTexture(Snowfall);
    snowOverlay = std::make_unique<SnowOverlay>(rawSnowTexture, 16, 0.15f);
    SetTextureWrap(rawSnowTexture, TEXTURE_WRAP_CLAMP);
}

void Playing::InitializeHats()
{
    using namespace Resources;

    hatFrameNormal = LoadTexture(HatFrameNormal);
    hatFrameHover = LoadTexture(HatFrameHover);
    hatFrameSelected = LoadTexture(HatFrameSelected);
    hatFrameLocked = LoadTexture(HatFrameLocked);
    hatFrameDenied = LoadTexture(HatFrameDenied);

    Hat* cowboyHat = new Hat("Cowboy Hat",
        CowboyHat,
        CowboyHatTurdlet,
        6,
        CowboyHatTurdletShoot,
        9,
        CowboyHatTeenage,
        6,
        CowboyHatTeenageShoot,
        8,
        CowboyHatBigTurd,
        6,
        CowboyHatBigTurdShoot,
        8,
        UNLOCKED);
    Hat* flowerHat = new Hat("Flower", Flower, UNLOCKED);
    Hat* doorag = new Hat("Doorag", Doorag, UNLOCKED);
    Hat* ballcap = new Hat("Ballcap", Ballcap, UNLOCKED);

    hats.push_back(cowboyHat);
    hats.push_back(flowerHat);
    hats.push_back(doorag);
    hats.push_back(ballcap);

    for (int i = 4; i < 18; i++) {
        hats.push_back(new Hat("Hat " + std::to_string(i + 1), "resources/hats/placeholder.png", LOCKED));
    }
    currentSelectedHat = cowboyHat;
    player->SetHat(currentSelectedHat);
}

void Playing::PreLoadLevels()
{
    levels.emplace_back(std::make_shared<ParkLevel>());
    levels.emplace_back(std::make_shared<SewerLevel>());
    levels.emplace_back(std::make_shared<DesertLevel>());
    levels.emplace_back(std::make_shared<SnowLevel>());
    levels.emplace_back(std::make_shared<CastleLevel>());
    levels.emplace_back(std::make_shared<BossLevel>());
}

Playing::~Playing()
{
    UnloadTexture(Scoreboard);
    UnloadTexture(_TurdHeart);
    UnloadTexture(_CoinBag);
    UnloadSound(ScoreSound);
    UnloadTexture(floppyButtonBlue);
    UnloadTexture(floppyButtonBlueHover);
    UnloadTexture(gameOverBackground);
    UnloadTexture(tryAgainBackground);
    UnloadTexture(deadFloppy);
    UnloadTexture(gameOverScore);
    UnloadTexture(pauseMenuBackground);

    delete gameOverMusic;
    delete bossHealthBar;

    for (int i = 0; i < 4; ++i) {
        UnloadTexture(skillNodeTextures[i]);
    }
    delete player; // Ensure player is deleted
}

void Playing::InitializeSkillNodes() {
    skillNodePositions[0] = { 172, 28 };
    skillNodePositions[1] = { 79, 130 };
    skillNodePositions[2] = { 28, 128 };
    skillNodePositions[3] = { 108, 90 };
    skillNodePositions[4] = { 196, 154 };
    skillNodePositions[5] = { 120, 184 };
}

void Playing::UnlockSkill(int idx)
{
    if (idx < 0 || idx >= totalSkillNodes) return;
    if (skillUnlocked[idx])                return;
    if (turdPoints <= 0)                   return;

    if (idx == 1 && !skillUnlocked[0]) return;
    if (idx == 3 && !skillUnlocked[1]) return;
    if (idx == 5 && !skillUnlocked[1]) return;

    turdPoints--;
    skillUnlocked[idx] = true;

    switch (idx)
    {
    case 0: player->EnableShooting(true);                 break;
    case 1: player->SetHeartMode(Player::HALVES);         break;
    case 2: player->ChangeForm(1);                        break;
    case 3: player->EnableHollowTurds(true);              break;
    case 4: player->ChangeForm(2);                        break;
    case 5: player->SetHeartMode(Player::THIRDS);         break;
    }
}

void Playing::OutputHatMenu()
{
    const int gridStartX = 50;
    const int gridStartY = 50;
    const int slotWidth = 32;
    const int slotHeight = 32;
    const int spacingX = 4;
    const int spacingY = 4;

    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 6; col++)
        {
            int index = row * 6 + col;
            if (index >= hats.size()) break;

            int x = gridStartX + col * (slotWidth + spacingX);
            int y = gridStartY + row * (slotHeight + spacingY);

            Texture2D backgroundFrame = hatFrameNormal;

            if (currentSelectedHat == hats[index])
                backgroundFrame = hatFrameSelected;

            DrawTexturePro(
                backgroundFrame,
                Rectangle{ 0, 0, (float)backgroundFrame.width, (float)backgroundFrame.height },
                Rectangle{ (float)x, (float)y, (float)slotWidth, (float)slotHeight },
                Vector2{ 0, 0 },
                0.0f,
                WHITE
            );

            bool clicked = AIGUI_ImageButton(
                backgroundFrame,
                backgroundFrame,
                x, y,
                slotWidth, slotHeight,
                nullptr, 0,
                WHITE,
                nullptr
            );

            if (clicked && hats[index]->status == UNLOCKED)
            {
                currentSelectedHat = hats[index];
                player->SetHat(currentSelectedHat);
            }
            else if (clicked && hats[index]->status == LOCKED)
            {
                DrawTexturePro(
                    hatFrameDenied,
                    Rectangle{ 0, 0, (float)hatFrameDenied.width, (float)hatFrameDenied.height },
                    Rectangle{ (float)x, (float)y, (float)slotWidth, (float)slotHeight },
                    Vector2{ 0, 0 },
                    0.0f,
                    WHITE
                );
            }

            if (hats[index]->status == UNLOCKED && hats[index]->icon.id != 0)
            {
                int iconX = x + (slotWidth - hats[index]->icon.width) / 2;
                int iconY = y + (slotHeight - hats[index]->icon.height) / 2;
                DrawTexture(hats[index]->icon, iconX, iconY, WHITE);
            }

            if (hats[index]->status == LOCKED)
            {
                DrawTexturePro(
                    hatFrameLocked,
                    Rectangle{ 0, 0, (float)hatFrameLocked.width, (float)hatFrameLocked.height },
                    Rectangle{ (float)x, (float)y, (float)slotWidth, (float)slotHeight },
                    Vector2{ 0, 0 },
                    0.0f,
                    WHITE
                );
            }
        }
    }
}

void Playing::DrawPauseMenu()
{
    DrawTexturePro(
        pauseMenuBackground,
        Rectangle{ 0, 0, (float)pauseMenuBackground.width, (float)pauseMenuBackground.height },
        Rectangle{ 10, 10, (float)pauseMenuBackground.width, (float)pauseMenuBackground.height },
        Vector2{ 0, 0 },
        0.0f,
        WHITE
    );

    const int buttonWidth = 64;
    const int buttonHeight = 16;
    const int buttonSpacing = 4;
    const int topRowY = 20;
    const int totalButtons = 4;
    int totalWidth = totalButtons * buttonWidth + (totalButtons - 1) * buttonSpacing;
    int topRowX = (320 - totalWidth) / 2;

    if (AIGUI_ImageButton(
        floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "Skills", 16,
        WHITE,
        nullptr
    )) {
        currentTab = SKILLS;
    }
    topRowX += buttonWidth + buttonSpacing;

    if (AIGUI_ImageButton(floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "Hats", 16,
        WHITE,
        nullptr
    )) {
        currentTab = HATS;
    }
    topRowX += buttonWidth + buttonSpacing;

    if (AIGUI_ImageButton(floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "Stats", 16,
        WHITE,
        nullptr
    )) {
        currentTab = STATS;
    }
    topRowX += buttonWidth + buttonSpacing;

    if (AIGUI_ImageButton(floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "System", 16,
        WHITE,
        nullptr
    )) {
        currentTab = SYSTEM;
    }

    switch (currentTab)
    {
    case SKILLS:
    {
        Rectangle container = { 24, 40, 200, 120 };
        scrollOffset.x = Clamp(scrollOffset.x, 0.0f, (float)(_TurdPointMenu.width - container.width));
        scrollOffset.y = Clamp(scrollOffset.y, 0.0f, (float)(_TurdPointMenu.height - container.height));

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            Vector2 rawDelta = GetMouseDelta();
            float scaleX = (float)GetScreenWidth() / 320.0f;
            float scaleY = (float)GetScreenHeight() / 180.0f;
            scrollOffset.x -= rawDelta.x / scaleX;
            scrollOffset.y -= rawDelta.y / scaleY;
        }

        BeginScissorMode(
            (int)container.x,
            (int)container.y,
            (int)container.width,
            (int)container.height
        );

        DrawTexturePro(
            _TurdPointMenu,
            Rectangle{ scrollOffset.x, scrollOffset.y, container.width, container.height },
            Rectangle{ container.x, container.y, container.width, container.height },
            Vector2{ 0, 0 },
            0.0f,
            WHITE
        );

        for (int i = 0; i < totalSkillNodes; ++i)
        {
            float sx = skillNodePositions[i].x - scrollOffset.x + container.x;
            float sy = skillNodePositions[i].y - scrollOffset.y + container.y;

            bool unlocked = skillUnlocked[i];
            Texture2D def = unlocked ? skillNodeTextures[2] : skillNodeTextures[0];
            Texture2D hov = unlocked ? skillNodeTextures[3] : skillNodeTextures[1];

            bool clicked = AIGUI_ImageButton(def, hov, sx, sy, 32, 32);
            if (clicked) selectedNode = i;
        }

        EndScissorMode();

        const Rectangle info = { 200, 40, (float)_TurdPointInfo.width, (float)_TurdPointInfo.height };
        DrawTexturePro(_TurdPointInfo,
            { 0,0,info.width,info.height }, info,
            { 0,0 }, 0.f, WHITE);

        DrawText(TextFormat("Points: %d", turdPoints),
            info.x + 10, info.y + 10, 14, WHITE);

        if (selectedNode >= 0)
        {
            int y = (int)info.y + 30;
            DrawText(skillNames[selectedNode], info.x + 10, y, 14, YELLOW); y += 16;

            const char* desc = skillDescs[selectedNode];
            DrawTextEx(GetFontDefault(), desc,
                { info.x + 10, (float)y }, 14, 0, WHITE); y += 40;

            if (skillUnlocked[selectedNode])
            {
                DrawText("Status: UNLOCKED", info.x + 10, y, 14, GREEN);
            }
            else
            {
                DrawText("Status: LOCKED", info.x + 10, y, 14, RED); y += 20;

                bool canBuy = (turdPoints > 0);
                if (canBuy)
                {
                    if (AIGUI_Button("Unlock", info.x + 10, y, 80, 18))
                        UnlockSkill(selectedNode);
                }
                else
                {
                    DrawText("Need more points!", info.x + 10, y, 14, WHITE);
                }
            }
        }
        break;
    }
    case HATS:
        OutputHatMenu();
        break;
    case STATS:
        break;
    case SYSTEM:
    {
        if (AIGUI_ImageButton(
            floppyButtonBlue, floppyButtonBlueHover,
            128, 140,
            64, 16,
            "Main Menu", 12,
            WHITE,
            nullptr
        )) {
            if (game->mainMenu) {
                game->mainMenu->ResetMusic();
            }
            game->SetGameState(Game::MAINMENU);
        }

        AudioManager::GetInstance().DrawAudioOptions(10, 50);
        break;
    }
    }
}

void Playing::DrawGameOverScreen()
{
    const float scale = 2.0f;

    float panelW = (float)(tryAgainBackground.width * scale);
    float panelH = (float)(tryAgainBackground.height * scale);
    float panelX = (320.0f - panelW) / 2.0f;
    float panelY = 180.0f - panelH - 12.0f;

    DrawTexturePro(tryAgainBackground,
        { 0, 0, (float)tryAgainBackground.width, (float)tryAgainBackground.height },
        { panelX, panelY, panelW, panelH },
        { 0, 0 }, 0.0f, WHITE);

    int goBkgX = (int)((320 - gameOverBackground.width) / 2.0f);
    int goBkgY = (int)(panelY - gameOverBackground.height + 32);
    DrawTexture(gameOverBackground, goBkgX, goBkgY, WHITE);

    float bob = sinf(gameOverHoverTimer * 2.0f) * 2.0f;
    int baseY = (int)(panelY - (deadFloppy.height / 2) + 20);
    int floppyY = std::max(baseY + (int)bob, 0);
    int floppyX = (int)((320 - deadFloppy.width) / 2.0f);
    DrawTexture(deadFloppy, floppyX, floppyY, WHITE);

    static const char* poopMessages[] = {
        "Ahh poop.", "You pooped.", "Oh crap!", "Poop happens.",
        "Down the drain!", "That stinks.", "Toilet Trouble!", "You flushed!"
    };
    static int poopMsgIndex = GetRandomValue(0, (int)(sizeof(poopMessages) / sizeof(char*) - 1));
    const char* msg = poopMessages[poopMsgIndex];
    int msgWidth = MeasureText(msg, 18);
    float labelW = (float)(msgWidth + 24);
    float labelH = 26.0f;
    float labelX = (320.0f - labelW) / 2.0f;
    float labelY = 6.0f;
    AIGUI_LabelRounded(msg, labelX, labelY, labelW, labelH, 0.3f, 18, BLACK);

    int btnW = 128, btnH = 22, spacing = 12;
    int btnY = (int)(panelY + panelH) - btnH - 64;
    int btnX = (int)(panelX + (panelW - 2 * btnW - spacing) / 2.0f);

    if (AIGUI_ButtonRounded("Try Again", (float)btnX, (float)btnY, (float)btnW, (float)btnH, 0.3f, 24, BLACK)) {
        AudioManager::GetInstance().StopMusic();
        if (gameOverMusic) gameOverMusic->Stop();

        switch (lastLevelType) {
        case LastLevelType::PARK:   levelManager->SetLevel(std::make_shared<ParkLevel>()); break;
        case LastLevelType::SEWER:  levelManager->SetLevel(std::make_shared<SewerLevel>()); break;
        case LastLevelType::SNOW:   levelManager->SetLevel(std::make_shared<SnowLevel>()); break;
        case LastLevelType::CASTLE: levelManager->SetLevel(std::make_shared<CastleLevel>()); break;
        case LastLevelType::BOSS:
            levelManager->SetLevel(std::make_shared<BossLevel>());
            // Recreate BossHealthBar for new RatKing
            delete bossHealthBar;
            if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get())) {
                bossHealthBar = new BossHealthBar(bossLevel->GetBoss(), "King of Rats");
                TraceLog(LOG_INFO, "[Playing] Recreated BossHealthBar for new RatKing");
            }
            break;
        case LastLevelType::DESERT: levelManager->SetLevel(std::make_shared<DesertLevel>()); break;
        default:                    levelManager->SetLevel(std::make_shared<ParkLevel>()); break;
        }
        player->Revive();
        SCORE = 0;

        // Reapply Quickplay settings after resetting the level
        levelManager->SetQuickplaySettings(quickplaySettings);

        GAMEOVER = false;
        gameOverTriggered = false;
        turdHasFallenOffScreen = false;
    }

    if (AIGUI_ButtonRounded("Quit", (float)(btnX + btnW + spacing), (float)btnY, (float)btnW, (float)btnH, 0.3f, 24, BLACK)) {
        AudioManager::GetInstance().StopMusic();
        if (gameOverMusic) gameOverMusic->Stop();
        player->Revive();
        SCORE = 0;

        GAMEOVER = false;
        gameOverTriggered = false;
        turdHasFallenOffScreen = false;

        if (game->mainMenu) game->mainMenu->ResetMusic();
        game->SetGameState(Game::MAINMENU);
    }

    float sbScale = 2.0f;
    float sbW = (float)(gameOverScore.width * sbScale);
    float sbH = (float)(gameOverScore.height * sbScale);
    float sbX = (320.0f - sbW) / 2.0f;
    float sbY = panelY + panelH - sbH + 14.0f;

    DrawTexturePro(gameOverScore,
        { 0, 0, (float)gameOverScore.width, (float)gameOverScore.height },
        { sbX, sbY, sbW, sbH },
        { 0, 0 }, 0.0f, WHITE);

    std::string scoreStr = std::to_string(SCORE);
    int scoreW = MeasureText(scoreStr.c_str(), 20);
    Font hdFont = game->GetScaledFont(1.2f);
    DrawTextEx(hdFont, scoreStr.c_str(), { sbX + (sbW - (float)scoreW) / 2.0f + 20, sbY + (sbH / 2.0f - 12.0f) - 12 }, 24.0f, 1.0f, BLACK);

    std::string coinStr = std::to_string(player->GetSessionCoins());
    int coinW = MeasureText(coinStr.c_str(), 20);
    DrawTextEx(hdFont, coinStr.c_str(), { sbX + (sbW - (float)coinW) / 2.0f + 20, sbY + (sbH / 2.0f + 4.0f) }, 24.0f, 1.0f, BLACK);
}

void Playing::Update()
{
    deltaTime = GetFrameTime();

    if (!player->isAlive && !gameOverTriggered) {
        gameOverTriggered = true;
        turdHasFallenOffScreen = false;
        GAMEOVER = false;

        auto current = levelManager->GetCurrentLevel();
        int levelIndex = -1;
        if (dynamic_cast<ParkLevel*>(current.get())) {
            lastLevelType = LastLevelType::PARK;
            levelIndex = 0;
        }
        else if (dynamic_cast<SewerLevel*>(current.get())) {
            lastLevelType = LastLevelType::SEWER;
            levelIndex = 1;
        }
        else if (dynamic_cast<SnowLevel*>(current.get())) {
            lastLevelType = LastLevelType::SNOW;
            levelIndex = 3;
        }
        else if (dynamic_cast<CastleLevel*>(current.get())) {
            lastLevelType = LastLevelType::CASTLE;
            levelIndex = 4;
        }
        else if (dynamic_cast<BossLevel*>(current.get())) {
            lastLevelType = LastLevelType::BOSS;
            levelIndex = 5;
        }
        else if (dynamic_cast<DesertLevel*>(current.get())) {
            lastLevelType = LastLevelType::DESERT;
            levelIndex = 2;
        }

        // Update session record for the current level
        if (levelIndex >= 0) {
            UpdateSessionRecord(levelIndex, SCORE);
        }

        // Update level unlocks in MainMenu
        if (game->mainMenu) {
            game->mainMenu->UpdateLevelUnlocks(TOTALCOINS, sessionRecords);
        }

        if (current) current->StopMusic();
        AudioManager::GetInstance().StopMusic();
        gameOverMusic->Stop();
        gameOverMusic->SetLooping(false);
        gameOverMusic->Play();
    }

    if (gameOverTriggered) {
        player->Update(deltaTime);
        if (snowOverlay) snowOverlay->Update(deltaTime);
        gameOverMusic->Update();

        if (!turdHasFallenOffScreen && player->GetPosition().y > 180) {
            turdHasFallenOffScreen = true;
        }

        if (turdHasFallenOffScreen && !GAMEOVER) {
            GAMEOVER = true;
        }
        return;
    }

    if (!isPaused && !GAMEOVER) {
        if (levelManager) {
            UpdatePlayerPositionInLevel();
            levelManager->Update(deltaTime);

            std::shared_ptr<Level> currentLevel = levelManager->GetCurrentLevel();
            if (currentLevel) {
                if (!player->isInvisible && currentLevel->checkForCollisions(player->GetCircleCenter(), player->GetCircleRadius())) {
                    player->PutTheHurtOn(33);
                }
                else if (currentLevel->checkForPointGain(player->GetCircleCenter(), player->GetCircleRadius())) {
                    SCORE++;
                    TOTALSCORE++;
                    PlaySound(ScoreSound);
                }

                if (auto snowLevel = dynamic_cast<SnowLevel*>(currentLevel.get())) {
                    for (const auto& enemy : levelManager->GetEnemies()) {
                        if (auto snowman = dynamic_cast<SnowmanEnemy*>(enemy.get())) {
                            for (auto* snowball : snowman->GetSnowballs()) {
                                if (!snowball->IsActive()) continue;

                                if (!player->isInvisible && CheckCollisionCircleRec(
                                    player->GetCircleCenter(),
                                    player->GetCircleRadius(),
                                    snowball->GetHitbox()))
                                {
                                    player->PutTheHurtOn(1);
                                    snowball->Deactivate();
                                    break;
                                }
                            }
                        }
                    }
                }

                auto& pickups = levelManager->GetCurrentLevel()->GetPickUps();
                for (auto it = pickups.begin(); it != pickups.end(); ) {
                    if (CheckCollisionCircleRec(
                        player->GetCircleCenter(),
                        player->GetCircleRadius(),
                        (*it)->GetHitbox()))
                    {
                        (*it)->OnPickup();
                        if (auto heart = dynamic_cast<PoopHeart*>((*it).get())) {
                            int healAmount = heart->GetHealAmount();
                            player->AddHeartSlice(healAmount);
                            if (healAmount == 9) {
                                player->ActivateInvisibility(10.0f);
                            }
                        }
                        else if (auto coin = dynamic_cast<Coin*>((*it).get())) {
                            int coinValue = coin->GetValue();
                            player->AddCoins(coinValue); // This updates TOTALCOINS via Player
                        }
                        it = pickups.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
        }
        else {
            std::cerr << "Warning: levelManager is null in Update!" << std::endl;
        }

        if (!turdHasFallenOffScreen)
            player->Update(deltaTime);

        for (const auto& enemy : levelManager->GetEnemies())
        {
            if (auto tp = dynamic_cast<ToiletPaper*>(enemy.get()))
            {
                if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), tp->GetHitbox()))
                {
                    player->PutTheHurtOn(1);
                }
            }
            else if (auto bird = dynamic_cast<Bird*>(enemy.get()))
            {
                if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), bird->GetHitbox()))
                {
                    player->PutTheHurtOn(1);
                }
            }
            else if (auto ratCopter = dynamic_cast<RatCopter*>(enemy.get()))
            {
                if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), ratCopter->GetHitbox()))
                {
                    player->PutTheHurtOn(1);
                    ratCopter->TakeDamage();
                }
            }
        }

        std::vector<Projectile*>& projectiles = player->GetProjectilesNonConst();
        std::vector<std::shared_ptr<Enemy>>& enemies = levelManager->GetEnemies();

        for (auto projIt = projectiles.begin(); projIt != projectiles.end(); )
        {
            Projectile* proj = *projIt;
            Rectangle projHitbox = proj->GetHitbox();
            bool projectileHit = false;

            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end(); ++enemyIt)
            {
                if (CheckCollisionRecs(projHitbox, (*enemyIt)->GetHitbox()))
                {
                    projectileHit = true;
                    (*enemyIt)->TakeDamage();
                    break;
                }
            }

            if (projectileHit)
            {
                delete proj;
                projIt = projectiles.erase(projIt);
            }
            else
            {
                ++projIt;
            }
        }

        if (snowOverlay) snowOverlay->Update(deltaTime);

        if (levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get()))
        {
            BossLevel* bossLevel = dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get());
            if (bossLevel)
            {
                std::shared_ptr<Boss> boss = bossLevel->GetBoss();
                if (boss && boss->isActive)
                {
                    for (auto projIt = projectiles.begin(); projIt != projectiles.end(); )
                    {
                        Projectile* projectile = *projIt;
                        Rectangle projHitbox = projectile->GetHitbox();
                        bool hitBoss = false;
                        for (const auto& bossHitbox : boss->GetHitboxes())
                        {
                            if (CheckCollisionRecs(projHitbox, bossHitbox))
                            {
                                boss->TakeDamage(10);
                                delete projectile;
                                projIt = projectiles.erase(projIt);
                                hitBoss = true;
                                break;
                            }
                        }
                        if (!hitBoss) ++projIt;
                    }
                    if (std::shared_ptr<RatKing> rk = std::dynamic_pointer_cast<RatKing>(boss))
                    {
                        for (auto* tp : rk->GetProjectiles())
                        {
                            if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), tp->GetHitbox()))
                            {
                                player->PutTheHurtOn(1);
                            }
                        }
                    }
                }
            }
        }
    }

    UpdateMusic();
}

void Playing::Draw()
{
    if (levelManager) {
        levelManager->Draw();
    }
    else {
        std::cerr << "Warning: levelManager is null in Draw!" << std::endl;
    }

    if (!gameOverTriggered || !turdHasFallenOffScreen)
        player->Draw();

    if (snowOverlay && dynamic_cast<SnowLevel*>(levelManager->GetCurrentLevel().get()))
        snowOverlay->Draw();

    if (GAMEOVER)
    {
        DrawGameOverScreen();
        return;
    }

    // Enable high-definition font for key UI elements
    UseHighDefFont(true);
    DrawUI();
    UseHighDefFont(false);

    if (isPaused)
    {
        DrawPauseMenu();
        return;
    }

    if (levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get()))
    {
        BossLevel* bossLevel = dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get());
        if (bossLevel)
        {
            std::shared_ptr<Boss> boss = bossLevel->GetBoss();
            if (boss && boss->isActive && bossHealthBar)
            {
                bossHealthBar->Update(GetFrameTime());
                bossHealthBar->Draw();
            }
        }
    }
}

void Playing::PlayMusic(AudioClip* clip) {
    if (currentMusic == clip) {
        UpdateMusic();
        return;
    }

    if (currentMusic) {
        currentMusic->Stop();
    }

    currentMusic = clip;

    if (currentMusic) {
        currentMusic->Play();
    }
}

void Playing::UpdateMusic() {
    if (currentMusic && !GAMEOVER) {
        float vol = AudioManager::GetInstance().IsMusicMuted() ? 0.0f :
            (float)AudioManager::GetInstance().GetMusicVolume() / 10.0f;
        currentMusic->SetVolume(vol);
        currentMusic->Update();
    }
}

void Playing::HandleInput()
{
    if (IsKeyPressed(KEY_ESCAPE)) {
        isPaused = !isPaused;
        currentTab = SYSTEM;
    }
    if (isPaused) {
        if (IsKeyPressed(KEY_P)) currentTab = STATS;
        if (IsKeyPressed(KEY_H)) currentTab = HATS;
        if (IsKeyPressed(KEY_K)) currentTab = SKILLS;
    }
    else {
        if (IsKeyPressed(KEY_SPACE)) player->Jump();
        if (IsKeyDown(KEY_F)) player->Shoot();
        if (IsKeyPressed(KEY_R)) player->Revive();
    }
}

void Playing::FadeOutMusic(float deltaTime)
{
}

void Playing::SetCurrentLevel(int levelIndex) {
    if (levelIndex >= 0 && levelIndex < levels.size()) {
        if (levelManager && levelManager->GetCurrentLevel()) {
            levelManager->GetCurrentLevel()->StopMusic();
        }

        std::shared_ptr<Level> newLevel = levels[levelIndex];
        std::function<std::shared_ptr<Enemy>(Vector2)> enemyFactory;
        if (dynamic_cast<SewerLevel*>(newLevel.get()) != nullptr) {
            enemyFactory = [newLevel](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                SewerLevel* sewer = dynamic_cast<SewerLevel*>(newLevel.get());
                float pipeSpeed = sewer->GetPipePanSpeed();
                return std::make_shared<ToiletPaper>(spawnPos, pipeSpeed);
                };
        }
        else {
            enemyFactory = [](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                float defaultSpeed = 80.0f;
                return std::make_shared<ToiletPaper>(spawnPos, defaultSpeed);
                };
        }

        // Fetch Quickplay settings from MainMenu if available
        if (game->mainMenu) {
            quickplaySettings = game->mainMenu->GetQuickplaySettings();
        }

        levelManager = std::make_unique<LevelManager>(newLevel);
        levelManager->SetQuickplaySettings(quickplaySettings);

        if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(newLevel.get()))
        {
            std::shared_ptr<Boss> boss = bossLevel->GetBoss();
            if (boss)
            {
                delete bossHealthBar;
                bossHealthBar = new BossHealthBar(boss, "King of Rats");
                TraceLog(LOG_INFO, "[Playing] Created BossHealthBar for initial RatKing");
            }
            player->SetMaxHearts(9);
            int initialHearts = 2;
            player->AddHeartSlice(initialHearts * (int)player->GetHeartMode());
        }
        PlayMusic(newLevel->GetAudioClip());
        isPaused = false;
        SCORE = 0;
        player->ResetSessionCoins();
    }
    else {
        std::cerr << "Try Again Error: Invalid level index " << levelIndex << std::endl;
    }
}

void Playing::DrawUI()
{
    using namespace Resources;
    using namespace GameSettings;

    if (!(levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get())))
    {
        DrawTexturePro(Scoreboard,
            { 0,0,(float)Scoreboard.width,(float)Scoreboard.height },
            { 320.f - Scoreboard.width - 10, 180.f - Scoreboard.height - 10,
             (float)Scoreboard.width,(float)Scoreboard.height },
            { 0,0 }, 0.f, WHITE);
        std::string scoreStr = TextFormat("%i", SCORE);
        Vector2 scoreTextSize = MeasureTextEx(g_AIGUI.defaultFont, scoreStr.c_str(), 20.0f, 1.0f);
        float scoreTextX = 320.0f - 42.5f - scoreTextSize.x / 2.0f;
        float scoreTextY = 180.0f - 35.0f;
        Font fontToUse = IsHighDefFont() ? game->GetScaledFont(1.2f) : g_AIGUI.defaultFont;
        DrawTextEx(fontToUse, scoreStr.c_str(), { scoreTextX, scoreTextY }, 24.0f, 1.0f, WHITE);
    }

    const int hearts = player->GetTotalHearts();
    const int slicesPH = player->GetSlicesPerHeart();
    int live = player->GetSlicesLeft();
    int ghost = player->GetGhostSlicesLeft();

    for (int h = 0; h < hearts; ++h)
    {
        int sliceStart = h * slicesPH;
        int liveHere = std::max(0, std::min(slicesPH, live - sliceStart));
        int ghostHere = std::max(0, std::min(slicesPH - liveHere,
            ghost - sliceStart));

        const char* texPath = nullptr;
        if (slicesPH == 1)
        {
            if (liveHere == 1)        texPath = TurdHeartSmall;
            else                      texPath = TurdHeart0Half;
        }
        else if (slicesPH == 2)
        {
            if (liveHere == 2)   texPath = TurdHeartSmall;
            else if (liveHere == 1)   texPath = TurdHeart1Half;
            else if (ghostHere >= 1)  texPath = TurdHeart0HalfHollow;
            else                      texPath = TurdHeart0Half;
        }
        else
        {
            if (liveHere == 3)   texPath = TurdHeartSmall;
            else if (liveHere == 2)   texPath = TurdHeart2Thirds;
            else if (liveHere == 1)   texPath = TurdHeart1Third;
            else if (ghostHere >= 1)  texPath = TurdHeart0ThirdHollow;
            else                      texPath = TurdHeart0ThirdHollow;
        }

        Texture2D tex = TextureCache::Get(texPath);
        DrawTexturePro(tex,
            { 0,0,(float)tex.width,(float)tex.height },
            { 4.0f + h * 32.0f, 4.0f, (float)tex.width,(float)tex.height },
            { 0,0 }, 0.0f, WHITE);
    }

    // Draw coin bag and session coins under hearts
    float coinBagX = 4.0f;
    float coinBagY = 4.0f + _TurdHeart.height + 4.0f;
    DrawTexturePro(_CoinBag,
        { 0,0,(float)_CoinBag.width,(float)_CoinBag.height },
        { coinBagX, coinBagY, (float)_CoinBag.width,(float)_CoinBag.height },
        { 0,0 }, 0.0f, WHITE);
    std::string coinStr = TextFormat("%i", player->GetSessionCoins());
    Vector2 coinTextSize = MeasureTextEx(g_AIGUI.defaultFont, coinStr.c_str(), 20.0f, 1.0f);
    float coinTextX = coinBagX + _CoinBag.width + 4.0f;
    float coinTextY = coinBagY + (_CoinBag.height - coinTextSize.y) / 2.0f;
    Font fontToUse = IsHighDefFont() ? game->GetScaledFont(1.2f) : g_AIGUI.defaultFont;
    DrawTextEx(fontToUse, coinStr.c_str(), { coinTextX, coinTextY }, 24.0f, 1.0f, WHITE);
}

void Playing::UpdatePlayerPositionInLevel()
{
    if (levelManager)
        levelManager->SetPlayerPosition(player->GetCircleCenter());
}

void Playing::UpdateSessionRecord(int levelIndex, int pipesPassed)
{
    if (levelIndex >= 0 && levelIndex < 6) {
        sessionRecords[levelIndex] = std::max(sessionRecords[levelIndex], pipesPassed);
    }
}