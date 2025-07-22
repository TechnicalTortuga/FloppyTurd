#include "MainMenu.h"
#include "AIGUI.h"
#include "AudioManager.h"
#include "AudioStateManager.h"
#include "ResourceCompat.h"
#include "ResourceManager.h"
#include "UIManager.h"
#include "RenderLayer.h"

#include "PlatformAPI.h"

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
	: game(game)
	, currentMenu(MAIN_MENU)
	, _MenuBackground({0})
	, _FloppyLogo({0})
	, emptyPainting({0})
	, currentLevelIndex(0)
	, levelSelectMode(false)
	, levelSelectScrollOffset(0.0f)
	, lastTouchX(0.0f)
	, isDragging(false)
	, lockedPainting({0})
	, paintingHoverScale(1.0f)
	, finLogo({0})
	, fHoverScale(1.0f)
	, fClickCount(0)
	, fClickCooldown(0.0f)
	, fartModeEnabled(false)
	, currentMusic(nullptr)
	, useHighDefFont(false)
	, difficultyIndex(1)
{
	using namespace Resources;

	try {
		GameLog::Log("[MAINMENU] Constructor starting");

		// Validate game pointer
		if (!game) {
			GameLog::Log("[MAINMENU] Constructor: game pointer is null");
			throw std::invalid_argument("Game pointer is null");
		}

		// Use ResourceManager instead of direct LoadTexture calls
		GameLog::Log("[MAINMENU] Loading textures from ResourceManager");
		
		try {
			emptyPainting = ResourceManager::GetInstance().GetTexture("empty_painting");
			
			// Use mobile background on mobile platforms
			if (IsMobilePlatform()) {
				_MenuBackground = ResourceManager::GetInstance().GetTexture("main_menu_bg_mobile");
				GameLog::Log("[MAINMENU] Using mobile background for mobile platform");
			} else {
				_MenuBackground = ResourceManager::GetInstance().GetTexture("main_menu_bg");
				GameLog::Log("[MAINMENU] Using desktop background for desktop platform");
			}
			
			_FloppyLogo = ResourceManager::GetInstance().GetTexture("floppy_logo");
			finLogo = ResourceManager::GetInstance().GetTexture("fin_logo");
			lockedPainting = ResourceManager::GetInstance().GetTexture("locked_painting");

			levelPaintings[0] = ResourceManager::GetInstance().GetTexture("park_painting");
			levelPaintings[1] = ResourceManager::GetInstance().GetTexture("sewer_painting");
			levelPaintings[2] = ResourceManager::GetInstance().GetTexture("desert_painting");
			levelPaintings[3] = ResourceManager::GetInstance().GetTexture("snow_painting");
			levelPaintings[4] = ResourceManager::GetInstance().GetTexture("castle_painting");
			levelPaintings[5] = ResourceManager::GetInstance().GetTexture("ratking_painting");

			GameLog::Log("[MAINMENU] Textures loaded successfully");
		} catch (const std::exception& e) {
			GameLog::Log("[MAINMENU] Exception loading MainMenu textures: %s", e.what());
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
			GameLog::Log("[MAINMENU] Unknown exception loading MainMenu textures");
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

		GameLog::Log("[MAINMENU] Loading fart sounds");
		for (int i = 0; i < 11; ++i) {
			try {
				fartSoundsLoaded[i] = LoadSound(fartPaths[i]);
				fartSoundsLoaded[i].length = 0; // Length will be set by platform implementation
			} catch (const std::exception& e) {
				GameLog::Log("[MAINMENU] Exception loading fart sound %d: %s", i, e.what());
				fartSoundsLoaded[i] = { 0, 0 }; // Initialize with empty sound
			} catch (...) {
				GameLog::Log("[MAINMENU] Unknown exception loading fart sound %d", i);
				fartSoundsLoaded[i] = { 0, 0 }; // Initialize with empty sound
			}
		}

		GameLog::Log("[MAINMENU] Main menu music will be handled by AudioStateManager during state transition");
		currentMusic = nullptr; // AudioStateManager will handle music

		if (game && game->playing) {
			GameLog::Log("[MAINMENU] Setting up level unlocks");
			for (int i = 0; i < 6; ++i) {
				levelsUnlocked[i] = game->playing->GetStats().levelUnlocked[i];
				if (i == 0) levelsUnlocked[i] = true; // Ensure Park is always unlocked
			}
		}

		// Initialize mobile level select variables
		levelSelectScrollOffset = 0.0f;
		lastTouchX = 0.0f;
		isDragging = false;
		
		GameLog::Log("[MAINMENU] Constructor completed successfully");
	} catch (const std::exception& e) {
		GameLog::Log("[MAINMENU] Exception in MainMenu constructor: %s", e.what());
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
			fartSoundsLoaded[i] = { 0, 0 };
		}
		currentMusic = nullptr;
	} catch (...) {
		GameLog::Log("[MAINMENU] Unknown exception in MainMenu constructor");
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
			fartSoundsLoaded[i] = { 0, 0 };
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

	// currentMusic is now managed by AudioStateManager, no need to delete
}

void MainMenu::PlayRandomFartSound()
{
	int index = GetRandomValue(0, 10);
	PlaySound(fartSoundsLoaded[index]);
}

void MainMenu::Update()
{
	// AudioStateManager handles all music updates
	AudioStateManager::GetInstance().Update(GetFrameTime());

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
		if (fHovered && IsPrimaryInputPressed() && fClickCooldown <= 0.0f)
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
	GameLog::Log("[MAINMENU] Attempting to purchase level %d", levelIndex);
	if (levelsUnlocked[levelIndex]) {
		GameLog::Log("[MAINMENU] Level %d already unlocked", levelIndex);
		return true;
	}
	if (!game || !game->playing || !game->playing->player) {
		GameLog::Log("[MAINMENU] Purchase failed: Invalid game or player pointers");
		return false;
	}

	int totalCoins = game->playing->GetTotalCoins();
	int sessionRecords[6];
	for (int i = 0; i < 6; ++i) {
		sessionRecords[i] = game->playing->GetSessionRecord(i);
	}

	if (!CanPurchaseLevel(levelIndex, totalCoins, sessionRecords)) {
		GameLog::Log("[MAINMENU] Purchase failed: Requirements not met for level %d", levelIndex);
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
	GameLog::Log("[MAINMENU] Level %d unlocked, deducted %d coins", levelIndex, cost);
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
			game->SetGameState(PLAYING);
			if (game->playing) game->playing->SetCurrentLevel(quickplaySettings.levelIndex);
		}
		buttonY += buttonH + spacingY;

		if (AIGUI_ButtonRounded("Exit", buttonX, buttonY, buttonW, buttonH, cornerRadius, 24, BLACK))
			game->SetGameState(SHUTDOWN);

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
			game->SetGameState(CREDITS);
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
					GameLog::Log("[MAINMENU] Unlock button hovered at (%f, %f)", g_AIGUI.mousePos.x, g_AIGUI.mousePos.y);
				}

				if (AIGUI_ButtonRounded("Unlock", unlockPos.x, unlockPos.y + 2, 70.0f, 24.0f, 0.3f, 18, WHITE)) { // Increased font to 20
					GameLog::Log("[MAINMENU] Unlock button clicked for level %d", currentLevelIndex);
					if (PurchaseLevel(currentLevelIndex)) {
						GameLog::Log("[MAINMENU] Level %d unlocked successfully!", currentLevelIndex);
						PlayRandomFartSound(); // Celebrate with a fart!
					}
					else {
						GameLog::Log("[MAINMENU] Failed to unlock level %d", currentLevelIndex);
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
			if (IsPrimaryInputReleased())
			{
				game->SetGameState(PLAYING);
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
		
		startY = 120.0f;
		if (AIGUI_ButtonRounded("Test Level Music", rightX - 25, startY + 28.0f, 150.0f, 20.0f, 0.1f, 20, BLACK)) {
			// Test the difficulty-based music system
			AudioStateManager::GetInstance().TestAllLevelMusic();
		}
		
		startY = 150.0f;
		if (AIGUI_ButtonRounded("Test Fart Mode", rightX - 25, startY + 28.0f, 150.0f, 20.0f, 0.1f, 20, BLACK)) {
			// Test the fart mode system
			AudioStateManager::GetInstance().TestFartMode();
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
	
	
	// Use UIManager for consistent coordinate handling
	UIManager& uiManager = UIManager::GetInstance();
	Rectangle pixelScreenRect = {0, 0, (float)uiManager.GetScreenWidth(), (float)uiManager.GetScreenHeight()};
	Rectangle safeAreaPx = uiManager.GetSafeArea();
	
	TraceLog(LOG_INFO, "[MAINMENU] PixelScreen: %.1fx%.1f, SafeAreaPx: x=%.1f y=%.1f w=%.1f h=%.1f", 
	         pixelScreenRect.width, pixelScreenRect.height, safeAreaPx.x, safeAreaPx.y, safeAreaPx.width, safeAreaPx.height);

	// --- Background: fill entire device screen using pixel coordinates ---
	float bgAspect = (float)_MenuBackground.width / (float)_MenuBackground.height;
	float screenAspect = pixelScreenRect.width / pixelScreenRect.height;
	float destWidth, destHeight, destX, destY;
	if (screenAspect > bgAspect) {
		destWidth = pixelScreenRect.width;
		destHeight = pixelScreenRect.width / bgAspect;
		destX = 0;
		destY = (pixelScreenRect.height - destHeight) / 2.0f;
	} else {
		destHeight = pixelScreenRect.height;
		destWidth = pixelScreenRect.height * bgAspect;
		destY = 0;
		destX = (pixelScreenRect.width - destWidth) / 2.0f;
	}
	TraceLog(LOG_INFO, "[MAINMENU] Background dest rect (pixels): x=%.1f y=%.1f w=%.1f h=%.1f", destX, destY, destWidth, destHeight);
	DrawTexturePro(_MenuBackground,
		Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
		Rectangle{ destX, destY, destWidth, destHeight },
		Vector2{ 0,0 }, 0.0f, WHITE);

	// --- Logo: center in full screen, scale up to 2.4x but not exceeding full screen width ---
	float logoMaxWidth = pixelScreenRect.width * 0.8f;
	float logoScale = fminf(2.4f, logoMaxWidth / (float)_FloppyLogo.width);
	float logoWidth = _FloppyLogo.width * logoScale;
	float logoHeight = _FloppyLogo.height * logoScale;
	float logoX = (pixelScreenRect.width - logoWidth) / 2.0f; // Center in full screen
	float logoY = pixelScreenRect.height * 0.08f; // 8% from top of full screen
	
	DrawTexturePro(_FloppyLogo,
		Rectangle{ 0, 0, (float)_FloppyLogo.width, (float)_FloppyLogo.height },
		Rectangle{ logoX, logoY, logoWidth, logoHeight },
		Vector2{ 0,0 }, 0.0f, WHITE);

	// --- Buttons: layout within full screen using percentages ---
	float buttonWidth = pixelScreenRect.width * 0.8f;
	float buttonHeight = pixelScreenRect.height * 0.08f; // 8% of full screen height (smaller buttons)
	float buttonSpacing = pixelScreenRect.height * 0.03f; // 3% spacing (tighter spacing)
	float centerX = pixelScreenRect.width / 2.0f; // Center of full screen
	float firstButtonY = logoY + logoHeight + pixelScreenRect.height * 0.20f; // 20% below logo (much lower)
	for (int i = 0; i < 4; ++i) {
		float btnY = firstButtonY + i * (buttonHeight + buttonSpacing);
		const char* label = (i == 0) ? "PLAY" : (i == 1) ? "OPTIONS" : (i == 2) ? "QUICKPLAY" : "QUIT";
		// Button drawing debug removed for cleaner logs
		if (AIGUI_ButtonRounded(label, centerX - buttonWidth / 2, btnY, buttonWidth, buttonHeight, 0.1f, buttonHeight * 0.4f, WHITE)) {
			if (i == 0) currentMenu = LEVEL_SELECT;
			else if (i == 1) currentMenu = MOBILE_OPTIONS_MENU;
			else if (i == 2) { 
				// Quickplay - start level 1 without anything special
				game->SetGameState(PLAYING);
				if (game->playing) game->playing->SetCurrentLevel(0); // Level 0 = Park
			}
			else if (i == 3) game->SetGameState(SHUTDOWN);
		}
	}
	
	// Handle different menu states for mobile
	if (currentMenu == LEVEL_SELECT) {
		DrawMobileLevelSelect();
	} else if (currentMenu == MOBILE_OPTIONS_MENU) {
		DrawMobileOptionsMenu();
	}
	
	
}

void MainMenu::DrawMobileOptionsMenu()
{
	
	
	// Use UIManager for consistent coordinate handling
	UIManager& uiManager = UIManager::GetInstance();
	Rectangle pixelScreenRect = {0, 0, (float)uiManager.GetScreenWidth(), (float)uiManager.GetScreenHeight()};
	Rectangle safeAreaPx = uiManager.GetSafeArea();
	
	// Draw background
	float bgAspect = (float)_MenuBackground.width / (float)_MenuBackground.height;
	float screenAspect = pixelScreenRect.width / pixelScreenRect.height;
	float destWidth, destHeight, destX, destY;
	if (screenAspect > bgAspect) {
		destWidth = pixelScreenRect.width;
		destHeight = pixelScreenRect.width / bgAspect;
		destX = 0;
		destY = (pixelScreenRect.height - destHeight) / 2.0f;
	} else {
		destHeight = pixelScreenRect.height;
		destWidth = pixelScreenRect.height * bgAspect;
		destY = 0;
		destX = (pixelScreenRect.width - destWidth) / 2.0f;
	}
	
	DrawTexturePro(_MenuBackground,
		Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
		Rectangle{ destX, destY, destWidth, destHeight },
		Vector2{ 0,0 }, 0.0f, WHITE);

	// Title
	float titleY = safeAreaPx.y + safeAreaPx.height * 0.1f;
	float titleFontSize = safeAreaPx.height * 0.06f;
	DrawText("OPTIONS", safeAreaPx.x + (safeAreaPx.width - MeasureText("OPTIONS", titleFontSize)) / 2.0f, titleY, titleFontSize, WHITE);

	// Options layout
	float buttonWidth = safeAreaPx.width * 0.8f;
	float buttonHeight = safeAreaPx.height * 0.08f;
	float buttonSpacing = safeAreaPx.height * 0.03f;
	float centerX = safeAreaPx.x + safeAreaPx.width / 2.0f;
	float firstButtonY = titleY + titleFontSize + safeAreaPx.height * 0.05f;

	// Audio options
	float currentY = firstButtonY;
	
	// Music Volume
	if (AIGUI_ButtonRounded("Music Volume", centerX - buttonWidth / 2, currentY, buttonWidth, buttonHeight, 0.1f, buttonHeight * 0.4f, WHITE)) {
		// Toggle music on/off for now (simplified)
		                AudioManager::GetInstance().ToggleMusicMute();
	}
	currentY += buttonHeight + buttonSpacing;

	// Sound Effects
	if (AIGUI_ButtonRounded("Sound Effects", centerX - buttonWidth / 2, currentY, buttonWidth, buttonHeight, 0.1f, buttonHeight * 0.4f, WHITE)) {
		// Toggle sound effects on/off for now (simplified)
		                AudioManager::GetInstance().ToggleSoundMute();
	}
	currentY += buttonHeight + buttonSpacing;

	// Difficulty
	const char* difficultyText = (difficultyIndex == 0) ? "Difficulty: RUNNY" : (difficultyIndex == 1) ? "Difficulty: REGULAR" : "Difficulty: ROUGH";
	if (AIGUI_ButtonRounded(difficultyText, centerX - buttonWidth / 2, currentY, buttonWidth, buttonHeight, 0.1f, buttonHeight * 0.4f, WHITE)) {
		difficultyIndex = (difficultyIndex + 1) % 3;
	}
	currentY += buttonHeight + buttonSpacing;

	// Quickplay Settings
	if (AIGUI_ButtonRounded("Quickplay Settings", centerX - buttonWidth / 2, currentY, buttonWidth, buttonHeight, 0.1f, buttonHeight * 0.4f, WHITE)) {
		currentMenu = QUICKPLAY_SETTINGS;
	}
	currentY += buttonHeight + buttonSpacing;

	// Back button
	if (AIGUI_ButtonRounded("Back", centerX - buttonWidth / 2, currentY, buttonWidth, buttonHeight, 0.1f, buttonHeight * 0.4f, WHITE)) {
		currentMenu = MAIN_MENU;
	}

	
}

void MainMenu::DrawMobileLevelSelect()
{
	// Use UIManager for consistent coordinate handling
	UIManager& uiManager = UIManager::GetInstance();
	Rectangle pixelScreenRect = {0, 0, (float)uiManager.GetScreenWidth(), (float)uiManager.GetScreenHeight()};
	Rectangle safeAreaPx = uiManager.GetSafeArea();
	
	// Draw background
	float bgAspect = (float)_MenuBackground.width / (float)_MenuBackground.height;
	float screenAspect = pixelScreenRect.width / pixelScreenRect.height;
	float destWidth, destHeight, destX, destY;
	if (screenAspect > bgAspect) {
		destWidth = pixelScreenRect.width;
		destHeight = pixelScreenRect.width / bgAspect;
		destX = 0;
		destY = (pixelScreenRect.height - destHeight) / 2.0f;
	} else {
		destHeight = pixelScreenRect.height;
		destWidth = pixelScreenRect.height * bgAspect;
		destY = 0;
		destX = (pixelScreenRect.width - destWidth) / 2.0f;
	}
	
	DrawTexturePro(_MenuBackground,
		Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
		Rectangle{ destX, destY, destWidth, destHeight },
		Vector2{ 0,0 }, 0.0f, WHITE);

	// Title
	float titleY = safeAreaPx.y + safeAreaPx.height * 0.05f;
	float titleFontSize = safeAreaPx.height * 0.06f;
	DrawText("LEVEL SELECT", safeAreaPx.x + (safeAreaPx.width - MeasureText("LEVEL SELECT", titleFontSize)) / 2.0f, titleY, titleFontSize, WHITE);

	// Handle touch input for swipe gestures
    if (IsPrimaryInputPressed()) {
        Vector2 touchPos = GetTouchPosition(0);
			if (!isDragging) {
				lastTouchX = touchPos.x;
				isDragging = true;
			} else {
				float deltaX = touchPos.x - lastTouchX;
				levelSelectScrollOffset += deltaX;
				lastTouchX = touchPos.x;
			}
    } else if (IsPrimaryInputReleased()) {
			isDragging = false;
			// Snap to nearest level
			float levelWidth = safeAreaPx.width * 0.8f;
			int targetLevel = (int)round(-levelSelectScrollOffset / levelWidth);
			targetLevel = std::max(0, std::min(5, targetLevel)); // Clamp to 0-5
			levelSelectScrollOffset = -targetLevel * levelWidth;
			currentLevelIndex = targetLevel;
	}

	// Level paintings layout
	float paintingWidth = safeAreaPx.width * 0.8f;
	float paintingHeight = safeAreaPx.height * 0.6f;
	float paintingSpacing = safeAreaPx.width * 0.1f;
	float centerX = safeAreaPx.x + safeAreaPx.width / 2.0f;
	float paintingY = titleY + titleFontSize + safeAreaPx.height * 0.05f;

	// Draw level paintings with scroll offset
	for (int i = 0; i < 6; ++i) {
		float paintingX = centerX - paintingWidth / 2.0f + i * (paintingWidth + paintingSpacing) + levelSelectScrollOffset;
		// Only draw if visible
		if (paintingX + paintingWidth > safeAreaPx.x && paintingX < safeAreaPx.x + safeAreaPx.width) {
			Texture2D& painting = levelsUnlocked[i] ? levelPaintings[i] : lockedPainting;
			// Check if this painting is clicked
			bool isClicked = false;
            if (IsPrimaryInputReleased() && !isDragging) {
                Vector2 touchPos = GetTouchPosition(0);
					Rectangle paintingRect = { paintingX, paintingY, paintingWidth, paintingHeight };
					if (CheckCollisionPointRec(touchPos, paintingRect)) {
						isClicked = true;
					}
				}
			// Draw painting
			DrawTexturePro(painting,
				Rectangle{ 0, 0, (float)painting.width, (float)painting.height },
				Rectangle{ paintingX, paintingY, paintingWidth, paintingHeight },
				Vector2{ 0,0 }, 0.0f, WHITE);
			// Handle painting click
			if (isClicked) {
				if (levelsUnlocked[i]) {
					// Start the level
					game->SetGameState(PLAYING);
					if (game->playing) game->playing->SetCurrentLevel(i);
				} else {
					// Try to purchase the level
					if (game && game->playing) {
						PurchaseLevel(i);
					}
				}
			}
		}
	}

	// Navigation arrows
	float arrowSize = safeAreaPx.height * 0.08f;
	float arrowY = paintingY + paintingHeight / 2.0f - arrowSize / 2.0f;
	// Left arrow
	float leftArrowX = safeAreaPx.x + safeAreaPx.width * 0.05f;
	if (AIGUI_ButtonRounded("<", leftArrowX, arrowY, arrowSize, arrowSize, 0.2f, arrowSize * 0.4f, WHITE)) {
		currentLevelIndex = std::max(0, currentLevelIndex - 1);
		levelSelectScrollOffset = -currentLevelIndex * (paintingWidth + paintingSpacing);
	}
	// Right arrow
	float rightArrowX = safeAreaPx.x + safeAreaPx.width * 0.95f - arrowSize;
	if (AIGUI_ButtonRounded(">", rightArrowX, arrowY, arrowSize, arrowSize, 0.2f, arrowSize * 0.4f, WHITE)) {
		currentLevelIndex = std::min(5, currentLevelIndex + 1);
		levelSelectScrollOffset = -currentLevelIndex * (paintingWidth + paintingSpacing);
	}
	// Back button
	float backButtonWidth = safeAreaPx.width * 0.3f;
	float backButtonHeight = safeAreaPx.height * 0.08f;
	float backButtonY = paintingY + paintingHeight + safeAreaPx.height * 0.05f;
	if (AIGUI_ButtonRounded("Back", centerX - backButtonWidth / 2, backButtonY, backButtonWidth, backButtonHeight, 0.1f, backButtonHeight * 0.4f, WHITE)) {
		currentMenu = MAIN_MENU;
	}
}

void MainMenu::ResetMusic()
{
	// AudioStateManager handles music transitions
	AudioStateManager::GetInstance().TransitionToState(AudioStateManager::AUDIO_MAIN_MENU, false, 0.5f);
}

void MainMenu::ToggleFartMusic()
{
	GameLog::Log("[MAINMENU] ToggleFartMusic: Switching music mode");
	
	// Update the AudioStateManager's fart mode
	AudioStateManager::GetInstance().SetFartMode(!fartModeEnabled);
	
	// Update local state
	fartModeEnabled = !fartModeEnabled;
	
	GameLog::Log("[MAINMENU] ToggleFartMusic: Fart mode %s", fartModeEnabled ? "ENABLED" : "DISABLED");
}

void MainMenu::HandleInput()
{
	if (IsKeyPressed(KEY_E))
	{
		if (game) {
			game->SetGameState(PLAYING);
			if (game->playing) game->playing->SetCurrentLevel(quickplaySettings.levelIndex);
		}
	}
}