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
	std::cout << "[INIT] ========================================" << std::endl;
	std::cout << "[INIT] Game constructor STARTING" << std::endl;
	std::cout << "[INIT] ========================================" << std::endl;
	
	// Constructor only sets initial state, doesn't start the game
	initialized.store(false);
	std::cout << "[DEBUG] Set initialized = false in constructor" << std::endl;
	window = nullptr;
	mainMenu = nullptr;
	playing = nullptr;
	credits = nullptr;
	loading = nullptr;
	gamestate = LOADING;
	
	// Initialize letterbox state
	gameScale = 1.0f;
	gameOffsetX = 0.0f;
	gameOffsetY = 0.0f;
	renderedWidth = 320.0f;
	renderedHeight = 180.0f;
	
	std::cout << "[INIT] Game constructor COMPLETED" << std::endl;
	std::cout << "[INIT] initialized=" << initialized << ", gamestate=LOADING" << std::endl;
	std::cout << "[INIT] ========================================" << std::endl;
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
icon.data
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

bool Game::Initialize()
{
	std::cout << "[INIT] ========================================" << std::endl;
	std::cout << "[INIT] Game::Initialize() STARTING" << std::endl;
	std::cout << "[INIT] ========================================" << std::endl;
	
	if (initialized) {
		std::cout << "[INIT] Game already initialized, returning early" << std::endl;
		return true;
	}
	
	try {
		// Basic initialization
		gamestate = LOADING;
		initialized.store(false);
		std::cout << "[DEBUG] Set initialized = false in Initialize() method" << std::endl;
		
		std::cout << "[DEBUG] Set gamestate to LOADING" << std::endl;
		
		// Initialize ResourceManager
		ResourceManager::GetInstance().Initialize(ResourceQuality::HIGH);
		std::cout << "[DEBUG] Initialized ResourceManager" << std::endl;
		
		// Initialize render target
		renderTarget = LoadRenderTexture(320, 180);
		std::cout << "[DEBUG] Created render target 320x180" << std::endl;
		
		// Initialize the window system
		window = new Window();
		std::cout << "[DEBUG] Created Window instance" << std::endl;
		
		// Initialize game states
		loading = new Loading(this);
		std::cout << "[DEBUG] Created Loading state" << std::endl;
		
		mainMenu = new MainMenu(this);
		std::cout << "[DEBUG] Created MainMenu state" << std::endl;
		
		playing = new Playing(this);
		std::cout << "[DEBUG] Created Playing state" << std::endl;
		
		credits = new Credits(this);
		std::cout << "[DEBUG] Created Credits state" << std::endl;
		
		// Initialize letterbox state
		gameScale = 1.0f;
		gameOffsetX = 0.0f;
		gameOffsetY = 0.0f;
		renderedWidth = 320.0f;
		renderedHeight = 180.0f;
		
		std::cout << "[DEBUG] Initialized letterbox state" << std::endl;
		
		// Set state to loading
		SetGameState(LOADING);
		std::cout << "[DEBUG] Set game state to LOADING" << std::endl;
		
		initialized.store(true);
		std::cout << "[INIT] ========================================" << std::endl;
		std::cout << "[INIT] Game::Initialize() COMPLETED SUCCESSFULLY" << std::endl;
		std::cout << "[INIT] initialized=" << initialized.load() << std::endl;
		std::cout << "[INIT] initialized variable address: " << &initialized << std::endl;
		std::cout << "[INIT] ========================================" << std::endl;
		return true;
		
	} catch (const std::exception& e) {
		std::cout << "[ERROR] Exception in Game::Initialize(): " << e.what() << std::endl;
		return false;
	} catch (...) {
		std::cout << "[ERROR] Unknown exception in Game::Initialize()" << std::endl;
		return false;
	}
}

void Game::RunGame()
{
	// Deprecated - use RunGameDesktop() for desktop platforms
	// or UpdateFrame()/RenderFrame() for frame-by-frame execution on iOS
	RunGameDesktop();
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
	
	case PAUSEMENU:
		// Pause menu drawing handled by playing state
		break;
		
	case LOADING:
		loading->Draw();
		break;
		
	case SHUTDOWN:
		// Nothing to draw during shutdown
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
		
	case LOADING:
		// Loading screen doesn't need input handling
		break;
		
	case SHUTDOWN:
		// No input handling during shutdown
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

void Game::UpdateFrame(float deltaTime)
{
	// Handle shutdown state
	if (gamestate == SHUTDOWN)
	{
		return; // Will be handled by platform-specific code
	}

	// Store deltaTime for potential future use (currently individual states get their own timing)
	(void)deltaTime; // Suppress unused parameter warning

	// -------------------------------------------------------------------------
	// 1) Calculate proper letterboxing with aspect ratio preservation
	// -------------------------------------------------------------------------
	const float GAME_WIDTH = 320.0f;
	const float GAME_HEIGHT = 180.0f;
	const float GAME_ASPECT = GAME_WIDTH / GAME_HEIGHT; // 16:9
	
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
	
	// Calculate the scale that fits the game while maintaining aspect ratio
	float scaleX = effectiveWidth / GAME_WIDTH;
	float scaleY = effectiveHeight / GAME_HEIGHT;
	gameScale = (scaleX < scaleY) ? scaleX : scaleY; // Use the smaller scale to ensure it fits
	
	// Calculate the actual rendered size (this should be smaller than or equal to effective size)
	renderedWidth = GAME_WIDTH * gameScale;
	renderedHeight = GAME_HEIGHT * gameScale;
	
	// Center the game area with proper rounding to avoid fractional pixels
	gameOffsetX = floorf((effectiveWidth - renderedWidth) / 2.0f);
	gameOffsetY = floorf((effectiveHeight - renderedHeight) / 2.0f);
	
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
		gameOffsetX = unusedSpaceX / 2.0f;
		gameOffsetY = unusedSpaceY / 2.0f;
	}
	else if (unusedSpaceY > 0.0f && unusedSpaceY < 50.0f) {
		// Small amount of letterboxing that should be symmetric
		// Force perfect vertical centering
		gameOffsetY = unusedSpaceY / 2.0f;
	}
	#endif
	
	// Debug output for first few frames
	static int frameCount = 0;
	if (frameCount < 10) {  // Show more frames to catch any changes
		// Get actual monitor information
		printf("Frame %d: Effective=%.0fx%.0f, Scale=%.3f, Rendered=%.0fx%.0f, Offset=(%.1f,%.1f)\n", 
			frameCount, effectiveWidth, effectiveHeight, gameScale, renderedWidth, renderedHeight, gameOffsetX, gameOffsetY);
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
	float adjustedMouseX = rawMouse.x - gameOffsetX;
	float adjustedMouseY = rawMouse.y - gameOffsetY;
	
	// Convert to game coordinates
	g_AIGUI.mousePos.x = adjustedMouseX / gameScale;
	g_AIGUI.mousePos.y = adjustedMouseY / gameScale;
	
	// Clamp to game bounds
	if (g_AIGUI.mousePos.x < 0) g_AIGUI.mousePos.x = 0;
	if (g_AIGUI.mousePos.x > GAME_WIDTH) g_AIGUI.mousePos.x = GAME_WIDTH;
	if (g_AIGUI.mousePos.y < 0) g_AIGUI.mousePos.y = 0;
	if (g_AIGUI.mousePos.y > GAME_HEIGHT) g_AIGUI.mousePos.y = GAME_HEIGHT;

	// -------------------------------------------------------------------------
	// 3) Update game logic
	// -------------------------------------------------------------------------
	PerformanceProfiler::GetInstance().BeginFrame();
	Update();
	PerformanceProfiler::GetInstance().EndFrame();
	
	// Handle input after update
	HandleInput();
}

void Game::RenderFrame()
{
	const float GAME_WIDTH = 320.0f;
	const float GAME_HEIGHT = 180.0f;
	
	// -------------------------------------------------------------------------
	// 4) Render to 320x180 texture
	// -------------------------------------------------------------------------
	BeginTextureMode(renderTarget);
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
		// Pause menu rendering is typically handled by the playing state
		if (playing) {
			playing->Draw(); // Let playing state handle pause menu rendering
		}
		break;
	case LOADING:
		loading->Draw(); // Draw loading screen with rotating poophat
		break;
	case SHUTDOWN:
		// Nothing to draw during shutdown
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
		gameOffsetX,
		gameOffsetY,
		renderedWidth,
		renderedHeight
	};
	
	// Only draw the game texture to the calculated rectangle
	// The areas outside this rectangle will remain black (letterbox bars)
	DrawCallTracker::TrackDrawTexturePro(
		renderTarget.texture,
		// Source rect - use negative height for proper RenderTexture orientation
		Rectangle{ 0, 0, GAME_WIDTH, -GAME_HEIGHT },
		// Destination rect - this should NOT fill entire screen when letterboxing
		destRect,
		Vector2{ 0, 0 },
		0.0f,
		WHITE
	);
	
	EndDrawing();
}

void Game::RunGameDesktop()
{
	Initialize(); // One-time initialization
	
	// Traditional desktop game loop using Raylib
	while (!WindowShouldClose())
	{
		if (gamestate == SHUTDOWN)
		{
			break;
		}
		
		float deltaTime = GetFrameTime();
		UpdateFrame(deltaTime); // Core game logic (platform-agnostic)
		RenderFrame();          // Core rendering logic (platform-agnostic)
	}
	
	Shutdown(); // Cleanup
}

void Game::Shutdown()
{
	// Unload render texture
	if (initialized && renderTarget.id > 0)
	{
		UnloadRenderTexture(renderTarget);
	}
	
	// Cleanup will be handled by destructor
	initialized.store(false);
	std::cout << "[DEBUG] Set initialized = false in Shutdown() method" << std::endl;
}

void Game::OnPause()
{
	// Stop music when pausing (we'll restart it on resume if needed)
	AudioManager::GetInstance().StopMusic();
	
	// TODO: Add any other pause logic
}

void Game::OnResume()
{
	// Note: Music will be restarted by the appropriate game state when needed
	// (each state manages its own music)
	
	// TODO: Add any other resume logic
}