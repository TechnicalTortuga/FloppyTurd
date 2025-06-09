#include "MainMenu.h"
#include "AIGUI.h"
#include "AudioManager.h"

struct AspectRatioOption {
    const char* name;
    int width;
    int height;
};

static const char* levelNames[] = {
    "A Flop in the Park",
    "Home Sweet Home",
    "The Good, the Bad and the Stinky",
    "Polar Pandemonium",
    "Dung in the Dungeon",
    "Curtains for Crap"
};

static AspectRatioOption aspectRatios[] = {
    {"Auto", 0, 0},
    {"320x180", 320, 180},
    {"640x360", 640, 360},
    {"1280x720", 1280, 720},
    {"1920x1080", 1920, 1080},
    {"2560x1440", 2560, 1440},
    {"320x240", 320, 240}
};

static Sound fartSoundsLoaded[11];

static int currentAspectIndex = 0;
static bool borderlessEnabled = false;
static bool fullscreenEnabled = false;

MainMenu::MainMenu(Game* game)
    : game(game), currentMenu(MAIN_MENU), fClickCount(0), fClickCooldown(0.0f), fHoverScale(1.0f), paintingHoverScale(1.0f), fartModeEnabled(false)
{
    using namespace Resources;

    emptyPainting = LoadTexture(EmptyPainting);
    _MenuBackground = LoadTexture(MainMenuBackground);
    _FloppyLogo = LoadTexture(FloppyLogo);
    finLogo = LoadTexture(FinLogo);
    lockedPainting = LoadTexture(LockedPainting);

    levelPaintings[0] = LoadTexture(ParkLevelPainting);
    levelPaintings[1] = LoadTexture(SewerLevelPainting);
    levelPaintings[2] = LoadTexture(DesertLevelPainting);
    levelPaintings[3] = LoadTexture(SnowLevelPainting);
    levelPaintings[4] = LoadTexture(CastleLevelPainting);
    levelPaintings[5] = LoadTexture(RatKingPainting);

    const char* fartPaths[] = {
        Resources::fart1, Resources::fart2, Resources::fart3, Resources::fart4, Resources::fart5,
        Resources::fart6, Resources::fart7, Resources::fart8, Resources::fart9, Resources::fart10, Resources::fart11
    };

    for (int i = 0; i < 11; ++i)
        fartSoundsLoaded[i] = LoadSound(fartPaths[i]);

    currentMusic = new AudioClip(MainMenuMusic);
    PlayMusic(currentMusic);
}

MainMenu::~MainMenu()
{
    UnloadTexture(emptyPainting);
    UnloadTexture(_MenuBackground);
    UnloadTexture(_FloppyLogo);
    UnloadTexture(finLogo);
    UnloadTexture(lockedPainting);

    for (int i = 0; i < 6; ++i)
        UnloadTexture(levelPaintings[i]);

    for (int i = 0; i < 11; ++i)
        UnloadSound(fartSoundsLoaded[i]);

    delete currentMusic;
}

void MainMenu::PlayRandomFartSound()
{
    int index = GetRandomValue(0, 10);
    PlaySound(fartSoundsLoaded[index]);
}

void MainMenu::Update()
{
    if (!currentMusic->IsPlaying())
        currentMusic->Play();

    UpdateMusic();

    if (currentMenu == MAIN_MENU)
    {
        float baseW = 28.0f;
        float baseH = 40.0f;
        float sw = baseW * fHoverScale;
        float sh = baseH * fHoverScale;

        int logoX = (int)((320 - _FloppyLogo.width) / 2.0f);
        int logoY = 10;

        float scaledX = (float)(logoX + 3);
        float scaledY = (float)(logoY + 5);

        Rectangle fRect = { scaledX, scaledY, sw, sh };
        bool fHovered = CheckCollisionPointRec(g_AIGUI.mousePos, fRect);

        float targetScale = fHovered ? 1.12f : 1.0f;
        fHoverScale += (targetScale - fHoverScale) * (10.0f * GetFrameTime());

        if (fClickCooldown > 0.0f)
            fClickCooldown -= GetFrameTime();

        if (fHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && fClickCooldown <= 0.0f)
        {
            fClickCooldown = 0.3f;
            fClickCount++;
            fHoverScale = 0.95f;
            PlayRandomFartSound();

            if (fClickCount >= 10)
            {
                ToggleFartMusic();
                fClickCount = 0;
            }
        }
    }
}

void MainMenu::PlayMusic(AudioClip* clip)
{
    if (currentMusic == clip)
    {
        UpdateMusic();
        return;
    }

    if (currentMusic) currentMusic->Stop();
    currentMusic = clip;
    if (currentMusic) currentMusic->Play();
}

void MainMenu::UpdateMusic()
{
    float vol = AudioManager::GetInstance().IsMusicMuted() ? 0.0f : (float)AudioManager::GetInstance().GetMusicVolume() / 10.0f;
    currentMusic->SetVolume(vol);
    currentMusic->Update();
}

void MainMenu::UpdateLevelUnlocks(int totalCoins, const int sessionRecords[6])
{
    // Park (0) and Sewer (1) are always unlocked
    levelsUnlocked[0] = true;
    levelsUnlocked[1] = true;

    // Desert (2): 100 coins, 50 Sewer pipes
    levelsUnlocked[2] = totalCoins >= 100 && sessionRecords[1] >= 50;

    // Snow (3): 250 coins, 50 Desert pipes
    levelsUnlocked[3] = totalCoins >= 250 && sessionRecords[2] >= 50;

    // Castle (4): 500 coins, 50 Snow pipes
    levelsUnlocked[4] = totalCoins >= 500 && sessionRecords[3] >= 50;

    // Rat King (5): 1000 coins, 50 Castle pipes
    levelsUnlocked[5] = totalCoins >= 1000 && sessionRecords[4] >= 50;
}

void MainMenu::Draw()
{
    using namespace GameSettings;

    DrawTexturePro(_MenuBackground,
        Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
        Rectangle{ 0, 0, 320, 180 },
        Vector2{ 0, 0 }, 0.0f, WHITE);

    // Enable high-definition font for all menu text
    UseHighDefFont(true);
    Font hdFont = game->GetScaledFont(1.2f);

    if (currentMenu == MAIN_MENU)
    {
        int logoX = (int)((320 - _FloppyLogo.width) / 2.0f);
        int logoY = 10;
        DrawTexture(_FloppyLogo, logoX, logoY, WHITE);

        float baseW = 28.0f;
        float baseH = 40.0f;
        float sw = baseW * fHoverScale;
        float sh = baseH * fHoverScale;
        float scaledX = (float)(logoX + 3);
        float scaledY = (float)(logoY + 5);

        DrawTexturePro(
            finLogo,
            Rectangle{ 0, 0, (float)finLogo.width, (float)finLogo.height },
            Rectangle{ scaledX, scaledY, sw, sh },
            Vector2{ 0, 0 },
            0.0f,
            WHITE);

        float buttonW = 120.0f, buttonH = 18.0f;
        float buttonX = (320.0f - buttonW) / 2.0f;
        float buttonY = (float)(logoY + _FloppyLogo.height + 4);
        float spacingY = 3.0f;
        float cornerRadius = 0.01f;

        if (AIGUI_ButtonRounded("Play", buttonX, buttonY, buttonW, buttonH, cornerRadius, 24, BLACK))
            currentMenu = LEVEL_SELECT;
        buttonY += buttonH + spacingY;

        if (AIGUI_ButtonRounded("Options", buttonX, buttonY, buttonW, buttonH, cornerRadius, 24, BLACK))
            currentMenu = OPTIONS_MENU;
        buttonY += buttonH + spacingY;

        if (AIGUI_ButtonRounded("Quickplay", buttonX, buttonY, buttonW, buttonH, cornerRadius, 24, BLACK)) {
            game->SetGameState(Game::PLAYING);
            game->playing->SetCurrentLevel(quickplaySettings.levelIndex);
        }
        buttonY += buttonH + spacingY;

        if (AIGUI_ButtonRounded("Exit", buttonX, buttonY, buttonW, buttonH, cornerRadius, 24, BLACK))
            game->SetGameState(game->SHUTDOWN);
    }
    else if (currentMenu == LEVEL_SELECT)
    {
        Texture2D& painting = levelPaintings[currentLevelIndex];
        float paintingX = (320.0f - painting.width) / 2.0f;
        float paintingY = (180.0f - painting.height) / 2.0f;
        Rectangle paintingRect = { paintingX, paintingY, (float)painting.width, (float)painting.height };

        bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, paintingRect);
        float targetScale = hovered ? 1.05f : 1.0f;
        paintingHoverScale += (targetScale - paintingHoverScale) * (5.0f * GetFrameTime());

        float scaledW = painting.width * paintingHoverScale;
        float scaledH = painting.height * paintingHoverScale;
        Rectangle scaledRect = { paintingX, paintingY, scaledW, scaledH };

        DrawTexturePro(painting,
            Rectangle{ 0, 0, (float)painting.width, (float)painting.height },
            scaledRect,
            Vector2{ 0, 0 },
            0.0f,
            WHITE);

        if (!levelsUnlocked[currentLevelIndex])
        {
            DrawTexturePro(lockedPainting,
                Rectangle{ 0, 0, (float)lockedPainting.width, (float)lockedPainting.height },
                scaledRect,
                Vector2{ 0, 0 },
                0.0f,
                WHITE);
        }

        const char* title = levelNames[currentLevelIndex];
        AIGUI_LabelRounded(title, 10.0f, 10.0f, 300.0f, 18.0f, 0.2f, 16, BLACK);

        // Display unlock requirements and progress
        float infoY = 40;
        int totalCoins = game->playing ? game->playing->TOTALCOINS : 0;
        int sessionRecords[6] = { 0 };
        if (game->playing) {
            for (int i = 0; i < 6; ++i) {
                sessionRecords[i] = game->playing->GetSessionRecord(i);
            }
        }

        if (!levelsUnlocked[currentLevelIndex] && currentLevelIndex >= 2) {
            int requiredCoins = 0;
            int requiredPipes = 50;
            int previousLevelIndex = currentLevelIndex - 1;

            switch (currentLevelIndex) {
            case 2: requiredCoins = 100; break; // Desert
            case 3: requiredCoins = 250; break; // Snow
            case 4: requiredCoins = 500; break; // Castle
            case 5: requiredCoins = 1000; break; // Rat King
            }

            std::string coinStatus = TextFormat("Coins: %d/%d", totalCoins, requiredCoins);
            std::string pipeStatus = TextFormat("Previous Pipes: %d/%d", sessionRecords[previousLevelIndex], requiredPipes);

            AIGUI_LabelRounded(coinStatus.c_str(), 80.0f, infoY + 20, 160.0f, 16.0f, 0.2f, 14, totalCoins >= requiredCoins ? GREEN : BLACK);
            AIGUI_LabelRounded(pipeStatus.c_str(), 60.0f, infoY, 200.0f, 16.0f, 0.2f, 14, sessionRecords[previousLevelIndex] >= requiredPipes ? GREEN : BLACK);
        }

        if (levelsUnlocked[currentLevelIndex] && CheckCollisionPointRec(g_AIGUI.mousePos, scaledRect))
        {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                game->SetGameState(Game::PLAYING);
                game->playing->SetCurrentLevel(currentLevelIndex);
            }
        }

        if (currentLevelIndex > 0 && AIGUI_ButtonRounded("Previous", 40.0f, 150.0f, 100.0f, 24.0f, 0.01f, 24, BLACK))
            currentLevelIndex--;

        if (currentLevelIndex < 5 && AIGUI_ButtonRounded("Next", 150.0f, 150.0f, 60.0f, 24.0f, 0.01f, 24, BLACK))
            currentLevelIndex++;

        if (AIGUI_ButtonRounded("Back", 220.0f, 150.0f, 80.0f, 24.0f, 0.01f, 24, BLACK))
            currentMenu = MAIN_MENU;
    }
    else if (currentMenu == OPTIONS_MENU)
    {
        float rightX = 185.0f;
        float startY = 20.0f;

        AudioManager::GetInstance().DrawAudioOptions(20.0f, 20.0f);

        // Aspect Ratio
        startY = 20.0f;
        AIGUI_LabelRounded("Resolution:", rightX, startY, 120.0f, 20.0f, 0.2f, 20, BLACK);
        if (AIGUI_ButtonRounded("<", rightX, startY + 24.0f, 20.0f, 20.0f, 0.01f, 20, BLACK)) {
            currentAspectIndex = (currentAspectIndex - 1 + 7) % 7;
            const auto& option = aspectRatios[currentLevelIndex];
            if (option.width > 0 && option.height > 0)
                SetWindowSize(option.width, option.height);
        }
        AIGUI_LabelRounded(aspectRatios[currentAspectIndex].name, rightX + 23.0f, startY + 24.0f, 80.0f, 20.0f, 0.2f, 20, BLACK);
        if (AIGUI_ButtonRounded(">", rightX + 106.0f, startY + 24.0f, 20.0f, 20.0f, 0.01f, 20, BLACK)) {
            currentAspectIndex = (currentAspectIndex + 1) % 7;
            const auto& option = aspectRatios[currentAspectIndex];
            if (option.width > 0 && option.height > 0)
                SetWindowSize(option.width, option.height);
        }

        // Difficulty
        startY = 68.0f;
        AIGUI_LabelRounded("Difficulty:", rightX, startY, 120.0f, 20.0f, 0.2f, 20, BLACK);
        if (AIGUI_ButtonRounded("<", rightX, startY + 24.0f, 20.0f, 20.0f, 0.01f, 20, BLACK)) {
            difficultyIndex = (difficultyIndex - 1 + 3) % 3;
        }
        AIGUI_LabelRounded(difficultyLevels[difficultyIndex], rightX + 23.0f, startY + 24.0f, 80.0f, 20.0f, 0.2f, 20, BLACK);
        if (AIGUI_ButtonRounded(">", rightX + 106.0f, startY + 24.0f, 20.0f, 20.0f, 0.01f, 20, BLACK)) {
            difficultyIndex = (difficultyIndex + 1) % 3;
        }

        // Fullscreen Toggle
        startY = 89.0f;
        if (AIGUI_ButtonRounded(fullscreenEnabled ? "[x] Fullscreen" : "[ ] Fullscreen", rightX - 25, startY + 28.0f, 150.0f, 20.0f, 0.01f, 20, BLACK)) {
            fullscreenEnabled = !fullscreenEnabled;
            ToggleFullscreen();
        }

        // Quickplay Settings
        if (AIGUI_ButtonRounded("Quickplay Settings", 20.0f, 148.0f, 184.0f, 20.0f, 0.01f, 20, BLACK)) {
            currentMenu = QUICKPLAY_SETTINGS;
        }

        // Back
        if (AIGUI_ButtonRounded("Back", 220.0f, 148.0f, 80.0f, 20.0f, 0.01f, 20, BLACK))
            currentMenu = MAIN_MENU;
    }
    else if (currentMenu == QUICKPLAY_SETTINGS)
    {
        float startX = 20.0f;
        float startY = 10.0f;
        float labelWidth = 120.0f;
        float labelHeight = 20.0f;
        float spacingY = 6.0f;

        AIGUI_LabelRounded(levelNames[quickplaySettings.levelIndex], 10.0f, startY, 300.0f, 20.0f, 0.2f, 20, BLACK);
        startY += labelHeight + spacingY;

        AIGUI_LabelRounded("Level:", startX, startY, 40.0f, 20.0f, 0.2f, 20, BLACK);
        if (AIGUI_ButtonRounded("<", startX + 50.0f, startY, 20.0f, 20.0f, 0.01f, 20, BLACK)) {
            quickplaySettings.levelIndex = (quickplaySettings.levelIndex - 1 + 6) % 6;
        }
        AIGUI_LabelRounded(levelNames[quickplaySettings.levelIndex], startX + 70.0f, startY, 124.0f, 20.0f, 0.2f, 20, BLACK);
        if (AIGUI_ButtonRounded(">", startX + 194.0f, startY, 20.0f, 20.0f, 0.01f, 20, BLACK)) {
            quickplaySettings.levelIndex = (quickplaySettings.levelIndex + 1) % 6;
        }
        startY += labelHeight + spacingY;

        if (AIGUI_ButtonRounded(quickplaySettings.chillMode ? "[x] Chill Mode" : "[ ] Chill Mode", startX, startY, 120.0f, 20.0f, 0.01f, 20, BLACK)) {
            quickplaySettings.chillMode = !quickplaySettings.chillMode;
            if (quickplaySettings.chillMode) {
                quickplaySettings.enableEnemies = false;
                quickplaySettings.enableObstacles = false;
            }
        }
        startY += labelHeight + spacingY;

        bool isParkLevel = (quickplaySettings.levelIndex == 0);
        if (AIGUI_ButtonRounded(quickplaySettings.enableEnemies ? "[x] Enemies" : "[ ] Enemies", startX, startY, 120.0f, 20.0f, 0.01f, 20, (quickplaySettings.chillMode || isParkLevel) ? GRAY : BLACK)) {
            if (!quickplaySettings.chillMode && !isParkLevel) {
                quickplaySettings.enableEnemies = !quickplaySettings.enableEnemies;
            }
        }
        startY += labelHeight + spacingY;

        if (AIGUI_ButtonRounded(quickplaySettings.enableObstacles ? "[x] Obstacles" : "[ ] Obstacles", startX, startY, 120.0f, 20.0f, 0.01f, 20, quickplaySettings.chillMode ? GRAY : BLACK)) {
            if (!quickplaySettings.chillMode) {
                quickplaySettings.enableObstacles = !quickplaySettings.enableObstacles;
            }
        }
        startY += labelHeight + spacingY;

        bool isUnsupportedLevel = (quickplaySettings.levelIndex == 0 || quickplaySettings.levelIndex == 1 || quickplaySettings.levelIndex == 2);
        if (AIGUI_ButtonRounded(quickplaySettings.swingingPipes ? "[x] Swinging Pipes" : "[ ] Swinging Pipes", startX, startY, 120.0f, 20.0f, 0.01f, 20, isUnsupportedLevel ? GRAY : BLACK)) {
            if (!isUnsupportedLevel) {
                quickplaySettings.swingingPipes = !quickplaySettings.swingingPipes;
            }
        }
        startY += labelHeight + spacingY;

        if (AIGUI_ButtonRounded("Back", 220.0f, 150.0f, 80.0f, 20.0f, 0.01f, 20, BLACK))
            currentMenu = OPTIONS_MENU;
    }
    else
    {
        AIGUI_LabelRounded("ERROR: Unhandled menu state!", 10.0f, 10.0f, 200.0f, 20.0f, 0.2f, 20, RED);
    }

    // Disable high-definition font after drawing
    UseHighDefFont(false);
}

void MainMenu::ResetMusic()
{
    if (currentMusic) {
        currentMusic->Stop();
        currentMusic->Play();
    }
}

void MainMenu::ToggleFartMusic()
{
    using namespace Resources;

    if (fartModeEnabled)
        PlayMusic(new AudioClip(MainMenuMusic));
    else
        PlayMusic(new AudioClip(MainMenuMusicAlt));

    fartModeEnabled = !fartModeEnabled;
}

void MainMenu::HandleInput()
{
    if (IsKeyPressed(KEY_E))
    {
        game->SetGameState(Game::PLAYING);
    }
}