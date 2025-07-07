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
    try {
        std::cout << "[INIT] ========================================" << std::endl;
        std::cout << "[INIT] Game constructor STARTING" << std::endl;
        std::cout << "[INIT] ========================================" << std::endl;
        
        // Constructor only sets initial state, doesn't start the game
        initialized.store(false);
        std::cout << "[DEBUG] Set initialized = false in constructor" << std::endl;
        
        // Initialize pointers to nullptr
        window = nullptr;
        mainMenu = nullptr;
        playing = nullptr;
        credits = nullptr;
        loading = nullptr;
        
        // Set initial state to loading
        gamestate = LOADING;
        
        // Initialize letterbox state
        gameScale = 1.0f;
        gameOffsetX = 0.0f;
        gameOffsetY = 0.0f;
        renderedWidth = 320.0f;
        renderedHeight = 180.0f;
        
        // Initialize loading state first
        std::cout << "[DEBUG] About to create Loading state..." << std::endl;
        try {
            loading = new Loading(this);
            if (loading) {
                std::cout << "[DEBUG] Created Loading state in constructor" << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to create Loading state - new returned nullptr" << std::endl;
                throw std::runtime_error("Failed to create Loading state - new returned nullptr");
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception creating Loading state: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception creating Loading state" << std::endl;
            throw;
        }
        
        std::cout << "[INIT] Game constructor COMPLETED" << std::endl;
        std::cout << "[INIT] initialized=" << initialized.load() << ", gamestate=LOADING" << std::endl;
        std::cout << "[INIT] ========================================" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in Game constructor: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in Game constructor" << std::endl;
        throw;
    }
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
    try {
        GameLog::Log("[INIT] =========================================");
        GameLog::Log("[INIT] Game::Initialize() STARTING");
        GameLog::Log("[INIT] =========================================");
        
        if (initialized) {
            GameLog::Log("[INIT] Game already initialized, returning early");
            return true;
        }
        
        // Basic initialization
        gamestate = LOADING;
        initialized.store(false);
        GameLog::Log("[INIT] Set initialized = false in Initialize() method");
        
        // Initialize ResourceManager first
        GameLog::Log("[INIT] Step 1: Initializing ResourceManager...");
        try {
            ResourceManager::GetInstance().Initialize(ResourceQuality::HIGH);
            GameLog::Log("[INIT] Step 1: Initialized ResourceManager - SUCCESS");
            
            // Test that ResourceManager is working by trying to load a simple texture
            GameLog::Log("[INIT] Step 1.5: Testing ResourceManager with a simple texture...");
            try {
                Texture2D testTexture = ResourceManager::GetInstance().GetTexture("main_menu_bg");
                if (
#if defined(__APPLE__) && TARGET_OS_IPHONE
                    testTexture.texture != nullptr
#else
                    testTexture.id != 0
#endif
                ) {
                    GameLog::Log("[INIT] Step 1.5: ResourceManager test successful - texture loaded");
                } else {
                    GameLog::Log("[WARNING] Step 1.5: ResourceManager test failed - texture is null/empty");
                }
            } catch (const std::exception& e) {
                GameLog::Log("[ERROR] Step 1.5: Exception testing ResourceManager: %s", e.what());
                // Don't throw here, continue with initialization
            } catch (...) {
                GameLog::Log("[ERROR] Step 1.5: Unknown exception testing ResourceManager");
                // Don't throw here, continue with initialization
            }
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 1: Exception initializing ResourceManager: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 1: Unknown exception initializing ResourceManager");
            throw;
        }
        
        // Create render target for the game
        GameLog::Log("[INIT] Step 2: Creating render target...");
        try {
            renderTarget = LoadRenderTexture(320, 180);
            if (renderTarget.id == 0) {
                GameLog::Log("[ERROR] Step 2: Failed to create render target");
                throw std::runtime_error("Failed to create render target");
            }
            GameLog::Log("[INIT] Step 2: Created render target 320x180 - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 2: Exception creating render target: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 2: Unknown exception creating render target");
            throw;
        }
        
        // On iOS, we don't create a Window instance as it's managed by the system
        #ifndef PLATFORM_IOS
        GameLog::Log("[INIT] Step 3: Initializing window...");
        try {
            window = new Window();
            if (!window) {
                GameLog::Log("[ERROR] Step 3: Failed to create window");
                throw std::runtime_error("Failed to create window");
            }
            GameLog::Log("[INIT] Step 3: Created Window instance - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 3: Exception creating window: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 3: Unknown exception creating window");
            throw;
        }
        #else
        window = nullptr; // No window on iOS
        GameLog::Log("[INIT] Step 3: Skipping Window creation on iOS - SUCCESS");
        #endif
        
        // Initialize game states (loading state was created in constructor)
        GameLog::Log("[INIT] Step 4: Checking Loading state...");
        try {
            if (!loading) {
                GameLog::Log("[INIT] Step 4: Loading state not found, creating new instance...");
                loading = new Loading(this);
                if (!loading) {
                    GameLog::Log("[ERROR] Step 4: Failed to create Loading state");
                    throw std::runtime_error("Failed to create Loading state");
                }
                GameLog::Log("[INIT] Step 4: Created Loading state - SUCCESS");
            } else {
                GameLog::Log("[INIT] Step 4: Loading state already exists - SUCCESS");
            }
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 4: Exception with Loading state: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 4: Unknown exception with Loading state");
            throw;
        }
        
        // Create other game states (but don't initialize them yet)
        GameLog::Log("[INIT] Step 5: Creating MainMenu state...");
        try {
            mainMenu = new MainMenu(this);
            if (!mainMenu) {
                GameLog::Log("[ERROR] Step 5: Failed to create MainMenu state");
                throw std::runtime_error("Failed to create MainMenu state");
            }
            GameLog::Log("[INIT] Step 5: Created MainMenu state - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 5: Exception creating MainMenu: %s", e.what());
            // Don't throw here, continue with initialization - MainMenu is not critical
            mainMenu = nullptr;
            GameLog::Log("[WARNING] Step 5: Continuing without MainMenu state");
        } catch (...) {
            GameLog::Log("[ERROR] Step 5: Unknown exception creating MainMenu");
            // Don't throw here, continue with initialization - MainMenu is not critical
            mainMenu = nullptr;
            GameLog::Log("[WARNING] Step 5: Continuing without MainMenu state");
        }
        
        GameLog::Log("[INIT] Step 6: Creating Playing state...");
        try {
            playing = new Playing(this);
            if (!playing) {
                GameLog::Log("[ERROR] Step 6: Failed to create Playing state");
                throw std::runtime_error("Failed to create Playing state");
            }
            GameLog::Log("[INIT] Step 6: Created Playing state - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 6: Exception creating Playing: %s", e.what());
            // Don't throw here, continue with initialization - Playing is not critical for basic functionality
            playing = nullptr;
            GameLog::Log("[WARNING] Step 6: Continuing without Playing state");
        } catch (...) {
            GameLog::Log("[ERROR] Step 6: Unknown exception creating Playing");
            // Don't throw here, continue with initialization - Playing is not critical for basic functionality
            playing = nullptr;
            GameLog::Log("[WARNING] Step 6: Continuing without Playing state");
        }
        
        GameLog::Log("[INIT] Step 7: Creating Credits state...");
        try {
            credits = new Credits(this);
            if (!credits) {
                GameLog::Log("[ERROR] Step 7: Failed to create Credits state");
                throw std::runtime_error("Failed to create Credits state");
            }
            GameLog::Log("[INIT] Step 7: Created Credits state - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 7: Exception creating Credits: %s", e.what());
            // Don't throw here, continue with initialization - Credits is not critical
            credits = nullptr;
            GameLog::Log("[WARNING] Step 7: Continuing without Credits state");
        } catch (...) {
            GameLog::Log("[ERROR] Step 7: Unknown exception creating Credits");
            // Don't throw here, continue with initialization - Credits is not critical
            credits = nullptr;
            GameLog::Log("[WARNING] Step 7: Continuing without Credits state");
        }
        
        // Initialize letterbox state
        GameLog::Log("[INIT] Step 8: Initializing letterbox state...");
        try {
            gameScale = 1.0f;
            gameOffsetX = 0.0f;
            gameOffsetY = 0.0f;
            renderedWidth = 320.0f;
            renderedHeight = 180.0f;
            GameLog::Log("[INIT] Step 8: Initialized letterbox state - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 8: Exception initializing letterbox: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 8: Unknown exception initializing letterbox");
            throw;
        }
        
        // Mark as initialized
        GameLog::Log("[INIT] Step 9: Marking game as initialized...");
        try {
            initialized.store(true);
            GameLog::Log("[INIT] Step 9: Game marked as initialized - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 9: Exception marking as initialized: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 9: Unknown exception marking as initialized");
            throw;
        }
        
        // Start loading resources
        GameLog::Log("[INIT] Step 10: Starting resource loading...");
        try {
            if (loading) {
                GameLog::Log("[INIT] Step 10: Calling Initialize() on Loading state...");
                loading->Initialize();
                GameLog::Log("[INIT] Step 10: Completed Initialize() on Loading state - SUCCESS");
            } else {
                GameLog::Log("[ERROR] Step 10: Loading state is null, skipping initialization");
                throw std::runtime_error("Loading state is null");
            }
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 10: Exception in Loading::Initialize(): %s", e.what());
            // Don't throw here, continue with initialization
            GameLog::Log("[WARNING] Step 10: Continuing despite Loading::Initialize() failure");
        } catch (...) {
            GameLog::Log("[ERROR] Step 10: Unknown exception in Loading::Initialize()");
            // Don't throw here, continue with initialization
            GameLog::Log("[WARNING] Step 10: Continuing despite Loading::Initialize() failure");
        }
        
        GameLog::Log("[INIT] =========================================");
        GameLog::Log("[INIT] Game::Initialize() COMPLETED SUCCESSFULLY");
        GameLog::Log("[INIT] initialized=%d", (int)initialized.load());
        GameLog::Log("[INIT] =========================================");
        
        return true;
        
    } catch (const std::exception& e) {
        GameLog::Log("[ERROR] Exception in Game::Initialize(): %s", e.what());
        return false;
    } catch (...) {
        GameLog::Log("[ERROR] Unknown exception in Game::Initialize()");
        return false;
    }
}

void Game::RunGame()
{
    // Deprecated - use RunGameDesktop() for desktop platforms
    // or UpdateFrame()/RenderFrame() for frame-by-frame execution on iOS
    RunGameDesktop();
}

void Game::Update(float deltaTime)
{
    static int updateCount = 0;
    static GAMESTATE prevState = SHUTDOWN; // Track previous state for reset
    
    if (updateCount++ % 60 == 0) {  // Log every 60 updates to avoid log spam
        std::cout << "[GAME] Update called (count: " << updateCount << ") - "
                  << "initialized: " << (initialized.load() ? "true" : "false") 
                  << ", gamestate: " << gamestate << std::endl;
    }
    
    if (!initialized.load()) {
        if (updateCount == 1) {  // Only log this once to avoid spam
            std::cout << "[GAME] Update called but game not initialized" << std::endl;
        }
        return;
    }

    if (prevState != gamestate) {
        std::cout << "[GAME] Game state changed from " << prevState << " to " << gamestate << std::endl;
        prevState = gamestate;
    }

    switch (gamestate) {
        case MAINMENU:
            if (mainMenu) mainMenu->Update();
            break;

        case PLAYING:
            if (playing) playing->Update();
            break;

        case CREDITS:
            if (credits) {
                credits->Update(deltaTime);
                if (credits->IsComplete()) {
                    AudioManager::GetInstance().StopMusic();
                    if (credits->GetMusic()) credits->GetMusic()->Stop();
                    SetGameState(MAINMENU);
                }
            }
            break;

        case LOADING:
            if (loading) {
                loading->Update(deltaTime);
                if (loading->IsComplete()) {
                    SetGameState(MAINMENU);
                }
            } else {
                std::cerr << "[ERROR] Loading state is null in LOADING state!" << std::endl;
                SetGameState(MAINMENU);
            }
            break;

        case PAUSEMENU:
            // Handle pause menu updates if needed
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
		if (mainMenu) mainMenu->Draw();
		break;

	case PLAYING:
		if (playing) playing->Draw();
		break;

	case CREDITS:
		if (credits) credits->Draw();
		break;
	
	case PAUSEMENU:
		// Pause menu drawing handled by playing state
		break;
		
	case LOADING:
		if (loading) loading->Draw();
		break;
		
	case SHUTDOWN:
		// Nothing to draw during shutdown
		break;
	}

	EndDrawing();
}

void Game::HandleInput()
{
	if (IsKeyPressed(KEY_F11) && window) {
		window->ToggleMode();
	}

	switch (gamestate) {
	case MAINMENU:
		if (mainMenu) mainMenu->HandleInput();
		break;

	case PLAYING:
		if (playing) playing->HandleInput();
		break;

	case CREDITS:
		if (credits) credits->HandleInput();
		break;
		
	case LOADING:
		// No input handling during loading
		break;
		
	case PAUSEMENU:
		// Handle pause menu input
		break;
		
	case SHUTDOWN:
		// No input handling during shutdown
		break;
	}
}

void Game::SetGameState(GAMESTATE newState)
{
    if (gamestate == newState) {
        return; // No state change needed
    }
    
    std::cout << "[GAME] State transition: " << gamestate << " -> " << newState << std::endl;
    
    // Handle exit from current state
    switch (gamestate) {
        case LOADING:
            std::cout << "[GAME] Exiting LOADING state" << std::endl;
            // No special cleanup needed for loading state
            break;
            
        case MAINMENU:
            std::cout << "[GAME] Exiting MAINMENU state" << std::endl;
            // No cleanup needed for main menu
            break;
            
        case PLAYING:
            std::cout << "[GAME] Exiting PLAYING state" << std::endl;
            // No cleanup needed for playing state
            break;
            
        case CREDITS:
            std::cout << "[GAME] Exiting CREDITS state" << std::endl;
            // No cleanup needed for credits
            break;
            
        case PAUSEMENU:
            std::cout << "[GAME] Exiting PAUSEMENU state" << std::endl;
            // No cleanup needed for pause menu
            break;
            
        case SHUTDOWN:
            // No cleanup needed for shutdown
            break;
    }
    
    // Update state
    GAMESTATE oldState = gamestate;
    gamestate = newState;
    
    // Handle entry to new state
    switch (newState) {
        case LOADING:
            std::cout << "[GAME] Entering LOADING state" << std::endl;
            if (loading) {
                loading->Initialize();
            }
            break;
            
        case MAINMENU:
            std::cout << "[GAME] Entering MAINMENU state" << std::endl;
            // Main menu initialization handled in constructor
            break;
            
        case PLAYING:
            std::cout << "[GAME] Entering PLAYING state" << std::endl;
            // Playing state initialization handled in constructor
            break;
            
        case CREDITS:
            std::cout << "[GAME] Entering CREDITS state" << std::endl;
            // Credits initialization handled in constructor
            break;
            
        case PAUSEMENU:
            std::cout << "[GAME] Entering PAUSEMENU state" << std::endl;
            // Handle pause menu entry if needed
            break;
            
        case SHUTDOWN:
            std::cout << "[GAME] SHUTDOWN requested" << std::endl;
            // Shutdown will be handled by the main loop
            break;
    }
}

// Implement GetScaledFont method
Font Game::GetScaledFont(float scaleFactor)
{
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

	// If we're in loading state, ensure the loading object is initialized
	if (gamestate == LOADING) {
		if (loading) {
			loading->Update(deltaTime);
			
			// Check if loading is complete and transition to main menu
			if (loading->IsComplete()) {
				SetGameState(MAINMENU);
			}
		} else {
			// If loading is null, log error and transition to main menu
			std::cerr << "[ERROR] Loading state is null in LOADING state!" << std::endl;
			SetGameState(MAINMENU);
		}
		return;
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
#if defined(PLATFORM_MOBILE)
    // --- MOBILE INPUT PATH ---
    // On mobile, the coordinates from GetMousePosition() (which is mapped to touch)
    // are already in the screen's coordinate space. No scaling is needed.
    g_AIGUI.mousePos = GetMousePosition();
#else
    // --- DESKTOP INPUT PATH (UNCHANGED) ---
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
#endif

	// -------------------------------------------------------------------------
	// 3) Update game logic
	// -------------------------------------------------------------------------
	PerformanceProfiler::GetInstance().BeginFrame();
	
	// Only update if not in loading state (loading is handled separately above)
	if (gamestate != LOADING) {
		Update();
	}
	
	PerformanceProfiler::GetInstance().EndFrame();
	
	// Handle input after update
	if (gamestate != LOADING) {
		HandleInput();
	}
}

void Game::RenderFrame()
{
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {  // Log every 60 frames to avoid log spam
        std::cout << "[GAME] RenderFrame called (count: " << frameCount << ") - "
                  << "initialized: " << (initialized.load() ? "true" : "false")
                  << ", gamestate: " << gamestate << std::endl;
    }
    
    if (!initialized.load()) {
        if (frameCount == 1) {  // Only log this once to avoid spam
            std::cout << "[GAME] RenderFrame called but game not initialized" << std::endl;
        }
        return;
    }
    
    // Special handling for loading state
    if (gamestate == LOADING && loading) {
        loading->Draw();
        return;
    }

#if defined(PLATFORM_MOBILE)
    // --- MOBILE RENDERING PATH ---
    ClearBackground(BLACK); // Clear the main framebuffer

    // The projection matrix will be updated in a subsequent step to match the screen.
    // For now, drawing is direct.
    BeginDrawing();
    
    switch (gamestate)
    {
        case MAINMENU:
            if (mainMenu) mainMenu->Draw();
            break;
        case PLAYING:
            if (playing) playing->Draw();
            break;
        case CREDITS:
            if (credits) credits->Draw();
            break;
        case PAUSEMENU:
            // The `playing` state is expected to render the pause menu over itself.
            if (playing) playing->Draw();
            break;
        case LOADING:
            if (loading) loading->Draw();
            break;
        case SHUTDOWN:
            // Nothing to draw.
            break;
    }

    EndDrawing();
#else
    // --- DESKTOP RENDERING PATH ---
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
            if (mainMenu) mainMenu->Draw();  // all drawing in 320x180 coords
            break;
        case PLAYING:
            if (playing) playing->Draw();   // includes your pause logic if needed
            break;
        case CREDITS:
            if (credits) credits->Draw();   // Draw credits screen
            break;
        case PAUSEMENU:
            // Pause menu rendering is typically handled by the playing state
            if (playing) {
                playing->Draw(); // Let playing state handle pause menu rendering
            }
            break;
        case LOADING:
            if (loading) loading->Draw(); // Draw loading screen with rotating poophat
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
        static_cast<float>(renderedWidth),
        static_cast<float>(renderedHeight)
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
#endif
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
