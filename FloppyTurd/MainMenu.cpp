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
    {"320x180 (16:9)", 320, 180},
    {"640x360 (16:9)", 640, 360},
    {"1280x720 (720p)", 1280, 720},
    {"1920x1080 (1080p)", 1920, 1080},
    {"2560x1440 (1440p)", 2560, 1440},
    {"320x240 (4:3)", 320, 240}
};

static Sound fartSoundsLoaded[11];

static int currentAspectIndex = 0;
static bool borderlessEnabled = false;
static bool fullscreenEnabled = false;
static int difficultyIndex = 1;
const char* difficultyLevels[] = { "Runny", "Regular", "Rough" };

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

        int logoX = (320 - _FloppyLogo.width) / 2;
        int logoY = 10;

        float scaledX = logoX + 3;
        float scaledY = logoY + 5;

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

void MainMenu::Draw()
{
    using namespace GameSettings;

    DrawTexturePro(_MenuBackground,
        Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
        Rectangle{ 0, 0, 320, 180 },
        Vector2{ 0, 0 }, 0.0f, WHITE);

    if (currentMenu == MAIN_MENU)
    {
        int logoX = (320 - _FloppyLogo.width) / 2;
        int logoY = 10;
        DrawTexture(_FloppyLogo, logoX, logoY, WHITE);

        float baseW = 28.0f;
        float baseH = 40.0f;
        float sw = baseW * fHoverScale;
        float sh = baseH * fHoverScale;
        float scaledX = logoX + 3;
        float scaledY = logoY + 5;

        DrawTexturePro(
            finLogo,
            Rectangle{ 0, 0, (float)finLogo.width, (float)finLogo.height },
            Rectangle{ scaledX, scaledY, sw, sh },
            Vector2{ 0, 0 },
            0.0f,
            WHITE);

        float buttonW = 120, buttonH = 16;
        float buttonX = (320 - buttonW) / 2.0f;
        float buttonY = logoY + _FloppyLogo.height + 8;
        float spacingY = 4;
        float cornerRadius = 0.01f;

        if (AIGUI_ButtonRounded("Play", buttonX, buttonY, buttonW, buttonH, cornerRadius, AIGUI_FONT_SIZE_LARGE, BLACK))
            currentMenu = LEVEL_SELECT;
        buttonY += buttonH + spacingY;

        if (AIGUI_ButtonRounded("Options", buttonX, buttonY, buttonW, buttonH, cornerRadius, AIGUI_FONT_SIZE_LARGE, BLACK))
            currentMenu = OPTIONS_MENU;
        buttonY += buttonH + spacingY;

        if (AIGUI_ButtonRounded("Quickplay", buttonX, buttonY, buttonW, buttonH, cornerRadius, AIGUI_FONT_SIZE_LARGE, BLACK)) {
            game->SetGameState(Game::PLAYING);
            game->playing->SetCurrentLevel(quickplaySettings.levelIndex);
        }
        buttonY += buttonH + spacingY;

        if (AIGUI_ButtonRounded("Exit", buttonX, buttonY, buttonW, buttonH, cornerRadius, AIGUI_FONT_SIZE_LARGE, BLACK))
            game->SetGameState(game->SHUTDOWN);
    }
    else if (currentMenu == LEVEL_SELECT)
    {
        Texture2D& painting = levelPaintings[currentLevelIndex];
        float paintingX = (320 - painting.width) / 2.0f;
        float paintingY = (180 - painting.height) / 2.0f;
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
        AIGUI_LabelRounded(title, 10, 10, 300, 20, 0.2f, AIGUI_FONT_SIZE_LARGE, BLACK);

        if (levelsUnlocked[currentLevelIndex] && CheckCollisionPointRec(g_AIGUI.mousePos, scaledRect))
        {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                game->SetGameState(Game::PLAYING);
                game->playing->SetCurrentLevel(currentLevelIndex);
            }
        }

        if (currentLevelIndex > 0 && AIGUI_ButtonRounded("Previous", 40, 150, 100, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, BLACK))
            currentLevelIndex--;

        if (currentLevelIndex < 5 && AIGUI_ButtonRounded("Next", 150, 150, 60, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, BLACK))
            currentLevelIndex++;

        if (AIGUI_ButtonRounded("Back", 220, 150, 80, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, BLACK))
            currentMenu = MAIN_MENU;
    }
    else if (currentMenu == OPTIONS_MENU)
    {
        float rightX = 185; // Right column X position
        float startY = 20;  // Top-right, parallel with Music:

        AudioManager::GetInstance().DrawAudioOptions(20, 20);

        // Aspect Ratio (top-right, parallel with Music:)
        AIGUI_LabelRounded("Aspect Ratio:", rightX, startY, 110, 16, 0.2f, AIGUI_FONT_SIZE_LARGE, BLACK);
        if (AIGUI_ButtonRounded("<", rightX, startY + 16, 16, 16, 0.01f, AIGUI_FONT_SIZE_LARGE, BLACK)) {
            currentAspectIndex = (currentAspectIndex - 1 + 7) % 7;
            const auto& option = aspectRatios[currentAspectIndex];
            if (option.width > 0 && option.height > 0)
                SetWindowSize(option.width, option.height);
        }
        AIGUI_LabelRounded(aspectRatios[currentAspectIndex].name, rightX + 16, startY + 16, 78, 16, 0.2f, AIGUI_FONT_SIZE_MEDIUM, BLACK);
        if (AIGUI_ButtonRounded(">", rightX + 110, startY + 16, 16, 16, 0.01f, AIGUI_FONT_SIZE_LARGE, BLACK)) {
            currentAspectIndex = (currentAspectIndex + 1) % 7;
            const auto& option = aspectRatios[currentAspectIndex];
            if (option.width > 0 && option.height > 0)
                SetWindowSize(option.width, option.height);
        }

        // Difficulty (aligned with Sound:)
        startY = 52; // Align with Sound: at y=52
        AIGUI_LabelRounded("Difficulty:", rightX, startY, 80, 16, 0.2f, AIGUI_FONT_SIZE_LARGE, BLACK);
        if (AIGUI_ButtonRounded("<", rightX, startY + 16, 16, 16, 0.01f, AIGUI_FONT_SIZE_LARGE, BLACK)) {
            difficultyIndex = (difficultyIndex - 1 + 3) % 3;
        }
        AIGUI_LabelRounded(difficultyLevels[difficultyIndex], rightX + 16, startY + 16, 48, 16, 0.2f, AIGUI_FONT_SIZE_MEDIUM, BLACK);
        if (AIGUI_ButtonRounded(">", rightX + 80, startY + 16, 16, 16, 0.01f, AIGUI_FONT_SIZE_LARGE, BLACK)) {
            difficultyIndex = (difficultyIndex + 1) % 3;
        }

        // Quickplay Settings (same position, larger font)
        if (AIGUI_ButtonRounded("Quickplay Settings", 20, 148, 120, 16, 0.01f, AIGUI_FONT_SIZE_LARGE, BLACK)) {
            currentMenu = QUICKPLAY_SETTINGS;
        }

        // Back (aligned with Quickplay Settings, bottom-right)
        if (AIGUI_ButtonRounded("Back", 220, 148, 80, 16, 0.01f, AIGUI_FONT_SIZE_LARGE, BLACK))
            currentMenu = MAIN_MENU;
    }
    else if (currentMenu == QUICKPLAY_SETTINGS)
    {
        float startX = 20;
        float startY = 10; // Moved up since we removed the header
        float labelWidth = 120;
        float labelHeight = 16;
        float spacingY = 12; // Increased spacing for better readability

        // Level name at the top, full width
        AIGUI_LabelRounded(levelNames[quickplaySettings.levelIndex], 10, startY, 300, 20, 0.2f, AIGUI_FONT_SIZE_LARGE, BLACK);
        startY += labelHeight + spacingY;

        // Level selection
        AIGUI_LabelRounded("Level:", startX, startY, 40, 16, 0.2f, AIGUI_FONT_SIZE_MEDIUM, BLACK);
        if (AIGUI_ButtonRounded("<", startX + 50, startY, 12, 12, 0.01f, AIGUI_FONT_SIZE_SMALL, BLACK)) {
            quickplaySettings.levelIndex = (quickplaySettings.levelIndex - 1 + 6) % 6;
        }
        AIGUI_LabelRounded(levelNames[quickplaySettings.levelIndex], startX + 70, startY, 120, 16, 0.2f, AIGUI_FONT_SIZE_MEDIUM, BLACK);
        if (AIGUI_ButtonRounded(">", startX + 190, startY, 12, 12, 0.01f, AIGUI_FONT_SIZE_SMALL, BLACK)) {
            quickplaySettings.levelIndex = (quickplaySettings.levelIndex + 1) % 6;
        }
        startY += labelHeight + spacingY;

        // Chill Mode
        if (AIGUI_ButtonRounded(quickplaySettings.chillMode ? "[x] Chill Mode" : "[ ] Chill Mode", startX, startY, 120, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, BLACK)) {
            quickplaySettings.chillMode = !quickplaySettings.chillMode;
            if (quickplaySettings.chillMode) {
                quickplaySettings.enableEnemies = false;
                quickplaySettings.enableObstacles = false;
            }
        }
        startY += labelHeight + spacingY;

        // Enemies - Gray out for Park Level (levelIndex 0)
        bool isParkLevel = (quickplaySettings.levelIndex == 0);
        if (AIGUI_ButtonRounded(quickplaySettings.enableEnemies ? "[x] Enemies" : "[ ] Enemies", startX, startY, 120, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, (quickplaySettings.chillMode || isParkLevel) ? GRAY : BLACK)) {
            if (!quickplaySettings.chillMode && !isParkLevel) {
                quickplaySettings.enableEnemies = !quickplaySettings.enableEnemies;
            }
        }
        startY += labelHeight + spacingY;

        // Obstacles
        if (AIGUI_ButtonRounded(quickplaySettings.enableObstacles ? "[x] Obstacles" : "[ ] Obstacles", startX, startY, 120, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, quickplaySettings.chillMode ? GRAY : BLACK)) {
            if (!quickplaySettings.chillMode) {
                quickplaySettings.enableObstacles = !quickplaySettings.enableObstacles;
            }
        }
        startY += labelHeight + spacingY;

        // Swinging Pipes - Gray out for Park Level (0) and Desert Level (2), in addition to Sewer Level (1)
        bool isUnsupportedLevel = (quickplaySettings.levelIndex == 0 || quickplaySettings.levelIndex == 1 || quickplaySettings.levelIndex == 2);
        if (AIGUI_ButtonRounded(quickplaySettings.swingingPipes ? "[x] Swinging Pipes" : "[ ] Swinging Pipes", startX, startY, 120, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, isUnsupportedLevel ? GRAY : BLACK)) {
            if (!isUnsupportedLevel) {
                quickplaySettings.swingingPipes = !quickplaySettings.swingingPipes;
            }
        }
        startY += labelHeight + spacingY;

        // Back button
        if (AIGUI_ButtonRounded("Back", 220, 150, 80, 16, 0.01f, AIGUI_FONT_SIZE_MEDIUM, BLACK)) {
            currentMenu = OPTIONS_MENU;
        }
    }
    else
    {
        AIGUI_LabelRounded("ERROR: Unhandled menu state!", 10, 10, 200, 16, 0.2f, AIGUI_FONT_SIZE_SMALL, RED);
    }
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