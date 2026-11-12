#include "Game.h"
#include "AIGUI.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "ResourceCompat.h"

// Enable draw call tracking
#define ENABLE_DRAW_CALL_TRACKING
#include "TextureAtlas.h"
#include "PerformanceProfiler.h"
#include "Viewport.h"
#include "TextureCache.h"
#include "PixelFont.h"
#include "EmbedInput.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

	window = new Window(true, 1280, 720);
	InitAudioDevice();
#ifdef __EMSCRIPTEN__
    // GLFW reports canvas focus; real DOM controls must not pause/black out the page.
    // The browser shell owns page visibility/focus in both standalone and embeds.
    hostManagedVisibility = true;
    audioActivated = !EM_ASM_INT({ return window.floppyStartMuted ? 1 : 0; });
    SetMasterVolume(audioActivated ? 1 : 0);
#endif

	// Initialize ResourceManager with mobile-aware quality detection
	ResourceManager::GetInstance().Initialize(ResourceQuality::AUTO);

	
	// Load the font with mobile-aware path resolution
	std::string fontPath = PlatformLayer::GetInstance().GetResourcePath("fonts/Whacky_Joe.ttf");
	whackyJoe = LoadPixelFont(fontPath.c_str(), 48);

	// Validate the font
	if (whackyJoe.baseSize <= 0 || whackyJoe.glyphCount <= 0 || whackyJoe.texture.id == 0) {
		printf("Error: Failed to load font '%s'\n", fontPath.c_str());
		printf("Falling back to default font.\n");
		whackyJoe = GetFontDefault(); // Fallback to default font
	}
	else {
		printf("Font 'whackyJoe' loaded successfully: baseSize=%d, glyphCount=%d, textureID=%u\n",
			whackyJoe.baseSize, whackyJoe.glyphCount, whackyJoe.texture.id);
		SetTextureFilter(whackyJoe.texture, TEXTURE_FILTER_POINT);
	}

	// Initialize AIGUI with mobile awareness
	AIGUI_Init();
	AIGUI_SetFont(whackyJoe);
	InitClasses();
	
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

#ifndef __EMSCRIPTEN__
	// Native builds can prepare all atlases; browser scenes load their own assets.
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
#endif

	// Initialize PerformanceProfiler with draw call tracking enabled
	PerformanceProfiler::GetInstance().SetTargetFPS(60);
	PerformanceProfiler::GetInstance().EnableOverlay(false);
	TraceLog(LOG_INFO, "PerformanceProfiler: Initialized with 60 FPS target and real draw call tracking enabled");

	gamestate = LOADING; // Start with loading state
	target = LoadRenderTexture(800, 450);
	SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
}

Game::~Game()
{
    // Destroy consumers while the graphics and audio devices still exist.
    if (playing) playing->GetStats().Save();
    delete mainMenu;
    delete playing;
    delete credits;
    delete loading;
    AudioManager::GetInstance().Shutdown();
    TextureAtlas::GetInstance().Shutdown();
    TextureCache::Clear();
    ResourceManager::GetInstance().Shutdown();
    if (whackyJoe.texture.id != GetFontDefault().texture.id) UnloadFont(whackyJoe);
    AIGUI_Shutdown();
    UnloadRenderTexture(target);
    CloseAudioDevice();
    delete window;
}

void Game::InitClasses()
{
	// Load and set the window icon with platform-aware path
	std::string iconPath = PlatformLayer::GetInstance().GetResourcePath("hats/poophat.png");
	Image icon = LoadImage(iconPath.c_str());
	if (icon.data) {
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
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* self) { static_cast<Game*>(self)->Tick(); }, this, 0, true);
#else
    while (!WindowShouldClose() && gamestate != SHUTDOWN) Tick();
#endif
}

void Game::SetHostPaused(bool paused)
{
    hostPaused = paused;
    if (paused && playing) playing->CancelTouchInput();
    if (paused) EmbedInput::Reset();
    SetMasterVolume(paused || orientationWaiting || !audioActivated ? 0.0f : 1.0f);
    if (paused && playing) playing->GetStats().Save();
}

void Game::SetAudioActivated(bool enabled) {
    audioActivated = enabled;
    SetMasterVolume(enabled && !hostPaused && !focusPaused && !orientationWaiting ? 1 : 0);
}

void Game::SetOrientationWaiting(bool waiting) {
    orientationWaiting=waiting;
    if (waiting) { EmbedInput::Reset(); if (playing) playing->CancelTouchInput(); }
    SetMasterVolume(waiting || hostPaused || !audioActivated ? 0 : 1);
}

void Game::SetInlinePresentation(bool enabled) {
    inlinePresentation = enabled;
    EmbedInput::Reset();
    if (playing) playing->CancelTouchInput();
}

void Game::SetSafeArea(float top, float right, float bottom, float left) {
    RenderLayout::safeTop = std::max(0.0f, top);
    RenderLayout::safeRight = std::max(0.0f, right);
    RenderLayout::safeBottom = std::max(0.0f, bottom);
    RenderLayout::safeLeft = std::max(0.0f, left);
}

void Game::RefreshLayout() {
    RenderLayout::Configure(GetScreenWidth(), GetScreenHeight(), touchPresentation && !inlinePresentation);
}

void Game::PointerInput(int phase, float x, float y) {
    RefreshLayout();
    const auto view = FloppyViewport::Fit(GetScreenWidth(), GetScreenHeight(), !touchPresentation,
        RenderLayout::surfaceWidth, RenderLayout::surfaceHeight);
    EmbedInput::Pointer(phase, {view.x + x * view.width, view.y + y * view.height});
}

void Game::TouchAction(int action, bool down)
{
    if (action == 3) { touchPresentation = down; return; }
    if (!hostPaused && gamestate == PLAYING && playing) playing->TouchAction(action, down);
}

void Game::Tick()
{
    RefreshLayout();
    const int targetWidth = (int)std::ceil(RenderLayout::surfaceWidth * RenderLayout::Density);
    const int targetHeight = (int)std::ceil(RenderLayout::surfaceHeight * RenderLayout::Density);
    if (target.texture.width != targetWidth || target.texture.height != targetHeight) {
        // Allocate before releasing the old image; visibility suspension can keep drawing it.
        const auto resized = LoadRenderTexture(targetWidth, targetHeight);
        if (resized.id) {
            BeginTextureMode(resized); ClearBackground(BLACK);
            DrawTexturePro(target.texture,{0,0,(float)target.texture.width,-(float)target.texture.height},
                {0,0,(float)targetWidth,(float)targetHeight},{0,0},0,WHITE);
            EndTextureMode();
            UnloadRenderTexture(target); target = resized; SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
        }
    }
    if (warmupFrames > 0) { --warmupFrames; BeginDrawing(); ClearBackground(BLACK); EndDrawing(); return; }
    // Keep polling events while suspended, but freeze all simulation timers.
    if (hostPaused || orientationWaiting || (!hostManagedVisibility && !IsWindowFocused())) {
        if (!focusPaused) {
            SetMasterVolume(0.0f);
            if (playing) playing->CancelTouchInput();
            EmbedInput::Reset();
            if (playing) playing->GetStats().Save();
        }
        focusPaused = true;
        BeginDrawing();
        ClearBackground(BLACK);
        const auto frozen = FloppyViewport::Fit(GetScreenWidth(), GetScreenHeight(), !touchPresentation,
            RenderLayout::surfaceWidth, RenderLayout::surfaceHeight);
        DrawTexturePro(target.texture, {0,0,(float)target.texture.width,-(float)target.texture.height}, {frozen.x,frozen.y,frozen.width,frozen.height}, {0,0}, 0, WHITE);
        EndDrawing();
        return;
    }
    if (focusPaused) {
        SetMasterVolume(audioActivated ? 1.0f : 0.0f);
        focusPaused = false;
        // Discard the first resumed frame so hidden-tab time cannot become a physics step.
        BeginDrawing(); EndDrawing();
        return;
    }
    if (gamestate == SHUTDOWN) {
#ifdef __EMSCRIPTEN__
        SetHostPaused(true);
        EM_ASM({ if (window.floppySend) window.floppySend('exit'); });
#endif
        return;
    }
    const auto view = FloppyViewport::Fit(GetScreenWidth(), GetScreenHeight(), !touchPresentation,
        RenderLayout::surfaceWidth, RenderLayout::surfaceHeight);
    const Vector2 rawMouse = EmbedInput::MousePosition();
    // Leave off-canvas input outside the UI instead of snapping it onto buttons.
    g_AIGUI.mousePos = RenderLayout::ToUi({(rawMouse.x - view.x) / view.scale, (rawMouse.y - view.y) / view.scale}, gamestate == PLAYING);
    HandleInput();
    PerformanceProfiler::GetInstance().BeginFrame();
    Update();
    PerformanceProfiler::GetInstance().EndFrame();
    BeginTextureMode(target);
    ClearBackground(BLACK);
    if (RenderLayout::mobile) DrawRectangleGradientV(0, 0, target.texture.width, target.texture.height, {20,35,30,255}, {6,12,10,255});
    if (gamestate == PLAYING) playing->Draw();
    else {
        AIGUI_SetRenderCamera(RenderLayout::UiCamera());
        BeginMode2D(RenderLayout::UiCamera());
        switch (gamestate) {
        case MAINMENU: if (mainMenu) mainMenu->Draw(); break;
        case CREDITS: credits->Draw(); break;
        case LOADING: loading->Draw(); break;
        default: break;
        }
        EndMode2D();
    }
    EndTextureMode();
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height},
        {view.x, view.y, view.width, view.height}, {0, 0}, 0, WHITE);
    EndDrawing();
    EmbedInput::EndFrame();
#ifdef __EMSCRIPTEN__
    const bool active = gamestate == PLAYING && !playing->IsLoadingLevel();
    const int webUiState = (active ? 1 : 0) | (active && playing->player->CanShoot() ? 2 : 0)
        | (active && playing->IsPaused() ? 4 : 0) | (active && playing->IsGameOver() ? 8 : 0)
        | (active ? playing->MobileOrientationMask() : 0);
    if (webUiState != lastWebUiState) {
        lastWebUiState = webUiState;
        EM_ASM({ if (window.floppySetUiState) window.floppySetUiState($0); }, webUiState);
    }
#endif
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
#ifndef __EMSCRIPTEN__
	if (EmbedInput::KeyPressed(KEY_F11)) window->ToggleMode();
#endif

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
    if (newState != PLAYING) { SetOrientationWaiting(false); GameSettings::SelectArena(false); }
}

// Implement GetScaledFont method
Font Game::GetScaledFont(float /*scaleFactor*/) {
    // Font metrics describe the atlas and must not change with display scale.
    // DrawTextEx's size argument owns the requested text size.
    return whackyJoe;
}
