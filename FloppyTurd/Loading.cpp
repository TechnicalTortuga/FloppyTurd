#include "Loading.h"
#include "ResourceManager.h"
#include "UIManager.h"
#include <thread>
#include <iostream>

Loading::Loading(Game* game)
    : game(game)
    , rotationAngle(0.0f)
    , rotationTimer(0.0f)
    , loadingProgress(0.0f)
    , loadingStarted(false)
    , loadingComplete(false)
    , poophatLoaded(false)
{
    try {
        std::cout << "[DEBUG] Loading constructor STARTING" << std::endl;
        
        // Validate game pointer
        if (!game) {
            std::cerr << "[ERROR] Loading constructor: game pointer is null" << std::endl;
            throw std::invalid_argument("Game pointer is null");
        }
        
        // Don't load the poophat texture here - defer until Initialize() is called
        // This avoids race condition with ResourceManager initialization
        poophat = {};
        
        std::cout << "[DEBUG] Loading constructor COMPLETED successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in Loading constructor: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in Loading constructor" << std::endl;
        throw;
    }
}

void Loading::Initialize() {
    try {
        if (loadingStarted) return;
        loadingStarted = true;
        
        TraceLog(LOG_INFO, "Loading::Initialize() STARTING");
        
        // Add iOS-specific logging
        GameLog::Log("[LOADING] Loading::Initialize() STARTING");
        
        // Add direct NSLog and printf calls for testing
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #ifdef __OBJC__
            NSLog(@"[LOADING_NSLOG] Loading::Initialize() STARTING - direct NSLog call");
        #endif
#endif
        printf("[LOADING_PRINTF] Loading::Initialize() STARTING - direct printf call\n");
        
        // Validate game pointer
        if (!game) {
            TraceLog(LOG_ERROR, "Loading::Initialize(): game pointer is null");
            GameLog::Log("[LOADING] ERROR: game pointer is null");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[LOADING_NSLOG] ERROR: game pointer is null");
            #endif
#endif
            printf("[LOADING_PRINTF] ERROR: game pointer is null\n");
            return;
        }
        
        // Load the poophat texture now that ResourceManager should be initialized
        if (!poophatLoaded) {
            try {
                TraceLog(LOG_INFO, "Loading::Initialize() - Attempting to load poop_hat texture");
                GameLog::Log("[LOADING] Attempting to load poop_hat texture");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                #ifdef __OBJC__
                    NSLog(@"[LOADING_NSLOG] Attempting to load poop_hat texture");
                #endif
#endif
                printf("[LOADING_PRINTF] Attempting to load poop_hat texture\n");
                
                poophat = ResourceManager::GetInstance().GetTexture("poop_hat");
#if defined(__APPLE__) && TARGET_OS_IPHONE
                if (poophat.texture != nullptr) {
#else
                if (poophat.id != 0) {
#endif
                    TraceLog(LOG_INFO, "Loading::Initialize() - Successfully loaded poop_hat texture");
                    GameLog::Log("[LOADING] Successfully loaded poop_hat texture");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                    #ifdef __OBJC__
                        NSLog(@"[LOADING_NSLOG] Successfully loaded poop_hat texture");
                    #endif
#endif
                    printf("[LOADING_PRINTF] Successfully loaded poop_hat texture\n");
                    poophatLoaded = true;
                } else {
                    TraceLog(LOG_WARNING, "Loading::Initialize() - Failed to load poop_hat texture");
                    GameLog::Log("[LOADING] WARNING: Failed to load poop_hat texture");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                    #ifdef __OBJC__
                        NSLog(@"[LOADING_NSLOG] WARNING: Failed to load poop_hat texture");
                    #endif
#endif
                    printf("[LOADING_PRINTF] WARNING: Failed to load poop_hat texture\n");
                }
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Loading::Initialize() - Exception loading poop_hat texture: %s", e.what());
                GameLog::Log("[LOADING] ERROR: Exception loading poop_hat texture: %s", e.what());
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                #ifdef __OBJC__
                    NSLog(@"[LOADING_NSLOG] ERROR: Exception loading poop_hat texture: %s", e.what());
                #endif
#endif
                printf("[LOADING_PRINTF] ERROR: Exception loading poop_hat texture: %s\n", e.what());
            }
        }
        
        // Start background loading thread
        if (!loadingThread.joinable()) {
            TraceLog(LOG_INFO, "Loading::Initialize() - Starting background loading thread");
            GameLog::Log("[LOADING] Starting background loading thread");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[LOADING_NSLOG] Starting background loading thread");
            #endif
#endif
            printf("[LOADING_PRINTF] Starting background loading thread\n");
            
            loadingThread = std::thread(&Loading::LoadResources, this);
        }
        
        TraceLog(LOG_INFO, "Loading::Initialize() COMPLETED");
        GameLog::Log("[LOADING] Loading::Initialize() COMPLETED");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #ifdef __OBJC__
            NSLog(@"[LOADING_NSLOG] Loading::Initialize() COMPLETED");
        #endif
#endif
        printf("[LOADING_PRINTF] Loading::Initialize() COMPLETED\n");
        
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Loading::Initialize() - Exception: %s", e.what());
        GameLog::Log("[LOADING] ERROR: Exception in Initialize(): %s", e.what());
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #ifdef __OBJC__
            NSLog(@"[LOADING_NSLOG] ERROR: Exception in Initialize(): %s", e.what());
        #endif
#endif
        printf("[LOADING_PRINTF] ERROR: Exception in Initialize(): %s\n", e.what());
    }
}

void Loading::LoadResources() {
    try {
        TraceLog(LOG_INFO, "LoadResources() STARTING");
        
        // Validate game pointer
        if (!game) {
            TraceLog(LOG_ERROR, "LoadResources(): game pointer is null");
            return;
        }
        
        try {
            // Update progress as we load each resource
            UpdateLoadingProgress(0.1f);
            TraceLog(LOG_INFO, "LoadResources() - Progress updated to 10%%");
            
            // Preload MainMenu assets
            if (!game->mainMenu) {
                try {
                    TraceLog(LOG_INFO, "LoadResources() - Creating MainMenu instance");
                    game->mainMenu = new MainMenu(game);
                    UpdateLoadingProgress(0.3f);
                    TraceLog(LOG_INFO, "LoadResources() - MainMenu created successfully, progress updated to 30%%");
                } catch (const std::exception& e) {
                    TraceLog(LOG_ERROR, "Exception creating MainMenu: %s", e.what());
                    // Continue without MainMenu - it's not critical for basic functionality
                    game->mainMenu = nullptr;
                } catch (...) {
                    TraceLog(LOG_ERROR, "Unknown exception creating MainMenu");
                    // Continue without MainMenu - it's not critical for basic functionality
                    game->mainMenu = nullptr;
                }
            } else {
                TraceLog(LOG_INFO, "LoadResources() - MainMenu already exists");
            }
            
            // Load main menu music
            if (game->mainMenu && !game->mainMenu->GetAudioClip()) {
                try {
                    TraceLog(LOG_INFO, "LoadResources() - Loading main menu music");
                    game->mainMenu->PlayMusic(new AudioClip("mainmenu/FloppyTurdMenu.mp3"));
                    UpdateLoadingProgress(0.6f);
                    TraceLog(LOG_INFO, "LoadResources() - Main menu music loaded successfully, progress updated to 60%%");
                } catch (const std::exception& e) {
                    TraceLog(LOG_ERROR, "Exception loading main menu music: %s", e.what());
                    // Continue without music - it's not critical for basic functionality
                } catch (...) {
                    TraceLog(LOG_ERROR, "Unknown exception loading main menu music");
                    // Continue without music - it's not critical for basic functionality
                }
            } else {
                TraceLog(LOG_INFO, "LoadResources() - Main menu music already loaded or MainMenu not available");
            }
            
            // Load other critical resources here
            // ...
            
            UpdateLoadingProgress(1.0f);
            TraceLog(LOG_INFO, "LoadResources() - Progress updated to 100%%, loading complete");
            
            // Set the resourcesLoaded flag to true to complete the loading process
            resourcesLoaded = true;
            TraceLog(LOG_INFO, "LoadResources() - resourcesLoaded set to true");
            GameLog::Log("[LOADING] resourcesLoaded set to true - loading process complete");
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Exception in LoadResources: %s", e.what());
            UpdateLoadingProgress(1.0f); // Mark as complete even if there was an error
            resourcesLoaded = true; // Still mark as loaded even with errors
            GameLog::Log("[LOADING] Exception in LoadResources, but setting resourcesLoaded=true");
        } catch (...) {
            TraceLog(LOG_ERROR, "Unknown exception in LoadResources");
            UpdateLoadingProgress(1.0f); // Mark as complete even if there was an error
            resourcesLoaded = true; // Still mark as loaded even with errors
            GameLog::Log("[LOADING] Unknown exception in LoadResources, but setting resourcesLoaded=true");
        }
        
        TraceLog(LOG_INFO, "LoadResources() COMPLETED");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Exception in LoadResources outer try block: %s", e.what());
        UpdateLoadingProgress(1.0f); // Mark as complete even if there was an error
    } catch (...) {
        TraceLog(LOG_ERROR, "Unknown exception in LoadResources outer try block");
        UpdateLoadingProgress(1.0f); // Mark as complete even if there was an error
    }
}

void Loading::UpdateLoadingProgress(float progress) {
    // Ensure progress is between 0 and 1
    loadingProgress = std::min(1.0f, std::max(0.0f, progress));
    TraceLog(LOG_INFO, "Loading progress: %.0f%%", loadingProgress * 100.0f);
}

Loading::~Loading() {
    UnloadTexture(poophat);
}

void Loading::Update(float deltaTime) {
    if (!loadingStarted) {
        Initialize();
        return;
    }
    
    // Update rotation (one full rotation every 2 seconds)
    rotationTimer += deltaTime;
    rotationAngle = fmodf(rotationTimer * 180.0f, 360.0f);
    
    // Check if loading is complete
    if (!loadingComplete && resourcesLoaded && loadingProgress >= 1.0f) {
        loadingComplete = true;
        TraceLog(LOG_INFO, "Loading complete, transitioning to main menu");
        GameLog::Log("[LOADING] Loading complete, transitioning to main menu");
        
        // Set the game state to MAINMENU
        if (game) {
            game->SetGameState(Game::MAINMENU);
            GameLog::Log("[LOADING] Game state set to MAINMENU");
        }
    }
    
    // Log progress every few seconds to track loading
    static float lastProgressLog = 0.0f;
    lastProgressLog += deltaTime;
    if (lastProgressLog >= 2.0f) { // Log every 2 seconds
        lastProgressLog = 0.0f;
        GameLog::Log("[LOADING] Progress: %.0f%%, resourcesLoaded=%s, loadingComplete=%s", 
                    loadingProgress * 100.0f, 
                    resourcesLoaded ? "true" : "false", 
                    loadingComplete ? "true" : "false");
    }
}

void Loading::Draw() {
    // Get screen dimensions
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Draw the loading background
    ClearBackground(BLACK);
    
    // Only attempt to draw the poophat if it's loaded
    if (this->poophat.id != 0) {
#if defined(PLATFORM_MOBILE)
        // --- MOBILE RENDERING PATH ---
        // Use the UIManager to get the center position within the safe area.
        Vector2 centerPos = UIManager::GetInstance().GetPosition(UIAnchor::CENTER);
        
        // The UIManager gives us the center, so we draw the texture there.
        // The origin of DrawTexturePro is the top-left of the destination rectangle,
        // so we need to offset it by half the texture size to truly center it.
        float textureWidth = 32.0f;
        float textureHeight = 32.0f;
        Rectangle dest = { 
            centerPos.x - textureWidth / 2.0f, 
            centerPos.y - textureHeight / 2.0f, 
            textureWidth, 
            textureHeight 
        };
        Rectangle source = { 0, 0, (float)this->poophat.width, (float)this->poophat.height };
        DrawTexturePro(this->poophat, source, dest, { 0, 0 }, this->rotationAngle, WHITE);
#else
        // --- DESKTOP RENDERING PATH ---
        // Draw rotating poophat in the center of the 320x180 canvas
        float x = 320.0f / 2.0f;
        float y = 180.0f / 2.0f;
        Rectangle dest = { x, y, 32.0f, 32.0f };
        Rectangle source = { 0, 0, (float)this->poophat.width, (float)this->poophat.height };
        DrawTexturePro(this->poophat, source, dest, { 16.0f, 16.0f }, this->rotationAngle, WHITE);
#endif
    }
    
    // Draw progress bar
    float progressBarWidth = screenWidth * 0.6f;
    float progressBarHeight = screenHeight * 0.03f;
    float progressBarX = (screenWidth - progressBarWidth) / 2.0f;
    float progressBarY = screenHeight * 0.8f;
    
    // Background of progress bar
    DrawRectangle(
        progressBarX,
        progressBarY,
        progressBarWidth,
        progressBarHeight,
        ColorAlpha(WHITE, 0.2f)
    );
    
    // Progress fill
    DrawRectangle(
        progressBarX,
        progressBarY,
        progressBarWidth * loadingProgress,
        progressBarHeight,
        WHITE
    );
    
    // Loading text
    const char* loadingText = "Loading...";
    int fontSize = screenHeight * 0.04f;
    int textWidth = MeasureText(loadingText, fontSize);
    DrawText(
        loadingText,
        (screenWidth - textWidth) / 2,
        progressBarY - fontSize * 1.5f,
        fontSize,
        WHITE
    );
    
    // Progress percentage
    char progressText[32];
    snprintf(progressText, sizeof(progressText), "%.0f%%", loadingProgress * 100.0f);
    int percentWidth = MeasureText(progressText, fontSize);
    DrawText(
        progressText,
        (screenWidth - percentWidth) / 2,
        progressBarY + progressBarHeight + fontSize * 0.5f,
        fontSize,
        WHITE
    );
}