#include "Playing.h"
#include "AIGUI.h"
#include "RaylibCompat.h"
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
#include "PlatformLayer.h"
#include <cmath>

Playing::Playing(Game* game) {
    using namespace Resources;
    this->game = game;
    player = new Player(game);

    stats.Load();
    TOTALCOINS = stats.totalCoins;

    if (game->mainMenu) {
        difficultyIndex = game->mainMenu->GetDifficultyIndex();
        player->SetInitialHearts(difficultyIndex);
    }

    LoadSessionRecords();

    pauseMenuBackground = Resources::RM().GetTexture("pause_menu_bg");
    Scoreboard = Resources::GetScoreBoard();
    _TurdHeart = Resources::GetTurdHeart();
    _CoinBag = Resources::RM().GetTexture("coin_bag");
    ScoreSound = Resources::GetGotScore();
    arrowLeft = Resources::GetArrowLeft();
    arrowRight = Resources::GetArrowRight();
    arrowLeftHover = Resources::GetArrowLeftHover();
    arrowRightHover = Resources::GetArrowRightHover();
    SCORE = 0;
    TOTALSCORE = 0;

    gameOverMusic = new AudioClip("resources/music/gameover.mp3");
    PreLoadLevels();
    SetCurrentLevel(0);

    gameOverBackground = Resources::RM().GetTexture("game_over_bg");
    tryAgainBackground = Resources::RM().GetTexture("try_again_bg");
    deadFloppy = Resources::RM().GetTexture("dead_floppy");
    gameOverScore = Resources::RM().GetTexture("game_over_score");

    InitializeSkillNodes();
    InitializeHats();

    // Sync loaded states with game objects
    for (int i = 0; i < totalSkills; ++i) {
        skillUnlocked[i] = stats.skillUnlocked[i];
        if (skillUnlocked[i]) {
            switch (i) {
            case 0: 
                player->EnableShooting(true); 
                if (touchControls) touchControls->SetShootingEnabled(true);
                break;
            case 1: player->SetHeartMode(Player::HALVES); break;
            case 2: player->EnableCoinMagnet(true); break;
            case 3: player->EnableHeartMagnet(true); break;
            case 4: player->SetHeartMode(Player::THIRDS); break;
            case 5: player->EnableCoinShield(true); break;
            }
        }
    }
    for (int i = 0; i < 15; ++i) {
        if (i < hats.size() && stats.hatUnlocked[i]) {
            hats[i]->status = UNLOCKED;
            if (i == 0) currentSelectedHat = hats[i];
        }
    }
    if (game->mainMenu) {
        for (int i = 0; i < 6; ++i) {
            game->mainMenu->levelsUnlocked[i] = stats.levelUnlocked[i];
            if (i == 0) game->mainMenu->levelsUnlocked[i] = true;
        }
    }

    floppyButtonBlue = Resources::GetBlueButton();
    floppyButtonBlueHover = Resources::GetBlueButtonHover();

    AudioManager::GetInstance().LoadSoundEffect("GotCoin", "resources/sounds/pickup.ogg");
    AudioManager::GetInstance().LoadSoundEffect("GotHealth", "resources/sounds/SmallHealthPickup.wav");
    AudioManager::GetInstance().LoadSoundEffect("GotHealthBig", "resources/sounds/BigHealthPickup.wav");

    Texture2D rawSnowTexture = Resources::RM().GetTexture("snowfall");
    snowOverlay = std::make_unique<SnowOverlay>(rawSnowTexture, 16, 0.15f);
    SetTextureWrap(rawSnowTexture, TEXTURE_WRAP_CLAMP);

    // Initialize touch controls for mobile
    touchControls = new TouchControls();
    touchControls->Initialize(320, 180);  // Virtual resolution
    
    // Enable touch controls only on mobile platforms
    auto& platform = PlatformLayer::GetInstance();
    touchControls->SetEnabled(platform.IsTouchSupported());

    // Initialize AIGUI with touch controls for gesture support
    AIGUI_Init();
    AIGUI_SetTouchControls(touchControls);

    savePending = false;
}

void Playing::LoadSessionRecords() {
    for (int i = 0; i < 6; ++i) {
        sessionRecords[i] = stats.levelHighScores[i];
    }
}

void Playing::InitializeHats() {
    using namespace Resources;

    hatFrameNormal = Resources::RM().GetTexture("hat_frame_normal");
    hatFrameHover = Resources::RM().GetTexture("hat_frame_hover");
    hatFrameSelected = Resources::RM().GetTexture("hat_frame_selected");
    hatFrameLocked = Resources::RM().GetTexture("hat_frame_locked");
    hatFrameDenied = Resources::RM().GetTexture("hat_frame_denied");

    // Clear existing hats
    for (auto hat : hats) {
        delete hat;
    }
    hats.clear();

    // Define all 15 hats with their icon paths, sprite paths, and initial status
    // All hats now start as LOCKED by default. Using actual file paths instead of constants.
    Hat* cowboyHat = new Hat("Cowboy Hat", "resources/hats/cowboyhat.png", "resources/hats/cowboyhatturdletjump.png", 6, "resources/hats/cowboyhatturdletshoot.png", 5, "resources/hats/cowboyhatbigturdjump.png", 6, "resources/hats/cowboyhatbigturdshoot.png", 5, LOCKED);
    Hat* flowerHat = new Hat("Flower", "resources/hats/flowerhat.png", "resources/hats/flowerhatturdletjump.png", 6, "resources/hats/flowerhatturdletshoot.png", 5, "resources/hats/flowerhatbigturdjump.png", 6, "resources/hats/flowerhatbigturdshoot.png", 5, LOCKED);
    Hat* doorag = new Hat("Doorag", "resources/hats/dooraghat.png", "resources/hats/dooragturdletjump.png", 6, "resources/hats/dooragturdletshoot.png", 5, "resources/hats/dooragbigturdjump.png", 6, "resources/hats/dooragbigturdshoot.png", 5, LOCKED);
    Hat* ballcap = new Hat("Ballcap", "resources/hats/ballcap.png", "resources/hats/ballcapturdletjump.png", 6, "resources/hats/ballcapturdletshoot.png", 5, "resources/hats/ballcapbigturdjump.png", 6, "resources/hats/ballcapbigturdshoot.png", 5, LOCKED);
    Hat* pinwheelHat = new Hat("Pinwheel Hat", "resources/hats/PinwheelHat.png", "resources/hats/pinwheelturdletjump.png", 6, "resources/hats/pinwheelturdletshoot.png", 5, "resources/hats/pinwheelbigturdjump.png", 6, "resources/hats/pinwheelbigturdshoot.png", 5, LOCKED);
    Hat* strawHat = new Hat("Straw Hat", "resources/hats/strawhat.png", "resources/hats/strawhatturdletjump.png", 6, "resources/hats/strawhatturdletshoot.png", 5, "resources/hats/strawhatbigturdjump.png", 6, "resources/hats/strawhatbigturdshoot.png", 5, LOCKED);
    Hat* samuraiHat = new Hat("Samurai Hat", "resources/hats/SamuraiHelmet.png", "resources/hats/samuraiturdletjump.png", 6, "resources/hats/samuraiturdletshoot.png", 5, "resources/hats/samuraibigturdjump.png", 6, "resources/hats/samuraibigturdshoot.png", 5, LOCKED);
    Hat* topHat = new Hat("Top Hat", "resources/hats/tophat.png", "resources/hats/tophatturdletjump.png", 6, "resources/hats/tophatturdletshoot.png", 5, "resources/hats/tophatbigturdjump.png", 6, "resources/hats/tophatbigturdshoot.png", 5, LOCKED);
    Hat* ushanka = new Hat("Ushanka", "resources/hats/ushanka.png", "resources/hats/ushankaturdletjump.png", 6, "resources/hats/ushankaturdletshoot.png", 5, "resources/hats/ushankabigturdjump.png", 6, "resources/hats/ushankabigturdshoot.png", 5, LOCKED);
    Hat* beret = new Hat("Beret", "resources/hats/Beret.png", "resources/hats/berethatturdletjump.png", 6, "resources/hats/berethatturdletshoot.png", 5, "resources/hats/berethatbigturdjump.png", 6, "resources/hats/berethatbigturdshoot.png", 5, LOCKED);
    Hat* crown = new Hat("Crown", "resources/hats/Crown.png", "resources/hats/crownhatturdletjump.png", 6, "resources/hats/crownhatturdletshoot.png", 5, "resources/hats/crownhatbigturdjump.png", 6, "resources/hats/crownhatbigturdshoot.png", 5, LOCKED);
    Hat* poopHat = new Hat("Poop Hat", "resources/hats/poophat.png", "resources/hats/poophatturdletjump.png", 6, "resources/hats/poophatturdletshoot.png", 5, "resources/hats/poophatbigturdjump.png", 6, "resources/hats/poophatbigturdshoot.png", 5, LOCKED);
    Hat* ramsesHat = new Hat("Ramses Hat", "resources/hats/RamsesHat.png", "resources/hats/ramsesturdletjump.png", 6, "resources/hats/ramsesturdletshoot.png", 5, "resources/hats/ramsesbigturdjump.png", 6, "resources/hats/ramsesbigturdshoot.png", 5, LOCKED);
    Hat* spartanHelmet = new Hat("Spartan Helmet", "resources/hats/SpartanHelmet.png", "resources/hats/spartanhatturdletjump.png", 6, "resources/hats/spartanhatturdletshoot.png", 5, "resources/hats/spartanhatbigturdjump.png", 6, "resources/hats/spartanhatbigturdshoot.png", 5, LOCKED);
    Hat* shellHat = new Hat("Shell Hat", "resources/hats/shellhat.png", "resources/hats/shellhatturdletjump.png", 6, "resources/hats/shellhatturdletshoot.png", 5, "resources/hats/shellhatbigturdjump.png", 6, "resources/hats/shellhatbigturdshoot.png", 5, LOCKED);

    hats.push_back(cowboyHat);
    hats.push_back(flowerHat);
    hats.push_back(doorag);
    hats.push_back(ballcap);
    hats.push_back(pinwheelHat);
    hats.push_back(strawHat);
    hats.push_back(samuraiHat);
    hats.push_back(topHat);
    hats.push_back(ushanka);
    hats.push_back(beret);
    hats.push_back(crown);
    hats.push_back(poopHat);
    hats.push_back(ramsesHat);
    hats.push_back(spartanHelmet);
    hats.push_back(shellHat);

    // Sync with saved states
    for (int i = 0; i < 15; ++i) {
        if (i < hats.size() && stats.hatUnlocked[i]) {
            hats[i]->status = UNLOCKED;
        }
    }

    // No default hat is equipped. currentSelectedHat remains nullptr unless loaded from a save.
    player->SetHat(currentSelectedHat);
}

void Playing::PreLoadLevels() {
    levels.emplace_back(std::make_shared<ParkLevel>());
    levels.emplace_back(std::make_shared<SewerLevel>());
    levels.emplace_back(std::make_shared<DesertLevel>());
    levels.emplace_back(std::make_shared<SnowLevel>());
    levels.emplace_back(std::make_shared<CastleLevel>());
    levels.emplace_back(std::make_shared<BossLevel>());

    for (int i = 0; i < 6; ++i) {
        levels[i]->SetDifficulty(1);
        levels[i]->SetPanSpeed(80.0f);
    }
}

Playing::~Playing() {
    // ResourceManager handles texture cleanup automatically
    UnloadSound(ScoreSound);

    delete gameOverMusic;
    delete bossHealthBar;

    for (auto hat : hats) {
        delete hat;
    }
    delete player;
    delete touchControls;
}

void Playing::InitializeSkillNodes() {
    for (int i = 0; i < totalSkills; ++i) {
        skillUnlocked[i] = false;
    }
    selectedSkill = 0;
}

bool Playing::PurchaseItem(int cost) {
    int sessionCoins = player->GetSessionCoins();
    if (sessionCoins + TOTALCOINS < cost) return false;

    int sessionDeduction = std::min(sessionCoins, cost);
    player->AddCoins(-sessionDeduction);
    TOTALCOINS -= (cost - sessionDeduction);
    stats.totalCoins = TOTALCOINS;
    savePending = true;
    return true;
}

void Playing::TriggerSaveIfPending() {
    if (savePending) {
        stats.Save();
        savePending = false;
        TraceLog(LOG_INFO, "Saved stats due to pending flag");
    }
}

void Playing::UnlockSkill(int idx) {
    if (idx < 0 || idx >= totalSkills || skillUnlocked[idx]) return;
    // Enforce dependency: Heart Thirds (index 4) requires Heart Halves (index 1)
    if (idx == 4 && !skillUnlocked[1]) return;
    if (PurchaseItem(skillCosts[idx])) {
        skillUnlocked[idx] = true;
        stats.skillUnlocked[idx] = true;
        switch (idx) {
        case 0: 
            player->EnableShooting(true); 
            if (touchControls) touchControls->SetShootingEnabled(true);
            break;
        case 1: player->SetHeartMode(Player::HALVES); break;
        case 2: player->EnableCoinMagnet(true); break;
        case 3: player->EnableHeartMagnet(true); break;
        case 4: player->SetHeartMode(Player::THIRDS); break;
        case 5: player->EnableCoinShield(true); break;
        }
    }
}

void Playing::OutputHatMenu() {
    const int gridStartX = 20;
    const int gridStartY = 50;
    const int slotWidth = 32;
    const int slotHeight = 32;
    const int spacingX = 4;
    const int spacingY = 4;
    const int detailsPanelX = 200;
    const int detailsPanelY = 50;

    static int selectedHatIndex = -1;

    DrawRectangleRec({ (float)gridStartX - 2, (float)gridStartY - 2, 5 * (slotWidth + spacingX) + 2, 3 * (slotHeight + spacingY) + 2 }, Fade(BLACK, 0.7f));

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 5; col++) {
            int index = row * 5 + col;
            if (index >= hats.size()) break;

            int x = gridStartX + col * (slotWidth + spacingX);
            int y = gridStartY + row * (slotHeight + spacingY);
            Rectangle hatSlotRect = { (float)x, (float)y, (float)slotWidth, (float)slotHeight };

            bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, hatSlotRect);
            bool isEquipped = (currentSelectedHat == hats[index]);
            Hat* currentHat = hats[index];

            // --- Corrected Drawing Logic ---

            // 1. First, determine the single correct background frame. Hover state takes priority over selected state.
            Texture2D backgroundToDraw = hatFrameNormal;
            if (hovered) {
                backgroundToDraw = hatFrameHover;
            }
            else if (isEquipped) {
                backgroundToDraw = hatFrameSelected;
            }

            // 2. Draw the chosen background frame.
            DrawTexturePro(backgroundToDraw, { 0, 0, (float)backgroundToDraw.width, (float)backgroundToDraw.height }, hatSlotRect, { 0, 0 }, 0.0f, WHITE);

            // 3. THEN, draw the content (the icon or the lock) on TOP of the background.
            //    This ensures the icon/lock is always visible.
            if (currentHat->status == UNLOCKED) {
                if (
#if defined(__APPLE__) && TARGET_OS_IPHONE
                    currentHat->icon.texture != nullptr
#else
                    currentHat->icon.id != 0
#endif
                ) {
                    int iconX = x + (slotWidth - currentHat->icon.width) / 2;
                    int iconY = y + (slotHeight - currentHat->icon.height) / 2;
                    DrawTexture(currentHat->icon, iconX, iconY, WHITE);
                }
            }
            else { // Hat is LOCKED
                // The lock frame is drawn on top of the background, preserving hover/selection state.
                DrawTexturePro(hatFrameLocked, { 0, 0, (float)hatFrameLocked.width, (float)hatFrameLocked.height }, hatSlotRect, { 0, 0 }, 0.0f, WHITE);
            }

            // --- Interaction Logic ---
            if (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                selectedHatIndex = index;
            }
        }
    }

    // --- Details Panel Logic (remains unchanged) ---
    if (selectedHatIndex >= 0 && selectedHatIndex < hats.size()) {
        Hat* selectedHat = hats[selectedHatIndex];
        Font font = game->GetScaledFont(1.2f);

        std::string hatName = selectedHat->name;
        Vector2 nameSize = MeasureTextEx(font, hatName.c_str(), 20.0f, 1.0f);
        DrawTextEx(font, hatName.c_str(), { detailsPanelX + (100 - nameSize.x) / 2, detailsPanelY }, 20.0f, 1.0f, YELLOW);

        if (selectedHat->status == LOCKED) {
            int hatCost = (selectedHatIndex < 5) ? 100 : (selectedHatIndex < 10) ? 200 : 300;
            std::string costStr = std::to_string(hatCost);

            DrawTexturePro(_CoinBag, { 0, 0, (float)_CoinBag.width, (float)_CoinBag.height }, { detailsPanelX + 20.0f, detailsPanelY + 30.0f, 16.0f, 16.0f }, { 0, 0 }, 0.0f, WHITE);
            DrawTextEx(font, costStr.c_str(), { detailsPanelX + 40.0f, detailsPanelY + 28.0f }, 18.0f, 1.0f, WHITE);

            if (AIGUI_ButtonRounded("Buy", detailsPanelX + 10, detailsPanelY + 60, 80.0f, 18.0f, 0.3f, 18, WHITE)) {
                if (PurchaseItem(hatCost)) {
                    selectedHat->status = UNLOCKED;
                    stats.hatUnlocked[selectedHatIndex] = true;
                    currentSelectedHat = selectedHat;
                    player->SetHat(currentSelectedHat);
                    savePending = true;
                }
            }
            if (player->GetSessionCoins() + TOTALCOINS < hatCost) {
                DrawTextEx(font, "Not enough coins!", { detailsPanelX, detailsPanelY + 85 }, 18.0f, 1.0f, WHITE);
            }
        }
        else {
            const char* buttonText = "Equip";
            bool isEquipped = (currentSelectedHat == selectedHat);

            if (isEquipped) {
                buttonText = "Equipped";
            }

            if (AIGUI_ButtonRounded(buttonText, detailsPanelX + 10, detailsPanelY + 60, 80.0f, 18.0f, 0.3f, 18, WHITE)) {
                if (isEquipped) {
                    currentSelectedHat = nullptr;
                    player->SetHat(nullptr);
                }
                else {
                    currentSelectedHat = selectedHat;
                    player->SetHat(currentSelectedHat);
                }
            }
        }
    }
}

void Playing::DrawPauseMenu() {
    // Adjust position based on safe area insets for iOS
    Rectangle safeArea = AIGUI_GetSafeAreaInsets();
    float offsetX = safeArea.x;
    float offsetY = safeArea.y;

    DrawTexturePro(
        pauseMenuBackground,
        Rectangle{ 0, 0, (float)pauseMenuBackground.width, (float)pauseMenuBackground.height },
        Rectangle{ 10 + offsetX, 10 + offsetY, (float)pauseMenuBackground.width, (float)pauseMenuBackground.height },
        Vector2{ 0, 0 },
        0.0f,
        WHITE
    );

    const int buttonWidth = 64;
    const int buttonHeight = 16;
    const int buttonSpacing = 4;
    const int topRowY = 20 + offsetY;
    const int totalButtons = 4;
    int totalWidth = totalButtons * buttonWidth + (totalButtons - 1) * buttonSpacing;
    int topRowX = (320 - totalWidth) / 2 + offsetX;

    if (AIGUI_ImageButton(
        floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "Skills", 18,
        WHITE,
        nullptr
    )) {
        currentTab = SKILLS;
    }
    topRowX += buttonWidth + buttonSpacing;

    if (AIGUI_ImageButton(
        floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "Hats", 18,
        WHITE,
        nullptr
    )) {
        currentTab = HATS;
    }
    topRowX += buttonWidth + buttonSpacing;

    if (AIGUI_ImageButton(
        floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "Stats", 18,
        WHITE,
        nullptr
    )) {
        currentTab = STATS;
    }
    topRowX += buttonWidth + buttonSpacing;

    if (AIGUI_ImageButton(
        floppyButtonBlue, floppyButtonBlueHover,
        topRowX, topRowY,
        buttonWidth, buttonHeight,
        "System", 18,
        WHITE,
        nullptr
    )) {
        currentTab = SYSTEM;
    }

    switch (currentTab) {
    case SKILLS: {
        Rectangle skillArea = { 24, 40, 272, 100 };
        DrawRectangleRec(skillArea, Fade(BLACK, 0.7f));

        int arrowSize = 24;
        int arrowY = skillArea.y + (skillArea.height - arrowSize) / 2 + 8;
        int leftArrowX = skillArea.x + 10;
        int rightArrowX = skillArea.x + skillArea.width - arrowSize - 10;

        bool leftClicked = AIGUI_ImageButton(
            arrowLeft, arrowLeftHover,
            leftArrowX, arrowY,
            arrowSize, arrowSize,
            nullptr, 0,
            WHITE,
            nullptr
        );
        bool rightClicked = AIGUI_ImageButton(
            arrowRight, arrowRightHover,
            rightArrowX, arrowY,
            arrowSize, arrowSize,
            nullptr, 0,
            WHITE,
            nullptr
        );

        if (leftClicked && selectedSkill > 0) {
            selectedSkill--;
        }
        if (rightClicked && selectedSkill < totalSkills - 1) {
            selectedSkill++;
        }

        Font font = game->GetScaledFont(1.2f);
        float y = skillArea.y + 10;

        DrawTextEx(font, skillNames[selectedSkill], { skillArea.x + 10, y }, 20, 1.0f, YELLOW);
        y += 20;

        DrawTextEx(font, skillDescs[selectedSkill], { skillArea.x + 10, y }, 18, 1.0f, WHITE);
        y += 36;

        if (skillUnlocked[selectedSkill]) {
            DrawTextEx(font, "Unlocked", { skillArea.x + 10, y + 5 }, 18, 1.0f, GREEN);
        }
        else {
            // Check if Heart Thirds (index 4) is selected and Heart Halves (index 1) is not unlocked
            bool canPurchase = (selectedSkill != 4 || skillUnlocked[1]);
            if (canPurchase) {
                std::string costStr = std::to_string(skillCosts[selectedSkill]);
                float costWidth = MeasureTextEx(font, costStr.c_str(), 18, 1.0f).x;
                float coinX = skillArea.x + 10;
                float coinY = y + 5;
                DrawTexturePro(
                    _CoinBag,
                    { 0, 0, (float)_CoinBag.width, (float)_CoinBag.height },
                    { coinX, coinY, 16, 16 },
                    { 0, 0 }, 0.0f, WHITE
                );
                DrawTextEx(font, costStr.c_str(), { coinX + 20, coinY }, 18, 1.0f, WHITE);
                const char* buttonText = (selectedSkill == 4) ? "Upgrade" : "Buy";
                if (AIGUI_ButtonRounded(buttonText, skillArea.x + 10 + costWidth + 30, y + 5, 80, 18, 0.3f, 18, WHITE)) {
                    UnlockSkill(selectedSkill);
                }
                if (player->GetSessionCoins() + TOTALCOINS < skillCosts[selectedSkill]) {
                    DrawTextEx(font, "Not enough coins!", { skillArea.x + 10, y + 25 }, 18, 1.0f, WHITE);
                }
            }
            else {
                DrawTextEx(font, "Requires Heart Halves!", { skillArea.x + 10, y + 5 }, 18, 1.0f, RED);
            }
        }
        break;
    }
    case HATS:
        OutputHatMenu();
        break;
    case STATS: {
        Rectangle statsArea = { 19, 40, 283, 120 };
        DrawRectangleRec(statsArea, Fade(BLACK, 0.7f));

        Font font = game->GetScaledFont(1.2f);
        float y = statsArea.y + 10;
        float x = statsArea.x + 2;

        DrawTextEx(font, TextFormat("Total Pipes: %d", stats.totalPipes), { x, y }, 18, 1.0f, WHITE);
        y += 16;
        DrawTextEx(font, TextFormat("Total Coins: %d", stats.totalCoins), { x, y }, 18, 1.0f, WHITE);
        y += 16;
        DrawTextEx(font, TextFormat("Enemies Killed: %d", stats.totalEnemiesKilled), { x, y }, 18, 1.0f, WHITE);
        y += 16;
        DrawTextEx(font, TextFormat("Times Jumped: %d", stats.totalJumps), { x, y }, 18, 1.0f, WHITE);
        y += 16;
        DrawTextEx(font, TextFormat("Times Flopped: %d", stats.totalFlops), { x, y }, 18, 1.0f, WHITE);
        y += 16;
        DrawTextEx(font, TextFormat("Level Tries: %d", stats.totalLevelTries), { x, y }, 18, 1.0f, WHITE);

        y = statsArea.y + 10;
        x = statsArea.x + 160;
        for (int i = 0; i < 5; ++i) {
            DrawTextEx(font, TextFormat("Level %d High: %d", i + 1, stats.levelHighScores[i]), { x, y }, 16, 1.0f, WHITE);
            y += 14;
        }
        break;
    }
    case SYSTEM: {
        if (AIGUI_ImageButton(
            floppyButtonBlue, floppyButtonBlueHover,
            122, 140,
            82, 20,
            "Main Menu", 16,
            WHITE,
            nullptr
        )) {
            auto current = levelManager->GetCurrentLevel();
            int levelIndex = -1;
            if (dynamic_cast<ParkLevel*>(current.get())) {
                levelIndex = 0;
            }
            else if (dynamic_cast<SewerLevel*>(current.get())) {
                levelIndex = 1;
            }
            else if (dynamic_cast<DesertLevel*>(current.get())) {
                levelIndex = 2;
            }
            else if (dynamic_cast<SnowLevel*>(current.get())) {
                levelIndex = 3;
            }
            else if (dynamic_cast<CastleLevel*>(current.get())) {
                levelIndex = 4;
            }
            else if (dynamic_cast<BossLevel*>(current.get())) {
                levelIndex = 5;
            }

            if (levelIndex >= 0) {
                UpdateSessionRecord(levelIndex, SCORE);
            }

            TOTALCOINS += player->GetSessionCoins();
            stats.totalCoins = TOTALCOINS;
            savePending = true;
            if (game->mainMenu) {
                game->mainMenu->UpdateLevelUnlocks(TOTALCOINS, sessionRecords);
                game->mainMenu->ResetMusic();
            }

            AudioManager::GetInstance().StopMusic();
            if (currentMusic) currentMusic->Stop();
            player->Revive();
            SCORE = 0;
            player->ResetSessionCoins();
            isPaused = false;
            game->SetGameState(Game::MAINMENU);
        }

        AudioManager::GetInstance().DrawAudioOptions(160 - 80, 40);
        break;
    }
    }

    if (savePending && isPaused) {
        stats.Save();
        savePending = false;
    }
}

void Playing::DrawGameOverScreen() {
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
    int floppyX = (int)((320 - deadFloppy.width) / 2);
    DrawTexture(deadFloppy, floppyX, floppyY, WHITE);

    static const char* poopMessages[] = {
        "Ahh poop.", "You pooped.", "Oh crap!", "Poop happens.",
        "Down the drain!", "That stinks.", "Toilet Trouble!", "You flushed!"
    };
    static int poopMsgIndex = GetRandomValue(0, (int)(sizeof(poopMessages) / sizeof(char*)) - 1);
    const char* msg = poopMessages[poopMsgIndex];
    int msgWidth = MeasureText(msg, 18);
    Font hdFont = game->GetScaledFont(1.2f);
    DrawTextEx(hdFont, msg, { 320.0f - msgWidth / 2.0f, 6.0f }, 18.0f, 1.0f, BLACK);

    int btnW = 128, btnH = 22, spacing = 12;
    int btnY = (int)(panelY + panelH) - btnH - 64;
    int btnX = (int)(panelX + (panelW - 2 * btnW - spacing) / 2.0f);

    if (AIGUI_ButtonRounded("Try Again", (float)btnX, (float)btnY, (float)btnW, (float)btnH, 0.3f, 24, BLACK)) {
        AudioManager::GetInstance().StopMusic();
        if (gameOverMusic) gameOverMusic->Stop();

        int levelIndex = 0;
        switch (lastLevelType) {
        case LastLevelType::PARK:   levelIndex = 0; break;
        case LastLevelType::SEWER:  levelIndex = 1; break;
        case LastLevelType::DESERT: levelIndex = 2; break;
        case LastLevelType::SNOW:   levelIndex = 3; break;
        case LastLevelType::CASTLE: levelIndex = 4; break;
        case LastLevelType::BOSS:   levelIndex = 5; break;
        default:                    levelIndex = 0; break;
        }

        SetCurrentLevel(levelIndex);
        player->Revive();
        SCORE = 0;

        GAMEOVER = false;
        gameOverTriggered = false;
        turdHasFallenOffScreen = false;
        stats.totalLevelTries++;
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
    Font font = game->GetScaledFont(1.2f);
    DrawTextEx(font, scoreStr.c_str(), { sbX + (sbW - (float)scoreW) / 2.0f + 20, sbY + (sbH / 2.0f - 12.0f) - 12 }, 24.0f, 1.0f, BLACK);

    std::string coinStr = std::to_string(player->GetSessionCoins());
    int coinW = MeasureText(coinStr.c_str(), 20);
    DrawTextEx(font, coinStr.c_str(), { sbX + (sbW - (float)coinW) / 2.0f + 20, sbY + (sbH / 2.0f + 4.0f) }, 24.0f, 1.0f, BLACK);
}

void Playing::Update() {
    deltaTime = GetFrameTime();

    if (isPaused) {
        return;
    }

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

        if (levelIndex >= 0) {
            UpdateSessionRecord(levelIndex, SCORE);
            savePending = true;
        }

        TOTALCOINS += player->GetSessionCoins();
        stats.totalCoins = TOTALCOINS;
        savePending = true;

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
        if (!turdHasFallenOffScreen && player->GetPosition().y >= 180) {
            turdHasFallenOffScreen = true;
        }
        if (turdHasFallenOffScreen && !GAMEOVER) {
            GAMEOVER = true;
        }
        TriggerSaveIfPending();
        return;
    }

    if (!GAMEOVER) {
        if (levelManager) {
            UpdatePlayerPosition();
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
                                    snowball->GetHitbox())) {
                                    player->PutTheHurtOn(1);
                                    snowball->Deactivate();
                                    break;
                                }
                            }
                        }
                    }
                }

                auto& pickups = currentLevel->GetPickUps();
                for (auto it = pickups.begin(); it != pickups.end(); ) {
                    if (CheckCollisionCircleRec(
                        player->GetCircleCenter(),
                        player->GetCircleRadius(),
                        (*it)->GetHitbox())) {
                        (*it)->OnPickup();
                        if (auto heart = dynamic_cast<PoopHeart*>((*it).get())) {
                            int healAmount = heart->GetHealAmount();
                            player->AddHeartSlice(healAmount);
                            if (healAmount == 9) {
                                player->ActivateInvisibility(10.0f);
                                if (!player->IsBigTurdActive()) {
                                    player->ActivateBigTurdBuff(15.0f);
                                }
                            }
                        }
                        else if (auto coin = dynamic_cast<Coin*>((*it).get())) {
                            int coinValue = coin->GetValue();
                            player->AddCoins(coinValue);
                        }
                        it = pickups.erase(it);
                    }
                    else {
                        ++it;
                    }
                }

                if (player->coinMagnet) {
                    for (auto& pickup : pickups) {
                        if (auto coin = dynamic_cast<Coin*>(pickup.get())) {
                            coin->SetPlayerPosition(&player->circleCenter);
                        }
                    }
                }
                if (player->heartMagnet) {
                    for (auto& pickup : pickups) {
                        if (auto heart = dynamic_cast<PoopHeart*>(pickup.get())) {
                            heart->SetPlayerPosition(&player->circleCenter);
                        }
                    }
                }
            }
            else {
                std::cerr << "Warning: currentLevel is null in Update!" << std::endl;
            }
        }
        else {
            std::cerr << "Warning: levelManager is null in Update!" << std::endl;
        }
        if (!turdHasFallenOffScreen) {
            player->Update(deltaTime);
        }

        for (const auto& enemy : levelManager->GetEnemies()) {
            if (auto tp = dynamic_cast<ToiletPaper*>(enemy.get())) {
                if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), tp->GetHitbox())) {
                    player->PutTheHurtOn(1);
                }
            }
            else if (auto bird = dynamic_cast<Bird*>(enemy.get())) {
                if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), bird->GetHitbox())) {
                    player->PutTheHurtOn(1);
                }
            }
            else if (auto ratCopter = dynamic_cast<RatCopter*>(enemy.get())) {
                if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), ratCopter->GetHitbox())) {
                    player->PutTheHurtOn(1);
                    ratCopter->TakeDamage();
                }
            }
        }

        std::vector<Projectile*>& projectiles = player->GetProjectilesNonConst();
        std::vector<std::shared_ptr<Enemy>>& enemies = levelManager->GetEnemies();

        for (auto projIt = projectiles.begin(); projIt != projectiles.end(); ) {
            Projectile* proj = *projIt;
            Rectangle projHitbox = proj->GetHitbox();
            bool projectileHit = false;

            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end(); ++enemyIt) {
                if (CheckCollisionRecs(projHitbox, (*enemyIt)->GetHitbox())) {
                    projectileHit = true;
                    (*enemyIt)->TakeDamage();
                    IncrementEnemiesKilled();
                    break;
                }
            }

            if (projectileHit) {
                delete proj;
                projIt = projectiles.erase(projIt);
            }
            else {
                ++projIt;
            }
        }

        if (snowOverlay) snowOverlay->Update(deltaTime);

        if (levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get())) {
            BossLevel* bossLevel = dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get());
            if (bossLevel) {
                if (bossLevel->IsComplete()) {
                    AudioManager::GetInstance().StopMusic();
                    if (currentMusic) currentMusic->Stop();
                    game->SetGameState(Game::CREDITS);
                }

                std::shared_ptr<Boss> boss = bossLevel->GetBoss();
                if (boss && boss->isActive) {
                    for (auto projIt = projectiles.begin(); projIt != projectiles.end(); ) {
                        Projectile* projectile = *projIt;
                        Rectangle projHitbox = projectile->GetHitbox();
                        bool hitBoss = false;
                        for (const auto& bossHitbox : boss->GetHitboxes()) {
                            if (CheckCollisionRecs(projHitbox, bossHitbox)) {
                                boss->TakeDamage(10);
                                delete projectile;
                                projIt = projectiles.erase(projIt);
                                hitBoss = true;
                                if (!boss->isActive) {
                                    IncrementEnemiesKilled();
                                }
                                break;
                            }
                        }
                        if (!hitBoss) ++projIt;
                    }
                    if (std::shared_ptr<RatKing> rk = std::dynamic_pointer_cast<RatKing>(boss)) {
                        for (auto* tp : rk->GetProjectiles()) {
                            if (!player->isInvisible && CheckCollisionCircleRec(player->GetCircleCenter(), player->GetCircleRadius(), tp->GetHitbox())) {
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

void Playing::Draw() {
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

    if (GAMEOVER) {
        DrawGameOverScreen();
        return;
    }

    UseHighDefFont(true);
    DrawUI();
    UseHighDefFont(false);

    // Draw touch controls for mobile
    if (touchControls && touchControls->IsEnabled() && !isPaused) {
        touchControls->Draw();
    }

    if (isPaused) {
        DrawPauseMenu();
        return;
    }

    if (levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get())) {
        BossLevel* bossLevel = dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get());
        if (bossLevel) {
            std::shared_ptr<Boss> boss = bossLevel->GetBoss();
            if (boss && boss->isActive && bossHealthBar) {
                bossHealthBar->Update(GetFrameTime());
                bossHealthBar->Draw();
            }
        }
    }

    // --- Debug Overlay for Physics Tuning ---
    static bool showDebug = false;
    if (IsKeyPressed(KEY_F1)) showDebug = !showDebug;
    if (IsKeyPressed(KEY_F2)) Player::godMode = !Player::godMode;
    if (showDebug) {
        // Use the default AIGUI font for debug overlay
        float x = 10.0f, y = 10.0f, width = 120.0f;
        DrawRectangleRec({x-6, y-6, width+12, 170}, Fade(BLACK, 0.85f));
        float v;
        v = player->GetJumpVelocity();
        AIGUI_SliderFloat("Jump Velocity", x, y, width, 0.0f, 800.0f, &v); player->SetJumpVelocity(v); y += 22;
        v = player->GetGravity();
        AIGUI_SliderFloat("Gravity", x, y, width, 0.0f, 800.0f, &v); player->SetGravity(v); y += 22;
        v = player->GetFastFallGravity();
        AIGUI_SliderFloat("Fast Fall Grav.", x, y, width, 0.0f, 1200.0f, &v); player->SetFastFallGravity(v); y += 22;
        v = player->GetMaxJumpSpeed();
        AIGUI_SliderFloat("Max Jump Speed", x, y, width, -800.0f, 0.0f, &v); player->SetMaxJumpSpeed(v); y += 22;
        v = player->GetMaxFallSpeed();
        AIGUI_SliderFloat("Max Fall Speed", x, y, width, 0.0f, 1200.0f, &v); player->SetMaxFallSpeed(v); y += 22;
        // God mode toggle
        const char* godText = Player::godMode ? "God Mode: ON" : "God Mode: OFF";
        AIGUI_LabelRounded(godText, x, y, width, 18, 0.2f, 14, YELLOW, Fade(BLACK, 0.7f)); y += 22;
        AIGUI_LabelRounded("[F1] hide  [F2] god", x, y, width, 18, 0.2f, 14, WHITE, Fade(BLACK, 0.5f));
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
    else {
        TraceLog(LOG_WARNING, "Attempted to play null music clip");
    }
}

void Playing::UpdateMusic() {
    if (currentMusic && !GAMEOVER) {
        float vol = AudioManager::GetInstance().IsMusicMuted() ? 0.0f : (float)AudioManager::GetInstance().GetMusicVolume() / 10.0f;
        currentMusic->SetVolume(vol);
        currentMusic->Update();
    }
}

void Playing::HandleInput() {
    if (gameOverState != NONE || paused) {
        return;
    }

    // Handle input based on platform
    auto& platform = PlatformLayer::GetInstance();
    if (platform.IsTouchSupported() && touchControls && touchControls->IsEnabled()) {
        // Handle touch input
        if (touchControls->IsJumpPressed()) {
            player->Jump();
        }
        if (touchControls->IsShootPressed()) {
            player->Shoot();
        }
        if (touchControls->IsShootHeld()) {
            player->Shoot();
        }
        // Integrate gesture recognition for additional controls
        if (touchControls->IsGestureDetected(GESTURE_SWIPE_UP)) {
            player->Jump(); // Swipe up can trigger a jump as an alternative input
        }
        if (touchControls->IsGestureDetected(GESTURE_SWIPE_DOWN)) {
            // Swipe down could trigger a special action if implemented
            // For now, just log for debugging
            TraceLog(LOG_INFO, "Swipe down detected");
        }
        if (touchControls->IsGestureDetected(GESTURE_SWIPE_LEFT) || touchControls->IsGestureDetected(GESTURE_SWIPE_RIGHT)) {
            // Swipe left/right could be used for dodging or quick menu navigation if needed
            TraceLog(LOG_INFO, "Swipe left/right detected");
        }
        if (touchControls->IsGestureDetected(GESTURE_PINCH_IN)) {
            // Pinch in could zoom out or trigger a defensive action
            TraceLog(LOG_INFO, "Pinch in detected");
        }
        if (touchControls->IsGestureDetected(GESTURE_PINCH_OUT)) {
            // Pinch out could zoom in or trigger an offensive action
            TraceLog(LOG_INFO, "Pinch out detected");
        }
    } else {
        // Handle keyboard/gamepad input
        if (IsKeyPressed(KEY_SPACE)) {
            player->Jump();
        }
        if (IsKeyDown(KEY_SPACE)) {
            player->Jump();
        }
        if (IsKeyPressed(KEY_ENTER)) {
            player->Shoot();
        }
        if (IsKeyDown(KEY_ENTER)) {
            player->Shoot();
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        paused = !paused;
    }
}

void Playing::FadeOutMusic(float deltaTime) {
}

void Playing::SetCurrentLevel(int levelIndex) {
    if (levelIndex < 0 || levelIndex >= levels.size()) {
        std::cerr << "Try Again Error: Invalid level index " << levelIndex << std::endl;
        return;
    }

    if (currentMusic) {
        currentMusic->Stop();
        currentMusic = nullptr;
    }

    std::shared_ptr<Level> newLevel;
    switch (levelIndex) {
    case 0: newLevel = std::make_shared<ParkLevel>(); break;
    case 1: newLevel = std::make_shared<SewerLevel>(); break;
    case 2: newLevel = std::make_shared<DesertLevel>(); break;
    case 3: newLevel = std::make_shared<SnowLevel>(); break;
    case 4: newLevel = std::make_shared<CastleLevel>(); break;
    case 5: newLevel = std::make_shared<BossLevel>(); break;
    default: newLevel = std::make_shared<ParkLevel>(); break;
    }

    if (game->mainMenu) {
        quickplaySettings = game->mainMenu->GetQuickplaySettings();
        difficultyIndex = game->mainMenu->GetDifficultyIndex();
        player->SetInitialHearts(difficultyIndex);
    }

    levelManager = std::make_unique<LevelManager>(newLevel);
    levelManager->SetQuickplaySettings(quickplaySettings);

    newLevel->SetDifficulty(difficultyIndex);
    float panSpeed = 80.0f;
    switch (difficultyIndex) {
    case 0: panSpeed = 60.0f; break;
    case 2: panSpeed = 120.0f; break;
    }
    newLevel->SetPanSpeed(panSpeed);

    if (BossLevel* bossLevel = dynamic_cast<BossLevel*>(newLevel.get())) {
        std::shared_ptr<Boss> boss = bossLevel->GetBoss();
        if (boss) {
            delete bossHealthBar;
            bossHealthBar = new BossHealthBar(boss, "King of Rats");
            TraceLog(LOG_INFO, "[Playing] Created BossHealthBar for initial RatKing");
        }
        player->SetMaxHearts(9);
    }

    player->ResetPosition();
    if (newLevel->GetAudioClip()) {
        PlayMusic(newLevel->GetAudioClip());
    }
    else {
        TraceLog(LOG_ERROR, "Failed to set music for level index %d", levelIndex);
    }
    isPaused = false;
    SCORE = 0;
    player->ResetSessionCoins();
}

void Playing::DrawUI() {
	using namespace Resources;
	using namespace GameSettings;

	if (!(levelManager && dynamic_cast<BossLevel*>(levelManager->GetCurrentLevel().get()))) {
		DrawTexturePro(Scoreboard,
			{ 0, 0, (float)Scoreboard.width, (float)Scoreboard.height },
			{ 320.0f - Scoreboard.width - 10, 180.0f - Scoreboard.height - 10,
			 (float)Scoreboard.width, (float)Scoreboard.height },
			{ 0, 0 }, 0.0f, WHITE);
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

	// Draw hearts only if there are live or ghost slices
	for (int h = 0; h < hearts; ++h) {
		int sliceStart = h * slicesPH;
		int liveHere = std::max(0, std::min(slicesPH, live - sliceStart));
		int ghostHere = std::max(0, std::min(slicesPH - liveHere, ghost - sliceStart));

		// Skip drawing if no live or ghost slices remain for this heart
		if (liveHere == 0 && ghostHere == 0) {
			continue;
		}

		const char* texPath = nullptr;
		if (slicesPH == 1) {
			if (liveHere == 1) texPath = TurdHeartSmall;
			else texPath = TurdHeart0Half;
		}
		else if (slicesPH == 2) {
			if (liveHere == 2) texPath = TurdHeartSmall;
			else if (liveHere == 1) texPath = TurdHeart1Half;
			else if (ghostHere >= 1) texPath = TurdHeart0HalfHollow;
			else texPath = TurdHeart0Half;
		}
		else {
			if (liveHere == 3) texPath = TurdHeartSmall;
			else if (liveHere == 2) texPath = TurdHeart2Thirds;
			else if (liveHere == 1) texPath = TurdHeart1Third;
			else if (ghostHere >= 1) texPath = TurdHeart0ThirdHollow;
			else texPath = TurdHeart0ThirdHollow;
		}

		Texture2D tex = TextureCache::Get(texPath);
		DrawTexturePro(tex,
			{ 0, 0, (float)tex.width, (float)tex.height },
			{ 4.0f + h * 32.0f, 4.0f, (float)tex.width, (float)tex.height },
			{ 0, 0 }, 0.0f, WHITE);
	}

	float coinBagX = 4.0f;
	float coinBagY = 4.0f + _TurdHeart.height + 4.0f;
	DrawTexturePro(_CoinBag,
		{ 0, 0, (float)_CoinBag.width, (float)_CoinBag.height },
		{ coinBagX, coinBagY, (float)_CoinBag.width, (float)_CoinBag.height },
		{ 0, 0 }, 0.0f, WHITE);
	std::string coinStr = TextFormat("%i", player->GetSessionCoins());
	Vector2 coinTextSize = MeasureTextEx(g_AIGUI.defaultFont, coinStr.c_str(), 20.0f, 1.0f);
	float coinTextX = coinBagX + _CoinBag.width + 4.0f;
	float coinTextY = coinBagY + (_CoinBag.height - coinTextSize.y) / 2.0f;
	Font fontToUse = IsHighDefFont() ? game->GetScaledFont(1.2f) : g_AIGUI.defaultFont;
	DrawTextEx(fontToUse, coinStr.c_str(), { coinTextX, coinTextY }, 24.0f, 1.0f, WHITE);
}

void Playing::UpdatePlayerPosition() {
    if (levelManager)
        levelManager->SetPlayerPosition(player->GetCircleCenter());
}

void Playing::UpdateSessionRecord(int levelIndex, int pipesPassed) {
	if (levelIndex >= 0 && levelIndex < 6) {
		stats.totalPipes += pipesPassed; // Add all pipes passed
		sessionRecords[levelIndex] = std::max(sessionRecords[levelIndex], pipesPassed);
		stats.levelHighScores[levelIndex] = std::max(stats.levelHighScores[levelIndex], pipesPassed);
		savePending = true;
		TraceLog(LOG_INFO, "Updated record for level %d: pipes=%d, totalPipes=%d", levelIndex, pipesPassed, stats.totalPipes);
	}
}

int Playing::GetTotalCoins() {
    if (player) {
        return TOTALCOINS + player->GetSessionCoins();
    }
    return TOTALCOINS;
}