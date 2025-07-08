#include "Loading.h"
#include "ResourceManager.h"
#include "UIManager.h"
#include <thread>
#include <iostream>
#include "AudioStateManager.h"

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
            
            // Note: Music will be started when transitioning to main menu state
            // No music should play during loading
            UpdateLoadingProgress(0.6f);
            TraceLog(LOG_INFO, "LoadResources() - Music loading skipped (will be handled by state transition)");
            
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
            game->SetGameState(MAINMENU);
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
    UIManager& ui = UIManager::GetInstance();
    Rectangle safeAreaPx = ui.GetSafeArea(true);
    float screenWidthPx = safeAreaPx.x + safeAreaPx.width;
    float screenHeightPx = safeAreaPx.y + safeAreaPx.height;
    ClearBackground(BLACK);

    // --- Centered, large rotating poophat ---
    if (this->poophat.id != 0) {
        Vector2 center = ui.GetPosition(UIAnchor::CENTER, {0, 0}, true);
        float poophatSize = fminf(safeAreaPx.width, safeAreaPx.height) * 0.18f;
        poophatSize = fmaxf(poophatSize, 96.0f);
        Rectangle dest = {
            center.x - poophatSize / 2.0f,
            center.y - poophatSize / 2.0f,
            poophatSize,
            poophatSize
        };
        Rectangle source = { 0, 0, (float)this->poophat.width, (float)this->poophat.height };
        DrawTexturePro(this->poophat, source, dest, { poophatSize/2.0f, poophatSize/2.0f }, this->rotationAngle, WHITE);
    }

    // --- Progress bar at bottom center ---
    float barWidth = safeAreaPx.width * 0.6f;
    float barHeight = fmaxf(12.0f, safeAreaPx.height * 0.025f);
    float barX = safeAreaPx.x + (safeAreaPx.width - barWidth) / 2.0f;
    float barY = safeAreaPx.y + safeAreaPx.height * 0.88f;
    Rectangle barRect = { barX, barY, barWidth, barHeight };
    Rectangle barFillRect = { barX, barY, barWidth * loadingProgress, barHeight };
    DrawRectangleRounded(barRect, 0.4f, 12, ColorAlpha(WHITE, 0.18f));
    DrawRectangleRounded(barFillRect, 0.4f, 12, WHITE);

    // --- Loading text above bar ---
    const char* loadingText = "Loading...";
    int fontSize = (int)fmaxf(18.0f, safeAreaPx.height * 0.035f);
    int textWidth = MeasureText(loadingText, fontSize);
    float textY = barY - fontSize * 1.6f;
    DrawText(loadingText, (screenWidthPx - textWidth) / 2, textY, fontSize, WHITE);

    // --- Progress percent below bar ---
    char progressText[32];
    snprintf(progressText, sizeof(progressText), "%.0f%%", loadingProgress * 100.0f);
    int percentWidth = MeasureText(progressText, fontSize);
    float percentY = barY + barHeight + fontSize * 0.5f;
    DrawText(progressText, (screenWidthPx - percentWidth) / 2, percentY, fontSize, WHITE);
}