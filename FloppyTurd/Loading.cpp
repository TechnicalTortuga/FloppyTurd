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
        
        // Validate game pointer
        if (!game) {
            TraceLog(LOG_ERROR, "Loading::Initialize(): game pointer is null");
            return;
        }
        
        // Load the poophat texture now that ResourceManager should be initialized
        if (!poophatLoaded) {
            try {
                TraceLog(LOG_INFO, "Loading::Initialize() - Attempting to load poop_hat texture");
                poophat = ResourceManager::GetInstance().GetTexture("poop_hat");
                if (
#if defined(__APPLE__) && TARGET_OS_IPHONE
                    poophat.texture == nullptr
#else
                    poophat.id == 0
#endif
                ) {
                    TraceLog(LOG_ERROR, "Failed to load PoopHat texture");
                    // Initialize with empty texture to prevent crashes
                    poophat = {};
                } else {
                    poophatLoaded = true;
                    TraceLog(LOG_INFO, "Successfully loaded PoopHat texture");
                }
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Exception loading PoopHat texture: %s", e.what());
                poophat = {};
            } catch (...) {
                TraceLog(LOG_ERROR, "Unknown exception loading PoopHat texture");
                poophat = {};
            }
        }
        
        // Start loading resources in a separate thread
        try {
            TraceLog(LOG_INFO, "Loading::Initialize() - Creating loading thread");
            std::thread([this]() {
                try {
                    TraceLog(LOG_INFO, "Loading thread - Starting LoadResources()");
                    LoadResources();
                    TraceLog(LOG_INFO, "Loading thread - LoadResources() completed successfully");
                    resourcesLoaded = true;
                } catch (const std::exception& e) {
                    TraceLog(LOG_ERROR, "Exception in LoadResources thread: %s", e.what());
                    resourcesLoaded = true; // Mark as loaded to prevent hanging
                } catch (...) {
                    TraceLog(LOG_ERROR, "Unknown exception in LoadResources thread");
                    resourcesLoaded = true; // Mark as loaded to prevent hanging
                }
            }).detach();
            TraceLog(LOG_INFO, "Loading::Initialize() - Loading thread created successfully");
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Exception creating loading thread: %s", e.what());
            // Fall back to synchronous loading
            try {
                TraceLog(LOG_INFO, "Loading::Initialize() - Falling back to synchronous loading");
                LoadResources();
                resourcesLoaded = true;
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Exception in synchronous LoadResources: %s", e.what());
                resourcesLoaded = true; // Mark as loaded to prevent hanging
            } catch (...) {
                TraceLog(LOG_ERROR, "Unknown exception in synchronous LoadResources");
                resourcesLoaded = true; // Mark as loaded to prevent hanging
            }
        } catch (...) {
            TraceLog(LOG_ERROR, "Unknown exception creating loading thread");
            // Fall back to synchronous loading
            try {
                TraceLog(LOG_INFO, "Loading::Initialize() - Falling back to synchronous loading (unknown exception)");
                LoadResources();
                resourcesLoaded = true;
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Exception in synchronous LoadResources (unknown exception path): %s", e.what());
                resourcesLoaded = true; // Mark as loaded to prevent hanging
            } catch (...) {
                TraceLog(LOG_ERROR, "Unknown exception in synchronous LoadResources (unknown exception path)");
                resourcesLoaded = true; // Mark as loaded to prevent hanging
            }
        }
        
        TraceLog(LOG_INFO, "Loading::Initialize() COMPLETED");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Exception in Loading::Initialize(): %s", e.what());
        // Don't re-throw - just log the error and continue
        resourcesLoaded = true; // Mark as loaded to prevent hanging
    } catch (...) {
        TraceLog(LOG_ERROR, "Unknown exception in Loading::Initialize()");
        // Don't re-throw - just log the error and continue
        resourcesLoaded = true; // Mark as loaded to prevent hanging
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
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Exception in LoadResources: %s", e.what());
            UpdateLoadingProgress(1.0f); // Mark as complete even if there was an error
        } catch (...) {
            TraceLog(LOG_ERROR, "Unknown exception in LoadResources");
            UpdateLoadingProgress(1.0f); // Mark as complete even if there was an error
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