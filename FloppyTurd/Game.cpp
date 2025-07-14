#include "Game.h"
#include "AIGUI.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "ResourceCompat.h"
#include "AudioStateManager.h"
#include "UIManager.h"
#include "LogManager.h"
#include <fstream>
#include <ctime>

// Enable draw call tracking
#define ENABLE_DRAW_CALL_TRACKING
#include "TextureAtlas.h"
#include "PerformanceProfiler.h"

Game::Game() : window(nullptr), gamestate(LOADING), credits(nullptr), loading(nullptr), initialized(false) {
    try {
        // Initialize LogManager first
        LogManager::GetInstance().Initialize();
        
        LogManager::GetInstance().Log("Game constructor STARTING", "INIT");
        
        // Constructor only sets initial state, doesn't start the game
        {
            std::lock_guard<std::mutex> lock(initializedMutex);
            initialized = false;
        }
        LogManager::GetInstance().Log("Set initialized = false in constructor", "DEBUG");
        
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
        LogManager::GetInstance().Log("About to create Loading state...", "DEBUG");
        try {
            loading = new Loading(this);
            if (loading) {
                LogManager::GetInstance().Log("Created Loading state in constructor", "DEBUG");
            } else {
                LogManager::GetInstance().Log("Failed to create Loading state - new returned nullptr", "ERROR");
                throw std::runtime_error("Failed to create Loading state - new returned nullptr");
            }
        } catch (const std::exception& e) {
            LogManager::GetInstance().Log("Exception creating Loading state: " + std::string(e.what()), "ERROR");
            throw;
        } catch (...) {
            LogManager::GetInstance().Log("Unknown exception creating Loading state", "ERROR");
            throw;
        }
        
        // Game instance will be registered globally by game_main() after construction
        
        LogManager::GetInstance().Log("Game constructor COMPLETED", "INIT");
        {
            std::lock_guard<std::mutex> lock(initializedMutex);
            LogManager::GetInstance().Log("initialized=" + std::string(initialized ? "true" : "false") + ", gamestate=LOADING", "INIT");
        }
        
    } catch (const std::exception& e) {
        LogManager::GetInstance().Log("Exception in Game constructor: " + std::string(e.what()), "ERROR");
        throw;
    } catch (...) {
        LogManager::GetInstance().Log("Unknown exception in Game constructor", "ERROR");
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
	SetOrientation(true);  // true = landscape
	
	TraceLog(LOG_INFO, "Mobile window initialized: %dx%d, Safe area: %.0fx%.0f",
		GetScreenWidth(), GetScreenHeight(),
		GetSafeArea().width, GetSafeArea().height);
#else
	// Desktop: Use native monitor resolution, start in fullscreen
	window = new Window(true, 0, 0);
#endif

	// Load and set the window icon with platform-aware path
	        std::string iconPath = GetResourcePath("poophat.ico");
	Image icon = LoadImage(iconPath.c_str());
	if (
icon.data
	) {
		// SetWindowIcon(icon); // Window icon setting not implemented yet
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
        
        {
            std::lock_guard<std::mutex> lock(initializedMutex);
            if (initialized) {
                GameLog::Log("[INIT] Game already initialized, returning early");
                return true;
            }
        }
        
        // Basic initialization
        gamestate = LOADING;
        {
            std::lock_guard<std::mutex> lock(initializedMutex);
            initialized = false;
        }
        GameLog::Log("[INIT] Set initialized = false in Initialize() method");
        
        // Initialize ResourceManager first
        GameLog::Log("[INIT] Step 1: Initializing ResourceManager...");
        try {
            ResourceManager::GetInstance().Initialize(ResourceQuality::HIGH);
            GameLog::Log("[INIT] Step 1: Initialized ResourceManager - SUCCESS");
            
            // AudioStateManager will be initialized later in Step 11
            GameLog::Log("[INIT] Step 1.5: AudioStateManager initialization deferred to Step 11");
            
            // Test that ResourceManager is working by trying to load a simple texture
            GameLog::Log("[INIT] Step 1.5: Testing ResourceManager...");
            
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
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
        // On iOS, the render target is the MTKView itself, not a separate texture
        GameLog::Log("[INIT] Step 2: Setting up iOS render target (MTKView)...");
        try {
            // Get the screen dimensions from the platform
            int screenWidth = GetScreenWidth();
            int screenHeight = GetScreenHeight();
            
            GameLog::Log("[INIT] Step 2: Screen dimensions: %dx%d", screenWidth, screenHeight);
            
            // For iOS, we don't create a separate render texture
            // The MetalRenderer will use the MTKView as the render target
            renderTarget = {0, {0, 0, 0, 0, 0, nullptr}, {0, 0, 0, 0, 0, nullptr}}; // Empty render target
            GameLog::Log("[INIT] Step 2: iOS render target set to MTKView - SUCCESS");
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 2: Exception setting up iOS render target: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 2: Unknown exception setting up iOS render target");
            throw;
        }
#else
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
#endif
        
        // Initialize window system for all platforms
        GameLog::Log("[INIT] Step 3: Initializing window system...");
        try {
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
            // On iOS, create a window that represents the full screen
            window = new Window(true, 0, 0); // Fullscreen window
            if (!window) {
                GameLog::Log("[ERROR] Step 3: Failed to create iOS window");
                throw std::runtime_error("Failed to create iOS window");
            }
            GameLog::Log("[INIT] Step 3: Created iOS fullscreen window - SUCCESS");
#else
            window = new Window();
            if (!window) {
                GameLog::Log("[ERROR] Step 3: Failed to create window");
                throw std::runtime_error("Failed to create window");
            }
            GameLog::Log("[INIT] Step 3: Created Window instance - SUCCESS");
#endif
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 3: Exception creating window: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 3: Unknown exception creating window");
            throw;
        }
        
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
            // Don't throw here, continue with initialization - MainMenu is not critical for basic functionality
            mainMenu = nullptr;
            GameLog::Log("[WARNING] Step 5: Continuing without MainMenu state");
        } catch (...) {
            GameLog::Log("[ERROR] Step 5: Unknown exception creating MainMenu");
            // Don't throw here, continue with initialization - MainMenu is not critical for basic functionality
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
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
            // On iOS, use full screen dimensions - no letterboxing needed
            gameScale = 1.0f;
            gameOffsetX = 0.0f;
            gameOffsetY = 0.0f;
            // Get screen dimensions from UIManager
            auto& uiManager = UIManager::GetInstance();
            renderedWidth = uiManager.GetScreenWidth();
            renderedHeight = uiManager.GetScreenHeight();
            GameLog::Log("[INIT] Step 8: Initialized iOS letterbox state - full screen %.0fx%.0f - SUCCESS", renderedWidth, renderedHeight);
#else
            // Desktop: use fixed game resolution with letterboxing
            gameScale = 1.0f;
            gameOffsetX = 0.0f;
            gameOffsetY = 0.0f;
            renderedWidth = 320.0f;
            renderedHeight = 180.0f;
            GameLog::Log("[INIT] Step 8: Initialized desktop letterbox state - SUCCESS");
#endif
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 8: Exception initializing letterbox: %s", e.what());
            throw;
        } catch (...) {
            GameLog::Log("[ERROR] Step 8: Unknown exception initializing letterbox");
            throw;
        }
        
        // Mark as initialized
        GameLog::Log("[INIT] Step 9: Marking game as initialized...");
        GameLog::Log("[INIT] Step 9: Before store - initialized value: %d", (int)initialized);
        try {
            {
                std::lock_guard<std::mutex> lock(initializedMutex);
                initialized = true;
            }
            GameLog::Log("[INIT] Step 9: After store - initialized value: %d", (int)initialized);
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
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #ifdef __OBJC__
            NSLog(@"[INIT_NSLOG] Step 10: Starting resource loading...");
        #endif
#endif
        printf("[INIT_PRINTF] Step 10: Starting resource loading...\n");
        try {
            if (loading) {
                GameLog::Log("[INIT] Step 10: Calling Initialize() on Loading state...");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                #ifdef __OBJC__
                    NSLog(@"[INIT_NSLOG] Step 10: Calling Initialize() on Loading state...");
                #endif
#endif
                printf("[INIT_PRINTF] Step 10: Calling Initialize() on Loading state...\n");
                loading->Initialize();
                GameLog::Log("[INIT] Step 10: Completed Initialize() on Loading state - SUCCESS");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                #ifdef __OBJC__
                    NSLog(@"[INIT_NSLOG] Step 10: Completed Initialize() on Loading state - SUCCESS");
                #endif
#endif
                printf("[INIT_PRINTF] Step 10: Completed Initialize() on Loading state - SUCCESS\n");
            } else {
                GameLog::Log("[ERROR] Step 10: Loading state is null, skipping initialization");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                #ifdef __OBJC__
                    NSLog(@"[INIT_NSLOG] ERROR: Loading state is null, skipping initialization");
                #endif
#endif
                printf("[INIT_PRINTF] ERROR: Loading state is null, skipping initialization\n");
                throw std::runtime_error("Loading state is null");
            }
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 10: Exception in Loading::Initialize(): %s", e.what());
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[INIT_NSLOG] ERROR: Exception in Loading::Initialize(): %s", e.what());
            #endif
#endif
            printf("[INIT_PRINTF] ERROR: Exception in Loading::Initialize(): %s\n", e.what());
            // Don't throw here, continue with initialization
            GameLog::Log("[WARNING] Step 10: Continuing despite Loading::Initialize() failure");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[INIT_NSLOG] WARNING: Continuing despite Loading::Initialize() failure");
            #endif
#endif
            printf("[INIT_PRINTF] WARNING: Continuing despite Loading::Initialize() failure\n");
        } catch (...) {
            GameLog::Log("[ERROR] Step 10: Unknown exception in Loading::Initialize()");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[INIT_NSLOG] ERROR: Unknown exception in Loading::Initialize()");
            #endif
#endif
            printf("[INIT_PRINTF] ERROR: Unknown exception in Loading::Initialize()\n");
            // Don't throw here, continue with initialization
            GameLog::Log("[WARNING] Step 10: Continuing despite Loading::Initialize() failure");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[INIT_NSLOG] WARNING: Continuing despite Loading::Initialize() failure");
            #endif
#endif
            printf("[INIT_PRINTF] WARNING: Continuing despite Loading::Initialize() failure\n");
        }
        
        GameLog::Log("[INIT] Step 11: Initializing AudioStateManager...");
        try {
            AudioStateManager::GetInstance().Initialize();
            GameLog::Log("[INIT] Step 11: AudioStateManager initialized - SUCCESS");
            
            // Set up initial audio state for main menu
            GameLog::Log("[INIT] Step 11a: Setting up initial audio state...");
            UpdateAudioState();
            GameLog::Log("[INIT] Step 11a: Initial audio state setup complete");
            
        } catch (const std::exception& e) {
            GameLog::Log("[ERROR] Step 11: Exception initializing AudioStateManager: %s", e.what());
            // Don't throw here, continue with initialization - Audio is not critical for basic functionality
            GameLog::Log("[WARNING] Step 11: Continuing without AudioStateManager");
        } catch (...) {
            GameLog::Log("[ERROR] Step 11: Unknown exception initializing AudioStateManager");
            // Don't throw here, continue with initialization - Audio is not critical for basic functionality
            GameLog::Log("[WARNING] Step 11: Continuing without AudioStateManager");
        }
        
        // Initialize TouchControls for mobile
#ifdef PLATFORM_MOBILE
    touchControls.Initialize(GetScreenWidth(), GetScreenHeight());
    AIGUI_SetTouchControls(&touchControls);
#endif
        
        GameLog::Log("[INIT] =========================================");
        GameLog::Log("[INIT] Game::Initialize() COMPLETED SUCCESSFULLY");
        {
            std::lock_guard<std::mutex> lock(initializedMutex);
            GameLog::Log("[INIT] initialized=%d", (int)initialized);
        }
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
    static float totalTime = 0.0f; // Track total running time
    static bool debugFileWritten = false; // Track if debug file has been written
    
    totalTime += deltaTime;
    
    if (updateCount++ % 60 == 0) {  // Log every 60 updates to avoid log spam
        std::cout << "[GAME] Update called (count: " << updateCount << ") - "
                  << "initialized: " << (initialized ? "true" : "false") 
                  << ", gamestate: " << gamestate 
                  << ", totalTime: " << totalTime << "s" << std::endl;
    }
    
    // Write debug file after 10 seconds of running
    if (totalTime >= 10.0f && !debugFileWritten) {
        WriteDebugFile();
        debugFileWritten = true;
    }
    
    if (!initialized) {
        if (updateCount == 1) {  // Only log this once to avoid spam
            std::cout << "[GAME] Update called but game not initialized" << std::endl;
        }
        return;
    }

    // Update AudioStateManager each frame
    AudioStateManager::GetInstance().Update(deltaTime);

    if (prevState != gamestate) {
        std::cout << "[GAME] Game state changed from " << prevState << " to " << gamestate << std::endl;
        prevState = gamestate;
        
        // Update audio state when game state changes
        UpdateAudioState();
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
    
    // Update audio state for the new game state
    UpdateAudioState();
    
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
    // Debug logging to see if UpdateFrame is being called
    static int frameCount = 0;
    if (++frameCount % 60 == 0) { // Log every 60 frames
        TraceLog(LOG_INFO, "[GAME] UpdateFrame called (count: %d), gamestate: %d, PLATFORM_MOBILE: %s", 
                 frameCount, gamestate, 
#ifdef PLATFORM_MOBILE
                 "DEFINED"
#else
                 "NOT DEFINED"
#endif
                 );
    }
    
#ifdef PLATFORM_MOBILE
    // --- MOBILE INPUT PATH ---
    TraceLog(LOG_INFO, "[GAME] Mobile input polling ENTRY");
    
    // Use touch input from PlatformLayer instead of GetMousePosition()
    Vector2 touchPos = GetTouchPosition(0); // Get primary touch position
    TraceLog(LOG_INFO, "[GAME] Mobile input: platform->GetTouchPosition(0) returned (%.1f,%.1f)", touchPos.x, touchPos.y);
    
    // Convert from pixels back to UI coordinate system (points)
    float scale = GetScreenScale();
    Vector2 uiPos = Vector2{touchPos.x / scale, touchPos.y / scale};
    TraceLog(LOG_INFO, "[GAME] Mobile input: converting pixels=(%.1f,%.1f) to UI points=(%.1f,%.1f), scale=%.1f", 
             touchPos.x, touchPos.y, uiPos.x, uiPos.y, scale);
    
    // Log touch position for debugging
    static int mobileLogCounter = 0;
    if (++mobileLogCounter % 60 == 0) { // Log every 60 frames
        TraceLog(LOG_INFO, "[GAME] Mobile input: touchPos=(%.1f,%.1f), uiPos=(%.1f,%.1f), mousePos=(%.1f,%.1f)", 
                 touchPos.x, touchPos.y, uiPos.x, uiPos.y, g_AIGUI.mousePos.x, g_AIGUI.mousePos.y);
    }
    
    // Update AIGUI mouse position with UI coordinates (points)
    Vector2 oldMousePos = g_AIGUI.mousePos;
    TraceLog(LOG_INFO, "[GAME] Mobile input: updating AIGUI mousePos from (%.1f,%.1f) to (%.1f,%.1f)", 
             oldMousePos.x, oldMousePos.y, uiPos.x, uiPos.y);
    g_AIGUI.mousePos = uiPos;
    
    // Log when mouse position changes (indicating touch input)
    if (oldMousePos.x != uiPos.x || oldMousePos.y != uiPos.y) {
        TraceLog(LOG_INFO, "[GAME] Touch input detected: oldPos=(%.1f,%.1f) -> newPos=(%.1f,%.1f)", 
                 oldMousePos.x, oldMousePos.y, uiPos.x, uiPos.y);
    }
    
    TraceLog(LOG_INFO, "[GAME] Mobile input polling EXIT");
    
    // Log TouchControls update
    static int touchControlsLogCounter = 0;
    if (++touchControlsLogCounter % 60 == 0) { // Log every 60 frames
        TraceLog(LOG_INFO, "[GAME] Calling TouchControls::Update() - frame %d", touchControlsLogCounter);
    }
    touchControls.Update();
#endif
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
	static int debugFrameCount = 0;
	if (debugFrameCount < 10) {  // Show more frames to catch any changes
		// Get actual monitor information
		printf("Frame %d: Effective=%.0fx%.0f, Scale=%.3f, Rendered=%.0fx%.0f, Offset=(%.1f,%.1f)\n", 
			debugFrameCount, effectiveWidth, effectiveHeight, gameScale, renderedWidth, renderedHeight, gameOffsetX, gameOffsetY);
		printf("  ScaleX=%.6f, ScaleY=%.6f, Diff=%.6f\n", scaleX, scaleY, fabs(scaleX - scaleY));
		printf("  Expected for 16:9: %.0fx%.0f\n", effectiveHeight * GAME_ASPECT, effectiveHeight);
		printf("  Unused space: X=%.1f, Y=%.1f\n", effectiveWidth - renderedWidth, effectiveHeight - renderedHeight);
		printf("  MONITOR: Real=%dx%d, Position=(%.0f,%.0f), Reported=%.0fx%.0f\n", 
			realMonitorWidth, realMonitorHeight, GetScreenCenter().x, GetScreenCenter().y, 
			screenWidth, screenHeight);
		printf("  DISCREPANCY: Width=%d, Height=%d, Fullscreen=%s, Using=%s\n", 
			realMonitorWidth - (int)screenWidth, realMonitorHeight - (int)screenHeight,
			IsWindowFullscreen() ? "YES" : "NO",
			(hasDiscrepancy && IsWindowFullscreen()) ? "REAL" : "REPORTED");
		debugFrameCount++;
	}
	
	// -------------------------------------------------------------------------
	// 2) Proper mouse coordinate mapping with letterbox offset
	// -------------------------------------------------------------------------
#if defined(PLATFORM_MOBILE)
    // --- MOBILE INPUT PATH (ALREADY HANDLED ABOVE) ---
    // Touch input is already processed in the earlier mobile input section
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
	
	// Update audio state manager
	AudioStateManager::GetInstance().Update(deltaTime);
	
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
                  << "initialized: " << (initialized ? "true" : "false")
                  << ", gamestate: " << gamestate << std::endl;
    }
    
    if (!initialized) {
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
    
    // Add SDF text rendering test
    if (frameCount <= 300) { // Show test for first 5 seconds (60fps * 5)
        Font testFont = GetFontDefault();
        DrawTextEx(testFont, "Hello, Floppy Turd!", {100, 100}, 32, 2, WHITE);
        DrawTextEx(testFont, "SDF Test with Chalkduster", {100, 140}, 24, 1, YELLOW);
    }
    
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
    
    // Add SDF text rendering test
    if (frameCount <= 300) { // Show test for first 5 seconds (60fps * 5)
        Font testFont = GetFontDefault();
        DrawTextEx(testFont, "Hello, Floppy Turd!", {100, 100}, 32, 2, WHITE);
        DrawTextEx(testFont, "SDF Test with Chalkduster", {100, 140}, 24, 1, YELLOW);
    }
    
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
	initialized = false;
	std::cout << "[DEBUG] Set initialized = false in Shutdown() method" << std::endl;
}

void Game::OnPause()
{
	// Pause music when app goes to background
	AudioStateManager::GetInstance().PauseMusic();
	
	// TODO: Add any other pause logic
}

void Game::OnResume()
{
	// Reactivate audio session and resume music when app returns to foreground
	GameLog::Log("[GAME] OnResume: Reactivating audio session and resuming music");
	
	// Reactivate the audio session first
	InitAudioDevice();
	
	// Resume music after audio session is active
	AudioStateManager::GetInstance().ResumeMusic();
	
	// TODO: Add any other resume logic
}

void Game::UpdateAudioState()
{
    // Update the AudioStateManager based on current game state
    auto audioState = AudioStateManager::GetInstance().GetAudioStateForGameState(gamestate);
    
    // Only transition if the audio state is different from current
    if (audioState != AudioStateManager::GetInstance().GetCurrentState()) {
        GameLog::Log("[GAME] Updating audio state to match game state %d -> audio state %d", 
                     (int)gamestate, (int)audioState);
        AudioStateManager::GetInstance().TransitionToState(audioState, true, 1.0f);
    }
}

void Game::SetLevelAudio(int levelNumber, AudioStateManager::Difficulty difficulty)
{
    GameLog::Log("[GAME] Setting level audio: Level %d, Difficulty %s", 
                 levelNumber, 
                 difficulty == AudioStateManager::DIFFICULTY_EASY ? "EASY" : 
                 difficulty == AudioStateManager::DIFFICULTY_NORMAL ? "NORMAL" : "HARD");
    AudioStateManager::GetInstance().TransitionToLevel(levelNumber, difficulty, true, 1.0f);
}

void Game::WriteDebugFile()
{
    try {
        LogManager::GetInstance().Log("=== FLOPPYTURD DEBUG REPORT ===", "DEBUG");
        
        // Game state information
        LogManager::GetInstance().Log("--- GAME STATE ---", "DEBUG");
        LogManager::GetInstance().Log("Initialized: " + std::string(initialized ? "true" : "false"), "DEBUG");
        LogManager::GetInstance().Log("Game State: " + std::to_string(static_cast<int>(gamestate)), "DEBUG");
        LogManager::GetInstance().Log("Window: " + std::string(window ? "valid" : "null"), "DEBUG");
        
        // Screen information
        LogManager::GetInstance().Log("--- SCREEN INFO ---", "DEBUG");
        LogManager::GetInstance().Log("Screen Width: " + std::to_string(GetScreenWidth()), "DEBUG");
        LogManager::GetInstance().Log("Screen Height: " + std::to_string(GetScreenHeight()), "DEBUG");
        LogManager::GetInstance().Log("Game Scale: " + std::to_string(gameScale), "DEBUG");
        LogManager::GetInstance().Log("Game Offset X: " + std::to_string(gameOffsetX), "DEBUG");
        LogManager::GetInstance().Log("Game Offset Y: " + std::to_string(gameOffsetY), "DEBUG");
        
        // Performance information
        LogManager::GetInstance().Log("--- PERFORMANCE ---", "DEBUG");
        LogManager::GetInstance().Log("FPS: " + std::to_string(GetCurrentFPS()), "DEBUG");
        LogManager::GetInstance().Log("Frame Time: " + std::to_string(GetCurrentFrameTime()) + "s", "DEBUG");
        
        // Resource information
        LogManager::GetInstance().Log("--- RESOURCES ---", "DEBUG");
        ResourceManager& rm = ResourceManager::GetInstance();
        LogManager::GetInstance().Log("Resource Manager Memory Usage: " + std::to_string(rm.GetMemoryUsage()) + " bytes", "DEBUG");
        
        // Font information
        LogManager::GetInstance().Log("--- FONT INFO ---", "DEBUG");
        LogManager::GetInstance().Log("Whacky Joe Font Base Size: " + std::to_string(whackyJoe.baseSize), "DEBUG");
        LogManager::GetInstance().Log("Whacky Joe Font Glyph Count: " + std::to_string(whackyJoe.glyphCount), "DEBUG");
        
        // UI information
        LogManager::GetInstance().Log("--- UI INFO ---", "DEBUG");
        UIManager& ui = UIManager::GetInstance();
        LogManager::GetInstance().Log("UI Manager Safe Area: " + std::to_string((int)ui.GetSafeArea().width) + "x" + std::to_string((int)ui.GetSafeArea().height), "DEBUG");
        
        // Current log file path
        LogManager::GetInstance().Log("--- LOG INFO ---", "DEBUG");
        LogManager::GetInstance().Log("Current Log File: " + LogManager::GetInstance().GetCurrentLogPath(), "DEBUG");
        
        LogManager::GetInstance().Log("=== END DEBUG REPORT ===", "DEBUG");
        
    } catch (const std::exception& e) {
        LogManager::GetInstance().Log("Error writing debug file: " + std::string(e.what()), "ERROR");
    } catch (...) {
        LogManager::GetInstance().Log("Unknown error writing debug file", "ERROR");
    }
}
