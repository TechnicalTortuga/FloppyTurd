#include "MainMenu.h"
#include "AIGUI.h"
#include "AudioManager.h"
#include "PlatformLayer.h"
#include "ResourceCompat.h"
#include "ResourceManager.h"
#include "UIManager.h"

// Enable draw call tracking
#define ENABLE_DRAW_CALL_TRACKING
#include "PerformanceProfiler.h"

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
static bool fullscreenEnabled = true; // Game starts in fullscreen mode

MainMenu::MainMenu(Game* game)
	: game(game), currentMenu(MAIN_MENU), fClickCount(0), fClickCooldown(0.0f), fHoverScale(1.0f), paintingHoverScale(1.0f), fartModeEnabled(false)
{
	using namespace Resources;

	try {
		TraceLog(LOG_INFO, "MainMenu constructor STARTING");

		// Validate game pointer
		if (!game) {
			TraceLog(LOG_ERROR, "MainMenu constructor: game pointer is null");
			throw std::invalid_argument("Game pointer is null");
		}

		// Use ResourceManager instead of direct LoadTexture calls
		TraceLog(LOG_INFO, "MainMenu constructor - Loading textures from ResourceManager");
		
		try {
			emptyPainting = ResourceManager::GetInstance().GetTexture("empty_painting");
			_MenuBackground = ResourceManager::GetInstance().GetTexture("main_menu_bg");
			_FloppyLogo = ResourceManager::GetInstance().GetTexture("floppy_logo");
			finLogo = ResourceManager::GetInstance().GetTexture("fin_logo");
			lockedPainting = ResourceManager::GetInstance().GetTexture("locked_painting");

			levelPaintings[0] = ResourceManager::GetInstance().GetTexture("park_painting");
			levelPaintings[1] = ResourceManager::GetInstance().GetTexture("sewer_painting");
			levelPaintings[2] = ResourceManager::GetInstance().GetTexture("desert_painting");
			levelPaintings[3] = ResourceManager::GetInstance().GetTexture("snow_painting");
			levelPaintings[4] = ResourceManager::GetInstance().GetTexture("castle_painting");
			levelPaintings[5] = ResourceManager::GetInstance().GetTexture("ratking_painting");

			TraceLog(LOG_INFO, "MainMenu constructor - Textures loaded successfully");
		} catch (const std::exception& e) {
			TraceLog(LOG_ERROR, "Exception loading MainMenu textures: %s", e.what());
			// Initialize with empty textures to prevent crashes
			emptyPainting = { 0 };
			_MenuBackground = { 0 };
			_FloppyLogo = { 0 };
			finLogo = { 0 };
			lockedPainting = { 0 };
			for (int i = 0; i < 6; i++) {
				levelPaintings[i] = { 0 };
			}
		} catch (...) {
			TraceLog(LOG_ERROR, "Unknown exception loading MainMenu textures");
			// Initialize with empty textures to prevent crashes
			emptyPainting = { 0 };
			_MenuBackground = { 0 };
			_FloppyLogo = { 0 };
			finLogo = { 0 };
			lockedPainting = { 0 };
			for (int i = 0; i < 6; i++) {
				levelPaintings[i] = { 0 };
			}
		}

		const char* fartPaths[] = {
			"sounds/fart1.ogg", "sounds/fart2.ogg", "sounds/fart3.ogg", "sounds/fart4.ogg", "sounds/fart5.ogg",
			"sounds/fart6.ogg", "sounds/fart7.ogg", "sounds/fart8.ogg", "sounds/fart9.ogg", "sounds/fart10.ogg", "sounds/fart11.ogg"
		};

		TraceLog(LOG_INFO, "MainMenu constructor - Loading fart sounds");
		for (int i = 0; i < 11; ++i) {
			try {
				fartSoundsLoaded[i] = LoadSound(fartPaths[i]);
			} catch (const std::exception& e) {
				TraceLog(LOG_ERROR, "Exception loading fart sound %d: %s", i, e.what());
				fartSoundsLoaded[i] = { 0 }; // Initialize with empty sound
			} catch (...) {
				TraceLog(LOG_ERROR, "Unknown exception loading fart sound %d", i);
				fartSoundsLoaded[i] = { 0 }; // Initialize with empty sound
			}
		}

		TraceLog(LOG_INFO, "MainMenu constructor - Creating AudioClip");
		try {
			currentMusic = new AudioClip("mainmenu/FloppyTurdMenu.mp3");
			PlayMusic(currentMusic);
		} catch (const std::exception& e) {
			TraceLog(LOG_ERROR, "Exception creating AudioClip: %s", e.what());
			currentMusic = nullptr;
		} catch (...) {
			TraceLog(LOG_ERROR, "Unknown exception creating AudioClip");
			currentMusic = nullptr;
		}

		if (game && game->playing) {
			TraceLog(LOG_INFO, "MainMenu constructor - Setting up level unlocks");
			for (int i = 0; i < 6; ++i) {
				levelsUnlocked[i] = game->playing->GetStats().levelUnlocked[i];
				if (i == 0) levelsUnlocked[i] = true; // Ensure Park is always unlocked
			}
		}

		TraceLog(LOG_INFO, "MainMenu constructor COMPLETED SUCCESSFULLY");
	} catch (const std::exception& e) {
		TraceLog(LOG_ERROR, "Exception in MainMenu constructor: %s", e.what());
		// Initialize with default values to prevent crashes
		emptyPainting = { 0 };
		_MenuBackground = { 0 };
		_FloppyLogo = { 0 };
		finLogo = { 0 };
		lockedPainting = { 0 };
		for (int i = 0; i < 6; ++i) {
			levelPaintings[i] = { 0 };
		}
		for (int i = 0; i < 11; ++i) {
			fartSoundsLoaded[i] = { 0 };
		}
		currentMusic = nullptr;
	} catch (...) {
		TraceLog(LOG_ERROR, "Unknown exception in MainMenu constructor");
		// Initialize with default values to prevent crashes
		emptyPainting = { 0 };
		_MenuBackground = { 0 };
		_FloppyLogo = { 0 };
		finLogo = { 0 };
		lockedPainting = { 0 };
		for (int i = 0; i < 6; ++i) {
			levelPaintings[i] = { 0 };
		}
		for (int i = 0; i < 11; ++i) {
			fartSoundsLoaded[i] = { 0 };
		}
		currentMusic = nullptr;
	}
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

		// Use platform-agnostic input
		auto& platform = PlatformLayer::GetInstance();
		if (fHovered && platform.IsPrimaryInputPressed() && fClickCooldown <= 0.0f)
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

bool MainMenu::CanPurchaseLevel(int levelIndex, int totalCoins, const int sessionRecords[6]) {
	if (levelIndex < 0 || levelIndex >= 6) return false;
	if (!game || !game->playing) return false;

	int previousPipes = (levelIndex > 0) ? game->playing->GetStats().levelHighScores[levelIndex - 1] : 0;

	switch (levelIndex) {
	case 0: // Park
		return true;
	case 1: // Sewer
		return previousPipes >= 50;
	case 2: // Desert
		return totalCoins >= 100 && previousPipes >= 50;
	case 3: // Snow
		return totalCoins >= 250 && previousPipes >= 50;
	case 4: // Castle
		return totalCoins >= 500 && previousPipes >= 50;
	case 5: // Rat King
		return totalCoins >= 1000 && previousPipes >= 50;
	default:
		return false;
	}
}

bool MainMenu::PurchaseLevel(int levelIndex) {
	TraceLog(LOG_INFO, "Attempting to purchase level %d", levelIndex);
	if (levelsUnlocked[levelIndex]) {
		TraceLog(LOG_INFO, "Level %d already unlocked", levelIndex);
		return true;
	}
	if (!game || !game->playing || !game->playing->player) {
		TraceLog(LOG_WARNING, "Purchase failed: Invalid game or player pointers");
		return false;
	}

	int totalCoins = game->playing->GetTotalCoins();
	int sessionRecords[6];
	for (int i = 0; i < 6; ++i) {
		sessionRecords[i] = game->playing->GetSessionRecord(i);
	}

	if (!CanPurchaseLevel(levelIndex, totalCoins, sessionRecords)) {
		TraceLog(LOG_WARNING, "Purchase failed: Requirements not met for level %d", levelIndex);
		return false;
	}

	int cost = 0;
	switch (levelIndex) {
	case 2: cost = 100; break; // Desert
	case 3: cost = 250; break; // Snow
	case 4: cost = 500; break; // Castle
	case 5: cost = 1000; break; // Rat King
	default: cost = 0; // Park and Sewer have no coin cost
	}

	// Deduct coins (0 for Sewer, others as specified)
	if (cost > 0) {
		game->playing->TOTALCOINS -= cost;
		game->playing->player->AddCoins(-cost); // Update session coins
	}
	levelsUnlocked[levelIndex] = true; // Unlock immediately
	game->playing->GetStats().levelUnlocked[levelIndex] = true; // Sync with stats
	game->playing->GetStats().Save();
	TraceLog(LOG_INFO, "Level %d unlocked, deducted %d coins", levelIndex, cost);
	return true;
}

void MainMenu::UpdateLevelUnlocks(int totalCoins, const int sessionRecords[6])
{
	// No automatic unlocking here; rely on PurchaseLevel to set levelsUnlocked
	// Only Park is always unlocked by default
	levelsUnlocked[0] = true; // Park
}

void MainMenu::Draw()
{
#if defined(PLATFORM_MOBILE)
	GameLog::Log("[MAINMENU] Using MOBILE UI path");
	DrawMobileUI();
#else
	GameLog::Log("[MAINMENU] Using DESKTOP UI path");
	DrawDesktopUI();
#endif
}

void MainMenu::DrawDesktopUI()
{
	using namespace GameSettings;

	DrawCallTracker::TrackDrawTexturePro(_MenuBackground,
		Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
		Rectangle{ 0, 0, 320, 180 },
		Vector2{ 0,0 }, 0.0f, WHITE);

	float logoWidth = _FloppyLogo.width;
	float logoHeight = _FloppyLogo.height;

	DrawTexturePro(_FloppyLogo,
		Rectangle{ 0, 0, (float)_FloppyLogo.width, (float)_FloppyLogo.height },
		Rectangle{ (320.0f - logoWidth) / 2, 10, logoWidth, logoHeight },
		Vector2{ 0,0 }, 0.0f, WHITE);

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

		DrawCallTracker::TrackDrawTexturePro(
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
			if (game->playing) game->playing->SetCurrentLevel(quickplaySettings.levelIndex);
		}
		buttonY += buttonH + spacingY;

		if (AIGUI_ButtonRounded("Exit", buttonX, buttonY, buttonW, buttonH, cornerRadius, 24, BLACK))
			game->SetGameState(game->SHUTDOWN);

		float creditsBtnX = 6.0f;          // a little padding from the left edge
		float creditsBtnY = 180.0f - 22.0f; // 2 px above bottom edge
		float creditsBtnW = 70.0f;
		float creditsBtnH = 18.0f;
		cornerRadius = 0.15f;        // small rounded corners

		if (AIGUI_ButtonRounded("Credits", creditsBtnX, creditsBtnY,
			creditsBtnW, creditsBtnH,
			cornerRadius, 18, BLACK))
		{
			// Stop main-menu music so it doesn't overlap the credits track
			AudioManager::GetInstance().StopMusic();

			// Jump straight into the scrolling-credits scene
			game->SetGameState(Game::CREDITS);
		}
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

		// Only draw locked painting if not unlocked
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
		AIGUI_LabelRounded(title, 10.0f, 10.0f, 300.0f, 24.0f, 0.2f, 18, BLACK); // Increased font to 20

		float infoY = 55.0f;
		int totalCoins = 0;
		int sessionRecords[6] = { 0 };
		if (game && game->playing) {
			totalCoins = game->playing->GetTotalCoins();
			for (int i = 0; i < 6; ++i) {
				sessionRecords[i] = game->playing->GetSessionRecord(i);
			}
		}

		if (!levelsUnlocked[currentLevelIndex]) {
			int requiredCoins = 0;
			int requiredPipes = 0;
			int previousLevelIndex = currentLevelIndex - 1;

			// Set requirements based on level
			switch (currentLevelIndex) {
			case 1: requiredPipes = 50; break; // Sewer requires 50 pipes from Park
			case 2: requiredCoins = 100; requiredPipes = 50; break; // Desert
			case 3: requiredCoins = 250; requiredPipes = 50; break; // Snow
			case 4: requiredCoins = 500; requiredPipes = 50; break; // Castle
			case 5: requiredCoins = 1000; requiredPipes = 50; break; // Rat King
			default: requiredPipes = 0; break; // Park
			}

			std::string coinStatus = TextFormat("Coins: %d/%d", totalCoins, requiredCoins);
			std::string pipeStatus = (requiredPipes > 0 && previousLevelIndex >= 0) ? TextFormat("Previous Pipes: %d/%d", sessionRecords[previousLevelIndex], requiredPipes) : "";

			AIGUI_LabelRounded(coinStatus.c_str(), 80.0f, infoY + 54.0f, 160.0f, 24.0f, 0.2f, 18, totalCoins >= requiredCoins ? GREEN : BLACK); // Increased font to 20
			if (!pipeStatus.empty()) {
				AIGUI_LabelRounded(pipeStatus.c_str(), 60.0f, infoY, 200.0f, 24.0f, 0.2f, 18, sessionRecords[previousLevelIndex] >= requiredPipes ? GREEN : BLACK); // Increased font to 20
			}

			if (CanPurchaseLevel(currentLevelIndex, totalCoins, sessionRecords)) {
				std::string costStr = std::to_string(requiredCoins);
				Vector2 unlockPos = { paintingX + painting.width + 10.0f, paintingY + painting.height / 2.0f - 9.0f };
				Rectangle buttonRect = { unlockPos.x, unlockPos.y + 18.0f, 70.0f, 24.0f }; // Increased button height

				// Draw coin icon and cost
				DrawTexturePro(
					game->playing->GetCoinBagTexture(),
					{ 0, 0, (float)game->playing->GetCoinBagTexture().width, (float)game->playing->GetCoinBagTexture().height },
					{ unlockPos.x, unlockPos.y, 16.0f, 16.0f },
					{ 0, 0 }, 0.0f, WHITE
				);
				DrawTextEx(hdFont, costStr.c_str(), { unlockPos.x + 18.0f, unlockPos.y - 2.0f }, 20.0f, 1.0f, WHITE); // Increased font to 20

				bool isButtonHovered = CheckCollisionPointRec(g_AIGUI.mousePos, buttonRect);
				if (isButtonHovered) {
					TraceLog(LOG_INFO, "Unlock button hovered at (%f, %f)", g_AIGUI.mousePos.x, g_AIGUI.mousePos.y);
				}

				if (AIGUI_ButtonRounded("Unlock", unlockPos.x, unlockPos.y + 2, 70.0f, 24.0f, 0.3f, 18, WHITE)) { // Increased font to 20
					TraceLog(LOG_INFO, "Unlock button clicked for level %d", currentLevelIndex);
					if (PurchaseLevel(currentLevelIndex)) {
						TraceLog(LOG_INFO, "Level %d unlocked successfully!", currentLevelIndex);
						PlayRandomFartSound(); // Celebrate with a fart!
					}
					else {
						TraceLog(LOG_WARNING, "Failed to unlock level %d", currentLevelIndex);
					}
				}
			}
			else {
				AIGUI_LabelRounded("Locked", paintingX + 8.0f, paintingY + painting.height / 2.0f - 2, 80.0f, 16.0f, 0.2f, 20, RED, Fade(RED, 0.3f)); // New styled label
			}
		}

		// Always allow click if unlocked, no need for separate hover check here
		if (levelsUnlocked[currentLevelIndex] && CheckCollisionPointRec(g_AIGUI.mousePos, scaledRect))
		{
			// Use platform-agnostic input
			auto& platform = PlatformLayer::GetInstance();
			if (platform.IsPrimaryInputReleased())
			{
				game->SetGameState(Game::PLAYING);
				if (game && game->playing) game->playing->SetCurrentLevel(currentLevelIndex);
			}
		}

		// Reverted button positions from MainMenuold.cpp
		if (currentLevelIndex > 0 && AIGUI_ButtonRounded("Previous", 40.0f, 150.0f, 100.0f, 24.0f, 0.1f, 20, BLACK)) // Increased font to 20
			currentLevelIndex--;
		if (currentLevelIndex < 5 && AIGUI_ButtonRounded("Next", 150.0f, 150.0f, 60.0f, 24.0f, 0.1f, 20, BLACK)) // Increased font to 20
			currentLevelIndex++;
		if (AIGUI_ButtonRounded("Back", 220.0f, 150.0f, 80.0f, 24.0f, 0.1f, 20, BLACK)) // Increased font to 20
			currentMenu = MAIN_MENU;
	}
	else if (currentMenu == OPTIONS_MENU)
	{
		float rightX = 185.0f;
		float startY = 20.0f;

		AudioManager::GetInstance().DrawAudioOptions(20.0f, 20.0f);

		startY = 20.0f;
		AIGUI_LabelRounded("Resolution:", rightX, startY, 120.0f, 20.0f, 0.2f, 20, BLACK);
		if (AIGUI_ButtonRounded("<", rightX, startY + 24.0f, 20.0f, 20.0f, 0.1f, 20, BLACK)) {
			currentAspectIndex = (currentAspectIndex - 1 + 7) % 7;
			const auto& option = aspectRatios[currentAspectIndex];
			if (option.width > 0 && option.height > 0)
				SetWindowSize(option.width, option.height);
		}
		AIGUI_LabelRounded(aspectRatios[currentAspectIndex].name, rightX + 23.0f, startY + 24.0f, 80.0f, 20.0f, 0.2f, 20, BLACK);
		if (AIGUI_ButtonRounded(">", rightX + 106.0f, startY + 24.0f, 20.0f, 20.0f, 0.1f, 20, BLACK)) {
			currentAspectIndex = (currentAspectIndex + 1) % 7;
			const auto& option = aspectRatios[currentAspectIndex];
			if (option.width > 0 && option.height > 0)
				SetWindowSize(option.width, option.height);
		}

		startY = 68.0f;
		AIGUI_LabelRounded("Difficulty:", rightX, startY, 120.0f, 20.0f, 0.2f, 20, BLACK);
		if (AIGUI_ButtonRounded("<", rightX, startY + 24.0f, 20.0f, 20.0f, 0.1f, 20, BLACK)) {
			difficultyIndex = (difficultyIndex - 1 + 3) % 3;
		}
		AIGUI_LabelRounded(difficultyLevels[difficultyIndex], rightX + 23.0f, startY + 24.0f, 80.0f, 20.0f, 0.2f, 20, BLACK);
		if (AIGUI_ButtonRounded(">", rightX + 106.0f, startY + 24.0f, 20.0f, 20.0f, 0.1f, 20, BLACK)) {
			difficultyIndex = (difficultyIndex + 1) % 3;
		}

		startY = 89.0f;
		if (AIGUI_ButtonRounded(fullscreenEnabled ? "[x] Fullscreen" : "[ ] Fullscreen", rightX - 25, startY + 28.0f, 150.0f, 20.0f, 0.1f, 20, BLACK)) {
			fullscreenEnabled = !fullscreenEnabled;
			ToggleFullscreen();
		}

		// No Quickplay Settings for Now
		//if (AIGUI_ButtonRounded("Quickplay Settings", 20.0f, 148.0f, 184.0f, 20.0f, 0.1f, 20, BLACK)) {
			//currentMenu = QUICKPLAY_SETTINGS;
		//}

		if (AIGUI_ButtonRounded("Back", 220.0f, 148.0f, 80.0f, 20.0f, 0.1f, 20, BLACK))
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
		if (AIGUI_ButtonRounded("<", startX + 50.0f, startY, 20.0f, 20.0f, 0.1f, 20, BLACK)) {
			quickplaySettings.levelIndex = (quickplaySettings.levelIndex - 1 + 6) % 6;
		}
		AIGUI_LabelRounded(levelNames[quickplaySettings.levelIndex], startX + 70.0f, startY, 124.0f, 20.0f, 0.2f, 20, BLACK);
		if (AIGUI_ButtonRounded(">", startX + 194.0f, startY, 20.0f, 20.0f, 0.1f, 20, BLACK)) {
			quickplaySettings.levelIndex = (quickplaySettings.levelIndex + 1) % 6;
		}
		startY += labelHeight + spacingY;

		if (AIGUI_ButtonRounded(quickplaySettings.chillMode ? "[x] Chill Mode" : "[ ] Chill Mode", startX, startY, 120.0f, 20.0f, 0.1f, 20, BLACK)) {
			quickplaySettings.chillMode = !quickplaySettings.chillMode;
			if (quickplaySettings.chillMode) {
				quickplaySettings.enableEnemies = false;
				quickplaySettings.enableObstacles = false;
			}
		}
		startY += labelHeight + spacingY;

		bool isParkLevel = (quickplaySettings.levelIndex == 0);
		if (AIGUI_ButtonRounded(quickplaySettings.enableEnemies ? "[x] Enemies" : "[ ] Enemies", startX, startY, 120.0f, 20.0f, 0.1f, 20, (quickplaySettings.chillMode || isParkLevel) ? GRAY : BLACK)) {
			if (!quickplaySettings.chillMode && !isParkLevel) {
				quickplaySettings.enableEnemies = !quickplaySettings.enableEnemies;
			}
		}
		startY += labelHeight + spacingY;

		if (AIGUI_ButtonRounded(quickplaySettings.enableObstacles ? "[x] Obstacles" : "[ ] Obstacles", startX, startY, 120.0f, 20.0f, 0.1f, 20, quickplaySettings.chillMode ? GRAY : BLACK)) {
			if (!quickplaySettings.chillMode) {
				quickplaySettings.enableObstacles = !quickplaySettings.enableObstacles;
			}
		}
		startY += labelHeight + spacingY;

		bool isUnsupportedLevel = (quickplaySettings.levelIndex == 0 || quickplaySettings.levelIndex == 1 || quickplaySettings.levelIndex == 2);
		if (AIGUI_ButtonRounded(quickplaySettings.swingingPipes ? "[x] Swinging Pipes" : "[ ] Swinging Pipes", startX, startY, 120.0f, 20.0f, 0.1f, 20, isUnsupportedLevel ? GRAY : BLACK)) {
			if (!isUnsupportedLevel) {
				quickplaySettings.swingingPipes = !quickplaySettings.swingingPipes;
			}
		}
		startY += labelHeight + spacingY;

		if (AIGUI_ButtonRounded("Back", 220.0f, 150.0f, 80.0f, 20.0f, 0.1f, 20, BLACK))
			currentMenu = OPTIONS_MENU;
	}
	else
	{
		AIGUI_LabelRounded("ERROR: Unhandled menu state!", 10.0f, 10.0f, 200.0f, 20.0f, 0.2f, 20, RED);
	}

	UseHighDefFont(false);
}

void MainMenu::DrawMobileUI()
{
	UIManager& ui = UIManager::GetInstance();

	// Draw background to fill the screen
	float screenWidth = ui.GetSafeArea().width + ui.GetSafeArea().x * 2;
	float screenHeight = ui.GetSafeArea().height + ui.GetSafeArea().y * 2;
	DrawTexturePro(_MenuBackground,
		Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
		Rectangle{ 0, 0, screenWidth, screenHeight },
		Vector2{ 0,0 }, 0.0f, WHITE);

	// Draw logo at the top-center of the safe area
	Vector2 logoPos = ui.GetPosition(UIAnchor::TOP_CENTER, {0, 50});
	float logoScale = ui.GetScaleFactor() * 1.2f;
	float logoWidth = _FloppyLogo.width * logoScale;
	float logoHeight = _FloppyLogo.height * logoScale;
	DrawTexturePro(_FloppyLogo,
		Rectangle{ 0, 0, (float)_FloppyLogo.width, (float)_FloppyLogo.height },
		Rectangle{ logoPos.x - logoWidth / 2, logoPos.y, logoWidth, logoHeight },
		Vector2{ 0,0 }, 0.0f, WHITE);

	// Draw buttons
	float buttonWidth = screenWidth * 0.8f; // 80% of screen width for better mobile fit
	float buttonHeight = 60 * ui.GetScaleFactor();
	float buttonSpacing = 20 * ui.GetScaleFactor();

	Vector2 playPos = ui.GetPosition(UIAnchor::CENTER, {0, -buttonHeight});
	Vector2 optionsPos = ui.GetPosition(UIAnchor::CENTER, {0, buttonSpacing});
	Vector2 creditsPos = ui.GetPosition(UIAnchor::CENTER, {0, buttonHeight + buttonSpacing * 2});
	Vector2 quitPos = ui.GetPosition(UIAnchor::BOTTOM_CENTER, {0, -buttonHeight});

	// Debug logging to see what coordinates UIManager is returning
	GameLog::Log("[UIMANAGER] Screen: %.0fx%.0f, SafeArea: %.0fx%.0f at (%.0f,%.0f)", 
		ui.GetSafeArea().width + ui.GetSafeArea().x * 2, 
		ui.GetSafeArea().height + ui.GetSafeArea().y * 2,
		ui.GetSafeArea().width, ui.GetSafeArea().height, ui.GetSafeArea().x, ui.GetSafeArea().y);
	GameLog::Log("[UIMANAGER] PLAY button: (%.1f,%.1f), size: %.1fx%.1f", 
		playPos.x - buttonWidth / 2, playPos.y, buttonWidth, buttonHeight);
	GameLog::Log("[UIMANAGER] OPTIONS button: (%.1f,%.1f), size: %.1fx%.1f", 
		optionsPos.x - buttonWidth / 2, optionsPos.y, buttonWidth, buttonHeight);

	if (AIGUI_Button("PLAY", playPos.x - buttonWidth / 2, playPos.y, buttonWidth, buttonHeight)) {
		currentMenu = LEVEL_SELECT;
	}
	if (AIGUI_Button("OPTIONS", optionsPos.x - buttonWidth / 2, optionsPos.y, buttonWidth, buttonHeight)) {
		currentMenu = OPTIONS_MENU;
	}
	if (AIGUI_Button("CREDITS", creditsPos.x - buttonWidth / 2, creditsPos.y, buttonWidth, buttonHeight)) {
		game->SetGameState(Game::CREDITS);
	}
	if (AIGUI_Button("QUIT", quitPos.x - buttonWidth / 2, quitPos.y, buttonWidth, buttonHeight)) {
		game->SetGameState(Game::SHUTDOWN);
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
		PlayMusic(new AudioClip("mainmenu/FloppyTurdMenu.mp3"));
	else
		PlayMusic(new AudioClip("mainmenu/FloppyTurdMenu Fart Variant.mp3"));

	fartModeEnabled = !fartModeEnabled;
}

void MainMenu::HandleInput()
{
	if (IsKeyPressed(KEY_E))
	{
		if (game) {
			game->SetGameState(Game::PLAYING);
			if (game->playing) game->playing->SetCurrentLevel(quickplaySettings.levelIndex);
		}
	}
}