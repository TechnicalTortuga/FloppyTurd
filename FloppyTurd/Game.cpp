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
    }

    AIGUI_Init();
    AIGUI_SetFont(whackyJoe);
    SetTextureFilter(whackyJoe.texture, TEXTURE_FILTER_POINT);
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
}

void Game::InitClasses()
{
	using namespace GameSettings;
	window = new Window(GameWidth, GameHeight);

	mainMenu = new MainMenu(this);
	playing = new Playing(this);

	gamestate = MAINMENU;
}

void Game::RunGame()
{
    // Create a render texture for 320 180 virtual resolution
    RenderTexture2D target = LoadRenderTexture(320, 180);

    // Optionally clamp wrapping on any textures that might tile if scrolled too far
    // e.g. if you have a skills texture: SetTextureWrap(_TurdPointMenu, TEXTURE_WRAP_CLAMP);

    while (!WindowShouldClose())
    {
        if (gamestate == SHUTDOWN)
        {
            CloseWindow();
            exit(0);
        }

        // -------------------------------------------------------------------------
        // 1) Scale the *raw* mouse (1280 720) down into a 320 180 coordinate system.
        // -------------------------------------------------------------------------
        float scaleX = (float)GetScreenWidth() / 320.0f;
        float scaleY = (float)GetScreenHeight() / 180.0f;

        Vector2 rawMouse = GetMousePosition(); // e.g. in 1280 720
        g_AIGUI.mousePos.x = rawMouse.x / scaleX; // now in 0..320
        g_AIGUI.mousePos.y = rawMouse.y / scaleY; // now in 0..180

        // If you use GetMouseDelta(), remember to scale that similarly, or you ll
        // see scrolling/dragging jumps. For example:
        // Vector2 rawDelta = GetMouseDelta();
        // Vector2 scaledDelta = { rawDelta.x / scaleX, rawDelta.y / scaleY };
        // (Use scaledDelta whenever you do drag-based scroll.)

        AIGUI_BeginFrame(); // any custom UI BeginFrame logic you have

        // -------------------------
        // 2) Update game logic
        // -------------------------
        Update();

        // ------------------------------------------------------
        // 3) Render to the 320 180 "virtual" RenderTexture
        // ------------------------------------------------------
        BeginTextureMode(target);
        ClearBackground(BLACK);

        switch (gamestate)
        {
        case MAINMENU:
            mainMenu->Draw();  // all drawing in 320 180 coords
            break;
        case PLAYING:
            playing->Draw();   // includes your pause logic if needed
            break;
        case PAUSEMENU:
            // If you have a separate pauseMenu->Draw(), do it here.
            // Or if your PAUSEMENU UI is inside playing->Draw() behind an if() check,
            // thats fine too.
            break;
        }

        EndTextureMode(); // Done rendering the 320 180 scene

        // ---------------------------------------------------------------------
        // 4) Draw the 320180 result to the *actual window* (e.g. 1280 720)
        // ---------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(BLACK);

        // Fill the window with the scaled 320 180 result
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
        // If you do input reading here, remember its in the raw 1280 720 coords
        // unless you re-scale

        AIGUI_EndFrame(); // your custom UI end logic
    }

    // Clean up
    UnloadRenderTexture(target);
    delete window; // whatever else youre cleaning
}

void Game::Update()
{
	switch (gamestate)
	{
	case MAINMENU:
		mainMenu->Update();
		break;

	case PLAYING:
		playing->Update();
		break;
	case SHUTDOWN:
		CloseWindow();
		break;
		//std::cout << "Default Gamestate Update" << std::endl;
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
		//std::cout << "Default Gamestate Draw" << std::endl;
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
