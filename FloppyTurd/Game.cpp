#include "Game.h"
#include "AIGUI.h"
#include "AudioManager.h"

Game::Game()
{
	SetTargetFPS(60);
	InitAudioDevice();

	InitClasses();
	// Load the font and validate it
	whackyJoe = LoadFont("resources/fonts/Whacky_Joe.fnt");

	// Validate the font
	if (whackyJoe.baseSize <= 0 || whackyJoe.glyphCount <= 0 || whackyJoe.texture.id == 0) {
		printf("Error: Failed to load font 'resources/fonts/whackyJoe.fnt'\n");
		printf("Falling back to default font.\n");
		whackyJoe = GetFontDefault(); // Fallback to default font
	}
	else {
		printf("Font 'whackyJoe' loaded successfully: baseSize=%d, glyphCount=%d, textureID=%u\n",
			whackyJoe.baseSize, whackyJoe.glyphCount, whackyJoe.texture.id);
		// Use bilinear filtering instead of trilinear to avoid mipmap warnings
		SetTextureFilter(whackyJoe.texture, TEXTURE_FILTER_BILINEAR);
	}

	AIGUI_Init();
	AIGUI_SetFont(whackyJoe);
	SetTextureFilter(whackyJoe.texture, TEXTURE_FILTER_POINT); // Default for UI, override for HD
	gamestate = LOADING; // Start with loading state
	RunGame();
}

Game::~Game()
{
	CloseAudioDevice();
	AIGUI_Shutdown();

	// Unload the custom font if it was loaded (not the default font)
	if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0 && g_AIGUI.defaultFont.texture.id != 0) {
		// Check if it's not the default font (default font is managed by raylib)
		Font defaultFont = GetFontDefault();
		if (g_AIGUI.defaultFont.texture.id != defaultFont.texture.id) {
			printf("Unloading custom font (textureID=%u)\n", g_AIGUI.defaultFont.texture.id);
			UnloadFont(g_AIGUI.defaultFont);
		}
	}
	delete credits; // Clean up Credits instance
	delete loading; // Clean up Loading instance
}

void Game::InitClasses()
{
	using namespace GameSettings;
	window = new Window(GameWidth, GameHeight);

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

	while (!WindowShouldClose())
	{
		if (gamestate == SHUTDOWN)
		{
			CloseWindow();
			exit(0);
		}

		// -------------------------------------------------------------------------
		// 1) Scale the raw mouse (1280x720) down into a 320x180 coordinate system.
		// -------------------------------------------------------------------------
		float scaleX = (float)GetScreenWidth() / 320.0f;
		float scaleY = (float)GetScreenHeight() / 180.0f;

		Vector2 rawMouse = GetMousePosition(); // e.g. in 1280x720
		g_AIGUI.mousePos.x = rawMouse.x / scaleX; // now in 0..320
		g_AIGUI.mousePos.y = rawMouse.y / scaleY; // now in 0..180

		AIGUI_BeginFrame(); // any custom UI BeginFrame logic you have

		// -------------------------
		// 2) Update game logic
		// -------------------------
		Update();

		// ------------------------------------------------------
		// 3) Render to the 320x180 "virtual" RenderTexture
		// ------------------------------------------------------
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

		// ---------------------------------------------------------------------
		// 4) Draw the 320x180 result to the actual window (e.g. 1280x720)
		// ---------------------------------------------------------------------
		BeginDrawing();
		ClearBackground(BLACK);

		// Fill the window with the scaled 320x180 result
		DrawTexturePro(
			target.texture,
			// Source rect note -height if your textures appear upside-down:
			Rectangle{ 0, 0, (float)target.texture.width, (float)-target.texture.height },
			// Dest rect entire window
			Rectangle{ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() },
			Vector2{ 0, 0 },
			0.0f,
			WHITE
		);

		EndDrawing();

		HandleInput();
		AIGUI_EndFrame(); // your custom UI end logic
	}

	// Clean up
	UnloadRenderTexture(target);
	delete window; // whatever else you're cleaning
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