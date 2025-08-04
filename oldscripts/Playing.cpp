#include "Playing.h"
#include "AIGUI.h"
#include "raymath.h"
#include <string>
#include <functional>
#include "LevelManager.h"
#include "RatCopter.h"  // For our enemy factory lambda
#include "ToiletPaper.h"
#include "AudioManager.h"
#include "SnowLevel.h"
#include "BossLevel.h"
#include "RatKing.h"
#include "BossHealthBar.h"

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
    player = new Player();
    pauseMenuBackground = LoadTexture(PauseMenuBackground);
    _TurdPointMenu = LoadTexture(TurdPointMenu);
    _TurdPointMenuBorder = LoadTexture(TurdPointMenuBorder);
    _TurdPointInfo = LoadTexture(TurdPointInfo);
    SetTextureWrap(_TurdPointMenu, TEXTURE_WRAP_CLAMP);
    Scoreboard = LoadTexture(ScoreBoard);
    _TurdHeart = LoadTexture(TurdHeart);
    ScoreSound = LoadSound(GotScore);
    SCORE = 0;

    PreLoadLevels();
    // Initialize the LevelManager with the first level (index 0) by default.
    SetCurrentLevel(0);

    // Load skill node textures
    skillNodeTextures[0] = LoadTexture(TPMenuButtonLocked);
    skillNodeTextures[1] = LoadTexture(TPMenuButtonAvailable);
    skillNodeTextures[2] = LoadTexture(TPMenuButtonSelected);
    skillNodeTextures[3] = LoadTexture(TPMenuButtonFocused);

    InitializeSkillNodes();
    InitializeHats();

    // Load button textures
    floppyButtonBlue = LoadTexture(blueButton);
    floppyButtonBlueHover = LoadTexture(blueButtonHover);
}

void Playing::InitializeHats()
{
    using namespace Resources;

    hatFrameNormal = LoadTexture(HatFrameNormal);   // from "resources/ui/HatFrame.png"
    hatFrameHover = LoadTexture(HatFrameHover);
    hatFrameSelected = LoadTexture(HatFrameSelected);
    hatFrameLocked = LoadTexture(HatFrameLocked);
    hatFrameDenied = LoadTexture(HatFrameDenied);

    // Initialize hats – for now, the CowboyHat is unlocked by default, the others locked.
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

    for (int i = 4; i < 18; i++) {  // 18 slots total (3 rows of 6)
        // For locked hats, you could use a placeholder icon or simply an empty texture.
        // Here we load a placeholder image; alternatively, pass a default Texture2D or a null-like texture.
        //Texture2D placeholder = LoadTexture("resources/hats/placeholder.png");
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
    UnloadSound(ScoreSound);
    UnloadTexture(floppyButtonBlue);
    UnloadTexture(floppyButtonBlueHover);

    for (int i = 0; i < 4; ++i) {
        UnloadTexture(skillNodeTextures[i]);
    }
}

void Playing::InitializeSkillNodes() {
    skillNodePositions[0] = { 172, 28 };
    skillNodePositions[1] = { 79, 130 };
    skillNodePositions[2] = { 28, 128 };
    skillNodePositions[3] = { 108, 90 };
    skillNodePositions[4] = { 196, 154 };
    skillNodePositions[5] = { 120, 184 };
}

void Playing::UnlockSkill(int nodeIndex) {
    switch (nodeIndex) {
    case 0:
        // Example skill: Increase speed
        break;
    case 1:
        // Example skill: Extra jump (e.g., player->AddJump();)
        break;
    default:
        break;
    }
}

void Playing::OutputHatMenu()
{
    const int gridStartX = 50;   // Within your pause menu
    const int gridStartY = 50;
    const int slotWidth = 32;   // Or 64, tweak as needed
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

            // Choose background frame: normal, hover, or selected.
            // For simplicity, assume 'backgroundFrame' is hatFrameNormal unless 
            // you detect the mouse is over it or it's the currently selected hat.
            Texture2D backgroundFrame = hatFrameNormal;

            // If it's the currently selected hat:
            if (currentSelectedHat == hats[index])
                backgroundFrame = hatFrameSelected;
            // Or if it's hovered, set backgroundFrame = hatFrameHover; etc.

            // 1) Draw the background frame scaled to our slot
            DrawTexturePro(
                backgroundFrame,
                Rectangle{ 0, 0, (float)backgroundFrame.width, (float)backgroundFrame.height },
                Rectangle{ (float)x, (float)y, (float)slotWidth, (float)slotHeight },
                Vector2{ 0, 0 },
                0.0f,
                WHITE
            );

            // 3) Handle clicks
            //    If you’re using AIGUI_ImageButton, do something like:
            bool clicked = AIGUI_ImageButton(
                backgroundFrame,
                backgroundFrame,  // or a real "hover" texture
                x, y,
                slotWidth, slotHeight,
                nullptr, 0,
                WHITE,
                nullptr
            );

            // If locked, we might skip the click logic or show a "denied" frame, etc.
            if (clicked && hats[index]->status == UNLOCKED)
            {
                currentSelectedHat = hats[index];
                player->SetHat(currentSelectedHat);
            }
            else if (clicked && hats[index]->status == LOCKED)
            {
                // Optionally draw a denied frame or show a message
                DrawTexturePro(
                    hatFrameDenied,
                    Rectangle{ 0, 0, (float)hatFrameDenied.width, (float)hatFrameDenied.height },
                    Rectangle{ (float)x, (float)y, (float)slotWidth, (float)slotHeight },
                    Vector2{ 0, 0 },
                    0.0f,
                    WHITE
                );
            }

            // 4) Draw hat icon if unlocked
            if (hats[index]->status == UNLOCKED && hats[index]->icon.id != 0)
            {
                // Center the hat icon in the frame
                int iconX = x + (slotWidth - hats[index]->icon.width) / 2;
                int iconY = y + (slotHeight - hats[index]->icon.height) / 2;
                DrawTexture(hats[index]->icon, iconX, iconY, WHITE);
            }

            // 2) If locked, overlay the locked frame the same way
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

        for (int i = 0; i < totalSkillNodes; i++)
        {
            float nodeScreenX = skillNodePositions[i].x - scrollOffset.x + container.x;
            float nodeScreenY = skillNodePositions[i].y - scrollOffset.y + container.y;

            bool clicked = AIGUI_ImageButton(
                skillNodeTextures[skillNodeStates[i] ? 2 : 0],
                skillNodeTextures[skillNodeStates[i] ? 3 : 1],
                nodeScreenX, nodeScreenY,
                32, 32,
                nullptr,
                20,
                WHITE,
                nullptr
            );
            if (clicked)
            {
                selectedNode = i;
            }
        }
        EndScissorMode();

        DrawTexturePro(
            _TurdPointMenuBorder,
            Rectangle{ 0, 0, container.width, container.height },
            Rectangle{ container.x, container.y, container.width, container.height },
            Vector2{ 0, 0 },
            0.0f,
            WHITE
        );

        DrawTexturePro(
            _TurdPointInfo,
            Rectangle{ 0, 0,(float)_TurdPointInfo.width,(float)_TurdPointInfo.height },
            Rectangle{ 200, 40, (float)_TurdPointInfo.width, (float)_TurdPointInfo.height },
            Vector2{ 0, 0 },
            0.0f,
            WHITE
        );
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
            // Reset the main menu music before switching states
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

void Playing::Update()
{
    deltaTime = GetFrameTime();

    if (!isPaused) {
        if (levelManager) {
            levelManager->Update(deltaTime);

            // Player-vs-level collision logic (already in your code):
            std::shared_ptr<Level> currentLevel = levelManager->GetCurrentLevel();
            if (currentLevel) {
                if (currentLevel->checkForCollisions(player->GetCircleCenter(), player->GetCircleRadius())) {
                    player->PutTheHurtOn(33);
                }
                else if (currentLevel->checkForPointGain(player->GetCircleCenter(), player->GetCircleRadius())) {
                    SCORE++;
                    PlaySound(ScoreSound);
                }
            }
        }
        else {
            std::cerr << "Warning: levelManager is null in Update!" << std::endl;
        }

        // Update the player (movement, animation, etc.)
        player->Update(deltaTime);

        // ─────────────────────────────────────────────────────────
        // NEW: Projectile-Enemy collision logic & safe erasing
        // ─────────────────────────────────────────────────────────

        // 1) Get the player's projectiles by reference (non-const).
        std::vector<Projectile*>& projectiles = player->GetProjectilesNonConst();

        // 2) Access the enemies from LevelManager. (Ensure LevelManager has a public
        //    GetEnemies() returning a non-const reference to its enemy vector.)
        std::vector<std::shared_ptr<Enemy>>& enemies = levelManager->GetEnemies();

        // 3) Loop over projectiles with an iterator so we can erase safely.
        for (auto projIt = projectiles.begin(); projIt != projectiles.end(); )
        {
            Projectile* proj = *projIt;
            Rectangle projHitbox = proj->GetHitbox();
            bool projectileHit = false;

            // Check collision against each enemy
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end(); ++enemyIt)
            {
                if (CheckCollisionRecs(projHitbox, (*enemyIt)->GetHitbox()))
                {
                    // If collision: mark the projectile for removal.
                    projectileHit = true;

                    // Example: call a "TakeDamage()" method on the enemy
                    // to trigger a hurt animation, etc. Or remove the enemy immediately.
                    (*enemyIt)->TakeDamage();
                    // You can break if you only want each projectile to hit 1 enemy.
                    break;
                }
            }

            if (projectileHit)
            {
                // If you allocated the projectile with 'new', delete it here.
                delete proj;
                // Erase returns the next valid iterator after removal.
                projIt = projectiles.erase(projIt);
            }
            else
            {
                // If no collision, just advance.
                ++projIt;
            }
        }

        if (levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get()))
        {
            BossLevel* bossLevel = dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get());
            if (bossLevel)
            {
                std::shared_ptr<Boss> boss = bossLevel->GetBoss();
                if (boss && boss->isActive)
                {
                    for (auto& projectile : projectiles)  // Assume player has projectiles vector
                    {
                        Rectangle projHitbox = projectile->GetHitbox();
                        for (const auto& bossHitbox : boss->GetHitboxes())
                        {
                            if (CheckCollisionRecs(projHitbox, bossHitbox))
                            {
                                boss->TakeDamage(10);  // Damage value, adjust as needed
                                delete projectile;
                                projectiles.erase(std::find(projectiles.begin(), projectiles.end(), projectile));
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // Update music volume (already in your code).
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
    player->Draw();
    DrawUI();

    if (isPaused) {
        DrawPauseMenu();
    }

    // Draw boss health bar if in BossLevel
    if (levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get()))
    {
        BossLevel* bossLevel = dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get());
        if (bossLevel)
        {
            std::shared_ptr<Boss> boss = bossLevel->GetBoss();
            if (boss && boss->isActive)
            {
                static BossHealthBar healthBar(boss.get(), "King of Rats");
                healthBar.Update(GetFrameTime());
                healthBar.Draw();
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
    if (currentMusic) {
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
        if (IsKeyPressed(KEY_R)) player->Reset();
    }
}

void Playing::FadeOutMusic(float deltaTime)
{
    // Implementation as needed.
}

void Playing::SetCurrentLevel(int levelIndex) {
    if (levelIndex >= 0 && levelIndex < levels.size()) {
        if (levelManager && levelManager->GetCurrentLevel()) {
            levelManager->GetCurrentLevel()->StopMusic();
        }

        std::shared_ptr<Level> newLevel = levels[levelIndex];
        // Choose enemy type based on the level.
        std::function<std::shared_ptr<Enemy>(Vector2)> enemyFactory;
        // If the level is a SewerLevel, spawn ToiletPaper enemies.
        if (dynamic_cast<SewerLevel*>(newLevel.get()) != nullptr) {
            // It's a SewerLevel
            enemyFactory = [newLevel](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                SewerLevel* sewer = dynamic_cast<SewerLevel*>(newLevel.get());
                float pipeSpeed = sewer->GetPipePanSpeed();  // read the sewer's pan speed
                return std::make_shared<ToiletPaper>(spawnPos, pipeSpeed);
                };
        }
        else {
            // Non-sewer level => default speed
            enemyFactory = [](Vector2 spawnPos) -> std::shared_ptr<Enemy> {
                float defaultSpeed = 80.0f;  // or any speed you want
                return std::make_shared<ToiletPaper>(spawnPos, defaultSpeed);
                };
        }

        levelManager = std::make_unique<LevelManager>(newLevel);
        PlayMusic(newLevel->GetAudioClip());
        isPaused = false;
    }
    else {
        std::cerr << "Error: Invalid level index " << levelIndex << std::endl;
    }
}

void Playing::DrawUI()
{
    using namespace GameSettings;

    if (!(levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get())))
    {
        // Only draw score and hearts if not in BossLevel
        DrawTexturePro(Scoreboard, Rectangle{ 0, 0, (float)Scoreboard.width, (float)Scoreboard.height },
            Rectangle{ 320.0f - Scoreboard.width - 10, 180.0f - Scoreboard.height - 10, (float)Scoreboard.width, (float)Scoreboard.height },
            Vector2{ 0,0 }, 0.0f, WHITE);
        DrawText(TextFormat("%i", SCORE), 320 - 50, 180 - 35, 20, WHITE);
    }

    for (int i = 0; i < player->GetHealth() / 33; i++)
        DrawTexturePro(_TurdHeart, Rectangle{ 0, 0, (float)_TurdHeart.width, (float)_TurdHeart.height },
            Rectangle{ 4.0f + (i * 32), 4, (float)_TurdHeart.width, (float)_TurdHeart.height },
            Vector2{ 0,0 }, 0.0f, WHITE);
}

void Playing::UpdatePlayerPositionInLevel()
{
    if (levelManager && levelManager->GetCurrentLevel())
    {
        //Vector2 playerPos = player->GetPosition();
        //levelManager->GetCurrentLevel()->SetPlayerPosition(playerPos);
    }
}