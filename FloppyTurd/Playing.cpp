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
#include <cmath>

Playing::Playing(Game* game) {
	using namespace Resources;
	this->game = game;
	player = new Player(game);

	stats.Load(); // Load persistent stats
	TOTALCOINS = stats.totalCoins; // Initialize TOTALCOINS from stats

	if (game->mainMenu) {
		difficultyIndex = game->mainMenu->GetDifficultyIndex();
		player->SetInitialHearts(difficultyIndex);
	}

	LoadSessionRecords(); // Initialize sessionRecords

	pauseMenuBackground = LoadTexture(PauseMenuBackground);
	Scoreboard = LoadTexture(ScoreBoard);
	_TurdHeart = LoadTexture(TurdHeart);
	_CoinBag = LoadTexture(coinbagtexture);
	ScoreSound = LoadSound(GotScore);
	arrowLeft = LoadTexture(ArrowLeft);
	arrowRight = LoadTexture(ArrowRight);
	arrowLeftHover = LoadTexture(ArrowLeftHover);
	arrowRightHover = LoadTexture(ArrowRightHover);
	SCORE = 0;
	TOTALSCORE = 0;

	gameOverMusic = new AudioClip(GameOverMusic);
	PreLoadLevels();
	SetCurrentLevel(0);

	gameOverBackground = LoadTexture(GameOverBackground);
	tryAgainBackground = LoadTexture(TryAgainBackground);
	deadFloppy = LoadTexture(DeadFloppy);
	gameOverScore = LoadTexture(GameOverScore);

	InitializeSkillNodes();
	InitializeHats();

	// Sync loaded states with game objects
	for (int i = 0; i < 5; ++i) {
		skillUnlocked[i] = stats.skillUnlocked[i];
		if (skillUnlocked[i]) {
			switch (i) {
			case 0: player->EnableShooting(true); break;
			case 1: player->SetHeartMode(Player::HALVES); break;
			case 2: player->EnableCoinMagnet(true); break;
			case 3: player->ActivateBigTurdBuff(15.0f); break;
			case 4: player->SetHeartMode(Player::THIRDS); break;
			}
		}
	}
	for (int i = 0; i < 18; ++i) {
		if (i < hats.size() && stats.hatUnlocked[i]) {
			hats[i]->status = UNLOCKED;
			if (i == 0) currentSelectedHat = hats[i]; // Default to first unlocked hat
		}
	}
	if (game->mainMenu) {
		for (int i = 0; i < 6; ++i) {
			game->mainMenu->levelsUnlocked[i] = stats.levelUnlocked[i];
			if (i == 0) game->mainMenu->levelsUnlocked[i] = true; // Ensure Park is always unlocked
		}
	}

	floppyButtonBlue = LoadTexture(blueButton);
	floppyButtonBlueHover = LoadTexture(blueButtonHover);

	AudioManager::GetInstance().LoadSoundEffect("GotCoin", Resources::GotCoin);
	AudioManager::GetInstance().LoadSoundEffect("GotHealth", Resources::GotHealth);
	AudioManager::GetInstance().LoadSoundEffect("GotHealthBig", Resources::GotHealthBig);

	Texture2D rawSnowTexture = LoadTexture(Snowfall);
	snowOverlay = std::make_unique<SnowOverlay>(rawSnowTexture, 16, 0.15f);
	SetTextureWrap(rawSnowTexture, TEXTURE_WRAP_CLAMP);

	savePending = false; // Ensure savePending starts false
}

void Playing::LoadSessionRecords() {
	for (int i = 0; i < 6; ++i) {
		sessionRecords[i] = stats.levelHighScores[i];
	}
}

void Playing::InitializeHats() {
	using namespace Resources;

	hatFrameNormal = LoadTexture(HatFrameNormal);
	hatFrameHover = LoadTexture(HatFrameHover);
	hatFrameSelected = LoadTexture(HatFrameSelected);
	hatFrameLocked = LoadTexture(HatFrameLocked);
	hatFrameDenied = LoadTexture(HatFrameDenied);

	Hat* cowboyHat = new Hat("Cowboy Hat",
		CowboyHat,
		CowboyHatTurdlet, 6,
		CowboyHatTurdletShoot, 9,
		CowboyHatTeenage, 6,
		CowboyHatTeenageShoot, 8,
		CowboyHatBigTurd, 6,
		CowboyHatBigTurdShoot, 8,
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
	UnloadTexture(Scoreboard);
	UnloadTexture(_TurdHeart);
	UnloadTexture(_CoinBag);
	UnloadTexture(arrowLeft);
	UnloadTexture(arrowRight);
	UnloadTexture(arrowLeftHover);
	UnloadTexture(arrowRightHover);
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

	for (auto hat : hats) {
		delete hat;
	}
	delete player;
}

void Playing::InitializeSkillNodes() {
	for (int i = 0; i < totalSkillNodes; ++i) {
		skillUnlocked[i] = false;
	}
	selectedNode = 0;
}

bool Playing::PurchaseItem(int cost) {
	int sessionCoins = player->GetSessionCoins();
	if (sessionCoins + TOTALCOINS < cost) return false;

	int sessionDeduction = std::min(sessionCoins, cost);
	player->AddCoins(-sessionDeduction);
	TOTALCOINS -= (cost - sessionDeduction);
	stats.totalCoins = TOTALCOINS; // Sync persistent coins
	savePending = true; // Flag for save on next death or pause
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
	if (idx < 0 || idx >= totalSkillNodes || skillUnlocked[idx]) return;
	if (PurchaseItem(skillCosts[idx])) {
		skillUnlocked[idx] = true;
		stats.skillUnlocked[idx] = true; // Sync with stats
		switch (idx) {
		case 0: player->EnableShooting(true); break;
		case 1: player->SetHeartMode(Player::HALVES); break;
		case 2: player->EnableCoinMagnet(true); break;
		case 3: player->ActivateBigTurdBuff(15.0f); break;
		case 4: player->SetHeartMode(Player::THIRDS); break;
		}
	}
}

void Playing::OutputHatMenu() {
	const int gridStartX = 50;
	const int gridStartY = 50;
	const int slotWidth = 32;
	const int slotHeight = 32;
	const int spacingX = 4;
	const int spacingY = 4;
	const int hatCosts = 50;

	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 6; col++) {
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

			if (clicked && hats[index]->status == UNLOCKED) {
				currentSelectedHat = hats[index];
				player->SetHat(currentSelectedHat);
			}
			else if (clicked && hats[index]->status == LOCKED) {
				Font font = game->GetScaledFont(1.2f);
				std::string costStr = std::to_string(hatCosts);
				float costX = static_cast<float>(x + slotWidth + 5);
				float costY = static_cast<float>(y + 8);
				DrawTexturePro(
					_CoinBag,
					{ 0, 0, (float)_CoinBag.width, (float)_CoinBag.height },
					{ costX, costY, 12.0f, 12.0f },
					{ 0, 0 }, 0.0f, WHITE
				);
				DrawTextEx(font, costStr.c_str(), { costX + 14.0f, costY }, 12.0f, 1.0f, WHITE);
				if (AIGUI_ButtonRounded("Buy", static_cast<float>(x + slotWidth + 5), static_cast<float>(y + 20), 40.0f, 12.0f, 0.3f, 12, WHITE)) {
					if (PurchaseItem(hatCosts)) {
						hats[index]->status = UNLOCKED;
						stats.hatUnlocked[index] = true; // Sync with stats
						currentSelectedHat = hats[index];
						player->SetHat(currentSelectedHat);
					}
				}
				if (player->GetSessionCoins() + TOTALCOINS < hatCosts) {
					DrawTextEx(font, "Not enough!", { static_cast<float>(x + slotWidth + 5), static_cast<float>(y + 34) }, 10.0f, 1.0f, RED);
				}
			}

			if (hats[index]->status == UNLOCKED && hats[index]->icon.id != 0) {
				int iconX = x + (slotWidth - hats[index]->icon.width) / 2;
				int iconY = y + (slotHeight - hats[index]->icon.height) / 2;
				DrawTexture(hats[index]->icon, iconX, iconY, WHITE);
			}

			if (hats[index]->status == LOCKED) {
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

void Playing::DrawPauseMenu() {
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

	if (AIGUI_ImageButton(
		floppyButtonBlue, floppyButtonBlueHover,
		topRowX, topRowY,
		buttonWidth, buttonHeight,
		"Hats", 16,
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
		"Stats", 16,
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
		"System", 16,
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

		if (leftClicked && selectedNode > 0) {
			selectedNode--;
		}
		if (rightClicked && selectedNode < totalSkillNodes - 1) {
			selectedNode++;
		}

		Font font = game->GetScaledFont(1.2f);
		float y = skillArea.y + 10;

		DrawTextEx(font, skillNames[selectedNode], { skillArea.x + 10, y }, 20, 1.0f, YELLOW);
		y += 20;

		DrawTextEx(font, skillDescs[selectedNode], { skillArea.x + 10, y }, 18, 1.0f, WHITE);
		y += 36;

		if (skillUnlocked[selectedNode]) {
			DrawTextEx(font, "Unlocked", { skillArea.x + 10, y + 5 }, 18, 1.0f, GREEN); // Lowered by 5 pixels
		}
		else {
			std::string costStr = std::to_string(skillCosts[selectedNode]);
			float costWidth = MeasureTextEx(font, costStr.c_str(), 18, 1.0f).x;
			float coinX = skillArea.x + 10;
			float coinY = y + 5; // Lowered by 5 pixels
			DrawTexturePro(
				_CoinBag,
				{ 0, 0, (float)_CoinBag.width, (float)_CoinBag.height },
				{ coinX, coinY, 16, 16 },
				{ 0, 0 }, 0.0f, WHITE
			);
			DrawTextEx(font, costStr.c_str(), { coinX + 20, coinY }, 18, 1.0f, WHITE);
			if (AIGUI_ButtonRounded("Buy", skillArea.x + 10 + costWidth + 30, y + 5, 80, 18, 0.3f, 18, WHITE)) { // Font size increased to 18
				UnlockSkill(selectedNode);
			}
			if (player->GetSessionCoins() + TOTALCOINS < skillCosts[selectedNode]) {
				DrawTextEx(font, "Not enough coins!", { skillArea.x + 10, y + 25 }, 16, 1.0f, RED);
			}
		}
		break;
	}
	case HATS:
		OutputHatMenu();
		break;
	case STATS: {
		// Increased height to 120 to cover all text
		Rectangle statsArea = { 19, 40, 283, 120 };
		DrawRectangleRec(statsArea, Fade(BLACK, 0.7f));

		Font font = game->GetScaledFont(1.2f);
		float y = statsArea.y + 10;
		float x = statsArea.x + 2; // Widened left column start

		// General stats
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

		// Per-level high scores (excluding Level 6)
		y = statsArea.y + 10;
		x = statsArea.x + 160; // Widened right column start to 180
		for (int i = 0; i < 5; ++i) { // Changed from 6 to 5 to exclude Level 6
			DrawTextEx(font, TextFormat("Level %d High: %d", i + 1, stats.levelHighScores[i]), { x, y }, 16, 1.0f, WHITE);
			y += 14;
		}
		break;
	}
	case SYSTEM: {
		if (AIGUI_ImageButton(
			floppyButtonBlue, floppyButtonBlueHover,
			128, 140,
			64, 16,
			"Main Menu", 12,
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
			savePending = true; // Flag save on pause
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

		AudioManager::GetInstance().DrawAudioOptions(160 - 80, 40); // Centered at 160 (screen width 320 / 2 - 50 for offset)
		break;
	}
	}

	// Perform save if pending and paused
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
	int floppyX = (int)((320 - deadFloppy.width) / 2.0f);
	DrawTexture(deadFloppy, floppyX, floppyY, WHITE);

	static const char* poopMessages[] = {
		"Ahh poop.", "You pooped.", "Oh crap!", "Poop happens.",
		"Down the drain!", "That stinks.", "Toilet Trouble!", "You flushed!"
	};
	static int poopMsgIndex = GetRandomValue(0, (int)(sizeof(poopMessages) / sizeof(char*)) - 1);
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
		stats.totalLevelTries++; // Increment tries
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

void Playing::Update() {
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

		if (levelIndex >= 0) {
			UpdateSessionRecord(levelIndex, SCORE);
			stats.totalPipes = std::max(stats.totalPipes, stats.totalPipes + SCORE);
			savePending = true; // Ensure save
		}

		TOTALCOINS += player->GetSessionCoins();
		stats.totalCoins = TOTALCOINS;
		savePending = true; // Set flag on death

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

		// Trigger save here when game over is fully processed
		TriggerSaveIfPending();
		return;
	}

	if (!isPaused && !GAMEOVER) {
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
			}
			else {
				std::cerr << "Warning: currentLevel is null in Update!" << std::endl;
			}
		}
		else {
			std::cerr << "Warning: levelManager is null in Update!" << std::endl;
		}

		if (!turdHasFallenOffScreen)
			player->Update(deltaTime);

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
			{ 320.f - Scoreboard.width - 10, 180.f - Scoreboard.height - 10,
			 (float)Scoreboard.width, (float)Scoreboard.height },
			{ 0, 0 }, 0.f, WHITE);
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

	for (int h = 0; h < hearts; ++h) {
		int sliceStart = h * slicesPH;
		int liveHere = std::max(0, std::min(slicesPH, live - sliceStart));
		int ghostHere = std::max(0, std::min(slicesPH - liveHere, ghost - sliceStart));

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
			{ 0, 0 }, 0.f, WHITE);
	}

	float coinBagX = 4.0f;
	float coinBagY = 4.0f + _TurdHeart.height + 4.0f;
	DrawTexturePro(_CoinBag,
		{ 0, 0, (float)_CoinBag.width, (float)_CoinBag.height },
		{ coinBagX, coinBagY, (float)_CoinBag.width, (float)_CoinBag.height },
		{ 0, 0 }, 0.f, WHITE);
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
		sessionRecords[levelIndex] = std::max(sessionRecords[levelIndex], pipesPassed);
		stats.levelHighScores[levelIndex] = std::max(stats.levelHighScores[levelIndex], pipesPassed);
		savePending = true; // Flag save
		TraceLog(LOG_INFO, "Updated record for level %d: pipes=%d", levelIndex, pipesPassed);
	}
}

int Playing::GetTotalCoins() {
	if (player) {
		return TOTALCOINS + player->GetSessionCoins();
	}
	return TOTALCOINS; // Fallback if player is null (shouldn't happen post-init)
}