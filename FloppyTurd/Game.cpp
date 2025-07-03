#include "Game.h"
#include "AIGUI.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "ResourceCompat.h"

// Enable draw call tracking
#define ENABLE_DRAW_CALL_TRACKING
#include "TextureAtlas.h"
#include "PerformanceProfiler.h"

Game::Game()
{
	// Initialize platform layer first for mobile detection
	PlatformLayer::GetInstance().Initialize();
	
#ifdef PLATFORM_MOBILE
	// Mobile-specific performance settings
	SetTargetFPS(60);  // Consistent 60fps on mobile
	SetConfigFlags(FLAG_VSYNC_HINT);
#else
	// Desktop can handle variable frame rates
	SetTargetFPS(60);
#endif

	InitAudioDevice();

	// Initialize ResourceManager with mobile-aware quality detection
	ResourceManager::GetInstance().Initialize(ResourceQuality::AUTO);

	InitClasses();
	
	// Load the font with mobile-aware path resolution
	std::string fontPath = PlatformLayer::GetInstance().GetResourcePath("fonts/Whacky_Joe.fnt");
	whackyJoe = LoadFont(fontPath.c_str());

	// Validate the font
	if (whackyJoe.baseSize <= 0 || whackyJoe.glyphCount <= 0 || 
#if defined(__APPLE__) && TARGET_OS_IPHONE
		whackyJoe.texture.texture == nullptr
#else
		whackyJoe.texture.id == 0
#endif
	) {
		printf("Failed to load Whacky Joe font, falling back to default\n");
		whackyJoe = GetFontDefault();
	} else {
		printf("Whacky Joe font loaded successfully: baseSize=%d, glyphCount=%d, texture=%p\n",
			whackyJoe.baseSize, whackyJoe.glyphCount, 
#if defined(__APPLE__) && TARGET_OS_IPHONE
			(void*)whackyJoe.texture.texture
#else
			(void*)whackyJoe.texture.id
#endif
		);
	}

	// Initialize AIGUI with mobile awareness
	AIGUI_Init();
	AIGUI_SetFont(whackyJoe);
	
	// Platform-specific UI scaling adjustments
#ifdef PLATFORM_MOBILE
	// Let AIGUI auto-detect and set appropriate scaling
	TraceLog(LOG_INFO, "Mobile platform: AIGUI scaling set to %.2f", AIGUI_GetUIScale());
#endif

	// Initialize Phase 4 systems with real draw call tracking
	TraceLog(LOG_INFO, "Game: Initializing Phase 4 systems with macro-based draw call tracking...");
	
	// Initialize TextureAtlas system
	TextureAtlas::GetInstance().Initialize();
	TraceLog(LOG_INFO, "TextureAtlas: Initialized successfully");

	// --- Build Atlases for all categories using ResourceManager ---
	ResourceManager& rm = ResourceManager::GetInstance();
	std::unordered_map<AtlasCategory, std::vector<std::string>> atlasCategoryToPaths;

	// Map resource IDs to atlas categories (customize as needed)
	auto categorize = [](const std::string& path) -> AtlasCategory {
		if (path.find("turd/") != std::string::npos || path.find("hats/") != std::string::npos)
			return AtlasCategory::PLAYER_SPRITES;
		if (path.find("enemies/") != std::string::npos)
			return AtlasCategory::ENEMY_SPRITES;
		if (path.find("ui/") != std::string::npos || path.find("mainmenu/") != std::string::npos)
			return AtlasCategory::UI_ELEMENTS;
		if (path.find("environment/") != std::string::npos || path.find("objects/") != std::string::npos)
			return AtlasCategory::ENVIRONMENT;
		if (path.find("vfx/") != std::string::npos)
			return AtlasCategory::PARTICLES;
		return AtlasCategory::UI_ELEMENTS; // fallback
	};

	for (const auto& [id, info] : rm.GetResourceRegistry()) {
		if (info.type == ResourceType::TEXTURE) {
			AtlasCategory cat = categorize(info.relativePath);
			atlasCategoryToPaths[cat].push_back(info.relativePath);
		}
	}

	for (const auto& [cat, paths] : atlasCategoryToPaths) {
		TextureAtlas::GetInstance().BuildAtlas(cat, paths);
	}

	// Initialize PerformanceProfiler with draw call tracking enabled
	PerformanceProfiler::GetInstance().SetTargetFPS(60);
	PerformanceProfiler::GetInstance().EnableOverlay(true);
	TraceLog(LOG_INFO, "PerformanceProfiler: Initialized with 60 FPS target and real draw call tracking enabled");

	gamestate = LOADING; // Start with loading state
	RunGame();
}

Game::~Game()
{
	// Shutdown Phase 4 systems
	TextureAtlas::GetInstance().Shutdown();
	TraceLog(LOG_INFO, "Phase 4 systems shutdown complete");
	
	// Shutdown ResourceManager before closing audio
	ResourceManager::GetInstance().Shutdown();
	
	CloseAudioDevice();
	AIGUI_Shutdown();

	// Unload the custom font if it was loaded (not the default font)
	if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0 && 
#if defined(__APPLE__) && TARGET_OS_IPHONE
		g_AIGUI.defaultFont.texture.texture != nullptr
#else
		g_AIGUI.defaultFont.texture.id != 0
#endif
	) {
		printf("AIGUI default font is valid, checking if it's different from system default\n");
		Font defaultFont = GetFontDefault();
		if (
#if defined(__APPLE__) && TARGET_OS_IPHONE
			g_AIGUI.defaultFont.texture.texture != defaultFont.texture.texture
#else
			g_AIGUI.defaultFont.texture.id != defaultFont.texture.id
#endif
		) {
			printf("Unloading custom font (textureID=%p)\n", 
#if defined(__APPLE__) && TARGET_OS_IPHONE
				g_AIGUI.defaultFont.texture.texture
#else
				(void*)g_AIGUI.defaultFont.texture.id
#endif
			);
			UnloadFont(g_AIGUI.defaultFont);
		}
	}
	delete credits; // Clean up Credits instance
	delete loading; // Clean up Loading instance
}

void Game::InitClasses()
{
	using namespace GameSettings;
	
#ifdef PLATFORM_MOBILE
	// On mobile, always use fullscreen and let the platform handle the display
	window = new Window(true, 0, 0);
	
	// Set preferred orientation for the game (landscape for this game)
	window->SetPreferredOrientation(true);  // true = landscape
	
	TraceLog(LOG_INFO, "Mobile window initialized: %dx%d, Safe area: %.0fx%.0f",
		GetScreenWidth(), GetScreenHeight(),
		window->GetSafeArea().width, window->GetSafeArea().height);
#else
	// Desktop: Use native monitor resolution, start in fullscreen
	window = new Window(true, 0, 0);
#endif

	// Load and set the window icon with platform-aware path
	std::string iconPath = PlatformLayer::GetInstance().GetResourcePath("poophat.ico");
	Image icon = LoadImage(iconPath.c_str());
	if (
#if defined(__APPLE__) && TARGET_OS_IPHONE
		icon.surface
#else
		icon.data
#endif
	) {
		SetWindowIcon(icon);
		UnloadImage(icon);
		printf("Poophat icon set successfully from %s\n", iconPath.c_str());
	}
	else {
		printf("Error: Failed to load poophat icon from %s\n", iconPath.c_str());
	}
	
#ifndef PLATFORM_MOBILE
	// Focus only makes sense on desktop
	SetWindowFocused();
#endif

	mainMenu = nullptr; // Initialized in Loading state
	playing = new Playing(this);
	credits = new Credits(this); // Pass Game pointer directly
	loading = new Loading(this); // New loading instance

	gamestate = LOADING; // Ensure initial state is loading
}

void Game::RunGame()
{
	// Create a render texture for 320x180 virtual resolution
	RenderTexture2D target = LoadRenderTexture(320, 180);
	
	// Game's native resolution
	const float GAME_WIDTH = 320.0f;
	const float GAME_HEIGHT = 180.0f;
	const float GAME_ASPECT = GAME_WIDTH / GAME_HEIGHT; // 16:9

	while (!WindowShouldClose())
	{
		
		if (gamestate == SHUTDOWN)
		{
			CloseWindow();
			exit(0);
		}

		// -------------------------------------------------------------------------
		// 1) Calculate proper letterboxing with aspect ratio preservation
		// -------------------------------------------------------------------------
		float screenWidth = (float)GetScreenWidth();
		float screenHeight = (float)GetScreenHeight();
		
		// Get the REAL monitor resolution to fix macOS fullscreen discrepancies
		// This needs to be checked every frame in case the user switches monitors or resolutions
		int monitor = GetCurrentMonitor();
		int realMonitorWidth = GetMonitorWidth(monitor);
		int realMonitorHeight = GetMonitorHeight(monitor);
		
		// Check if there's a discrepancy between reported and real resolution
		bool hasDiscrepancy = (realMonitorWidth != (int)screenWidth || realMonitorHeight != (int)screenHeight);
		
		// Use the appropriate resolution for calculations
		float effectiveWidth, effectiveHeight;
		if (hasDiscrepancy && IsWindowFullscreen()) {
			// In fullscreen with discrepancy, use real monitor resolution
			effectiveWidth = (float)realMonitorWidth;
			effectiveHeight = (float)realMonitorHeight;
		} else {
			// In windowed mode or when no discrepancy, use reported resolution
			effectiveWidth = screenWidth;
			effectiveHeight = screenHeight;
		}
		
		// Declare variables
		float scale, offsetX = 0, offsetY = 0;
		
		// Calculate the scale that fits the game while maintaining aspect ratio
		float scaleX = effectiveWidth / GAME_WIDTH;
		float scaleY = effectiveHeight / GAME_HEIGHT;
		scale = (scaleX < scaleY) ? scaleX : scaleY; // Use the smaller scale to ensure it fits
		
		// Calculate the actual rendered size (this should be smaller than or equal to effective size)
		float renderedWidth = GAME_WIDTH * scale;
		float renderedHeight = GAME_HEIGHT * scale;
		
		// Center the game area with proper rounding to avoid fractional pixels
		offsetX = floorf((effectiveWidth - renderedWidth) / 2.0f);
		offsetY = floorf((effectiveHeight - renderedHeight) / 2.0f);
		
		// macOS fullscreen quirk detection and compensation
		#ifdef __APPLE__
		// On macOS, fullscreen mode sometimes reports incorrect screen dimensions
		// that can cause asymmetrical letterboxing. Detect and compensate for this.
		float unusedSpaceX = effectiveWidth - renderedWidth;
		float unusedSpaceY = effectiveHeight - renderedHeight;
		
		// If there's minimal unused space that should result in perfect centering,
		// but we detect potential asymmetry, force perfect centering
		if (unusedSpaceX < 2.0f && unusedSpaceY < 2.0f) {
			// Perfect fit - ensure exact centering
			offsetX = unusedSpaceX / 2.0f;
			offsetY = unusedSpaceY / 2.0f;
		}
		else if (unusedSpaceY > 0.0f && unusedSpaceY < 50.0f) {
			// Small amount of letterboxing that should be symmetric
			// Force perfect vertical centering
			offsetY = unusedSpaceY / 2.0f;
		}
		#endif
		
		// Debug output for first few frames
		static int frameCount = 0;
		if (frameCount < 10) {  // Show more frames to catch any changes
			// Get actual monitor information
			printf("Frame %d: Effective=%.0fx%.0f, Scale=%.3f, Rendered=%.0fx%.0f, Offset=(%.1f,%.1f)\n", 
				frameCount, effectiveWidth, effectiveHeight, scale, renderedWidth, renderedHeight, offsetX, offsetY);
			printf("  ScaleX=%.6f, ScaleY=%.6f, Diff=%.6f\n", scaleX, scaleY, fabs(scaleX - scaleY));
			printf("  Expected for 16:9: %.0fx%.0f\n", effectiveHeight * GAME_ASPECT, effectiveHeight);
			printf("  Unused space: X=%.1f, Y=%.1f\n", effectiveWidth - renderedWidth, effectiveHeight - renderedHeight);
			printf("  MONITOR: Real=%dx%d, Position=(%.0f,%.0f), Reported=%.0fx%.0f\n", 
				realMonitorWidth, realMonitorHeight, GetMonitorPosition(monitor).x, GetMonitorPosition(monitor).y, 
				screenWidth, screenHeight);
			printf("  DISCREPANCY: Width=%d, Height=%d, Fullscreen=%s, Using=%s\n", 
				realMonitorWidth - (int)screenWidth, realMonitorHeight - (int)screenHeight,
				IsWindowFullscreen() ? "YES" : "NO",
				(hasDiscrepancy && IsWindowFullscreen()) ? "REAL" : "REPORTED");
			frameCount++;
		}
		
		// -------------------------------------------------------------------------
		// 2) Proper mouse coordinate mapping with letterbox offset
		// -------------------------------------------------------------------------
		Vector2 rawMouse = GetMousePosition();
		
		// Account for letterbox offsets
		float adjustedMouseX = rawMouse.x - offsetX;
		float adjustedMouseY = rawMouse.y - offsetY;
		
		// Convert to game coordinates
		g_AIGUI.mousePos.x = adjustedMouseX / scale;
		g_AIGUI.mousePos.y = adjustedMouseY / scale;
		
		// Clamp to game bounds
		if (g_AIGUI.mousePos.x < 0) g_AIGUI.mousePos.x = 0;
		if (g_AIGUI.mousePos.x > GAME_WIDTH) g_AIGUI.mousePos.x = GAME_WIDTH;
		if (g_AIGUI.mousePos.y < 0) g_AIGUI.mousePos.y = 0;
		if (g_AIGUI.mousePos.y > GAME_HEIGHT) g_AIGUI.mousePos.y = GAME_HEIGHT;

		// -------------------------------------------------------------------------
		// 3) Update game logic and performance profiling
		// -------------------------------------------------------------------------
		PerformanceProfiler::GetInstance().BeginFrame();
		Update();
		PerformanceProfiler::GetInstance().EndFrame();

		// -------------------------------------------------------------------------
		// 4) Render to 320x180 texture
		// -------------------------------------------------------------------------
		BeginTextureMode(target);
		ClearBackground(BLACK);
		
		switch (gamestate)
		{
		case MAINMENU:
			mainMenu->Draw();  // all drawing in 320x180 coords
			break;
		case PLAYING:
			playing->Draw();   // includes your pause logic if needed
			break;
		case CREDITS:
			credits->Draw();   // Draw credits screen
			break;
		case PAUSEMENU:
			// If you have a separate pauseMenu->Draw(), do it here.
			// Or if your PAUSEMENU UI is inside playing->Draw() behind an if() check,
			// that's fine too.
			break;
		case LOADING:
			loading->Draw(); // Draw loading screen with rotating poophat
			break;
		}

		EndTextureMode(); // Done rendering the 320x180 scene

		// -------------------------------------------------------------------------
		// 5) Draw texture to screen with proper letterboxing
		// -------------------------------------------------------------------------
		BeginDrawing();
		ClearBackground(BLACK); // This creates the letterbox bars
		
		// Calculate destination rectangle for the game area
		Rectangle destRect = {
			offsetX,
			offsetY,
			renderedWidth,
			renderedHeight
		};
		
		// Only draw the game texture to the calculated rectangle
		// The areas outside this rectangle will remain black (letterbox bars)
		DrawCallTracker::TrackDrawTexturePro(
			target.texture,
			// Source rect - use negative height for proper RenderTexture orientation
			Rectangle{ 0, 0, GAME_WIDTH, -GAME_HEIGHT },
			// Destination rect - this should NOT fill entire screen when letterboxing
			destRect,
			Vector2{ 0, 0 },
			0.0f,
			WHITE
		);
		
		EndDrawing();

		HandleInput();
	}

	UnloadRenderTexture(target);
}

void Game::Update()
{
	static GAMESTATE prevState = SHUTDOWN; // Track previous state for reset
	if (prevState != gamestate)
	{
		if (gamestate == CREDITS && credits)
		{
			credits->Reset(); // Reset Credits when entering CREDITS state
		}
		prevState = gamestate;
	}

	switch (gamestate)
	{
	case MAINMENU:
		mainMenu->Update();
		break;

	case PLAYING:
		playing->Update();
		break;

	case CREDITS:
		credits->Update(GetFrameTime());
		if (credits->IsComplete()) // Check if credits have finished scrolling
		{
			AudioManager::GetInstance().StopMusic();
			if (credits->GetMusic()) credits->GetMusic()->Stop();
			SetGameState(MAINMENU);
		}
		break;

	case LOADING:
		loading->Update();
		if (loading->IsComplete()) {
			SetGameState(MAINMENU); // Transition to MainMenu after one rotation
		}
		break;

	case SHUTDOWN:
		CloseWindow();
		break;
	}
}

void Game::Draw()
{
	BeginDrawing();
	ClearBackground(BLACK);

	switch (gamestate)
	{
	case MAINMENU:
		mainMenu->Draw();
		break;

	case PLAYING:
		playing->Draw();
		break;

	case CREDITS:
		credits->Draw(); // Additional draw pass if needed (optional)
		break;
	}

	EndDrawing();
}

void Game::HandleInput()
{
	if (IsKeyPressed(KEY_F11)) window->ToggleMode();

	switch (gamestate)
	{
	case MAINMENU:
		mainMenu->HandleInput();
		break;

	case PLAYING:
		playing->HandleInput();
		break;

	case CREDITS:
		credits->HandleInput();
		break;

	case PAUSEMENU:
		//pauseMenu->HandleInput();
		break;
	}
}

void Game::SetGameState(GAMESTATE newState)
{
	std::cout << "Switching to " << newState << std::endl;
	gamestate = newState;
}

// Implement GetScaledFont method
Font Game::GetScaledFont(float scaleFactor) {
	Font scaledFont = whackyJoe;
	scaledFont.baseSize = (int)(whackyJoe.baseSize * scaleFactor);
	SetTextureFilter(scaledFont.texture, TEXTURE_FILTER_BILINEAR); // Use bilinear for compatibility
	return scaledFont;
}